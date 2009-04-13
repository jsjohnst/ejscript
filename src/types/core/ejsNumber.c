/**
 *  ejsNumber.c - Number type class
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejs.h"

/**************************** Forward Declarations ****************************/
/*
 *  TODO - move to ejsNumber.h. But would have to rename fixed() to ejsFixed()
 */
#if BLD_FEATURE_FLOATING_POINT
#define fixed(n) ((int64) (floor(n)))
#else
#define fixed(n) (n)
#endif

#if BLD_WIN_LIKE
static double rint(double num)
{
    double low = floor(num);
    double high = ceil(num);
    return ((high - num) >= (num - low)) ? low : high;
}
#endif
/******************************************************************************/
/*
 *  Cast the operand to the specified type
 */

static EjsVar *castNumber(Ejs *ejs, EjsNumber *vp, EjsType *type)
{
    char    numBuf[512];

    switch (type->id) {

    case ES_Boolean:
        return (EjsVar*) ((vp->value) ? ejs->trueValue : ejs->falseValue);

    case ES_String:
#if BLD_FEATURE_FLOATING_POINT
        /* TODO - refactor */
        if (rint(vp->value) == vp->value && ((-1 - MAXINT) <= vp->value && vp->value <= MAXINT)) {
            mprSprintf(numBuf, sizeof(numBuf), "%.0f", vp->value);
        } else {
            mprSprintf(numBuf, sizeof(numBuf), "%g", vp->value);
        }
        return (EjsVar*) ejsCreateString(ejs, numBuf);
#elif MPR_64_BIT
        mprSprintf(numBuf, sizeof(numBuf), "%Ld", vp->value);
        return (EjsVar*) ejsCreateString(ejs, numBuf);
#else
        mprItoa(numBuf, sizeof(numBuf), (int) vp->value, 10);
        return (EjsVar*) ejsCreateString(ejs, numBuf);
#endif

    case ES_Number:
        return (EjsVar*) vp;
            
    default:
        return (EjsVar*) ejs->zeroValue;
    }
}


static EjsVar *coerceNumberOperands(Ejs *ejs, EjsVar *lhs, int opcode, EjsVar *rhs)
{
    switch (opcode) {
    /*
     *  Binary operators
     */
    case EJS_OP_ADD:
        if (ejsIsUndefined(rhs)) {
            return (EjsVar*) ejs->nanValue;
        } else if (ejsIsNull(rhs)) {
            return (EjsVar*) lhs;
        } else if (ejsIsBoolean(rhs) || ejsIsDate(rhs)) {
            return ejsInvokeOperator(ejs, lhs, opcode, (EjsVar*) ejsToNumber(ejs, rhs));
        } else {
            return ejsInvokeOperator(ejs, (EjsVar*) ejsToString(ejs, lhs), opcode, rhs);
        }
        break;

    case EJS_OP_AND: case EJS_OP_DIV: case EJS_OP_MUL: case EJS_OP_OR: case EJS_OP_REM:
    case EJS_OP_SHL: case EJS_OP_SHR: case EJS_OP_SUB: case EJS_OP_USHR: case EJS_OP_XOR:
        return ejsInvokeOperator(ejs, lhs, opcode, (EjsVar*) ejsToNumber(ejs, rhs));

    case EJS_OP_COMPARE_EQ: case EJS_OP_COMPARE_NE:
    case EJS_OP_COMPARE_LE: case EJS_OP_COMPARE_LT:
    case EJS_OP_COMPARE_GE: case EJS_OP_COMPARE_GT:
        if (ejsIsString(rhs)) {
            return ejsInvokeOperator(ejs, (EjsVar*) ejsToString(ejs, lhs), opcode, rhs);
        }
        return ejsInvokeOperator(ejs, lhs, opcode, (EjsVar*) ejsToNumber(ejs, rhs));

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
        return (EjsVar*) (((EjsNumber*) lhs)->value ? ejs->trueValue : ejs->falseValue);

    case EJS_OP_COMPARE_ZERO:
    case EJS_OP_COMPARE_FALSE:
        return (EjsVar*) (((EjsNumber*) lhs)->value ? ejs->falseValue: ejs->trueValue);

    case EJS_OP_COMPARE_UNDEFINED:
    case EJS_OP_COMPARE_NULL:
        return (EjsVar*) ejs->falseValue;

    default:
        ejsThrowTypeError(ejs, "Opcode %d not valid for type %s", opcode, lhs->type->qname.name);
        return ejs->undefinedValue;
    }
    return 0;
}


