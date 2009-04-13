/**
 *  ejsGarbage.c - EJS Garbage collector.
 *
 *  This implements a non-compacting, generational mark and sweep collection algorithm.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejs.h"

/****************************** Forward Declarations **************************/

static void addRoot(Ejs *ejs, int generation, EjsVar *obj);
static inline void addVar(struct Ejs *ejs, struct EjsVar *vp, int generation);
static inline void linkVar(EjsGen *gen, EjsVar *vp);
static void mark(Ejs *ejs, int generation);
static void markFrame(Ejs *ejs, EjsFrame *frame);
static void markGlobal(Ejs *ejs);
static inline bool memoryUsageOk(Ejs *ejs);
static inline void pruneTypePools(Ejs *ejs);
static inline void moveGen(Ejs *ejs, EjsVar *vp, EjsVar *prev, int oldGen, int newGen);
static void resetRoots(Ejs *ejs, int generation);
static int sweep(Ejs *ejs, int generation);
static inline void unlinkVar(EjsGen *gen, EjsVar *prev, EjsVar *vp);

#if BLD_DEBUG
static void checkMarks(Ejs *ejs);
#endif

#if BLD_DEBUG || 1
static void *ejsBreakAddr = (void*) 0x555dd8;
static void checkAddr(EjsVar *addr) {
    if ((void*) addr == ejsBreakAddr) { 
        addr = ejsBreakAddr;
    }
}
#else
#define checkAddr(addr)
#endif

/************************************* Code ***********************************/
/*
 *  Create the GC service
 */
int ejsCreateGCService(Ejs *ejs)
{
    EjsGC       *gc;
    EjsGen      *gen;
    int         i;

    mprAssert(ejs);

    gc = &ejs->gc;
    gc->enabled = !(ejs->flags & EJS_FLAG_COMPILER);
    gc->enableIdleCollect = 1;
    gc->enableDemandCollect = 1;
    gc->workQuota = EJS_GC_WORK_QUOTA;
    gc->firstGlobal = ES_global_NUM_CLASS_PROP;

    /*
     *  Start in the eternal generation for all builtin types. ejsSetGeneration will be called later to set to the NEW gen.
     */
    gc->allocGeneration = EJS_GEN_ETERNAL;

    gc->pools = mprAllocZeroed(ejs, sizeof(EjsPool) * EJS_MAX_TYPE);
    if (gc->pools == 0) {
        return MPR_ERR_NO_MEMORY;
    }
    gc->numPools = EJS_MAX_TYPE;

    /*
     *  Allocate space for the cross generational root links
     */
    for (i = 0; i < EJS_MAX_GEN; i++) {
        gen = &gc->generations[i];
        gen->roots = mprAllocZeroed(ejs, sizeof(EjsVar*) * EJS_NUM_CROSS_GEN);
        if (gen->roots == 0) {
            return MPR_ERR_NO_MEMORY;
        }
        gen->nextRoot = gen->roots;
        gen->peakRoot = gen->roots;
        gen->lastRoot = &gen->roots[EJS_NUM_CROSS_GEN - 1];
    }
    return 0;
}


/*
 *  Collect the garbage. This is a mark and sweep over all possible objects. If an object is not referenced, it and 
 *  all contained properties will be freed. Collection is done in generations.
 */
