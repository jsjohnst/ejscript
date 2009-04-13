/**
 *  ejsObject.c - Object class
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejs.h"

/****************************** Forward Declarations **************************/

static EjsName  getObjectPropertyName(Ejs *ejs, EjsObject *obj, int slotNum);
static int      growNames(EjsObject *obj, int size);
static int      growSlots(Ejs *ejs, EjsObject *obj, int size);
static int      hashProperty(EjsObject *obj, int slotNum, EjsName *qname);
static int      lookupObjectProperty(struct Ejs *ejs, EjsObject *obj, EjsName *qname);
static int      makeHash(EjsObject *obj);
static inline int cmpName(EjsName *a, EjsName *b);
static inline int cmpQname(EjsName *a, EjsName *b);
static void     removeHashEntry(EjsObject  *obj, EjsName *qname);
static EjsVar   *objectToString(Ejs *ejs, EjsVar *vp, int argc, EjsVar **argv);

#define CMP_NAME(a,b) cmpName(a, b)
#define CMP_QNAME(a,b) cmpQname(a, b)

/************************************* Code ***********************************/
/*
 *  Cast the operand to a primitive type
 *
 *  intrinsic function cast(type: Type) : Object
 */
static EjsVar *castObject(Ejs *ejs, EjsObject *obj, EjsType *type)
{
    EjsString   *result;
    char        *buf;
    
    mprAssert(ejsIsType(type));

    switch (type->id) {
    case ES_Boolean:
        return (EjsVar*) ejsCreateBoolean(ejs, 1);

    case ES_Number:
        result = ejsToString(ejs, (EjsVar*) obj);
        if (result == 0) {
            ejsThrowMemoryError(ejs);
            return 0;
        }
        return ejsParseVar(ejs, ejsGetString(result), ES_Number);

    case ES_String:
        mprAllocSprintf(ejs, &buf, 0, "[object %s]", obj->var.type->qname.name);
        result = ejsCreateString(ejs, buf);
        mprFree(buf);
        return (EjsVar*) result;

    default:
        if (ejsIsA(ejs, (EjsVar*) obj, type)) {
            return (EjsVar*) obj;
        }
        ejsThrowTypeError(ejs, "Can't cast to this type");
        return 0;
    }
}


EjsObject *ejsCreateSimpleObject(Ejs *ejs)
{
    //  TODO - should this not be dynamic?
    return ejsCreateObjectEx(ejs, ejs->objectType, 0, 0);
}


EjsObject *ejsCreateObject(Ejs *ejs, EjsType *type, int numExtraSlots)
{
    return ejsCreateObjectEx(ejs, type, numExtraSlots, 0);
}


/*
 *  Create an object which is an instance of a given type. This is used by all scripted types to create objects. NOTE: 
 *  we only initialize the Object base class. It is up to the  caller to complete the initialization for all other base 
 *  classes by calling the appropriate constructors. capacity is the number of property slots to pre-allocate. Slots are 
 *  allocated and the property hash is configured. 
 */
EjsObject *ejsCreateObjectEx(Ejs *ejs, EjsType *type, int numExtraSlots, bool separateSlots)
{
    EjsObject   *obj;
    EjsBlock    *instanceBlock;
    int         numSlots, hashSize;

    mprAssert(type);
    mprAssert(numExtraSlots >= 0);

    instanceBlock = type->instanceBlock;
    numSlots = (instanceBlock ? instanceBlock->obj.numProp : 0) + numExtraSlots;
    mprAssert(numSlots >= 0);
    
    separateSlots |= type->separateInstanceSlots;

    /*
     *  Create the object from the variable allocator. We say how much room we need to reserve for property slots.
     */
    obj = (EjsObject*) ejsAllocVar(ejs, type, (separateSlots) ? 0 : (numSlots * sizeof(EjsVar*)));
    if (obj == 0) {
        return 0;
    }
    obj->var.isObject = 1;
    obj->var.dynamic = type->dynamicInstance;

    if (obj->var.dynamic) {
        mprAssert(separateSlots);
    }

    if (numSlots > 0) {
        if (separateSlots) {
            /*
             *  If the object requires separate slots then allocate the slots and property hash.
             */
            mprAssert(obj->numProp == 0);
            if (ejsGrowObject(ejs, obj, numSlots) < 0) {
                return 0;
            }
            mprAssert(obj->numProp == numSlots);

            if (instanceBlock && instanceBlock->obj.names) {
                
                if (instanceBlock->obj.names->sizeEntries) {
                    obj->names->sizeEntries = instanceBlock->obj.names->sizeEntries;
                    memcpy(obj->names->entries, instanceBlock->obj.names->entries, 
                        obj->names->sizeEntries * sizeof(EjsHashEntry));
                }
                if (instanceBlock->obj.names->buckets) {
                    hashSize = ejsGetHashSize(instanceBlock->obj.numProp);
                    obj->names->buckets = (int*) mprAlloc(obj->names, hashSize * sizeof(EjsHashEntry*));
                    if (obj->names->buckets == 0) {
                        return 0;
                    }
                    obj->names->sizeBuckets = instanceBlock->obj.names->sizeBuckets;
                    memcpy(obj->names->buckets, instanceBlock->obj.names->buckets, obj->names->sizeBuckets * sizeof(int*));
                }
            }
            
        } else {
            /*
             *  The slots are allocated as part of the object in a single memory chunk.
             */
            obj->slots = (EjsVar**) &(((char*) obj)[type->instanceSize]);
            ejsZeroSlots(ejs, obj->slots, numSlots);
            obj->capacity = numSlots;
            obj->numProp = numSlots;
            if (instanceBlock) {
                obj->names = instanceBlock->obj.names;
            }
        }
    }

    ejsSetFmtDebugName(obj, "obj %s", type->qname.name);

    return obj;
}


