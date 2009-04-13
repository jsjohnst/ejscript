/**
 *  ejsString.c - Ejscript string class
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejs.h"

/***************************** Forward Declarations ***************************/

static int catString(Ejs *ejs, EjsString *dest, char *str, int len);
static int indexof(cchar *str, int len, cchar *pattern, int patlen, int dir);

/******************************************************************************/
/*
 *  Cast the string operand to a primitive type
 */

static EjsVar *castString(Ejs *ejs, EjsString *sp, EjsType *type)
{
    mprAssert(sp);
    mprAssert(type);

    switch (type->id) {

    case ES_Boolean:
        return (EjsVar*) ejs->trueValue;

    case ES_Number:
        return (EjsVar*) ejsParseVar(ejs, sp->value, ES_Number);

#if ES_RegExp && BLD_FEATURE_REGEXP
    case ES_RegExp:
        if (sp->value && sp->value[0] == '/') {
            return (EjsVar*) ejsCreateRegExp(ejs, sp->value);
        } else {
            EjsVar      *result;
            char        *buf;
            mprAllocStrcat(ejs, &buf, -1, NULL, "/", sp->value, "/", NULL);
            result = (EjsVar*) ejsCreateRegExp(ejs, buf);
            mprFree(buf);
            return result;
        }
#endif

    default:
        ejsThrowTypeError(ejs, "Can't cast to this type");
        return 0;
    }
    return 0;
}


/*
 *  Clone a string. Shallow copies simply return a reference as strings are immutable.
 */
static EjsString *cloneString(Ejs *ejs, EjsString *sp, bool deep)
{
    if (deep) {
        return ejsCreateStringWithLength(ejs, sp->value, sp->length);
    }
    return sp;
}


static void destroyString(Ejs *ejs, EjsString *sp)
{
    mprAssert(sp);

    mprFree(sp->value);
    sp->value = 0;
    ejsFreeVar(ejs, (EjsVar*) sp);
}


/*
 *  Get a string element. Slot numbers correspond to character indicies.
 */
static EjsVar *getStringProperty(Ejs *ejs, EjsString *sp, int index)
{
    if (index < 0 || index >= sp->length) {
        ejsThrowOutOfBoundsError(ejs, "Bad string subscript");
        return 0;
    }
    return (EjsVar*) ejsCreateStringWithLength(ejs, &sp->value[index], 1);
}


static EjsVar *coerceStringOperands(Ejs *ejs, EjsVar *lhs, int opcode,  EjsVar *rhs)
{
    switch (opcode) {
    /*
     *  Binary operators
     */
    case EJS_OP_ADD:
        return ejsInvokeOperator(ejs, lhs, opcode, (EjsVar*) ejsToString(ejs, rhs));

    /*
     *  Overloaded operators
     */
    case EJS_OP_MUL:
        if (ejsIsNumber(rhs)) {
            return 0;
        }
        return ejsInvokeOperator(ejs, lhs, opcode, (EjsVar*) ejsToNumber(ejs, rhs));

    case EJS_OP_REM:
        return 0;

    case EJS_OP_AND: case EJS_OP_DIV: case EJS_OP_OR:
    case EJS_OP_SHL: case EJS_OP_SHR: case EJS_OP_SUB: case EJS_OP_USHR: case EJS_OP_XOR:
        return ejsInvokeOperator(ejs, (EjsVar*) ejsToNumber(ejs, lhs), opcode, rhs);

    case EJS_OP_COMPARE_EQ: case EJS_OP_COMPARE_NE:
    case EJS_OP_COMPARE_LE: case EJS_OP_COMPARE_LT:
    case EJS_OP_COMPARE_GE: case EJS_OP_COMPARE_GT:
        return ejsInvokeOperator(ejs, lhs, opcode, (EjsVar*) ejsToString(ejs, rhs));

    case EJS_OP_COMPARE_STRICTLY_NE:
        return (EjsVar*) ejs->trueValue;

    case EJS_OP_COMPARE_STRICTLY_EQ:
        return (EjsVar*) ejs->falseValue;

    /*
     *  Unary operators
     */
    case EJS_OP_LOGICAL_NOT:
        return ejsInvokeOperator(ejs, (EjsVar*) ejsToBoolean(ejs, lhs), opcode, rhs);

    case EJS_OP_NOT:
    case EJS_OP_NEG:
        return ejsInvokeOperator(ejs, (EjsVar*) ejsToNumber(ejs, lhs), opcode, rhs);

    case EJS_OP_COMPARE_NOT_ZERO:
    case EJS_OP_COMPARE_TRUE:
        return (EjsVar*) (((EjsString*) lhs)->value ? ejs->trueValue : ejs->falseValue);

    case EJS_OP_COMPARE_ZERO:
    case EJS_OP_COMPARE_FALSE:
        return (EjsVar*) (((EjsString*) lhs)->value ? ejs->falseValue: ejs->trueValue);

    case EJS_OP_COMPARE_UNDEFINED:
    case EJS_OP_COMPARE_NULL:
        return (EjsVar*) ejs->falseValue;

    default:
        ejsThrowTypeError(ejs, "Opcode %d not valid for type %s", opcode, lhs->type->qname.name);
        return ejs->undefinedValue;
    }
    return 0;
}


static EjsVar *invokeStringOperator(Ejs *ejs, EjsString *lhs, int opcode,  EjsString *rhs, void *data)
{
    EjsVar      *result;
#if BLD_FEATURE_EJS_LANG >= EJS_SPEC_PLUS
    EjsVar      *arg;
#endif

    if (rhs == 0 || lhs->var.type != rhs->var.type) {
        if ((result = coerceStringOperands(ejs, (EjsVar*) lhs, opcode, (EjsVar*) rhs)) != 0) {
            return result;
        }
    }
    /*
     *  Types now match, both strings
     */
    switch (opcode) {
    case EJS_OP_COMPARE_STRICTLY_EQ:
    case EJS_OP_COMPARE_EQ:
        if (lhs == rhs || (lhs->value == rhs->value)) {
            return (EjsVar*) ejs->trueValue;
        }
        return (EjsVar*) ejsCreateBoolean(ejs,  mprMemcmp(lhs->value, lhs->length, rhs->value, rhs->length) == 0);

    case EJS_OP_COMPARE_NE:
    case EJS_OP_COMPARE_STRICTLY_NE:
        if (lhs->length != rhs->length) {
            return (EjsVar*) ejs->trueValue;
        }
        return (EjsVar*) ejsCreateBoolean(ejs,  mprMemcmp(lhs->value, lhs->length, rhs->value, rhs->length) != 0);

    case EJS_OP_COMPARE_LT:
        return (EjsVar*) ejsCreateBoolean(ejs,  mprMemcmp(lhs->value, lhs->length, rhs->value, rhs->length) < 0);

    case EJS_OP_COMPARE_LE:
        return (EjsVar*) ejsCreateBoolean(ejs,  mprMemcmp(lhs->value, lhs->length, rhs->value, rhs->length) <= 0);

    case EJS_OP_COMPARE_GT:
        return (EjsVar*) ejsCreateBoolean(ejs,  mprMemcmp(lhs->value, lhs->length, rhs->value, rhs->length) > 0);

    case EJS_OP_COMPARE_GE:
        return (EjsVar*) ejsCreateBoolean(ejs,  mprMemcmp(lhs->value, lhs->length, rhs->value, rhs->length) >= 0);

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

    /*
     *  Binary operators
     */
    case EJS_OP_ADD:
        result = (EjsVar*) ejsCreateString(ejs, lhs->value);
        ejsStrcat(ejs, (EjsString*) result, (EjsVar*) rhs);
        return result;

    case EJS_OP_AND: case EJS_OP_DIV: case EJS_OP_OR:
    case EJS_OP_SHL: case EJS_OP_SHR: case EJS_OP_USHR: case EJS_OP_XOR:
        return ejsInvokeOperator(ejs, (EjsVar*) ejsToNumber(ejs, (EjsVar*) lhs), opcode, (EjsVar*) rhs);

#if BLD_FEATURE_EJS_LANG >= EJS_SPEC_PLUS
    /*
     *  Overloaded
     */
    case EJS_OP_SUB:
        arg = (EjsVar*) rhs;
        //  TODO OPT - inline this capability
        return ejsRunFunctionBySlot(ejs, (EjsVar*) lhs, ES_String_MINUS, 1, &arg);

    case EJS_OP_REM:
        arg = (EjsVar*) rhs;
        return ejsRunFunctionBySlot(ejs, (EjsVar*) lhs, ES_String_MOD, 1, &arg);
#endif

    case EJS_OP_NEG:
    case EJS_OP_LOGICAL_NOT:
    case EJS_OP_NOT:
        /* Already handled in coerceStringOperands */
    default:
        ejsThrowTypeError(ejs, "Opcode %d not implemented for type %s", opcode, lhs->var.type->qname.name);
        return 0;
    }
    mprAssert(0);
}


