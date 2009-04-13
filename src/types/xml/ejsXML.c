/**
 *  ejsXML.c - E4X XML type.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejs.h"

#if BLD_FEATURE_EJS_E4X

/****************************** Forward Declarations **************************/
/*
 *  XML methods
 */
static EjsVar   *loadXml(Ejs *ejs, EjsXML *xml, int argc, EjsVar **argv);
static EjsVar   *saveXml(Ejs *ejs, EjsXML *xml, int argc, EjsVar **argv);

#if UNUSED
static EjsVar   *toString(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);
static EjsVar   *valueOf(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv);
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

static bool allDigitsForXml(cchar *name);
static bool deepCompare(EjsXML *lhs, EjsXML *rhs);
static int readStringData(MprXml *xp, void *data, char *buf, int size);
static int readFileData(MprXml *xp, void *data, char *buf, int size);

/*********************************** Helpers **********************************/

static EjsXML *createXml(Ejs *ejs, EjsType *type, int size)
{
    return (EjsXML*) ejsCreateXML(ejs, 0, NULL, NULL, NULL);
}


static void destroyXml(Ejs *ejs, EjsXML *xml)
{
    ejsFreeVar(ejs, (EjsVar*) xml);
}


static EjsVar *cloneXml(Ejs *ejs, EjsXML *xml, bool deep)
{
    EjsXML  *newXML;

    //  TODO - implement deep copy.

    newXML = (EjsXML*) ejsCreateVar(ejs, xml->var.type, 0);
    if (newXML == 0) {
        ejsThrowMemoryError(ejs);
        return 0;
    }

    //  TODO complete
    return (EjsVar*) newXML;
}


/*
 *  Cast the object operand to a primitive type
 */
static EjsVar *castXml(Ejs *ejs, EjsXML *xml, EjsType *type)
{
    EjsXML      *item;
    EjsVar      *result;
    MprBuf      *buf;

    mprAssert(ejsIsXML(xml));

    switch (type->id) {
    case ES_Object:
    case ES_XMLList:
        return (EjsVar*) xml;

    case ES_Boolean:
        return (EjsVar*) ejsCreateBoolean(ejs, 1);

    case ES_Number:
        result = castXml(ejs, xml, ejs->stringType);
        result = (EjsVar*) ejsToNumber(ejs, result);
        return result;

    case ES_String:
        if (xml->kind == EJS_XML_ELEMENT) {
            if (xml->elements == 0) {
                return (EjsVar*) ejs->emptyStringValue;
            }
            if (xml->elements && mprGetListCount(xml->elements) == 1) {
                //  TODO - what about PI and comments?
                item = mprGetFirstItem(xml->elements);
                if (item->kind == EJS_XML_TEXT) {
                    return (EjsVar*) ejsCreateString(ejs, item->value);
                }
            }
        }
        buf = mprCreateBuf(ejs, MPR_BUFSIZE, -1);
        if (ejsXMLToString(ejs, buf, xml, -1) < 0) {
            mprFree(buf);
            return 0;
        }
        result = (EjsVar*) ejsCreateString(ejs, (char*) buf->start);
        mprFree(buf);
        return result;

    default:
        ejsThrowTypeError(ejs, "Can't cast to this type");
        return 0;
    }
    return 0;
}


static int deleteXmlPropertyByName(Ejs *ejs, EjsXML *xml, EjsName *qname)
{
    EjsXML      *item;
    bool        removed;
    int         next;

    removed = 0;

    if (qname->name[0] == '@') {
        /* @ and @* */
        if (xml->attributes) {
            for (next = 0; (item = mprGetNextItem(xml->attributes, &next)) != 0; ) {
                mprAssert(qname->name[0] == '@');
                if (qname->name[1] == '*' || strcmp(item->qname.name, &qname->name[1]) == 0) {
                    mprRemoveItemAtPos(xml->attributes, next - 1);
                    item->parent = 0;
                    removed = 1;
                    next -= 1;
                }
            }
        }

    } else {
        /* name and * */
        if (xml->elements) {
            for (next = 0; (item = mprGetNextItem(xml->elements, &next)) != 0; ) {
                mprAssert(item->qname.name);
                if (qname->name[0] == '*' || strcmp(item->qname.name, qname->name) == 0) {
                    mprRemoveItemAtPos(xml->elements, next - 1);
                    item->parent = 0;
                    removed = 1;
                    next -= 1;
                }
            }
        }
    }
    return (removed) ? 0 : EJS_ERR;
}


