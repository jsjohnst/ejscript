/*
 *  ejsVm.h - Virtual Machine header.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#ifndef _h_EJS_VM_h
#define _h_EJS_VM_h 1

#include    "mpr.h"
#include    "ejsGC.h"

#ifdef __cplusplus
extern "C" {
#endif

/********************************** Defines ***********************************/
#if !DOXYGEN
/*
 *  Forward declare types
 */
struct Ejs;
struct EjsBlock;
struct EjsList;
struct EjsFrame;
struct EjsFunction;
struct EjsModule;
struct EjsStack;
struct EjsVar;
#endif

/*
 *  Language compliance levels
 */
#define EJS_SPEC_ECMA           0           /**< Run in strict ECMA-262 compliance mode */
#define EJS_SPEC_PLUS           1           /**< Run in enhanced ECMA-262 with non-breaking changes */
#define EJS_SPEC_FIXED          2           /**< Run with ECMA-262 plus enhancements and add breaking fixes */

#define LANG_ECMA(cp)       (cp->fileState->mode == PRAGMA_MODE_STANDARD)
#define LANG_PLUS(cp)         (cp->fileState->mode == PRAGMA_MODE_STRICT)
#define LANG_FIXED(cp)         (cp->fileState->mode == PRAGMA_MODE_STRICT)

/*
 *  Interpreter flags
 */
#define EJS_FLAG_EVENT          0x1         /**< Event pending */
#define EJS_FLAG_EMPTY          0x2         /**< Create an empty interpreter without native elements */
#define EJS_FLAG_COMPILER       0x4         /**< Running inside the compiler */
#define EJS_FLAG_NO_EXE         0x8         /**< VM will not execute code. Used by compiler without --run */
#define EJS_FLAG_MASTER         0x20        /**< Create a master interpreter */
#define EJS_FLAG_DOC            0x40        /**< Load documentation from modules */
#define EJS_FLAG_EXIT           0x80        /**< Interpreter should exit */
#define EJS_FLAG_LOADING        0x100       /**< Loading a module */
#define EJS_FLAG_NOEXIT         0x200       /**< App should service events and not exit */

#define EJS_STACK_ARG           -1          /* Offset to locate first arg */

#define EJS_FRAME_SIZE          MPR_ALLOC_ALIGN(sizeof(EjsFrame))
#define EJS_FRAME_VAR_SIZE      (MPR_ALLOC_ALIGN(sizeof(EjsFrame)) / sizeof(EjsVar*))

/**
 *  Qualified name structure
 *  @description All names in Ejscript consist of a property name and a name space. Namespaces provide discrete
 *      spaces to manage and minimize name conflicts. These names will soon be converted to unicode.
 *  @stability Prototype
 *  @defgroup EjsName EjsName
 *  @see EjsName ejsName ejsAllocName ejsDupName ejsCopyName
 */       
typedef struct EjsName {
    cchar       *name;                          /**< Property name */
    cchar       *space;                         /**< Property namespace */
} EjsName;


/**
 *  Initialize a Qualified Name structure
 *  @description Initialize the statically allocated qualified name structure using a name and namespace.
 *  @param qname Reference to an existing, uninitialized EjsName structure
 *  @param space Namespace string
 *  @param name Name string
 *  @return A reference to the qname structure
 *  @ingroup EjsName
 */
extern EjsName *ejsName(struct EjsName *qname, cchar *space, cchar *name);

/**
 *  Allocate and Initialize  a Qualified Name structure
 *  @description Create and initialize a qualified name structure using a name and namespace.
 *  @param ctx Any memory context returned by mprAlloc
 *  @param space Namespace string
 *  @param name Name string
 *  @return A reference to an allocated EjsName structure. Caller must free.
 *  @ingroup EjsName
 */
extern EjsName *ejsAllocName(MprCtx ctx, cchar *space, cchar *name);

extern EjsName *ejsDupName(MprCtx ctx, EjsName *qname);
extern EjsName ejsCopyName(MprCtx ctx, EjsName *qname);

/**
 *  Evaluation stack. 
 *  The VM Stacks grow forward in memory. A push is done by incrementing first, then storing. ie.
 *      *++top = value
 *  A pop is done by extraction then decrement. ie.
 *      value = *top--
 *  @ingroup EjsVm
 */
typedef struct EjsStack {
    struct EjsVar   **bottom;               /* Pointer to start of stack mem */
    struct EjsVar   **top;                  /* Last element pushed */
    struct EjsVar   **end;                  /* Only used on non-virtual memory systems */
    int             size;
} EjsStack;


/**
 *  Lookup State.
 *  @description Location information returned when looking up properties.
 *  @ingroup EjsVm
 */
