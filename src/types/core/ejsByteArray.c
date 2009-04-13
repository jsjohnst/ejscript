/*
 *  ejsByteArray.c - Ejscript ByteArray class
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejs.h"

/***************************** Forward Declarations ***************************/

static int  flushByteArray(Ejs *ejs, EjsByteArray *ap);
static int  getInput(Ejs *ejs, EjsByteArray *ap, int required);
static int  growByteArray(Ejs *ejs, EjsByteArray *ap, int len);
static int  lookupByteArrayProperty(Ejs *ejs, EjsByteArray *ap, EjsName *qname);
 static bool makeRoom(Ejs *ejs, EjsByteArray *ap, int require);
static EjsVar *byteArrayToString(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv);

static MPR_INLINE int swap16(EjsByteArray *ap, int a);
static MPR_INLINE int swap32(EjsByteArray *ap, int a);
static MPR_INLINE int64 swap64(EjsByteArray *ap, int64 a);
static MPR_INLINE double swapDouble(EjsByteArray *ap, double a);
static void putByte(EjsByteArray *ap, int value);
static void putInteger(EjsByteArray *ap, int value);
static void putLong(EjsByteArray *ap, int64 value);
static void putShort(EjsByteArray *ap, int value);
static void putString(EjsByteArray *ap, cchar *value, int len);
static void putNumber(EjsByteArray *ap, MprNumber value);

#if BLD_FEATURE_FLOATING_POINT
static void putDouble(EjsByteArray *ap, double value);
#endif

#define availableBytes(ap)  (((EjsByteArray*) ap)->writePosition - ((EjsByteArray*) ap)->readPosition)
#define room(ap) (ap->length - ap->writePosition)
#define adjustReadPosition(ap, amt) \
    if (1) { \
        ap->readPosition += amt; \
        if (ap->readPosition == ap->writePosition) {    \
            ap->readPosition = ap->writePosition = 0; \
        } \
    } else

/******************************************************************************/
/*
 *  Cast the object operand to a primitive type
 */

static EjsVar *castByteArrayVar(Ejs *ejs, EjsByteArray *vp, EjsType *type)
{
    switch (type->id) {
    case ES_Boolean:
        return (EjsVar*) ejs->trueValue;

    case ES_Number:
        return (EjsVar*) ejs->zeroValue;

    case ES_String:
        return byteArrayToString(ejs, vp, 0, 0);

    default:
        ejsThrowTypeError(ejs, "Can't cast to this type");
        return 0;
    }
}


static EjsByteArray *cloneByteArrayVar(Ejs *ejs, EjsByteArray *ap, bool deep)
{
    EjsByteArray    *newArray;
    int             i;

    newArray = ejsCreateByteArray(ejs, ap->length);
    if (newArray == 0) {
        ejsThrowMemoryError(ejs);
        return 0;
    }

    for (i = 0; i < ap->length; i++) {
        newArray->value[i] = ap->value[i];
    }

    return newArray;
}


/*
 *  Delete a property and update the length
 */
static int deleteByteArrayProperty(struct Ejs *ejs, EjsByteArray *ap, int slot)
{
    if (slot >= ap->length) {
        ejsThrowOutOfBoundsError(ejs, "Bad subscript");
        return EJS_ERR;
    }
    if ((slot + 1) == ap->length) {
        ap->length--;
        if (ap->readPosition >= ap->length) {
            ap->readPosition = ap->length - 1;
        }
        if (ap->writePosition >= ap->length) {
            ap->writePosition = ap->length - 1;
        }
    }

    if (ejsSetProperty(ejs, (EjsVar*) ap, slot, (EjsVar*) ejs->undefinedValue) < 0) {
        return EJS_ERR;
    }
    return 0;
}


/*
 *  Return the number of elements in the array
 */
static int getByteArrayPropertyCount(Ejs *ejs, EjsByteArray *ap)
{
    return ap->length;
}


/*
 *  Get an array element. Slot numbers correspond to indicies.
 */
static EjsVar *getByteArrayProperty(Ejs *ejs, EjsByteArray *ap, int slotNum)
{
    if (slotNum < 0 || slotNum >= ap->length) {
        ejsThrowOutOfBoundsError(ejs, "Bad array subscript");
        return 0;
    }
    return (EjsVar*) ejsCreateNumber(ejs, ap->value[slotNum]);
}


/*
 *  Lookup an array index.
 */
static int lookupByteArrayProperty(struct Ejs *ejs, EjsByteArray *ap, EjsName *qname)
{
    int     index;

    if (qname == 0 || ! isdigit((int) qname->name[0])) {
        return EJS_ERR;
    }
    index = atoi(qname->name);
    if (index < ap->length) {
        return index;
    }
    return EJS_ERR;
}


/*
 *  Cast operands as required for invokeOperator
 */
