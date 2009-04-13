/**
 *  ejsHttp.c - Http class. This implements a HTTP client.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejs.h"

#if ES_ejs_io_Http && BLD_FEATURE_HTTP_CLIENT
/**************************** Forward Declarations ****************************/

static EjsVar   *getDateHeader(Ejs *ejs, EjsHttp *hp, cchar *key);
static EjsVar   *getStringHeader(Ejs *ejs, EjsHttp *hp, cchar *key);
static void     prepForm(Ejs *ejs, EjsHttp *hp, EjsVar *data);
static char     *prepUri(MprCtx ctx, char *uri);
static void     responseCallback(MprHttp *http, int nbytes);
static int      startRequest(Ejs *ejs, EjsHttp *hp, char *method, int argc, EjsVar **argv);

/************************************ Methods *********************************/
/*
 *  Constructor
 *
 *  function Http(uri: String = null)
 */
static EjsVar *httpConstructor(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    hp->ejs = ejs;
    hp->http = mprCreateHttp(hp);
    if (hp->http == 0) {
        ejsThrowMemoryError(ejs);
    }

    if (argc == 1 && argv[0] != ejs->nullValue) {
        hp->uri = prepUri(hp, ejsGetString(argv[0]));
    }
    hp->method = mprStrdup(hp, "GET");
    //  TODO - should have limit here
    hp->content = mprCreateBuf(hp, MPR_HTTP_BUFSIZE, -1);
    mprSetHttpCallback(hp->http, responseCallback, hp);

    return (EjsVar*) hp;
}


/*
 *  function addRequestHeader(key: String, value: String, overwrite: Boolean = true): Void
 */
EjsVar *addRequestHeader(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    char    *key, *value;
    bool    overwrite;

    mprAssert(argc >= 2);

    key = ejsGetString(argv[0]);
    value = ejsGetString(argv[1]);
    overwrite = (argc == 3) ? ejsGetBoolean(argv[2]) : 1;

    mprSetHttpHeader(hp->http, key, value, overwrite);
    return 0;
}


/*
 *  function get available(): Number
 */
EjsVar *httpAvailable(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, mprGetBufLength(hp->content));
}


/*
 *  function set callback(cb: Function): Void
 */
EjsVar *setHttpCallback(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    mprAssert(argc == 1);

    hp->callback = (EjsFunction*) argv[0];
    return 0;
}


/*
 *  function close(graceful: Boolean = true): Void
 */
static EjsVar *closeHttp(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    if (hp->http) {
        mprDisconnectHttp(hp->http);
    }
    return 0;
}


/*
 *  function connect(): Void
 */
static EjsVar *connectHttp(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    startRequest(ejs, hp, NULL, argc, argv);
    return 0;
}


/**
 *  function get certificateFile(): String
 */
static EjsVar *getCertificateFile(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    if (hp->certFile) {
        return (EjsVar*) ejsCreateString(ejs, hp->certFile);
    }
    return ejs->nullValue;
}


/*
 *  function set setCertificateFile(value: String): Void
 */
static EjsVar *setCertificateFile(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    mprFree(hp->certFile);
    hp->certFile = mprStrdup(hp, ejsGetString(argv[0]));
    return 0;
}


/*
 *  function get code(): Number
 */
static EjsVar *code(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    if (!hp->gotResponse) {
        // throwHttpError(ejs, "Http has not received a response");
        return 0;
    }
    return (EjsVar*) ejsCreateNumber(ejs, mprGetHttpCode(hp->http));
}


/*
 *  function get codeString(): String
 */
static EjsVar *codeString(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    if (!hp->gotResponse) {
        // TODO throwHttpError(ejs, "Http has not received a response");
        return 0;
    }
    return (EjsVar*) ejsCreateString(ejs, mprGetHttpMessage(hp->http));
}


/*
 *  function get contentEncoding(): String
 */
static EjsVar *contentEncoding(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    return getStringHeader(ejs, hp, "CONTENT-ENCODING");
}


/*
 *  function get contentLength(): Number
 */
static EjsVar *contentLength(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    int     length;

    if (!hp->gotResponse) {
        // throwHttpError(ejs, "Http has not received a response");
        return 0;
    }
    length = mprGetHttpContentLength(hp->http);
    return (EjsVar*) ejsCreateNumber(ejs, length);
}


/*
 *  function get contentType(): String
 */
static EjsVar *contentType(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    return getStringHeader(ejs, hp, "CONTENT-TYPE");
}


/**
 *  function get date(): Date
 */
