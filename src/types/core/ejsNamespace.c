/**
 *  ejsNamespace.c - Ejscript Namespace class
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejs.h"

/************************************* Code ***********************************/
/*
 *  Cast the operand to the specified type
 */

static EjsVar *castNamespace(Ejs *ejs, EjsNamespace *vp, EjsType *type)
{
    switch (type->id) {
    case ES_Boolean:
        return (EjsVar*) ejsCreateBoolean(ejs, 1);

    case ES_String:
        return (EjsVar*) ejsCreateString(ejs, "[object Namespace]");

    default:
        ejsThrowTypeError(ejs, "Can't cast to this type");
        return 0;
    }
}


static EjsVar *invokeNamespaceOperator(Ejs *ejs, EjsNamespace *lhs, int opCode, EjsNamespace *rhs)
{
    bool        boolResult;

    switch (opCode) {
    case EJS_OP_COMPARE_EQ:
        boolResult = (strcmp(lhs->name, rhs->name) == 0 && strcmp(lhs->uri, rhs->uri) == 0);
        break;

    case EJS_OP_COMPARE_STRICTLY_EQ:
        boolResult = lhs == rhs;
        break;

    case EJS_OP_COMPARE_NE:
        boolResult = ! (strcmp(lhs->name, rhs->name) == 0 && strcmp(lhs->uri, rhs->uri) == 0);
        break;

    case EJS_OP_COMPARE_STRICTLY_NE:
        boolResult = !(lhs == rhs);
        break;

    default:
        ejsThrowTypeError(ejs, "Operation is not valid on this type");
        return 0;
    }
    return (EjsVar*) ejsCreateBoolean(ejs, boolResult);
}


/*
 *  Define a reserved namespace in a block.
 */
EjsNamespace *ejsDefineReservedNamespace(Ejs *ejs, EjsBlock *block, EjsName *typeName, cchar *spaceName)
{
    EjsNamespace    *namespace;

    namespace = ejsCreateReservedNamespace(ejs, typeName, spaceName);
    if (namespace) {
        if (ejsAddNamespaceToBlock(ejs, block, namespace) < 0) {
            return 0;
        }
    }
    return namespace;
}


/*
 *  Format a reserved namespace to create a unique namespace URI. "internal, public, private, protected"
 *
 *  Namespaces are formatted as strings using the following format, where type is optional. Types may be qualified.
 *      [type,space]
 *
 *  Example:
 *      [debug::Shape,public] where Shape was declared as "debug class Shape"
 */
char *ejsFormatReservedNamespace(MprCtx ctx, EjsName *typeName, cchar *spaceName)
{
    cchar   *typeNameSpace;
    char    *namespace, *sp;
    int     len, typeLen, spaceLen, l;

    len = typeLen = spaceLen = 0;
    typeNameSpace = 0;

    if (typeName) {
        if (typeName->name == 0) {
            typeName = 0;
        }
        typeNameSpace = typeName->space ? typeName->space : EJS_PUBLIC_NAMESPACE;
    }

    if (typeName && typeName->name) {
        //  Join the qualified typeName to be "space::name"
        mprAssert(typeName->name);
        typeLen = (int) strlen(typeNameSpace);
        typeLen += 2 + (int) strlen(typeName->name);          //  Allow for the "::" between space::name
        len += typeLen;
    }
    spaceLen = (int) strlen(spaceName);

    /*
     *  Add 4 for [,,]
     *  Add 2 for the trailing "::" and one for the null
     */
    len += 4 + spaceLen + 2 + 1;

    namespace = mprAlloc(ctx, len);
    if (namespace == 0) {
        return 0;
    }

    sp = namespace;
    *sp++ = '[';

    if (typeName) {
        if (strcmp(typeNameSpace, EJS_PUBLIC_NAMESPACE) != 0) {
            l = (int) strlen(typeNameSpace);
            strcpy(sp, typeNameSpace);
            sp += l;
            *sp++ = ':';
            *sp++ = ':';
        }
        l = (int) strlen(typeName->name);
        strcpy(sp, typeName->name);
        sp += l;
    }

    *sp++ = ',';
    strcpy(sp, spaceName);
    sp += spaceLen;

    *sp++ = ']';
    *sp = '\0';

    mprAssert(sp <= &namespace[len]);

    return namespace;
}


/*********************************** Factory **********************************/
/*
 *  Create a namespace with the given URI as its definition qualifying value.
 */
EjsNamespace *ejsCreateNamespace(Ejs *ejs, cchar *name, cchar *uri)
{
    EjsNamespace    *np;

    if (uri == 0) {
        uri = name;
    } else if (name == 0) {
        name = uri;
    }

    np = (EjsNamespace*) ejsCreateVar(ejs, ejs->namespaceType, 0);
    if (np) {
        np->name = (char*) name;
        np->uri = (char*) uri;
    }
    ejsSetDebugName(np, np->uri);
    return np;
}


/*
 *  Create a reserved namespace. Format the package, type and space names to create a unique namespace URI.
 *  packageName, typeName and uri are optional.
 */
EjsNamespace *ejsCreateReservedNamespace(Ejs *ejs, EjsName *typeName, cchar *spaceName)
{
    EjsNamespace    *namespace;
    char            *formattedName;

    mprAssert(spaceName);

    if (typeName) {
        formattedName = (char*) ejsFormatReservedNamespace(ejs, typeName, spaceName);
    } else {
        formattedName = (char*) spaceName;
    }

    namespace = ejsCreateNamespace(ejs, formattedName, formattedName);

    return namespace;
}


void ejsCreateNamespaceType(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;

    type = ejsCreateCoreType(ejs, ejsName(&qname, EJS_INTRINSIC_NAMESPACE, "Namespace"), ejs->objectType, sizeof(EjsNamespace),
        ES_Namespace, ES_Namespace_NUM_CLASS_PROP, ES_Namespace_NUM_INSTANCE_PROP, EJS_ATTR_NATIVE);
    ejs->namespaceType = type;
    
    /*
     *  Define the helper functions.
     */
    //  TODO - need to provide lookupProperty to access name and uri
    type->helpers->castVar = (EjsCastVarHelper) castNamespace;
    type->helpers->invokeOperator = (EjsInvokeOperatorHelper) invokeNamespaceOperator;
}


void ejsConfigureNamespaceType(Ejs *ejs)
{
    ejsSetProperty(ejs, ejs->global, ES_intrinsic, (EjsVar*) ejs->intrinsicSpace);
    ejsSetProperty(ejs, ejs->global, ES_iterator, (EjsVar*) ejs->iteratorSpace);
    ejsSetProperty(ejs, ejs->global, ES_public, (EjsVar*) ejs->publicSpace);
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