static EjsVar *coerceByteArrayOperands(Ejs *ejs, EjsVar *lhs, int opcode,  EjsVar *rhs)
{
    switch (opcode) {
    /*
     *  Binary operators
     */
    case EJS_OP_ADD:
        return ejsInvokeOperator(ejs, byteArrayToString(ejs, (EjsByteArray*) lhs, 0, 0), opcode, rhs);

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


static EjsVar *invokeByteArrayOperator(Ejs *ejs, EjsVar *lhs, int opcode,  EjsVar *rhs)
{
    EjsVar      *result;

    if (rhs == 0 || lhs->type != rhs->type) {
        if ((result = coerceByteArrayOperands(ejs, lhs, opcode, rhs)) != 0) {
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

    default:
        ejsThrowTypeError(ejs, "Opcode %d not implemented for type %s", opcode, lhs->type->qname.name);
        return 0;
    }

    mprAssert(0);
}


static void markByteArrayVar(Ejs *ejs, EjsVar *parent, EjsByteArray *ap)
{
    mprAssert(ejsIsByteArray(ap));

    if (ap->input) {
        ejsMarkVar(ejs, (EjsVar*) ap, (EjsVar*) ap->input);
    }
}


/*
 *  Create or update an array elements. If slotNum is < 0, then create the next free array slot. If slotNum is greater
 *  than the array length, grow the array.
 */
static int setByteArrayProperty(struct Ejs *ejs, EjsByteArray *ap, int slotNum,  EjsVar *value)
{
    if (slotNum >= ap->length) {
        if (growByteArray(ejs, ap, slotNum + 1) < 0) {
            return EJS_ERR;
        }
    }
    if (ejsIsNumber(value)) {
        ap->value[slotNum] = ejsGetInt(value);
    } else {
        ap->value[slotNum] = ejsGetInt(ejsToNumber(ejs, value));
    }

    if (slotNum >= ap->length) {
        ap->length = slotNum + 1;
    }
    return slotNum;
}


/********************************** Methods *********************************/
/*
 *  ByteArray constructor.
 *
 *  function ByteArray(size: Number = -1, growable: Boolean = true): ByteArray
 */
static EjsVar *byteArrayConstructor(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    bool    growable;
    int     size;

    mprAssert(0 <= argc && argc <= 2);

    size = (argc >= 1) ? ejsGetInt(argv[0]) : MPR_BUFSIZE;
    if (size <= 0) {
        size = 1;
    }
    growable = (argc == 2) ? ejsGetBoolean(argv[1]): 1;

    if (growByteArray(ejs, ap, size) < 0) {
        return 0;
    }
    mprAssert(ap->value);
    ap->growable = growable;
    ap->growInc = MPR_BUFSIZE;
    ap->length = size;
    ap->endian = mprGetEndian(ejs);

    return (EjsVar*) ap;
}


/**
 *  Get the number of bytes that are currently available on this stream for reading.
 *
 *  function get available(): Number
 */
static EjsVar *availableProc(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, ap->writePosition - ap->readPosition);
}


/*
 *  Copy data into the array. Data is written at the $destOffset.
 *
 *  function copyIn(destOffset: Number, src: ByteArray, srcOffset: Number = 0, count: Number = -1): Void
 */
static EjsVar *copyIn(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    EjsByteArray    *src;
    int             i, destOffset, srcOffset, count;

    destOffset = ejsGetInt(argv[0]);
    src = (EjsByteArray*) argv[1];
    srcOffset = (argc > 2) ? ejsGetInt(argv[2]) : 0;
    count = (argc > 3) ? ejsGetInt(argv[3]) : MAXINT;

    if (srcOffset >= src->length) {
        ejsThrowOutOfBoundsError(ejs, "Bad source data offset");
        return 0;
    }
    count = min(src->length - srcOffset, count);

    makeRoom(ejs, ap, destOffset + count);
    if ((destOffset + count) > src->length) {
        ejsThrowOutOfBoundsError(ejs, "Insufficient room for data");
        return 0;
    }

    for (i = 0; i < count; i++) {
        ap->value[destOffset++] = src->value[srcOffset++];
    }
    return 0;
}


/*
 *  Copy data from the array. Data is copied from the $srcOffset.
 *
 *  function copyOut(srcOffset: Number, dest: ByteArray, destOffset: Number = 0, count: Number = -1): Number
 */
static EjsVar *copyOut(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    EjsByteArray    *dest;
    int             i, srcOffset, destOffset, count;

    srcOffset = ejsGetInt(argv[0]);
    dest = (EjsByteArray*) argv[1];
    destOffset = (argc > 2) ? ejsGetInt(argv[2]) : 0;
    count = (argc > 3) ? ejsGetInt(argv[3]) : MAXINT;

    if (srcOffset >= ap->length) {
        ejsThrowOutOfBoundsError(ejs, "Bad source data offset");
        return 0;
    }
    count = min(ap->length - srcOffset, count);

    makeRoom(ejs, dest, destOffset + count);
    if ((destOffset + count) > dest->length) {
        ejsThrowOutOfBoundsError(ejs, "Insufficient room for data");
        return 0;
    }

    for (i = 0; i < count; i++) {
        dest->value[destOffset++] = ap->value[srcOffset++];
    }
    return 0;
}


/*
 *  Determine if the system is using little endian byte ordering
 *
 *  function get endian(): Number
 */
static EjsVar *endian(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, ap->endian);
}


/*
 *  Set the system encoding to little or big endian.
 *
 *  function set endian(value: Number): Void
 */
static EjsVar *setEndian(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    int     endian;

    mprAssert(argc == 1 && ejsIsNumber(argv[0]));

    endian = ejsGetInt(argv[0]);
    if (endian != 0 && endian != 1) {
        ejsThrowArgError(ejs, "Bad endian value");
        return 0;
    }

    ap->endian = endian;
    ap->swap = (ap->endian != mprGetEndian(ejs));

    return 0;
}


/*
 *  Function to iterate and return the next element index.
 *  NOTE: this is not a method of Array. Rather, it is a callback function for Iterator
 */
static EjsVar *nextByteArrayKey(Ejs *ejs, EjsIterator *ip, int argc, EjsVar **argv)
{
    EjsByteArray    *ap;

    ap = (EjsByteArray*) ip->target;
    if (!ejsIsByteArray(ap)) {
        ejsThrowReferenceError(ejs, "Wrong type");
        return 0;
    }

    if (ip->index < ap->readPosition) {
        ip->index = ap->readPosition;
    }
    if (ip->index < ap->writePosition) {
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
static EjsVar *getByteArrayIterator(Ejs *ejs, EjsVar *ap, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateIterator(ejs, ap, (EjsNativeFunction) nextByteArrayKey, 0, NULL);
}


/*
 *  Function to iterate and return the next element value.
 *  NOTE: this is not a method of Array. Rather, it is a callback function for Iterator
 */
static EjsVar *nextByteArrayValue(Ejs *ejs, EjsIterator *ip, int argc, EjsVar **argv)
{
    EjsByteArray    *ap;

    ap = (EjsByteArray*) ip->target;
    if (!ejsIsByteArray(ap)) {
        ejsThrowReferenceError(ejs, "Wrong type");
        return 0;
    }

    if (ip->index < ap->readPosition) {
        ip->index = ap->readPosition;
    }
    if (ip->index < ap->writePosition) {
        return (EjsVar*) ejsCreateNumber(ejs, ap->value[ip->index++]);
    }

    ejsThrowStopIteration(ejs);
    return 0;
}


/*
 *  Return an iterator to return the next array element value.
 *
 *  iterator native function getValues(): Iterator
 */
static EjsVar *getByteArrayValues(Ejs *ejs, EjsVar *ap, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateIterator(ejs, ap, (EjsNativeFunction) nextByteArrayValue, 0, NULL);
}


/**
 *  Callback function to called when read data is required. Callback signature:
 *      function callback(buffer: ByteArray, offset: Number, count: Number): Number
 *
 *  function set input(value: Function): Void
 */
static EjsVar *inputByteArrayData(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    mprAssert(argc == 1 && ejsIsFunction(argv[0]));

    ap->input = (EjsFunction*) argv[0];
    return 0;
}


/*
 *  Flush the data in the byte array and reset the read and write position pointers
 *
 *  function flush(): Void
 */
static EjsVar *flushProc(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    mprAssert(argc == 0);

    flushByteArray(ejs, ap);
    ap->writePosition = ap->readPosition = 0;
    return 0;
}


/*
 *  Get the length of an array.
 *  @return Returns the number of items in the array
 *
 *  intrinsic override function get length(): Number
 */
static EjsVar *getByteArrayLength(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, ap->length);
}


#if UNUSED && TODO
/*
 *  Set the length of an array.
 *
 *  intrinsic override function set length(value: Number): void
 */
static EjsVar *setByteArrayLength(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    mprAssert(argc == 1 && ejsIsNumber(argv[0]));
    mprAssert(ejsIsByteArray(ap));

    ap->length = ejsGetInt(argv[0]);
    if (ap->readPosition >= ap->length) {
        ap->readPosition = ap->length - 1;
    }
    if (ap->writePosition >= ap->length) {
        ap->writePosition = ap->length - 1;
    }

    return 0;
}
#endif


/**
 *  Function to call to flush data. Callback signature:
 *      function callback(...data): Number
 *
 *  function set output(value: Function): Void
 */
static EjsVar *output(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    mprAssert(argc == 1 && ejsIsFunction(argv[0]));

    ap->output = (EjsFunction*) argv[0];
    return 0;
}


/*
 *  Read data from the array into another byte array. Data is read from the current read $position pointer.
 *  Data is written to the write position if offset is -1. Othwise at the given offset. If offset is < 0, the write position is
 *  updated.
 *
 *  function read(buffer: ByteArray, offset: Number = -1, count: Number = -1): Number
 */
static EjsVar *byteArrayReadProc(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    EjsByteArray    *buffer;
    int             offset, count, i, originalOffset;

    mprAssert(1 <= argc && argc <= 3);

    buffer = (EjsByteArray*) argv[0];
    offset = (argc == 2) ? ejsGetInt(argv[1]) : -1;
    count = (argc == 3) ? ejsGetInt(argv[2]) : buffer->length;

    if (count < 0) {
        count = buffer->length;
    }
    originalOffset = offset;
    if (offset < 0) {
        offset = buffer->writePosition;
    } else if (offset >= buffer->length) {
        offset = 0;
    }

    if (getInput(ejs, ap, 1) <= 0) {
        return (EjsVar*) ejs->zeroValue;
    }

    count = min(availableBytes(ap), count);
    for (i = 0; i < count; i++) {
        buffer->value[offset++] = ap->value[ap->readPosition++];
    }
    if (originalOffset < 0) {
        buffer->writePosition += count;
    }

    return (EjsVar*) ejsCreateNumber(ejs, count);
}


/*
 *  Read a boolean from the array.
 *
 *  function readBoolean(): Boolean
 */
static EjsVar *readBoolean(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    int     result;

    if (getInput(ejs, ap, 1) <= 0) {
        return 0;
    }
    result = ap->value[ap->readPosition];
    adjustReadPosition(ap, 1);

    return (EjsVar*) ejsCreateBoolean(ejs, result);
}


/*
 *  Read a byte from the array.
 *
 *  function readByte(): Number
 */
static EjsVar *readByte(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    int     result;

    if (getInput(ejs, ap, 1) <= 0) {
        return 0;
    }
    result = ap->value[ap->readPosition];
    adjustReadPosition(ap, 1);

    return (EjsVar*) ejsCreateNumber(ejs, result);
}


/**
 *  Read a date from the array.
 *
 *  function readDate(): Date
 */
static EjsVar *readDate(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    double  value;

    if (getInput(ejs, ap, EJS_SIZE_DOUBLE) <= 0) {
        return 0;
    }

    value = * (double*) &ap->value[ap->readPosition];
    value = swapDouble(ap, value);
    adjustReadPosition(ap, sizeof(double));

    return (EjsVar*) ejsCreateDate(ejs, (MprTime) value);
}


#if BLD_FEATURE_FLOATING_POINT && ES_ByteArray_readDouble
/**
 *  Read a double from the array. The data will be decoded according to the encoding property.
 *
 *  function readDouble(): Date
 */
static EjsVar *readDouble(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    double  value;

    if (getInput(ejs, ap, EJS_SIZE_DOUBLE) <= 0) {
        return 0;
    }

    value = * (double*) &ap->value[ap->readPosition];
    value = swapDouble(ap, value);
    adjustReadPosition(ap, sizeof(double));

    return (EjsVar*) ejsCreateNumber(ejs, (MprNumber) value);
}
#endif

/*
 *  Read a 32-bit integer from the array. The data will be decoded according to the encoding property.
 *
 *  function readInteger(): Number
 */
static EjsVar *readInteger(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    int     value;

    if (getInput(ejs, ap, EJS_SIZE_INT) <= 0) {
        return 0;
    }

    value = * (int*) &ap->value[ap->readPosition];
    value = swap32(ap, value);
    adjustReadPosition(ap, sizeof(int));

    return (EjsVar*) ejsCreateNumber(ejs, value);
}


/*
 *  Read a 64-bit long from the array.The data will be decoded according to the encoding property.
 *
 *  function readLong(): Number
 */
static EjsVar *readLong(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    int64   value;

    if (getInput(ejs, ap, EJS_SIZE_LONG) <= 0) {
        return 0;
    }

    value = * (int64*) &ap->value[ap->readPosition];
    value = swap64(ap, value);
    adjustReadPosition(ap, sizeof(int64));

    return (EjsVar*) ejsCreateNumber(ejs, (MprNumber) value);
}


/*
 *  Get the current read position offset
 *
 *  function get readPosition(): Number
 */
static EjsVar *readPosition(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, ap->readPosition);
}


/*
 *  Set the current read position offset
 *
 *  function set readPosition(position: Number): Void
 */
static EjsVar *setReadPosition(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    int     pos;

    mprAssert(argc == 1 && ejsIsNumber(argv[0]));

    pos = ejsGetInt(argv[0]);
    if (pos < 0 || pos > ap->length) {
        ejsThrowOutOfBoundsError(ejs, "Bad position value");
        return 0;
    }
    if (pos > ap->writePosition) {
        ejsThrowStateError(ejs, "Read position is greater than write position");
    } else {
        ap->readPosition = pos;
    }
    return 0;
}


/*
 *  Read a 16-bit short integer from the array. The data will be decoded according to the encoding property.
 *
 *  function readShort(): Number
 */
static EjsVar *readShort(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    int     value;

    if (getInput(ejs, ap, EJS_SIZE_SHORT) <= 0) {
        return 0;
    }

    value = * (short*) &ap->value[ap->readPosition];
    value = swap16(ap, value);
    adjustReadPosition(ap, sizeof(short));

    return (EjsVar*) ejsCreateNumber(ejs, value);
}


/*
 *  Read a UTF-8 string from the array. Read data from the read position up to the write position but not more than count
 *  characters.
 *
 *  function readString(count: Number = -1): String
 */
static EjsVar *baReadString(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    EjsVar  *result;
    int     count;

    count = (argc == 1) ? ejsGetInt(argv[0]) : -1;

    if (count < 0) {
        if (getInput(ejs, ap, 1) < 0) {
            return 0;
        }
        count = availableBytes(ap);

    } else if (getInput(ejs, ap, count) < 0) {
        return 0;
    }

    count = min(count, availableBytes(ap));
    result = (EjsVar*) ejsCreateStringWithLength(ejs, (cchar*) &ap->value[ap->readPosition], count);
    adjustReadPosition(ap, count);

    return result;
}


#if ES_ByteArray_readXML && BLD_FEATURE_EJS_E4X
/*
 *  Read an XML document from the array.
 *
 *  function readXML(): XML
 */
static EjsVar *readXML(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    EjsXML      *xml;

    //  TODO - does this work?
    if (getInput(ejs, ap, -1) <= 0) {
        return 0;
    }

    xml = ejsCreateXML(ejs, 0, 0, 0, 0);
    if (xml == 0) {
        mprAssert(ejs->exception);
        return 0;
    }

    //  TODO - need to make sure that the string is null terminated
    //  TODO - need a rc
    ejsLoadXMLString(ejs, xml, (cchar*) &ap->value[ap->readPosition]);

    //  TODO -
    adjustReadPosition(ap, 0);

    return (EjsVar*) xml;
}
#endif


/*
 *  Reset the read and write position pointers if there is no available data.
 *
 *  function reset(): Void
 */
static EjsVar *reset(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    mprAssert(argc == 0);

    if (ap->writePosition == ap->readPosition) {
        ap->writePosition = ap->readPosition = 0;
    }
    return 0;
}


/**
 *  Get the number of data bytes that the array can store from the write position till the end of the array.
 *
 *  function get room(): Number
 */
static EjsVar *roomProc(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, ap->length - ap->writePosition);
}


