/**
 *  ejsXMLList.c - E4X XMLList type.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejs.h"

#if BLD_FEATURE_EJS_E4X

/****************************** Forward Declarations **************************/
/*
 *  XMLList methods
 */

#if UNUSED
static EjsVar   *valueOf(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);
static EjsVar   *xlLength(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);
static EjsVar   *toXmlString(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);
static EjsVar   *appendChild(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);
static EjsVar   *attributes(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);
static EjsVar   *child(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);
static EjsVar   *elements(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);
static EjsVar   *comments(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);
static EjsVar   *decendants(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);
static EjsVar   *elements(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);
static EjsVar   *insertChildAfter(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);
static EjsVar   *insertChildBefore(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);
static EjsVar   *replace(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);
static EjsVar   *setName(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);
static EjsVar   *text(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);

#endif

static bool allDigitsForXmlList(cchar *name);
static EjsXML *resolve(Ejs *ejs, EjsXML *obj);
static EjsXML *shallowCopy(Ejs *ejs, EjsXML *xml);

/*********************************** Helpers **********************************/

static EjsXML *createXmlListVar(Ejs *ejs, EjsType *type, int size)
{
    return (EjsXML*) ejsCreateXMLList(ejs, NULL, NULL);
}


static void destroyXmlList(Ejs *ejs, EjsXML *list)
{
    ejsFreeVar(ejs, (EjsVar*) list);
}


static EjsVar *cloneXmlList(Ejs *ejs, EjsXML *list, bool deep)
{
    EjsXML  *newList;

    //  TODO - implement deep copy
    newList = (EjsXML*) ejsCreateVar(ejs, list->var.type, 0);
    if (newList == 0) {
        ejsThrowMemoryError(ejs);
        return 0;
    }
    //  TODO incomplete
    return (EjsVar*) newList;
}


/*
 *  Cast the object operand to a primitive type
 */
static EjsVar *xlCast(Ejs *ejs, EjsXML *vp, EjsType *type)
{
    MprBuf      *buf;
    EjsVar      *result;
    EjsXML      *elt, *item;
    int         next;

    switch (type->id) {
    case ES_Object:
    case ES_XML:
        return (EjsVar*) vp;

    case ES_Boolean:
        return (EjsVar*) ejsCreateBoolean(ejs, 1);

    case ES_Number:
        result = xlCast(ejs, vp, ejs->stringType);
        result = (EjsVar*) ejsToNumber(ejs, result);
        return result;

    case ES_String:
        buf = mprCreateBuf(ejs, MPR_BUFSIZE, -1);
        if (mprGetListCount(vp->elements) == 1) {
            elt = mprGetFirstItem(vp->elements);
            if (elt->kind == EJS_XML_ELEMENT) {
                if (elt->elements == 0) {
                    return (EjsVar*) ejs->emptyStringValue;
                }
                if (elt->elements && mprGetListCount(elt->elements) == 1) {
                    //  TODO - what about PI and comments?
                    item = mprGetFirstItem(elt->elements);
                    if (item->kind == EJS_XML_TEXT) {
                        return (EjsVar*) ejsCreateString(ejs, item->value);
                    }
                }
            }
        }
        for (next = 0; (elt = mprGetNextItem(vp->elements, &next)) != 0; ) {
            if (ejsXMLToString(ejs, buf, elt, -1) < 0) {
                mprFree(buf);
                return 0;
            }
        }
        result = (EjsVar*) ejsCreateString(ejs, (char*) buf->start);
        mprFree(buf);
        return result;

    default:
        ejsThrowTypeError(ejs, "Can't cast to this type");
        return 0;
    }
}


//  TODO - seems the return code for delete should be boolean?

static int deleteXmlListPropertyByName(Ejs *ejs, EjsXML *list, EjsName *qname)
{
    EjsXML      *elt;
    int         index, next;

    if (isdigit((int) qname->name[0]) && allDigitsForXmlList(qname->name)) {
        index = atoi(qname->name);

        elt = (EjsXML*) mprGetItem(list->elements, index);
        if (elt) {
            if (elt->parent) {
                if (elt->kind == EJS_XML_ATTRIBUTE) {
                    ejsDeletePropertyByName(ejs, (EjsVar*) elt->parent, &elt->qname);
                } else {
                    //  TODO - let q be the property of parent where parent[q] == x[i]
                    mprRemoveItem(elt->parent->elements, elt);
                    elt->parent = 0;
                }
            }
        }
        //  Spec says return true even if index is out of range. We return 0 for true and < 0 for false.
        //  TODO - should ejs throw?
        return 0;
    }

    for (next = 0; (elt = mprGetNextItem(list->elements, &next)) != 0; ) {
        if (elt->kind == EJS_XML_ELEMENT && elt->parent) {
            ejsDeletePropertyByName(ejs, (EjsVar*) elt->parent, qname);
        }
    }

    return 0;
}


