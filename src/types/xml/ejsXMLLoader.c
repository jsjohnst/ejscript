/**
 *  ejsXMLLoader.c - Load and save XML data.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejs.h"

#if BLD_FEATURE_EJS_E4X

/****************************** Forward Declarations **************************/

static void indent(MprBuf *bp, int level);
static int  parserHandler(MprXml *xp, int state, cchar *tagName, cchar *attName, cchar *value);

/************************************* Code ***********************************/

MprXml *ejsCreateXmlParser(Ejs *ejs, EjsXML *xml, cchar *filename)
{
    EjsXmlState *parser;
    MprXml      *xp;
    
    xp = mprXmlOpen(ejs, MPR_BUFSIZE, EJS_E4X_BUF_MAX);
    mprAssert(xp);

    /*
     *  Create the parser stack
     */
    parser = mprAllocObjZeroed(xp, EjsXmlState);
    if (parser == 0) {
        mprFree(xp);
        return 0;
    }
    parser->ejs = ejs;
    parser->nodeStack[0].obj = xml;
    
    //  TODO - these 2 are not really needed. Can use ejs->
    parser->xmlType = ejs->xmlType;
    parser->xmlListType = ejs->xmlListType;
    parser->filename = filename;

    mprXmlSetParseArg(xp, parser);
    mprXmlSetParserHandler(xp, parserHandler);

    return xp;
}



/*
 *  XML parsing callback. Called for each elt and attribute/value pair. 
 *  For speed, we handcraft the object model here rather than calling 
 *  putXmlProperty.
 *
 *  "<!-- txt -->"      parserHandler(, , MPR_XML_COMMENT);
 *  "<elt"              parserHandler(, , MPR_XML_NEW_ELT);
 *  "...att=value"      parserHandler(, , MPR_XML_NEW_ATT);
 *  "<elt ...>"         parserHandler(, , MPR_XML_ELT_DEFINED);
 *  "<elt/>"            parserHandler(, , MPR_XML_SOLO_ELT_DEFINED);
 *  "<elt> ...<"        parserHandler(, , MPR_XML_ELT_DATA);
 *  "...</elt>"         parserHandler(, , MPR_XML_END_ELT);
 *
 *  Note: we recurse on every new nested elt.
 */

static int parserHandler(MprXml *xp, int state, cchar *tagName, cchar *attName, cchar *value)
{
    Ejs             *ejs;
    EjsXmlState     *parser;
    EjsXmlTagState  *tos;
    EjsName         qname;
    EjsXML          *xml, *node, *parent;

    parser = (EjsXmlState*) xp->parseArg;
    ejs = parser->ejs;
    tos = &parser->nodeStack[parser->topOfStack];
    xml = tos->obj;
    
    mprAssert(xml);

    mprAssert(state >= 0);
    mprAssert(tagName && *tagName);

    switch (state) {
    case MPR_XML_PI:
        node = ejsCreateXML(ejs, EJS_XML_PROCESSING, NULL, xml, value);
        ejsAppendToXML(ejs, xml, node);
        break;

    case MPR_XML_COMMENT:
        node = ejsCreateXML(ejs, EJS_XML_COMMENT, NULL, xml, value);
        ejsAppendToXML(ejs, xml, node);
        break;

    case MPR_XML_NEW_ELT:
        if (parser->topOfStack > E4X_MAX_NODE_DEPTH) {
            ejsThrowSyntaxError(ejs,  "XML nodes nested too deeply in %s at line %d", parser->filename, mprXmlGetLineNumber(xp));
            return MPR_ERR_BAD_SYNTAX;
        }
        if (xml->kind <= 0) {
            ejsConfigureXML(ejs, xml, EJS_XML_ELEMENT, tagName, xml, NULL);
        } else {
            ejsName(&qname, 0, tagName);
            xml = ejsCreateXML(ejs, EJS_XML_ELEMENT, &qname, xml, NULL);
            tos = &parser->nodeStack[++(parser->topOfStack)];
            tos->obj = (EjsXML*) xml;
            tos->attributes = 0;
            tos->comments = 0;
        }
        break;

    case MPR_XML_NEW_ATT:
        ejsName(&qname, 0, attName);
        node = ejsCreateXML(ejs, EJS_XML_ATTRIBUTE, &qname, xml, value);
        //  TODO - rc
        ejsAppendAttributeToXML(ejs, xml, node);
        //  TODO RC
        break;

    case MPR_XML_SOLO_ELT_DEFINED:
        if (parser->topOfStack > 0) {
            parent = parser->nodeStack[parser->topOfStack - 1].obj;
            //  TODO - rc
            ejsAppendToXML(ejs, parent, xml);
            parser->topOfStack--;
            mprAssert(parser->topOfStack >= 0);
            tos = &parser->nodeStack[parser->topOfStack];
        }
        break;

    case MPR_XML_ELT_DEFINED:
        if (parser->topOfStack > 0) {
            parent = parser->nodeStack[parser->topOfStack - 1].obj;
            //  TODO - rc
            ejsAppendToXML(ejs, parent, xml);
        }
        break;

    case MPR_XML_ELT_DATA:
    case MPR_XML_CDATA:
        ejsName(&qname, 0, attName);
        node = ejsCreateXML(ejs, EJS_XML_TEXT, &qname, xml, value);
        //  TODO - rc
        ejsAppendToXML(ejs, xml, node);
        break;

    case MPR_XML_END_ELT:
        /*
         *  This is the closing element in a pair "<x>...</x>".
         *  Pop the stack frame off the elt stack
         */
        if (parser->topOfStack > 0) {
            parser->topOfStack--;
            mprAssert(parser->topOfStack >= 0);
            tos = &parser->nodeStack[parser->topOfStack];
        }
        break;

    default:
        ejsThrowSyntaxError(ejs, "XML error in %s at %d\nDetails %s", parser->filename, mprXmlGetLineNumber(xp), mprXmlGetErrorMsg(xp));
        mprAssert(0);
        return MPR_ERR_BAD_SYNTAX;
    }
    return 0;
}



