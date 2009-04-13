/*
 *  ejsCgi.c -- CGI web framework handler.
 *
 *  The ejs handler supports the Ejscript web framework for applications using server-side Javascript. 
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 *
 *  TODO
 *      - Input post data
 *          - How does Ejscript get post data that is not www encoded?
 *      - make more robust against bad input
 *      - return code diagnostics
 */
/********************************** Includes **********************************/

#include    "ejs.h"

/***************************** Forward Declarations *****************************/

static void copyFile(cchar *url);
static int  createFormData();
static void decodeFormData(cchar *data);
static void emitHeaders();
static void flushOutput(MprBuf *buf);
static char *getDateString(MprFileInfo *sbuf);
static int  getPostData();
static int  getRequest();
static int  initControlBlock();
static void processRequest();

/*
 *  Control callbacks
 */
static void defineParams(void *handle);
static void discardOutput(void *handle);
static void error(void *handle, int code, cchar *msg, ...);
static cchar *getHeader(void *handle, cchar *key);
static EjsVar *getVar(void *handle, int collection, int field);
static void redirect(void *handle, int code, cchar *url);
static void setCookie(void *handle, cchar *name, cchar *value, int lifetime, cchar *path, bool secure);
static void setHeader(void *handle, bool allowMultiple, cchar *key, cchar *fmt, ...);
static void setHttpCode(void *handle, int code);
static void setMimeType(void *handle, cchar *mimeType);
static int setVar(void *handle, int collection, int field, EjsVar *value);
static int writeBlock(void *handle, cchar *buf, int size);

/*
 *  Routines for Request, Response and Host objects
 */
static EjsVar *createHeaderObject(Ejs *ejs, MprHashTable *table);
static EjsVar *createString(Ejs *ejs, cchar *value);
static EjsVar *getHostVar(void *handle, int field);
static EjsVar *getRequestVar(void *handle, int field);
static EjsVar *getResponseVar(void *handle, int field);

#if UNUSED
static char hex2Char(char* s);
static void descape(char* src);
#endif

/************************************ Locals **********************************/

static int              debug;                      /* Trace request details to /tmp/ejscgi.log */
static Ejs              *ejs;
static Mpr              *mpr;
static EjsWeb           *web;
static EjsWebControl    *control;
static MprHashTable     *requestHeaders;
static MprHashTable     *formVars;
static FILE             *debugFile;

/*
 *  Parsed request details
 */
static int              contentLength = -1;
static char             *contentType;
static char             *cookie;
static char             *currentDate;
static char             *documentRoot;
static char             *ext;
static char             *pathInfo;
static char             *pathTranslated;
static char             *query;
static char             *scriptName;
static char             *uri;

/*
 *  Response fields
 */
static int              headersEmitted;
static char             *input;
static char             *responseMsg;
static int              responseCode;
static MprHashTable     *responseHeaders;
static char             *responseMimeType;
static MprBuf           *output;
static MprBuf           *headerOutput;

#if VXWORKS
static char             **ppGlobalEnviron;
#endif
#if BLD_DEBUG
static int              dummy;                      /* Mock up a dummy request */
#endif

/************************************* Code ***********************************/