static EjsVar *getXmlNodeName(Ejs *ejs, EjsXML *xml, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateString(ejs, xml->qname.name);
}



/*
 *  Function to iterate and return the next element name.
 *  NOTE: this is not a method of Xml. Rather, it is a callback function for Iterator
 */
static EjsVar *nextXmlKey(Ejs *ejs, EjsIterator *ip, int argc, EjsVar **argv)
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
static EjsVar *getXmlIterator(Ejs *ejs, EjsVar *xml, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateIterator(ejs, xml, (EjsNativeFunction) nextXmlKey, 0, NULL);
}


/*
 *  Function to iterate and return the next element value.
 *  NOTE: this is not a method of Xml. Rather, it is a callback function for Iterator
 */
static EjsVar *nextXmlValue(Ejs *ejs, EjsIterator *ip, int argc, EjsVar **argv)
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
static EjsVar *getXmlValues(Ejs *ejs, EjsVar *ap, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateIterator(ejs, ap, (EjsNativeFunction) nextXmlValue, 0, NULL);
}


static int getXmlPropertyCount(Ejs *ejs, EjsXML *xml)
{
    return mprGetListCount(xml->elements);
}


/*
 *  Lookup a property by name. There are 7 kinds of lookups:
 *       prop, @att, [prop], *, @*, .name, .@name
 */
static EjsVar *getXmlPropertyByName(Ejs *ejs, EjsXML *xml, EjsName *qname)
{
    EjsXML      *item, *result, *list;
    int         next, nextList;

    result = 0;

    mprAssert(xml->kind < 5);
#if UNUSED
    if (ejsIsCall(*ejs->frame->pc)) {
        return 0;
    }
#endif
#if WAS
    if (ejsIsCall(ejs->opcode)) {
        return 0;
    }
#endif

    if (isdigit((int) qname->name[0]) && allDigitsForXml(qname->name)) {
        /*
         *  Consider xml as a list with only one entry == xml. Then return the 0'th entry
         */
        return (EjsVar*) xml;
    }

    if (qname->name[0] == '@') {
        /* @ and @* */
        result = ejsCreateXMLList(ejs, xml, qname);
        if (xml->attributes) {
            for (next = 0; (item = mprGetNextItem(xml->attributes, &next)) != 0; ) {
                mprAssert(qname->name[0] == '@');
                if (qname->name[1] == '*' || strcmp(item->qname.name, &qname->name[1]) == 0) {
                    result = ejsAppendToXML(ejs, result, item);
                }
            }
        }

    } else if (qname->name[0] == '.') {
        /* Decenders (do ..@ also) */
        result = ejsXMLDescendants(ejs, xml, qname);

    } else {
        /* name and * */
        result = ejsCreateXMLList(ejs, xml, qname);
        if (xml->elements) {
            for (next = 0; (item = mprGetNextItem(xml->elements, &next)) != 0; ) {
        mprAssert(xml->kind !=24);
                if (item->kind == EJS_XML_LIST) {
                    list = item;
                    for (nextList = 0; (item = mprGetNextItem(list->elements, &nextList)) != 0; ) {
                        mprAssert(item->qname.name);
                        if (qname->name[0] == '*' || strcmp(item->qname.name, qname->name) == 0) {
                            result = ejsAppendToXML(ejs, result, item);
                        }
                    }

                } else {
                    mprAssert(item->qname.name);
                    if (qname->name[0] == '*' || strcmp(item->qname.name, qname->name) == 0) {
                        result = ejsAppendToXML(ejs, result, item);
                    }
                }
            }
        }
    }

    return (EjsVar*) result;
}


static EjsVar *invokeXmlOperator(Ejs *ejs, EjsXML *lhs, int opcode,  EjsXML *rhs)
{
    EjsVar      *result;

    if ((result = ejsCoerceOperands(ejs, (EjsVar*) lhs, opcode, (EjsVar*) rhs)) != 0) {
        return result;
    }

    switch (opcode) {
    case EJS_OP_COMPARE_EQ:
        return (EjsVar*) ejsCreateBoolean(ejs, deepCompare(lhs, rhs));

    case EJS_OP_COMPARE_NE:
        return (EjsVar*) ejsCreateBoolean(ejs, !deepCompare(lhs, rhs));

    default:
        return ejsObjectOperator(ejs, (EjsVar*) lhs, opcode, (EjsVar*) rhs);
    }
}


