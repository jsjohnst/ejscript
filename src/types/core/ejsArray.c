/**
 *  ejsArray.c - Ejscript Array class
 *
 *  This module implents the standard Array type. It provides the type methods and manages the special "length" property.
 *  The array elements with numeric indicies are stored in EjsArray.data[]. Non-numeric properties are stored in EjsArray.obj.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejs.h"

/***************************** Forward Declarations ***************************/

static int  checkSlot(Ejs *ejs, EjsArray *ap, int slotNum);
static bool compare(Ejs *ejs, EjsVar *v1, EjsVar *v2);
static int growArray(Ejs *ejs, EjsArray *ap, int len);
static int lookupArrayProperty(Ejs *ejs, EjsArray *ap, EjsName *qname);
static EjsVar *pushArray(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv);
static EjsVar *spliceArray(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv);
static EjsVar *arrayToString(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv);

#if BLD_FEATURE_EJS_LANG >= EJS_SPEC_PLUS
static EjsVar *makeIntersection(Ejs *ejs, EjsArray *lhs, EjsArray *rhs);
static EjsVar *makeUnion(Ejs *ejs, EjsArray *lhs, EjsArray *rhs);
static EjsVar *removeArrayElements(Ejs *ejs, EjsArray *lhs, EjsArray *rhs);
#endif

/******************************************************************************/
/*
 *  Cast the object operand to a primitive type
 */

static EjsVar *castArray(Ejs *ejs, EjsArray *vp, EjsType *type)
{
    switch (type->id) {
    case ES_Boolean:
        return (EjsVar*) ejs->trueValue;

    case ES_Number:
        return (EjsVar*) ejs->zeroValue;

    case ES_String:
        return arrayToString(ejs, vp, 0, 0);

    default:
        ejsThrowTypeError(ejs, "Can't cast to this type");
        return 0;
    }
}


static EjsArray *cloneArray(Ejs *ejs, EjsArray *ap, bool deep)
{
    EjsArray    *newArray;
    EjsVar      **dest, **src;
    int         i;

    newArray = (EjsArray*) ejsCopyObject(ejs, (EjsObject*) ap, deep);
    if (newArray == 0) {
        ejsThrowMemoryError(ejs);
        return 0;
    }

    if (ap->length > 0) {
        if (growArray(ejs, newArray, ap->length) < 0) {
            ejsThrowMemoryError(ejs);
            return 0;
        }
        
        src = ap->data;
        dest = newArray->data;

        if (deep) {
            for (i = 0; i < ap->length; i++) {
                dest[i] = ejsCloneVar(ejs, src[i], 1);
                ejsSetReference(ejs, (EjsVar*) ap, dest[i]);
            }

        } else {
            memcpy(dest, src, ap->length * sizeof(EjsVar*));
        }
    }
    return newArray;
}


/*
 *  Delete a property and update the length
 */
static int deleteArrayProperty(Ejs *ejs, EjsArray *ap, int slot)
{
    if (slot >= ap->length) {
        //  TODO DIAG
        mprAssert(0);
        return EJS_ERR;
    }
    if ((slot + 1) == ap->length) {
        ap->length--;
    }

    if (ejsSetProperty(ejs, (EjsVar*) ap, slot, (EjsVar*) ejs->undefinedValue) < 0) {
        //  TODO - DIAG
        return EJS_ERR;
    }

    return 0;
}


/*
 *  Delete an element by name.
 */
static int deleteArrayPropertyByName(Ejs *ejs, EjsArray *ap, EjsName *qname)
{
    if (isdigit((int) qname->name[0])) {
        return deleteArrayProperty(ejs, ap, atoi(qname->name));
    }

    return (ejs->objectHelpers->deletePropertyByName)(ejs, (EjsVar*) ap, qname);
}


/*
 *  Return the number of elements in the array
 */
static int getArrayPropertyCount(Ejs *ejs, EjsArray *ap)
{
    return ap->length;
}


/*
 *  Get an array element. Slot numbers correspond to indicies.
 */
static EjsVar *getArrayProperty(Ejs *ejs, EjsArray *ap, int slotNum)
{
    if (slotNum < 0 || slotNum >= ap->length) {
        return ejs->undefinedValue;
#if UNUSED
        ejsThrowOutOfBoundsError(ejs, "Bad array subscript");
        return 0;
#endif
    }
    return ap->data[slotNum];
}


static EjsVar *getArrayPropertyByName(Ejs *ejs, EjsArray *ap, EjsName *qname)
{
    int     slotNum;

    if (isdigit((int) qname->name[0])) { 
        slotNum = atoi(qname->name);
        if (slotNum < 0 || slotNum >= ap->length) {
            return 0;
        }
        return getArrayProperty(ejs, ap, slotNum);
    }

    /* The "length" property is a method getter */
    if (strcmp(qname->name, "length") == 0) {
        return 0;
    }
    slotNum = (ejs->objectHelpers->lookupProperty)(ejs, (EjsVar*) ap, qname);
    if (slotNum < 0) {
        return 0;
    }
    return (ejs->objectHelpers->getProperty)(ejs, (EjsVar*) ap, slotNum);
}


/*
 *  Lookup an array index.
 */
static int lookupArrayProperty(Ejs *ejs, EjsArray *ap, EjsName *qname)
{
    int     index;

    if (qname == 0 || !isdigit((int) qname->name[0])) {
        return EJS_ERR;
    }
    index = atoi(qname->name);
    if (index < ap->length) {
        return index;
    }

    return EJS_ERR;
}


/*
 *  Cast operands as required for invokeArrayOperator
 */
