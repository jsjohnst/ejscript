/*
 *  http.c -- Http client program
 *
 *  The http program is a client to issue HTTP requests. It is also a test platform for loading and testing web servers. 
 *
 *  Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */
 
/******************************** Includes ***********************************/

#include    "mpr.h"

#if BLD_FEATURE_HTTP_CLIENT
/*********************************** Locals ***********************************/

static int      activeLoadThreads;  /* Still running test threads */
static int      benchmark;          /* Output benchmarks */
static char     *chunkSize;         /* Request response data to be chunked in this quanta */
static int      continueOnErrors;   /* Continue testing even if an error occurs. Default is to stop */
static char     *cmpDir;            /* Dir of web content to compare results */
static int      success;            /* Total success flag */
static int      fetchCount;         /* Total count of fetches */
static char     *fileList;          /* File of URLs */
static Mpr      *mpr;               /* Portable runtime */
static MprList  *headers;           /* Request headers */
static int      iterations;         /* URLs to fetch */
static int      isBinary;           /* Looks like a binary output file */
static int      httpVersion;        /* HTTP/1.x */
static char     *host;              /* Host to connect to */
static int      loadThreads;        /* Number of threads to use for URL requests */
static char     *method;            /* HTTP method when URL on cmd line */
static int      nextArg;            /* Next arg to parse */
static int      noout;              /* Don't output files */
static int      showHeaders;        /* Write headers as well as content */
static int      printable;          /* Make binary output printable */
static char     *password;          /* Password for authentication */
static int      poolThreads;        /* Pool threads. >0 if multi-threaded */
static MprList  *formData;          /* Form body data. Overrides body data */
static char     *bodyData;          /* Body data, usually (but not always) for POST requests */
static char     *bodyFile;          /* Filename containing Body data */
static int      bodyLen;            /* Length of body data */
static char     *ranges;            /* Request ranges */
static int      retries;            /* Times to retry a failed request */
static int      saveArgc;           /* Count of args to process */
static char     **saveArgv;         /* Saved command line args */
static char     *saveDir;           /* Save output to this directory */
static MprFile  *saveFile;          /* Saved output */
static char     saveFileName[MPR_MAX_FNAME];
static int      singleStep;         /* Pause between requests */
static int      timeout;            /* Timeout in secs for a non-responsive server */
static char     *username;          /* User name for authentication of requests */
static int      verbose;            /* Trace progress */

#if BLD_FEATURE_MULTITHREAD
static MprMutex *mutex;
#endif

/***************************** Forward Declarations ***************************/

static int      compare(MprHttp *http, cchar *url);
static void     doTests(void *data, MprThread *tp);
static int      fetch(MprHttp *client, char *method, char *url);
static char     *getBody(MprCtx ctx, cchar *path, int *len);
static char     *getPassword(MprCtx ctx);
static void     initSettings(Mpr *mpr);
static bool     parseArgs(Mpr *mpr, int argc, char **argv);
static int      save(MprHttp *http, cchar *url);
static void     showOutput(MprHttp *http, cchar *content, int contentLen);
static int      startLogging(Mpr *mpr, char *logSpec);
static void     showUsage(Mpr *mpr);
static void     trace(MprHttp *http, cchar *url, int fetchCount, cchar *method, int code, int contentLen);
static bool     validateArgs(Mpr *mpr, bool ok);

#if BLD_FEATURE_MULTITHREAD
static void     lock();
static void     unlock();
#else
static inline void lock() {};
static inline void unlock() {};
#endif

/*********************************** Code *************************************/

