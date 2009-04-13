/**
 *  ejsWebController.c - Native code for the Controller class
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejs.h"

#if BLD_FEATURE_EJS_WEB

/*********************************** Methods **********************************/
/*
 *  Add a cache-contol header
 *
 *  function cache(enable: Boolean = true): Void
 */
static EjsVar *cache(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    //  TODO
    return 0;
}


/*
 *  Enable sessions for this request.
 *
 *  function createSession(timeout: Number = 0): String
 */
static EjsVar *createSession(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    EjsWeb      *web;
    int         timeout;

    mprAssert(argc == 0 || argc == 1);

    timeout = (argc == 1) ? ejsGetInt(argv[0]): 0;
    web = ejsGetHandle(ejs);

    return (EjsVar*) ejsCreateSession(ejs, timeout, 0 /* TODO - need secure arg */);
}


/*
 *  function destroySession(): Void
 */
static EjsVar *destroySession(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    ejsDestroySession(ejs);
#if FUTURE
    EjsWeb     *web;

    mprAssert(argc == 0);

    web = ejsGetHandle(ejs);
    web->control->destroySession(web->handle);
#endif
    return 0;
}


/*
 *  Discard all pending output to the client
 *
 *  function discardOutput(): Void
 */
static EjsVar *discardOutput(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    ejsDiscardOutput(ejs);
    return 0;
}


/*
 *  Send an error response back to the client.
 *
 *  function sendError(code: Number, msg: String): Void
 */
static EjsVar *sendError(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    EjsWeb      *web;
    cchar       *msg;
    int         code;

    mprAssert(argc == 2);

    web = ejs->handle;
    code = ejsGetInt(argv[0]);
    msg = ejsGetString(argv[1]);

    ejsSetHttpCode(ejs, code);

    if (web->flags & EJS_WEB_FLAG_BROWSER_ERRORS) {
        ejsDiscardOutput(ejs);
        ejsWrite(ejs, msg);

    } else {
        //  TODO - this erorr should not go into the general web log
        mprLog(web, 3, "Web request error: %s", msg);
    }

    return 0;
}


/*
 *  Control whether the HTTP connection is kept alive after this request
 */
static EjsVar *keepAlive(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    //  TODO
    return 0;
}


/*
 *  Load a view. 
 *
 *  NOTE: The view name is not the file name nor is it the View class name. Rather it is a component of the
 *  class name. For example, in a stand-alone file with a .ejs extension, the view name will be StandAlone and the class
 *  name will be: Application_StandAloneView. For an application view, the name will be the name of a view within the 
 *  scope of a controller. For example: in a controller called "Portfolio", there may be a view called "edit". The resulting
 *  view class would be "Portfolio_editView"
 *
 *  If the view argument is not supplied, use the action name.
 *
 *  loadView(viewName: String = null)
 */
static EjsVar *loadView(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    EjsWeb      *web;
    EjsError    *e;
    int         rc;

    web = ejsGetHandle(ejs);
    if (argc > 0) {
        web->viewName = ejsGetString(argv[0]);
    }
    rc = ejsLoadView(ejs);
    if (rc < 0) {
        e = (EjsError*) ejsThrowError(ejs, "%s", web->error ? web->error : "Can't load view");
        if (ejsIsError(e)) {
            e->code = (rc == MPR_ERR_NOT_FOUND) ? MPR_HTTP_CODE_NOT_FOUND : MPR_HTTP_CODE_INTERNAL_SERVER_ERROR;
        }
        return 0;
    }
    return 0;
}


/*
 *  Redirect the client's browser to a new URL.
 *
 *  function redirectUrl(url: String, code: Number = 302): Void
 */
static EjsVar *redirectUrl(Ejs *ejs, EjsVar *controller, int argc, EjsVar **argv)
{
    char        *url;
    int         code;

    mprAssert(argc == 1 || argc == 2);

    url = ejsGetString(argv[0]);
    code = (argc == 2) ? ejsGetInt(argv[1]): 302;

    ejsRedirect(ejs, code, url);

    ejsSetProperty(ejs, controller, ES_ejs_web_Controller_rendered, (EjsVar*) ejs->trueValue);

    return 0;
}


/*
 *  Set a response header.
 *
 *  function setHeader(key: String, value: String, allowMultiple: Boolean = true): Void
 */
static EjsVar *setHeader(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    EjsWeb      *web;
    char        *key, *value;
    bool        allowMultiple;

    mprAssert(argc == 1 || argc == 2);

    web = ejsGetHandle(ejs);
    key = ejsGetString(argv[0]);
    value = ejsGetString(argv[1]);
    allowMultiple = (argc == 3) ? ejsGetBoolean(argv[2]) : 1;

    web->control->setHeader(web->handle, allowMultiple, key, value);
    return 0;
}


/**
 *  Define a cookie header to include in the reponse
 
 *  native function setCookie(name: String, value: String, lifetime: Number, path: String, secure: Boolean = false): Void
 */