/*
 *  Lookup an string index.
 */
static int lookupStringProperty(struct Ejs *ejs, EjsString *sp, EjsName *qname)
{
    int     index;

    if (qname == 0 || ! isdigit((int) qname->name[0])) {
        return EJS_ERR;
    }
    index = atoi(qname->name);
    if (index < sp->length) {
        return index;
    }

    return EJS_ERR;
}


/************************************ Methods *********************************/
/*
 *  String constructor.
 *
 *      function String()
 *      function String(str: String)
 */
//  TODO - refactor

static EjsVar *stringConstructor(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    EjsArray    *args;
    EjsString   *str;

    mprAssert(argc == 0 || (argc == 1 && ejsIsArray(argv[0])));

    if (argc == 1) {
        args = (EjsArray*) argv[0];
        if (args->length > 0) {
            str = ejsToString(ejs, ejsGetProperty(ejs, (EjsVar*) args, 0));
            if (str) {
                sp->value = mprStrdup(sp, str->value);
                sp->length = str->length;
            }
        } else {
            sp->value = mprStrdup(ejs, "");
            if (sp->value == 0) {
                return 0;
            }
            sp->length = 0;
        }

    } else {
        sp->value = mprStrdup(ejs, "");
        if (sp->value == 0) {
            return 0;
        }
        sp->length = 0;
    }
    return (EjsVar*) sp;
}


/*
 *  Do a case sensitive comparison between this string and another.
 *
 *  function caseCompare(compare: String): Number
 */
static EjsVar *caseCompare(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    int     result;

    mprAssert(argc == 1 && ejsIsString(argv[0]));

    result = mprStrcmp(sp->value, ((EjsString*) argv[0])->value);

    return (EjsVar*) ejsCreateNumber(ejs, result);
}


/*
 *  Return a string containing the character at a given index
 *
 *  function charAt(index: Number): String
 */
static EjsVar *charAt(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    int     index;

    mprAssert(argc == 1 && ejsIsNumber(argv[0]));

    index = ejsGetInt(argv[0]);
    if (index < 0 || index >= sp->length) {
        ejsThrowOutOfBoundsError(ejs, "Bad string subscript");
        return 0;
    }

    return (EjsVar*) ejsCreateStringWithLength(ejs, &sp->value[index], 1);
}


/*
 *  Return an integer containing the character at a given index
 *
 *  function charCodeAt(index: Number = 0): Number
 */

static EjsVar *charCodeAt(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    int     index;

    index = (argc == 1) ? ejsGetInt(argv[0]) : 0;

    if (index < 0 || index >= sp->length) {
        ejsThrowOutOfBoundsError(ejs, "Bad string subscript");
        return 0;
    }

    return (EjsVar*) ejsCreateNumber(ejs, sp->value[index]);
}


/*
 *  Catenate args to a string and return a new string.
 *
 *  function concat(...args): String
 */
static EjsVar *concatString(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    EjsArray    *args;
    EjsString   *result;
    int         i, count;

    mprAssert(argc == 1 && ejsIsArray(argv[0]));
    args = (EjsArray*) argv[0];

    result = ejsDupString(ejs, sp);

    count = ejsGetPropertyCount(ejs, (EjsVar*) args);
    for (i = 0; i < args->length; i++) {
        if (ejsStrcat(ejs, result, ejsGetProperty(ejs, (EjsVar*) args, i)) < 0) {
            ejsThrowMemoryError(ejs);
            return 0;
        }
    }
    return (EjsVar*) result;
}


/**
 *  Check if a string contains the pattern (string or regexp)
 *
 *  function contains(pattern: Object): Boolean
 */
static EjsVar *containsString(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    EjsVar      *pat;

    pat = argv[0];

    if (ejsIsString(pat)) {
        return (EjsVar*) ejsCreateBoolean(ejs, strstr(sp->value, ((EjsString*) pat)->value) != 0);

#if BLD_FEATURE_REGEXP
    } else if (ejsIsRegExp(pat)) {
        EjsRegExp   *rp;
        int         count;
        rp = (EjsRegExp*) argv[0];
        count = pcre_exec(rp->compiled, NULL, sp->value, sp->length, 0, 0, 0, 0);
        return (EjsVar*) ejsCreateBoolean(ejs, count >= 0);
#endif
    }
    ejsThrowTypeError(ejs, "Wrong argument type. Must be string or regular expression");
    return 0;
}


/**
 *  Check if a string ends with a given pattern
 *
 *  function endsWith(pattern: String): Boolean
 */
static EjsVar *endsWith(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    char        *pattern;
    int         len;

    mprAssert(argc == 1 && ejsIsString(argv[0]));

    pattern = ejsGetString(argv[0]);
    len = (int) strlen(pattern);
    if (len > sp->length) {
        return (EjsVar*) ejs->falseValue;
    }
    return (EjsVar*) ejsCreateBoolean(ejs, strncmp(&sp->value[sp->length - len], pattern, len) == 0);
}


/**
 *  Format the arguments
 *
 *  function format(...args): String
 *
 *  Format:         %[modifier][width][precision][bits][type]
 */