static int getXmlListPropertyCount(Ejs *ejs, EjsXML *list)
{
    return mprGetListCount(list->elements);
}


/*
 *  Lookup a property by name. There are 7 kinds of lookups:
 *       prop, @att, [prop], *, @*, .name, .@name
 */
static EjsVar *getXmlListPropertyByName(Ejs *ejs, EjsXML *list, EjsName *qname)
{
    EjsXML      *result, *subList, *item;
    int         nextItem;

#if UNUSED
    if (ejsIsCall(ejs->opcode)) {
        return 0;
    }
#endif

    /*
     *  Get the n'th item in the list
     */
    if (isdigit((int) qname->name[0]) && allDigitsForXmlList(qname->name)) {
        return mprGetItem(list->elements, atoi(qname->name));
    }

    result = ejsCreateXMLList(ejs, list, qname);

    /*
     *  Build a list of all the elements that themselves have a property qname.
     */
    for (nextItem = 0; (item = mprGetNextItem(list->elements, &nextItem)) != 0; ) {
        if (item->kind == EJS_XML_ELEMENT) {
            subList = (EjsXML*) ejsGetPropertyByName(ejs, (EjsVar*) item, qname);
            mprAssert(ejsIsXML(subList));
            ejsAppendToXML(ejs, result, subList);

        } else {
            //  TODO - do we ever get a list in a list?
            mprAssert(0);
        }
    }

    return (EjsVar*) result;
}


static EjsVar *getXmlListNodeName(Ejs *ejs, EjsXML *xml, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateString(ejs, xml->qname.name);
}



/*
 *  Function to iterate and return the next element name.
 *  NOTE: this is not a method of Xml. Rather, it is a callback function for Iterator
 */