typedef struct EjsLookup
{
    struct EjsVar   *obj;                   /* Final object / Type containing the variable */
    int             slotNum;                /* Final slot in obj containing the variable reference */

    uint            nthBase: 8;             /* Property on Nth super type -- count from the object */
    uint            nthBlock: 8;            /* Property on Nth block in the scope chain -- count from the end */
    uint            useThis: 1;             /* Property accessible via "this." */
    uint            instanceProperty: 1;    /* Property is an instance property */
    uint            ownerIsType: 1;         /* Original object owning the property is a type */

    /*
     *  Just for the compiler
     */
    struct EjsVar   *originalObj;           /* Original object used for the search */
    struct EjsVar   *ref;                   /* Actual property reference */
    struct EjsTrait *trait;                 /* Property trait describing the property */
    struct EjsName  name;                   /* Name and namespace used to find the property */

} EjsLookup;


/**
 *  Ejsript Interperter Management
 *  @description The Ejs structure contains the state for a single interpreter. The #ejsCreate routine may be used
 *      to create multiple interpreters and returns a reference to be used in subsequent Ejscript API calls.
 *  @stability Prototype.
 *  @defgroup Ejs Ejs
 *  @see ejsCreate, ejsCreateService, ejsSetSearchPath, ejsEvalFile, ejsEvalScript, ejsExit
 */
typedef struct Ejs {
    struct EjsFrame     *frame;             /* Current frame */
    struct EjsVar       *result;            /* Last expression result */
    struct EjsStack     stack;              /* Evaluation operand stack */
    struct EjsService   *service;           /* Back pointer to the service */
    struct Ejs          *master;            /* Inherit builtin types from the master */
    EjsGC               gc;                 /* Garbage collector state */

    /*
     *  Essential types
     */
    struct EjsType      *arrayType;         /* Array type */
    struct EjsType      *blockType;         /* Block type */
    struct EjsType      *booleanType;       /* Boolean type */
    struct EjsType      *byteArrayType;     /* ByteArray type */
    struct EjsType      *dateType;          /* DAte type */
    struct EjsType      *errorType;         /* Error type */
    struct EjsType      *functionType;      /* Function type */
    struct EjsType      *iteratorType;      /* Iterator type */
    struct EjsType      *namespaceType;     /* Namespace type */
    struct EjsType      *nullType;          /* Null type */
    struct EjsType      *numberType;        /* Default numeric type */
    struct EjsType      *objectType;        /* Object type */
    struct EjsType      *regExpType;        /* RegExp type */
    struct EjsType      *stringType;        /* String type */
    struct EjsType      *stopIterationType; /* StopIteration type */
    struct EjsType      *typeType;          /* Type type */
    struct EjsType      *voidType;          /* Void type */
    struct EjsType      *xmlType;           /* XML type */
    struct EjsType      *xmlListType;       /* XMLList type */

    /*
     *  Key values
     */
    struct EjsVar       *global;            /* The "global" object as an EjsVar */
    struct EjsBlock     *globalBlock;       /* The "global" object as an EjsBlock */

    struct EjsString    *emptyStringValue;  /* "" value */
    struct EjsBoolean   *falseValue;        /* The "false" value */
    struct EjsNumber    *infinityValue;     /* The infinity number value */
    struct EjsNumber    *maxValue;          /* Maximum number value */
    struct EjsNumber    *minValue;          /* Minimum number value */
    struct EjsNumber    *minusOneValue;     /* The -1 number value */
    struct EjsNumber    *nanValue;          /* The "NaN" value if floating point numbers, else zero */
    struct EjsNumber    *negativeInfinityValue; /* The negative infinity number value */
    struct EjsVar       *nullValue;         /* The "null" value */
    struct EjsNumber    *oneValue;          /* The 1 number value */
    struct EjsBoolean   *trueValue;         /* The "true" value */
    struct EjsVar       *undefinedValue;    /* The "void" value */
    struct EjsNumber    *zeroValue;         /* The 0 number value */

    struct EjsNamespace *configSpace;       /* CONFIG namespace */
    struct EjsNamespace *emptySpace;        /* Empty namespace */
    struct EjsNamespace *intrinsicSpace;    /* Intrinsic namespace */
    struct EjsNamespace *iteratorSpace;     /* Iterator namespace */
    struct EjsNamespace *internalSpace;     /* Internal namespace */
    struct EjsNamespace *publicSpace;       /* Public namespace */
    struct EjsNamespace *eventsSpace;       /* ejs.events namespace */
    struct EjsNamespace *ioSpace;           /* ejs.io namespace */
    struct EjsNamespace *sysSpace;          /* ejs.sys namespace */

    char                *castTemp;          /* Temporary string for casting */
    char                *errorMsg;          /* Error message */
    char                **argv;             /* Command line args */
    int                 argc;               /* Count of command line args */
    int                 state;              /* Execution state */
    int                 flags;              /* Execution flags */
    int                 exitStatus;         /* Status to exit() */
    int                 initialized;        /* Interpreter fully initialized */
    int                 hasError;           /* Interpreter has an initialization error */
    int                 serializeDepth;     /* Serialization depth */

    struct EjsVar       *exception;         /* Pointer to exception object */
    uint                noExceptions: 1;    /* Suppress exceptions */
    bool                attention;          /* VM needs attention */

    struct EjsFrame     *frames;            /* Free list of frames */

    struct EjsTypeHelpers *defaultHelpers;  /* Default EjsVar helpers */
    struct EjsTypeHelpers *blockHelpers;    /* EjsBlock helpers */
    struct EjsTypeHelpers *objectHelpers;   /* EjsObject helpers */

    MprList             *modules;           /* Loaded modules */
    MprList             *typeFixups;        /* Loaded types to fixup */

    void                (*loaderCallback)(struct Ejs *ejs, int kind, ...);
    void                *userData;          /* User data */
    void                *handle;            /* Hosting environment handle */

    MprHashTable        *nativeModules;     /* Native module initialization callbacks */
    MprHashTable        *coreTypes;         /* Core type instances */
    MprHashTable        *standardSpaces;    /* Hash of standard namespaces (global namespaces) */

    MprHashTable        *doc;               /* Documentation */
} Ejs;


