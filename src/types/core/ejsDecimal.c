/**
 *  ejsDecimal.c - Ejscript Decimal class
 *
 *  TODO - This class is not finished yet.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/
/*
 *  Select 128 bit decimals
 */
#define     DECNUMDIGITS    38

#include    "ejs.h"

#if BLD_FEATURE_DECIMAL
/******************************************************************************/
/*
 *  Cast the operand to the specified type
 *
 *  intrinsic function cast(type: Type) : Object
 */

static EjsVar *castDecimal(Ejs *ejs, EjsDecimal *vp, EjsType *type)
{
    char        numBuf[16];

    switch (type->primitiveType) {
    default:
        ejsThrowTypeError(ejs, "Can't cast to this type");
        return 0;

#if FUTURE
    case EJS_TYPE_BOOLEAN:
        //  TODO - not complete
        mprAssert(0);
        return (EjsVar*) ejsCreateBoolean(ejs, (vp->value != 0) ? 1 : 0);

    case EJS_TYPE_DECIMAL:
        return vp;

#if BLD_FEATURE_FLOATING_POINT
    case EJS_TYPE_DOUBLE:
        return (EjsVar*) ejsCreateDouble(ejs, (double) vp->value);
#endif

    case EJS_TYPE_INTEGER:
        return (EjsVar*) vp;

    case EJS_TYPE_LONG:
        return (EjsVar*) ejsCreateLong(ejs, (int64) vp->value);

    case EJS_TYPE_NUMBER:
        return (EjsVar*) ejsCreateNumber(ejs, (MprNumber) vp->value);
#endif
        
    case EJS_TYPE_STRING:
        decNumberToString(&vp->value, numBuf);
        return (EjsVar*) ejsCreateString(ejs, numBuf);
    }

    ejsThrowTypeError(ejs, "Unknown type");
    return 0;
}



static EjsVar *invokeDecimalOperator(Ejs *ejs, EjsDecimal *lhs, int opCode, EjsDecimal *rhs)
{
    MprDecimal  result;
    decContext  *context;

    mprAssert(ejsIsDecimal(lhs));
    mprAssert(rhs == 0 || ejsIsDecimal(rhs));

    context = (decContext*) lhs->var.type->typeData;

    switch (opCode) {
    case EJS_OP_MUL:
        decNumberMultiply(&result, &lhs->value, &rhs->value, context);
        return (EjsVar*) ejsCreateDecimal(ejs, result);

    case EJS_OP_DIV:
        decNumberDivide(&result, &lhs->value, &rhs->value, context);
        return (EjsVar*) ejsCreateDecimal(ejs, result);

    case EJS_OP_REM:
        decNumberRemainder(&result, &lhs->value, &rhs->value, context);
        return (EjsVar*) ejsCreateDecimal(ejs, result);

    case EJS_OP_ADD:
        decNumberAdd(&result, &lhs->value, &rhs->value, context);
        return (EjsVar*) ejsCreateDecimal(ejs, result);

    case EJS_OP_SUB:
        decNumberSubtract(&result, &lhs->value, &rhs->value, context);
        return (EjsVar*) ejsCreateDecimal(ejs, result);

#if 0
    case EJS_OP_AND:
        return (EjsVar*) ejsCreateDecimal(ejs, lhs->value & rhs->value);

    case EJS_OP_OR:
        return (EjsVar*) ejsCreateDecimal(ejs, lhs->value | rhs->value);

    case EJS_OP_SHL:
        return (EjsVar*) ejsCreateDecimal(ejs, lhs->value << rhs->value);

    case EJS_OP_SHR:
        return (EjsVar*) ejsCreateDecimal(ejs, lhs->value >> rhs->value);

    case EJS_OP_USHR:
        return (EjsVar*) ejsCreateDecimal(ejs, 
            (uint) lhs->value >> rhs->value);

    case EJS_OP_XOR:
        return (EjsVar*) ejsCreateDecimal(ejs, lhs->value ^ rhs->value);

    case EJS_OP_COMPARE_EQ:
    case EJS_OP_COMPARE_STRICTLY_EQ:
    case EJS_OP_COMPARE_NE:
    case EJS_OP_COMPARE_STRICTLY_NE:
    case EJS_OP_COMPARE_LE:
    case EJS_OP_COMPARE_LT:
    case EJS_OP_COMPARE_GE:
    case EJS_OP_COMPARE_GT:
        boolResult = 0;
        if (ejsIsNan(lhs->value) || ejsIsNan(rhs->value)) {
            boolResult = 0;

        } else if (lhs->value == rhs->value) {
            boolResult = 1;

        } else if (lhs->value == +0.0 && rhs->value == -0.0) {
            boolResult = 1;

        } else if (lhs->value == -0.0 && rhs->value == +0.0) {
            boolResult = 1;

        } else {

            switch (opCode) {
            case EJS_OP_COMPARE_EQ:
            case EJS_OP_COMPARE_STRICTLY_EQ:
                return (EjsVar*) ejsCreateBoolean(ejs, lhs->value == rhs->value);

            case EJS_OP_COMPARE_NE:
            case EJS_OP_COMPARE_STRICTLY_NE:
                return (EjsVar*) ejsCreateBoolean(ejs, !(lhs->value == rhs->value));

            case EJS_OP_COMPARE_LE:
                return (EjsVar*) ejsCreateBoolean(ejs, lhs->value <= rhs->value);

            case EJS_OP_COMPARE_LT:
                return (EjsVar*) ejsCreateBoolean(ejs, lhs->value < rhs->value);

            case EJS_OP_COMPARE_GE:
                return (EjsVar*) ejsCreateBoolean(ejs, lhs->value >= rhs->value);

            case EJS_OP_COMPARE_GT:
                return (EjsVar*) ejsCreateBoolean(ejs, lhs->value > rhs->value);

            default:
                mprAssert(0);
            }
        }
        return (EjsVar*) ejsCreateBoolean(ejs, boolResult);

    case EJS_OP_INC:
        return (EjsVar*) ejsCreateDecimal(ejs, lhs->value + rhs->value);

    case EJS_OP_NEG:
        mprAssert(0);
        return (EjsVar*) ejsCreateDecimal(ejs, -lhs->value);

    case EJS_OP_NOT:
        return (EjsVar*) ejsCreateDecimal(ejs, ~lhs->value);

#endif

    default:
        ejsThrowTypeError(ejs, "Operation not valid on this type");
        return 0;
    }
    /* Should never get here */
}


