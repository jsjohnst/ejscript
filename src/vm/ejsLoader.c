/**
 *  ejsLoader.c - Ejscript module file file loader
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

//  TODO - refactor entire loader

/********************************** Includes **********************************/

#include    "ejs.h"

/****************************** Forward Declarations **************************/

static int  addFixup(Ejs *ejs, int kind, EjsVar *target, int slotNum, EjsTypeFixup *fixup);
static EjsTypeFixup *createFixup(Ejs *ejs, EjsName *qname, int slotNum);
static int  fixupTypes(Ejs *ejs);
static int  loadBlockSection(Ejs *ejs, MprFile *file, EjsModule *mp);
static int  loadClassSection(Ejs *ejs, MprFile *file, EjsModule *mp);
static int  loadDependencySection(Ejs *ejs, MprFile *file, EjsModule *mp);
static int  loadEndBlockSection(Ejs *ejs, MprFile *file, EjsModule *mp);
static int  loadEndFunctionSection(Ejs *ejs, MprFile *file, EjsModule *mp);
static int  loadEndClassSection(Ejs *ejs, MprFile *file, EjsModule *mp);
static int  loadEndModuleSection(Ejs *ejs, MprFile *file, EjsModule *mp);
static int  loadExceptionSection(Ejs *ejs, MprFile *file, EjsModule *mp);
static int  loadFunctionSection(Ejs *ejs, MprFile *file, EjsModule *mp);
static EjsModule *loadModuleSection(Ejs *ejs, MprFile *file, EjsModuleHdr *hdr, int *created, int flags);
static int  loadSections(Ejs *ejs, MprFile *file, EjsModuleHdr *hdr, MprList *modules, int flags);
static int  loadPropertySection(Ejs *ejs, MprFile *file, EjsModule *mp, int sectionType);
static int  loadScriptModule(Ejs *ejs, MprFile *file, cchar *path, MprList *modules, int flags);
static int  readNumber(Ejs *ejs, MprFile *file, int *number);
static char *tokenToString(EjsModule *mp, int   token);

#if !BLD_FEATURE_STATIC
static int  loadNativeLibrary(Ejs *ejs, cchar *name, cchar *path);
#endif

#if BLD_FEATURE_EJS_DOC
static int  loadDocSection(Ejs *ejs, MprFile *file, EjsModule *mp);
static void setDoc(Ejs *ejs, EjsModule *mp, EjsVar *block, int slotNum);
#endif

/******************************************************************************/
/**
 *  Load a module file and return a list of the loaded modules. This is used to load scripted module files with
 *  optional native (shared / DLL) implementations. If loading a scripted module that has native declarations, a
 *  search for the corresponding native DLL will be performed and both scripted and native module files will be loaded.
 *  NOTE: this may recursively call itself as it loads dependent modules.
 *
 *  @param ejs Ejs handle
 *  @param nameArg Module name to load. May be "." separated path. May include or omit the ".mod" extension.
 *  @param url Optional URL to locate the module. Currently ignored
 *  @param callback Callback notification on load events.
 *  @param flags Reserved. Must be set to zero.
 *  @return Returns the last loaded module.
 *
 *  TODO - refactor. cleanup error handling
 */
MprList *ejsLoadModule(Ejs *ejs, cchar *nameArg, cchar *url, EjsLoaderCallback callback, int flags)
{
    MprFile             *file;
    MprList             *modules;
    EjsNativeCallback   moduleCallback;
    EjsModule           *mp;
    char                *cp, *path, cwd[MPR_MAX_FNAME], dir[MPR_MAX_FNAME], name[MPR_MAX_FNAME], filename[MPR_MAX_FNAME];
    int                 rc, alreadyLoading, next;

    mprAssert(nameArg && *nameArg);
    path = 0;

    /*
     *  Add .mod extension
     */
    mprStrcpy(name, sizeof(name), nameArg);
    if ((cp = strrchr(name, '.')) != 0 && strcmp(cp, EJS_MODULE_EXT) == 0) {
        *cp = '\0';
    }
    mprSprintf(filename, sizeof(filename), "%s%s", name, EJS_MODULE_EXT);

    /*
     *  TODO should distinguish between module names and module names?  A module may not match the name of any module.
     */
    mp = ejsLookupModule(ejs, name);
    if (mp && mp->loaded) {
        return mprCreateList(ejs);
    }

    mp = 0;
    ejs->loaderCallback = callback;

    /* TODO - Refactor need api for ejs->flags */
    alreadyLoading = ejs->flags & EJS_FLAG_LOADING;
    ejs->flags |= EJS_FLAG_LOADING;
    if (!alreadyLoading) {
        //  TODO - not a great place to store this
        if ((ejs->typeFixups = mprCreateList(ejs)) == 0) {
            ejsThrowMemoryError(ejs);
            return 0;
        }
    }

    if (ejsSearch(ejs, &path, filename) < 0) {
        mprLog(ejs, 2, "Can't find module file \"%s.mod\" in search path \"%s\"", name,
            ejs->service->ejsPath ? ejs->service->ejsPath : "");
        ejsThrowReferenceError(ejs,  "Can't find module file \"%s", filename);
        return 0;
    }
    mprAssert(path);

    getcwd(cwd, sizeof(cwd));
    mprLog(ejs, 3, "Loading module %s, cwd %s", path, cwd);

    if ((file = mprOpen(ejs, path, O_RDONLY | O_BINARY, 0666)) == 0) {
        ejsThrowIOError(ejs, "Can't open module file %s", path);
        mprFree(path);
        return 0;
    }
    mprEnableFileBuffering(file, 0, 0);

    modules = mprCreateList(ejs);
    if (modules == 0) {
        mprFree(file);
        mprFree(path);
        ejsThrowMemoryError(ejs);
        return 0;
    }

    if (loadScriptModule(ejs, file, path, modules, flags) < 0) {
        mprFree(file);
        mprFree(modules);
        mprFree(path);
        return 0;
    }
    mprGetDirName(dir, sizeof(dir), path);

    /*
     *  Fixup type references across all modules. This solves the forward type reference problem.
     */
    if (! alreadyLoading) {
        ejs->flags &= ~EJS_FLAG_LOADING;
        if ((rc = fixupTypes(ejs)) < 0) {
            mprFree(file);
            mprFree(modules);
            mprFree(path);
            return 0;
        }
        mprFree(ejs->typeFixups);
        ejs->typeFixups = 0;
    }

    for (next = 0; (mp = mprGetNextItem(modules, &next)) != 0; ) {

        if (mp->hasNative && !mp->configured) {
            /*
             *  See if a native module initialization routine has been registered. If so, use that. Otherwise, look
             *  for a backing DSO.
             */
            if ((moduleCallback = (EjsNativeCallback) mprLookupHash(ejs->nativeModules, mp->name)) != 0) {
                if ((moduleCallback)(ejs, mp, path) < 0) {
                    if (ejs->exception == 0) {
                        ejsThrowIOError(ejs, "Can't load the native module file \"%s\"", path);
                    }
                    mprFree(file);
                    mprFree(modules);
                    mprFree(path);
                    return 0;
                }
                
            } else {
#if !BLD_FEATURE_STATIC
                rc = loadNativeLibrary(ejs, mp->name, dir);
                if (rc < 0) {
                    if (ejs->exception == 0) {
                        ejsThrowIOError(ejs, "Can't load the native module file \"%s\"", path);
                    }
                    mprFree(file);
                    mprFree(modules);
                    mprFree(path);
                    return 0;
                }
#endif
            }
        }

        mp->configured = 1;
            
        if (!(ejs->flags & EJS_FLAG_NO_EXE)) {
            if (ejsRunInitializer(ejs, mp) == 0) {
                mprFree(modules);
                mprFree(file);
                mprFree(path);
                return 0;
            }
        }
    }

    mprFree(file);
    mprFree(path);

    return modules;
}