static EjsVar *formatString(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    EjsArray    *args, *inner;
    EjsString   *result;
    EjsVar      *value;
    char        *buf;
    char        fmt[16];
    int         c, i, len, nextArg, start, kind, last;

    mprAssert(argc == 1 && ejsIsArray(argv[0]));

    args = (EjsArray*) argv[0];

    /*
     *  Flatten the args if there is only one element and it is itself an array. This happens when invoked
     *  via the overloaded operator '%' which in turn invokes format()
     */
    if (args->length == 1) {
        inner = (EjsArray*) ejsGetProperty(ejs, (EjsVar*) args, 0);
        if (ejsIsArray(inner)) {
            args = inner;
        }
    }

    result = ejsCreateString(ejs, 0);

    if (result == 0) {
        ejsThrowMemoryError(ejs);
        return 0;
    }

    /*
     *  Parse the format string and extract one specifier at a time.
     */
    last = 0;
    for (i = 0, nextArg = 0; i < sp->length && nextArg < args->length; i++) {
        c = sp->value[i];
        if (c != '%') {
            continue;
        }

        if (i > last) {
            catString(ejs, result, &sp->value[last], i - last);
        }

        /*
         *  Find the end of the format specifier and determine the format type (kind)
         */
        start = i++;
        i += (int) strspn(&sp->value[i], "-+ #,0*123456789.hlL");
        kind = sp->value[i];

        if (strchr("cdefginopsSuxX", kind)) {
            len = i - start + 1;
            mprMemcpy(fmt, sizeof(fmt), &sp->value[start], len);
            fmt[len] = '\0';

            value = ejsGetProperty(ejs, (EjsVar*) args, nextArg);

            buf = 0;
            switch (kind) {
            case 'd': case 'i': case 'o': case 'u':
                value = (EjsVar*) ejsToNumber(ejs, value);
                len = mprAllocSprintf(ejs, &buf, -1, fmt, (int64) ejsGetNumber(value));
                break;
#if BLD_FEATURE_FLOATING_POINT
            case 'e': case 'g': case 'f':
                value = (EjsVar*) ejsToNumber(ejs, value);
                len = mprAllocSprintf(ejs, &buf, -1, fmt, (double) ejsGetNumber(value));
                break;
#endif
            case 's':
                value = (EjsVar*) ejsToString(ejs, value);
                len = mprAllocSprintf(ejs, &buf, -1, fmt, ejsGetString(value));
                break;

            case 'X': case 'x':
                len = mprAllocSprintf(ejs, &buf, -1, fmt, (int64) ejsGetNumber(value));
                break;

            case 'n':
                len = mprAllocVsprintf(ejs, &buf, -1, fmt, 0);

            default:
                ejsThrowArgError(ejs, "Bad format specifier");
                return 0;
            }
            catString(ejs, result, buf, len);
            mprFree(buf);
            last = i + 1;
            nextArg++;
        }
    }

    i = (int) strlen(sp->value);
    if (i > last) {
        catString(ejs, result, &sp->value[last], i - last);
    }

    return (EjsVar*) result;
}


/*
 *  Create a string from character codes
 *
 *  static function fromCharCode(...codes): String
 */
static EjsVar *fromCharCode(Ejs *ejs, EjsString *unused, int argc, EjsVar **argv)
{
    EjsString   *result;
    EjsArray    *args;
    EjsVar      *vp;
    int         i;

    mprAssert(argc == 1 && ejsIsArray(argv[0]));
    args = (EjsArray*) argv[0];

    result = (EjsString*) ejsCreateBareString(ejs, argc + 1);
    if (result == 0) {
        return 0;
    }

    for (i = 0; i < args->length; i++) {
        vp = ejsGetProperty(ejs, (EjsVar*) args, i);
        result->value[i] = ejsGetInt(ejsToNumber(ejs, vp));
    }
    result->value[i] = '\0';
    result->length = args->length;

    return (EjsVar*) result;
}


/*
 *  Function to iterate and return the next character code.
 *  NOTE: this is not a method of String. Rather, it is a callback function for Iterator
 */
static EjsVar *nextStringKey(Ejs *ejs, EjsIterator *ip, int argc, EjsVar **argv)
{
    EjsString   *sp;

    sp = (EjsString*) ip->target;

    if (!ejsIsString(sp)) {
        ejsThrowReferenceError(ejs, "Wrong type");
        return 0;
    }

    if (ip->index < sp->length) {
        return (EjsVar*) ejsCreateNumber(ejs, ip->index++);
    }
    ejsThrowStopIteration(ejs);
    return 0;
}


/*
 *  Return the default iterator. This returns the array index names.
 *
 *  iterator function get(): Iterator
 */
static EjsVar *getStringIterator(Ejs *ejs, EjsVar *sp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateIterator(ejs, sp, (EjsNativeFunction) nextStringKey, 0, NULL);
}


/*
 *  Function to iterate and return the next string character (as a string).
 *  NOTE: this is not a method of Array. Rather, it is a callback function for Iterator
 */
static EjsVar *nextStringValue(Ejs *ejs, EjsIterator *ip, int argc, EjsVar **argv)
{
    EjsString   *sp;

    sp = (EjsString*) ip->target;
    if (!ejsIsString(sp)) {
        ejsThrowReferenceError(ejs, "Wrong type");
        return 0;
    }

    if (ip->index < sp->length) {
        return (EjsVar*) ejsCreateStringWithLength(ejs, &sp->value[ip->index++], 1);
    }
    ejsThrowStopIteration(ejs);
    return 0;
}


/*
 *  Return an iterator to return the next array element value.
 *
 *  iterator function getValues(): Iterator
 */
static EjsVar *getStringValues(Ejs *ejs, EjsVar *sp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateIterator(ejs, sp, (EjsNativeFunction) nextStringValue, 0, NULL);
}


/*
 *  Get the length of a string.
 *  @return Returns the number of characters in the string
 *
 *  override function get length(): Number
 */

static EjsVar *stringLength(Ejs *ejs, EjsString *ap, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, ap->length);
}


/*
 *  Return the position of the first occurance of a substring
 *
 *  function indexOf(pattern: String, startIndex: Number = 0): Number
 */
static EjsVar *indexOf(Ejs *ejs, EjsString *sp, int argc,  EjsVar **argv)
{
    char    *pattern;
    int     index, start, patternLength;

    mprAssert(1 <= argc && argc <= 2);
    mprAssert(ejsIsString(argv[0]));

    pattern = ejsGetString(argv[0]);
    patternLength = ((EjsString*) argv[0])->length;

    if (argc == 2) {
        start = ejsGetInt(argv[1]);
        if (start > sp->length) {
            start = sp->length;
        }
    } else {
        start = 0;
    }

    index = indexof(&sp->value[start], sp->length - start, pattern, patternLength, 1);
    if (index < 0) {
        return (EjsVar*) ejs->minusOneValue;
    }
    return (EjsVar*) ejsCreateNumber(ejs, index + start);
}


static EjsVar *isAlpha(Ejs *ejs, EjsString *sp, int argc,  EjsVar **argv)
{
    return (EjsVar*) ejsCreateBoolean(ejs, isalpha((int) sp->value[0]));
}


static EjsVar *isDigit(Ejs *ejs, EjsString *sp, int argc,  EjsVar **argv)
{
    return (EjsVar*) ejsCreateBoolean(ejs, isdigit((int) sp->value[0]));
}