/*
 *  Convert the byte array data between the read and write positions into a string.
 *
 *  override function toString(): String
 */
static EjsVar *byteArrayToString(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateStringWithLength(ejs, (cchar*) &ap->value[ap->readPosition], availableBytes(ap));
}


/*
 *  Write data to the array. Data is written to the current write $position pointer.
 *
 *  function write(...data): Number
 */
EjsNumber *ejsWriteToByteArray(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    EjsArray        *args;
    EjsByteArray    *bp;
    EjsString       *sp;
    EjsVar          *vp;
    int             i, start, len;

    mprAssert(argc == 1 && ejsIsArray(argv[0]));

    /*
     *  Unwrap nested arrays
     */
    args = (EjsArray*) argv[0];
    while (args && ejsIsArray(args) && args->length == 1) {
        vp = ejsGetProperty(ejs, (EjsVar*) args, 0);
        if (!ejsIsArray(vp)) {
            break;
        }
        args = (EjsArray*) vp;
    }

    start = ap->writePosition;

    for (i = 0; i < args->length; i++) {

        vp = ejsGetProperty(ejs, (EjsVar*) args, i);
        if (vp == 0) {
            continue;
        }

        switch (vp->type->id) {

        case ES_Boolean:
            if (!makeRoom(ejs, ap, EJS_SIZE_BOOLEAN)) {
                return 0;
            }
            putByte(ap, ejsGetBoolean(vp));
            break;

        case ES_Date:
            if (!makeRoom(ejs, ap, EJS_SIZE_DOUBLE)) {
                return 0;
            }
            putNumber(ap, (MprNumber) ((EjsDate*) vp)->value);
            break;

        case ES_Number:
            if (!makeRoom(ejs, ap, EJS_SIZE_DOUBLE)) {
                return 0;
            }
            putNumber(ap, ejsGetNumber(vp));
            break;

        case ES_String:
            if (!makeRoom(ejs, ap, ((EjsString*) vp)->length)) {
                return 0;
            }
            sp = (EjsString*) vp;
            putString(ap, sp->value, sp->length);
            break;

        default:
            sp = ejsToString(ejs, vp);
            putString(ap, sp->value, sp->length);
            break;

        case ES_ByteArray:
            bp = (EjsByteArray*) vp;
            len = availableBytes(bp);
            if (!makeRoom(ejs, ap, len)) {
                return 0;
            }
            /*
             *  Note: this only copies between the read/write positions of the source byte array
             */
            ejsCopyToByteArray(ejs, ap, ap->writePosition, (char*) &bp->value[bp->readPosition], len);
            ap->writePosition += len;
            break;
        }
    }
    return ejsCreateNumber(ejs, ap->writePosition - start);
}