void ejsCollectGarbage(Ejs *ejs, int mode)
{
    EjsGC       *gc;
    int         generation, i, count, totalCreated, prevMax;
    
    gc = &ejs->gc;

    if (!gc->enabled || gc->collecting || !ejs->initialized) {
        return;
    }
    gc->collecting = 1;
    gc->totalSweeps++;

    /*
     *  Collecting a generation implicitly collects all younger generations. If collecting all, just start at old.
     *  If any cross-generational root storage has overflowed, we must do a full GC.
     */
    if (mode == EJS_GC_ALL || gc->overflow) {
        gc->overflow = 0;
        generation = EJS_GEN_OLD;
        mark(ejs, generation);
        sweep(ejs, generation);
        
    } else if (mode == EJS_GC_QUICK) {
        generation = EJS_GEN_NEW;
        mark(ejs, generation);
        sweep(ejs, generation);
        
    } else {
        
        /*
         *  Smart collection. Find the oldest generation worth examining. The sweep may promote objects to older generations,
         *  so keep collecting any worthwhile generations.
         */
        while (1) {
            /*
             */
            generation = 0;
            prevMax = 0;
            totalCreated = 0;
            for (i = EJS_GEN_NEW; i < EJS_GEN_ETERNAL; i++) {
                count = gc->generations[i].newlyCreated;
                if (count > prevMax || count > EJS_GC_WORK_QUOTA) {
                    prevMax = count;
                    generation = i;
                }
                totalCreated += count;
            }

            /*
             *  Collect from this generation and all younger generations.
             */
            if (totalCreated < EJS_GC_WORK_QUOTA) {
                break;
            }
            mark(ejs, generation);
            sweep(ejs, generation);
        }
    }

    if (!memoryUsageOk(ejs)) {
        pruneTypePools(ejs);
    }

    gc->workDone = 0;
    gc->collecting = 0;
    ejs->gc.required = 0;
}


/*
 *  Mark phase. Mark objects that are still in use and should not be collected.
 */
static void mark(Ejs *ejs, int generation)
{
    EjsVar      *vp, **sp, **lim, **src;
    EjsFrame    *frame;
    EjsGC       *gc;
    EjsModule   *mp;
    EjsGen      *gen;
    int         next, i;

    gc = &ejs->gc;
    gc->collectGeneration = generation;

#if BLD_DEBUG
    mprLog(ejs, 6, "\nGC: Marked Blocks: generation %d", generation);
#endif

    markGlobal(ejs);

    if (ejs->result) {
        ejsMarkVar(ejs, NULL, ejs->result);
    }
    if (ejs->exception) {
        ejsMarkVar(ejs, NULL, ejs->exception);
    }

    /*
     *  Mark initializers
     */
    for (next = 0; (mp = (EjsModule*) mprGetNextItem(ejs->modules, &next)) != 0;) {
        if (mp->initializer /* TODO MOB TEMP && !mp->initialized */) {
            ejsMarkVar(ejs, NULL, (EjsVar*) mp->initializer);
        }
    }

    /*
     *  Mark each frame. This includes all stack slots, ie. (local vars, arguments and expression temporaries)
     */
    lim = ejs->stack.top + 1;
    for (frame = ejs->frame; frame; frame = frame->prev) {
        markFrame(ejs, frame);
        for (sp = frame->stackBase; sp < lim; sp++) {
            vp = *sp;
            if (vp) {
                ejsMarkVar(ejs, NULL, vp);
            }
        }
        lim = frame->prevStackTop + 1;
    }

    /*
     *  Mark the cross-generational roots. Compact and remove old roots in the process. Must now traverse all objects
     *  that are referenced from these roots - so set the collectGeneration to eternal.
     */
    gc->collectGeneration = EJS_GEN_ETERNAL;
    for (i = 0; i <= generation; i++) {
        gen = &gc->generations[i];
        for (src = gen->roots; src < gen->nextRoot; src++) {
            ejsMarkVar(ejs, NULL, *src);
        }
    }
}


/*
 *  Sweep up the garbage for a given generation
 */
