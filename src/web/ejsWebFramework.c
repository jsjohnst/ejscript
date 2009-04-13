/*
 *  ejsWebFramework.c -- Ejscript web framework processing.
 *
 *  Ejscript provides an MVC paradigm for efficiently creating dynamic applications using server-side Javascript.
 *
 *  Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */

/********************************** Includes **********************************/

#include    "ejs.h"

#if BLD_FEATURE_EJS_WEB

/*********************************** Locals ***********************************/
/*
 *  Singleton instance for the Web server control structure
 */
static EjsWebControl *webControl;

/***************************** Forward Declarations ***************************/

static int  compile(EjsWeb *web, cchar *kind, cchar *name);
static void createCookie(Ejs *ejs, EjsVar *cookies, cchar *name, cchar *value, cchar *domain, cchar *path);
static int  initInterp(Ejs *ejs, EjsWebControl *control);
static int  loadApplication(EjsWeb *web);
static int  loadController(EjsWeb *web);
static int  loadComponent(EjsWeb *web, cchar *kind, cchar *name, cchar *ext);
static int  build(EjsWeb *web, cchar *kind, cchar *name, cchar *base, cchar *ext);
static int  parseControllerAction(EjsWeb *web);

/************************************ Code ************************************/
/*
 *  Create and configure web framework types
 */
void ejsConfigureWebTypes(Ejs *ejs)
{
    ejsConfigureWebRequestType(ejs);
    ejsConfigureWebResponseType(ejs);
    ejsConfigureWebHostType(ejs);
    ejsConfigureWebControllerType(ejs);
    ejsConfigureWebSessionType(ejs);
}


/*
 *  Loadable module interface. Called when loaded from a shared library.
 */
MprModule *ejs_webModuleInit(Ejs *ejs)
{
    MprModule   *module;

    module = mprCreateModule(ejs, "ejsWeb", BLD_VERSION, 0, 0, 0);
    if (module == 0) {
        return 0;
    }

    ejsSetGeneration(ejs, EJS_GEN_ETERNAL);
    ejsConfigureWebTypes(ejs);
    ejsSetGeneration(ejs, EJS_GEN_OLD);

    return module;
}


/*
 *  Called once by the web server handler when it it loaded.
 */
int ejsOpenWebFramework(EjsWebControl *control, bool useMaster)
{
    mprAssert(control);

    /*
     *  Create the Ejscript service
     */
    control->service = ejsCreateService(control);
    if (control->service == 0) {
        return MPR_ERR_NO_MEMORY;
    }

    if (useMaster) {
        /*
         *  Create the master interpreter
         */
        control->master = ejsCreate(control->service, 0, EJS_FLAG_MASTER);
        if (control->master == 0) {
            mprAssert(control->master);
            mprFree(control->service);
            return MPR_ERR_NO_MEMORY;
        }

        if (initInterp(control->master, control) < 0) {
            return MPR_ERR_CANT_INITIALIZE;
        }
    }
    webControl = control;
    return 0;
}


static int initInterp(Ejs *ejs, EjsWebControl *control)
{
    EjsVar      *sessions;

#if !BLD_FEATURE_STATIC
    if (ejsLoadModule(ejs, "ejs.web", NULL, NULL, 0) == 0) {
        mprError(control, "Can't load ejs.web.mod: %s", ejsGetErrorMsg(ejs, 1));
        return MPR_ERR_CANT_INITIALIZE;;
    }
#endif

#if FUTURE
    control->applications = ejsCreateSimpleObject(ejs);
#endif

    //  TODO - should session timeouts per different per app?
    control->sessionTimeout = EJS_SESSION_TIMEOUT;
#if ES_ejs_web_sessions
    sessions = ejsGetProperty(ejs, ejs->global, ES_ejs_web_sessions);
#else
{
    EjsName qname;
    sessions = ejsGetPropertyByName(ejs, ejs->global, ejsName(&qname, "ejs.web", "sessions"));
}
#endif
    ejsMakePermanent(ejs, sessions);
    control->sessions = (EjsObject*) sessions;

    return 0;
}


/*
 *  Return the web server master interpreter. If not using a master interpreter, it returns the current interp.
 */
