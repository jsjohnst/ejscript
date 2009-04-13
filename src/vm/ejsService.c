/**
 *  ejsService.c - Ejscript interpreter factory
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejs.h"

#if BLD_FEATURE_EJS_WEB
#include    "ejsWeb.h"
#endif

/*********************************** Forward **********************************/

static void allocNotifier(Ejs *ejs, uint size, uint total, bool granted);
static int  cloneMaster(Ejs *ejs, Ejs *master);
static int  configureEjsModule(Ejs *ejs, EjsModule *mp, cchar *path);
static int  defineTypes(Ejs *ejs);
static void defineHelpers(Ejs *ejs);
static void destroyEjs(Ejs *ejs);
static int  runSpecificMethod(Ejs *ejs, cchar *className, cchar *methodName);
static int  searchForMethod(Ejs *ejs, cchar *methodName, EjsType **typeReturn);
static void setEjsPath(EjsService *sp);

#if BLD_FEATURE_STATIC || BLD_FEATURE_EJS_ALL_IN_ONE
#if BLD_FEATURE_EJS_DB
static int configureDbModule(Ejs *ejs, EjsModule *mp, cchar *path);
#endif
#if BLD_FEATURE_EJS_WEB
static int configureWebModule(Ejs *ejs, EjsModule *mp, cchar *path);
#endif
#endif

/*
 *  Global singleton for the Ejs service
 */
EjsService *_globalEjsService;

/************************************* Code ***********************************/
/*
 *  Initialize the EJS subsystem
 */
EjsService *ejsCreateService(MprCtx ctx)
{
    EjsService  *sp;

    sp = mprAllocObjZeroed(ctx, EjsService);
    if (sp == 0) {
        return 0;
    }
    _globalEjsService = sp;

    setEjsPath(sp);
    return sp;
}


Ejs *ejsCreate(MprCtx ctx, Ejs *master, int flags)
{
    Ejs     *ejs;

    /*
     *  Create interpreter structure
     */
    ejs = mprAllocObjWithDestructorZeroed(ctx, Ejs, destroyEjs);
    if (ejs == 0) {
        return 0;
    }
    mprSetAllocNotifier(ejs, (MprAllocNotifier) allocNotifier);

    ejs->service = _globalEjsService;
    ejs->flags |= (flags & (EJS_FLAG_EMPTY | EJS_FLAG_COMPILER | EJS_FLAG_NO_EXE | EJS_FLAG_DOC));

    if (ejsInitStack(ejs) < 0) {
        mprFree(ejs);
        return 0;
    }

    ejsCreateGCService(ejs);

    if (master == 0) {
        ejs->modules = mprCreateList(ejs);
        ejs->coreTypes = mprCreateHash(ejs, 0);
        ejs->nativeModules = mprCreateHash(ejs, 0);
        ejs->standardSpaces = mprCreateHash(ejs, 0);
        defineHelpers(ejs);
        if (defineTypes(ejs) < 0) {
            mprFree(ejs);
            return 0;
        }

    } else {
        cloneMaster(ejs, master);
    }

    if (mprHasAllocError(ejs)) {
        mprError(ejs, "Memory allocation error during initialization");
        mprFree(ejs);
        return 0;
    }

    ejs->initialized = 1;
    ejsCollectGarbage(ejs, EJS_GC_ALL);

    ejsSetGeneration(ejs, EJS_GEN_NEW);

    return ejs;
}


static void destroyEjs(Ejs *ejs)
{
    if (ejs->stack.bottom) {
        mprMapFree(ejs->stack.bottom, ejs->stack.size);
    }
}