static EjsVar *isLower(Ejs *ejs, EjsString *sp, int argc,  EjsVar **argv)
{
    return (EjsVar*) ejsCreateBoolean(ejs, islower((int) sp->value[0]));
}


static EjsVar *isSpace(Ejs *ejs, EjsString *sp, int argc,  EjsVar **argv)
{
    return (EjsVar*) ejsCreateBoolean(ejs, isspace((int) sp->value[0]));
}


static EjsVar *isUpper(Ejs *ejs, EjsString *sp, int argc,  EjsVar **argv)
{
    return (EjsVar*) ejsCreateBoolean(ejs, isupper((int) sp->value[0]));
}


/*
 *  Return the position of the last occurance of a substring
 *
 *  function lastIndexOf(pattern: String, start: Number = -1): Number
 */
static EjsVar *lastIndexOf(Ejs *ejs, EjsString *sp, int argc,  EjsVar **argv)
{
    char    *pattern;
    int     start, patternLength, index;

    mprAssert(1 <= argc && argc <= 2);

    pattern = ejsGetString(argv[0]);
    patternLength = ((EjsString*) argv[0])->length;

    if (argc == 2) {
        start = ejsGetInt(argv[1]);
        if (start > sp->length) {
            start = sp->length;
        }

    } else {
        start = 0;
    }

    if (start < 0 || start >= sp->length) {
        ejsThrowOutOfBoundsError(ejs, "Bad start subscript");
        return 0;
    }

    index = indexof(sp->value, sp->length, pattern, patternLength, -1);
    if (index < 0) {
        return (EjsVar*) ejs->minusOneValue;
    }
    return (EjsVar*) ejsCreateNumber(ejs, index);
}


#if BLD_FEATURE_REGEXP
/*
 *  Match a pattern
 *
 *  function match(pattern: RegExp): Array
 */
static EjsVar *match(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    EjsRegExp   *rp;
    EjsArray    *results;
    EjsString   *match;
    int         matches[EJS_MAX_REGEX_MATCHES * 3];
    int         count, len, resultCount;

    rp = (EjsRegExp*) argv[0];
    rp->endLastMatch = 0;
    results = NULL;
    resultCount = 0;

    do {
        count = pcre_exec(rp->compiled, NULL, sp->value, sp->length, rp->endLastMatch, 0, matches, sizeof(matches) / sizeof(int));
        if (count <= 0) {
            break;
        }
        if (results == 0) {
            results = ejsCreateArray(ejs, count);
        }
        len = matches[1] - matches[0];
        match = ejsCreateStringWithLength(ejs, &sp->value[matches[0]], len);
        ejsSetProperty(ejs, (EjsVar*) results, resultCount++, (EjsVar*) match);
        rp->endLastMatch = matches[1];

    } while (rp->global);

    if (results == NULL) {
        return (EjsVar*) ejs->nullValue;
    }
    return (EjsVar*) results;
}
#endif


#if ES_String_parseJSON
static EjsVar *parseJSON(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    return 0;
}
#endif


static EjsVar *printable(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    EjsString       *result;
    char            buf[8];
    int             i, j, k, len, nonprint;

    nonprint = 0;
    for (i = 0; i < sp->length; i++)  {
        if (!isprint((int) sp->value[i])) {
            nonprint++;
        }
    }
    if (nonprint == 0) {
        return (EjsVar*) sp;
    }

    result = ejsCreateBareString(ejs, sp->length + (nonprint * 5));
    if (result == 0) {
        return 0;
    }
    for (i = 0, j = 0; i < sp->length; i++)  {
        if (isprint((int) sp->value[i])) {
            result->value[j++] = sp->value[i];
        } else {
            result->value[j++] = '\\';
            result->value[j++] = 'u';
            mprItoa(buf, 4, sp->value[i], 16);
            len = (int) strlen(buf);
            for (k = len; k < 4; k++) {
                result->value[j++] = '0';
            }
            for (k = 0; buf[k]; k++) {
                result->value[j++] = buf[k];
            }
        }
    }
    result->value[j] = '\0';
    return (EjsVar*) result;
}


static EjsVar *quote(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    EjsString       *result;

    result = ejsCreateBareString(ejs, sp->length + 2);
    if (result == 0) {
        return 0;
    }

    memcpy(&result->value[1], sp->value, sp->length);
    result->value[0] = '"';
    result->value[sp->length + 1] = '"';
    result->value[sp->length + 2] = '\0';
    result->length = sp->length + 2;

    return (EjsVar*) result;
}


/*
 *  Remove characters and return a new string.
 *
 *  function remove(start: Number, end: Number = -1): String
 *
 */
static EjsVar *removeCharsFromString(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    EjsString       *result;
    int             start, end, i, j;

    mprAssert(1 <= argc && argc <= 2);

    start = ejsGetInt(argv[0]);
    end = ejsGetInt(argv[1]);

    if (start < 0) {
        start += sp->length;
    }
    if (end < 0) {
        end += sp->length;
    }
    if (start >= sp->length) {
        start = sp->length - 1;
    }
    if (end > sp->length) {
        end = sp->length;
    }

    result = ejsCreateBareString(ejs, sp->length - (end - start));
    if (result == 0) {
        return 0;
    }
    for (j = i = 0; i < start; i++, j++) {
        result->value[j] = sp->value[i];
    }
    for (i = end; i < sp->length; i++, j++) {
        result->value[j] = sp->value[i];
    }
    result->value[j] = '\0';

    return (EjsVar*) result;
}


/*
 *  Search and replace.
 *
 *  function replace(pattern: (String|Regexp), replacement: String): String
 *
 */