int main(int argc, char **argv)
{
    cchar   *searchPath, *argp, *program;
    int     nextArg, err;

    /*
     *  Create the Embedthis Portable Runtime (MPR) and setup a memory failure handler
     */
    mpr = mprCreate(argc, argv, ejsMemoryFailure);
    program = mprGetBaseName(argv[0]);
    mprSetAppName(mpr, program, 0, 0);

    if (strcmp(program, "ejscgi-debug") == 0) {
        debug++;
    }

    if (mprStart(mpr, 0) < 0) {
        mprError(mpr, "Can't start mpr services");
        return EJS_ERR;
    }

    for (err = 0, nextArg = 1; nextArg < argc; nextArg++) {
        argp = argv[nextArg];
        if (*argp != '-') {
            break;
        }

        if (strcmp(argp, "--debug") == 0 || strcmp(argp, "-d") == 0) {
            debug++;

#if BLD_DEBUG
        } else if (strcmp(argp, "--dummy") == 0) {
            dummy++;
#endif
        } else if (strcmp(argp, "--log") == 0 || strcmp(argp, "-l") == 0) {
            if (nextArg >= argc) {
                err++;
            } else {
                ejsStartLogging(mpr, argv[++nextArg]);
            }

        } else if (strcmp(argp, "--searchpath") == 0) {
            if (nextArg >= argc) {
                err++;
            } else {
                searchPath = argv[++nextArg];
            }

        } else if (strcmp(argp, "--version") == 0 || strcmp(argp, "-V") == 0) {
            mprErrorPrintf(mpr, "%s %s\n"
                "Copyright (C) Embedthis Software 2003-2009\n"
                "Copyright (C) Michael O'Brien 2003-2009\n",
               BLD_NAME, BLD_VERSION);
            exit(0);

        } else {
            err++;
            break;
        }
    }

    if (err) {
        mprErrorPrintf(mpr,
            "Usage: %s [options]\n"
            "  Options:\n"
            "  --log logSpec            # Diagnostic trace\n"
            "  --searchpath ejsPath     # Module search path\n"
            "  --version                # Emit the program version information\n\n",
            mpr->name);
        return -1;
    }

    if (initControlBlock() < 0) {
        error(NULL, 0, "Can't initialize control block");
        exit(1);
    }

    //  FAST CGI must update this
    currentDate = getDateString(0);

    if (getRequest() < 0) {
        error(NULL, 0, "Can't get request");
    } else {
        processRequest();
    }

    if (responseCode && responseMsg) {
        fprintf(stderr, "ejscgi: ERROR: %s\n", responseMsg);
    }

    return 0;
}


/*********************************** Request Processing ***************************/
/*
 *  Create an initialize the Ejscript Web Framework control block
 */
static int initControlBlock() 
{
    control = mprAllocZeroed(mpr, sizeof(EjsWebControl));
    if (control == 0) {
        return MPR_ERR_NO_MEMORY;
    }

    /*
     *  These are the callbacks from the web framework into the gateway
     */
    control->defineParams = defineParams;
    control->discardOutput = discardOutput;
    control->error = error;
    control->getHeader = getHeader;
    control->getVar = getVar;
    control->redirect = redirect;
    control->setCookie = setCookie;
    control->setHeader = setHeader;
    control->setHttpCode = setHttpCode;
    control->setMimeType = setMimeType;
    control->setVar = setVar;
    control->write = writeBlock;

    if (ejsOpenWebFramework(control, 0) < 0) {
        return EJS_ERR;
    }

    output = mprCreateBuf(mpr, EJS_CGI_MIN_BUF, EJS_CGI_MAX_BUF);
    headerOutput = mprCreateBuf(mpr, MPR_BUFSIZE, -1);
    if (output == 0 || headerOutput == 0) {
        return EJS_ERR;
    }
    return 0;
}


/*
 *  Get a request and parse environment and request URI. Given:
 *
 *      http /simple.cgi/abc/def?a=b&c=d
 *      DOCUMENT_ROOT=/Users/mob/hg/appweb/src/server/web
 *      GATEWAY_INTERFACE=CGI/1.1
 *      CONTENT_LENGTH=NNN 
 *      CONTENT_TYPE
 *      PATH_INFO=/abc/def
 *      PATH_TRANSLATED=/Users/mob/hg/appweb/src/server/web/ab/def
 *      QUERY_STRING=a=b&c=d
 *      REMOTE_ADDR=10.0.0.1234
 *      REMOTE_HOST
 *      REMOTE_PORT
 *      REQUEST_URI=/simple.cgi
 *      REQUEST_METHOD=GET
 *      REQUEST_TRANSPORT=http
 *      SCRIPT_FILENAME=/Users/mob/hg/appweb/src/server/web/simple.cgi
 *      SERVER_NAME=127.0.0.1:7777
 *      SERVER_PORT=7777
 *      SERVER_SOFTWARE=Embedthis-Appweb/3.0A.1
 *      SERVER_PROTOCOL=http
 *      HTTP_ACCEPT=
 *      HTTP_COOKIE=
 *      HTTP_REFERRER=
 *      HTTP_CONNECTION=Keep-Alive
 *      HTTP_HOST=127.0.0.1
 *      HTTP_USER_AGENT=Embedthis-http/3.0A.1
 *      HTTPS
 *      HTTP_*
 *      PATH=
 */