static void defineHelpers(Ejs *ejs)
{
    ejs->defaultHelpers = (EjsTypeHelpers*) mprAllocZeroed(ejs, sizeof(EjsTypeHelpers));
    ejsInitializeDefaultHelpers(ejs->defaultHelpers);

    /*
     *  Object inherits the default helpers. Block inherits the object helpers
     */
    ejs->objectHelpers = (EjsTypeHelpers*) mprMemdup(ejs, (void*) ejs->defaultHelpers, sizeof(EjsTypeHelpers));
    ejsInitializeObjectHelpers(ejs->objectHelpers);

    ejs->blockHelpers = (EjsTypeHelpers*) mprMemdup(ejs, (void*) ejs->objectHelpers, sizeof(EjsTypeHelpers));
    ejsInitializeBlockHelpers(ejs->blockHelpers);
}


/*
 *  Create the core language types. These are native types and are created prior to loading ejs.mod.
 *  The loader then matches these types to the loaded definitions.
 */
static int createTypes(Ejs *ejs)
{
    /*
     *  Create the essential bootstrap types: Object, Type and the global object, these are the foundation.
     *  All types are instances of Type. Order matters here.
     */
    ejsCreateObjectType(ejs);
    ejsCreateTypeType(ejs);
    ejsCreateBlockType(ejs);
    ejsCreateNamespaceType(ejs);
    ejsCreateFunctionType(ejs);
    ejsCreateGlobalBlock(ejs);
    ejsCreateNullType(ejs);

    /*
     *  Now create the rest of the native types. Order does not matter.
     */
    ejsCreateArrayType(ejs);
    ejsCreateBlockType(ejs);
    ejsCreateBooleanType(ejs);
    ejsCreateByteArrayType(ejs);
    ejsCreateDateType(ejs);
    ejsCreateErrorType(ejs);
    ejsCreateIteratorType(ejs);
    ejsCreateNumberType(ejs);
    ejsCreateReflectType(ejs);
    ejsCreateStringType(ejs);
    ejsCreateVoidType(ejs);
#if ES_XML && BLD_FEATURE_EJS_E4X
    ejsCreateXMLType(ejs);
#endif
#if ES_XMLList && BLD_FEATURE_EJS_E4X
    ejsCreateXMLListType(ejs);
#endif
#if ES_RegExp && BLD_FEATURE_REGEXP
    ejsCreateRegExpType(ejs);
#endif
    ejsCreateAppType(ejs);
    ejsCreateConfigType(ejs);
    ejsCreateGCType(ejs);
    ejsCreateMemoryType(ejs);
    ejsCreateSystemType(ejs);
#if ES_ejs_events_Timer
    ejsCreateTimerType(ejs);
#endif
#if ES_ejs_io_File
    ejsCreateFileType(ejs);
#endif
#if ES_ejs_io_Http && BLD_FEATURE_HTTP_CLIENT
    ejsCreateHttpType(ejs);
#endif

    /*
     *  The native module callbacks are invoked after loading the mod file. This allows the callback routines to configure
     *  native methods and do other native type adjustments. configureEjsModule is always invoked even if SHARED because it
     *  is never loaded from a shared library. Normally, when loading from a shared library, the init routine is invoked
     *  immediately after loading the mod file and it should call the configuration routine.
     */
    ejsAddNativeModule(ejs, "ejs", configureEjsModule);

#if BLD_FEATURE_STATIC || BLD_FEATURE_EJS_ALL_IN_ONE
    if (!(ejs->flags & EJS_FLAG_EMPTY)) {
#if BLD_FEATURE_EJS_DB
        ejsAddNativeModule(ejs, "ejs.db", configureDbModule);
#endif
#if BLD_FEATURE_EJS_WEB
        ejsAddNativeModule(ejs, "ejs.web", configureWebModule);
#endif
    }
#endif

    if (ejs->hasError || ejs->errorType == 0 || mprHasAllocError(ejs)) {
        return MPR_ERR;
    }
    return 0;
}


/*
 *  This will configure all the core types by defining native methods and properties
 */
