/*
 *  ejsAppweb.c -- Appweb in-memory handler for the Ejscript Web Framework.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */
/********************************** Includes **********************************/

#include    "ejs.h"

#if BLD_APPWEB_PRODUCT || BLD_FEATURE_APPWEB

 #include    "appweb.h"

/*********************************** Locals ***********************************/

#if BLD_FEATURE_MULTITHREAD
static void ejsWebLock(void *lockData);
static void ejsWebUnlock(void *lockData);
#endif

/***************************** Forward Declarations *****************************/

static void error(void *handle, int code, cchar *fmt, ...);
static int  parseUrl(MaConn *conn);
static void redirect(void *handle, int code, cchar *url);
static void setCookie(void *handle, cchar *name, cchar *value, int lifetime, cchar *path, bool secure);
static void setHeader(void *handle, bool allowMultiple, cchar *key, cchar *fmt, ...);
static int  writeBlock(void *handle, cchar *buf, int size);

/************************************* Code ***********************************/
/*
 *  When we come here, we've already matched either a location block or an extension
 */
static bool matchEjs(MaConn *conn, MaStage *handler, cchar *url)
{
    MaRequest   *req;
    MaLocation  *loc;
    EjsWeb      *web;
    cchar       *ext;
    char        urlbuf[MPR_MAX_FNAME];

    loc = conn->request->location;

#if UNUSED
    if (!(loc->flags & (MA_LOC_APP | MA_LOC_APP_DIR))) {
        return 1;
    }
#endif

    /*
     *  Send non-ejs content under web to another handler, typically the file handler.
     */
    req = conn->request;
    ext = conn->response->extension;

    if (ext && strcmp(ext, "mod") == 0) {
        maFormatBody(conn, "Bad Request", "Can't serve *.mod files");
        maFailRequest(conn, MPR_HTTP_CODE_BAD_REQUEST, "Can't server *.mod files");
        return 1;
    }

    if (parseUrl(conn) < 0) {
        return 1;
    }
    web = (EjsWeb*) conn->response->handlerData;

    /*
     *  TODO - need a more general way of handling these routes. Push back into the Ejscript framework
     */
    url = web->url;
    if (*url == '\0' || strcmp(url, "/") == 0) {
        mprSprintf(urlbuf, sizeof(urlbuf), "%s/web/index.ejs", web->appUrl);
        maSetRequestUri(conn, urlbuf);
        return 0;

    } else if (strcmp(url, "/favicon.ico") == 0) {
        mprSprintf(urlbuf, sizeof(urlbuf), "%s/web/favicon.ico", web->appUrl);
        maSetRequestUri(conn, urlbuf);
        return 0;

    } else if (strncmp(url, "/web/", 5) == 0 || *url == '\0') {
        if (!(ext && strcmp(ext, "ejs") == 0)) {
            return 0;
        }
    } else {
        if (loc->flags & (MA_LOC_APP | MA_LOC_APP_DIR) && ext && strcmp(ext, "ejs") == 0) {
            maFormatBody(conn, "Bad Request", "Can't serve *.ejs files outside web directory");
            maFailRequest(conn, MPR_HTTP_CODE_BAD_REQUEST, "Can't server *.ejs files outside web directory");
        }
    }
    return 1;
}