static EjsVar *date(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    return getDateHeader(ejs, hp, "DATE");
}


/*
 *  function del(uri: String = null): Void
 */
static EjsVar *del(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    startRequest(ejs, hp, "DELETE", argc, argv);
    return 0;
}


/**
 *  function get expires(): Date
 */
static EjsVar *expires(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    return getDateHeader(ejs, hp, "EXPIRES");
}


/*
 *  function form(uri: String = null, formData: Object = null): Void
 */
static EjsVar *form(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    if (argc == 2 && argv[1] != ejs->nullValue) {
        prepForm(ejs, hp, argv[1]);
    }
    startRequest(ejs, hp, "POST", argc, argv);
    return 0;
}


/*
 *
 *  static function get followRedirects(): Boolean
 */
static EjsVar *getFollowRedirects(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateBoolean(ejs, hp->followRedirects);
}


/*
 *  function set followRedirects(flag: Boolean): Void
 */
static EjsVar *setFollowRedirects(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    hp->followRedirects = ejsGetBoolean(argv[0]);
    return 0;
}


/*
 *  function get(uri: String = null): Void
 */
static EjsVar *getHttpIterator(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    startRequest(ejs, hp, "GET", argc, argv);
    return 0;
}


/*
 *  function head(uri: String = null): Void
 */
static EjsVar *head(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    startRequest(ejs, hp, "HEAD", argc, argv);
    return 0;
}


/*
 *  function header(key: String): String
 */
static EjsVar *header(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateString(ejs, mprGetHttpHeader(hp->http, ejsGetString(argv[0])));
}


/*
 *  function get headers(): Object
 */
static EjsVar *headers(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    MprHashTable    *hash;
    MprHash         *p;
    EjsObject       *results;
    EjsName         qname;
    int             i;

    hash = mprGetHttpHeadersHash(hp->http);
    if (hash == 0) {
        return (EjsVar*) ejs->nullValue;
    }

    results = ejsCreateSimpleObject(ejs);

    for (i = 0, p = mprGetFirstHash(hash); p; p = mprGetNextHash(hash, p), i++) {
        ejsName(&qname, "", p->key);
        ejsSetPropertyByName(ejs, (EjsVar*) results, &qname, (EjsVar*) ejsCreateString(ejs, p->data));
    }
    return (EjsVar*) results;
}


/*
 *  function get isSecure(): Boolean
 */
static EjsVar *isSecure(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    if (hp->http == 0 || hp->http->sock == 0) {
        return (EjsVar*) ejs->falseValue;
    }
    return (EjsVar*) ejsCreateBoolean(ejs, mprSocketIsSecure(hp->http->sock));
}


/*
 *  function get keyFile(): String
 */
static EjsVar *getKeyFile(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    if (hp->keyFile) {
        return (EjsVar*) ejsCreateString(ejs, hp->keyFile);
    }
    return ejs->nullValue;
}


/*
 *  function set keyFile(keyFile: String): Void
 */
static EjsVar *setKeyFile(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    mprFree(hp->keyFile);
    hp->keyFile = mprStrdup(hp, ejsGetString(argv[0]));
    return 0;
}


/*
 *  function get lastModified(): Date
 */
static EjsVar *lastModified(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    return getDateHeader(ejs, hp, "LAST-MODIFIED");
}


/*
 *  function get method(): String
 */
static EjsVar *getMethod(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateString(ejs, hp->method);
}


/*
 *  function set method(value: String): Void
 */
static EjsVar *setMethod(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    cchar    *method;

    method = ejsGetString(argv[0]);
    if (strcmp(method, "DELETE") != 0 && strcmp(method, "GET") != 0 &&  strcmp(method, "HEAD") != 0 &&
            strcmp(method, "OPTIONS") != 0 && strcmp(method, "POST") != 0 && strcmp(method, "PUT") != 0 &&
            strcmp(method, "TRACE") != 0) {
        ejsThrowArgError(ejs, "Unknown HTTP method");
        return 0;
    }
    mprFree(hp->method);
    hp->method = mprStrdup(hp, ejsGetString(argv[0]));
    return 0;
}


/*
 *  function post(uri: String = null, postData: Array): Void
 */
static EjsVar *post(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    EjsByteArray    *data;
    EjsArray        *args;
    EjsNumber       *written;

    if (argc == 2 && ejsIsArray(argv[1])) {
        args = (EjsArray*) argv[1];
        if (args->length > 0) {
            data = ejsCreateByteArray(ejs, -1);
            written = ejsWriteToByteArray(ejs, data, 1, &argv[1]);
            hp->postData = (char*) data->value;
            hp->contentLength = (int) written->value;
        }
    }
    startRequest(ejs, hp, "POST", argc, argv);
    return 0;
}