EjsObject *ejsCopyObject(Ejs *ejs, EjsObject *src, bool deep)
{
    EjsObject   *dest;
    bool        separateSlots;
    int         numProp, i;

    numProp = src->numProp;

    separateSlots = ejsIsType(src) ? ((EjsType*) src)->separateInstanceSlots: src->var.type->separateInstanceSlots;

    dest = ejsCreateObjectEx(ejs, src->var.type, numProp, separateSlots);
    if (dest == 0) {
        return 0;
    }
    
    /*
     *  Copy var flags but preserve generation. Don't copy rootLinks.
     *  TODO OPT - bit fields
     */
    dest->var.refLinks = src->var.refLinks;
    dest->var.builtin = src->var.builtin;
    dest->var.dynamic = src->var.dynamic;
    dest->var.hasGetterSetter = src->var.hasGetterSetter;
    dest->var.isFunction = src->var.isFunction;
    dest->var.isObject = src->var.isObject;
    dest->var.isInstanceBlock = src->var.isInstanceBlock;
    dest->var.isType = src->var.isType;
    dest->var.isFrame = src->var.isFrame;
    dest->var.hidden = src->var.hidden;
    dest->var.marked = src->var.marked;
    dest->var.native = src->var.native;
    dest->var.nativeProc = src->var.nativeProc;
    dest->var.permanent = src->var.permanent;
    dest->var.survived = src->var.survived;
    dest->var.visited = src->var.visited;

    
    ejsSetDebugName(dest, src->var.debugName);

    if (numProp <= 0) {
        return dest;
    }

    for (i = 0; i < numProp; i++) {
        if (deep) {
            dest->slots[i] = ejsCloneVar(ejs, src->slots[i], deep);
        } else {
            dest->slots[i] = src->slots[i];
        }
        ejsSetReference(ejs, (EjsVar*) dest, dest->slots[i]);
    }

    if (separateSlots) {
        if (dest->names == NULL && growNames(dest, numProp) < 0) {
            return 0;
        }

        for (i = 0; i < numProp; i++) {
            dest->names->entries[i] = src->names->entries[i];
        }
        if (makeHash(dest) < 0) {
            return 0;
        }

    } else {
        dest->names = src->names;
    }

    return dest;
}


/*
 *  Define a new property.
 */
static int defineObjectProperty(Ejs *ejs, EjsBlock *block, int slotNum, EjsName *qname, EjsType *propType, int attributes, 
    EjsVar *value)
{

    if (ejsIsBlock(block)) {
        return (ejs->blockHelpers->defineProperty)(ejs, (EjsVar*) block, slotNum, qname, propType, attributes, value);

    } else {
        ejsThrowInternalError(ejs, "Helper not defined for non-block object");
        return 0;
    }
}


/*
 *  Delete an instance property. To delete class properties, use the type as the obj.
 */
static int deleteObjectProperty(Ejs *ejs, EjsObject *obj, int slotNum)
{
    EjsName     qname;

    mprAssert(obj);
    mprAssert(obj->var.type);
    mprAssert(slotNum >= 0);

    if (!obj->var.dynamic && !(ejs->flags & EJS_FLAG_COMPILER)) {
        ejsThrowTypeError(ejs, "Can't delete properties in a non-dynamic object");
        return EJS_ERR;
    }

    if (slotNum < 0 || slotNum >= obj->numProp) {
        ejsThrowReferenceError(ejs, "Invalid property slot to delete");
        return EJS_ERR;
    }

    qname = getObjectPropertyName(ejs, obj, slotNum);
    if (qname.name == 0) {
        return EJS_ERR;
    }

    removeHashEntry(obj, &qname);
    obj->slots[slotNum] = 0;

    return 0;
}


/*
 *  Delete an instance property by name
 */
static int deleteObjectPropertyByName(Ejs *ejs, EjsObject *obj, EjsName *qname)
{
    int     slotNum;

    slotNum = lookupObjectProperty(ejs, obj, qname);
    if (slotNum < 0) {
        ejsThrowReferenceError(ejs, "Property does not exist");
        return EJS_ERR;
    } else {
        return deleteObjectProperty(ejs, obj, slotNum);
    }
}


/*
 *  Object is being destroy. May be added to the type free list for recycling, so preserve the hash for non-dynamic objects.
 */
static void destroyObject(Ejs *ejs, EjsObject *obj)
{
    mprAssert(obj);

    if (obj->var.dynamic) {
        mprFree(obj->slots);
        obj->slots = 0;
        mprFree(obj->names);
        obj->names = 0;
    }
    
    ejsFreeVar(ejs, (EjsVar*) obj);
}


/**
 *  Finalizer hook for types. This should call a "finalize" method if one exists to release any internally allocated 
 *  resources. This routine will be called by the GC if it has not been previously called explicitly by the user. 
 *  So release is guaranteed to be called exactly and only once. For objects, release will remove all properties. 
 *  If called by the user, this call is a hint to GC that the object may be available for immmediate collection. 
 *  Not yet implemented.
 */
static int finalizeObject(Ejs *ejs, EjsObject *obj)
{
    //  TODO - Objects need an ability to define a user finalize() method.
    return 0;
}


static EjsVar *getObjectProperty(Ejs *ejs, EjsObject *obj, int slotNum)
{
    mprAssert(obj);
    mprAssert(obj->slots);
    mprAssert(slotNum >= 0);

    if (slotNum < 0 || slotNum >= obj->numProp) {
        ejsThrowReferenceError(ejs, "Property at slot \"%d\" is not found", slotNum);
        return 0;
    }
    return obj->slots[slotNum];
}


/*
 *  Return the number of properties in the object
 */
static int getObjectPropertyCount(Ejs *ejs, EjsObject *obj)
{
    mprAssert(obj);
    mprAssert(ejsIsObject(obj));

    return obj->numProp;
}


static EjsName getObjectPropertyName(Ejs *ejs, EjsObject *obj, int slotNum)
{
    EjsName     qname;

    mprAssert(obj);
    mprAssert(ejsIsObject(obj));
    mprAssert(obj->slots);
    mprAssert(slotNum >= 0);
    mprAssert(slotNum < obj->numProp);

    if (slotNum < 0 || slotNum >= obj->numProp) {
        qname.name = 0;
        qname.space = 0;
        return qname;
    }
    return obj->names->entries[slotNum].qname;
}


/*
 *  Cast the operands depending on the operation code
 */