static int parseUrl(MaConn *conn)
{
    EjsWeb          *web;
    EjsWebControl   *control;
    MaRequest       *req;
    MaResponse      *resp;
    MaAlias         *alias;
    MaLocation      *location;
    char            *baseDir, *cp, *url, *baseUrl;
    int             flags, locFlags;

    resp = conn->response;
    req = conn->request;
    alias = req->alias;
    location = req->location;
    locFlags = location->flags;
    url = req->url;
    
    flags = 0;
    if (locFlags & MA_LOC_APP_DIR) {
        flags |= EJS_WEB_FLAG_APP;
        mprAllocSprintf(resp, &baseDir, -1, "%s%s", alias->filename, &req->url[alias->prefixLen]);
        if ((cp = strchr(&baseDir[strlen(alias->filename) + 1], '/')) != 0) {
            *cp = '\0';
        }
        mprAllocSprintf(resp, &baseUrl, -1, "%s%s", alias->prefix, &req->url[alias->prefixLen]);
        if ((cp = strchr(&baseUrl[alias->prefixLen + 1], '/')) != 0) {
            *cp = '\0';
        }
        if (*url) {
            /* Step over the directory and app name */
            while (*++url != '/') ;
            if (*url) {
                while (*++url != '/') ;
            }
        }
            
    } else {
        if (locFlags & MA_LOC_APP) {
            flags |= EJS_WEB_FLAG_APP;
        }
        baseDir = alias->filename;
        if (alias->prefixLen > 0) {
            /* Step over the application name (same as alias prefix) */
            url = &url[alias->prefixLen];
            if (*url != '/' && url[-1] == '/') {
                url--;
            }
        }
        baseUrl = alias->prefix;
    }
    
    if (location->flags & MA_LOC_BROWSER) {
        flags |= EJS_WEB_FLAG_BROWSER_ERRORS;
    }
    if (location->flags & MA_LOC_AUTO_SESSION) {
        flags |= EJS_WEB_FLAG_SESSION;
    }

    control = conn->http->ejsHandler->stageData;
    web = ejsCreateWebRequest(req, control, conn, baseUrl, url, baseDir, flags);
    if (web == 0) {
        maFailRequest(conn, MPR_HTTP_CODE_INTERNAL_SERVER_ERROR, "Can't create Ejs web object for %s", req->url);
        return EJS_ERR;
    }
    resp->handlerData = web;

    return 0;
}


/*
 *  Run the Ejscript request. The routine runs when all input data has been received.
 */
static void runEjs(MaQueue *q)
{
    MaConn      *conn;
    MaRequest   *req;
    EjsWeb      *web;
    char        msg[MPR_MAX_STRING];

    conn = q->conn;
    req = conn->request;
    web = q->queueData = conn->response->handlerData;

    maSetHeader(conn, 0, "Last-Modified", req->host->currentDate);
    maDontCacheResponse(conn);

    maPutForService(q, maCreateHeaderPacket(conn), 0);

    if (ejsRunWebRequest(web) < 0) {
        //  TODO - refactor. Want request failed to have an option which says send this output to the client also.
        if (web->flags & EJS_WEB_FLAG_BROWSER_ERRORS) {
            //  TODO - this API should allocate a buffer and not use a static buffer
            mprEscapeHtml(msg, sizeof(msg), web->error);
            maFormatBody(conn, "Request Failed", 
                "<h1>Ejscript error for \"%s\"</h1>\r\n<h2>%s</h2>\r\n"
                "<p>To prevent errors being displayed in the browser, "
                "use <b>\"EjsErrors log\"</b> in the config file.</p>\r\n",
            web->url, web->error);
        }
        maFailRequest(conn, MPR_HTTP_CODE_BAD_GATEWAY, web->error);
    }
    maPutForService(q, maCreateEndPacket(conn), 1);
}


/****************************** Control Callbacks ****************************/
/*
 *  Define params[]
 */
static void defineParams(void *handle)
{
    Ejs             *ejs;
    MaConn          *conn;
    MprHashTable    *formVars;
    MprHash         *hp;

    conn = (MaConn*) handle;
    formVars = conn->request->formVars;
    ejs = ((EjsWeb*) maGetHandlerQueueData(conn))->ejs;

    hp = mprGetFirstHash(formVars);
    while (hp) {
        ejsDefineWebParam(ejs, hp->key, hp->data);
        hp = mprGetNextHash(formVars, hp);
    }
}


static void discardOutput(void *handle)
{
    MaConn      *conn;

    conn = (MaConn*) handle;
    maDiscardData(conn->response->queue[MA_QUEUE_SEND].nextQ->nextQ, 0);
}


static void error(void *handle, int code, cchar *fmt, ...)
{
    va_list     args;
    char        *msg;

    va_start(args, fmt);
    mprAllocVsprintf(handle, &msg, -1, fmt, args);

    maFailRequest(handle, code, "%s", msg);
}


static EjsVar *createString(Ejs *ejs, cchar *value)
{
    if (value == 0) {
        return ejs->nullValue;
    }
    return (EjsVar*) ejsCreateString(ejs, value);
}


static EjsVar *createHeaders(Ejs *ejs, MprHashTable *table)
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


