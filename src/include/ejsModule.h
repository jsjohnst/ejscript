/*
 *  ejsModule.h - Module file format definition.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#ifndef _h_EJS_MODULE
#define _h_EJS_MODULE 1

#include    "ejsCore.h"

#ifdef __cplusplus
extern "C" {
#endif

/********************************* Prototypes *********************************/
/*
 *  Module File Format and Layout:
 *
 *      Notes:
 *          - (N) Numbers are 1-3 little-endian encoded bytes using the 0x80
 *            as the continuation character
 *          - (S) Strings are pointers into the constant pool encoded as
 *            number offsets.
 *          - (T) Types are encoded and ored with the type encoding masks
 *            below. Types are either: untyped, unresolved or primitive (builtin).
 *            The relevant mask is ored into the lower 2 bits. Slot numbers and
 *            name tokens are shifted up 2 bits. Zero means untyped.
 *
 *      Notes:
 *          - Strings are UTF-8 and are always null terminated.
 */

/*
 *  Type encoding masks
 */
#define EJS_ENCODE_GLOBAL_NOREF         0x0
#define EJS_ENCODE_GLOBAL_NAME          0x1
#define EJS_ENCODE_GLOBAL_SLOT          0x2
#define EJS_ENCODE_GLOBAL_MASK          0x3

/*
 *  Fixup kinds
 */
#define EJS_FIXUP_BASE_TYPE             1
#define EJS_FIXUP_INTERFACE_TYPE        2
#define EJS_FIXUP_RETURN_TYPE           3
#define EJS_FIXUP_TYPE_PROPERTY         4
#define EJS_FIXUP_INSTANCE_PROPERTY     5
#define EJS_FIXUP_LOCAL                 6
#define EJS_FIXUP_EXCEPTION             7

typedef struct EjsTypeFixup
{
    int                 kind;                       /* Kind of fixup */
#if UNUSED
    union {
        EjsVar          *target;                    /* Target to fixup */
        EjsType         *targetType;
        EjsFunction     *targetFunction;
        EjsEx           *targetException;
    };
#else
    EjsVar              *target;                    /* Target to fixup */
#endif
    int                 slotNum;                    /* Slot of target */
    EjsName             typeName;                   /* Type name */
    int                 typeSlotNum;                /* Type slot number */
} EjsTypeFixup;


/*
 *  Module File Header
 *
 *      |---------------|---------------|---------------|---------------|
 *      | Magic Number (2)              | Major Version | Minor Version |
 *      |---------------|---------------|---------------|---------------|
 *      | Flags (1)     | filler (1)    |               |               |
 *      |---------------|---------------|---------------|---------------|
 *
 *  TODO - refactor and put slot first after Section(1)
 *
 *  Module Section (includes the string constant Pool)
 *
 *      |---------------|---------------|---------------|---------------|
 *      | Section (1)   | Name (S)      | Url (S)       | initSlot (N)  |
 *      |---------------|---------------|---------------|---------------|
 *      | Pool len (N)  | Pool Strings  |
 *      |---------------|---------------|---------------|---------------|
 *
 *      Notes:
 *      - Strings are null terminated
 *
 *      TODO
 *      - Java is optimized for string indexes
 *      - C is optimized for string pointers
 *      - Should we include string lengths in the constant pool
 *
 *
 *  Dependencies Section
 *      |---------------|---------------|---------------|
 *      | Section (1)   | Module Name String (S)        |
 *      |---------------|---------------|---------------|
 *      | Module Name URL (S)           |               |
 *      |---------------|---------------|---------------|
 *
 *
 *
 *  Type Definition Section
 *      |---------------|---------------|---------------|---------------|
 *      | Section (1)   | Type Name (S) | Namespace (S) | Attributes (N)|
 *      |---------------|---------------|---------------|---------------|
 *      | Prop slot (N) | Base type (T) | Num Static Props (N)          |
 *      |---------------|---------------|---------------|---------------|
 *      | Num Instance Props (N)        | Num Interface (N)             |
 *      |---------------|---------------|---------------|---------------|
 *      | Interface (T) ....                                            |
 *      |---------------|---------------|---------------|---------------|
 *
 *      Attributes:
 *          EJS_ATTR_DYNAMIC (TODO - more attributes are actually set)
 *
 *      Notes:
 *          - Prop slot not used for global types unless --out used
 *          - See below for attribute definitions
 *          - Type definition may appear where a property definition is
 *            expected. In that case, it is created as a property of the
 *            preceeding type. Otherwise, the type is global.
 *          - (S) token references point into the costant pool.
 *          - If base type slot is provided (non-zero), then base type name
 *            will be zero and must be ignored.
 *          - Expect to see up to N sections following this record where
 *              N == sum of num static, methods, inherited and instance
 *              property counts.
 *
 *  Property Definition Section
 *      |---------------|---------------|---------------|---------------|
 *      | Section (1)   | Name (S)      | Namespace (S) | Attributes (N)|
 *      |---------------|---------------|---------------|---------------|
 *      | Prop slot (N) | Type (T)      |               |               |
 *      |---------------|---------------|---------------|---------------|
 *
 *      Attributes:
 *          See property attributes below
 *
 *      Notes:
 *          - All types are always defined in the global object. Types must
 *              be predefined before properties that use them.
 *          - Only generate property definition sections if the object is
 *              "indexed", ie. accessed as obj["name"].
 *          - Name tokens refer to the type constant pool
 */