/*
 *  Set a property attribute by name.
 */
static int setXmlPropertyAttributeByName(Ejs *ejs, EjsXML *xml, EjsName *qname, EjsVar *value)
{
    EjsXML      *elt, *attribute, *rp, *xvalue, *lastElt;
    EjsString   *sv;
    EjsName     qn;
    char        *str;
    int         index, last, next, len;

    /*
     *  Attribute. If the value is an XML list, convert to a space separated string
     */
    xvalue = (EjsXML*) value;
    if (ejsIsXML(xvalue) && xvalue->kind == EJS_XML_LIST) {
        str = 0;
        len = 0;
        for (next = 0; (elt = mprGetNextItem(xvalue->elements, &next)) != 0; ) {
            sv = (EjsString*) ejsCastVar(ejs, (EjsVar*) elt, ejs->stringType);
            len = mprReallocStrcat(ejs, &str, -1, len, " ", sv->value, 0);
        }
        value = (EjsVar*) ejsCreateString(ejs, str);
        mprFree(str);

    } else {
        value = ejsCastVar(ejs, value, ejs->stringType);
    }
    mprAssert(ejsIsString(value));

    /*
     *  Find the first attribute that matches. Delete all other attributes of the same name.
     */
    index = 0;
    if (xml->attributes) {
        lastElt = 0;
        for (last = -1, index = -1; (elt = mprGetPrevItem(xml->attributes, &index)) != 0; ) {
            mprAssert(qname->name[0] == '@');
            if (strcmp(elt->qname.name, &qname->name[1]) == 0) {
                if (last >= 0) {
                    rp = mprGetItem(xml->attributes, last);
                    mprRemoveItemAtPos(xml->attributes, last);
                }
                last = index;
                lastElt = elt;
            }
        }
        if (lastElt) {
            /*
             *  Found a match. So replace its value
             */
            mprFree(lastElt->value);
            lastElt->value = mprStrdup(lastElt, ((EjsString*) value)->value);
            return last;

        } else {
            index = mprGetListCount(xml->attributes);
        }
    }
    //  TODO - namespace work to do here

    /*
     *  Not found. Create a new attribute node
     */
    mprAssert(ejsIsString(value));
    ejsName(&qn, 0, &qname->name[1]);
    attribute = ejsCreateXML(ejs, EJS_XML_ATTRIBUTE, &qn, xml, ((EjsString*) value)->value);
    if (xml->attributes == 0) {
        xml->attributes = mprCreateList(xml);
    }
    mprSetItem(xml->attributes, index, attribute);

    return index;
}


/*
 *  Create a value node. If the value is an XML node already, we are done. Otherwise, cast the value to a string
 *  and create a text child node containing the string value.
 */
static EjsXML *createValueNode(Ejs *ejs, EjsXML *elt, EjsVar *value)
{
    EjsXML      *text;
    EjsString   *str;

    if (ejsIsXML(value)) {
        return (EjsXML*) value;
    }

    str = (EjsString*) ejsCastVar(ejs, value, ejs->stringType);
    if (str == 0) {
        return 0;
    }

    if (mprGetListCount(elt->elements) == 1) {
        /*
         *  Update an existing text element
         */
        text = mprGetFirstItem(elt->elements);
        if (text->kind == EJS_XML_TEXT) {
            mprFree(text->value);
            text->value = mprStrdup(elt, str->value);
            return elt;
        }
    }

    /*
     *  Create a new text element
     */
    if (str->value && str->value[0] != '\0') {
        text = ejsCreateXML(ejs, EJS_XML_TEXT, NULL, elt, str->value);
        elt = ejsAppendToXML(ejs, elt, text);
    }
    return elt;
}


