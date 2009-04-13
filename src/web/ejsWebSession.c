/**
 *  ejsWebSession.c - Native code for the Session class.
 *
 *  The Session class serializes objects that are stored to the session object.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejs.h"

#if BLD_FEATURE_EJS_WEB
/*********************************** Forwards *********************************/

static void sessionActivity(Ejs *ejs, EjsWebSession *sp);
static void sessionTimer(EjsWebControl *control, MprEvent *event);

/************************************* Code ***********************************/
#if UNUSED
static void descape(EjsVar *vp)
{
    char    *str, *cp, *dp;
    
    if (!ejsIsString(vp)) {
        return;
    }
    
    str = ((EjsString*) vp)->value;
    
    if (str == 0 || *str == '\0') {
        return;
    }
    for (dp = cp = str; *cp; ) {
        if (*cp == '\\') {
            cp++;
        }
        *dp++ = *cp++;
    }
    *dp = '\0';
}
#endif


static EjsVar *getSessionProperty(Ejs *ejs, EjsWebSession *sp, int slotNum)
{
    EjsVar      *vp;
    EjsWeb      *web;

    web = ejs->handle;
    if (web->session != sp) {
        return (EjsVar*) ejs->emptyStringValue;
    }

    vp = ejs->objectHelpers->getProperty(ejs, (EjsVar*) sp, slotNum);
    if (vp) {
        vp = ejsDeserialize(ejs, vp);
#if UNUSED
        descape(vp);
#endif
    }
    if (vp == ejs->undefinedValue) {
        vp = (EjsVar*) ejs->emptyStringValue;
    }
    sessionActivity(ejs, sp);
    return vp;
}


static EjsVar *getSessionPropertyByName(Ejs *ejs, EjsWebSession *sp, EjsName *qname)
{
    EjsVar      *vp;
    EjsWeb      *web;
    int         slotNum;

    web = ejs->handle;
    if (web->session != sp) {
        return (EjsVar*) ejs->emptyStringValue;
    }

    qname->space = EJS_PUBLIC_NAMESPACE;
    slotNum = ejs->objectHelpers->lookupProperty(ejs, (EjsVar*) sp, qname);
    if (slotNum < 0) {
        /*
         *  Return empty string so that web pages can access session values without having to test for null/undefined
         */
        vp = (EjsVar*) ejs->emptyStringValue;
    } else {
        vp = ejs->objectHelpers->getProperty(ejs, (EjsVar*) sp, slotNum);
        if (vp) {
            vp = ejsDeserialize(ejs, vp);
#if UNUSED
            descape(vp);
#endif
        }
    }
    sessionActivity(ejs, sp);
    return vp;
}


#if UNUSED
static char *escape(MprCtx ctx, char *str)
{
    char    *cp, *bp, *buf;
    int     len;

    len = 0;
    for (cp = str; *cp; cp++) {
        if (*cp == '\'' || *cp == '"' || *cp == '\\') {
            len++;
        }
        len++;
    }

    bp = buf = mprAlloc(ctx, len + 1);
    if (bp == 0) {
        return 0;
    }

    for (bp = buf, cp = str; *cp; ) {
        if (*cp == '\'' || *cp == '"' || *cp == '\\') {
            *bp++ = '\\';
        }
        *bp++ = *cp++;
    }
    *bp = '\0';
    return buf;
}
#endif


static int setSessionProperty(Ejs *ejs, EjsWebSession *sp, int slotNum, EjsVar *value)
{
    EjsWeb  *web;
    
    web = ejs->handle;
    if (web->session != sp) {
        mprAssert(0);
        return EJS_ERR;
    }

    /*
     *  Allocate the serialized object using the master interpreter
     */
    if (ejs->master) {
        ejs = ejs->master;
    }

#if UNUSED
    if (value != ejs->undefinedValue && value != ejs->nullValue) {
        value = (EjsVar*) ejsSerialize(ejs, value, 0, 0, 0);
        estr = escape(ejs, ((EjsString*) value)->value);
        value = (EjsVar*) ejsCreateString(ejs, estr);
        mprFree(estr);
    }
#else
    value = (EjsVar*) ejsSerialize(ejs, value, 0, 0, 0);
#endif
    slotNum = ejs->objectHelpers->setProperty(ejs, (EjsVar*) sp, slotNum, value);
    
    sessionActivity(ejs, sp);
    return slotNum;
}


/******************************************************************************/
/*
 *  Update the session expiration time due to activity
 */
static void sessionActivity(Ejs *ejs, EjsWebSession *sp)
{
    sp->expire = mprGetTime(ejs) + sp->timeout * MPR_TICKS_PER_SEC;
}


/*
 *  Check for expired sessions
 */
static void sessionTimer(EjsWebControl *control, MprEvent *event)
{
    Ejs             *master;
    EjsObject       *sessions;
    EjsWebSession   *session;
    MprTime         now;
    int             i, count, deleted;

    sessions = control->sessions;
    master = control->master;
    if (master == 0) {
        mprAssert(master);
        return;
    }

    now = mprGetTime(control);

    //  TODO - locking MULTITHREAD
 
    count = ejsGetPropertyCount(master, (EjsVar*) sessions);
    deleted = 0;
    for (i = 0; i < count; i++) {
        session = (EjsWebSession*) ejsGetProperty(master, (EjsVar*) sessions, i);
        if (session && session->expire <= now) {
            ejsDeleteProperty(master, (EjsVar*) sessions, i);
            deleted++;
            i--;
            count--;
        }
    }
    if (deleted) {
        ejsCollectGarbage(master, EJS_GC_ALL);
    }
}