/*  TODO -- need to have default value for each arg...
 *      But that could be any time including a structural type
 *  TODO - the format of each section time is not very consistent (See prop slot). Refactor
 *  TODO - whan attributes are rationalized. Merge lang into attributes?
 */

/*
 *      Function Code Definition Section
 *      |---------------|---------------|---------------|---------------|
 *      | Section (1)   | Name (S)      | Namespace (S) | NextSlot (N)  |
 *      |---------------|---------------|---------------|---------------|
 *      | Attributes (N)| Lang (1)      | Ret Type (T)  | Prop Slot (N) | 
 *      |---------------|---------------|---------------|---------------|
 *      | Arg Count (N) | Local Count(N)| Excpt Count(N)| Code Len (N)  | 
 *      |---------------|---------------|---------------|---------------|
 *      | Code ...      |               |               |               |
 *      |---------------|---------------|---------------|---------------|
 *
 *      Properties, nested functions, blocks an exception sections follow.
 *
 *      Attributes:
 *          See property attributes below
 *
 *      Exception Section
 *      |---------------|---------------|---------------|---------------|
 *      | Section (1)   | Flags (1)     | Try block start offset (N)    |
 *      |---------------|---------------|---------------|---------------|
 *      | Try block end offset (N)      | Handler block start offset (N)|
 *      |---------------|---------------|---------------|---------------|
 *      | Handler block end offset (N)  | Catch type (N)                |
 *      |---------------|---------------|---------------|---------------|
 *
 *
 *  TODO CHECK
 *
 *  Property Attributes (bits) (low order bits first)
 *      |---------------|---------------|---------------|---------------|
 *      | Visibility (2)| Hidden (1)    | Final (1)     | Readonly (1)  |
 *      |---------------|---------------|---------------|---------------|
 *      | Permanent (1) | Const (1)     | Fixed type (1)| Virtual (1)   |
 *      |---------------|---------------|---------------|---------------|
 *
 *          - public (0), internal (1), protected (2), private (3)
 *          - dynamic
 *          - enumerable
 *          - const
 *          - (virtual)
 *
 *  Block Definition Section
 *      |---------------|---------------|---------------|---------------|
 *      | Section (1)   | Block Name (S)| Slot (N)      | Prop Count (N)|
 *      |---------------|---------------|---------------|---------------|
 *
 *
 *  Documentation Section (may preceed any Type, Function or Property section)
 *      |---------------|---------------|---------------|---------------|
 *      | Section (1)   | Text (S)      |               |               |
 *      |---------------|---------------|---------------|---------------|
 *
 *  Module file names: a.b.c.d, a.b.c.*;
 *  Search strategy:
 *      1. File named a.b.c
 *      2. File named a/b/c
 *      3. File named a.b.c in EJSPATH
 *      4. File named a/b/c in EJSPATH
 *      5. File named c in EJSPATH
 *
 *  Default EJSPath "{INSDIR}/classes:."
 */