Ejs *ejsGetMaster(MprCtx ctx)
{
    return (Ejs*) webControl->master;
}


/*
 *  Initialize a new web request structure. 
 *  Given request       "/carmen/admin/list/1?a=b&c=d", then the args would be:
 *      baseUrl         "/carmen"
 *      url             "/admin/list/1"
 *      query           "a=b&c=d"
 *      baseDir         "/path/carmen"
 */
EjsWeb *ejsCreateWebRequest(MprCtx ctx, EjsWebControl *control, void *handle, cchar *baseUrl, cchar *url,
        cchar *baseDir, int flags)
{
    Ejs             *ejs;
    EjsWeb          *web;
    cchar           *appUrl;

    web = (EjsWeb*) mprAllocObjZeroed(ctx, EjsWeb);
    if (web == 0) {
        return 0;
    }

    if (flags & EJS_WEB_FLAG_APP) {
        appUrl = baseUrl;

    } else {
        appUrl = 0;
        flags |= EJS_WEB_FLAG_SOLO;
    }

    web->appDir = mprStrdup(web, baseDir);
    mprStrTrim((char*) web->appDir, "/");
    web->appUrl = appUrl;
    web->url = url;

    web->flags = flags;
    web->handle = handle;
    web->control = control;

    if (control->master) {
        ejs = web->ejs = ejsCreate(ctx, control->master, 0);
        ejs->master = control->master;
    } else {
        ejs = web->ejs = ejsCreate(ctx, 0, 0);
        if (ejs) {
            if (initInterp(ejs, control) < 0) {
                mprFree(web);
                return 0;
            }
        }
    }
    if (ejs == 0) {
        mprFree(web);
        return 0;
    }

    ejsSetHandle(ejs, web);

    //  TODO - temp
    if (control->master) {
        control->master->gc.enabled = 0;
    }
    ejs->gc.enabled = 0;

    mprLog(ctx, 3, "EJS: new request: AppDir %s, AppUrl %s, URL %s", web->appDir, web->appUrl, web->url);

    return web;
}


/*
 *  Parse the request URI and create the controller and action names. URI is in the form: "controller/action"
 */
static int parseControllerAction(EjsWeb *web)
{
    cchar   *url;
    char    *cp, *controllerName;

    if (web->flags & EJS_WEB_FLAG_SOLO || strstr(web->url, EJS_WEB_EXT)) {
        if (web->flags & EJS_WEB_FLAG_SOLO) {
            ejsName(&web->controllerName, "ejs.web", "_SoloController");
        } else {
            ejsName(&web->controllerName, EJS_PUBLIC_NAMESPACE, "BaseController");
        }
        ejsName(&web->doActionName, "ejs.web", "renderView");

        /*
         *  View name strips extension and converts "/" to "_"
         */
        web->viewName = mprStrdup(web, &web->url[1]);
        if ((cp = strchr(web->viewName, '.')) != 0) {
            *cp = '\0';
        }
        for (cp = web->viewName; *cp; cp++) {
            if (*cp == '/') {
                *cp = '_';
            }
        }
        return 0;
    }

    /*
     *  Request as part of an Ejscript application (not stand-alone)
     */
    for (url = web->url; *url == '/'; url++) {
        ;
    }
    controllerName = mprStrdup(web, url);
    controllerName[0] = toupper((int) controllerName[0]);
    mprStrTrim(controllerName, "/");

    web->viewName = "";
    if ((cp = strchr(controllerName, '/')) != 0) {
        *cp++ = '\0';
        web->viewName = cp;
        if ((cp = strchr(cp, '/')) != 0) {
            *cp++ = '\0';
        }
    }
    if (*controllerName == '\0') {
        controllerName = "Base";
    }
    if (*web->viewName == '\0') {
        web->viewName = "index";
    }

    mprAllocSprintf(web, &cp, -1, "%sController", controllerName);
    ejsName(&web->controllerName, EJS_PUBLIC_NAMESPACE, cp);
    web->controllerFile = controllerName;

    ejsName(&web->doActionName, "ejs.web", "doAction");

    return 0;
}


static int loadApplication(EjsWeb *web)
{
    return loadComponent(web, "app", "App", ".es");
}


/*
 *  Load the controller module
 */
