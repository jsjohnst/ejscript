/**
 *  ejsScope.c - Lookup variables in the scope chain.
 *
 *  This modules provides scope chain management including lookup, get and set services for variables. It will 
 *  lookup variables using the current execution variable scope and the set of open namespaces.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejs.h"

static int ejsLookupVarInNamespaces(Ejs *ejs, EjsVar *vp, EjsName *name, bool anySpace, EjsLookup *lookup);

/************************************* Code ***********************************/
/*
 *  Look for a variable by name in the scope chain and return the location in "lookup" and a positive slot number if found. 
 *  If the name.space is non-null/non-empty, then only the given namespace will be used. otherwise the set of open 
 *  namespaces will be used. The lookup structure will contain details about the location of the variable.
 */
int ejsLookupScope(Ejs *ejs, EjsName *name, bool anySpace, EjsLookup *lookup)
{
    EjsFrame        *frame;
    EjsBlock        *block;
    int             slotNum, nth;

    mprAssert(ejs);
    mprAssert(name);
    mprAssert(lookup);

    slotNum = -1;
    frame = ejs->frame;

    /*
     *  Look for the name in the scope chain considering each block scope. LookupVar will consider base classes and 
     *  namespaces. Don't search the last scope chain entry which will be global. For cloned interpreters, global 
     *  will belong to the master interpreter, so we must do that explicitly below to get the right global.
     */
    for (nth = 0, block = &frame->function.block; block->scopeChain; block = block->scopeChain) {

        if (block == (EjsBlock*) frame->thisObj->type) {
            /*
             *  This will lookup the instance and all base classes
             */
            if ((slotNum = ejsLookupVar(ejs, frame->thisObj, name, anySpace, lookup)) >= 0) {
                lookup->nthBlock = nth;
                break;
            }
            
        } else {
            if ((slotNum = ejsLookupVar(ejs, (EjsVar*) block, name, anySpace, lookup)) >= 0) {
                lookup->nthBlock = nth;
                break;
            }
        }
        nth++;
    }

    if (slotNum < 0 && ((slotNum = ejsLookupVar(ejs, ejs->global, name, anySpace, lookup)) >= 0)) {
        lookup->nthBlock = nth;
    }

    lookup->slotNum = slotNum;

    return slotNum;
}


/*
 *  Find a property in an object or type and its base classes.
 */
int ejsLookupVar(Ejs *ejs, EjsVar *vp, EjsName *name, bool anySpace, EjsLookup *lookup)
{
    EjsType     *type;
    int         slotNum;

    mprAssert(vp);
    mprAssert(vp->type);
    mprAssert(name);

    /*
     *  OPT - bit field initialization
     */
    lookup->nthBase = 0;
    lookup->nthBlock = 0;
    lookup->useThis = 0;
    lookup->instanceProperty = 0;
    lookup->ownerIsType = 0;

    /*
     *  Search through the inheritance chain of base classes. nthBase counts the subtypes that must be traversed. 
     */
    for (slotNum = -1, lookup->nthBase = 0; vp; lookup->nthBase++) {

        if ((slotNum = ejsLookupVarInBlock(ejs, vp, name, anySpace, lookup)) >= 0) {
            break;
        }

        /*
         *  Follow the base type chain. If an instance, first base type is vp->type.
         */
        vp = (vp->isType) ? (EjsVar*) ((EjsType*) vp)->baseType: (EjsVar*) vp->type;
        type = (EjsType*) vp;
        if (type == 0 || type->skipScope) {
            break;
        }
    }

    return lookup->slotNum = slotNum;
}


/*
 *  Find a variable in a block. Scope blocks are provided by the global object, types, functions and statement blocks.
 */
int ejsLookupVarInBlock(Ejs *ejs, EjsVar *vp, EjsName *name, bool anySpace, EjsLookup *lookup)
{
    int     slotNum;

    mprAssert(vp);
    mprAssert(name);
    mprAssert(name->name);
    mprAssert(name->space);
    mprAssert(lookup);

    /*
     *  This code is ordered according the the frequency of various patterns of access. Change with care!
     */
    lookup->obj = vp;
    lookup->name = *name;

    if (name->space[0] || !anySpace) {
        /*
         *  If doing a search with an explicit namespace or not doing a namespace search or the type doesn't have or use
         *  namespaces, then ignore the set of open namespaces.
         */
        return ejsLookupProperty(ejs, vp, name);

    } else if (ejsIsObject(vp) && !ejsIsArray(vp)) {            //  TODO - fix this Array
        /*
         *  Fast lookup. This will return a valid slot number ONLY if there is only one property of the given name AND 
         *  that property has been defined with a standard (global) namespace. This catches 90% of scope lookups.
         */
        slotNum = ejsLookupSingleProperty(ejs, (EjsObject*) vp, &lookup->name);
        if (slotNum >= -1) {
            return slotNum;
        }
    }

    return ejsLookupVarInNamespaces(ejs, vp, name, anySpace, lookup);
}