static EjsVar *getRequestVar(void *handle, int field)
{
    Ejs         *ejs;
    EjsWeb      *web;
    MaConn      *conn;
    MaRequest   *req;

    conn = handle;
    req = conn->request;
    ejs = ((EjsWeb*) maGetHandlerQueueData(conn))->ejs;

    switch (field) {
    case ES_ejs_web_Request_accept:
        return createString(ejs, req->accept);

    case ES_ejs_web_Request_acceptCharset:
        return createString(ejs, req->acceptCharset);

    case ES_ejs_web_Request_acceptEncoding:
        return createString(ejs, req->acceptEncoding);

    case ES_ejs_web_Request_authAcl:
    case ES_ejs_web_Request_authGroup:
    case ES_ejs_web_Request_authUser:
        //  TODO 
        return (EjsVar*) ejs->undefinedValue;

    case ES_ejs_web_Request_authType:
        return createString(ejs, req->authType);

    case ES_ejs_web_Request_connection:
        return createString(ejs, req->connection);

    case ES_ejs_web_Request_contentLength:
        return (EjsVar*) ejsCreateNumber(ejs, req->length);

    case ES_ejs_web_Request_cookies:
        if (req->cookie) {
            return (EjsVar*) ejsCreateCookies(ejs);
        } else {
            return ejs->nullValue;
        }
        break;

    case ES_ejs_web_Request_extension:
        return createString(ejs, req->parsedUri->ext);

    case ES_ejs_web_Request_files:
        return (EjsVar*) ejs->undefinedValue;

    case ES_ejs_web_Request_headers:
        return (EjsVar*) createHeaders(ejs, conn->request->headers);

    case ES_ejs_web_Request_hostName:
        return createString(ejs, req->hostName);

    case ES_ejs_web_Request_method:
        return createString(ejs, req->methodName);

    case ES_ejs_web_Request_mimeType:
        return createString(ejs, req->mimeType);

    case ES_ejs_web_Request_pathInfo:
        return createString(ejs, req->pathInfo);

    case ES_ejs_web_Request_pathTranslated:
        return createString(ejs, req->pathTranslated);

    case ES_ejs_web_Request_pragma:
        return createString(ejs, req->pragma);

    case ES_ejs_web_Request_query:
        return createString(ejs, req->parsedUri->query);

    case ES_ejs_web_Request_originalUri:
        return createString(ejs, req->parsedUri->originalUri);

    case ES_ejs_web_Request_referrer:
        return createString(ejs, req->referer);

    case ES_ejs_web_Request_remoteAddress:
        return createString(ejs, conn->remoteIpAddr);

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
        return createString(ejs, conn->remoteIpAddr);

    case ES_ejs_web_Request_sessionID:
        web = ejs->handle;
        return createString(ejs, web->session ? web->session->id : "");

    case ES_ejs_web_Request_url:
        return createString(ejs, req->url);

    case ES_ejs_web_Request_userAgent:
        return createString(ejs, req->userAgent);
    }

    ejsThrowOutOfBoundsError(ejs, "Bad property slot reference");
    return 0;
}


static EjsVar *getHostVar(void *handle, int field)
{
    Ejs         *ejs;
    MaConn      *conn;
    MaHost      *host;
    EjsWeb      *web;

    conn = handle;
    host = conn->host;
    ejs = ((EjsWeb*) maGetHandlerQueueData(conn))->ejs;

    switch (field) {
    case ES_ejs_web_Host_documentRoot:
        return createString(ejs, host->documentRoot);

    case ES_ejs_web_Host_name:
        return createString(ejs, host->name);

    case ES_ejs_web_Host_protocol:
        return createString(ejs, host->secure ? "https" : "http");

    case ES_ejs_web_Host_isVirtualHost:
        return (EjsVar*) ejsCreateBoolean(ejs, host->flags & MA_HOST_VHOST);

    case ES_ejs_web_Host_isNamedVirtualHost:
        return (EjsVar*) ejsCreateBoolean(ejs, host->flags & MA_HOST_NAMED_VHOST);

    case ES_ejs_web_Host_software:
        return createString(ejs, MA_SERVER_NAME);

    case ES_ejs_web_Host_logErrors:
        web = ejs->handle;
        return (EjsVar*) ((web->flags & EJS_WEB_FLAG_BROWSER_ERRORS) ? ejs->falseValue : ejs->trueValue);
    }

    ejsThrowOutOfBoundsError(ejs, "Bad property slot reference");
    return 0;
}