/*
 *  Write a byte to the array
 *
 *  function writeByte(value: Number): Void
 */
static EjsVar *writeByte(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    if (!makeRoom(ejs, ap, 1)) {
        return 0;
    }
    putByte(ap, ejsGetInt(argv[0]));
    return 0;
}


/*
 *  Write a short to the array
 *
 *  function writeShort(value: Number): Void
 */
static EjsVar *writeShort(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    if (!makeRoom(ejs, ap, sizeof(short))) {
        return 0;
    }
    putShort(ap, ejsGetInt(argv[0]));
    return 0;
}


#if ES_ByteArray_writeDouble && BLD_FEATURE_FLOATING_POINT
/*
 *  Write a double to the array
 *
 *  function writeDouble(value: Number): Void
 */
static EjsVar *writeDouble(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    if (!makeRoom(ejs, ap, sizeof(double))) {
        return 0;
    }
    putDouble(ap, ejsGetDouble(argv[0]));
    return 0;
}
#endif


/*
 *  Write an integer (32 bits) to the array
 *
 *  function writeInteger(value: Number): Void
 */

static EjsVar *writeInteger(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    if (!makeRoom(ejs, ap, sizeof(int))) {
        return 0;
    }
    putInteger(ap, ejsGetInt(argv[0]));
    return 0;
}