void ejsParseWebSessionCookie(EjsWeb *web)
{
    EjsName         qname;
    EjsWebControl   *control;
    char            *id, *cp, *value;
    int             quoted, len;

    if ((value = strstr(web->cookie, EJS_SESSION)) == 0) {
        return;
    }
    value += strlen(EJS_SESSION);

    while (isspace(*value) || *value == '=') {
        value++;
    }
    quoted = 0;
    if (*value == '"') {
        value++;
        quoted++;
    }
    for (cp = value; *cp; cp++) {
        if (quoted) {
            if (*cp == '"' && cp[-1] != '\\') {
                break;
            }
        } else {
            if ((*cp == ',' || *cp == ';') && cp[-1] != '\\') {
                break;
            }
        }
    }
    control = web->control;

    len = cp - value;
    id = mprMemdup(web, value, len + 1);
    id[len] = '\0';

    if (control->master) {
        ejsName(&qname, "", id);
        web->session = (EjsWebSession*) ejsGetPropertyByName(control->master, (EjsVar*) control->sessions, &qname);
    }
    mprFree(id);
}


/*********************************** Factory **********************************/
/*
 *  Create a new session object. This is created in the master interpreter and will persist past the life of the current
 *  request. This will allocate a new session ID. Timeout is in seconds.
 */
EjsWebSession *ejsCreateSession(Ejs *ejs, int timeout, bool secure)
{
    Ejs             *master;
    EjsWeb          *web;
    EjsWebControl   *control;
    EjsWebSession   *session;
    EjsType         *sessionType;
    EjsName         qname;
    char            idBuf[64], *id;

    master = ejs->master;
    if (master == 0) {
        return 0;
    }
    web = ejsGetHandle(ejs);
    control = web->control;

    if (timeout <= 0) {
        timeout = control->sessionTimeout;
    }

#if ES_ejs_web_Session
    sessionType = ejsGetType(ejs, ES_ejs_web_Session);
#else
    sessionType = (EjsType*) ejsGetPropertyByName(ejs, ejs->global, ejsName(&qname, "ejs.web", "Session"));
#endif
    if (sessionType == 0) {
        mprAssert(0);
        return 0;
    }

    session = (EjsWebSession*) ejsCreateObject(master, sessionType, 0);
    if (session == 0) {
        return 0;
    }

    session->timeout = timeout;
    session->expire = mprGetTime(ejs) + timeout * MPR_TICKS_PER_SEC;

    mprSprintf(idBuf, sizeof(idBuf), "%08x%08x%08x", PTOI(ejs) + PTOI(web) + PTOI(session->expire), 
        (int) time(0), (int) control->nextSession++);
    id = mprGetMD5Hash(session, (uchar*) idBuf, sizeof(idBuf), "x");
    if (id == 0) {
        mprFree(session);
        return 0;
    }
    session->id = mprStrdup(session, id);

    /*
     *  Create a cookie that will only live while the browser is not exited. (Set timeout to zero).
     */
    ejsSetCookie(ejs, EJS_SESSION, id, 0, "/", secure);

    /*
     *  We use an MD5 prefix of "x" above so we can avoid the hash being interpreted as a numeric index.
     */
    ejsName(&qname, "", session->id);
    ejsSetPropertyByName(control->master, (EjsVar*) control->sessions, &qname, (EjsVar*) session);
    web->session = session;

    mprLog(ejs, 3, "Created new session %s", id);

    if (control->sessions->numProp == 1 /* TODO && !mprGetDebugMode(master) */) {
        mprCreateTimerEvent(master, (MprEventProc) sessionTimer, EJS_TIMER_PERIOD, MPR_NORMAL_PRIORITY, control, 
            MPR_EVENT_CONTINUOUS);
    }

    return session;
}


bool ejsDestroySession(Ejs *ejs)
{
    EjsWeb          *web;
    EjsWebControl   *control;
    EjsName         qname;
    int             rc;

    web = ejs->handle;
    control = web->control;

    if (web->session == 0) {
        //  TODO - remove
        mprAssert(0);
        return 0;
    }

    rc = ejsDeletePropertyByName(control->master, (EjsVar*) control->sessions, ejsName(&qname, "", web->session->id));
    web->session = 0;
    return rc;
}


EjsWebSession *ejsCreateWebSessionObject(Ejs *ejs, void *handle)
{
    EjsWebSession   *vp;
    EjsType         *requestType;
    EjsName         qname;

    requestType = (EjsType*) ejsGetPropertyByName(ejs, ejs->global, ejsName(&qname, "ejs.web", "Session"));
    vp = (EjsWebSession*) ejsCreateVar(ejs, requestType, 0);
    ejsSetDebugName(vp, "EjsWeb Session Instance");

    return vp;
}


void ejsConfigureWebSessionType(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;

    type = (EjsType*) ejsGetPropertyByName(ejs, ejs->global, ejsName(&qname, "ejs.web", "Session"));
    if (type == 0) {
        if (!(ejs->flags & EJS_FLAG_EMPTY)) {
            mprError(ejs, "Can't find web Session class");
            ejs->hasError = 1;
        }
        return;
    }
    type->instanceSize = sizeof(EjsWebSession);
    mprAssert(type->hasObject);

    /*
     *  Re-define the helper functions.
     */
    type->helpers->getProperty = (EjsGetPropertyHelper) getSessionProperty;
    type->helpers->getPropertyByName = (EjsGetPropertyByNameHelper) getSessionPropertyByName;
    type->helpers->setProperty = (EjsSetPropertyHelper) setSessionProperty;
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