static EjsVar *coerceArrayOperands(Ejs *ejs, EjsVar *lhs, int opcode,  EjsVar *rhs)
{
    switch (opcode) {
    /*
     *  Binary operators
     */
    case EJS_OP_ADD:
        return ejsInvokeOperator(ejs, arrayToString(ejs, (EjsArray*) lhs, 0, 0), opcode, rhs);

    case EJS_OP_AND: case EJS_OP_DIV: case EJS_OP_MUL: case EJS_OP_OR: case EJS_OP_REM:
    case EJS_OP_SHL: case EJS_OP_SHR: case EJS_OP_SUB: case EJS_OP_USHR: case EJS_OP_XOR:
        return ejsInvokeOperator(ejs, (EjsVar*) ejs->zeroValue, opcode, rhs);

    case EJS_OP_COMPARE_EQ: case EJS_OP_COMPARE_NE:
        if (ejsIsNull(rhs) || ejsIsUndefined(rhs)) {
            return (EjsVar*) ((opcode == EJS_OP_COMPARE_EQ) ? ejs->falseValue: ejs->trueValue);
        } else if (ejsIsNumber(rhs)) {
            return ejsInvokeOperator(ejs, (EjsVar*) ejsToNumber(ejs, lhs), opcode, rhs);
        }
        return ejsInvokeOperator(ejs, (EjsVar*) ejsToString(ejs, lhs), opcode, rhs);

    case EJS_OP_COMPARE_LE: case EJS_OP_COMPARE_LT:
    case EJS_OP_COMPARE_GE: case EJS_OP_COMPARE_GT:
        if (ejsIsNumber(rhs)) {
            return ejsInvokeOperator(ejs, (EjsVar*) ejsToNumber(ejs, lhs), opcode, rhs);
        }
        return ejsInvokeOperator(ejs, (EjsVar*) ejsToString(ejs, lhs), opcode, rhs);

    case EJS_OP_COMPARE_STRICTLY_NE:
    case EJS_OP_COMPARE_UNDEFINED:
    case EJS_OP_COMPARE_NOT_ZERO:
    case EJS_OP_COMPARE_NULL:
        return (EjsVar*) ejs->trueValue;

    case EJS_OP_COMPARE_STRICTLY_EQ:
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


static EjsVar *invokeArrayOperator(Ejs *ejs, EjsVar *lhs, int opcode,  EjsVar *rhs)
{
    EjsVar      *result;

    if (rhs == 0 || lhs->type != rhs->type) {
        if ((result = coerceArrayOperands(ejs, lhs, opcode, rhs)) != 0) {
            return result;
        }
    }

    switch (opcode) {

    case EJS_OP_COMPARE_EQ: case EJS_OP_COMPARE_STRICTLY_EQ:
    case EJS_OP_COMPARE_LE: case EJS_OP_COMPARE_GE:
        return (EjsVar*) ejsCreateBoolean(ejs, (lhs == rhs));

    case EJS_OP_COMPARE_NE: case EJS_OP_COMPARE_STRICTLY_NE:
    case EJS_OP_COMPARE_LT: case EJS_OP_COMPARE_GT:
        return (EjsVar*) ejsCreateBoolean(ejs, !(lhs == rhs));

    /*
     *  Unary operators
     */
    case EJS_OP_COMPARE_NOT_ZERO:
        return (EjsVar*) ejs->trueValue;

    case EJS_OP_COMPARE_UNDEFINED:
    case EJS_OP_COMPARE_NULL:
    case EJS_OP_COMPARE_FALSE:
    case EJS_OP_COMPARE_TRUE:
    case EJS_OP_COMPARE_ZERO:
        return (EjsVar*) ejs->falseValue;

    case EJS_OP_LOGICAL_NOT: case EJS_OP_NOT: case EJS_OP_NEG:
        return (EjsVar*) ejs->oneValue;

    /*
     *  Binary operators
     */
    case EJS_OP_DIV: case EJS_OP_MUL: case EJS_OP_REM:
    case EJS_OP_SHR: case EJS_OP_USHR: case EJS_OP_XOR:
        return (EjsVar*) ejs->zeroValue;

#if BLD_FEATURE_EJS_LANG >= EJS_SPEC_PLUS
    /*
     *  Operator overload
     */
    case EJS_OP_ADD:
        result = (EjsVar*) ejsCreateArray(ejs, 0);
        pushArray(ejs, (EjsArray*) result, 1, (EjsVar**) &lhs);
        pushArray(ejs, (EjsArray*) result, 1, (EjsVar**) &rhs);
        return result;

    case EJS_OP_AND:
        return (EjsVar*) makeIntersection(ejs, (EjsArray*) lhs, (EjsArray*) rhs);

    case EJS_OP_OR:
        return (EjsVar*) makeUnion(ejs, (EjsArray*) lhs, (EjsArray*) rhs);

    case EJS_OP_SHL:
        return pushArray(ejs, (EjsArray*) lhs, 1, &rhs);

    case EJS_OP_SUB:
        return (EjsVar*) removeArrayElements(ejs, (EjsArray*) lhs, (EjsArray*) rhs);
#endif

    default:
        ejsThrowTypeError(ejs, "Opcode %d not implemented for type %s", opcode, lhs->type->qname.name);
        return 0;
    }

    mprAssert(0);
}


static void markArrayVar(Ejs *ejs, EjsVar *parent, EjsArray *ap)
{
    EjsVar          *vp;
    int             i;

    mprAssert(ejsIsArray(ap));

    ejsMarkObject(ejs, parent, (EjsObject*) ap);
    
    for (i = ap->length - 1; i >= 0; i--) {
        if ((vp = ap->data[i]) == 0) {
            continue;
        }
        ejsMarkVar(ejs, (EjsVar*) ap, vp);
    }
}


/*
 *  Create or update an array elements. If slotNum is < 0, then create the next free array slot. If slotNum is greater
 *  than the array length, grow the array.
 */
static int setArrayProperty(Ejs *ejs, EjsArray *ap, int slotNum,  EjsVar *value)
{
    if ((slotNum = checkSlot(ejs, ap, slotNum)) < 0) {
        return EJS_ERR;
    }
    ap->data[slotNum] = value;
    ejsSetReference(ejs, (EjsVar*) ap, value);
    return slotNum;
}


static int setArrayPropertyByName(Ejs *ejs, EjsArray *ap, EjsName *qname, EjsVar *value)
{
    int     slotNum;

    if (!isdigit((int) qname->name[0])) { 
        /* The "length" property is a method getter */
        if (strcmp(qname->name, "length") == 0) {
            return EJS_ERR;
        }
        slotNum = (ejs->objectHelpers->lookupProperty)(ejs, (EjsVar*) ap, qname);
        if (slotNum < 0) {
            slotNum = (ejs->objectHelpers->setProperty)(ejs, (EjsVar*) ap, slotNum, value);
            if (slotNum < 0) {
                return EJS_ERR;
            }
            if ((ejs->objectHelpers->setPropertyName)(ejs, (EjsVar*) ap, slotNum, qname) < 0) {
                return EJS_ERR;
            }
            return slotNum;

        } else {
            return (ejs->objectHelpers->setProperty)(ejs, (EjsVar*) ap, slotNum, value);
        }
    }

    if ((slotNum = checkSlot(ejs, ap, atoi(qname->name))) < 0) {
        return EJS_ERR;
    }
    ap->data[slotNum] = value;
    ejsSetReference(ejs, (EjsVar*) ap, value);

    return slotNum;
}


#if BLD_FEATURE_EJS_LANG >= EJS_SPEC_PLUS
static EjsVar *makeIntersection(Ejs *ejs, EjsArray *lhs, EjsArray *rhs)
{
    EjsArray    *result;
    EjsVar      **l, **r, **resultSlots;
    int         i, j, k;

    result = ejsCreateArray(ejs, 0);
    l = lhs->data;
    r = rhs->data;

    for (i = 0; i < lhs->length; i++) {
        for (j = 0; j < rhs->length; j++) {
            if (compare(ejs, l[i], r[j])) {
                resultSlots = result->data;
                for (k = 0; k < result->length; k++) {
                    if (compare(ejs, l[i], resultSlots[k])) {
                        break;
                    }
                }
                if (result->length == 0 || k == result->length) {
                    setArrayProperty(ejs, result, -1, l[i]);
                }
            }
        }
    }
    return (EjsVar*) result;
}


static int addUnique(Ejs *ejs, EjsArray *ap, EjsVar *element)
{
    int     i;

    for (i = 0; i < ap->length; i++) {
        if (compare(ejs, ap->data[i], element)) {
            break;
        }
    }
    if (i == ap->length) {
        if (setArrayProperty(ejs, ap, -1, element) < 0) {
            return EJS_ERR;
        }
    }
    return 0;
}


static EjsVar *makeUnion(Ejs *ejs, EjsArray *lhs, EjsArray *rhs)
{
    EjsArray    *result;
    EjsVar      **l, **r;
    int         i;

    result = ejsCreateArray(ejs, 0);

    l = lhs->data;
    r = rhs->data;

    for (i = 0; i < lhs->length; i++) {
        addUnique(ejs, result, l[i]);
    }
    for (i = 0; i < rhs->length; i++) {
        addUnique(ejs, result, r[i]);
    }

    return (EjsVar*) result;
}


static EjsVar *removeArrayElements(Ejs *ejs, EjsArray *lhs, EjsArray *rhs)
{
    EjsVar  **l, **r;
    int     i, j, k;

    l = lhs->data;
    r = rhs->data;

    for (j = 0; j < rhs->length; j++) {
        for (i = 0; i < lhs->length; i++) {
            if (compare(ejs, l[i], r[j])) {
                for (k = i + 1; k < lhs->length; k++) {
                    l[k - 1] = l[k];
                }
                lhs->length--;
            }
        }
    }

    return (EjsVar*) lhs;
}
#endif


static int checkSlot(Ejs *ejs, EjsArray *ap, int slotNum)
{
    if (slotNum < 0) {
        if (!ap->obj.var.dynamic) {
            ejsThrowTypeError(ejs, "Object is not dynamic");
            return EJS_ERR;
        }
        
        slotNum = ap->length;
        if (growArray(ejs, ap, ap->length + 1) < 0) {
            ejsThrowMemoryError(ejs);
            return EJS_ERR;
        }

    } else if (slotNum >= ap->length) {
        if (growArray(ejs, ap, slotNum + 1) < 0) {
            ejsThrowMemoryError(ejs);
            return EJS_ERR;
        }
    }
    return slotNum;
}


/********************************** Methods *********************************/
/*
 *  Array constructor.
 *
 *  function Array(...args): Array
 *
 *  Support the forms:
 *
 *      var arr = Array();
 *      var arr = Array(size);
 *      var arr = Array(elt, elt, elt, ...);
 */

static EjsVar *arrayConstructor(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    EjsArray    *args;
    EjsVar      *arg0, **src, **dest;
    int         size, i;

    mprAssert(argc == 1 && ejsIsArray(argv[0]));

    args = (EjsArray*) argv[0];
    
    if (args->length == 0) {
        return 0;
    }

    size = 0;
    arg0 = getArrayProperty(ejs, args, 0);

    if (args->length == 1 && ejsIsNumber(arg0)) {
        /*
         *  x = new Array(size);
         */
        size = ejsGetInt(arg0);
        if (size > 0 && growArray(ejs, ap, size) < 0) {
            ejsThrowMemoryError(ejs);
            return 0;
        }

    } else {

        /*
         *  x = new Array(element0, element1, ..., elementN):
         */
        size = args->length;
        if (size > 0 && growArray(ejs, ap, size) < 0) {
            ejsThrowMemoryError(ejs);
            return 0;
        }

        src = args->data;
        dest = ap->data;
        for (i = 0; i < size; i++) {
            dest[i] = src[i];
        }
    }
    ap->length = size;

    return (EjsVar*) ap;
}


/*
 *  Append an item to an array
 *
 *  function append(obj: Object) : Array
 */
static EjsVar *appendArray(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    if (setArrayProperty(ejs, ap, ap->length, argv[0]) < 0) {
        return 0;
    }
    return (EjsVar*) ap;
}


/*
 *  Clear an array. Remove all elements of the array.
 *
 *  function clear() : void
 */
static EjsVar *clearArray(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    ap->length = 0;
    return 0;
}


/*
 *  Clone an array.
 *
 *  function clone(deep: Boolean = false) : Array
 */
static EjsArray *cloneArrayMethod(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    bool    deep;

    mprAssert(argc == 0 || ejsIsBoolean(argv[0]));

    deep = (argc == 1) ? ((EjsBoolean*) argv[0])->value : 0;

    return cloneArray(ejs, ap, deep);
}


/*
 *  Compact an array. Remove all null elements.
 *
 *  function compact() : Array
 */
static EjsArray *compactArray(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    EjsVar      **data, **src, **dest;
    int         i;

    data = ap->data;
    src = dest = &data[0];
    for (i = 0; i < ap->length; i++, src++) {
        if (*src == 0 || *src == ejs->undefinedValue || *src == ejs->nullValue) {
            continue;
        }
        *dest++ = *src;
    }

    ap->length = (int) (dest - &data[0]);
    return ap;
}


/*
 *  Concatenate the supplied elements with the array to create a new array. If any arguments specify an array,
 *  their elements are catenated. This is a one level deep copy.
 *
 *  function concat(...args): Array
 */
static EjsVar *concatArray(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    EjsArray    *args, *newArray, *vpa;
    EjsVar      *vp, **src, **dest;
    int         i, k, next;

    mprAssert(argc == 1 && ejsIsArray(argv[0]));

    args = ((EjsArray*) argv[0]);

    /*
     *  Guess the new array size. May exceed this if args has elements that are themselves arrays.
     */
    newArray = ejsCreateArray(ejs, ap->length + ((EjsArray*) argv[0])->length);

    src = ap->data;
    dest = newArray->data;

    /*
     *  Copy the original array
     */
    for (next = 0; next < ap->length; next++) {
        dest[next] = src[next];
    }

    /*
     *  Copy the args. If any element is itself an array, then flatten it and copy its elements.
     */
    for (i = 0; i < args->length; i++) {
        vp = args->data[i];
        if (ejsIsArray(vp)) {

            vpa = (EjsArray*) vp;
            if (growArray(ejs, newArray, newArray->length + vpa->length - 1) < 0) {
                ejsThrowMemoryError(ejs);
                return 0;
            }
            for (k = 0; k < vpa->length; k++) {
                dest[next++] = src[k];
            }

        } else {
            dest[next++] = vp;
        }
    }

    return 0;
}


/*
 *  Function to iterate and return the next element name.
 *  NOTE: this is not a method of Array. Rather, it is a callback function for Iterator
 */
static EjsVar *nextArrayKey(Ejs *ejs, EjsIterator *ip, int argc, EjsVar **argv)
{
    EjsArray        *ap;
    EjsVar          *vp, **data;

    ap = (EjsArray*) ip->target;
    if (!ejsIsArray(ap)) {
        ejsThrowReferenceError(ejs, "Wrong type");
        return 0;
    }
    data = ap->data;

    for (; ip->index < ap->length; ip->index++) {
        vp = data[ip->index];
        if (vp == 0) {
            continue;
        }
        return (EjsVar*) ejsCreateNumber(ejs, ip->index++);
    }
    ejsThrowStopIteration(ejs);
    return 0;
}


/*
 *  Return the default iterator. This returns the array index names.
 *
 *  iterator native function get(): Iterator
 */
static EjsVar *getArrayIterator(Ejs *ejs, EjsVar *ap, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateIterator(ejs, ap, (EjsNativeFunction) nextArrayKey, 0, NULL);
}


/*
 *  Function to iterate and return the next element value.
 *  NOTE: this is not a method of Array. Rather, it is a callback function for Iterator
 */
static EjsVar *nextArrayValue(Ejs *ejs, EjsIterator *ip, int argc, EjsVar **argv)
{
    EjsArray    *ap;
    EjsVar      *vp, **data;

    ap = (EjsArray*) ip->target;
    if (!ejsIsArray(ap)) {
        ejsThrowReferenceError(ejs, "Wrong type");
        return 0;
    }

    data = ap->data;
    for (; ip->index < ap->length; ip->index++) {
        vp = data[ip->index];
        if (vp == 0) {
            continue;
        }
        ip->index++;
        return vp;
    }
    ejsThrowStopIteration(ejs);
    return 0;
}


/*
 *  Return an iterator to return the next array element value.
 *
 *  iterator native function getValues(): Iterator
 */
static EjsVar *getArrayValues(Ejs *ejs, EjsVar *ap, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateIterator(ejs, ap, (EjsNativeFunction) nextArrayValue, 0, NULL);
}


#if UNUSED && KEEP
static EjsVar *find(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    return 0;
}


/**
 *  Iterate over all elements in the object and find all elements for which the matching function is true.
 *  The match is called with the following signature:
 *
 *      function match(arrayElement: Object, elementIndex: Number, arr: Array): Boolean
 *
 *  @param match Matching function
 *  @return Returns a new array containing all matching elements.
 */
static EjsVar *findAll(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    EjsVar      *funArgs[3];
    EjsBoolean  *result;
    EjsArray    *elements;
    int         i;

    mprAssert(argc == 1 && ejsIsFunction(argv[0]));

    elements = ejsCreateArray(ejs, 0);
    if (elements == 0) {
        ejsThrowMemoryError(ejs);
        return 0;
    }

    for (i = 0; i < ap->length; i++) {
        funArgs[0] = ap->obj.properties.slots[i];               /* Array element */
        funArgs[1] = (EjsVar*) ejsCreateNumber(ejs, i);             /* element index */
        funArgs[2] = (EjsVar*) ap;                                  /* Array */
        result = (EjsBoolean*) ejsRunFunction(ejs, (EjsFunction*) argv[0], 0, 3, funArgs);
        if (result == 0 || !ejsIsBoolean(result) || !result->value) {
            setArrayProperty(ejs, elements, elements->length, ap->obj.properties.slots[i]);
        }
    }
    return (EjsVar*) elements;
}
#endif


//  TODO - this routine should be somewhere common
static bool compare(Ejs *ejs, EjsVar *v1, EjsVar *v2)
{
    if (v1 == v2) {
        return 1;
    }
    if (v1->type != v2->type) {
        return 0;
    }
    if (ejsIsNumber(v1)) {
        return ((EjsNumber*) v1)->value == ((EjsNumber*) v2)->value;
    }
    if (ejsIsString(v1)) {
        return strcmp(((EjsString*) v1)->value, ((EjsString*) v2)->value) == 0;
    }
    //  TODO - is this right?
    return 0;
}


/*
 *  Search for an item using strict equality "===". This call searches from
 *  the start of the array for the specified element.
 *  @return Returns the items index into the array if found, otherwise -1.
 *
 *  function indexOf(element: Object, startIndex: Number = 0): Number
 */
static EjsVar *indexOfArray(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    EjsVar          *element;
    int             i, start;

    mprAssert(argc == 1 || argc == 2);

    element = argv[0];
    start = (argc == 2) ? (int) ((EjsNumber*) argv[1])->value : 0;

    for (i = start; i < ap->length; i++) {
        if (compare(ejs, ap->data[i], element)) {
            return (EjsVar*) ejsCreateNumber(ejs, i);
        }
    }
    return (EjsVar*) ejs->minusOneValue;
}


/*
 *  Insert elements. Insert elements at the specified position. Negative indicies are measured from the end of the array.
 *  @return Returns a the original array.
 *
 *  function insert(pos: Number, ...args): Array
 */
static EjsVar *insertArray(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    EjsArray    *args;
    EjsVar      **src, **dest;
    int         i, pos, delta, oldLen, endInsert;

    mprAssert(argc == 2 && ejsIsArray(argv[1]));

    pos = ejsGetInt(argv[0]);
    if (pos < 0) {
        pos += ap->length;
    }
    args = (EjsArray*) argv[1];

    oldLen = ap->length;
    if (growArray(ejs, ap, ap->length + args->length) < 0) {
        return 0;
    }

    delta = args->length;
    dest = ap->data;
    src = args->data;

    endInsert = pos + delta;
    for (i = ap->length - 1; i >= endInsert; i--) {
        dest[i] = dest[i - delta];
    }
    for (i = 0; i < delta; i++) {
        dest[pos++] = src[i];
    }

    return (EjsVar*) ap;
}


/*
 *  Joins the elements in the array into a single string.
 *  @param sep Element separator.
 *  @return Returns a string.
 *
 *  function join(sep: String = undefined): String
 */
static EjsVar *joinArray(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    EjsString       *result, *sep;
    EjsVar          *vp;
    int             i;

    if (argc == 1) {
        sep = (EjsString*) argv[0];
    } else {
        sep = 0;
    }

    result = ejsCreateString(ejs, "");
    for (i = 0; i < ap->length; i++) {
        vp = ap->data[i];
        if (vp == 0 || ejsIsUndefined(vp) || ejsIsNull(vp)) {
            continue;
        }
        if (i > 0 && sep) {
            ejsStrcat(ejs, result, (EjsVar*) sep);
        }
        ejsStrcat(ejs, result, vp);
    }
    return (EjsVar*) result;
}


/*
 *  Search for an item using strict equality "===". This call searches from
 *  the end of the array for the specified element.
 *  @return Returns the items index into the array if found, otherwise -1.
 *
 *  function lastIndexOf(element: Object, fromIndex: Number = 0): Number
 */
static EjsVar *lastArrayIndexOf(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    EjsVar          *element;
    int             i, start;

    mprAssert(argc == 1 || argc == 2);

    element = argv[0];
    start = ((argc == 2) ? (int) ((EjsNumber*) argv[1])->value : ap->length - 1);

    for (i = start; i >= 0; i--) {
        if (compare(ejs, ap->data[i], element)) {
            return (EjsVar*) ejsCreateNumber(ejs, i);
        }
    }
    return (EjsVar*) ejs->minusOneValue;
}


/*
 *  Get the length of an array.
 *  @return Returns the number of items in the array
 *
 *  intrinsic override function get length(): Number
 */

static EjsVar *getArrayLength(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, ap->length);
}


/*
 *  Set the length of an array.
 *
 *  intrinsic override function set length(value: Number): void
 */

static EjsVar *setArrayLength(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    EjsVar      **data, **dest;
    int         length;

    mprAssert(argc == 1 && ejsIsNumber(argv[0]));
    mprAssert(ejsIsArray(ap));

    length = (int) ((EjsNumber*) argv[0])->value;

    if (length > ap->length) {
        if (growArray(ejs, ap, length) < 0) {
            return 0;
        }
        data = ap->data;
        for (dest = &data[ap->length]; dest < &data[length]; dest++) {
            *dest = 0;
        }
    }

    ap->length = length;
    return 0;
}


/*
 *  Remove and return the last value in the array.
 *  @return Returns the last element in the array.
 *
 *  function pop(): Object
 */
static EjsVar *popArray(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    if (ap->length == 0) {
        return (EjsVar*) ejs->undefinedValue;
    }
    return ap->data[--ap->length];
}


/*
 *  Append items to the end of the array.
 *  @return Returns the new length of the array.
 *
 *  function push(...items): Number
 */
static EjsVar *pushArray(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    EjsArray    *args;
    EjsVar      **src, **dest;
    int         i, oldLen;

    mprAssert(argc == 1 && ejsIsArray(argv[0]));

    args = (EjsArray*) argv[0];

    oldLen = ap->length;
    if (growArray(ejs, ap, ap->length + args->length) < 0) {
        return 0;
    }

    dest = ap->data;
    src = args->data;
    for (i = 0; i < args->length; i++) {
        dest[i + oldLen] = src[i];
    }
    return (EjsVar*) ejsCreateNumber(ejs, ap->length);
}


/*
 *  Reverse the order of the objects in the array. The elements are reversed in the original array.
 *  @return Returns a reference to the array.
 *
 *  function reverse(): Array
 */
static EjsVar *reverseArray(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    EjsVar  *tmp, **data;
    int     i, j;

    if (ap->length <= 1) {
        return (EjsVar*) ap;
    }

    data = ap->data;
    i = (ap->length - 2) / 2;
    j = (ap->length + 1) / 2;

    for (; i >= 0; i--, j++) {
        tmp = data[i];
        data[i] = data[j];
        data[j] = tmp;
    }
    return (EjsVar*) ap;
}


/*
 *  Remove and return the first value in the array.
 *  @return Returns the first element in the array.
 *
 *  function shift(): Object
 */
static EjsVar *shiftArray(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    EjsVar      *result, **data;
    int         i;

    if (ap->length == 0) {
        return ejs->undefinedValue;
    }

    data = ap->data;
    result = data[0];

    for (i = 1; i < ap->length; i++) {
        data[i - 1] = data[i];
    }
    ap->length--;

    return result;
}


/*
 *  Create a new array by taking a slice from an array.
 *
 *  function slice(start: Number, end: Number, step: Number = 1): Array
 */
static EjsVar *sliceArray(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    EjsArray    *result;
    EjsVar      **src, **dest;
    int         start, end, step, i, j, len;

    mprAssert(1 <= argc && argc <= 3);

    start = ejsGetInt(argv[0]);
    if (argc == 2) {
        end = ejsGetInt(argv[1]);
    } else {
        end = ap->length;
    }
    if (argc == 3) {
        step = ejsGetInt(argv[2]);
    } else {
        step = 1;
    }
    if (step == 0) {
        step = 1;
    }

    if (start < 0) {
        start += ap->length;
    }
    if (start < 0) {
        start = 0;
    } else if (start >= ap->length) {
        start = ap->length;
    }

    if (end < 0) {
        end += ap->length;
    }
    if (end < 0) {
        end = 0;
    } else if (end >= ap->length) {
        end = ap->length;
    }

    /*
     *  This may allocate too many elements if step is > 0, but length will still be correct.
     */
    result = ejsCreateArray(ejs, end - start);
    if (result == 0) {
        ejsThrowMemoryError(ejs);
        return 0;
    }
    src = ap->data;
    dest = result->data;

    len = 0;
    if (step > 0) {
        for (i = start, j = 0; i < end; i += step, j++) {
            dest[j] = src[i];
            len++;
        }

    } else {
        for (i = start, j = 0; i > end; i += step, j++) {
            dest[j] = src[i];
            len++;
        }
    }

    result->length = len;

    return (EjsVar*) result;
}


static int partition(Ejs *ejs, EjsVar **data, int p, int r)
{
    EjsVar          *tmp, *x;
    EjsString       *sx, *so;
    int             i, j;

    x = data[r];
    j = p - 1;

    for (i = p; i < r; i++) {

        sx = ejsToString(ejs, x);
        so = ejsToString(ejs, data[i]);
        if (sx == 0 || so == 0) {
            return 0;
        }
        if (strcmp(sx->value, so->value) > 0) {
            j = j + 1;
            tmp = data[j];
            data[j] = data[i];
            data[i] = tmp;
        }
    }

    data[r] = data[j + 1];
    data[j + 1] = x;

    return j + 1;
}


void quickSort(Ejs *ejs, EjsArray *ap, int p, int r)
{
    int     q;

    if (p < r) {
        q = partition(ejs, ap->data, p, r);
        quickSort(ejs, ap, p, q - 1);
        quickSort(ejs, ap, q + 1, r);
    }
}


/**
 *  Sort the array using the supplied compare function
 *  intrinsic native function sort(compare: Function = null, order: Number = 1): Array
 */
static EjsVar *sortArray(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    if (ap->length <= 1) {
        return (EjsVar*) ap;
    }
    quickSort(ejs, ap, 0, ap->length - 1);
    return (EjsVar*) ap;
}


/*
 *  Insert, remove or replace array elements. Return the removed elements.
 *
 *  function splice(start: Number, deleteCount: Number, ...values): Array
 *
 */
static EjsVar *spliceArray(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    EjsArray    *result, *values;
    EjsVar      **data, **dest, **items;
    int         start, deleteCount, i, delta, endInsert, oldLen;

    mprAssert(1 <= argc && argc <= 3);

    start = ejsGetInt(argv[0]);
    deleteCount = ejsGetInt(argv[1]);
    values = (EjsArray*) argv[2];

    if (start < 0) {
        start += ap->length;
    }
    if (start >= ap->length) {
        start = ap->length - 1;
    }

    if (deleteCount < 0) {
        deleteCount = ap->length - start + 1;
    }

    result = ejsCreateArray(ejs, deleteCount);
    if (result == 0) {
        ejsThrowMemoryError(ejs);
        return 0;
    }

    data = ap->data;
    dest = result->data;
    items = values->data;

    /*
     *  Copy removed items to the result
     */
    for (i = 0; i < deleteCount; i++) {
        dest[i] = data[i + start];
    }

    oldLen = ap->length;
    delta = values->length - deleteCount;
    
    if (delta > 0) {
        /*
         *  Make room for items to insert
         */
        if (growArray(ejs, ap, ap->length + delta) < 0) {
            return 0;
        }
        endInsert = start + delta;
        for (i = ap->length - 1; i >= endInsert; i--) {
            data[i] = data[i - delta];
        }
        
    } else {
        ap->length += delta;
    }

    /*
     *  Copy in new values
     */
    for (i = 0; i < values->length; i++) {
        data[start + i] = items[i];
    }

    /*
     *  Remove holes
     */
    if (delta < 0) {
        for (i = start + values->length; i < oldLen; i++) {
            data[i] = data[i - delta];
        }
    }
    
    return (EjsVar*) result;
}



#if ES_Object_toLocaleString
/*
 *  Convert the array to a single localized string each member of the array
 *  has toString called on it and the resulting strings are concatenated.
 *  Currently just calls toString.
 *
 *  function toLocaleString(): String
 */
static EjsVar *toLocaleString(Ejs *ejs, EjsVar *ap, int argc, EjsVar **argv)
{
    return 0;
}
#endif


/*
 *  Convert the array to a single string each member of the array has toString called on it and the resulting strings 
 *  are concatenated.
 *
 *  override function toString(): String
 */
static EjsVar *arrayToString(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    EjsString       *result;
    EjsVar          *vp;
    int             i, rc;

    result = ejsCreateString(ejs, "");
    if (result == 0) {
        ejsThrowMemoryError(ejs);
        return 0;
    }

    for (i = 0; i < ap->length; i++) {
        vp = ap->data[i];
        rc = 0;
        if (i > 0) {
            //  TODO OPT - this is slow
            rc = ejsStrcat(ejs, result, (EjsVar*) ejsCreateString(ejs, ","));
        }
        if (vp != 0 && vp != ejs->undefinedValue && vp != ejs->nullValue) {
            //  TODO OPT - this is slow
            rc = ejsStrcat(ejs, result, vp);
        }
        if (rc < 0) {
            ejsThrowMemoryError(ejs);
            return 0;
        }
    }
    return (EjsVar*) result;
}


/*
 *  Return an array with duplicate elements removed where duplicates are detected by using "==" (ie. content equality, 
 *  not strict equality).
 *
 *  function unique(): Array
 */
static EjsVar *uniqueArray(Ejs *ejs, EjsArray *ap, int argc, EjsVar **argv)
{
    EjsVar      **data;
    int         i, j, k;

    data = ap->data;

    for (i = 0; i < ap->length; i++) {
        for (j = i + 1; j < ap->length; j++) {
            if (compare(ejs, data[i], data[j])) {
                for (k = j + 1; k < ap->length; k++) {
                    data[k - 1] = data[k];
                }
                ap->length--;
            }
        }
    }
    return (EjsVar*) ap;
}


/*********************************** Support **********************************/

static int growArray(Ejs *ejs, EjsArray *ap, int len)
{
    int         size, count, factor;

    mprAssert(ap);

    if (len <= 0) {
        return 0;
    }

    if (len <= ap->length) {
        return EJS_ERR;
    }

    size = mprGetBlockSize(ap->data) / sizeof(EjsVar*);

    /*
     *  Allocate or grow the data structures
     */
    if (len > size) {
        if (size > EJS_LOTSA_PROP) {
            /*
             *  Looks like a big object so grow by a bigger chunk
             */
            factor = max(size / 4, EJS_NUM_PROP);
            len = (len + factor) / factor * factor;
        }
        count = EJS_PROP_ROUNDUP(len);

        if (ap->data == 0) {
            mprAssert(ap->length == 0);
            mprAssert(count > 0);
            ap->data = (EjsVar**) mprAllocZeroed(ap, sizeof(EjsVar*) * count);
            if (ap->data == 0) {
                return EJS_ERR;
            }

        } else {
            mprAssert(size > 0);
            ap->data = (EjsVar**) mprRealloc(ap, ap->data, sizeof(EjsVar*) * count);
            if (ap->data == 0) {
                return EJS_ERR;
            }
            ejsZeroSlots(ejs, &ap->data[ap->length], (count - ap->length));
        }
    }
    ap->length = len;

    return 0;
}


/*********************************** Factory **********************************/

EjsArray *ejsCreateArray(Ejs *ejs, int size)
{
    EjsArray    *ap;

    /*
     *  No need to invoke constructor
     */
    ap = (EjsArray*) ejsCreateObject(ejs, ejs->arrayType, 0);
    if (ap != 0) {
        if (size > 0 && growArray(ejs, ap, size) < 0) {
            ejsThrowMemoryError(ejs);
            return 0;
        }
    }

    ejsSetDebugName(ap, "array instance");

    return ap;
}


void ejsCreateArrayType(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;

    type = ejsCreateCoreType(ejs, ejsName(&qname, EJS_INTRINSIC_NAMESPACE, "Array"), ejs->objectType, sizeof(EjsArray),
        ES_Array, ES_Array_NUM_CLASS_PROP, ES_Array_NUM_INSTANCE_PROP,
        EJS_ATTR_NATIVE | EJS_ATTR_OBJECT | EJS_ATTR_DYNAMIC_INSTANCE | EJS_ATTR_HAS_CONSTRUCTOR | 
        EJS_ATTR_OBJECT_HELPERS |EJS_ATTR_OPER_OVERLOAD);
    ejs->arrayType = type;

    /*
     *  Define the helper functions.
     */
    type->helpers->castVar = (EjsCastVarHelper) castArray;
    type->helpers->cloneVar = (EjsCloneVarHelper) cloneArray;
    type->helpers->getProperty = (EjsGetPropertyHelper) getArrayProperty;
    type->helpers->getPropertyCount = (EjsGetPropertyCountHelper) getArrayPropertyCount;
    type->helpers->getPropertyByName = (EjsGetPropertyByNameHelper) getArrayPropertyByName;
    type->helpers->deleteProperty = (EjsDeletePropertyHelper) deleteArrayProperty;
    type->helpers->deletePropertyByName = (EjsDeletePropertyByNameHelper) deleteArrayPropertyByName;
    type->helpers->invokeOperator = (EjsInvokeOperatorHelper) invokeArrayOperator;
    type->helpers->lookupProperty = (EjsLookupPropertyHelper) lookupArrayProperty;
    type->helpers->markVar = (EjsMarkVarHelper) markArrayVar;
    type->helpers->setProperty = (EjsSetPropertyHelper) setArrayProperty;
    type->helpers->setPropertyByName = (EjsSetPropertyByNameHelper) setArrayPropertyByName;
    type->numericIndicies = 1;
}


void ejsConfigureArrayType(Ejs *ejs)
{
    EjsType     *type;

    type = ejs->arrayType;

    /*
     *  We override some object methods
     */
    ejsBindMethod(ejs, type, ES_Object_get, getArrayIterator);
    ejsBindMethod(ejs, type, ES_Object_getValues, getArrayValues);
    ejsBindMethod(ejs, type, ES_Object_clone, (EjsNativeFunction) cloneArrayMethod);
    ejsBindMethod(ejs, type, ES_Object_toString, (EjsNativeFunction) arrayToString);
#if UNUSED
    ejsSetAccessors(ejs, type, ES_Object_length, (EjsNativeFunction) getArrayLength, ES_Array_set_length, 
        (EjsNativeFunction) setArrayLength);
#else
    ejsBindMethod(ejs, type, ES_Object_length, (EjsNativeFunction) getArrayLength);
    ejsBindMethod(ejs, type, ES_Array_set_length, (EjsNativeFunction) setArrayLength);
#endif

    /*
     *  Methods and Operators, including constructor.
     */
    ejsBindMethod(ejs, type, ES_Array_Array, (EjsNativeFunction) arrayConstructor);
    ejsBindMethod(ejs, type, ES_Array_append, (EjsNativeFunction) appendArray);
    ejsBindMethod(ejs, type, ES_Array_clear, (EjsNativeFunction) clearArray);
    ejsBindMethod(ejs, type, ES_Array_compact, (EjsNativeFunction) compactArray);
    ejsBindMethod(ejs, type, ES_Array_concat, (EjsNativeFunction) concatArray);

    ejsBindMethod(ejs, type, ES_Array_indexOf, (EjsNativeFunction) indexOfArray);
    ejsBindMethod(ejs, type, ES_Array_insert, (EjsNativeFunction) insertArray);
    ejsBindMethod(ejs, type, ES_Array_join, (EjsNativeFunction) joinArray);
    ejsBindMethod(ejs, type, ES_Array_lastIndexOf, (EjsNativeFunction) lastArrayIndexOf);
    ejsBindMethod(ejs, type, ES_Array_pop, (EjsNativeFunction) popArray);
    ejsBindMethod(ejs, type, ES_Array_push, (EjsNativeFunction) pushArray);
    ejsBindMethod(ejs, type, ES_Array_reverse, (EjsNativeFunction) reverseArray);
    ejsBindMethod(ejs, type, ES_Array_shift, (EjsNativeFunction) shiftArray);
    ejsBindMethod(ejs, type, ES_Array_slice, (EjsNativeFunction) sliceArray);
    ejsBindMethod(ejs, type, ES_Array_sort, (EjsNativeFunction) sortArray);
    ejsBindMethod(ejs, type, ES_Array_splice, (EjsNativeFunction) spliceArray);
    ejsBindMethod(ejs, type, ES_Array_unique, (EjsNativeFunction) uniqueArray);

#if FUTURE
    ejsBindMethod(ejs, type, ES_Array_toLocaleString, toLocaleString);
    ejsBindMethod(ejs, type, ES_Array_toJSONString, toJSONString);
    ejsBindMethod(ejs, type, ES_Array_LBRACKET, operLBRACKET);
    ejsBindMethod(ejs, type, ES_Array_AND, operAND);
    ejsBindMethod(ejs, type, ES_Array_EQ, operEQ);
    ejsBindMethod(ejs, type, ES_Array_GT, operGT);
    ejsBindMethod(ejs, type, ES_Array_LT, operLT);
    ejsBindMethod(ejs, type, ES_Array_LSH, operLSH);
    ejsBindMethod(ejs, type, ES_Array_MINUS, operMINUS);
    ejsBindMethod(ejs, type, ES_Array_OR, operOR);
    ejsBindMethod(ejs, type, ES_Array_AND, operAND);
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