static int getRequest() 
{
    char    key[MPR_MAX_STRING], *ep, *cp, *value;
    int     len, i;

    formVars = mprCreateHash(mpr, EJS_CGI_HDR_HASH);
    requestHeaders = mprCreateHash(mpr, EJS_CGI_HDR_HASH);
    responseHeaders = mprCreateHash(mpr, EJS_CGI_HDR_HASH);

    if (debug) {
        debugFile = fopen("/tmp/ejscgi.log", "w+");
    }

    for (i = 0; environ && environ[i]; i++) {
        if ((ep = environ[i]) == 0) {
            continue;
        }
        if ((cp = strchr(ep, '=')) != 0) {
            len = cp - ep;
            mprMemcpy(key, sizeof(key), ep, len);
            key[len] = '\0';
            mprAddHash(requestHeaders, key, ++cp);
            if (debugFile) {
                fprintf(debugFile, "%-20s = %s\n", key, cp);
            }
        }
    }
    if (debugFile) {
        fclose(debugFile);
        debugFile = 0;
    }

#if BLD_DEBUG
    if (dummy) {
        mprAddHash(requestHeaders, "SCRIPT_NAME", mprStrdup(mpr, "/cgi/ejscgi"));
        mprAddHash(requestHeaders, "DOCUMENT_ROOT", mprStrdup(mpr, "/Users/mob/hg/carmen"));
        mprAddHash(requestHeaders, "PATH_TRANSLATED", mprStrdup(mpr, "/Users/mob/hg/carmen"));
        mprAddHash(requestHeaders, "QUERY_STRING", mprStrdup(mpr, "a=b&c=d"));

        mprAddHash(requestHeaders, "PATH_INFO", mprStrdup(mpr, "/carmen/stock/"));
        mprAddHash(requestHeaders, "REQUEST_URI", mprStrdup(mpr, "/cgi-bin/ejscgi/carmen/stock/"));

//        mprAddHash(requestHeaders, "PATH_INFO", mprStrdup(mpr, "/carmen/web/style.css"));
//        mprAddHash(requestHeaders, "REQUEST_URI", mprStrdup(mpr, "/cgi-bin/ejscgi/carmen/web/style.css"));

//        mprAddHash(requestHeaders, "PATH_INFO", mprStrdup(mpr, "/carmen/web/images/banner.jpg"));
//        mprAddHash(requestHeaders, "REQUEST_URI", mprStrdup(mpr, "/cgi-bin/ejscgi/carmen/web/images/banner.jpg"));

    }
#endif

    documentRoot = (char*) mprLookupHash(requestHeaders, "DOCUMENT_ROOT");
    cookie = (char*) mprLookupHash(requestHeaders, "HTTP_COOKIE");
    contentType = (char*) mprLookupHash(requestHeaders, "CONTENT_TYPE");
    query = (char*) mprLookupHash(requestHeaders, "QUERY_STRING");
    pathTranslated = (char*) mprLookupHash(requestHeaders, "PATH_TRANSLATED");
    pathInfo = (char*) mprLookupHash(requestHeaders, "PATH_INFO");
    scriptName = (char*) mprLookupHash(requestHeaders, "SCRIPT_NAME");
    uri = (char*) mprLookupHash(requestHeaders, "REQUEST_URI");

    if (documentRoot == 0 || pathInfo == 0 || scriptName == 0 || uri == 0) {
        error(NULL, 0, "CGI environment not setup correctly");
        return EJS_ERR;
    }

    value = (char*) mprLookupHash(requestHeaders, "CONTENT_LENGTH");
    if (value) {
        contentLength = atoi(value);
    }
    ext = strrchr(uri, '.');

    if (createFormData() < 0) {
        return EJS_ERR;
    }
    return 0;
}