static int loadController(EjsWeb *web)
{
    return loadComponent(web, "controller", web->controllerFile, ".es");
}


static int createController(EjsWeb *web)
{
    Ejs         *ejs;
    EjsVar      *host, *request, *response, *argv[16];
    EjsType     *type;
    int         oldGen, slotNum;

    ejs = web->ejs;

    if (web->flags & EJS_WEB_FLAG_APP) {
        if (loadApplication(web) < 0) {
            return MPR_ERR_NOT_FOUND;
        }
    }

    web->controllerType = (EjsType*) ejsGetPropertyByName(ejs, ejs->global, &web->controllerName);
    if (web->controllerType == 0 || !ejsIsType(web->controllerType)) {
        if (web->controllerFile && loadController(web) < 0) {
            return MPR_ERR_NOT_FOUND;
        }
        web->controllerType = (EjsType*) ejsGetPropertyByName(ejs, ejs->global, &web->controllerName);
        if (web->controllerType == 0 || !ejsIsType(web->controllerType)) {
            mprAllocSprintf(web, &web->error, -1, "Can't find controller %s", web->controllerName.name);
            return MPR_ERR_NOT_FOUND;
        }
    }

    oldGen = ejsSetGeneration(ejs, EJS_GEN_ETERNAL);

    if ((web->cookie = (char*) ejsGetHeader(ejs, "HTTP_COOKIE")) != 0) {
        ejsParseWebSessionCookie(web);
    }

    if (web->flags & EJS_WEB_FLAG_SESSION && web->session == 0) {
        //  TODO - not setting secure appropriately. Last parameter is "secure"
        web->session = ejsCreateSession(ejs, 0, 0);
    }

    /*
     *  Create the Host, Request and Response objects. These are virtual objects that lazily create their properties.
     */
    host =      (EjsVar*) ejsCreateWebHostObject(ejs, web->handle);
    response =  (EjsVar*) ejsCreateWebResponseObject(ejs, web->handle);
    request =   (EjsVar*) ejsCreateWebRequestObject(ejs, web->handle);

    argv[0] = (web->flags & EJS_WEB_FLAG_SOLO) ? (EjsVar*) ejs->falseValue : (EjsVar*) ejs->trueValue;
    argv[1] = (EjsVar*) ejsCreateString(ejs, web->appDir);
    argv[2] = (EjsVar*) ejsCreateString(ejs, web->appUrl);
    argv[3] = (EjsVar*) web->session;
    argv[4] = host;
    argv[5] = request;
    argv[6] = response;

    web->controller = (EjsVar*) ejsCreateObject(ejs, web->controllerType, 0);
    if (web->controller == 0) {
        /* TODO - functionalize this */
        web->error = "Memory allocation failure";
        return MPR_ERR_NO_MEMORY;
    }

    ejsRunFunctionBySlot(ejs, web->controller, ES_ejs_web_Controller_ejs_web_initialize, 7, argv);

    type = (EjsType*) web->controllerType;
    if (type->hasConstructor) {
        slotNum = type->block.numInherited;
        ejsRunFunctionBySlot(ejs, web->controller, slotNum, 0, NULL);
    }

    web->params = ejsGetProperty(ejs, web->controller, ES_ejs_web_Controller_params);
    //  TODO - rc
    ejsDefineParams(ejs);
    ejsSetGeneration(ejs, oldGen);
    
    return 0;
}


static int getDoAction(EjsWeb *web)
{
    Ejs     *ejs;

    ejs = web->ejs;
    web->doAction = ejsGetPropertyByName(ejs, (EjsVar*) web->controllerType, &web->doActionName);
    if (web->doAction == 0 || !ejsIsFunction(web->doAction)) {
        mprAllocSprintf(web, &web->error, -1, "Internal error: Can't find function %s::%s", web->doActionName.space,
                web->doActionName.name);
        return EJS_ERR;
    }
    return 0;
}


/*
 *  Run a web request
 */