EjsVar *ejsCoerceOperands(Ejs *ejs, EjsVar *lhs, int opcode, EjsVar *rhs)
{
    switch (opcode) {

    /*
     *  Binary operators
     */
    case EJS_OP_ADD:
        return ejsInvokeOperator(ejs, (EjsVar*) ejsToString(ejs, lhs), opcode, rhs);

    case EJS_OP_AND: case EJS_OP_DIV: case EJS_OP_MUL: case EJS_OP_OR: case EJS_OP_REM:
    case EJS_OP_SHL: case EJS_OP_SHR: case EJS_OP_SUB: case EJS_OP_USHR: case EJS_OP_XOR:
        return ejsInvokeOperator(ejs, (EjsVar*) ejs->zeroValue, opcode, rhs);

    case EJS_OP_COMPARE_EQ:  case EJS_OP_COMPARE_NE:
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
    case EJS_OP_LOGICAL_NOT: case EJS_OP_NOT:
        return 0;

    default:
        ejsThrowTypeError(ejs, "Opcode %d not valid for type %s", opcode, lhs->type->qname.name);
        return ejs->undefinedValue;
    }
    return 0;
}


EjsVar *ejsObjectOperator(Ejs *ejs, EjsVar *lhs, int opcode, EjsVar *rhs)
{
    EjsVar      *result;

    if (rhs == 0 || lhs->type != rhs->type) {
        if ((result = ejsCoerceOperands(ejs, lhs, opcode, rhs)) != 0) {
            return result;
        }
    }

    /*
     *  Types now match
     */
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
    case EJS_OP_ADD: case EJS_OP_AND: case EJS_OP_DIV: case EJS_OP_MUL:
    case EJS_OP_REM: case EJS_OP_OR: case EJS_OP_SHL: case EJS_OP_SHR:
    case EJS_OP_SUB: case EJS_OP_USHR: case EJS_OP_XOR:
        return ejsInvokeOperator(ejs, (EjsVar*) ejsToNumber(ejs, lhs), opcode, (EjsVar*) ejsToNumber(ejs, rhs));

    default:
        ejsThrowTypeError(ejs, "Opcode %d not implemented for type %s", opcode, lhs->type->qname.name);
        return 0;
    }

    mprAssert(0);
}


/*
 *  Lookup a property with a namespace qualifier in an object and return the slot if found. Return EJS_ERR if not found.
 */
static int lookupObjectProperty(struct Ejs *ejs, EjsObject *obj, EjsName *qname)
{
    EjsNames    *names;
    EjsName     *propName;
    int         slotNum, index;

    mprAssert(qname);
    mprAssert(qname->name);
    mprAssert(qname->space);

    names = obj->names;

    if (names == 0) {
        return EJS_ERR;
    }

    if (names->buckets == 0) {
        /*
         *  No hash. Just do a linear search
         */
        for (slotNum = 0; slotNum < obj->numProp; slotNum++) {
            propName = &names->entries[slotNum].qname;
            if (CMP_QNAME(propName, qname)) {
                return slotNum;
            }
        }
        return EJS_ERR;
    }
    
    /*
     *  Find the property in the hash chain if it exists. Note the hash does not include the namespace portion.
     *  We assume that names rarely clash with different namespaces. We do this so variable lookup and do a one
     *  hash probe and find matching names. Lookup will then pick the right namespace.
     */
    index = ejsComputeHashCode(names, qname);

    for (slotNum = names->buckets[index]; slotNum >= 0;  slotNum = names->entries[slotNum].nextSlot) {
        propName = &names->entries[slotNum].qname;
        /*
         *  Compare the name including the namespace portion
         */
        if (CMP_QNAME(propName, qname)) {
            return slotNum;
        }
    }

    return EJS_ERR;
}


/*
 *  Lookup a qualified property name and count the number of name portion matches. This routine is used to quickly lookup a 
 *  qualified name AND determine if there are other names with different namespaces but having the same name portion.
 *  Returns EJS_ERR if more than one matching property is found (ie. two properties of the same name but with different 
 *  namespaces). This should be rare! Otherwise, return the slot number of the unique matching property.
 *
 *  This is a special lookup routine for fast varible lookup in the scope chain. Used by ejsLookupVar and ejsLookupScope.
 *  WARNING: updates qname->space
 */
int ejsLookupSingleProperty(Ejs *ejs, EjsObject *obj, EjsName *qname)
{
    EjsNames    *names;
    EjsName     *propName;
    int         i, slotNum, index, count;

    mprAssert(qname);
    mprAssert(qname->name);
    mprAssert(qname->space);
    mprAssert(qname->space[0] == '\0');

    names = obj->names;
    slotNum = -1;
    count = 0;

    if (names) {
        if (names->buckets == 0) {
            /*
             *  No hash. Just do a linear search. Examine all properties.
             */
            for (i = 0; i < obj->numProp; i++) {
                propName = &names->entries[i].qname;
                if (CMP_NAME(propName, qname)) {
                    count++;
                    slotNum = i;
                }
            }

        } else {
            /*
             *  Find the property in the hash chain if it exists. Note the hash does NOT include the namespace portion.
             *  We assume that names rarely clash with different namespaces. We do this so variable lookup and a single hash 
             *  probe will find matching names.
             */
            index = ejsComputeHashCode(names, qname);

            for (i = names->buckets[index]; i >= 0;  i = names->entries[i].nextSlot) {
                propName = &names->entries[i].qname;
                if (CMP_NAME(propName, qname)) {
                    slotNum = i;
                    count++;
                }
            }
        }
        if (count == 1) {
            if (mprLookupHash(ejs->standardSpaces, names->entries[slotNum].qname.space)) {
                qname->space = names->entries[slotNum].qname.space;
            } else {
                slotNum = -2;
            }
        }
    }

    return (count <= 1) ? slotNum : -2;
}


/*
 *  Mark the object properties for the garbage collector
 */
void ejsMarkObject(Ejs *ejs, EjsVar *parent, EjsObject *obj)
{
    EjsType     *type;
    EjsVar      *vp;
    int         i;

    mprAssert(ejsIsObject(obj) || ejsIsBlock(obj) || ejsIsFunction(obj));

    type = obj->var.type;

    for (i = 0; i < obj->numProp; i++) {
        vp = obj->slots[i];
        if (vp == 0 || vp == ejs->nullValue || vp->generation == EJS_GEN_ETERNAL) {
            continue;
        }
        ejsMarkVar(ejs, (EjsVar*) obj, vp);
    }
}


