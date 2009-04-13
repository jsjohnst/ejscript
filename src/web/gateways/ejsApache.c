/**
 *  ejsApache.c - Apache Handler
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejs.h"

#if BLD_FEATURE_APACHE

/*
 *  Prevent normal auto-compilation. Must compile via apxs. See Makefile.
 */
#if ENABLE_COMPILE
 #include    "httpd.h"
 #include    "http_config.h"

#if 0
 #include    "http_core.h"
 #include    "http_log.h"
 #include    "http_protocol.h"
 #include    "ap_config.h"
#endif

#if BLD_FEATURE_EJS_WEB
/*********************************** Locals ***********************************/

#if BLD_FEATURE_MULTITHREAD && TODO
static void ejsWebLock(void *lockData);
static void ejsWebUnlock(void *lockData);
#endif

/***************************** Forward Declarations *****************************/

static void error(void *handle, int code, cchar *msg);
static void redirect(void *handle, int code, cchar *url);
static void setCookie(void *handle, cchar *name, cchar *value, int lifetime, cchar *path, bool secure);
static void setHeader(void *handle, bool allowMultiple, cchar *key, cchar *value);
static int  writeBlock(void *handle, cchar *buf, int size);

module AP_MODULE_DECLARE_DATA ejsModule;

typedef struct {
} EjsDirConfig;

typedef struct {
} EjsServerConfig;

static EjsConfig getDir(const request_rec *r) {
    return ap_get_module_config(r->per_dir_config, &ejsModule);
}
static EjsConfig getServer(const server_rec *s) {
    return ap_get_module_config(s->module_config, &ejsModule);
}
static EjsConfig getReq(const request_rec *r) {
    return ap_get_module_config(r->request_config, &ejsModule);
}

/************************************* Code ***********************************/
/*
 *  When we come here, we've already matched either a location block or an extension
 */
static int run(request_rec *r) 
{
    EjsDirConfig        *dir;
    EjsServerConfig     *server;
    cchar               *ext;

    MaRequest       *req;
    MaResponse      *resp;
    MaConn          *conn;
    MaAlias         *alias;
    MaLocation      *location;
    EjsWeb          *web;
    cchar           *sep, *prefix;
    char            *urlBase, *dir, *app, *url, *cp;
    int             flags, locFlags;

    if (!r->handler || strcasecmp(r->handler, "ejs") != 0) {
        return DECLINED;
    }

    dir = getDir(r)
    server = getServer(r->XXX);

    //  EjsAlias should probably be creating a directory block. These flags should probably be in a directory
    if (loc->flags & (MA_LOC_APP | MA_LOC_APP_DIR)) {

        /*
         *  Send non-ejs content under web to another handler, typically the file handler.
         */
        if (strncmp(&req->url[loc->prefixLen], "web/", 4) == 0) {
            if (!(ext && strcmp(ext, "ejs") == 0)) {
                return DECLINED;
            }

        } else {
            if (ext && strcmp(ext, "ejs") == 0) {
                maFormatBody(conn, "Bad Request", "Can't serve *.ejs files outside web directory");
                maFailRequest(conn, MPR_HTTP_CODE_BAD_REQUEST, "Can't server *.ejs files outside web directory");
                return HTTP_XXX;
            }
        }
    }

    flags = 0;
    url = 0;

    locFlags = location->flags;
    
    if (locFlags & MA_LOC_APP) {
        app = mprStrTrim(mprStrdup(q, prefix), "/");
        url = &req->url[alias->prefixLen];
        dir = mprStrdup(resp, alias->filename);
        if (*url != '/') {
            url--;
        }
        urlBase = mprStrdup(resp, prefix);
        
    } else if (locFlags & MA_LOC_APP_DIR) {
        url = &req->url[alias->prefixLen];
        app = mprStrdup(resp, url);
        if ((cp = strchr(app, '/')) != 0) {
            url = mprStrdup(resp, cp);
            *cp = '\0';
        }
        sep = prefix[strlen(prefix) - 1] == '/' ? "" : "/";
        mprAllocStrcat(resp, &dir, -1, NULL, alias->filename, sep, app, NULL);
        mprAllocStrcat(resp, &urlBase, -1, NULL, prefix, sep, app, NULL);

    } else {
        app = 0;
        dir = mprStrdup(resp, alias->filename);
        url = &req->url[alias->prefixLen];
        flags |= EJS_WEB_FLAG_SOLO;
        if (*url != '/') {
            url--;
        }        
        urlBase = mprStrdup(resp, prefix);
    }
    mprStrTrim(urlBase, "/");
    mprStrTrim(dir, "/");
    
    if (location->flags & MA_LOC_BROWSER) {
        flags |= EJS_WEB_FLAG_BROWSER_ERRORS;
    }
    if (location->flags & MA_LOC_AUTO_SESSION) {
        flags |= EJS_WEB_FLAG_SESSION;
    }

    //  TODO - why have extension here?
    /*
     *  Var         Stand-Alone             App                         AppDir
     *  app         0                       carmen                      carmen
     *  dir         /Users/mob/....         /Users/mob/hg/carmen        /Users/mob/hg/carmen
     *  urlBase                             /xg/carmen                  /carmen
     *  url                                 stock                       stock
     */
    web = ejsCreateWebRequest(req, q->stage->stageData, conn, app, dir, urlBase, url, req->cookie, flags);
    if (web == 0) {
        maFailRequest(conn, MPR_HTTP_CODE_INTERNAL_SERVER_ERROR, "Can't create Ejs web object for %s", req->url);
        return;
    }
    q->queueData = web;
    maSetHeader(conn, 0, "Last-Modified", req->host->currentDate);
    maDontCacheResponse(conn);

    if (r->method_number != M_GET) {
        //  TODO - other methods
        return HTTP_METHOD_NOT_ALLOWED;
    }

    if (ejsProcessWebRequest((EjsWeb*) r, &msg) < 0) {
        if (web->flags & EJS_WEB_FLAG_BROWSER_ERRORS) {
            maFormatBody(conn, "Request Failed", "%s", msg);
        }
        maFailRequest(conn, MPR_HTTP_CODE_BAD_GATEWAY, msg);
        mprFree(msg);
    }

    ap_set_content_type(r, "text/html");
    ap_rputs("<html><title>Hello World!</title><body><p>Hello World</p></body></html>\r\n", r);

#if 0
    if ((err = set_content_length(r, r->finfo.st_stize)) || (err = set_last_modified(r, r->finfo.st_mtime)))
        return err;
    if (r->finof.st_mode == 0) 
        return NOT_FOUND;
    fopen(r->filename, "r");

    if (!r->main) {
        /* Not internal redirect */
        apr_table_set(r->headers_out, "X-ejs", "Under construction");
    }
    register_timeout("send", r);
    send_http_header(r);
    if (!r->header_only)
        send_fd(f, r);
    pfclose(r->pool, f);
#endif

    return OK;
}