static int sweep(Ejs *ejs, int maxGeneration)
{
    EjsVar      *vp, *next, *prev;
    EjsGC       *gc;
    EjsGen      *gen;
    int         total, count, aliveCount, generation, i;

    gc = &ejs->gc;
    
    total = 0;
    count = 0;
    aliveCount = 0;

    /*
     *  Must go from oldest to youngest generation incase moving objects to elder generations and we clear the mark. Must
     *  not re-examine.
     */
    for (generation = maxGeneration; generation >= 0; generation--) {

        count = 0;
        gc->collectGeneration = generation;

        /*
         *  Traverse all objects in the required generation
         */
        gen = &gc->generations[generation];
        for (prev = 0, vp = gen->next; vp; vp = next) {

            next = vp->next;
            checkAddr(vp);
            mprAssert(vp->generation == generation);

            if (vp->marked) {
                /*
                 *  In use and surviving at least one collection cycle. Move to a new generation if it has survived 2 cycles.
                 */
                if (vp->generation < EJS_GEN_OLD) {
                    if (vp->survived) {
                        vp->survived = 0;
                        moveGen(ejs, vp, prev, vp->generation, vp->generation + 1);

                    } else {
                        vp->survived = 1;
                        prev = vp;
                    }
                
                } else {
                    prev = vp;
                }
                vp->marked = 0;
                aliveCount++;
                continue;

            } else if (vp->permanent) {
                prev = vp;
                aliveCount++;
                continue;
            }

            unlinkVar(gen, prev, vp);
            if (vp->type->hasFinalizer) {
                ejsFinalizeVar(ejs, vp);
            }
            ejsDestroyVar(ejs, vp);
            count++;
        }

        gc->allocatedObjects -= count;
        gc->totalReclaimed += count;
        gen->totalReclaimed += count;
        gen->totalSweeps++;
        gen->newlyCreated = 0;

        total += count;

    }

    for (i = maxGeneration; i < EJS_MAX_GEN; i++) {
        gen = &gc->generations[i];
        for (vp = gen->next; vp; vp = vp->next) {
            if (vp->marked) {
                vp->marked = 0;
            }
        }
    }
    
    resetRoots(ejs, maxGeneration);

#if BLD_DEBUG
    mprLog(ejs, 6, "GC: Sweep freed %d objects, alive %d", total, aliveCount);
    checkMarks(ejs);
#endif
    return total;
}


/*
 *  Compact the cross generation root objects.
 */
static void resetRoots(Ejs *ejs, int generation)
{
    EjsFrame    *frame;
    EjsVar      *vp, **src, **dest;
    EjsGen      *gen;
    int         i, j;

    for (frame = ejs->frame; frame; frame = frame->prev) {
        frame->function.block.obj.var.marked = 0;
    }

    ejs->gc.collectGeneration = EJS_GEN_ETERNAL;
    for (i = 0; i <= generation; i++) {
        gen = &ejs->gc.generations[i];
        for (src = dest = gen->roots; src < gen->nextRoot; ) {
            vp = *src;
            checkAddr(vp);

            if (vp->rootLinks & (1 << i)) {
                *dest++ = *src++;

            } else {
                vp->rootLinks &= ~(1 << i);
                vp->refLinks &= ~(1 << i);
                if (vp->refLinks) {
                    for (j = i + 1; j <= generation; j++) {
                        if (vp->refLinks & (1 << j)) {
                            addRoot(ejs, j, *src);
                            break;
                        }
                    }
                }
                src++;
            }
        }
        gen->nextRoot = dest;
        gen->rootCount = gen->nextRoot - gen->roots;
        *gen->nextRoot = 0;
    }
}


#if BLD_DEBUG
static void checkMarks(Ejs *ejs)
{
    EjsVar      *vp;
    EjsGen      *gen;
    int         i;

    ejs->gc.collectGeneration = EJS_GEN_ETERNAL;
    for (i = 0; i < EJS_MAX_GEN; i++) {
        gen = &ejs->gc.generations[i];
        for (vp = gen->next; vp; vp = vp->next) {
            mprAssert(!vp->marked);
            mprAssert(vp->magic != 0xf1f1f1f1);
        }
    }
}
#endif


static void markFrame(Ejs *ejs, EjsFrame *frame)
{
    EjsBlock    *block;
    int         next;

    if (frame->returnValue) {
        ejsMarkVar(ejs, NULL, frame->returnValue);
    }
    if (frame->saveException) {
        ejsMarkVar(ejs, NULL, frame->saveException);
    }
    if (frame->saveFrame) {
        markFrame(ejs, frame->saveFrame);
    }    
    if (frame->thisObj) {
        ejsMarkVar(ejs, NULL, frame->thisObj);
    } 

    if (frame->needClosure.length > 0) {
        next = 0;
        while ((block = ejsGetNextItem(&frame->needClosure, &next)) != 0) {
            ejsMarkVar(ejs, NULL, (EjsVar*) block);
        }
    }

    ejsMarkVar(ejs, NULL, (EjsVar*) &frame->function);
}