/*
 *  Validate the supplied slot number. If set to -1, then return the next available property slot number.
 */
inline int ejsCheckObjSlot(Ejs *ejs, EjsObject *obj, int slotNum)
{
    if (slotNum < 0) {
        if (!obj->var.dynamic) {
            ejsThrowReferenceError(ejs, "Object is not dynamic");
            return EJS_ERR;
        }

        slotNum = obj->numProp;
        if (obj->numProp >= obj->capacity) {
            if (ejsGrowObject(ejs, obj, obj->numProp + 1) < 0) {
                ejsThrowMemoryError(ejs);
                return EJS_ERR;
            }
        } else {
            obj->numProp++;
        }

    } else if (slotNum >= obj->numProp) {
        if (ejsGrowObject(ejs, obj, slotNum + 1) < 0) {
            ejsThrowMemoryError(ejs);
            return EJS_ERR;
        }
    }
    return slotNum;
}


/**
 *  Set the value of a property.
 *  @param slot If slot is -1, then allocate the next free slot
 *  @return Return the property slot if successful. Return < 0 otherwise.
 */
static int setObjectProperty(Ejs *ejs, EjsObject *obj, int slotNum, EjsVar *value)
{
    mprAssert(ejs);
    mprAssert(obj);
    
    if ((slotNum = ejsCheckObjSlot(ejs, obj, slotNum)) < 0) {
        return EJS_ERR;
    }
    
    mprAssert(slotNum < obj->numProp);
    mprAssert(obj->numProp <= obj->capacity);
    
    obj->slots[slotNum] = value;
    ejsSetReference(ejs, (EjsVar*) obj, value);

    return slotNum;
}


/*
 *  Set the name for a property. Objects maintain a hash lookup for property names. This is hash is created on demand 
 *  if there are more than N properties. If an object is not dynamic, it will use the types name hash. If dynamic, 
 *  then the types name hash will be copied when required. Callers must supply persistent names strings in qname. 
 */
static int setObjectPropertyName(Ejs *ejs, EjsObject *obj, int slotNum, EjsName *qname)
{
    EjsNames    *names;

    mprAssert(obj);
    mprAssert(qname);
    mprAssert(slotNum >= 0);

    if ((slotNum = ejsCheckObjSlot(ejs, obj, slotNum)) < 0) {
        return EJS_ERR;
    }

    /*
     *  If the hash is owned by the base type and this is a dynamic object, we need a new hash dedicated to the object.
     */
    if (obj->names == NULL) {
        if (growNames(obj, slotNum + 1) < 0) {
            return EJS_ERR;
        }

    } else if (obj->var.dynamic && obj->names == obj->var.type->block.obj.names) {
        /*
         *  Object is using the type's original names, must copy and use own names from here on.
         */
        obj->names = NULL;
        if (growNames(obj, slotNum + 1) < 0) {
            return EJS_ERR;
        }

//  TODO - BUG - names may not be a context
    } else if (!ejsIsType(obj) && obj != mprGetParent(obj->names)) {
        /*
         *  This case occurs when a dynamic local var is created in a function frame.
         */
        if (growNames(obj, slotNum + 1) < 0) {
            return EJS_ERR;
        }
        
    } else if (slotNum >= obj->names->sizeEntries) {
        if (growNames(obj, slotNum + 1) < 0) {
            return EJS_ERR;
        }
    }

    names = obj->names;

    /*
     *  Remove the old hash entry if the name will change
     */
    if (names->entries[slotNum].nextSlot >= 0) {
        if (CMP_QNAME(&names->entries[slotNum].qname, qname)) {
            return slotNum;
        }
        removeHashEntry(obj, &names->entries[slotNum].qname);
    }

    /*
     *  Set the property name
     */
    names->entries[slotNum].qname = *qname;
    
    mprAssert(slotNum < obj->numProp);
    mprAssert(obj->numProp <= obj->capacity);
    
    if (obj->numProp <= EJS_HASH_MIN_PROP || qname->name == NULL) {
        return slotNum;
    }

    if (hashProperty(obj, slotNum, qname) < 0) {
        ejsThrowMemoryError(ejs);
        return EJS_ERR;
    }

    return slotNum;
}


/*
 *  Return a memory context that should be used for any dynamically allocated data. Using this memory context will ensure
 *  the memory is automatically freed and all destructors are called when the object is freed. If the object is not dynamic, then
 *  a context cannot be supplied and the caller will have to use the destroyVar helper and explicitly release memory.
 */
MprCtx ejsGetContext(EjsObject *obj)
{
    if (!obj->var.dynamic && obj->slots) {
        return obj->slots;
    }
    return 0;
}


//  TODO - rename MakePropertyDeletable
void ejsMakePropertyDontDelete(EjsVar *vp, int dontDelete)
{
#if FUTURE
    vp->preventDelete = dontDelete;
#endif
}


/*
 *  Set a property's enumerability by for/in. Return true if the property was enumerable.
 */

int ejsMakePropertyEnumerable(EjsVar *vp, bool enumerate)
{
    int     oldValue;

    oldValue = vp->hidden;
    vp->hidden = !enumerate;
    return oldValue;
}


#if FUTURE
/*
 *  Make a variable read only. Can still be deleted.
 */
void ejsMakePropertyReadOnly(EjsVar *vp, int readonly)
{
    vp->readonly = readonly;
}
#endif


void ejsSetAllocIncrement(Ejs *ejs, EjsType *type, int num)
{
    if (type == 0) {
        return;
    }
    type->numAlloc = num;
}


/******************************* Slot Routines ********************************/
/*
 *  Grow the slot storage for the object and increase numProp
 */
int ejsGrowObject(Ejs *ejs, EjsObject *obj, int count)
{
    int     size;
    
    if (count <= 0) {
        return 0;
    }

    if (obj->capacity < count) {
        size = EJS_PROP_ROUNDUP(count);

        if (growNames(obj, size) < 0) {
            return EJS_ERR;
        }
        if (growSlots(ejs, obj, size) < 0) {
            return EJS_ERR;
        }
        if (obj->numProp > 0 && makeHash(obj) < 0) {
            return EJS_ERR;
        }
    }

    if (count > obj->numProp) {
        obj->numProp = count;
    }
    
    mprAssert(count <= obj->capacity);
    mprAssert(obj->numProp <= obj->capacity);
    
    return 0;
}