static int configureEjsModule(Ejs *ejs, EjsModule *mp, cchar *path)
{
    EjsModule   *pp;
    
    if (ejs->flags & EJS_FLAG_EMPTY) {
        return 0;
    }

#if UNUSED
#if _ES_CHECKSUM_ejs
    if (mp->seq != _ES_CHECKSUM_ejs) {
        ejsThrowIOError(ejs, "Module \"%s\" does not match native code", path);
        return EJS_ERR;
    }
#endif
#endif

    /*
     *  Order matters. Put in dependency order
     */
    ejsConfigureObjectType(ejs);
    ejsConfigureArrayType(ejs);
    ejsConfigureBlockType(ejs);
    ejsConfigureBooleanType(ejs);
    ejsConfigureByteArrayType(ejs);
    ejsConfigureDateType(ejs);
    ejsConfigureFunctionType(ejs);
    ejsConfigureGlobalBlock(ejs);
    ejsConfigureErrorType(ejs);
    ejsConfigureIteratorType(ejs);
    ejsConfigureNamespaceType(ejs);
    ejsConfigureNumberType(ejs);
    ejsConfigureNullType(ejs);
    ejsConfigureReflectType(ejs);
    ejsConfigureStringType(ejs);
    ejsConfigureTypeType(ejs);
    ejsConfigureVoidType(ejs);
#if ES_XML && BLD_FEATURE_EJS_E4X
    ejsConfigureXMLType(ejs);
#endif
#if ES_XMLList && BLD_FEATURE_EJS_E4X
    ejsConfigureXMLListType(ejs);
#endif
#if ES_RegExp && BLD_FEATURE_REGEXP
    ejsConfigureRegExpType(ejs);
#endif

    ejsConfigureAppType(ejs);
    ejsConfigureConfigType(ejs);
    ejsConfigureGCType(ejs);
    ejsConfigureMemoryType(ejs);
    ejsConfigureSystemType(ejs);
#if ES_ejs_events_Timer
    ejsConfigureTimerType(ejs);
#endif
#if ES_ejs_io_File
    ejsConfigureFileType(ejs);
#endif
#if ES_ejs_io_Http && BLD_FEATURE_HTTP_CLIENT
    ejsConfigureHttpType(ejs);
#endif

    if (ejs->hasError || ejs->errorType == 0 || mprHasAllocError(ejs)) {
        mprAssert(0);
        return MPR_ERR;
    }
    
    if ((pp = ejsLookupModule(ejs, "ejs.events")) != 0) {
        pp->configured = 1;
    }
    if ((pp = ejsLookupModule(ejs, "ejs.sys")) != 0) {
        pp->configured = 1;
    }
    if ((pp = ejsLookupModule(ejs, "ejs.io")) != 0) {
        pp->configured = 1;
    }
    return 0;
}


#if BLD_FEATURE_STATIC || BLD_FEATURE_EJS_ALL_IN_ONE
#if BLD_FEATURE_EJS_DB
static int configureDbModule(Ejs *ejs, EjsModule *mp, cchar *path)
{
    ejsConfigureDbTypes(ejs);
    if (ejs->hasError || ejs->errorType == 0 || mprHasAllocError(ejs)) {
        mprAssert(0);
        return MPR_ERR;
    }
    mp->configured = 1;
    return 0;
}
#endif


#if BLD_FEATURE_EJS_WEB
static int configureWebModule(Ejs *ejs, EjsModule *mp, cchar *path)
{
    ejsConfigureWebTypes(ejs);

    if (ejs->hasError || ejs->errorType == 0 || mprHasAllocError(ejs)) {
        mprAssert(0);
        return MPR_ERR;
    }
    mp->configured = 1;
    return 0;
}
#endif
#endif


/*
 *  Register a native module callback to be invoked when it it time to configure the module. This is used by loadable modules
 *  when they are built statically.
 */
int ejsAddNativeModule(Ejs *ejs, char *name, EjsNativeCallback callback)
{
    if (mprAddHash(ejs->nativeModules, name, callback) == 0) {
        return EJS_ERR;
    }
    return 0;
}