int ejsRunWebRequest(EjsWeb *web)
{
    Ejs             *ejs;
    EjsVar          *result, *argv[1];

    ejs = web->ejs;

    /*
     *  Parse the url and extract the controller and action name
     */
    if (parseControllerAction(web) < 0) {
        mprAllocSprintf(web, &web->error, -1, "URL is not in the right form: \"%s\"", web->url);
        return MPR_ERR_BAD_ARGS;
    }
    if (createController(web) < 0) {
        return MPR_ERR_CANT_CREATE;
    }
    if (getDoAction(web) < 0) {
        return MPR_ERR_CANT_CREATE;
    }

    argv[0] = (EjsVar*) ejsCreateString(ejs, web->viewName);
    result = ejsRunFunction(ejs, (EjsFunction*) web->doAction, web->controller, 1, argv);
    if (result == 0 && ejs->exception) {
        web->error = ejsGetErrorMsg(ejs, 1);
        return MPR_ERR_CANT_COMPLETE;
    }
    return 0;
}


int ejsLoadView(Ejs *ejs)
{
    EjsWeb      *web;
    char        name[MPR_MAX_FNAME], *cp;

    mprAssert(ejs);
    web = ejs->handle;

    if (!(web->flags & EJS_WEB_FLAG_SOLO) && !strstr(web->url, EJS_WEB_EXT)) {
        /*
         *  Normal views/...
         */
        mprSprintf(name, sizeof(name), "%s/%s", web->controllerFile, web->viewName);
        return loadComponent(web, "view", name, EJS_WEB_EXT);

    }
    mprStrcpy(name, sizeof(name), &web->url[1]);
    if ((cp = strrchr(name, '.')) && strcmp(cp, EJS_WEB_EXT) == 0) {
        *cp = '\0';
    }
    return loadComponent(web, "", name, EJS_WEB_EXT);
}


/*
 *  Load a module corresponding to a source component. If the source is newer, then recompile the component. Build the 
 *  name to the module from web->appdir and name depending on the kind. Kind will be "app", "controller", "view" or "".
 */
static int loadComponent(EjsWeb *web, cchar *kind, cchar *name, cchar *ext)
{
    Ejs         *ejs;
    char        base[MPR_MAX_FNAME], nameBuf[MPR_MAX_FNAME], *delim;
    int         rc;

    ejs = web->ejs;

    delim = (web->appDir[strlen(web->appDir) - 1] == '/') ? "" : "/";

    if (strcmp(kind, "app") == 0) {
        mprSprintf(base, sizeof(base), "%s%s%s", web->appDir, delim, name);
        //  TODO - need to recompile the app with models somehow

    } else if (*kind) {
        /* Note we pluralize the kind (e.g. view to views) */
        mprSprintf(base, sizeof(base), "%s%s%ss/%s", web->appDir, delim, kind, name);
        if ((rc = build(web, kind, name, base, ext)) < 0) {
            return rc;
        }

    } else {
        /*
         *  Solo web pages
         */
        mprSprintf(base, sizeof(base), "%s%s%s", web->appDir, delim, name);
        mprSprintf(nameBuf, sizeof(nameBuf), "%s%s", name, ext);
        name = nameBuf;
        if ((rc = build(web, kind, name, base, ext)) < 0) {
            return rc;
        }
    }

    if (ejsLoadModule(web->ejs, base, NULL, NULL, 0) == 0) {
        mprAllocSprintf(web, &web->error, -1, "Can't load module : \"%s.mod\"\n%s", base, ejsGetErrorMsg(ejs, 1));
        return MPR_ERR_CANT_READ;
    }
    return 0;
}


/*
 *  Compile a component into a loadable module. Return true if the compile succeeded.
 */
