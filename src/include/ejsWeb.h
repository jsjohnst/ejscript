/**
 *  ejsWeb.h -- Header for the Ejscript Web Framework
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#ifndef _h_EJS_WEB_h
#define _h_EJS_WEB_h 1

#if BLD_FEATURE_EJS_WEB
/*
 *  Indent to protect from genDepend first time.
 */
 #include    "ejs.web.slots.h"

/*********************************** Defines **********************************/

#if BLD_APPWEB_PRODUCT
    #define EJS_EJSWEB          "ajsweb"    /* Ejscript framework generator */
#else
    #define EJS_EJSWEB          "ejsweb"    /* Ejscript framework generator */
#endif

#define EJS_EJSWEB_EXE          EJS_EJSWEB BLD_EXE
#define EJS_WEB_EXT             ".ejs"      /* Web page extension */
#define EJS_SESSION             "-ejs-session-"
#define EJS_SERVER_NAME         "Embedthis-Ejscript/" BLD_VERSION

#define EJS_MAX_HEADERS         2048        /* Max size of the headers */

/*
 *  Var collections
 */
#define EJS_WEB_HOST_VAR        3           /* Fields of the Host object */
#define EJS_WEB_REQUEST_VAR     1           /* Fields of the Request object */
#define EJS_WEB_RESPONSE_VAR    2           /* Fields of the Response object */

/*********************************** Types ************************************/
/*
 *  Service control block. This defines the function callbacks for a web server module to implement.
 *  Aall these functions as required to interact with the web server.
 */
typedef struct EjsWebControl {
    EjsService  *service;                   /* EJS service */
    Ejs         *master;                    /* Master interpreter */
    EjsVar      *applications;              /* Application cache */
    EjsObject   *sessions;                  /* Session cache */
    cchar       *modulePath;                /* Path to the ejs web server module and handler */
    int         sessionTimeout;             /* Default session timeout */
    int         nextSession;                /* Session ID counter */

    void        (*defineParams)(void *handle);
    void        (*discardOutput)(void *handle);
    void        (*error)(void *handle, int code, cchar *fmt, ...);
    cchar       *(*getHeader)(void *handle, cchar *key);
    EjsVar      *(*getVar)(void *handle, int collection, int field);
    void        (*redirect)(void *handle, int code, cchar *url);
    void        (*setCookie)(void *handle, cchar *name, cchar *value, int lifetime, cchar *path, bool secure);
    void        (*setHeader)(void *handle, bool allowMultiple, cchar *key, cchar *fmt, ...);
    void        (*setHttpCode)(void *handle, int code);
    void        (*setMimeType)(void *handle, cchar *mimeType);
    int         (*setVar)(void *handle, int collection, int field, EjsVar *value);
    int         (*write)(void *handle, cchar *buf, int size);

#if BLD_FEATURE_MULTITHREAD
    void        (*lock)(void *lockData);
    void        (*unlock)(void *lockData);
    void        *lockData;
#endif

} EjsWebControl;


/*
 *  Flags for EjsWeb.flags
 *  TODO - remove _FLAG
 */
#define EJS_WEB_FLAG_BROWSER_ERRORS      0x1    /* Send errors to the browser (typical dev use) */
#define EJS_WEB_FLAG_SESSION             0x2    /* Auto create sessions for each request */
#define EJS_WEB_FLAG_APP                 0x4    /* Request for content inside an Ejscript Application*/
#define EJS_WEB_FLAG_SOLO                0x8    /* Solo ejs file */

/*
 *  Per request control block
 */
typedef struct EjsWeb {
    Ejs             *ejs;           /* Ejscript interpreter handle */
    cchar           *appDir;        /* Directory containing the application */
    cchar           *appUrl;        /* Base url for the application. No trailing "/" */
    void            *handle;        /* Web server connection/request handle */
    EjsWebControl   *control;       /* Pointer to control block */
    cchar           *url;           /* App relative url: /controller/action. Starts with "/"  */
    int             flags;          /* Control flags */

    char            *controllerFile;/* Simple controller file name without path */
    EjsName         controllerName; /* Qualified Controller name (with "Controller" suffix) */
    EjsName         doActionName;   /* Qualified do action function name */
    char            *viewName;      /* Name of the view function */

    EjsVar          *params;        /* Form variables */
    EjsVar          *cookies;       /* Parsed cookies cache */
    struct EjsWebSession *session;  /* Current session */

    char            *error;         /* Error message */
    char            *cookie;        /* Cookie header */
    EjsType         *appType;       /* Application type instance */
    EjsName         appName;        /* Qualified Application name */

    EjsType         *controllerType;/* Controller type instance */
    EjsVar          *controller;    /* Controller instance to run */
    EjsVar          *doAction;      /* doAction() function to run. May be renderView() for Stand-Alone views. */

} EjsWeb;