void ejsMarkXML(Ejs *ejs, EjsVar *parent, EjsXML *xml)
{
    EjsVar          *item;
    int             next;

    if (xml->parent && !xml->parent->var.visited) {
        ejsMarkVar(ejs, (EjsVar*) xml, (EjsVar*) xml->parent);
    }
    if (xml->targetObject && !xml->targetObject->var.visited) {
        ejsMarkVar(ejs, (EjsVar*) xml, (EjsVar*) xml->targetObject);
    }

    for (next = 0; (item = mprGetNextItem(xml->attributes, &next)) != 0; ) {
        ejsMarkVar(ejs, (EjsVar*) xml, (EjsVar*) item);
    }
    for (next = 0; (item = mprGetNextItem(xml->elements, &next)) != 0; ) {
        ejsMarkVar(ejs, (EjsVar*) xml, (EjsVar*) item);
    }
}


/*
 *  Set a property by name. Implements the ECMA-357 [[put]] method.
 *  There are 7 kinds of qname's: prop, @att, [prop], *, @*, .name, .@name
 */
static int setXmlPropertyByName(Ejs *ejs, EjsXML *xml, EjsName *qname, EjsVar *value)
{
    EjsXML      *elt, *xvalue, *rp, *lastElt;
    EjsVar      *originalValue;
    int         index, last;

    mprLog(ejs, 9, "XMLSet %s.%s = \"%s\"", xml->qname.name, qname->name,
        ((EjsString*) ejsCastVar(ejs, value, ejs->stringType))->value);

    if (isdigit((int) qname->name[0]) && allDigitsForXml(qname->name)) {
        ejsThrowTypeError(ejs, "Integer indicies for set are not allowed");
        return EJS_ERR;
    }

    if (xml->kind != EJS_XML_ELEMENT) {
        //  TODO spec requires this -- but why? -- surely throw?
        return 0;
    }

    /*
     *  Massage the value type.
     */
    originalValue = value;

    xvalue = (EjsXML*) value;
    if (ejsIsXML(xvalue)) {
        if (xvalue->kind == EJS_XML_LIST) {
            value = (EjsVar*) ejsDeepCopyXML(ejs, xvalue);

        } else if (xvalue->kind == EJS_XML_TEXT || xvalue->kind == EJS_XML_ATTRIBUTE) {
            value = ejsCastVar(ejs, originalValue, ejs->stringType);

        } else {
            value = (EjsVar*) ejsDeepCopyXML(ejs, xvalue);
        }

    } else {
        value = ejsCastVar(ejs, value, ejs->stringType);
    }

    if (qname->name[0] == '@') {
        return setXmlPropertyAttributeByName(ejs, xml, qname, value);
    }

    /*
     *  Delete redundant elements by the same name.
     */
    lastElt = 0;
    if (xml->elements) {
        for (last = -1, index = -1; (elt = mprGetPrevItem(xml->elements, &index)) != 0; ) {
            if (qname->name[0] == '*' || (elt->kind == EJS_XML_ELEMENT && strcmp(elt->qname.name, qname->name) == 0)) {
                /*
                 *  Must remove all redundant elements of the same name except the first one
                 */
                if (last >= 0) {
                    rp = mprGetItem(xml->elements, last);
                    rp->parent = 0;
                    mprRemoveItemAtPos(xml->elements, last);
                }
                last = index;
                lastElt = elt;
            }
        }
    }

    if (xml->elements == 0) {
        //  TODO - need routine to do this centrally so we can control the default number of elements in the list?
        xml->elements = mprCreateList(xml);
    }

    elt = lastElt;

    if (qname->name[0] == '*') {
        /*
         *  Special case when called from XMLList to update the value of an element
         */
        xml = createValueNode(ejs, xml, value);

    } else if (elt == 0) {
        /*
         *  Not found. New node required.
         */
        elt = ejsCreateXML(ejs, EJS_XML_ELEMENT, qname, xml, NULL);
        if (elt == 0) {
            return 0;
        }
        index = mprGetListCount(xml->elements);
        xml = ejsAppendToXML(ejs, xml, createValueNode(ejs, elt, value));

    } else {
        /*
         *  Update existing element.
         */
        xml = ejsSetXML(ejs, xml, index, createValueNode(ejs, elt, value));
    }

    if (xml == 0) {
        return EJS_ERR;
    }
    return index;
}


/****************************** Support Routines ****************************/
/*
 *  Deep compare
 */
