/*
 *  ejsApp.c -- App class
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejs.h"

/*********************************** Methods **********************************/
/*
 *  Get the application command line arguments
 *
 *  static function get args(): String
 */
static EjsVar *getArgs(Ejs *ejs, EjsObject *unused, int argc, EjsVar **argv)
{
    EjsArray    *args;
    int         i;

    args = ejsCreateArray(ejs, ejs->argc);
    for (i = 0; i < ejs->argc; i++) {
        ejsSetProperty(ejs, (EjsVar*) args, i, (EjsVar*) ejsCreateString(ejs, ejs->argv[i]));
    }
    return (EjsVar*) args;
}


/*
 *  Get the application startup directory
 *
 *  static function get dir(): String
 */
static EjsVar *getDir(Ejs *ejs, EjsObject *unused, int argc, EjsVar **argv)
{
    char        path[MPR_MAX_FNAME];

    if (mprGetAppDir(ejs, path, sizeof(path)) == 0) {
        ejsThrowIOError(ejs, "Can't get application directory");
        return 0;
    }

    return (EjsVar*) ejsCreateString(ejs, path);
}


/**
 *  Exit the application
 *
 *  static function exit(status: Number): void
 */
static EjsVar *exitMethod(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    int     status;

    status = argc == 0 ? 0 : ejsGetInt(argv[0]);
    if (status != 0) {
        exit(status);
    } else {
        mprTerminate(mprGetMpr(ejs), 1);
    }
    return 0;
}


/**
 *  Control if the application will exit when the last script completes.
 *
 *  static function noexit(exit: Boolean): void
 */
static EjsVar *noexit(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    ejs->flags |= EJS_FLAG_NOEXIT;
    return 0;
}


/**
 *  Service events
 *
 *  static function serviceEvents(count: Number = -1, timeout: Number = -1): void
 */
static EjsVar *serviceEvents(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    MprTime     start;
    int         count, timeout;

    count = (argc > 1) ? ejsGetInt(argv[0]) : MAXINT;
    timeout = (argc > 1) ? ejsGetInt(argv[1]) : MAXINT;
    if (count < 0) {
        count = MAXINT;
    }
    if (timeout < 0) {
        timeout = MAXINT;
    }

    start = mprGetTime(ejs);
    do {
        mprServiceEvents(ejs, timeout, MPR_SERVICE_ONE_THING);
        timeout -= (int) (mprGetTime(ejs) - start);
        count--;
    } while (count > 0 && timeout > 0);

    return 0;
}


/**
 *  Pause the application
 *
 *  static function sleep(delay: Number = -1): void
 */
static EjsVar *sleepProc(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    MprTime     start;
    int         delay;

    delay = (argc > 0) ? ejsGetInt(argv[0]): MAXINT;
    if (delay < 0) {
        delay = MAXINT;
    }

    start = mprGetTime(ejs);
    do {
        mprServiceEvents(ejs, delay, 0);
        delay -= (int) (mprGetTime(ejs) - start);
    } while (delay > 0);

    return 0;
}


static EjsVar *workingDir(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    char    path[MPR_MAX_FNAME];

    getcwd(path, sizeof(path));
#if BLD_WIN_LIKE
    mprMapDelimiters(ejs, path, '/');
#endif
    return (EjsVar*) ejsCreateString(ejs, path);
}


static EjsVar *setWorkingDir(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    char    *path;

    mprAssert(argc == 1 && ejsIsString(argv[0]));

    path = ejsGetString(argv[0]);

    if (chdir(path) < 0) {
        ejsThrowIOError(ejs, "Can't change the working directory");
    }
    return 0;
}

/*********************************** Factory **********************************/

void ejsCreateAppType(Ejs *ejs)
{
    EjsName     qname;

    ejsCreateCoreType(ejs, ejsName(&qname, "ejs.sys", "App"), ejs->objectType, sizeof(EjsObject), ES_ejs_sys_App,
        ES_ejs_sys_App_NUM_CLASS_PROP, ES_ejs_sys_App_NUM_INSTANCE_PROP, EJS_ATTR_NATIVE | EJS_ATTR_OBJECT_HELPERS);
}


void ejsConfigureAppType(Ejs *ejs)
{
    EjsType         *type;

    type = ejsGetType(ejs, ES_ejs_sys_App);

    ejsBindMethod(ejs, type, ES_ejs_sys_App_args, (EjsNativeFunction) getArgs);
    ejsBindMethod(ejs, type, ES_ejs_sys_App_dir, (EjsNativeFunction) getDir);
    ejsBindMethod(ejs, type, ES_ejs_sys_App_exit, (EjsNativeFunction) exitMethod);
    ejsBindMethod(ejs, type, ES_ejs_sys_App_noexit, (EjsNativeFunction) noexit);
    ejsBindMethod(ejs, type, ES_ejs_sys_App_serviceEvents, (EjsNativeFunction) serviceEvents);
    ejsBindMethod(ejs, type, ES_ejs_sys_App_sleep, (EjsNativeFunction) sleepProc);
    ejsBindMethod(ejs, type, ES_ejs_sys_App_workingDir, (EjsNativeFunction) workingDir);
    ejsBindMethod(ejs, type, ES_ejs_sys_App_set_workingDir, (EjsNativeFunction) setWorkingDir);

#if FUTURE
    (ejs, type, ES_ejs_sys_App_permissions, (EjsNativeFunction) getPermissions,
        ES_ejs_sys_App_set_permissions, (EjsNativeFunction) setPermissions);
#endif
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