static EjsVar *invokeNumberOperator(Ejs *ejs, EjsNumber *lhs, int opcode, EjsNumber *rhs)
{
    EjsVar      *result;

    if (rhs == 0 || lhs->var.type != rhs->var.type) {
        if ((result = coerceNumberOperands(ejs, (EjsVar*) lhs, opcode, (EjsVar*) rhs)) != 0) {
            return result;
        }
    }

    /*
     *  Types now match, both numbers
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

    /*
     *  Unary operators
     */
    case EJS_OP_COMPARE_NOT_ZERO:
        return (EjsVar*) ((lhs->value) ? ejs->trueValue: ejs->falseValue);

    case EJS_OP_COMPARE_ZERO:
        return (EjsVar*) ((lhs->value == 0) ? ejs->trueValue: ejs->falseValue);

    case EJS_OP_COMPARE_UNDEFINED:
    case EJS_OP_COMPARE_NULL:
    case EJS_OP_COMPARE_FALSE:
    case EJS_OP_COMPARE_TRUE:
        return (EjsVar*) ejs->falseValue;

    case EJS_OP_NEG:
        return (EjsVar*) ejsCreateNumber(ejs, -lhs->value);

    case EJS_OP_LOGICAL_NOT:
        return (EjsVar*) ejsCreateBoolean(ejs, !fixed(lhs->value));

    case EJS_OP_NOT:
        return (EjsVar*) ejsCreateNumber(ejs, (MprNumber) (~fixed(lhs->value)));


    /*
     *  Binary operations
     */
    case EJS_OP_ADD:
        return (EjsVar*) ejsCreateNumber(ejs, lhs->value + rhs->value);

    case EJS_OP_AND:
        return (EjsVar*) ejsCreateNumber(ejs, (MprNumber) (fixed(lhs->value) & fixed(rhs->value)));

    case EJS_OP_DIV:
#if BLD_FEATURE_FLOATING_POINT
        if (rhs->value == 0) {
            ejsThrowArithmeticError(ejs, "Divisor is zero");
            return 0;
        }
#endif
        return (EjsVar*) ejsCreateNumber(ejs, lhs->value / rhs->value);

    case EJS_OP_MUL:
        return (EjsVar*) ejsCreateNumber(ejs, lhs->value * rhs->value);

    case EJS_OP_OR:
        return (EjsVar*) ejsCreateNumber(ejs, (MprNumber) (fixed(lhs->value) | fixed(rhs->value)));

    case EJS_OP_REM:
#if BLD_FEATURE_FLOATING_POINT
        if (rhs->value == 0) {
            ejsThrowArithmeticError(ejs, "Divisor is zero");
            return 0;
        }
        return (EjsVar*) ejsCreateNumber(ejs, (MprNumber) (fixed(lhs->value) % fixed(rhs->value)));
#else
        return (EjsVar*) ejsCreateNumber(ejs, (MprNumber) (fixed(lhs->value) % fixed(rhs->value)));
#endif

    case EJS_OP_SHL:
        return (EjsVar*) ejsCreateNumber(ejs, (MprNumber) (fixed(lhs->value) << fixed(rhs->value)));

    case EJS_OP_SHR:
        return (EjsVar*) ejsCreateNumber(ejs, (MprNumber) (fixed(lhs->value) >> fixed(rhs->value)));

    case EJS_OP_SUB:
        return (EjsVar*) ejsCreateNumber(ejs, lhs->value - rhs->value);

    case EJS_OP_USHR:
        return (EjsVar*) ejsCreateNumber(ejs, (MprNumber) (fixed(lhs->value) >> fixed(rhs->value)));

    case EJS_OP_XOR:
        return (EjsVar*) ejsCreateNumber(ejs, (MprNumber) (fixed(lhs->value) ^ fixed(rhs->value)));

    default:
        ejsThrowTypeError(ejs, "Opcode %d not implemented for type %s", opcode, lhs->var.type->qname.name);
        return 0;
    }
}