static int defineTypes(Ejs *ejs)
{
    /*
     *  Create all the builtin types. These are defined and hashed. Not defined in global.
     */
    if (createTypes(ejs) < 0 || ejs->hasError) {
        mprError(ejs, "Can't create core types");
        return EJS_ERR;
    }

    /*
     *  Load the builtin module. This will create all the type definitions and match with builtin native types.
     *  This will call the configure routines defined in moduleConfig and will run the module initializers.
     */
    if (! (ejs->flags & EJS_FLAG_EMPTY)) {
        if (ejsLoadModule(ejs, EJS_MOD, NULL, NULL, EJS_MODULE_BUILTIN) == 0) {
            mprError(ejs, "Can't load " EJS_MOD);
            return EJS_ERR;
        }
    }

    return 0;
}


#if BLD_DEBUG
/*
 *  TODO - remove this
 */
static void checkType(Ejs *ejs, EjsType *type)
{
    EjsVar          *vp;
    EjsTrait        *trait;
    EjsName         qname;
    int             i, count;
    
    i = type->block.obj.var.dynamic;
    mprAssert(type->block.obj.var.dynamic == 0);
    mprAssert(i == 0);
    
    count = ejsGetPropertyCount(ejs, (EjsVar*) type);
    for (i = 0; i < count; i++) {
        vp = ejsGetProperty(ejs, (EjsVar*) type, i);
/* TODO - temp workaround */
        if (vp == 0 || vp->type == 0) {
            continue;
        }
        if (ejsIsFunction(vp)) {
            continue;
        }
        trait = ejsGetTrait((EjsBlock*) type, i);
        if (trait == 0) {
            continue;
        }
        if (trait->attributes & EJS_ATTR_CONST) {
            continue;
        } else {
            qname = ejsGetPropertyName(ejs, (EjsVar*) type, i);
            if (qname.name && qname.name[0] == '\0') {
                continue;
            }
            mprLog(ejs, 6, "Found non-const property %s.%s", type->qname.name, qname.name);
        }
    }
    //  TODO - got to test all properties of the type
}
#endif


