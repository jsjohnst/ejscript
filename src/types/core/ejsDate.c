/**
 *  ejsDate.c - Date type class
 *
 *  Date/time is store internally as milliseconds since 1970/01/01 GMT
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejs.h"

/***************************** Forward Declarations ***************************/
/*
 *  TODO - move to ejsNumber.h. But would have to rename fixed() to ejsFixed()
 */
#if BLD_FEATURE_FLOATING_POINT
#define fixed(n) ((int64) (floor(n)))
#else
#define fixed(n) (n)
#endif

#if WIN
#pragma warning (disable:4244)
#endif

/******************************************************************************/
/*
 *  Cast the operand to the specified type
 *
 *  intrinsic function cast(type: Type) : Object
 */

static EjsVar *castDate(Ejs *ejs, EjsDate *dp, EjsType *type)
{
    struct tm   tm;
    char        buf[80];

    switch (type->id) {

    case ES_Boolean:
        return (EjsVar*) ejs->trueValue;

    case ES_Number:
        return (EjsVar*) ejsCreateNumber(ejs, (MprNumber) dp->value);

    case ES_String:
        /*
         *  Format:  Tue Jul 15 2009 10:53:23 GMT-0700 (PDT)
         */
        mprLocaltime(ejs, &tm, dp->value);
        mprStrftime(ejs, buf, sizeof(buf), "%a %b %d %Y %T GMT%z (%Z)", &tm);
        return (EjsVar*) ejsCreateString(ejs, buf);

    default:
        ejsThrowTypeError(ejs, "Can't cast to this type");
        return 0;
    }

    return 0;
}



/*
 *  TODO - this is the same as number. Should share code
 */
static EjsVar *coerceDateOperands(Ejs *ejs, EjsVar *lhs, int opcode, EjsVar *rhs)
{
    switch (opcode) {
    /*
     *  Binary operators
     */
    case EJS_OP_ADD:
        if (ejsIsUndefined(rhs)) {
            return (EjsVar*) ejs->nanValue;
        } else if (ejsIsNull(rhs)) {
            rhs = (EjsVar*) ejs->zeroValue;
        } else if (ejsIsBoolean(rhs) || ejsIsNumber(rhs)) {
            return ejsInvokeOperator(ejs, (EjsVar*) ejsToNumber(ejs, lhs), opcode, rhs);
        } else {
            return ejsInvokeOperator(ejs, (EjsVar*) ejsToString(ejs, lhs), opcode, rhs);
        }
        break;

    case EJS_OP_AND: case EJS_OP_DIV: case EJS_OP_MUL: case EJS_OP_OR: case EJS_OP_REM:
    case EJS_OP_SHL: case EJS_OP_SHR: case EJS_OP_SUB: case EJS_OP_USHR: case EJS_OP_XOR:
        return ejsInvokeOperator(ejs, (EjsVar*) ejsToNumber(ejs, lhs), opcode, rhs);

    case EJS_OP_COMPARE_EQ: case EJS_OP_COMPARE_NE:
    case EJS_OP_COMPARE_LE: case EJS_OP_COMPARE_LT:
    case EJS_OP_COMPARE_GE: case EJS_OP_COMPARE_GT:
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
        return (EjsVar*) (((EjsDate*) lhs)->value ? ejs->trueValue : ejs->falseValue);

    case EJS_OP_COMPARE_ZERO:
    case EJS_OP_COMPARE_FALSE:
        return (EjsVar*) (((EjsDate*) lhs)->value ? ejs->falseValue: ejs->trueValue);

    case EJS_OP_COMPARE_UNDEFINED:
    case EJS_OP_COMPARE_NULL:
        return (EjsVar*) ejs->falseValue;

    default:
        ejsThrowTypeError(ejs, "Opcode %d not valid for type %s", opcode, lhs->type->qname.name);
        return ejs->undefinedValue;
    }

    return 0;
}