/*
 *  Process one request
 */
static void processRequest() 
{
    char        *appName, *url;
    int         flags;

    appName = mprStrdup(mpr, pathInfo);
    if (*appName == '/') {
        appName++;
    }
    if ((url = strchr(appName, '/')) != 0) {
        *url++ = '\0';
    }
    if (strncmp(url, "web/", 4) == 0) {
        copyFile(url);
        flushOutput(output);
        return;
    }

    /*
     *  Set default response headers
     */
    setHeader(NULL, 0, "Content-Type", "text/html");
    setHeader(NULL, 0, "Last-Modified", currentDate);
    setHeader(NULL, 0, "Cache-Control", "no-cache");

    flags = 0;
    if (ext == 0) {
        flags |= EJS_WEB_FLAG_APP;
    }
    if ((web = ejsCreateWebRequest(mpr, control, NULL, scriptName, pathInfo, documentRoot, flags)) == 0) {
        error(NULL, 0, "Can't create web request");
        return;
    }
    ejs = web->ejs;

    if (ejsRunWebRequest(web) < 0) {
        error(NULL, 0, "%s", web->error);
        return;
    }
    flushOutput(output);
}


static void copyFile(cchar *url)
{
    MprFile     *file;
    char        path[MPR_MAX_FNAME], buf[MPR_BUFSIZE], *ext;
    int         len;

    ext = strrchr(url, '.');
    if (ext) {
        ++ext;
        if (strcmp(ext, "htm") == 0 || strcmp(ext, "html") == 0) {
            setHeader(NULL, 0, "Content-Type", "text/html");
        } else if (strcmp(ext, "jpg") == 0 || strcmp(ext, "jpeg") == 0) {
            setHeader(NULL, 0, "Content-Type", "image/jpeg");
        } else if (strcmp(ext, "png") == 0) {
            setHeader(NULL, 0, "Content-Type", "image/png");
        } else if (strcmp(ext, "gif") == 0) {
            setHeader(NULL, 0, "Content-Type", "image/gif");
        } else if (strcmp(ext, "tiff") == 0) {
            setHeader(NULL, 0, "Content-Type", "image/tiff");
        } else if (strcmp(ext, "ico") == 0) {
            setHeader(NULL, 0, "Content-Type", "image/x-ico");
        } else if (strcmp(ext, "bmp") == 0) {
            setHeader(NULL, 0, "Content-Type", "image/bmp");
        } else if (strcmp(ext, "pdf") == 0) {
            setHeader(NULL, 0, "Content-Type", "image/pdf");
        } else if (strcmp(ext, "txt") == 0) {
            setHeader(NULL, 0, "Content-Type", "text/plain");
        } else if (strcmp(ext, "css") == 0) {
            setHeader(NULL, 0, "Content-Type", "text/css");
        }
    }
    mprSprintf(path, sizeof(path), "%s/%s", documentRoot, url);
    file = mprOpen(mpr, path, O_RDONLY, 0);
    if (file == 0) {
        error(NULL, 0, "Can't open %s", path);
        return;
    }
    while ((len = mprRead(file, buf, sizeof(buf))) > 0) {
        if (writeBlock(web, buf, len) != len) {
            error(NULL, 0, "Can't write data from %s", path);
            return;
        }
    }
}


/****************************** Control Callbacks ****************************/
/*
 *  Define params[] using query and post form data
 */