static int cloneMaster(Ejs *ejs, Ejs *master)
{
    EjsName     qname;
    EjsType     *type;
    EjsVar      *vp;
    EjsTrait    *trait;
    int         i, count;

    mprAssert(master);

    ejs->master = master;
    ejs->service = master->service;
    ejs->defaultHelpers = master->defaultHelpers;
    ejs->objectHelpers = master->objectHelpers;
    ejs->blockHelpers = master->blockHelpers;
    ejs->objectType = master->objectType;

    ejs->arrayType = master->arrayType;
    ejs->blockType = master->blockType;
    ejs->booleanType = master->booleanType;
    ejs->byteArrayType = master->byteArrayType;
    ejs->dateType = master->dateType;
    ejs->errorType = master->errorType;
    ejs->functionType = master->functionType;
    ejs->iteratorType = master->iteratorType;
    ejs->namespaceType = master->namespaceType;
    ejs->nullType = master->nullType;
    ejs->numberType = master->numberType;
    ejs->objectType = master->objectType;
    ejs->regExpType = master->regExpType;
    ejs->stringType = master->stringType;
    ejs->stopIterationType = master->stopIterationType;
    ejs->typeType = master->typeType;
    ejs->voidType = master->voidType;

#if BLD_FEATURE_EJS_E4X
    ejs->xmlType = master->xmlType;
    ejs->xmlListType = master->xmlListType;
#endif

    ejs->emptyStringValue = master->emptyStringValue;
    ejs->falseValue = master->falseValue;
    ejs->infinityValue = master->infinityValue;
    ejs->minusOneValue = master->minusOneValue;
    ejs->nanValue = master->nanValue;
    ejs->negativeInfinityValue = master->negativeInfinityValue;
    ejs->nullValue = master->nullValue;
    ejs->oneValue = master->oneValue;
    ejs->trueValue = master->trueValue;
    ejs->undefinedValue = master->undefinedValue;
    ejs->zeroValue = master->zeroValue;

    ejs->configSpace = master->configSpace;
    ejs->emptySpace = master->emptySpace;
    ejs->eventsSpace = master->eventsSpace;
    ejs->ioSpace = master->ioSpace;
    ejs->intrinsicSpace = master->intrinsicSpace;
    ejs->iteratorSpace = master->iteratorSpace;
    ejs->internalSpace = master->internalSpace;
    ejs->publicSpace = master->publicSpace;
    ejs->sysSpace = master->sysSpace;

    ejs->argv = master->argv;
    ejs->argc = master->argc;
    ejs->coreTypes = master->coreTypes;
    ejs->standardSpaces = master->standardSpaces;

    ejs->modules = mprDupList(ejs, master->modules);

    //  Push this code into ejsGlobal.c. Call ejsCloneGlobal
    
    ejs->globalBlock = ejsCreateBlock(ejs, EJS_GLOBAL, master->globalBlock->obj.capacity);
    ejs->global = (EjsVar*) ejs->globalBlock; 
    ejs->globalBlock->obj.numProp = master->globalBlock->obj.numProp;
    ejsGrowBlock(ejs, ejs->globalBlock, ejs->globalBlock->obj.numProp);
    
    ejsCopyList(ejs->globalBlock, &ejs->globalBlock->namespaces, &master->globalBlock->namespaces);

    /*
     *  TODO - some form of copy on write or 2 level global would be better.
     *  TODO - OPT. Could accelerate this. Push into ejsGlobal.c and do block copies of slots, traits etc and then rehash.
     */
    count = ejsGetPropertyCount(master, master->global);
    for (i = 0; i < count; i++) {
        vp = ejsGetProperty(ejs, master->global, i);
        if (vp) {
            //TODO  mprAssert(vp->generation == EJS_GEN_ETERNAL || vp == master->global);
            ejsSetProperty(ejs, ejs->global, i, ejsGetProperty(master, master->global, i));
            qname = ejsGetPropertyName(master, master->global, i);
            ejsSetPropertyName(ejs, ejs->global, i, &qname);
            trait = ejsGetTrait(master->globalBlock, i);
            ejsSetTrait(ejs->globalBlock, i, trait->type, trait->attributes);
#if BLD_DEBUG
            //  TODO - remove
            if (ejsIsType(vp) && vp != (EjsVar*) ejs->objectType) {
                checkType(ejs, (EjsType*) vp);
            }
#endif
        }
    }
    
    /*
     *  Clone some mutable types
     *  TODO - remove this. Must handle mutable types better.
     */
    type = (EjsType*) ejsGetProperty(ejs, ejs->global, ES_XML); 
    ejsSetProperty(ejs, ejs->global, ES_XML, ejsCloneVar(ejs, (EjsVar*) type, 0));

#if FUTURE
    type = (EjsType*) ejsGetProperty(ejs, ejs->global, ES_ejs_sys_GC); 
    ejsSetProperty(ejs, ejs->global, ES_ejs_sys_GC, ejsCloneVar(ejs, (EjsVar*) type, 0));     
#endif
     
#if ES_ejs_db_Database
    type = (EjsType*) ejsGetProperty(ejs, ejs->global, ES_ejs_db_Database); 
    ejsSetProperty(ejs, ejs->global, ES_ejs_db_Database, ejsCloneVar(ejs, (EjsVar*) type, 0));
#else
    /*
     *  Building shared
     */
    type = (EjsType*) ejsGetPropertyByName(ejs, ejs->global, ejsName(&qname, "ejs.db", "Database"));
    ejsSetPropertyByName(ejs, ejs->global, &qname, ejsCloneVar(ejs, (EjsVar*) type, 0));
#endif

#if UNUSED
#if ES_ejs_db_Record
    type = (EjsType*) ejsGetProperty(ejs, ejs->global, ES_ejs_db_Record); 
    ejsSetProperty(ejs, ejs->global, ES_ejs_db_Record, ejsCloneVar(ejs, (EjsVar*) type, 0));
#endif
#endif

#if ES_ejs_web_GoogleConnector
    type = (EjsType*) ejsGetProperty(ejs, ejs->global, ES_ejs_web_GoogleConnector); 
    ejsSetProperty(ejs, ejs->global, ES_ejs_web_GoogleConnector, ejsCloneVar(ejs, (EjsVar*) type, 0));
#else
    /*
     *  Shared. TODO - this is just for the Google.nextId
     */
    type = (EjsType*) ejsGetPropertyByName(ejs, ejs->global, ejsName(&qname, "ejs.web", "GoogleConnector")); 
    ejsSetPropertyByName(ejs, ejs->global, &qname, ejsCloneVar(ejs, (EjsVar*) type, 0));
#endif
     
    ejsSetProperty(ejs, ejs->global, ES_global, ejs->global);

    return 0;
}