/****************************** Control Callbacks ****************************/
/*
 *  Define form variables using any query data
 */
static void defineFormVars(void *handle)
{
    MprHash     *hp;
    MaConn      *conn;
    MaRequest   *req;
    Ejs         *ejs;

    conn = (MaConn*) handle;
    req = conn->request;
    mprAssert(req->formVars);

    ejs = ((EjsWeb*) maGetHandlerQueueData(conn))->ejs;

    hp = mprGetFirstHash(req->formVars);
    while (hp) {
        ejsDefineWebParam(ejs, hp->key, hp->data);
        hp = mprGetNextHash(req->formVars, hp);
    }
}


static void discardOutput(void *handle)
{
    MaConn      *conn;

    conn = (MaConn*) handle;
    maDiscardData(conn->response->queue[MA_QUEUE_SEND].nextQ->nextQ, 0);
}


static void error(void *handle, int code, cchar *msg)
{
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
        //  TODO 
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


static void setHeader(void *handle, bool allowMultiple, cchar *key, cchar *value)
{
    maSetHeader(handle, allowMultiple, key, value);
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
MprModule *mprEjsHandlerInit(MaHttp *http)
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
    handler->open = openEjs;
    handler->run = runEjs;
    handler->parse = parseEjs;

    /*
     *  Setup the control block
     */
    handler->stageData = control = mprAllocObjZeroed(handler, EjsWebControl);

    control->defineFormVars = defineFormVars;
    control->discardOutput = discardOutput;
    control->error = error;
    control->getVar = getVar;
    control->redirect = redirect;
    control->setCookie = setCookie;
    control->setHeader = setHeader;
    control->setHttpCode = setHttpCode;
    control->setMimeType = setMimeType;
    control->write = writeBlock;

#if BLD_FEATURE_MULTITHREAD && FUTURE
    /*
     *  This mutex is used very sparingly and must be an application global lock.
     */
    mutex = mprCreateLock(control);
    control->lock = ejsWebLock;
    control->unlock = ejsWebUnlock;
    control->lockData = mutex;
#endif

    if (ejsOpenWebService(control) < 0) {
        return 0;
    }
    return module;
}


static const char *ejsAlias(cmd_parms *cmd, void *dummy, int arg)
{
    EjsConfig *config = getServer(cmd->server);

    config->enabled = arg;
    return NULL;
}


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


static const command_rec ejsCmds[] =
{
    //  TODO - put EjsAlias stuff here
    AP_INIT_FLAG("EjsApp", ejsAlias, NULL, RSRC_CONF, "Run an echo server on this host"),
    AP_INIT_FLAG("EjsAppDir", ejsAlias, NULL, RSRC_CONF, "Run an echo server on this host"),
    AP_INIT_FLAG("EjsAlias", ejsAlias, NULL, RSRC_CONF, "Run an echo server on this host"),
    AP_INIT_FLAG("EjsAppDirAlias", ejsAlias, NULL, RSRC_CONF, "Run an echo server on this host"),
    AP_INIT_FLAG("EjsErrors", ejsAlias, NULL, RSRC_CONF, "Run an echo server on this host"),
    AP_INIT_FLAG("EjsSession", ejsAlias, NULL, RSRC_CONF, "Run an echo server on this host"),
    AP_INIT_FLAG("EjsSessionTimeout", ejsAlias, NULL, RSRC_CONF, "Run an echo server on this host"),
    { NULL }
};

static void registerHooks(apr_pool_t *pool) {
    ap_hook_pre_config(preConfig, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_post_config(postConfig, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_handler(run, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_child_init(childInit, NULL, NULL, APR_HOOK_MIDDLE);
}

module AP_MODULE_DECLARE_DATA ejsModule = {
    STANDARD20_MODULE_STUFF,
    createDirConfig,
    mergeDirConfig,
    createServerConfig,
    mergeServerConfig,
    ejsCmds,
    registerHooks
};

#endif /* BLD_FEATURE_EJS_WEB */
#endif /* ENABLE_COMPILE */
#endif /* BLD_FEATURE_APACHE */


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