static void defineParams(void *handle)
{
    MprHash     *hp;

    hp = mprGetFirstHash(formVars);
    while (hp) {
        ejsDefineWebParam(ejs, hp->key, hp->data);
        hp = mprGetNextHash(formVars, hp);
    }
}


static void discardOutput(void *handle)
{
    mprFlushBuf(output);
}


//  TODO - do we need code?
void error(void *handle, int code, cchar *fmt, ...)
{
    va_list args;

    if (responseMsg == 0) {
        if (code == 0) {
            code = MPR_HTTP_CODE_BAD_GATEWAY;
        }
        responseCode = code;
        va_start(args, fmt);
        mprAllocVsprintf(mpr, &responseMsg, -1, fmt, args);
        va_end(args);
    }
}


static cchar *getHeader(void *handle, cchar *key)
{
    return (cchar*) mprLookupHash(requestHeaders, key);
}


/*
 *  Get a variable from one of the virtual objects: Host, Request, Response
 */
static EjsVar *getVar(void *handle, int collection, int field)
{
    switch (collection) {
    case EJS_WEB_REQUEST_VAR:
        return getRequestVar(handle, field);
    case EJS_WEB_RESPONSE_VAR:
        return getResponseVar(handle, field);
    case EJS_WEB_HOST_VAR:
        return getHostVar(handle, field);
    default:
        return 0;
    }
}


/*
 *  Redirect the client
 */
static void redirect(void *handle, int code, cchar *url)
{
    setHeader(handle, 0, "Location", url);
}


/*
 *  Add a response cookie
 */
static void setCookie(void *handle, cchar *name, cchar *value, int lifetime, cchar *path, bool secure)
{
    struct tm   tm;
    time_t      when;
    MprTime     time;
    char        dateStr[64];

    if (path == 0) {
        path = "/";
    }

    if (lifetime > 0) {
        time = mprGetTime(mpr);
        when = time + lifetime * MPR_TICKS_PER_SEC;
        mprGmtime(mpr, &tm, when);
        mprRfctime(mpr, dateStr, sizeof(dateStr), &tm);

        setHeader(handle, 1, "Set-Cookie", "%s=%s; path=%s; Expires=%s;%s", name, value, path, dateStr, 
            secure ? " secure" : "");

    } else {
        setHeader(handle, 1, "Set-Cookie", "%s=%s; path=%s;%s", name, value, path, secure ? " secure" : "");
    }
    setHeader(handle, 0, "Cache-control", "no-cache=\"set-cookie\"");
}


/*
 *  Add a response header
 */
static void setHeader(void *handle, bool allowMultiple, cchar *key, cchar *fmt, ...)
{
    char            *value;
    va_list         vargs;

    va_start(vargs, fmt);
    mprAllocVsprintf(mpr, &value, EJS_MAX_HEADERS, fmt, vargs);

    if (allowMultiple) {
        mprAddDuplicateHash(responseHeaders, key, value);
    } else {
        mprAddHash(responseHeaders, key, value);
    }
}


/*
 *  Set the Http response code
 */
static void setHttpCode(void *handle, int code)
{
    responseCode = code;
}


/*
 *  Set the response mime type
 */
static void setMimeType(void *handle, cchar *mimeType)
{
    setHeader(handle, 0, "Content-Type", mimeType);
}


static int setVar(void *handle, int collection, int field, EjsVar *value)
{
    //  TODO - currently all fields are read-only
    return EJS_ERR;
}


/*
 *  Write data to the client. Will buffer and flush as required. will create headers if required.
 */
static int writeBlock(void *handle, cchar *buf, int size)
{
    int     len, rc;

    len = mprGetBufLength(output);
    if ((len + size) < EJS_CGI_MAX_BUF) {
        rc = mprPutBlockToBuf(output, buf, size);
    } else {
        flushOutput(output);
        if (size < EJS_CGI_MAX_BUF) {
            rc = mprPutBlockToBuf(output, buf, size);
        } else {
            rc = write(1, (char*) buf, size);
        }
    }
    return rc;
}


