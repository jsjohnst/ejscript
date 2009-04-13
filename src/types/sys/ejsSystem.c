/*
 *  ejsSystem.c -- System class
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejs.h"

/************************************ Methods *********************************/
#if BLD_FEATURE_CMD
#if ES_ejs_sys_System_run
/*
 *  function run(cmd: String): String
 */
static EjsVar *run(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    MprCmd      *cmd;
    EjsString   *result;
    char        *err, *output;
    int         status;

    mprAssert(argc == 1 && ejsIsString(argv[0]));

    cmd = mprCreateCmd(ejs);
    status = mprRunCmd(cmd, ejsGetString(argv[0]), &output, &err, 0);
    if (status) {
        ejsThrowError(ejs, "Command failed: %s, status %d", err, status);
        mprFree(cmd);
        return 0;
    }
    result = ejsCreateString(ejs, output);
    mprFree(cmd);
    return (EjsVar*) result;
}
#endif


//  TODO - refactor and rename
#if ES_ejs_sys_System_runx
/*
 *  function runx(cmd: String): Void
 */
static EjsVar *runx(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    MprCmd      *cmd;
    char        *err;
    int         status;

    mprAssert(argc == 1 && ejsIsString(argv[0]));

    cmd = mprCreateCmd(ejs);
    status = mprRunCmd(cmd, ejsGetString(argv[0]), NULL, &err, 0);
    if (status) {
        ejsThrowError(ejs, "Can't run command: %s\nDetails: %s", ejsGetString(argv[0]), err);
        mprFree(err);
    }
    mprFree(cmd);
    return 0;
}
#endif
#endif

/************************************ Factory *********************************/

void ejsCreateSystemType(Ejs *ejs)
{
    EjsName     qname;

    ejsCreateCoreType(ejs, ejsName(&qname, "ejs.sys", "System"), ejs->objectType, sizeof(EjsObject), ES_ejs_sys_System,
        ES_ejs_sys_System_NUM_CLASS_PROP, ES_ejs_sys_System_NUM_INSTANCE_PROP, EJS_ATTR_NATIVE | EJS_ATTR_OBJECT_HELPERS);
}


void ejsConfigureSystemType(Ejs *ejs)
{
    EjsType         *type;

    type = ejsGetType(ejs, ES_ejs_sys_System);

#if BLD_FEATURE_CMD
#if ES_ejs_sys_System_run
    ejsBindMethod(ejs, type, ES_ejs_sys_System_run, (EjsNativeFunction) run);
#endif
#if ES_ejs_sys_System_runx
    ejsBindMethod(ejs, type, ES_ejs_sys_System_runx, (EjsNativeFunction) runx);
#endif
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