static void markGlobal(Ejs *ejs)
{
    EjsGC       *gc;
    EjsVar      *vp;
    EjsObject   *obj;
    int         i, eternalSoFar;

    gc = &ejs->gc;

    obj = (EjsObject*) ejs->global;
    obj->var.marked = 1;
    eternalSoFar = 0;

    for (i = gc->firstGlobal; i < obj->numProp; i++) {
        vp = obj->slots[i];
        if (vp->generation == EJS_GEN_ETERNAL) {
            if (eternalSoFar) {
                gc->firstGlobal = i;
            }
            continue;
        }
        eternalSoFar = 0;
        if (vp == 0 || vp == ejs->nullValue) {
            continue;
        }
        ejsMarkVar(ejs, NULL, vp);
    }
}


/*
 *  Mark a variable as used. All variable marking comes through here.
 */
void ejsMarkVar(Ejs *ejs, EjsVar *container, EjsVar *vp)
{  
    if (vp->marked) {
        return;
    }
    
    /*
     *  Don't traverse generations older than the one being marked. 
     */
    if (vp->generation <= ejs->gc.collectGeneration) {
        checkAddr(vp);
        vp->marked = 1;
        if (container) {
            if (vp->generation < EJS_GEN_ETERNAL) {
                container->refLinks |= (1 << vp->generation);
            }
        }
        (vp->type->helpers->markVar)(ejs, container, vp);
    }
}


static inline bool memoryUsageOk(Ejs *ejs)
{
    MprAlloc    *alloc;
    uint        memory;

    memory = mprGetUsedMemory(ejs);
    alloc = mprGetAllocStats(ejs);
    return memory < alloc->redLine;
}


static inline void pruneTypePools(Ejs *ejs)
{
    EjsPool     *pool;
    MprAlloc    *alloc;
    EjsGC       *gc;
    EjsVar      *vp, *nextVp;
    uint        memory;
    int         i;

    gc = &ejs->gc;

    /*
     *  Still insufficient memory, must reclaim all objects from the type pools.
     */
    for (i = 0; i < gc->numPools; i++) {
        pool = &gc->pools[i];
        if (pool->count) {
            for (vp = pool->next; vp; vp = nextVp) {
                nextVp = vp->next;
                mprFree(vp);
            }
            pool->count = 0;
        }
    }
    gc->totalRedlines++;

    memory = mprGetUsedMemory(ejs);
    alloc = mprGetAllocStats(ejs);

    if (memory >= alloc->maxMemory) {
        /*
         *  Could not provide sufficient memory. Go into graceful degrade mode
         */
        ejsThrowMemoryError(ejs);
        ejsGracefulDegrade(ejs);
    }
}


void ejsMakePermanent(Ejs *ejs, EjsVar *vp)
{
    vp->permanent = 1;
}


void ejsMakeTransient(Ejs *ejs, EjsVar *vp)
{
    vp->permanent = 0;
}


/*
 *  Allocate a new variable. Size is set to the extra bytes for properties in addition to the type's instance size.
 */