int main(int argc, char *argv[])
{
    MprTime         start;
    double          elapsed;
    int             ok;
#if BLD_FEATURE_MULTITHREAD
    MprThread       *threadp;
#endif

    mpr = mprCreate(argc, argv, NULL);
    mprSetAppName(mpr, mprGetBaseName(argv[0]), 0, 0);

    initSettings(mpr);
    ok = parseArgs(mpr, argc, argv);

    if (!validateArgs(mpr, ok)) {
        showUsage(mpr);
        return MPR_ERR_BAD_ARGS;
    }

    saveArgc = argc - nextArg;
    saveArgv = &argv[nextArg];

    if (bodyFile && (bodyData = getBody(mpr, bodyFile, &bodyLen)) == 0) {
		exit(2);
    }
    if (chunkSize) {
        mprAddItem(headers, mprCreateKeyPair(headers, "X-Appweb-Chunk-Size", chunkSize));
    }

#if BLD_FEATURE_MULTITHREAD
    mprSetMaxPoolThreads(mpr, poolThreads);
#endif

#if BLD_FEATURE_SSL
    if (!mprLoadSsl(mpr, 1)) {
        exit(1);
    }
#endif

    /*
     *  Start the Timer, Socket and Pool services
     */
    if (mprStart(mpr, MPR_SERVICE_THREAD) < 0) {
        mprError(mpr, "Can't start MPR for %s", mprGetAppTitle(mpr));
        exit(2);
    }

    /*
     *  Create extra test threads to run the tests as required. We use the main thread also (so start with j==1)
     */
    start = mprGetTime(mpr);
#if BLD_FEATURE_MULTITHREAD
    {
        int     j;

        activeLoadThreads = loadThreads;
        for (j = 1; j < loadThreads; j++) {
            char name[64];
            mprSprintf(name, sizeof(name), "t.%d", j - 1);
            threadp = mprCreateThread(mpr, name, doTests, mpr, MPR_NORMAL_PRIORITY, 0); 
            mprStartThread(threadp);
        }
    }
#endif

    if (saveArgc > 0 || fileList) {
        doTests(mpr, 0);
    } else {
        showUsage(mpr);
    }

    /*
     *  Wait for all the threads to complete (simple but effective). Keep servicing events as we wind down.
     */
    while (activeLoadThreads > 1) {
        mprSleep(mpr, 100);
    }

    if (benchmark) {
        elapsed = (double) (mprGetTime(mpr) - start);
        if (fetchCount == 0) {
            elapsed = 0;
            fetchCount = 1;
        }
        mprPrintf(mpr, "\tThreads %d, Pool Threads %d   \t%13.2f\t%12.2f\t%6d\n", 
            loadThreads, poolThreads, elapsed * 1000.0 / fetchCount, elapsed / 1000.0, fetchCount);

        mprPrintf(mpr, "\nTime elapsed:        %13.4f sec\n", elapsed / 1000.0);
        mprPrintf(mpr, "Time per request:    %13.4f sec\n", elapsed / 1000.0 / fetchCount);
        mprPrintf(mpr, "Requests per second: %13.4f\n", fetchCount * 1.0 / (elapsed / 1000.0));
    }
    if (!success && verbose) {
        mprError(mpr, "Request failed");
    }

    return (success) ? 0 : 255;
}


static void initSettings(Mpr *mpr)
{
    method = 0;
    fileList = cmpDir = saveDir = 0;
    verbose = continueOnErrors = showHeaders = 0;

    /* 
     *  Need at least one thread to run efficiently 
     */  
    poolThreads = 4;            

    /*
     *  Default to HTTP/1.1
     */
    httpVersion = 1;
    success = 1;
    host = "localhost";
    bodyData = 0;
    bodyFile = 0;
    bodyLen = 0;
    retries = MPR_HTTP_RETRIES;
    iterations = 1;
    loadThreads = 1;
    timeout = (60 * 1000);
    
    headers = mprCreateList(mpr);

#if BLD_FEATURE_MULTITHREAD
    mutex = mprCreateLock(mpr);
#endif
}