/**
 *  Ejscript Service structure
 *  @description The Ejscript service manages the overall language runtime. It 
 *      is the factory that creates interpreter instances via #ejsCreate.
 *  @ingroup Ejs
 */
typedef struct EjsService {
    char        *ejsPath;           /* Module load search path */
} EjsService;


/*********************************** Prototypes *******************************/
/**
 *  Open the Ejscript service
 *  @description One Ejscript service object is required per application. From this service, interpreters
 *      can be created.
 *  @param ctx Any memory context returned by mprAlloc
 *  @return An ejs service object
 *  @ingroup Ejs
 */
extern EjsService *ejsCreateService(MprCtx ctx);

/**
 *  Create an ejs interpreter
 *  @description Create an interpreter object to evalute Ejscript programs. Ejscript supports multiple interpreters.
 *      One interpreter can be designated as a master interpreter and then it can be cloned by supplying the master
 *      interpreter to this call. A master interpreter provides the standard system types and clone interpreters can
 *      quickly be created an utilize the master interpreter's types. This saves memory and speeds initialization.
 *  @param ctx Any memory context returned by mprAlloc
 *  @param master Optional master interpreter to clone.
 *  @param flags Optional flags to modify the interpreter behavior. Valid flags are:
 *      @li    EJS_FLAG_COMPILER       - Interpreter will compile code from source
 *      @li    EJS_FLAG_NO_EXE         - Don't execute any code. Just compile.
 *      @li    EJS_FLAG_MASTER         - Create a master interpreter
 *      @li    EJS_FLAG_DOC            - Load documentation from modules
 *      @li    EJS_FLAG_NOEXIT         - App should service events and not exit unless explicitly instructed
 *  @return A new interpreter
 *  @ingroup Ejs
 */
extern Ejs *ejsCreate(MprCtx ctx, struct Ejs *master, int flags);

/**
 *  Set the module search path
 *  @description Set the ejs module search path. The search path is by default set to the value of the EJSPATH
 *      environment directory. Ejsript will search for modules by name. The search strategy is:
 *      Given a name "a.b.c", scan for:
 *      @li File named a.b.c
 *      @li File named a/b/c
 *      @li File named a.b.c in EJSPATH
 *      @li File named a/b/c in EJSPATH
 *      @li File named c in EJSPATH
 *
 *  Ejs will search for files with no extension and also search for modules with a ".mod" extension. If there is
 *  a shared library of the same name with a shared library extension (.so, .dll, .dylib) and the module requires 
 *  native code, then the shared library will also be loaded.
 *  @param sp Ejs service returned from #ejsCreateService
 *  @param ejsPath Search path. This is a colon (or semicolon on Windows) separated string of directories.
 *  @ingroup Ejs
 */
extern void ejsSetSearchPath(EjsService *sp, cchar *ejsPath);

/**
 *  Evaluate a file
 *  @description Evaluate a file containing an Ejscript. This requires linking with the Ejscript compiler library (libec). 
 *  @param path Filename of the script to evaluate
 *  @return Return zero on success. Otherwise return a negative Mpr error code.
 *  @ingroup Ejs
 */
extern int ejsEvalFile(cchar *path);

//  TODO - document
extern int ejsLoadScript(Ejs *ejs, cchar *path, int flags);

/**
 *  Evaluate a module
 *  @description Evaluate a module containing compiled Ejscript.
 *  @param path Filename of the module to evaluate.
 *  @return Return zero on success. Otherwise return a negative Mpr error code.
 *  @ingroup Ejs
 */