EjsVar *ejsAllocVar(Ejs *ejs, EjsType *type, int extra)
{
    EjsPool     *pool;
    EjsGC       *gc;
    EjsVar      *vp;
    uint        size;
    int         generation;
#if BLD_DEBUG
    int         seqno;
#endif

    mprAssert(ejs);
    mprAssert(type);

    gc = &ejs->gc;
    generation = gc->allocGeneration;

    if (0 <= type->id && type->id < gc->numPools) {
        pool = &gc->pools[type->id];
        vp = (EjsVar*) pool->next;
        if (type == ejs->typeType) {
            generation = EJS_GEN_ETERNAL;
        }

    } else {
        vp = 0;
        pool = 0;
    }

    if (vp) {
        pool->next = vp->next;
        pool->reuse++;
        pool->count--;
        mprAssert(pool->count >= 0);

#if BLD_DEBUG
        seqno = vp->seqno;
        memset(vp, 0, type->instanceSize);
        vp->type = type;
        vp->seqno = seqno;
#else
        memset(vp, 0, type->instanceSize);
        vp->type = type;
#endif
        
    } else {
        /*
         *  TODO - remove zeroed
         */
        size = max(1, type->numAlloc) * (extra + type->instanceSize);
        vp = (EjsVar*) mprAllocZeroed(ejs, size);
        if (vp == 0) {
            ejsThrowMemoryError(ejs);
            return 0;
        }
        vp->type = type;
        vp->next = 0;

        /*
         *  TODO OPT - bitfields
         */
        vp->generation = 0;
        vp->rootLinks = 0;
        vp->refLinks = 0;
        vp->builtin = 0;
        vp->dynamic = 0;
        vp->hasGetterSetter = 0;
        vp->isFunction = 0;
        vp->isObject = 0;
        vp->isInstanceBlock = 0;
        vp->isType = 0;
        vp->isFrame = 0;
        vp->hidden = 0;
        vp->marked = 0;
        vp->native = 0;
        vp->nativeProc = 0;
        vp->permanent = 0;
        vp->survived = 0;
        vp->visited = 0;
    }

    addVar(ejs, vp, generation);

    if (pool) {
        pool->type = type;
        pool->allocated++;
        if (pool->allocated > pool->peakAllocated) {
            pool->peakAllocated = pool->allocated;
        }
    }

#if BLD_DEBUG
    mprAssert(vp->magic != EJS_GC_IN_USE);
    vp->magic = EJS_GC_IN_USE;
    vp->seqno++;
#endif

    return (EjsVar*) vp;
}


/*
 *  Free a variable. This is should only ever be called by the destroyVar helpers to free or recycle the object to a type 
 *  specific free pool. Users should let the GC discovery unused objects which will then call ejsDestroyObject when an object
 *  is no longer referenced.
 */
void ejsFreeVar(Ejs *ejs, EjsVar *vp)
{
    EjsType     *type;
    EjsGC       *gc;
    EjsPool     *pool;

    mprAssert(vp);
    mprAssert(vp->next == 0);
    
#if BLD_DEBUG
    mprAssert(vp->magic == EJS_GC_IN_USE);
    vp->magic = EJS_GC_FREE;
#endif

    type = vp->type;

#if BLD_FEATURE_MEMORY_DEBUG
    {
        int seqno = vp->seqno;
        memset(vp, 0xf4, type->instanceSize);
        vp->seqno = seqno;
    }
#else
    //  TODO - MOB - TEMP
    memset(vp, 0xf4, type->instanceSize);
#endif

    /*
     *  Return the object to the type pool
     */
    gc = &ejs->gc;
    if (type->id >= 0 && type->id < gc->numPools) {
        pool = &gc->pools[type->id];
        vp->next = pool->next;
        pool->next = (EjsVar*) vp;

        /*
         *  Update stats
         */
        pool->allocated--;
        pool->count++;
        if (pool->count > pool->peakCount) {
            pool->peakCount = pool->count;
        }

    } else {
        mprFree(vp);
    }
}


static inline void linkVar(EjsGen *gen, EjsVar *vp)
{
    mprAssert(gen);
    mprAssert(vp);
    mprAssert(vp->next == 0);

    vp->next = gen->next;
    gen->next = vp;

#if BLD_DEBUG
    gen->inUse++;
#endif
}


static inline void unlinkVar(EjsGen *gen, EjsVar *prev, EjsVar *vp)
{
    mprAssert(gen);
    mprAssert(vp);

    if (prev) {
        prev->next = vp->next;
    } else {
        gen->next = vp->next;
    }
#if BLD_DEBUG
    gen->inUse--;
    mprAssert(gen->inUse >= 0);
    vp->next = 0;
#endif
}