static EjsVar *replace(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    EjsString   *result, *replacement;
    char        *pattern;
    int         index, patternLength;

    mprAssert(argc == 2);
    result = 0;
    replacement = (EjsString*) argv[1];

    if (ejsIsString(argv[0])) {
        pattern = ejsGetString(argv[0]);
        patternLength = ((EjsString*) argv[0])->length;

        index = indexof(sp->value, sp->length, pattern, patternLength, 1);
        if (index >= 0) {
            result = ejsCreateString(ejs, 0);
            if (result == 0) {
                return 0;
            }
            catString(ejs, result, sp->value, index);
            catString(ejs, result, replacement->value, replacement->length);

            index += patternLength;
            if (index < sp->length) {
                catString(ejs, result, &sp->value[index], sp->length - index);
            }

        } else {
            result = ejsDupString(ejs, sp);
        }

#if BLD_FEATURE_REGEXP
    } else if (ejsIsRegExp(argv[0])) {
        EjsRegExp   *rp;
        char        *cp, *lastReplace, *end;
        int         matches[EJS_MAX_REGEX_MATCHES * 3];
        int         count, endLastMatch, submatch;

        rp = (EjsRegExp*) argv[0];
        result = ejsCreateString(ejs, 0);
        endLastMatch = 0;

        do {
            count = pcre_exec(rp->compiled, NULL, sp->value, sp->length, endLastMatch, 0, matches, sizeof(matches) / sizeof(int));
            if (count <= 0) {
                break;
            }

            if (endLastMatch < matches[0]) {
                /* Append prior string text */
                catString(ejs, result, &sp->value[endLastMatch], matches[0] - endLastMatch);
            }

            /*
             *  Process the replacement template
             */
            end = &replacement->value[replacement->length];
            lastReplace = replacement->value;

            for (cp = replacement->value; cp < end; ) {
                if (*cp == '$') {
                    if (lastReplace < cp) {
                        catString(ejs, result, lastReplace, (int) (cp - lastReplace));
                    }
                    switch (*++cp) {
                    case '$':
                        catString(ejs, result, "$", 1);
                        break;
                    case '&':
                        /* Replace the matched string */
                        catString(ejs, result, &sp->value[matches[0]], matches[1] - matches[0]);
                        break;
                    case '`':
                        /* Insert the portion that preceeds the matched string */
                        catString(ejs, result, sp->value, matches[0]);
                        break;
                    case '\'':
                        /* Insert the portion that follows the matched string */
                        catString(ejs, result, &sp->value[matches[1]], sp->length - matches[1]);
                        break;
                    default:
                        /* Insert the nth submatch */
                        if (isdigit((int) *cp)) {
                            submatch = mprAtoi(cp, 10);
                            while (isdigit((int) *++cp))
                                ;
                            cp--;
                            if (submatch < count) {
                                submatch *= 2;
                                catString(ejs, result, &sp->value[matches[submatch]], 
                                    matches[submatch + 1] - matches[submatch]);
                            }

                        } else {
                            ejsThrowArgError(ejs, "Bad replacement $ specification");
                            return 0;
                        }
                    }
                    lastReplace = cp + 1;
                }
                cp++;
            }
            if (lastReplace < cp && lastReplace < end) {
                catString(ejs, result, lastReplace, (int) (cp - lastReplace));
            }
            endLastMatch = matches[1];

        } while (rp->global);

        if (endLastMatch < sp->length) {
            /* Append remaining string text */
            catString(ejs, result, &sp->value[endLastMatch], sp->length - endLastMatch);
        }
#endif

    } else {
        ejsThrowTypeError(ejs, "Wrong argument type. Must be string or regular expression");
        return 0;
    }
    return (EjsVar*) result;
}


static EjsVar *reverseString(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    int         i, j, tmp;

    if (sp->length <= 1) {
        return (EjsVar*) sp;
    }
    i = (sp->length - 2) / 2;
    j = (sp->length + 1) / 2;
    for (; i >= 0; i--, j++) {
        tmp = sp->value[i];
        sp->value[i] = sp->value[j];
        sp->value[j] = tmp;
    }
    return (EjsVar*) sp;
}


/*
 *  Search for a pattern
 *
 *  function search(pattern: (String | RegExp)): Number
 *
 */
static EjsVar *searchString(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    char        *pattern;
    int         index, patternLength;

    if (ejsIsString(argv[0])) {
        pattern = ejsGetString(argv[0]);
        patternLength = ((EjsString*) argv[0])->length;

        index = indexof(sp->value, sp->length, pattern, patternLength, 1);
        return (EjsVar*) ejsCreateNumber(ejs, index);

#if BLD_FEATURE_REGEXP
    } else if (ejsIsRegExp(argv[0])) {
        EjsRegExp   *rp;
        int         matches[EJS_MAX_REGEX_MATCHES * 3];
        int         count;
        rp = (EjsRegExp*) argv[0];
        count = pcre_exec(rp->compiled, NULL, sp->value, sp->length, 0, 0, matches, sizeof(matches) / sizeof(int));
        if (count < 0) {
            return (EjsVar*) ejs->minusOneValue;
        }
        return (EjsVar*) ejsCreateNumber(ejs, matches[0]);
#endif

    } else {
        ejsThrowTypeError(ejs, "Wrong argument type. Must be string or regular expression");
        return 0;
    }
}


/*
 *  Return a substring. End is one past the last character.
 *
 *  function slice(start: Number, end: Number = -1, step: Number = 1): String
 */
static EjsVar *sliceString(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    EjsString       *result;
    int             start, end, step, i, j;

    mprAssert(1 <= argc && argc <= 2);

    start = ejsGetInt(argv[0]);
    if (argc == 2) {
        end = ejsGetInt(argv[1]);
    } else {
        end = sp->length;
    }
    if (argc == 3) {
        step = ejsGetInt(argv[1]);
    } else {
        step = 1;
    }

    if (start < 0) {
        start += sp->length;
    }
    if (end < 0) {
        end += sp->length;
    }
    if (step == 0) {
        step = 1;
    }
    if (start < 0 || start >= sp->length) {
        ejsThrowOutOfBoundsError(ejs, "Bad start subscript");
        return 0;
    }
    if (end < 0 || end > sp->length) {
        ejsThrowOutOfBoundsError(ejs, "Bad end subscript");
        return 0;
    }

    result = ejsCreateBareString(ejs, (end - start) / abs(step));
    if (result == 0) {
        return 0;
    }
    if (step > 0) {
        for (i = start, j = 0; i < end; i += step) {
            result->value[j++] = sp->value[i];
        }

    } else {
        for (i = end - 1, j = 0; i >= start; i += step) {
            result->value[j++] = sp->value[i];
        }
    }

    result->value[j] = '\0';
    result->length = j;

    return (EjsVar*) result;
}


/*
 *  Split a string
 *
 *  function split(delimiter: (String | RegExp), limit: Number = -1): Array
 */
static EjsVar *split(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    EjsArray    *results;
    EjsString   *elt;
    char        *delim, *cp, *endp;
    int         delimLen, limit;

    mprAssert(1 <= argc && argc <= 2);

    limit = (argc == 2) ? ejsGetInt(argv[1]): -1;
    results = ejsCreateArray(ejs, 0);

    if (ejsIsString(argv[0])) {
        delim = ejsGetString(argv[0]);
        delimLen = (int) strlen(delim);

        endp = cp = sp->value;
        for (; *endp; endp++) {
            if (strncmp(endp, delim, delimLen) == 0 && endp > cp) {
                if (--limit == -1) {
                    return (EjsVar*) results;
                }
                elt = ejsCreateStringWithLength(ejs, cp, (int) (endp - cp));
                ejsSetProperty(ejs, (EjsVar*) results, -1, (EjsVar*) elt);
                cp = endp + delimLen;
                endp = cp;
            }
        }
        if (endp > cp) {
            elt = ejsCreateStringWithLength(ejs, cp, (int) (endp - cp));
            ejsSetProperty(ejs, (EjsVar*) results, -1, (EjsVar*) elt);
        }
        return (EjsVar*) results;

#if BLD_FEATURE_REGEXP
    } else if (ejsIsRegExp(argv[0])) {
        EjsRegExp   *rp;
        EjsString   *match;
        int         matches[EJS_MAX_REGEX_MATCHES * 3], count, resultCount;
        rp = (EjsRegExp*) argv[0];
        rp->endLastMatch = 0;
        resultCount = 0;
        do {
            count = pcre_exec(rp->compiled, NULL, sp->value, sp->length, rp->endLastMatch, 0, matches, 
                sizeof(matches) / sizeof(int));
            if (count <= 0) {
                break;
            }
            if (rp->endLastMatch < matches[0]) {
                match = ejsCreateStringWithLength(ejs, &sp->value[rp->endLastMatch], matches[0] - rp->endLastMatch);
                ejsSetProperty(ejs, (EjsVar*) results, resultCount++, (EjsVar*) match);
            }
            rp->endLastMatch = matches[1];
        } while (rp->global);

        if (rp->endLastMatch < sp->length) {
            match = ejsCreateStringWithLength(ejs, &sp->value[rp->endLastMatch], sp->length - rp->endLastMatch);
            ejsSetProperty(ejs, (EjsVar*) results, resultCount++, (EjsVar*) match);
        }
        return (EjsVar*) results;
#endif
    }

    ejsThrowTypeError(ejs, "Wrong argument type. Must be string or regular expression");
    return 0;
}