/*********************************** Methods **********************************/
/*
 *  Number constructor.
 *
 *      function Number()
 *      function Number(value)
 */

static EjsVar *numberConstructor(Ejs *ejs, EjsNumber *np, int argc, EjsVar **argv)
{
    EjsArray    *args;
    EjsNumber   *num;

    mprAssert(argc == 0 || (argc == 1 && ejsIsArray(argv[0])));

    if (argc == 1) {
        args = (EjsArray*) argv[0];

        num = ejsToNumber(ejs, ejsGetProperty(ejs, (EjsVar*) args, 0));
        if (num) {
            np->value = num->value;
        }
    }
    return (EjsVar*) np;
}


/*
 *  Function to iterate and return each number in sequence.
 *  NOTE: this is not a method of Number. Rather, it is a callback function for Iterator.
 */
static EjsVar *nextNumber(Ejs *ejs, EjsIterator *ip, int argc, EjsVar **argv)
{
    EjsNumber   *np;

    np = (EjsNumber*) ip->target;
    if (!ejsIsNumber(np)) {
        ejsThrowReferenceError(ejs, "Wrong type");
        return 0;
    }

    if (ip->index < np->value) {
        return (EjsVar*) ejsCreateNumber(ejs, ip->index++);
    }
    ejsThrowStopIteration(ejs);
    return 0;
}


/*
 *  Return the default iterator. This returns the index names.
 *
 *  iterator native function get(): Iterator
 */
static EjsVar *getNumberIterator(Ejs *ejs, EjsVar *np, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateIterator(ejs, np, (EjsNativeFunction) nextNumber, 0, NULL);
}


/*********************************** Support **********************************/

#ifndef ejsIsNan
int ejsIsNan(double f)
{
#if BLD_FEATURE_FLOATING_POINT
#if WIN
    return _isnan(f);
#elif VXWORKS
    /* TODO */
    return 0;
#else
    return (f == FP_NAN);
#endif
#else
    return 0;
#endif
}
#endif


bool ejsIsInfinite(MprNumber f)
{
#if BLD_FEATURE_FLOATING_POINT
#if WIN
    return !_finite(f);
#elif VXWORKS
    /* TODO */
    return 0;
#else
    return (f == FP_INFINITE);
#endif
#else
    return 0;
#endif
}

/*********************************** Factory **********************************/
/*
 *  Create an initialized number
 */

EjsNumber *ejsCreateNumber(Ejs *ejs, MprNumber value)
{
    EjsNumber   *vp;

    if (value == 0) {
        return ejs->zeroValue;
    } else if (value == 1) {
        return ejs->oneValue;
    } else if (value == -1) {
        return ejs->minusOneValue;
    }

    vp = (EjsNumber*) ejsCreateVar(ejs, ejs->numberType, 0);
    if (vp != 0) {
        vp->value = value;
    }

    ejsSetDebugName(vp, "number value");

    return vp;
}