static EjsVar *invokeDateOperator(Ejs *ejs, EjsDate *lhs, int opcode, EjsDate *rhs)
{
    EjsVar      *result;

    if (rhs == 0 || lhs->var.type != rhs->var.type) {
        if ((result = coerceDateOperands(ejs, (EjsVar*) lhs, opcode, (EjsVar*) rhs)) != 0) {
            return result;
        }
    }

    switch (opcode) {
    case EJS_OP_COMPARE_EQ: case EJS_OP_COMPARE_STRICTLY_EQ:
        return (EjsVar*) ejsCreateBoolean(ejs, lhs->value == rhs->value);

    case EJS_OP_COMPARE_NE: case EJS_OP_COMPARE_STRICTLY_NE:
        return (EjsVar*) ejsCreateBoolean(ejs, !(lhs->value == rhs->value));

    case EJS_OP_COMPARE_LT:
        return (EjsVar*) ejsCreateBoolean(ejs, lhs->value < rhs->value);

    case EJS_OP_COMPARE_LE:
        return (EjsVar*) ejsCreateBoolean(ejs, lhs->value <= rhs->value);

    case EJS_OP_COMPARE_GT:
        return (EjsVar*) ejsCreateBoolean(ejs, lhs->value > rhs->value);

    case EJS_OP_COMPARE_GE:
        return (EjsVar*) ejsCreateBoolean(ejs, lhs->value >= rhs->value);

    case EJS_OP_COMPARE_NOT_ZERO:
        return (EjsVar*) ((lhs->value) ? ejs->trueValue: ejs->falseValue);

    case EJS_OP_COMPARE_ZERO:
        return (EjsVar*) ((lhs->value == 0) ? ejs->trueValue: ejs->falseValue);

    case EJS_OP_COMPARE_UNDEFINED:
    case EJS_OP_COMPARE_NULL:
    case EJS_OP_COMPARE_FALSE:
    case EJS_OP_COMPARE_TRUE:
        return (EjsVar*) ejs->falseValue;

    /*
     *  Unary operators
     */
    case EJS_OP_NEG:
        return (EjsVar*) ejsCreateNumber(ejs, - (MprNumber) lhs->value);

    case EJS_OP_LOGICAL_NOT:
        return (EjsVar*) ejsCreateBoolean(ejs, (MprNumber) !fixed(lhs->value));

    case EJS_OP_NOT:
        return (EjsVar*) ejsCreateNumber(ejs, (MprNumber) (~fixed(lhs->value)));

    /*
     *  Binary operators
     */
    case EJS_OP_ADD:
        return (EjsVar*) ejsCreateDate(ejs, lhs->value + rhs->value);

    case EJS_OP_AND:
        return (EjsVar*) ejsCreateDate(ejs, (MprNumber) (fixed(lhs->value) & fixed(rhs->value)));

    case EJS_OP_DIV:
#if BLD_FEATURE_FLOATING_POINT
        if (rhs->value == 0) {
            ejsThrowArithmeticError(ejs, "Divisor is zero");
            return 0;
        }
#endif
        return (EjsVar*) ejsCreateDate(ejs, lhs->value / rhs->value);

    case EJS_OP_MUL:
        return (EjsVar*) ejsCreateDate(ejs, lhs->value * rhs->value);

    case EJS_OP_OR:
        return (EjsVar*) ejsCreateDate(ejs, (MprNumber) (fixed(lhs->value) | fixed(rhs->value)));

    case EJS_OP_REM:
#if BLD_FEATURE_FLOATING_POINT
        if (rhs->value == 0) {
            ejsThrowArithmeticError(ejs, "Divisor is zero");
            return 0;
        }
#endif
        return (EjsVar*) ejsCreateDate(ejs, (MprNumber) (fixed(lhs->value) % fixed(rhs->value)));

    case EJS_OP_SHL:
        return (EjsVar*) ejsCreateDate(ejs, (MprNumber) (fixed(lhs->value) << fixed(rhs->value)));

    case EJS_OP_SHR:
        return (EjsVar*) ejsCreateDate(ejs, (MprNumber) (fixed(lhs->value) >> fixed(rhs->value)));

    case EJS_OP_SUB:
        return (EjsVar*) ejsCreateDate(ejs, (MprNumber) (fixed(lhs->value) - fixed(rhs->value)));

    case EJS_OP_USHR:
        return (EjsVar*) ejsCreateDate(ejs, (MprNumber) (fixed(lhs->value) >> fixed(rhs->value)));

    case EJS_OP_XOR:
        return (EjsVar*) ejsCreateDate(ejs, (MprNumber) (fixed(lhs->value) ^ fixed(rhs->value)));

    default:
        ejsThrowTypeError(ejs, "Opcode %d not implemented for type %s", opcode, lhs->var.type->qname.name);
        return 0;
    }
    /* Should never get here */
}