#define EJS_MODULE_MAGIC            0xC7DA


/*
 *  Module file format version
 *  TODO - rename EJS_MODULE_MAJOR
 */
#define EJS_MAJOR               1
#define EJS_MINOR               4
#define EJS_MODULE_VERSION      (EJS_MAJOR << 8 | EJS_MINOR)


/*
 *  Section types
 */
#define EJS_SECT_MODULE         1           /* Module section */
#define EJS_SECT_MODULE_END     2           /* End of a module */
#define EJS_SECT_DEPENDENCY     3           /* Module dependency */
#define EJS_SECT_CLASS          4           /* Class definition */
#define EJS_SECT_CLASS_END      5           /* End of class definition */
#define EJS_SECT_FUNCTION       6           /* Function */
#define EJS_SECT_FUNCTION_END   7           /* End of function definition */
#define EJS_SECT_BLOCK          8           /* Nested block */
#define EJS_SECT_BLOCK_END      9           /* End of Nested block */
#define EJS_SECT_PROPERTY       10          /* Property (variable) definition */
#define EJS_SECT_EXCEPTION      11          /* Exception definition */
#define EJS_SECT_DOC            12          /* Documentation for an element */
#define EJS_SECT_MAX            13

/*
 *  Psudo section types for loader callback
 */
#define EJS_SECT_START          (EJS_SECT_MAX + 1)
#define EJS_SECT_END            (EJS_SECT_MAX + 2)


/*
 *  Align headers on a 4 byte boundary
 */
#define EJS_HDR_ALIGN           4


/*
 *  Module header flags
 */
#define EJS_MODULE_BOUND_GLOBALS 0x1        /* Module global slots are fully bound */
#define EJS_MODULE_INTERP_EMPTY 0x2         /* Module compiled with --empty. ie. not using native elements */


/*
 *  Module loader flags
 */
#define EJS_MODULE_DONT_INIT    0x1         /* Don't run initializers */
#define EJS_MODULE_BUILTIN      0x2         /* Loading builtins */


/*
 *  File format is little-endian. All headers are aligned on word boundaries
 */
typedef struct EjsModuleHdr {
    ushort      magic;                      /* Magic number for Ejscript modules */
    uchar       major;                      /* Module format major version number */
    uchar       minor;                      /* Module format minor version number */
    uchar       flags;                      /* Module flags */
    uchar       filler;                     /* Reserved */
#if UNUSED
    int         seq;                        /* Sequence number to match native code slots with module file */
#endif
    int         version;                    /* Reserved for an application module version number */
} EjsModuleHdr;


/*
 *  Structure for the string constant pool
 */
typedef struct EjsConst {
    char        *pool;                      /* Constant pool storage */
    int         size;                       /* Size of constant pool storage */
    int         len;                        /* Length of active constant pool */
    int         base;                       /* Base used during relocations */
    int         locked;                     /* No more additions allowed */
    MprHashTable *table;                    /* Hash table for fast lookup */
} EjsConst;



/*
 *  Type for globalProperties list which keeps a list of global properties for this module.
 */
//  TODO - get rid of this and just use EjsName
typedef struct EcModuleProp {
    EjsName     qname;
} EcModuleProp;


/*
 *  Module type
 */