/*
 *  function set postLength(length: Number): Void
 */
static EjsVar *setPostLength(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    hp->contentLength = (int) ejsGetNumber(argv[0]);
    return 0;
}



/*
 *  function put(uri: String = null, ...putData): Void
 */
static EjsVar *putToHttp(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    EjsByteArray    *data;
    EjsArray        *args;
    EjsNumber       *written;

    if (argc == 2 && argv[1] != ejs->nullValue) {
        args = (EjsArray*) argv[1];
        if (args->length > 0) {
            data = ejsCreateByteArray(ejs, -1);
            written = ejsWriteToByteArray(ejs, data, 1, &argv[1]);
            hp->postData = (char*) data->value;
            hp->contentLength = (int) written->value;
        }
    }
    startRequest(ejs, hp, "PUT", argc, argv);
    return 0;
}


/*
 *  function read(buffer: ByteArray, offset: Number = 0, count: Number = -1): Number
 */
static EjsVar *readHttpData(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    EjsByteArray    *ba;
    int             offset, count, len, contentLength;

    count = -1;
    offset = -1;

    ba = (EjsByteArray*) argv[0];
    if (argc > 1) {
        offset = ejsGetInt(argv[1]);
    }
    if (argc > 2) {
        count = ejsGetInt(argv[2]);
    }
    if (count < 0) {
        count = MAXINT;
    }
    
    if (!hp->requestStarted && startRequest(ejs, hp, NULL, 0, NULL) < 0) {
        return 0;
    }

    contentLength = mprGetBufLength(hp->content);
    len = min(contentLength - hp->readOffset, count);
    if (len > 0) {
        if (offset < 0) {
            ejsCopyToByteArray(ejs, ba, ba->writePosition, (char*) mprGetBufStart(hp->content), len);
            ejsSetByteArrayPositions(ejs, ba, -1, ba->writePosition + len);

        } else {
            ejsCopyToByteArray(ejs, ba, offset, (char*) mprGetBufEnd(hp->content), len);
        }
        mprAdjustBufStart(hp->content, len);
    }
    return (EjsVar*) ejsCreateNumber(ejs, len);
}


/*
 *  function readString(count: Number = -1): String
 *
 *  Read count bytes (default all) of content as a string. This always starts at the first character of content.
 */
static EjsVar *httpReadString(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    EjsVar  *result;
    int     count;
    
    if (!hp->requestStarted && startRequest(ejs, hp, NULL, 0, NULL) < 0) {
        return 0;
    }

    count = (argc == 1) ? ejsGetInt(argv[0]) : MAXINT;
    count = min(count, mprGetBufLength(hp->content));
    result = (EjsVar*) ejsCreateStringWithLength(ejs, mprGetBufStart(hp->content), count);

    mprAdjustBufStart(hp->content, count);
    
    return result;
}


/*
 *  function readLines(count: Number = -1): Array
 *
 *  Read count lines (default all) of content as a string. This always starts at the first line of content.
 */
static EjsVar *readLines(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    EjsArray    *results;
    EjsVar      *str;
    cchar       *data, *cp, *last;
    int         count, nextIndex;

    if (!hp->requestStarted && startRequest(ejs, hp, NULL, 0, NULL) < 0) {
        return 0;
    }

    count = (argc == 1) ? ejsGetInt(argv[0]) : MAXINT;
    results = ejsCreateArray(ejs, 0);

    if (mprGetBufLength(hp->content) > 0) {
        data = mprGetBufStart(hp->content);
        last = data;
        nextIndex = 0;

        for (cp = data; count > 0 && *cp; cp++) {
            if (*cp == '\r' || *cp == '\n') {
                if (last < cp) {
                    str = (EjsVar*) ejsCreateStringWithLength(ejs, last, (int) (cp - last));
                    ejsSetProperty(ejs, (EjsVar*) results, nextIndex++, str);
                }
                while (*cp == '\r' || *cp == '\n') {
                    cp++;
                }
                last = cp;
                if (*cp == '\0') {
                    cp--;
                }
                count--;
            }
        }
        if (last < cp) {
            str = (EjsVar*) ejsCreateStringWithLength(ejs, last, (int) (cp - last));
            ejsSetProperty(ejs, (EjsVar*) results, nextIndex++, str);
        }
        mprAdjustBufStart(hp->content, (int) (cp - data));
    }
    return (EjsVar*) results;
}


