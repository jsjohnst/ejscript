/**
 *  ejsNull.c - Ejscript Null class
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejs.h"

/******************************************************************************/
/*
 *  Cast the null operand to a primitive type
 */

static EjsVar *castNull(Ejs *ejs, EjsVar *vp, EjsType *type)
{
    switch (type->id) {
    case ES_Boolean:
        return (EjsVar*) ejs->falseValue;

    case ES_Number:
        return (EjsVar*) ejs->zeroValue;

    case ES_Object:
    default:
        /*
         *  Cast null to anything else results in a null
         */
        return vp;

    case ES_String:
        return (EjsVar*) ejsCreateString(ejs, "null");
    }
}


static EjsVar *coerceNullOperands(Ejs *ejs, EjsVar *lhs, int opcode, EjsVar *rhs)
{
    switch (opcode) {

    case EJS_OP_ADD:
        if (!ejsIsNumber(rhs)) {
            return ejsInvokeOperator(ejs, (EjsVar*) ejsToString(ejs, lhs), opcode, rhs);
        }
        /* Fall through */

    case EJS_OP_AND: case EJS_OP_DIV: case EJS_OP_MUL: case EJS_OP_OR: case EJS_OP_REM:
    case EJS_OP_SHL: case EJS_OP_SHR: case EJS_OP_SUB: case EJS_OP_USHR: case EJS_OP_XOR:
        return ejsInvokeOperator(ejs, (EjsVar*) ejs->zeroValue, opcode, rhs);

    /*
     *  Comparision
     */
    case EJS_OP_COMPARE_LE: case EJS_OP_COMPARE_LT:
    case EJS_OP_COMPARE_GE: case EJS_OP_COMPARE_GT:
        if (ejsIsNumber(rhs)) {
            return ejsInvokeOperator(ejs, (EjsVar*) ejs->zeroValue, opcode, rhs);
        } else if (ejsIsString(rhs)) {
            return ejsInvokeOperator(ejs, (EjsVar*) ejsToString(ejs, lhs), opcode, rhs);
        }
        break;

    case EJS_OP_COMPARE_NE:
    case EJS_OP_COMPARE_STRICTLY_NE:
        if (ejsIsUndefined(rhs)) {
            return (EjsVar*) ejs->falseValue;
        }
        return (EjsVar*) ejs->trueValue;

    case EJS_OP_COMPARE_EQ:
    case EJS_OP_COMPARE_STRICTLY_EQ:
        if (ejsIsUndefined(rhs)) {
            return (EjsVar*) ejs->trueValue;
        }
        return (EjsVar*) ejs->falseValue;

    case EJS_OP_COMPARE_UNDEFINED:
    case EJS_OP_COMPARE_NOT_ZERO:
    case EJS_OP_COMPARE_NULL:
        return (EjsVar*) ejs->trueValue;

    case EJS_OP_COMPARE_FALSE:
    case EJS_OP_COMPARE_TRUE:
    case EJS_OP_COMPARE_ZERO:
        return (EjsVar*) ejs->falseValue;

    /*
     *  Unary operators
     */
    case EJS_OP_LOGICAL_NOT: case EJS_OP_NOT: case EJS_OP_NEG:
        return 0;

    default:
        ejsThrowTypeError(ejs, "Opcode %d not valid for type %s", opcode, lhs->type->qname.name);
        return ejs->undefinedValue;
    }
    return 0;
}


static EjsVar *invokeNullOperator(Ejs *ejs, EjsVar *lhs, int opcode, EjsVar *rhs)
{
    EjsVar      *result;

    if (rhs == 0 || lhs->type != rhs->type) {
        if ((result = coerceNullOperands(ejs, lhs, opcode, rhs)) != 0) {
            return result;
        }
    }

    /*
     *  Types now match. Both left and right types are both "null"
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
        return (EjsVar*) ejs->oneValue;

    /*
     *  Binary operators. Reinvoke with left = zero
     */
    case EJS_OP_ADD: case EJS_OP_AND: case EJS_OP_DIV: case EJS_OP_MUL: case EJS_OP_OR: case EJS_OP_REM:
    case EJS_OP_SHL: case EJS_OP_SHR: case EJS_OP_SUB: case EJS_OP_USHR: case EJS_OP_XOR:
        return ejsInvokeOperator(ejs, (EjsVar*) ejs->zeroValue, opcode, rhs);

    default:
        ejsThrowTypeError(ejs, "Opcode %d not implemented for type %s", opcode, lhs->type->qname.name);
        return 0;
    }
}


/*
 *  iterator native function get(): Iterator
 */
static EjsVar *getNullIterator(Ejs *ejs, EjsVar *np, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateIterator(ejs, np, NULL, 0, NULL);
}


/*********************************** Factory **********************************/
/*
 *  We dont actually allocate any nulls. We just reuse the singleton instance.
 */

EjsNull *ejsCreateNull(Ejs *ejs)
{
    return (EjsNull*) ejs->nullValue;
}


void ejsCreateNullType(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;

    type = ejsCreateCoreType(ejs, ejsName(&qname, EJS_INTRINSIC_NAMESPACE, "Null"), ejs->objectType, sizeof(EjsNull),
        ES_Null, ES_Null_NUM_CLASS_PROP, ES_Null_NUM_INSTANCE_PROP, EJS_ATTR_NATIVE);
    ejs->nullType = type;

    /*
     *  Define the helper functions.
     */
    type->helpers->castVar = (EjsCastVarHelper) castNull;
    type->helpers->invokeOperator = (EjsInvokeOperatorHelper) invokeNullOperator;

    ejs->nullValue = ejsCreateVar(ejs, type, 0);
    ejsSetDebugName(ejs->nullValue, "null");
    
    if (!(ejs->flags & EJS_FLAG_EMPTY)) {
        ejsSetProperty(ejs, ejs->global, ES_null, ejs->nullValue);
    }
}


void ejsConfigureNullType(Ejs *ejs)
{
    EjsType     *type;

    type = ejs->nullType;

    ejsSetProperty(ejs, ejs->global, ES_null, ejs->nullValue);

    ejsBindMethod(ejs, type, ES_Object_get, getNullIterator);
    ejsBindMethod(ejs, type, ES_Object_getValues, getNullIterator);
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
