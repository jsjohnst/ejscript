/**
 *  ejsVoid.c - Ejscript Void class (aka undefined)
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejs.h"

/********************************* Void Helpers *******************************/
/*
 *  Cast the void operand to a primitive type
 */

static EjsVar *castVoid(Ejs *ejs, EjsVoid *vp, EjsType *type)
{
    switch (type->id) {
    case ES_Boolean:
        return (EjsVar*) ejs->falseValue;

    case ES_Number:
        return (EjsVar*) ejs->nanValue;

    case ES_Object:
        return vp;

    case ES_String:
        return (EjsVar*) ejsCreateString(ejs, "undefined");

    default:
        ejsThrowTypeError(ejs, "Can't cast to this type");
        return 0;
    }
}



static EjsVar *coerceVoidOperands(Ejs *ejs, EjsVoid *lhs, int opcode, EjsVoid *rhs)
{
    switch (opcode) {

    case EJS_OP_ADD:
        if (!ejsIsNumber(rhs)) {
            return ejsInvokeOperator(ejs, (EjsVar*) ejsToString(ejs, lhs), opcode, rhs);
        }
        /* Fall through */

    case EJS_OP_AND: case EJS_OP_DIV: case EJS_OP_MUL: case EJS_OP_OR: case EJS_OP_REM:
    case EJS_OP_SHL: case EJS_OP_SHR: case EJS_OP_SUB: case EJS_OP_USHR: case EJS_OP_XOR:
        return ejsInvokeOperator(ejs, (EjsVar*) ejs->nanValue, opcode, rhs);

    /*
     *  Comparision
     */
    case EJS_OP_COMPARE_LE: case EJS_OP_COMPARE_LT:
    case EJS_OP_COMPARE_GE: case EJS_OP_COMPARE_GT:
        return (EjsVar*) ejs->falseValue;

    case EJS_OP_COMPARE_NE:
    case EJS_OP_COMPARE_STRICTLY_NE:
        if (ejsIsNull(rhs)) {
            return (EjsVar*) ejs->falseValue;
        }
        return (EjsVar*) ejs->trueValue;


    case EJS_OP_COMPARE_EQ:
    case EJS_OP_COMPARE_STRICTLY_EQ:
        if (ejsIsNull(rhs)) {
            return (EjsVar*) ejs->trueValue;
        }
        return (EjsVar*) ejs->falseValue;

    /*
     *  Unary operators
     */
    case EJS_OP_LOGICAL_NOT: case EJS_OP_NOT: case EJS_OP_NEG:
        return 0;

    case EJS_OP_COMPARE_UNDEFINED:
    case EJS_OP_COMPARE_NOT_ZERO:
    case EJS_OP_COMPARE_NULL:
        return (EjsVar*) ejs->trueValue;

    case EJS_OP_COMPARE_FALSE:
    case EJS_OP_COMPARE_TRUE:
    case EJS_OP_COMPARE_ZERO:
        return (EjsVar*) ejs->falseValue;

    default:
        ejsThrowTypeError(ejs, "Opcode %d not valid for type %s", opcode, lhs->type->qname.name);
        return ejs->undefinedValue;
    }
    return 0;
}



static EjsVar *invokeVoidOperator(Ejs *ejs, EjsVoid *lhs, int opcode, EjsVoid *rhs)
{
    EjsVar      *result;

    if (rhs == 0 || lhs->type != rhs->type) {
        if ((result = coerceVoidOperands(ejs, lhs, opcode, rhs)) != 0) {
            return result;
        }
    }

    /*
     *  Types match, left and right types are both "undefined"
     */
    switch (opcode) {

    case EJS_OP_COMPARE_EQ: case EJS_OP_COMPARE_STRICTLY_EQ:
    case EJS_OP_COMPARE_LE: case EJS_OP_COMPARE_GE:
    case EJS_OP_COMPARE_UNDEFINED:
    case EJS_OP_COMPARE_NOT_ZERO:
    case EJS_OP_COMPARE_NULL:
        return (EjsVar*) ejs->trueValue;

    case EJS_OP_COMPARE_NE: case EJS_OP_COMPARE_STRICTLY_NE:
    case EJS_OP_COMPARE_LT: case EJS_OP_COMPARE_GT:
    case EJS_OP_COMPARE_FALSE:
    case EJS_OP_COMPARE_TRUE:
    case EJS_OP_COMPARE_ZERO:
        return (EjsVar*) ejs->falseValue;

    /*
     *  Unary operators
     */
    case EJS_OP_LOGICAL_NOT: case EJS_OP_NOT: case EJS_OP_NEG:
        return (EjsVar*) ejs->nanValue;

    /*
     *  Binary operators
     */
    case EJS_OP_ADD: case EJS_OP_AND: case EJS_OP_DIV: case EJS_OP_MUL: case EJS_OP_OR: case EJS_OP_REM:
    case EJS_OP_SHL: case EJS_OP_SHR: case EJS_OP_SUB: case EJS_OP_USHR: case EJS_OP_XOR:
        return (EjsVar*) ejs->nanValue;

    default:
        ejsThrowTypeError(ejs, "Opcode %d not implemented for type %s", opcode, lhs->type->qname.name);
        return 0;
    }

    mprAssert(0);
}


/*
 *  iterator native function get(): Iterator
 */
static EjsVar *getVoidIterator(Ejs *ejs, EjsVar *np, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateIterator(ejs, np, NULL, 0, NULL);
}


/*********************************** Factory **********************************/
/*
 *  We don't actually create any instances. We just use a reference to the undefined singleton instance.
 */
EjsVoid *ejsCreateUndefined(Ejs *ejs)
{
    return (EjsVoid*) ejs->undefinedValue;
}



void ejsCreateVoidType(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;

    type = ejsCreateCoreType(ejs, ejsName(&qname, EJS_INTRINSIC_NAMESPACE, "Void"), ejs->objectType, sizeof(EjsVoid),
        ES_Void, ES_Void_NUM_CLASS_PROP, ES_Void_NUM_INSTANCE_PROP, EJS_ATTR_NATIVE);
    ejs->voidType = type;

    /*
     *  Define the helper functions.
     */
    type->helpers->castVar = (EjsCastVarHelper) castVoid;
    type->helpers->invokeOperator = (EjsInvokeOperatorHelper) invokeVoidOperator;

    ejs->undefinedValue = ejsCreateVar(ejs, type, 0);
    ejsSetDebugName(ejs->undefinedValue, "undefined");

    if (!(ejs->flags & EJS_FLAG_EMPTY)) {
        /*
         *  Define the "undefined" value
         */
        ejsSetProperty(ejs, ejs->global, ES_undefined, ejs->undefinedValue);
    }
}


void ejsConfigureVoidType(Ejs *ejs)
{
    EjsType     *type;

    type = ejs->voidType;

    ejsSetProperty(ejs, ejs->global, ES_undefined, ejs->undefinedValue);

    ejsBindMethod(ejs, type, ES_Object_get, getVoidIterator);
    ejsBindMethod(ejs, type, ES_Object_getValues, getVoidIterator);
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
