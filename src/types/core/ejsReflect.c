/**
 *  ejsReflect.c - Reflection class and API
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejs.h"

#if ES_Reflect
/*********************************** Methods **********************************/
/*
 *  Constructor
 *
 *  public function Reflect(o: Object)
 */
static EjsVar *reflectConstructor(Ejs *ejs, EjsReflect *rp, int argc,  EjsVar **argv)
{
    mprAssert(argc == 1);
    rp->subject = argv[0];
    return (EjsVar*) rp;
}


/*
 *  Get the name of the subject
 *
 *  function get name(): String
 */
static EjsVar *getReflectedName(Ejs *ejs, EjsReflect *rp, int argc, EjsVar **argv)
{
    EjsFunction     *fun;
    EjsName         qname;
    EjsVar      *vp;

    mprAssert(argc == 0);

    vp = rp->subject;

    if (ejsIsType(vp)) {
        return (EjsVar*) ejsCreateString(ejs, ((EjsType*) vp)->qname.name);

    } else if (ejsIsFunction(vp)) {
        fun = (EjsFunction*) vp;
        qname = ejsGetPropertyName(ejs, fun->owner, fun->slotNum);
        return (EjsVar*) ejsCreateString(ejs, qname.name);

    } else {
        return (EjsVar*) ejsCreateString(ejs, vp->type->qname.name);
    }
    return ejs->undefinedValue;
}


/*
 *  Get the type of the object.
 *
 *  function get type(): Object
 */
static EjsVar *getReflectedType(Ejs *ejs, EjsReflect *rp, int argc, EjsVar **argv)
{
    EjsType     *type;
    EjsVar      *vp;

    vp = rp->subject;

    if (ejsIsType(vp)) {
        type = (EjsType*) vp;
        if (type->baseType) {
            return (EjsVar*) type->baseType;
        } else {
            return (EjsVar*) ejs->undefinedValue;
        }
    }
    return (EjsVar*) vp->type;
}


/*
 *  Get the type name of the subject
 *
 *  function get typeName(): String
 */
static EjsVar *getReflectedTypeName(Ejs *ejs, EjsReflect *rp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsGetTypeName(ejs, rp->subject);
}


/*
 *  Return the type name of a var as a string. If the var is a type, get the base type.
 */
EjsVar *ejsGetTypeName(Ejs *ejs, EjsVar *vp)
{
    EjsType     *type;

    if (vp == 0) {
        return ejs->undefinedValue;
    }
    if (ejsIsType(vp)) {
        type = (EjsType*) vp;
        if (type->baseType) {
            return (EjsVar*) ejsCreateString(ejs, type->baseType->qname.name);
        } else {
            /* NOTE: the base type of Object is null */
            return (EjsVar*) ejs->nullValue;
        }
    }
    return (EjsVar*) ejsCreateString(ejs, vp->type->qname.name);
}


/*
 *  Get the ecma "typeof" value for an object. Unfortunately, typeof is pretty lame.
 */
EjsVar *ejsGetTypeOf(Ejs *ejs, EjsVar *vp)
{
    if (vp == ejs->undefinedValue) {
        return (EjsVar*) ejsCreateString(ejs, "undefined");

    } else if (vp == ejs->nullValue) {
        /* Yea - I know, ECMAScript is broken */
        return (EjsVar*) ejsCreateString(ejs, "object");

    } if (ejsIsBoolean(vp)) {
        return (EjsVar*) ejsCreateString(ejs, "boolean");

    } else if (ejsIsNumber(vp)) {
        return (EjsVar*) ejsCreateString(ejs, "number");

    } else if (ejsIsString(vp)) {
        return (EjsVar*) ejsCreateString(ejs, "string");

    } else if (ejsIsType(vp)) {
        /* Pretend it is a constructor function */
        return (EjsVar*) ejsCreateString(ejs, "function");
               
    } else {
        return (EjsVar*) ejsCreateString(ejs, "object");
    }
}


/*********************************** Factory **********************************/

void ejsCreateReflectType(Ejs *ejs)
{
    EjsName     qname;

    ejsCreateCoreType(ejs, ejsName(&qname, EJS_INTRINSIC_NAMESPACE, "Reflect"), ejs->objectType, sizeof(EjsReflect),
        ES_Reflect, ES_Reflect_NUM_CLASS_PROP, ES_Reflect_NUM_INSTANCE_PROP, EJS_ATTR_NATIVE | EJS_ATTR_HAS_CONSTRUCTOR);
}


void ejsConfigureReflectType(Ejs *ejs)
{
    EjsType     *type;

    type = ejsGetType(ejs, ES_Reflect);

    ejsBindMethod(ejs, type, type->block.numInherited, (EjsNativeFunction) reflectConstructor);
    ejsBindMethod(ejs, type, ES_Reflect_name, (EjsNativeFunction) getReflectedName);
    ejsBindMethod(ejs, type, ES_Reflect_type, (EjsNativeFunction) getReflectedType);
    ejsBindMethod(ejs, type, ES_Reflect_typeName, (EjsNativeFunction) getReflectedTypeName);
}
#endif /* ES_Reflect */


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