/*
 *  Notifier callback function. Invoked by mprAlloc on allocation errors
 */
static void allocNotifier(Ejs *ejs, uint size, uint total, bool granted)
{
    ejs->attention = 1;
}


/*
 *  Prepend a search path to the system defaults
 */
void ejsSetSearchPath(EjsService *sp, cchar *newpath)
{
    char    *syspath;

    mprAssert(sp);
    mprAssert(sp->ejsPath);

    setEjsPath(sp);
    syspath = sp->ejsPath;

    mprAllocSprintf(sp, &sp->ejsPath, -1, "%s" MPR_SEARCH_DELIM "%s", newpath, syspath);
    mprFree(syspath);
    mprLog(sp, 4, "Search path set to %s", sp->ejsPath);
}


static void setEjsPath(EjsService *sp)
{
    char    *search, *env, *modDir, dir[MPR_MAX_FNAME];

    mprGetAppDir(sp, dir, sizeof(dir));

    mprAllocSprintf(sp, &modDir, -1, "%s/../lib/modules", dir);

    mprAllocSprintf(sp, &search, -1, 
        "%s" MPR_SEARCH_DELIM "%s" MPR_SEARCH_DELIM "%s" MPR_SEARCH_DELIM "%s" MPR_SEARCH_DELIM ".", 
        dir, mprCleanFilename(modDir, modDir), BLD_MOD_PREFIX, BLD_ABS_MOD_DIR);
    mprFree(modDir);

    env = getenv("EJSPATH");
    if (env && *env) {
        mprAllocSprintf(sp, &search, -1, "%s" MPR_SEARCH_DELIM "%s", env, search);
#if FUTURE
        putenv(env);
#endif
    }
    mprFree(sp->ejsPath);
    sp->ejsPath = mprStrdup(sp, search);
    mprMapDelimiters(sp, sp->ejsPath, '/');
}


EjsVar *ejsGetGlobalObject(Ejs *ejs)
{
    return (EjsVar*) ejs->global;
}


void ejsSetHandle(Ejs *ejs, void *handle)
{
    ejs->handle = handle;
}


void *ejsGetHandle(Ejs *ejs)
{
    return ejs->handle;
}


#if BLD_FEATURE_MULTITHREAD && FUTURE
void ejsSetServiceLocks(EjsService *sp, EjsLockFn lock, EjsUnlockFn unlock, void *data)
{
    mprAssert(sp);

    sp->lock = lock;
    sp->unlock = unlock;
    sp->lockData = data;
    return 0;
}
#endif


/*
 *  Evaluate a module file
 */
int ejsEvalModule(cchar *path)
{
    EjsService      *vm;   
    Ejs             *ejs;
    Mpr             *mpr;

    mpr = mprCreate(0, NULL, NULL);
    if ((vm = ejsCreateService(mpr)) == 0) {
        mprFree(mpr);
        return MPR_ERR_NO_MEMORY;
    }
    if ((ejs = ejsCreate(vm, NULL, EJS_FLAG_COMPILER)) == 0) {
        mprFree(mpr);
        return MPR_ERR_NO_MEMORY;
    }
    if (ejsLoadModule(ejs, path, NULL, NULL, 0) < 0) {
        mprFree(mpr);
        return MPR_ERR_CANT_READ;
    }
    if (ejsRun(ejs) < 0) {
        mprFree(mpr);
        return EJS_ERR;
    }
    mprFree(mpr);
    return 0;
}