static int compile(EjsWeb *web, cchar *kind, cchar *name)
{
    MprCmd      *cmd;
    cchar       *dir;
    char        target[MPR_MAX_FNAME], appDir[MPR_MAX_FNAME], commandLine[MPR_MAX_FNAME * 4];
    char        *path, *err;
    int         status;

    if (strcmp(kind, "view") == 0) {
        mprSprintf(target, sizeof(target), "views/%s", name);
        name = target;
    }

    cmd = mprCreateCmd(web);
    mprSetCmdDir(cmd, web->appDir);

#if FUTURE
    /*
     *  Enable this when supported external Ejscript modules for Appweb
     */
    if (web->control->modulePath && *web->control->modulePath) {
        /*
         *  Location where the Ejscript module was loaded from (typically BLD_MOD_PREFIX)
         */
        dir = web->control->modulePath;
        /*
         *  Add this below to the search path when enabled
         */
        dir, "/../../bin", MPR_SEARCH_DELIM,    //  Search relative to the module path for dev trees
    } else {
#endif

    dir = mprGetAppDir(web, appDir, sizeof(appDir));

    /*
     *  Search for ejsweb
     */
    path = mprSearchForFile(web, EJS_EJSWEB_EXE, MPR_SEARCH_EXE, 
        dir, MPR_SEARCH_DELIM,                  //  Search in same dir as application (or override module path) (Windows)
        BLD_BIN_PREFIX, MPR_SEARCH_DELIM,       //  Search the standard binary install directory
        BLD_ABS_BIN_DIR, NULL);                 //  Search the local dev bin
	if (path == 0) {
        mprError(web, "Can't find %s program", EJS_EJSWEB_EXE);
        return MPR_ERR_CANT_ACCESS;
    }

    mprSprintf(commandLine, sizeof(commandLine), "\"%s\" --quiet compile %s \"%s\"", path, kind, name);
    mprLog(web, 4, "Running %s", commandLine);

    status = mprRunCmd(cmd, commandLine, NULL, &err, 0);
    if (status) {
        web->error = mprStrdup(web, err);
        mprLog(web, 3, "Compilation failure for %s\n%s", commandLine, err);
    }
    mprFree(cmd);
    return status;
}


/*
 *  Buidl a resource
 */
static int build(EjsWeb *web, cchar *kind, cchar *name, cchar *base, cchar *ext)
{
    MprFileInfo     moduleInfo, sourceInfo;
    char            module[MPR_MAX_FNAME], source[MPR_MAX_FNAME];

    mprSprintf(module, sizeof(module), "%s.mod", base);
    mprSprintf(source, sizeof(source), "%s%s", base, ext);
    mprGetFileInfo(web, module, &moduleInfo);
    mprGetFileInfo(web, source, &sourceInfo);

    if (!sourceInfo.valid) {
        mprLog(web, 3, "Can't find resource %s", source);
        mprAllocSprintf(web, &web->error, -1, "Can't find resource: \"%s\"", source);
        return MPR_ERR_NOT_FOUND;
    }
    if (moduleInfo.valid && sourceInfo.valid && sourceInfo.mtime < moduleInfo.mtime) {
        /* Up to date already */
        mprLog(web, 5, "Resource %s is up to date", source);
        return 0;
    }
    if (compile(web, kind, name) != 0) {
        return MPR_ERR_BAD_STATE;
    }
    return 0;
}


/*
 *  This routine parses the cookie header to search for a session cookie.
 *  There may be multiple cookies where the most qualified path come first
 */
EjsVar *ejsCreateCookies(Ejs *ejs)
{
    EjsWeb      *web;
    cchar       *domain, *path, *version, *cookieName, *cookieValue;
    char        *cookieString, *value, *tok, *key, *dp, *sp;
    int         seenSemi;

    web = ejs->handle;
    if (web->cookies) {
        return web->cookies;
    }

    web->cookies = (EjsVar*) ejsCreateSimpleObject(ejs);

    cookieString = mprStrdup(web, web->cookie);
    key = cookieString;
    cookieName = cookieValue = domain = path = 0;

    while (*key) {
        while (*key && isspace(*key)) {
            key++;
        }
        tok = key;
        while (*tok && !isspace(*tok) && *tok != ';' && *tok != '=') {
            tok++;
        }
        if (*tok) {
            *tok++ = '\0';
        }
        while (isspace(*tok)) {
            tok++;
        }

        seenSemi = 0;
        if (*tok == '\"') {
            value = ++tok;
            while (*tok != '\"' && *tok != '\0') {
                tok++;
            }
            if (*tok) {
                *tok++ = '\0';
            }

        } else {
            value = tok;
            while (*tok != ';' && *tok != '\0') {
                tok++;
            }
            if (*tok) {
                seenSemi++;
                *tok++ = '\0';
            }
        }

        /*
         *  Handle back-quoting in value
         */
        if (strchr(value, '\\')) {
            for (dp = sp = value; *sp; sp++) {
                if (*sp == '\\') {
                    sp++;
                }
                *dp++ = *sp++;
            }
            *dp = '\0';
        }

        /*
         *  Example:
         *  $Version="1"; NAME="value"; $Path="PATH"; $Domain="DOMAIN"; NAME="value"; $Path="PATH"; $Domain="DOMAIN"; 
         */
        if (*key == '$') {
            key++;
            switch (tolower(*key)) {
            case 'd':
                if (mprStrcmpAnyCase(key, "domain") == 0) {
                    domain = value;
                }
                break;

            case 'p':
                if (mprStrcmpAnyCase(key, "path") == 0) {
                    path = value;
                }
                break;

            case 'v':
                if (mprStrcmpAnyCase(key, "version") == 0) {
                    version = value;
                }
                break;
            default:
                break;
            }
            
        } else {
            if (cookieName) {
                createCookie(ejs, web->cookies, cookieName, cookieValue, domain, path);
                cookieName = cookieValue = path = domain = 0;
            }
            cookieName = key;
            cookieValue = value;
        }

        key = tok;
        if (!seenSemi) {
            while (*key && *key != ';') {
                key++;
            }
            if (*key) {
                key++;
            }
        }
    }
    if (cookieName) {
        createCookie(ejs, web->cookies, cookieName, cookieValue, domain, path);
    }
    mprFree(cookieString);
    return web->cookies;
}


static EjsVar *createString(Ejs *ejs, cchar *value)
{
    return value ? (EjsVar*) ejsCreateString(ejs, value) : ejs->nullValue;
}


static void createCookie(Ejs *ejs, EjsVar *cookies, cchar *name, cchar *value, cchar *domain, cchar *path)
{
    EjsType     *cookieType;
    EjsName     qname;
    EjsVar      *cookie;
    
#if ES_ejs_web_Cookie
    cookieType = ejsGetType(ejs, ES_ejs_web_Cookie);
#else
    cookieType = (EjsType*) ejsGetPropertyByName(ejs, ejs->global, ejsName(&qname, "ejs.web", "Cookie"));
#endif
    mprAssert(cookieType);

    cookie = (EjsVar*) ejsCreateObject(ejs, cookieType, 0);

    ejsSetProperty(ejs, cookie, ES_ejs_web_Cookie_name, createString(ejs, name));
    ejsSetProperty(ejs, cookie, ES_ejs_web_Cookie_value, createString(ejs, value));
    ejsSetProperty(ejs, cookie, ES_ejs_web_Cookie_path, createString(ejs, path));
    ejsSetProperty(ejs, cookie, ES_ejs_web_Cookie_domain, createString(ejs, domain));

    ejsSetPropertyByName(ejs, cookies, ejsName(&qname, "", name), (EjsVar*) cookie);
}


/*
 *  Define a form variable as an ejs property in the params[] collection. Support a.b.c syntax
 */
void ejsDefineWebParam(Ejs *ejs, cchar *key, cchar *value)
{
    EjsName     qname;
    EjsWeb      *web;
    EjsVar      *where, *vp;
    char        *subkey, *end;
    int         slotNum;

    web = ejsGetHandle(ejs);

    where = web->params;
    mprAssert(where);

    /*
     *  name.name.name
     */
    if (strchr(key, '.') == 0) {
        ejsName(&qname, "", key);
        ejsSetPropertyByName(ejs, where, &qname, (EjsVar*) ejsCreateString(ejs, value));

    } else {
        key = subkey = mprStrdup(ejs, key);
        for (end = strchr(subkey, '.'); end; subkey = end, end = strchr(subkey, '.')) {
            *end++ = '\0';
            ejsName(&qname, "", subkey);
            vp = ejsGetPropertyByName(ejs, where, &qname);
            if (vp == 0) {
                slotNum = ejsSetPropertyByName(ejs, where, &qname, (EjsVar*) ejsCreateObject(ejs, ejs->objectType, 0));
                vp = ejsGetProperty(ejs, where, slotNum);
            }
            where = vp;
        }
        mprAssert(where);
        ejsName(&qname, "", subkey);
        ejsSetPropertyByName(ejs, where, &qname, (EjsVar*) ejsCreateString(ejs, value));
        mprFree((char*) key);
    }
}


#endif /* BLD_FEATURE_EJS_WEB */

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