/*
 *  Load the sections: classes, properties and functions. Return the first module loaded in pup.
 */
static int loadSections(Ejs *ejs, MprFile *file, EjsModuleHdr *hdr, MprList *modules, int flags)
{
    EjsModule   *mp, *firstModule;
    int         rc, sectionType, created, oldGen;

    created = 0;

    firstModule = mp = 0;

    /*
     *  For now, all loaded types are eternal.
     *  TODO - OPT. should relax this.
     */
    oldGen = ejsSetGeneration(ejs, EJS_GEN_ETERNAL);

    while ((sectionType = mprGetc(file)) >= 0) {

        if (sectionType < 0 || sectionType >= EJS_SECT_MAX) {
            //  TODO
            mprError(ejs, "Bad section type %d in %s", sectionType, mp->name);
            ejsSetGeneration(ejs, oldGen);
            return EJS_ERR;
        }
        mprLog(ejs, 7, "Load section type %d", sectionType);

        rc = 0;
        switch (sectionType) {

        case EJS_SECT_BLOCK:
            rc = loadBlockSection(ejs, file, mp);
            break;

        case EJS_SECT_BLOCK_END:
            rc = loadEndBlockSection(ejs, file, mp);
            break;

        case EJS_SECT_CLASS:
            rc = loadClassSection(ejs, file, mp);
            break;

        case EJS_SECT_CLASS_END:
            rc = loadEndClassSection(ejs, file, mp);
            break;

        case EJS_SECT_DEPENDENCY:
            rc = loadDependencySection(ejs, file, mp);
            break;

        case EJS_SECT_EXCEPTION:
            rc = loadExceptionSection(ejs, file, mp);
            break;

        case EJS_SECT_FUNCTION:
            rc = loadFunctionSection(ejs, file, mp);
            break;

        case EJS_SECT_FUNCTION_END:
            rc = loadEndFunctionSection(ejs, file, mp);
            break;

        case EJS_SECT_MODULE:
            mp = loadModuleSection(ejs, file, hdr, &created, flags);
            if (mp == 0) {
                ejsSetGeneration(ejs, oldGen);
                return 0;
            }
            if (firstModule == 0) {
                firstModule = mp;
            }
            ejsAddModule(ejs, mp);
            mprAddItem(modules, mp);
            break;

        case EJS_SECT_MODULE_END:
            rc = loadEndModuleSection(ejs, file, mp);
            break;

        case EJS_SECT_PROPERTY:
            rc = loadPropertySection(ejs, file, mp, sectionType);
            break;

#if BLD_FEATURE_EJS_DOC
        case EJS_SECT_DOC:
            rc = loadDocSection(ejs, file, mp);
            break;
#endif

        default:
            mprAssert(0);
            ejsSetGeneration(ejs, oldGen);
            return EJS_ERR;
        }

        if (rc < 0) {
            if (mp && mp->name && created) {
                ejsRemoveModule(ejs, mp);
                mprRemoveItem(modules, mp);
                mprFree(mp);
            }
            ejsSetGeneration(ejs, oldGen);
            return rc;
        }
    }

    ejsSetGeneration(ejs, oldGen);
    return 0;
}


/*
 *  Load a module section and constant pool.
 */
static EjsModule *loadModuleSection(Ejs *ejs, MprFile *file, EjsModuleHdr *hdr, int *created, int flags)
{
    EjsModule   *mp;
    char        *pool, *name, *url;
    int         rc, poolSize, nameToken, urlToken;

    mprAssert(created);

    *created = 0;

    /*
     *  We don't have the constant pool yet so we cant resolve the name yet.
     */
    if (readNumber(ejs, file, &nameToken) < 0) {
        return 0;
    }
    if (readNumber(ejs, file, &urlToken) < 0) {
        return 0;
    }

    if (readNumber(ejs, file, &poolSize) < 0) {
        //  TODO DIAG on all returns
        return 0;
    }
    if (poolSize <= 0 || poolSize > EJS_MAX_POOL) {
        return 0;
    }

    /*
     *  Read the string constant pool
     */
    pool = (char*) mprAlloc(file, poolSize);
    if (pool == 0) {
        return 0;
    }
    if (mprRead(file, pool, poolSize) != poolSize) {
        mprFree(pool);
        return 0;
    }

    /*
     *  Convert module token into a name
     */
    //  TODO - need an API for this
    if (nameToken < 0 || nameToken >= poolSize) {
        //  TODO DIAG
        mprAssert(0);
        return 0;
    }
    name = &pool[nameToken];
    if (name == 0) {
        mprAssert(name);
        mprFree(pool);
        return 0;
    }
    if (urlToken < 0 || urlToken >= poolSize) {
        //  TODO DIAG
        mprAssert(0);
        return 0;
    }
    url = &pool[urlToken];
    if (url == 0) {
        mprAssert(url);
        mprFree(pool);
        return 0;
    }

    /*
     *  Check if the module is already loaded
     */
    rc = ejsCheckModuleLoaded(ejs, name);
    if (rc < 0) {
        mprFree(pool);
        return 0;

    } else if (rc == 1) {
        mprFree(pool);
        return ejsLookupModule(ejs, name);
    }

    mp = ejsCreateModule(ejs, name, url, pool, poolSize);
    if (mp == 0) {
        mprFree(pool);
        return 0;
    }
    *created = 1;

    if (strcmp(name, EJS_DEFAULT_MODULE) != 0) {
        /*
         *  Signify that loading the module has begun. We allow multiple loads into the default module.
         */
        mp->loaded = 1;
        mp->constants->locked = 1;
    }
    if (hdr->flags & EJS_MODULE_BOUND_GLOBALS) {
        mp->boundGlobals = 1;
    }

    mp->file = file;
    mp->flags = flags;
#if UNUSED
    mp->seq = hdr->seq;
#endif

    mp->firstGlobalSlot = ejsGetPropertyCount(ejs, ejs->global);

    if (ejs->loaderCallback) {
        (ejs->loaderCallback)(ejs, EJS_SECT_MODULE, mp);
    }

    mprLog(ejs, 6, "Load module section %s", name);

    return mp;
}