static EjsVar *setCookie(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    EjsWeb      *web;
    char        *name, *value, *path;
    int         lifetime, secure;
    
    mprAssert(4 <= argc && argc <= 5);
    web = ejsGetHandle(ejs);

    name = ejsGetString(argv[0]);
    value = ejsGetString(argv[1]);
    lifetime = ejsGetInt(argv[2]);
    path = ejsGetString(argv[3]);
    secure = (argc == 5) ? ejsGetBoolean(argv[4]): 0;
    web->control->setCookie(web->handle, name, value, lifetime, path, secure);
    return 0;
}


/*
 *  Set the HTTP response code
 *
 *  native function setHttpCode(code: Number): Void
 */
static EjsVar *setHttpCode(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    EjsWeb      *web;
    int         code;
    
    mprAssert(argc == 1);
    web = ejsGetHandle(ejs);

    code = ejsGetInt(argv[0]);

    web->control->setHttpCode(web->handle, code);
    return 0;
}


/*
 *  Set the HTTP response code
 *
 *  native function setMimeType(format: String): Void
 */
static EjsVar *setMimeType(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    EjsWeb      *web;
    char        *mimeType;
    
    mprAssert(argc == 1);
    mprAssert(ejsIsString(argv[0]));

    web = ejsGetHandle(ejs);
    mimeType = ejsGetString(argv[0]);

    web->control->setMimeType(web->handle, mimeType);
    return 0;
}


/*
 *  Write text to the client. This call writes the arguments back to the client's browser. The arguments are converted
 *  to strings before writing back to the client. Text written using write, will be buffered by the web server.
 *  This allows text to be written prior to setting HTTP headers with setHeader.
 *
 *  function write(...args): Void
 */
static EjsVar *writeMethod(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    EjsString       *s;
    EjsVar          *args, *vp;
    EjsByteArray    *ba;
    int             err, i, count;

    mprAssert(argc == 1 && ejsIsArray(argv[0]));

    err = 0;
    args = argv[0];
    count = ejsGetPropertyCount(ejs, args);

    for (i = 0; i < count; i++) {
        vp = ejsGetProperty(ejs, args, i);
        if (vp) {
            switch (vp->type->id) {
            case ES_String:
                s = (EjsString*) vp;
                if (ejsWriteBlock(ejs, s->value, s->length) != s->length) {
                    err++;
                }
                break;

            case ES_ByteArray:
                ba = (EjsByteArray*) vp;
                if (ejsWriteBlock(ejs, (char*) ba->value, ba->length) != ba->length) {
                    err++;
                }

            default:
                s = (EjsString*) ejsToString(ejs, vp);
                if (s) {
                    if (ejsWriteBlock(ejs, s->value, s->length) != s->length) {
                        err++;
                    }
                }
            }
            if (ejs->exception) {
                return 0;
            }
        }
    }
#if UNUSED
    if (ejsWriteString(ejs, "\r\n") != 2) {
        err++;
    }
#endif
    if (err) {
        ejsThrowIOError(ejs, "Can't write to browser");
    }

    return 0;
}


/*********************************** Factory **********************************/
/*
 *  The controller type is a scripted class augmented by native methods.
 */
void ejsConfigureWebControllerType(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;

    type = (EjsType*) ejsGetPropertyByName(ejs, ejs->global, ejsName(&qname, "ejs.web", "Controller"));
    if (type == 0) {
        if (!(ejs->flags & EJS_FLAG_EMPTY)) {
            mprError(ejs, "Can't find ejs.web Controller class");
            ejs->hasError = 1;
        }
        return;
    }

    ejsBindMethod(ejs, type, ES_ejs_web_Controller_ejs_web_cache, (EjsNativeFunction) cache);
    ejsBindMethod(ejs, type, ES_ejs_web_Controller_ejs_web_createSession, (EjsNativeFunction) createSession);
    ejsBindMethod(ejs, type, ES_ejs_web_Controller_ejs_web_destroySession, (EjsNativeFunction) destroySession);
    ejsBindMethod(ejs, type, ES_ejs_web_Controller_ejs_web_discardOutput, (EjsNativeFunction) discardOutput);
    ejsBindMethod(ejs, type, ES_ejs_web_Controller_ejs_web_sendError, (EjsNativeFunction) sendError);
    ejsBindMethod(ejs, type, ES_ejs_web_Controller_ejs_web_keepAlive, (EjsNativeFunction) keepAlive);
    ejsBindMethod(ejs, type, ES_ejs_web_Controller_ejs_web_loadView, (EjsNativeFunction) loadView);
    ejsBindMethod(ejs, type, ES_ejs_web_Controller_ejs_web_redirectUrl, (EjsNativeFunction) redirectUrl);
    ejsBindMethod(ejs, type, ES_ejs_web_Controller_ejs_web_setCookie, (EjsNativeFunction) setCookie);
    ejsBindMethod(ejs, type, ES_ejs_web_Controller_ejs_web_setHeader, (EjsNativeFunction) setHeader);
    ejsBindMethod(ejs, type, ES_ejs_web_Controller_ejs_web_setHttpCode, (EjsNativeFunction) setHttpCode);
    ejsBindMethod(ejs, type, ES_ejs_web_Controller_ejs_web_setMimeType, (EjsNativeFunction) setMimeType);
    ejsBindMethod(ejs, type, ES_ejs_web_Controller_ejs_web_write, (EjsNativeFunction) writeMethod);
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
