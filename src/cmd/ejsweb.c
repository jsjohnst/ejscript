/**
 *  ejsweb.c - Ejs Web Framework Application Manager and Generator
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejs.h"
#include    "ecCompiler.h"

/************************************ Code ************************************/

int main(int argc, char **argv)
{
    EjsService  *vm;
    EcCompiler  *cp;
    Ejs         *ejs;
    Mpr         *mpr;
    char        *path, dir[MPR_MAX_FNAME];

    mpr = mprCreate(argc, argv, NULL);
    mprSetAppName(mpr, argv[0], "Embedthis Ejsweb", BLD_VERSION);

    if (mprStart(mpr, 0) < 0) {
        mprError(mpr, "Can't start mpr services");
        return MPR_ERR_CANT_INITIALIZE;
    }

    //  TODO - convert these both to not use static buffers
    mprGetAppDir(mpr, dir, sizeof(dir));

	path = mprSearchForFile(mpr, EJS_EJSWEB ".mod", MPR_SEARCH_EJSMOD, 
		dir, MPR_SEARCH_DELIM,                          //  Search in same dir as the application. (Works for windows)
        dir, "/../lib/modules", MPR_SEARCH_DELIM,       //  Search in relative mod dir. (Works for Unix dev tree)
        BLD_MOD_PREFIX, MPR_SEARCH_DELIM,               //  Search in standard MOD_DIR. (Works for Unix installed)
        BLD_ABS_MOD_DIR, NULL);                         //  Search in dev bin dir
	if (path == 0) {
        mprError(mpr, "Can't find %s module file %s.mod", EJS_EJSWEB, EJS_EJSWEB);
        return MPR_ERR_CANT_ACCESS;
	}

    if ((vm = ejsCreateService(mpr)) == 0) {
        mprFree(mpr);
        return MPR_ERR_NO_MEMORY;
    }
    if ((ejs = ejsCreate(vm, NULL, 0)) == 0) {
        mprFree(mpr);
        return MPR_ERR_NO_MEMORY;
    }
    ejs->argc = argc;
    ejs->argv = argv;

    if ((cp = ecCreateCompiler(ejs, EC_FLAGS_BIND_GLOBALS, EJS_SPEC_FIXED)) == 0) {
        mprFree(mpr);
        return MPR_ERR_NO_MEMORY;
    }

    if (ejsLoadModule(ejs, path, NULL, NULL, 0) < 0) {
        mprFree(mpr);
        return MPR_ERR_CANT_READ;
    }
    if (ejsRun(ejs) < 0) {
        mprFree(mpr);
        return EJS_ERR;
    }
    if (ejs->exception) {
        ejsReportError(ejs, EJS_EJSWEB);
    }
    mprFree(mpr);
    return 0;
}


#if OLD
static char *search(MprCtx ctx, cchar *file, cchar *searchPath)
{
	char	path[MPR_MAX_FNAME], *search, *tok, *dir;

    search = mprStrdup(ctx, searchPath);
    dir = mprStrTok(search, MPR_SEARCH_DELIM, &tok);
    while (dir && *dir) {
        mprSprintf(path, sizeof(path), "%s/%s", dir, file);
		if (mprAccess(ctx, path, R_OK)) {
			mprCleanFilename(ctx, path);
            return mprStrdup(ctx, path);
        }
        dir = mprStrTok(0, MPR_SEARCH_DELIM, &tok);
    }
    mprFree(search);
    return 0;
}
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