static bool deepCompare(EjsXML *lhs, EjsXML *rhs)
{
    EjsXML      *l, *r;
    int         i;

    if (lhs == rhs) {
        return 1;
    }

    //  TODO - must compare all the namespaces?
    if (lhs->kind != rhs->kind) {
        return 0;

    } else  if (mprStrcmp(lhs->qname.name, rhs->qname.name) != 0) {
        return 0;

    } else if (mprGetListCount(lhs->attributes) != mprGetListCount(rhs->attributes)) {
        //  TODO - must compare all the attributes
        return 0;

    } else if (mprGetListCount(lhs->elements) != mprGetListCount(rhs->elements)) {
        //  TODO - must compare all the children
        return 0;

    } else if (mprStrcmp(lhs->value, rhs->value) != 0) {
        return 0;

    } else {
        for (i = 0; i < mprGetListCount(lhs->elements); i++) {
            l = mprGetItem(lhs->elements, i);
            r = mprGetItem(rhs->elements, i);
            if (! deepCompare(l, r)) {
                return 0;
            }
        }
    }
    return 1;
}


//  TODO - rename ejsGetXMLDescendants. Check all other names.
EjsXML *ejsXMLDescendants(Ejs *ejs, EjsXML *xml, EjsName *qname)
{
    EjsXML          *item, *result;
    int             next;

    result = ejsCreateXMLList(ejs, xml, qname);
    if (result == 0) {
        return 0;
    }

    if (qname->name[0] == '@') {
        if (xml->attributes) {
            for (next = 0; (item = mprGetNextItem(xml->attributes, &next)) != 0; ) {
                mprAssert(qname->name[0] == '@');
                if (qname->name[1] == '*' || strcmp(item->qname.name, &qname->name[1]) == 0) {
                    result = ejsAppendToXML(ejs, result, item);
                }
            }
        }

    } else {
        if (xml->elements) {
            for (next = 0; (item = mprGetNextItem(xml->elements, &next)) != 0; ) {
                if (qname->name[0] == '*' || strcmp(item->qname.name, &qname->name[1]) == 0) {
                    result = ejsAppendToXML(ejs, result, item);
                } else {
                    result = ejsAppendToXML(ejs, result, ejsXMLDescendants(ejs, item, qname));
                }
            }
        }
    }
    return result;
}