/*********************************** Methods **********************************/
/*
 *  TODO Date constructor
 *
 *      Date()
 *      Date(milliseconds)
 *      Date(dateString)
 *      Date(year, month, date)
 *      Date(year, month, date, hour, minute, second)
 */

static EjsVar *dateConstructor(Ejs *ejs, EjsDate *date, int argc, EjsVar **argv)
{
    EjsArray    *args;
    EjsVar      *vp;

    mprAssert(argc == 1 && ejsIsArray(argv[0]));

    args = (EjsArray*) argv[0];

    switch (args->length) {
    case 0:
        /* Now */
        date->value = mprGetTime(ejs);
        break;

    case 1:
        vp = ejsGetProperty(ejs, (EjsVar*) args, 0);
        if (ejsIsNumber(vp)) {
            /* Milliseconds */
            date->value = ejsGetNumber(vp);

        } else if (ejsIsString(vp)) {
            if (mprParseTime(ejs, &date->value, ejsGetString(vp)) < 0) {
                ejsThrowArgError(ejs, "Can't parse date string: %s", ejsGetString(vp));
                return 0;
            }
        } else if (ejsIsDate(vp)) {
            date->value = ((EjsDate*) vp)->value;

        } else {
            ejsThrowArgError(ejs, "Can't construct date from this argument");
        }
        break;

    //  TODO - must do other args

    default:
        ejsThrowArgError(ejs, "Too many arguments");
    }

    return (EjsVar*) date;
}




static EjsVar *getDay(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    struct tm   tm;

    mprLocaltime(ejs, &tm, dp->value);
    return (EjsVar*) ejsCreateNumber(ejs, tm.tm_wday);
}



/*
 *  Return day of year (0 - 366)
 */
static EjsVar *getDayOfYear(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    struct tm   tm;

    mprLocaltime(ejs, &tm, dp->value);
    return (EjsVar*) ejsCreateNumber(ejs, tm.tm_yday);
}



/*
 *  Return day of month (0-31)
 */
static EjsVar *getDate(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    struct tm   tm;

    mprLocaltime(ejs, &tm, dp->value);
    return (EjsVar*) ejsCreateNumber(ejs, tm.tm_mday);
}



/*
 *  Get the elapsed time in milliseconds since the Date object was constructed
 */
static EjsVar *elapsed(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, mprGetElapsedTime(ejs, dp->value));
}



/*
 *  Return year in 4 digits
 */
static EjsVar *getFullYear(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    struct tm   tm;

    mprLocaltime(ejs, &tm, dp->value);
    return (EjsVar*) ejsCreateNumber(ejs, tm.tm_year + 1900);
}



/*
 *  Update the year component using a 4 digit year
 */
static EjsVar *setFullYear(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    struct tm   tm;

    mprLocaltime(ejs, &tm, dp->value);
    tm.tm_yday  = ejsGetNumber(argv[0]) - 1900;
    dp->value = mprMakeLocalTime(ejs, &tm);
    return 0;
}



/*
 *  function format(layout: String): String
 */
static EjsVar *formatDate(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    char        buf[80];
    struct tm   tm;

    mprLocaltime(ejs, &tm, dp->value);
    mprStrftime(ejs, buf, sizeof(buf), ejsGetString(argv[0]), &tm);
    return (EjsVar*) ejsCreateString(ejs, buf);
}



/*
 *  Return hour of day (0-23)
 */
static EjsVar *getHours(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    struct tm   tm;

    mprLocaltime(ejs, &tm, dp->value);
    return (EjsVar*) ejsCreateNumber(ejs, tm.tm_hour);
}