/*
 *  Write a long (64 bit) to the array
 *
 *  function writeLong(value: Number): Void
 */
static EjsVar *writeLong(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    if (!makeRoom(ejs, ap, sizeof(int))) {
        return 0;
    }
    putLong(ap, ejsGetInt(argv[0]));
    return 0;
}


/*
 *  Get the current write position offset
 *
 *  function get writePosition(): Number
 */
static EjsVar *writePosition(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, ap->writePosition);
}


/*
 *  Set the current write position offset
 *
 *  function set writePosition(position: Number): Void
 */
static EjsVar *setWritePosition(Ejs *ejs, EjsByteArray *ap, int argc, EjsVar **argv)
{
    int     pos;

    mprAssert(argc == 1 && ejsIsNumber(argv[0]));

    pos = ejsGetInt(argv[0]);
    if (pos < 0 || pos >= ap->length) {
        pos = 0;
    }
    if (pos < ap->readPosition) {
        ejsThrowStateError(ejs, "Write position is less than read position");
    } else {
        ap->writePosition = pos;
    }
    return 0;
}


/*********************************** Support **********************************/
/*
 *  Flush the array. If an output function is defined, invoke it to accept the data.
 */
static int flushByteArray(Ejs *ejs, EjsByteArray *ap)
{
    EjsVar      *arg;

    if (ap->output == 0) {
        return 0;
    }

    while (availableBytes(ap) && !ejs->exception) {
        arg = (EjsVar*) ap;
        ejsRunFunction(ejs, ap->output, (EjsVar*) ap, 1, &arg);
    }
    ap->writePosition = ap->readPosition = 0;

    return 0;
}