/**
 *  Check if a string starts with a given pattern
 *
 *  function startsWith(pattern: String): Boolean
 */
static EjsVar *startsWith(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    char        *pattern;
    int         len;

    mprAssert(argc == 1 && ejsIsString(argv[0]));

    pattern = ejsGetString(argv[0]);
    len = (int) strlen(pattern);

    return (EjsVar*) ejsCreateBoolean(ejs, strncmp(&sp->value[0], pattern, len) == 0);
}


#if ES_String_substr
/*
 *  Extract a substring using a length
 *
 *  function substr(start: Number, length: Number = -1): String
 */
static EjsVar *substr(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    int     start, length;

    length = -1;

    start = ejsGetInt(argv[0]);
    if (argc == 2) {
        length = ejsGetInt(argv[1]);
    }

    if (start < 0) {
        start += sp->length - start;
    }
    if (start < 0 || start >= sp->length) {
        ejsThrowOutOfBoundsError(ejs, "Bad start subscript");
        return 0;
    }
    if (length < 0 || length > sp->length) {
        length = sp->length - start;
    }


    return (EjsVar*) ejsCreateBinaryString(ejs, &sp->value[start], length);
}
#endif


/*
 *  Extract a substring. Simple routine with positive indicies.
 *
 *  function substring(start: Number, end: Number = -1): String
 */
static EjsVar *substring(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    int     start, end, tmp;

    start = ejsGetInt(argv[0]);
    if (argc == 2) {
        end = ejsGetInt(argv[1]);
    } else {
        end = sp->length;
    }

    if (start < 0) {
        start = 0;
    }
    if (start >= sp->length) {
        start = sp->length - 1;
    }
    if (end < 0) {
        end = sp->length;
    }
    if (end > sp->length) {
        end = sp->length;
    }

    /*
     *  Swap if start is bigger than end
     */
    if (start > end) {
        tmp = start;
        start = end;
        end = tmp;
    }

    return (EjsVar*) ejsCreateStringWithLength(ejs, &sp->value[start], end - start);
}


/*
 *  Convert the string to camelCase. Return a new string.
 *
 *  function toCamel(): String
 */
static EjsVar *toCamel(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    EjsString   *result;

    result = ejsCreateStringWithLength(ejs, sp->value, sp->length);
    if (result == 0) {
        return 0;
    }
    result->value[0] = tolower((int) sp->value[0]);

    return (EjsVar*) result;
}


#if ES_String_toJSONString
static EjsVar *toJSONString(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    return 0;
}
#endif


/*
 *  Convert the string to PascalCase. Return a new string.
 *
 *  function toPascal(): String
 */
static EjsVar *toPascal(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    EjsString   *result;

    result = ejsCreateStringWithLength(ejs, sp->value, sp->length);
    if (result == 0) {
        return 0;
    }
    result->value[0] = toupper((int) sp->value[0]);

    return (EjsVar*) result;
}


/*
 *  Convert the string to lower case.
 *  @return Returns a new lower case version of the string.
 *  @spec ejs-11
 *
 *  function toLower(locale: String = null): String
 *  TODO - locale not supported yet.
 */
static EjsVar *toLower(Ejs *ejs, EjsString *sp, int argc,  EjsVar **argv)
{
    EjsString       *result;
    int             i;

    result = ejsCreateStringWithLength(ejs, sp->value, sp->length);
    if (result == 0) {
        return 0;
    }
    for (i = 0; i < result->length; i++) {
        result->value[i] = tolower((int) result->value[i]);
    }
    return (EjsVar*) result;
}


/*
 *  Convert to a string
 *
 *  override function toString(): String
 */
static EjsVar *stringToString(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    return (EjsVar*) sp;
}


/*
 *  Convert the string to upper case.
 *  @return Returns a new upper case version of the string.
 *  @spec ejs-11
 *
 *  function toUpper(locale: String = null): String
 *  TODO - locale not supported yet.
 */
static EjsVar *toUpper(Ejs *ejs, EjsString *sp, int argc,  EjsVar **argv)
{
    EjsString       *result;
    int             i;

    result = ejsCreateStringWithLength(ejs, sp->value, sp->length);
    if (result == 0) {
        return 0;
    }
    for (i = 0; i < result->length; i++) {
        result->value[i] = toupper((int) result->value[i]);
    }
    return (EjsVar*) result;
}


/*
 *  Scan the input and tokenize according to the format string
 *
 *  function tokenize(format: String): Array
 */
static EjsVar *tokenize(Ejs *ejs, EjsString *sp, int argc, EjsVar **argv)
{
    EjsArray    *result;
    cchar       *fmt;
    char        *end, *buf;

    mprAssert(argc == 1 && ejsIsString(argv[0]));

    buf = sp->value;
    fmt = ejsGetString(argv[0]);
    result = ejsCreateArray(ejs, 0);

    for (fmt = ejsGetString(argv[0]); *fmt && buf < &sp->value[sp->length]; ) {
        if (*fmt++ != '%') {
            continue;
        }
        //  TODO - what other types should we support
        switch (*fmt) {
        case 's':
            for (end = buf; *end; end++) {
                if (isspace((int) *end)) {
                    break;
                }
            }
            ejsSetProperty(ejs, (EjsVar*) result, -1, (EjsVar*) ejsCreateStringWithLength(ejs, buf, (int) (end - buf)));
            buf = end;
            break;

        case 'd':
            ejsSetProperty(ejs, (EjsVar*) result, -1, ejsParseVar(ejs, buf, ES_Number));
            while (*buf && !isspace((int) *buf)) {
                buf++;
            }
            break;

        default:
            ejsThrowArgError(ejs, "Bad format specifier");
            return 0;
        }
        while (*buf && isspace((int) *buf)) {
            buf++;
        }
    }
    return (EjsVar*) result;
}


/**
 *  Returns a trimmed copy of the string. Normally used to trim white space, but can be used to trim any substring
 *  from the start or end of the string.
 *  @param str May be set to a substring to trim from the string. If not set, it defaults to any white space.
 *  @return Returns a (possibly) modified copy of the string
 *  @spec ecma-4
 *
 *  function trim(str: String = null): String
 */