typedef struct EjsModule {
    char            *name;                  /* Name of this module */
    char            *url;                   /* Loading URL */
    int             version;                /* Module file version number */
    char            *path;                  /* Module file path name */
    MprList         *dependencies;          /* Module file dependencies*/

    /*
     *  Used during code generation
     */
    struct EcCodeGen *code;                 /* Code generation buffer */
    MprFile         *file;                  /* File handle */
    MprList         *globalProperties;      /* List of global properties */
    EjsFunction     *initializer;           /* Initializer method */

    /*
     *  Used only while loading modules
     */
    EjsBlock        *scopeChain;            /* Scope of nested types/functions/blocks, being loaded */
    EjsConst        *constants;             /* Constant pool */
    int             nameToken;              /* TODO ?? what */
    int             firstGlobalSlot;        /* First global slot (if used) */
    struct EjsFunction  *currentMethod;     /* Current method being loaded */

    uint            boundGlobals    : 1;    /* Global slots are bound by compiler */
    uint            compiling       : 1;    /* Module currently being compiled from source */
    uint            loaded          : 1;    /* Module has been loaded from an external file */
    uint            nativeLoaded    : 1;    /* Backing shared library loaded */
    uint            configured      : 1;    /* Module is already configured */
    uint            hasNative       : 1;    /* Has native property definitions */
    uint            hasInitializer  : 1;    /* Has initializer function */
    uint            initialized     : 1;    /* Initializer has run */
    uint            hasError        : 1;    /* Module has a loader error */

    int             flags;                  /* Loading flags */
#if UNUSED
    int             seq;                    /* Generation sequence number to match native slots with mod file */
#endif

#if BLD_FEATURE_EJS_DOC
    char            *doc;                   /* Current doc string */
#endif

} EjsModule;


/*
 *  Documentation string information
 */
#if BLD_FEATURE_EJS_DOC

/*
 *  Element documentation string. The loader will create if required.
 */
typedef struct EjsDoc {
    char        *docString;                         /* Original doc string */
    char        *brief;
    char        *description;
    char        *example;
    char        *requires;
    char        *returns;
    char        *stability;                         /* prototype, evolving, stable, mature, deprecated */
    char        *spec;                              /* Where specified */
    bool        hide;                               /* Hide doc if true */

    MprList     *defaults;                          /* Parameter default values */
    MprList     *params;                            /* Function parameters */
    MprList     *options;                           /* Option parameter values */
    MprList     *see;
    MprList     *throws;

    EjsTrait    *trait;                             /* Back pointer to trait */
} EjsDoc;
#endif


typedef void (*EjsLoaderCallback)(struct Ejs *ejs, int kind, ...);


/******************************** Prototypes **********************************/
//DDD
extern EjsModule    *ejsCreateModule(struct Ejs *ejs, cchar *name, cchar *uri, cchar *pool, int poolSize);
//DDD
extern MprList      *ejsLoadModule(struct Ejs *ejs, cchar *name, cchar *uri, EjsLoaderCallback callback, int flags);
extern int          ejsSearch(Ejs *ejs, char **path, cchar *name);

extern int          ejsModuleReadName(struct Ejs *ejs, MprFile *file, char **name, int len);
extern int          ejsModuleReadNumber(struct Ejs *ejs, EjsModule *module, int *number);
extern int          ejsModuleReadByte(struct Ejs *ejs, EjsModule *module, int *number);
extern char         *ejsModuleReadString(struct Ejs *ejs, EjsModule *module);
extern int          ejsModuleReadType(struct Ejs *ejs, EjsModule *module, EjsType **typeRef, EjsTypeFixup **fixup, 
                        EjsName *typeName, int *slotNum);

#if BLD_FEATURE_EJS_DOC
extern char         *ejsGetDocKey(struct Ejs *ejs, EjsBlock *block, int slotNum, char *buf, int bufsize);
extern EjsDoc       *ejsCreateDoc(struct Ejs *ejs, EjsBlock *block, int slotNum, cchar *docString);
#endif

#ifdef __cplusplus
}
#endif
#endif /* _h_EJS_MODULE */

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
