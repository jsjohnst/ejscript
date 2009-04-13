/**
 *  ejsGlobal.c - Global functions and variables
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejs.h"

/***************************** Forward Declarations ***************************/

static EjsVar *parseLiteral(Ejs *ejs, char **next);

/*********************************** Locals ***********************************/
/**
 *  Assert a condition is true.
 *
 *  public static function assert(condition: Boolean): Boolean
 */
static EjsVar *assertMethod(Ejs *ejs, EjsVar *vp, int argc, EjsVar **argv)
{
    EjsBoolean      *b;

    mprAssert(argc == 1);

    if (! ejsIsBoolean(argv[0])) {
        b = (EjsBoolean*) ejsCastVar(ejs, argv[0], ejs->booleanType);
    } else {
        b = (EjsBoolean*) argv[0];
    }
    mprAssert(b);

    if (b == 0 || !b->value) {
#if BLD_DEBUG
        if (ejs->frame->currentLine) {
            mprLog(ejs, 0, "Assertion error: %s", ejs->frame->currentLine);
            ejsThrowAssertError(ejs, "Assertion error: %s", ejs->frame->currentLine);
        } else
#endif
            ejsThrowAssertError(ejs, "Assertion error");
        return 0;
    }
    return vp;
}


#if MOVE_TO_DEBUG_CLASS || 1
/**
 *  Trap to the debugger
 *
 *  public static function breakpoint(): Void
 */
static EjsVar *breakpoint(Ejs *ejs, EjsVar *vp, int argc, EjsVar **argv)
{
    mprBreakpoint();
    return 0;
}
#endif


/**
 *  Clone the base class. Used by Record.es
 *  TODO - where to move this?
 *
 *  public static function cloneBase(klass: Type): Void
 */
static EjsVar *cloneBase(Ejs *ejs, EjsVar *ignored, int argc, EjsVar **argv)
{
    EjsType     *type;
    
    mprAssert(argc == 1);
    
    type = (EjsType*) argv[0];
    type->baseType = (EjsType*) ejsCloneVar(ejs, (EjsVar*) type->baseType, 0);
    ejsSetReference(ejs, (EjsVar*) type, (EjsVar*) type->baseType);
    return 0;
}



//  TODO - merge with parse() to handle all types
//  TODO - this parser is not robust enough. Need lookahead - rewrite.
EjsVar *ejsDeserialize(Ejs *ejs, EjsVar *value)
{
    EjsVar      *obj;
    char        *str, *next;

    if (!ejsIsString(value)) {
        return 0;
    }
    str = ejsGetString(value);
    if (str == 0) {
        return 0;
    } else if (*str == '\0') {
        return (EjsVar*) ejs->emptyStringValue;
    }

    next = str;
    if ((obj = parseLiteral(ejs, &next)) == 0) {
        ejsThrowSyntaxError(ejs, "Can't parse object literal");
        return 0;
    }
    return obj;
}


/*
 *  Convert a string into an object.
 *
 *  intrinsic native static function deserialize(obj: String): Object
 */
EjsVar *deserialize(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    mprAssert(argc == 1 && ejsIsString(argv[0]));
    return ejsDeserialize(ejs, argv[0]);
}


typedef enum Token {
    TOK_EOF,
    TOK_LBRACE,
    TOK_LBRACKET,
    TOK_RBRACE,
    TOK_RBRACKET,
    TOK_COLON,
    TOK_COMMA,
    TOK_ID,
    TOK_QID,            /* QUOTED */
} Token;

//  TODO - get more unique name for all-in-one