static int growByteArray(Ejs *ejs, EjsByteArray *ap, int len)
{
    if (len > ap->length) {
        ap->value = mprRealloc(ap, ap->value, len * sizeof(char*));
        if (ap->value == 0) {
            ejsThrowMemoryError(ejs);
            return EJS_ERR;
        }
        memset(&ap->value[ap->length], 0, len - ap->length);
        ap->growInc = min(ap->growInc * 2, 32 * 1024);
        ap->length = len;
    }
    return 0;
}


/*
 *  Get more input, sufficient to satisfy the rquired number of bytes. The required parameter specifies how many bytes MUST be 
 *  read. Short fills are not permitted. Return the count of bytes available or 0 if the required number of bytes can't be read. 
 *  Return -ve on errors.
 */
static int getInput(Ejs *ejs, EjsByteArray *ap, int required)
{
    EjsVar      *argv[3];
    int         lastPos;

    if (availableBytes(ap) == 0) {
        ap->writePosition = ap->readPosition = 0;
    }
    if (ap->input) {
        while (availableBytes(ap) < required && !ejs->exception) {
            lastPos = ap->writePosition;

            /*
             *  Run the input function. Get as much data as possible without blocking.
             *  The input function will write to the byte array and will update the writePosition.
             */
            argv[0] = (EjsVar*) ap;
            ejsRunFunction(ejs, ap->input, (EjsVar*) ap, 1, argv);
            if (lastPos == ap->writePosition) {
                break;
            }
#if UNUSED
            if (!ejsIsNumber(result)) {
                if (ejs->exception == NULL) {
                    ejsThrowIOError(ejs, "input callback did not return a number");
                }
                return EJS_ERR;
            }
            count = ejsGetInt(result);
            if (count < 0) {
                ejsThrowIOError(ejs, "Can't read data");
                return EJS_ERR;

            } else if (count == 0) {
                return 0;
            }
#endif
        }
    }
    if (availableBytes(ap) < required) {
        return 0;
    }
    return availableBytes(ap);
}


static bool makeRoom(Ejs *ejs, EjsByteArray *ap, int require)
{
    int     newLen;

    if (room(ap) < require) {
        if (flushByteArray(ejs, ap) < 0) {
            return 0;
        }
        newLen = max(ap->length + require, ap->length + ap->growInc);
        if (!ap->growable || growByteArray(ejs, ap, newLen) < 0) {
            if (ejs->exception == NULL) {
                ejsThrowResourceError(ejs, "Byte array is too small");
            }
            return 0;
        }
    }
    return 1;
}


static MPR_INLINE int swap16(EjsByteArray *ap, int a)
{
    if (!ap->swap) {
        return a;
    }
    return (a & 0xFF) << 8 | (a & 0xFF00 >> 8);
}


static MPR_INLINE int swap32(EjsByteArray *ap, int a)
{
    if (!ap->swap) {
        return a;
    }
    return (a & 0xFF) << 24 | (a & 0xFF00 << 8) | (a & 0xFF0000 >> 8) | (a & 0xFF000000 >> 16);
}