static inline void addVar(Ejs *ejs, EjsVar *vp, int generation)
{
    EjsGC       *gc;
    EjsGen      *gen;

    gc = &ejs->gc;

    mprAssert(gc);
    mprAssert(vp);
    mprAssert(0 <= generation && generation <= EJS_GEN_ETERNAL);

    if (vp == 0) {
        return;
    }
    checkAddr(vp);

    vp->generation = generation;
    vp->rootLinks = 0;
    vp->refLinks = 0;

    gen = &gc->generations[generation];
    linkVar(gen, vp);

    /*
     *  Update GC stats
     */
    gen->newlyCreated++;
    gc->totalAllocated++;
    gc->allocatedObjects++;
    if (gc->allocatedObjects >= gc->peakAllocatedObjects) {
        gc->peakAllocatedObjects = gc->allocatedObjects;
    }

    if (vp->type == ejs->typeType) {
        gc->allocatedTypes++;
        if (gc->allocatedTypes >= gc->peakAllocatedTypes) {
            gc->peakAllocatedTypes = gc->allocatedTypes;
        }
    }

    if (++gc->workDone >= gc->workQuota) {
        ejs->gc.required = 1;
        ejs->attention = 1;
        gc->workDone = 0;
    }
}


/*
 *  Move a var to a new, older generation
 */
static inline void moveGen(Ejs *ejs, EjsVar *vp, EjsVar *prev, int oldGen, int newGen)
{
    EjsGC       *gc;
    EjsGen      *nextGen;
    
    mprAssert(vp);
    mprAssert(oldGen < newGen);
    
    gc = &ejs->gc;
    nextGen = &gc->generations[newGen];

    unlinkVar(&gc->generations[oldGen], prev, vp);
    linkVar(nextGen, vp);
    nextGen->newlyCreated++;    

    addRoot(ejs, oldGen, vp);
    vp->generation = newGen;
}


/*
 *  Return true if there is time to do a garbage collection and if we will benefit from it.
 *  TODO - this is currently not called.
 */
int ejsIsTimeForGC(Ejs *ejs, int timeTillNextEvent)
{
    EjsGC       *gc;

    if (timeTillNextEvent < EJS_MIN_TIME_FOR_GC) {
        /*
         *  This is a heuristic where we want a good amount of idle time so that a proactive garbage collection won't 
         *  delay any I/O events.
         */
        return 0;
    }

    /*
     *  Return if we haven't done enough work to warrant a collection Trigger a little short of the work quota to try to run 
     *  GC before a demand allocation requires it.
     */
    gc = &ejs->gc;
    if (!gc->enabled || !gc->enableIdleCollect || gc->workDone < (gc->workQuota - EJS_GC_MIN_WORK_QUOTA)) {
        return 0;
    }

    mprLog(ejs, 6, "Time for GC. Work done %d, time till next event %d", gc->workDone, timeTillNextEvent);

    return 1;
}


void ejsEnableGC(Ejs *ejs, bool on)
{
    ejs->gc.enabled = on;
}


/*
 *  On a memory allocation failure, go into graceful degrade mode. Set all slab allocation chunk increments to 1 
 *  so we can create an exception block to throw.
 */
void ejsGracefulDegrade(Ejs *ejs)
{
    mprLog(ejs, 1, "WARNING: Memory almost depleted. In graceful degrade mode");

    //  TODO -- need to notify MPR slabs to allocate in chunks of 1
    ejs->gc.degraded = 1;
    mprSignalExit(ejs);
}


/*
 *  Set a GC reference from object to value, in response to: "obj->field = value"
 */
void ejsSetReference(Ejs *ejs, EjsVar *obj, EjsVar *value)
{
    if (value && value->generation < obj->generation) {

        mprAssert(value->generation < EJS_GEN_ETERNAL);

        //  TODO - this won't work with master interpreters.
        //  mprAssert(!obj->master);
        
        if ((obj->rootLinks & (1 << value->generation)) == 0) {
            addRoot(ejs, value->generation, obj);
        }
    }
}


static void addRoot(Ejs *ejs, int generation, EjsVar *obj)
{
    EjsGen  *gen;

    mprAssert(obj);
    mprAssert(obj->type);
    mprAssert(0 <= generation && generation <= EJS_GEN_ETERNAL);
    
    if (obj->isFrame) {
        return;
    }
    obj->rootLinks |= (1 << generation);
    obj->refLinks |= (1 << generation);

    gen = &ejs->gc.generations[generation];
    if (gen->nextRoot < gen->lastRoot) {
        *gen->nextRoot++ = obj;
        *gen->nextRoot = 0;
        if (gen->nextRoot > gen->peakRoot) {
            gen->peakRoot = gen->nextRoot;
        }

    } else {
        ejs->gc.overflow = 1;
        ejs->gc.totalOverflows++;
    }
}