Token getToken(char *token, char **next)
{
    char    *cp, *start, *firstQuote, *src, *dest;
    int     quote, tid;

    for (cp = *next; isspace((int) *cp); cp++) {
        ;
    }

    *next = cp + 1;
    firstQuote = 0;

    if (*cp == '\0') {
        tid = TOK_EOF;

    } else  if (*cp == '{') {
        tid = TOK_LBRACE;

    } else if (*cp == '[') {
        tid = TOK_LBRACKET;

    } else if (*cp == '}' || *cp == ']') {
        tid = *cp == '}' ? TOK_RBRACE: TOK_RBRACKET;
        while (*++cp && isspace((int) *cp)) {
            ;
        }
        if (*cp == ',' || *cp == ':') {
            cp++;
        }
        *next = cp;

    } else if (*cp == ':') {
        tid = TOK_COLON;

    } else {
        quote = 0;
        start = cp;
        for (start = cp; *cp; cp++) {
            if (*cp == '\\') {
                cp++;
            } else if (*cp == '\'' || *cp == '\"') {
                if (quote) {
                    if (*cp == quote && cp > firstQuote && cp[-1] != '\\') {
                        *cp = '\0';
                        quote = 0;
                    }
                } else {
                    firstQuote = cp;
                    start = cp + 1;
                    quote = *cp;
                }
                continue;
            }
            if (!quote && (*cp == ',' || *cp == ':')) {
                break;
            }
            if (!quote && (*cp == ']' || *cp == '}')) {
                break;
            }
        }
        if (quote && *start == quote) {
            /* No matching end */
            start--;
        }
        strncpy(token, start, min(MPR_MAX_STRING, cp - start));
        token[cp - start] = '\0';
        if (*cp == ',' || *cp == ':') {
            cp++;
        }
        *next = cp;

        for (dest = src = token; *src; ) {
            if (*src == '\\') {
                src++;
            }
            *dest++ = *src++;
        }
        *dest = '\0';

        tid = (firstQuote) ? TOK_QID : TOK_ID;
    }
    return tid;
}


/*
 *  Parse an object literal string pointed to by *next into the given object. Update *next to point
 *  to the next input token in the object literal. Supports nested object literals.
 *  TODO - should be rewritten to handle unbounded key and value tokens
 */
static EjsVar *parseLiteral(Ejs *ejs, char **next)
{
    EjsName     qname;
    EjsVar      *obj, *vp;
    char        *kp, *prior;
    char        token[MPR_MAX_STRING], key[MPR_MAX_STRING], value[MPR_MAX_STRING];
    int         tid, isArray;

    isArray = 0;

    tid = getToken(token, next);
    if (tid == TOK_EOF) {
        return 0;
    }

    if (tid == TOK_LBRACKET) {
        isArray = 1;
        obj = (EjsVar*) ejsCreateArray(ejs, 0);

    } else if (tid == TOK_LBRACE) {
        obj = (EjsVar*) ejsCreateObject(ejs, ejs->objectType, 0);

    } else {
        return ejsParseVar(ejs, token, -1);
    }
    if (obj == 0) {
        ejsThrowMemoryError(ejs);
        return 0;
    }

    while (1) {
        prior = *next;
        tid = getToken(key, next);
        if (tid == TOK_EOF) {
            return obj;
        } else if (tid == TOK_RBRACE || tid == TOK_RBRACKET) {
            break;
        }

        if (tid == TOK_LBRACKET) {
            *next = prior;
            vp = parseLiteral(ejs, next);

        } else if (tid == TOK_LBRACE) {
            *next = prior;
            vp = parseLiteral(ejs, next);

        } else if (isArray) {
            vp = ejsParseVar(ejs, key, -1);

        } else {
            //  TODO - really need a peek
            prior = *next;
            tid = getToken(value, next);
            if (tid == TOK_LBRACE || tid == TOK_LBRACKET) {
                *next = prior;
                vp = parseLiteral(ejs, next);

            } else if (tid != TOK_ID && tid != TOK_QID) {
                return 0;

            } else {
                if (tid == TOK_QID) {
                    /* Quoted */
                    vp = (EjsVar*) ejsCreateString(ejs, value);
                } else {
                    if (strcmp(value, "null") == 0) {
                        vp = ejs->nullValue;
                    } else if (strcmp(value, "undefined") == 0) {
                        vp = ejs->undefinedValue;
                    } else {
                        vp = ejsParseVar(ejs, value, -1);
                    }
                }
            }
        }
        if (vp == 0) {
            return 0;
        }

        if (isArray) {
            if (ejsSetProperty(ejs, obj, -1, vp) < 0) {
                return 0;
            }

        } else {
            //  TODO - bug. Could leak if this object is put back on the object type pool and reused in this manner.
            kp = mprStrdup(obj, key);
            ejsName(&qname, EJS_PUBLIC_NAMESPACE, kp);
            if (ejsSetPropertyByName(ejs, obj, &qname, vp) < 0) {
                return 0;
            }
        }

    }
    return obj;
}


/**
 *  Format the stack
 *
 *  public static function formatStack(): String
 */
static EjsVar *formatStackMethod(Ejs *ejs, EjsVar *vp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateString(ejs, ejsFormatStack(ejs));
}