/*
 *  Find a variable in a block. Scope blocks are provided by the global object, types, functions and statement blocks.
 */
static int ejsLookupVarInNamespaces(Ejs *ejs, EjsVar *vp, EjsName *name, bool anySpace, EjsLookup *lookup)
{
    EjsNamespace    *nsp;
    EjsName         qname;
    EjsBlock        *block, *currentBlock, *b;
    EjsVar          *owner;
    int             slotNum, nextNsp;

    mprAssert(vp);
    mprAssert(name);
    mprAssert(name->name);
    mprAssert(name->space);
    mprAssert(lookup);

    slotNum = -1;
    qname = *name;

    currentBlock = &ejs->frame->function.block;
    for (block = currentBlock; block; block = block->scopeChain) {

        for (b = currentBlock; b; b = b->scopeChain) {
            for (nextNsp = -1; (nsp = (EjsNamespace*) ejsGetPrevItem(&b->namespaces, &nextNsp)) != 0; ) {

                if (nsp->flags & EJS_NSP_PROTECTED && vp->isType) {
                    /*
                     *  Protected access. See if the type containing the method we are executing is a sub class of the type 
                     *  containing the property ie. Can we see protected properties?
                     */
                    owner = (EjsVar*) ejs->frame->function.owner;
                    if (owner && !ejsIsA(ejs, owner, (EjsType*) vp)) {
                        continue;
                    }
                }

                qname.space = nsp->uri;
                slotNum = ejsLookupProperty(ejs, vp, &qname);
                if (slotNum >= 0) {
                    lookup->name = qname;
                    lookup->obj = vp;
                    lookup->slotNum = slotNum;
                    return slotNum;
                }
            }
            if (b == block) {
                break;
            }
        }
    }
    return -1;
}


/*
 *  Get a variable by name. If vp is specified, it contains an explicit object in which to search for the variable name. 
 *  Otherwise, the full execution scope is consulted. The lookup fields will be set as residuals.
 */
EjsVar *ejsGetVarByName(Ejs *ejs, EjsVar *vp, EjsName *name, bool anySpace, EjsLookup *lookup)
{
    EjsVar  *result;
    int     slotNum;

    mprAssert(ejs);
    mprAssert(name);

    //  TODO - really nice to remove this
    if (vp && vp->type->helpers->getPropertyByName) {
        result = (*vp->type->helpers->getPropertyByName)(ejs, vp, name);
        if (result) {
            return result;
        }
    }

    if (vp) {
        slotNum = ejsLookupVar(ejs, vp, name, anySpace, lookup);
    } else {
        slotNum = ejsLookupScope(ejs, name, anySpace, lookup);
    }
    if (slotNum < 0) {
        return ejs->undefinedValue;
    }
    return ejsGetProperty(ejs, lookup->obj, slotNum);
}


void ejsShowBlockScope(Ejs *ejs, EjsBlock *block)
{
#if BLD_DEBUG
    EjsNamespace    *nsp;
    EjsList         *namespaces;
    int             nextNsp;

    mprLog(ejs, 6, "\n  Block scope");
    for (; block; block = block->scopeChain) {
        mprLog(ejs, 6, "    Block \"%s\" 0x%08x", block->obj.var.debugName, block);
        namespaces = &block->namespaces;
        if (namespaces) {
            for (nextNsp = 0; (nsp = (EjsNamespace*) ejsGetNextItem(namespaces, &nextNsp)) != 0; ) {
                mprLog(ejs, 6, "        \"%s\"", nsp->uri);
            }
        }
    }
#endif
}


void ejsShowCurrentScope(Ejs *ejs)
{
#if BLD_DEBUG
    EjsNamespace    *nsp;
    EjsList         *namespaces;
    EjsBlock        *block;
    int             nextNsp;

    if (ejs->frame == 0) {
        return;
    }

    mprLog(ejs, 6, "\n  Current scope");
    for (block = &ejs->frame->function.block; block; block = block->scopeChain) {
        mprLog(ejs, 6, "    Block \"%s\" 0x%08x", block->obj.var.debugName, block);
        namespaces = &block->namespaces;
        if (namespaces) {
            for (nextNsp = 0; (nsp = (EjsNamespace*) ejsGetNextItem(namespaces, &nextNsp)) != 0; ) {
                mprLog(ejs, 6, "        \"%s\"", nsp->uri);
            }
        }
    }
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