static int loadEndModuleSection(Ejs *ejs, MprFile *file, EjsModule *mp)
{
    mprLog(ejs, 7, "End module section %s", mp->name);

    if (ejs->loaderCallback) {
        (ejs->loaderCallback)(ejs, EJS_SECT_MODULE_END, mp);
    }

    return 0;
}


static int loadDependencySection(Ejs *ejs, MprFile *file, EjsModule *mp)
{
    EjsModule   *module;
    MprList     *modules;
    void        *callback;
    char        *name, *url;
    int         next;

    mprAssert(ejs);
    mprAssert(file);
    mprAssert(mp);

    name = ejsModuleReadString(ejs, mp);
    url = ejsModuleReadString(ejs, mp);

    if (mp->hasError) {
        return MPR_ERR_CANT_READ;
    }
    
    mprLog(ejs, 7, "    Load dependency section %s", name);

    callback = ejs->loaderCallback;
    modules = ejsLoadModule(ejs, name, url, NULL, mp->flags);
    ejs->loaderCallback = callback;

    if (modules == 0) {
        return MPR_ERR_CANT_READ;
    }

    if (mp->dependencies == 0) {
        mp->dependencies = mprCreateList(mp);
    }

    for (next = 0; (module = mprGetNextItem(modules, &next)) != 0; ) {
        mprAddItem(mp->dependencies, module);

        if (ejs->loaderCallback) {
            (ejs->loaderCallback)(ejs, EJS_SECT_DEPENDENCY, module, name, url);
        }
    }
    return 0;
}


static int loadBlockSection(Ejs *ejs, MprFile *file, EjsModule *mp)
{
    EjsBlock    *block;
    EjsVar      *owner;
    EjsName     qname;
    int         slotNum, numSlot;

    qname.space = EJS_BLOCK_NAMESPACE;
    qname.name = ejsModuleReadString(ejs, mp);
    ejsModuleReadNumber(ejs, mp, &slotNum);
    ejsModuleReadNumber(ejs, mp, &numSlot);

    if (mp->hasError) {
        return MPR_ERR_CANT_READ;
    }
    
    block = ejsCreateBlock(ejs, qname.name, numSlot);
    owner = (EjsVar*) mp->scopeChain;

    if (ejsLookupProperty(ejs, owner, &qname) >= 0) {
        ejsThrowReferenceError(ejs, "Block \"%s\" already loaded", qname.name);
        return MPR_ERR_CANT_CREATE;
    }

    if (owner == ejs->global && !mp->boundGlobals) {
        slotNum = -1;
    }
    slotNum = ejsDefineProperty(ejs, owner, slotNum, &qname, ejs->blockType, 0, (EjsVar*) block);
    if (slotNum < 0) {
        return MPR_ERR_CANT_WRITE;
    }

    if (ejs->loaderCallback) {
        (ejs->loaderCallback)(ejs, EJS_SECT_BLOCK, mp, owner, slotNum, qname.name, numSlot, block);
    }

    block->scopeChain = mp->scopeChain;
    mp->scopeChain = block;

    return 0;
}


static int loadEndBlockSection(Ejs *ejs, MprFile *file, EjsModule *mp)
{
    mprLog(ejs, 7, "    End block section %s", mp->name);

    if (ejs->loaderCallback) {
        (ejs->loaderCallback)(ejs, EJS_SECT_BLOCK_END, mp);
    }

    mp->scopeChain = mp->scopeChain->scopeChain;

    return 0;
}