/*
 *  Insert new slots at the specified offset and move up slots to make room. Increase numProp.
 */
int ejsInsertGrowObject(Ejs *ejs, EjsObject *obj, int incr, int offset)
{
    EjsHashEntry    *entries;
    EjsNames        *names;
    int             i, size, mark;

    mprAssert(obj);
    mprAssert(incr >= 0);

    if (incr == 0) {
        return 0;
    }
    
    /*
     *  Base this comparison on numProp and not on capacity as we may already have room to fit the inserted properties.
     */
    size = obj->numProp + incr;

    if (obj->capacity < size) {
        size = EJS_PROP_ROUNDUP(size);
        if (growNames(obj, size) < 0) {
            return EJS_ERR;
        }
        if (growSlots(ejs, obj, size) < 0) {
            return EJS_ERR;
        }
    }
    obj->numProp += incr;
    
    mprAssert(obj->capacity == obj->names->sizeEntries);
    mprAssert(obj->numProp <= obj->capacity);
    
    names = obj->names;
    mark = offset + incr;
    for (i = obj->numProp - 1; i >= mark; i--) {
        obj->slots[i] = obj->slots[i - mark];
        names->entries[i] = names->entries[i - mark];
    }

    ejsZeroSlots(ejs, &obj->slots[offset], incr);

    entries = names->entries;
    for (i = offset; i < mark; i++) {
        entries[i].nextSlot = -1;
        entries[i].qname.name = "";
        entries[i].qname.space = "";
    }

    if (makeHash(obj) < 0) {
        return EJS_ERR;
    }   
    
    return 0;
}


/*
 *  Allocate or grow the slots storage for an object
 */
static int growSlots(Ejs *ejs, EjsObject *obj, int capacity)
{
    int         factor;

    mprAssert(obj);

    if (capacity <= obj->capacity) {
        return 0;
    }

    /*
     *  Allocate or grow the slots structures
     */
    if (capacity > obj->capacity) {
        if (obj->capacity > EJS_LOTSA_PROP) {
            /*
             *  Looks like a big object so grow by a bigger chunk. TODO - should we also grow names?
             */
            factor = max(obj->capacity / 4, EJS_NUM_PROP);
            capacity = (capacity + factor) / factor * factor;
        }
        capacity = EJS_PROP_ROUNDUP(capacity);

        if (obj->slots == 0) {
            mprAssert(obj->capacity == 0);
            mprAssert(capacity > 0);
            obj->slots = (EjsVar**) mprAlloc(obj, sizeof(EjsVar*) * capacity);
            if (obj->slots == 0) {
                return EJS_ERR;
            }
            ejsZeroSlots(ejs, obj->slots, capacity);

        } else {
            mprAssert(obj->capacity > 0);
            obj->slots = (EjsVar**) mprRealloc(obj, obj->slots, sizeof(EjsVar*) * capacity);
            if (obj->slots == 0) {
                return EJS_ERR;
            }
            ejsZeroSlots(ejs, &obj->slots[obj->capacity], (capacity - obj->capacity));
        }
        obj->capacity = capacity;
    }

    return 0;
}


/*
 *  Remove a slot and name. Copy up all other properties. WARNING: this can only be used before property binding and 
 *  should only be used by the compiler.
 */
void ejsRemoveSlot(Ejs *ejs, EjsObject *obj, int slotNum, int compact)
{
    EjsNames    *names;
    int         i;

    mprAssert(obj);
    mprAssert(slotNum >= 0);
    mprAssert(slotNum >= 0);
    mprAssert(ejs->flags & EJS_FLAG_COMPILER);

    names = obj->names;

    if (compact) {
        mprAssert(names);

        for (i = slotNum + 1; i < obj->numProp; i++) {
            obj->slots[i - 1] = obj->slots[i];
            names->entries[i - 1] = names->entries[i];
        }
        obj->numProp--;
        i--;

    } else {
        i = slotNum;
    }

    obj->slots[i] = 0;
    names->entries[i].qname.name = "";
    names->entries[i].qname.space = "";
    names->entries[i].nextSlot = -1;
    
    makeHash(obj);
}


/******************************* Hash Routines ********************************/

/*
 *  Exponential primes
 */
static int hashSizes[] = {
     19, 29, 59, 79, 97, 193, 389, 769, 1543, 3079, 6151, 12289, 24593, 49157, 98317, 196613, 0
};


int ejsGetHashSize(int numProp)
{
    int     i;

    for (i = 0; i < hashSizes[i]; i++) {
        if (numProp < hashSizes[i]) {
            return hashSizes[i];
        }
    }
    return hashSizes[i - 1];
}


/*
 *  Grow the names vector
 */
static int growNames(EjsObject *obj, int size)
{
    EjsNames        *names;
    EjsHashEntry    *entries;
    bool            ownNames;
    int             i, oldSize;

    if (size == 0) {
        return 0;
    }

    names = obj->names;
    ownNames = (obj == mprGetParent(names));
    oldSize = (names) ? names->sizeEntries: 0;

    if (names == NULL || !ownNames) {
        names = mprAllocObj(obj, EjsNames);
        if (names == 0) {
            return EJS_ERR;
        }
        names->buckets = 0;
        names->entries = 0;
        names->sizeEntries = 0;
        names->sizeBuckets = 0;
    }

    if (size < names->sizeEntries) {
        return 0;
    }
    size = EJS_PROP_ROUNDUP(size);
    
    if (ownNames) {
        entries = (EjsHashEntry*) mprRealloc(names, names->entries, sizeof(EjsHashEntry) * size);
        if (entries == 0) {
            return EJS_ERR;
        }

    } else {
        entries = (EjsHashEntry*) mprAlloc(names, sizeof(EjsHashEntry) * size);
        if (entries == 0) {
            return EJS_ERR;
        }
        if (obj->names) {
            for (i = 0; i < oldSize; i++) {
                entries[i] = obj->names->entries[i];
            }
        }
    }

    for (i = oldSize; i < size; i++) {
        entries[i].nextSlot = -1;
        entries[i].qname.name = "";
        entries[i].qname.space = "";
    }
                
    names->sizeEntries = size;
    names->entries = entries;
    obj->names = names;

    return 0;
}


