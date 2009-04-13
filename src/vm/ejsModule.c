/**
 *  ejsModule.c - Ejscript module management
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejs.h"

/******************************************************************************/
/*
 *  Lookup a module name in the set of loaded modules
 */
EjsModule *ejsLookupModule(Ejs *ejs, cchar *name)
{
    EjsModule   *mp;
    int         next;

    for (next = 0; (mp = (EjsModule*) mprGetNextItem(ejs->modules, &next)) != 0; ) {
        if (strcmp(mp->name, name) == 0) {
            return mp;
        }
    }
    return 0;
}



int ejsAddModule(Ejs *ejs, EjsModule *mp)
{
    mprAssert(ejs->modules);
    return mprAddItem(ejs->modules, mp);
}



int ejsRemoveModule(Ejs *ejs, EjsModule *mp)
{
    mprAssert(ejs->modules);
    return mprRemoveItem(ejs->modules, mp);
}



MprList *ejsGetModuleList(Ejs *ejs)
{
    return ejs->modules;
}



int ejsCheckModuleLoaded(Ejs *ejs, cchar *name)
{
    EjsModule       *mp;

    mp = (EjsModule*) ejsLookupModule(ejs, name);

    if (mp) {
        if (mp->loaded) {
            return 1;
        }
        if (mp->compiling && strcmp(name, EJS_DEFAULT_MODULE) != 0) {
            ejsThrowStateError(ejs, "Attempt to load module \"%s\" that is currently being compiled.", name);
            return EJS_ERR;
        }
    }
    return 0;
}


/*
 *  TODO - do we really need the name as an arg here or can they be defined by their property names.
 */
EjsModule *ejsCreateModule(Ejs *ejs, cchar *name, cchar *url, cchar *pool, int poolSize)
{
    EjsModule   *mp;

    /*
     *  We can't use ejsCreateType as our instances need to be EjsModules and not just EjsTypes.
     */
    mp = (EjsModule*) mprAllocZeroed(ejs, sizeof(EjsModule));
    if (mp == 0) {
        mprAssert(mp);
        return 0;
    }

    //  TODO OPT - should these be interned
    mp->name = mprStrdup(mp, name);
    mp->url = mprStrdup(mp, url);

    //  TODO - manage the versions somewhere
    //  TODO - warn when running wrong version
    mp->version = 1;

    //  TODO - don't zero
    mp->constants = mprAllocZeroed(mp, sizeof(EjsConst));
    if (mp->constants == 0) {
        return 0;
    }

    //  TODO - should only be created by the compiler
    mp->constants->table = mprCreateHash(mp->constants, 0);

    if (pool) {
        mprStealBlock(mp, pool);
        mp->constants->pool = (char*) pool;
        mp->constants->size = poolSize;
        mp->constants->len = poolSize;
    }

    mp->scopeChain = ejs->globalBlock;

    return mp;
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