static EjsVar *trimString(Ejs *ejs, EjsString *sp, int argc,  EjsVar **argv)
{
    cchar           *start, *end, *pattern, *mark;
    int             index, patternLength;

    mprAssert(argc == 0 || (argc == 1 && ejsIsString(argv[0])));

    if (argc == 0) {
        for (start = sp->value; start < &sp->value[sp->length]; start++) {
            if (!isspace((int) *start)) {
                break;
            }
        }
        for (end = &sp->value[sp->length - 1]; end >= start; end--) {
            if (!isspace((int) *end)) {
                break;
            }
        }
        end++;

    } else {
        pattern = ejsGetString(argv[0]);
        patternLength = ((EjsString*) argv[0])->length;
        if (patternLength <= 0 || patternLength > sp->length) {
            return (EjsVar*) sp;
        }

        /*
         *  Trim the front
         */
        for (mark = sp->value; &mark[patternLength] < &sp->value[sp->length]; mark += patternLength) {
            index = indexof(mark, patternLength, pattern, patternLength, 1);
            if (index != 0) {
                break;
            }
        }
        start = mark;

        /*
         *  Trim the back
         */
        for (mark = &sp->value[sp->length - patternLength]; mark >= sp->value; mark -= patternLength) {
            index = indexof(mark, patternLength, pattern, patternLength, 1);
            if (index != 0) {
                break;
            }
        }
        end = mark + patternLength;
    }

    return (EjsVar*) ejsCreateStringWithLength(ejs, start, (int) (end - start));
}


/*********************************** Support **********************************/
/**
 *  Fast append a string. This modifies the original "dest" string. BEWARE: strings are meant to be immutable.
 *  Only use this when constructing strings.
 */
static int catString(Ejs *ejs, EjsString *dest, char *str, int len)
{
    EjsString   *castSrc;
    char        *oldBuf, *buf;
    int         oldLen, newLen;

    mprAssert(dest);

    castSrc = 0;

    oldBuf = dest->value;
    oldLen = dest->length;
    newLen = oldLen + len + 1;

#if FUTURE
    if (newLen < MPR_SLAB_STR_MAX) {
        buf = oldBuf;
    } else {
#endif
        buf = (char*) mprRealloc(ejs, oldBuf, newLen);
        if (buf == 0) {
            return -1;
        }
        dest->value = buf;
#if FUTURE
    }
#endif
    memcpy(&buf[oldLen], str, len);
    dest->length += len;
    buf[dest->length] = '\0';

    return 0;
}


/**
 *  Fast append a string. This modifies the original "dest" string. BEWARE: strings are meant to be immutable.
 *  Only use this when constructing strings.
 */
int ejsStrcat(Ejs *ejs, EjsString *dest, EjsVar *src)
{
    EjsString   *castSrc;
    char        *str;
    int         len;

    mprAssert(dest);

    castSrc = 0;

    if (ejsIsString(dest)) {
        if (! ejsIsString(src)) {
            castSrc = (EjsString*) ejsToString(ejs, src);
            if (castSrc == 0) {
                return -1;
            }
            len = (int) strlen(castSrc->value);
            str = castSrc->value;

        } else {
            str = ((EjsString*) src)->value;
            len = ((EjsString*) src)->length;
        }

        if (catString(ejs, dest, str, len) < 0) {
            return -1;
        }

    } else {
        /*
         *  Convert the source to a string and then steal the rusult buffer and assign to the destination
         *  TODO - should be freeing the destination string.
         */
        castSrc = (EjsString*) ejsToString(ejs, src);
        dest->value = castSrc->value;
        mprStealBlock(dest, dest->value);
        castSrc->value = 0;
    }

    return 0;
}


/*
 *  Copy a string. Always null terminate.
 */
int ejsStrdup(MprCtx ctx, uchar **dest, cvoid *src, int nbytes)
{
    mprAssert(dest);
    mprAssert(src);

    if (nbytes > 0) {
        *dest = (uchar*) mprAlloc(ctx, nbytes + 1);
        if (*dest == 0) {
            return MPR_ERR_NO_MEMORY;
        }
        strncpy((char*) *dest, (char*) src, nbytes);

    } else {
        *dest = (uchar*) mprAlloc(ctx, 1);
        nbytes = 0;
    }

    (*dest)[nbytes] = '\0';

    return nbytes;
}


/*
 *  Find a substring. Search forward or backwards. Return the index in the string where the pattern was found.
 *  Return -1 if not found.
 */
static int indexof(cchar *str, int len, cchar *pattern, int patternLength, int dir)
{
    cchar   *s1, *s2;
    int     i, j;

    mprAssert(dir == 1 || dir == -1);

    if (dir > 0) {
        for (i = 0; i < len; i++) {
            s1 = &str[i];
            for (j = 0, s2 = pattern; j < patternLength; s1++, s2++, j++) {
                if (*s1 != *s2) {
                    break;
                }
            }
            if (*s2 == '\0') {
                return i;
            }
        }

    } else {
        for (i = len - 1; i >= 0; i--) {
            s1 = &str[i];
            for (j = 0, s2 = pattern; j < patternLength; s1++, s2++, j++) {
                if (*s1 != *s2) {
                    break;
                }
            }
            if (*s2 == '\0') {
                return i;
            }
        }
    }
    return -1;
}


/*********************************** Factory **********************************/

EjsString *ejsCreateString(Ejs *ejs, cchar *value)
{
    EjsString   *sp;

    /*
     *  No need to invoke constructor
     */
    sp = (EjsString*) ejsCreateVar(ejs, ejs->stringType, 0);
    if (sp != 0) {
        sp->value = mprStrdup(ejs, value);
        if (sp->value == 0) {
            return 0;
        }
        sp->length = (int) strlen(sp->value);

        ejsSetDebugName(sp, sp->value);
    }
    return sp;
}


EjsString *ejsDupString(Ejs *ejs, EjsString *sp)
{
    return ejsCreateStringWithLength(ejs, sp->value, sp->length);
}


/*
 *  Initialize a binary string value.
 */
EjsString *ejsCreateStringWithLength(Ejs *ejs, cchar *value, int len)
{
    EjsString   *sp;
    uchar       *dest;

    //  TODO - OPT Would be much faster to allocate the string value in the actual object since strings are 
    //      immutable
    sp = (EjsString*) ejsCreateVar(ejs, ejs->stringType, 0);
    if (sp != 0) {
        sp->length = ejsStrdup(ejs, &dest, value, len);
        sp->value = (char*) dest;
        if (sp->length < 0) {
            return 0;
        }
    }
    return sp;
}


/*
 *  Initialize an string with a pre-allocated buffer but without data..
 */
EjsString *ejsCreateBareString(Ejs *ejs, int len)
{
    EjsString   *sp;

    //  TODO - OPT Would be much faster to allocate the string value in the actual object since strings are immutable
    sp = (EjsString*) ejsCreateVar(ejs, ejs->stringType, 0);
    if (sp != 0) {
        sp->value = mprAlloc(sp, len + 1);
        if (sp->value == 0) {
            return 0;
        }
        sp->length = len;
        sp->value[len] = '\0';
    }
    return sp;
}