static int loadClassSection(Ejs *ejs, MprFile *file, EjsModule *mp)
{
    EjsType         *type, *baseType, *iface, *nativeType;
    EjsTypeFixup    *fixup, *ifixup;
    EjsName         qname, baseClassName, ifaceClassName;
    EjsBlock        *block;
    int             attributes, numTypeProp, numInstanceProp, slotNum, numInterfaces, i;

    fixup = 0;
    ifixup = 0;
    
    qname.name = ejsModuleReadString(ejs, mp);
    qname.space = ejsModuleReadString(ejs, mp);
    ejsModuleReadNumber(ejs, mp, &attributes);
    ejsModuleReadNumber(ejs, mp, &slotNum);
    ejsModuleReadType(ejs, mp, &baseType, &fixup, &baseClassName, 0);
    ejsModuleReadNumber(ejs, mp, &numTypeProp);
    ejsModuleReadNumber(ejs, mp, &numInstanceProp);
    ejsModuleReadNumber(ejs, mp, &numInterfaces);

    if (mp->hasError) {
        return MPR_ERR_CANT_READ;
    }
    if (ejsLookupProperty(ejs, ejs->global, &qname) >= 0) {
        ejsThrowReferenceError(ejs, "Class \"%s\" already loaded", qname.name);
        return MPR_ERR_CANT_CREATE;
    }
    if (fixup || (baseType && baseType->needFixup)) {
        attributes |= EJS_ATTR_SLOTS_NEED_FIXUP;
    }

    /*
     *  Find pre-existing native types.
     */
    if (attributes & EJS_ATTR_NATIVE) {
        type = nativeType = (EjsType*) mprLookupHash(ejs->coreTypes, qname.name);
        if (type == 0) {
            mprLog(ejs, 1, "WARNING: can't find native type \"%s\"", qname.name);
        }
    } else {
        type = nativeType = 0;
#if BLD_DEBUG
        if (mprLookupHash(ejs->coreTypes, qname.name)) {
            mprError(ejs, "WARNING: type \"%s\" defined as a native type but not declared as native", qname.name);
        }
#endif
    }

    if (mp->flags & EJS_MODULE_BUILTIN) {
        attributes |= EJS_ATTR_BUILTIN;
    }
    if (attributes & EJS_ATTR_SLOTS_NEED_FIXUP) {
        baseType = 0;
        if (fixup == 0) {
            // TODO - was &qname, slotNum
            fixup = createFixup(ejs, (baseType) ? &baseType->qname : &ejs->objectType->qname, -1);
        }
    }
    
    mprAssert(slotNum >= 0);
    
    /*
     *  If the module is fully bound (--merge), then we install the type at the prescribed slot number.
     */
    if (! mp->boundGlobals) {
        slotNum = -1;
    }
    if (slotNum < 0) {
        slotNum = ejs->globalBlock->obj.numProp;
    }
    
    if (type == 0) {
        attributes |= EJS_ATTR_OBJECT | EJS_ATTR_OBJECT_HELPERS;
        type = ejsCreateType(ejs, &qname, mp, baseType, sizeof(EjsObject), slotNum, numTypeProp, numInstanceProp, 
            attributes, 0);
        if (type == 0) {
            ejsThrowInternalError(ejs, "Can't create class %s", qname.name);
            return MPR_ERR_BAD_STATE;
        }

    } else {
        mp->hasNative = 1;
        if (attributes & EJS_ATTR_HAS_CONSTRUCTOR && !type->hasConstructor) {
            mprError(ejs, "WARNING: module indicates a constructor required but none exists for \"%s\"", type->qname.name);
        }
        if (!type->block.obj.var.native) {
            mprError(ejs, "WARNING: type not defined as native: \"%s\"", type->qname.name);
        }
    }
    
    /*
     *  Read implemented interfaces. Add to type->implements. Create fixup record if the interface type is not yet known.
     */
    if (numInterfaces > 0) {
        type->implements = mprCreateList(type);
        for (i = 0; i < numInterfaces; i++) {
            if (ejsModuleReadType(ejs, mp, &iface, &ifixup, &ifaceClassName, 0) < 0) {
                return MPR_ERR_CANT_READ;
            }
            if (iface) {
                mprAddItem(type->implements, iface);
            } else {
                if (addFixup(ejs, EJS_FIXUP_INTERFACE_TYPE, (EjsVar*) type, -1, ifixup) < 0) {
                    ejsThrowMemoryError(ejs);
                    return MPR_ERR_NO_MEMORY;
                }
            }
        }
    }

    if (mp->flags & EJS_MODULE_BUILTIN) {
        type->block.obj.var.builtin = 1;
    }
    if (attributes & EJS_ATTR_HAS_STATIC_INITIALIZER) {
        type->hasStaticInitializer = 1;
    }
    if (attributes & EJS_ATTR_DYNAMIC_INSTANCE) {
        type->dynamicInstance = 1;
    }

    mprLog(ejs, 6, "    Load %s class %s for module %s at slot %d", qname.space, qname.name, mp->name, slotNum);

    slotNum = ejsDefineProperty(ejs, ejs->global, slotNum, &qname, ejs->typeType, attributes, (EjsVar*) type);
    if (slotNum < 0) {
        ejsThrowMemoryError(ejs);
        return MPR_ERR_NO_MEMORY;
    }
    type->module = mp;

    if (fixup) {
        if (addFixup(ejs, EJS_FIXUP_BASE_TYPE, (EjsVar*) type, -1, fixup) < 0) {
            ejsThrowMemoryError(ejs);
            return MPR_ERR_NO_MEMORY;
        }
        
    } else {
        if (ejs->flags & EJS_FLAG_EMPTY) {
            if (attributes & EJS_ATTR_NATIVE) {
                /*
                 *  When empty, native types are created with no properties and with numTraits equal to zero. 
                 *  This is so the compiler can compile the core ejs module. For ejsmod which may also run in 
                 *  empty mode, we set numInherited here to the correct value for native types.
                 */
                if (type->block.numInherited == 0 && type->baseType) {
                    type->block.numInherited = type->baseType->block.numTraits;
                }
            }

#if FUTURE
        } else {
            if (nativeType && !type->isInterface) {
                /*
                 *  Inherit native methods. Need to clone the function if it is a native function
                 */
                mprAssert(baseType);
                for (i = 0; i < type->block.obj.numProp; i++) {
                    existingFun = (EjsFunction*) ejsGetProperty(ejs, (EjsVar*) type, i);
                    if (ejsIsFunction(existingFun)) {
                        fun = (EjsFunction*) ejsGetProperty(ejs, (EjsVar*) baseType, i);
                        if (existingFun->override) {
                            continue;
                        }
                        if (ejsIsNativeFunction(fun)) {
                            fun = (EjsFunction*) ejsCloneVar(ejs, fun);
                        }
                        ejsSetProperty(ejs, (EjsVar*) type, i, (EjsVar*) fun);
                    }
                }
            }
#endif
        }
    }

#if BLD_FEATURE_EJS_DOC
    setDoc(ejs, mp, ejs->global, slotNum);
#endif

    block = (EjsBlock*) type;
    block->scopeChain = mp->scopeChain;
    mp->scopeChain = block;

    if (ejs->loaderCallback) {
        (ejs->loaderCallback)(ejs, EJS_SECT_CLASS, mp, slotNum, qname, type, attributes);
    }

    return 0;
}


static int loadEndClassSection(Ejs *ejs, MprFile *file, EjsModule *mp)
{
    EjsType     *type;

    mprLog(ejs, 7, "    End class section");

    if (ejs->loaderCallback) {
        (ejs->loaderCallback)(ejs, EJS_SECT_CLASS_END, mp, mp->scopeChain);
    }

    type = (EjsType*) mp->scopeChain;
    if (type->block.hasScriptFunctions && type->baseType) {
        ejsDefineTypeNamespaces(ejs, type);
    }
    mp->scopeChain = mp->scopeChain->scopeChain;

    return 0;
}


//  TODO - break into functions.

