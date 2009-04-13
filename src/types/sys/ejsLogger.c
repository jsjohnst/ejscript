/*
 *  ejsLogger.c - Logger class 
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */
/********************************** Includes **********************************/

#include    "ejs.h"

#if UNUSED
/*********************************** Usage ************************************/
/*
 *  System.Log.setLog(path);
 *  System.Log.enable;
 */
/******************************************************************************/

static void logHandler(MPR_LOC_DEC(ctx, loc), int flags, int level, 
    const char *msg)
{
    Mpr *app;
    char    *buf;
    int     len;

    app = mprGetApp(ctx);
    if (app->logFile == 0) {
        return;
    }

    if (flags & MPR_LOG_SRC) {
        len = mprAllocSprintf(MPR_LOC_PASS(ctx, loc), &buf, 0, 
            "Log %d: %s\n", level, msg);

    } else if (flags & MPR_ERROR_SRC) {
        len = mprAllocSprintf(MPR_LOC_PASS(ctx, loc), &buf, 0, 
            "Error: %s\n", msg);

    } else if (flags & MPR_FATAL_SRC) {
        len = mprAllocSprintf(MPR_LOC_PASS(ctx, loc), &buf, 0, 
            "Fatal: %s\n", msg);
        
    } else if (flags & MPR_ASSERT_SRC) {
#if BLD_FEATURE_ALLOC_LEAK_TRACK
        len = mprAllocSprintf(MPR_LOC_PASS(ctx, loc), &buf, 0, 
            "Assertion %s, failed at %s\n",
            msg, loc);
#else
        len = mprAllocSprintf(MPR_LOC_PASS(ctx, loc), &buf, 0, 
            "Assertion %s, failed\n", msg);
#endif

    } else if (flags & MPR_RAW) {
        /* OPT */
        len = mprAllocSprintf(MPR_LOC_PASS(ctx, loc), &buf, 0, 
            "%s", msg);

    } else {
        return;
    }

    mprPuts(app->logFile, buf, len);

    mprFree(buf);
}

/******************************************************************************/
/************************************ Methods *********************************/
/******************************************************************************/
/*
 *  function int setLog(string path)
 */

static int setLog(EjsFiber *fp, EjsVar *thisObj, int argc, EjsVar **argv)
{
    const char  *path;
    MprFile     *file;
    Mpr     *app;

    if (argc != 1 || !ejsVarIsString(argv[0])) {
        ejsArgError(fp, "Usage: setLog(path)");
        return -1;
    }

    app = mprGetApp(fp);

    /*
     *  Ignore errors if we can't create the log file.
     *  Use the app context so this will live longer than the interpreter
     *  MOB -- this leaks files.
     */
    path = argv[0]->string;
    file = mprOpen(app, path, O_CREAT | O_TRUNC | O_WRONLY, 0664);
    if (file) {
        app->logFile = file;
        mprSetLogHandler(fp, logHandler);
    }
    mprLog(fp, 0, "Test log");

    return 0;
}

/******************************************************************************/
#if UNUSED

static int enableSetAccessor(EjsFiber *fp, EjsVar *thisObj, int argc, EjsVar **argv)
{
    if (argc != 1) {
        ejsArgError(fp, "Usage: set(value)");
        return -1;
    }
    ejsSetProperty(fp, thisObj, "_enabled", argv[0]);
    return 0;
}

/******************************************************************************/

static int enableGetAccessor(EjsFiber *fp, EjsVar *thisObj, int argc, EjsVar **argv)
{
    ejsSetReturnValue(fp, ejsGetPropertyAsVar(fp, thisObj, "_enabled"));
    return 0;
}

#endif
/******************************************************************************/
/******************************** Initialization ******************************/
/******************************************************************************/

int ejsDefineLogClass(EjsFiber *fp)
{
    EjsVar          *logClass;

    logClass =  ejsDefineClass(fp, "System.Log", "Object", 0);
    if (logClass == 0) {
        return MPR_ERR_CANT_INITIALIZE;
    }

    ejsDefineCMethod(fp, logClass, "setLog", setLog, 0);

#if UNUSED
    EjsProperty     *pp;
    ejsDefineCAccessors(fp, logClass, "enable", enableSetAccessor, 
        enableGetAccessor, 0);

    pp = ejsSetPropertyToBoolean(fp, logClass, "_enabled", 0);
    ejsMakePropertyEnumerable(pp, 0);
#endif

    return ejsObjHasErrors(logClass) ? MPR_ERR_CANT_INITIALIZE : 0;
}

/******************************************************************************/
#else
void __dummyEjsLog() {}
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