#if ES_hashcode
/*
 *  Get the hash code for the object.
 *
 *  intrinsic function hashcode(o: Object): Number
 */
static EjsVar *hashcode(Ejs *ejs, EjsVar *vp, int argc, EjsVar **argv)
{
    mprAssert(argc == 1);
    return (EjsVar*) ejsCreateNumber(ejs, (MprNumber) PTOL(argv[0]));
}
#endif


/**
 *  Load a script or module. Name should have an extension. Name will be located according to the EJSPATH search strategy.
 *
 *  public static function load(name: String): void
 */
static EjsVar *load(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    cchar       *name, *cp;
    char        *path;

    name = ejsGetString(argv[0]);

    if ((cp = strrchr(name, '.')) != NULL && strcmp(cp, ".es") == 0) {
        ejsThrowIOError(ejs, "load: Compiling is not enabled for %s", name);

    } else {
        if (ejsSearch(ejs, &path, name) < 0) {
            ejsThrowIOError(ejs, "Can't find %s to load", name);
        } else {
            ejsLoadModule(ejs, path, NULL, NULL, 0);
        }
    }
    return 0;
}


/**
 *  Parse the input and convert to a primitive type
 *
 *  public static function parse(input: String, preferredType: Type = null): void
 */
static EjsVar *parse(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    cchar       *input;
    int         preferred;

    input = ejsGetString(argv[0]);

    if (argc == 2 && !ejsIsType(argv[1])) {
        ejsThrowArgError(ejs, "Argument is not a type");
        return 0;
    }
    preferred = (argc == 2) ? ((EjsType*) argv[1])->id : -1;

    return ejsParseVar(ejs, input, preferred);
}


/**
 *  Print the arguments to the standard error with a new line.
 *
 *  public static function eprint(...args): void
 */
static EjsVar *eprintMethod(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    EjsString   *s;
    EjsVar      *args, *vp;
    int         i, count;

    mprAssert(argc == 1 && ejsIsArray(argv[0]));

    args = argv[0];
    count = ejsGetPropertyCount(ejs, args);

    for (i = 0; i < count; i++) {
        vp = ejsGetProperty(ejs, args, i);
        if (vp == 0) {
            s = 0;
        } else {
            s = (EjsString*) ejsToString(ejs, vp);
            if (ejs->exception) {
                return 0;
            }
        }
        if (s) {
            mprErrorPrintf(ejs, "%s", s->value);
        }
    }
    mprErrorPrintf(ejs, "\n");
    return 0;
}


/**
 *  Print the arguments to the standard output with a new line.
 *
 *  public static function print(...args): void
 */
static EjsVar *printMethod(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    EjsString   *s;
    EjsVar      *args, *vp;
    int         i, count;

    mprAssert(argc == 1 && ejsIsArray(argv[0]));

    args = argv[0];
    count = ejsGetPropertyCount(ejs, args);

    for (i = 0; i < count; i++) {
        vp = ejsGetProperty(ejs, args, i);
        if (vp == 0) {
            s = 0;
        } else {
            s = (EjsString*) ejsToString(ejs, vp);
            if (ejs->exception) {
                return 0;
            }
        }
        if (s) {
            mprPrintf(ejs, "%s", s->value);
        }
    }
    mprPrintf(ejs, "\n");
    return 0;
}


#if ES_printv && BLD_DEBUG
/**
 *  Print the named variables for debugging.
 *
 *  public static function printv(...args): void
 */
static EjsVar *printv(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    EjsString   *s;
    EjsVar      *args, *vp;
    int         i, count;

    mprAssert(argc == 1 && ejsIsArray(argv[0]));

    args = argv[0];
    count = ejsGetPropertyCount(ejs, args);

    for (i = 0; i < count; i++) {
        vp = ejsGetProperty(ejs, args, i);
        if (vp == 0) {
            continue;
        }

        s = (EjsString*) ejsToString(ejs, vp);

        if (ejs->exception) {
            return 0;
        }

        mprAssert(s && ejsIsString(s));
        mprPrintf(ejs, "%s = %s\n", vp->debugName, s->value);
    }
    mprPrintf(ejs, "\n");
    return 0;
}
#endif