static EjsVar *getResponseVar(void *handle, int field)
{
    Ejs         *ejs;
    MaConn      *conn;
    MaResponse  *resp;

    conn = handle;
    resp = conn->response;
    ejs = ((EjsWeb*) maGetHandlerQueueData(conn))->ejs;

    switch (field) {
    case ES_ejs_web_Response_code:
        return (EjsVar*) ejsCreateNumber(ejs, resp->code);

    case ES_ejs_web_Response_filename:
        return (EjsVar*) createString(ejs, resp->filename);

    case ES_ejs_web_Response_headers:
        return (EjsVar*) createHeaders(ejs, conn->response->headers);

    case ES_ejs_web_Response_mimeType:
        return (EjsVar*) createString(ejs, resp->mimeType);

    default:
        ejsThrowOutOfBoundsError(ejs, "Bad property slot reference");
        return 0;
    }
}


static cchar *getHeader(void *handle, cchar *key)
{
    MaRequest   *req;
    MaConn      *conn;

    conn = handle;
    req = conn->request;
    return (cchar*) mprLookupHash(req->headers, key);
}


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


static void redirect(void *handle, int code, cchar *url)
{
    maRedirect(handle, code, url);
}


static void setCookie(void *handle, cchar *name, cchar *value, int lifetime, cchar *path, bool secure)
{
    maSetCookie(handle, name, value, lifetime, path, secure);
}


static void setHeader(void *handle, bool allowMultiple, cchar *key, cchar *fmt, ...)
{
    char        *value;
    va_list     vargs;

    va_start(vargs, fmt);
    mprAllocVsprintf(handle, &value, -1, fmt, vargs);
    maSetHeader(handle, allowMultiple, key, "%s", value);
}


static void setHttpCode(void *handle, int code)
{
    maSetResponseCode(handle, code);
}


static void setMimeType(void *handle, cchar *mimeType)
{
    maSetResponseMimeType(handle, mimeType);
}


static int writeBlock(void *handle, cchar *buf, int size)
{
    MaConn      *conn;
    MaQueue     *q;

    conn = (MaConn*) handle;

    /*
     *  We write to the service queue of the handler
     */
    q = conn->response->queue[MA_QUEUE_SEND].nextQ;
    mprAssert(q->stage->flags & MA_STAGE_HANDLER);

    return maWriteBlock(q, buf, size, 1);
}