void ejsCreateStringType(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;

    type = ejsCreateCoreType(ejs, ejsName(&qname, EJS_INTRINSIC_NAMESPACE, "String"), ejs->objectType, sizeof(EjsString),
        ES_String, ES_String_NUM_CLASS_PROP,  ES_String_NUM_INSTANCE_PROP,
        EJS_ATTR_NATIVE | EJS_ATTR_HAS_CONSTRUCTOR | EJS_ATTR_OPER_OVERLOAD);
    ejs->stringType = type;

    /*
     *  Define the helper functions.
     */
    type->helpers->castVar = (EjsCastVarHelper) castString;
    type->helpers->cloneVar = (EjsCloneVarHelper) cloneString;
    type->helpers->destroyVar = (EjsDestroyVarHelper) destroyString;
    type->helpers->getProperty = (EjsGetPropertyHelper) getStringProperty;
    type->helpers->invokeOperator = (EjsInvokeOperatorHelper) invokeStringOperator;
    type->helpers->lookupProperty = (EjsLookupPropertyHelper) lookupStringProperty;

    type->numericIndicies = 1;

    /*
     *  Pre-create the empty string.
     */
    ejs->emptyStringValue = (EjsString*) ejsCreateString(ejs, "");
    ejsSetDebugName(ejs->emptyStringValue, "emptyString");
}


void ejsConfigureStringType(Ejs *ejs)
{
    EjsType     *type;

    type = ejs->stringType;

    /*
     *  Define the "string" alias
     */
    ejsSetProperty(ejs, ejs->global, ES_string, (EjsVar*) type);
    ejsBindMethod(ejs, type, ES_String_String, (EjsNativeFunction) stringConstructor);
    ejsBindMethod(ejs, type, ES_String_caseCompare, (EjsNativeFunction) caseCompare);
    ejsBindMethod(ejs, type, ES_String_charAt, (EjsNativeFunction) charAt);
    ejsBindMethod(ejs, type, ES_String_charCodeAt, (EjsNativeFunction) charCodeAt);
    ejsBindMethod(ejs, type, ES_String_concat, (EjsNativeFunction) concatString);
    ejsBindMethod(ejs, type, ES_String_contains, (EjsNativeFunction) containsString);
    ejsBindMethod(ejs, type, ES_String_endsWith, (EjsNativeFunction) endsWith);
    ejsBindMethod(ejs, type, ES_String_format, (EjsNativeFunction) formatString);
    ejsBindMethod(ejs, type, ES_String_fromCharCode, (EjsNativeFunction) fromCharCode);
    ejsBindMethod(ejs, type, ES_Object_get, (EjsNativeFunction) getStringIterator);
    ejsBindMethod(ejs, type, ES_Object_getValues, (EjsNativeFunction) getStringValues);
    ejsBindMethod(ejs, type, ES_String_indexOf, (EjsNativeFunction) indexOf);
#if UNUSED
    ejsSetAccessors(ejs, type, ES_String_isDigit, (EjsNativeFunction) isDigit, -1, 0);
    ejsSetAccessors(ejs, type, ES_String_isAlpha, (EjsNativeFunction) isAlpha, -1, 0);
    ejsSetAccessors(ejs, type, ES_String_isLower, (EjsNativeFunction) isLower, -1, 0);
    ejsSetAccessors(ejs, type, ES_String_isSpace, (EjsNativeFunction) isSpace, -1, 0);
    ejsSetAccessors(ejs, type, ES_String_isUpper, (EjsNativeFunction) isUpper, -1, 0);
    ejsSetAccessors(ejs, type, ES_Object_length, (EjsNativeFunction) stringLength, -1, 0);
#else
    ejsBindMethod(ejs, type, ES_String_isDigit, (EjsNativeFunction) isDigit);
    ejsBindMethod(ejs, type, ES_String_isAlpha, (EjsNativeFunction) isAlpha);
    ejsBindMethod(ejs, type, ES_String_isLower, (EjsNativeFunction) isLower);
    ejsBindMethod(ejs, type, ES_String_isSpace, (EjsNativeFunction) isSpace);
    ejsBindMethod(ejs, type, ES_String_isUpper, (EjsNativeFunction) isUpper);
    ejsBindMethod(ejs, type, ES_Object_length, (EjsNativeFunction) stringLength);
#endif
    ejsBindMethod(ejs, type, ES_String_lastIndexOf, (EjsNativeFunction) lastIndexOf);
#if BLD_FEATURE_REGEXP
    ejsBindMethod(ejs, type, ES_String_match, (EjsNativeFunction) match);
#endif
    ejsBindMethod(ejs, type, ES_String_remove, (EjsNativeFunction) removeCharsFromString);
    ejsBindMethod(ejs, type, ES_String_slice, (EjsNativeFunction) sliceString);
    ejsBindMethod(ejs, type, ES_String_split, (EjsNativeFunction) split);
#if ES_String_localeCompare && FUTURE
    ejsBindMethod(ejs, type, ES_String_localeCompare, (EjsNativeFunction) localeCompare);
#endif
#if ES_String_parseJSON
    ejsBindMethod(ejs, type, ES_String_parseJSON, (EjsNativeFunction) parseJSON);
#endif
    ejsBindMethod(ejs, type, ES_String_printable, (EjsNativeFunction) printable);
    ejsBindMethod(ejs, type, ES_String_quote, (EjsNativeFunction) quote);
    ejsBindMethod(ejs, type, ES_String_replace, (EjsNativeFunction) replace);
    ejsBindMethod(ejs, type, ES_String_reverse, (EjsNativeFunction) reverseString);
    ejsBindMethod(ejs, type, ES_String_search, (EjsNativeFunction) searchString);
    ejsBindMethod(ejs, type, ES_String_startsWith, (EjsNativeFunction) startsWith);
    ejsBindMethod(ejs, type, ES_String_substring, (EjsNativeFunction) substring);
#if ES_String_substr
    ejsBindMethod(ejs, type, ES_String_substr, (EjsNativeFunction) substr);
#endif
    ejsBindMethod(ejs, type, ES_String_toCamel, (EjsNativeFunction) toCamel);
    ejsBindMethod(ejs, type, ES_String_toLower, (EjsNativeFunction) toLower);
    ejsBindMethod(ejs, type, ES_String_toPascal, (EjsNativeFunction) toPascal);
    ejsBindMethod(ejs, type, ES_Object_toString, (EjsNativeFunction) stringToString);
    ejsBindMethod(ejs, type, ES_String_toUpper, (EjsNativeFunction) toUpper);
    ejsBindMethod(ejs, type, ES_String_tokenize, (EjsNativeFunction) tokenize);
    ejsBindMethod(ejs, type, ES_String_trim, (EjsNativeFunction) trimString);

#if FUTURE
    ejsBindMethod(ejs, type, ES_String_LBRACKET, operLBRACKET);
    ejsBindMethod(ejs, type, ES_String_PLUS, operPLUS);
    ejsBindMethod(ejs, type, ES_String_MINUS, operMINUS);
    ejsBindMethod(ejs, type, ES_String_LT, operLT);
    ejsBindMethod(ejs, type, ES_String_GT, operGT);
    ejsBindMethod(ejs, type, ES_String_EQ, operEQ);
    ejsBindMethod(ejs, type, ES_String_MOD, operMOD);
    ejsBindMethod(ejs, type, ES_String_MUL, operMUL);
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