/*********************************************************************************/
/*
 *  Virtual Host, Request and Response objects. Better to create properties virtually as it is much faster
 */

static EjsVar *getRequestVar(void *handle, int field)
{
    switch (field) {
    case ES_ejs_web_Request_accept:
        return createString(ejs, getHeader(handle, "HTTP_ACCEPT"));

    case ES_ejs_web_Request_acceptCharset:
        return createString(ejs, getHeader(handle, "HTTP_ACCEPT_CHARSET"));

    case ES_ejs_web_Request_acceptEncoding:
        return createString(ejs, getHeader(handle, "HTTP_ACCEPT_ENCODING"));

    case ES_ejs_web_Request_authAcl:
    case ES_ejs_web_Request_authGroup:
    case ES_ejs_web_Request_authUser:
        //  TODO 
        return (EjsVar*) ejs->undefinedValue;

    case ES_ejs_web_Request_authType:
        return createString(ejs, getHeader(handle, "HTTP_AUTH_TYPE"));

#if TODO
    case ES_ejs_web_Request_connection:
        return createString(ejs, req->connection);
#endif

    case ES_ejs_web_Request_contentLength:
        return (EjsVar*) ejsCreateNumber(ejs, contentLength);

    case ES_ejs_web_Request_cookies:
        if (cookie) {
            return (EjsVar*) ejsCreateCookies(ejs);
        } else {
            return ejs->nullValue;
        }
        break;

    case ES_ejs_web_Request_extension:
        return createString(ejs, ext);

    case ES_ejs_web_Request_files:
        //  TODO 
        return (EjsVar*) ejs->undefinedValue;

    case ES_ejs_web_Request_headers:
        return (EjsVar*) createHeaderObject(ejs, requestHeaders);

    case ES_ejs_web_Request_hostName:
        return createString(ejs, getHeader(handle, "HTTP_HOST"));

    case ES_ejs_web_Request_method:
        return createString(ejs, getHeader(handle, "REQUEST_METHOD"));

    case ES_ejs_web_Request_mimeType:
        return createString(ejs, getHeader(handle, "CONTENT_TYPE"));

    case ES_ejs_web_Request_pathInfo:
        return createString(ejs, getHeader(handle, "PATH_INFO"));

    case ES_ejs_web_Request_pathTranslated:
        return createString(ejs, getHeader(handle, "PATH_TRANSLATED"));

    case ES_ejs_web_Request_pragma:
        return createString(ejs, getHeader(handle, "HTTP_PRAGMA"));

    case ES_ejs_web_Request_query:
        return createString(ejs, getHeader(handle, "QUERY_STRING"));

    case ES_ejs_web_Request_originalUri:
        return createString(ejs, getHeader(handle, "REQUEST_URI"));

    case ES_ejs_web_Request_referrer:
        return createString(ejs, getHeader(handle, "HTTP_REFERRER"));

    case ES_ejs_web_Request_remoteAddress:
        return createString(ejs, getHeader(handle, "REMOTE_ADDR"));

    case ES_ejs_web_Request_remoteHost:
#if BLD_FEATURE_REVERSE_DNS && BLD_UNIX_LIKE
        {
            /*
             *  This feature has denial of service risks. Doing a reverse DNS will be slower,
             *  and can potentially hang the web server. Use at your own risk!!  Not supported for windows.
             */
            struct addrinfo *result;
            char            name[MPR_MAX_STRING];
            int             rc;

            if (getaddrinfo(remoteIpAddr, NULL, NULL, &result) == 0) {
                rc = getnameinfo(result->ai_addr, sizeof(struct sockaddr), name, sizeof(name), NULL, 0, NI_NAMEREQD);
                freeaddrinfo(result);
                if (rc == 0) {
                    return createString(ejs, name);
                }
            }
        }
#endif
        return createString(ejs, getHeader(handle, "REMOTE_ADDR"));

    case ES_ejs_web_Request_sessionID:
        return createString(ejs, web->session ? web->session->id : "");

    case ES_ejs_web_Request_url:
        return createString(ejs, uri);

    case ES_ejs_web_Request_userAgent:
        return createString(ejs, getHeader(handle, "HTTP_USER_AGENT"));
    }

    ejsThrowOutOfBoundsError(ejs, "Bad property slot reference");
    return 0;
}