#if BLD_FEATURE_CONFIG_PARSE
static int parseEjs(MaHttp *http, cchar *key, char *value, MaConfigState *state)
{
    MaLocation      *location;
    MaServer        *server;
    MaHost          *host;
    char            *prefix, *path;
    int             flags;
    
    host = state->host;
    server = state->server;
    location = state->location;
    
    flags = location->flags & (MA_LOC_BROWSER | MA_LOC_AUTO_SESSION);

#if UNUSED
    MaStage         *ejsHandler;
    EjsWebControl   *web;
    if (mprStrcmpAnyCase(key, "Ejs") == 0) {
        path = mprStrTrim(value, "\"");
        mprCleanFilename(http, path);
        if (!mprAccess(http, path, X_OK)) {
            mprError(http, "Can't access Ejs path %s", path);
            return MPR_ERR_BAD_SYNTAX;
        }
        if ((ejsHandler = maLookupStage(http, "ejsHandler")) == 0) {
            mprError(http, "Ejscript module is not loaded");
            return MPR_ERR_BAD_SYNTAX;
        }
        web = (EjsWebControl*) ejsHandler->stageData;
        web->ejsLibDir = path;

    } else 
#endif
    if (mprStrcmpAnyCase(key, "EjsApp") == 0) {
        if (mprStrcmpAnyCase(value, "on") == 0) {
            location->flags |= MA_LOC_APP;
        } else {
            location->flags &= ~MA_LOC_APP;
        }
        return 1;

    } else if (mprStrcmpAnyCase(key, "EjsAppDir") == 0) {
        if (mprStrcmpAnyCase(value, "on") == 0) {
            location->flags |= MA_LOC_APP_DIR;
        } else {
            location->flags &= ~MA_LOC_APP_DIR;
        }
        return 1;

    } else if (mprStrcmpAnyCase(key, "EjsAppAlias") == 0) {
        if (maSplitConfigValue(server, &prefix, &path, value, 1) < 0 || path == 0 || prefix == 0) {
            return MPR_ERR_BAD_SYNTAX;
        }
        location = maCreateLocationAlias(http, state, prefix, path, "ejsHandler", MA_LOC_APP | flags);
        if (location == 0) {
            return MPR_ERR_BAD_SYNTAX;
        }
        return 1;

    } else if (mprStrcmpAnyCase(key, "EjsAppDirAlias") == 0) {
        if (maSplitConfigValue(server, &prefix, &path, value, 1) < 0 || path == 0 || prefix == 0) {
            return MPR_ERR_BAD_SYNTAX;
        }
        location = maCreateLocationAlias(http, state, prefix, path, "ejsHandler", MA_LOC_APP_DIR | flags);
        if (location == 0) {
            return MPR_ERR_BAD_SYNTAX;
        }
        return 1;

    } else if (mprStrcmpAnyCase(key, "EjsErrors") == 0) {
        if (mprStrcmpAnyCase(value, "browser") == 0) {
            location->flags |= MA_LOC_BROWSER;
        } else {
            location->flags &= ~MA_LOC_BROWSER;
        }
        return 1;

    } else if (mprStrcmpAnyCase(key, "EjsSessionTimeout") == 0) {
        if (value == 0) {
            return MPR_ERR_BAD_SYNTAX;
        }
        if (! mprGetDebugMode(http)) {
            location->sessionTimeout = atoi(mprStrTrim(value, "\""));
        }
        return 1;

    } else if (mprStrcmpAnyCase(key, "EjsSession") == 0) {
        if (mprStrcmpAnyCase(value, "on") == 0) {
            location->flags |= MA_LOC_AUTO_SESSION;
        } else {
            location->flags &= ~MA_LOC_AUTO_SESSION;
        }
        return 1;
    }

    return 0;
}
#endif


#if BLD_FEATURE_MULTITHREAD && TODO
static void ejsWebLock(void *lockData)
{
    MprMutex    *mutex = (MprMutex*) lockData;

    mprAssert(mutex);
    mutex->lock();
}


static void ejsWebUnlock(void *lockData)
{
    MprMutex    *mutex = (MprMutex*) lockData;

    mprAssert(mutex);

    mutex->unlock();
}
#endif /* BLD_FEATURE_MULTITHREAD */


/*
 *  Dynamic module initialization
 */
MprModule *maEjsHandlerInit(MaHttp *http, cchar *path)
{
    MprModule       *module;
    MaStage         *handler;
    EjsWebControl   *control;

    module = mprCreateModule(http, "ejsHandler", BLD_VERSION, 0, 0, 0);
    if (module == 0) {
        return 0;
    }

    handler = maCreateHandler(http, "ejsHandler", 
        MA_STAGE_GET | MA_STAGE_HEAD | MA_STAGE_POST | MA_STAGE_PUT | MA_STAGE_FORM_VARS | MA_STAGE_VIRTUAL);
    if (handler == 0) {
        mprFree(module);
        return 0;
    }
    http->ejsHandler = handler;
    handler->match = matchEjs;
    handler->run = runEjs;
    handler->parse = parseEjs;

    /*
     *  Setup the control block
     */
    handler->stageData = control = mprAllocObjZeroed(handler, EjsWebControl);

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
    control->write = writeBlock;
    control->modulePath = mprStrdup(control, path);

#if BLD_FEATURE_MULTITHREAD && FUTURE
    /*
     *  This mutex is used very sparingly and must be an application global lock.
     */
    mutex = mprCreateLock(control);
    control->lock = ejsWebLock;
    control->unlock = ejsWebUnlock;
    control->lockData = mutex;
#endif

    if (ejsOpenWebFramework(control, 1) < 0) {
        return 0;
    }
    return module;
}


#else
void __ejsAppwebDummy() {}
#endif /* BLD_FEATURE_EJS */


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