/*
 *  Parse context
 */
typedef struct EjsWebParse {
    char    *inBuf;                 /* Input data to parse */
    char    *inp;                   /* Next character for input */
    char    *endp;                  /* End of storage (allow for null) */
    char    *tokp;                  /* Pointer to current parsed token */
    char    *token;                 /* Storage buffer for token */
    int     tokLen;                 /* Length of buffer */
} EjsWebParse;


/*
 *  Host class
 */
typedef struct EjsWebHost
{
    EjsVar      var;
    void        *handle;
} EjsWebHost;


/*
 *  Request class
 */
typedef struct EjsWebRequest
{
    EjsVar      var;
    void        *handle;
} EjsWebRequest;



/*
 *  Response class
 */
typedef struct EjsWebResponse
{
    EjsVar      var;
    void        *handle;
} EjsWebResponse;


typedef struct EjsWebSession
{
    EjsObject   obj;
    MprTime     expire;                     /* When the session should expire */
    cchar       *id;                        /* Session ID */
    int         timeout;                    /* Session inactivity lifespan */
} EjsWebSession;


/******************************* Internal APIs ********************************/

extern void         ejsConfigureWebTypes(Ejs *ejs);
extern int          ejsOpenWebFramework(EjsWebControl *control, bool useMaster);

//DDD
extern Ejs          *ejsGetMaster(MprCtx ctx);

extern void         ejsConfigureWebRequestType(Ejs *ejs);
extern void         ejsConfigureWebResponseType(Ejs *ejs);
extern void         ejsConfigureWebHostType(Ejs *ejs);
extern void         ejsConfigureWebControllerType(Ejs *ejs);
extern void         ejsConfigureWebSessionType(Ejs *ejs);

//DDD
extern EjsWeb       *ejsCreateWebRequest(MprCtx ctx, EjsWebControl *control, void *req, cchar *scriptName, cchar *uri,
                        cchar *dir, int flags);
extern int          ejsRunWebRequest(EjsWeb *web);
//DDD
extern EjsWebRequest *ejsCreateWebRequestObject(Ejs *ejs, void *handle);
//DDD
extern EjsWebHost   *ejsCreateWebHostObject(Ejs *ejs, void *handle);
//DDD
extern EjsWebResponse *ejsCreateWebResponseObject(Ejs *ejs, void *handle);
//DDD
extern EjsWebSession *ejsCreateWebSessionObject(Ejs *ejs, void *handle);

extern void         ejsDefineWebParam(Ejs *ejs, cchar *key, cchar *value);

//DDD
extern int          ejsLoadView(Ejs *ejs);
//DDD
extern void         ejsParseWebSessionCookie(EjsWeb *web);

/******************************** Published API *******************************/
#ifdef  __cplusplus
extern "C" {
#endif

//DDD -- All these
extern EjsWebSession *ejsCreateSession(Ejs *ejs, int timeout, bool secure);
extern bool         ejsDestroySession(Ejs *ejs);
extern void         ejsDefineParams(Ejs *ejs);
extern void         ejsDiscardOutput(Ejs *ejs);
extern EjsVar       *ejsCreateCookies(Ejs *ejs);
extern cchar        *ejsGetHeader(Ejs *ejs, cchar *key);
extern EjsVar       *ejsGetWebVar(Ejs *ejs, int collection, int field);
extern int          ejsMapToStorage(Ejs *ejs, char *path, int pathsize, cchar *uri);
extern int          ejsReadFile(Ejs *ejs, char **buf, int *len, cchar *path);
extern void         ejsRedirect(Ejs *ejs, int code, cchar *url);
extern void         ejsSetCookie(Ejs *ejs, cchar *name, cchar *value, int lifetime, cchar *path, bool secure);
extern void         ejsSetWebHeader(Ejs *ejs, bool allowMultiple, cchar *key, cchar *fmt, ...);
extern void         ejsSetHttpCode(Ejs *ejs, int code);
extern void         ejsSetMimeType(Ejs *ejs, int code);
extern int          ejsSetWebVar(Ejs *ejs, int collection, int field, EjsVar *value);
extern void         ejsWebError(Ejs *ejs, int code, cchar *fmt, ...);
extern int          ejsWriteBlock(Ejs *ejs, cchar *buf, int size);
extern int          ejsWriteString(Ejs *ejs, cchar *buf);
extern int          ejsWrite(Ejs *ejs, cchar *fmt, ...);

#ifdef  __cplusplus
}
#endif
#endif /* BLD_FEATURE_EJS_WEB */
#endif /* _h_EJS_WEB_h */

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
