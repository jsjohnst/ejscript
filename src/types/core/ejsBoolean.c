/**
 *  ejsBoolean.c - Boolean native class
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejs.h"

/******************************************************************************/
/*
 *  Cast the operand to a primitive type
 *
 *  intrinsic function cast(type: Type) : Object
 */

static EjsVar *castBooleanVar(Ejs *ejs, EjsBoolean *vp, EjsType *type)
{
    mprAssert(ejsIsBoolean(vp));

    switch (type->id) {

    case ES_Number:
        return (EjsVar*) ((vp->value) ? ejs->oneValue: ejs->zeroValue);

    case ES_String:
        return (EjsVar*) ejsCreateString(ejs, (vp->value) ? "true" : "false");

    default:
        ejsThrowTypeError(ejs, "Can't cast to this type");
        return 0;
    }
}


/*
 *  Coerce operands for invokeOperator
 */
static EjsVar *coerceBooleanOperands(Ejs *ejs, EjsVar *lhs, int opcode, EjsVar *rhs)
{
    switch (opcode) {

    case EJS_OP_ADD:
        if (ejsIsUndefined(rhs)) {
            return (EjsVar*) ejs->nanValue;
        } else if (ejsIsNull(rhs) || ejsIsNumber(rhs) || ejsIsDate(rhs)) {
            return ejsInvokeOperator(ejs, (EjsVar*) ejsToNumber(ejs, lhs), opcode, rhs);
        } else {
            return ejsInvokeOperator(ejs, (EjsVar*) ejsToString(ejs, lhs), opcode, rhs);
        }
        break;

    case EJS_OP_AND: case EJS_OP_DIV: case EJS_OP_MUL: case EJS_OP_OR: case EJS_OP_REM:
    case EJS_OP_SHL: case EJS_OP_SHR: case EJS_OP_SUB: case EJS_OP_USHR: case EJS_OP_XOR:
        return ejsInvokeOperator(ejs, (EjsVar*) ejsToNumber(ejs, lhs), opcode, rhs);

    case EJS_OP_COMPARE_LE: case EJS_OP_COMPARE_LT:
    case EJS_OP_COMPARE_GE: case EJS_OP_COMPARE_GT:
    case EJS_OP_COMPARE_EQ: case EJS_OP_COMPARE_NE:
        if (ejsIsString(rhs)) {
            return ejsInvokeOperator(ejs, (EjsVar*) ejsToString(ejs, lhs), opcode, rhs);
        }
        return ejsInvokeOperator(ejs, (EjsVar*) ejsToNumber(ejs, lhs), opcode, rhs);

    case EJS_OP_COMPARE_STRICTLY_NE:
        return (EjsVar*) ejs->trueValue;

    case EJS_OP_COMPARE_STRICTLY_EQ:
        return (EjsVar*) ejs->falseValue;

    /*
     *  Unary operators
     */
    case EJS_OP_LOGICAL_NOT: case EJS_OP_NOT: case EJS_OP_NEG:
        return 0;

    case EJS_OP_COMPARE_NOT_ZERO:
    case EJS_OP_COMPARE_TRUE:
        return (EjsVar*) (((EjsBoolean*) lhs)->value ? ejs->trueValue: ejs->falseValue);

    case EJS_OP_COMPARE_ZERO:
    case EJS_OP_COMPARE_FALSE:
        return (EjsVar*) (((EjsBoolean*) lhs)->value ? ejs->falseValue : ejs->trueValue);

    case EJS_OP_COMPARE_UNDEFINED:
    case EJS_OP_COMPARE_NULL:
        return (EjsVar*) ejs->falseValue;

    default:
        ejsThrowTypeError(ejs, "Opcode %d not valid for type %s", opcode, lhs->type->qname.name);
        return ejs->undefinedValue;
    }
}


/*
 *  Run an operator on the operands
 */