static int loadFunctionSection(Ejs *ejs, MprFile *file, EjsModule *mp)
{
    EjsType         *returnType;
    EjsTypeFixup    *fixup;
    EjsFunction     *fun;
    EjsName         qname, returnTypeName;
    EjsBlock        *block;
    uchar           *code;
    int             slotNum, numArgs, codeLen, numLocals, numExceptions, attributes, nextSlot, lang;

    lang = 0;

    qname.name = ejsModuleReadString(ejs, mp);
    qname.space = ejsModuleReadString(ejs, mp);
    ejsModuleReadNumber(ejs, mp, &nextSlot);
    ejsModuleReadNumber(ejs, mp, &attributes);
    ejsModuleReadByte(ejs, mp, &lang);
 
    ejsModuleReadType(ejs, mp, &returnType, &fixup, &returnTypeName, 0);
    ejsModuleReadNumber(ejs, mp, &slotNum);
    ejsModuleReadNumber(ejs, mp, &numArgs);
    ejsModuleReadNumber(ejs, mp, &numLocals);
    ejsModuleReadNumber(ejs, mp, &numExceptions);
    ejsModuleReadNumber(ejs, mp, &codeLen);

    if (mp->hasError) {
        return MPR_ERR_CANT_READ;
    }

    block = (EjsBlock*) mp->scopeChain;
    mprAssert(block);
    mprAssert(numArgs >= 0 && numArgs < EJS_MAX_ARGS);
    mprAssert(numLocals >= 0 && numLocals < EJS_MAX_LOCALS);
    mprAssert(numExceptions >= 0 && numExceptions < EJS_MAX_EXCEPTIONS);

    mprLog(ejs, 6, "Loading function %s:%s at slot %d", qname.space, qname.name, slotNum);

    /*
     *  Read the code. We don't need to store the code length as the verifier will ensure we always have returns 
     *  in all cases. ie. can't fall off the end. We pass ownership of the code to createMethod i.e. don't free.
     *  TODO - read the code after the function is created. That way won't need to steal the block.
     */
    if (codeLen > 0) {
        code = (uchar*) mprAlloc(ejs, codeLen);
        if (code == 0) {
            return MPR_ERR_NO_MEMORY;
        }
        if (mprRead(file, code, codeLen) != codeLen) {
            mprFree(code);
            return MPR_ERR_CANT_READ;
        }
        block->hasScriptFunctions = 1;
        
    } else {
        code = 0;
    }

    if (attributes & EJS_ATTR_NATIVE) {
        mp->hasNative = 1;
    }
    if (attributes & EJS_ATTR_INITIALIZER) {
        mp->hasInitializer = 1;
    }
    if (mp->flags & EJS_MODULE_BUILTIN) {
        attributes |= EJS_ATTR_BUILTIN;
    }

    if (ejsLookupProperty(ejs, (EjsVar*) block, &qname) >= 0 && !(attributes & EJS_ATTR_OVERRIDE)) {
        if (ejsIsType(block)) {
            ejsThrowReferenceError(ejs,
                "function \"%s\" already defined in type \"%s\". Try adding \"override\" to the function declaration.", 
                qname.name, ((EjsType*) block)->qname.name);
        } else {
            ejsThrowReferenceError(ejs,
                "function \"%s\" already defined in block \"%s\". Try adding \"override\" to the function declaration.", 
                qname.name, block->name);
        }
        return MPR_ERR_CANT_CREATE;
    }

    /*
     *  Create the function using the current scope chain. Non-methods revise this scope chain via the 
     *  DefineFunction op code.
     */
    fun = ejsCreateFunction(ejs, code, codeLen, numArgs, numExceptions, returnType, attributes, mp->constants, 
        mp->scopeChain, lang);
    if (fun == 0) {
        mprFree(code);
        return MPR_ERR_NO_MEMORY;
    }
    mprStealBlock(fun, code);

    ejsSetDebugName(fun, qname.name);

    if (block == (EjsBlock*) ejs->global && !mp->boundGlobals) {
        /*
         *  Global property and not using --merge
         */
        if (attributes & EJS_ATTR_OVERRIDE) {
            //  TODO - namespace
            slotNum = ejsLookupProperty(ejs, (EjsVar*) block, &qname);
            if (slotNum < 0) {
                mprError(ejs, "Can't find method \"%s\" to override", qname.name);
                return MPR_ERR_NO_MEMORY;
            }

        } else {
            slotNum = -1;
        }
    }

    if (mp->flags & EJS_MODULE_BUILTIN) {
        fun->block.obj.var.builtin = 1;
    }

    if (attributes & EJS_ATTR_INITIALIZER && block == (EjsBlock*) ejs->global) {
        mp->initializer = fun;
        fun->isInitializer = 1;
        slotNum = -1;

    } else {
        slotNum = ejsDefineProperty(ejs, (EjsVar*) block, slotNum, &qname, ejs->functionType, attributes, (EjsVar*) fun);
        if (slotNum < 0) {
            return MPR_ERR_NO_MEMORY;
        }
    }
    
    ejsSetNextFunction(fun, nextSlot);

    if (fixup) {
        mprAssert(returnType == 0);
        if (addFixup(ejs, EJS_FIXUP_RETURN_TYPE, (EjsVar*) fun, -1, fixup) < 0) {
            ejsThrowMemoryError(ejs);
            return MPR_ERR_NO_MEMORY;
        }
    }

#if BLD_FEATURE_EJS_DOC
    setDoc(ejs, mp, (EjsVar*) block, slotNum);
#endif

    mp->currentMethod = fun;
    fun->block.scopeChain = mp->scopeChain;
    mp->scopeChain = &fun->block;

    if (ejs->loaderCallback) {
        (ejs->loaderCallback)(ejs, EJS_SECT_FUNCTION, mp, block, slotNum, qname, fun, attributes);
    }

    return 0;
}


static int loadEndFunctionSection(Ejs *ejs, MprFile *file, EjsModule *mp)
{
    EjsTrait            *trait;
    EjsFunction         *fun;
    int                 i;

    mprLog(ejs, 7, "    End function section");

    fun = (EjsFunction*) mp->scopeChain;

    for (i = 0; i < (int) fun->numArgs; i++) {
        trait = ejsGetPropertyTrait(ejs, (EjsVar*) fun, i);
        if (trait && trait->attributes & EJS_ATTR_INITIALIZER) {
            fun->numDefault++;
        }
    }

    if (ejs->loaderCallback) {
        (ejs->loaderCallback)(ejs, EJS_SECT_FUNCTION_END, mp, fun);
    }

    mp->scopeChain = mp->scopeChain->scopeChain;

    return 0;
}


static int loadExceptionSection(Ejs *ejs, MprFile *file, EjsModule *mp)
{
    EjsFunction         *fun;
    EjsType             *catchType;
    EjsTypeFixup        *fixup;
    EjsCode             *code;
    EjsEx               *ex;
    int                 tryStart, tryEnd, handlerStart, handlerEnd;
    int                 flags, i;

    fun = mp->currentMethod;
    mprAssert(fun);

    flags = 0;
    code = &fun->body.code;

    for (i = 0; i < code->numHandlers; i++) {
        ejsModuleReadByte(ejs, mp, &flags);
        ejsModuleReadNumber(ejs, mp, &tryStart);
        ejsModuleReadNumber(ejs, mp, &tryEnd);
        ejsModuleReadNumber(ejs, mp, &handlerStart);
        ejsModuleReadNumber(ejs, mp, &handlerEnd);
        ejsModuleReadType(ejs, mp, &catchType, &fixup, 0, 0);

        if (mp->hasError) {
            return MPR_ERR_CANT_READ;
        }
    
        ex = ejsAddException(fun, tryStart, tryEnd, catchType, handlerStart, handlerEnd, flags, i);

        if (fixup) {
            mprAssert(catchType == 0);
            if (addFixup(ejs, EJS_FIXUP_EXCEPTION, (EjsVar*) ex, 0, fixup) < 0) {
                mprAssert(0);
                return MPR_ERR_NO_MEMORY;
            }
        }
    }

    if (ejs->loaderCallback) {
        (ejs->loaderCallback)(ejs, EJS_SECT_EXCEPTION, mp, fun);
    }

    return 0;
}

int mystrcmp(char *s1, char *s2)
{
    return strcmp(s1, s2);
}

/*
 *  Define a global, class or block property. Not used for function locals or args.
 */