static EjsVar *getHostVar(void *handle, int field)
{
    switch (field) {
    case ES_ejs_web_Host_documentRoot:
        return createString(ejs, getHeader(handle, "DOCUMENT_ROOT"));

    case ES_ejs_web_Host_name:
        return createString(ejs, getHeader(handle, "SERVER_NAME"));

    case ES_ejs_web_Host_protocol:
        return createString(ejs, getHeader(handle, "REQUEST_TRANSPORT"));

#if TODO
    case ES_ejs_web_Host_isVirtualHost:
        return (EjsVar*) ejsCreateBoolean(ejs, host->flags & MA_HOST_VHOST);

    case ES_ejs_web_Host_isNamedVirtualHost:
        return (EjsVar*) ejsCreateBoolean(ejs, host->flags & MA_HOST_NAMED_VHOST);
#endif

    case ES_ejs_web_Host_software:
        return createString(ejs, EJS_SERVER_NAME);

    case ES_ejs_web_Host_logErrors:
        return (EjsVar*) ((web->flags & EJS_WEB_FLAG_BROWSER_ERRORS) ? ejs->falseValue : ejs->trueValue);
    }

    ejsThrowOutOfBoundsError(ejs, "Bad property slot reference");
    return 0;
}


static EjsVar *getResponseVar(void *handle, int field)
{
    switch (field) {
    case ES_ejs_web_Response_code:
        return (EjsVar*) ejsCreateNumber(ejs, responseCode);

#if TODO
    case ES_ejs_web_Response_filename:
        return (EjsVar*) createString(ejs, programName);
#endif

    case ES_ejs_web_Response_headers:
        return (EjsVar*) createHeaderObject(ejs, responseHeaders);

    case ES_ejs_web_Response_mimeType:
        return (EjsVar*) createString(ejs, responseMimeType);

    default:
        ejsThrowOutOfBoundsError(ejs, "Bad property slot reference");
        return 0;
    }
}

/**********************************************************************************/

static int createFormData()
{ 
    char    *pat;

    decodeFormData(query);

    pat = "application/x-www-form-urlencoded";
    if (mprStrcmpAnyCaseCount(contentType, pat, (int) strlen(pat)) == 0) {
        if (getPostData() < 0) {
            return EJS_ERR;
        }
        if (contentLength > 0) {
            pat = "application/x-www-form-urlencoded";
            if (mprStrcmpAnyCaseCount(contentType, pat, (int) strlen(pat)) == 0) {
                decodeFormData(input);
            }
        }
    }
    return 0;
}


/*
 *  Create a string variable
 */
static EjsVar *createString(Ejs *ejs, cchar *value)
{
    if (value == 0) {
        return ejs->nullValue;
    }
    return (EjsVar*) ejsCreateString(ejs, value);
}


/*
 *  A header object from a given hash table
 */  
static EjsVar *createHeaderObject(Ejs *ejs, MprHashTable *table)
{
    MprHash     *hp;
    EjsVar      *headers, *header;
    EjsName     qname;
    int         index;
    
    headers = (EjsVar*) ejsCreateArray(ejs, mprGetHashCount(table));
    for (index = 0, hp = 0; (hp = mprGetNextHash(table, hp)) != 0; ) {
        header = (EjsVar*) ejsCreateSimpleObject(ejs);
        ejsSetPropertyByName(ejs, header, ejsName(&qname, "", hp->key), (EjsVar*) ejsCreateString(ejs, hp->data));
        ejsSetProperty(ejs, headers, index++, header);
    }
    return headers;
}


/*
 *  Emit all headers
 */