/*
 *  Run a program. This will run a program assuming that all the required modules are already loaded. It will
 *  optionally service events until instructed to exit.
 */
int ejsRunProgram(Ejs *ejs, cchar *className, cchar *methodName)
{
    /*
     *  Run all module initialization code. This includes plain old scripts.
     */
    if (ejsRun(ejs) < 0) {
        return EJS_ERR;
    }

    /*
     *  Run the requested method. This will block until completion
     */
    if (className || methodName) {
        if (runSpecificMethod(ejs, className, methodName) < 0) {
            return EJS_ERR;
        }
    }

    if (ejs->flags & EJS_FLAG_NOEXIT) {
        /*
         *  This will service events until App.exit() is called
         */
        mprServiceEvents(ejs, -1, 0);
    }

    return 0;
}


/*
 *  Run the specified method in the named class. If methodName is null, default to "main".
 *  If className is null, search for the first class containing the method name.
 */
static int runSpecificMethod(Ejs *ejs, cchar *className, cchar *methodName)
{
    EjsType         *type;
    EjsFunction     *fun;
    EjsName         qname;
    EjsVar          *args;
    int             attributes, i;

    /*
     *  TODO - this is bugged. What about package names? MethodSlot and type are not being set in some cases.
     */
    type = 0;

    if (className == 0 && methodName == 0) {
        return 0;
    }

    if (methodName == 0) {
        methodName = "main";
    }

    /*
     *  Search for the first class with the given name
     */
    if (className == 0) {
        if (searchForMethod(ejs, methodName, &type) < 0) {
            return EJS_ERR;
        }

    } else {
        ejsName(&qname, EJS_PUBLIC_NAMESPACE, className);
        type = (EjsType*) ejsGetPropertyByName(ejs, ejs->global, &qname);
    }

    if (type == 0 || !ejsIsType(type)) {
        mprError(ejs, "Can't find class \"%s\"", className);
        return EJS_ERR;
    }

    ejsName(&qname, EJS_PUBLIC_NAMESPACE, methodName);
    fun = (EjsFunction*) ejsGetPropertyByName(ejs, (EjsVar*) type, &qname);
    if (fun == 0) {
        return MPR_ERR_CANT_ACCESS;
    }
    if (! ejsIsFunction(fun)) {
        mprError(ejs, "Property \"%s\" is not a function");
        return MPR_ERR_BAD_STATE;
    }

    attributes = ejsGetTypePropertyAttributes(ejs, (EjsVar*) type, fun->slotNum);
    if (!(attributes & EJS_ATTR_STATIC)) {
        mprError(ejs, "Method \"%s\" is not declared static");
        return EJS_ERR;
    }

    args = (EjsVar*) ejsCreateArray(ejs, ejs->argc);
    for (i = 0; i < ejs->argc; i++) {
        ejsSetProperty(ejs, args, i, (EjsVar*) ejsCreateString(ejs, ejs->argv[i]));
    }

    if (ejsRunFunction(ejs, fun, 0, 1, &args) == 0) {
        return EJS_ERR;
    }
    return 0;
}


/*
 *  Search for the named method in all types.
 */
static int searchForMethod(Ejs *ejs, cchar *methodName, EjsType **typeReturn)
{
    EjsFunction *method;
    EjsType     *type;
    EjsName     qname;
    EjsVar      *global, *vp;
    int         globalCount, slotNum, methodCount;
    int         methodSlot;

    mprAssert(methodName && *methodName);
    mprAssert(typeReturn);

    global = ejs->global;
    globalCount = ejsGetPropertyCount(ejs, global);

    /*
     *  Search for the named method in all types
     */
    for (slotNum = 0; slotNum < globalCount; slotNum++) {
        vp = ejsGetProperty(ejs, global, slotNum);
        if (vp == 0 || !ejsIsType(vp)) {
            continue;
        }
        type = (EjsType*) vp;

        methodCount = ejsGetPropertyCount(ejs, (EjsVar*) type);

        for (methodSlot = 0; methodSlot < methodCount; methodSlot++) {
            method = (EjsFunction*) ejsGetProperty(ejs, (EjsVar*) type, methodSlot);
            if (method == 0) {
                continue;
            }

            qname = ejsGetPropertyName(ejs, (EjsVar*) type, methodSlot);
            if (qname.name && strcmp(qname.name, methodName) == 0) {
                *typeReturn = type;
            }
        }
    }
    return 0;
}