static int loadPropertySection(Ejs *ejs, MprFile *file, EjsModule *mp, int sectionType)
{
    EjsType         *type;
    EjsTypeFixup    *fixup;
    EjsName         qname, propTypeName;
    EjsVar          *block, *value;
    cchar           *str;
    int             slotNum, attributes, fixupKind;

    value = 0;
    block = (EjsVar*) mp->scopeChain;
    mprAssert(block);
    
    qname.name = ejsModuleReadString(ejs, mp);
    qname.space = ejsModuleReadString(ejs, mp);
    ejsModuleReadNumber(ejs, mp, &attributes);
    ejsModuleReadNumber(ejs, mp, &slotNum);
    ejsModuleReadType(ejs, mp, &type, &fixup, &propTypeName, 0);

    //  TODO - temporary and not ideal. Must encode name and uri. Should do this for all constants.
    if (attributes & EJS_ATTR_HAS_VALUE) {
        if ((str = ejsModuleReadString(ejs, mp)) == 0) {
            return MPR_ERR_CANT_READ;
        }
        /*  Only doing for namespaces currently */
        value = (EjsVar*) ejsCreateNamespace(ejs, str, str);
    }

    mprLog(ejs, 7, "Loading property %s:%s at slot %d", qname.space, qname.name, slotNum);

    if (attributes & EJS_ATTR_NATIVE) {
        mp->hasNative = 1;
    }
    if (mp->flags & EJS_MODULE_BUILTIN) {
        attributes |= EJS_ATTR_BUILTIN;
    }

    if (ejsLookupProperty(ejs, block, &qname) >= 0) {
        ejsThrowReferenceError(ejs, "property \"%s\" already loaded", qname.name);
        return MPR_ERR_CANT_CREATE;
    }

    if (ejsIsFunction(block)) {
        fixupKind = EJS_FIXUP_LOCAL;

    } else if (ejsIsType(block) && !(attributes & EJS_ATTR_STATIC) && block != ejs->global) {
        mprAssert(((EjsType*) block)->instanceBlock);
        block = (EjsVar*) ((EjsType*) block)->instanceBlock;
        fixupKind = EJS_FIXUP_INSTANCE_PROPERTY;

    } else {
        fixupKind = EJS_FIXUP_TYPE_PROPERTY;
    }

    if (block == ejs->global && !mp->boundGlobals) {
        slotNum = -1;
    }

    slotNum = ejsDefineProperty(ejs, block, slotNum, &qname, type, attributes, value);
    if (slotNum < 0) {
        return MPR_ERR_CANT_WRITE;
    }

    if (fixup) {
        mprAssert(type == 0);
        if (addFixup(ejs, fixupKind, block, slotNum, fixup) < 0) {
            ejsThrowMemoryError(ejs);
            return MPR_ERR_NO_MEMORY;
        }
    }

#if BLD_FEATURE_EJS_DOC
    setDoc(ejs, mp, block, slotNum);
#endif

    if (ejs->loaderCallback) {
        (ejs->loaderCallback)(ejs, EJS_SECT_PROPERTY, mp, block, slotNum, qname, attributes, propTypeName);
    }

    return 0;
}


#if BLD_FEATURE_EJS_DOC
static int loadDocSection(Ejs *ejs, MprFile *file, EjsModule *mp)
{
    char        *doc;

    mprLog(ejs, 7, "    Documentation section");

    doc = ejsModuleReadString(ejs, mp);

    if (ejs->flags & EJS_FLAG_DOC) {
        mp->doc = doc;
        if (ejs->loaderCallback) {
            (ejs->loaderCallback)(ejs, EJS_SECT_DOC, doc);
        }
    }
    return 0;
}
#endif



#if !BLD_FEATURE_STATIC
#if UNUSED
static bool sharedLibrary(cchar *path)
{
    int     len, extLen;

    len = (int) strlen(path);
    extLen = (int) strlen(BLD_SHOBJ);

    if (len <= extLen || mprStrcmpAnyCase(&path[len - extLen], BLD_SHOBJ) != 0) {
        return 0;
    }
    return 1;
}
#endif


/*
 *  Check if a native module exists at the given path. If so, load it. If the path is a scripted module
 *  but has a corresponding native module, then load that. Return 1 if loaded, -1 for errors, 0 if no
 *  native module found.
 */
static int loadNativeLibrary(Ejs *ejs, cchar *name, cchar *dir)
{
    char    path[MPR_MAX_PATH], initName[MPR_MAX_PATH], moduleName[MPR_MAX_PATH], *cp;

    if (ejs->flags & EJS_FLAG_NO_EXE) {
        return 0;
    }

    mprSprintf(path, sizeof(path), "%s/%s%s", dir, name, BLD_SHOBJ);
    if (! mprAccess(ejs, path, R_OK)) {
        return 0;
    }

    /*
     *  Build the DSO entry point name. Format is "NameModuleInit" where Name has "." converted to "_"
     */
    mprStrcpy(moduleName, sizeof(moduleName), name);
    moduleName[0] = tolower(moduleName[0]);
    mprSprintf(initName, sizeof(initName), "%sModuleInit", moduleName);
    for (cp = initName; *cp; cp++) {
        if (*cp == '.') {
            *cp = '_';
        }
    }

    if (mprLookupModule(ejs, name) != 0) {
        mprLog(ejs, 1, "Native module \"%s\" is already loaded", path);
        return 0;
    }

    if (mprLoadModule(ejs, path, initName) == 0) {
        return MPR_ERR_CANT_OPEN;
    }
    return 1;
}
#endif


/*
 *  Load a scripted module file. Return a modified list of modules.
 */
static int loadScriptModule(Ejs *ejs, MprFile *file, cchar *path, MprList *modules, int flags)
{
    EjsModuleHdr    hdr;

    mprAssert(path);

    /*
     *  Read module file header
     *  TODO - module header byte order
     */
    if ((mprRead(file, &hdr, sizeof(hdr))) != sizeof(hdr)) {
        //  TODO - should not throw exceptions in this file. It is used by the compiler.
        ejsThrowIOError(ejs, "Error reading module file %s, corrupt header", path);
        return EJS_ERR;
    }

    if ((int) hdr.magic != EJS_MODULE_MAGIC) {
        ejsThrowIOError(ejs, "Bad module file format in %s", path);
        return EJS_ERR;
    }
    if ((int) hdr.major != EJS_MAJOR || (int) hdr.minor != EJS_MINOR) {
        ejsThrowIOError(ejs, "Incompatible module file format in %s", path);
        return EJS_ERR;
    }

    if (ejs->loaderCallback) {
        (ejs->loaderCallback)(ejs, EJS_SECT_START, path, &hdr);
    }

    /*
     *  Load the sections: classes, properties and functions.
     *  NOTE: this may load multiple modules.
     */
    if (loadSections(ejs, file, &hdr, modules, flags) < 0) {
        if (ejs->exception == 0) {
            ejsThrowReferenceError(ejs, "Can't load module file %s", path);
        }
        return EJS_ERR;
    }
    if (ejs->loaderCallback) {
        (ejs->loaderCallback)(ejs, EJS_SECT_END, modules, 0);
    }

    return 0;
}