static EjsVar *nextXmlListKey(Ejs *ejs, EjsIterator *ip, int argc, EjsVar **argv)
{
    EjsXML  *xml;

    xml = (EjsXML*) ip->target;
    if (!ejsIsXML(xml)) {
        ejsThrowReferenceError(ejs, "Wrong type");
        return 0;
    }

    for (; ip->index < mprGetListCount(xml->elements); ip->index++) {
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
static EjsVar *getXmlListIterator(Ejs *ejs, EjsVar *xml, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateIterator(ejs, xml, (EjsNativeFunction) nextXmlListKey, 0, NULL);
}


/*
 *  Function to iterate and return the next element value.
 *  NOTE: this is not a method of Xml. Rather, it is a callback function for Iterator
 */
static EjsVar *nextXmlListValue(Ejs *ejs, EjsIterator *ip, int argc, EjsVar **argv)
{
    EjsXML      *xml, *vp;

    xml = (EjsXML*) ip->target;
    if (!ejsIsXML(xml)) {
        ejsThrowReferenceError(ejs, "Wrong type");
        return 0;
    }

    for (; ip->index < mprGetListCount(xml->elements); ip->index++) {
        vp = (EjsXML*) mprGetItem(xml->elements, ip->index);
        if (vp == 0) {
            continue;
        }
        ip->index++;
        return (EjsVar*) vp;
    }
    ejsThrowStopIteration(ejs);
    return 0;
}


/*
 *  Return an iterator to return the next array element value.
 *
 *  iterator native function getValues(): Iterator
 */
static EjsVar *getXmlListValues(Ejs *ejs, EjsVar *ap, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateIterator(ejs, ap, (EjsNativeFunction) nextXmlListValue, 0, NULL);
}


#if OLD
/*
 *  Handle all core operators. We currenly handle only === and !==
 *  TODO. Must implement: +, -, <, >, <=, >=, ==, ===, !=, !==, &, |
 */
static EjsVar *invokeOperator(Ejs *ejs, EjsXML *lhs, int opCode,  EjsXML *rhs)
{
    EjsVar      *l, *r;
    bool        boolResult;

    mprAssert(ejsIsXML(lhs));
    mprAssert(ejsIsXML(rhs));

    //  TODO - Complete
    switch (opCode) {
    case EJS_OP_COMPARE_EQ:
    case EJS_OP_COMPARE_STRICTLY_EQ:
        boolResult = (lhs == rhs);
        break;

    case EJS_OP_COMPARE_NE:
    case EJS_OP_COMPARE_STRICTLY_NE:
        boolResult = !(lhs == rhs);
        break;

    default:
        /*
         *  Cast to strings and re-invoke
         */
        l = ejsCastVar(ejs, (EjsVar*) lhs, ejs->stringType);
        r = ejsCastVar(ejs, (EjsVar*) rhs, ejs->stringType);
        return ejsInvokeOperator(ejs, l, opCode, r);
    }
    return (EjsVar*) ejsCreateBoolean(ejs, boolResult);
}
#endif


/*
 *  Set an alpha property by name.
 */
static int setAlphaPropertyByName(Ejs *ejs, EjsXML *list, EjsName *qname, EjsVar *value)
{
    EjsXML      *elt, *targetObject;
    int         count;

    targetObject = 0;

    count = ejsGetPropertyCount(ejs, (EjsVar*) list);
    if (count > 1) {
        //  TODO - why no error in spec?
        mprAssert(0);
        return 0;
    }

    if (count == 0) {
        /*
         *  Empty list so resolve the real target object and append it to the list.
         */
        targetObject = resolve(ejs, list);
        if (targetObject == 0) {
            return 0;
        }
        if (ejsGetPropertyCount(ejs, (EjsVar*) targetObject) != 1) {
            return 0;
        }
        ejsAppendToXML(ejs, list, targetObject);
    }

    /*
     *  Update the element
     */
    mprAssert(ejsGetPropertyCount(ejs, (EjsVar*) list) == 1);
    elt = mprGetItem(list->elements, 0);                        //  TODO OPT - GetFirstItem
    mprAssert(elt);
    ejsSetPropertyByName(ejs, (EjsVar*) elt, qname, value);

    return 0;
}


static EjsXML *createElement(Ejs *ejs, EjsXML *list, EjsXML *targetObject, EjsName *qname, EjsVar *value)
{
    EjsXML      *elt, *last, *attList;
    int         index;
    int         j;

    if (targetObject && ejsIsXML(targetObject) && targetObject->kind == EJS_XML_LIST) {

        /*
         *  If the target is a list it must have 1 element. So switch to it.
         *  TODO - could we get resolve to do this?
         */
        if (mprGetListCount(targetObject->elements) != 1) {
            /* Spec says so - TODO why no error? */
            return 0;
        }
        targetObject = mprGetFirstItem(targetObject->elements);
    }

    /*
     *  Return if the target object is not an XML element
     */
    if (!ejsIsXML(targetObject) || targetObject->kind != EJS_XML_ELEMENT) {
            /* Spec says so - TODO why no error? */
        return 0;
    }

    elt = ejsCreateXML(ejs, EJS_XML_ELEMENT, &list->targetProperty, targetObject, NULL);

    if (list->targetProperty.name && list->targetProperty.name[0] == '@') {
        elt->kind = EJS_XML_ATTRIBUTE;
        attList = (EjsXML*) ejsGetPropertyByName(ejs, (EjsVar*) targetObject, &list->targetProperty);
        if (attList && mprGetListCount(attList->elements) > 0) {
            /* Spec says so. But this surely means you can't update an attribute? */
            return 0;
        }
    } else if (list->targetProperty.name == 0 || qname->name[0] == '*') {
        elt->kind = EJS_XML_TEXT;
        elt->qname.name = 0;
    }

    index = mprGetListCount(list->elements);

    if (elt->kind != EJS_XML_ATTRIBUTE) {
        if (targetObject) {
            if (index > 0) {
                /*
                 *  Find the place of the last list item in the resolved target object.
                 */
                last = mprGetItem(list->elements, index - 1);
                j = mprLookupItem(targetObject->elements, last);

            } else {
                j = mprGetListCount(targetObject->elements) - 1;
            }
            //  TODO - really need to wrap this ejsInsertXML(EjsXML *xml, int index, EjsXML *node)
            if (targetObject->elements == 0) {
                targetObject->elements = mprCreateList(targetObject);
            }
            /*
             *  Insert into the target object
             */
            mprInsertItemAtPos(targetObject->elements, j + 1, elt);
        }

        if (ejsIsXML(value)) {
            if (((EjsXML*) value)->kind == EJS_XML_LIST) {
                elt->qname = ((EjsXML*) value)->targetProperty;
            } else {
                elt->qname = ((EjsXML*) value)->qname;
            }
        }

        /*
         *  Insert into the XML list
         */
        mprSetItem(list->elements, index, elt);
    }

    return (EjsXML*) mprGetItem(list->elements, index);
}


/*
 *  Update an existing element
 */
static int updateElement(Ejs *ejs, EjsXML *list, EjsXML *elt, int index, EjsVar *value)
{
    EjsXML      *node;
    EjsName     name;
    int         i, j;

    if (!ejsIsXML(value)) {
        /* Not XML or XMLList -- convert to string */
        value = ejsCastVar(ejs, value, ejs->stringType);                //  TODO - seem to be doing this in too many places
    }
    mprSetItem(list->elements, index, value);

    if (elt->kind == EJS_XML_ATTRIBUTE) {
        mprAssert(ejsIsString(value));
        i = mprLookupItem(elt->parent->elements, elt);
        ejsSetXML(ejs, elt->parent, i, elt);
        //  TODO - why do this. Doesn't above do this?
        ejsSetPropertyByName(ejs, (EjsVar*) elt->parent, &elt->qname, value);
        mprFree(elt->value);
        elt->value = mprStrdup(elt, ((EjsString*) value)->value);
    }

    if (ejsIsXML(value) && ((EjsXML*) value)->kind == EJS_XML_LIST) {
        value = (EjsVar*) shallowCopy(ejs, (EjsXML*) value);
        if (elt->parent) {
            index = mprLookupItem(elt->parent->elements, elt);
            for (j = 0; j < mprGetListCount(((EjsXML*) value)->elements); j++) {
                mprInsertItemAtPos(elt->parent->elements, index, value);
            }
        }

    } else if (ejsIsXML(value) || elt->kind != EJS_XML_ELEMENT) {
        if (elt->parent) {
            index = mprLookupItem(elt->parent->elements, elt);
            mprSetItem(elt->parent->elements, index, value);
            if (ejsIsString(value)) {
                node = ejsCreateXML(ejs, EJS_XML_TEXT, NULL, list, ((EjsString*) value)->value);
                mprSetItem(list->elements, index, node);

            } else {
                mprSetItem(list->elements, index, value);
            }
        }

    } else {
        ejsName(&name, 0, "*");
        ejsSetPropertyByName(ejs, (EjsVar*) elt, &name, value);
    }

    return index;
}


/*
 *  Set a property by name.
 */
static int setXmlListPropertyByName(Ejs *ejs, EjsXML *list, EjsName *qname, EjsVar *value)
{
    EjsXML      *elt, *targetObject;
    int         index;

    if (!isdigit((int) qname->name[0])) {
        return setAlphaPropertyByName(ejs, list, qname, value);
    }

    /*
     *  Numeric property
     */
    targetObject = 0;
    if (list->targetObject) {
        /*
         *  Find the real underlying target object. May be an XML object or XMLList if it contains multiple elements.
         */
        targetObject = resolve(ejs, list->targetObject);
        if (targetObject == 0) {
            /* Spec says so - TODO why no error? */
            return 0;
        }
    }

    index = atoi(qname->name);
    if (index >= mprGetListCount(list->elements)) {
        /*
         *  Create, then fall through to update
         */
        elt = createElement(ejs, list, targetObject, qname, value);
        if (elt == 0) {
            return 0;
        }

    } else {
        elt = mprGetItem(list->elements, index);
    }
    mprAssert(elt);

    updateElement(ejs, list, elt, index, value);

    return index;
}


/******************************** Support Routines **************************/


static bool allDigitsForXmlList(cchar *name)
{
    cchar   *cp;

    for (cp = name; *cp; cp++) {
        if (!isdigit((int) *cp) || *cp == '.') {
            return 0;
        }
    }
    return 1;
}


static EjsXML *shallowCopy(Ejs *ejs, EjsXML *xml)
{
    EjsXML      *root, *elt;
    int         next;

    mprAssert(xml->kind == EJS_XML_LIST);

    if (xml == 0) {
        return 0;
    }

    root = ejsCreateXMLList(ejs, xml->targetObject, &xml->targetProperty);
    if (root == 0) {
        return 0;
    }

    if (xml->elements) {
        root->elements = mprCreateList(root);
        for (next = 0; (elt = mprGetNextItem(xml->elements, &next)) != 0; ) {
            mprAssert(ejsIsXML(elt));
            if (elt) {
                mprAddItem(root->elements, elt);
            }
        }
    }

    if (mprHasAllocError(ejs)) {
        mprFree(root);
        return 0;
    }

    return root;
}


/*
 *  Resolve empty XML list objects to an actual XML object. This is used by SetPropertyByName to find the actual object to update.
 *  This method resolves the value of empty XMLLists. If the XMLList is not empty, the list will be returned. If list is empty,
 *  this method attempts to create an element based on the list targetObject and targetProperty.
 */
static EjsXML *resolve(Ejs *ejs, EjsXML *xml)
{
    EjsXML  *targetObject, *targetPropertyList;

    if (!ejsIsXML(xml) || xml->kind != EJS_XML_LIST) {
        /* Resolved to an XML object */
        return xml;
    }

    if (mprGetListCount(xml->elements) > 0) {
        /* Resolved to a list of items */
        return xml;
    }

    if (xml->targetObject == 0 || xml->targetProperty.name == 0 || xml->targetProperty.name[0] == '*') {
        /* End of chain an no more target objects */
        return 0;
    }

    targetObject = resolve(ejs, xml->targetObject);
    if (targetObject == 0) {
        return 0;
    }
    //  TODO - OPT. targetPropertyList is also being created below.
    targetPropertyList = (EjsXML*) ejsGetPropertyByName(ejs, (EjsVar*) targetObject, &xml->targetProperty);
    if (targetPropertyList == 0) {
        return 0;
    }

    if (ejsGetPropertyCount(ejs, (EjsVar*) targetPropertyList) == 0) {
        /*
         *  Property does not exist in the target.
         */
        if (targetObject->kind == EJS_XML_LIST && ejsGetPropertyCount(ejs, (EjsVar*) targetObject) > 1) {
            return 0;
        }
        /*
         *  Create the property as an element (The text value will be optimized away).
         *  TODO - OPT. Need an empty string value in EjsFiber.
         */
        ejsSetPropertyByName(ejs, (EjsVar*) targetObject, &xml->targetProperty, (EjsVar*) ejsCreateString(ejs, ""));
        targetPropertyList = (EjsXML*) ejsGetPropertyByName(ejs, (EjsVar*) targetObject, &xml->targetProperty);
    }
    return targetPropertyList;
}


/************************************ Methods ********************************/

static EjsVar *xmlListConstructor(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv)
{
#if FUTURE
    EjsVar      *vp;
    cchar       *str;

    if (argc == 1) {
        vp = argv[0];

        if (ejsIsObject(vp)) {
            /* Convert DOM to XML. Not implemented */;

        } else if (ejsIsString(vp)) {
            str = ((EjsString*) vp)->value;
            if (str == 0) {
                return 0;
            }
            if (*str == '<') {
                /* XML Literal */
                return loadXmlString(ejs, (EjsXML*) thisObj, str);

            } else {
                /* Load from file */
                return load(ejs, (EjsXML*) thisObj, argc, argv);
            }
        } else {
            ejsThrowArgError(ejs, "Bad type passed to XML constructor");
            return 0;
        }
    }
#endif
    return (EjsVar*) thisObj;
}


/*
 *  Convert the XML object to a string.
 *
 *  intrinsic function toString() : String
 */
static EjsVar *xmlListToString(Ejs *ejs, EjsVar *vp, int argc, EjsVar **argv)
{
    return (vp->type->helpers->castVar)(ejs, vp, ejs->stringType);
}


/*
 *  Get the length of an array.
 *  @return Returns the number of items in the array
 *
 *  intrinsic public override function get length(): int
 */

static EjsVar *xlLength(Ejs *ejs, EjsXML *xml, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, mprGetListCount(xml->elements));
}


#if FUTURE
/*
 *  Set the length. TODO - what does this do?
 *  intrinsic public override function set length(value: int): void
 */
static EjsVar *setLength(Ejs *ejs, EjsXMLList *xml, int argc, EjsVar **argv)
{
    int         length;

    mprAssert(ejsIsXMLList(xml));

    if (argc != 1) {
        ejsThrowArgError(ejs, "usage: obj.length = value");
        return 0;
    }
    length = ejsVarToInteger(ejs, argv[0]);

#if UNUSED
    if (length < ap->length) {
        for (i = length; i < ap->length; i++) {
            if (ejsSetProperty(ejs, (EjsVar*) ap, i, (EjsVar*) ejs->undefinedValue) < 0) {
                //  TODO - DIAG
                return 0;
            }
        }

    } else if (length > ap->length) {
        if (ejsSetProperty(ejs, (EjsVar*) ap, length - 1,  (EjsVar*) ejs->undefinedValue) < 0) {
            //  TODO - DIAG
            return 0;
        }
    }

    ap->length = length;
#endif

    return 0;
}
#endif


/*********************************** Factory **********************************/

EjsXML *ejsCreateXMLList(Ejs *ejs, EjsXML *targetObject, EjsName *targetProperty)
{
    EjsType     *type;
    EjsXML      *list;

    type = ejs->xmlListType;

    list = (EjsXML*) ejsAllocVar(ejs, type, 0);
    if (list == 0) {
        return 0;
    }

    list->kind = EJS_XML_LIST;
    list->elements = mprCreateList(list);
    list->targetObject = targetObject;

    if (targetProperty) {
        list->targetProperty.name = mprStrdup(list, targetProperty->name);
    }

#if NOT_NEEDED
    /*
     *  Temporary until we have namespaces
     */
    char        *cp;
    for (cp = name; *cp; cp++) {
        if (*cp == ':') {
            *cp = '_';
        }
    }
#endif

    return list;
}


int ejsCreateXMLListType(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;

    type = ejsCreateCoreType(ejs, ejsName(&qname, EJS_INTRINSIC_NAMESPACE, "XMLList"), ejs->objectType, sizeof(EjsXML), 
        ES_XMLList, ES_XMLList_NUM_CLASS_PROP, ES_XMLList_NUM_INSTANCE_PROP, EJS_ATTR_NATIVE | EJS_ATTR_HAS_CONSTRUCTOR);
    if (type == 0) {
        return EJS_ERR;
    }
    ejs->xmlListType = type;

    type->nobind = 1;

    /*
     *  Define the helper functions.
     */
    type->helpers->cloneVar = (EjsCloneVarHelper) cloneXmlList;
    type->helpers->castVar = (EjsCastVarHelper) xlCast;
    type->helpers->createVar = (EjsCreateVarHelper) createXmlListVar;
    type->helpers->destroyVar = (EjsDestroyVarHelper) destroyXmlList;
    type->helpers->getPropertyByName = (EjsGetPropertyByNameHelper) getXmlListPropertyByName;
    type->helpers->getPropertyCount = (EjsGetPropertyCountHelper) getXmlListPropertyCount;
    type->helpers->deletePropertyByName = (EjsDeletePropertyByNameHelper) deleteXmlListPropertyByName;
    type->helpers->invokeOperator = (EjsInvokeOperatorHelper) ejsObjectOperator;
    type->helpers->markVar = (EjsMarkVarHelper) ejsMarkXML;
    type->helpers->setPropertyByName = (EjsSetPropertyByNameHelper) setXmlListPropertyByName;
    return 0;
}


int ejsConfigureXMLListType(Ejs *ejs)
{
    EjsType     *type;

    type = ejs->xmlListType;

    /*
     *  Define the XMLList class methods
     */
    ejsBindMethod(ejs, type, ES_XMLList_XMLList, (EjsNativeFunction) xmlListConstructor);
    ejsBindMethod(ejs, type, ES_XML_name, (EjsNativeFunction) getXmlListNodeName);

    /*
     *  Override these methods
     */
    ejsBindMethod(ejs, type, ES_Object_toString, (EjsNativeFunction) xmlListToString);
    ejsBindMethod(ejs, type, ES_Object_length, (EjsNativeFunction) xlLength);

    ejsBindMethod(ejs, type, ES_Object_get, getXmlListIterator);
    ejsBindMethod(ejs, type, ES_Object_getValues, getXmlListValues);
#if FUTURE
    ejsBindMethod(ejs, type, "name", name, NULL);
    ejsBindMethod(ejs, type, "valueOf", valueOf, NULL);
#endif
    return 0;
}


/******************************************************************************/
#else
void __ejsXMLListDummy() {}
#endif /* BLD_FEATURE_EJS_E4X */


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