static void logHandler(MprCtx ctx, int flags, int level, const char *msg)
{
    Mpr         *mpr;
    MprFile     *file;
    char        *prefix;

    mpr = mprGetMpr(ctx);
    file = (MprFile*) mpr->logHandlerData;
    prefix = mpr->name;

    while (*msg == '\n') {
        mprFprintf(file, "\n");
        msg++;
    }

    if (flags & MPR_LOG_SRC) {
        mprFprintf(file, "%s: %d: %s\n", prefix, level, msg);

    } else if (flags & MPR_ERROR_SRC) {
        /*
         *  Use static printing to avoid malloc when the messages are small.
         *  This is important for memory allocation errors.
         */
        if (strlen(msg) < (MPR_MAX_STRING - 32)) {
            mprStaticErrorPrintf(file, "%s: Error: %s\n", prefix, msg);
        } else {
            mprFprintf(file, "%s: Error: %s\n", prefix, msg);
        }

    } else if (flags & MPR_FATAL_SRC) {
        mprFprintf(file, "%s: Fatal: %s\n", prefix, msg);
        
    } else if (flags & MPR_RAW) {
        mprFprintf(file, "%s", msg);
    }
}


int ejsStartLogging(Mpr *mpr, char *logSpec)
{
    MprFile     *file;
    char        *levelSpec;
    int         level;

    level = 0;
    logSpec = mprStrdup(mpr, logSpec);

    //  TODO - move should not be changinging logSpec.
    if ((levelSpec = strchr(logSpec, ':')) != 0) {
        *levelSpec++ = '\0';
        level = atoi(levelSpec);
    }

    if (strcmp(logSpec, "stdout") == 0) {
        file = mpr->fileService->console;

    } else if (strcmp(logSpec, "stderr") == 0) {
        file = mpr->fileService->error;

    } else {
        if ((file = mprOpen(mpr, logSpec, O_CREAT | O_WRONLY | O_TRUNC | O_TEXT, 0664)) == 0) {
            mprErrorPrintf(mpr, "Can't open log file %s\n", logSpec);
            mprFree(logSpec);
            return EJS_ERR;
        }
    }

    mprSetLogLevel(mpr, level);
    mprSetLogHandler(mpr, logHandler, (void*) file);

    mprFree(logSpec);
    return 0;
}


/*
 *  Global memory allocation handler
 */
void ejsMemoryFailure(MprCtx ctx, uint size, uint total, bool granted)
{
    if (!granted) {
        //  TODO use mprPrintError
        mprPrintf(ctx, "Can't allocate memory block of size %d\n", size);
        mprPrintf(ctx, "Total memory used %d\n", total);
        exit(255);
    }
    mprPrintf(ctx, "Memory request for %d bytes exceeds memory red-line\n", size);
    mprPrintf(ctx, "Total memory used %d\n", total);

    //  TODO - should we not do something more here (run GC if running?)
}


void ejsReportError(Ejs *ejs, char *fmt, ...)
{
    va_list     arg;
    const char  *msg;
    char        *buf;

    va_start(arg, fmt);
    
    mprAllocVsprintf(ejs, &buf, 0, fmt, arg);
    
    /*
     *  Compiler error format is:
     *      program:line:errorCode:SEVERITY: message
     *  Where program is either "ec" or "ejs"
     *  Where SEVERITY is either "error" or "warn"
     */
    msg = ejsGetErrorMsg(ejs, 1);
    
    mprError(ejs, "%s", (msg) ? msg: buf);
    mprFree(buf);
    va_end(arg);
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