static int fixupTypes(Ejs *ejs)
{
    MprList         *list;
    EjsTypeFixup    *fixup;
    EjsModule       *mp;
    EjsType         *type, *targetType;
    EjsBlock        *instanceBlock;
    EjsTrait        *trait;
    EjsFunction     *targetFunction;
    EjsEx           *targetException;
    int             next;

    list = ejs->typeFixups;

    /*
        TODO - this could be optimized. We often are looking up the same property.
        When creating fixups, if the type is the same as the previous fixup, we could point to the previous entry.
     */
    for (next = 0; (fixup = (EjsTypeFixup*) mprGetNextItem(list, &next)) != 0; ) {

        mp = 0;
        type = 0;

        if (fixup->typeSlotNum >= 0) {
            type = (EjsType*) ejsGetProperty(ejs, ejs->global, fixup->typeSlotNum);

        } else if (fixup->typeName.name) {
            mprAssert(fixup->typeSlotNum < 0);
            type = (EjsType*) ejsGetPropertyByName(ejs, ejs->global, &fixup->typeName);
            
        } else {
            continue;
        }
        if (type == 0) {
            ejsThrowReferenceError(ejs, "Can't fixup forward type reference for %s. Fixup kind %d", 
                fixup->typeName.name, fixup->kind);
            mprError(ejs, "Can't fixup forward type reference for %s. Fixup kind %d", fixup->typeName.name, fixup->kind);
            return EJS_ERR;
        }

        switch (fixup->kind) {
        case EJS_FIXUP_BASE_TYPE:
            mprAssert(fixup->target);
            targetType = (EjsType*) fixup->target;
            targetType->needFixup = 1;
            ejsFixupClass(ejs, targetType, type, targetType->implements, 0);
            instanceBlock = targetType->instanceBlock;
            if (instanceBlock && type) {
                ejsFixupBlock(ejs, instanceBlock, type->instanceBlock, targetType->implements, 0);
            }
            if (targetType->block.namespaces.length == 0 && type->block.hasScriptFunctions) {
                ejsDefineTypeNamespaces(ejs, targetType);
            }
            break;

        case EJS_FIXUP_INTERFACE_TYPE:
            targetType = (EjsType*) fixup->target;
            mprAddItem(targetType->implements, type);
            break;

        case EJS_FIXUP_RETURN_TYPE:
            mprAssert(fixup->target);
            targetFunction = (EjsFunction*) fixup->target;
            targetFunction->resultType = type;
            break;

        case EJS_FIXUP_TYPE_PROPERTY:
            mprAssert(fixup->target);
            trait = ejsGetPropertyTrait(ejs, fixup->target, fixup->slotNum);
            mprAssert(trait);
            if (trait) {
                trait->type = type;
            }
            break;

        case EJS_FIXUP_INSTANCE_PROPERTY:
            mprAssert(fixup->target);
            mprAssert(ejsIsBlock(fixup->target));
            mprAssert(fixup->target->isInstanceBlock);
            trait = ejsGetPropertyTrait(ejs, fixup->target, fixup->slotNum);
            mprAssert(trait);
            if (trait) {
                trait->type = type;
            }
            break;

        case EJS_FIXUP_LOCAL:
            mprAssert(fixup->target);
            trait = ejsGetPropertyTrait(ejs, fixup->target, fixup->slotNum);
            mprAssert(trait);
            if (trait) {
                trait->type = type;
            }
            break;

        case EJS_FIXUP_EXCEPTION:
            mprAssert(fixup->target);
            targetException = (EjsEx*) fixup->target;
            targetException->catchType = type;
            break;

        default:
            mprAssert(0);
        }
    }
    return 0;
}


/*
 *  Search for a file. If found, Return the path where the file was located. Otherwise return null.
 */
static char *probe(Ejs *ejs, cchar *path)
{
    mprAssert(ejs);
    mprAssert(path);

    mprLog(ejs, 9, "Probe for file %s", path);
    if (mprAccess(ejs, path, R_OK)) {
        return mprStrdup(ejs, path);
    }
    return 0;
}


/*
 *  Search for a file. Return the full path name of the file.
 *  The search strategy is: Given a name "a.b.c", scan for:
 *
 *      1. File named a.b.c
 *      2. File named a/b/c
 *      3. File named a.b.c in EJSPATH
 *      4. File named a/b/c in EJSPATH
 *      5. File named c in EJSPATH
 */
int ejsSearch(Ejs *ejs, char **path, cchar *name)
{
    cchar   *baseName, *ejsPath;
    char    fileName[MPR_MAX_FNAME];
    char    *searchPath, *dir, *tok, *cp, *slashName;

    slashName = 0;
    ejsPath = ejs->service->ejsPath;

    mprLog(ejs, 7, "Search for module \"%s\" in ejspath %s\n", name, ejsPath);

    /*
     *  1. Search for path directly
     */
    if ((*path = probe(ejs, name)) != 0) {
        mprLog(ejs, 7, "Found %s at %s", name, *path);
        return 0;
    }

    /*
     *  2. Search for "a/b/c"
     */
    slashName = mprStrdup(ejs, name);
    for (cp = slashName; *cp; cp++) {
        if (*cp == '.') {
            *cp = '/';
        }
    }
    if ((*path = probe(ejs, slashName)) != 0) {
        mprLog(ejs, 7, "Found %s at %s", name, *path);
        return 0;
    }

    /*
     *  3. Search for "a.b.c" in EJSPATH
     */
    searchPath = mprStrdup(ejs, ejsPath);
    dir = mprStrTok(searchPath, MPR_SEARCH_DELIM, &tok);
    while (dir && *dir) {
        mprSprintf(fileName, sizeof(fileName), "%s/%s", dir, name);
        if ((*path = probe(ejs, fileName)) != 0) {
            mprLog(ejs, 7, "Found %s at %s", name, *path);
            return 0;
        }
        dir = mprStrTok(0, MPR_SEARCH_DELIM, &tok);
    }
    mprFree(searchPath);

    /*
     *  4. Search for "a/b/c" in EJSPATH
     */
    searchPath = mprStrdup(ejs, ejsPath);
    dir = mprStrTok(searchPath, MPR_SEARCH_DELIM, &tok);
    while (dir && *dir) {
        mprSprintf(fileName, sizeof(fileName), "%s/%s", dir, slashName);
        if ((*path = probe(ejs, fileName)) != 0) {
            mprLog(ejs, 7, "Found %s at %s", name, *path);
            return 0;
        }
        dir = mprStrTok(0, MPR_SEARCH_DELIM, &tok);
    }
    mprFree(searchPath);

    /*
     *  5. Search for "c" in EJSPATH
     */
    baseName = mprGetBaseName(slashName);
    searchPath = mprStrdup(ejs, ejsPath);
    dir = mprStrTok(searchPath, MPR_SEARCH_DELIM, &tok);
    while (dir && *dir) {
        mprSprintf(fileName, sizeof(fileName), "%s/%s", dir, baseName);
        if ((*path = probe(ejs, fileName)) != 0) {
            mprLog(ejs, 7, "Found %s at %s", name, *path);
            return 0;
        }
        dir = mprStrTok(0, MPR_SEARCH_DELIM, &tok);
    }

    mprFree(searchPath);
    mprFree(slashName);

    return MPR_ERR_NOT_FOUND;
}


/*
 *  Read a string constant. String constants are stored as token offsets into
 *  the constant pool. The pool contains null terminated UTF-8 strings.
 *  TODO - rename ejsReadModuleString
 */
char *ejsModuleReadString(Ejs *ejs, EjsModule *mp)
{
    int     t;

    mprAssert(mp);

    if (ejsModuleReadNumber(ejs, mp, &t) < 0) {
        return 0;
    }
    return tokenToString(mp, t);
}


/*
 *  Read a type reference. Types are stored as either global property slot numbers or as strings (token offsets into the 
 *  constant pool). The lowest bit is set if the reference is a string. The type and name arguments are optional and may 
 *  be set to null. Return EJS_ERR for errors, otherwise 0. Return the 0 if successful, otherwise return EJS_ERR. If the 
 *  type could not be resolved, allocate a fixup record and return in *fixup. The caller should then call addFixup.
 */