static int hashProperty(EjsObject *obj, int slotNum, EjsName *qname)
{
    EjsNames    *names;
    EjsName     *slotName;
    int         chainSlotNum, lastSlot, index;

    mprAssert(qname);

    names = obj->names;
    mprAssert(names);
  
    /*
     *  Test if the number of hash buckets is too small or non-existant and re-make the hash.
     */
    if (names->sizeBuckets < obj->numProp) {
        return makeHash(obj);
    }

    index = ejsComputeHashCode(names, qname);

    /*
     *  Scan the collision chain
     */
    lastSlot = -1;
    chainSlotNum = names->buckets[index];
    mprAssert(chainSlotNum < obj->numProp);
    mprAssert(chainSlotNum < obj->capacity);

    while (chainSlotNum >= 0) {
        slotName = &names->entries[chainSlotNum].qname;
        if (CMP_QNAME(slotName, qname)) {
            return 0;
        }
        mprAssert(lastSlot != chainSlotNum);
        lastSlot = chainSlotNum;
        mprAssert(chainSlotNum != names->entries[chainSlotNum].nextSlot);
        chainSlotNum = names->entries[chainSlotNum].nextSlot;

        mprAssert(0 <= lastSlot && lastSlot < obj->numProp);
        mprAssert(0 <= lastSlot && lastSlot < obj->capacity);
    }

    if (lastSlot >= 0) {
        mprAssert(lastSlot < obj->numProp);
        mprAssert(lastSlot != slotNum);
        names->entries[lastSlot].nextSlot = slotNum;

    } else {
        /* Start a new hash chain */
        names->buckets[index] = slotNum;
    }

    names->entries[slotNum].nextSlot = -2;
    names->entries[slotNum].qname = *qname;

#if BLD_DEBUG
    if (obj->slots[slotNum] && obj->slots[slotNum]->debugName[0] == '\0') {
        ejsSetDebugName(obj->slots[slotNum], qname->name);
    }
#endif

    return 0;
}


/*
 *  Allocate or grow the properties storage for an object. This routine will also manage the hash index for the object. 
 *  If numInstanceProp is < 0, then grow the number of properties by an increment. Otherwise, set the number of properties 
 *  to numInstanceProp. We currently don't allow reductions.
 */
static int makeHash(EjsObject *obj)
{
    EjsHashEntry    *entries;
    EjsNames        *names;
    int             i, newHashSize;

    mprAssert(obj);

    names = obj->names;

    /*
     *  Don't make the hash if too few properties. Once we have a hash, keep using it even if we have too few properties now.
     */
    if (obj->numProp <= EJS_HASH_MIN_PROP && names->buckets == 0) {
        return 0;
    }

    /*
     *  Only reallocate the hash buckets if the hash needs to grow larger
     */
    newHashSize = ejsGetHashSize(obj->numProp);
    if (names->sizeBuckets < newHashSize) {
        mprFree(names->buckets);
        names->buckets = (int*) mprAlloc(names, newHashSize * sizeof(int));
        if (names->buckets == 0) {
            return EJS_ERR;
        }
        names->sizeBuckets = newHashSize;
    }
    mprAssert(names->buckets);

    /*
     *  Clear out hash linkage
     */
    memset(names->buckets, -1, names->sizeBuckets * sizeof(int));
    entries = names->entries;
    for (i = 0; i < names->sizeEntries; i++) {
        entries[i].nextSlot = -1;
    }

    /*
     *  Rehash all existing properties
     */
    for (i = 0; i < obj->numProp; i++) {
        if (entries[i].qname.name && hashProperty(obj, i, &entries[i].qname) < 0) {
            return EJS_ERR;
        }
    }

    return 0;
}


static void removeHashEntry(EjsObject *obj, EjsName *qname)
{
    EjsNames        *names;
    EjsHashEntry    *he;
    EjsName         *nextName;
    int             index, slotNum, lastSlot;

    names = obj->names;
    if (names == 0) {
        return;
    }

    if (names->buckets == 0) {
        /*
         *  No hash. Just do a linear search
         */
        for (slotNum = 0; slotNum < obj->numProp; slotNum++) {
            he = &names->entries[slotNum];
            if (CMP_QNAME(&he->qname, qname)) {
                he->qname.name = "";
                he->qname.space = "";
                he->nextSlot = -1;
                return;
            }
        }
        mprAssert(0);
        return;
    }


    index = ejsComputeHashCode(names, qname);
    slotNum = names->buckets[index];
    lastSlot = -1;
    while (slotNum >= 0) {
        he = &names->entries[slotNum];
        nextName = &he->qname;
        if (CMP_QNAME(nextName, qname)) {
            if (lastSlot >= 0) {
                names->entries[lastSlot].nextSlot = names->entries[slotNum].nextSlot;
            } else {
                names->buckets[index] = names->entries[slotNum].nextSlot;
            }
            he->qname.name = "";
            he->qname.space = "";
            he->nextSlot = -1;
            return;
        }
        lastSlot = slotNum;
        slotNum = names->entries[slotNum].nextSlot;
    }
    mprAssert(0);
}


int ejsRebuildHash(Ejs *ejs, EjsObject *obj)
{
    return makeHash(obj);
}


/*
 *  Compute a property name hash. Based on work by Paul Hsieh.
 */
int ejsComputeHashCode(EjsNames *names, EjsName *qname)
{
    ushort  *data;
    uchar   *cdata;
    uint    len, hash, rem, tmp;

    mprAssert(names);
    mprAssert(qname);
    mprAssert(qname->name);

    data = (ushort*) qname->name;
    len = (int) strlen(qname->name);

    if (len == 0) {
        return 0;
    }

    /*
     *  Do 32-bit wide computation
     */
    rem = len & 3;
    hash = len;

    for (len >>= 2; len > 0; len--, data += 2) {
        hash  += *data;
        tmp   =  (data[1] << 11) ^ hash;
        hash  =  (hash << 16) ^ tmp;
        hash  += hash >> 11;
    }

    /* Handle end cases */
    cdata = (uchar*) data;
    switch (rem) {
    case 3: 
        hash += cdata[0] + (cdata[1] << 8);
        hash ^= hash << 16;
        hash ^= cdata[sizeof(ushort)] << 18;
        hash += hash >> 11;
        break;
    case 2: 
        hash += cdata[0] + (cdata[1] << 8);
        hash ^= hash << 11;
        hash += hash >> 17;
        break;
    case 1: hash += cdata[0];
        hash ^= hash << 10;
        hash += hash >> 1;
    }

    /* 
     *  Force "avalanching" of final 127 bits 
     */
    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    mprAssert(names->sizeBuckets);
    
    return hash % names->sizeBuckets;
}


