/**
 *  ejsIterator.c - Iterator class
 *
 *  This provides a high performance iterator construction for native classes.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejs.h"

/************************************ Code ************************************/

static void markIteratorVar(Ejs *ejs, EjsVar *parent, EjsIterator *ip)
{
    if (ip->target) {
        ejsMarkVar(ejs, (EjsVar*) ip, ip->target);
    }
}


/*
 *  Call the supplied next() function to return the next enumerable item
 */
static EjsVar *nextIterator(Ejs *ejs, EjsIterator *ip, int argc, EjsVar **argv)
{
    if (ip->nativeNext) {
        return (ip->nativeNext)(ejs, (EjsVar*) ip, argc, argv);
    } else {
        ejsThrowStopIteration(ejs);
        return 0;
    }
#if UNUSED && TODO
    if (ip->nativeNext) {
        return (ip->nativeNext)(ejs, (EjsVar*) ip, argc, argv);
    }
    return ejsRunFunction(ejs, ip->next, ip->target, argc, argv);
#endif
}


/*
 *  Throw the StopIteration object
 */
EjsVar *ejsThrowStopIteration(Ejs *ejs)
{
    return ejsThrowException(ejs, (EjsVar*) ejs->stopIterationType);
}


/*********************************** Methods **********************************/
#if UNUSED
/*
 *  Constructor to create an iterator using a scripted next().
 *
 *  public function Iterator(obj, f,    TODO ECMA deep, ...namespaces)
 */
static EjsVar *iteratorConstructor(Ejs *ejs, EjsIterator *ip, int argc, EjsVar **argv)
{
    if (argc != 2 || !ejsIsFunction(argv[1])) {
        ejsThrowArgError(ejs, "usage: Iterator(obj, function)");
        return 0;
    }
    ip->target = argv[0];
    ip->next = (EjsFunction*) argv[1];
    mprAssert(ip->nativeNext == 0);

    return (EjsVar*) ip;
}
#endif


/*********************************** Factory **********************************/
/*
 *  Create an iterator.
 */
EjsIterator *ejsCreateIterator(Ejs *ejs, EjsVar *obj, EjsNativeFunction nativeNext, bool deep, EjsArray *namespaces)
{
    EjsIterator     *ip;

    ip = (EjsIterator*) ejsCreateVar(ejs, ejs->iteratorType, 0);
    if (ip) {
        ip->nativeNext = nativeNext;
        ip->target = obj;
        ip->deep = deep;
        ip->namespaces = namespaces;
        ejsSetDebugName(ip, "iterator");
    }
    return ip;
}


/*
 *  Create the Iterator and StopIteration types
 */
void ejsCreateIteratorType(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;

    type = ejsCreateCoreType(ejs, ejsName(&qname, EJS_ITERATOR_NAMESPACE, "Iterator"), ejs->objectType, sizeof(EjsIterator),
        ES_Iterator, ES_Iterator_NUM_CLASS_PROP, ES_Iterator_NUM_INSTANCE_PROP, EJS_ATTR_NATIVE);
    ejs->iteratorType = type;

    type->helpers->markVar  = (EjsMarkVarHelper) markIteratorVar;

    type = ejsCreateCoreType(ejs, ejsName(&qname, EJS_ITERATOR_NAMESPACE, "StopIteration"), ejs->objectType, sizeof(EjsVar), 
        ES_StopIteration, ES_StopIteration_NUM_CLASS_PROP,  ES_StopIteration_NUM_INSTANCE_PROP, EJS_ATTR_NATIVE);
    ejs->stopIterationType = type;
}


void ejsConfigureIteratorType(Ejs *ejs)
{
    EjsType     *type;

    type = ejs->iteratorType;

    /*
     *  Define the "next" method
     */
    ejsBindMethod(ejs, ejs->iteratorType, ES_Iterator_next, (EjsNativeFunction) nextIterator);
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