int ejsSetGeneration(Ejs *ejs, int generation)
{
    int     old;
    
    old = ejs->gc.allocGeneration;
    ejs->gc.allocGeneration = generation;
    return old;
}


void ejsPrintAllocReport(Ejs *ejs)
{
    EjsType         *type;
    EjsGC           *gc;
    EjsGen          *gen;
    EjsPool         *pool;
    MprAlloc        *ap;
    int             i, maxSlot, typeMemory;

    gc = &ejs->gc;
    ap = mprGetAllocStats(ejs);
    
    /*
     *  EJS stats
     */
    mprLog(ejs, 0, "\n\nEJS Memory Statistics");
    mprLog(ejs, 0, "  Types allocated        %,14d", gc->allocatedTypes / 2);
    mprLog(ejs, 0, "  Objects allocated      %,14d", gc->allocatedObjects);
    mprLog(ejs, 0, "  Peak objects allocated %,14d", gc->peakAllocatedObjects);

    /*
     *  Per type
     */
    mprLog(ejs, 0, "\nObject Cache Statistics");
    mprLog(ejs, 0, "------------------------");
    mprLog(ejs, 0, "Name                TypeSize  ObjectSize  ObjectCount  PeakCount  FreeList  PeakFreeList   ReuseCount");
    
    maxSlot = ejsGetPropertyCount(ejs, ejs->global);
    typeMemory = 0;

    for (i = 0; i < gc->numPools; i++) {
        pool = &ejs->gc.pools[i];
        type = pool->type;
        if (type == 0 || !ejsIsType(type)) {
            continue;
        }

        if (type->id < 0 || type->id >= gc->numPools) {
            continue;
        }

        pool = &ejs->gc.pools[type->id];

        mprLog(ejs, 0, "%-22s %,5d %,8d %,10d  %,10d, %,9d, %,10d, %,14d", type->qname.name, ejsGetTypeSize(ejs, type), 
            type->instanceSize, pool->allocated, pool->peakAllocated, pool->count, pool->peakCount, pool->reuse);

#if FUTURE
        mprLog(ejs, 0, "  Current object memory  %,14d K", / 1024);
        mprLog(ejs, 0, "  Peak object memory     %,14d K", / 1024);
#endif
        typeMemory += ejsGetTypeSize(ejs, type);
    }
    mprLog(ejs, 0, "\nTotal type memory        %,14d K", typeMemory / 1024);


    mprLog(ejs, 0, "\nEJS Garbage Collector Statistics");
    mprLog(ejs, 0, "  Total allocations      %,14d", gc->totalAllocated);
    mprLog(ejs, 0, "  Total reclaimations    %,14d", gc->totalReclaimed);
    mprLog(ejs, 0, "  Total sweeps           %,14d", gc->totalSweeps);
    mprLog(ejs, 0, "  Total redlines         %,14d", gc->totalRedlines);
    mprLog(ejs, 0, "  Total overflows        %,14d", gc->totalOverflows);

    mprLog(ejs, 0, "\nGC Generation Statistics");
    for (i = 0; i < EJS_MAX_GEN; i++) {
        gen = &gc->generations[i];
        mprLog(ejs, 0, "  Generation %d", i);
        mprLog(ejs, 0, "    Newly created        %,14d", gen->newlyCreated);
        mprLog(ejs, 0, "    Objects in-use       %,14d", gen->inUse);
        mprLog(ejs, 0, "    Total reclaimations  %,14d", gen->totalReclaimed);
        mprLog(ejs, 0, "    Total sweeps         %,14d", gen->totalSweeps);
        mprLog(ejs, 0, "    Peak root usage      %,14d", gen->peakRoot - gen->roots);
    }

    mprLog(ejs, 0, "  Object GC work quota   %,14d", gc->workQuota);
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