static inline int cmpQname(EjsName *a, EjsName *b) 
{
    mprAssert(a);
    mprAssert(b);
    mprAssert(a->name);
    mprAssert(a->space);
    mprAssert(b->name);
    mprAssert(b->space);

    if (a->name == b->name && a->space == b->space) {
        return 1;
    }
    if (a->name[0] == b->name[0] && strcmp(a->name, b->name) == 0) {
        if (a->space[0] == b->space[0] && strcmp(a->space, b->space) == 0) {
            return 1;
        }
    }
    return 0;
}


static inline int cmpName(EjsName *a, EjsName *b) 
{
    mprAssert(a);
    mprAssert(b);
    mprAssert(a->name);
    mprAssert(b->name);

    if (a->name == b->name) {
        return 1;
    }
    if (a->name[0] == b->name[0] && strcmp(a->name, b->name) == 0) {
        return 1;
    }
    return 0;
}


/*********************************** Methods **********************************/
/*
 *  WARNING: All methods here may be invoked by Native classes who are based on EjsVar and not on EjsObject. Because 
 *  all classes subclass Object, they need to be able to use these methods. So these methods must use the generic helper 
 *  interface (see ejsVar.h). They MUST NOT use EjsObject internals.
 */

static EjsVar *cloneObjectMethod(Ejs *ejs, EjsVar *op, int argc, EjsVar **argv)
{
    bool    deep;

    deep = (argc == 1 && argv[0] == (EjsVar*) ejs->trueValue);

    return ejsCloneVar(ejs, op, deep);
}


/*
 *  Function to iterate and return the next element name.
 *  NOTE: this is not a method of Object. Rather, it is a callback function for Iterator.
 */
static EjsVar *nextObjectKey(Ejs *ejs, EjsIterator *ip, int argc, EjsVar **argv)
{
    EjsObject   *obj;
    EjsName     qname;

    obj = (EjsObject*) ip->target;
    if (!ejsIsObject(obj)) {
        ejsThrowReferenceError(ejs, "Wrong type");
        return 0;
    }

    for (; ip->index < obj->numProp; ip->index++) {
        qname = ejsGetPropertyName(ejs, (EjsVar*) obj, ip->index);
        if (qname.name == 0) {
            continue;
        }
        /*
         *  Enumerate over properties that have a public public or "" namespace 
         */
        if (qname.space[0] && strcmp(qname.space, EJS_PUBLIC_NAMESPACE) != 0) {
            continue;
        }
        ip->index++;
        //  TODO - what about returning a Name?
        return (EjsVar*) ejsCreateString(ejs, qname.name);
    }
    ejsThrowStopIteration(ejs);
    return 0;
}


/*
 *  Return the default iterator.
 *
 *  iterator native function get(deep: Boolean = false, namespaces: Array = null): Iterator
 */
static EjsVar *getObjectIterator(Ejs *ejs, EjsVar *op, int argc, EjsVar **argv)
{
    EjsVar      *namespaces;
    bool        deep;

    deep = (argc == 1) ? ejsGetBoolean(argv[0]): 0;
    namespaces =  (argc == 2) ? argv[1]: 0;

    return (EjsVar*) ejsCreateIterator(ejs, op, (EjsNativeFunction) nextObjectKey, deep, (EjsArray*) namespaces);
}


/*
 *  Function to iterate and return the next element value.
 *  NOTE: this is not a method of Object. Rather, it is a callback function for Iterator
 */
static EjsVar *nextObjectValue(Ejs *ejs, EjsIterator *ip, int argc, EjsVar **argv)
{
    EjsObject   *obj;
    EjsVar      *vp;
    EjsName     qname;

    obj = (EjsObject*) ip->target;
    if (!ejsIsObject(obj)) {
        ejsThrowReferenceError(ejs, "Wrong type");
        return 0;
    }

    for (; ip->index < obj->numProp; ip->index++) {
        qname = ejsGetPropertyName(ejs, (EjsVar*) obj, ip->index);
        if (qname.name == 0) {
            continue;
        }
        if (qname.space[0] && strcmp(qname.space, EJS_PUBLIC_NAMESPACE) != 0) {
            continue;
        }
        vp = obj->slots[ip->index];
        if (vp) {
            ip->index++;
            return vp;
        }
    }
    ejsThrowStopIteration(ejs);
    return 0;
}


/*
 *  Return an iterator to return the next array element value.
 *
 *  iterator native function getValues(deep: Boolean = false, namespaces: Array = null): Iterator
 */
static EjsVar *getObjectValues(Ejs *ejs, EjsVar *op, int argc, EjsVar **argv)
{
    EjsVar      *namespaces;
    bool        deep;

    deep = (argc == 1) ? ejsGetBoolean(argv[0]): 0;
    namespaces =  (argc == 2) ? argv[1]: 0;

    return (EjsVar*) ejsCreateIterator(ejs, op, (EjsNativeFunction) nextObjectValue, deep, (EjsArray*) namespaces);
}


/*
 *  Get the length for the object.
 *
 *  intrinsic function get length(): Number
 */
static EjsVar *getObjectLength(Ejs *ejs, EjsVar *vp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, ejsGetPropertyCount(ejs, vp));
}


#if ES_Object_propertyIsEnumerable && FUTURE
/**
 *  Test and optionally set the enumerability flag for a property.
 *  intrinsic native function propertyIsEnumerable(property: String, flag: Object = undefined): Boolean
 */