static bool parseArgs(Mpr *mpr, int argc, char **argv)
{
    char    *argp, *key, *value;
    int     i;

    for (nextArg = 1; nextArg < argc; nextArg++) {
        argp = argv[nextArg];
        if (*argp != '-') {
            break;
        }

        if (strcmp(argp, "--benchmark") == 0) {
            benchmark++;

        } else if (strcmp(argp, "--compare") == 0) {
            if (nextArg >= argc) {
                return 0;
            } else {
                cmpDir = argv[++nextArg];
            }

        } else if (strcmp(argp, "--chunk") == 0) {
            if (nextArg >= argc) {
                return 0;
            } else {
                chunkSize = argv[++nextArg];
                i = atoi(chunkSize);
                if (i < 0) {
                    mprError(mpr, "Bad chunksize %d", i);
                    return 0;
                }
            }

        } else if (strcmp(argp, "--continue") == 0) {
            continueOnErrors++;

        } else if (strcmp(argp, "--cookie") == 0) {
            if (nextArg >= argc) {
                return 0;
            } else {
                mprAddItem(headers, mprCreateKeyPair(headers, "Cookie", argv[++nextArg]));
            }

        } else if (strcmp(argp, "--data") == 0) {
            if (nextArg >= argc) {
                return 0;
            } else {
                /* TODO - would be nice to support multiple --data args */
                bodyData = argv[++nextArg];
                bodyLen = strlen(bodyData);
            }

        } else if (strcmp(argp, "--datafile") == 0) {
            if (nextArg >= argc) {
                return 0;
            } else {
                bodyFile = argv[++nextArg];
            }

        } else if (strcmp(argp, "--debug") == 0 || strcmp(argp, "-d") == 0) {
            mprSetDebugMode(mpr, 1);

        } else if (strcmp(argp, "--files") == 0) {
            if (nextArg >= argc) {
                return 0;
            } else {
                fileList = argv[++nextArg];
            }

        } else if (strcmp(argp, "--form") == 0 || strcmp(argp, "-f") == 0) {
            if (nextArg >= argc) {
                return 0;
            } else {
                if (formData == 0) {
                    formData = mprCreateList(mpr);
                }
                mprAddItem(formData, argv[++nextArg]);
            }

        } else if (strcmp(argp, "--header") == 0) {
            if (nextArg >= argc) {
                return 0;
            } else {
                key = argv[++nextArg];
                if ((value = strchr(key, ':')) == 0) {
                    mprError(mpr, "Bad header format. Must be \"key: value\"");
                    return 0;
                }
                *value++ = '\0';
                while (isspace((int) *++value));
                mprAddItem(headers, mprCreateKeyPair(headers, key, value));
            }

        } else if (strcmp(argp, "--host") == 0) {
            if (nextArg >= argc) {
                return 0;
            } else {
                host = argv[++nextArg];
            }

        } else if (strcmp(argp, "--http") == 0) {
            if (nextArg >= argc) {
                return 0;
            } else {
                httpVersion = atoi(argv[++nextArg]);
            }

        } else if (strcmp(argp, "--iterations") == 0 || strcmp(argp, "-i") == 0) {
            if (nextArg >= argc) {
                return 0;
            } else {
                iterations = atoi(argv[++nextArg]);
            }

        } else if (strcmp(argp, "--log") == 0 || strcmp(argp, "-l") == 0) {
            if (nextArg >= argc) {
                return 0;
            } else {
                startLogging(mpr, argv[++nextArg]);
            }

        } else if (strcmp(argp, "--method") == 0) {
            if (nextArg >= argc) {
                return 0;
            } else {
                method = argv[++nextArg];
            }

        } else if (strcmp(argp, "--noout") == 0 || strcmp(argp, "-n") == 0) {
            noout++;

        } else if (strcmp(argp, "--password") == 0 || strcmp(argp, "-p") == 0) {
            if (nextArg >= argc) {
                return 0;
            } else {
                password = argv[++nextArg];
            }
        } else if (strcmp(argp, "--printable") == 0) {
            printable++;

        } else if (strcmp(argp, "--poolTheads") == 0) {
            if (nextArg >= argc) {
                return 0;
            } else {
                poolThreads = atoi(argv[++nextArg]);
            }

        } else if (strcmp(argp, "--range") == 0) {
            if (nextArg >= argc) {
                return 0;
            } else {
                if (ranges == 0) {
                    mprAllocSprintf(mpr, &ranges, -1, "bytes=%s", argv[++nextArg]);
                } else {
                    mprAllocStrcat(mpr, &ranges, -1, 0, ranges, ",", argv[++nextArg], 0);
                }
            }
            
        } else if (strcmp(argp, "--retries") == 0) {
            if (nextArg >= argc) {
                return 0;
            } else {
                retries = atoi(argv[++nextArg]);
            }
            
        } else if (strcmp(argp, "--save") == 0) {
            if (nextArg >= argc) {
                return 0;
            } else {
                saveDir = argv[++nextArg];
            }

        } else if (strcmp(argp, "--showHeaders") == 0) {
            showHeaders++;

        } else if (strcmp(argp, "--single") == 0 || strcmp(argp, "-s") == 0) {
            singleStep++;

        } else if (strcmp(argp, "--threads") == 0) {
            if (nextArg >= argc) {
                return 0;
            } else {
                loadThreads = atoi(argv[++nextArg]);
            }

        } else if (strcmp(argp, "--timeout") == 0) {
            if (nextArg >= argc) {
                return 0;
            } else {
                timeout = atoi(argv[++nextArg]) * 1000;
            }

        } else if (strcmp(argp, "--user") == 0 || strcmp(argp, "-u") == 0) {
            if (nextArg >= argc) {
                return 0;
            } else {
                username = argv[++nextArg];
            }

        } else if (strcmp(argp, "--verbose") == 0 || strcmp(argp, "-v") == 0) {
            verbose++;

        } else {
            return 0;
            break;
        }
    }

    if ((bodyData != 0) + (formData != 0) + (bodyFile != 0) > 1) {
        mprError(mpr, "Can only use one of --data, --datafile or --form");
        return 0;
    }

    return 1;
}


