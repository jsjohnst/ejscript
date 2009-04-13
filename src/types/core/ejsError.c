/**
 *  ejsError.c - Error Exception class
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejs.h"

/************************************* Code ***********************************/
/*
 *  Cast the operand to the specified type
 *
 *  intrinsic function cast(type: Type) : Object
 */

static EjsVar *castError(Ejs *ejs, EjsError *vp, EjsType *type)
{
    EjsVar      *sp;
    char        *buf;

    switch (type->id) {

    case ES_Boolean:
        return (EjsVar*) ejsCreateBoolean(ejs, 1);

    case ES_String:
        if (mprAllocSprintf(ejs, &buf, 0, 
                "%s Exception: %s\nStack:\n%s\n", vp->obj.var.type->qname.name, vp->message, vp->stack) < 0) {
            ejsThrowMemoryError(ejs);
        }
        sp = (EjsVar*) ejsCreateString(ejs, buf);
        mprFree(buf);
        return sp;

    default:
        ejsThrowTypeError(ejs, "Unknown type");
        return 0;
    }
}


/*
 *  Get a property.
 */
static EjsVar *getErrorProperty(Ejs *ejs, EjsError *error, int slotNum)
{
    switch (slotNum) {
    case ES_Error_stack:
        return (EjsVar*) ejsCreateString(ejs, error->stack);

    case ES_Error_message:
        return (EjsVar*) ejsCreateString(ejs, error->message);
    }
    return (ejs->objectHelpers->getProperty)(ejs, (EjsVar*) error, slotNum);
}


/*********************************** Methods **********************************/
/*
 *  Error Constructor
 *
 *  public function Error(message: String = null)
 */
static EjsVar *errorConstructor(Ejs *ejs, EjsError *vp, int argc,  EjsVar **argv)
{
    mprFree(vp->message);
    if (argc == 0) {
        vp->message = mprStrdup(vp, "");
    } else {
        vp->message = mprStrdup(vp, ejsGetString(argv[0]));
    }

    mprFree(vp->stack);
    vp->stack = ejsFormatStack(ejs);

    return (EjsVar*) vp;
}


static EjsVar *getCode(Ejs *ejs, EjsError *vp, int argc,  EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, vp->code);
}


static EjsVar *setCode(Ejs *ejs, EjsError *vp, int argc,  EjsVar **argv)
{
    vp->code = ejsGetInt(argv[0]);
    return 0;
}



/************************************ Factory *********************************/

static EjsType *createErrorType(Ejs *ejs, cchar *name, int numClassProp, int numInstanceProp)
{
    EjsType     *type, *baseType;
    EjsName     qname;
    int         flags;

    flags = EJS_ATTR_NATIVE | EJS_ATTR_OBJECT | EJS_ATTR_DYNAMIC_INSTANCE | EJS_ATTR_OBJECT_HELPERS | 
        EJS_ATTR_HAS_CONSTRUCTOR;
    baseType = (ejs->errorType) ? ejs->errorType: ejs->objectType;

    type = ejsCreateCoreType(ejs, ejsName(&qname, EJS_INTRINSIC_NAMESPACE, name), baseType, sizeof(EjsError), 
        ES_Error, numClassProp, numInstanceProp, flags);
    type->helpers->castVar = (EjsCastVarHelper) castError;
    type->helpers->getProperty = (EjsGetPropertyHelper) getErrorProperty;

    return type;
}


static void defineType(Ejs *ejs, int slotNum)
{
    EjsType     *type;

    type = ejsGetType(ejs, slotNum);
    ejsBindMethod(ejs, type, type->block.numInherited, (EjsNativeFunction) errorConstructor);
}


void ejsCreateErrorType(Ejs *ejs)
{
    ejs->errorType = createErrorType(ejs, "Error",  ES_Error_NUM_CLASS_PROP, ES_Error_NUM_INSTANCE_PROP);

    createErrorType(ejs, "ArgError", ES_ArgError_NUM_CLASS_PROP, ES_ArgError_NUM_INSTANCE_PROP);
    createErrorType(ejs, "ArithmeticError", ES_ArithmeticError_NUM_CLASS_PROP, ES_ArithmeticError_NUM_INSTANCE_PROP);
    createErrorType(ejs, "AssertError", ES_AssertError_NUM_CLASS_PROP, ES_AssertError_NUM_INSTANCE_PROP);
    createErrorType(ejs, "InstructionError", ES_InstructionError_NUM_CLASS_PROP, ES_InstructionError_NUM_INSTANCE_PROP);
    createErrorType(ejs, "IOError", ES_IOError_NUM_CLASS_PROP, ES_IOError_NUM_INSTANCE_PROP);
    createErrorType(ejs, "InternalError", ES_InternalError_NUM_CLASS_PROP, ES_InternalError_NUM_INSTANCE_PROP);
    createErrorType(ejs, "MemoryError", ES_MemoryError_NUM_CLASS_PROP, ES_MemoryError_NUM_INSTANCE_PROP);
    createErrorType(ejs, "OutOfBoundsError", ES_OutOfBoundsError_NUM_CLASS_PROP, ES_OutOfBoundsError_NUM_INSTANCE_PROP);
    createErrorType(ejs, "ReferenceError", ES_ReferenceError_NUM_CLASS_PROP, ES_ReferenceError_NUM_INSTANCE_PROP);
    createErrorType(ejs, "ResourceError", ES_ResourceError_NUM_CLASS_PROP, ES_ResourceError_NUM_INSTANCE_PROP);
    
#if ES_SecurityError
    createErrorType(ejs, "SecurityError", ES_SecurityError_NUM_CLASS_PROP, ES_SecurityError_NUM_INSTANCE_PROP);
#endif
    createErrorType(ejs, "StateError", ES_StateError_NUM_CLASS_PROP, ES_StateError_NUM_INSTANCE_PROP);
    createErrorType(ejs, "SyntaxError", ES_SyntaxError_NUM_CLASS_PROP, ES_SyntaxError_NUM_INSTANCE_PROP);
    createErrorType(ejs, "TypeError", ES_TypeError_NUM_CLASS_PROP, ES_TypeError_NUM_INSTANCE_PROP);
    createErrorType(ejs, "URIError", ES_URIError_NUM_CLASS_PROP, ES_URIError_NUM_INSTANCE_PROP);
}


void ejsConfigureErrorType(Ejs *ejs)
{
    defineType(ejs, ES_Error);

    ejsBindMethod(ejs, ejs->errorType, ES_Error_code, (EjsNativeFunction) getCode);
    ejsBindMethod(ejs, ejs->errorType, ES_Error_set_code, (EjsNativeFunction) setCode);

    defineType(ejs, ES_ArgError);
    defineType(ejs, ES_ArithmeticError);
    defineType(ejs, ES_AssertError);
    defineType(ejs, ES_InstructionError);
    defineType(ejs, ES_IOError);
    defineType(ejs, ES_InternalError);
    defineType(ejs, ES_MemoryError);
    defineType(ejs, ES_OutOfBoundsError);
    defineType(ejs, ES_ReferenceError);
    defineType(ejs, ES_ResourceError);
#if ES_SecurityError
    defineType(ejs, ES_SecurityError);
#endif
    defineType(ejs, ES_StateError);
    defineType(ejs, ES_SyntaxError);
    defineType(ejs, ES_TypeError);
    defineType(ejs, ES_URIError);
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