//  TODO - must support reading an empty type to mean untyped.
//  TODO - this routine need much more error checking.

int ejsModuleReadType(Ejs *ejs, EjsModule *mp, EjsType **typeRef, EjsTypeFixup **fixup, EjsName *typeName, int *slotNum)
{
    EjsType         *type;
    EjsName         qname;
    int             t, slot;

    mprAssert(mp);
    mprAssert(typeRef);
    mprAssert(fixup);

    *typeRef = 0;
    *fixup = 0;

    if (typeName) {
        typeName->name = 0;
        typeName->space = 0;
    }

    if (ejsModuleReadNumber(ejs, mp, &t) < 0) {
        mprAssert(0);
        return EJS_ERR;
    }

    slot = -1;
    qname.name = 0;
    qname.space = 0;
    type = 0;

    switch (t & EJS_ENCODE_GLOBAL_MASK) {
    default:
        mp->hasError = 1;
        mprAssert(0);
        return EJS_ERR;

    case EJS_ENCODE_GLOBAL_NOREF:
        return 0;

    case EJS_ENCODE_GLOBAL_SLOT:
        /*
         *  Type is a builtin primitive type or we are binding globals.
         */
        slot = t >> 2;
        if (0 <= slot && slot < ejsGetPropertyCount(ejs, ejs->global)) {
            type = (EjsType*) ejsGetProperty(ejs, ejs->global, slot);
            if (type) {
                qname = type->qname;
            }
        }
        break;

    case EJS_ENCODE_GLOBAL_NAME:
        /*
         *  Type was unbound at compile time
         */
        qname.name = tokenToString(mp, t >> 2);
        if (qname.name == 0) {
            mp->hasError = 1;
            mprAssert(0);
            //  TODO - DIAG
            return EJS_ERR;
        }
        if ((qname.space = ejsModuleReadString(ejs, mp)) == 0) {
            mp->hasError = 1;
            mprAssert(0);
            return EJS_ERR;
        }
        if (qname.name) {
            slot = ejsLookupProperty(ejs, ejs->global, &qname);
            if (slot >= 0) {
                type = (EjsType*) ejsGetProperty(ejs, ejs->global, slot);
            }
        }
        break;
    }

    if (type) {
        if (!ejsIsType(type)) {
            mp->hasError = 1;
            mprAssert(0);
            return EJS_ERR;
        }
        *typeRef = type;

    } else if (type == 0 && fixup) {
        *fixup = createFixup(ejs, &qname, slot);
    }

    if (typeName) {
        *typeName = qname;
    }
    if (slotNum) {
        *slotNum = slot;
    }

    return 0;
}


static EjsTypeFixup *createFixup(Ejs *ejs, EjsName *qname, int slotNum)
{
    EjsTypeFixup    *fixup;

    fixup = mprAllocZeroed(ejs->typeFixups, sizeof(EjsTypeFixup));
    if (fixup == 0) {
        return 0;
    }
    fixup->typeName = *qname;
    fixup->typeSlotNum = slotNum;
    return fixup;
}


/*
 *  Convert a token index into a string.
 */
static char *tokenToString(EjsModule *mp, int token)
{
    if (token < 0 || token >= mp->constants->len) {
        mprAssert(0);
        return 0;
    }

    mprAssert(mp->constants);
    if (mp->constants == 0) {
        mprAssert(0);
        return 0;
    }

    return &mp->constants->pool[token];
}


/*
 *  Read an encoded number. Numbers are little-endian encoded in 7 bits with
 *  the 0x80 bit of each byte being a continuation bit.
 */
static int readNumber(Ejs *ejs, MprFile *file, int *number)
{
    int         c, t;

    mprAssert(file);
    mprAssert(number);

    if ((c = mprGetc(file)) < 0) {
        return MPR_ERR_CANT_READ;
    }

    t = c & 0x7f;

    if (c & 0x80) {
        if ((c = mprGetc(file)) < 0) {
            return MPR_ERR_CANT_READ;
        }
        t |= ((c &0x7f) << 7);

        if (c & 0x80) {
            if ((c = mprGetc(file)) < 0) {
                return MPR_ERR_CANT_READ;
            }
            t |= ((c &0x7f) << 14);

            if (c & 0x80) {
                if ((c = mprGetc(file)) < 0) {
                    return MPR_ERR_CANT_READ;
                }
                t |= ((c &0x7f) << 21);

                if (c & 0x80) {
                    if ((c = mprGetc(file)) < 0) {
                        return MPR_ERR_CANT_READ;
                    }
                    t |= ((c &0x7f) << 28);
                }
            }
        }
    }

    *number = t;
    return 0;
}


/*
 *  Read an encoded number. Numbers are little-endian encoded in 7 bits with
 *  the 0x80 bit of each byte being a continuation bit.
 */
int ejsModuleReadNumber(Ejs *ejs, EjsModule *mp, int *number)
{
    mprAssert(ejs);
    mprAssert(mp);
    mprAssert(number);

    if (readNumber(ejs, mp->file, number) < 0) {
        mp->hasError = 1;
        return -1;
    }
    return 0;
}


int ejsModuleReadByte(Ejs *ejs, EjsModule *mp, int *number)
{
    int     c;

    mprAssert(mp);
    mprAssert(number);

    // return mprGetc(mp->file);
    if ((c = mprGetc(mp->file)) < 0) {
        mp->hasError = 1;
        return MPR_ERR_CANT_READ;
    }
    *number = c;

    return 0;
}


static int addFixup(Ejs *ejs, int kind, EjsVar *target, int slotNum, EjsTypeFixup *fixup)
{
    int     index;

    mprAssert(ejs);
    mprAssert(fixup);

    fixup->kind = kind;
    fixup->target = target;
    fixup->slotNum = slotNum;

    index = mprAddItem(ejs->typeFixups, fixup);
    if (index < 0) {
        mprAssert(0);
        return EJS_ERR;
    }
    return 0;
}


#if BLD_FEATURE_EJS_DOC
static void setDoc(Ejs *ejs, EjsModule *mp, EjsVar *block, int slotNum)
{
    if (mp->doc && ejsIsBlock(block)) {
        ejsCreateDoc(ejs, (EjsBlock*) block, slotNum, mp->doc);
        mp->doc = 0;
    }
}


EjsDoc *ejsCreateDoc(Ejs *ejs, EjsBlock *block, int slotNum, cchar *docString)
{
    EjsDoc      *doc;
    char        key[32];

    //  TODO OPT - don't zero
    doc = mprAllocZeroed(ejs, sizeof(EjsDoc));
    if (doc == 0) {
        return 0;
    }

    doc->docString = mprStrdup(doc, docString);

    if (ejs->doc == 0) {
        ejs->doc = mprCreateHash(ejs, EJS_DOC_HASH_SIZE);
    }

    /*
     *  This is slow, but not critical path
     */
    mprSprintf(key, sizeof(key), "%Lx %d", PTOL(block), slotNum);
    mprAddHash(ejs->doc, key, doc);

    return doc;
}
#endif


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