static MPR_INLINE int64 swap64(EjsByteArray *ap, int64 a)
{
    int64   low, high;

    if (!ap->swap) {
        return a;
    }

    low = a & 0xFFFFFFFF;
    high = (a >> 32) & 0xFFFFFFFF;

    return  (low & 0xFF) << 24 | (low & 0xFF00 << 8) | (low & 0xFF0000 >> 8) | (low & 0xFF000000 >> 16) |
            ((high & 0xFF) << 24 | (high & 0xFF00 << 8) | (high & 0xFF0000 >> 8) | (high & 0xFF000000 >> 16)) << 32;
}


static MPR_INLINE double swapDouble(EjsByteArray *ap, double a)
{
    int64   low, high;

    if (!ap->swap) {
        return a;
    }

    low = ((int64) a) & 0xFFFFFFFF;
    high = (((int64) a) >> 32) & 0xFFFFFFFF;

    return  (double) ((low & 0xFF) << 24 | (low & 0xFF00 << 8) | (low & 0xFF0000 >> 8) | (low & 0xFF000000 >> 16) |
            ((high & 0xFF) << 24 | (high & 0xFF00 << 8) | (high & 0xFF0000 >> 8) | (high & 0xFF000000 >> 16)) << 32);
}


static void putByte(EjsByteArray *ap, int value)
{
    ap->value[ap->writePosition++] = (char) value;
}


static void putShort(EjsByteArray *ap, int value)
{
    value = swap16(ap, value);

    *((short*) &ap->value[ap->writePosition]) = (short) value;
    ap->writePosition += sizeof(short);
}


static void putInteger(EjsByteArray *ap, int value)
{
    value = swap32(ap, value);

    *((int*) &ap->value[ap->writePosition]) = (int) value;
    ap->writePosition += sizeof(int);
}


static void putLong(EjsByteArray *ap, int64 value)
{
    value = swap64(ap, value);

    *((int64*) &ap->value[ap->writePosition]) = value;
    ap->writePosition += sizeof(int64);
}


#if BLD_FEATURE_FLOATING_POINT
static void putDouble(EjsByteArray *ap, double value)
{
    value = swapDouble(ap, value);

    *((double*) &ap->value[ap->writePosition]) = value;
    ap->writePosition += sizeof(double);
}
#endif


/*
 *  Write a number in the default number encoding
 */
static void putNumber(EjsByteArray *ap, MprNumber value)
{
#if BLD_FEATURE_FLOATING_POINT
    putDouble(ap, value);
#elif MPR_64_BIT
    putLong(ap, value);
#else
    putInteger(ap, value);
#endif
}


static void putString(EjsByteArray *ap, cchar *value, int len)
{
    mprMemcpy(&ap->value[ap->writePosition], room(ap), value, len);
    ap->writePosition += len;
}


void ejsSetByteArrayPositions(Ejs *ejs, EjsByteArray *ap, int readPosition, int writePosition)
{
    //  TODO - should validate the positions here against valid limits
    if (readPosition >= 0) {
        ap->readPosition = readPosition;
    }
    if (writePosition >= 0) {
        ap->writePosition = writePosition;
    }
}


//  TODO - should return a count byte copied
int ejsCopyToByteArray(Ejs *ejs, EjsByteArray *ap, int offset, char *data, int length)
{
    int     i;

    mprAssert(ap);
    mprAssert(data);

    if (!makeRoom(ejs, ap, offset + length)) {
        return EJS_ERR;
    }

    if (ap->length < (offset + length)) {
        return EJS_ERR;
    }

    for (i = 0; i < ap->length; i++) {
        ap->value[offset++] = data[i];
    }
    return 0;
}


int ejsGetAvailableData(EjsByteArray *ap)
{
    return availableBytes(ap);
}

/*********************************** Factory **********************************/

EjsByteArray *ejsCreateByteArray(Ejs *ejs, int size)
{
    EjsByteArray    *ap;

    /*
     *  No need to invoke constructor
     */
    ap = (EjsByteArray*) ejsCreateVar(ejs, ejs->byteArrayType, 0);
    if (ap == 0) {
        return 0;
    }

    if (size <= 0) {
        size = MPR_BUFSIZE;
    }

    if (growByteArray(ejs, ap, size) < 0) {
        return 0;
    }
    ap->length = size;
    ap->growable = 1;
    ap->growInc = MPR_BUFSIZE;
    ap->endian = mprGetEndian(ejs);

    ejsSetDebugName(ap, "ByteArray instance");

    return ap;
}


void ejsCreateByteArrayType(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;

    type = ejsCreateCoreType(ejs, ejsName(&qname, EJS_INTRINSIC_NAMESPACE, "ByteArray"), ejs->objectType, sizeof(EjsByteArray),
        ES_ByteArray, ES_ByteArray_NUM_CLASS_PROP, ES_ByteArray_NUM_INSTANCE_PROP, EJS_ATTR_NATIVE | EJS_ATTR_HAS_CONSTRUCTOR);
    ejs->byteArrayType = type;

    /*
     *  Define the helper functions.
     */
    type->helpers->castVar = (EjsCastVarHelper) castByteArrayVar;
    type->helpers->cloneVar = (EjsCloneVarHelper) cloneByteArrayVar;
    type->helpers->getProperty = (EjsGetPropertyHelper) getByteArrayProperty;
    type->helpers->getPropertyCount = (EjsGetPropertyCountHelper) getByteArrayPropertyCount;
    type->helpers->deleteProperty = (EjsDeletePropertyHelper) deleteByteArrayProperty;
    type->helpers->invokeOperator = (EjsInvokeOperatorHelper) invokeByteArrayOperator;
    type->helpers->markVar = (EjsMarkVarHelper) markByteArrayVar;
    type->helpers->lookupProperty = (EjsLookupPropertyHelper) lookupByteArrayProperty;
    type->helpers->setProperty = (EjsSetPropertyHelper) setByteArrayProperty;
}