static bool validateArgs(Mpr *mpr, bool ok)
{
    if (saveDir && (loadThreads > 1)) {
        ok = 0;
    }
    if (saveDir) {
        noout = 1;
    }
    if (method == 0) {
        if (bodyData || formData || bodyFile) {
            method = "POST";
        } else {
            method = "GET";
        }
    }
    if (bodyData && bodyFile) {
        ok = 0;
    }
    return ok;
}


static void showUsage(Mpr *mpr)
{
    mprErrorPrintf(mpr,
        "usage: %s [options] urls...\n"
        "  Http client options:\n"
        "  --benchmark           # Compute benchmark results\n"
        "  --chunk size          # Request response data to use this chunk size\n"
        "  --compare dir         # Directory containing files to compare with\n"
        "  --continue            # Continue on errors\n"
        "  --cookie CookieString # Define a cookie header. Multiple uses okay.\n"
        "  --data string         # Raw body data\n"
        "  --datafile filename   # Fail containing raw body data\n"
        "  --debug               # Run in debug mode. No timeouts\n"
        "  --files fileList      # File list containing a list of files to fetch\n"
        "  --form string         # Form data. Should be form-www-urlencoded\n"
        "  --header 'key: value' # Add a custom request header\n"
        "  --host hostName       # Host name or IP address for unqualified URLs\n"
        "  --http 0|1            # HTTP version. 0 for HTTP 1.0, 1 for HTTP 1.1\n"
        "  --iterations count    # Number of times to fetch the urls (default 1)\n"
        "  --log logFile:level   # Log to the file at the verbosity level\n"
        "  --method KIND         # HTTP request method GET|OPTIONS|POST|PUT|TRACE (default GET)\n"
        "  --noout               # Don't output files to stdout\n"
        "  --password pass       # Password for authentication\n"
        "  --poolThreads count   # Set maximum pool threads\n"
        "  --printable           # Make binary output printable\n"
        "  --range byteRanges    # Request a subset range of the document \n"
        "  --retries count       # Number of times to retry failing requests\n"
        "  --save dir            # Directory to save downloaded files\n"
        "  --showHeaders         # Output response headers\n"
        "  --single              # Single step. Pause for input between requests\n"
        "  --timeout secs        # Request timeout period in seconds\n"
        "  --threads count       # Number of thread instances to spawn\n"
        "  --user name           # User name for authentication\n"
        "  --verbose             # Verbose operation. Trace progress\n",
        mprGetAppName(mpr));
}


/*
 *  Do the real work here
 */ 