#if BLD_FEATURE_EJS_E4X
/*
 *  function readXml(): Stream
 */
static EjsVar *readXml(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    EjsXML  *xml;

    if (!hp->requestStarted && startRequest(ejs, hp, NULL, 0, NULL) < 0) {
        return 0;
    }

    xml = ejsCreateXML(ejs, 0, NULL, NULL, NULL);
    mprAddNullToBuf(hp->content);
    ejsLoadXMLString(ejs, xml, mprGetBufStart(hp->content));

    mprFlushBuf(hp->content);

    return (EjsVar*) xml;
}
#endif


/*
 *  function get responseStream(): Stream
 */
static EjsVar *responseStream(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    if (!hp->requestStarted && startRequest(ejs, hp, NULL, 0, NULL) < 0) {
        return 0;
    }
    return (EjsVar*) hp;
}



/*
 *  function get requestStream(): Stream
 */
static EjsVar *requestStream(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    //  TODO
    return 0;
}


/*
 *  function setCredentials(username: String, password: String): Void
 */
static EjsVar *setCredentials(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    mprSetHttpCredentials(hp->http, ejsGetString(argv[0]), ejsGetString(argv[1]));
    return 0;
}


/*
 *  function get timeout(): Number
 */
static EjsVar *getTimeout(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, hp->http->timeoutPeriod);
}



/*
 *  function set timeout(value: Number): Void
 */
static EjsVar *setTimeout(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    mprSetHttpTimeout(hp->http, (int) ejsGetNumber(argv[0]));
    return 0;
}



/*
 *  function get uri(): String
 */
static EjsVar *getUri(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateString(ejs, hp->uri);
}


/*
 *  function set uri(uri: String): Void
 */
static EjsVar *setUri(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    mprFree(hp->uri);
    hp->uri = prepUri(hp, ejsGetString(argv[0]));
    return 0;
}


/*
 *  Write post data to the request stream. Connection must be in async mode by defining a callback.
 *
 *  function write(data: ByteArray): Void
 */
static EjsVar *httpWrite(Ejs *ejs, EjsHttp *hp, int argc, EjsVar **argv)
{
    EjsByteArray    *data;
    EjsNumber       *written;

    if (hp->callback == 0) {
        ejsThrowIOError(ejs, "Callback must be defined to use write");
        return 0;
    }

    if (!hp->requestStarted && startRequest(ejs, hp, NULL, 0, NULL) < 0) {
        return 0;
    }
    mprAssert(hp->http->request);
    mprAssert(hp->http->sock);

    data = ejsCreateByteArray(ejs, -1);
    written = ejsWriteToByteArray(ejs, data, 1, &argv[0]);

    if (mprWriteHttpBody(hp->http, (char*) data->value, (int) written->value, 1) != (int) written->value) {
        ejsThrowIOError(ejs, "Can't write post data");
    }
    return 0;
}


/*********************************** Support **********************************/
/*
 *  Issue a request
 */
static int startRequest(Ejs *ejs, EjsHttp *hp, char *method, int argc, EjsVar **argv)
{
    int     flags;

    if (argc >= 1 && argv[0] != ejs->nullValue) {
        mprFree(hp->uri);
        hp->uri = prepUri(hp, ejsGetString(argv[0]));
    }
                                  
#if BLD_FEATURE_SSL
    if (strncmp(hp->uri, "https", 5) == 0) {
        if (!mprLoadSsl(ejs, 0)) {
            ejsThrowIOError(ejs, "Can't load SSL provider");
            return 0;
        }
    }
#endif
                                  
    if (method && strcmp(hp->method, method) != 0) {
        mprFree(hp->method);
        hp->method = mprStrdup(hp, method);
    }
    if (hp->method[0] != 'P' || hp->method[1] != 'O') {
        hp->contentLength = 0;
        hp->postData = 0;
    }

    mprFlushBuf(hp->content);

    hp->requestStarted = 1;
    hp->gotResponse = 0;


    if (hp->postData) {
        mprSetHttpBody(hp->http, hp->postData, hp->contentLength);
    }
    /*
     *  Block if a callback has been defined
     */
    flags = (hp->callback) ? MPR_HTTP_DONT_BLOCK : 0;
    if (mprHttpRequest(hp->http, hp->method, hp->uri, flags) < 0) {
        ejsThrowIOError(ejs, "Can't issue request for \"%s\"", hp->uri);
        return EJS_ERR;
    }
    return 0;
}