EjsVar *ejsSerialize(Ejs *ejs, EjsVar *vp, int maxDepth, bool showAll, bool showBase)
{
    EjsString       *result;
    EjsVar          *pp;
    EjsObject       *obj;
    EjsString       *sv;
    EjsBlock        *block;
    MprBuf          *buf;
    EjsName         qname;
    char            key[16], *cp;
    int             i, slotNum, numInherited, flags, isArray, count;

    if (maxDepth == 0) {
        maxDepth = MAXINT;
    }
    flags = 0;
    if (showAll) {
        flags |= EJS_FLAGS_ENUM_ALL;
    }
    if (showBase) {
        flags |= EJS_FLAGS_ENUM_INHERITED;
    }

    buf = mprCreateBuf(vp, 0, 0);
    if (buf == 0) {
        ejsThrowMemoryError(ejs);
        return 0;
    }

    /*
     *  TODO - this whole strategy is flawed. Need a serialize helper per object
     */
    count = ejsGetPropertyCount(ejs, vp);

    if (count == 0 || ejsIsXML(vp)) {

        if (ejsIsFunction(vp)) {
            mprPutStringToBuf(buf, "[function]");

        } else if (ejsIsString(vp) || ejsIsXML(vp)) {
            mprPutCharToBuf(buf, '\"');
            sv = (EjsString*) ejsToString(ejs, vp);
            if (strchr(sv->value, '"')) {
                for (cp = sv->value; cp < &sv->value[sv->length]; cp++) {
                    if (*cp == '"') {
                        mprPutCharToBuf(buf, '\\');
                        mprPutCharToBuf(buf, *cp);
                    } else {
                        mprPutCharToBuf(buf, *cp);
                    }
                }
            } else {
                mprPutStringToBuf(buf, sv->value);
            }
            mprPutCharToBuf(buf, '\"');

        } else if (ejsIsObject(vp)) {
            mprPutStringToBuf(buf, "{}");

        } else {
            sv = (EjsString*) ejsToString(ejs, vp);
            mprPutStringToBuf(buf, sv->value);
        }

    } else {

        if (vp->visited) {
            return (EjsVar*) ejsCreateString(ejs, "this");
        }

        isArray = ejsIsArray(vp);
        mprPutStringToBuf(buf, isArray ? "[\n" : "{\n");

        vp->visited = 1;
        if (++ejs->serializeDepth <= maxDepth /* && ejsIsObject(vp) */) {

            obj = (EjsObject*) vp;

            for (slotNum = 0; slotNum < count; slotNum++) {
                if (ejsIsBlock(vp)) {
                    block = (EjsBlock*) vp;
                    numInherited = ejsGetNumInheritedTraits((EjsBlock*) obj);
                    if (slotNum < numInherited && !(flags & EJS_FLAGS_ENUM_INHERITED)) {
                        continue;
                    }
                }
                pp = ejsGetProperty(ejs, vp, slotNum);
                
                if (pp == 0 || (pp->hidden && !(flags & EJS_FLAGS_ENUM_ALL))) {
                    if (ejs->exception) {
                        return 0;
                    }
                    continue;
                }
                if (isArray) {
                    mprItoa(key, sizeof(key), slotNum, 10);
                    qname.name = key;
                    qname.space = "";
                } else {
                    qname = ejsGetPropertyName(ejs, vp, slotNum);
                }
                for (i = 0; i < ejs->serializeDepth; i++) {
                    mprPutStringToBuf(buf, "  ");
                }
                if (!isArray) {
                    if (flags & EJS_FLAGS_ENUM_ALL) {
                        mprPutFmtToBuf(buf, "%s_%s: ", qname.space, qname.name);
                    } else {
                        mprPutFmtToBuf(buf, "%s: ", qname.name);
                    }
                }

                sv = (EjsString*) ejsSerialize(ejs, pp, maxDepth, showAll, showBase);
                if (sv == 0 || !ejsIsString(sv)) {
                    if (!ejs->exception) {
                        ejsThrowTypeError(ejs, "Cant serialize property %s", qname.name);
                    }
                    return 0;

                } else {
                    mprPutStringToBuf(buf, sv->value);
                }

                mprPutStringToBuf(buf, ",\n");
            }
        }
        vp->visited = 0;

        ejs->serializeDepth--;
        for (i = 0; i < ejs->serializeDepth; i++) {
            mprPutStringToBuf(buf, "  ");
        }
        mprPutCharToBuf(buf, isArray ? ']' : '}');
    }

    mprAddNullToBuf(buf);
    result = ejsCreateString(ejs, mprGetBufStart(buf));
    mprFree(buf);

    return (EjsVar*) result;
}