static void doTests(void *data, MprThread *threadp)
{
    MprBuf      *body;
    MprFile     *file;
    MprCtx      ctx;
    Mpr         *mpr;
    MprHttp     *http;
    char        urlBuf[4096], urlBuf2[4096];
    char        bodyBuf[4096];
    char        *cp, *operation, *url, *tok;
    int         rc, j, line;

    file = 0;
    mpr = (Mpr*) data;

    ctx = (threadp) ? (MprCtx) threadp : (MprCtx) mpr;
    http = mprCreateHttp(ctx);

    mprSetHttpTimeout(http, timeout);
    mprSetHttpRetries(http, retries);

    if (httpVersion == 0) {
        mprSetHttpKeepAlive(http, 0);
        mprSetHttpProtocol(http, "HTTP/1.0");
    }
    if (username) {
        if (password == 0) {
            password = getPassword(http);
        }
        mprSetHttpCredentials(http, username, password);
    }

    body = mprCreateBuf(http, MPR_HTTP_BUFSIZE, -1);

    while (!mprIsExiting(http) && success) {
        lock();
        if (fetchCount >= iterations) {
            unlock();
            break;
        }
        unlock();
        rc = 0;

        for (j = 0; j < saveArgc; j++) {
            url = saveArgv[j];
            if (*url == '/') {
                mprSprintf(urlBuf2, sizeof(urlBuf2), "%s%s", host, url);
                url = urlBuf2;
            }
            rc = fetch(http, method, url);
            if (rc < 0 && !continueOnErrors) {
                success = 0;
                goto commonExit;
            }
        }
        if (fileList == 0) {
            continue;
        }

        //  TODO - refactor functionalize
        file = mprOpen(mpr, fileList, O_RDONLY | O_TEXT, 0);
        if (file == 0) {
            mprError(http, "Can't open %s", fileList);
            goto commonExit;
        }

        line = 0;
        while (mprGets(file, urlBuf, sizeof(urlBuf)) != NULL && !mprIsExiting(http)) {
            lock();
            if (fetchCount >= iterations) {
                unlock();
                break;
            }
            unlock();
            if ((cp = strchr(urlBuf, '\n')) != 0) {
                *cp = '\0';
            }
            operation = mprStrTok(urlBuf, " \t\n", &tok);
            url = mprStrTok(0, " \t\n", &tok);
    
            if (*url == '/') {
                mprSprintf(urlBuf2, sizeof(urlBuf2), "%s%s", host, url);
                url = urlBuf2;
            }

            if (strcmp(operation, "GET") == 0) {
                rc = fetch(http, operation, url);

            } else if (strcmp(operation, "HEAD") == 0) {
                rc = fetch(http, operation, url);

            } else if (strcmp(operation, "POST") == 0) {
                while (mprGets(file, bodyBuf, sizeof(bodyBuf)) != NULL) {
                    if (bodyBuf[0] != '\t') {
                        break;
                    }
                    if (strlen(bodyBuf) == 1) {
                        break;
                    }
                    mprPutBlockToBuf(body, (char*) &bodyBuf[1], strlen(bodyBuf) - 2);
                    mprAddNullToBuf(body);
                }
                if (mprGetBufLength(body) > 0) {
                    mprSetHttpForm(http, mprGetBufStart(body), mprGetBufLength(body));
                }
                rc = fetch(http, operation, url);
                mprFlushBuf(body);

            } else {
                rc = -1;
                mprError(http, "Bad operation on line %d", line);
            }
            if (rc < 0 && !continueOnErrors) {
                success = 0;
                mprFree(file);
                file = 0;
                goto commonExit;
            }
        }
        mprFree(file);
        file = 0;
    }

commonExit:

#if BLD_FEATURE_MULTITHREAD
    if (threadp) {
        lock();
        activeLoadThreads--;
        unlock();
    }
#endif
    return;
}


/*
 *  Invoked for content and completion
 */
static void callback(MprHttp *http, int nbytes)
{
    MprHttpResponse     *resp;
    char                *buf;

    if (nbytes > 0) {
        resp = http->response;
        buf = (char*) resp->content->start;
        if (!noout) {
            showOutput(http, buf, nbytes);
        }
        if (saveDir || cmpDir) {
            if (saveFile == 0) {
               mprMakeTempFileName(http, saveFileName, sizeof(saveFileName), 0);
                if ((saveFile = mprOpen(http, saveFileName, O_CREAT | O_TRUNC | O_RDWR | O_BINARY, 0)) == 0) {
                    mprError(http, "Can't open %s", saveFileName);
                    return;
                }
            }
            mprWrite(saveFile, buf, nbytes);
        }
    }
}


static bool isPort(cchar *name)
{
    cchar   *cp;

    for (cp = name; *cp && *cp != '/'; cp++) {
        if (!isdigit((int) *cp) || *cp == '.') {
            return 0;
        }
    }
    return 1;
}


/*
 *  Issue the HTTP request
 */