void ejsCreateNumberType(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;
#if BLD_FEATURE_FLOATING_POINT
    static int  zero = 0;
#endif

    type = ejsCreateCoreType(ejs, ejsName(&qname, EJS_INTRINSIC_NAMESPACE, "Number"), ejs->objectType, sizeof(EjsNumber),
        ES_Number, ES_Number_NUM_CLASS_PROP, ES_Number_NUM_INSTANCE_PROP, EJS_ATTR_NATIVE | EJS_ATTR_HAS_CONSTRUCTOR);
    ejs->numberType = type;

    /*
     *  Define the helper functions.
     */
    type->helpers->castVar = (EjsCastVarHelper) castNumber;
    type->helpers->invokeOperator = (EjsInvokeOperatorHelper) invokeNumberOperator;

    ejs->zeroValue = (EjsNumber*) ejsCreateVar(ejs, ejs->numberType, 0);
    ejs->zeroValue->value = 0;
    ejs->oneValue = (EjsNumber*) ejsCreateVar(ejs, ejs->numberType, 0);
    ejs->oneValue->value = 1;
    ejs->minusOneValue = (EjsNumber*) ejsCreateVar(ejs, ejs->numberType, 0);
    ejs->minusOneValue->value = -1;

#if BLD_FEATURE_FLOATING_POINT
    ejs->infinityValue = (EjsNumber*) ejsCreateVar(ejs, ejs->numberType, 0);
    ejs->infinityValue->value = 1.0 / zero;
    ejs->negativeInfinityValue = (EjsNumber*) ejsCreateVar(ejs, ejs->numberType, 0);
    ejs->negativeInfinityValue->value = -1.0 / zero;
    ejs->nanValue = (EjsNumber*) ejsCreateVar(ejs, ejs->numberType, 0);
    ejs->nanValue->value = 0.0 / zero;

    ejs->maxValue = (EjsNumber*) ejsCreateVar(ejs, ejs->numberType, 0);
    ejs->maxValue->value = 1.7976931348623157e+308;
    ejs->minValue = (EjsNumber*) ejsCreateVar(ejs, ejs->numberType, 0);
    ejs->minValue->value = 5e-324;

    ejsSetDebugName(ejs->infinityValue, "Infinity");
    ejsSetDebugName(ejs->negativeInfinityValue, "NegativeInfinity");
    ejsSetDebugName(ejs->nanValue, "NaN");

#else
    ejs->maxValue = (EjsNumber*) ejsCreateVar(ejs, ejs->numberType, 0);
    ejs->maxValue->value = MAXINT;
    ejs->minValue = (EjsNumber*) ejsCreateVar(ejs, ejs->numberType, 0);
    ejs->minValue->value = -MAXINT;
    ejs->nanValue = ejs->zeroValue;
#endif

    ejsSetDebugName(ejs->minusOneValue, "-1");
    ejsSetDebugName(ejs->oneValue, "1");
    ejsSetDebugName(ejs->zeroValue, "0");
    ejsSetDebugName(ejs->maxValue, "MaxValue");
    ejsSetDebugName(ejs->minValue, "MinValue");
}


void ejsConfigureNumberType(Ejs *ejs)
{
    EjsType     *type;

    type = ejs->numberType;

    ejsSetProperty(ejs, (EjsVar*) type, ES_Number_MaxValue, (EjsVar*) ejs->maxValue);
    ejsSetProperty(ejs, (EjsVar*) type, ES_Number_MinValue, (EjsVar*) ejs->minValue);
    ejsBindMethod(ejs, type, ES_Number_Number, (EjsNativeFunction) numberConstructor);
    ejsBindMethod(ejs, type, ES_Object_get, getNumberIterator);
    ejsBindMethod(ejs, type, ES_Object_getValues, getNumberIterator);

    ejsSetProperty(ejs, ejs->global, ES_NegativeInfinity, (EjsVar*) ejs->negativeInfinityValue);
    ejsSetProperty(ejs, ejs->global, ES_Infinity, (EjsVar*) ejs->infinityValue);
    ejsSetProperty(ejs, ejs->global, ES_NaN, (EjsVar*) ejs->nanValue);
    ejsSetProperty(ejs, ejs->global, ES_num, (EjsVar*) type);
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