void ejsConfigureByteArrayType(Ejs *ejs)
{
    EjsType     *type;

    type = ejs->byteArrayType;
    
    ejsBindMethod(ejs, type, ES_ByteArray_ByteArray, (EjsNativeFunction) byteArrayConstructor);
    ejsBindMethod(ejs, type, ES_ByteArray_available, (EjsNativeFunction) availableProc);
    ejsBindMethod(ejs, type, ES_ByteArray_copyIn, (EjsNativeFunction) copyIn);
    ejsBindMethod(ejs, type, ES_ByteArray_copyOut, (EjsNativeFunction) copyOut);
    ejsBindMethod(ejs, type, ES_ByteArray_set_input, (EjsNativeFunction) inputByteArrayData);
    ejsBindMethod(ejs, type, ES_ByteArray_flush, (EjsNativeFunction) flushProc);
    ejsBindMethod(ejs, type, ES_Object_length, (EjsNativeFunction) getByteArrayLength);
    ejsBindMethod(ejs, type, ES_Object_get, (EjsNativeFunction) getByteArrayIterator);
    ejsBindMethod(ejs, type, ES_Object_getValues, (EjsNativeFunction) getByteArrayValues);
    ejsBindMethod(ejs, type, ES_ByteArray_endian, (EjsNativeFunction) endian);
    ejsBindMethod(ejs, type, ES_ByteArray_set_endian, (EjsNativeFunction) setEndian);
    ejsBindMethod(ejs, type, ES_ByteArray_set_output, (EjsNativeFunction) output);
    ejsBindMethod(ejs, type, ES_ByteArray_read, (EjsNativeFunction) byteArrayReadProc);
    ejsBindMethod(ejs, type, ES_ByteArray_readBoolean, (EjsNativeFunction) readBoolean);
    ejsBindMethod(ejs, type, ES_ByteArray_readByte, (EjsNativeFunction) readByte);
    ejsBindMethod(ejs, type, ES_ByteArray_readDate, (EjsNativeFunction) readDate);
#if BLD_FEATURE_FLOATING_POINT && ES_ByteArray_readDouble
    ejsBindMethod(ejs, type, ES_ByteArray_readDouble, (EjsNativeFunction) readDouble);
#endif
    ejsBindMethod(ejs, type, ES_ByteArray_readInteger, (EjsNativeFunction) readInteger);
    ejsBindMethod(ejs, type, ES_ByteArray_readLong, (EjsNativeFunction) readLong);
    ejsBindMethod(ejs, type, ES_ByteArray_readPosition, (EjsNativeFunction) readPosition);
    ejsBindMethod(ejs, type, ES_ByteArray_set_readPosition, (EjsNativeFunction) setReadPosition);
    ejsBindMethod(ejs, type, ES_ByteArray_readShort, (EjsNativeFunction) readShort);
    ejsBindMethod(ejs, type, ES_ByteArray_readString, (EjsNativeFunction) baReadString);
#if ES_ByteArray_readXML && BLD_FEATURE_EJS_E4X
    ejsBindMethod(ejs, type, ES_ByteArray_readXML, (EjsNativeFunction) readXML);
#endif
    ejsBindMethod(ejs, type, ES_ByteArray_reset, (EjsNativeFunction) reset);
    ejsBindMethod(ejs, type, ES_ByteArray_room, (EjsNativeFunction) roomProc);
    ejsBindMethod(ejs, type, ES_Object_toString, (EjsNativeFunction) byteArrayToString);
    ejsBindMethod(ejs, type, ES_ByteArray_write, (EjsNativeFunction) ejsWriteToByteArray);
    ejsBindMethod(ejs, type, ES_ByteArray_writeByte, (EjsNativeFunction) writeByte);
    ejsBindMethod(ejs, type, ES_ByteArray_writeShort, (EjsNativeFunction) writeShort);
    ejsBindMethod(ejs, type, ES_ByteArray_writeInteger, (EjsNativeFunction) writeInteger);
    ejsBindMethod(ejs, type, ES_ByteArray_writeLong, (EjsNativeFunction) writeLong);
#if ES_ByteArray_writeDouble && BLD_FEATURE_FLOATING_POINT
    ejsBindMethod(ejs, type, ES_ByteArray_writeDouble, (EjsNativeFunction) writeDouble);
#endif
    ejsBindMethod(ejs, type, ES_ByteArray_writePosition, (EjsNativeFunction) writePosition);
    ejsBindMethod(ejs, type, ES_ByteArray_set_writePosition, (EjsNativeFunction) setWritePosition);
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
