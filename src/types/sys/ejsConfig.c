/*
 *  ejsConfig.c -- Config class
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejs.h"

/*********************************** Methods **********************************/

void ejsCreateConfigType(Ejs *ejs)
{
    EjsName     qname;

    ejsCreateCoreType(ejs, ejsName(&qname, "ejs.sys", "Config"), ejs->objectType, sizeof(EjsObject), 
        ES_Config, ES_ejs_sys_Config_NUM_CLASS_PROP, ES_ejs_sys_Config_NUM_INSTANCE_PROP, 
        EJS_ATTR_NATIVE | EJS_ATTR_OBJECT_HELPERS);
}


void ejsConfigureConfigType(Ejs *ejs)
{
    EjsVar      *vp;
    char        version[16];

    vp = (EjsVar*) ejsGetType(ejs, ES_Config);
    if (vp == 0) {
        return;
    }
    ejsSetProperty(ejs, vp, ES_ejs_sys_Config_Debug, BLD_DEBUG ? (EjsVar*) ejs->oneValue: (EjsVar*) ejs->zeroValue);
    ejsSetProperty(ejs, vp, ES_ejs_sys_Config_CPU, (EjsVar*) ejsCreateString(ejs, BLD_HOST_CPU));
    ejsSetProperty(ejs, vp, ES_ejs_sys_Config_DB, BLD_FEATURE_EJS_DB ? (EjsVar*) ejs->oneValue: (EjsVar*) ejs->zeroValue);
    ejsSetProperty(ejs, vp, ES_ejs_sys_Config_E4X, BLD_FEATURE_EJS_E4X ? (EjsVar*) ejs->oneValue: (EjsVar*) ejs->zeroValue);
    ejsSetProperty(ejs, vp, ES_ejs_sys_Config_Floating, 
        BLD_FEATURE_FLOATING_POINT ? (EjsVar*) ejs->oneValue: (EjsVar*) ejs->zeroValue);
    ejsSetProperty(ejs, vp, ES_ejs_sys_Config_Http, BLD_FEATURE_HTTP ? (EjsVar*) ejs->oneValue: (EjsVar*) ejs->zeroValue);

    if (BLD_FEATURE_EJS_LANG == EJS_SPEC_ECMA) {
        ejsSetProperty(ejs, vp, ES_ejs_sys_Config_Lang, (EjsVar*) ejsCreateString(ejs, "ecma"));
    } else if (BLD_FEATURE_EJS_LANG == EJS_SPEC_PLUS) {
        ejsSetProperty(ejs, vp, ES_ejs_sys_Config_Lang, (EjsVar*) ejsCreateString(ejs, "plus"));
    } else {
        ejsSetProperty(ejs, vp, ES_ejs_sys_Config_Lang, (EjsVar*) ejsCreateString(ejs, "fixed"));
    }

    ejsSetProperty(ejs, vp, ES_ejs_sys_Config_Legacy, 
        BLD_FEATURE_LEGACY_API ? (EjsVar*) ejs->oneValue: (EjsVar*) ejs->zeroValue);
    ejsSetProperty(ejs, vp, ES_ejs_sys_Config_Multithread, 
        BLD_FEATURE_MULTITHREAD ? (EjsVar*) ejs->oneValue: (EjsVar*) ejs->zeroValue);

    ejsSetProperty(ejs, vp, ES_ejs_sys_Config_NumberType, 
        (EjsVar*) ejsCreateString(ejs, MPR_STRINGIFY(BLD_FEATURE_NUM_TYPE)));

    ejsSetProperty(ejs, vp, ES_ejs_sys_Config_OS, (EjsVar*) ejsCreateString(ejs, BLD_OS));
    ejsSetProperty(ejs, vp, ES_ejs_sys_Config_Product, (EjsVar*) ejsCreateString(ejs, BLD_PRODUCT));
    ejsSetProperty(ejs, vp, ES_ejs_sys_Config_RegularExpressions, 
        BLD_FEATURE_REGEXP ? (EjsVar*) ejs->trueValue: (EjsVar*) ejs->falseValue);
    ejsSetProperty(ejs, vp, ES_ejs_sys_Config_Title, (EjsVar*) ejsCreateString(ejs, BLD_NAME));

    mprSprintf(version, sizeof(version), "%s-%s", BLD_VERSION, BLD_NUMBER);
    ejsSetProperty(ejs, vp, ES_ejs_sys_Config_Version, (EjsVar*) ejsCreateString(ejs, version));

#if BLD_WIN_LIKE
{
	/*
	 *	Users may install Ejscript in a different location
	 */
	char	path[MPR_MAX_FNAME], dir[MPR_MAX_FNAME];
	mprGetAppPath(ejs, path, sizeof(path));
	mprGetDirName(dir, sizeof(dir), path);
    ejsSetProperty(ejs, vp, ES_ejs_sys_Config_BinDir, (EjsVar*) ejsCreateString(ejs, dir));
    ejsSetProperty(ejs, vp, ES_ejs_sys_Config_LibDir, (EjsVar*) ejsCreateString(ejs, dir));
}
#else
    ejsSetProperty(ejs, vp, ES_ejs_sys_Config_BinDir, (EjsVar*) ejsCreateString(ejs, BLD_BIN_PREFIX));
    ejsSetProperty(ejs, vp, ES_ejs_sys_Config_LibDir, (EjsVar*) ejsCreateString(ejs, BLD_LIB_PREFIX));
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