EjsXML *ejsDeepCopyXML(Ejs *ejs, EjsXML *xml)
{
    EjsXML      *root, *elt;
    int         next;

    if (xml == 0) {
        return 0;
    }

    if (xml->kind == EJS_XML_LIST) {
        root = ejsCreateXMLList(ejs, xml->targetObject, &xml->targetProperty);
    } else {
        root = ejsCreateXML(ejs, xml->kind, &xml->qname, NULL, xml->value);
    }
    if (root == 0) {
        return 0;
    }

    //  TODO - must copy inScopeNamespaces?

    if (xml->attributes) {
        root->attributes = mprCreateList(xml);
        for (next = 0; (elt = (EjsXML*) mprGetNextItem(xml->attributes, &next)) != 0; ) {
            elt = ejsDeepCopyXML(ejs, elt);
            if (elt) {
                elt->parent = root;
                mprAddItem(root->attributes, elt);
            }
        }
    }

    if (xml->elements) {
        root->elements = mprCreateList(root);
        for (next = 0; (elt = mprGetNextItem(xml->elements, &next)) != 0; ) {
            mprAssert(ejsIsXML(elt));
            elt = ejsDeepCopyXML(ejs, elt);
            if (elt) {
                elt->parent = root;
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

/************************************ Methods ********************************/
/*
 *  native function XML(value: Object = null)
 */
static EjsVar *xmlConstructor(Ejs *ejs, EjsXML *thisObj, int argc, EjsVar **argv)
{
    EjsVar      *arg;
    cchar       *str;

    //  TODO - should be also able to handle a File object

    if (thisObj == 0) {
        /*
         *  Called as a function - cast the arg
         */
        if (argc > 0){
            arg = ejsCastVar(ejs, argv[0], ejs->stringType);
            if (arg == 0) {
                return 0;
            }
        }
        thisObj = ejsCreateXML(ejs, 0, NULL, NULL, NULL);
    }

    if (argc == 0) {
        return (EjsVar*) thisObj;
    }

    arg = argv[0];
    mprAssert(arg);

    if (ejsIsNull(arg) || ejsIsUndefined(arg)) {
        return (EjsVar*) thisObj;
    }

    if (ejsIsObject(arg)) {
        arg = ejsCastVar(ejs, argv[0], ejs->stringType);
    }
    if (arg && ejsIsString(arg)) {
        str = ((EjsString*) arg)->value;
        if (str == 0) {
            return 0;
        }
        if (*str == '<') {
            /* XML Literal */
            ejsLoadXMLString(ejs, thisObj, str);

        } else {
            /* Load from file */
            loadXml(ejs, thisObj, argc, argv);
        }

    } else {
        ejsThrowArgError(ejs, "Bad type passed to XML constructor");
        return 0;
    }
    return (EjsVar*) thisObj;
}


static EjsVar *loadXml(Ejs *ejs, EjsXML *xml, int argc, EjsVar **argv)
{
    MprFile     *file;
    MprXml      *xp;
    char        *filename;

    mprAssert(argc == 1 && ejsIsString(argv[0]));

    filename = ejsGetString(argv[0]);

    file = mprOpen(ejs, filename, O_RDONLY, 0664);
    if (file == 0) {
        ejsThrowIOError(ejs, "Can't open: %s", filename);
        return 0;
    }

    //  TODO - convert to open/close
    xp = ejsCreateXmlParser(ejs, xml, filename);
    if (xp == 0) {
        ejsThrowMemoryError(ejs);
        mprFree(xp);
        mprFree(file);
        return 0;
    }
    mprXmlSetInputStream(xp, readFileData, (void*) file);

    if (mprXmlParse(xp) < 0) {
        if (! ejsHasException(ejs)) {
            ejsThrowIOError(ejs, "Can't parse XML file: %s\nDetails %s",  filename, mprXmlGetErrorMsg(xp));
        }
        mprFree(xp);
        mprFree(file);
        return 0;
    }

    mprFree(xp);
    mprFree(file);

    return 0;
}


static EjsVar *saveXml(Ejs *ejs, EjsXML *xml, int argc, EjsVar **argv)
{
    MprBuf      *buf;
    MprFile     *file;
    cchar       *filename;
    int         bytes, len;

    if (argc != 1 || !ejsIsString(argv[0])) {
        ejsThrowArgError(ejs, "Bad args. Usage: save(filename);");
        return 0;
    }
    filename = ((EjsString*) argv[0])->value;

    /*
     *  Create a buffer to hold the output. All in memory.
     */
    buf = mprCreateBuf(ejs, MPR_BUFSIZE, -1);
    mprPutStringToBuf(buf, "<?xml version=\"1.0\"?>\n");

    /*
     * Convert XML to a string
     */
    if (ejsXMLToString(ejs, buf, xml, 0) < 0) {
        mprFree(buf);
        return 0;
    }

    file = mprOpen(ejs, filename,  O_CREAT | O_TRUNC | O_WRONLY | O_TEXT, 0664);
    if (file == 0) {
        ejsThrowIOError(ejs, "Can't open: %s, %d", filename,  mprGetOsError(ejs));
        return 0;
    }

    len = mprGetBufLength(buf);
    bytes = mprWrite(file, buf->start, len);
    if (bytes != len) {
        ejsThrowIOError(ejs, "Can't write to: %s", filename);
        mprFree(file);
        return 0;
    }
    mprWrite(file, "\n", 1);
    mprFree(buf);

    mprFree(file);

    return 0;
}


/*
 *  Convert the XML object to a string.
 *
 *  intrinsic function toString() : String
 */
static EjsVar *xmlToString(Ejs *ejs, EjsVar *vp, int argc, EjsVar **argv)
{
    return (vp->type->helpers->castVar)(ejs, vp, ejs->stringType);
}


/*
 *  Get the length of an array.
 *  @return Returns the number of items in the array
 *
 *  intrinsic public override function get length(): int
 */
static EjsVar *xmlLength(Ejs *ejs, EjsXML *xml, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, mprGetListCount(xml->elements));
}


#if UNUSED
/*
 *  Set the length. TODO - what does this do?
 *  intrinsic public override function set length(value: int): void
 */
static EjsVar *setLength(Ejs *ejs, EjsXML *xml, int argc, EjsVar **argv)
{
    int         length;

    mprAssert(ejsIsXML(xml));

    if (argc != 1) {
        ejsThrowArgError(ejs, "usage: obj.length = value");
        return 0;
    }
    length = ejsVarToInteger(ejs, argv[0]);

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
    return 0;
}
#endif

/********************************** Support **********************************/
/*
 *  Set an indexed element to an XML value
 */
EjsXML *ejsSetXML(Ejs *ejs, EjsXML *xml, int index, EjsXML *node)
{
    EjsXML      *old;

    if (xml == 0 || node == 0) {
        return 0;
    }

    if (xml->elements == 0) {
        xml->elements = mprCreateList(xml);

    } else {
        old = (EjsXML*) mprGetItem(xml->elements, index);
        if (old && old != node) {
            old->parent = 0;
        }
    }

    if (xml->kind != EJS_XML_LIST) {
        node->parent = xml;
    }

    if (mprSetItem(xml->elements, index, node) < 0) {
        return 0;
    }

    return xml;
}


#if UNUSED
int ejsCopyName(MprCtx ctx, EjsName *to, EjsName *from)
{
/*
 *
 *  TODO -
 *
    mprFree((char*) to->name);
    mprFree((char*) to->space);

    to->name = mprStrdup(ctx, from->name);
    to->space = mprStrdup(ctx, from->space);
    if (to->name == 0 || to->space == 0) {
        return EJS_ERR;
    }
*/
    *to = *from;

    return 0;
}
#endif


EjsXML *ejsAppendToXML(Ejs *ejs, EjsXML *xml, EjsXML *node)
{
    EjsXML      *elt;
    int         next;

    if (xml == 0 || node == 0) {
        return 0;
    }
    if (xml->elements == 0) {
        xml->elements = mprCreateList(xml);
    }

    if (node->kind == EJS_XML_LIST) {
        for (next = 0; (elt = mprGetNextItem(node->elements, &next)) != 0; ) {
            if (xml->kind != EJS_XML_LIST) {
                elt->parent = xml;
            }
            mprAddItem(xml->elements, elt);
        }
        xml->targetObject = node->targetObject;
        xml->targetProperty = node->targetProperty;

    } else {
        if (xml->kind != EJS_XML_LIST) {
            node->parent = xml;
        }
        mprAddItem(xml->elements, node);
    }

    return xml;
}


int ejsAppendAttributeToXML(Ejs *ejs, EjsXML *parent, EjsXML *node)
{
    if (parent->attributes == 0) {
        parent->attributes = mprCreateList(parent);
    }
    node->parent = parent;
    return mprAddItem(parent->attributes, node);
}


static int readFileData(MprXml *xp, void *data, char *buf, int size)
{
    mprAssert(xp);
    mprAssert(data);
    mprAssert(buf);
    mprAssert(size > 0);

    return mprRead((MprFile*) data, buf, size);
}


static int readStringData(MprXml *xp, void *data, char *buf, int size)
{
    EjsXmlState *parser;
    int         rc, len;

    mprAssert(xp);
    mprAssert(buf);
    mprAssert(size > 0);

    parser = (EjsXmlState*) xp->parseArg;

    if (parser->inputPos < parser->inputSize) {
        len = min(size, (parser->inputSize - parser->inputPos));
        rc = mprMemcpy(buf, size, &parser->inputBuf[parser->inputPos], len);
        parser->inputPos += len;
        return rc;
    }
    return 0;
}


static bool allDigitsForXml(cchar *name)
{
    cchar   *cp;

    for (cp = name; *cp; cp++) {
        if (!isdigit((int) *cp) || *cp == '.') {
            return 0;
        }
    }
    return 1;
}

/*********************************** Factory **********************************/

EjsXML *ejsCreateXML(Ejs *ejs, int kind, EjsName *qname, EjsXML *parent, cchar *value)
{
    EjsXML      *xml;

    xml = (EjsXML*) ejsAllocVar(ejs, ejs->xmlType, 0);
    if (xml == 0) {
        return 0;
    }

    if (qname) {
        xml->qname.name = mprStrdup(xml, qname->name);
        xml->qname.space = mprStrdup(xml, qname->space);
    }

    xml->kind = kind;
    xml->parent = parent;
    if (value) {
        xml->value = mprStrdup(xml, value);
    }
    //  TODO - should we create the elements list here?
    return xml;
}


EjsXML *ejsConfigureXML(Ejs *ejs, EjsXML *xml, int kind, cchar *name, EjsXML *parent, cchar *value)
{
    mprFree((char*) xml->qname.name);
    //  TODO - RC
    xml->qname.name = mprStrdup(xml, name);
    xml->kind = kind;
    xml->parent = parent;
    if (value) {
        mprFree(xml->value);
        //  TODO - RC
        xml->value = mprStrdup(xml, value);
    }
    return xml;
}


/*
 *  Support routine. Not an class method
 */
void ejsLoadXMLString(Ejs *ejs, EjsXML *xml, cchar *xmlString)
{
    EjsXmlState *parser;
    MprXml      *xp;

    xp = ejsCreateXmlParser(ejs, xml, "string");
    parser = mprXmlGetParseArg(xp);

    parser->inputBuf = xmlString;
    parser->inputSize = (int) strlen(xmlString);

    mprXmlSetInputStream(xp, readStringData, (void*) 0);

    if (mprXmlParse(xp) < 0 && !ejsHasException(ejs)) {
        ejsThrowSyntaxError(ejs, "Can't parse XML string: %s",  mprXmlGetErrorMsg(xp));
    }

    mprFree(xp);
}


int ejsCreateXMLType(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;

    type = ejsCreateCoreType(ejs, ejsName(&qname, EJS_INTRINSIC_NAMESPACE, "XML"), ejs->objectType, sizeof(EjsXML), ES_XML,
        ES_XML_NUM_CLASS_PROP, ES_XML_NUM_INSTANCE_PROP, EJS_ATTR_NATIVE | EJS_ATTR_HAS_CONSTRUCTOR);
    if (type == 0) {
        return EJS_ERR;
    }
    ejs->xmlType = type;
    type->nobind = 1;

    /*
     *  Define the helper functions.
     */
    type->helpers->cloneVar = (EjsCloneVarHelper) cloneXml;
    type->helpers->castVar = (EjsCastVarHelper) castXml;
    type->helpers->createVar = (EjsCreateVarHelper) createXml;
    type->helpers->destroyVar = (EjsDestroyVarHelper) destroyXml;
    type->helpers->getPropertyByName = (EjsGetPropertyByNameHelper) getXmlPropertyByName;
    type->helpers->getPropertyCount = (EjsGetPropertyCountHelper) getXmlPropertyCount;
    type->helpers->deletePropertyByName = (EjsDeletePropertyByNameHelper) deleteXmlPropertyByName;
    type->helpers->invokeOperator = (EjsInvokeOperatorHelper) invokeXmlOperator;
    type->helpers->markVar = (EjsMarkVarHelper) ejsMarkXML;
    type->helpers->setPropertyByName = (EjsSetPropertyByNameHelper) setXmlPropertyByName;
    
    return 0;
}


int ejsConfigureXMLType(Ejs *ejs)
{
    EjsType     *type;

    type = ejs->xmlType;

    /*
     *  Define the XML class methods
     */
    ejsBindMethod(ejs, type, ES_XML_XML, (EjsNativeFunction) xmlConstructor);
    ejsBindMethod(ejs, type, ES_XML_load, (EjsNativeFunction) loadXml);
    ejsBindMethod(ejs, type, ES_XML_save, (EjsNativeFunction) saveXml);
    ejsBindMethod(ejs, type, ES_XML_name, (EjsNativeFunction) getXmlNodeName);

    /*
     *  Override these methods
     */
    ejsBindMethod(ejs, type, ES_Object_length, (EjsNativeFunction) xmlLength);
    ejsBindMethod(ejs, type, ES_Object_toString, (EjsNativeFunction) xmlToString);

    ejsBindMethod(ejs, type, ES_Object_get, getXmlIterator);
    ejsBindMethod(ejs, type, ES_Object_getValues, getXmlValues);

#if FUTURE
    ejsBindMethod(ejs, type, ES_XML_parent, parent);
    ejsBindMethod(ejs, type, "valueOf", valueOf, NULL);
#endif
    return 0;
}


/******************************************************************************/
#else
void __ejsXMLDummy() {}
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