static EjsVar *propertyIsEnumerable(Ejs *ejs, EjsVar *vp, int argc, EjsVar **argv)
{
    EjsVar      *pp;

    mprAssert(argc == 1 || argc == 2);
    pp = getObjectProperty(ejs, vp, argv[0]);
    if (pp == 0) {
        return ejs->falseValue;
    }

    if (argc == 2) {
        if (ejsIsBoolean(argv[1])) {
            pp->hidden = (((EjsBoolean*) argv[1])->value);
        }
    }

    if (pp->hidden) {
        return ejs->falseValue;
    }
    return ejs->trueValue;
}
#endif


#if ES_Object_seal
/**
 *  Seal a dynamic object. Once an object is sealed, further attempts to create or delete properties will fail and will throw 
 *  @spec ejs-11
 */
static EjsVar *seal(Ejs *ejs, EjsVar *vp, int argc, EjsVar **argv)
{
    //  TODO - not implemented or used anywhere
    vp->sealed = 1;
    return 0;
}
#endif


#if ES_Object_toLocaleString
/*
 *  Convert the object to a localized string.
 *
 *  intrinsic function toLocaleString(): String
 *
 *  TODO - currently just calls toString
 */
static EjsVar *toLocaleString(Ejs *ejs, EjsVar *vp, int argc, EjsVar **argv)
{
    return objectToString(ejs, vp, argc, argv);
}
#endif


/*
 *  Convert the object to a string.
 *
 *  intrinsic function toString(): String
 */
static EjsVar *objectToString(Ejs *ejs, EjsVar *vp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsToString(ejs, vp);
}


/************************************ Factory *********************************/
/*
 *  Create the object type
 */
void ejsCreateObjectType(Ejs *ejs)
{
    EjsName     qname;

    /*
     *  As instances based on pure "Object" are dynamic, we store the slots separately. So we create
     *  instances of size == sizeof(EjsObject).
     */
    ejs->objectType = ejsCreateCoreType(ejs, ejsName(&qname, EJS_INTRINSIC_NAMESPACE, "Object"), 0, sizeof(EjsObject), 
        ES_Object, ES_Object_NUM_CLASS_PROP, ES_Object_NUM_INSTANCE_PROP, 
        EJS_ATTR_NATIVE | EJS_ATTR_OBJECT | EJS_ATTR_DYNAMIC_INSTANCE | EJS_ATTR_OBJECT_HELPERS);
}


void ejsConfigureObjectType(Ejs *ejs)
{
    EjsType     *type, *t2;
    EjsFunction *fun, *existingFun;
    int         count, i, j;

    type = ejs->objectType;
    mprAssert(type);

    ejsBindMethod(ejs, type, ES_Object_clone, cloneObjectMethod);
    ejsBindMethod(ejs, type, ES_Object_get, getObjectIterator);
    ejsBindMethod(ejs, type, ES_Object_getValues, getObjectValues);
#if UNUSED
    ejsSetAccessors(ejs, type, ES_Object_length, getObjectLength, -1, 0);
#else
    ejsBindMethod(ejs, type, ES_Object_length, getObjectLength);
#endif
    ejsBindMethod(ejs, type, ES_Object_toString, objectToString);

#if ES_Object_propertyIsEnumerable && FUTURE
    ejsBindMethod(ejs, type, ES_Object_propertyIsEnumerable, propertyIsEnumerable);
#endif
#if ES_Object_seal
    ejsBindMethod(ejs, type, ES_Object_seal, seal);
#endif
#if ES_Object_toLocaleString
    ejsBindMethod(ejs, type, ES_Object_toLocaleString, toLocaleString);
#endif

#if FUTURE
    ejsBindMethod(ejs, type, ES_Object___defineProperty__, __defineProperty__);
    ejsBindMethod(ejs, type, ES_Object_hasOwnProperty, hasOwnProperty);
    ejsBindMethod(ejs, type, ES_Object_isPrototypeOf, isPrototypeOf);
#endif

    /*
     *  Patch native methods into all objects inheriting from object
     *  TODO - generalize and move into ejsBlock
     */
    count = ejsGetPropertyCount(ejs, ejs->global);
    for (i = 0; i < count; i++) {
        t2 = (EjsType*) ejsGetProperty(ejs, ejs->global, i);
        if (t2 != type && ejsIsType(t2) && !t2->isInterface && t2->hasObject) {
            for (j = 0; j < type->block.obj.numProp; j++) {
                fun = (EjsFunction*) ejsGetProperty(ejs, (EjsVar*) type, j);
                if (ejsIsNativeFunction(fun)) {
                    existingFun = (EjsFunction*) ejsGetProperty(ejs, (EjsVar*) t2, j);
                    if (!ejsIsFunction(existingFun) || !existingFun->override) {
                        ejsSetProperty(ejs, (EjsVar*) t2, j, (EjsVar*) fun);
                    }
                }
            }
        }
    }
}


void ejsInitializeObjectHelpers(EjsTypeHelpers *helpers)
{
    /*
     *  call is not implemented, EjsObject does not override and it is handled by the vm.
     */
    helpers->castVar                = (EjsCastVarHelper) castObject;
    helpers->cloneVar               = (EjsCloneVarHelper) ejsCopyObject;
    helpers->createVar              = (EjsCreateVarHelper) ejsCreateObject;
    helpers->defineProperty         = (EjsDefinePropertyHelper) defineObjectProperty;
    helpers->destroyVar             = (EjsDestroyVarHelper) destroyObject;
    helpers->deleteProperty         = (EjsDeletePropertyHelper) deleteObjectProperty;
    helpers->deletePropertyByName   = (EjsDeletePropertyByNameHelper) deleteObjectPropertyByName;
    helpers->finalizeVar            = (EjsFinalizeVarHelper) finalizeObject;
    helpers->getProperty            = (EjsGetPropertyHelper) getObjectProperty;
    helpers->getPropertyCount       = (EjsGetPropertyCountHelper) getObjectPropertyCount;
    helpers->getPropertyName        = (EjsGetPropertyNameHelper) getObjectPropertyName;
    helpers->lookupProperty         = (EjsLookupPropertyHelper) lookupObjectProperty;
    helpers->invokeOperator         = (EjsInvokeOperatorHelper) ejsObjectOperator;
    helpers->markVar                = (EjsMarkVarHelper) ejsMarkObject;
    helpers->setProperty            = (EjsSetPropertyHelper) setObjectProperty;
    helpers->setPropertyName        = (EjsSetPropertyNameHelper) setObjectPropertyName;
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