/*
 *  Update the hour of the day using a 0-23 hour
 */
static EjsVar *setHours(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    struct tm   tm;

    mprLocaltime(ejs, &tm, dp->value);
    tm.tm_hour = ejsGetNumber(argv[0]);
    dp->value = mprMakeLocalTime(ejs, &tm);
    return 0;
}



static EjsVar *getMilliseconds(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, ((int64) dp->value) % 1000);
}



static EjsVar *setMilliseconds(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, ((int) dp->value) % 1000);
}



static EjsVar *getMinutes(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    struct tm   tm;

    mprLocaltime(ejs, &tm, dp->value);
    return (EjsVar*) ejsCreateNumber(ejs, tm.tm_min);
}



static EjsVar *setMinutes(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    struct tm   tm;

    mprLocaltime(ejs, &tm, dp->value);
    tm.tm_min = ejsGetNumber(argv[0]);
    dp->value = mprMakeLocalTime(ejs, &tm);
    return 0;
}



/*
 *  Get the month (0-11)
 */
static EjsVar *getMonth(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    struct tm   tm;

    mprLocaltime(ejs, &tm, dp->value);
    return (EjsVar*) ejsCreateNumber(ejs, tm.tm_mon);
}



static EjsVar *setMonth(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    struct tm   tm;

    mprLocaltime(ejs, &tm, dp->value);
    tm.tm_mon = ejsGetNumber(argv[0]);
    dp->value = mprMakeLocalTime(ejs, &tm);
    return 0;
}



static EjsVar *nextDay(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    int64       inc;

    if (argc == 1) {
        inc = ejsGetNumber(argv[0]);
    } else {
        inc = 1;
    }
    return (EjsVar*) ejsCreateDate(ejs, dp->value + (inc * 86400 * 1000));
}



static EjsVar *now(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, mprGetTime(ejs));
}



static EjsVar *parseDate(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    MprTime     when;

    if (mprParseTime(ejs, &when, ejsGetString(argv[0])) < 0) {
        ejsThrowArgError(ejs, "Can't parse date string: %s", ejsGetString(argv[0]));
        return 0;
    }
    return (EjsVar*) ejsCreateDate(ejs, when);
}



/*
 *  Get seconds (0-60)
 */
static EjsVar *getSeconds(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    struct tm   tm;

    mprLocaltime(ejs, &tm, dp->value);
    return (EjsVar*) ejsCreateNumber(ejs, tm.tm_sec);
}



static EjsVar *setSeconds(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    struct tm   tm;

    mprLocaltime(ejs, &tm, dp->value);
    tm.tm_sec = ejsGetNumber(argv[0]);
    dp->value = mprMakeLocalTime(ejs, &tm);
    return 0;
}



static EjsVar *getTime(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, dp->value);
}



static EjsVar *setTime(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    dp->value = ejsGetNumber(argv[0]);
    return 0;
}



static EjsVar *dateToString(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    return castDate(ejs, dp, ejs->stringType);
}



static EjsVar *getYear(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    struct tm   tm;

    mprLocaltime(ejs, &tm, dp->value);
    return (EjsVar*) ejsCreateNumber(ejs, tm.tm_year + 1900);
}


static EjsVar *setYear(Ejs *ejs, EjsDate *dp, int argc, EjsVar **argv)
{
    struct tm   tm;

    mprLocaltime(ejs, &tm, dp->value);
    tm.tm_year = ejsGetNumber(argv[0]) - 1900;
    dp->value = mprMakeLocalTime(ejs, &tm);
    return 0;
}


/*********************************** Factory **********************************/
/*
 *  Create an initialized date object. Set to the current time if value is zero.
 */

EjsDate *ejsCreateDate(Ejs *ejs, MprTime value)
{
    EjsDate *vp;

    vp = (EjsDate*) ejsCreateVar(ejs, ejs->dateType, 0);
    if (vp != 0) {
        vp->value = value;
    }
    return vp;
}