#if FUTURE
static EjsVar *getNumericHeader(Ejs *ejs, EjsHttp *hp, cchar *key)
{
    cchar   *value;

    if (!hp->gotResponse) {
        ejsThrowStateError(ejs, "Http has not received a response");
        return 0;
    }
    value = mprGetHttpHeader(hp->http, key);
    if (value == 0) {
        return (EjsVar*) ejs->nullValue;
    }
    return (EjsVar*) ejsCreateNumber(ejs, mprAtoi(value, 10));
}
#endif


static EjsVar *getDateHeader(Ejs *ejs, EjsHttp *hp, cchar *key)
{
    MprTime     when;
    cchar       *value;

    if (!hp->gotResponse) {
        ejsThrowStateError(ejs, "Http has not received a response");
        return 0;
    }
    value = mprGetHttpHeader(hp->http, key);
    if (value == 0) {
        return (EjsVar*) ejs->nullValue;
    }
    if (mprParseTime(ejs, &when, value) < 0) {
        value = 0;
    }
    return (EjsVar*) ejsCreateDate(ejs, when);
}


static EjsVar *getStringHeader(Ejs *ejs, EjsHttp *hp, cchar *key)
{
    cchar       *value;

    if (!hp->gotResponse) {
        ejsThrowStateError(ejs, "Http has not received a response");
        return 0;
    }
    value = mprGetHttpHeader(hp->http, key);
    if (value == 0) {
        return (EjsVar*) ejs->nullValue;
    }
    return (EjsVar*) ejsCreateString(ejs, mprGetHttpHeader(hp->http, key));
}


/*
 *  Prepare form data as a series of key-value pairs. Data is formatted according to www-url-encoded specs by mprSetHttpFormData.
 *  E.g.  name=value&address=77%20Park%20Lane
 */
static void prepForm(Ejs *ejs, EjsHttp *hp, EjsVar *data)
{
    EjsName     qname;
    EjsVar      *vp;
    EjsString   *value;
    cchar       *key, *sep;
    char        *formData;
    int         i, count, len;

    len = 0;
    formData = 0;

    count = ejsGetPropertyCount(ejs, data);
    for (i = 0; i < count; i++) {
        qname = ejsGetPropertyName(ejs, data, i);
        key = qname.name;

        vp = ejsGetProperty(ejs, data, i);
        value = ejsToString(ejs, vp);

        sep = (formData) ? "&" : "";
        len = mprReallocStrcat(hp, &formData, -1, len, 0, sep, key, "=", value->value, 0);
    }
    len = (int) strlen(formData) * 3 + 1;
    hp->postData = mprAlloc(hp, len);
    mprUrlEncode(hp->postData, len, formData);
    hp->contentLength = (int) strlen(hp->postData);

    mprSetHttpHeader(hp->http, "Content-Type", "application/x-www-form-urlencoded", 1);
}


/*
 *  Called by MprHttp on response data and request failure or completion.
 */
static void responseCallback(MprHttp *http, int nbytes)
{
    Ejs         *ejs;
    EjsHttp     *hp;
    EjsObject   *event;
    EjsType     *eventType;
    EjsName     qname;
    EjsVar      *arg;

    hp = http->callbackArg;
    hp->gotResponse = 1;
    
    if (http->response && nbytes > 0) {
        mprResetBufIfEmpty(hp->content);        
        mprPutBlockToBuf(hp->content, mprGetBufStart(http->response->content), nbytes);
    }
    
    if (hp->callback) {
        ejs = hp->ejs;
        if (http->error) {
            /*
             *  Some kind of error
             */
            eventType = ejsGetType(ejs, ES_ejs_io_HttpErrorEvent);
            arg = (EjsVar*) ejsCreateString(ejs, http->error);
        } else {
            eventType = ejsGetType(ejs, ES_ejs_io_HttpDataEvent);
            arg = (EjsVar*) ejs->nullValue;
        }
        event = (EjsObject*) ejsCreateInstance(ejs, eventType, 1, (EjsVar**) &arg);
        if (event) {
            /*
             *  Invoked as:  callback(e: Event)  where e.data == http
             */
            ejsSetPropertyByName(ejs, (EjsVar*) event, ejsName(&qname, EJS_PUBLIC_NAMESPACE, "data"), (EjsVar*) hp);
            arg = (EjsVar*) event;
            ejsRunFunction(hp->ejs, hp->callback, 0, 1, &arg);
        }
    }
}