/******************************************************************************/
/******************************** Support API *********************************/
/******************************************************************************/

MprDecimal ejsParseDecimal(Ejs *ejs, cchar *str)
{
    EjsType     *type;
    MprDecimal  d;

    type = ejs->decimalType;
    mprAssert(type);

    decNumberFromString(&d, str, (decContext*) type->typeData);

    return d;
}


/*********************************** Factory **********************************/

EjsDecimal *ejsCreateDecimal(Ejs *ejs, MprDecimal value)
{
    EjsDecimal  *vp;

    vp = (EjsDecimal*) ejsCreateVar(ejs, ejs->decimalType, 0);
    if (vp != 0) {
        vp->value = value;
    }
    return vp;
}



#if BLD_FEATURE_FLOATING_POINT
EjsDecimal *ejsCreateDecimalFromDouble(Ejs *ejs, double value)
{
    EjsType     *type;
    EjsDecimal  *vp;
    char        buf[16];

    type = ejs->decimalType;
    mprAssert(type);

    vp = (EjsDecimal*) ejsCreateVar(ejs, type, 0);
    if (vp != 0) {
        /*
         *  FUTURE -- must get ability to convert to decimal from other 
         *  than string!
         */
        mprSprintf(buf, sizeof(buf), "%f", value);
        decNumberFromString(&vp->value, buf, type->typeData);
    }
    return vp;
}
#endif



EjsDecimal *ejsCreateDecimalFromInteger(Ejs *ejs, int value)
{
    EjsType     *type;
    EjsDecimal  *vp;
    char        buf[16];

    type = ejs->decimalType;
    mprAssert(type);

    vp = (EjsDecimal*) ejsCreateVar(ejs, type, 0);
    if (vp != 0) {
        /*
         *  FUTURE -- must get ability to convert to decimal from other 
         *  than string!
         */
        mprItoa(buf, sizeof(buf), value);
        decNumberFromString(&vp->value, buf, type->typeData);
    }
    return vp;
}



EjsDecimal *ejsCreateDecimalFromLong(Ejs *ejs, int64 value)
{
    EjsType     *type;
    EjsDecimal  *vp;
    char        buf[16];

    type = ejs->decimalType;
    mprAssert(type);

    vp = (EjsDecimal*) ejsCreateVar(ejs, type, 0);
    if (vp != 0) {
        /*
         *  FUTURE -- must get ability to convert to decimal from other 
         *  than string!
         */
        mprSprintf(buf, sizeof(buf), "%Ld", value);
        decNumberFromString(&vp->value, buf, type->typeData);
    }
    return vp;
}



void ejsCreateDecimalType(Ejs *ejs)
{
    EjsType     *type;
    decContext  *context;                               // working context

    /*
     *  Allocate a decimal working context
     */
    context = mprAlloc(ejs, sizeof(decContext));
    type = ejsCreateIntrinsicType(ejs, "Decimal", 0, sizeof(EjsDecimal), 
        EJSLOT_Decimal_NUM_CLASS_PROP, EJSLOT_Decimal_NUM_INSTANCE_PROP, EJS_ATTR_NATIVE);

    /*
     *  Define the helper functions.
     */
    type->helpers->castVar = (EjsCastVarHelper) castDecimal;
    type->helpers->invokeOperator = (EjsInvokeOperatorHelper) invokeDecimalOperator;

    /*
     *  Initalize with no traps and define the precision
     */
    decContextDefault(context, DEC_INIT_BASE); 
    context->traps = 0;
    context->digits = DECNUMDIGITS;
    type->typeData = context;

    ejs->decimalType = type;
    ejsAddCoreType(ejs, EJS_TYPE_DECIMAL, type);
}



void ejsConfigureDecimalType(Ejs *ejs)
{
    EjsType     *type;

    type = ejs->decimalType;
    mprAssert(type);
    
    /*
     *  Define the "decimal" alias
     */
    ejsSetProperty(ejs, ejs->global, EJSLOT_decimal, (EjsVar*) type);
}


#else
void __dummyDecimal() { }
#endif // BLD_FEATURE_DECIMAL

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