extern int ejsEvalModule(cchar *path);

/**
 *  Evaluate a script
 *  @description Evaluate a script. This requires linking with the Ejscript compiler library (libec). 
 *  @param script Script to evaluate
 *  @return Return zero on success. Otherwise return a negative Mpr error code.
 *  @ingroup Ejs
 */
extern int ejsEvalScript(cchar *script);

/**
 *  Instruct the interpreter to exit.
 *  @description This will instruct the interpreter to cease interpreting any further script code.
 *  @param ejs Interpeter object returned from #ejsCreate
 *  @param status Reserved and ignored
 *  @ingroup Ejs
 */
extern void ejsExit(Ejs *ejs, int status);

/**
 *  Get the hosting handle
 *  @description The interpreter can store a hosting handle. This is typically a web server object if hosted inside
 *      a web server
 *  @param ejs Interpeter object returned from #ejsCreate
 *  @return Hosting handle
 *  @ingroup Ejs
 */
extern void *ejsGetHandle(Ejs *ejs);

/**
 *  Run a script
 *  @description Run a script that has previously ben compiled by ecCompile
 *  @param ejs Interpeter object returned from #ejsCreate
 *  @return Zero if successful, otherwise a non-zero Mpr error code.
 */
extern int ejsRun(Ejs *ejs);

/**
 *  Throw an exception
 *  @description Throw an exception object 
 *  @param ejs Interpeter object returned from #ejsCreate
 *  @param error Exception argument object.
 *  @return The exception argument for chaining.
 *  @ingroup Ejs
 */
extern struct EjsVar *ejsThrowException(Ejs *ejs, struct EjsVar *error);

/**
 *  Report an error message using the MprLog error channel
 *  @description This will emit an error message of the format:
 *      @li program:line:errorCode:SEVERITY: message
 *  @param ejs Interpeter object returned from #ejsCreate
 *  @param fmt Is an alternate printf style format to emit if the interpreter has no valid error message.
 *  @param ... Arguments for fmt
 *  @ingroup Ejs
 */
extern void ejsReportError(Ejs *ejs, char *fmt, ...);

extern int ejsAddModule(Ejs *ejs, struct EjsModule *up);
extern struct EjsVar *ejsCastOperands(Ejs *ejs, struct EjsVar *lhs, int opcode,  struct EjsVar *rhs);
extern int ejsCheckModuleLoaded(Ejs *ejs, cchar *name);
extern void ejsClearExiting(Ejs *ejs);
extern struct EjsVar *ejsCreateException(Ejs *ejs, int slot, cchar *fmt, va_list fmtArgs);
extern MprList *ejsGetModuleList(Ejs *ejs);
extern struct EjsVar *ejsGetVarByName(Ejs *ejs, struct EjsVar *vp, EjsName *name, bool anySpace, EjsLookup *lookup);
extern int ejsInitStack(Ejs *ejs);
extern void ejsLog(Ejs *ejs, cchar *fmt, ...);
extern int ejsLookupVar(Ejs *ejs, struct EjsVar *vp, EjsName *name, bool anySpace, EjsLookup *lookup);
extern int ejsLookupVarInBlock(Ejs *ejs, struct EjsVar *vp, EjsName *name, bool anySpace, EjsLookup *lookup);
extern struct EjsModule *ejsLookupModule(Ejs *ejs, cchar *name);
extern int ejsLookupScope(Ejs *ejs, EjsName *name, bool anySpace, EjsLookup *lookup);
extern void ejsMemoryFailure(MprCtx ctx, uint size, uint total, bool granted);
extern int ejsPushBlock(MprCtx ctx, struct EjsList *scopeChain, struct EjsBlock *scopeBlock);
extern struct EjsVar *ejsPopBlock(struct EjsList *scopeChain);
extern struct EjsVar *ejsPopBlock(struct EjsList *scopeChain);
extern int ejsRemoveModule(Ejs *ejs, struct EjsModule *up);
extern int ejsRunProgram(Ejs *ejs, cchar *className, cchar *methodName);
extern void ejsSetHandle(Ejs *ejs, void *handle);
extern void ejsShowCurrentScope(Ejs *ejs);
extern void ejsShowStack(Ejs *ejs);
extern void ejsShowBlockScope(Ejs *ejs, struct EjsBlock *block);
extern int ejsStartLogging(Mpr *mpr, char *logSpec);
extern struct EjsTypeHelpers *ejsGetDefaultHelpers(Ejs *ejs);
extern struct EjsTypeHelpers *ejsGetObjectHelpers(Ejs *ejs);
extern struct EjsTypeHelpers *ejsGetBlockHelpers(Ejs *ejs);

#ifdef __cplusplus
}
#endif

#endif /* _h_EJS_VM_h */

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