static int fetch(MprHttp *http, char *method, char *url)
{
    MprFile     *file;
    MprTime     mark;
    MprKeyValue *header;
    cchar       *msg, *item;
    char        urlBuf[MPR_MAX_STRING], *responseHeaders;
    int         code, rc, contentLen, elapsed, next;

    file = 0;
    mprAssert(url && *url);

    lock();
    fetchCount++;
    unlock();

    if (*url == '/') {
        mprSprintf(urlBuf, sizeof(urlBuf), "http://127.0.0.1%s", url);
        url = urlBuf;

    } else if (mprStrcmpAnyCaseCount(url, "http://", 7) != 0 && mprStrcmpAnyCaseCount(url, "https://", 8) != 0) {
        if (isPort(url)) {
            mprSprintf(urlBuf, sizeof(urlBuf), "http://127.0.0.1:%s", url);
        } else {
            mprSprintf(urlBuf, sizeof(urlBuf), "http://%s", url);
        }
        url = urlBuf;
    }

    mprLog(http, MPR_DEBUG, "fetch: %s %s", method, url);
    mark = mprGetTime(mpr);

    mprSetHttpCallback(http, callback, 0);

    for (next = 0; (header = mprGetNextItem(headers, &next)) != 0; ) {
        mprSetHttpHeader(http, header->key, header->value, 0);
    }
    if (ranges) {
        mprSetHttpHeader(http, "Range", ranges, 1);
    }

    if (formData) {
        for (next = 0; (item = mprGetNextItem(formData, &next)) != 0; ) {
            mprAddHttpFormItem(http, item, NULL);
        }

    } else if (bodyData) {
        mprSetHttpBody(http, bodyData, bodyLen);
    }

    if (mprHttpRequest(http, method, url, 0) < 0) {
        mprError(http, "Can't retrieve \"%s\". %s.", url, mprGetHttpError(http));
        return MPR_ERR_CANT_OPEN;
    }

    code = mprGetHttpCode(http);
    contentLen = mprGetHttpContentLength(http);
    msg = mprGetHttpCodeString(http, code);

    elapsed = (int) (mprGetTime(mpr) - mark);
    mprLog(http, 6, "Response code %d, content len %d", code, contentLen);

    if (showHeaders) {
        responseHeaders = mprGetHttpHeaders(http);
        mprPrintf(http, "%s\n", responseHeaders);
        mprFree(responseHeaders);
    }

    if (code < 0) {
        mprError(http, "Can't retrieve \"%s\", %s", url, mprGetHttpError(http));
        return MPR_ERR_CANT_READ;

    } else if (code == 0 && http->protocolVersion == 0) {
        ;

    } else if (!(200 <= code && code <= 206) && !(301 <= code && code <= 304)) {
        mprError(http, "Can't retrieve \"%s\" (%d), %s.", url, code, mprGetHttpError(http));
        return MPR_ERR_CANT_READ;
    }

    if (cmpDir && (rc = compare(http, url)) < 0) {
        return rc;
    }

    if (saveDir && (rc = save(http, url)) < 0) {
        return rc;
    }
    if (saveFile) {
        mprFree(saveFile);
        saveFile = 0;
    }

    lock();
    if (verbose && noout) {
        trace(http, url, fetchCount, method, code, contentLen);
    }

    if (singleStep) {
        mprPrintf(http, "Pause: ");
        read(0, (char*) &rc, 1);
    }
    unlock();

    return 0;
}


static int compare(MprHttp *http, cchar *url)
{
    MprUri      *parsedUri;
    MprFile     *file;
    struct stat sbuf;
    char        *content, *diffBuf, path[MPR_MAX_PATH], tmp[MPR_MAX_PATH];
    int         i, contentLen;

    if (saveFile == 0) {
        mprError(http, "Failed comparison for %s no output\n");
        return MPR_ERR_CANT_ACCESS;
    }
    mprSeek(saveFile, SEEK_END, 0);
    contentLen = mprSeek(saveFile, SEEK_CUR, 0);

    content = (char*) mprAlloc(http, contentLen);
    if (content == 0) {
        mprError(http, "Can't allocate memory %d", contentLen);
        return MPR_ERR_NO_MEMORY;
    }
    mprSeek(saveFile, SEEK_SET, 0);
    mprRead(saveFile, content, contentLen);

    parsedUri = mprParseUri(http, url);
    mprSprintf(path, sizeof(path), "%s%s", cmpDir, parsedUri->url);
    if (path[strlen(path) - 1] == '/') {
        path[strlen(path) - 1] = '\0';
    }
    if (stat(path, &sbuf) < 0) {
        mprError(http, "Can't access %s", path);
        return MPR_ERR_CANT_ACCESS;
    }
    if (sbuf.st_mode & S_IFDIR) {
        strcpy(tmp, path);
        mprSprintf(path, sizeof(path), "%s/_DEFAULT_.html", tmp);
    }
    if (stat(path, &sbuf) < 0) {
        mprError(http, "Can't access %s", path);
        return MPR_ERR_CANT_ACCESS;
    }
    if ((int) sbuf.st_size != contentLen) {
        mprError(http, "Failed comparison for %s ContentLen %d, size %d\n", url, contentLen, (int) sbuf.st_size);
        return MPR_ERR_CANT_ACCESS;
    }
    if ((file = mprOpen(http, path, O_RDONLY | O_BINARY, 0)) == 0) {
        mprError(http, "Can't open %s", path);
        return MPR_ERR_CANT_OPEN;
    }
    diffBuf = (char*) mprAlloc(http, contentLen);
    if (mprRead(file, diffBuf, contentLen) != contentLen) {
        mprError(http, "Can't read content from %s", path);
        return MPR_ERR_CANT_READ;
    }

    for (i = 0; i < contentLen; i++) {
        if (diffBuf[i] != content[i]) {
            mprError(http, "Failed comparison for at byte %d: %x vs %x\n", i,  (uchar) diffBuf[i], (uchar) content[i]);
            return MPR_ERR_GENERAL;
        }
    }
    mprFree(file);
    mprFree(diffBuf);

    return 0;
}