#if UNUSED
static bool checkTagName(char *name)
{
    char    *cp;

    for (cp = name; *cp; cp++) {
        if (!isalnum(*cp) && *cp != '_' && *cp != '$' && *cp != '@') {
            return 0;
        }
    }
    return 1;
}
#endif



int ejsXMLToString(Ejs *ejs, MprBuf *buf, EjsXML *node, int indentLevel)
{
    EjsXML      *xml, *child, *attribute, *elt;
    int         sawElements, next;
    
    if (node->var.visited) {
        return 0;
    }
    node->var.visited = 1;

    if (node->kind == EJS_XML_LIST) {
        for (next = 0; (elt = mprGetNextItem(node->elements, &next)) != 0; ) {
            ejsXMLToString(ejs, buf, elt, indentLevel);
        }
        return 0;
    }
    
    mprAssert(ejsIsXML(node));
    xml = (EjsXML*) node;
    
    switch (xml->kind) {
    case EJS_XML_ELEMENT:
        /*
         *  XML object is complex (has elements) so return full XML content.
         */
        if (indentLevel > 0) {
            mprPutCharToBuf(buf, '\n');
        }
        indent(buf, indentLevel);

        mprPutFmtToBuf(buf, "<%s", xml->qname.name);
        if (xml->attributes) {
            for (next = 0; (attribute = mprGetNextItem(xml->attributes, &next)) != 0; ) {
                mprPutFmtToBuf(buf, " %s=\"%s\"",  attribute->qname.name, attribute->value);
            }
        }
        
        sawElements = 0;
        if (xml->elements) {
            mprPutStringToBuf(buf, ">"); 
            for (next = 0; (child = mprGetNextItem(xml->elements, &next)) != 0; ) {
                if (child->kind != EJS_XML_TEXT) {
                    sawElements++;
                }
    
                /* Recurse */
                if (ejsXMLToString(ejs, buf, child, indentLevel < 0 ? -1 : indentLevel + 1) < 0) {
                    return -1;
                }
            }
            if (sawElements && indentLevel >= 0) {
                mprPutCharToBuf(buf, '\n');
                indent(buf, indentLevel);
            }
            mprPutFmtToBuf(buf, "</%s>", xml->qname.name);
            
        } else {
            /* Solo */
            mprPutStringToBuf(buf, "/>");
        }
        break;
        
    case EJS_XML_COMMENT:
        mprPutCharToBuf(buf, '\n');
        indent(buf, indentLevel);
        mprPutFmtToBuf(buf, "<!--%s -->", xml->value);
        break;
        
    case EJS_XML_ATTRIBUTE:
        /*
         *  Only here when converting solo attributes to a string
         */
        mprPutStringToBuf(buf, xml->value);
        break;
        
    case EJS_XML_TEXT:
        mprPutStringToBuf(buf, xml->value);
        break;
    }
    
    node->var.visited = 0;
    
    return 0;
}



static void indent(MprBuf *bp, int level)
{
    int     i;

    for (i = 0; i < level; i++) {
        mprPutCharToBuf(bp, '\t');
    }
}


/******************************************************************************/
#else
void __ejsXMLLoaderDummy() {}
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