void ejsCreateDateType(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;

    type = ejsCreateCoreType(ejs, ejsName(&qname, EJS_INTRINSIC_NAMESPACE, "Date"), ejs->objectType, sizeof(EjsDate),
        ES_Date, ES_Date_NUM_CLASS_PROP, ES_Date_NUM_INSTANCE_PROP, EJS_ATTR_NATIVE | EJS_ATTR_HAS_CONSTRUCTOR);
    ejs->dateType = type;

    /*
     *  Define the helper functions.
     */
    type->helpers->castVar = (EjsCastVarHelper) castDate;
    type->helpers->invokeOperator = (EjsInvokeOperatorHelper) invokeDateOperator;
}


void ejsConfigureDateType(Ejs *ejs)
{
    EjsType     *type;

    type = ejs->dateType;

    ejsBindMethod(ejs, type, ES_Date_Date, (EjsNativeFunction) dateConstructor);
    ejsBindMethod(ejs, type, ES_Date_day, (EjsNativeFunction) getDay);
    ejsBindMethod(ejs, type, ES_Date_dayOfYear, (EjsNativeFunction) getDayOfYear);
    ejsBindMethod(ejs, type, ES_Date_date, (EjsNativeFunction) getDate);
    ejsBindMethod(ejs, type, ES_Date_fullYear, (EjsNativeFunction) getFullYear);
    ejsBindMethod(ejs, type, ES_Date_set_fullYear, (EjsNativeFunction) setFullYear);
    ejsBindMethod(ejs, type, ES_Date_elapsed, (EjsNativeFunction) elapsed);

    ejsBindMethod(ejs, type, ES_Date_format, (EjsNativeFunction) formatDate);

#if ES_Date_getDay
    ejsBindMethod(ejs, type, ES_Date_getDay, (EjsNativeFunction) getDay);
#endif
#if ES_Date_getDate
    ejsBindMethod(ejs, type, ES_Date_getDate, (EjsNativeFunction) getDate);
#endif
#if ES_Date_getFullYear
    ejsBindMethod(ejs, type, ES_Date_getFullYear, (EjsNativeFunction) getFullYear);
#endif
#if ES_Date_getHours
    ejsBindMethod(ejs, type, ES_Date_getHours, (EjsNativeFunction) getHours);
#endif
#if ES_Date_getMilliseconds
    ejsBindMethod(ejs, type, ES_Date_getMilliseconds, (EjsNativeFunction) getMilliseconds);
#endif
#if ES_Date_getMinutes
    ejsBindMethod(ejs, type, ES_Date_getMinutes, (EjsNativeFunction) getMinutes);
#endif
#if ES_Date_getMonth
    ejsBindMethod(ejs, type, ES_Date_getMonth, (EjsNativeFunction) getMonth);
#endif
#if ES_Date_getSeconds
    ejsBindMethod(ejs, type, ES_Date_getSeconds, (EjsNativeFunction) getSeconds);
#endif
#if ES_Date_getTime
    ejsBindMethod(ejs, type, ES_Date_getTime, (EjsNativeFunction) getTime);
#endif
#if ES_Date_getUTCMonth
    ejsBindMethod(ejs, type, ES_Date_getUTCMonth, (EjsNativeFunction) getUTCMonth);
#endif
#if ES_Date_getTimezoneOffset
    ejsBindMethod(ejs, type, ES_Date_getTimezoneOffset, (EjsNativeFunction) getTimezoneOffset);
#endif

    ejsBindMethod(ejs, type, ES_Date_hours, (EjsNativeFunction) getHours);
    ejsBindMethod(ejs, type, ES_Date_set_hours, (EjsNativeFunction) setHours);
    ejsBindMethod(ejs, type, ES_Date_milliseconds, (EjsNativeFunction) getMilliseconds);
    ejsBindMethod(ejs, type, ES_Date_set_milliseconds, (EjsNativeFunction) setMilliseconds);
    ejsBindMethod(ejs, type, ES_Date_minutes, (EjsNativeFunction) getMinutes);
    ejsBindMethod(ejs, type, ES_Date_set_minutes, (EjsNativeFunction) setMinutes);
    ejsBindMethod(ejs, type, ES_Date_month, (EjsNativeFunction) getMonth);
    ejsBindMethod(ejs, type, ES_Date_set_month, (EjsNativeFunction) setMonth);
    ejsBindMethod(ejs, type, ES_Date_nextDay, (EjsNativeFunction) nextDay);
    ejsBindMethod(ejs, type, ES_Date_now, (EjsNativeFunction) now);
    ejsBindMethod(ejs, type, ES_Date_parseDate, (EjsNativeFunction) parseDate);

#if ES_Date_parse
    ejsBindMethod(ejs, type, ES_Date_parse, (EjsNativeFunction) parse);
#endif

    ejsBindMethod(ejs, type, ES_Date_seconds, (EjsNativeFunction) getSeconds);
    ejsBindMethod(ejs, type, ES_Date_set_seconds, (EjsNativeFunction) setSeconds);

#if ES_Date_setFullYear
    ejsBindMethod(ejs, type, ES_Date_setFullYear, (EjsNativeFunction) setFullYear);
#endif
#if ES_Date_setMilliseconds
    ejsBindMethod(ejs, type, ES_Date_setMilliseconds, (EjsNativeFunction) setMilliseconds);
#endif
#if ES_Date_setMinutes
    ejsBindMethod(ejs, type, ES_Date_setMinutes, (EjsNativeFunction) setMinutes);
#endif
#if ES_Date_setMonth
    ejsBindMethod(ejs, type, ES_Date_setMonth, (EjsNativeFunction) setMonth);
#endif
#if ES_Date_setSeconds
    ejsBindMethod(ejs, type, ES_Date_setSeconds, (EjsNativeFunction) setSeconds);
#endif
#if ES_Date_setTime
    ejsBindMethod(ejs, type, ES_Date_setTime, (EjsNativeFunction) setTime);
#endif

    ejsBindMethod(ejs, type, ES_Date_time, (EjsNativeFunction) getTime);
    ejsBindMethod(ejs, type, ES_Date_set_time, (EjsNativeFunction) setTime);

#if ES_Date_toDateString
    ejsBindMethod(ejs, type, ES_Date_toDateString, (EjsNativeFunction) toDateString);
#endif
#if ES_Date_toIOString
    ejsBindMethod(ejs, type, ES_Date_toIOString, (EjsNativeFunction) toIOString);
#endif
#if ES_Date_toJSONString
    ejsBindMethod(ejs, type, ES_Date_toJSONString, (EjsNativeFunction) toJSONString);
#endif
#if ES_Date_toLocaleDateString
    ejsBindMethod(ejs, type, ES_Date_toLocaleDateString, (EjsNativeFunction) toLocaleDateString);
#endif
#if ES_Date_toLocaleString
    ejsBindMethod(ejs, type, ES_Date_toLocaleString, (EjsNativeFunction) toLocaleString);
#endif
#if ES_Date_toLocaleTimeString
    ejsBindMethod(ejs, type, ES_Date_toLocaleTimeString, (EjsNativeFunction) toLocaleTimeString);
#endif
    ejsBindMethod(ejs, type, ES_Object_toString, (EjsNativeFunction) dateToString);
#if ES_Date_toTimeString
    ejsBindMethod(ejs, type, ES_Date_toTimeString, (EjsNativeFunction) toTimeString);
#endif
#if ES_Date_toUTCString
    ejsBindMethod(ejs, type, ES_Date_toUTCString, (EjsNativeFunction) toUTCString);
#endif
#if ES_Date_UTC
    ejsBindMethod(ejs, type, ES_Date_UTC, (EjsNativeFunction) UTC);
#endif
#if ES_Date_UTCmonth
    ejsBindMethod(ejs, type, ES_Date_UTCmonth, (EjsNativeFunction) UTCmonth);
#endif

#if ES_Date_valueOf
    ejsBindMethod(ejs, type, ES_Date_valueOf, (EjsNativeFunction) valueOf);
#endif
    ejsBindMethod(ejs, type, ES_Date_year, (EjsNativeFunction) getYear);
    ejsBindMethod(ejs, type, ES_Date_set_year, (EjsNativeFunction) setYear);
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