static EjsVar *invokeBooleanOperator(Ejs *ejs, EjsBoolean *lhs, int opcode, EjsBoolean *rhs)
{
    EjsVar      *result;

    if (rhs == 0 || lhs->var.type != rhs->var.type) {
        if ((result = coerceBooleanOperands(ejs, (EjsVar*) lhs, opcode, (EjsVar*) rhs)) != 0) {
            return result;
        }
    }

    /*
     *  Types now match
     */
    switch (opcode) {

    case EJS_OP_COMPARE_EQ: case EJS_OP_COMPARE_STRICTLY_EQ:
        return (EjsVar*) ((lhs->value == rhs->value) ? ejs->trueValue: ejs->falseValue);

    case EJS_OP_COMPARE_NE: case EJS_OP_COMPARE_STRICTLY_NE:
        return (EjsVar*) ((lhs->value != rhs->value) ? ejs->trueValue: ejs->falseValue);

    case EJS_OP_COMPARE_LT:
        return (EjsVar*) ((lhs->value < rhs->value) ? ejs->trueValue: ejs->falseValue);

    case EJS_OP_COMPARE_LE:
        return (EjsVar*) ((lhs->value <= rhs->value) ? ejs->trueValue: ejs->falseValue);

    case EJS_OP_COMPARE_GT:
        return (EjsVar*) ((lhs->value > rhs->value) ? ejs->trueValue: ejs->falseValue);

    case EJS_OP_COMPARE_GE:
        return (EjsVar*) ((lhs->value >= rhs->value) ? ejs->trueValue: ejs->falseValue);

    case EJS_OP_COMPARE_NOT_ZERO:
        return (EjsVar*) ((lhs->value) ? ejs->trueValue: ejs->falseValue);

    case EJS_OP_COMPARE_ZERO:
        return (EjsVar*) ((lhs->value == 0) ? ejs->trueValue: ejs->falseValue);

    case EJS_OP_COMPARE_UNDEFINED:
    case EJS_OP_COMPARE_NULL:
        return (EjsVar*) ejs->falseValue;

    case EJS_OP_COMPARE_FALSE:
        return (EjsVar*) ((lhs->value) ? ejs->falseValue: ejs->trueValue);

    case EJS_OP_COMPARE_TRUE:
        return (EjsVar*) ((lhs->value) ? ejs->trueValue: ejs->falseValue);

    /*
     *  Unary operators
     */
    case EJS_OP_NEG:
        return (EjsVar*) ejsCreateNumber(ejs, - lhs->value);

    case EJS_OP_LOGICAL_NOT:
        return (EjsVar*) ejsCreateBoolean(ejs, !lhs->value);

    case EJS_OP_NOT:
        return (EjsVar*) ejsCreateBoolean(ejs, ~lhs->value);

    /*
     *  Binary operations
     */
    case EJS_OP_ADD:
        return (EjsVar*) ejsCreateBoolean(ejs, lhs->value + rhs->value);

    case EJS_OP_AND:
        return (EjsVar*) ejsCreateBoolean(ejs, lhs->value & rhs->value);

    case EJS_OP_DIV:
        return (EjsVar*) ejsCreateBoolean(ejs, lhs->value / rhs->value);

    case EJS_OP_MUL:
        return (EjsVar*) ejsCreateBoolean(ejs, lhs->value * rhs->value);

    case EJS_OP_OR:
        return (EjsVar*) ejsCreateBoolean(ejs, lhs->value | rhs->value);

    case EJS_OP_REM:
        return (EjsVar*) ejsCreateBoolean(ejs, lhs->value % rhs->value);

    case EJS_OP_SUB:
        return (EjsVar*) ejsCreateBoolean(ejs, lhs->value - rhs->value);

    case EJS_OP_USHR:
        return (EjsVar*) ejsCreateBoolean(ejs, lhs->value >> rhs->value);

    case EJS_OP_XOR:
        return (EjsVar*) ejsCreateBoolean(ejs, lhs->value ^ rhs->value);

    default:
        ejsThrowTypeError(ejs, "Opcode %d not implemented for type %s", opcode, lhs->var.type->qname.name);
        return 0;
    }
}


/*********************************** Methods **********************************/
/*
 *  Boolean constructor.
 *
 *      function Boolean()
 *      function Boolean(value: Boolean)
 *
 *  If the value is omitted or 0, -1, NaN, false, null, undefined or the empty string, then set the boolean value to
 *  to false.
 */

static EjsVar *booleanConstructor(Ejs *ejs, EjsBoolean *bp, int argc, EjsVar **argv)
{
    EjsArray    *args;
    EjsVar      *vp;

    mprAssert(argc == 0 || (argc == 1 && ejsIsArray(argv[0])));

    if (argc == 1) {
        args = (EjsArray*) argv[0];

        vp = ejsGetProperty(ejs, (EjsVar*) args, 0);
        if (vp) {
            /* Change the bp value */
            bp = ejsToBoolean(ejs, vp);
        }
    }
    return (EjsVar*) bp;
}


/*********************************** Factory **********************************/

EjsBoolean *ejsCreateBoolean(Ejs *ejs, int value)
{
    return (value) ? ejs->trueValue : ejs->falseValue;
}


static void defineBooleanConstants(Ejs *ejs)
{
    EjsType     *type;

    type = ejs->booleanType;

    if (ejs->flags & EJS_FLAG_EMPTY) {
        return;
    }

    ejsSetProperty(ejs, ejs->global, ES_boolean, (EjsVar*) type);
    ejsSetProperty(ejs, ejs->global, ES_true, (EjsVar*) ejs->trueValue);
    ejsSetProperty(ejs, ejs->global, ES_false, (EjsVar*) ejs->falseValue);
}


void ejsCreateBooleanType(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;

    type = ejsCreateCoreType(ejs, ejsName(&qname, EJS_INTRINSIC_NAMESPACE, "Boolean"), ejs->objectType, sizeof(EjsBoolean),
        ES_Boolean, ES_Boolean_NUM_CLASS_PROP, ES_Boolean_NUM_INSTANCE_PROP, EJS_ATTR_NATIVE | EJS_ATTR_HAS_CONSTRUCTOR);
    ejs->booleanType = type;

    /*
     *  Define the helper functions.
     */
    type->helpers->castVar = (EjsCastVarHelper) castBooleanVar;
    type->helpers->invokeOperator = (EjsInvokeOperatorHelper) invokeBooleanOperator;

    /*
     *  Pre-create the only two valid instances for boolean
     */
    ejs->trueValue = (EjsBoolean*) ejsCreateVar(ejs, type, 0);
    ejs->trueValue->value = 1;

    ejs->falseValue = (EjsBoolean*) ejsCreateVar(ejs, type, 0);
    ejs->falseValue->value = 0;

    ejsSetDebugName(ejs->falseValue, "false");
    ejsSetDebugName(ejs->trueValue, "true");

    defineBooleanConstants(ejs);
}


void ejsConfigureBooleanType(Ejs *ejs)
{
    EjsType     *type;

    type = ejs->booleanType;

    defineBooleanConstants(ejs);
    ejsBindMethod(ejs, type, ES_Boolean_Boolean, (EjsNativeFunction) booleanConstructor);
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