/*
 *  Prepare a URL by adding http:// as required
 */
static char *prepUri(MprCtx ctx, char *uri) 
{
    char    *newUri;

    if (*uri == '/') {
        mprAllocSprintf(ctx, &newUri, MPR_MAX_STRING, "http://127.0.0.1%s", uri);

    } else if (strstr(uri, "http://") == 0 && strstr(uri, "https://") == 0) {
        mprAllocSprintf(ctx, &newUri, MPR_MAX_STRING, "http://%s", uri);

    } else {
        newUri = mprStrdup(ctx, uri);
    }

    return newUri;
}
    

/*********************************** Factory **********************************/

#if FUTURE
EjsHttp *ejsCreateHttp(Ejs *ejs)
{
    EjsHttp     *hp;

    hp = (EjsHttp*) ejsCreateVar(ejs, ejsGetType(ejs, ES_ejs_io_Http), 0);
    if (hp == 0) {
        return 0;
    }
    return hp;
}
#endif


int ejsCreateHttpType(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;

    type = ejsCreateCoreType(ejs, ejsName(&qname, "ejs.io", "Http"), ejs->objectType, sizeof(EjsHttp), ES_ejs_io_Http,
        ES_ejs_io_Http_NUM_CLASS_PROP, ES_ejs_io_Http_NUM_INSTANCE_PROP, 
        EJS_ATTR_NATIVE | EJS_ATTR_OBJECT | EJS_ATTR_HAS_CONSTRUCTOR | EJS_ATTR_OBJECT_HELPERS);
    if (type == 0) {
        return EJS_ERR;
    }
    return 0;
}


int ejsConfigureHttpType(Ejs *ejs)
{
    EjsType     *type;
    int         rc;

    type = ejsGetType(ejs, ES_ejs_io_Http);

    rc = 0;
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_Http, (EjsNativeFunction) httpConstructor);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_addRequestHeader, (EjsNativeFunction) addRequestHeader);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_available, (EjsNativeFunction) httpAvailable);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_set_callback, (EjsNativeFunction) setHttpCallback);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_close, (EjsNativeFunction) closeHttp);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_connect, (EjsNativeFunction) connectHttp);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_certificateFile, (EjsNativeFunction) getCertificateFile);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_set_certificateFile, (EjsNativeFunction) setCertificateFile);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_code, (EjsNativeFunction) code);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_codeString, (EjsNativeFunction) codeString);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_contentEncoding, (EjsNativeFunction) contentEncoding);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_contentLength, (EjsNativeFunction) contentLength);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_contentType, (EjsNativeFunction) contentType);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_date, (EjsNativeFunction) date);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_del, (EjsNativeFunction) del);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_expires, (EjsNativeFunction) expires);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_followRedirects, (EjsNativeFunction) getFollowRedirects);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_set_followRedirects, (EjsNativeFunction) setFollowRedirects);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_form, (EjsNativeFunction) form);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_get, (EjsNativeFunction) getHttpIterator);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_head, (EjsNativeFunction) head);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_header, (EjsNativeFunction) header);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_headers, (EjsNativeFunction) headers);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_isSecure, (EjsNativeFunction) isSecure);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_keyFile, (EjsNativeFunction) getKeyFile);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_set_keyFile, (EjsNativeFunction) setKeyFile);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_lastModified, (EjsNativeFunction) lastModified);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_method, (EjsNativeFunction) getMethod);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_set_method, (EjsNativeFunction) setMethod);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_post, (EjsNativeFunction) post);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_set_postLength, (EjsNativeFunction) setPostLength);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_put, (EjsNativeFunction) putToHttp);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_read, (EjsNativeFunction) readHttpData);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_readString, (EjsNativeFunction) httpReadString);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_readLines, (EjsNativeFunction) readLines);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_requestStream, (EjsNativeFunction) requestStream);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_responseStream, (EjsNativeFunction) responseStream);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_setCredentials, (EjsNativeFunction) setCredentials);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_timeout, (EjsNativeFunction) getTimeout);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_set_timeout, (EjsNativeFunction) setTimeout);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_uri, (EjsNativeFunction) getUri);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_set_uri, (EjsNativeFunction) setUri);
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_write, (EjsNativeFunction) httpWrite);

#if BLD_FEATURE_EJS_E4X
    rc += ejsBindMethod(ejs, type, ES_ejs_io_Http_readXml, (EjsNativeFunction) readXml);
#endif
    return rc;
}


#endif /* ES_Http */


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