/*
 *  Convert the object to a source code string.
 *
 *  intrinsic function serialize(obj: Object, maxDepth: Number = 0, showAll: Boolean = false, 
 *      showBase: Boolean = false): String
 */
EjsVar *serialize(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    EjsVar          *vp;
    int             flags, maxDepth;
    bool            showBase, showAll;

    flags = 0;
    maxDepth = MAXINT;

    vp = argv[0];

    if (argc >= 2) {
        maxDepth = ejsGetInt(argv[1]);
    }

    showAll = (argc >= 3 && argv[2] == (EjsVar*) ejs->trueValue);
    showBase = (argc == 4 && argv[3] == (EjsVar*) ejs->trueValue);
    return ejsSerialize(ejs, argv[0], maxDepth, showAll, showBase);
}


static EjsNamespace *addNamespace(Ejs *ejs, EjsBlock *block, cchar *space)
{
    EjsNamespace    *ns;

    ns = ejsDefineReservedNamespace(ejs, block, 0, space);
    mprAddHash(ejs->standardSpaces, space, ns);

    return ns;
}


void ejsCreateGlobalBlock(Ejs *ejs)
{
    EjsBlock    *block;

    /*
     *  Pre-create extra global slots
     */
    ejs->globalBlock = ejsCreateBlock(ejs, EJS_GLOBAL, max(ES_global_NUM_CLASS_PROP, 256));
    ejs->global = (EjsVar*) ejs->globalBlock;
    
    if ((ejs->flags & EJS_FLAG_COMPILER) && (ejs->flags & EJS_FLAG_EMPTY)) {
        ejs->globalBlock->obj.numProp = 0;
    } else {
        ejs->globalBlock->obj.numProp = ES_global_NUM_CLASS_PROP;
    }
    
    block = (EjsBlock*) ejs->global;
    
    /*
     *  Create the standard namespaces. Order matters here. This is the (reverse) order of lookup.
     */
    ejs->emptySpace =       addNamespace(ejs, block, EJS_EMPTY_NAMESPACE);
    ejs->configSpace =      addNamespace(ejs, block, EJS_CONFIG_NAMESPACE);
    ejs->iteratorSpace =    addNamespace(ejs, block, EJS_ITERATOR_NAMESPACE);
    ejs->intrinsicSpace =   addNamespace(ejs, block, EJS_INTRINSIC_NAMESPACE);
    ejs->eventsSpace =      addNamespace(ejs, block, EJS_EVENTS_NAMESPACE);
    ejs->ioSpace =          addNamespace(ejs, block, EJS_IO_NAMESPACE);
    ejs->sysSpace =         addNamespace(ejs, block, EJS_SYS_NAMESPACE);
    ejs->publicSpace =      addNamespace(ejs, block, EJS_PUBLIC_NAMESPACE);
}


void ejsConfigureGlobalBlock(Ejs *ejs)
{
    EjsBlock    *block;

    block = ejs->globalBlock;
    mprAssert(block);

#if ES_assert
    ejsBindFunction(ejs, block, ES_assert, assertMethod);
#endif
#if ES_breakpoint
    ejsBindFunction(ejs, block, ES_breakpoint, breakpoint);
#endif
#if ES_cloneBase
    ejsBindFunction(ejs, block, ES_cloneBase, (EjsNativeFunction) cloneBase);
#endif
#if ES_deserialize
    ejsBindFunction(ejs, block, ES_deserialize, deserialize);
#endif
#if ES_exit
    ejsBindFunction(ejs, block, ES_exit, exitMethod);
#endif
#if ES_formatStack
    ejsBindFunction(ejs, block, ES_formatStack, formatStackMethod);
#endif
#if ES_hashcode
    ejsBindFunction(ejs, block, ES_hashcode, hashcode);
#endif
#if ES_load
    ejsBindFunction(ejs, block, ES_load, load);
#endif
#if ES_parse
    ejsBindFunction(ejs, block, ES_parse, parse);
#endif
#if ES_eprint
    ejsBindFunction(ejs, block, ES_eprint, eprintMethod);
#endif
#if ES_print
    ejsBindFunction(ejs, block, ES_print, printMethod);
#endif
#if ES_printv && BLD_DEBUG
    ejsBindFunction(ejs, block, ES_printv, printv);
#endif
#if ES_serialize
    ejsBindFunction(ejs, block, ES_serialize, serialize);
#endif
    /*
     *  Update the global reference
     */
    ejsSetProperty(ejs, ejs->global, ES_global, ejs->global);
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