static int save(MprHttp *http, cchar *url)
{
    MprUri      *parsedUri;
    MprFile     *file, *from;
    struct stat sbuf;
    char        buf[MPR_BUFSIZE], path[MPR_MAX_PATH], dir[MPR_MAX_PATH], tmp[MPR_MAX_PATH], *headers;
    int         count;

    parsedUri = mprParseUri(http, url);
    mprSprintf(path, sizeof(path), "%s%s", saveDir, parsedUri->url);
    if (path[strlen(path) - 1] == '/') {
        path[strlen(path) - 1] = '\0';
    }
    mprGetDirName(dir, sizeof(dir), path);
    mprMakeDirPath(http, dir, 0755);
    if (stat(path, &sbuf) == 0 && sbuf.st_mode & S_IFDIR) {
        strcpy(tmp, path);
        mprSprintf(path, sizeof(path), "%s/_DEFAULT_.html", tmp);
    }

    if ((file = mprOpen(http, path, O_WRONLY | O_TRUNC | O_CREAT | O_BINARY, 0660)) == 0) {
        mprError(http, "Can't open %s", path);
        return MPR_ERR_CANT_OPEN;
    }
    
    if (showHeaders) {
        headers = mprGetHttpHeaders(http);
        if (mprPuts(file, headers, strlen(headers)) < 0) {
            mprError(http, "Can't write header to %s", path);
            mprFree(headers);
            mprFree(file);
            return MPR_ERR_CANT_WRITE;
        }
        mprFree(headers);
        mprPutc(file, '\n');
    }

    /*
     *  Copy saved data in temp into the output file
     */
    if ((from = mprOpen(http, saveFileName, O_RDONLY | O_BINARY, 0)) == 0) {
        mprError(http, "Can't open %s", saveFileName);
        return MPR_ERR_CANT_OPEN;
    }
    
    while ((count = mprRead(from, buf, sizeof(buf))) > 0) {
        mprWrite(file, buf, count);
    }

    mprFree(from);
    mprFree(file);
    return 0;
}


static void showOutput(MprHttp *http, cchar *buf, int count)
{
    MprHttpResponse     *resp;
    int                 i, c;
    
    resp = http->response;

    if (resp->code == 401 || (301 <= resp->code && resp->code <= 302)) {
        return;
    }

    for (i = 0; i < count; i++) {
        if (!isprint((int) buf[i]) && buf[i] != '\n' && buf[i] != '\r' && buf[i] != '\t') {
            isBinary = 1;
            break;
        }
    }
    for (i = 0; i < count; i++) {
        c = (uchar) buf[i];
        if (printable && isBinary) {
            mprPrintf(http, "%02x ", c & 0xff);
        } else {
            putchar(buf[i]);
        }
    }
    fflush(stdout);
}


static void trace(MprHttp *http, cchar *url, int fetchCount, cchar *method, int code, int contentLen)
{
    if (mprStrcmpAnyCaseCount(url, "http://", 7) != 0) {
        url += 7;
    }
    if ((fetchCount % 200) == 1) {
        if (fetchCount == 1 || (fetchCount % 5000) == 1) {
            if (fetchCount > 1) {
                mprPrintf(http, "\n");
            }
            mprPrintf(http, "  Count  Thread   Op  Code   Bytes  Url\n");
        }
        mprPrintf(http, "%7d %7s %4s %5d %7d  %s\n", fetchCount - 1,
            mprGetCurrentThreadName(http), method, code, contentLen, url);
    }
}


#if BLD_FEATURE_MULTITHREAD
static void lock()
{
    mprLock(mutex);
}
    

static void unlock()
{
    mprUnlock(mutex);
}
#endif



static void logHandler(MprCtx ctx, int flags, int level, const char *msg)
{
    Mpr         *mpr;
    MprFile     *file;
    char        *prefix;

    mpr = mprGetMpr(ctx);
    file = (MprFile*) mpr->logHandlerData;
    prefix = mpr->name;

    while (*msg == '\n') {
        mprFprintf(file, "\n");
        msg++;
    }

    if (flags & MPR_LOG_SRC) {
        mprFprintf(file, "%s: %d: %s\n", prefix, level, msg);

    } else if (flags & MPR_ERROR_SRC) {
        /*
         *  Use static printing to avoid malloc when the messages are small.
         *  This is important for memory allocation errors.
         */
        if (strlen(msg) < (MPR_MAX_STRING - 32)) {
            mprStaticPrintf(file, "%s: Error: %s\n", prefix, msg);
        } else {
            mprFprintf(file, "%s: Error: %s\n", prefix, msg);
        }

    } else if (flags & MPR_FATAL_SRC) {
        mprFprintf(file, "%s: Fatal: %s\n", prefix, msg);
        
    } else if (flags & MPR_ASSERT_SRC) {
        mprFprintf(file, "%s: Assertion %s, failed\n", prefix, msg);

    } else if (flags & MPR_RAW) {
        mprFprintf(file, "%s", msg);
    }
    
    if (flags & (MPR_ERROR_SRC | MPR_FATAL_SRC | MPR_ASSERT_SRC)) {
        mprBreakpoint();
    }
}