static void emitHeaders()
{
    MprHash     *hp;
    int         len;

    hp = mprGetFirstHash(responseHeaders);
    while (hp) {
        len = strlen(hp->key) + strlen(hp->data) + 4;
        if (mprGetBufSpace(headerOutput) < len) {
            flushOutput(headerOutput);
        }
        mprPutStringToBuf(headerOutput, hp->key);
        mprPutStringToBuf(headerOutput, ": ");
        mprPutStringToBuf(headerOutput, hp->data);
        mprPutStringToBuf(headerOutput, "\r\n");
        hp = mprGetNextHash(responseHeaders, hp);
    }
    mprPutStringToBuf(headerOutput, "\r\n");
    flushOutput(headerOutput);
    headersEmitted = 1;
}


/*
 *  Flush all output and emit headers first if required
 */
static void flushOutput(MprBuf *buf)
{
    int     rc, len;

    if (!headersEmitted && buf != headerOutput) {
        emitHeaders();
    }

    while ((len = mprGetBufLength(buf)) > 0) {
        rc = write(1, mprGetBufStart(buf), len);
        if (rc < 0) {
            //  TODO diag
            return;
        }
        mprAdjustBufStart(buf, rc);
    }
    mprFlushBuf(buf);
}


/*
 *  Decode the query and post form data into formVars
 */
static void decodeFormData(cchar *data)
{
    char    *value, *key, *buf;
    int     buflen;

    buf = mprStrdup(mpr, data);
    buflen = strlen(buf);

    /*
     *  Crack the input into name/value pairs 
     */
    for (key = strtok(buf, "&"); key; key = strtok(0, "&")) {
        if ((value = strchr(key, '=')) != 0) {
            *value++ = '\0';
            mprUrlDecode(value, (int) strlen(value) + 1, value);
        }
        mprUrlDecode(key, (int) strlen(key) + 1, key);
        mprAddHash(formVars, key, value);
    }
}


/*
 *  Read post data
 */
static int getPostData()
{
    int     bytes, len;

    if (contentLength == 0) {
        return 0;
    }

    input = (char*) malloc(contentLength + 1);
    if (input == 0) {
        error(NULL, 0, "Content length is too large");
        return MPR_ERR_NO_MEMORY;
    }

    for (len = 0; len < contentLength; ) {
        errno = 0;
        bytes = read(0, &input[len], contentLength - len);
        if (bytes < 0) {
            error(NULL, 0, "Couldn't read CGI input %d", errno);
            return MPR_ERR_CANT_READ;
        }
        len += bytes;
    }
    input[contentLength] = '\0';
    contentLength = len;
    return 0;
}


#if UNUSED
static char hex2Char(char* s) 
{
    char    c;

    if (*s >= 'A') {
        c = (*s & 0xDF) - 'A';
    } else {
        c = *s - '0';
    }
    s++;

    if (*s >= 'A') {
        c = c * 16 + ((*s & 0xDF) - 'A');
    } else {
        c = c * 16 + (*s - '0');
    }
    return c;
}


static void descape(char* src) 
{
    char    *dest;

    dest = src;
    while (*src) {
        if (*src == '%') {
            *dest++ = hex2Char(++src) ;
            src += 2;
        } else {
            *dest++ = *src++;
        }
    }
    *dest = '\0';
}
#endif


/*
 *  Get a date string. If sbuf is non-null, get the modified time of that file. If null, then get the current system time.
 */
static char *getDateString(MprFileInfo *sbuf)
{
    MprTime     when;
    struct tm   tm;
    char        *dateStr;

    if (sbuf == 0) {
        when = mprGetTime(mpr);
    } else {
        when = (MprTime) sbuf->mtime * MPR_TICKS_PER_SEC;
    }

    dateStr = (char*) mprAlloc(mpr, 64);
    mprGmtime(mpr, &tm, when);
    mprStrftime(mpr, dateStr, 64, "%a, %d %b %Y %T %Z", &tm);

    return dateStr;
}


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