static int startLogging(Mpr *mpr, char *logSpec)
{
    MprFile     *file;
    char        *levelSpec;
    int         level;

    level = 0;

    //  TODO - move should not be changing logSpec.
    if ((levelSpec = strchr(logSpec, ':')) != 0) {
        *levelSpec++ = '\0';
        level = atoi(levelSpec);
    }

    if (strcmp(logSpec, "stdout") == 0) {
        file = mpr->fileService->console;

    } else {
        if ((file = mprOpen(mpr, logSpec, O_CREAT | O_WRONLY | O_TRUNC | O_TEXT, 0664)) == 0) {
            mprErrorPrintf(mpr, "Can't open log file %s\n", logSpec);
            return -1;
        }
    }

    mprSetLogLevel(mpr, level);
    mprSetLogHandler(mpr, logHandler, (void*) file);

    return 0;
}


#if WIN || VXWORKS
static char *getpass(char *prompt)
{
    static char password[MPR_MAX_STRING];
    int     c, i;

    fputs(prompt, stderr);
    for (i = 0; i < (int) sizeof(password) - 1; i++) {
#if VXWORKS
        c = getchar();
#else
        c = _getch();
#endif
        if (c == '\r' || c == EOF) {
            break;
        }
        if ((c == '\b' || c == 127) && i > 0) {
            password[--i] = '\0';
            fputs("\b \b", stderr);
            i--;
        } else if (c == 26) {           /* Control Z */
            c = EOF;
            break;
        } else if (c == 3) {            /* Control C */
            fputs("^C\n", stderr);
            exit(255);
        } else if (!iscntrl(c) && (i < (int) sizeof(password) - 1)) {
            password[i] = c;
            fputc('*', stderr);
        } else {
            fputc('', stderr);
            i--;
        }
    }
    if (c == EOF) {
        return "";
    }
    fputc('\n', stderr);
    password[i] = '\0';
    return password;
}

#endif /* WIN */


static char *getPassword(MprCtx ctx)
{
    char    *password;

    password = getpass("Password: ");
    return mprStrdup(ctx, password);
}


static char *getBody(MprCtx ctx, cchar *path, int *len)
{
    MprFile     *file;
    MprBuf      *data;
    char        buffer[MPR_BUFSIZE];
    int         bytes, rc;

    file = mprOpen(ctx, path, O_RDONLY | O_BINARY, 0);
    if (file == 0) {
        mprError(ctx, "Can't open %s", path);
        return 0;
    }

    data = mprCreateBuf(ctx, 0, (int) mprGetFileSize(file) + 1);
    if (data == 0) {
        return 0;
    }

    rc = 0;
    while ((bytes = mprRead(file, buffer, MPR_BUFSIZE)) > 0) {
        if (mprPutBlockToBuf(data, buffer, bytes) != bytes) {
            return 0;
        }
    }

    mprAddNullToBuf(data);
    *len = mprGetBufLength(data);
    return mprGetBufStart(data);
}


#else /* BLD_FEATURE_HTTP_CLIENT */
void __dummy_httpClient() {}
#endif

/*
 *  @copy   default
 *  
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *  
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire 
 *  a commercial license from Embedthis Software. You agree to be fully bound 
 *  by the terms of either license. Consult the LICENSE.TXT distributed with 
 *  this software for full details.
 *  
 *  This software is open source; you can redistribute it and/or modify it 
 *  under the terms of the GNU General Public License as published by the 
 *  Free Software Foundation; either version 2 of the License, or (at your 
 *  option) any later version. See the GNU General Public License for more 
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *  
 *  This program is distributed WITHOUT ANY WARRANTY; without even the 
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *  
 *  This GPL license does NOT permit incorporating this software into 
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses 
 *  for this software and support services are available from Embedthis 
 *  Software at http://www.embedthis.com 
 *  
 *  @end
 */
