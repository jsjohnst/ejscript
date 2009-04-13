#include "mpr.h"

/******************************************************************************/
/* 
 *  This file is an amalgamation of all the individual source code files for
 *  Michaels Portable Runtime 2.1.0.
 *
 *  Catenating all the source into a single file makes embedding simpler and
 *  the resulting application faster, as many compilers can do whole file
 *  optimization.
 *
 *  If you want to modify mpr, you can still get the whole source
 *  as individual files if you need.
 */


/************************************************************************/
/*
 *  Start of file "../mpr.c"
 */
/************************************************************************/

/**
 *  mpr.c - Michael's Portable Runtime (MPR). Initialization, start/stop and control of the MPR.
 *
 *  Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */




static void memoryFailure(MprCtx ctx, uint size, uint total, bool granted);
static int  mprDestructor(Mpr *mpr);

#if BLD_FEATURE_MULTITHREAD
static void serviceEvents(void *data, MprThread *tp);
#endif

/*
 *  Create the MPR service. This routine is the first call an MPR application must do. It creates the top 
 *  level memory context.
 */

Mpr *mprCreate(int argc, char **argv, MprAllocNotifier cback)
{
    return mprCreateEx(argc, argv, cback, NULL);
}


/*
 *  Add a shell parameter then do the regular init
 */
Mpr *mprCreateEx(int argc, char **argv, MprAllocNotifier cback, void *shell)
{
    Mpr     *mpr;

    if (cback == 0) {
        cback = memoryFailure;
    }
    mpr = (Mpr*) mprCreateAllocService(cback, (MprDestructor) mprDestructor);

    if (mpr == 0) {
        mprAssert(mpr);
        return 0;
    }
    mpr->argc = argc;
    mpr->argv = argv;
#if DEBUG_IDE
    mpr->debugMode = 1;
#endif

    if (mprCreateTimeService(mpr) < 0) {
        goto error;
    }
    if ((mpr->osService = mprCreateOsService(mpr)) < 0) {
        goto error;
    }

    mpr->name = mprStrdup(mpr, BLD_PRODUCT);
    mpr->title = mprStrdup(mpr, BLD_NAME);
    mpr->version = mprStrdup(mpr, BLD_VERSION);
    mpr->table = mprCreateHash(mpr, 0);

    /*
     *  See if any of the preceeding allocations failed and mark all blocks allocated so far as required.
     *  They will then be omitted from leak reports.
     */
    if (mprHasAllocError(mpr)) {
        goto error;
    }

    mprSetShell(mpr, shell);

#if BLD_FEATURE_MULTITHREAD
    if ((mpr->threadService = mprCreateThreadService(mpr)) == 0) {
        goto error;
    }
    mpr->mutex = mprCreateLock(mpr);
    mpr->spin = mprCreateSpinLock(mpr);
#endif

    if ((mpr->fileService = mprCreateFileService(mpr)) == 0) {
        goto error;
    }

    if ((mpr->moduleService = mprCreateModuleService(mpr)) == 0) {
        goto error;
    }

    if ((mpr->eventService = mprCreateEventService(mpr)) == 0) {
        goto error;
    }
#if BLD_FEATURE_MULTITHREAD
    if ((mpr->poolService = mprCreatePoolService(mpr)) == 0) {
        goto error;
    }
#endif
    if ((mpr->waitService = mprCreateWaitService(mpr)) == 0) {
        goto error;
    }
    if ((mpr->socketService = mprCreateSocketService(mpr)) == 0) {
        goto error;
    }
#if BLD_FEATURE_HTTP
    if ((mpr->httpService = mprCreateHttpService(mpr)) == 0) {
        goto error;
    }
#endif

    /*
     *  Now catch all memory allocation errors up to this point. Should be none.
     */
    if (mprHasAllocError(mpr)) {
        goto error;
    }

    return mpr;

/*
 *  Error return
 */
error:
    mprFree(mpr);
    return 0;
}


static int mprDestructor(Mpr *mpr)
{
    if ((mpr->flags & MPR_STARTED) && !(mpr->flags & MPR_STOPPED)) {
        mprStop(mpr);
    }
    return 0;

}


/*
 *  Start the Mpr and all services
 */
int mprStart(Mpr *mpr, int startFlags)
{
    int     rc;

    rc = 0;

    rc += mprStartOsService(mpr->osService);

#if BLD_FEATURE_MULTITHREAD
    rc += mprStartThreadService(mpr->threadService);
#endif
    rc += mprStartModuleService(mpr->moduleService);

    rc += mprStartEventService(mpr->eventService);
#if BLD_FEATURE_MULTITHREAD
    rc += mprStartPoolService(mpr->poolService);
#endif
    rc += mprStartWaitService(mpr->waitService);
    rc += mprStartSocketService(mpr->socketService);
#if BLD_FEATURE_HTTP
    rc += mprStartHttpService(mpr->httpService);
#endif

    if (rc != 0) {
        mprUserError(mpr, "Can't start MPR services");
        return MPR_ERR_CANT_INITIALIZE;
    }

#if BLD_FEATURE_MULTITHREAD
    if ((startFlags & MPR_SERVICE_THREAD)) {
        mprStartEventsThread(mpr);
    }
#endif

    mpr->flags |= MPR_STARTED | (startFlags & MPR_USER_START_FLAGS);

    mprLog(mpr, MPR_INFO, "MPR services are ready");
    return 0;
}


void mprStop(Mpr *mpr)
{
    mprLock(mpr->mutex);
    if (! (mpr->flags & MPR_STARTED) || (mpr->flags & MPR_STOPPED)) {
		mprUnlock(mpr->mutex);
        return;
    }
    mpr->flags |= MPR_STOPPED;

    /*
     *  Trigger graceful termination. This will prevent further tasks and events being created.
     */
    mprTerminate(mpr, 1);

#if BLD_FEATURE_HTTP
    mprStopHttpService(mpr->httpService);
#endif
    mprStopSocketService(mpr->socketService);
    mprStopWaitService(mpr->waitService);
#if BLD_FEATURE_MULTITHREAD
    mprStopPoolService(mpr->poolService, MPR_TIMEOUT_STOP_TASK);
#endif
    mprStopEventService(mpr->eventService);
    mprStopModuleService(mpr->moduleService);
#if BLD_FEATURE_MULTITHREAD
    mprStopThreadService(mpr->threadService, MPR_TIMEOUT_STOP_THREAD);
#endif
    mprStopOsService(mpr->osService);
}


#if BLD_FEATURE_MULTITHREAD
/*
 *  Thread to service the event queue. Used if the user does not have their own main event loop.
 */
int mprStartEventsThread(Mpr *mpr)
{
    MprThread   *tp;

    mprLog(mpr, MPR_CONFIG, "Starting service thread");

    mpr->hasEventsThread = 1;

    tp = mprCreateThread(mpr, "events", serviceEvents, 0, MPR_NORMAL_PRIORITY, 0);
    if (tp == 0) {
        return MPR_ERR_CANT_CREATE;
    }
    mprStartThread(tp);
    return 0;
}


/*
 *  Thread main for serviceEvents
 */
static void serviceEvents(void *data, MprThread *tp)
{
#if BLD_WIN_LIKE
    mprInitWindow(mprGetMpr(tp)->waitService);
#endif
    mprServiceEvents(tp, -1, 0);
}
#endif


bool mprIsRunningEventsThread(MprCtx ctx)
{
    return mprGetMpr(ctx)->hasEventsThread != 0;
}


/*
 *  Exit the mpr gracefully. Instruct the event loop to exit.
 */
void mprTerminate(MprCtx ctx, bool graceful)
{
    if (! graceful) {
        exit(0);
    }

    mprSignalExit(ctx);
}


bool mprIsExiting(MprCtx ctx)
{
    Mpr *mpr;

    mpr = mprGetMpr(ctx);
    if (mpr == 0) {
        return 1;
    }
    return mpr->flags & MPR_EXITING;
}


void mprSignalExit(MprCtx ctx)
{
    Mpr     *mpr;

    mpr = mprGetMpr(ctx);

    mprSpinLock(mpr->spin);
    mpr->flags |= MPR_EXITING;
    mprSpinUnlock(mpr->spin);

    mprAwakenWaitService(mpr->waitService);
}


int mprSetAppName(MprCtx ctx, cchar *name, cchar *title, cchar *version)
{
    Mpr     *mpr;
    char    *cp;

    mpr = mprGetMpr(ctx);

    if (name) {
        mprFree(mpr->name);
        if ((mpr->name = (char*) mprGetBaseName(mprStrdup(mpr, name))) == 0) {
            return MPR_ERR_CANT_ALLOCATE;
        }
        if ((cp = strrchr(mpr->name, '.')) != 0) {
            *cp = '\0';
        }
    }

    if (title) {
        mprFree(mpr->title);
        if ((mpr->title = mprStrdup(ctx, title)) == 0) {
            return MPR_ERR_CANT_ALLOCATE;
        }
    }

    if (version) {
        mprFree(mpr->version);
        if ((mpr->version = mprStrdup(ctx, version)) == 0) {
            return MPR_ERR_CANT_ALLOCATE;
        }
    }
    return 0;
}


cchar *mprGetAppName(MprCtx ctx)
{
    return mprGetMpr(ctx)->name;
}


cchar *mprGetAppTitle(MprCtx ctx)
{
    return mprGetMpr(ctx)->title;
}


/*
 *  Full host name with domain. E.g. "server.domain.com"
 */
void mprSetHostName(MprCtx ctx, cchar *s)
{
    Mpr     *mpr;

    mpr = mprGetMpr(ctx);
    mprLock(mpr->mutex);
    mprFree(mpr->hostName);
    mpr->hostName = mprStrdup(mpr, s);
    mprUnlock(mpr->mutex);
    return;
}


/*
 *  Return the fully qualified host name
 */
cchar *mprGetHostName(MprCtx ctx)
{
    return mprGetMpr(ctx)->hostName;
}


/*
 *  Server name portion (no domain name)
 */
void mprSetServerName(MprCtx ctx, cchar *s)
{
    Mpr     *mpr;

    mpr = mprGetMpr(ctx);
    mprLock(mpr->mutex);

    if (mpr->serverName) {
        mprFree(mpr->serverName);
    }
    mpr->serverName = mprStrdup(mpr, s);

    mprUnlock(mpr->mutex);
    return;
}


/*
 *  Return the server name
 */
cchar *mprGetServerName(MprCtx ctx)
{
    return mprGetMpr(ctx)->serverName;
}


/*
 *  Set the domain name
 */
void mprSetDomainName(MprCtx ctx, cchar *s)
{
    Mpr     *mpr;

    mpr = mprGetMpr(ctx);
    mprLock(mpr->mutex);

    if (mpr->domainName) {
        mprFree(mpr->domainName);
    }
    mpr->domainName = mprStrdup(mpr, s);

    mprUnlock(mpr->mutex);
    return;
}


/*
 *  Return the domain name
 */
cchar *mprGetDomainName(MprCtx ctx)
{
    return mprGetMpr(ctx)->domainName;
}


cchar *mprGetAppVersion(MprCtx ctx)
{
    Mpr *mpr;

    mpr = mprGetMpr(ctx);
    return mpr->version;
}


#if UNUSED
int mprSetKeyValue(MprCtx ctx, cchar *key, void *ptr)
{
    Mpr *mpr;

    mpr = mprGetMpr(ctx);
    if (mprAddHash(mpr->table, key, ptr) == 0) {
        return MPR_ERR_CANT_WRITE;
    }
    return 0;
}


int mprRemoveKeyValue(MprCtx ctx, cchar *key)
{
    Mpr *mpr;

    mpr = mprGetMpr(ctx);
    return mprRemoveHash(mpr->table, key);
}


cvoid *mprGetKeyValue(MprCtx ctx, cchar *key)
{
    Mpr *mpr;

    mpr = mprGetMpr(ctx);
    return mprLookupHash(mpr->table, key);
}
#endif


bool mprGetDebugMode(MprCtx ctx)
{
    return mprGetMpr(ctx)->debugMode;
}


void mprSetDebugMode(MprCtx ctx, bool on)
{
    mprGetMpr(ctx)->debugMode = on;
}


void mprSetLogHandler(MprCtx ctx, MprLogHandler handler, void *handlerData)
{
    Mpr     *mpr;

    mpr = mprGetMpr(ctx);

    mpr->logHandler = handler;
    mpr->logHandlerData = handlerData;
}


MprLogHandler mprGetLogHandler(MprCtx ctx)
{
    return mprGetMpr(ctx)->logHandler;
}


cchar *mprCopyright()
{
    return  "Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.\n"
    		"Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.";
}


int mprGetEndian(MprCtx ctx)
{
    char    *probe;
    int     test;

    test = 1;
    probe = (char*) &test;
    return (*probe == 1) ? MPR_LITTLE_ENDIAN : MPR_BIG_ENDIAN;
}


/*
 *  Default memory handler
 */
static void memoryFailure(MprCtx ctx, uint size, uint total, bool granted)
{
    if (!granted) {
        mprErrorPrintf(ctx, "Can't allocate memory block of size %d\n", size);
        mprErrorPrintf(ctx, "Total memory used %d\n", total);
        exit(255);
    }
    mprErrorPrintf(ctx, "Memory request for %d bytes exceeds memory red-line\n", size);
    mprErrorPrintf(ctx, "Total memory used %d\n", total);
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
/************************************************************************/
/*
 *  End of file "../mpr.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../mprAlloc.c"
 */
/************************************************************************/

/**
 *  mprAlloc.c - Memory Allocator. This is a layer above malloc providing memory services including: virtual memory mapping,
 *               slab based and arena based allocations.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */


/*
 *  This is a memory "turbo-charger" that sits above malloc. It provides arena and slab based allocations. The goal is
 *  to provide a scalable memory allocator that supports hierarchical allocations and performs well in multi-threaded apps.
 *  It suports arena-based and slab-based allocations.
 * 
 *  This module uses several preprocessor directives to control features:
 *
 *      BLD_FEATURE_MEMORY_DEBUG            Enable checks for block integrity. Fills blocks on allocation and free.
 *      BLD_FEATURE_MEMORY_STATS            Enables accumulation of memory stats.
 *      BLD_FEATURE_MULTITHREAD             Defined if building a multi-threaded application.
 *      BLD_FEATURE_VERIFY                  Adds deep and slow integrity tests.
 *      BLD_FEATURE_MMU                     Enabled if the system has a memory management unit supporting virtual memory.
 */

/*
 *  Disable regions to force all memory allocations to go through malloc. This is useful when using the Mac Instruments.
 */
#define USE_REGIONS 1



/*
 *  Convert from user pointers to memory blocks and back again.
 */
#define GET_BLK(ptr)            ((MprBlk*) (((char*) (ptr)) - MPR_ALLOC_HDR_SIZE))
#define GET_PTR(bp)             ((char*) (((char*) (bp)) + MPR_ALLOC_HDR_SIZE))

//	TODO - remove most of these macros now that bit fields are being used
#define GET_SIZE(bp)            ((bp)->size)
#define GET_USIZE(bp)           ((bp->size) - MPR_ALLOC_HDR_SIZE)
#define SET_SIZE(bp, len)       (bp->size = len)
#define GET_FLAG(bp, flag)      (bp->flags & flag)
#define RESET_FLAG(bp, flag)    (bp->flags &= ~flag)
#define SET_FLAG(bp, flag)      (bp->flags |= flag)

/*
 *  Macros to extract flags.
 *
 *  The next field has:     hasDestructor, hasError and isHeap
 *  The children field has: fromArena
 */
#define HAS_DESTRUCTOR(bp)      GET_FLAG(bp, MPR_ALLOC_HAS_DESTRUCTOR)
#define HAS_ERROR(bp)           GET_FLAG(bp, MPR_ALLOC_HAS_ERROR)
#define SET_ERROR(bp)           SET_FLAG(bp, MPR_ALLOC_HAS_ERROR)
#define RESET_ERROR(bp)         RESET_FLAG(bp, MPR_ALLOC_HAS_ERROR)

#define DESTRUCTOR_PTR(bp)      (GET_PTR(bp) + GET_SIZE(bp) - MPR_ALLOC_HDR_SIZE - sizeof(MprDestructor))
#define GET_DESTRUCTOR(bp)      (HAS_DESTRUCTOR(bp) ? (MprDestructor) (*(MprDestructor*) (DESTRUCTOR_PTR(bp))) : 0)
#define SET_DESTRUCTOR(bp, d)   if (d) { bp->flags |= MPR_ALLOC_HAS_DESTRUCTOR; \
                                    *((MprDestructor*) DESTRUCTOR_PTR(bp)) = d; } else
/*
 *  Store the isHeap flag in the size word
 */
#define IS_HEAP(bp)             GET_FLAG(bp, MPR_ALLOC_IS_HEAP)
#define SET_IS_HEAP(bp)         SET_FLAG(bp, MPR_ALLOC_IS_HEAP)

#if BLD_FEATURE_MEMORY_DEBUG
#define VALID_BLK(bp)           ((bp)->magic == MPR_ALLOC_MAGIC)
#define VALID_CTX(ptr)          (VALID_BLK(GET_BLK(ptr)))
#define SET_MAGIC(bp)           (bp)->magic = MPR_ALLOC_MAGIC

/*
 *  Set this address to break when this address is allocated or freed. This is a block address (not a user ptr).
 */
static MprBlk *stopAlloc;

#else
#define VALID_BLK(bp)           (1)
#define VALID_CTX(ptr)          (1)
#define SET_MAGIC(bp)
#endif

/*
 *  Heaps may be "thread-safe" such that lock and unlock requests on a single heap can come from different threads.
 *  The lock and unlock macros will use spin locks because we expect contention to be very low.
 */
#if BLD_FEATURE_MULTITHREAD
#define lock(heap)               if (unlikely(heap->flags & MPR_ALLOC_THREAD_SAFE)) { mprSpinLock(&heap->spin); } else
#define unlock(heap)             if (unlikely(heap->flags & MPR_ALLOC_THREAD_SAFE)) { mprSpinUnlock(&heap->spin); } else
#else
#define lock(ctx)
#define unlock(ctx)
#endif

/*
 *  Mpr control and root memory context. This is a constant and a permissible global.
 */
Mpr  *_globalMpr;

/*
 *  Allocation control and statistics
 */
static MprAlloc alloc;


static inline MprBlk *allocBlock(MprHeap *heap, uint size);
static int allocException(MprBlk *bp, uint size, bool granted);
static inline void *allocMemory(uint size);
static int  approveAllocation(MprHeap *heap, MprBlk *parent, uint size);
static void *allocError(MprCtx ctx, uint size);
static inline void freeBlock(MprHeap *heap, MprBlk *parent, MprBlk *bp);
static inline void freeMemory(MprBlk *bp);
static inline MprHeap *getHeap(MprBlk *bp);
static inline void initHeap(MprHeap *heap, cchar *name, bool threadSafe);
static inline void linkBlock(MprHeap *heap, MprBlk *parent, MprBlk *bp);
static void sysinit(Mpr *mpr);
static void inline unlinkBlock(MprHeap *heap, MprBlk *bp);

#if USE_REGIONS
static MprRegion *createRegion(MprHeap *heap, uint size);
#endif

#if BLD_WIN_LIKE
static int mapProt(int flags);
#endif

/*
 *  Initialize the memory subsystem
 */
Mpr *mprCreateAllocService(MprAllocNotifier cback, MprDestructor destructor)
{
    Mpr             *mpr;
    MprBlk          *bp;
    uint            usize, size;

    /*
     *  Set defaults to no memory limit
     */
    alloc.redLine = INT_MAX;
    alloc.maxMemory = INT_MAX;

    /*
     *  Hand-craft the first block to optimize subsequent use of mprAlloc. Layout is:
     *      HDR
     *      Mpr
     *          MprHeap
     *      Destructor
     */
    usize = sizeof(Mpr) + sizeof(MprDestructor);
    size = MPR_ALLOC_ALIGN(MPR_ALLOC_HDR_SIZE + usize);
    usize = size - MPR_ALLOC_HDR_SIZE;

    bp = (MprBlk*) allocMemory(size);
    if (bp == 0) {
        if (cback) {
            (*cback)(0, sizeof(Mpr), 0, 0);
        }
        return 0;
    }
    memset(bp, 0, size);

    bp->parent = 0;
    SET_SIZE(bp, size);
    SET_DESTRUCTOR(bp, destructor);

    SET_MAGIC(bp);
    SET_IS_HEAP(bp);

    _globalMpr = mpr = (Mpr*) GET_PTR(bp);

    alloc.bytesAllocated += size;
    alloc.peakAllocated = alloc.bytesAllocated;
    alloc.stackStart = (void*) &mpr;

    mpr->heap.notifier = cback;

    sysinit(mpr);
    initHeap(&mpr->pageHeap, "page", 1);
    mpr->pageHeap.flags = MPR_ALLOC_PAGE_HEAP;
    initHeap(&mpr->heap, "mpr", 1);

#if BLD_FEATURE_MEMORY_DEBUG
    stopAlloc = 0;
#endif

    return mpr;
}


static MprCtx allocHeap(MprCtx ctx, cchar *name, uint heapSize, bool threadSafe, MprDestructor destructor)
{
    MprHeap     *pageHeap, *heap;
    MprRegion   *region;
    MprBlk      *bp, *parent;
    int         headersSize, usize, size;

    mprAssert(ctx);
    mprAssert(VALID_CTX(ctx));

    /*
     *  Allocate the full arena/slab out of one memory allocation. This includes the user object, heap object and 
     *  heap memory. Do this because heaps should generally be initially sized to be sufficient for the apps needs 
     *  (they are virtual with MMUs)
     *
     *  Layout is:
     *      HDR
     *      MprHeap structure
     *      MprRegion structure
     *      Heap data (>= heapSize)
     *
     *  The MprHeap and MprRegion structures are aligned. This may result in the size allocated being bigger 
     *  than the requested heap size.
     */
    headersSize = MPR_ALLOC_ALIGN(sizeof(MprHeap) + sizeof(MprRegion));
    usize = headersSize + heapSize;
    size = MPR_PAGE_ALIGN(MPR_ALLOC_HDR_SIZE + usize, alloc.pageSize);
    usize = (size - MPR_ALLOC_HDR_SIZE);
    heapSize = usize - headersSize;

    parent = GET_BLK(ctx);
    mprAssert(parent);

    /*
     *  All heaps are allocated from the page heap
     */
    pageHeap = &_globalMpr->pageHeap;
    mprAssert(pageHeap);

    lock(pageHeap);

    if (unlikely((bp = allocBlock(pageHeap, usize)) == 0)) {
        allocError(GET_PTR(parent), usize);
        return 0;
    }
    SET_IS_HEAP(bp);

    linkBlock(pageHeap, parent, bp);
    unlock(pageHeap);

    heap = (MprHeap*) GET_PTR(bp);
    heap->destructor = destructor;
    initHeap((MprHeap*) heap, name, threadSafe);

    region = (MprRegion*) ((char*) heap + sizeof(MprHeap));
    region->next = 0;
    region->memory = (char*) heap + headersSize;
    region->nextMem = region->memory;
    region->vmSize = MPR_ALLOC_ALIGN(MPR_ALLOC_HDR_SIZE + usize);
    region->size = heapSize;
    region->remaining = heapSize;

    heap->region = region;

    return GET_PTR(bp);
}


/*
 *  Create an arena memory context. An arena context is a memory heap which allocates all child requests from a 
 *  single (logical) memory block. Allocations are done like simple salami slices. Arenas may be created thread-safe, 
 *  but are thread insensitive by default and will allocate memory without any locking. Hence allocations will be 
 *  fast and scalable.
 *
 *  This call is typically made via the macro: mprCreateArena. userObjSize is the size of the object to create that will
 *  represent the memory context for the heap. ArenaSize is the total size of the arena. If arenaMax is > arenaSize, then
 *  the arena can grow in chunks of arenaSize.
 */
MprHeap *mprAllocArena(MprCtx ctx, cchar *name, uint arenaSize, bool threadSafe, MprDestructor destructor)
{
    MprHeap     *heap;

    mprAssert(ctx);
    mprAssert(VALID_CTX(ctx));
    mprAssert(arenaSize > 0);

    heap = (MprHeap*) allocHeap(ctx, name, arenaSize, threadSafe, destructor);
    if (heap == 0) {
        return 0;
    }
    heap->flags = MPR_ALLOC_ARENA_HEAP;

    return heap;
}


/*
 *  Create an object slab memory context. An object slab context is a memory heap which allocates constant size objects from
 *  a single (logical) memory block. The object slab keeps a free list of freed blocks. Object slabs may be created thread-safe,
 *  but are thread insensitive by default and will allocate memory without any locking. Hence allocations will be fast and
 *  scalable.
 *
 *  This call is typically made via the macro mprCreateSlab. ObjSize is the size of objects to create from the slab heap.
 *  The count parameter indicates how may objects the slab heap will initially contain. MaxCount is the the maximum the heap
 *  will ever support. If maxCount is greater than count, then the slab is growable.
 */
MprHeap *mprAllocSlab(MprCtx ctx, cchar *name, uint objSize, uint count, bool threadSafe, MprDestructor destructor)
{
    MprHeap     *heap;
    uint        size;

    mprAssert(ctx);
    mprAssert(VALID_CTX(ctx));
    mprAssert(objSize > 0);
    mprAssert(count > 0);

    size = MPR_ALLOC_ALIGN(objSize) * count;

    heap = (MprHeap*) allocHeap(ctx, name, size, threadSafe, destructor);
    if (heap == 0) {
        return 0;
    }
    heap->flags = MPR_ALLOC_SLAB_HEAP;
    return heap;
}


/*
 *  Allocate a block. Not used to allocate heaps.
 */
void *mprAlloc(MprCtx ctx, uint usize)
{
    MprBlk      *bp, *parent;
    MprHeap     *heap;
    int         size;

    mprAssert(ctx);
    mprAssert(usize >= 0);
    mprAssert(VALID_CTX(ctx));

    parent = GET_BLK(ctx);
    mprAssert(parent);

    heap = getHeap(parent);
    mprAssert(heap);

    lock(heap);

    size = MPR_ALLOC_ALIGN(MPR_ALLOC_HDR_SIZE + usize);
    usize = size - MPR_ALLOC_HDR_SIZE;

    if (unlikely(approveAllocation(heap, parent, size) < 0)) {
        allocError(GET_PTR(parent), usize);
        return 0;
    }
    if (unlikely((bp = allocBlock(heap, usize)) == 0)) {
        allocError(GET_PTR(parent), usize);
        return 0;
    }
    mprAssert(GET_SIZE(bp) == size);

#if BLD_FEATURE_MEMORY_DEBUG
    /*
     *  Catch uninitialized use
     */
    memset(GET_PTR(bp), 0xf7, usize);
#endif

    linkBlock(heap, parent, bp);
    unlock(heap);

    return GET_PTR(bp);
}


/*
 *  Allocate and zero a block
 */
void *mprAllocZeroed(MprCtx ctx, uint size)
{
    void    *newBlock;

    newBlock = mprAlloc(ctx, size);
    mprAssert(newBlock);

    if (newBlock) {
        memset(newBlock, 0, size);
    }
    return newBlock;
}


/*
 *  Allocate an object. Typically used via the macro: mprAllocObj
 */
void *mprAllocObject(MprCtx ctx, uint size, MprDestructor destructor)
{
    MprBlk      *bp;
    void        *ptr;

    mprAssert(VALID_CTX(ctx));
    mprAssert(size > 0);

    ptr = mprAlloc(ctx, size + sizeof(MprDestructor));
    mprAssert(ptr);
    if (ptr == 0) {
        return 0;
    }

    bp = GET_BLK(ptr);
    SET_DESTRUCTOR(bp, destructor);

    return ptr;
}


void mprSetDestructor(void *ptr, MprDestructor destructor)
{
    MprBlk      *bp;

    bp = GET_BLK(ptr);
    SET_DESTRUCTOR(bp, destructor);
}


void mprInitBlock(MprCtx ctx, void *ptr, uint size)
{
    MprBlk      *bp;

    bp = GET_BLK(ptr);

    memset(ptr, 0, size);
    SET_SIZE(bp, size);
    bp->parent = MPR_GET_BLK(mprGetMpr(ctx));
    bp->size = 0;
    bp->flags = 0;
    bp->next = 0;
    bp->children = 0;
    SET_MAGIC(bp);
}


/*
 *  Allocate and zero a block
 */
void *mprAllocObjectZeroed(MprCtx ctx, uint size, MprDestructor destructor)
{
    void    *newBlock;

    newBlock = mprAllocObject(ctx, size, destructor);
    if (newBlock) {
        memset(newBlock, 0, size);
    }
    return newBlock;
}


/*
 *  Free a block of memory. Free all children recursively. Return 0 if the memory was freed. A destructor may prevent
 *  memory being deleted by returning MPR_ALLOC_DONT_FREE.
 */
int mprFree(void *ptr)
{
    MprHeap     *heap, *hp;
    MprBlk      *bp, *parent, *child;

    if (unlikely(ptr == 0)) {
        return 0;
    }
    
    bp = GET_BLK(ptr);
    mprAssert(VALID_BLK(bp));
    mprAssert(bp->size > 0);

#if BLD_FEATURE_MEMORY_DEBUG
    if (bp == stopAlloc) {
        mprBreakpoint();
    }

    /*
     *  Test if already freed
     */
    if (unlikely(bp->parent == 0 && ptr != _globalMpr)) {
        mprAssert(bp->parent);
        return 0;
    }
#endif

    /*
     *  We need to run destructors first if there is a destructor and it isn't a heap
     */
    if (unlikely(HAS_DESTRUCTOR(bp))) {
        (GET_DESTRUCTOR(bp))(ptr);
    }
    
    /*
     *  Free the children. They are linked in LIFO order. So free from the start and it will actually free in reverse order.
     *  ie. last allocated will be first freed.
     */
    while (likely((child = bp->children) != NULL)) {
        mprAssert(VALID_BLK(child));
        mprFree(GET_PTR(child));
    }

    parent = bp->parent;

    if (unlikely(IS_HEAP(bp))) {
        hp = (MprHeap*) ptr;
        if (hp->destructor) {
            hp->destructor(ptr);
        }
        heap = &_globalMpr->pageHeap;

    } else {
        mprAssert(VALID_BLK(parent));
        heap = getHeap(parent);
    }
    mprAssert(heap);

    lock(heap);
    unlinkBlock(heap, bp);
    freeBlock(heap, parent, bp);
    unlock(heap);

    return 0;
}



/*
 *  Free the children of a block of memory
 */
void mprFreeChildren(void *ptr)
{
    MprBlk          *bp, *child, *next;

    if (unlikely(ptr == 0)) {
        return;
    }

    bp = GET_BLK(ptr);
    mprAssert(VALID_BLK(bp));

    /*
     *  Free the children. They are linked in LIFO order.
     */
    if (likely((child = bp->children) != NULL)) {
        do {
            mprAssert(VALID_BLK(child));
            next = child->next;
            mprFree(GET_PTR(child));
        } while ((child = next) != 0);
        bp->children = 0;
    }
}



/*
 *  Rallocate a block
 */
void *mprRealloc(MprCtx ctx, void *ptr, uint usize)
{
    MprHeap     *heap;
    MprBlk      *parent, *bp, *newbp, *child;
    void        *newPtr;

    mprAssert(VALID_CTX(ctx));
    mprAssert(usize > 0);

    if (ptr == 0) {
        return mprAlloc(ctx, usize);
    }

    mprAssert(VALID_CTX(ptr));
    bp = GET_BLK(ptr);
    mprAssert(bp);
    mprAssert(bp->parent);

    if (usize < GET_USIZE(bp)) {
        return ptr;
    }

    parent = GET_BLK(ctx);
    mprAssert(parent);

    newPtr = mprAlloc(ctx, usize);
    if (newPtr == 0) {
        return 0;
    }

    newbp = GET_BLK(newPtr);
    mprAssert(newbp->parent == parent);

    memcpy(GET_PTR(newbp), GET_PTR(bp), GET_USIZE(bp));

    heap = getHeap(parent);
    mprAssert(heap);
    lock(heap);

    /*
     *  Remove old block
     */
    unlinkBlock(heap, bp);

    /*
     *  Fix the parent pointer of all children
     */
    for (child = bp->children; child; child = child->next) {
        child->parent = newbp;
    }
    newbp->children = bp->children;
    unlock(heap);

    freeBlock(heap, parent, bp);
    return newPtr;
}



static int getBlockSize(MprBlk *bp) 
{
    MprBlk      *child;
    int         size;
    
    size = GET_SIZE(bp);
    for (child = bp->children; child; child = child->next) {
        size += getBlockSize(child);
    }
    return size;
}



/*
 *  Steal a block from one context and insert in a new context. Ptr is inserted into the Ctx context.
 */
int mprStealBlock(MprCtx ctx, cvoid *ptr)
{
    MprHeap     *heap, *newHeap;
    MprBlk      *bp, *parent, *newParent;
    int         size;

    if (ptr == 0) {
        return 0;
    }

    mprAssert(VALID_CTX(ctx));
    mprAssert(VALID_CTX(ptr));

    bp = GET_BLK(ptr);

#if BLD_FEATURE_MEMORY_VERIFY
    /*
     *  Ensure bp is not a parent of the nominated context.
     */
    for (parent = GET_BLK(ctx); parent; parent = parent->parent) {
        mprAssert(parent != bp);
    }
#endif

    parent = bp->parent;
    mprAssert(VALID_BLK(parent));
    heap = getHeap(parent);
    mprAssert(heap);

    newParent = GET_BLK(ctx);
    mprAssert(VALID_BLK(newParent));
    newHeap = getHeap(newParent);
    mprAssert(newHeap);

    size = getBlockSize(bp);

    lock(heap);
    unlinkBlock(heap, bp);
    heap->allocBytes += GET_SIZE(bp) - size;
    unlock(heap);

#if FUTURE
    if (newHeap != heap) {
        /* BETTER DEFINE FOR from malloc pool */
        if ((bp->flags & MPR_ALLOC_IS_HEAP) || (heap->flags & (MPR_ALLOC_ARENA_HEAP | MPR_ALLOC_SLAB_HEAP))) {
            mprError(ctx, "Stealing block across heaps");
            return 0;
        }
    }
#endif

    lock(newHeap);
    linkBlock(newHeap, newParent, bp);
    newHeap->allocBytes += size - GET_SIZE(bp);
    unlock(newHeap);

    return 0;
}


char *mprStrdup(MprCtx ctx, cchar *str)
{
    char    *newp;
    int     len;

    mprAssert(VALID_CTX(ctx));

    if (str == 0) {
        str = "";
    }

    len = (int) strlen(str) + 1;

    //  TODO - should we do something special for arenas
    newp = (char*) mprAlloc(ctx, len);
    if (newp) {
        memcpy(newp, str, len);
    }

    return newp;
}


/*
 *  strndup function
 */
char *mprStrndup(MprCtx ctx, cchar *str, uint usize)
{
    char    *newp;
    uint    len;

    mprAssert(VALID_CTX(ctx));

    if (str == 0) {
        str = "";
    }
    len = (int) strlen(str) + 1;
    len = min(len, usize);

    newp = (char*) mprAlloc(ctx, len);

    if (newp) {
        memcpy(newp, str, len);
    }

    return newp;
}


void *mprMemdup(MprCtx ctx, const void *ptr, uint usize)
{
    char    *newp;

    mprAssert(VALID_CTX(ctx));

    newp = (char*) mprAlloc(ctx, usize);
    if (newp) {
        memcpy(newp, ptr, usize);
    }

    return newp;
}


/*
 *  Check a memory allocation request against configured maximums and redlines. We do this so that the application does
 *  not need to check the result of every little memory allocation. Rather, an application-wide memory allocation failure
 *  can be invoked proactively when a memory redline is exceeded. It is the application's responsibility to set the red-line
 *  value suitable for the system.
 */
static int approveAllocation(MprHeap *heap, MprBlk *parent, uint size)
{
    int     diff;

    /*
     *  Don't worry about races on bytesAllocated here. Not critical
     */
    if ((size + alloc.bytesAllocated) > alloc.maxMemory) {
        /*
         *  Prevent allocation if over the maximum
         */
        if (allocException(parent, size, 0) < 0) {
            mprAssert(0);
            mprSetAllocError(parent);
            return -1;
        }

    } else if ((size + alloc.bytesAllocated) > alloc.redLine) {
        /*
         *  Warn if allocation puts us over the red line
         */
        if (allocException(parent, size, 1) < 0) {
            mprAssert(0);
            return 0;
        }
    }

    /*
     *  Update the global memory usage stats. Very short duration lock.
     */
    mprSpinLock(&_globalMpr->heap.spin);
    alloc.bytesAllocated += size;
    if (alloc.bytesAllocated > alloc.peakAllocated) {
        alloc.peakAllocated = alloc.bytesAllocated;
    }
    mprSpinUnlock(&_globalMpr->heap.spin);

    /*
     *  Monitor stack usage. Don't worry about races here. Not critically important.
     */
    diff = (int) ((char*) alloc.stackStart - (char*) &diff);
    if (diff < 0) {
        alloc.peakStack -= diff;
        alloc.stackStart = (void*) &diff;
        diff = 0;
    }

    if ((uint) diff > alloc.peakStack) {
        alloc.peakStack = diff;
    }

    return 0;
}


/*
 *  Allocate a block from a heap
 */
static inline MprBlk *allocBlock(MprHeap *heap, uint usize)
{
    MprBlk      *bp;
    uint        size;
#if USE_REGIONS
    MprRegion   *region;
#endif

    size = MPR_ALLOC_ALIGN(MPR_ALLOC_HDR_SIZE + usize);
    usize = size - MPR_ALLOC_HDR_SIZE;

#if USE_REGIONS
    if (likely(heap->flags & MPR_ALLOC_ARENA_HEAP)) {
        /*
         *  Allocate a block from an arena heap
         */
        region = heap->region;
        mprAssert(region);
        mprAssert(region->nextMem);
        mprAssert(region->size > 0);

        if ((region->nextMem + size) > &region->memory[region->size]) {
            if ((region = createRegion(heap, size)) == NULL) {
                return 0;
            }
        }
        bp = (MprBlk*) region->nextMem;
        mprAssert(bp);
        bp->flags = 0;
        bp->size = 0;
        region->nextMem += size;
        region->remaining -= size;
        mprAssert(region->remaining >= 0);

    } else if (likely(heap->flags & MPR_ALLOC_SLAB_HEAP)) {
        /*
         *  Allocate a block from a slab heap
         */
        region = heap->region;
        mprAssert(region);
        mprAssert(size == heap->objSize);

        if ((bp = heap->freeList) != 0) {
            heap->freeList = bp->next;
            heap->reuseCount++;

        } else {
            if ((region->nextMem + size) > &region->memory[region->size]) {
                if ((region = createRegion(heap, size)) == NULL) {
                    return 0;
                }
            }
            bp = (MprBlk*) region->nextMem;
            mprAssert(bp);
            region->nextMem += size;
            region->remaining -= size;
            mprAssert(region->remaining >= 0);
        }
        bp->size = 0;
        bp->flags = 0;

    } else if (heap->flags & MPR_ALLOC_PAGE_HEAP) {
        if ((bp = (MprBlk*) mprMapAlloc(size, MPR_MAP_READ | MPR_MAP_WRITE)) == 0) {
            return 0;
        }
        bp->size = 0;
        bp->flags = 0;

    } else {
#endif
        if ((bp = (MprBlk*) allocMemory(size)) == 0) {
            return 0;
        }
        //  TODO - can now set size inline here
        bp->size = 0;
        bp->flags = 0;
        bp->flags |= MPR_ALLOC_FROM_MALLOC;
#if USE_REGIONS
    }
#endif
    mprAssert(bp);

#if BLD_FEATURE_MEMORY_DEBUG
    if (bp == stopAlloc) {
        mprBreakpoint();
    }
#endif

    bp->children = 0;
    bp->next = 0;

    SET_SIZE(bp, size);
    SET_MAGIC(bp);

    return bp;
}


/*
 *  Free a block back to a heap. WARNING: calls to mprFree to free a block allocated from an arean will not come here.
 */
static inline void freeBlock(MprHeap *heap, MprBlk *parent, MprBlk *bp)
{
    int         size;

#if USE_REGIONS
    MprHeap     *hp;
    MprRegion   *region, *next;
#endif

    if (IS_HEAP(bp) && bp != GET_BLK(_globalMpr)) {
#if USE_REGIONS
        hp = (MprHeap*) GET_PTR(bp);
        if (hp->depleted) {
            /*
             *  If there are depleted blocks, then the region contained in the heap memory block will be on 
             *  the depleted list. Must not free it here. Also, the region pointer for the original heap 
             *  block does not point to the start of the memory block to free.
             */
            region = hp->depleted;
            while (region) {
                next = region->next;
                if ((char*) region != ((char*) hp + sizeof(MprHeap))) {
                    /*
                     *  Don't free the initial region which is part of the heap (hp) structure
                     */
                    mprMapFree(region, region->vmSize);
                }
                region = next;
            }
            mprMapFree(hp->region, hp->region->vmSize);
        }
        mprMapFree(bp, GET_SIZE(bp));
#else
        freeMemory(bp);
#endif
        return;
    }

    size = GET_SIZE(bp);
    mprSpinLock(&_globalMpr->heap.spin);
    alloc.bytesAllocated -= size;
    mprAssert(alloc.bytesAllocated >= 0);
    mprSpinUnlock(&_globalMpr->heap.spin);

#if USE_REGIONS
    if (!(bp->flags & MPR_ALLOC_FROM_MALLOC)) {
        if (heap->flags & MPR_ALLOC_ARENA_HEAP) {
            /*
             *  Just ignore the memory. It will be reclaimed when the arena is freed.
             */
#if BLD_FEATURE_MEMORY_DEBUG
            bp->parent = 0;
            bp->next = 0;
#endif
            return;

        } else if (heap->flags & MPR_ALLOC_SLAB_HEAP) {
            bp->next = heap->freeList;
            bp->parent = 0;
            heap->freeList = bp;
            heap->freeListCount++;
            if (heap->freeListCount > heap->peakFreeListCount) {
                heap->peakFreeListCount = heap->freeListCount;
            }
            return;
        }
    }
#endif

    freeMemory(bp);
}


#if USE_REGIONS
/*
 *  Create a new region to satify the request if no memory exists in any depleted regions. 
 */
static MprRegion *createRegion(MprHeap *heap, uint usize)
{
    MprRegion   *region;
    uint        size, regionSize, regionStructSize;

    /*
     *  Scavenge the depleted regions for scraps. We don't expect there to be many of these.
     *  TODO - do we really want to do this?
     */
    if (usize < 512) {
        for (region = heap->depleted; region; region = region->next) {
            if ((region->nextMem + usize) < &region->memory[region->size]) {
                return region;
            }
        }
    }

    /*
     *  Each time we grow the heap, double the size of the next region of memory.
     */
    regionSize = heap->region->size * 2;

    regionStructSize = MPR_ALLOC_ALIGN(sizeof(MprRegion));
    size = max(usize, (regionStructSize + regionSize));
    size = MPR_PAGE_ALIGN(size, alloc.pageSize);
    usize = size - regionStructSize;

    if ((region = (MprRegion*) mprMapAlloc(size, MPR_MAP_READ | MPR_MAP_WRITE)) == 0) {
        return 0;
    }

    region->memory = (char*) region + regionStructSize;
    region->nextMem = region->memory;
    region->vmSize = size;
    region->size = usize;
    region->remaining = usize;

    /*
     *  Move old region to depleted and install new region as the current heap region
     */
    heap->region->next = heap->depleted;
    heap->depleted = heap->region;
    heap->region = region;

    return region;
}
#endif


static inline void linkBlock(MprHeap *heap, MprBlk *parent, MprBlk *bp)
{
#if BLD_FEATURE_MEMORY_VERIFY
    MprBlk      *sibling;

    /*
     *  Test that bp is not already in the list
     */
    mprAssert(bp != parent);
    for (sibling = parent->children; sibling; sibling = sibling->next) {
        mprAssert(sibling != bp);
    }
#endif

    bp->parent = parent;
    bp->next = parent->children;
    parent->children = bp;

    if (unlikely(IS_HEAP(bp))) {
        heap->reservedBytes += GET_SIZE(bp);
    } else {
        heap->totalAllocCalls++;
        heap->allocBlocks++;
        if (heap->allocBlocks > heap->peakAllocBlocks) {
            heap->peakAllocBlocks = heap->allocBlocks;
        }
        heap->allocBytes += GET_SIZE(bp);
        if (heap->allocBytes > heap->peakAllocBytes) {
            heap->peakAllocBytes = heap->allocBytes;
        }
    }
}


static inline void unlinkBlock(MprHeap *heap, MprBlk *bp)
{
    MprBlk      *sibling, *parent;

    mprAssert(bp);

    if (unlikely(IS_HEAP(bp))) {
        heap->reservedBytes += GET_SIZE(bp);
    } else {
        heap->allocBytes -= GET_SIZE(bp);
        heap->allocBlocks--;
    }
    mprAssert(heap->allocBytes >= 0);

    parent = bp->parent;
    if (unlikely(parent == 0)) {
        return;
    }

    if (unlikely(parent->children == bp)) {
        parent->children = bp->next;
        bp->parent = 0;
        bp->next = 0;
        return;

    } else {
        for (sibling = parent->children; sibling; sibling = sibling->next) {
            if (sibling->next == bp) {
                sibling->next = bp->next;
                bp->parent = 0;
                bp->next = 0;
                return;
            }
        }
    }
    mprAssert(0);
}


static inline void initHeap(MprHeap *heap, cchar *name, bool threadSafe)
{
    heap->name = name;
    heap->region = 0;
    heap->depleted = 0;
    heap->flags = 0;
    heap->objSize = 0;
    heap->freeList = 0;
    heap->freeListCount = 0;
    heap->reuseCount = 0;

    heap->allocBlocks = 0;
    heap->peakAllocBlocks = 0;
    heap->allocBytes = 0;
    heap->peakAllocBytes = 0;
    heap->totalAllocCalls = 0;
    heap->peakFreeListCount = 0;

    heap->notifier = 0;

#if BLD_FEATURE_MULTITHREAD
    if (threadSafe) {
        mprCreateStaticSpinLock(heap, &heap->spin);
    }
#endif
}


/*
 *  Find the heap from which a block has been allocated. Chase up the parent chain.
 */
static inline MprHeap *getHeap(MprBlk *bp)
{
    mprAssert(bp);
    mprAssert(VALID_BLK(bp));

    while (!(IS_HEAP(bp))) {
        bp = bp->parent;
        mprAssert(bp);
    }
    return (MprHeap*) GET_PTR(bp);
}


void mprSetAllocNotifier(MprCtx ctx, MprAllocNotifier cback)
{
    MprHeap     *heap;

    heap = getHeap(GET_BLK(ctx));
    heap->notifier = cback;
}


/*
 *  Monitor stack usage. Return true if the stack has grown. This routine uses no locking and thus yields approximate results.
 */
bool mprStackCheck(MprCtx ptr)
{
    int     size;

    mprAssert(VALID_CTX(ptr));

    size = (int) ((char*) alloc.stackStart - (char*) &size);
    if (size < 0) {
        alloc.peakStack -= size;
        alloc.stackStart = (void*) &size;
        size = 0;
    }
    if ((uint) size > alloc.peakStack) {
        alloc.peakStack = size;
        return 1;
    }
    return 0;
}


void mprSetAllocLimits(MprCtx ctx, uint redLine, uint maxMemory)
{
    if (redLine > 0) {
        alloc.redLine = redLine;
    }
    if (maxMemory > 0) {
        alloc.maxMemory = maxMemory;
    }
}


#if NOT_MACRO
int mprGetBlockSize(MprCtx ptr)
{
    return GET_SIZE(GET_BLK(ptr));
}
#endif


void *mprGetParent(MprCtx ptr)
{
    MprBlk  *bp;

    if (ptr == 0) {
        return 0;
    }

    mprAssert(VALID_CTX(ptr));

    bp = GET_BLK(ptr);
    mprAssert(VALID_BLK(bp));
    mprAssert(bp->parent);

    return GET_PTR(bp->parent);
}



MprAlloc *mprGetAllocStats(MprCtx ctx)
{
    return &alloc;
}



int mprGetUsedMemory(MprCtx ctx)
{
    return alloc.bytesAllocated;
}



int mprIsValid(MprCtx ptr)
{
    MprBlk  *bp;

    bp = GET_BLK(ptr);
    return (bp && VALID_BLK(bp));
}


#if BLD_WIN_LIKE
/*
 *  Get the ultimate block parent
 */
Mpr *mprGetMpr(MprCtx ignored)
{
    return (Mpr*) _globalMpr;
}
#endif



bool mprHasAllocError(MprCtx ctx)
{
    MprBlk  *bp;

    bp = GET_BLK(ctx);
    return HAS_ERROR(bp) ? 1 : 0;
}



/*
 *  Reset the allocation error flag at this block and all parent blocks
 */
void mprResetAllocError(MprCtx ctx)
{
    MprBlk  *bp;

    //  TODO - locking
    bp = GET_BLK(ctx);
    while (bp) {
        RESET_ERROR(bp);
        bp = bp->parent;
    }
}



/*
 *  Set the allocation error flag at this block and all parent blocks
 */
void mprSetAllocError(MprCtx ctx)
{
    MprBlk  *bp;

    //  TODO - locking
    bp = GET_BLK(ctx);
    while (bp) {
        SET_ERROR(bp);
        bp = bp->parent;
    }
}


/*
 *  Called to invoke the memory failure handler on a memory allocation error
 */
static int allocException(MprBlk *parent, uint size, bool granted)
{
    MprHeap     *heap;

    mprAssert(VALID_BLK(parent));

    if (alloc.inAllocException == 0) {

        alloc.inAllocException = 1;

        /*
         *  Notify all the heaps up the chain
         */
        for (heap = getHeap(parent); heap; heap = getHeap(parent)) {
            if (heap->notifier) {
                //  TODO - notifier returns int - what is this?
                (heap->notifier)(parent, size, alloc.bytesAllocated, granted);
            }
            parent = parent->parent;
            if (parent == 0) {
                break;
            }
        }

        alloc.inAllocException = 0;
        return -1;
    }

    return 0;
}


/*
 *  Handle an allocation error. Return zero so callers can return the result of this function.
 */
static void *allocError(MprCtx ctx, uint size)
{
    alloc.errors++;
    mprSetAllocError(ctx);
    allocException(GET_BLK(ctx), size, 0);
    return 0;
}


/*
 *  Get information about the system. Get page size and number of CPUs.
 */
static void sysinit(Mpr *mpr)
{
    alloc.numCpu = 1;
    alloc.pageSize = 4096;

#if MACOSX
    alloc.numCpu = sysconf(_SC_NPROCESSORS_ONLN);
    alloc.pageSize = sysconf(_SC_PAGESIZE);
#elif BLD_WIN_LIKE
{
    SYSTEM_INFO     info;

    GetSystemInfo(&info);
    alloc.numCpu = info.dwNumberOfProcessors;
    alloc.pageSize = info.dwPageSize;

}
#elif FREEBSD
    {
        int     cmd[2];
        size_t  len;

        /*
         *  Get number of CPUs
         */
        cmd[0] = CTL_HW;
        cmd[1] = HW_NCPU;
        len = sizeof(alloc.numCpu);
        if (sysctl(cmd, 2, &alloc.numCpu, &len, 0, 0) < 0) {
            alloc.numCpu = 1;
        }

        /*
         *  Get page size
         */
        alloc.pageSize = sysconf(_SC_PAGESIZE);
    }
#elif LINUX
    {
        static const char processor[] = "processor\t:";
        char    c;
        int     fd, col, match;

        fd = open("/proc/cpuinfo", O_RDONLY);
        if (fd < 0) {
            return;
        }
        match = 1;
        for (col = 0; read(fd, &c, 1) == 1; ) {
            if (c == '\n') {
                col = 0;
                match = 1;
            } else {
                if (match && col < (sizeof(processor) - 1)) {
                    if (c != processor[col]) {
                        match = 0;
                    }
                    col++;

                } else if (match) {
                    alloc.numCpu++;
                    match = 0;
                }
            }
        }
        --alloc.numCpu;
        close(fd);

        /*
         *  Get page size
         */
        alloc.pageSize = sysconf(_SC_PAGESIZE);
    }
#endif
}



int mprGetPageSize(MprCtx ctx)
{
    return alloc.pageSize;
}


/*
 *  Virtual memory support. Map virutal memory into the address space and commit.
 */
void *mprMapAlloc(uint size, int mode)
{
    void        *ptr;

    size = MPR_PAGE_ALIGN(size, alloc.pageSize);

#if BLD_FEATURE_MMU
        /*
         *  Has virtual memory
         */
    #if BLD_UNIX_LIKE
        ptr = mmap(0, size, mode, MAP_PRIVATE | MAP_ANON, -1, 0);
    #elif BLD_WIN_LIKE
        ptr = VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT, mapProt(mode));
    #else
        ptr = malloc(size);
    #endif
#else
    /*
     *  No MMU
     */
    ptr = malloc(size);
#endif

    if (ptr == 0) {
        return 0;
    }
    return ptr;
}



void mprMapFree(void *ptr, uint size)
{
#if BLD_FEATURE_MMU
        /*
         *  Has virtual memory
         */
    #if BLD_UNIX_LIKE
        if (munmap(ptr, size) != 0) {
            mprAssert(0);
        }
    #elif BLD_WIN_LIKE
        VirtualFree(ptr, 0, MEM_RELEASE);
    #else
        mprFree(ptr);
    #endif
#else
    /*
     *  Has no MMU
     */
    free(ptr);
#endif
}


#if BLD_WIN_LIKE
static int mapProt(int flags)
{
    if (flags & MPR_MAP_EXECUTE) {
        return PAGE_EXECUTE_READWRITE;
    } else if (flags & MPR_MAP_WRITE) {
        return PAGE_READWRITE;
    }
    return PAGE_READONLY;
}
#endif



/*
 *  Actually allocate memory. Just use ordinary malloc. Arenas and slabs will use MapAlloc not this.
 */
static inline void *allocMemory(uint size)
{
    return malloc(size);
}



static inline void freeMemory(MprBlk *bp)
{
#if BLD_FEATURE_MEMORY_DEBUG
    int     size;
    
    /*
     *  Free with unique signature to catch block-reuse
     */
    size = GET_SIZE(bp);
    memset(bp, 0xF1, size);
#endif
    free(bp);
}



#if BLD_FEATURE_MEMORY_DEBUG
void mprValidateBlock(MprCtx ctx)
{
    MprBlk      *bp, *parent, *sibling, *child;

    mprAssert(VALID_CTX(ctx));

    bp = GET_BLK(ctx);

    if (bp == GET_BLK(_globalMpr)) {
        return;
    }

    mprAssert(bp->parent);
    mprAssert(VALID_BLK(bp->parent));
    parent = bp->parent;

    /*
     *  Find this block in the parent chain
     */
    for (sibling = parent->children; sibling; sibling = sibling->next) {
        mprAssert(VALID_BLK(sibling));
        mprAssert(sibling != parent);
        mprAssert(sibling->parent == parent);
        if (sibling->children) {
            mprAssert(VALID_BLK(sibling->children));
        }
        if (sibling == bp) {
            break;
        }
    }
    mprAssert(sibling);

    /*
     *  Check the rest of the siblings
     */
    if (sibling) {
        for (sibling = sibling->next; sibling; sibling = sibling->next) {
            mprAssert(VALID_BLK(sibling));
            mprAssert(sibling != parent);
            mprAssert(sibling->parent == parent);
            if (sibling->children) {
                mprAssert(VALID_BLK(sibling->children));
            }
            mprAssert(sibling != bp);
        }
    }

    /*
     *  Validate children (recursively)
     */
    for (child = bp->children; child; child = child->next) {
        mprAssert(child != bp);
        mprValidateBlock(GET_PTR(child));
    }
}
#endif



#if BLD_FEATURE_MEMORY_STATS

#define percent(a,b) ((a / 1000) * 100 / (b / 1000))


/*
 *  Traverse all blocks and look for heaps
 */
static void printMprHeaps(MprCtx ctx)
{
    MprAlloc    *ap;
    MprBlk      *bp, *child;
    MprHeap     *heap;
    MprRegion   *region;
    cchar       *kind;
    int         available, total;

    bp = MPR_GET_BLK(ctx);

    if (bp->size & MPR_ALLOC_IS_HEAP) {
        ap = mprGetAllocStats(ctx);
        heap = (MprHeap*) ctx;
        if (heap->flags & MPR_ALLOC_PAGE_HEAP) {
            kind = "page";
        } else if (heap->flags & MPR_ALLOC_ARENA_HEAP) {
            kind = "arena";
        } else if (heap->flags & MPR_ALLOC_SLAB_HEAP) {
            kind = "slab";
        } else {
            kind = "general";
        }
        mprLog(ctx, 0, "\n    Heap                     %10s (%s)",       heap->name, kind);

        available = 0;
        total = 0;
        for (region = heap->depleted; region; region = region->next) {
            available += region->remaining;
            total += region->size;
        }
        if (heap->region) {
            total += heap->region->size;
        }

        mprLog(ctx, 0, "    Allocated memory         %,10d K",          heap->allocBytes / 1024);
        mprLog(ctx, 0, "    Peak heap memory         %,10d K",          heap->peakAllocBytes / 1024);
        mprLog(ctx, 0, "    Allocated blocks         %,10d",            heap->allocBlocks);
        mprLog(ctx, 0, "    Peak heap blocks         %,10d",            heap->peakAllocBlocks);
        mprLog(ctx, 0, "    Alloc calls              %,10d",            heap->totalAllocCalls);

        if (heap->flags & (MPR_ALLOC_PAGE_HEAP | MPR_ALLOC_ARENA_HEAP | MPR_ALLOC_SLAB_HEAP)) {
            mprLog(ctx, 0, "    Heap Regions             %,10d K",      total / 1024);
            mprLog(ctx, 0, "    Depleted regions         %,10d K",      available / 1024);
            if (heap->region) {
                mprLog(ctx, 0, "    Unallocated memory       %,10d K",  heap->region->remaining / 1024);
            }            
        }
            
        if (heap->flags & MPR_ALLOC_PAGE_HEAP) {
            mprLog(ctx, 0, "    Page size                %,10d",         ap->pageSize);

        } else if (heap->flags & MPR_ALLOC_ARENA_HEAP) {

        } else if (heap->flags & MPR_ALLOC_SLAB_HEAP) {
            mprLog(ctx, 0, "    Heap object size         %,10d bytes",   heap->objSize);
            mprLog(ctx, 0, "    Heap free list count     %,10d",         heap->freeListCount);
            mprLog(ctx, 0, "    Heap peak free list      %,10d",         heap->peakFreeListCount);
            mprLog(ctx, 0, "    Heap reuse count         %,10d",         heap->reuseCount);
        }
    }
    for (child = bp->children; child; child = child->next) {
        printMprHeaps(MPR_GET_PTR(child));
    }
}
#endif


void mprPrintAllocReport(MprCtx ctx, cchar *msg)
{
#if BLD_FEATURE_MEMORY_STATS
    MprAlloc    *ap;

    ap = &alloc;

    /*
     *  MPR stats
     */
    mprLog(ctx, 0, "\n\n\nMPR Memory Report %s", msg);
    mprLog(ctx, 0, "------------------------------------------------------------------------------------------\n");
    mprLog(ctx, 0, "  Current heap memory  %,14d K",              ap->bytesAllocated / 1024);
    mprLog(ctx, 0, "  Peak heap memory     %,14d K",              ap->peakAllocated / 1024);
    mprLog(ctx, 0, "  Peak stack size      %,14d K",              ap->peakStack / 1024);
    mprLog(ctx, 0, "  Allocation errors    %,14d",                ap->errors);
    
    /*
     *  Limits
     */
    mprLog(ctx, 0, "  Memory limit         %,14d MB (%d %%)",    ap->maxMemory / (1024 * 1024), 
           percent(ap->bytesAllocated, ap->maxMemory));
    mprLog(ctx, 0, "  Memory redline       %,14d MB (%d %%)",    ap->redLine / (1024 * 1024), 
           percent(ap->bytesAllocated, ap->redLine));

    /*
     *  Heaps
     */
    mprLog(ctx, 0, "\n  Heaps");
    mprLog(ctx, 0, "  -----");
    printMprHeaps(ctx);
#endif /* BLD_FEATURE_MEMORY_STATS */
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

/************************************************************************/
/*
 *  End of file "../mprAlloc.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../mprAsyncSelectWait.c"
 */
/************************************************************************/

/**
 *  mprAsyncSelectWait.c - Wait for I/O on Windows.
 *
 *  This module provides io management for sockets on Windows like systems. 
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



#if BLD_WIN_LIKE


static LRESULT msgProc(HWND hwnd, uint msg, uint wp, long lp);


int mprInitSelectWait(MprWaitService *ws)
{   
    return 0;
}


/*
 *  Wait for I/O on a single descriptor. Return the number of I/O events found. Mask is the events of interest.
 *  Timeout is in milliseconds.
 *  TODO - why is there an fd parameter when wp->fd exists?
 */
int mprWaitForSingleIO(MprWaitHandler *wp, int fd, int desiredMask, int timeout)
{
    MprWaitService  *ws;
    int             winMask;

    winMask = 0;
    if (desiredMask & MPR_READABLE) {
        winMask |= FD_CONNECT | FD_CLOSE | FD_READ;
    } 
    if (desiredMask & MPR_WRITEABLE) {
        winMask |= FD_WRITE;
    }

    ws = mprGetMpr(wp)->waitService;
    return WSAAsyncSelect(fd, ws->hwnd, ws->socketMessage, winMask);
}


/*
 *  Wait for I/O on all registered descriptors. Timeout is in milliseconds. Return the number of events serviced.
 */
int mprWaitForIO(MprWaitService *ws, int timeout)
{
    MSG             msg;
    int             count;

    count = 0;
    
#if UNUSED
    if (ws->flags & MPR_NEED_RECALL) {

        mprLock(ws->mutex);
        ws->flags &= ~MPR_NEED_RECALL;
        lastChange = ws->listGeneration;

        for (count = index = 0; (wp = (MprWaitHandler*) mprGetNextItem(ws->list, &index)) != 0; ) {
            if (wp->flags & MPR_WAIT_RECALL_HANDLER) {
                count++;

                mprUnlock(ws->mutex);
                mprInvokeWaitCallback(wp, 0);
                mprLock(ws->mutex);

                if (lastChange != ws->listGeneration) {
                    index = 0;
                }
            }
        }
        mprUnlock(ws->mutex);
    }
#endif

    mprAssert(ws->hwnd);
    SetTimer(ws->hwnd, 0, timeout, NULL);

    if (GetMessage(&msg, NULL, 0, 0) != 0) {
        TranslateMessage(&msg);

        DispatchMessage(&msg);
        count++;
    } else {
        mprTerminate(ws, 1);
    }
    
    return count;
}


void mprServiceWinIO(MprWaitService *ws, int sockFd, int winMask)
{
    MprWaitHandler      *wp;
    int                 index, mask;

    mprLock(ws->mutex);

    for (index = 0; (wp = (MprWaitHandler*) mprGetNextItem(ws->list, &index)) != 0; ) {
        if (wp->fd == sockFd) {
            break;
        }
    }
    if (wp == 0) {
        /*
         *  If the server forcibly closed the socket, we may still get a read event. Just ignore it.
         */
        mprUnlock(ws->mutex);
        return;
    }

    /*
     *  disableMask will be zero if we are already servicing an event
     */
    mask = wp->desiredMask & wp->disableMask;

    if (mask == 0) {
        /*
         *  Already have an event scheduled so we must not schedule another yet
         *  We should have disabled events, but a message may already be in the 
         *  message queue.
         */
        mprUnlock(ws->mutex);
        return;
    }

    /*
     *  Mask values: READ==1, WRITE=2, ACCEPT=8, CONNECT=10, CLOSE=20
     */
    wp->presentMask = 0;
    if (winMask & (FD_READ | FD_ACCEPT | FD_CLOSE)) {
        wp->presentMask |= MPR_READABLE;
    }
    if (winMask & (FD_WRITE | FD_CONNECT)) {
        wp->presentMask |= MPR_WRITEABLE;
    }

    if (wp->presentMask) {
#if BLD_FEATURE_MULTITHREAD
        wp->disableMask = 0;
        ws->maskGeneration++;
#endif
        mprUnlock(ws->mutex);
        mprInvokeWaitCallback(wp, 0);

    } else {
        mprUnlock(ws->mutex);
    }
}


#if BLD_FEATURE_MULTITHREAD
/*
 *  Awaken the wait service
 */
void mprAwakenWaitService(MprWaitService *ws)
{
    if (mprGetCurrentThread(ws) == ws->serviceThread) {
        return;
    }
    if (ws->hwnd) {
        int rc = PostMessage(ws->hwnd, WM_NULL, 0, 0L);
        rc = rc;
    }
}
#endif


/*
 *  Set a handler to be recalled without further I/O
 */
void mprRecallWaitHandler(MprWaitHandler *wp)
{
    MprWaitService  *ws;

    ws = wp->waitService;
    PostMessage(ws->hwnd, ws->socketMessage, wp->fd, FD_READ);
}


static void setWinInterest(MprWaitHandler *wp)
{
    int     rc, eligible, winMask;

    winMask = 0;
    eligible = wp->desiredMask & wp->disableMask;

    if (eligible & MPR_READABLE) {
        winMask |= FD_ACCEPT | FD_CONNECT | FD_CLOSE | FD_READ;
    }
    if (eligible & MPR_WRITEABLE) {
        winMask |= FD_WRITE;
    }
    rc = WSAAsyncSelect(wp->fd, wp->waitService->hwnd, wp->waitService->socketMessage, winMask);
    mprAssert(rc == 0);
}


/*
 *  Modify a select handlers interested events
 */
void mprSetWaitInterest(MprWaitHandler *wp, int mask)
{
    MprWaitService  *ws;

    ws = wp->waitService;
    mprLock(ws->mutex);
	wp->desiredMask = mask;
	setWinInterest(wp);
	mprModifyWaitHandler(ws, wp, 1);
    mprUnlock(ws->mutex);
}


void mprDisableWaitEvents(MprWaitHandler *wp, bool wakeUp)
{
    //  TODO OPT -- better if we locked just our select handler 

    mprLock(wp->waitService->mutex);
    wp->disableMask = 0;

    setWinInterest(wp);

    mprModifyWaitHandler(wp->waitService, wp, wakeUp);
    mprUnlock(wp->waitService->mutex);
}


void mprEnableWaitEvents(MprWaitHandler *wp, bool wakeUp)
{
    mprLock(wp->waitService->mutex);

    wp->disableMask = -1;

    setWinInterest(wp);

    mprModifyWaitHandler(wp->waitService, wp, wakeUp);
    mprUnlock(wp->waitService->mutex);
}


#if UNUSED
/*
 *  Set a handler to be recalled without further I/O
 */
void mprRecallWaitHandler(MprWaitHandler *wp)
{
    MprWaitService  *ws;

    ws = wp->waitService;

    /*
     *  No locking needed, order important
     */
    wp->flags |= MPR_WAIT_RECALL_HANDLER;
    ws->flags |= MPR_NEED_RECALL;

    mprAwakenWaitService(wp->waitService);
}
#endif


/*
 *  Create a default window if the application has not already created one.
 */ 
int mprInitWindow(MprWaitService *ws)
{
    Mpr         *mpr;
    WNDCLASS    wc;
    HWND        hwnd;
    int         rc;

    mpr = mprGetMpr(ws);

    wc.style            = CS_HREDRAW | CS_VREDRAW;
    wc.hbrBackground    = (HBRUSH) (COLOR_WINDOW+1);
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
    wc.hInstance        = 0;
    wc.hIcon            = NULL;
    wc.lpfnWndProc      = (WNDPROC) msgProc;

    wc.lpszMenuName     = wc.lpszClassName = mprGetAppName(mpr);

    rc = RegisterClass(&wc);
    if (rc == 0) {
        mprError(mpr, "Can't register windows class");
        return MPR_ERR_CANT_INITIALIZE;
    }

    hwnd = CreateWindow(mprGetAppName(mpr), mprGetAppTitle(mpr), WS_OVERLAPPED, CW_USEDEFAULT, 0, 0, 0, NULL, NULL, 0, NULL);

    if (!hwnd) {
        mprError(mpr, "Can't create window");
        return -1;
    }

    ws->hwnd = hwnd;
    ws->socketMessage = MPR_SOCKET_MESSAGE;

    return 0;
}


/*
 *  Windows message processing loop for wakeup and socket messages
 */
static LRESULT msgProc(HWND hwnd, uint msg, uint wp, long lp)
{
    Mpr                 *mpr;
    MprWaitService      *ws;
    int                 sock, winMask;

    mpr = mprGetMpr(0);
    ws = mpr->waitService;

    if (msg == WM_DESTROY || msg == WM_QUIT) {
        mprTerminate(mpr, 1);

    } else if (msg && msg == ws->socketMessage) {
        sock = wp;
        winMask = LOWORD(lp);
        mprServiceWinIO(mpr->waitService, sock, winMask);

    } else if (ws->msgCallback) {
        ws->msgCallback(hwnd, msg, wp, lp);

    } else {
        return DefWindowProc(hwnd, msg, wp, lp);
    }
    return 0;
}


void mprSetWinMsgCallback(MprWaitService *ws, MprMsgCallback callback)
{
    ws->msgCallback = callback;
}


#else
void __mprAsyncDummy() {}
#endif /* BLD_WIN_LIKE */

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
/************************************************************************/
/*
 *  End of file "../mprAsyncSelectWait.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../mprBrewFile.c"
 */
/************************************************************************/

/**
 *  mprBrewFile.c - Brew based File services
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



#if BREW

static void closeFile(MprFile *file);
static int  consoleWrite(MprFile *file, const void *buf, uint count);
static int  brewFileMode(int omode);
static void fileServiceDestructor(MprDiskFileService *fs)


static MprFile *openFile(MprCtx ctx, MprFileService *fileSystem, cchar *path, int omode, int perms)
{
    MprBrewFileService  *bfs;
    MprFile             *file;

    mprAssert(path && *path);

    bfs = (MprBrewFileSystem*) fileSystem;
    file = mprAllocObjZeroed(ctx, MprFile, closeFile);
    if (file == 0) {
        mprAssert(file);
        return 0;
    }
    file->mode = omode;

    if (omode & O_CREAT) {
        IFILEMGR_Remove(fileSystem->fileMgr, path);
    }
    file->fd = IFILEMGR_OpenFile(fileSystem->fileMgr, path, brewFileMode(omode));
    if (file->fd == 0) {
        /* int err = IFILEMGR_GetLastError(fileSystem->fileMgr); */
        return 0;
    }

    return file;
}



static void closeFile(MprFile *file)
{
    mprAssert(file);

    if (file == 0) {
        return;
    }
    mprFlush(file);
    IFILE_Release(file->fd);
}
 


static int readFile(MprFile *file, void *buf, uint size)
{
    mprAssert(file);
    mprAssert(buf);

    if (file == 0) {
        return MPR_ERR_BAD_HANDLE;
    }
    return IFILE_Read(file->fd, buf, size);
}



static int writeFile(MprFile *file, const void *buf, uint count)
{
    mprAssert(file);
    mprAssert(buf);

    if (file == 0) {
        return MPR_ERR_BAD_HANDLE;
    }

    /*
     *  Handle == 1 is for console writes
     */
    if (file->fd == (IFile*) 1) {
        return consoleWrite(file, buf, count);
    }

    return IFILE_Write(file->fd, buf, count);
}



static int seekFile(MprFile *file, int seekType, long distance)
{
    int     type;

    mprAssert(file);

    if (file == 0) {
        return MPR_ERR_BAD_HANDLE;
    }

    if (seekType == SEEK_SET) {
        type = _SEEK_START;
    } else if (seekType == SEEK_END) {
        type = _SEEK_END;
    } else {
        type = _SEEK_CURRENT;
    }

    return IFILE_Seek(file->fd, type, distance);
}



static bool accessFile(MprBrewFileService *fileSystem, cchar *path, int omode)
{
    return getFileInfo(fileSystem, path);
}



static int deleteFile(MprBrewFileService *fileSystem, cchar *path)
{
    if (IFILEMGR_Remove(fileSystem->fileMgr, path) == EFAILED) {
        return MPR_ERR_CANT_ACCESS;
    }
    return 0;
}



static int deleteDir(MprBrewFileService *fileSystem, cchar *path)
{
    if (IFILEMGR_RmDir(fileSystem->fileMgr, path) == EFAILED) {
        mprError(ctx, "Can't remove directory %s, error %d", path, IFILEMGR_GetLastError(fileSystem->fileMgr));
        return MPR_ERR_CANT_ACCESS;
    }
    return 0;
}
 


static int makeDir(MprBrewFileService *fileSystem, cchar *path, int perms)
{
    if (IFILEMGR_MkDir(fileSystem->fileMgr, path) == EFAILED) {
        mprError(ctx, "Can't make directory %s, error %d", path, IFILEMGR_GetLastError(fileSystem->fileMgr));
        return MPR_ERR_CANT_ACCESS;
    }
    return 0;
}
 


static int getFileInfo(MprBrewFileService *fileSystem, cchar *path, MprFileInfo *info)
{
    FileInfo        brewFileInfo;

    mprAssert(path && *path);
    mprAssert(info);

    info->valid = 0;
    if (IFILEMGR_GetInfo(fileSystem->fileMgr, path, &brewFileInfo) == EFAILED) {
        mprError(ctx, "Can't get file info for %s, error %d", path, 
            IFILEMGR_GetLastError(fileSystem->fileMgr));
        return -1;
    }

    info->size = brewFileInfo.dwSize;
    info->ctime = brewFileInfo.dwCreationDate;
    info->isDir = brewFileInfo.attrib & _FA_DIR;
    info->isReg = brewFileInfo.attrib & _FA_NORMAL;
    info->valid = 1;

    return 0;
}
 


static int consoleWrite(MprFile *file, const void *writeBuf, uint count)
{
    MprBuf  *bp;
    char    *start, *cp, *end, *np, *buf;
    int     total, bytes;

    mprAssert(file);
    mprAssert(writeBuf);

    /*
     *  Buffer output and flush on a '\n'. This is necesary because 
     *  BREW appends a new line to all calls to DBGPRINTF.
     */
    if (file->buf == 0) {
#if BREWSIM
        file->buf = mprCreateBuf(file, 128, 128);
#else
        file->buf = mprCreateBuf(file, 35, 35);
#endif
    }
    bp = file->buf;

    if (mprGetBufLength(bp) > 0 && mprGetBufSpace(bp) < (int) count) {
        printf(" MP: %s", mprGetBufStart(bp));
        mprFlushBuf(bp);
    }

    total = 0;
    buf = (char*) writeBuf;

    while (count > 0) {
        bytes = mprPutBlockToBuf(bp, buf, count);
        buf += bytes;
        count -= bytes;
        total += bytes;

        /*
         *  Output the line if we find a newline or the line is too long to 
         *  buffer (count > 0).
         */
        if (strchr(mprGetBufStart(bp), '\n') || count > 0) {
            end = cp = mprGetBufEnd(bp);
            start = cp = mprGetBufStart(bp);

            /*
             *  Brew can't handle tabs
             */
            for (; cp < end && *cp; cp++) {
                if (*cp == '\t') {
                    *cp = ' ';
                }
            }

            cp = start;
            for (np = cp; np < end; np++) {
                if (*np == '\n') {
                    *np = '\0';
                    /* BREW appends '\n' */
                    if (count > 0) {
                        printf("_MP: %s", cp);
                    } else {
                        printf(" MP: %s", cp);
                    }
                    cp = np + 1;
                }
            }
            if (cp < np) {
                if (cp == start) {
                    /* Nothing output. Line must be too long */
                    printf("_MP: %s", cp);
                    mprFlushBuf(bp);

                } else if (count > 0) {
                    /* We did output text, but there is more of this line */
                    mprAdjustBufStart(bp, (int) (cp - start));
                    mprCompactBuf(bp);

                } else {
                    printf(" MP: %s", cp);
                    mprFlushBuf(bp);
                }
            } else {
                mprFlushBuf(bp);
            }
        }
    }
    return total;
}



void mprSetFileMgr(MprCtx ctx, void *fileMgr)
{
    mprGetMpr(ctx)->fileService->fileMgr = fileMgr;
}



void *mprGetFileMgr(MprCtx ctx)
{
    return mprGetMpr(ctx)->fileService->fileMgr;
}



static int brewFileMode(int omode)
{
    uint        mode;

    mode = 0;

    if (omode & (O_RDONLY | O_RDWR)) {
        mode |= _OFM_READ;
    }
    if (omode & (O_RDWR)) {
        mode |= _OFM_READWRITE;
    }
    if (omode & (O_CREAT)) {
        mode |= _OFM_CREATE;
    }
    if (omode & (O_APPEND)) {
        mode |= _OFM_APPEND;
    }
    return mode;
}


int *mprSetBrewFileSystem(MprCtx ctx)
{
    MprBrewFileService  *bfs;

    /*
     *  We assume that STDOUT is 1 and STDERR is 2
     */
    bfs->console = mprAllocObjZeroed(bfs, MprFile);
    bfs->error = mprAllocObjZeroed(bfs, MprFile);
    bfs->console->fd = (IFile*) 1;
    bfs->error->fd = (IFile*) 2;

    mprAssert(bfs->fileMgr);
    if (ISHELL_CreateInstance(mprGetMpr(bfs)->shell, AEECLSID_FILEMGR, (void**) &bfs->fileMgr) != SUCCESS) {
        mprError(fs, "Can't open file manager.");
        return MPR_ERR_CANT_OPEN;
    }

    return fs;
}


MprBrewFileService *mprCreateBrewFileService(MprCtx ctx)
{
    MprBrewFileService  *bfs;

    bfs = mprAllocObjZeroed(ctx, MprBrewFileService, fileServiceDestructor);
    if (bfs == 0) {
        return 0;
    }

    bfs->accessFile = accessFile;
    bfs->deleteFile = deleteFile;
    bfs->deleteDir = deleteDir;
    bfs->getFileInfo = getFileInfo;
    bfs->makeDir = makeDir;
    bfs->openFile = openFile;
    bfs->closeFile = closeFile;
    bfs->readFile = readFile;
    bfs->seekFile = seekFile;
    bfs->writeFile = writeFile;

    return bfs;
}



static void fileServiceDestructor(MprDiskFileService *fs)
{
    IFILEMGR_Release(fs->fileMgr);
}


#else
void __dummyBrewFileService() {}
#endif /* !BREW */

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
/************************************************************************/
/*
 *  End of file "../mprBrewFile.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../mprBuf.c"
 */
/************************************************************************/

/**
 *  mprBuf.c - Dynamic buffer module
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

//  TODO - fix uchar* should all be char*


/*
 *  Create a new buffer. "maxsize" is the limit to which the buffer can ever grow. -1 means no limit. "initialSize" is 
 *  used to define the amount to increase the size of the buffer each time if it becomes full. (Note: mprGrowBuf() will 
 *  exponentially increase this number for performance.)
 */
MprBuf *mprCreateBuf(MprCtx ctx, int initialSize, int maxSize)
{
    MprBuf      *bp;
    
    if (initialSize <= 0) {
        initialSize = MPR_DEFAULT_ALLOC;
    }
    bp = mprAllocObjZeroed(ctx, MprBuf);
    bp->growBy = MPR_BUFSIZE;
    mprSetBufSize(bp, initialSize, maxSize);
    return bp;
}


/*
 *  Set the current buffer size and maximum size limit.
 */
int mprSetBufSize(MprBuf *bp, int initialSize, int maxSize)
{
    mprAssert(bp);

    if (initialSize <= 0) {
        if (maxSize > 0) {
            bp->maxsize = maxSize;
        }
        return 0;
    }

    if (maxSize > 0 && initialSize > maxSize) {
        initialSize = maxSize;
    }

    mprAssert(initialSize > 0);

    if (bp->data) {
        /*
         *  Buffer already exists
         */
        if (bp->buflen < initialSize) {
            if (mprGrowBuf(bp, initialSize - bp->buflen) < 0) {
                return MPR_ERR_NO_MEMORY;
            }
        }
        bp->maxsize = maxSize;
        return 0;
    }

    /*
     *  New buffer - create storage for the data
     */
    bp->data = (uchar*) mprAlloc(bp, initialSize);
    bp->growBy = initialSize;
    bp->maxsize = maxSize;
    bp->buflen = initialSize;
    bp->endbuf = &bp->data[bp->buflen];
    bp->start = bp->data;
    bp->end = bp->data;
    *bp->start = '\0';

    return 0;
}


void mprSetBufMax(MprBuf *bp, int max)
{
    bp->maxsize = max;
}


char *mprStealBuf(MprCtx ctx, MprBuf *bp)
{
    char    *str;

    str = (char*) bp->start;

    mprStealBlock(ctx, bp->start);

    bp->start = bp->end = bp->data = bp->endbuf = 0;
    bp->buflen = 0;

    return str;
}


/*
 *  This appends a silent null. It does not count as one of the actual bytes in the buffer
 */
void mprAddNullToBuf(MprBuf *bp)
{
    int     space;

    space = bp->buflen - mprGetBufLength(bp);
    if (space < (int) sizeof(char)) {
        if (mprGrowBuf(bp, 1) < 0) {
            return;
        }
    }

    mprAssert(bp->end < bp->endbuf);
    if (bp->end < bp->endbuf) {
        *((char*) bp->end) = (char) '\0';
    }
}


void mprAdjustBufEnd(MprBuf *bp, int size)
{
    mprAssert(bp->buflen == (bp->endbuf - bp->data));
    mprAssert(size <= bp->buflen);

    bp->end += size;
    if (bp->end > bp->endbuf) {
        mprAssert(bp->end <= bp->endbuf);
        bp->end = bp->endbuf;
    }
}


/*
 *  Adjust the start pointer after a user copy
 */
void mprAdjustBufStart(MprBuf *bp, int size)
{
    mprAssert(bp->buflen == (bp->endbuf - bp->data));
    mprAssert(size <= bp->buflen);

    bp->start += size;
    if (bp->start > bp->end) {
        mprAssert(bp->start <= bp->end);
        bp->start = bp->end;
    }
}


void mprFlushBuf(MprBuf *bp)
{
    bp->start = bp->data;
    bp->end = bp->data;
}


int mprGetCharFromBuf(MprBuf *bp)
{
    if (bp->start == bp->end) {
        return -1;
    }
    return (uchar) *bp->start++;
}


int mprGetBlockFromBuf(MprBuf *bp, uchar *buf, int size)
{
    int     thisLen, bytesRead;

    mprAssert(buf);
    mprAssert(size > 0);
    mprAssert(bp->buflen == (bp->endbuf - bp->data));

    /*
     *  Get the max bytes in a straight copy
     */
    bytesRead = 0;
    while (size > 0) {
        thisLen = mprGetBufLength(bp);
        thisLen = min(thisLen, size);
        if (thisLen <= 0) {
            break;
        }

        memcpy(buf, bp->start, thisLen);
        buf += thisLen;
        bp->start += thisLen;
        size -= thisLen;
        bytesRead += thisLen;
    }
    return bytesRead;
}


int mprGetBufLength(MprBuf *bp)
{
    return (int) (bp->end - bp->start);
}


int mprGetBufSize(MprBuf *bp)
{
    return bp->buflen;
}


int mprGetBufSpace(MprBuf *bp)
{
    return (int) (bp->endbuf - bp->end);
}


char *mprGetBufOrigin(MprBuf *bp)
{
    return (char*) bp->data;
}


char *mprGetBufStart(MprBuf *bp)
{
    return (char*) bp->start;
}


char *mprGetBufEnd(MprBuf *bp)
{
    return (char*) bp->end;
}


int mprInsertCharToBuf(MprBuf *bp, int c)
{
    if (bp->start == bp->data) {
        return MPR_ERR_BAD_STATE;
    }
    *--bp->start = c;
    return 0;
}


int mprLookAtNextCharInBuf(MprBuf *bp)
{
    if (bp->start == bp->end) {
        return -1;
    }
    return *bp->start;
}


int mprLookAtLastCharInBuf(MprBuf *bp)
{
    if (bp->start == bp->end) {
        return -1;
    }
    return bp->end[-1];
}


int mprPutCharToBuf(MprBuf *bp, int c)
{
    char    *cp;
    int     space;

    mprAssert(bp->buflen == (bp->endbuf - bp->data));

    space = bp->buflen - mprGetBufLength(bp);
    if (space < (int) sizeof(char)) {
        if (mprGrowBuf(bp, 1) < 0) {
            return -1;
        }
    }

    cp = (char*) bp->end;
    *cp++ = (char) c;
    bp->end = (uchar *) cp;

    if (bp->end < bp->endbuf) {
        *((char*) bp->end) = (char) '\0';
    }

    return 1;
}


int mprPutBlockToBuf(MprBuf *bp, cchar *str, int size)
{
    int     thisLen, bytes, space;

    mprAssert(str);
    mprAssert(size >= 0);

    /*
     *  Add the max we can in one copy
     */
    bytes = 0;
    while (size > 0) {
        space = mprGetBufSpace(bp);
        thisLen = min(space, size);
        if (thisLen <= 0) {
            if (mprGrowBuf(bp, size) < 0) {
                break;
            }
            space = mprGetBufSpace(bp);
            thisLen = min(space, size);
        }

        memcpy(bp->end, str, thisLen);
        str += thisLen;
        bp->end += thisLen;
        size -= thisLen;
        bytes += thisLen;
    }
    if (bp->end < bp->endbuf) {
        *((char*) bp->end) = (char) '\0';
    }
    return bytes;
}


int mprPutStringToBuf(MprBuf *bp, cchar *str)
{
    return mprPutBlockToBuf(bp, str, (int) strlen(str));
}


int mprPutFmtToBuf(MprBuf *bp, cchar *fmt, ...)
{
    va_list     ap;
    char        *buf;
    int         rc, len, space;

    va_start(ap, fmt);
    space = mprGetBufSpace(bp);

    /*
     *  Add max that the buffer can grow 
     */
    space += (bp->maxsize - bp->buflen);

    len = mprAllocVsprintf(bp, &buf, space, fmt, ap);
    rc = mprPutBlockToBuf(bp, buf, len);

    mprFree(buf);
    va_end(ap);
    return rc;
}


/*
 *  Grow the buffer. Return 0 if the buffer grows. Increase by the growBy size specified when creating the buffer. 
 */
int mprGrowBuf(MprBuf *bp, int need)
{
    uchar   *newbuf;
    int     growBy;

    if (bp->maxsize > 0 && bp->buflen >= bp->maxsize) {
        return MPR_ERR_TOO_MANY;
    }

    if (bp->start > bp->end) {
        mprCompactBuf(bp);
    }

    if (need > 0) {
        growBy = max(bp->growBy, need);
    } else {
        growBy = bp->growBy;
    }

    newbuf = (uchar*) mprAlloc(bp, bp->buflen + growBy);
    if (bp->data) {
        memcpy(newbuf, bp->data, bp->buflen);
        mprFree(bp->data);
    }

    bp->buflen += growBy;
    bp->end = newbuf + (bp->end - bp->data);
    bp->start = newbuf + (bp->start - bp->data);
    bp->data = newbuf;
    bp->endbuf = &bp->data[bp->buflen];

    /*
     *  Increase growBy to reduce overhead
     */
    if (bp->maxsize > 0) {
        if ((bp->buflen + (bp->growBy * 2)) > bp->maxsize) {
            bp->growBy = min(bp->maxsize - bp->buflen, bp->growBy * 2);
        }
    }
    return 0;
}


/*
 *  Add a number to the buffer (always null terminated).
 */
int mprPutIntToBuf(MprBuf *bp, int i)
{
    char    numBuf[16];
    int     rc;

    mprItoa(numBuf, sizeof(numBuf), i, 10);
    rc = mprPutStringToBuf(bp, numBuf);

    if (bp->end < bp->endbuf) {
        *((char*) bp->end) = (char) '\0';
    }

    return rc;
}


void mprCompactBuf(MprBuf *bp)
{
    if (mprGetBufLength(bp) == 0) {
        mprFlushBuf(bp);
        return;
    }
    if (bp->start > bp->data) {
        memmove(bp->data, bp->start, (bp->end - bp->start));
        bp->end -= (bp->start - bp->data);
        bp->start = bp->data;
    }
}


MprBufProc mprGetBufRefillProc(MprBuf *bp) 
{
    return bp->refillProc;
}


void mprSetBufRefillProc(MprBuf *bp, MprBufProc fn, void *arg)
{ 
    bp->refillProc = fn; 
    bp->refillArg = arg; 
}


int mprRefillBuf(MprBuf *bp) 
{ 
    return (bp->refillProc) ? (bp->refillProc)(bp, bp->refillArg) : 0; 
}


void mprResetBufIfEmpty(MprBuf *bp)
{
    if (mprGetBufLength(bp) == 0) {
        mprFlushBuf(bp);
    }
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
/************************************************************************/
/*
 *  End of file "../mprBuf.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../mprCmd.c"
 */
/************************************************************************/

/* 
 *  mprCmd.c - Run external commands
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



#if BLD_FEATURE_CMD

static void cmdDestructor(MprCmd *cmd);
static int  makePipe(MprCmd *cmd, int index);
static void resetCmd(MprCmd *cmd);
static int  startProcess(MprCmd *cmd);
static void stdinCallback(MprCmd *cmd, int mask, int isPoolThread);
static void stdoutCallback(MprCmd *cmd, int mask, int isPoolThread);
static void stderrCallback(MprCmd *cmd, int mask, int isPoolThread);

#if BLD_WIN_LIKE
static void sanitizeArgs(MprCmd *cmd, int argc, char **argv, char **env);
#endif

#if VXWORKS
typedef int (*MprCmdTaskFn)(int argc, char **argv);
static void newTaskWrapper(char *program, MprCmdTaskFn *entry, int cmdArg);
#endif

/*
 *  Create a new command object
 */
MprCmd *mprCreateCmd(MprCtx ctx)
{
    MprCmd          *cmd;
    MprCmdFile      *files;
    int             i;
    
    cmd = mprAllocObjWithDestructorZeroed(ctx, MprCmd, cmdDestructor);
    if (cmd == 0) {
        return 0;
    }
    
    files = cmd->files;
    for (i = 0; i < MPR_CMD_MAX_PIPE; i++) {
        files[i].clientFd = -1;
        files[i].fd = -1;
    }

#if BLD_FEATURE_MULTITHREAD
    cmd->mutex = mprCreateLock(cmd);
#endif
#if VXWORKS
    cmd->startCond = semCCreate(SEM_Q_PRIORITY, SEM_EMPTY);
    cmd->exitCond = semCCreate(SEM_Q_PRIORITY, SEM_EMPTY);
#endif

    return cmd;
}


static void resetCmd(MprCmd *cmd)
{
    MprCmdFile      *files;
    int             i;

    files = cmd->files;
    for (i = 0; i < MPR_CMD_MAX_PIPE; i++) {
        if (cmd->handlers[i]) {
            mprFree(cmd->handlers[i]);
            cmd->handlers[i] = 0;
        }
        if (files[i].clientFd >= 0) {
            close(files[i].clientFd);
            files[i].clientFd = -1;
        }
        if (files[i].fd >= 0) {
            close(files[i].fd);
            files[i].fd = -1;
        }
    }
    cmd->eofCount = 0;
    cmd->completed = 0;
    cmd->status = -1;
    
    mprStopCmd(cmd);
}


static void cmdDestructor(MprCmd *cmd)
{
    resetCmd(cmd);
}


/*
 *  Callback routine that is run whenever there is I/O to read/write to talk to the CGI gateway.
 */
static void runCmdCallback(MprCmd *cmd, int fd, int channel, void *data)
{
    MprBuf  *buf;
    int     len, space;

    buf = 0;

    switch (channel) {
    case MPR_CMD_STDIN:
        mprAssert(0);
        return;
    case MPR_CMD_STDOUT:
        buf = cmd->stdoutBuf;
        break;
    case MPR_CMD_STDERR:
        buf = cmd->stderrBuf;
        break;
    }

    /*
     *  Read and aggregate the result into a single string
     */
    space = mprGetBufSpace(buf);
    if (space < (MPR_BUFSIZE / 4)) {
        if (mprGrowBuf(buf, MPR_BUFSIZE) < 0) {
            mprAssert(0);
            mprCloseCmdFd(cmd, channel);
            return;
        }
        space = mprGetBufSpace(buf);
    }

    len = mprReadCmdPipe(cmd, channel, mprGetBufEnd(buf), space);
    if (len <= 0) {
        if (len == 0 || (len < 0 && !(errno == EAGAIN || EWOULDBLOCK))) {
            mprCloseCmdFd(cmd, channel);
        }

    } else {
        mprAdjustBufEnd(buf, len);
    }
}


/*
 *  Run a simple blocking command.
 */
int mprRunCmd(MprCmd *cmd, cchar *command, char **out, char **err, int flags)
{
    char    **argv;
    int     argc;

    if (mprMakeArgv(cmd, NULL, command, &argc, &argv) < 0 || argv == 0) {
        return 0;
    }
    return mprRunCmdV(cmd, argc, argv, out, err, flags);
}


int mprRunCmdV(MprCmd *cmd, int argc, char **argv, char **out, char **err, int flags)
{
    int     rc, status;

    if (err) {
        *err = 0;
        flags |= MPR_CMD_ERR;
    }
    if (out) {
        *out = 0;
        flags |= MPR_CMD_OUT;
    }

    if (flags & MPR_CMD_OUT) {
        mprFree(cmd->stdoutBuf);
        cmd->stdoutBuf = mprCreateBuf(cmd, MPR_BUFSIZE, -1);
    }
    if (flags & MPR_CMD_ERR) {
        mprFree(cmd->stderrBuf);
        cmd->stderrBuf = mprCreateBuf(cmd, MPR_BUFSIZE, -1);
    }
    mprSetCmdCallback(cmd, runCmdCallback, NULL);
    
    rc = mprStartCmd(cmd, argc, argv, NULL, flags);
    mprCloseCmdFd(cmd, MPR_CMD_STDIN);

    if (rc < 0) {
        if (err) {
            if (rc == MPR_ERR_CANT_ACCESS) {
                mprAllocSprintf(cmd, err, -1, "Can't access command %s", cmd->program);
            } else if (MPR_ERR_CANT_OPEN) {
                mprAllocSprintf(cmd, err, -1, "Can't open standard I/O for command %s", cmd->program);
            } else if (rc == MPR_ERR_CANT_CREATE) {
                mprAllocSprintf(cmd, err, -1, "Can't create process for %s", cmd->program);
            }
        }
        return rc;
    }

    if (cmd->flags & MPR_CMD_DETACHED) {
        return 0;
    }
    if (mprWaitForCmd(cmd, -1) < 0) {
        return MPR_ERR_NOT_READY;
    }
    if (mprGetCmdExitStatus(cmd, &status) < 0) {
        return MPR_ERR;
    }
    if (err && flags & MPR_CMD_ERR) {
        mprAddNullToBuf(cmd->stderrBuf);
        *err = mprGetBufStart(cmd->stderrBuf);
    }
    if (out && flags & MPR_CMD_OUT) {
        mprAddNullToBuf(cmd->stdoutBuf);
        *out = mprGetBufStart(cmd->stdoutBuf);
    }
    return status;
}


/*
 *  Start the command to run (stdIn and stdOut are named from the client's perspective). This is the lower-level way to 
 *  run a command. The caller needs to do code like mprRunCmd() themselves to wait for completion and to send/receive data.
 */
int mprStartCmd(MprCmd *cmd, int argc, char **argv, char **envp, int flags)
{
    char    path[MPR_MAX_FNAME], *program;
    int     i;

    mprAssert(argv);
    mprAssert(argc > 0);

    if (argc <= 0 || argv == NULL || argv[0] == NULL) {
        return MPR_ERR_BAD_STATE;
    }
    program = argv[0];

    path[0] = '\0';
    resetCmd(cmd);

    cmd->program = program;
    cmd->argv = argv;
    cmd->env = envp;
    cmd->flags = flags;

    for (i = 0; i < argc; i++) {
        mprLog(cmd, 6, "cmd: arg[%d]: %s", i, argv[i]);
    }
    cmd->argc = argc;

    if (cmd->env) {
        for (i = 0; cmd->env[i]; i++) {
            mprLog(cmd, 6, "cmd: env[%d]: %s", i, cmd->env[i]);
        }
    }

    if (access(program, X_OK) < 0) {
#if WIN
        mprSprintf(path, sizeof(path), "%s.exe", program);
        if (access(path, X_OK) == 0) {
            program = path;
        } else
#endif
        {
            mprLog(cmd, 5, "cmd: can't access %s, errno %d", program, mprGetOsError());
            return MPR_ERR_CANT_ACCESS;
        }
    }

#if WIN
    sanitizeArgs(cmd, argc, argv, envp);
#endif

    if (mprMakeCmdIO(cmd) < 0) {
        return MPR_ERR_CANT_OPEN;
    }

    cmd->requiredEof = 0;
    if (cmd->flags & MPR_CMD_OUT) {
        cmd->requiredEof++;
    }
    if (cmd->flags & MPR_CMD_ERR) {
        cmd->requiredEof++;
    }

#if BLD_UNIX_LIKE || VXWORKS
    {
        int     stdinFd, stdoutFd, stderrFd, nonBlock;
      
        stdinFd = cmd->files[MPR_CMD_STDIN].fd; 
        stdoutFd = cmd->files[MPR_CMD_STDOUT].fd; 
        stderrFd = cmd->files[MPR_CMD_STDERR].fd; 
        nonBlock = 1;

        /*
         *  Put the stdout and stderr into non-blocking mode. Windows can't do this because both ends of the pipe
         *  share the same blocking mode (Ugh!).
         */
#if VXWORKS
        if (stdoutFd >= 0) {
            ioctl(stdoutFd, FIONBIO, (int) &nonBlock);
        }
        if (stderrFd >= 0) {
            ioctl(stderrFd, FIONBIO, (int) &nonBlock);
        }
#else
        if (stdoutFd >= 0) {
            fcntl(stdoutFd, F_SETFL, fcntl(stdoutFd, F_GETFL) | O_NONBLOCK);
        }
        if (stderrFd >= 0) {
            fcntl(stderrFd, F_SETFL, fcntl(stderrFd, F_GETFL) | O_NONBLOCK);
        }
#endif
        if (stderrFd >= 0) {
            cmd->handlers[MPR_CMD_STDIN] = mprCreateWaitHandler(cmd, stderrFd, MPR_READABLE, 
                (MprWaitProc) stdinCallback, cmd, MPR_NORMAL_PRIORITY, MPR_WAIT_THREAD);
        }
        if (stdoutFd >= 0) {
            cmd->handlers[MPR_CMD_STDOUT] = mprCreateWaitHandler(cmd, stdoutFd, MPR_READABLE, 
                (MprWaitProc) stdoutCallback, cmd, MPR_NORMAL_PRIORITY, MPR_WAIT_THREAD);
        }
        if (stderrFd >= 0) {
            cmd->handlers[MPR_CMD_STDERR] = mprCreateWaitHandler(cmd, stderrFd, MPR_READABLE, 
                (MprWaitProc) stderrCallback, cmd, MPR_NORMAL_PRIORITY, MPR_WAIT_THREAD);
        }
    }
#endif
    return startProcess(cmd);
}


int mprMakeCmdIO(MprCmd *cmd)
{
    MprCmdFile  *files;
    int         rc;

    files = cmd->files;

    rc = 0;
    if (cmd->flags & MPR_CMD_IN) {
        rc += makePipe(cmd, MPR_CMD_STDIN);
    }
    if (cmd->flags & MPR_CMD_OUT) {
        rc += makePipe(cmd, MPR_CMD_STDOUT);
    }
    if (cmd->flags & MPR_CMD_ERR) {
        rc += makePipe(cmd, MPR_CMD_STDERR);
    }
    return rc;
}


void mprCloseCmdFd(MprCmd *cmd, int channel)
{
    mprAssert(0 <= channel && channel <= MPR_CMD_MAX_PIPE);

    /*
     *  Must free handler first to prevent select/poll errors
     */
    mprFree(cmd->handlers[channel]);
    cmd->handlers[channel] = 0;

    if (cmd->files[channel].fd != -1) {
        close(cmd->files[channel].fd);
        cmd->files[channel].fd = -1;
#if BLD_WIN_LIKE
        cmd->files[channel].handle = 0;
#endif
        if (channel != MPR_CMD_STDIN) {
            if (++cmd->eofCount >= cmd->requiredEof) {
                cmd->completed = 1;
            }
        }
    }
}


/*
 *  Stop the command
 */
void mprStopCmd(MprCmd *cmd)
{
    mprLog(cmd, 7, "cmd: stop");

    if (cmd->process) {
#if BLD_WIN_LIKE
        TerminateProcess((HANDLE) cmd->process, 2);
#elif VXWORKS
        taskDelete(cmd->process);
#else
        kill(cmd->process, SIGTERM);
#endif
        cmd->process = 0;
    }
}


/*
 *  Non-blocking read from a pipe. For windows which doesn't seem to have non-blocking pipes!
 */
int mprReadCmdPipe(MprCmd *cmd, int channel, char *buf, int bufsize)
{
#if BLD_WIN_LIKE
    int     count, rc;

    rc = PeekNamedPipe(cmd->files[channel].handle, NULL, 0, NULL, &count, NULL);
    if (rc && count > 0) {
        return read(cmd->files[channel].fd, buf, bufsize);
    }
    if (cmd->completed) {
        return 0;
    }
    /*
     *  No waiting. Use this just to check if the process has exited and thus EOF on the pipe.
     */
    if (WaitForSingleObject((HANDLE) cmd->process, 0) == WAIT_OBJECT_0) {
        return 0;
    }
    errno = EAGAIN;
    return -1;
#else
    /*
     *  File is already in non-blocking mode
     */
    return read(cmd->files[channel].fd, buf, bufsize);
#endif
}


/*
 *  Non-blocking read from a pipe. For windows which doesn't seem to have non-blocking pipes!
 */
int mprWriteCmdPipe(MprCmd *cmd, int channel, char *buf, int bufsize)
{
#if BLD_WIN_LIKE
    /*
     *  No waiting. Use this just to check if the process has exited and thus EOF on the pipe.
     */
    if (WaitForSingleObject((HANDLE) cmd->process, 0) == WAIT_OBJECT_0) {
        return -1;
    }
#endif

    /*
     *  Non-windows, this is a non-blocking write.
     *  There really isn't a good way to not block on windows. You can't use
     *  PeekNamedPipe because it will hang if the gateway is blocked reading it.
     *  You can't use NtQueryInformationFile on Windows SDK 6.0+. You also can't
     *  put the socket into non-blocking mode because Windows pipes share the
     *  blocking mode for both ends. So we block on Windows.
     */
    return write(cmd->files[channel].fd, buf, bufsize);
}


void mprEnableCmdEvents(MprCmd *cmd, int channel)
{
#if BLD_UNIX_LIKE || VXWORKS
    if (cmd->handlers[channel]) {
        mprEnableWaitEvents(cmd->handlers[channel], 1);
    }
#endif
}


void mprDisableCmdEvents(MprCmd *cmd, int channel)
{
#if BLD_UNIX_LIKE || VXWORKS
    if (cmd->handlers[channel]) {
        mprDisableWaitEvents(cmd->handlers[channel], 1);
    }
#endif
}


#if BLD_WIN_LIKE
/*
 *  Check for I/O and return a count of characters that can be read without blocking. If the proces has completed,
 *  then return 1 to indicate that EOF can be read.
 */
static int checkIO(MprCmd *cmd, int channel)
{
    int     rc, count, status;

    if (cmd->files[channel].handle) {
        rc = PeekNamedPipe(cmd->files[channel].handle, NULL, 0, NULL, &count, NULL);
        if (rc && count > 0) {
            return count;
        }
    }
    if (cmd->completed) {
        return 1;
    }
    if ((status = WaitForSingleObject((HANDLE) cmd->process, 10)) == WAIT_OBJECT_0) {
        if (cmd->requiredEof == 0) {
            cmd->completed = 1;
            return 0;
        }
        return 1;
    }
    return 0;
}
#endif


void mprPollCmd(MprCmd *cmd)
{
#if BLD_WIN_LIKE
    if (!cmd->completed) {
        if (cmd->files[MPR_CMD_STDOUT].handle) {
            if (checkIO(cmd, MPR_CMD_STDOUT) > 0 && (cmd->flags & MPR_CMD_OUT)) {
                stdoutCallback(cmd, MPR_READABLE, 0);
                /* WARNING - the command may be deleted here */
            }
        } else if (cmd->files[MPR_CMD_STDERR].handle) {
            if (checkIO(cmd, MPR_CMD_STDERR) > 0 && (cmd->flags & MPR_CMD_ERR)) {
                stderrCallback(cmd, MPR_READABLE, 0);
                /* WARNING - the command may be deleted here */
            }
        }
    }
#else
    if (cmd->requiredEof == 0) {
        if (mprReapCmd(cmd, 10) == 0) {
            cmd->completed = 1;
            return;
        }
    }
    mprServiceEvents(cmd, 10, MPR_SERVICE_ONE_THING);
#endif
}


/*
 *  Collect the child's exit status on some O/Ss. Return zero if the exit status is successfully reaped.
 *  Return -1 if an error and return > 0 if process still running.
 */
int mprReapCmd(MprCmd *cmd, int timeout)
{
    MprTime     mark;
    int         rc, status;

    rc = 0;

    if (timeout < 0) {
        timeout = MAXINT;
    }
    mark = mprGetTime(cmd);

    while (cmd->process && mprGetElapsedTime(cmd, mark) < timeout) {
#if BLD_UNIX_LIKE
        if ((rc = waitpid(cmd->process, &status, WNOHANG | __WALL)) < 0) {
            mprLog(cmd, 0, "waitpid failed for pid %d, errno %d", cmd->process, errno);
            mprAssert(0);
            return -1;

        } else if (rc == cmd->process && !WIFSTOPPED(status)) {
            cmd->status = WEXITSTATUS(status);
            cmd->process = 0;
            break;
        }
#endif
#if VXWORKS
        //  TODO - missing exit status
        status = 0;
        cmd->status = status;
        if (semTake(cmd->exitCond, MPR_TIMEOUT_STOP_TASK) != OK) {
            mprError("cmd: mprReapCmd: child %s did not exit, errno %d", cmd->program);
            return MPR_ERR_CANT_CREATE;
        }
        cmd->process = 0;
#endif
#if BLD_WIN_LIKE
        if ((rc = WaitForSingleObject((HANDLE) cmd->process, 10)) != WAIT_OBJECT_0) {
            if (rc == WAIT_TIMEOUT) {
                return 1;
            }
            mprLog(cmd, 6, "cmd: mprReapCmd: WaitForSingleObject no child to reap rc %d, %d", rc, GetLastError());
            return -1;
        }
        if (GetExitCodeProcess((HANDLE) cmd->process, (ulong*) &status) == 0) {
            mprLog(cmd, 7, "cmd: mprReapCmd: GetExitProcess error");
            mprAssert(0);
            return -1;
        }
        if (status != STILL_ACTIVE) {
            cmd->status = status;
            CloseHandle((HANDLE) cmd->process);
            CloseHandle(cmd->thread);
            cmd->process = 0;
            break;
        }
        /* Prevent busy waiting */
        mprSleep(cmd, 10);
#endif
    }
    return cmd->process != 0 ? 1: 0;
}


int mprWaitForCmd(MprCmd *cmd, int timeout)
{
    MprTime     mark;

    if (timeout < 0) {
        timeout = MAXINT;
    }
    mark = mprGetTime(cmd);

    while (cmd->process && mprGetElapsedTime(cmd, mark) < timeout) {
        if (cmd->completed) {
            if (mprReapCmd(cmd, 10) == 0) {
                break;
            }
        }
        mprPollCmd(cmd);
    }
    if (!cmd->completed || cmd->process) {
        mprLog(cmd, 7, "cmd: mprWaitForCmd: timeout waiting to collect exit status");
        return MPR_ERR_TIMEOUT;
    }
    mprLog(cmd, 7, "cmd: waitForChild: status %d", cmd->status);
    return 0;
}


static void stdoutCallback(MprCmd *cmd, int mask, int isPoolThread)
{
    if (cmd->files[MPR_CMD_STDOUT].fd > 0) {
        (*cmd->callback)(cmd, cmd->files[MPR_CMD_STDOUT].fd, MPR_CMD_STDOUT, cmd->callbackData);
    }
}


static void stdinCallback(MprCmd *cmd, int mask, int isPoolThread)
{
    if (cmd->files[MPR_CMD_STDIN].fd > 0) {
        cmd->callback(cmd, cmd->files[MPR_CMD_STDIN].fd, MPR_CMD_STDIN, cmd->callbackData);
    }
}


static void stderrCallback(MprCmd *cmd, int mask, int isPoolThread)
{
    if (cmd->files[MPR_CMD_STDERR].fd > 0) {
        cmd->callback(cmd, cmd->files[MPR_CMD_STDERR].fd, MPR_CMD_STDERR, cmd->callbackData);
    }
}


void mprSetCmdCallback(MprCmd *cmd, MprCmdProc proc, void *data)
{
    cmd->callback = proc;
    cmd->callbackData = data;
}


int mprGetCmdExitStatus(MprCmd *cmd, int *statusp)
{
    mprAssert(statusp);

    if (!cmd->completed) {
        return MPR_ERR_NOT_READY;
    }
    *statusp = cmd->status;
    return 0;
}


bool mprIsCmdRunning(MprCmd *cmd)
{
    return cmd->process > 0;
}


int mprGetCmdFd(MprCmd *cmd, int channel) 
{ 
    return cmd->files[channel].fd; 
}


MprBuf *mprGetCmdBuf(MprCmd *cmd, int channel)
{
    return (channel == MPR_CMD_STDOUT) ? cmd->stdoutBuf : cmd->stderrBuf;
}


void mprSetCmdDir(MprCmd *cmd, cchar *dir)
{
    mprAssert(dir && *dir);

    mprFree(cmd->dir);
    cmd->dir = mprStrdup(cmd, dir);
}


#if BLD_WIN_LIKE
/*
 *  Sanitize args. Convert "/" to "\" and converting '\r' and '\n' to spaces, quote all args and put the program as argv[0].
 */
static void sanitizeArgs(MprCmd *cmd, int argc, char **argv, char **env)
{
    char    *program, *systemRoot, **ep, **ap, *destp, *cp, *key, *progBuf, *localArgv[2], *saveArg0;
    int     len;

    mprAssert(argc > 0 && argv[0] != NULL);

    program = argv[0];
    progBuf = mprAlloc(cmd, (int) strlen(program) * 2 + 1);
    strcpy(progBuf, program);
    program = progBuf;

    for (cp = program; *cp; cp++) {
        if (*cp == '/') {
            *cp = '\\';
        } else if (*cp == '\r' || *cp == '\n') {
            *cp = ' ';
        }
    }
    if (*program == '"') {
        if ((cp = strrchr(++program, '"')) != 0) {
            *cp = '\0';
        }
    }

    if (argv == 0) {
        argv = localArgv;
        argv[1] = 0;
        saveArg0 = program;
    } else {
        saveArg0 = argv[0];
    }
    /*
     *  Set argv[0] to the program name while creating the command line. Restore later
     */
    argv[0] = program;

    /*
     *  Determine the command line length and arg count
     */
    argc = 0;
    for (len = 0, ap = argv; *ap; ap++) {
        len += (int) strlen(*ap) + 1 + 2;         /* Space and possible quotes */
        argc++;
    }
    cmd->command = (char*) mprAlloc(cmd, len + 1);
    cmd->command[len] = '\0';
    
    /*
     *  Add quotes to all args that have spaces in them including "program"
     */
    destp = cmd->command;
    for (ap = &argv[0]; *ap; ) {
        cp = *ap;
        if ((strchr(cp, ' ') != 0) && cp[0] != '\"') {
            *destp++ = '\"';
            strcpy(destp, cp);
            destp += strlen(cp);
            *destp++ = '\"';
        } else {
            strcpy(destp, cp);
            destp += strlen(cp);
        }
        if (*++ap) {
            *destp++ = ' ';
        }
    }
    *destp = '\0';
    argv[0] = saveArg0;

    /*
     *  Now work on the environment
     */
    cmd->env = 0;
    if (env) {
        for (len = 0, ep = env; *ep; ep++) {
            len += (int) strlen(*ep) + 1;
        }

        key = "SYSTEMROOT";
        systemRoot = getenv(key);
        if (systemRoot) {
            len += (int) strlen(key) + 1 + (int) strlen(systemRoot) + 1;
        }

        destp = (char*) mprAlloc(cmd, len + 2);        /* Windows Requires two nulls */
        cmd->env = (char**) destp;
        for (ep = env; *ep; ep++) {
            strcpy(destp, *ep);
            mprLog(cmd, 7, "cmd: Set env variable: %s", destp);
            destp += strlen(*ep) + 1;
        }

        strcpy(destp, key);
        destp += strlen(key);
        *destp++ = '=';
        strcpy(destp, systemRoot);
        destp += strlen(systemRoot) + 1;

        *destp++ = '\0';
        *destp++ = '\0';                        /* Windows requires two nulls */
    }
}


static int startProcess(MprCmd *cmd)
{
    PROCESS_INFORMATION procInfo;
    STARTUPINFO         startInfo;
    int                 err;

    memset(&startInfo, 0, sizeof(startInfo));
    startInfo.cb = sizeof(startInfo);

    startInfo.dwFlags = STARTF_USESHOWWINDOW;
    if (cmd->flags & MPR_CMD_SHOW) {
        startInfo.wShowWindow = SW_SHOW;
    } else {
        startInfo.wShowWindow = SW_HIDE;
    }

    startInfo.dwFlags |= STARTF_USESTDHANDLES;

    if (cmd->flags & MPR_CMD_IN) {
        if (cmd->files[MPR_CMD_STDIN].clientFd > 0) {
            startInfo.hStdInput = (HANDLE) _get_osfhandle(cmd->files[MPR_CMD_STDIN].clientFd);
        }
    } else {
        startInfo.hStdInput = (HANDLE) _get_osfhandle(fileno(stdin));
    }
    if (cmd->flags & MPR_CMD_OUT) {
        if (cmd->files[MPR_CMD_STDOUT].clientFd > 0) {
            startInfo.hStdOutput = (HANDLE)_get_osfhandle(cmd->files[MPR_CMD_STDOUT].clientFd);
        }
    } else {
        startInfo.hStdOutput = (HANDLE)_get_osfhandle(fileno(stdout));
    }
    if (cmd->flags & MPR_CMD_ERR) {
        if (cmd->files[MPR_CMD_STDERR].clientFd > 0) {
            startInfo.hStdError = (HANDLE) _get_osfhandle(cmd->files[MPR_CMD_STDERR].clientFd);
        }
    } else {
        startInfo.hStdError = (HANDLE) _get_osfhandle(fileno(stderr));
    }

    /* TODO - cleanup. TODO - should NEW_CONSOLE be an option via cmd->flags */
    if (! CreateProcess(0, cmd->command, 0, 0, 1, /* CREATE_NEW_CONSOLE */ 0, cmd->env, cmd->dir, &startInfo, &procInfo)) {
        err = mprGetOsError();
        if (err == ERROR_DIRECTORY) {
            mprError(cmd, "Can't create process: %s, directory %s is invalid", cmd->program, cmd->dir);
        } else {
            mprError(cmd, "Can't create process: %s, %d", cmd->program, err);
        }
        return MPR_ERR_CANT_CREATE;
    }
    cmd->process = (int64) procInfo.hProcess;

    /*
     *  Wait for the child to initialize
     */
    WaitForInputIdle((HANDLE) cmd->process, 1000);

    return 0;
}


static int makePipe(MprCmd *cmd, int index)
{
    SECURITY_ATTRIBUTES clientAtt, serverAtt, *att;
    HANDLE              readHandle, writeHandle;
    MprCmdFile          *file;
    MprTime             now;
    char                pipeBuf[MPR_MAX_FNAME];
    int                 openMode, pipeMode, readFd, writeFd;
    static int          tempSeed = 0;

    memset(&clientAtt, 0, sizeof(clientAtt));
    clientAtt.nLength = sizeof(SECURITY_ATTRIBUTES);
    clientAtt.bInheritHandle = TRUE;

    /*
     *  Server fds are not inherited by the child
     */
    memset(&serverAtt, 0, sizeof(serverAtt));
    serverAtt.nLength = sizeof(SECURITY_ATTRIBUTES);
    serverAtt.bInheritHandle = FALSE;

    file = &cmd->files[index];
    now = ((int) mprGetTime(cmd) & 0xFFFF) % 64000;

    mprSprintf(pipeBuf, sizeof(pipeBuf), "\\\\.\\pipe\\MPR_%d_%d_%d.tmp", getpid(), (int) now, ++tempSeed);

    /*
     *  Pipes are always inbound. The file below is outbound. we swap whether the client or server
     *  inherits the pipe or file. MPR_CMD_STDIN is the clients input pipe.
     *  Pipes are blocking since both ends share the same blocking mode. Client must be blocking.
     */
    openMode = PIPE_ACCESS_INBOUND;
    pipeMode = 0;

    att = (index == MPR_CMD_STDIN) ? &clientAtt : &serverAtt;
    readHandle = CreateNamedPipe(pipeBuf, openMode, pipeMode, 1, 0, 256 * 1024, 1, att);
    if (readHandle == INVALID_HANDLE_VALUE) {
        mprError(cmd, "Can't create stdio pipes %s. Err %d\n", pipeBuf, mprGetOsError());
        return MPR_ERR_CANT_CREATE;
    }
    readFd = (int) (int64) _open_osfhandle((long) readHandle, 0);

    att = (index == MPR_CMD_STDIN) ? &serverAtt: &clientAtt;
    writeHandle = CreateFile(pipeBuf, GENERIC_WRITE, 0, att, OPEN_EXISTING, openMode, 0);
    writeFd = (int) _open_osfhandle((long) writeHandle, 0);

    if (readFd < 0 || writeFd < 0) {
        mprError(cmd, "Can't create stdio pipes %s. Err %d\n", pipeBuf, mprGetOsError());
        return MPR_ERR_CANT_CREATE;
    }
    if (index == MPR_CMD_STDIN) {
        file->clientFd = readFd;
        file->fd = writeFd;
        file->handle = writeHandle;
    } else {
        file->clientFd = writeFd;
        file->fd = readFd;
        file->handle = readHandle;
    }
    return 0;
}


#elif BLD_UNIX_LIKE
static int startProcess(MprCmd *cmd)
{
    MprCmdFile      *files;
    char            dir[MPR_MAX_FNAME];
    int             i, err;

    files = cmd->files;

    /*
     *  Create the child
     */
    cmd->process = vfork();

    if (cmd->process < 0) {
        mprLog(cmd, 0, "cmd: Can't fork a new process to run %s", cmd->program);
        return MPR_ERR_CANT_INITIALIZE;

    } else if (cmd->process == 0) {
        /*
         *  Child
         */
        umask(022);
        if (cmd->flags & MPR_CMD_NEW_SESSION) {
            setsid();
        }
        if (cmd->dir) {
            if (chdir(cmd->dir) < 0) {
                mprLog(cmd, 0, "cmd: Can't change directory to %s", cmd->dir);
                return MPR_ERR_CANT_INITIALIZE;
            }
        }

        /*  
         *  FUTURE -- could chroot as a security feature (perhaps cgi-bin)
         */
        if (cmd->flags & MPR_CMD_IN) {
            if (files[MPR_CMD_STDIN].clientFd >= 0) {
                dup2(files[MPR_CMD_STDIN].clientFd, 0);
            } else {
                close(0);
            }
        }
        if (cmd->flags & MPR_CMD_OUT) {
            if (files[MPR_CMD_STDOUT].clientFd >= 0) {
                dup2(files[MPR_CMD_STDOUT].clientFd, 1);
            } else {
                close(1);
            }
        }
        if (cmd->flags & MPR_CMD_ERR) {
            if (files[MPR_CMD_STDERR].clientFd >= 0) {
                dup2(files[MPR_CMD_STDERR].clientFd, 2);
            } else {
                close(2);
            }
        }

        /*
         *  FUTURE -- need to get a better max file limit than this
         */
        for (i = 3; i < 32; i++) {
            close(i);
        }

        if (cmd->env) {
            execve(cmd->program, cmd->argv, cmd->env);

        } else {
            execv(cmd->program, cmd->argv);
        }

        err = errno;
        getcwd(dir, sizeof(dir));

        mprErrorPrintf(cmd, "Can't exec %s, err %d, cwd %s\n", cmd->program, err, dir);

        /*
         *  Use _exit to avoid flushing I/O any other I/O.
         */
        _exit(-(MPR_ERR_CANT_INITIALIZE));

    } else {

        /*
         *  Close the client handles
         */
        for (i = 0; i < MPR_CMD_MAX_PIPE; i++) {
            if (files[i].clientFd >= 0) {
                close(files[i].clientFd);
                files[i].clientFd = -1;
            }
        }
    }
    return 0;
}


static int makePipe(MprCmd *cmd, int index)
{
    MprCmdFile      *file;
    int             fds[2];

    file = &cmd->files[index];

    if (pipe(fds) < 0) {
        mprError(cmd, "Can't create stdio pipes. Err %d", mprGetOsError());
        return MPR_ERR_CANT_CREATE;
    }
    if (index == MPR_CMD_STDIN) {
        file->clientFd = fds[0];        /* read fd */
        file->fd = fds[1];              /* write fd */
    } else {
        file->clientFd = fds[1];        /* write fd */
        file->fd = fds[0];              /* read fd */
    }
    mprLog(cmd, 7, "mprMakeCmdIO: pipe handles[%d] read %d, write %d", index, fds[0], fds[1]);
   
    return 0;
}


void initSignals(MprCtx ctx)
{
    struct sigaction    act;

    memset(&act, 0, sizeof(act));

    act.sa_sigaction = (void*) SIG_IGN;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_NOCLDSTOP | SA_RESTART | SA_SIGINFO;

    if (sigaction(SIGCHLD, &act, NULL) < 0) {
        mprError(ctx, "Can't initialize signals");
    }
}

#endif /* BLD_WIN_LIKE */


#if VXWORKS
/*
 *  Start the command to run (stdIn and stdOut are named from the client's perspective)
 */
int startProcess(MprCmd *cmd)
{
    MprCmdTaskFn    entryFn;
    SYM_TYPE        symType;
    char            *entryPoint;
    int             i, pri;

    mprLog(cmd, 4, "cmd: start %s", cmd->program);

    entryPoint = 0;
    if (cmd->env) {
        for (i = 0; cmd->env[i]; i++) {
            if (strncmp(cmd->env[i], "entryPoint=", 11) == 0) {
                entryPoint = mprStrdup(cmd, cmd->env[i]);
            }
        }
    }
    if (entryPoint == 0) {
        entryPoint = mprStrdup(cmd, mprGetBaseName(cmd->program));
    }

    if (symFindByName(sysSymTbl, entryPoint, (char**) &entryFn, &symType) < 0) {
#if MPR_BLD_FEATURE_DLL
        if (mprGetMpr()->loadDll(program, mprGetBaseName(program), 0, 0) < 0) {
            mprError(cmd, "start: can't load DLL %s, errno %d", program, mprGetOsError());
            return MPR_ERR_CANT_READ;
        }
#endif
        if (symFindByName(sysSymTbl, entryPoint, (char**) &entryFn, &symType) < 0) {
            mprError(cmd, "start: can't find symbol %s, errno %d", entryPoint, mprGetOsError());
            return MPR_ERR_CANT_ACCESS;
        }
    }

    taskPriorityGet(taskIdSelf(), &pri);

    /*
     *  Pass the server output file to become the client stdin.
     */
    cmd->process = taskSpawn(entryPoint, pri, 0, MPR_DEFAULT_STACK, (FUNCPTR) newTaskWrapper, 
        (int) cmd->program, (int) entryFn, (int) cmd, 0, 0, 0, 0, 0, 0, 0);

    if (cmd->process < 0) {
        mprError(cmd, "start: can't create task %s, errno %d", entryPoint, mprGetOsError());
        mprFree(entryPoint);
        return MPR_ERR_CANT_CREATE;
    }

    mprLog(cmd, 7, "cmd, child taskId %d", cmd->process);
    mprFree(entryPoint);

    if (semTake(cmd->startCond, MPR_TIMEOUT_START_TASK) != OK) {
        mprError("start: child %s did not initialize, errno %d", cmd->program, mprGetOsError());
        return MPR_ERR_CANT_CREATE;
    }
    semDelete(cmd->startCond);
    cmd->startCond = 0;

    return 0;
}


/*
 *  Executed by the child process
 */
static void newTaskWrapper(char *program, MprCmdTaskFn *entry, int cmdArg)
{
    MprCmd          *cmd;
    MprCmdFile      *files;
    char            **ep, dir[MPR_MAX_FNAME];
    int             inFd, outFd, errFd, id, rc;

    cmd = (MprCmd*) cmdArg;


    /*
     *  Open standard I/O files (in/out are from the server's perspective)
     */
    files = cmd->files;
    inFd = open(files[MPR_CMD_STDOUT].name, O_RDONLY, 0666);
    outFd = open(files[MPR_CMD_STDIN].name, O_WRONLY, 0666);
    errFd = open(files[MPR_CMD_STDERR].name, O_WRONLY, 0666);
    if (inFd < 0 || outFd < 0 || errFd < 0) {
        exit(255);
    }

    id = taskIdSelf();
    ioTaskStdSet(id, 0, inFd);
    ioTaskStdSet(id, 1, outFd);
    ioTaskStdSet(id, 2, errFd);

    /*
     *  Now that we have opened the stdin and stdout, wakeup our parent.
     */
    semGive(cmd->startCond);

    /*
     *  Create the environment
     */
    if (envPrivateCreate(id, -1) < 0) {
        exit(254);
    }
    for (ep = cmd->env; ep && *ep; ep++) {
        putenv(*ep);
    }

    /*
     *  Set current directory if required
     */
    if (cmd->dir) {
        rc = chdir(cmd->dir);
    } else {
        mprGetDirName(dir, sizeof(dir), cmd->program);
        rc = chdir(dir);
    }
    if (rc < 0) {
        mprLog(cmd, 0, "cmd: Can't change directory to %s", cmd->dir);
        exit(255);
    }

    /*
     *  Call the user's entry point
     */
    (*entry)(cmd->argc, cmd->argv);

    /*
     *  Cleanup
     */
    envPrivateDestroy(id);
    close(inFd);
    close(outFd);
    close(errFd);

    semGive(cmd->exitCond);
    semDelete(cmd->exitCond);
    cmd->exitCond = 0;

    exit(0);
}


static int makePipe(MprCmd *cmd, int index)
{
    MprCmdFile      *file;

    file = &cmd->files[index];

//unique name
    mprAllocSprintf(cmd, &file->name, -1, "/pipe/%s%d", BLD_PRODUCT, taskIdSelf());

    if (pipeDevCreate(file->name, 5, MPR_BUFSIZE) < 0) {
        mprError("Can't create pipes to run %s", cmd->program);
        return MPR_ERR_CANT_OPEN;
    }
    
    if (index == MPR_CMD_STDIN) {
        file->fd = open(file->name, O_RDONLY, 0644);
    } else {
        file->fd = open(file->name, O_WRONLY, 0644);
    }
    if (file->fd < 0) {
        mprError(cmd, "Can't create stdio pipes. Err %d", mprGetOsError());
        return MPR_ERR_CANT_CREATE;
    }
    return 0;
}
#endif /* VXWORKS */


#else /* BLD_FEATURE_CMD */
void __mprCmdDummy() {}
#endif /* BLD_FEATURE_CMD */

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
/************************************************************************/
/*
 *  End of file "../mprCmd.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../mprCond.c"
 */
/************************************************************************/

/**
 *  mprCond.c - Thread Conditional variables
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */


#if BLD_FEATURE_MULTITHREAD

static int condDestructor(MprCond *cp);

/*
 *  Create a condition variable for use by single or multiple waiters
 */

MprCond *mprCreateCond(MprCtx ctx)
{
    MprCond     *cp;

    cp = mprAllocObjWithDestructor(ctx, MprCond, condDestructor);
    if (cp == 0) {
        return 0;
    }

    cp->mutex = mprCreateLock(cp);
    cp->triggered = 0;

#if BLD_WIN_LIKE
    cp->cv = CreateEvent(NULL, FALSE, FALSE, NULL);
#elif VXWORKS
    cp->cv = semCCreate(SEM_Q_PRIORITY, SEM_EMPTY);
#else
    pthread_cond_init(&cp->cv, NULL);
#endif

    return cp;
}



/*
 *  Condition variable destructor
 */
static int condDestructor(MprCond *cp)
{
    mprAssert(cp);

    mprLock(cp->mutex);

#if BLD_WIN_LIKE
    CloseHandle(cp->cv);
#elif VXWORKS
    semDelete(cp->cv);
#else
    pthread_cond_destroy(&cp->cv);
#endif

    /* mprFree will call the mutex lock destructor */
    return 0;
}



/*
 *  Wait for the event to be triggered. Should only be used when there are single waiters. If the event is already
 *  triggered, then it will return immediately. Timeout of -1 means wait forever. Timeout of 0 means no wait.
 *  Returns 0 if the event was signalled. Returns < 0 if the timeout.
 */
int mprWaitForCond(MprCond *cp, int timeout)
{
    int     rc;

    rc = 0;
    if (timeout < 0) {
        timeout = MAXINT;
    }

#if BLD_WIN_LIKE
    /*  TODO -- should we not test triggered first like other O/S? */

    if (WaitForSingleObject(cp->cv, timeout) != WAIT_OBJECT_0) {
        return -1;
    }

    /*
     *  Reset the event
     */
    mprLock(cp->mutex);

    mprAssert(cp->triggered != 0);
    cp->triggered = 0;
    ResetEvent(cp->cv);

    mprUnlock(cp->mutex);
    rc = 0;

#elif VXWORKS
{
    mprLock(cp->mutex);
    if (cp->triggered == 0) {
        mprUnlock(cp->mutex);
        if (timeout < 0) {
            rc = semTake(cp->cv, WAIT_FOREVER);
        } else {
            rc = semTake(cp->cv, timeout);
        }
        mprLock(cp->mutex);
    }
    cp->triggered = 0;
    mprUnlock(cp->mutex);

    if (rc < 0) {
        if (errno == S_objLib_OBJ_UNAVAILABLE) {
            rc = MPR_ERR_TIMEOUT;
        } else {
            rc = MPR_ERR_GENERAL;
        }
    }
    return rc;
}
#else
{
    struct timespec     waitTill;
    struct timeval      current;
    int                 rc;

    mprLock(cp->mutex);
    rc = 0;
    if (cp->triggered == 0) {
        /*
         *  The pthread_cond_wait routines will atomically unlock the mutex
         *  before sleeping and will relock on awakening.
         */
        if (timeout < 0) {
            rc = pthread_cond_wait(&cp->cv, &cp->mutex->cs);
        } else {
            gettimeofday(&current, NULL);
            waitTill.tv_sec = current.tv_sec + (timeout / 1000);
            waitTill.tv_nsec = current.tv_usec + (timeout % 1000) * 1000000;
            rc = pthread_cond_timedwait(&cp->cv, &cp->mutex->cs,  &waitTill);
        }
    }
    cp->triggered = 0;
    mprUnlock(cp->mutex);

    if (rc == ETIMEDOUT) {
        rc = MPR_ERR_TIMEOUT;
    } else if (rc != 0) {
        rc = MPR_ERR_GENERAL;
    }
}
#endif

    /*  FUTURE -- should be consistent with return codes in tryLock */
    return rc;
}



/*
 *  Signal a condition and wakeup the waiter. Note: this may be called prior to the waiter waiting.
 */
void mprSignalCond(MprCond *cp)
{
    mprLock(cp->mutex);
    if (! cp->triggered) {
        cp->triggered = 1;
#if BLD_WIN_LIKE
        SetEvent(cp->cv);
#elif VXWORKS
        semGive(cp->cv);
#else
        pthread_cond_signal(&cp->cv);
#endif
    }
    mprUnlock(cp->mutex);
}



#else /* BLD_FEATURE_MULTITHREAD */
void __dummyMprCond() {}
#endif /* BLD_FEATURE_MULTITHREAD */

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
/************************************************************************/
/*
 *  End of file "../mprCond.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../mprCrypt.c"
 */
/************************************************************************/

/*
 *  mprCrypt.c - Base-64 encoding and decoding and MD5 support.
 *
 *  Algorithms by RSA.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



/*
 *  Constants for transform routine.
 */
#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21


static uchar PADDING[64] = {
  0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};


/*
 * F, G, H and I are basic MD5 functions.
 */
#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

/*
 * ROTATE_LEFT rotates x left n bits.
 */
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

/*
 *   FF, GG, HH, and II transformations for rounds 1, 2, 3, and 4.
 *   Rotation is separate from addition to prevent recomputation.
 */
 
#define FF(a, b, c, d, x, s, ac) { \
 (a) += F ((b), (c), (d)) + (x) + (uint)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }
#define GG(a, b, c, d, x, s, ac) { \
 (a) += G ((b), (c), (d)) + (x) + (uint)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }
#define HH(a, b, c, d, x, s, ac) { \
 (a) += H ((b), (c), (d)) + (x) + (uint)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }
#define II(a, b, c, d, x, s, ac) { \
 (a) += I ((b), (c), (d)) + (x) + (uint)(ac); \
 (a) = ROTATE_LEFT ((a), (s)); \
 (a) += (b); \
  }

typedef struct {
    uint state[4];
    uint count[2];
    uchar buffer[64];
} MD5CONTEXT;



#define CRYPT_HASH_SIZE   16

/*
 *  Encoding map lookup
 */
static char encodeMap[] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3',
    '4', '5', '6', '7', '8', '9', '+', '/',
};


/*
 *  Decode map
 */
static signed char decodeMap[] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
    -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, 
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};


static void decode(uint *output, uchar *input, uint len);
static void encode(uchar *output, uint *input, uint len);
static void finalize(uchar digest[16], MD5CONTEXT *context);
static void initMD5(MD5CONTEXT *context);
static void transform(uint state[4], uchar block[64]);
static void update(MD5CONTEXT *context, uchar *input, uint inputLen);


int mprDecode64(char *buffer, int bufsize, cchar *s)
{
    uint        bitBuf;
    char        *bp;
    int         c, i, j, shift;

    bp = buffer;
    *bp = '\0';
    while (*s && *s != '=') {
        bitBuf = 0;
        shift = 18;
        for (i = 0; i < 4 && *s && *s != '='; i++, s++) {
            c = decodeMap[*s & 0xff];
            if (c == -1) {
                return -1;
            } 
            bitBuf = bitBuf | (c << shift);
            shift -= 6;
        }
        --i;
        if ((bp + i) >= &buffer[bufsize]) {
            *buffer = '\0';
            return MPR_ERR_WONT_FIT;
        }
        for (j = 0; j < i; j++) {
            *bp++ = (char) ((bitBuf >> (8 * (2 - j))) & 0xff);
        }
        *bp = '\0';
    }
    return 0;
}


void mprEncode64(char *buffer, int bufsize, cchar *s)
{
    uint    shiftbuf;
    char    *bp;
    int     x, i, j, shift;

    bp = buffer;
    *bp = '\0';
    while (*s) {
        shiftbuf = 0;
        for (j = 2; j >= 0 && *s; j--, s++) {
            shiftbuf |= ((*s & 0xff) << (j * 8));
        }
        shift = 18;
        for (i = ++j; i < 4 && bp < &buffer[bufsize] ; i++) {
            x = (shiftbuf >> shift) & 0x3f;
            *bp++ = encodeMap[(shiftbuf >> shift) & 0x3f];
            shift -= 6;
        }
        while (j-- > 0) {
            *bp++ = '=';
        }
        *bp = '\0';
    }
}


/*
 *  Return the MD5 hash of a block
 */
char *mprGetMD5Hash(MprCtx ctx, uchar *buf, int length, cchar *prefix)
{
    MD5CONTEXT      context;
    uchar           hash[CRYPT_HASH_SIZE];
    cchar           *hex = "0123456789abcdef";
    char            *r, *str;
    char            result[(CRYPT_HASH_SIZE * 2) + 1];
    int             i, len;

    /*
     *  Take the MD5 hash of the string argument.
     */
    initMD5(&context);
    update(&context, buf, (uint) length);
    finalize(hash, &context);

    for (i = 0, r = result; i < 16; i++) {
        *r++ = hex[hash[i] >> 4];
        *r++ = hex[hash[i] & 0xF];
    }
    *r = '\0';

    len = (prefix) ? (int) strlen(prefix) : 0;
    str = (char*) mprAlloc(ctx, sizeof(result) + len);
    if (str) {
        if (prefix) {
            strcpy(str, prefix);
        }
        strcpy(str + len, result);
    }

    return str;
}


/*
 *  Convenience call 
 */ 
static char *md5(MprCtx ctx, cchar *string)
{
    return mprGetMD5Hash(ctx, (uchar*) string, (int) strlen(string), NULL);
}


/*
 *  Get a Nonce value for passing along to the client.  This function composes the string "secret:eTag:time:realm" 
 *  and calculates the MD5 digest.
 */ 
int mprCalcDigestNonce(MprCtx ctx, char **nonce, cchar *secret, cchar *etag, cchar *realm)
{
    time_t      now;
    char        nonceBuf[256];

    mprAssert(realm && *realm);

    time(&now);

    mprSprintf(nonceBuf, sizeof(nonceBuf), "%s:%s:%x:%s", secret, etag, (uint) now, realm); 

    *nonce = md5(ctx, nonceBuf);
    return 0;
}


/*
 *  Get a Digest value using the MD5 algorithm -- See RFC 2617 to understand this code.
 */ 
int mprCalcDigest(MprCtx ctx, char **digest, cchar *userName, cchar *password, cchar *realm, cchar *uri, 
    cchar *nonce, cchar *qop, cchar *nc, cchar *cnonce, cchar *method)
{
    char    a1Buf[256], a2Buf[256], digestBuf[256];
    char    *ha1, *ha2;

    mprAssert(qop);

    /*
     *  Compute HA1. If userName == 0, then the password is already expected to be in the HA1 format 
     *  (MD5(userName:realm:password).
     */
    if (userName == 0) {
        ha1 = mprStrdup(ctx, password);
    } else {
        mprSprintf(a1Buf, sizeof(a1Buf), "%s:%s:%s", userName, realm, password);
        ha1 = md5(ctx, a1Buf);
    }

    /*
     *  HA2
     */ 
    mprSprintf(a2Buf, sizeof(a2Buf), "%s:%s", method, uri);
    ha2 = md5(ctx, a2Buf);

    /*
     *  H(HA1:nonce:HA2)
     */
    if (strcmp(qop, "auth") == 0) {
        mprSprintf(digestBuf, sizeof(digestBuf), "%s:%s:%s:%s:%s:%s", ha1, nonce, nc, cnonce, qop, ha2);

    } else if (strcmp(qop, "auth-int") == 0) {
        mprSprintf(digestBuf, sizeof(digestBuf), "%s:%s:%s:%s:%s:%s", ha1, nonce, nc, cnonce, qop, ha2);

    } else {
        mprSprintf(digestBuf, sizeof(digestBuf), "%s:%s:%s", ha1, nonce, ha2);
    }

    *digest = md5(ctx, digestBuf);

    mprFree(ha1);
    mprFree(ha2);

    return 0;
}


/*
 *  MD5 initialization. Begins an MD5 operation, writing a new context.
 */ 
static void initMD5(MD5CONTEXT *context)
{
    context->count[0] = context->count[1] = 0;

    /*
     *  Load constants
     */
    context->state[0] = 0x67452301;
    context->state[1] = 0xefcdab89;
    context->state[2] = 0x98badcfe;
    context->state[3] = 0x10325476;
}


/*
 *  MD5 block update operation. Continues an MD5 message-digest operation, processing another message block, 
 *  and updating the context.
 */
static void update(MD5CONTEXT *context, uchar *input, uint inputLen)
{
    uint    i, index, partLen;

    index = (uint) ((context->count[0] >> 3) & 0x3F);

    if ((context->count[0] += ((uint)inputLen << 3)) < ((uint)inputLen << 3)){
        context->count[1]++;
    }
    context->count[1] += ((uint)inputLen >> 29);
    partLen = 64 - index;

    if (inputLen >= partLen) {
        memcpy((uchar*) &context->buffer[index], (uchar*) input, partLen);
        transform(context->state, context->buffer);

        for (i = partLen; i + 63 < inputLen; i += 64) {
            transform(context->state, &input[i]);
        }
        index = 0;
    } else {
        i = 0;
    }

    memcpy((uchar*) &context->buffer[index], (uchar*) &input[i], inputLen-i);
}


/*
 *  MD5 finalization. Ends an MD5 message-digest operation, writing the message digest and zeroizing the context.
 */ 
static void finalize(uchar digest[16], MD5CONTEXT *context)
{
    uchar   bits[8];
    uint    index, padLen;

    /* Save number of bits */
    encode(bits, context->count, 8);

    /* Pad out to 56 mod 64. */
    index = (uint)((context->count[0] >> 3) & 0x3f);
    padLen = (index < 56) ? (56 - index) : (120 - index);
    update(context, PADDING, padLen);

    /* Append length (before padding) */
    update(context, bits, 8);
    /* Store state in digest */
    encode(digest, context->state, 16);

    /* Zero sensitive information. */
    memset((uchar*)context, 0, sizeof (*context));
}


/*
 *  MD5 basic transformation. Transforms state based on block.
 */
static void transform(uint state[4], uchar block[64])
{
    uint a = state[0], b = state[1], c = state[2], d = state[3], x[16];

    decode(x, block, 64);

    /* Round 1 */
    FF(a, b, c, d, x[ 0], S11, 0xd76aa478); /* 1 */
    FF(d, a, b, c, x[ 1], S12, 0xe8c7b756); /* 2 */
    FF(c, d, a, b, x[ 2], S13, 0x242070db); /* 3 */
    FF(b, c, d, a, x[ 3], S14, 0xc1bdceee); /* 4 */
    FF(a, b, c, d, x[ 4], S11, 0xf57c0faf); /* 5 */
    FF(d, a, b, c, x[ 5], S12, 0x4787c62a); /* 6 */
    FF(c, d, a, b, x[ 6], S13, 0xa8304613); /* 7 */
    FF(b, c, d, a, x[ 7], S14, 0xfd469501); /* 8 */
    FF(a, b, c, d, x[ 8], S11, 0x698098d8); /* 9 */
    FF(d, a, b, c, x[ 9], S12, 0x8b44f7af); /* 10 */
    FF(c, d, a, b, x[10], S13, 0xffff5bb1); /* 11 */
    FF(b, c, d, a, x[11], S14, 0x895cd7be); /* 12 */
    FF(a, b, c, d, x[12], S11, 0x6b901122); /* 13 */
    FF(d, a, b, c, x[13], S12, 0xfd987193); /* 14 */
    FF(c, d, a, b, x[14], S13, 0xa679438e); /* 15 */
    FF(b, c, d, a, x[15], S14, 0x49b40821); /* 16 */

    /* Round 2 */
    GG(a, b, c, d, x[ 1], S21, 0xf61e2562); /* 17 */
    GG(d, a, b, c, x[ 6], S22, 0xc040b340); /* 18 */
    GG(c, d, a, b, x[11], S23, 0x265e5a51); /* 19 */
    GG(b, c, d, a, x[ 0], S24, 0xe9b6c7aa); /* 20 */
    GG(a, b, c, d, x[ 5], S21, 0xd62f105d); /* 21 */
    GG(d, a, b, c, x[10], S22,  0x2441453); /* 22 */
    GG(c, d, a, b, x[15], S23, 0xd8a1e681); /* 23 */
    GG(b, c, d, a, x[ 4], S24, 0xe7d3fbc8); /* 24 */
    GG(a, b, c, d, x[ 9], S21, 0x21e1cde6); /* 25 */
    GG(d, a, b, c, x[14], S22, 0xc33707d6); /* 26 */
    GG(c, d, a, b, x[ 3], S23, 0xf4d50d87); /* 27 */
    GG(b, c, d, a, x[ 8], S24, 0x455a14ed); /* 28 */
    GG(a, b, c, d, x[13], S21, 0xa9e3e905); /* 29 */
    GG(d, a, b, c, x[ 2], S22, 0xfcefa3f8); /* 30 */
    GG(c, d, a, b, x[ 7], S23, 0x676f02d9); /* 31 */
    GG(b, c, d, a, x[12], S24, 0x8d2a4c8a); /* 32 */

    /* Round 3 */
    HH(a, b, c, d, x[ 5], S31, 0xfffa3942); /* 33 */
    HH(d, a, b, c, x[ 8], S32, 0x8771f681); /* 34 */
    HH(c, d, a, b, x[11], S33, 0x6d9d6122); /* 35 */
    HH(b, c, d, a, x[14], S34, 0xfde5380c); /* 36 */
    HH(a, b, c, d, x[ 1], S31, 0xa4beea44); /* 37 */
    HH(d, a, b, c, x[ 4], S32, 0x4bdecfa9); /* 38 */
    HH(c, d, a, b, x[ 7], S33, 0xf6bb4b60); /* 39 */
    HH(b, c, d, a, x[10], S34, 0xbebfbc70); /* 40 */
    HH(a, b, c, d, x[13], S31, 0x289b7ec6); /* 41 */
    HH(d, a, b, c, x[ 0], S32, 0xeaa127fa); /* 42 */
    HH(c, d, a, b, x[ 3], S33, 0xd4ef3085); /* 43 */
    HH(b, c, d, a, x[ 6], S34,  0x4881d05); /* 44 */
    HH(a, b, c, d, x[ 9], S31, 0xd9d4d039); /* 45 */
    HH(d, a, b, c, x[12], S32, 0xe6db99e5); /* 46 */
    HH(c, d, a, b, x[15], S33, 0x1fa27cf8); /* 47 */
    HH(b, c, d, a, x[ 2], S34, 0xc4ac5665); /* 48 */

    /* Round 4 */
    II(a, b, c, d, x[ 0], S41, 0xf4292244); /* 49 */
    II(d, a, b, c, x[ 7], S42, 0x432aff97); /* 50 */
    II(c, d, a, b, x[14], S43, 0xab9423a7); /* 51 */
    II(b, c, d, a, x[ 5], S44, 0xfc93a039); /* 52 */
    II(a, b, c, d, x[12], S41, 0x655b59c3); /* 53 */
    II(d, a, b, c, x[ 3], S42, 0x8f0ccc92); /* 54 */
    II(c, d, a, b, x[10], S43, 0xffeff47d); /* 55 */
    II(b, c, d, a, x[ 1], S44, 0x85845dd1); /* 56 */
    II(a, b, c, d, x[ 8], S41, 0x6fa87e4f); /* 57 */
    II(d, a, b, c, x[15], S42, 0xfe2ce6e0); /* 58 */
    II(c, d, a, b, x[ 6], S43, 0xa3014314); /* 59 */
    II(b, c, d, a, x[13], S44, 0x4e0811a1); /* 60 */
    II(a, b, c, d, x[ 4], S41, 0xf7537e82); /* 61 */
    II(d, a, b, c, x[11], S42, 0xbd3af235); /* 62 */
    II(c, d, a, b, x[ 2], S43, 0x2ad7d2bb); /* 63 */
    II(b, c, d, a, x[ 9], S44, 0xeb86d391); /* 64 */

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;

    /* Zero sensitive information. */
    memset((uchar*) x, 0, sizeof(x));
}


/*
 *  Encodes input(uint) into output(uchar). Assumes len is a multiple of 4.
 */
static void encode(uchar *output, uint *input, uint len)
{
    uint i, j;

    for (i = 0, j = 0; j < len; i++, j += 4) {
        output[j] = (uchar) (input[i] & 0xff);
        output[j+1] = (uchar) ((input[i] >> 8) & 0xff);
        output[j+2] = (uchar) ((input[i] >> 16) & 0xff);
        output[j+3] = (uchar) ((input[i] >> 24) & 0xff);
    }
}


/*
 *  Decodes input(uchar) into output(uint). Assumes len is a multiple of 4.
 */
static void decode(uint *output, uchar *input, uint len)
{
    uint    i, j;

    for (i = 0, j = 0; j < len; i++, j += 4)
        output[i] = ((uint) input[j]) | (((uint) input[j+1]) << 8) | (((uint) input[j+2]) << 16) | (((uint) input[j+3]) << 24);
}



/*
 *  @copy   custom
 *  
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1994-2009. All Rights Reserved.
 *  Portions Copyright (C) 1991-2, RSA Data Security, Inc. All rights reserved. 
 *  
 *  RSA License:
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  
 *  THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 *  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 *  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 *  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 *  SUCH DAMAGE.
 *  
 *  RSA License Details
 *  -------------------
 *  
 *  License to copy and use this software is granted provided that it is 
 *  identified as the "RSA Data Security, Inc. MD5 Message-Digest Algorithm" 
 *  in all material mentioning or referencing this software or this function.
 *  
 *  License is also granted to make and use derivative works provided that such
 *  works are identified as "derived from the RSA Data Security, Inc. MD5 
 *  Message-Digest Algorithm" in all material mentioning or referencing the 
 *  derived work.
 *  
 *  RSA Data Security, Inc. makes no representations concerning either the 
 *  merchantability of this software or the suitability of this software for 
 *  any particular purpose. It is provided "as is" without express or implied 
 *  warranty of any kind.
 *  
 *  These notices must be retained in any copies of any part of this
 *  documentation and/or software.
 *  
 *  @end
 */
/************************************************************************/
/*
 *  End of file "../mprCrypt.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../mprDiskFile.c"
 */
/************************************************************************/

/**
 *  mprDiskFile.c - File services for systems with a (disk) based file system.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



#if !BLD_FEATURE_ROMFS


static void closeFile(MprFile *file);


static MprFile *openFile(MprCtx ctx, MprFileService *fileSystem, cchar *path, int omode, int perms)
{
    MprDiskFileService  *dfs;
    MprFile             *file;
    
    mprAssert(path && *path);

    dfs = (MprDiskFileService*) fileSystem;
    file = mprAllocObjWithDestructorZeroed(ctx, MprFile, closeFile);
    
    file->fd = open(path, omode, perms);
    if (file->fd < 0) {
        mprFree(file);
        return 0;
    }
    return file;
}


static void closeFile(MprFile *file)
{
    MprBuf  *bp;

    mprAssert(file);

    if (file == 0) {
        return;
    }

    bp = file->buf;
    if (bp && (file->mode & (O_WRONLY | O_RDWR))) {
        mprFlush(file);
    }
    if (file->fd >= 0) {
        close(file->fd);
    }
}


static int readFile(MprFile *file, void *buf, uint size)
{
    mprAssert(file);
    mprAssert(buf);

    return read(file->fd, buf, size);
}


static int writeFile(MprFile *file, cvoid *buf, uint count)
{
    mprAssert(file);
    mprAssert(buf);

#if VXWORKS
    return write(file->fd, (void*) buf, count);
#else
    return write(file->fd, buf, count);
#endif
}


static long seekFile(MprFile *file, int seekType, long distance)
{
    mprAssert(file);

    if (file == 0) {
        return MPR_ERR_BAD_HANDLE;
    }

    return lseek(file->fd, distance, seekType);
}


static bool accessFile(MprDiskFileService *fileSystem, cchar *path, int omode)
{
    return access(path, omode) == 0;
}


static int deleteFile(MprDiskFileService *fileSystem, cchar *path)
{
    return unlink((char*) path);
}


static int deleteDir(MprDiskFileService *fileSystem, cchar *path)
{
    return rmdir((char*) path);
}
 

static int makeDir(MprDiskFileService *fileSystem, cchar *path, int perms)
{
#if VXWORKS
    return mkdir((char*) path);
#else
    return mkdir(path, perms);
#endif
}


static int getFileInfo(MprDiskFileService *fileSystem, cchar *path, MprFileInfo *info)
{
    struct stat s;
#if BLD_WIN_LIKE
    char        *allocPath;

    mprAssert(path);
    mprAssert(info);

    allocPath = 0;
    info->valid = 0;

    if (stat(path, &s) < 0) {
        int     rc;
        /*
         *  Handle trailing "/"
         */
        if (path[strlen(path) - 1] == '/') {
            allocPath = mprStrdup(fileSystem, path);
            allocPath[strlen(allocPath) - 1] = '\0';
            path = allocPath;
        }
        rc = stat(path, &s);
        if (rc < 0) {
            mprFree(allocPath);
            return -1;
        }
        /* Fall through */
    }

    info->valid = 1;
    info->size = s.st_size;
    info->atime = s.st_atime;
    info->ctime = s.st_ctime;
    info->mtime = s.st_mtime;
    info->inode = s.st_ino;
    info->isDir = (s.st_mode & S_IFDIR) != 0;
    info->isReg = (s.st_mode & S_IFREG) != 0;

    /*
     *  Work hard on windows to determine if the file is a regular file.
     *  FUTURE -- OPT. Eliminate this CreateFile.
     */
    if (info->isReg) {
        long    fileType, att;

        if ((att = GetFileAttributes(path)) == -1) {
            mprFree(allocPath);
            return -1;
        }
        if (att & (FILE_ATTRIBUTE_REPARSE_POINT |
                FILE_ATTRIBUTE_DIRECTORY |
                FILE_ATTRIBUTE_ENCRYPTED |
                FILE_ATTRIBUTE_SYSTEM |
                FILE_ATTRIBUTE_OFFLINE)) {
            /*
             *  Catch accesses to devices like CON, AUX, NUL, LPT etc
             *  att will be set to ENCRYPTED on Win9X and NT.
             */
            info->isReg = 0;
        }
        if (info->isReg) {
            HANDLE handle;
            handle = CreateFile(path, 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
                0, OPEN_EXISTING, 0, 0);
            if (handle == INVALID_HANDLE_VALUE) {
                info->isReg = 0;
            } else {
                fileType = GetFileType(handle);
                if (fileType == FILE_TYPE_CHAR || fileType == FILE_TYPE_PIPE) {
                    info->isReg = 0;
                }
                CloseHandle(handle);
            }
        }
    }
    if (strcmp(path, "nul") == 0) {
        info->isReg = 0;
    }
    mprFree(allocPath);

#else /* !BLD_WIN_LIKE */
    mprAssert(path);
    mprAssert(info);

    info->valid = 0;

    if (stat((char*) path, &s) < 0) {
        return MPR_ERR_CANT_ACCESS;
    }

    info->valid = 1;
    info->size = s.st_size;
    info->atime = s.st_atime;
    info->ctime = s.st_ctime;
    info->mtime = s.st_mtime;
    info->inode = s.st_ino;
    info->isDir = (s.st_mode & S_IFDIR) != 0;
    info->isReg = (s.st_mode & S_IFREG) != 0;
    info->perms = s.st_mode & 07777;

    if (strcmp(path, "/dev/null") == 0) {
        info->isReg = 0;
    }

#endif
    return 0;
}
 

MprDiskFileService *mprCreateDiskFileService(MprCtx ctx)
{
    MprDiskFileService  *dfs;

    dfs = mprAllocObjZeroed(ctx, MprDiskFileService);
    if (dfs == 0) {
        return 0;
    }

    dfs->accessFile = accessFile;
    dfs->deleteFile = deleteFile;
    dfs->deleteDir = deleteDir;
    dfs->getFileInfo = getFileInfo;
    dfs->makeDir = makeDir;
    dfs->openFile = openFile;
    dfs->closeFile = closeFile;
    dfs->readFile = readFile;
    dfs->seekFile = seekFile;
    dfs->writeFile = writeFile;

    dfs->console = mprAllocObjZeroed(dfs, MprFile);
    if (dfs->console == 0) {
        mprFree(dfs);
    }
    dfs->console->fd = 1;

    dfs->error = mprAllocObjZeroed(dfs, MprFile);
    if (dfs->error == 0) {
        mprFree(dfs);
    }
    dfs->error->fd = 2;

    return dfs;
}
#endif /* !BLD_FEATURE_ROMFS */


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
/************************************************************************/
/*
 *  End of file "../mprDiskFile.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../mprEvent.c"
 */
/************************************************************************/

/*
 *  mprEvent.c - Event queue and event service
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */




static void appendEvent(MprEvent *eventQ, MprEvent *event);
static int  eventDestructor(MprEvent *event);
static void queueEvent(MprEventService *es, MprEvent *event);
static void removeEvent(MprEvent *event);

/*
 *  Initialize the event service.
 */
MprEventService *mprCreateEventService(MprCtx ctx)
{
    MprEventService     *es;

    es = mprAllocObjWithDestructorZeroed(ctx, MprEventService, eventDestructor);
    if (es == 0) {
        return 0;
    }

#if BLD_FEATURE_MULTITHREAD
    es->spin = mprCreateSpinLock(es);
    if (es->spin == 0) {
        mprFree(es);
        return 0;
    }
#endif

    es->eventQ.next = &es->eventQ;
    es->eventQ.prev = &es->eventQ;

    es->timerQ.next = &es->timerQ;
    es->timerQ.prev = &es->timerQ;

    es->now = mprGetTime(ctx);

    return es;
}


int mprStartEventService(MprEventService *es)
{
    return 0;
}


int mprStopEventService(MprEventService *es)
{
    return 0;
}


/*
 *  Queue a new event for service according to its priority and position in the event queue. Period is used as 
 *  the delay before running the event and as the period between events for continuous events.
 */
MprEvent *mprCreateEvent(MprCtx ctx, MprEventProc proc, int period, int priority, void *data, int flags)
{
    MprEventService *es;
    MprEvent        *event;

    if (mprIsExiting(ctx)) {
        return 0;
    }

    es = mprGetMpr(ctx)->eventService;

    event = mprAllocObjWithDestructor(ctx, MprEvent, eventDestructor);
    if (event == 0) {
        return 0;
    }

    event->proc = proc;
    event->period = period;
    event->priority = priority;
    event->data = data;
    event->flags = flags;
    event->timestamp = es->now;
    event->due = event->timestamp + period;
    event->service = es;

    /*
     *  Append in delay and priority order
     */
    queueEvent(mprGetMpr(ctx)->eventService, event);

    return event;
}


/*
 *  Called in response to mprFree on the event service
 */
static int eventDestructor(MprEvent *event)
{
    mprAssert(event);

    if (event->next) {
        mprRemoveEvent(event);
    }

    return 0;
}


/*  
 *  Remove an event from the event queues. Use mprRescheduleEvent to restart.
 */
void mprRemoveEvent(MprEvent *event)
{
    MprEventService     *es;
    Mpr                 *mpr;

    mpr = mprGetMpr(event);
    es = mpr->eventService;

    mprSpinLock(es->spin);
    removeEvent(event);

    if (es->timerQ.next != &es->timerQ) {
        es->lastEventDue = es->timerQ.prev->due;
    } else {
        es->lastEventDue = es->now;
    }
    mprSpinUnlock(es->spin);
}


void mprStopContinuousEvent(MprEvent *event)
{
    event->flags &= ~MPR_EVENT_CONTINUOUS;
}


void mprRestartContinuousEvent(MprEvent *event)
{
    event->flags |= MPR_EVENT_CONTINUOUS;
    mprRescheduleEvent(event, event->period);
}


/*
 *  Internal routine to queue an event to the event queue in delay and priority order. 
 */
static void queueEvent(MprEventService *es, MprEvent *event)
{
    MprEvent    *np, *q;

    /*  TODO OPT Spinlock */
    mprSpinLock(es->spin);

    if (event->due > es->now) {
        /*
         *  Due in the future some time
         */
        q = &es->timerQ;

        if (event->due > es->lastEventDue) {
            np = q->prev;

        } else {
            for (np = q->prev; np != q; np = np->prev) {
                if (event->due <= np->due) {
                    break;
                }
                if (event->priority >= np->priority) {
                    break;
                }
            }
        }
    } else {
        q = &es->eventQ;
        for (np = q->prev; np != q; np = np->prev) {
            if (event->priority >= np->priority) {
                break;
            }
        }
        es->eventCounter++;
    }

    /*
     *  Will assert if already in the queue
     */
    mprAssert(np != event);

    appendEvent(np, event);
    mprSpinUnlock(es->spin);

    mprAwakenWaitService(mprGetMpr(es)->waitService);
}


/*
 *  Get the next event from the front of the event queue
 *  Return 0 if not event.
 */
MprEvent *mprGetNextEvent(MprEventService *es)
{
    MprEvent    *event, *next;

    mprSpinLock(es->spin);

    event = es->eventQ.next;
    if (event != &es->eventQ) {
        removeEvent(event);

    } else {
        /*
         *  Move due timer events to the event queue. Allows priorities to work.
         */
        for (event = es->timerQ.next; event != &es->timerQ; event = next) {
            if (event->due > es->now) {
                break;
            }
            /*  TODO OPT -- OPTIMIZE event loop here */
            next = event->next;
            removeEvent(event);
            appendEvent(&es->eventQ, event);
            es->eventCounter++;
        }
            
        event = es->eventQ.next;
        if (event != &es->eventQ) {
            removeEvent(event);

        } else {
            event = 0;
        }
    }
    mprSpinUnlock(es->spin);

    return event;
}


/*
 *  Flags can be a combination of:
 *      MPR_SERVICE_ONE_THING which means one event or one I/O should be serviced before returning.
 */     
int mprServiceEvents(MprCtx ctx, int maxDelay, int flags)
{
    MprEventService *es;
    MprWaitService  *ws;
    MprEvent        *event;
    Mpr             *mpr;
    MprTime         start;
    int             delay, count;

    mpr = mprGetMpr(ctx);
    es = mpr->eventService;
    ws = mpr->waitService;
    start = es->now = mprGetTime(mpr);

#if BLD_FEATURE_MULTITHREAD
    mprSetCurrentThreadPriority(mpr, MPR_EVENT_PRIORITY);
    mprSetWaitServiceThread(mpr->waitService, mprGetCurrentThread(es));
#endif
    
    count = 0;

    do {

        event = mprGetNextEvent(es);
        
        if (event) {
            mprDoEvent(event, 0);
            
        } else {
            delay = mprGetIdleTime(es);
            if (maxDelay >= 0) {
                delay = min(maxDelay, delay);
            }
            mprWaitForIO(ws, delay);
            es->now = mprGetTime(es);
        }

        if (maxDelay >= 0) {
            maxDelay -= (int) (es->now - start);
            if (maxDelay <= 0) {
                break;
            }
        }
        
    } while (!mprIsExiting(mpr) && !(flags & MPR_SERVICE_ONE_THING));
    
    return 0;
}


void mprDoEvent(MprEvent *event, void *poolThread)
{
    MprEventService     *es;

#if BLD_FEATURE_MULTITHREAD
    if (event->flags & MPR_EVENT_THREAD && poolThread == 0) {
        /*
         *  Recall mprDoEvent but via a pool thread
         */
        mprStartPoolThread(event->service, (MprPoolProc) mprDoEvent, (void*) event, event->priority);
        return;
    }
#endif

    /*
     *  If it is a continuous event, we requeue here so that the event callback has the option of deleting the event.
     */
    es = mprGetMpr(event)->eventService;
    if (event->flags & MPR_EVENT_CONTINUOUS) {
        mprRescheduleEvent(event, event->period);
    }
    
    /*
     *  The callback can delete the event. NOTE: callback events MUST NEVER block.
     */
    if (event->proc) {
        (*event->proc)(event->data, event);
    }
}


/*
 *  Return the time till the next event.
 */
int mprGetIdleTime(MprEventService *es)
{
    int     delay;
    
    es->now = mprGetTime(es);

    mprSpinLock(es->spin);

    if (es->eventQ.next != &es->eventQ) {
        delay = 0;

    } else if (es->timerQ.next != &es->timerQ) {
        delay = (int) (es->timerQ.next->due - es->now);
        if (delay < 0) {
            delay = 0;
        }
        
    } else {
        delay = INT_MAX;
    }

    mprSpinUnlock(es->spin);

    return delay;
}


int mprGetEventCounter(MprEventService *es)
{
    return es->eventCounter;
}


void mprRescheduleEvent(MprEvent *event, int period)
{
    MprEventService     *es;
    Mpr                 *mpr;

    mpr = mprGetMpr(event);
    es = mprGetMpr(event)->eventService;

    event->period = period;
    event->timestamp = es->now;
    event->due = event->timestamp + period;

    if (event->next) {
        mprRemoveEvent(event);
    }

    queueEvent(mpr->eventService, event);
}


MprEvent *mprCreateTimerEvent(MprCtx ctx, MprEventProc proc, int period, int priority, void *data, int flags)
{
    return mprCreateEvent(ctx, proc, period, priority, data, MPR_EVENT_CONTINUOUS | flags);
}


/*
 *  Append a new event. Must be locked when called.
 */
static void appendEvent(MprEvent *eventQ, MprEvent *event)
{
    event->next = eventQ;
    event->prev = eventQ->prev;

    eventQ->prev->next = event;
    eventQ->prev = event;
}



/*
 *  Remove an event. Must be locked when called.
 */
static void removeEvent(MprEvent *event)
{
    event->next->prev = event->prev;
    event->prev->next = event->next;
    event->next = 0;
    event->prev = 0;
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
/************************************************************************/
/*
 *  End of file "../mprEvent.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../mprFile.c"
 */
/************************************************************************/

/**
 *  mprFile.c - File services.
 *
 *  This modules provides a simple cross platform file I/O abstraction. It uses a file system switch and 
 *  underneath a file system provider that implements actual I/O.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */




static int  fillBuf(MprFile *file);

#if BLD_WIN_LIKE
#define isDelim(mpr, c)               ((c) == '/' || (c) == '\\')
#define isAbsPath(mpr, path)          ((isalpha(*path) && path[1] == ':' && isDelim(mpr, path[2])) || isDelim(mpr, path[0]))
#define isAbsPathWithDrive(mpr, path) ((isalpha(*path) && path[1] == ':' && isDelim(mpr, path[2])))
#else
#define isDelim(mpr, c)               ((c) == mpr->pathDelimiter)
#define isAbsPath(mpr, path)          (path[0] == mpr->pathDelimiter)
#endif

#if BLD_WIN_LIKE || MACOSX
#define cmp(c1, c2)                   (tolower(c1) == tolower(c2))
#else
#define cmp(c1, c2)                   (c1 == c2)
#endif



MprFileService *mprCreateFileService(MprCtx ctx)
{
    Mpr     *mpr;

    mpr = mprGetMpr(ctx);

    /*
     *  TODO - these should really be set per file system. Need a file system object because Samba shares may be used
     *  on Unix.
     */
#if BLD_WIN_LIKE
    /*
     *  Windows understands both '/' and '\', but we prefer '/' because it makes all app code to only support
     *  one path delimiter.
     */
    mpr->pathDelimiter = '/';
    mpr->newline = "\r\n";
#else
    mpr->pathDelimiter = '/';
    mpr->newline = "\n";
#endif

#if BLD_WIN_LIKE || MACOSX
    mpr->caseSensitive = 0;
#else
    mpr->caseSensitive = 1;
#endif

#if WIN && FUTURE
    mprReadRegistry(ctx, &mpr->cygdrive, MPR_BUFSIZE, "HKEY_LOCAL_MACHINE\\SOFTWARE\\Cygnus Solutions\\Cygwin\\mounts v2",
        "cygdrive prefix");
#endif

#if BLD_FEATURE_ROMFS
    return (MprFileService*) mprCreateRomFileService(ctx);
#elif BREW
    return (MprFileService*) mprCreateBrewFileService(ctx);
#else
    return (MprFileService*) mprCreateDiskFileService(ctx);
#endif
}


MprFile *mprOpen(MprCtx ctx, cchar *path, int omode, int perms)
{
    MprFileService  *fs;
    MprFile         *file;
    MprFileInfo     info;

    fs = mprGetMpr(ctx)->fileService;

    file = fs->openFile(ctx, fs, path, omode, perms);
    if (file) {
#if BLD_DEBUG
        file->path = mprStrdup(file, path);
#endif
        fs->getFileInfo(fs, path, &info);
        file->size = (MprOffset) info.size;
        file->mode = omode;
        file->perms = perms;
    }
    return file;
}


int mprRead(MprFile *file, void *buf, uint size)
{
    MprFileService  *fs;
    MprBuf          *bp;
    void            *bufStart;
    int             bytes, totalRead;

    fs = mprGetMpr(file)->fileService;

    mprAssert(file);

    if (file == 0) {
        return MPR_ERR_BAD_HANDLE;
    }

    bp = file->buf;
    if (bp == 0) {
        totalRead = fs->readFile(file, buf, size);

    } else {
        bufStart = buf;

        while (size > 0) {
            if (mprGetBufLength(bp) == 0) {
                bytes = fillBuf(file);
                if (bytes <= 0) {
                    return -1;
                }
            }
            bytes = min((int) size, mprGetBufLength(bp));
            memcpy(buf, mprGetBufStart(bp), bytes);
            mprAdjustBufStart(bp, bytes);
            buf = (void*) (((char*) buf) + bytes);
            size -= bytes;
        }
        totalRead = (int) ((char*) buf - (char*) bufStart);
    }

    file->pos += totalRead;

    return totalRead;
}


int mprWrite(MprFile *file, const void *buf, uint count)
{
    MprFileService  *fs;
    MprBuf          *bp;
    int             bytes, written;

    mprAssert(file);

    fs = mprGetMpr(file)->fileService;

    if (file == 0) {
        return MPR_ERR_BAD_HANDLE;
    }

    bp = file->buf;
    if (bp == 0) {
        written = fs->writeFile(file, buf, count);

    } else {
        written = 0;
        while (count > 0) {
            bytes = mprPutBlockToBuf(bp, buf, count);
            if (bytes < 0) {
                return bytes;
            } 
            if (bytes != (int) count) {
                mprFlush(file);
            }
            count -= bytes;
            written += bytes;
            buf = (char*) buf + bytes;
        }
    }

    file->pos += written;
    if (file->pos > file->size) {
        file->size = file->pos;
    }

    return written;
}


int mprFlush(MprFile *file)
{
    MprFileService  *fs;
    MprBuf          *bp;
    int             len, rc;

    mprAssert(file);

    fs = mprGetMpr(file)->fileService;

    if (file == 0) {
        return MPR_ERR_BAD_HANDLE;
    }
    if (file->buf == 0) {
        return 0;
    }

    if (file->mode & (O_WRONLY | O_RDWR)) {
        bp = file->buf;
        while (mprGetBufLength(bp) > 0) {
            len = mprGetBufLength(bp);
            rc = fs->writeFile(file, mprGetBufStart(bp), len);
            if (rc < 0) {
                return rc;
            }
            mprAdjustBufStart(bp, rc);
        }
        mprFlushBuf(bp);
    }

    return 0;
}


long mprSeek(MprFile *file, int seekType, long pos)
{
    MprFileService  *fs;

    fs = mprGetMpr(file)->fileService;

    if (file->buf) {
        if (! (seekType == SEEK_CUR && pos == 0)) {
            /*
             *  Discard buffering as we may be seeking outside the buffer.
             *  TODO OPT. Could be smarter about this and preserve the buffer.
             */
            if (file->mode & (O_WRONLY | O_RDWR)) {
                if (mprFlush(file) < 0) {
                    return MPR_ERR_CANT_WRITE;
                }
            }
#if FUTURE || 1
            if (file->buf) {
                mprFlushBuf(file->buf);
            }
#endif
        }
    }

    if (seekType == SEEK_SET) {
        file->pos = pos;

    } else if (seekType == SEEK_CUR) {
        file->pos += pos;

    } else {
        file->pos = fs->seekFile(file, SEEK_END, 0);
    }

    if (fs->seekFile(file, SEEK_SET, (long) file->pos) != (long) file->pos) {
        return MPR_ERR;
    }

    if (file->mode & (O_WRONLY | O_RDWR)) {
        if (file->pos > file->size) {
            file->size = file->pos;
        }
    }

    return (long) file->pos;
}


bool mprAccess(MprCtx ctx, cchar *path, int omode)
{
    MprFileService  *fs;

    fs = mprGetMpr(ctx)->fileService;

    return fs->accessFile(fs, path, omode);
}


int mprDelete(MprCtx ctx, cchar *path)
{
    MprFileService  *fs;

    fs = mprGetMpr(ctx)->fileService;

    return fs->deleteFile(fs, path);
}


int mprDeleteDir(MprCtx ctx, cchar *path)
{
    MprFileService  *fs;

    fs = mprGetMpr(ctx)->fileService;

    return fs->deleteDir(fs, path);
}
 

int mprMakeDir(MprCtx ctx, cchar *path, int perms)
{
    MprFileService  *fs;

    fs = mprGetMpr(ctx)->fileService;

    return fs->makeDir(fs, path, perms);
}


int mprGetFileInfo(MprCtx ctx, cchar *path, MprFileInfo *info)
{
    MprFileService  *fs;

    fs = mprGetMpr(ctx)->fileService;

    return fs->getFileInfo(fs, path, info);
}


MprOffset mprGetFilePosition(MprFile *file)
{
    return file->pos;
}


MprOffset mprGetFileSize(MprFile *file)
{
    return file->size;
}


/*
 *  Fill the read buffer. Return the new buffer length. Only called when the buffer is empty.
 */
static int fillBuf(MprFile *file)
{
    MprFileService  *fs;
    MprBuf          *bp;
    int             len;

    bp = file->buf;
    fs = mprGetMpr(file)->fileService;

    mprAssert(mprGetBufLength(bp) == 0);
    mprFlushBuf(bp);

    len = fs->readFile(file, mprGetBufStart(bp), mprGetBufSpace(bp));
    if (len <= 0) {
        return len;
    }
    mprAdjustBufEnd(bp, len);
    return len;
}


/*
 *  Get a string from the file. This will put the file into buffered mode.
 */
char *mprGets(MprFile *file, char *buf, uint size)
{
    MprBuf  *bp;
    int     count, c;

    mprAssert(file);

    if (file == 0) {
        return 0;
    }

    if (file->buf == 0) {
        file->buf = mprCreateBuf(file, MPR_BUFSIZE, MPR_MAX_STRING);
    }
    bp = file->buf;

    /*
     *  Must leave room for null
     */
    count = 0;
    while (--size > 0) {
        if (mprGetBufLength(bp) == 0) {
            if (fillBuf(file) <= 0) {
                return 0;
            }
        }
        if ((c = mprGetCharFromBuf(bp)) == '\n') {
            buf[count] = '\0';
            return buf;
        }
        buf[count++] = c;
    }
    buf[count] = '\0';

    file->pos += count;

    return buf;
}


/*
 *  Put a string to the file. This will put the file into buffered mode.
 */
int mprPuts(MprFile *file, cchar *writeBuf, uint count)
{
    MprBuf  *bp;
    char    *buf;
    int     total, bytes;

    mprAssert(file);

    /*
     *  Buffer output and flush when full.
     */
    if (file->buf == 0) {
        file->buf = mprCreateBuf(file, MPR_BUFSIZE, 0);
        if (file->buf == 0) {
            return MPR_ERR_CANT_ALLOCATE;
        }
    }
    bp = file->buf;

    if (mprGetBufLength(bp) > 0 && mprGetBufSpace(bp) < (int) count) {
        mprFlush(file);
    }

    total = 0;
    buf = (char*) writeBuf;

    while (count > 0) {

        bytes = mprPutBlockToBuf(bp, buf, count);
        if (bytes < 0) {
            return MPR_ERR_CANT_ALLOCATE;

        } else if (bytes == 0) {
            if (mprFlush(file) < 0) {
                return MPR_ERR_CANT_WRITE;
            }
            continue;
        }

        count -= bytes;
        buf += bytes;
        total += bytes;
        file->pos += bytes;
    }

    return total;
}


/*
 *  Get a character from the file. This will put the file into buffered mode.
 */
int mprGetc(MprFile *file)
{
    MprBuf  *bp;
    int     len;

    mprAssert(file);

    if (file == 0) {
        return MPR_ERR;
    }

    if (file->buf == 0) {
        file->buf = mprCreateBuf(file, MPR_BUFSIZE, MPR_MAX_STRING);
    }
    bp = file->buf;

    if (mprGetBufLength(bp) == 0) {
        len = fillBuf(file);
        if (len <= 0) {
            return -1;
        }
    }
    if (mprGetBufLength(bp) == 0) {
        return 0;
    }
    file->pos++;
    return mprGetCharFromBuf(bp);
}


/*
 *  Peek at a character from the file without disturbing the read position. This will put the file into buffered mode.
 */
int mprPeekc(MprFile *file)
{
    MprBuf  *bp;
    int     len;

    mprAssert(file);

    if (file == 0) {
        return MPR_ERR;
    }

    if (file->buf == 0) {
        file->buf = mprCreateBuf(file, MPR_BUFSIZE, MPR_MAX_STRING);
    }
    bp = file->buf;

    if (mprGetBufLength(bp) == 0) {
        len = fillBuf(file);
        if (len <= 0) {
            return -1;
        }
    }
    if (mprGetBufLength(bp) == 0) {
        return 0;
    }
    return ((uchar*) mprGetBufStart(bp))[0];
}


/*
 *  Put a character to the file. This will put the file into buffered mode.
 */
int mprPutc(MprFile *file, int c)
{
    mprAssert(file);

    if (file == 0) {
        return -1;
    }

    if (file->buf) {
        if (mprPutCharToBuf(file->buf, c) != 1) {
            return MPR_ERR_CANT_WRITE;

        }
        file->pos++;
        return 1;

    }
    return mprWrite(file, &c, 1);
}


/*
 *  Enable and control file buffering
 */
int mprEnableFileBuffering(MprFile *file, int initialSize, int maxSize)
{
    mprAssert(file);

    if (file == 0) {
        return MPR_ERR_BAD_STATE;
    }

    if (initialSize <= 0) {
        initialSize = MPR_BUFSIZE;
    }
    if (maxSize <= 0) {
        maxSize = MPR_BUFSIZE;
    }
    if (maxSize <= initialSize) {
        maxSize = initialSize;
    }

    if (file->buf == 0) {
        file->buf = mprCreateBuf(file, initialSize, maxSize);
    }
    return 0;
}


void mprDisableFileBuffering(MprFile *file)
{
    mprFlush(file);
    mprFree(file->buf);
    file->buf = 0;
}


/*
 *  Make a directory with all necessary intervening directories.
 */
int mprMakeDirPath(MprCtx ctx, cchar *path, int perms)
{
    MprFileInfo info;
    char        dir[MPR_MAX_PATH], buf[MPR_MAX_PATH];
    char        *dirSep;
    char        *next, *tok;

    dir[0] = '\0';
    dirSep = "/\\";

    if (path == 0 || *path == '\0') {
        return MPR_ERR_BAD_ARGS;
    }

    mprStrcpy(buf, sizeof(buf), path);

    tok = 0;
    next = mprStrTok(buf, dirSep, &tok);
    if (*buf == '/') {
        dir[0] = '/';
        dir[1] = '\0';
    }
    while (next != NULL) {
        if (strcmp(next, ".") == 0 ) {
            next = mprStrTok(NULL, dirSep, &tok);
            continue;
        }
        strcat(dir, next);
        if (mprGetFileInfo(ctx, dir, &info) != 0) {
            if (mprMakeDir(ctx, dir, perms) < 0) {
                return MPR_ERR_CANT_CREATE;
            }
        }
        strcat(dir, "/");
        next = mprStrTok(NULL, dirSep, &tok);
    }
    return 0;
}


/*
 *  Compare two file path portions up to the given length. This uses the systems file system case sensitivity setting.
 *  If len is zero, compare the full paths.
 */
int mprCompareFilename(MprCtx ctx, cchar *path1, cchar *path2, int len)
{
    Mpr     *mpr;
#if WIN
    char    *tmpPath1, *tmpPath2;
#endif
    int     rc;

    mpr = mprGetMpr(ctx);

#if WIN
    tmpPath1 = tmpPath2 = 0;
    if (isAbsPathWithDrive(mpr, path1) && !isAbsPathWithDrive(mpr, path2)) {
        tmpPath2 = mprGetAbsFilename(ctx, path2);
        path2 = tmpPath2;
    }
    if (isAbsPathWithDrive(mpr, path2) && !isAbsPathWithDrive(mpr, path1)) {
        tmpPath1 = mprGetAbsFilename(ctx, path1);
        path1 = tmpPath1;
    }
#endif

    //  TODO - really should incorporate getpath.c style code here
    if (len == 0) {
        if (mpr->caseSensitive) {
            rc = strcmp(path1, path2);
        } else {
            rc = mprStrcmpAnyCase(path1, path2);
        }

    } else {
        if (mpr->caseSensitive) {
            rc = strncmp(path1, path2, len);
        } else {
            rc = mprStrcmpAnyCaseCount(path1, path2, len);
        }
    }
#if WIN
    mprFree(tmpPath1);
    mprFree(tmpPath2);
#endif
    return rc;
}


void mprMapDelimiters(MprCtx ctx, char *path, int delimiter)
{
    Mpr     *mpr;
    char    *cp;

    mpr = mprGetMpr(ctx);
    for (cp = path; *cp; cp++) {
        if (isDelim(mpr, *cp)) {
            *cp = delimiter;
        }
    }
}



/*
 *  Clean a path to remove redundant "./" and cleanup "../".
 */
char *mprCleanFilename(MprCtx ctx, cchar *pathArg)
{
    Mpr     *mpr;
    char    *start, *sp, *dp, *xp, *dot;

    if (pathArg == 0 || pathArg == '\0') {
        return mprStrdup(ctx, ".");
    }

    mpr = mprGetMpr(ctx);
    start = mprStrdup(ctx, pathArg);

    /*
     *  Remove multiple path separators and map '\\' to '/' for windows
     */
    sp = dp = start;
    while (*sp) {
#if BLD_WIN_LIKE
        if (*sp == '\\') {
            *sp = '/';
        }
#endif
        if (sp[0] == '/' && sp[1] == '/') {
            sp++;
        } else {
            *dp++ = *sp++;
        }
    }
    *dp = '\0';

    dot = strchr(start, '.');
    if (dot == 0) {
        mprMapDelimiters(mpr, start, mpr->pathDelimiter);
        return start;
    }

    /*
     *  Remove "./" segments
     */
    dp = dot;
    for (sp = dot; *sp; ) {
        /*
         *  "./" at the start or after a prior "/"
         */
        if (*sp == '.' && sp[1] == '/' && (sp == start || sp[-1] == '/')) {
            sp += 2;
        } else {
            *dp++ = *sp++;
        }
    }
    *dp = '\0';


    /*
     *  Remove "../segment"
     */
    for (sp = dot; *sp; ) {
        /*
         *  "/../" 
         */
        if (*sp == '.' && sp[1] == '.' && sp[2] == '/' && (sp > start && sp[-1] == '/')) {
            xp = sp + 3;
            sp -= 2;
            if (sp < start) {
                sp = start;
            } else {
                while (sp >= start && *sp != '/') {
                    sp--;
                }
                sp++;
            }
            dp = sp;
            while ((*dp++ = *xp) != 0) {
                xp++;
            }
        } else {
            sp++;
        }
    }
    *dp = '\0';

    /*
     *  Remove trailing "/.."
     */
    if (sp == &start[2] && *start == '.' && start[1] == '.') {
        *start = '\0';
    } else {
        if (sp > &start[2] && sp[-1] == '.' && sp[-2] == '.' && sp[-3] == '/') {
            sp -= 4;
            if (sp < start) {
                sp = start;
            } else {
                while (sp >= start && *sp != '/') {
                    sp--;
                }
                sp++;
            }
            *sp = '\0';
        }
    }

#if BLD_WIN_LIKE
    if (*start != '\0') {
        /*
         *  Windows will ignore trailing "." and " ". 
         */
        char *cp = &start[strlen(start) - 1];
        while (cp > start) {
            if (*cp == '.' || *cp == ' ' ) {
                *cp-- = '\0';
            } else {
                break;
            }
        }
    }
#endif
    mprMapDelimiters(mpr, start, mpr->pathDelimiter);
    return start;
}


/*
 *  Return an absolute path. This is not guaranteed to be clean.
//  TODO - compare with getpath logic and code
 */
char *mprGetAbsFilename(MprCtx ctx, cchar *pathArg)
{
    Mpr     *mpr;
    char    dir[MPR_MAX_FNAME];

    if (pathArg == 0 || *pathArg == '\0') {
        pathArg = ".";
    }

    mpr = mprGetMpr(ctx);

#if BLD_WIN_LIKE
    if (isAbsPathWithDrive(mpr, pathArg)) {
        return mprStrdup(ctx, pathArg);
    }
    GetFullPathName(pathArg, sizeof(dir) - 1, dir, NULL);
    mprMapDelimiters(mpr, dir, mpr->pathDelimiter);
    dir[sizeof(dir) - 1] = '\0';
    return mprStrdup(ctx, dir);

#else
{
    char    *path;
    if (isDelim(mpr, *pathArg)) {
        return mprStrdup(ctx, pathArg);
    }
    getcwd(dir, sizeof(dir));
    mprAllocSprintf(ctx, &path, -1, "%s/%s", dir, pathArg);
    return path;
}
#endif
}


char *mprGetRelFilename(MprCtx ctx, cchar *pathArg)
{
    Mpr     *mpr;
    char    home[MPR_MAX_FNAME], *hp, *cp, *result, *tmp, *path;
    int     homeSegments, len, i, commonSegments;

    mpr = mprGetMpr(ctx);
    
    if (pathArg == 0 || *pathArg == '\0') {
        return mprStrdup(ctx, ".");
    }

    if (!isAbsPath(mpr, pathArg)) {
        return mprStrdup(ctx, pathArg);
    }
    
    /*
     *  Must clean to ensure a minimal relative path result.
     */
    path = tmp = mprCleanFilename(ctx, pathArg);
#if WIN
{
    char    apath[MPR_MAX_FNAME];
    GetFullPathName(path, sizeof(apath) - 1, apath, NULL);
    apath[sizeof(apath) - 1] = '\0';
    path = apath;
    mprMapDelimiters(mpr, path, mpr->pathDelimiter);
}
#endif
    /*
     *  Get the working directory. Ensure it is null terminated and leave room to append a trailing "/"
     */
    getcwd(home, sizeof(home));
    home[sizeof(home) - 2] = '\0';
    len = (int) strlen(home);

    /*
     *  Count segments in home working directory. Ignore trailing delimiters.
     */
    for (homeSegments = 0, cp = home; *cp; cp++) {
        if (isDelim(mpr, *cp) && cp[1]) {
            homeSegments++;
        }
    }

    /*
     *  Find portion of path that matches the home directory, if any. Start at -1 because matching root doesn't count.
     */
    commonSegments = -1;
    for (hp = home, cp = path; *hp && *cp; hp++, cp++) {
        if (isDelim(mpr, *hp)) {
            if (isDelim(mpr, *cp)) {
                commonSegments++;
            }
        } else if (!cmp(*hp, *cp)) {
            break;
        }
    }
    mprAssert(commonSegments >= 0);

    /*
     *  Add one if the last segment matches. Handle trailing "/"
     */
    if ((isDelim(mpr, *hp) || *hp == '\0') && (isDelim(mpr, *cp) || *cp == '\0')) {
        commonSegments++;
    }
    
    if (isDelim(mpr, *cp)) {
        cp++;
    }
    
    hp = result = mprAlloc(ctx, homeSegments * 3 + (int) strlen(path) + 2);
    for (i = commonSegments; i < homeSegments; i++) {
        *hp++ = '.';
        *hp++ = '.';
        *hp++ = mpr->pathDelimiter;
    }
    if (*cp) {
        strcpy(hp, cp);
    } else if (hp > result) {
        /*
         *  Cleanup trailing "/" if "../" is the end of the new path
         */
        hp[-1] = '\0';
    } else {
        strcpy(result, ".");
    }
    mprMapDelimiters(mpr, result, mpr->pathDelimiter);
    mprFree(tmp);
    return result;
}


/*
 *  Return a unix style path with forward slash delimiters ("/") and no drive spec. Handles cygwin paths.
 */
char *mprGetUnixFilename(MprCtx ctx, cchar *pathArg)
{
#if BLD_WIN_LIKE
    Mpr     *mpr;
    char    *path;
    int     len;

    mpr = mprGetMpr(ctx);

    if (isAbsPath(mpr, pathArg)) {
        if (mpr->cygdrive) {
            len = (int) strlen(mpr->cygdrive);
            if (mprCompareFilename(mpr, mpr->cygdrive, &pathArg[2], len) == 0 && isDelim(mpr, pathArg[len+2])) {
                /*
                 *  If path is like: "c:/cygdrive/c/..."
                 *  Just strip the "c:" portion. Still validly qualified.
                 */
                path = mprStrdup(ctx, &pathArg[len + 2]);

            } else {
                /*
                 *  Path is like: "c:/some/other/path". Prepend "/cygdrive/c/"
                 */
                len = mprAllocSprintf(ctx, &path, -1, "%s/%c%s", mpr->cygdrive, pathArg[0], &pathArg[2]);
                if (isDelim(mpr, path[len-1])) {
                    path[len-1] = '\0';
                }
            }
        } else {
            /*
             *  Best we can do is get a relative path
             */
            path = mprGetRelFilename(ctx, pathArg);
        }
        mprMapDelimiters(mpr, path, '/');

    } else {
        path = (char*) mprStrdup(ctx, pathArg);
        mprMapDelimiters(mpr, path, '/');
    }
    return path;
#else
    return mprStrdup(ctx, pathArg);
#endif
}


/*
 *  Return a fully qualified absolute path with a drive spec. On systems with cygwin, map cygwin paths appropriately.
 */
char *mprGetWinFilename(MprCtx ctx, cchar *pathArg)
{
#if BLD_WIN_LIKE
    Mpr     *mpr;
    char    *buf, *path;
    int     len;

    mpr = mprGetMpr(ctx);

    if (isAbsPathWithDrive(mpr, pathArg)) {
        return mprStrdup(ctx, pathArg);
    }
    if (mpr->cygdrive) {
        len = (int) strlen(mpr->cygdrive);
        if (mprCompareFilename(mpr, mpr->cygdrive, pathArg, len) == 0 && isDelim(mpr, pathArg[len]) && 
                isalpha(pathArg[len+1]) && isDelim(mpr, pathArg[len+2])) {
            /*
             *  Has a "/cygdrive/c/" style prefix
             */
            mprAllocSprintf(ctx, &buf, -1, "%c:", pathArg[len+1], &pathArg[len + 2]);

        } else {
            /*
             *  Cygwin path. Prepend "c:/cygdrive"
             */
            mprAllocSprintf(ctx, &buf, -1, "%s/%s", mpr->cygdrive, pathArg);
        }
        path = mprGetAbsFilename(ctx, buf);
        mprFree(buf);

    } else {
        path = mprGetAbsFilename(ctx, pathArg);
    }
    mprMapDelimiters(mpr, path, mpr->pathDelimiter);
    return path;
#else
    return mprStrdup(ctx, pathArg);
#endif
}


//  TODO - this is inconsistent with mprGetDirName
char *mprGetParentDir(MprCtx ctx, cchar *pathArg)
{
    MprFileInfo     info;
    char            *path, buf[MPR_MAX_FNAME], buf2[MPR_MAX_FNAME];

    if (pathArg == 0 || pathArg[0] == '\0') {
        path = mprGetAbsFilename(ctx, ".");
        mprMapDelimiters(ctx, path, mprGetMpr(ctx)->pathDelimiter);
        return path;
    }

    if (mprGetFileInfo(ctx, pathArg, &info) == 0 && info.isDir == 1) {
        mprGetDirName(buf2, sizeof(buf2), pathArg);

    } else {
        mprGetDirName(buf, sizeof(buf), pathArg);
        mprGetDirName(buf2, sizeof(buf2), buf);
    }
    return mprStrdup(ctx, buf2);
}


void mprSetFileDelimiter(MprCtx ctx, cchar *path, int delimiter)
{
    mprGetMpr(ctx)->pathDelimiter = delimiter;
}


int mprGetFileDelimiter(MprCtx ctx, cchar *path)
{
    return mprGetMpr(ctx)->pathDelimiter;
}


void mprSetFileNewline(MprCtx ctx, cchar *path, cchar *newline)
{
    mprGetMpr(ctx)->newline = newline;
}


cchar *mprGetFileNewline(MprCtx ctx, cchar *path)
{
    return mprGetMpr(ctx)->newline;
}


int mprMakeTempFileName(MprCtx ctx, char *buf, int bufsize, cchar *tempDir)
{
    MprFile     *file;
    char        *dir;
    int         i, now;
    static int  tempSeed = 0;

    if (tempDir == 0) {
#if BLD_WIN_LIKE
        char    *cp;
        dir = mprStrdup(ctx, getenv("TEMP"));
        for (cp = dir; *cp; cp++) {
            if (*cp == '\\') {
                *cp = '/';
            }
        }
#else
        dir = mprStrdup(ctx, "/tmp");
#endif
    } else {
        dir = mprStrdup(ctx, tempDir);
    }

    now = ((int) mprGetTime(ctx) & 0xFFFF) % 64000;
    file = 0;

    for (i = 0; i < 128; i++) {
        mprSprintf(buf, bufsize, "%s/MPR_%d_%d_%d.tmp", dir, getpid(), now, ++tempSeed);
        file = mprOpen(ctx, buf, O_CREAT | O_EXCL | O_BINARY, 0664);
        if (file) {
            break;
        }
    }

    if (file == 0) {
        return MPR_ERR_CANT_CREATE;
    }

    mprFree(file);
    mprFree(dir);

    return 0;
}


/*
 *  Return the last portion of a pathname
 */
cchar *mprGetBaseName(cchar *name)
{
    char *cp;

    cp = strrchr(name, '/');

    if (cp == 0) {
        cp = strrchr(name, '\\');
        if (cp == 0) {
            return name;
        }
    } 
    if (cp == name) {
        if (cp[1] == '\0') {
            return name;
        }
    } else {
        if (cp[1] == '\0') {
            return "";
        }
    }
    return &cp[1];
}


/*
 *  Return the directory portion of a pathname into the users buffer.
 */
char *mprGetDirName(char *buf, int bufsize, cchar *path)
{
    cchar   *cp;
    int         dlen, len;

    mprAssert(path);
    mprAssert(buf);
    mprAssert(bufsize > 0);

    if (bufsize <= 0) {
        return 0;
    }

    if (*path == '\0') {
        strcpy(buf, ".");
        return buf;
    }

    len = (int) strlen(path);
    cp = &path[len - 1];

    /*
     *  Step over trailing slashes
     */
    while (cp > path && (*cp == '/' ||  *cp == '\\')) {
        cp--;
    }
    for (; cp > path && (*cp != '/' && *cp != '\\'); cp--) {
        ;
    }

    if (cp == path) {
        if (*cp != '/' && *cp != '\\') {
            /* No slashes found, parent is current dir */
            strcpy(buf, ".");
        } else {
            strcpy(buf, "/");
        }
        return buf;
    }

    dlen = (int) (cp - path);
    if (dlen < bufsize) {
        if (dlen == 0) {
            dlen++;
        }
        mprMemcpy(buf, bufsize, path, dlen);
        buf[dlen] = '\0';
    }
    return buf;
}


/*
 *  Return the extension portion of a pathname. Caller must not free the result.
 */
cchar *mprGetExtension(cchar *name)
{
    char *cp;

    if ((cp = strrchr(name, '.')) != NULL) {
        return ++cp;
    } 
    return 0;
}


int mprCopyFile(MprCtx ctx, cchar *fromName, cchar *toName, int mode)
{
    MprFile     *from, *to;
    char        buf[MPR_BUFSIZE];
    int         count;

    if ((from = mprOpen(ctx, fromName, O_RDONLY | O_BINARY, 0)) == 0) {
        mprError(ctx, "Can't open %s", fromName);
        return MPR_ERR_CANT_OPEN;
    }
    if ((to = mprOpen(ctx, toName, O_WRONLY | O_TRUNC | O_CREAT | O_BINARY, mode)) == 0) {
        mprError(ctx, "Can't open %s", toName);
        return MPR_ERR_CANT_OPEN;
    }
    
    while ((count = mprRead(from, buf, sizeof(buf))) > 0) {
        mprWrite(to, buf, count);
    }
    mprFree(from);
    mprFree(to);
    return 0;
}


#if WIN
MprList *mprGetDirList(MprCtx ctx, cchar *dir, bool enumDirs)
{
    HANDLE          h;
    WIN32_FIND_DATA findData;
    MprDirEntry     *dp;
    MprFileInfo     fileInfo;
    MprList         *list;
    char            path[MPR_MAX_FNAME];
    int             numSlots;

    list = 0;
    dp = 0;
    numSlots = 0;

    if (mprSprintf(path, sizeof(path), "%s/*.*", dir) < 0) {
        return 0;
    }

    h = FindFirstFile(path, &findData);
    if (h == INVALID_HANDLE_VALUE) {
        return 0;
    }

    list = mprCreateList(ctx);

    do {
        if (findData.cFileName[0] == '.') {
            continue;
        }
        if (enumDirs || !(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            dp = mprAllocObjZeroed(list, MprDirEntry);
            if (dp == 0) {
                return 0;
            }

            dp->name = mprStrdup(dp, findData.cFileName);
            if (dp->name == 0) {
                return 0;
            }

            /* dp->lastModified = (uint) findData.ftLastWriteTime.dwLowDateTime; */

            if (mprSprintf(path, sizeof(path), "%s/%s", dir, dp->name) < 0) {
                dp->lastModified = 0;
            } else {
                mprGetFileInfo(ctx, path, &fileInfo);
                dp->lastModified = fileInfo.mtime;
            }

            dp->isDir = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? 1 : 0;

#if FUTURE_64_BIT
            if (findData.nFileSizeLow < 0) {
                dp->size = (((uint64) findData.nFileSizeHigh) * INT64(4294967296)) + (4294967296L - 
                    (uint64) findData.nFileSizeLow);
            } else {
                dp->size = (((uint64) findData.nFileSizeHigh * INT64(4294967296))) +  (uint64) findData.nFileSizeLow;
            }
#else
            dp->size = (uint) findData.nFileSizeLow;
#endif
            mprAddItem(list, dp);
        }
    } while (FindNextFile(h, &findData) != 0);

    FindClose(h);
    return list;
}
#endif /* WIN */


#if BLD_HOST_UNIX || VXWORKS || CYGWIN
MprList *mprGetDirList(MprCtx ctx, cchar *path, bool enumDirs)
{
    DIR             *dir;
    MprFileInfo     fileInfo;
    MprList         *list;
    struct dirent   *dirent;
    MprDirEntry     *dp;
    char            fileName[MPR_MAX_FNAME];
    int             numSlots, rc;

    numSlots = 0;
    dp = 0;

    dir = opendir((char*) path);
    if (dir == 0) {
        return 0;
    }

    list = mprCreateList(ctx);

    while ((dirent = readdir(dir)) != 0) {
        if (dirent->d_name[0] == '.') {
            continue;
        }

        mprSprintf(fileName, sizeof(fileName), "%s/%s", path, dirent->d_name);
        rc = mprGetFileInfo(ctx, fileName, &fileInfo);

        if (enumDirs || (rc == 0 && !fileInfo.isDir)) { 
            dp = mprAllocObjZeroed(list, MprDirEntry);
            if (dp == 0) {
                return 0;
            }

            dp->name = mprStrdup(dp, dirent->d_name);
            if (dp->name == 0) {
                return 0;
            }

            if (rc == 0) {
                dp->lastModified = fileInfo.mtime;
                dp->size = fileInfo.size;
                dp->isDir = fileInfo.isDir;

            } else {
                dp->lastModified = 0;
                dp->size = 0;
                dp->isDir = 0;
            }
            mprAddItem(list, dp);
        }
    }
    closedir(dir);
    return list;
}
#endif


char *mprSearchForFile(MprCtx ctx, cchar *file, int flags, cchar *search, ...)
{
    va_list     ap;
	char	    path[MPR_MAX_FNAME], *tok, *dir, *searchPath;
    int         access;

    va_start(ap, search);
    searchPath = 0;
    mprCoreStrcat(ctx, &searchPath, -1, 0, NULL, search, ap);
    va_end(ap);

    mprLog(ctx, 4, "mprSearchForFile: %s search path %s", file, searchPath);

    access = (flags & MPR_SEARCH_EXE) ? X_OK : R_OK;

    tok = NULL;
    dir = mprStrTok(searchPath, MPR_SEARCH_DELIM, &tok);
    while (dir && *dir) {
        mprSprintf(path, sizeof(path), "%s/%s", dir, file);
		if (mprAccess(ctx, path, R_OK)) {
            mprLog(ctx, 4, "mprSearchForFile: found %s", path);
			return mprCleanFilename(ctx, path);
        }
        dir = mprStrTok(0, MPR_SEARCH_DELIM, &tok);
    }
    mprFree(searchPath);
    return 0;
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
/************************************************************************/
/*
 *  End of file "../mprFile.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../mprHash.c"
 */
/************************************************************************/

/*
 *  mprHash.cpp - Fast hashing table lookup module
 *
 *  This hash table uses a fast key lookup mechanism. Keys are strings and the value entries are arbitrary pointers.
 *  The keys are hashed into a series of buckets which then have a chain of hash entries using the standard doubly
 *  linked list classes (List/Link). The chain in in collating sequence so search time through the chain is on
 *  average (N/hashSize)/2.
 *
 *  This module is not thread-safe. It is the callers responsibility to perform all thread synchronization.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */




static int hashIndex(cchar *key, int size);
static MprHash  *lookupInner(int *bucketIndex, MprHash **prevSp, MprHashTable *table, cchar *key);

/*
 *  Create a new hash table of a given size. Caller should provide a size that is a prime number for the greatest
 *  efficiency. Caller should use mprFree to free the hash table.
 */

MprHashTable *mprCreateHash(MprCtx ctx, int hashSize)
{
    MprHashTable    *table;

    table = mprAllocObjZeroed(ctx, MprHashTable);
    if (table == 0) {
        return 0;
    }

    /*  TODO -- should support rehashing */
    if (hashSize < MPR_DEFAULT_HASH_SIZE) {
        hashSize = MPR_DEFAULT_HASH_SIZE;
    }
    table->hashSize = hashSize;

    table->count = 0;
    table->hashSize = hashSize;
    table->buckets = (MprHash**) mprAllocZeroed(table, sizeof(MprHash*) * hashSize);

    if (table->buckets == 0) {
        mprFree(table);
        return 0;
    }

    return table;
}


MprHashTable *mprCopyHash(MprCtx ctx, MprHashTable *master)
{
    MprHash         *hp;
    MprHashTable    *table;

    table = mprCreateHash(ctx, master->hashSize);
    if (table == 0) {
        return 0;
    }

    hp = mprGetFirstHash(master);
    while (hp) {
        mprAddHash(table, hp->key, hp->data);
        hp = mprGetNextHash(master, hp);
    }
    return table;
}


/*
 *  Insert an entry into the hash table. If the entry already exists, update its value. Order of insertion is not preserved.
 */
MprHash *mprAddHash(MprHashTable *table, cchar *key, cvoid *ptr)
{
    MprHash     *sp, *prevSp;
    int         index;

    sp = lookupInner(&index, &prevSp, table, key);

    if (sp != 0) {
        /*
         *  Already exists. Just update the data.
         */
        sp->data = ptr;
        return sp;
    }

    /*
     *  New entry
     */
    sp = mprAllocObjZeroed(table, MprHash);
    if (sp == 0) {
        return 0;
    }

    sp->data = ptr;
    sp->key = mprStrdup(sp, key);
    sp->bucket = index;

    sp->next = table->buckets[index];
    table->buckets[index] = sp;

    table->count++;
    return sp;
}


/*
 *  Multiple insertion. Insert an entry into the hash table allowing for multiple entries with the same key.
 *  Order of insertion is not preserved. Lookup cannot be used to retrieve all duplicate keys, some will be shadowed. 
 *  Use enumeration to retrieve the keys.
 */
MprHash *mprAddDuplicateHash(MprHashTable *table, cchar *key, cvoid *ptr)
{
    MprHash     *sp;
    int         index;

    sp = mprAllocObjZeroed(table, MprHash);
    if (sp == 0) {
        return 0;
    }

    index = hashIndex(key, table->hashSize);

    sp->data = ptr;
    sp->key = mprStrdup(sp, key);
    sp->bucket = index;

    sp->next = table->buckets[index];
    table->buckets[index] = sp;

    table->count++;
    return sp;
}


/*
 *  Remove an entry from the table
 */
int mprRemoveHash(MprHashTable *table, cchar *key)
{
    MprHash     *sp, *prevSp;
    int         index;

    if ((sp = lookupInner(&index, &prevSp, table, key)) == 0) {
        return MPR_ERR_NOT_FOUND;
    }

    if (prevSp) {
        prevSp->next = sp->next;
    } else {
        table->buckets[index] = sp->next;
    }
    table->count--;

    mprFree(sp);
    return 0;
}


/*
 *  Lookup a key and return the hash entry
 */
MprHash *mprLookupHashEntry(MprHashTable *table, cchar *key)
{
    mprAssert(key);

    return lookupInner(0, 0, table, key);
}


/*
 *  Lookup a key and return the hash entry data
 */
cvoid *mprLookupHash(MprHashTable *table, cchar *key)
{
    MprHash     *sp;

    mprAssert(key);

    sp = lookupInner(0, 0, table, key);
    if (sp == 0) {
        return 0;
    }
    return sp->data;
}


static MprHash *lookupInner(int *bucketIndex, MprHash **prevSp, MprHashTable *table, cchar *key)
{
    MprHash     *sp, *prev;
    int         index, rc;

    mprAssert(key);

    index = hashIndex(key, table->hashSize);
    if (bucketIndex) {
        *bucketIndex = index;
    }

    sp = table->buckets[index];
    prev = 0;

    while (sp) {
        rc = strcmp(sp->key, key);
        if (rc == 0) {
            if (prevSp) {
                *prevSp = prev;
            }
            return sp;
        }
        prev = sp;
        mprAssert(sp != sp->next);
        sp = sp->next;
    }
    return 0;
}


int mprGetHashCount(MprHashTable *table)
{
    return table->count;
}


/*
 *  Return the first entry in the table.
 */
MprHash *mprGetFirstHash(MprHashTable *table)
{
    MprHash     *sp;
    int         i;

    mprAssert(table);

    for (i = 0; i < table->hashSize; i++) {
        if ((sp = (MprHash*) table->buckets[i]) != 0) {
            return sp;
        }
    }
    return 0;
}


/*
 *  Return the next entry in the table
 */
MprHash *mprGetNextHash(MprHashTable *table, MprHash *last)
{
    MprHash     *sp;
    int         i;

    mprAssert(table);

    if (last == 0) {
        return mprGetFirstHash(table);
    }

    if (last->next) {
        return last->next;
    }

    for (i = last->bucket + 1; i < table->hashSize; i++) {
        if ((sp = (MprHash*) table->buckets[i]) != 0) {
            return sp;
        }
    }
    return 0;
}


/*
 *  Hash the key to produce a hash index.
 */
static int hashIndex(cchar *key, int size)
{
    uint        sum;

    sum = 0;
    while (*key) {
        sum += (sum * 33) + *key++;
    }

    return sum % size;
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
/************************************************************************/
/*
 *  End of file "../mprHash.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../mprHttp.c"
 */
/************************************************************************/

/**
 *  mprHttp.c - HTTP client (and Http code support)
 *
 *  The HTTP client supports HTTP/1.1 including all methods (DELELTE, GET, OPTIONS, POST, PUT, TRACE), SSL, keep-alive and
 *  chunked transfers. This module is thread-safe. The locking strategy for HTTP allows for a service events thread to 
 *  receive and respond to I/O events while a foreground thread issues HTTP threads. User callbacks can be registered via the
 *  mprSetHttpCallback API.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */




#if BLD_FEATURE_HTTP_CLIENT
static bool needRetry(MprHttp *http, cchar **url);
static void closeConnection(MprHttp *http);
static void completeRequest(MprHttp *http);
static MprHttpRequest *createRequest(MprHttp *http);
static void failRequest(MprHttp *http, cchar *fmt, ...);
static bool getChunkSize(MprHttp *http, MprBuf *buf, int *boundaryLen, int *size);
static char *getHttpToken(MprBuf *buf, cchar *delim);
static int  httpRequest(MprHttp *http, cchar *method, cchar *url, int flags);
static bool issueRequest(MprHttp *http, cchar *method, cchar *requestUrl, int flags);
static int  parseAuthenticate(MprHttp *http, char *authDetails);
static bool parseFirstLine(MprHttp *http, MprBuf *buf);
static bool parseHeaders(MprHttp *http, MprBuf *buf);
static void processResponse(MprHttp *http, MprBuf *buf);
static void readEvent(MprHttp *http, MprSocket *sp, int mask, int isPoolThread);
static void prepRequest(MprHttp *http);
static int  timeout(MprHttp *http, MprEvent *tp);
static void waitForResponse(MprHttp *http);
static int  writeBody(MprHttp *http, cchar *body, int bodyLen, bool block);

#if BLD_DEBUG
static void traceResponseData(MprCtx ctx, cchar *buf, int size);
#define traceData(ctx, buf, size) traceResponseData(ctx, buf, size);
#else
#define traceData(ctx, buf, size)
#endif
#endif /* BLD_FEATURE_HTTP_CLIENT */

#if BLD_FEATURE_HTTP
/**
 *  Standard HTTP error code table. This gets defined regardless of whether the HTTP feature is enabled.
 *  This is because some consumers of the MPR want to use server side HTTP, but not embed the client.
 */
typedef struct MprHttpCode {
    int     code;                           /**< Http error code */
    char    *codeString;                    /**< Code as a string (for hashing) */
    char    *msg;                           /**< Error message */
} MprHttpCode;


MprHttpCode MprHttpCodes[] = {
    { 100, "100", "Continue" },
    { 200, "200", "OK" },
    { 201, "201", "Created" },
    { 202, "202", "Accepted" },
    { 204, "204", "No Content" },
    { 205, "205", "Reset Content" },
    { 206, "206", "Partial Content" },
    { 301, "301", "Moved Permanently" },
    { 302, "302", "Moved Temporarily" },
    { 304, "304", "Not Modified" },
    { 305, "305", "Use Proxy" },
    { 307, "307", "Temporary Redirect" },
    { 400, "400", "Bad Request" },
    { 401, "401", "Unauthorized" },
    { 402, "402", "Payment Required" },
    { 403, "403", "Forbidden" },
    { 404, "404", "Not Found" },
    { 405, "405", "Method Not Allowed" },
    { 406, "406", "Not Acceptable" },
    { 408, "408", "Request Time-out" },
    { 409, "409", "Conflict" },
    { 410, "410", "Length Required" },
    { 411, "411", "Length Required" },
    { 413, "413", "Request Entity Too Large" },
    { 414, "414", "Request-URI Too Large" },
    { 415, "415", "Unsupported Media Type" },
    { 416, "416", "Requested Range Not Satisfiable" },
    { 417, "417", "Expectation Failed" },
    { 500, "500", "Internal Server Error" },
    { 501, "501", "Not Implemented" },
    { 502, "502", "Bad Gateway" },
    { 503, "503", "Service Unavailable" },
    { 504, "504", "Gateway Time-out" },
    { 505, "505", "Http Version Not Supported" },
    { 507, "507", "Insufficient Storage" },

    /*
     *  Proprietary codes (used internally) when connection to client is severed
     */
    { 550, "550", "Comms Error" },
    { 551, "551", "General Client Error" },
    { 0,   0 }
};


/*
 *  Initialize the http service.
 */
MprHttpService *mprCreateHttpService(MprCtx ctx)
{
    MprHttpService      *hs;
    MprHttpCode         *ep;

    hs = mprAllocObjZeroed(ctx, MprHttpService);
    if (hs == 0) {
        return 0;
    }

    hs->codes = mprCreateHash(hs, 41);
    for (ep = MprHttpCodes; ep->code; ep++) {
        mprAddHash(hs->codes, ep->codeString, ep);
    }
    return hs;
}

cchar *mprGetHttpCodeString(MprCtx ctx, int code)
{
    char            key[8];
    MprHttpCode     *ep;
    
    mprItoa(key, sizeof(key), code, 10);
    ep = (MprHttpCode*) mprLookupHash(mprGetMpr(ctx)->httpService->codes, key);
    if (ep == 0) {
        return "Custom error";
    }
    return ep->msg;
}


int mprStartHttpService(MprHttpService *hs)
{
    return 0;
}


int mprStopHttpService(MprHttpService *hs)
{
    return 0;
}

#endif /* BLD_FEATURE_HTTP */


#if BLD_FEATURE_HTTP_CLIENT
/*
 *  Create a new http instance which represents a single connection to a remote server. Only one request may be active
 *  on a http instance at a time.
 */
MprHttp *mprCreateHttp(MprCtx ctx)
{
    MprHttpService  *hs;
    MprHttp         *http;

    hs = mprGetMpr(ctx)->httpService;
    mprAssert(hs);

    http = mprAllocObjZeroed(hs, MprHttp);
    if (http == 0) {
        return 0;
    }

    http->protocolVersion = 1;
    http->protocol = mprStrdup(http, "HTTP/1.1");
    http->state = MPR_HTTP_STATE_BEGIN;
    http->currentPort = -1;
    http->proxyPort = -1;
    http->followRedirects = 1;

    http->defaultHost = mprStrdup(http, "localhost");
    http->defaultPort = 80;

    http->httpService = hs;
    http->timeoutPeriod = MPR_HTTP_TIMEOUT;
    http->retries = MPR_HTTP_RETRIES;
    http->useKeepAlive = 1;

    http->bufsize = MPR_HTTP_BUFSIZE;
    http->bufmax = -1;
    http->headerBuf = mprCreateBuf(http, http->bufsize, http->bufmax);

#if BLD_FEATURE_MULTITHREAD
    http->completeCond = mprCreateCond(http);
    http->mutex = mprCreateLock(http);
#endif

    http->request = createRequest(http);

    return http;
}


/*
 *  Close the HTTP connection
 */
static void closeConnection(MprHttp *http)
{
    mprAssert(http);

    mprLog(http, 3, "Http: close connection");

    mprFree(http->sock);
    http->sock = 0;
    mprFree(http->timer);
    http->timer = 0;
}


void mprDisconnectHttp(MprHttp *http)
{
    mprAssert(http);

    closeConnection(http);
}


/*
 *  Connection inactivity timeout handler
 */
static int timeout(MprHttp *http, MprEvent *tp)
{
    MprTime         now;

    now = mprGetTime(0);

    if (now < (http->timestamp + http->timeoutPeriod)) {
        return 0;
    }

    mprFree(http->timer);
    http->timer = 0;

    //  TODO - race
    if (http->request) {
        failRequest(http, "Timeout for %s", http->request->uri->url);

    } else {
        mprLog(http, 3, "Http: connection timeout");
        closeConnection(http);
    }

#if BLD_FEATURE_MULTITHREAD
    //  TODO - Race?
    if (!(http->userFlags & MPR_HTTP_DONT_BLOCK)) {
        mprSignalCond(http->completeCond);
    }
#endif
    return 0;
}


/*
 *  Open a new connection to a remote server
 */
static int openConnection(MprHttp *http, cchar *host, int port, bool secure)
{
    int     rc;

    mprAssert(http);
    mprAssert(http->sock == 0);

    mprLog(http, 3, "Http: Opening socket on: %s:%d", host, port);

#if BLD_FEATURE_SSL
    if (secure) {
        http->sock = mprCreateSocket(http, MPR_SECURE_CLIENT);
    } else
#endif
        http->sock = mprCreateSocket(http, NULL);

    rc = mprOpenClientSocket(http->sock, host, port, 0);
    if (rc < 0) {
        mprFree(http->sock);
        http->sock = 0;
        return MPR_ERR_CANT_OPEN;
    }

    mprFree(http->currentHost);
    http->currentHost = mprStrdup(http, host);
    http->currentPort = port;
    http->keepAlive = http->useKeepAlive;

    if (http->timeoutPeriod > 0 && !mprGetDebugMode(http)) {
        mprAssert(http->timer == 0);
        http->timer = mprCreateTimerEvent(http, (MprEventProc) timeout, http->timeoutPeriod, MPR_NORMAL_PRIORITY, 
                (void*) http, 0);
        if (http->timer == 0) {
            closeConnection(http);
            return MPR_ERR_NO_MEMORY;
        }
    }
    return 0;
}


/*
 *  Create a new request instance
 */
static MprHttpRequest *createRequest(MprHttp *http)
{
    MprHttpRequest  *req;

    mprAssert(http->state == MPR_HTTP_STATE_BEGIN);

    req = mprAllocObjZeroed(http, MprHttpRequest);
    if (req == 0) {
        return 0;
    }
    req->http = http;
    req->headers = mprCreateHash(req, -1);
    req->outBuf = mprCreateBuf(req, http->bufsize, http->bufmax);

    return req;
}


/*
 *  Reset the request instance
 */
static void resetRequest(MprHttp *http)
{
    MprHttpRequest  *req;

    mprAssert(http->state == MPR_HTTP_STATE_BEGIN);

    req = http->request;
    mprAssert(req);

    mprFree(req->headers);
    req->headers = 0;

    req->formData = 0;
    req->formLen = 0;
    req->bodyData = 0;
    req->bodyLen = 0;
    req->uploadFilename = 0;
}


static void prepRequest(MprHttp *http)
{
    MprHttpRequest  *req;

    mprAssert(http->state == MPR_HTTP_STATE_BEGIN);

    req = http->request;
    mprAssert(req);

    if (req->headers == 0) {
        req->headers = mprCreateHash(req, -1);
    }
    mprFlushBuf(req->outBuf);

    mprFree(req->outBuf);
    req->outBuf = mprCreateBuf(req, http->bufsize, http->bufmax);
    req->sentCredentials = 0;

    mprFree(http->error);
    http->error = 0;

    /*
     *  Response is preserved incase any auth data is required for the next request
     */
}


/*
 *  Create a new response object. 
 */
static MprHttpResponse *createResponse(MprHttp *http)
{
    MprHttpResponse *resp;

    resp = mprAllocObjZeroed(http, MprHttpResponse);
    if (resp == 0) {
        return 0;
    }

    resp->headers = mprCreateHash(resp, -1);
    resp->content = mprCreateBuf(resp, MPR_HTTP_BUFSIZE, http->bufmax);
    resp->http = http;
    resp->code = -1;

    return resp;
}


int mprHttpRequest(MprHttp *http, cchar *method, cchar *requestUrl, int flags)
{
    return httpRequest(http, method, requestUrl, flags);
}


/*
 *  Process a new request
 */
static int httpRequest(MprHttp *http, cchar *method, cchar *url, int flags)
{
    int     authCount, count;

    authCount = 0;
    http->userFlags = flags;

    /*
     *  Retry requests incase the client disconnects due to keep-alive. May happen at anytime.
     */
    count = -1;
    do {
        prepRequest(http);
        if (issueRequest(http, method, url, flags)) {
            if (http->response && needRetry(http, &url)) {
                mprFlushBuf(http->response->content);
                count--;
                continue;
            }
            break;
        }
    } while (++count < http->retries && !mprIsExiting(http));

    if (count > http->retries) {
        mprLog(http, 4, "http: failed to \"%s\" %s after %d attempt(s)", method, url, count);
        return MPR_ERR_TOO_MANY;
    }
    if (mprIsExiting(http)) {
        return MPR_ERR_BAD_STATE;
    }
    return 0;
}


/*
 *  Check the response for authentication failures and redirections. Return true if a retry is requried.
 */
static bool needRetry(MprHttp *http, cchar **url)
{
    MprHttpResponse     *resp;
    MprHttpRequest      *req;

    mprAssert(http->response);

    /*
     *  For sync mode requests (no callback), handle authorization and redirections inline
     */
    resp = http->response;
    req = http->request;

    if (resp->code == MPR_HTTP_CODE_UNAUTHORIZED) {
        if (http->user == 0) {
            http->error = mprStrdup(http, "Authentication required");

        } else if (http->request->sentCredentials) {
            http->error = mprStrdup(http, "Authentication failed");

        } else {
            return 1;
        }

    } else if (MPR_HTTP_CODE_MOVED_PERMANENTLY <= resp->code && resp->code <= MPR_HTTP_CODE_MOVED_TEMPORARILY && 
            http->followRedirects) {
        *url = resp->location;
        return 1;
    }

    return 0;
}


/*
 *  Create and issue a HTTP request
 */
static bool issueRequest(MprHttp *http, cchar *method, cchar *requestUrl, int flags)
{
    MprHttpRequest      *req;
    MprHttpResponse     *resp;
    MprUri              *url;
    MprBuf              *outBuf;
    MprHashTable        *headers;
    MprHash             *header;
    char                *host;
    int                 port, len, rc;

    mprAssert(http);
    mprAssert(method && *method);
    mprAssert(requestUrl && *requestUrl);
    mprAssert(http->state == MPR_HTTP_STATE_BEGIN);

    mprLog(http, 4, "Http: issueRequest: %s %s", method, requestUrl);

    rc = 0;
    req = http->request;
    resp = http->response;

    http->timestamp = mprGetTime(req);

    outBuf = req->outBuf;

    mprFree(req->method);
    req->method = mprStrdup(req, method);

    mprFree(req->uri);
    url = req->uri = mprParseUri(req, requestUrl);

    if (req->formData) {
        req->bodyData = req->formData;
        req->bodyLen = req->formLen;
    }

    if (*requestUrl == '/') {
        host = (http->proxyHost) ? http->proxyHost : http->defaultHost;
        port = (http->proxyHost) ? http->proxyPort : http->defaultPort;

    } else {
        host = (http->proxyHost) ? http->proxyHost : url->host;
        port = (http->proxyHost) ? http->proxyPort : url->port;
    }

    if (http->sock) {
        if (port != http->currentPort || strcmp(host, http->currentHost) != 0) {
            /*
             *  This request is for a different host or port. Must close socket.
             */
            closeConnection(http);
        }
    }

    if (http->sock == 0) {
        if (openConnection(http, host, port, url->secure) < 0) {
            failRequest(http, "Can't open socket on %s:%d", host, port);
            return 0;
        }

    } else {
        mprLog(http, 4, "Http: reusing keep-alive socket on: %s:%d", host, port);
    }

    /*
     *  Emit the request
     */
    if (http->proxyHost && *http->proxyHost) {
        if (url->query && *url->query) {
            mprPutFmtToBuf(outBuf, "%s http://%s:%d%s?%s %s\r\n", method, http->proxyHost, http->proxyPort, 
                url->url, url->query, http->protocol);
        } else {
            mprPutFmtToBuf(outBuf, "%s http://%s:%d%s %s\r\n", method, http->proxyHost, http->proxyPort, url->url,
                http->protocol);
        }
    } else {
        if (url->query && *url->query) {
            mprPutFmtToBuf(outBuf, "%s %s?%s %s\r\n", method, url->url, url->query, http->protocol);
        } else {
            mprPutFmtToBuf(outBuf, "%s %s %s\r\n", method, url->url, http->protocol);
        }
    }

    if (http->authType && strcmp(http->authType, "basic") == 0) {
        char    abuf[MPR_MAX_STRING], encDetails[MPR_MAX_STRING];
        mprSprintf(abuf, sizeof(abuf), "%s:%s", http->user, http->password);
        mprEncode64(encDetails, sizeof(encDetails), abuf);
        mprPutFmtToBuf(outBuf, "Authorization: basic %s\r\n", encDetails);
        req->sentCredentials = 1;

    } else if (http->authType && strcmp(http->authType, "digest") == 0) {
        char    a1Buf[256], a2Buf[256], digestBuf[256];
        char    *ha1, *ha2, *digest, *qop;

        if (http->httpService->secret == 0 && mprCreateHttpSecret(http) < 0) {
            mprLog(req, MPR_ERROR, "Http: Can't create secret for digest authentication");
            mprFree(req);
            http->request = 0;
            return 0;
        }
        mprFree(http->authCnonce);
        mprCalcDigestNonce(http, &http->authCnonce, http->httpService->secret, 0, http->authRealm);

        len = mprSprintf(a1Buf, sizeof(a1Buf), "%s:%s:%s", http->user, http->authRealm, http->password);
        ha1 = mprGetMD5Hash(req, (uchar*) a1Buf, len, NULL);

        len = mprSprintf(a2Buf, sizeof(a2Buf), "%s:%s", method, url->url);
        ha2 = mprGetMD5Hash(req, (uchar*) a2Buf, len, NULL);

        qop = (http->authQop) ? http->authQop : (char*) "";

        http->authNc++;
        if (mprStrcmpAnyCase(http->authQop, "auth") == 0) {
            len = mprSprintf(digestBuf, sizeof(digestBuf), "%s:%s:%08x:%s:%s:%s",
                ha1, http->authNonce, http->authNc, http->authCnonce, http->authQop, ha2);

        } else if (mprStrcmpAnyCase(http->authQop, "auth-int") == 0) {
            len = mprSprintf(digestBuf, sizeof(digestBuf), "%s:%s:%08x:%s:%s:%s",
                ha1, http->authNonce, http->authNc, http->authCnonce, http->authQop, ha2);

        } else {
            qop = "";
            len = mprSprintf(digestBuf, sizeof(digestBuf), "%s:%s:%s", ha1, http->authNonce, ha2);
        }

        mprFree(ha1);
        mprFree(ha2);
        digest = mprGetMD5Hash(req, (uchar*) digestBuf, len, NULL);

        if (*qop == '\0') {
            mprPutFmtToBuf(outBuf, "Authorization: Digest username=\"%s\", realm=\"%s\", nonce=\"%s\", "
                "uri=\"%s\", response=\"%s\"\r\n",
                http->user, http->authRealm, http->authNonce, url->url, digest);

        } else if (strcmp(qop, "auth") == 0) {
            mprPutFmtToBuf(outBuf, "Authorization: Digest username=\"%s\", realm=\"%s\", domain=\"%s\", "
                "algorithm=\"MD5\", qop=\"%s\", cnonce=\"%s\", nc=\"%08x\", nonce=\"%s\", opaque=\"%s\", "
                "stale=\"FALSE\", uri=\"%s\", response=\"%s\"\r\n",
                http->user, http->authRealm, http->authDomain, http->authQop, http->authCnonce, http->authNc,
                http->authNonce, http->authOpaque, url->url, digest);

        } else if (strcmp(qop, "auth-int") == 0) {
            ;
        }
        mprFree(digest);
        req->sentCredentials = 1;
    }

    mprPutFmtToBuf(outBuf, "Host: %s\r\n", host);
    mprPutFmtToBuf(outBuf, "User-Agent: %s\r\n", MPR_HTTP_NAME);

    if (http->protocolVersion == 1) {
        if (http->keepAlive) {
            mprPutFmtToBuf(outBuf, "Connection: Keep-Alive\r\n");
        } else {
            mprPutFmtToBuf(outBuf, "Connection: close\r\n");
        }

        if (req->bodyLen > 0) {
            mprPutFmtToBuf(outBuf, "Content-Length: %d\r\n", req->bodyLen);
        }
    }

    if (req->uploadFilename) {
        mprPutStringToBuf(outBuf, "Content-Type: multipart/form-data\r\n");

    } else if (req->formData) {
        mprPutStringToBuf(outBuf, "Content-Type: application/x-www-form-urlencoded\r\n");
    }

    headers = http->request->headers;
    if (mprGetHashCount(headers) > 0) {
        for (header = 0; (header = mprGetNextHash(headers, header)) != 0; ) {
            mprPutFmtToBuf(outBuf, "%s: %s\r\n", header->key, header->data);
        }
    }

    mprAddNullToBuf(outBuf);
    mprLog(req, 3, "\nHttp: @@@ Request =>\n%s", mprGetBufStart(outBuf));

    mprPutStringToBuf(outBuf, "\r\n");

    /*
     *  Write the request as a blocking write.
     */
    len = mprGetBufLength(outBuf);
    mprSetSocketBlockingMode(http->sock, 1);
    if ((rc = mprWriteSocket(http->sock, mprGetBufStart(outBuf), len)) != len) {
        failRequest(http, "Can't write request to socket");
        return 0;
    }
    mprSetSocketBlockingMode(http->sock, 0);

    mprFree(http->response);
    http->response = 0;

    /*
     *  Form data must be already url encoded. Takes precedence over body data.
     */
    if (req->bodyData && writeBody(http, req->bodyData, req->bodyLen, 1) < 0) {
        failRequest(http, "Can't write body data");
        return 0;
    }

    if (http->protocolVersion == 0 && req->bodyData) {
        http->keepAlive = 0;
        completeRequest(http);

    } else {
        if (http->sock->handler == 0) {
            mprSetSocketCallback(http->sock, (MprSocketProc) readEvent, (void*) http, 0, MPR_READABLE, MPR_NORMAL_PRIORITY);
        } else {
            mprSetSocketEventMask(http->sock, MPR_READABLE);
        }
        http->state = MPR_HTTP_STATE_WAIT;

        if (!(http->userFlags & MPR_HTTP_DONT_BLOCK)) {
            waitForResponse(http);
        }
    }

    /*
     *  Cleanup for the next request
     */
    resetRequest(http);

    return 1;
}


/*
 *  Wait for the request to complete
 */
static void waitForResponse(MprHttp *http)
{
    while (http->state != MPR_HTTP_STATE_BEGIN) {
#if BLD_FEATURE_MULTITHREAD
        if (mprIsRunningEventsThread(http) > 0) {
            mprWaitForCond(http->completeCond, http->timeoutPeriod);
        } else
#endif
            mprServiceEvents(http, -1, MPR_SERVICE_ONE_THING);
    }
}


/*
 *  Write body data
 */
static int writeBody(MprHttp *http, cchar *body, int bodyLen, bool block)
{
    int     written, rc, nbytes;

    mprSetSocketBlockingMode(http->sock, block);

    for (written = 0; written < bodyLen; ) {
        nbytes = bodyLen - written;
        rc = mprWriteSocket(http->sock, (char*) &body[written], nbytes);
        if (rc < 0) {
            mprSetSocketBlockingMode(http->sock, 0);
            return 0;
        }
        written += rc;
        if (rc != nbytes && !block) {
            break;
        }
    }
    mprSetSocketBlockingMode(http->sock, 0);

    return written;
}


int mprSetHttpBody(MprHttp *http, cchar *body, int len)
{
    MprHttpRequest      *req;

    req = http->request;

    req->bodyLen = len;
    req->bodyData = mprMemdup(req, body, len);
    if (req->bodyData == 0) {
        return MPR_ERR_NO_MEMORY;
    }
    return 0;
}


int mprSetHttpForm(MprHttp *http, cchar *body, int len)
{
    MprHttpRequest      *req;

    req = http->request;
    req->formData = mprRealloc(req, req->formData, req->formLen + len + 1);
    if (req->formData == 0) {
        return MPR_ERR_NO_MEMORY;
    }

    memcpy(&req->formData[req->formLen], body, len);
    req->formLen += len;
    req->formData[req->formLen] = '\0';

    return 0;
}


int mprAddHttpFormItem(MprHttp *http, cchar *keyArg, cchar *valueArg)
{
    MprHttpRequest  *req;
    char            *value, *key, *sep, *encodedKey, *encodedValue;
    int             len;

    req = http->request;
    if (req == 0) {
        return MPR_ERR_BAD_STATE;
    }

    key = (char*) keyArg;
    value = (char*) valueArg;

    if (value == 0) {
        key = mprStrdup(http, key);
        if ((value = strchr(key, '=')) != 0) {
            *value++ = '\0';
        }
    }
    if (key == 0 || value == 0) {
        return MPR_ERR_BAD_ARGS;
    }

    /*
     *  Encode key and value separately
     */
    len = (int) strlen(key) * 3 + 1;
    encodedKey = mprAlloc(http, len);
    mprUrlEncode(encodedKey, len, key);

    len = (int) strlen(value) * 3 + 1;
    encodedValue = mprAlloc(http, len);
    mprUrlEncode(encodedValue, len, value);

    sep = (req->formData) ? "&" : "";
    len = mprReallocStrcat(req, &req->formData, -1, req->formLen, 0, sep, encodedKey, "=", encodedValue, NULL);
    if (len < 0) {
        return len;
    }
    req->formLen = len;
    return 0;
}


//  TODO - incomplete
int mprSetHttpUpload(MprHttp *http, cchar *key, cchar *value)
{
    http->request->uploadFilename = (void*) 0;
    return 0;
}


int mprWriteHttpBody(MprHttp *http, cchar *buf, int len, bool block)
{
    mprAssert(http);
    mprAssert(buf);
    mprAssert(len > 0);

    if (len == 0 && buf) {
        len = (int) strlen(buf);
    }
    return writeBody(http, buf, len, block);
}


/*
 *  Determine how much data to read into the input buffer and grow the buffer (for headers) if required.
 */
static int getReadSize(MprHttp *http, MprBuf *buf)
{
    MprHttpResponse     *resp;
    int                 space;

    resp = http->response;

    if (http->callback) {
        /*
         *  Only compact if there is a callback. Without a callback, the data is buffered and preserved until the 
         *  request completes.
         */
        mprCompactBuf(buf);
        mprResetBufIfEmpty(buf);
    }
    space = mprGetBufSpace(buf);

    //  TODO - should have a limit here 
    if (space < MPR_HTTP_BUFSIZE) {
        if (mprGrowBuf(buf, MPR_HTTP_BUFSIZE) < 0) {
            return MPR_ERR_NO_MEMORY;
        }
    }

    space = mprGetBufSpace(buf);
    if (resp && resp->contentRemaining > 0 && http->state >= MPR_HTTP_STATE_CONTENT) {
        space = min(space, resp->contentRemaining);
    }

    return space;
}


/*
 *  Process to an incoming HTTP response
 */
static void readEvent(MprHttp *http, MprSocket *sp, int mask, int isPoolThread)
{
    MprBuf  *buf;
    int     nbytes, len;

    if (http->sock == 0 || http->request == 0) {
        return;
    }

    http->timestamp = mprGetTime(0);

    /*
     *  Use the header buffer if still waiting for headers. Otherwise append to the content buffer.
     */
    buf = (http->state <= MPR_HTTP_STATE_WAIT) ? http->headerBuf : http->response->content;
    mprAssert(buf);

    if ((len = getReadSize(http, buf)) < 0) {
        return;
    }

    nbytes = mprReadSocket(http->sock, mprGetBufEnd(buf), len);
    
    if (nbytes < 0 || (nbytes == 0 && mprGetSocketEof(http->sock))) {
        /* 
         *  Server disconnection
         */
        http->keepAlive = 0;
        if (http->state != MPR_HTTP_STATE_COMPLETE && http->response && http->response->contentLength == 0) {
            mprLog(http, 5, "Socket end of file from server, rc %d, errno %d", nbytes, errno);
            http->state = MPR_HTTP_STATE_COMPLETE;
            processResponse(http, buf);

        } else {
            failRequest(http, "Communications error");
        }

    } else if (nbytes > 0) {
        /* 
         *  Data available
         */
        mprLog(http, 5, "Read %d bytes from socket, ask for %d", nbytes, len);
        traceData(http, mprGetBufStart(buf), nbytes);
        mprAdjustBufEnd(buf, nbytes);
        processResponse(http, buf);
    }

#if BLD_FEATURE_MULTITHREAD
    if (http->state != MPR_HTTP_STATE_BEGIN) {
        mprEnableWaitEvents(http->sock->handler, MPR_READABLE);

    } else if (!(http->userFlags & MPR_HTTP_DONT_BLOCK)) {
        /*
         *  Must do this here (and in timeout()) rather than in completeCond as it could signal the originating thread
         *  and immediately delete the http/resp objects.
         */
        mprSignalCond(http->completeCond);
    }
#endif
}


/*
 *  Main HTTP state machine. Process whatever data is available so far.
 */
static void processResponse(MprHttp *http, MprBuf *buf)
{
    MprHttpResponse     *resp;
    MprHttpRequest      *req;
    char                *token, *start, *end;
    int                 len, boundaryLen;

    resp = http->response;
    req = http->request;
    token = 0;
    boundaryLen = 0;

    while (http->state != MPR_HTTP_STATE_BEGIN) {
        len = mprGetBufLength(buf);

        switch(http->state) {
        case MPR_HTTP_STATE_WAIT:
            /*
             *  Start of response
             */
            len = mprGetBufLength(buf);
            start = mprGetBufStart(buf);
            if (len == 0 || (end = mprStrnstr(start, "\r\n\r\n", len)) == 0) {
                /* Request is currently incomplete, need wait for more data */
                return;
            }

#if BLD_DEBUG
            *end = '\0';
            mprLog(http, 3, "\nHttp: @@@ Response =>\n%s\n", start);
            *end = '\r';
#endif

            if (!parseFirstLine(http, buf) || !parseHeaders(http, buf)) {
                return;
            }
            resp = http->response;
            buf = resp->content;
            if (resp->flags & MPR_HTTP_RESP_INPUT_CHUNKED) {
                http->state = MPR_HTTP_STATE_CHUNK;
                break;

            } else {
                if (resp->contentRemaining == 0 && !(resp->flags & MPR_HTTP_RESP_INPUT_CHUNKED)) {
                    http->state = MPR_HTTP_STATE_COMPLETE;
                    break;
                }
                http->state = MPR_HTTP_STATE_CONTENT;
                if (mprGetBufLength(buf) == 0) {
                    return;
                }
            }
            /* Fall through to state content */

        case MPR_HTTP_STATE_CONTENT:
            /*
             *  May come here multiple times if doing chunked input
             */
            mprAssert(mprGetBufLength(buf) > 0);
            len = mprGetBufLength(buf);
            if (resp->flags & MPR_HTTP_RESP_INPUT_CHUNKED) {
                len = min(len, resp->chunkRemaining);
                resp->chunkRemaining -= len;
            } else {
                resp->contentRemaining -= len;
            }
            resp->length += len;
            mprAssert(resp->contentRemaining >= 0);
            mprAssert(resp->chunkRemaining >= 0);

            if (http->callback) {
                (http->callback)(http, len);
            }
            mprAdjustBufStart(buf, len);
            if (resp->flags & MPR_HTTP_RESP_INPUT_CHUNKED) {
                if (resp->chunkRemaining <= 0) {
                    http->state = MPR_HTTP_STATE_CHUNK;
                    break;
                }
                return;
            }
            if (resp->contentRemaining > 0) {
                return;
            }
            http->state = MPR_HTTP_STATE_COMPLETE;
            /* Fall through */
                
        case MPR_HTTP_STATE_COMPLETE:
            if (http->callback == 0) {
                buf->start = buf->data;
            }
            completeRequest(http);
            return;
                
        case MPR_HTTP_STATE_CHUNK:
            if (!getChunkSize(http, buf, &boundaryLen, &resp->chunkRemaining)) {
                return;
            }
            if (http->callback == 0) {
                start = mprGetBufStart(buf);
                memmove(start, start + boundaryLen, mprGetBufLength(buf));
                mprAdjustBufEnd(buf, -boundaryLen);
            } else {
                mprAdjustBufStart(buf, boundaryLen);
            }
            if (resp->chunkRemaining == 0) {
                /*
                 *  Received all the data. Discard last "\r\n" 
                 */
                mprAdjustBufEnd(buf, -2);
                http->state = MPR_HTTP_STATE_COMPLETE;
                
            } else {
                http->state = MPR_HTTP_STATE_CONTENT;
                if (mprGetBufLength(buf) == 0) {
                    return;
                }
            }
            break;

        case MPR_HTTP_STATE_PROCESSING:
        default:
            failRequest(http, "Bad state");
            return;
        }
    }
}


/*
 *  Process the first line of data from the HTTP response
 */
static bool parseFirstLine(MprHttp *http, MprBuf *buf)
{
    MprHttpResponse     *resp;
    char                *code;

    mprAssert(buf);
    mprAssert(http->response == 0);

    http->response = resp = createResponse(http);
    if (resp == 0) {
        failRequest(http, "No memory");
        return 0;
    }

    resp->protocol = getHttpToken(buf, " ");
    if (resp->protocol == 0 || resp->protocol[0] == '\0') {
        failRequest(http, "Bad HTTP response");
        return 0;
    }

    if (strncmp(resp->protocol, "HTTP/1.", 7) != 0) {
        failRequest(http, "Unsupported protocol");
        return 0;
    }

    code = getHttpToken(buf, " ");
    if (code == 0 || *code == '\0') {
        failRequest(http, "Bad HTTP response");
        return 0;
    }
    resp->code = atoi(code);
    resp->message = getHttpToken(buf, "\r\n");

    return 1;
}


/*
 *  Parse the response headers. Only come here when all the headers are resident.
 */
static bool parseHeaders(MprHttp *http, MprBuf *buf)
{
    MprHttpResponse *resp;
    char            *key, *value, *tp;
    int             boundaryLen, len;

    resp = http->response;
    boundaryLen = 0;

    while (mprGetBufLength(buf) > 0 && buf->start[0] != '\r') {
        if ((key = getHttpToken(buf, ":")) == 0) {
            failRequest(http, "Bad HTTP header");
            return 0;
        }

        /*
         *  Tokenize the headers insitu. This modifies the data in the input buffer
         */
        value = getHttpToken(buf, "\r\n");
        while (isspace((int) *value)) {
            value++;
        }
        
        /*
         *  Save each header in the headers hash. Not strduped, these are references into the buffer.
         */
        mprAddHash(resp->headers, mprStrUpper(key), value);

        switch (key[0]) {
        case 'C':
            if (strcmp("CONTENT-LENGTH", key) == 0) {
                resp->contentLength = atoi(value);
                if (resp->contentLength < 0) {
                    resp->contentLength = 0;
                }
                if (mprStrcmpAnyCase(resp->http->request->method, "HEAD") == 0 || 
                        (resp->flags & MPR_HTTP_RESP_INPUT_CHUNKED)) {
                    resp->contentLength = 0;
                    resp->contentRemaining = 0;
                } else {
                    resp->contentRemaining = resp->contentLength;
                }

            } else if (strcmp("CONNECTION", key) == 0) {
                /*  TODO OPT - better to have a case independent strcmp */
                mprStrLower(value);
                if (strcmp(value, "close") == 0) {
                    http->keepAlive = 0;
                    if (resp->contentLength == 0) {
                        resp->contentRemaining = MAXINT;
                    }
                }
            }
            break;
                
        case 'K':
            if (strcmp("KEEP-ALIVE", key) == 0) {
                /*
                 *  Quick compare for "Keep-Alive: timeout=N, max=1"
                 */
                len = (int) strlen(value);
                if (len > 2 && value[len - 1] == '1' && value[len - 2] == '=' && 
                        tolower((int)(value[len - 3])) == 'x') {
                    http->keepAlive = 0;
                }
            }
            break;                
                
        case 'L':
            if (strcmp("LOCATION", key) == 0) {
                resp->location = value;
            }
            break;

        case 'T':
            if (strcmp("TRANSFER-ENCODING", key) == 0) {
                /*  TODO OPT - better to have a case independent strcmp */
                mprStrLower(value);
                if (strcmp(value, "chunked") == 0) {
                    resp->flags |= MPR_HTTP_RESP_INPUT_CHUNKED;
                    resp->contentLength = 0;
                    resp->contentRemaining = 0;
                }
            }
            break;
        
        case 'W':
            if (strcmp("WWW-AUTHENTICATE", key) == 0) {
                tp = value;
                while (*value && !isspace((int) *value)) {
                    value++;
                }
                *value++ = '\0';
                mprStrLower(tp);
                
                http->authType = mprStrdup(http, tp);
                
                if (parseAuthenticate(http, value) < 0) {
                    failRequest(http, "Bad Authentication header");
                    return 0;
                }
            }
            break;
        }
    }

    if (!(resp->flags & MPR_HTTP_RESP_INPUT_CHUNKED)) {
        /* Step over "\r\n" */
        mprAdjustBufStart(buf, 2);
    }

    if (resp->contentLength > 0) {
        len = min(resp->contentLength, mprGetBufLength(buf));
    } else {
        //  TODO BUG. This won't work for pipelined requests where the first request is using content encoding -- as we are 
        //  copying all available data. This may be more than content data and may include the headers for a subsequent 
        //  pipelined request.
        len = mprGetBufLength(buf);
    }

    mprPutBlockToBuf(resp->content, mprGetBufStart(buf), len);
    mprAdjustBufStart(buf, len);

    return 1;
}


/*
 *  Parse an authentication response
 */
static int parseAuthenticate(MprHttp *http, char *authDetails)
{
    MprHttpResponse *resp;
    char            *value, *tok, *key, *dp, *sp;
    int             seenComma;

    key = (char*) authDetails;
    resp = http->response;

    while (*key) {
        while (*key && isspace((int) *key)) {
            key++;
        }
        tok = key;
        while (*tok && !isspace((int) *tok) && *tok != ',' && *tok != '=') {
            tok++;
        }
        *tok++ = '\0';

        while (isspace((int) *tok)) {
            tok++;
        }
        seenComma = 0;
        if (*tok == '\"') {
            value = ++tok;
            while (*tok != '\"' && *tok != '\0') {
                tok++;
            }
        } else {
            value = tok;
            while (*tok != ',' && *tok != '\0') {
                tok++;
            }
            seenComma++;
        }
        *tok++ = '\0';

        /*
         *  Handle back-quoting
         */
        if (strchr(value, '\\')) {
            for (dp = sp = value; *sp; sp++) {
                if (*sp == '\\') {
                    sp++;
                }
                *dp++ = *sp++;
            }
            *dp = '\0';
        }

        /*
         *  algorithm, domain, nonce, oqaque, realm, qop, stale
         *  We don't strdup any of the values as the headers are persistently saved.
         */
        switch (tolower((int) *key)) {
        case 'a':
            if (mprStrcmpAnyCase(key, "algorithm") == 0) {
                mprFree(resp->authAlgorithm);
                resp->authAlgorithm = value;
                break;
#if UNUSED
            } else if (mprStrcmpAnyCase(key, "server-param") == 0) {
                break;
#endif
            }
            break;

        case 'd':
            if (mprStrcmpAnyCase(key, "domain") == 0) {
                mprFree(http->authDomain);
                http->authDomain = mprStrdup(http, value);
                break;
            }
            break;

        case 'n':
            if (mprStrcmpAnyCase(key, "nonce") == 0) {
                mprFree(http->authNonce);
                http->authNonce = mprStrdup(http, value);
                resp->http->authNc = 0;
            }
            break;

        case 'o':
            if (mprStrcmpAnyCase(key, "opaque") == 0) {
                mprFree(http->authOpaque);
                http->authOpaque = mprStrdup(http, value);
            }
            break;

        case 'q':
            if (mprStrcmpAnyCase(key, "qop") == 0) {
                mprFree(http->authQop);
                http->authQop = mprStrdup(http, value);
            }
            break;

        case 'r':
            if (mprStrcmpAnyCase(key, "realm") == 0) {
                mprFree(http->authRealm);
                http->authRealm = mprStrdup(http, value);
            }
            break;

        case 's':
            if (mprStrcmpAnyCase(key, "stale") == 0) {
                resp->authStale = mprStrdup(resp, value);
                break;
            }

        default:
            /*  For upward compatibility --  ignore keywords we don't understand */
            ;
        }
        key = tok;
        if (!seenComma) {
            while (*key && *key != ',') {
                key++;
            }
            if (*key) {
                key++;
            }
        }
    }
    if (strcmp(resp->http->authType, "basic") == 0) {
        if (http->authRealm == 0) {
            return MPR_ERR_BAD_ARGS;
        } else {
            return 0;
        }
    }
    if (http->authRealm == 0 || http->authNonce == 0) {
        return MPR_ERR_BAD_ARGS;
    }
    if (http->authQop) {
        /*  FUTURE -- add checking for auth-int here */
        if (http->authDomain == 0 || http->authOpaque == 0 || resp->authAlgorithm == 0 || resp->authStale == 0) {
            return MPR_ERR_BAD_ARGS;
        }
    }
    return 0;
}


#if BLD_DEBUG
static void traceResponseData(MprCtx ctx, cchar *src, int size)
{
    char    dest[512];
    int     index, i;

    mprRawLog(ctx, 5, "@@@ Response data => \n");

    for (index = 0; index < size; ) { 
        for (i = 0; i < (sizeof(dest) - 1) && index < size; i++) {
            dest[i] = src[index];
            index++;
        }
        dest[i] = '\0';
        mprRawLog(ctx, 5, "%s", dest);
    }
    mprRawLog(ctx, 5, "\n");
}
#endif


/*
 *  Fail a request. If keepAlive is zero, the connection will be closed.
 */
static void failRequest(MprHttp *http, cchar *fmt, ...)
{
    va_list     args;

    mprAssert(fmt);

    if (http->error) {
        mprFree(http->error);
    }

    va_start(args, fmt);
    mprAllocVsprintf(http, &http->error, MPR_MAX_STRING, fmt, args);
    va_end(args);

    mprLog(http, 3, "Http: failRequest: %s", http->error);

    if (http->response) {
        http->keepAlive = 0;
    }
    completeRequest(http);
}


/*
 *  Complete a request. And prepare the http object for a new request.
 */
static void completeRequest(MprHttp *http)
{
    if (http->sock) {
        if (http->keepAlive) {
            mprLog(http, 4, "Http: completeRequest: Attempting keep-alive");
        } else {
            closeConnection(http);
        }
    }

    if (http->state != MPR_HTTP_STATE_BEGIN) {
        http->state = MPR_HTTP_STATE_COMPLETE;
        if (http->callback) {
            (http->callback)(http, 0);
        }
        http->state = MPR_HTTP_STATE_BEGIN;
    }
}


int mprGetHttpCode(MprHttp *http)
{
    if (http->response == 0) {
        return 0;
    }
    return http->response->code;
}


cchar *mprGetHttpMessage(MprHttp *http)
{
    if (http->response == 0) {
        return 0;
    }
    return http->response->message;
}


int mprGetHttpContentLength(MprHttp *http)
{
    MprHttpResponse     *resp;

    mprAssert(http);

    resp = http->response;

    if (resp == 0) {
        return 0;
    }
    return resp->length;
}


//  TODO - is this the best name?  mprGetHttpResponseData may be better?
cchar *mprGetHttpContent(MprHttp *http)
{
    MprHttpResponse     *resp;

    mprAssert(http);

    resp = http->response;

    if (resp == 0) {
        return 0;
    }
    mprAddNullToBuf(resp->content);
    return mprGetBufStart(resp->content);
}


cchar *mprGetHttpHeader(MprHttp *http, cchar *key)
{
    MprHttpResponse     *resp;

    resp = http->response;

    if (resp == 0) {
        return 0;
    }
    return (cchar*) mprLookupHash(resp->headers, key);
}



char *mprGetHttpHeaders(MprHttp *http)
{
    MprHttpResponse     *resp;
    MprHash             *hp;
    char                *headers, *key, *cp;
    int                 len, oldlen;

    resp = http->response;

    if (resp == 0) {
        return 0;
    }

    headers = 0;
    for (len = 0, hp = mprGetFirstHash(resp->headers); hp; ) {
        oldlen = len;
        len = mprReallocStrcat(http, &headers, -1, len, 0, hp->key, 0);
        key = &headers[oldlen];
        for (cp = &key[1]; *cp; cp++) {
            *cp = tolower((int) *cp);
            if (*cp == '-') {
                cp++;
            }
        }
        len = mprReallocStrcat(http, &headers, -1, len, 0, ": ", hp->data, "\n", 0);
        hp = mprGetNextHash(resp->headers, hp);
    }
    return headers;
}


MprHashTable *mprGetHttpHeadersHash(MprHttp *http)
{
    MprHttpResponse     *resp;

    resp = http->response;

    if (resp == 0) {
        return 0;
    }
    return resp->headers;
}


cchar *mprGetHttpError(MprHttp *http)
{
    if (http->error) {
        return http->error;

    } else if (http->response) {
        return mprGetHttpCodeString(http, http->response->code);

    } else {
        return "";
    }
}


void mprSetHttpProxy(MprHttp *http, cchar *host, int port)
{
    mprFree(http->proxyHost);
    http->proxyHost = mprStrdup(http, host);
    http->proxyPort = port;
}


void mprSetHttpCallback(MprHttp *http, MprHttpProc fn, void *arg)
{
    http->callback = fn;
    http->callbackArg = arg;
}


/*
 *  If maxSize is set to 0, this means don't buffer the response content and instead progressively call the callback.
 *  Set maxSize to -1 to mean unlimited.
 */
void mprSetHttpBufferSize(MprHttp *http, int initialSize, int maxSize)
{
    http->bufsize = initialSize;
    http->bufmax = maxSize;
}


int mprGetHttpState(MprHttp *http)
{
    return http->state;
}


int mprGetHttpFlags(MprHttp *http)
{
    return http->response->flags;
}


void mprSetHttpKeepAlive(MprHttp *http, bool on)
{
    http->useKeepAlive = on;
    http->keepAlive = on;
}


void mprSetHttpProtocol(MprHttp *http, cchar *protocol)
{
    mprFree(http->protocol);
    http->protocol = mprStrdup(http, protocol);
    if (strcmp(http->protocol, "HTTP/1.0") == 0) {
        http->useKeepAlive = 0;
        http->keepAlive = 0;
        http->protocolVersion = 0;
    }
}


void mprSetHttpRetries(MprHttp *http, int num)
{
    http->retries = num;
}


int mprGetHttpDefaultPort(MprHttp *http)
{
    return http->defaultPort;
}


cchar *mprGetHttpDefaultHost(MprHttp *http)
{
    return http->defaultHost;
}


void mprSetHttpDefaultPort(MprHttp *http, int num)
{
    http->defaultPort = num;
}


void mprSetHttpDefaultHost(MprHttp *http, cchar *host)
{
    mprFree(http->defaultHost);
    http->defaultHost = mprStrdup(http, host);
}


void mprSetHttpCredentials(MprHttp *http, cchar *username, cchar *password)
{
    mprResetHttpCredentials(http);
    http->user = mprStrdup(http, username);
    http->password = mprStrdup(http, password);
}


void mprResetHttpCredentials(MprHttp *http)
{
    mprFree(http->user);
    mprFree(http->password);
    mprFree(http->authDomain);
    mprFree(http->authCnonce);
    mprFree(http->authNonce);
    mprFree(http->authOpaque);
    mprFree(http->authRealm);
    mprFree(http->authQop);
    mprFree(http->authType);

    http->user = 0;
    http->password = 0;
    http->authType = 0;
    http->authDomain = 0;
    http->authCnonce = 0;
    http->authNonce = 0;
    http->authOpaque = 0;
    http->authRealm = 0;
    http->authQop = 0;
    http->authType = 0;
}



void mprSetHttpFollowRedirects(MprHttp *http, bool follow)
{
    http->followRedirects = follow;
}


int mprSetHttpHeader(MprHttp *http, cchar *key, cchar *value, bool overwrite)
{
    MprHttpRequest  *req;
    char            *persistKey;

    req = http->request;

    if (req->headers == 0) {
        req->headers = mprCreateHash(req, -1);
    }
    persistKey = mprStrdup(req, key);
    if (overwrite) {
        mprAddHash(req->headers, persistKey, value);
    } else {
        mprAddDuplicateHash(req->headers, persistKey, value);
    }
    return 0;
}


void mprSetHttpTimeout(MprHttp *http, int timeout)
{
    http->timeoutPeriod = timeout;
}


/*
 *  Create a random secret for use in authentication. Create once for the entire http service. Created on demand.
 *  Users can recall as required to update.
 */
int mprCreateHttpSecret(MprCtx ctx)
{
    MprHttpService  *hs;
    char            *hex = "0123456789abcdef";
    uchar           bytes[MPR_HTTP_MAX_SECRET];
    char            ascii[MPR_HTTP_MAX_SECRET * 2 + 1], *ap;
    int             i;

    hs = mprGetMpr(ctx)->httpService;

    if (mprGetRandomBytes(hs, bytes, sizeof(bytes), 0) < 0) {
        mprAssert(0);
        return MPR_ERR_CANT_INITIALIZE;
    }

    ap = ascii;
    for (i = 0; i < (int) sizeof(bytes); i++) {
        *ap++ = hex[bytes[i] >> 4];
        *ap++ = hex[bytes[i] & 0xf];
    }
    *ap = '\0';

    hs->secret = mprStrdup(hs, ascii);

    return 0;
}


/*
 *  Get the next input token. The conn->input content buffer is advanced to the next token. This routine
 *  always returns a non-zero token. The empty string means the delimiter was not found.
 */
static char *getHttpToken(MprBuf *buf, cchar *delim)
{
    char    *token, *nextToken;
    int     len;

    len = mprGetBufLength(buf);
    if (len == 0) {
        return "";
    }

    token = mprGetBufStart(buf);
    nextToken = mprStrnstr(mprGetBufStart(buf), delim, len);
    if (nextToken) {
        *nextToken = '\0';
        len = (int) strlen(delim);
        nextToken += len;
        buf->start = (uchar*) nextToken;

    } else {
        buf->start = (uchar*) mprGetBufEnd(buf);
    }
    return token;
}


static bool getChunkSize(MprHttp *http, MprBuf *buf, int *boundaryLen, int *size)
{
    char    *start, *nextContent;
    int     available;

    /*
     *  Must at least have "\r\nDIGIT\r\n"
     */
    start = mprGetBufStart(buf), 
    available = (int) (mprGetBufEnd(buf) - start);
    mprAssert(available >= 0);

    if (available < 5) {
        return 0;
    }

    /*
     *  Chunk delimiter is: "\r\nHEX_COUNT; chunk length DECIMAL_COUNT\r\n". The "; chunk length DECIMAL_COUNT is optional.
     *  Step over the existing content and get the next chunk count.
     */
    if (start[0] != '\r' || start[1] != '\n') {
        failRequest(http, "Bad chunk specification");
        return 0;
    }
    nextContent = mprStrnstr(start + 2, "\r\n", available);
    if (nextContent == 0) {
        if (available > 80) {
            failRequest(http, "Bad chunk specification");
        }
        return 0;
    }
    nextContent += 2;

    //  TODO - should sanity check this against limits
    *boundaryLen = (int) (nextContent - start);

    *size = mprAtoi(start + 2, 16);
    if (*size < 0) {
        failRequest(http, "Bad chunk size");
        return 0;
    }

#if BLD_DEBUG
    nextContent[-2] = '\0';
    mprRawLog(http, 5, "\r\nContent Boundary length %d", *size);
    mprRawLog(http, 4, "%s", start);
    nextContent[-2] = '\r';
#endif
    return 1;
}


#endif /* BLD_FEATURE_HTTP_CLIENT */

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
/************************************************************************/
/*
 *  End of file "../mprHttp.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../mprList.c"
 */
/************************************************************************/

/**
 *  mprList.c - Simple list type.
 *
 *  The list supports two modes of operation. Compact mode where the list is compacted after removing list items, 
 *  and no-compact mode where removed items are zeroed. No-compact mode implies that all valid list entries must 
 *  be non-zero.
 *
 *  Most routines in this file are not thread-safe. It is the callers responsibility to perform all thread 
 *  synchronization.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/*
 *  TODO - add push pop routines
 */



static int growList(MprList *lp, int incr);

/*
 *  Create a general growable list structure. Use mprFree to destroy.
 */

MprList *mprCreateList(MprCtx ctx)
{
    MprList     *lp;

    lp = mprAllocObj(ctx, MprList);
    if (lp == 0) {
        return 0;
    }

    lp->capacity = 0;
    lp->length = 0;
    lp->maxSize = MAXINT;
    lp->items = 0;

    return lp;
}


/*
 *  Initialize a list which may not be a memory context.
 */
void mprInitList(MprList *lp)
{
    lp->capacity = 0;
    lp->length = 0;
    lp->maxSize = MAXINT;
    lp->items = 0;
}


/*
 *  Define the list maximum size. If the list has not yet been written to, the initialSize will be observed.
 */
int mprSetListLimits(MprList *lp, int initialSize, int maxSize)
{
    int         size;

    if (initialSize <= 0) {
        initialSize = MPR_LIST_INCR;
    }
    if (maxSize <= 0) {
        maxSize = MAXINT;
    }
    size = initialSize * sizeof(void*);

    if (lp->items == 0) {
        lp->items = (void**) mprAllocZeroed(lp, size);

        if (lp->items == 0) {
            mprFree(lp);
            return MPR_ERR_NO_MEMORY;
        }
        lp->capacity = initialSize;
    }

    lp->maxSize = maxSize;

    return 0;
}


int mprCopyList(MprList *dest, MprList *src)
{
    void        *item;
    int         next;

    mprClearList(dest);

    if (mprSetListLimits(dest, src->capacity, src->maxSize) < 0) {
        return MPR_ERR_NO_MEMORY;
    }

    for (next = 0; (item = mprGetNextItem(src, &next)) != 0; ) {
        if (mprAddItem(dest, item) < 0) {
            return MPR_ERR_NO_MEMORY;
        }
    }
    return 0;
}


MprList *mprDupList(MprCtx ctx, MprList *src)
{
    MprList     *list;

    list = mprCreateList(ctx);
    if (list == 0) {
        return 0;
    }

    if (mprCopyList(list, src) < 0) {
        mprFree(list);
        return 0;
    }
    return list;
}


MprList *mprAppendList(MprList *list, MprList *add)
{
    void        *item;
    int         next;

    mprAssert(list);

    for (next = 0; ((item = mprGetNextItem(add, &next)) != 0); ) {
        if (mprAddItem(list, item) < 0) {
            mprFree(list);
            return 0;
        }
    }
    return list;
}


/*
 *  Change the item in the list at index. Return the old item.
 */
void *mprSetItem(MprList *lp, int index, cvoid *item)
{
    void    *old;

    mprAssert(lp);
    mprAssert(lp->capacity >= 0);
    mprAssert(lp->length >= 0);

    if (index >= lp->length) {
        lp->length = index + 1;
    }
    if (lp->length > lp->capacity) {
        if (growList(lp, lp->length - lp->capacity) < 0) {
            return 0;
        }
    }

    old = lp->items[index];
    lp->items[index] = (void*) item;

    return old;
}



/*
 *  Add an item to the list and return the item index.
 */
int mprAddItem(MprList *lp, cvoid *item)
{
    int     index;

    mprAssert(lp);
    mprAssert(lp->capacity >= 0);
    mprAssert(lp->length >= 0);

    if (lp->length >= lp->capacity) {
        if (growList(lp, 1) < 0) {
            return MPR_ERR_TOO_MANY;
        }
    }

    index = lp->length++;
    lp->items[index] = (void*) item;

    return index;
}


/*
 *  Insert an item to the list at a specified position. We insert before "index".
 */
int mprInsertItemAtPos(MprList *lp, int index, cvoid *item)
{
    void    **items;
    int     i;

    mprAssert(lp);
    mprAssert(lp->capacity >= 0);
    mprAssert(lp->length >= 0);
    mprAssert(index >= 0);

    if (index < 0) {
        index = 0;
    }
    if (index >= lp->capacity) {
        if (growList(lp, index - lp->capacity + 1) < 0) {
            return MPR_ERR_TOO_MANY;
        }

    } else if (lp->length >= lp->capacity) {
        if (growList(lp, 1) < 0) {
            return MPR_ERR_TOO_MANY;
        }
    }

    if (index >= lp->length) {
        lp->length = index + 1;

    } else {
        /*
         *  Copy up items to make room to insert
         */
        items = lp->items;
        for (i = lp->length; i > index; i--) {
            items[i] = items[i - 1];
        }
        lp->length++;
    }

    lp->items[index] = (void*) item;

    return index;
}


/*
 *  Remove an item from the list. Return the index where the item resided.
 */
int mprRemoveItem(MprList *lp, void *item)
{
    int     index;

    mprAssert(lp);
    mprAssert(lp->capacity > 0);
    mprAssert(lp->length > 0);

    index = mprLookupItem(lp, item);
    if (index < 0) {
        return index;
    }

    return mprRemoveItemAtPos(lp, index);
}


int mprRemoveLastItem(MprList *lp)
{
    mprAssert(lp);
    mprAssert(lp->capacity > 0);
    mprAssert(lp->length > 0);

    if (lp->length <= 0) {
        return MPR_ERR_NOT_FOUND;
    }
    return mprRemoveItemAtPos(lp, lp->length - 1);
}


/*
 *  Remove an index from the list. Return the index where the item resided.
 */
int mprRemoveItemAtPos(MprList *lp, int index)
{
    void    **items;
    int     i;

    mprAssert(lp);
    mprAssert(lp->capacity > 0);
    mprAssert(index >= 0 && index < lp->capacity);
    mprAssert(lp->length > 0);

    if (index < 0 || index >= lp->length) {
        return MPR_ERR_NOT_FOUND;
    }

    items = lp->items;
    for (i = index; i < (lp->length - 1); i++) {
        items[i] = items[i + 1];
    }
    lp->length--;
    lp->items[lp->length] = 0;

    return index;
}


/*
 *  Remove a set of items. Return 0 if successful.
 */
int mprRemoveRangeOfItems(MprList *lp, int start, int end)
{
    void    **items;
    int     i, count;

    mprAssert(lp);
    mprAssert(lp->capacity > 0);
    mprAssert(lp->length > 0);
    mprAssert(start > end);

    if (start < 0 || start >= lp->length) {
        return MPR_ERR_NOT_FOUND;
    }
    if (end < 0 || end >= lp->length) {
        return MPR_ERR_NOT_FOUND;
    }
    if (start > end) {
        return MPR_ERR_BAD_ARGS;
    }

    /*
     *  Copy down to compress
     */
    items = lp->items;
    count = end - start;
    for (i = start; i < (lp->length - count); i++) {
        items[i] = items[i + count];
    }
    lp->length -= count;
    for (i = lp->length; i < lp->capacity; i++) {
        items[i] = 0;
    }

    return 0;
}


void *mprGetItem(MprList *lp, int index)
{
    mprAssert(lp);

    if (index < 0 || index >= lp->length) {
        return 0;
    }
    return lp->items[index];
}


void *mprGetFirstItem(MprList *lp)
{
    mprAssert(lp);

    if (lp == 0) {
        return 0;
    }

    if (lp->length == 0) {
        return 0;
    }
    return lp->items[0];
}


void *mprGetLastItem(MprList *lp)
{
    mprAssert(lp);

    if (lp == 0) {
        return 0;
    }

    if (lp->length == 0) {
        return 0;
    }
    return lp->items[lp->length - 1];
}


void *mprGetNextItem(MprList *lp, int *next)
{
    void    *item;
    int     index;

    mprAssert(next);
    mprAssert(*next >= 0);

    if (lp == 0) {
        return 0;
    }

    index = *next;

    if (index < lp->length) {
        item = lp->items[index];
        *next = ++index;
        return item;
    }
    return 0;
}


void *mprGetPrevItem(MprList *lp, int *next)
{
    int     index;

    mprAssert(next);

    if (lp == 0) {
        return 0;
    }

    if (*next < 0) {
        *next = lp->length;
    }
    index = *next;

    if (--index < lp->length && index >= 0) {
        *next = index;
        return lp->items[index];
    }
    return 0;
}


int mprGetListCount(MprList *lp)
{
    if (lp == 0) {
        return 0;
    }

    return lp->length;
}


int mprGetListCapacity(MprList *lp)
{
    mprAssert(lp);

    if (lp == 0) {
        return 0;
    }

    return lp->capacity;
}


void mprClearList(MprList *lp)
{
    int     i;

    mprAssert(lp);

    for (i = 0; i < lp->length; i++) {
        lp->items[i] = 0;
    }
    lp->length = 0;
}


int mprLookupItem(MprList *lp, cvoid *item)
{
    int     i;

    mprAssert(lp);
    
    for (i = 0; i < lp->length; i++) {
        if (lp->items[i] == item) {
            return i;
        }
    }
    return MPR_ERR_NOT_FOUND;
}


/*
 *  Grow the list by the requried increment
 */
static int growList(MprList *lp, int incr)
{
    int     len, memsize;

    if (lp->maxSize <= 0) {
        lp->maxSize = MAXINT;
    }

    /*
     *  Need to grow the list
     */
    if (lp->capacity >= lp->maxSize) {
        mprAssert(lp->capacity < lp->maxSize);
        return MPR_ERR_TOO_MANY;
    }

    /*
     *  If growing by 1, then use the default increment which exponentially grows. Otherwise, assume the caller knows exactly
     *  how much the list needs to grow.
     */
    if (incr <= 1) {
        len = MPR_LIST_INCR + (lp->capacity * 2);
    } else {
        len = lp->capacity + incr;
    }
    memsize = len * sizeof(void*);

    /*
     *  Grow the list of items. Use the existing context for lp->items if it already exists. Otherwise use the list as the
     *  memory context owner.
     */
    lp->items = (void**) mprRealloc((lp->items) ? mprGetParent(lp->items): lp, lp->items, memsize);

    /*
     *  Zero the new portion (required for no-compact lists)
     */
    memset(&lp->items[lp->capacity], 0, sizeof(void*) * (len - lp->capacity));
    lp->capacity = len;

    return 0;
}


void mprSortList(MprList *lp, MprListCompareProc compare)
{
    qsort(lp->items, lp->length, sizeof(void*), compare);
}


MprKeyValue *mprCreateKeyPair(MprCtx ctx, cchar *key, cchar *value)
{
    MprKeyValue     *pair;
    
    pair = mprAllocObj(ctx, MprKeyValue);
    if (pair == 0) {
        return 0;
    }
    pair->key = mprStrdup(pair, key);
    pair->value = mprStrdup(pair, value);
    return pair;
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
/************************************************************************/
/*
 *  End of file "../mprList.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../mprLock.c"
 */
/************************************************************************/

/**
 *  mprLock.c - Thread Locking Support
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



#if BLD_FEATURE_MULTITHREAD

static int destroyLock(MprMutex *lock);
static int destroySpinLock(MprSpin *lock);


MprMutex *mprCreateLock(MprCtx ctx)
{
    MprMutex    *lock;

    mprAssert(ctx);

    lock = mprAllocObjWithDestructor(ctx, MprMutex, destroyLock);
    if (lock == 0) {
        return 0;
    }

#if BLD_UNIX_LIKE
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&lock->cs, &attr);
    pthread_mutexattr_destroy(&attr);
#elif BLD_WIN_LIKE
    InitializeCriticalSectionAndSpinCount(&lock->cs, 5000);
#elif VXWORKS
    /* Removed SEM_INVERSION_SAFE */
    lock->cs = semMCreate(SEM_Q_PRIORITY | SEM_DELETE_SAFE);
    if (lock->cs == 0) {
        mprAssert(0);
        mprFree(lock);
        return 0;
    }
#endif
    return lock;
}


MprMutex *mprCreateStaticLock(MprCtx ctx, MprMutex *lock)
{
    mprAssert(ctx);

#if BLD_UNIX_LIKE
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&lock->cs, &attr);
    pthread_mutexattr_destroy(&attr);
#elif BLD_WIN_LIKE
    InitializeCriticalSectionAndSpinCount(&lock->cs, 5000);
#elif VXWORKS
    /* Removed SEM_INVERSION_SAFE */
    lock->cs = semMCreate(SEM_Q_PRIORITY | SEM_DELETE_SAFE);
    if (lock->cs == 0) {
        mprAssert(0);
        mprFree(lock);
        return 0;
    }
#endif
    return lock;
}


/*
 *  Destroy a lock. Must be locked on entrance.
 */
static int destroyLock(MprMutex *lock)
{
    mprAssert(lock);
#if BLD_UNIX_LIKE
    pthread_mutex_unlock(&lock->cs);
    pthread_mutex_destroy(&lock->cs);
#elif BLD_WIN_LIKE
    DeleteCriticalSection(&lock->cs);
#elif VXWORKS
    semDelete(lock->cs);
#endif
    return 0;
}



#if !BLD_USE_LOCK_MACROS
/*
 *  Lock a mutex
 */
void mprLock(MprMutex *lock)
{
#if BLD_UNIX_LIKE
    pthread_mutex_lock(&lock->cs);
#elif BLD_WIN_LIKE
    EnterCriticalSection(&lock->cs);
#elif VXWORKS
    semTake(lock->cs, WAIT_FOREVER);
#endif
}


void mprUnlock(MprMutex *lock)
{
#if BLD_UNIX_LIKE
    pthread_mutex_unlock(&lock->cs);
#elif BLD_WIN_LIKE
    LeaveCriticalSection(&lock->cs);
#elif VXWORKS
    semGive(lock->cs);
#endif
}
#endif




/*
 *  Try to attain a lock. Do not block! Returns true if the lock was attained.
 */
bool mprTryLock(MprMutex *lock)
{
    int     rc;
#if BLD_UNIX_LIKE
    rc = pthread_mutex_trylock(&lock->cs) != 0;
#elif BLD_WIN_LIKE
    rc = TryEnterCriticalSection(&lock->cs) == 0;
#elif VXWORKS
    rc = semTake(lock->cs, NO_WAIT) != OK;
#endif
    return (rc) ? 0 : 1;
}



MprSpin *mprCreateSpinLock(MprCtx ctx)
{
    MprSpin    *lock;

    mprAssert(ctx);

    lock = mprAllocObjWithDestructor(ctx, MprSpin, destroySpinLock);
    if (lock == 0) {
        return 0;
    }

#if USE_MPR_LOCK
    mprCreateStaticLock(ctx, &lock->cs);

#elif MACOSX
    lock->cs = OS_SPINLOCK_INIT;

#elif BLD_UNIX_LIKE && BLD_HAS_SPINLOCK
    pthread_spin_init(&lock->cs, 0);

#elif BLD_UNIX_LIKE
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&lock->cs, &attr);
    pthread_mutexattr_destroy(&attr);

#elif BLD_WIN_LIKE
    InitializeCriticalSectionAndSpinCount(&lock->cs, 5000);

#elif VXWORKS
    /* Removed SEM_INVERSION_SAFE */
    lock->cs = semMCreate(SEM_Q_PRIORITY | SEM_DELETE_SAFE);
    if (lock->cs == 0) {
        mprAssert(0);
        mprFree(lock);
        return 0;
    }
#endif

#if BLD_DEBUG
    lock->owner = 0;
#endif

    return lock;
}



/*
 *  Static version just for mprAlloc which needs locks that don't allocate memory.
 */
MprSpin *mprCreateStaticSpinLock(MprCtx ctx, MprSpin *lock)
{
    mprAssert(ctx);

#if USE_MPR_LOCK
    mprCreateStaticLock(ctx, &lock->cs);

#elif MACOSX
    lock->cs = OS_SPINLOCK_INIT;

#elif BLD_UNIX_LIKE && BLD_HAS_SPINLOCK
    pthread_spin_init(&lock->cs, 0);

#elif BLD_UNIX_LIKE
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&lock->cs, &attr);
    pthread_mutexattr_destroy(&attr);

#elif BLD_WIN_LIKE
    InitializeCriticalSectionAndSpinCount(&lock->cs, 5000);

#elif VXWORKS
    /* Removed SEM_INVERSION_SAFE */
    lock->cs = semMCreate(SEM_Q_PRIORITY | SEM_DELETE_SAFE);
    if (lock->cs == 0) {
        mprAssert(0);
        mprFree(lock);
        return 0;
    }
#endif

#if BLD_DEBUG
    lock->owner = 0;
#endif

    return lock;
}


/*
 *  Destroy a lock. Must be locked on entrance.
 */
static int destroySpinLock(MprSpin *lock)
{
    mprAssert(lock);
#if USE_MPR_LOCK
    ;
#elif MACOSX
    ;
#elif BLD_UNIX_LIKE && BLD_HAS_SPINLOCK
    pthread_spin_destroy(&lock->cs);
#elif BLD_UNIX_LIKE
    pthread_mutex_destroy(&lock->cs);
#elif BLD_WIN_LIKE
    DeleteCriticalSection(&lock->cs);
#elif VXWORKS
    semDelete(lock->cs);
#endif
    return 0;
}



/*
 *  Use functions for debug mode. Production release uses macros
 */
#if BLD_DEBUG
/*
 *  Lock a mutex
 */
void mprSpinLock(MprSpin *lock)
{
    
#if BLD_DEBUG
    /*
     *  Spin locks don't support recursive locking
     */
    mprAssert(lock->owner != mprGetCurrentOsThread());
#endif

#if USE_MPR_LOCK
    mprLock(&lock->cs);
#elif MACOSX
    OSSpinLockLock(&lock->cs);
#elif BLD_UNIX_LIKE && BLD_HAS_SPINLOCK
    pthread_spin_lock(&lock->cs);
#elif BLD_UNIX_LIKE
    pthread_mutex_lock(&lock->cs);
#elif BLD_WIN_LIKE
    EnterCriticalSection(&lock->cs);
#elif VXWORKS
    semTake(lock->cs, WAIT_FOREVER);
#endif

#if BLD_DEBUG
    mprAssert(lock->owner != mprGetCurrentOsThread());
    lock->owner = mprGetCurrentOsThread();
#endif
}



void mprSpinUnlock(MprSpin *lock)
{
#if BLD_DEBUG
    lock->owner = 0;
#endif

#if USE_MPR_LOCK
    mprUnlock(&lock->cs);
#elif MACOSX
    OSSpinLockUnlock(&lock->cs);
#elif BLD_UNIX_LIKE && BLD_HAS_SPINLOCK
    pthread_spin_unlock(&lock->cs);
#elif BLD_UNIX_LIKE
    pthread_mutex_unlock(&lock->cs);
#elif BLD_WIN_LIKE
    LeaveCriticalSection(&lock->cs);
#elif VXWORKS
    semGive(lock->cs);
#endif
}
#endif



/*
 *  Try to attain a lock. Do not block! Returns true if the lock was attained.
 */
bool mprTrySpinLock(MprSpin *lock)
{
    int     rc;

#if USE_MPR_LOCK
    mprTryLock(&lock->cs);
#elif MACOSX
    rc = !OSSpinLockTry(&lock->cs);
#elif BLD_UNIX_LIKE && BLD_HAS_SPINLOCK
    rc = pthread_spin_trylock(&lock->cs) != 0;
#elif BLD_UNIX_LIKE
    rc = pthread_mutex_trylock(&lock->cs) != 0;
#elif BLD_WIN_LIKE
    rc = TryEnterCriticalSection(&lock->cs) == 0;
#elif VXWORKS
    rc = semTake(lock->cs, NO_WAIT) != OK;
#endif
#if BLD_DEBUG
    if (rc == 0) {
        mprAssert(lock->owner != mprGetCurrentOsThread());
        lock->owner = mprGetCurrentOsThread();
    }
#endif
    return (rc) ? 0 : 1;
}



/*
 *  Big global lock. Avoid using this.
 */
void mprGlobalLock(MprCtx ctx)
{
    Mpr *mpr;

    mpr = mprGetMpr(ctx);
    mprAssert(mpr);

    if (mpr && mpr->mutex) {
        mprLock(mpr->mutex);
    }
}



void mprGlobalUnlock(MprCtx ctx)
{
    Mpr *mpr;

    mpr = mprGetMpr(ctx);
    mprAssert(mpr);

    if (mpr && mpr->mutex) {
        mprUnlock(mpr->mutex);
    }
}


#else /* BLD_FEATURE_MULTITHREAD */
void __dummyMprLock() {}
#endif /* BLD_FEATURE_MULTITHREAD */

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
/************************************************************************/
/*
 *  End of file "../mprLock.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../mprLog.c"
 */
/************************************************************************/

/**
 *  mprLog.c - Michael's Portable Runtime (MPR) Logging and error reporting.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */




static void defaultLogHandler(MprCtx ctx, int flags, int level, cchar *msg);
static void logOutput(MprCtx ctx, int flags, int level, cchar *msg);

/*
 *  Put first in file so it is easy to locate in a debugger
 */
void mprBreakpoint()
{
#if BLD_DEBUG && DEBUG_IDE
#if BLD_HOST_CPU_ARCH == MPR_CPU_IX86 || BLD_HOST_CPU_ARCH == MPR_CPU_IX64
#if BLD_WIN_LIKE
    __asm { int 3 };
#else
    asm("int $03");
    /*  __asm__ __volatile__ ("int $03"); */
#endif
#endif
#endif
}


void mprLog(MprCtx ctx, int level, cchar *fmt, ...)
{
    va_list     args;
    char        *buf;

    mprAssert(ctx);

    if (level > mprGetLogLevel(ctx)) {
        return;
    }

    va_start(args, fmt);
    mprAllocVsprintf(ctx, &buf, 0, fmt, args);
    va_end(args);

    logOutput(ctx, MPR_LOG_SRC, level, buf);

    va_end(args);
    mprFree(buf);
}


/*
 *  Do raw output
 */
void mprRawLog(MprCtx ctx, int level, cchar *fmt, ...)
{
    va_list     args;
    char        *buf;
    int         len;

    if (level > mprGetLogLevel(ctx)) {
        return;
    }

    va_start(args, fmt);
    len = mprAllocVsprintf(ctx, &buf, 0, fmt, args);
    va_end(args);
    
    logOutput(ctx, MPR_RAW, 0, buf);
    mprFree(buf);
}


/*
 *  Handle an error
 */
void mprError(MprCtx ctx, cchar *fmt, ...)
{
    va_list     args;
    char        *buf;
    int         len;

    va_start(args, fmt);
    len = mprAllocVsprintf(ctx, &buf, 0, fmt, args);
    va_end(args);
    
    logOutput(ctx, MPR_ERROR_MSG | MPR_ERROR_SRC, 0, buf);

    mprFree(buf);
    mprBreakpoint();
}


/*
 *  Handle a memory allocation error
 */
void mprMemoryError(MprCtx ctx, cchar *fmt, ...)
{
    va_list     args;
    char        *buf;
    int         len;

    if (fmt == 0) {
        len = mprAllocSprintf(ctx, &buf, 0, "Memory allocation error");
    } else {
        va_start(args, fmt);
        len = mprAllocVsprintf(ctx, &buf, 0, fmt, args);
        va_end(args);
    }
    
    logOutput(ctx, MPR_ERROR_MSG | MPR_ERROR_SRC, 0, buf);

    mprFree(buf);
}


/*
 *  Handle an error that should be displayed to the user
 */
void mprUserError(MprCtx ctx, cchar *fmt, ...)
{
    va_list     args;
    char        *buf;
    int         len;

    va_start(args, fmt);
    len = mprAllocVsprintf(ctx, &buf, 0, fmt, args);
    va_end(args);
    
    logOutput(ctx, MPR_USER_MSG | MPR_ERROR_SRC, 0, buf);

    mprFree(buf);
}


/*
 *  Handle a fatal error. Forcibly shutdown the application.
 */
void mprFatalError(MprCtx ctx, cchar *fmt, ...)
{
    va_list     args;
    char        *buf;
    int         len;

    va_start(args, fmt);
    len = mprAllocVsprintf(ctx, &buf, 0, fmt, args);
    va_end(args);
    
    logOutput(ctx, MPR_USER_MSG | MPR_FATAL_SRC, 0, buf);

    mprFree(buf);

#if BREW
    mprSignalExit(ctx);
#else
    exit(2);
#endif
}


/*
 *  Handle an error without allocating memory.
 */
void mprStaticError(MprCtx ctx, cchar *fmt, ...)
{
    va_list     args;
    int         len;
    char        buf[MPR_MAX_STRING];

    va_start(args, fmt);
    len = mprVsprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    logOutput(ctx, MPR_ERROR_MSG | MPR_ERROR_SRC, 0, buf);
}


/*
 *  Direct output to the standard error. Does not hook into the logging system and does not allocate memory.
 */
void mprStaticAssert(cchar *loc, cchar *msg)
{
#if BLD_DEBUG
    char    buf[MPR_MAX_STRING];
    int     len;

    len = mprSprintf(buf, sizeof(buf), "Assertion %s, failed at %s\n", msg, loc);
    
#if BLD_UNIX_LIKE
    write(2, buf, len);
#elif BREW || BLD_WIN_LIKE
    /*
     *  Only time we use printf. We can't get an alloc context so we have to use real print
     */
#if BREW && !BREWSIM
    fprintf(stderr, " MP: %s\n", buf);
#else
    fprintf(stderr, "%s\n", buf);
#endif

#endif
    mprBreakpoint();
#endif
}


int mprGetLogLevel(MprCtx ctx)
{
    Mpr     *mpr;

    /*
     *  Leave the code like this so debuggers can patch logLevel before returning.
     */
    mpr = mprGetMpr(ctx);
    return mpr->logLevel;
}


void mprSetLogLevel(MprCtx ctx, int level)
{
    mprGetMpr(ctx)->logLevel = level;
}


/*
 *  Output a log message to the log handler
 */
static void logOutput(MprCtx ctx, int flags, int level, cchar *msg)
{
    MprLogHandler   handler;


    mprAssert(ctx != 0);
    handler = mprGetMpr(ctx)->logHandler;
    if (handler != 0) {
        (handler)(ctx, flags, level, msg);
        return;
    }
    defaultLogHandler(ctx, flags, level, msg);
}


/*
 *  Default log output is just to the console
 */
static void defaultLogHandler(MprCtx ctx, int flags, int level, cchar *msg)
{
    Mpr     *mpr;
    char    *prefix;

    mpr = mprGetMpr(ctx);
    prefix = mpr->name;

    while (*msg == '\n') {
        mprErrorPrintf(ctx, "\n");
        msg++;
    }

    if (flags & MPR_LOG_SRC) {
#if BREW && !BREWSIM
        mprErrorPrintf(ctx, "%s\n", msg);
#else
        mprErrorPrintf(ctx, "%s: %d: %s\n", prefix, level, msg);
#endif

    } else if (flags & MPR_ERROR_SRC) {
        /*
         *  Use static printing to avoid malloc when the messages are small.
         *  This is important for memory allocation errors.
         */
        if (strlen(msg) < (MPR_MAX_STRING - 32)) {
            mprStaticErrorPrintf(ctx, "%s: Error: %s\n", prefix, msg);
        } else {
            mprErrorPrintf(ctx, "%s: Error: %s\n", prefix, msg);
        }

    } else if (flags & MPR_FATAL_SRC) {
        mprErrorPrintf(ctx, "%s: Fatal: %s\n", prefix, msg);

    } else if (flags & MPR_RAW) {
        mprErrorPrintf(ctx, "%s", msg);

    } else {
        return;
    }
}


/*
 *  Map the O/S error code to portable error codes.
 */
int mprGetOsError()
{
#if BLD_WIN_LIKE
    int     rc;
    rc = GetLastError();

    /*
     *  Client has closed the pipe
     */
    if (rc == ERROR_NO_DATA) {
        return EPIPE;
    }
    return rc;
#elif BLD_UNIX_LIKE || VXWORKS
    return errno;
#elif BREW
    /*
     *  No such thing on Brew. Errors are per class
     */
    return 0;
#else
    return 0;
#endif
}


#if MACOSX
/*
 *  Just for conditional breakpoints when debugging in Xcode
 */
int _cmp(char *s1, char *s2)
{
    return !strcmp(s1, s2);
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
/************************************************************************/
/*
 *  End of file "../mprLog.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../mprModule.c"
 */
/************************************************************************/

/**
 *  mprModule.c - Dynamic module loading support.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



/*
 *  Open the module service
 */
MprModuleService *mprCreateModuleService(MprCtx ctx)
{
    MprModuleService    *ms;
    cchar               *searchPath;

    ms = mprAllocObjZeroed(ctx, MprModuleService);
    if (ms == 0) {
        return 0;
    }

    //  TODO - do we really need to keep a list of modules?
    ms->modules = mprCreateList(ms);

    /*
     *  Define the default module search path
     */
    if (ms->searchPath == 0) {
#if BLD_DEBUG
        /*
         *  Put the mod prefix here incase running an installed debug build
         */
        searchPath = ".:./lib/modules:../lib/modules:../../lib/modules:../../../lib/modules:" BLD_MOD_PREFIX;
#else
        searchPath = BLD_MOD_PREFIX ":.";
#endif
    } else {
        searchPath = ms->searchPath;
    }
    ms->searchPath = mprStrdup(ms, (searchPath) ? searchPath : ".");

#if BLD_FEATURE_MULTITHREAD
    ms->mutex = mprCreateLock(ms);
#endif

    return ms;
}


/*
 *  Call the start routine for each module
 */
int mprStartModuleService(MprModuleService *ms)
{
    MprModule       *mp;
    int             next;

    mprAssert(ms);

    mprLock(ms->mutex);
    for (next = 0; (mp = mprGetNextItem(ms->modules, &next)) != 0; ) {
        if (mp->start) {
            if (mp->start(mp) < 0) {
                return MPR_ERR_CANT_INITIALIZE;
            }
        }
    }
    mprUnlock(ms->mutex);
    return 0;
}


/*
 *  Stop all modules
 */
void mprStopModuleService(MprModuleService *ms)
{
    MprModule       *mp;
    int             next;

    mprAssert(ms);

    mprLock(ms->mutex);
    for (next = 0; (mp = mprGetNextItem(ms->modules, &next)) != 0; ) {
        if (mp->stop) {
            mp->stop(mp);
        }
    }
    mprUnlock(ms->mutex);
}


/*
 *  Create a new module
 */
MprModule *mprCreateModule(MprCtx ctx, cchar *name, cchar *version, void *moduleData, 
    MprModuleProc start, MprModuleProc stop)
{
    MprModuleService    *ms;
    MprModule           *mp;
    Mpr                 *mpr;
    int                 index;

    mpr = mprGetMpr(ctx);
    ms = mpr->moduleService;
    mprAssert(ms);

    mp = mprAllocObj(ctx, MprModule);
    if (mp == 0) {
        return 0;
    }

    index = mprAddItem(ms->modules, mp);
    mp->name = mprStrdup(mp, name);
    mp->version = mprStrdup(mp, version);
    mp->moduleData = moduleData;
    mp->handle = 0;

    if (index < 0 || mp->name == 0 || mp->version == 0) {
        mprFree(mp);
        return 0;
    }

    mp->start = start;
    mp->stop = stop;

    if (mpr->flags & MPR_STARTED) {
        if (mp->start && mp->start(mp) < 0) {
            return 0;
        }
    }
    return mp;
}


/*
 *  See if a module is already loaded
 */
MprModule *mprLookupModule(MprCtx ctx, cchar *name)
{
    MprModuleService    *ms;
    MprModule           *mp;
    int                 next;

    ms = mprGetMpr(ctx)->moduleService;
    mprAssert(ms);

    for (next = 0; (mp = mprGetNextItem(ms->modules, &next)) != 0; ) {
        if (strcmp(mp->name, name) == 0) {
            return mp;
        }
    }
    return 0;
}


/*
 *  Update the module search path
 */
void mprSetModuleSearchPath(MprCtx ctx, char *searchPath)
{
    MprModuleService    *ms;
    Mpr                 *mpr;

    mprAssert(ctx);
    mprAssert(searchPath && *searchPath);

    mpr = mprGetMpr(ctx);
    mprAssert(mpr);
    ms = mpr->moduleService;

    mprFree(ms->searchPath);
    ms->searchPath = mprStrdup(ms, searchPath);
}


cchar *mprGetModuleSearchPath(MprCtx ctx)
{
    MprModuleService    *ms;
    Mpr                 *mpr;

    mprAssert(ctx);

    mpr = mprGetMpr(ctx);
    mprAssert(mpr);
    ms = mpr->moduleService;

    return ms->searchPath;
}


#if !VXWORKS
/*
 *  Return true if the shared library in "file" can be found. Return the actual path in *path. The filename
 *  may not have a shared library extension which is typical so calling code can be cross platform.
 */
static int probe(MprCtx ctx, const char *filename, char **path)
{
    mprAssert(ctx);
    mprAssert(filename && *filename);
    mprAssert(path);

    *path = 0;

    mprLog(ctx, 4, "Probe for filename %s", filename);
    if (mprAccess(ctx, filename, R_OK)) {
        *path = mprStrdup(ctx, filename);
        return 1;
    }

    if (strstr(filename, BLD_SHOBJ) == 0) {
        mprAllocSprintf(ctx, path, MPR_MAX_FNAME, "%s%s", filename, BLD_SHOBJ);
        mprLog(ctx, 4, "Probe for library %s", *path);
        if (mprAccess(ctx, *path, R_OK)) {
            return 1;
        }
        mprFree(*path);
    }

    return 0;
}


/*
 *  Search for a module in the modulePath.
 */
static int searchForFile(MprCtx ctx, char **path, cchar *name)
{
    char    fileName[MPR_MAX_FNAME];
    char    *searchPath, *dir, *tok;

    /*
     *  Search for path directly
     */
    if (probe(ctx, name, path)) {
        mprLog(ctx, 4, "Found package %s at %s", name, *path);
        return 0;
    }

    /*
     *  Search in the searchPath
     */
    searchPath = mprStrdup(ctx, mprGetModuleSearchPath(ctx));

    tok = 0;
    dir = mprStrTok(searchPath, MPR_SEARCH_DELIM, &tok);
    while (dir && *dir) {
        mprSprintf(fileName, sizeof(fileName), "%s/%s", dir, name);
        if (probe(ctx, fileName, path)) {
            mprLog(ctx, 4, "Found package %s at %s", name, *path);
            return 0;
        }
        dir = mprStrTok(0, MPR_SEARCH_DELIM, &tok);
    }
    mprFree(searchPath);
    return MPR_ERR_NOT_FOUND;
}
#endif


#if BLD_WIN_LIKE
/*
 *  Load a module for windows
 */
MprModule *mprLoadModule(MprCtx ctx, cchar *filename, cchar *initFunction)
{
    MprModule       *mp;
    MprModuleEntry  fn;
    char            localPath[MPR_MAX_FNAME];
    void            *handle;
    char            *cp, *path;

    mprAssert(filename && *filename);
    mprAssert(initFunction && *initFunction);

    mprStrcpy(localPath, sizeof(localPath), filename);
    for (cp = localPath; *cp; cp++) {
        if (*cp == '/') {
            *cp = '\\';
        }
    }

    if (searchForFile(ctx, &path, localPath) < 0) {
        mprError(ctx, "Can't find module \"%s\" in search path \"%s\"", filename, mprGetModuleSearchPath(ctx));
        return 0;
    }

    if ((handle = GetModuleHandle(mprGetBaseName(localPath))) == 0) {
        if ((handle = LoadLibrary(localPath)) == 0) {
            mprError(ctx, "Can't load %s\nReason: \"%d\", %d\n",  path, mprGetOsError(), GetLastError());
            return 0;
        }
    }

    fn = (MprModuleEntry) GetProcAddress((HINSTANCE) handle, initFunction);
    if (fn == 0) {
        FreeLibrary((HINSTANCE) handle);
        mprError(ctx, "Can't load %s\nReason: can't find function \"%s\"\n",  localPath, initFunction);
        return 0;
    }

    mprLog(ctx, MPR_INFO, "Loading module %s", path);

    if ((mp = (fn)(ctx, path)) == 0) {
        FreeLibrary((HINSTANCE) handle);
        mprError(ctx, "Initialization for %s failed.", path);
        return 0;
    }

    mp->handle = handle;

    return mp;
}


void mprUnloadModule(MprModule *mp)
{
    mprAssert(mp->handle);

    if (mp->stop) {
        mp->stop(mp);
    }
    //  TODO - locking
    mprRemoveItem(mprGetMpr(mp)->moduleService->modules, mp);
    FreeLibrary((HINSTANCE) mp->handle);
}


#elif VXWORKS
/*
 *  Load a shared object for VxWorks specified by path. initFunction is the entryPoint.
 */

MprModule *mprLoadModule(MprCtx ctx, cchar *path, cchar *initFunction)
{
    MprModule       *mp;
    MprModuleEntry  fn;
    SYM_TYPE        symType;
    void            *handle;
    char            entryPoint[MPR_MAX_FNAME];
    int             fd, flags;

    mprAssert(path && *path);
    mprAssert(initFunction && *initFunction);

    if (moduleFindByName((char*) path) != 0) {
        /*  Already loaded */
        return 0;
    }

#if BLD_HOST_CPU_ARCH == MPR_CPU_IX86 || BLD_HOST_CPU_ARCH == MPR_CPU_IX64
    mprSprintf(entryPoint, sizeof(entryPoint), "_%s", initFunction);
#else
    mprStrcpy(entryPoint, sizeof(entryPoint), initFunction);
#endif

    if ((fd = open(path, O_RDONLY, 0664)) < 0) {
        mprError(ctx, "Can't open %s", path);
        return 0;
    }

    flags = LOAD_GLOBAL_SYMBOLS;
#if BLD_DEBUG
    flags |= LOAD_LOCAL_SYMBOLS;
#endif
    if ((handle = loadModule(fd, flags)) == 0) {
        mprError(ctx, "Can't load %s", path);
        close(fd);
        return 0;
    }
    close(fd);

    fn = 0;
    if (symFindByName(sysSymTbl, entryPoint, (char**) &fn, &symType) == -1) {
        mprError(ctx, "Can't find symbol %s when loading %s", initFunction, path);
        return 0;
    }

    mprLog(ctx, MPR_INFO, "Loading module %s", path);

    if ((mp = (fn)(ctx, path)) == 0) {
        mprError(ctx, "Initialization for %s failed.", path);
        return 0;
    }

    return mp;
}


void mprUnloadModule(MprModule *mp)
{
    //  TODO - locking
    mprRemoveItem(mprGetMpr(mp)->moduleService->modules, mp);
    unldByModuleId((MODULE_ID) mp->handle, 0);
}


#else /* UNIX_LIKE */

/*
 *  Load a shared library on unix like systems
 */
MprModule *mprLoadModule(MprCtx ctx, cchar *filename, cchar *initFunction)
{
    MprModuleEntry  fn;
    MprModule       *mp;
    char            *path;
    void            *handle;

    mprAssert(filename && *filename);
    mprAssert(initFunction && *initFunction);

    if (searchForFile(ctx, &path, filename) < 0) {
        mprError(ctx, "Can't find module \"%s\" in search path \"%s\"", filename, mprGetModuleSearchPath(ctx));
        return 0;
    }

    mprAssert(path);
    if ((handle = dlopen(path, RTLD_LAZY | RTLD_GLOBAL)) == 0) {
        mprError(ctx, "Can't load %s\nReason: \"%s\"",  path, dlerror());
        return 0;
    }

    if ((fn = (MprModuleEntry) dlsym(handle, initFunction)) == 0) {
        mprError(ctx, "Can't load %s\nReason: can't find function \"%s\"",  path, initFunction);
        dlclose(handle);
        return 0;
    }

    mprLog(ctx, MPR_INFO, "Loading module %s", path);

    if ((mp = (fn)(ctx, path)) == 0) {
        dlclose(handle);
        return 0;
    }
    mp->handle = handle;
    return mp;
}


//  TODO - who frees the module
void mprUnloadModule(MprModule *mp)
{
    if (mp->handle) {
        dlclose(mp->handle);
    }
    //  TODO - locking
    mprRemoveItem(mprGetMpr(mp)->moduleService->modules, mp);
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
/************************************************************************/
/*
 *  End of file "../mprModule.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../mprOs.c"
 */
/************************************************************************/

/**
 *  mprOs.c - Mixed bag of Per operating system adaption
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */




static int          osDestructor(MprOsService *os);

#if BLD_WIN_LIKE
static cchar    *getHive(cchar *key, HKEY *root);
#endif

/*
 *  Initialize the O/S platform layer
 */ 

MprOsService *mprCreateOsService(MprCtx ctx)
{
    MprOsService    *os;

    os = mprAllocObjWithDestructor(ctx, MprOsService, osDestructor);
    if (os == 0) {
        return 0;
    }

#if BLD_UNIX_LIKE
    umask(022);

    /*
     *  Cleanup the environment. IFS is often a security hole
     */
     putenv("IFS=\t ");
#endif

    return os;
}



/*
 *  Terminate the O/S platform layer
 */ 
static int osDestructor(MprOsService *os)
{
    return 0;
}



/*
 *  Start any required O/S platform services
 */ 
int mprStartOsService(MprOsService *os)
{
#if BLD_WIN_LIKE
    WSADATA     wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return -1;
    }
#endif
#if BLD_UNIX_LIKE
    /* 
     *  Open a syslog connection
     */
    openlog(mprGetAppName(os), LOG_CONS || LOG_PERROR, LOG_LOCAL0);
#endif

    return 0;
}



/*
 *  Stop the platform services
 */ 
void mprStopOsService(MprOsService *os)
{
#if BLD_WIN_LIKE
    WSACleanup();
#endif
}


char *mprGetAppDir(MprCtx ctx, char *path, int pathsize)
{ 
    char    *cp;

    mprGetAppPath(ctx, path, pathsize);
    if ((cp = strrchr(path, '/')) != 0) {
        *cp = '\0';
    }
    return path; 
} 

#if BLD_WIN_LIKE
/*
 *  Sleep. Period given in milliseconds.
 */

void mprSleep(MprCtx ctx, int milliseconds)
{
    Sleep(milliseconds);
}



int mprGetRandomBytes(MprCtx ctx, uchar *buf, int length, int block)
{
    HCRYPTPROV      prov;
    int             rc;

    rc = 0;

    if (!CryptAcquireContext(&prov, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | 0x40)) {
        return -mprGetOsError();
    }
    if (!CryptGenRandom(prov, length, buf)) {
        rc = -mprGetOsError();
    }
    CryptReleaseContext(prov, 0);
    return rc;
}



void mprWriteToOsLog(MprCtx ctx, cchar *message, int flags, int level)
{
    HKEY        hkey;
    void        *event;
    long        errorType;
    char        buf[MPR_MAX_STRING];
    char        logName[MPR_MAX_STRING];
    char        *lines[9];
    char        *cp, *value;
    int         type;
    ulong       exists;
    static int  once = 0;

    mprStrcpy(buf, sizeof(buf), message);
    cp = &buf[strlen(buf) - 1];
    while (*cp == '\n' && cp > buf) {
        *cp-- = '\0';
    }

    type = EVENTLOG_ERROR_TYPE;

    lines[0] = buf;
    lines[1] = 0;
    lines[2] = lines[3] = lines[4] = lines[5] = 0;
    lines[6] = lines[7] = lines[8] = 0;

    if (once == 0) {
        /*  Initialize the registry */
        once = 1;
        mprSprintf(logName, sizeof(logName), 
            "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\%s",
            mprGetAppName(ctx));
        hkey = 0;

        if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, logName, 0, NULL, 0, 
                KEY_ALL_ACCESS, NULL, &hkey, &exists) == ERROR_SUCCESS) {

            value = "%SystemRoot%\\System32\\netmsg.dll";
            if (RegSetValueEx(hkey, "EventMessageFile", 0, REG_EXPAND_SZ, 
                    (uchar*) value, (int) strlen(value) + 1) != ERROR_SUCCESS) {
                RegCloseKey(hkey);
                return;
            }

            errorType = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | 
                EVENTLOG_INFORMATION_TYPE;
            if (RegSetValueEx(hkey, "TypesSupported", 0, REG_DWORD, 
                    (uchar*) &errorType, sizeof(DWORD)) != ERROR_SUCCESS) {
                RegCloseKey(hkey);
                return;
            }
            RegCloseKey(hkey);
        }
    }

    event = RegisterEventSource(0, mprGetAppName(ctx));
    if (event) {
        /*
         *  3299 is the event number for the generic message in netmsg.dll.
         *  "%1 %2 %3 %4 %5 %6 %7 %8 %9" -- thanks Apache for the tip
         */
        ReportEvent(event, EVENTLOG_ERROR_TYPE, 0, 3299, NULL, 
            sizeof(lines) / sizeof(char*), 0, (LPCSTR*) lines, 0);
        DeregisterEventSource(event);
    }
}



void mprSetShell(MprCtx ctx, void *shell)
{
}



void *mprGetShell(MprCtx ctx)
{
    return 0;
}



long mprGetInst(Mpr *mpr)
{
    return (long) mpr->appInstance;
}



void mprSetInst(Mpr *mpr, long inst)
{
    mpr->appInstance = inst;
}



HWND mprGetHwnd(MprCtx ctx)
{
    Mpr     *mpr;

    mpr = mprGetMpr(ctx);
    return mpr->waitService->hwnd;
}



void mprSetHwnd(MprCtx ctx, HWND h)
{
    Mpr     *mpr;

    mpr = mprGetMpr(ctx);
    mpr->waitService->hwnd = h;
}


void mprSetSocketMessage(MprCtx ctx, int socketMessage)
{
    Mpr     *mpr;

    mpr = mprGetMpr(ctx);
    mpr->waitService->socketMessage = socketMessage;
}



/*
 *  Read a registry value. Returns allocated memory in buf.
 */ 
int mprReadRegistry(MprCtx ctx, char **buf, int max, cchar *key, cchar *name)
{
    HKEY        top, h;
    char        *value;
    ulong       type, size;

    mprAssert(key && *key);
    mprAssert(buf);

    /*
     *  Get the registry hive
     */
    if ((key = getHive(key, &top)) == 0) {
        return MPR_ERR_CANT_ACCESS;
    }

    if (RegOpenKeyEx(top, key, 0, KEY_READ, &h) != ERROR_SUCCESS) {
        return MPR_ERR_CANT_ACCESS;
    }

    /*
     *  Get the type
     */
    if (RegQueryValueEx(h, name, 0, &type, 0, &size) != ERROR_SUCCESS) {
        RegCloseKey(h);
        return MPR_ERR_CANT_READ;
    }
    if (type != REG_SZ && type != REG_EXPAND_SZ) {
        RegCloseKey(h);
        return MPR_ERR_BAD_TYPE;
    }

    value = (char*) mprAlloc(ctx, size);
    if ((int) size > max) {
        RegCloseKey(h);
        return MPR_ERR_WONT_FIT;
    }
    if (RegQueryValueEx(h, name, 0, &type, (uchar*) value, &size) != ERROR_SUCCESS) {
        mprFree(value);
        RegCloseKey(h);
        return MPR_ERR_CANT_READ;
    }

    RegCloseKey(h);
    *buf = value;
    return 0;
}



/*
 *  Write a string registry value. Returns allocated memory in buf.
 */ 
int mprWriteRegistry(MprCtx ctx, cchar *key, cchar *name, cchar *value)
{
    HKEY    top, h, subHandle;
    ulong   disposition;

    mprAssert(key && *key);
    mprAssert(name && *name);
    mprAssert(value && *value);

    /*
     *  Get the registry hive
     */
    if ((key = getHive(key, &top)) == 0) {
        return MPR_ERR_CANT_ACCESS;
    }

    if (name) {
        /*
         *  Write a registry string value
         */
        if (RegOpenKeyEx(top, key, 0, KEY_ALL_ACCESS, &h) != ERROR_SUCCESS) {
            return MPR_ERR_CANT_ACCESS;
        }
        if (RegSetValueEx(h, name, 0, REG_SZ, value, (int) strlen(value) + 1) != ERROR_SUCCESS) {
            RegCloseKey(h);
            return MPR_ERR_CANT_READ;
        }

    } else {
        /*
         *  Create a new sub key
         */
        if (RegOpenKeyEx(top, key, 0, KEY_CREATE_SUB_KEY, &h) != ERROR_SUCCESS){
            return MPR_ERR_CANT_ACCESS;
        }
        if (RegCreateKeyEx(h, name, 0, NULL, REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS, NULL, &subHandle, &disposition) != ERROR_SUCCESS) {
            return MPR_ERR_CANT_ACCESS;
        }
        RegCloseKey(subHandle);
    }
    RegCloseKey(h);
    return 0;
}



/*
 *  Determine the registry hive by the first portion of the path. Return 
 *  a pointer to the rest of key path after the hive portion.
 */ 
static cchar *getHive(cchar *keyPath, HKEY *hive)
{
    char    key[MPR_MAX_STRING], *cp;
    int     len;

    mprAssert(keyPath && *keyPath);

    *hive = 0;

    mprStrcpy(key, sizeof(key), keyPath);
    key[sizeof(key) - 1] = '\0';

    if (cp = strchr(key, '\\')) {
        *cp++ = '\0';
    }
    if (cp == 0 || *cp == '\0') {
        return 0;
    }

    if (!mprStrcmpAnyCase(key, "HKEY_LOCAL_MACHINE")) {
        *hive = HKEY_LOCAL_MACHINE;
    } else if (!mprStrcmpAnyCase(key, "HKEY_CURRENT_USER")) {
        *hive = HKEY_CURRENT_USER;
    } else if (!mprStrcmpAnyCase(key, "HKEY_USERS")) {
        *hive = HKEY_USERS;
    } else if (!mprStrcmpAnyCase(key, "HKEY_CLASSES_ROOT")) {
        *hive = HKEY_CLASSES_ROOT;
    } else {
        return 0;
    }

    if (*hive == 0) {
        return 0;
    }
    len = (int) strlen(key) + 1;
    return keyPath + len;
}



char *mprGetAppPath(MprCtx ctx, char *path, int pathsize)
{
    char    *cp;

    mprAssert(path);

    if (GetModuleFileName(0, path, pathsize) <= 0) {
        return 0;
    }
    for (cp = path; *cp; cp++) {
        if (*cp == '\\') {
            *cp = '/';
        }
    }
    return path;
}


#elif VXWORKS
/*
 *  Sleep. Period given in milliseconds.
 */
void mprSleep(MprCtx ctx, int milliseconds)
{
    struct timespec timeout;
    int             rc;

    mprAssert(milliseconds >= 0);
    timeout.tv_sec = milliseconds / 1000;
    timeout.tv_nsec = (milliseconds % 1000) * 1000000;
    do {
        rc = nanosleep(&timeout, &timeout);
    } while (rc < 0 && errno == EINTR);
}



/*
 *  access() 
 */
int access(const char *path, int mode)
{
    struct stat sbuf;

    return stat((char*) path, &sbuf);
}



int mprGetRandomBytes(MprCtx ctx, uchar *buf, int length, int block)
{
    int     i;

    for (i = 0; i < length; i++) {
        buf[i] = (char) (mprGetTime(ctx) >> i);
    }
    return 0;
}



void mprSetShell(MprCtx ctx, void *shell)
{
}



void *mprGetShell(MprCtx ctx)
{
    return 0;
}


char *mprGetAppPath(MprCtx ctx, char *path, int pathsize)
{
    return BLD_PRODUCT;
}


#elif BLD_UNIX_LIKE

/*
 *  Sleep. Period given in milliseconds.
 */

void mprSleep(MprCtx ctx, int milliseconds)
{
    struct timespec timeout;
    int             rc;

    mprAssert(milliseconds >= 0);
    timeout.tv_sec = milliseconds / 1000;
    timeout.tv_nsec = (milliseconds % 1000) * 1000000;
    do {
        rc = nanosleep(&timeout, &timeout);
    } while (rc < 0 && errno == EINTR);
}


int mprGetRandomBytes(MprCtx ctx, uchar *buf, int length, int block)
{
    int     fd, sofar, rc;

    fd = open((block) ? "/dev/random" : "/dev/urandom", O_RDONLY, 0666);
    if (fd < 0) {
        return MPR_ERR_CANT_OPEN;
    }

    sofar = 0;
    do {
        rc = read(fd, &buf[sofar], length);
        if (rc < 0) {
            mprAssert(0);
            return MPR_ERR_CANT_READ;
        }
        length -= rc;
        sofar += rc;
    } while (length > 0);
    close(fd);
    return 0;
}



/*  
 *  Write a message in the O/S native log (syslog in the case of LINUX)
 */
void mprWriteToOsLog(MprCtx ctx, cchar *message, int flags, int level)
{
    char    msg[MPR_MAX_FNAME];

    if (flags & MPR_FATAL_SRC) {
        mprSprintf(msg, sizeof(msg), "%s fatal error: ", mprGetAppName(ctx));

    } else if (flags & MPR_ASSERT_SRC) {
        mprSprintf(msg, sizeof(msg), "%s program assertion error: ", 
            mprGetAppName(ctx));

    } else {
        mprSprintf(msg, sizeof(msg), "%s error: ", mprGetAppName(ctx));
    }
    syslog(flags, "%s: %s\n", msg, message);
}



void mprSetShell(MprCtx ctx, void *shell)
{
}



void *mprGetShell(MprCtx ctx)
{
    return 0;
}



#if MACOSX
//  TODO - these should return an allocated path
char *mprGetAppPath(MprCtx ctx, char *path, int pathsize)
{ 
    uint    size;

    size = pathsize - 1;
    if (_NSGetExecutablePath(path, &size) < 0) {
        path[0] = '\0';
        return NULL;
    }
    return path; 
} 


#else /* LINUX */
char *mprGetAppPath(MprCtx ctx, char *path, int pathsize)
{ 
    char    pbuf[MPR_MAX_STRING];
    int     len;

    mprSprintf(pbuf, sizeof(pbuf), "/proc/%i/exe", getpid()); 

    path[pathsize - 1] = '\0';
    len = readlink(pbuf, path, pathsize - 1);
    if (len < 0) {
        path[0] = '\0';
        return NULL; 
    }
    path[len] = '\0';
    return path; 
} 
#endif

 
#elif BREW


int mprGetRandomBytes(MprCtx ctx, uchar *buf, int length, int block)
{
    MprTime now;
    int     i;

    for (i = 0; i < length; i++) {
        now = mprGetTime(ctx);
        buf[i] = (uchar) (now >> i);
    }
    return 0;
}



void mprWriteToOsLog(MprCtx ctx, cchar *message, int flags, int level)
{
}



void mprSetShell(MprCtx ctx, void *shell)
{
    mprGetMpr(ctx)->shell = shell;
}



void *mprGetShell(MprCtx ctx)
{
    return mprGetMpr(ctx)->shell;
}



void mprSetClassId(MprCtx ctx, uint classId)
{
    mprGetMpr(ctx)->classId = classId;
}



uint mprGetClassId(MprCtx ctx)
{
    return mprGetMpr(ctx)->classId;
}



void mprSetDisplay(MprCtx ctx, void *display)
{
    mprGetMpr(ctx)->display = display;
}



void *mprGetDisplay(MprCtx ctx)
{
    return mprGetMpr(ctx)->display;
}



/*
 *  Sleep. Period given in milliseconds.
 *  WARNING: not a good idea to call this as it will hang the phone !!!!
 */
void mprSleep(MprCtx ctx, int milliseconds)
{
    MprTime     then;

    then = mprGetTime(ctx) + milliseconds;

    while (mprCompareTime(mprGetTime(ctx), then) < 0) {
        ;
    }
}



/*
 *  Replacement for gethostbyname that is multi-thread safe
 */
struct hostent *mprGetHostByName(MprCtx ctx, cchar *name)
{
    return 0;
}



int getpid()
{
    return 0;
}



int isalnum(int c)
{
    return (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') ||
        ('0' <= c && c <= '9'));
} 



int isalpha(int c)
{
    return (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z'));
} 



int isdigit(int c)
{
    return ('0' <= c && c <= '9');
}
 


int islower(int c)
{
    return ('a' <= c && c <= 'z');
}
 


int isspace(int c)
{
    return (c == ' ' || c == '\t' || c == '\n' || c == '\r');
}
 


int isupper(int c)
{
    return ('A' <= c && c <= 'Z');
}
 


int isxdigit(int c)
{
    return ('0' <= c && c <= '9') || ('A' <= c && c <= 'F') ||
        ('a' <= c && c <= 'f');
}
 


char *strpbrk(cchar *str, cchar *set)
{
    uchar   charMap[32];
    int     i;

    for (i = 0; i < 32; i++) {
        charMap[i] = 0;
    }

    while (*set)
    {
        charMap[*set >> 0x3] |= (1 << (*set & 0x7));
        set++;
    }

    while (*str)
    {
        if (charMap[*str >> 3] & (1 << (*str & 7)))
            return (char*) str;
        str++;
    }
    return 0;
} 



uint strspn(cchar *str, cchar *set)
{
    uchar   charMap[32];
    int     i;

    for (i = 0; i < 32; i++) {
        charMap[i] = 0;
    }

    while (*set)
    {
        charMap[*set >> 0x3] |= (1 << (*set & 0x7));
        set++;
    }

    if (*str) {
        i = 0;
        while (charMap[*str >> 0x3] & (1 << (*str & 0x7))) {
            i++;
            str++;
        }
        return i;
    }

    return 0;
}
 


char *strstr(cchar *str, cchar *subString)
{
    char *cp, *s1, *s2;

    if (subString == 0 || *subString == '\0') {
        return (char*) str;
    }

    for (cp = (char*) str; *cp; cp++) {
        s1 = cp;
        s2 = (char *) subString;

        while (*s1 && *s2 && (*s1 == *s2)) {
            s1++;
            s2++;
        }

        if (*s2 == '\0') {
            return cp;
        }
    }

    return 0;
}



#if !BREWSIM

uint strlen(cchar *str)
{
    return STRLEN(str);
}



void *memset(const void *dest, int c, uint count)
{
    return MEMSET((void*) dest, c, count);
}



int toupper(int c)
{
    if (islower(c)) {
        c = 'A' + c - 'a';
    }
    return c;
}



void *memcpy(void *dest, const void *src, uint count)
{
    return MEMCPY(dest, src, count);
}


/*
 *  Copy memory supporting overlapping regions
 */

void *memmove(void *destPtr, const void *srcPtr, uint count)
{
    char    *dest, *src;

    dest = (char*) destPtr;
    src = (char*) srcPtr;

    if (dest <= src || dest >= &src[count]) {
        /*
         *  Disjoint
         */
        while (count--) {
            *dest++ = *src++;
        }

    } else {
        /*
         * Overlapping region
         */
        dest = &dest[count - 1];
        src = &src[count - 1];

        while (count--) {
            *dest-- = *src--;
        }
    }
    return destPtr;
}



char *strrchr(cchar *str, int c)
{
    return STRRCHR(str, c);
}



char *strcat(char *dest, cchar *src)
{
    return STRCAT(dest, src);
}



int strcmp(cchar *s1, cchar *s2)
{
    return STRCMP(s1, s2);
}



int strncmp(cchar *s1, cchar *s2, uint count)
{
    return STRNCMP(s1, s2, count);
}



char *strcpy(char *dest, cchar *src)
{
    return STRCPY(dest, src);
}



char *strncpy(char *dest, cchar *src, uint count)
{
    return STRNCPY(dest, src, count);
}



char *strchr(cchar *str, int c)
{
    return STRCHR(str, c);
}



int atoi(cchar *str)
{
    return ATOI(str);
}



int tolower(int c)
{
    if (isupper(c)) {
        c = 'a' + c - 'A';
    }
    return c;
}



void *malloc(uint size)
{
    void    *ptr;
    ptr = MALLOC(size);
    if (ptr == 0) {
        mprAssert(0);
    }
    return ptr; 
}



void *realloc(void *ptr, uint size)
{
    void    *newPtr;

    newPtr = REALLOC(ptr, size);
    if (newPtr == 0) {
        mprAssert(0);
    }
    return newPtr;
}



void free(void *ptr)
{
    if (ptr) {
        FREE(ptr);
    }
}



#endif /* ! BREWSIM */
#endif /* BREW */

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
/************************************************************************/
/*
 *  End of file "../mprOs.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../mprPool.c"
 */
/************************************************************************/

/**
 *  mprPool.c - Thread pool service
 *
 *  The MPR provides a high peformance thread pool service where pre-allocated threads can be dispatched to 
 *  service tasks.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */


#if BLD_FEATURE_MULTITHREAD

static int  changeState(MprPoolThread *pt, int state);
static MprPoolThread *createPoolThread(MprPoolService *ps, int stackSize);
static int  getNextThreadNum(MprPoolService *ps);
static int  poolThreadDestructor(MprPoolThread *pt);
static void pruneThreads(MprPoolService *ps, MprEvent *timer);
static void poolMain(MprPoolThread *pt, MprThread *tp);

/*
 *  Constructor for a thread pool
 */

MprPoolService *mprCreatePoolService(MprCtx ctx)
{
    MprPoolService      *ps;

    ps = mprAllocObjZeroed(ctx, MprPoolService);
    if (ps == 0) {
        return 0;
    }

    ps->mutex = mprCreateLock(ps);
    
    ps->minThreads = MPR_DEFAULT_MIN_THREADS;
    ps->maxThreads = MPR_DEFAULT_MIN_THREADS;

    /*
     *  Presize the lists so they cannot get memory allocation failures later on.
     */
    ps->idleThreads = mprCreateList(ps);
    mprSetListLimits(ps->idleThreads, ps->maxThreads, -1);

    ps->busyThreads = mprCreateList(ps);
    mprSetListLimits(ps->busyThreads, ps->maxThreads, -1);

    return ps;
}



/*
 *  Start the thread service
 */
int mprStartPoolService(MprPoolService *ps)
{
    mprLock(ps->mutex);

    mprSetMinPoolThreads(ps, ps->minThreads);

    /*
     *  Create a timer to trim excess threads in the pool
     */
    if (!mprGetDebugMode(ps)) {
        ps->pruneTimer = mprCreateTimerEvent(ps, (MprEventProc) pruneThreads, MPR_TIMEOUT_PRUNER, 
            MPR_NORMAL_PRIORITY, (void*) ps, 0);
    }

    mprUnlock(ps->mutex);
    return 0;
}



/*
 *  Stop the thread service. Wait for all threads to complete.
 */
void mprStopPoolService(MprPoolService *ps, int timeout)
{
    MprPoolThread       *pt;
    int                 next;

    mprLock(ps->mutex);

    if (ps->pruneTimer) {
        mprFree(ps->pruneTimer);
        ps->pruneTimer = 0;
    }

    /*
     *  Wake up all idle threads. Busy threads take care of themselves. An idle thread will wakeup, exit and be 
     *  removed from the busy list and then delete the thread. We progressively remove the last thread in the idle
     *  list. ChangeState will move the threads to the busy queue.
     */
    for (next = -1; (pt = (MprPoolThread*) mprGetPrevItem(ps->idleThreads, &next)) != 0; next = -1) {
        changeState(pt, MPR_POOL_THREAD_BUSY);
    }

    /*
     *  Wait until all tasks and threads have exited
     */
    while (timeout > 0 && ps->numThreads > 0) {
        mprUnlock(ps->mutex);
        mprSleep(ps, 50);
        timeout -= 10;
        mprLock(ps->mutex);
    }

    mprAssert(ps->idleThreads->length == 0);
    mprAssert(ps->busyThreads->length == 0);

    mprUnlock(ps->mutex);
}



/*
 *  Define the new minimum number of threads. Pre-allocate the minimum.
 */
void mprSetMinPoolThreads(MprCtx ctx, int n)
{ 
    MprPoolThread   *pt;
    MprPoolService  *ps;

    ps = mprGetMpr(ctx)->poolService;

    mprLock(ps->mutex);

    ps->minThreads = n; 
    
    while (ps->numThreads < ps->minThreads) {
        pt = createPoolThread(ps, ps->stackSize);

        ps->numThreads++;
        ps->maxUseThreads = max(ps->numThreads, ps->maxUseThreads);
        ps->pruneHighWater = max(ps->numThreads, ps->pruneHighWater);

        mprStartThread(pt->thread);
    }
    
    mprUnlock(ps->mutex);
}



/*
 *  Define a new maximum number of theads. Prune if currently over the max
 */
void mprSetMaxPoolThreads(MprCtx ctx, int n)
{
    MprPoolService  *ps;

    ps = mprGetMpr(ctx)->poolService;

    mprLock(ps->mutex);

    ps->maxThreads = n; 
    if (ps->numThreads > ps->maxThreads) {
        pruneThreads(ps, 0);
    }

    if (ps->minThreads > ps->maxThreads) {
        ps->minThreads = ps->maxThreads;
    }
    mprUnlock(ps->mutex);
}



int mprGetMaxPoolThreads(MprCtx ctx)
{
    return mprGetMpr(ctx)->poolService->maxThreads;
}



int mprStartPoolThread(MprCtx ctx, MprPoolProc proc, void *data, int priority)
{
    MprPoolService  *ps;
    MprPoolThread   *pt;
    int             next;

    ps = mprGetMpr(ctx)->poolService;

    mprLock(ps->mutex);

    /*
     *  Try to find an idle thread and wake it up. It will wakeup in poolMain(). If not any available, then add 
     *  another thread to the pool. Must account for threads we've already created but have not yet gone to work 
     *  and inserted themselves in the idle/busy queues.
     */
    next = 0;
    pt = (MprPoolThread*) mprGetNextItem(ps->idleThreads, &next);
    if (pt) {

        pt->proc = proc;
        pt->data = data;
        pt->priority = priority;
        changeState(pt, MPR_POOL_THREAD_BUSY);

    } else if (ps->numThreads < ps->maxThreads) {

        /*
         *  Can't find an idle thread. Try to create more threads in the pool. Otherwise, we will have to wait. 
         *  No need to wakeup the thread -- it will immediately go to work.
         */
        pt = createPoolThread(ps, ps->stackSize);

        ps->numThreads++;
        ps->maxUseThreads = max(ps->numThreads, ps->maxUseThreads);
        ps->pruneHighWater = max(ps->numThreads, ps->pruneHighWater);

        pt->proc = proc;
        pt->data = data;
        pt->priority = priority;

        changeState(pt, MPR_POOL_THREAD_BUSY);

        mprStartThread(pt->thread);

    } else {
        /*
         *  No free threads and can't create anymore. We will have to wait. Busy threads will call dispatch when 
         *  they become idle
         */
        mprUnlock(ps->mutex);
        return MPR_ERR_BUSY;
    }

    mprUnlock(ps->mutex);
    return 0;
}



/*
 *  Trim idle threads from a task
 */
static void pruneThreads(MprPoolService *ps, MprEvent *timer)
{
    MprPoolThread   *pt;
    int             index, toTrim;

    if (mprIsExiting(ps)) {
        return;
    }

    mprLock(ps->mutex);

    /*
     *  Prune half of what we could prune. This gives exponentional decay. We use the high water mark seen in 
     *  the last period.
     */
    toTrim = (ps->pruneHighWater - ps->minThreads) / 2;

    for (index = 0; toTrim-- > 0 && index < ps->idleThreads->length; index++) {
        pt = (MprPoolThread*) mprGetItem(ps->idleThreads, index);
        /*
         *  Leave floating -- in no queue. The thread will kill itself.
         */
        changeState(pt, MPR_POOL_THREAD_PRUNED);
    }
    ps->pruneHighWater = ps->minThreads;

    mprUnlock(ps->mutex);
}



int mprAvailablePoolThreads(MprCtx ctx)
{
    MprPoolService  *ps;

    ps = mprGetMpr(ctx)->poolService;
    return ps->idleThreads->length + (ps->maxThreads - ps->numThreads); 
}



static int getNextThreadNum(MprPoolService *ps)
{
    int     rc;

    mprLock(ps->mutex);
    rc = ps->nextThreadNum++;
    mprUnlock(ps->mutex);

    return rc;
}



/*
 *  Define a new stack size for new threads. Existing threads unaffected.
 */
void mprSetPoolThreadStackSize(MprCtx ctx, int n)
{
    MprPoolService  *ps;

    ps = mprGetMpr(ctx)->poolService;
    ps->stackSize = n; 
}



#if BLD_DEBUG
void mprGetPoolServiceStats(MprPoolService *ps, MprPoolStats *stats)
{
    mprAssert(ps);

    stats->maxThreads = ps->maxThreads;
    stats->minThreads = ps->minThreads;
    stats->numThreads = ps->numThreads;
    stats->maxUse = ps->maxUseThreads;
    stats->pruneHighWater = ps->pruneHighWater;
    stats->idleThreads = ps->idleThreads->length;
    stats->busyThreads = ps->busyThreads->length;
}
#endif /* BLD_DEBUG */



/*
 *  Create a new thread for the task
 */
static MprPoolThread *createPoolThread(MprPoolService *ps, int stackSize)
{
    MprPoolThread   *pt;

    char    name[16];

    pt = mprAllocObjWithDestructor(ps, MprPoolThread, poolThreadDestructor);
    if (pt == 0) {
        return 0;
    }

    pt->proc = 0;
    pt->data = 0;
    pt->priority = 0;
    pt->state = 0;
    pt->pool = ps;
    pt->idleCond = mprCreateCond(pt);

    changeState(pt, MPR_POOL_THREAD_IDLE);

    mprSprintf(name, sizeof(name), "pool.%u", getNextThreadNum(ps));

    pt->thread = mprCreateThread(ps, name, (MprThreadProc) poolMain, (void*) pt, MPR_POOL_PRIORITY, 0);

    return pt;
}



static int poolThreadDestructor(MprPoolThread *pt)
{
    if (pt->thread != 0) {
        mprAssert(pt->thread);
        return 1;
    }
    return 0;
}



/*
 *  Pool thread main service routine
 */
static void poolMain(MprPoolThread *pt, MprThread *tp)
{
    MprPoolService  *ps;

    ps = mprGetMpr(pt)->poolService;

    mprLock(pt->pool->mutex);

    while (!mprIsExiting(pt) && !(pt->state & MPR_POOL_THREAD_PRUNED)) {

        if (pt->proc) {

            mprUnlock(ps->mutex);
            mprSetThreadPriority(pt->thread, pt->priority);
            (*pt->proc)(pt->data, pt);
            pt->proc = 0;
            mprSetThreadPriority(pt->thread, MPR_POOL_PRIORITY);
            mprLock(ps->mutex);
        }

        changeState(pt, MPR_POOL_THREAD_SLEEPING);
        
        mprUnlock(ps->mutex);
        mprWaitForCond(pt->idleCond, -1);
        mprLock(ps->mutex);
    }

    changeState(pt, 0);

    ps->numThreads--;
    mprUnlock(ps->mutex);
}



static int changeState(MprPoolThread *pt, int state)
{
    MprPoolService  *ps;
    MprList         *lp;

    ps = pt->pool;
    lp = 0;

    mprLock(ps->mutex);
    switch (pt->state) {
    case MPR_POOL_THREAD_BUSY:
        lp = ps->busyThreads;
        break;

    case MPR_POOL_THREAD_IDLE:
        lp = ps->idleThreads;
        break;

    case MPR_POOL_THREAD_SLEEPING:
        lp = ps->idleThreads;
        mprSignalCond(pt->idleCond); 
        break;
        
    case MPR_POOL_THREAD_PRUNED:
    default:
        break;
    }

    if (lp) {
        mprRemoveItem(lp, pt);
    }

    lp = 0;

    switch (state) {
    case MPR_POOL_THREAD_BUSY:
        lp = ps->busyThreads;
        break;

    case MPR_POOL_THREAD_IDLE:
    case MPR_POOL_THREAD_SLEEPING:
        lp = ps->idleThreads;
        break;

    case MPR_POOL_THREAD_PRUNED:
        /* Don't put on a queue and the thread will exit */
        
    default:
        break;
    }
    
    pt->state = state;

    if (lp) {
        if (mprAddItem(lp, pt) < 0) {
            mprUnlock(ps->mutex);
            return MPR_ERR_NO_MEMORY;
        }
    }
    mprUnlock(ps->mutex);

    return 0;
}



#else
void __dummyMprPool() {}
#endif /* BLD_FEATURE_MULTITHREAD */

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
/************************************************************************/
/*
 *  End of file "../mprPool.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../mprPrintf.c"
 */
/************************************************************************/

/**
 *  mprPrintf.c - Printf routines safe for embedded programming
 *
 *  This module provides safe replacements for the standard printf formatting routines. Most routines in this file 
 *  are not thread-safe. It is the callers responsibility to perform all thread synchronization.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



/*
 *  Class definitions
 */
#define CLASS_NORMAL    0               /* [All other]      Normal characters */
#define CLASS_PERCENT   1               /* [%]              Begin format */
#define CLASS_MODIFIER  2               /* [-+ #,]          Modifiers */
#define CLASS_ZERO      3               /* [0]              Special modifier - zero pad */
#define CLASS_STAR      4               /* [*]              Width supplied by arg */
#define CLASS_DIGIT     5               /* [1-9]            Field widths */
#define CLASS_DOT       6               /* [.]              Introduce precision */
#define CLASS_BITS      7               /* [hlL]            Length bits */
#define CLASS_TYPE      8               /* [cdefginopsSuxX] Type specifiers */

#define STATE_NORMAL    0               /* Normal chars in format string */
#define STATE_PERCENT   1               /* "%" */
#define STATE_MODIFIER  2               /* -+ #,*/
#define STATE_WIDTH     3               /* Width spec */
#define STATE_DOT       4               /* "." */
#define STATE_PRECISION 5               /* Precision spec */
#define STATE_BITS      6               /* Size spec */
#define STATE_TYPE      7               /* Data type */
#define STATE_COUNT     8

/*
 *  Format:         %[modifier][width][precision][bits][type]
 *
 *  [-+ #,]         Modifiers
 *  [hlL]           Length bits
 */


/*
 *  Flags
 */
#define SPRINTF_LEFT        0x1         /* Left align */
#define SPRINTF_SIGN        0x2         /* Always sign the result */
#define SPRINTF_LEAD_SPACE  0x4         /* put leading space for +ve numbers */
#define SPRINTF_ALTERNATE   0x8         /* Alternate format */
#define SPRINTF_LEAD_ZERO   0x10        /* Zero pad */
#define SPRINTF_SHORT       0x20        /* 16-bit */
#define SPRINTF_LONG        0x40        /* 32-bit */
#define SPRINTF_INT64       0x80        /* 64-bit */
#define SPRINTF_COMMA       0x100       /* Thousand comma separators */
#define SPRINTF_UPPER_CASE  0x200       /* As the name says for numbers */

typedef struct Format {
    uchar   *buf;
    uchar   *endbuf;
    uchar   *start;
    uchar   *end;
    int     growBy;
    int     maxsize;

    int     precision;
    int     radix;
    int     width;
    int     flags;
    int     len;
} Format;

#define BPUT(ctx, loc, fmt, c) \
    if (1) { \
        /* Less one to allow room for the null */ \
        if ((fmt)->end >= ((fmt)->endbuf - sizeof(char))) { \
            if (growBuf(ctx, fmt) > 0) { \
                *(fmt)->end++ = (c); \
            } \
        } else { \
            *(fmt)->end++ = (c); \
        } \
    } else

#define BPUTNULL(ctx, loc, fmt) \
    if (1) { \
        if ((fmt)->end > (fmt)->endbuf) { \
            if (growBuf(ctx, fmt) > 0) { \
                *(fmt)->end = '\0'; \
            } \
        } else { \
            *(fmt)->end = '\0'; \
        } \
    } else 


/*
 *  Numeric types to use internall
 */
#define unum    uint64
#define num     int64


static int  getState(char c, int state);
static int  growBuf(MprCtx ctx, Format *fmt);
static int  mprSprintfCore(MprCtx ctx, char **s, int maxsize, cchar *fmt, va_list arg);
static void outNum(MprCtx ctx, Format *fmt, cchar *prefix, unum val);

#if BLD_FEATURE_FLOATING_POINT
static void outFloat(MprCtx ctx, Format *fmt, char specChar, double value);
#endif


int mprPrintf(MprCtx ctx, cchar *fmt, ...)
{
    MprFileService  *fs;
    va_list         ap;
    char            *buf;
    int             len;

    /* No asserts here as this is used as part of assert reporting */

    fs = mprGetMpr(ctx)->fileService;

    va_start(ap, fmt);
    len = mprAllocVsprintf(ctx, &buf, 0, fmt, ap);
    va_end(ap);
    if (len >= 0 && fs->console) {
        len = mprWrite(fs->console, buf, len);
    }
    mprFree(buf);

    return len;
}



int mprErrorPrintf(MprCtx ctx, cchar *fmt, ...)
{
    MprFileService  *fs;
    va_list         ap;
    char            *buf;
    int             len;

    /* No asserts here as this is used as part of assert reporting */

    fs = mprGetMpr(ctx)->fileService;

    va_start(ap, fmt);
    len = mprAllocVsprintf(ctx, &buf, 0, fmt, ap);
    va_end(ap);
    if (len >= 0 && fs->error) {
        len = mprWrite(fs->error, buf, len);
    }
    mprFree(buf);

    return len;
}



int mprFprintf(MprFile *file, cchar *fmt, ...)
{
    va_list     ap;
    char        *buf;
    int         len;

    if (file == 0) {
        return MPR_ERR_BAD_HANDLE;
    }

    va_start(ap, fmt);
    len = mprAllocVsprintf(file, &buf, 0, fmt, ap);
    va_end(ap);

    if (len >= 0) {
        len = mprWrite(file, buf, len);
    }
    mprFree(buf);
    return len;
}


/*
 *  Printf with a static buffer. Used internally only. WILL NOT MALLOC.
 */
int mprStaticPrintf(MprCtx ctx, cchar *fmt, ...)
{
    MprFileService  *fs;
    va_list         ap;
    char            buf[MPR_MAX_STRING];
    char            *bufp;
    int             len;

    fs = mprGetMpr(ctx)->fileService;

    va_start(ap, fmt);
    bufp = buf;
    len = mprSprintfCore(0, &bufp, MPR_MAX_STRING, fmt, ap);
    va_end(ap);
    if (len >= 0) {
        len = mprWrite(fs->console, buf, len);
    }
    return len;
}


/*
 *  Printf with a static buffer. Used internally only. WILL NOT MALLOC.
 */
int mprStaticErrorPrintf(MprCtx ctx, cchar *fmt, ...)
{
    MprFileService  *fs;
    va_list         ap;
    char            buf[MPR_MAX_STRING];
    char            *bufp;
    int             len;

    fs = mprGetMpr(ctx)->fileService;

    va_start(ap, fmt);
    bufp = buf;
    len = mprSprintfCore(0, &bufp, MPR_MAX_STRING, fmt, ap);
    va_end(ap);
    if (len >= 0) {
        len = mprWrite(fs->error, buf, len);
    }
    return len;
}


int mprSprintf(char *buf, int n, cchar *fmt, ...)
{
    va_list     ap;
    int         result;

    mprAssert(buf);
    mprAssert(fmt);
    mprAssert(n > 0);

    va_start(ap, fmt);
    result = mprSprintfCore(0, &buf, n, fmt, ap);
    va_end(ap);
    return result;
}



int mprVsprintf(char *buf, int n, cchar *fmt, va_list arg)
{
    mprAssert(buf);
    mprAssert(fmt);
    mprAssert(n > 0);

    return mprSprintfCore(0, &buf, n, fmt, arg);
}



int mprAllocSprintf(MprCtx ctx, char **buf, int maxSize, cchar *fmt, ...)
{
    va_list ap;
    int     result;

    mprAssert(buf);
    mprAssert(fmt);

    *buf = 0;
    va_start(ap, fmt);
    result = mprSprintfCore(ctx, buf, maxSize, fmt, ap);
    va_end(ap);
    return result;
}



int mprAllocVsprintf(MprCtx ctx, char **buf, int maxSize, cchar *fmt, va_list arg)
{
    mprAssert(buf);
    mprAssert(fmt);

    *buf = 0;
    return mprSprintfCore(ctx, buf, maxSize, fmt, arg);
}



static int getState(char c, int state)
{
    /*
     *  Declared here for Brew which can't handle globals.
     */
    char stateMap[] = {
    /*     STATES:  Normal Percent Modifier Width  Dot  Prec Bits Type */
    /* CLASS           0      1       2       3     4     5    6    7  */
    /* Normal   0 */   0,     0,      0,      0,    0,    0,   0,   0,
    /* Percent  1 */   1,     0,      1,      1,    1,    1,   1,   1,
    /* Modifier 2 */   0,     2,      2,      0,    0,    0,   0,   0,
    /* Zero     3 */   0,     2,      2,      3,    5,    5,   0,   0,
    /* Star     4 */   0,     3,      3,      0,    5,    0,   0,   0,
    /* Digit    5 */   0,     3,      3,      3,    5,    5,   0,   0,
    /* Dot      6 */   0,     4,      4,      4,    0,    0,   0,   0,
    /* Bits     7 */   0,     6,      6,      6,    6,    6,   6,   0,
    /* Types    8 */   0,     7,      7,      7,    7,    7,   7,   0,
    };

    /*
     *  Format:         %[modifier][width][precision][bits][type]
     *
     *  The Class map will map from a specifier letter to a state.
     */
    char classMap[] = {
        /*   0  ' '    !     "     #     $     %     &     ' */
                 2,    0,    0,    2,    0,    1,    0,    0,
        /*  07   (     )     *     +     ,     -     .     / */
                 0,    0,    4,    2,    2,    2,    6,    0,
        /*  10   0     1     2     3     4     5     6     7 */
                 3,    5,    5,    5,    5,    5,    5,    5,
        /*  17   8     9     :     ;     <     =     >     ? */
                 5,    5,    0,    0,    0,    0,    0,    0,
        /*  20   @     A     B     C     D     E     F     G */
                 0,    0,    0,    0,    0,    0,    0,    0,
        /*  27   H     I     J     K     L     M     N     O */
                 0,    0,    0,    0,    7,    0,    0,    0,
        /*  30   P     Q     R     S     T     U     V     W */
                 0,    0,    0,    8,    0,    0,    0,    0,
        /*  37   X     Y     Z     [     \     ]     ^     _ */
                 8,    0,    0,    0,    0,    0,    0,    0,
        /*  40   '     a     b     c     d     e     f     g */
                 0,    0,    0,    8,    8,    8,    8,    8,
        /*  47   h     i     j     k     l     m     n     o */
                 7,    8,    0,    0,    7,    0,    8,    8,
        /*  50   p     q     r     s     t     u     v     w */
                 8,    0,    0,    8,    0,    8,    0,    0,
        /*  57   x     y     z  */
                 8,    0,    0,
    };

    int     chrClass;

    if (c < ' ' || c > 'z') {
        chrClass = CLASS_NORMAL;
    } else {
        mprAssert((c - ' ') < (int) sizeof(classMap));
        chrClass = classMap[(c - ' ')];
    }
    mprAssert((chrClass * STATE_COUNT + state) < (int) sizeof(stateMap));
    state = stateMap[chrClass * STATE_COUNT + state];
    return state;
}



static int mprSprintfCore(MprCtx ctx, char **bufPtr, int maxsize, cchar *spec, va_list arg)
{
    Format      fmt;
    char        *cp, *sValue, c, *tmpBuf;
    num         iValue;
    unum        uValue;
    int         count, i, len, state;

    mprAssert(bufPtr);

    if (spec == 0) {
        spec = "";
    }

    if (*bufPtr != 0) {
        mprAssert(maxsize > 0);
        fmt.buf = (uchar*) *bufPtr;
        fmt.endbuf = &fmt.buf[maxsize];
        fmt.growBy = -1;

    } else {
        if (maxsize <= 0) {
            maxsize = MAXINT;
        }

        len = min(MPR_DEFAULT_ALLOC, maxsize);
        fmt.buf = (uchar*) mprAlloc(ctx, len);
        if (fmt.buf == 0) {
            return MPR_ERR_NO_MEMORY;
        }
        fmt.endbuf = &fmt.buf[len];
        fmt.growBy = min(MPR_DEFAULT_ALLOC * 2, maxsize - len);
    }

    fmt.maxsize = maxsize;
    fmt.start = fmt.buf;
    fmt.end = fmt.buf;
    fmt.len = 0;
    *fmt.start = '\0';

    state = STATE_NORMAL;

    while ((c = *spec++) != '\0') {
        state = getState(c, state);

        switch (state) {
        case STATE_NORMAL:
            BPUT(ctx, loc, &fmt, c);
            break;

        case STATE_PERCENT:
            fmt.precision = -1;
            fmt.width = 0;
            fmt.flags = 0;
            break;

        case STATE_MODIFIER:
            switch (c) {
            case '+':
                fmt.flags |= SPRINTF_SIGN;
                break;
            case '-':
                fmt.flags |= SPRINTF_LEFT;
                break;
            case '#':
                fmt.flags |= SPRINTF_ALTERNATE;
                break;
            case '0':
                fmt.flags |= SPRINTF_LEAD_ZERO;
                break;
            case ' ':
                fmt.flags |= SPRINTF_LEAD_SPACE;
                break;
            case ',':
                fmt.flags |= SPRINTF_COMMA;
                break;
            }
            break;

        case STATE_WIDTH:
            if (c == '*') {
                fmt.width = va_arg(arg, int);
                if (fmt.width < 0) {
                    fmt.width = -fmt.width;
                    fmt.flags |= SPRINTF_LEFT;
                }
            } else {
                while (isdigit((int)c)) {
                    fmt.width = fmt.width * 10 + (c - '0');
                    c = *spec++;
                }
                spec--;
            }
            break;

        case STATE_DOT:
            fmt.precision = 0;
            fmt.flags &= ~SPRINTF_LEAD_ZERO;
            break;

        case STATE_PRECISION:
            if (c == '*') {
                fmt.precision = va_arg(arg, int);
            } else {
                while (isdigit((int) c)) {
                    fmt.precision = fmt.precision * 10 + (c - '0');
                    c = *spec++;
                }
                spec--;
            }
            break;

        case STATE_BITS:
            switch (c) {
            case 'L':
                fmt.flags |= SPRINTF_INT64;
                break;

            case 'l':
                fmt.flags |= SPRINTF_LONG;
                break;

            case 'h':
                fmt.flags |= SPRINTF_SHORT;
                break;
            }
            break;

        case STATE_TYPE:
            switch (c) {
#if BLD_FEATURE_FLOATING_POINT
            case 'e':
            case 'g':
            case 'f':
                fmt.radix = 10;
                outFloat(ctx, &fmt, c, (double) va_arg(arg, double));
                break;
#endif
            case 'c':
                BPUT(ctx, loc, &fmt, (char) va_arg(arg, int));
                break;

#if FUTURE
            case 'N':
                qualifier = va_arg(arg, char*);
                len = strlen(qualifier);
                name = va_arg(arg, char*);
                tmpBuf = mprAlloc(ctx, len + strlen(name) + 2);
                if (tmpBuf == 0) {
                    return MPR_ERR_NO_MEMORY;
                }
                strcpy(tmpBuf, qualifier);
                tmpBuf[len++] = ':';
                strcpy(&tmpBuf[len], name);
                sValue = tmpBuf;
                goto emitString;
#endif

            case 's':
            case 'S':
                sValue = va_arg(arg, char*);
                tmpBuf = 0;

#if FUTURE
            emitString:
#endif
                if (sValue == 0) {
                    sValue = "null";
                    len = (int) strlen(sValue);
                } else if (fmt.flags & SPRINTF_ALTERNATE) {
                    sValue++;
                    len = (int) *sValue;
                } else if (fmt.precision >= 0) {
                    /*
                     *  Can't use strlen(), the string may not have a null
                     */
                    cp = sValue;
                    for (len = 0; len < fmt.precision; len++) {
                        if (*cp++ == '\0') {
                            break;
                        }
                    }
                } else {
                    len = (int) strlen(sValue);
                }
                if (!(fmt.flags & SPRINTF_LEFT)) {
                    for (i = len; i < fmt.width; i++) {
                        BPUT(ctx, loc, &fmt, (char) ' ');
                    }
                }
                for (i = 0; i < len && *sValue; i++) {
                    BPUT(ctx, loc, &fmt, *sValue++);
                }
                if (fmt.flags & SPRINTF_LEFT) {
                    for (i = len; i < fmt.width; i++) {
                        BPUT(ctx, loc, &fmt, (char) ' ');
                    }
                }
                if (tmpBuf) {
                    mprFree(tmpBuf);
                }
                break;

            case 'i':
                ;
            case 'd':
                fmt.radix = 10;
                if (fmt.flags & SPRINTF_SHORT) {
                    iValue = (short) va_arg(arg, int);
                } else if (fmt.flags & SPRINTF_LONG) {
                    iValue = va_arg(arg, long);
                } else if (fmt.flags & SPRINTF_INT64) {
                    iValue = va_arg(arg, num);
                } else {
                    iValue = va_arg(arg, int);
                }
                if (iValue >= 0) {
                    if (fmt.flags & SPRINTF_LEAD_SPACE) {
                        outNum(ctx, &fmt, " ", iValue);
                    } else if (fmt.flags & SPRINTF_SIGN) {
                        outNum(ctx, &fmt, "+", iValue);
                    } else {
                        outNum(ctx, &fmt, 0, iValue);
                    }
                } else {
                    outNum(ctx, &fmt, "-", -iValue);
                }
                break;

            case 'X':
                fmt.flags |= SPRINTF_UPPER_CASE;
                /*  Fall through  */
            case 'o':
            case 'x':
            case 'u':
                if (fmt.flags & SPRINTF_SHORT) {
                    uValue = (ushort) va_arg(arg, uint);
                } else if (fmt.flags & SPRINTF_LONG) {
                    uValue = va_arg(arg, ulong);
                } else if (fmt.flags & SPRINTF_INT64) {
                    uValue = va_arg(arg, unum);
                } else {
                    uValue = va_arg(arg, uint);
                }
                if (c == 'u') {
                    fmt.radix = 10;
                    outNum(ctx, &fmt, 0, uValue);
                } else if (c == 'o') {
                    fmt.radix = 8;
                    if (fmt.flags & SPRINTF_ALTERNATE && uValue != 0) {
                        outNum(ctx, &fmt, "0", uValue);
                    } else {
                        outNum(ctx, &fmt, 0, uValue);
                    }
                } else {
                    fmt.radix = 16;
                    if (fmt.flags & SPRINTF_ALTERNATE && uValue != 0) {
                        if (c == 'X') {
                            outNum(ctx, &fmt, "0X", uValue);
                        } else {
                            outNum(ctx, &fmt, "0x", uValue);
                        }
                    } else {
                        outNum(ctx, &fmt, 0, uValue);
                    }
                }
                break;

            case 'n':       /* Count of chars seen thus far */
                if (fmt.flags & SPRINTF_SHORT) {
                    short *count = va_arg(arg, short*);
                    *count = (int) (fmt.end - fmt.start);
                } else if (fmt.flags & SPRINTF_LONG) {
                    long *count = va_arg(arg, long*);
                    *count = (int) (fmt.end - fmt.start);
                } else {
                    int *count = va_arg(arg, int *);
                    *count = (int) (fmt.end - fmt.start);
                }
                break;

            case 'p':       /* Pointer */
#if __WORDSIZE == 64 || X86_64
                uValue = (unum) va_arg(arg, void*);
#else
                uValue = (uint) PTOI(va_arg(arg, void*));
#endif
                fmt.radix = 16;
                outNum(ctx, &fmt, "0x", uValue);
                break;

            default:
                BPUT(ctx, loc, &fmt, c);
            }
        }
    }
    BPUTNULL(ctx, loc, &fmt);

    count = (int) (fmt.end - fmt.start);
    if (*bufPtr == 0) {
        *bufPtr = (char*) fmt.buf;
    }
    return count;
}



/*
 *  Output a number according to the given format. 
 */
static void outNum(MprCtx ctx, Format *fmt, cchar *prefix, unum value)
{
    char    numBuf[64];
    char    *cp;
    char    *endp;
    char    c;
    int     letter, len, leadingZeros, i, fill;

    endp = &numBuf[sizeof(numBuf) - 1];
    *endp = '\0';
    cp = endp;

    /*
     *  Convert to ascii
     */
    if (fmt->radix == 16) {
        do {
            letter = (int) (value % fmt->radix);
            if (letter > 9) {
                if (fmt->flags & SPRINTF_UPPER_CASE) {
                    letter = 'A' + letter - 10;
                } else {
                    letter = 'a' + letter - 10;
                }
            } else {
                letter += '0';
            }
            *--cp = letter;
            value /= fmt->radix;
        } while (value > 0);

    } else if (fmt->flags & SPRINTF_COMMA) {
        i = 1;
        do {
            *--cp = '0' + (int) (value % fmt->radix);
            value /= fmt->radix;
            if ((i++ % 3) == 0 && value > 0) {
                *--cp = ',';
            }
        } while (value > 0);
    } else {
        do {
            *--cp = '0' + (int) (value % fmt->radix);
            value /= fmt->radix;
        } while (value > 0);
    }

    len = (int) (endp - cp);
    fill = fmt->width - len;

    if (prefix != 0) {
        fill -= (int) strlen(prefix);
    }
    leadingZeros = (fmt->precision > len) ? fmt->precision - len : 0;
    fill -= leadingZeros;

    if (!(fmt->flags & SPRINTF_LEFT)) {
        c = (fmt->flags & SPRINTF_LEAD_ZERO) ? '0': ' ';
        for (i = 0; i < fill; i++) {
            BPUT(ctx, loc, fmt, c);
        }
    }
    if (prefix != 0) {
        while (*prefix) {
            BPUT(ctx, loc, fmt, *prefix++);
        }
    }
    for (i = 0; i < leadingZeros; i++) {
        BPUT(ctx, loc, fmt, '0');
    }
    while (*cp) {
        BPUT(ctx, loc, fmt, *cp);
        cp++;
    }
    if (fmt->flags & SPRINTF_LEFT) {
        for (i = 0; i < fill; i++) {
            BPUT(ctx, loc, fmt, ' ');
        }
    }
}


#if BLD_FEATURE_FLOATING_POINT
/*
 *  Output a floating point number
 */

static void outFloat(MprCtx ctx, Format *fmt, char specChar, double value)
{
    char    *cp;
#if FUTURE
    char    numBuf[128];
    char    *endp;
    char    c;
    int     letter, len, leadingZeros, i, fill, width, precision;

    endp = &numBuf[sizeof(numBuf) - 1];
    *endp = '\0';

    precision = fmt->precision;
    if (precision < 0) {
        precision = 6;
    } else if (precision > (sizeof(numBuf) - 1)) {
        precision = (sizeof(numBuf) - 1);
    }
    width = min(fmt->width, sizeof(numBuf) - 1);

    if (__isnanl(value)) {
        "nan"
    } else if (__isinfl(value)) {
        "infinity"
    } else if (value < 0) {
        prefix = "-";
    } else if (fmt.flags & SPRINTF_LEAD_SPACE) {
        prefix = " ";
    } else if (fmt.flags & SPRINTF_SIGN) {
        prefix = "+";
    } 


    /*
     *  Do the exponent part
     */
    cp = &numBuf[sizeof(numBuf) - precision];
    for (i = 0; i < precision; i++) {
        *cp++ = '0' + (int) (value % fmt->radix);
        value /= fmt->radix;
    }

    /*
     *  Do the decimal part
     */
    if (fmt->flags & SPRINTF_COMMA) {
        i = 1;
        do {
            *--cp = '0' + (int) (value % fmt->radix);
            value /= fmt->radix;
            if ((i++ % 3) == 0 && value > 0) {
                *--cp = ',';
            }
        } while (value >= 1.0);

    } else {
        do {
            *--cp = '0' + (int) (value % fmt->radix);
            value /= fmt->radix;
        } while (value > 1.0);
    }

    len = endp - cp;
    fill = fmt->width - len;

    if (prefix != 0) {
        fill -= strlen(prefix);
    }

    leadingZeros = (fmt->precision > len) ? fmt->precision - len : 0;
    fill -= leadingZeros;

    if (!(fmt->flags & SPRINTF_LEFT)) {
        c = (fmt->flags & SPRINTF_LEAD_ZERO) ? '0': ' ';
        for (i = 0; i < fill; i++) {
            BPUT(ctx, loc, fmt, c);
        }
    }
    if (prefix != 0) {
        BPUT(ctx, loc, fmt, prefix);
    }
    for (i = 0; i < leadingZeros; i++) {
        BPUT(ctx, loc, fmt, '0');
    }
    BPUT(ctx, loc, fmt, cp);
    if (fmt->flags & SPRINTF_LEFT) {
        for (i = 0; i < fill; i++) {
            BPUT(ctx, loc, fmt, ' ');
        }
    }
#else
    /*
     *  Must be able to store ~300 digits if using %f
     */
    char    numBuf[512];
#if BLD_WIN_LIKE
    int     oldFormat = 0;
    oldFormat = _set_output_format(_TWO_DIGIT_EXPONENT);
#endif
    if (specChar == 'f') {
        sprintf(numBuf, "%*.*f", fmt->width, fmt->precision, value);
    } else if (specChar == 'g') {
        sprintf(numBuf, "%*.*g", fmt->width, fmt->precision, value);
    } else if (specChar == 'e') {
        sprintf(numBuf, "%*.*e", fmt->width, fmt->precision, value);
    }
#if BLD_WIN_LIKE
    _set_output_format(oldFormat);
#endif
    for (cp = numBuf; *cp; cp++) {
        BPUT(ctx, loc, fmt, *cp);
    }
#endif
}

#endif /* BLD_FEATURE_FLOATING_POINT */


/*
 *  Grow the buffer to fit new data. Return 1 if the buffer can grow. 
 *  Grow using the growBy size specified when creating the buffer. 
 */
static int growBuf(MprCtx ctx, Format *fmt)
{
    uchar   *newbuf;
    int     buflen;

    buflen = (int) (fmt->endbuf - fmt->buf);
    if (fmt->maxsize >= 0 && buflen >= fmt->maxsize) {
        return 0;
    }
    if (fmt->growBy <= 0) {
        /*
         *  User supplied buffer
         */
        return 0;
    }

    newbuf = (uchar*) mprAlloc(ctx, buflen + fmt->growBy);
    if (newbuf == 0) {
        return MPR_ERR_NO_MEMORY;
    }
    if (fmt->buf) {
        memcpy(newbuf, fmt->buf, buflen);
        mprFree(fmt->buf);
    }

    buflen += fmt->growBy;
    fmt->end = newbuf + (fmt->end - fmt->buf);
    fmt->start = newbuf + (fmt->start - fmt->buf);
    fmt->buf = newbuf;
    fmt->endbuf = &fmt->buf[buflen];

    /*
     *  Increase growBy to reduce overhead
     */
    if ((buflen + (fmt->growBy * 2)) < fmt->maxsize) {
        fmt->growBy *= 2;
    }
    return 1;
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
/************************************************************************/
/*
 *  End of file "../mprPrintf.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../mprRomFile.c"
 */
/************************************************************************/

/*
 *  mprRomFile.c - ROM File system
 *
 *  ROM support for systems without disk or flash based file systems. This module provides read-only file retrieval 
 *  from compiled file images. Use the mprRomComp program to compile files into C code and then link them into your 
 *  application. This module uses a hashed symbol table for fast file lookup.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



#if BLD_FEATURE_ROMFS 

static void closeFile(MprFile *file);
static int getFileInfo(MprRomFileService *rfs, cchar *path, MprFileInfo *info);
static MprRomInode *lookup(MprRomFileService *rfs, cchar *path);


static MprFile *openFile(MprCtx ctx, MprFileService *fileService, cchar *path, int flags, int omode)
{
    MprRomFileService   *rfs;
    MprFile             *file;
    
    mprAssert(path && *path);

    rfs = (MprRomFileService*) fileService;
    file = mprAllocObjZeroed(ctx, MprFile, closeFile);
    file->fileService = fileService;
    file->mode = omode;

    if ((file->inode = lookup(rfs, path)) == 0) {
        return 0;
    }
    return file;
}


static void closeFile(MprFile *file)
{
}


static int readFile(MprFile *file, void *buf, uint size)
{
    MprRomInode     *inode;
    int             len;

    mprAssert(buf);

    inode = file->inode;
    len = min(inode->size - file->pos, size);
    memcpy(buf, &inode->data[file->pos], len);
    file->pos += len;

    return len;
}



static int writeFile(MprFile *file, const void *buf, uint size)
{
    return MPR_ERR_CANT_WRITE;
}



static long seekFile(MprFile *file, int seekType, long distance)
{
    MprRomInode     *inode;

    mprAssert(seekType == SEEK_SET || seekType == SEEK_CUR || seekType == SEEK_END);

    inode = file->inode;

    switch (seekType) {
    case SEEK_CUR:
        file->pos += distance;
        break;

    case SEEK_END:
        file->pos = inode->size + distance;
        break;

    default:
        file->pos = distance;
        break;
    }

    if (file->pos < 0) {
        errno = EBADF;
        return MPR_ERR_BAD_STATE;
    }
    return file->pos;
}



static bool accessFile(MprRomFileService *fileService, cchar *path, int omode)
{
    MprFileInfo     info;
    return getFileInfo(fileService, path, &info) == 0 ? 1 : 0;
}



static int deleteFile(MprRomFileService *fileService, cchar *path)
{
    return MPR_ERR_CANT_WRITE;
}



static int deleteDir(MprRomFileService *fileService, cchar *path)
{
    return MPR_ERR_CANT_WRITE;
}
 


static int makeDir(MprRomFileService *fileService, cchar *path, int perms)
{
    return MPR_ERR_CANT_WRITE;
}



static int getFileInfo(MprRomFileService *rfs, cchar *path, MprFileInfo *info)
{
    MprRomInode *ri;

    mprAssert(path && *path);

    if ((ri = (MprRomInode*) lookup(rfs, path)) == 0) {
        return MPR_ERR_NOT_FOUND;
    }
    memset(info, 0, sizeof(MprFileInfo));

    info->valid = 1;
    info->size = ri->size;
    info->mtime = 0;
    info->inode = ri->num;

    if (ri->data == 0) {
        info->isDir = 1;
        info->isReg = 0;
    } else {
        info->isReg = 1;
        info->isDir = 0;
    }
    return 0;
}



static MprRomInode *lookup(MprRomFileService *rfs, cchar *path)
{
    if (path == 0) {
        return 0;
    }

    /*
     *  Remove "./" segments
     */
    while (*path == '.') {
        if (path[1] == '\0') {
            path++;
        } else if (path[1] == '/') {
            path += 2;
        } else {
            break;
        }
    }

    /*
     *  Skip over the leading "/"
     */
    if (*path == '/') {
        path++;
    }

    return (MprRomInode*) mprLookupHash(rfs->fileIndex, path);
}



int mprSetRomFileSystem(MprCtx ctx, MprRomInode *inodeList)
{
    MprRomFileService   *rfs;
    MprRomInode         *ri;

    rfs = (MprRomFileService*) mprGetMpr(ctx)->fileService;

    rfs->romInodes = inodeList;
    rfs->fileIndex = mprCreateHash(rfs, MPR_FILES_HASH_SIZE);

    for (ri = inodeList; ri->path; ri++) {
        if (mprAddHash(rfs->fileIndex, ri->path, ri) < 0) {
            return MPR_ERR_NO_MEMORY;
        }
    }

    return 0;
}



MprRomFileService *mprCreateRomFileService(MprCtx ctx)
{
    MprFileService      *fs;
    MprRomFileService   *rfs;

    rfs = mprAllocObjZeroed(ctx, MprRomFileService);
    if (rfs == 0) {
        return rfs;
    }

    fs = &rfs->fileService;
    fs->accessFile = (MprAccessFileProc) accessFile;
    fs->deleteFile = (MprDeleteFileProc) deleteFile;
    fs->deleteDir = (MprDeleteDirProc) deleteDir;
    fs->getFileInfo = (MprGetFileInfoProc) getFileInfo;
    fs->makeDir = (MprMakeDirProc) makeDir;
    fs->openFile = (MprOpenFileProc) openFile;
    fs->closeFile = closeFile;
    fs->readFile = readFile;
    fs->seekFile = seekFile;
    fs->writeFile = writeFile;

    return rfs;
}


#else /* BLD_FEATURE_ROMFS */
void __dummy_romfs() {};
#endif /* BLD_FEATURE_ROMFS */

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
/************************************************************************/
/*
 *  End of file "../mprRomFile.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../mprSelectWait.c"
 */
/************************************************************************/

/**
 *  mprSelectWait.c - Wait for I/O by using select.
 *
 *  This module provides I/O wait management for sockets on Unix like systems. Windows uses a different mechanism
 *  see mprAsyncSelectWait. This module is thread-safe.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



#if BLD_UNIX_LIKE

static void getWaitFds(MprWaitService *ws);
static void growFds(MprWaitService *ws);
static void serviceIO(MprWaitService *ws);


int mprInitSelectWait(MprWaitService *ws)
{
    /*
     *  Initialize the "wakeup" pipe. This is used to wakeup the service thread if other threads need to wait for I/O.
     */
    if (pipe(ws->breakPipe) < 0) {
        return MPR_ERR_CANT_INITIALIZE;
    }

    fcntl(ws->breakPipe[0], F_SETFL, fcntl(ws->breakPipe[0], F_GETFL) | O_NONBLOCK);
    fcntl(ws->breakPipe[1], F_SETFL, fcntl(ws->breakPipe[1], F_GETFL) | O_NONBLOCK);

    return 0;
}


/*
 *  Wait for I/O on a single file descriptor. Return a mask of events found. Mask is the events of interest.
 *  timeout is in milliseconds.
 *  TODO - why is there an fd argument and wp is ignored? Remove one or the other
 */
int mprWaitForSingleIO(MprWaitHandler *wp, int fd, int mask, int timeout)
{
    struct pollfd   fds[1];

    fds[0].fd = fd;
    fds[0].events = 0;
    fds[0].revents = 0;

    if (mask & MPR_READABLE) {
        fds[0].events |= POLLIN;
    }
    if (mask & MPR_WRITEABLE) {
        fds[0].events |= POLLOUT;
    }
    if (poll(fds, 1, timeout) > 0) {
        if (fds[0].revents & POLLIN) {
            mask |= MPR_READABLE;
        }
        if (fds[0].revents & POLLOUT) {
            mask |= MPR_WRITEABLE;
        }
        return 1;
    }
    return 0;
}


/*
 *  Wait for I/O on all registered file descriptors. Timeout is in milliseconds. Return the number of events detected.
 */
int mprWaitForIO(MprWaitService *ws, int timeout)
{
    MprWaitHandler  *wp;
    int             count, index, lastChange;

    /*
     *  There is a race here, but no problem. If the masks are updated after this test, the breakout port will wake us up soon.
     */
    if (ws->lastMaskGeneration != ws->maskGeneration) {
        getWaitFds(ws);
    }

    if (ws->flags & MPR_NEED_RECALL) {

        mprLock(ws->mutex);
        ws->flags &= ~MPR_NEED_RECALL;
        lastChange = ws->listGeneration;

        for (count = index = 0; (wp = (MprWaitHandler*) mprGetNextItem(ws->list, &index)) != 0; ) {
            if (wp->flags & MPR_WAIT_RECALL_HANDLER) {
                count++;

                mprUnlock(ws->mutex);
                mprInvokeWaitCallback(wp, 0);
                mprLock(ws->mutex);

                if (lastChange != ws->listGeneration) {
                    index = 0;
                }
            }
        }
        mprUnlock(ws->mutex);

    } else if ((count = poll(ws->fds, ws->fdsCount, timeout)) > 0) {
        serviceIO(ws);
    }
    return count;
}


/*
 *  Get the waiting file descriptors
 */
static void getWaitFds(MprWaitService *ws)
{
    MprWaitHandler  *wp;
    struct pollfd   *pollfd;
    int             mask, next;

    mprLock(ws->mutex);

    ws->lastMaskGeneration = ws->maskGeneration;

    /*
     *  Add the breakout port to wakeup the service thread when other threads need selecting services.
     */
    growFds(ws);
    pollfd = ws->fds;
    pollfd->fd = ws->breakPipe[MPR_READ_PIPE];
    pollfd->events = POLLIN;
    pollfd++;

    /*
     *  Add an entry for each descriptor desiring service.
     */
    next = 0;
    wp = (MprWaitHandler*) mprGetNextItem(ws->list, &next);
    while (wp) {
        mprAssert(wp->fd >= 0);

        if (wp->proc && wp->desiredMask) {
            /*
             *  The disable mask will be zero when we are already servicing an event. This prevents recursive service.
             */
            mask = wp->desiredMask & wp->disableMask;
            pollfd->events = 0;
            if (mask & MPR_READABLE) {
                pollfd->events |= POLLIN;
            }
            if (mask & MPR_WRITEABLE) {
                pollfd->events |= POLLOUT;
            }
            if (pollfd->events) {
                pollfd->fd = wp->fd;
                pollfd++;
            }
        }
        wp = (MprWaitHandler*) mprGetNextItem(ws->list, &next);
    }
    ws->fdsCount = (int) (pollfd - ws->fds);
    mprAssert(ws->fdsCount <= ws->fdsSize);

    mprUnlock(ws->mutex);
}


/*
 *  Service I/O events
 */
static void serviceIO(MprWaitService *ws)
{
    MprWaitHandler      *wp;
    struct pollfd       *fds;
    int                 i, mask, lastChange, index;

    /*
     *  Must have the wait list stable while we service events
     */
    mprLock(ws->mutex);

    /*
     *  Service the breakout pipe first
     */
    if (ws->fds[0].revents & POLLIN) {
        char    buf[128];
        read(ws->breakPipe[MPR_READ_PIPE], buf, sizeof(buf));
        ws->flags &= ~MPR_BREAK_REQUESTED;
    }

    lastChange = ws->listGeneration;

    /*
     *  Now service all IO wait handlers
     */
    for (i = 1; i < ws->fdsCount; ) {
startAgain:
        mprAssert(i < ws->fdsSize);
        fds = &ws->fds[i++];
        if (fds->revents == 0) {
            continue;
        }

        /*
         *  Go in reverse order to maximize the chance of getting the most active connection
         *  TODO OPT. Would be faster to have an fd lookup array where the entries are wp's
         */
        for (index = -1; (wp = (MprWaitHandler*) mprGetPrevItem(ws->list, &index)) != 0; ) {
            mprAssert(wp->fd >= 0);
            if (wp->fd != fds->fd) {
                continue;
            }

            /*
             *  Present mask is only cleared after the io handler callback has completed
             */
            mask = 0;
            if ((wp->desiredMask & MPR_READABLE) && fds->revents & (POLLIN | POLLHUP | POLLERR | POLLNVAL)) {
                mask |= MPR_READABLE;
            }
            if ((wp->desiredMask & MPR_WRITEABLE) && fds->revents & POLLOUT) {
                mask |= MPR_WRITEABLE;
            }
            if (mask == 0) {
                break;
            }

            if (mask & wp->desiredMask) {

#if BLD_FEATURE_MULTITHREAD
                /*
                 *  Disable events to prevent recursive I/O events. Callback must call mprEnableWaitEvents
                 */
                wp->disableMask = 0;
                ws->maskGeneration++;
#endif
                wp->presentMask = mask;

                mprUnlock(ws->mutex);
                mprInvokeWaitCallback(wp, 0);
                mprLock(ws->mutex);

                if (lastChange != ws->listGeneration) {
                    /*
                     *  Note: the list may have changed while unlocked. So we must rescan.
                     */
                    if (i < ws->fdsCount) {
                        fds->revents = 0;
                        goto startAgain;
                    }
                }
            }
            break;
        }
        fds->revents = 0;
    }

    mprUnlock(ws->mutex);
}


#if BLD_FEATURE_MULTITHREAD
/*
 *  Awaken the wait service (i.e. select/poll call)
 */
void mprAwakenWaitService(MprWaitService *ws)
{
    MprThread   *current;
    int         c;

    current = mprGetCurrentThread(ws);
    if (current == ws->serviceThread) {
        return;
    }

    mprLock(ws->mutex);
    if (!(ws->flags & MPR_BREAK_REQUESTED)) {
        c = 0;
        write(ws->breakPipe[MPR_WRITE_PIPE], (char*) &c, 1);
        ws->flags |= MPR_BREAK_REQUESTED;
    }
    mprUnlock(ws->mutex);
}
#endif


/*
 *  Grow the fds list as required. Never shrink.
 */
static void growFds(MprWaitService *ws)
{
    int     len;

    len = max(ws->fdsSize, mprGetListCount(ws->list) + 1);
    if (len > ws->fdsSize) {
        ws->fds = mprRealloc(ws, ws->fds, len * sizeof(struct pollfd));
        if (ws->fds == 0) {
            /*  Global memory allocation handler will handle this */
            return;
        }
        memset(&ws->fds[ws->fdsSize], 0, (len - ws->fdsSize) * sizeof(struct pollfd));
        ws->fdsSize = len;
    }
}


/*
 *  Set a handler to be recalled without further I/O
 */
void mprRecallWaitHandler(MprWaitHandler *wp)
{
    MprWaitService  *ws;

    ws = wp->waitService;

    /*
     *  No locking needed, order important
     */
    wp->flags |= MPR_WAIT_RECALL_HANDLER;
    ws->flags |= MPR_NEED_RECALL;

    mprAwakenWaitService(wp->waitService);
}


/*
 *  Modify a handler's interested events
 */
void mprSetWaitInterest(MprWaitHandler *wp, int mask)
{
    MprWaitService  *ws;

    ws = wp->waitService;
    mprLock(ws->mutex);
	wp->desiredMask = mask;
	mprModifyWaitHandler(ws, wp, 1);
    mprUnlock(ws->mutex);
}


void mprDisableWaitEvents(MprWaitHandler *wp, bool wakeUp)
{
    /*  TODO -- better if we locked just our io handler  */
    mprLock(wp->waitService->mutex);
    wp->disableMask = 0;
    mprModifyWaitHandler(wp->waitService, wp, wakeUp);
    mprUnlock(wp->waitService->mutex);
}


void mprEnableWaitEvents(MprWaitHandler *wp, bool wakeUp)
{
    mprLock(wp->waitService->mutex);
    wp->disableMask = -1;
    mprModifyWaitHandler(wp->waitService, wp, wakeUp);
    mprUnlock(wp->waitService->mutex);
}

#endif /* BLD_UNIX_LIKE */

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
/************************************************************************/
/*
 *  End of file "../mprSelectWait.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../mprSocket.c"
 */
/************************************************************************/

/**
 *  mprSocket.c - Convenience class for the management of sockets
 *
 *  This module provides a higher level interface to interact with the standard sockets API. It does not perform
 *  buffering.
 *
 *  This module is thread-safe.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



//#if !MACOSX
/*
 *  Set this to 0 to disable the IPv6 address service. You can do this if you only need IPv4.
 *  On MAC OS X, getaddrinfo is not thread-safe and crashes when called by a 2nd thread at any time. ie. locking wont help.
 */
#define BLD_HAS_GETADDRINFO 1
//#endif


static void acceptHandler(void *sp, int mask, bool isPoolThread);
static MprSocket *acceptSocket(MprSocket *sp, bool invokeCallback);
static void closeSocket(MprSocket *sp, bool gracefully);
static int  connectSocket(MprSocket *sp, cchar *host, int port, int initialFlags);
static MprSocket *createSocket(MprCtx ctx, struct MprSsl *ssl);
static MprSocketProvider *createStandardProvider(MprSocketService *ss);
static int  flushSocket(MprSocket *sp);
static int  getSocketAddress(MprSocket *sp, cchar *host, int port, int *family, struct sockaddr **addr, socklen_t *addrlen);
static int  getSocketName(MprSocket *sp, struct sockaddr *addr, int addrlen, char *host, int hostLen, int *port);
static void ioProc(MprSocket *sp, int mask, bool isPoolThread);
static int  listenSocket(MprSocket *sp, cchar *host, int port, MprSocketAcceptProc acceptFn, void *data, int initialFlags);
static int  readSocket(MprSocket *sp, void *buf, int bufsize);
static int  socketDestructor(MprSocket *sp);
static int  writeSocket(MprSocket *sp, void *buf, int bufsize);

/*
 *  Open the socket service
 */

MprSocketService *mprCreateSocketService(MprCtx ctx)
{
    MprSocketService    *ss;

    mprAssert(ctx);

    ss = mprAllocObj(ctx, MprSocketService);
    if (ss == 0) {
        return 0;
    }
    ss->maxClients = INT_MAX;
    ss->numClients = 0;

    ss->standardProvider = createStandardProvider(ss);
    if (ss->standardProvider == NULL) {
        mprFree(ss);
        return 0;
    }
    ss->secureProvider = NULL;

#if BLD_FEATURE_MULTITHREAD
    ss->mutex = mprCreateLock(ss);
    if (ss->mutex == 0) {
        mprFree(ss);
        return 0;
    }
#endif
    return ss;
}


/*
 *  Start the socket service
 */
int mprStartSocketService(MprSocketService *ss)
{
    char    hostName[MPR_MAX_IP_NAME], serverName[MPR_MAX_IP_NAME], domainName[MPR_MAX_IP_NAME], *dp;

    mprAssert(ss);

    serverName[0] = '\0';
    domainName[0] = '\0';
    hostName[0] = '\0';

    if (gethostname(serverName, sizeof(serverName)) < 0) {
        mprStrcpy(serverName, sizeof(serverName), "localhost");
        mprUserError(ss, "Can't get host name. Using \"localhost\".");
        /* Keep going */
    }

    if ((dp = strchr(serverName, '.')) != 0) {
        mprStrcpy(hostName, sizeof(hostName), serverName);
        *dp++ = '\0';
        mprStrcpy(domainName, sizeof(domainName), dp);

    } else {
        mprStrcpy(hostName, sizeof(hostName), serverName);
    }

    mprLock(ss->mutex);

    mprSetServerName(ss, serverName);
    mprSetDomainName(ss, domainName);
    mprSetHostName(ss, hostName);

    mprUnlock(ss->mutex);

    return 0;
}


void mprStopSocketService(MprSocketService *ss)
{
}


static MprSocketProvider *createStandardProvider(MprSocketService *ss)
{
    MprSocketProvider   *provider;

    provider = mprAllocObj(ss, MprSocketProvider);
    if (provider == 0) {
        return 0;
    }

    provider->name = "standard";
    provider->acceptSocket = acceptSocket;
    provider->closeSocket = closeSocket;
    provider->connectSocket = connectSocket;
    provider->createSocket = createSocket;
    provider->flushSocket = flushSocket;
    provider->listenSocket = listenSocket;
    provider->readSocket = readSocket;
    provider->writeSocket = writeSocket;

    return provider;
}


void mprSetSecureProvider(MprCtx ctx, MprSocketProvider *provider)
{
    mprGetMpr(ctx)->socketService->secureProvider = provider;
}


bool mprHasSecureSockets(MprCtx ctx)
{
    return (mprGetMpr(ctx)->socketService->secureProvider != 0);
}


int mprSetMaxSocketClients(MprCtx ctx, int max)
{
    MprSocketService    *ss;

    mprAssert(ctx);
    mprAssert(max >= 0);

    ss = mprGetMpr(ctx)->socketService;
    ss->maxClients = max;

    return 0;
}


/*
 *  Create a new socket
 */
static MprSocket *createSocket(MprCtx ctx, struct MprSsl *ssl)
{
    MprSocket       *sp;

    sp = mprAllocObjWithDestructorZeroed(ctx, MprSocket, socketDestructor);
    if (sp == 0) {
        return 0;
    }

    sp->handlerPriority = MPR_NORMAL_PRIORITY;
    sp->port = -1;
    sp->fd = -1;

#if BLD_FEATURE_MULTITHREAD
    sp->mutex = mprCreateLock(sp);
#endif

    sp->provider = mprGetMpr(ctx)->socketService->standardProvider;
    sp->service = mprGetMpr(ctx)->socketService;

    return sp;
}


/*
 *  Create a new socket
 */
MprSocket *mprCreateSocket(MprCtx ctx, struct MprSsl *ssl)
{
    MprSocketService    *ss;
    MprSocket           *sp;

    ss = mprGetMpr(ctx)->socketService;

    if (ssl) {
#if !BLD_FEATURE_SSL
        return 0;
#endif
        if (ss->secureProvider == NULL || ss->secureProvider->createSocket == NULL) {
            return 0;
        }
        sp = ss->secureProvider->createSocket(ctx, ssl);

    } else {
        mprAssert(ss->standardProvider->createSocket);
        sp = ss->standardProvider->createSocket(ctx, NULL);
    }
    return sp;
}


static int socketDestructor(MprSocket *sp)
{
    mprLock(sp->mutex);

    if (sp->fd >= 0) {
        mprCloseSocket(sp, 1);
        sp->fd = 0;
    }
    return 0;
}


/*
 *  Re-initialize all socket variables so the Socket can be reused.
 */
static void resetSocket(MprSocket *sp)
{
    if (sp->fd >= 0) {
        mprCloseSocket(sp, 0);
    }

    if (sp->flags & MPR_SOCKET_CLOSED) {
        sp->acceptCallback = 0;
        sp->acceptData = 0;
        sp->waitForEvents = 0;
        sp->currentEvents = 0;
        sp->error = 0;
        sp->flags = 0;
        sp->ioCallback = 0;
        sp->ioData = 0;
        sp->ioData2 = 0;
        sp->handlerMask = 0;
        sp->handlerPriority = MPR_NORMAL_PRIORITY;
        sp->interestEvents = 0;
        sp->port = -1;
        sp->fd = -1;

        mprFree(sp->ipAddr);
        sp->ipAddr = 0;
    }
    mprAssert(sp->provider);
}


/*
 *  Open a server socket connection
 */
int mprOpenServerSocket(MprSocket *sp, cchar *host, int port, MprSocketAcceptProc acceptFn, void *data, int flags)
{
    return sp->provider->listenSocket(sp, host, port, acceptFn, data, flags);
}


static int listenSocket(MprSocket *sp, cchar *host, int port, MprSocketAcceptProc acceptFn, void *data, int initialFlags)
{
    struct sockaddr     *addr;
    socklen_t           addrlen;
    int                 datagram, family, rc;

    mprLock(sp->mutex);

    if (host == 0 || *host == '\0') {
        mprLog(sp, 6, "mprSocket: openServer *:%d, flags %x", port, initialFlags);
    } else {
        mprLog(sp, 6, "mprSocket: openServer %s:%d, flags %x", host, port, initialFlags);
    }

    resetSocket(sp);

    sp->ipAddr = mprStrdup(sp, host);
    sp->port = port;
    sp->acceptCallback = acceptFn;
    sp->acceptData = data;

    sp->flags = (initialFlags &
        (MPR_SOCKET_BROADCAST | MPR_SOCKET_DATAGRAM | MPR_SOCKET_BLOCK |
         MPR_SOCKET_LISTENER | MPR_SOCKET_NOREUSE | MPR_SOCKET_NODELAY | MPR_SOCKET_THREAD));

    datagram = sp->flags & MPR_SOCKET_DATAGRAM;

    if (getSocketAddress(sp, host, port, &family, &addr, &addrlen) < 0) {
        return MPR_ERR_NOT_FOUND;
    }

    /*
     *  Create the O/S socket
     */
    sp->fd = (int) socket(family, datagram ? SOCK_DGRAM: SOCK_STREAM, 0);
    if (sp->fd < 0) {
        mprUnlock(sp->mutex);
        return MPR_ERR_CANT_OPEN;
    }

#if !BLD_WIN_LIKE && !VXWORKS
    /*
     *  Children won't inherit this fd
     */
    fcntl(sp->fd, F_SETFD, FD_CLOEXEC);
#endif

#if BLD_UNIX_LIKE
    if (!(sp->flags & MPR_SOCKET_NOREUSE)) {
        rc = 1;
        setsockopt(sp->fd, SOL_SOCKET, SO_REUSEADDR, (char*) &rc, sizeof(rc));
    }
#endif

    rc = bind(sp->fd, addr, addrlen);
    if (rc < 0) {
        rc = errno;
        mprFree(addr);
        closesocket(sp->fd);
        sp->fd = -1;
        mprUnlock(sp->mutex);
        return MPR_ERR_CANT_OPEN;
    }
    mprFree(addr);

    if (! datagram) {
        sp->flags |= MPR_SOCKET_LISTENER;
        if (listen(sp->fd, SOMAXCONN) < 0) {
            mprLog(sp, 3, "Listen error %d", mprGetOsError());
            closesocket(sp->fd);
            sp->fd = -1;
            mprUnlock(sp->mutex);
            return MPR_ERR_CANT_OPEN;
        }
        sp->handler = mprCreateWaitHandler(sp, sp->fd, MPR_SOCKET_READABLE, (MprWaitProc) acceptHandler,
            (void*) sp, sp->handlerPriority, (sp->flags & MPR_SOCKET_THREAD) ? MPR_WAIT_THREAD : 0);
    }
    sp->handlerMask |= MPR_SOCKET_READABLE;

#if BLD_WIN_LIKE
    /*
     *  Delay setting reuse until now so that we can be assured that we
     *  have exclusive use of the port.
     */
    if (!(sp->flags & MPR_SOCKET_NOREUSE)) {
        rc = 1;
        setsockopt(sp->fd, SOL_SOCKET, SO_REUSEADDR, (char*) &rc, sizeof(rc));
    }
#endif

    mprSetSocketBlockingMode(sp, (bool) (sp->flags & MPR_SOCKET_BLOCK));

    /*
     *  TCP/IP stacks have the No delay option (nagle algorithm) on by default.
     */
    if (sp->flags & MPR_SOCKET_NODELAY) {
        mprSetSocketNoDelay(sp, 1);
    }

    mprUnlock(sp->mutex);

    return sp->fd;
}


/*
 *  Open a client socket connection
 */
int mprOpenClientSocket(MprSocket *sp, cchar *host, int port, int flags)
{
    return sp->provider->connectSocket(sp, host, port, flags);
}


static int connectSocket(MprSocket *sp, cchar *host, int port, int initialFlags)
{
    struct sockaddr     *addr;
    socklen_t           addrlen;
    int                 broadcast, datagram, family, rc, err;

    mprLock(sp->mutex);

    resetSocket(sp);

    mprLog(sp, 6, "openClient: %s:%d, flags %x", host, port, initialFlags);

    sp->port = port;
    sp->flags = (initialFlags &
        (MPR_SOCKET_BROADCAST | MPR_SOCKET_DATAGRAM | MPR_SOCKET_BLOCK |
         MPR_SOCKET_LISTENER | MPR_SOCKET_NOREUSE | MPR_SOCKET_NODELAY | MPR_SOCKET_THREAD));
    sp->flags |= MPR_SOCKET_CLIENT;

    mprFree(sp->ipAddr);
    sp->ipAddr = mprStrdup(sp, host);

    broadcast = sp->flags & MPR_SOCKET_BROADCAST;
    if (broadcast) {
        sp->flags |= MPR_SOCKET_DATAGRAM;
    }
    datagram = sp->flags & MPR_SOCKET_DATAGRAM;

    if (getSocketAddress(sp, host, port, &family, &addr, &addrlen) < 0) {
        err = mprGetSocketError(sp);
        closesocket(sp->fd);
        sp->fd = -1;
        mprUnlock(sp->mutex);
        return -err;
    }
    /*
     *  Create the O/S socket
     */
    sp->fd = (int) socket(family, datagram ? SOCK_DGRAM: SOCK_STREAM, 0);
    if (sp->fd < 0) {
        err = mprGetSocketError(sp);
        mprUnlock(sp->mutex);
        return -err;
    }

#if !BLD_WIN_LIKE && !VXWORKS
    /*
     *  Children shoudl not inherit this fd
     */
    fcntl(sp->fd, F_SETFD, FD_CLOEXEC);
#endif

    if (broadcast) {
        int flag = 1;
        if (setsockopt(sp->fd, SOL_SOCKET, SO_BROADCAST, (char *) &flag, sizeof(flag)) < 0) {
            err = mprGetSocketError(sp);
            closesocket(sp->fd);
            sp->fd = -1;
            mprUnlock(sp->mutex);
            return -err;
        }
    }

    if (!datagram) {

        sp->flags |= MPR_SOCKET_CONNECTING;
        rc = connect(sp->fd, addr, addrlen);
        mprFree(addr);
        if (rc < 0) {
            err = mprGetSocketError(sp);
            closesocket(sp->fd);
            sp->fd = -1;
            mprUnlock(sp->mutex);
            return -err;
        }
    }

    mprSetSocketBlockingMode(sp, (bool) (sp->flags & MPR_SOCKET_BLOCK));

    /*
     *  TCP/IP stacks have the no delay option (nagle algorithm) on by default.
     */
    if (sp->flags & MPR_SOCKET_NODELAY) {
        mprSetSocketNoDelay(sp, 1);
    }

    mprUnlock(sp->mutex);

    return sp->fd;
}


/*
 *  Close a socket
 */
void mprCloseSocket(MprSocket *sp, bool gracefully)
{
    mprAssert(sp);
    mprAssert(sp->provider);

    sp->provider->closeSocket(sp, gracefully);
    mprAssert(sp->provider);
}


/*
 *  Standard (non-SSL) close
 */
static void closeSocket(MprSocket *sp, bool gracefully)
{
    MprSocketService    *ss;
    MprWaitService      *waitService;
    MprTime             timesUp;
    char                buf[16];
    int                 handlerFlags, shutdownFlag;

    mprAssert(sp->fd >= 0);

    waitService = mprGetMpr(sp)->waitService;

    mprLock(sp->mutex);

    mprAssert(!(sp->flags & MPR_SOCKET_CLOSED));
    if (sp->flags & MPR_SOCKET_CLOSED) {
        mprUnlock(sp->mutex);
        return;
    }

    sp->flags |= MPR_SOCKET_CLOSED;
    handlerFlags = (sp->handler) ? sp->handler->flags : 0;
    shutdownFlag = SHUT_RDWR;

    if (sp->handler) {
        mprFree(sp->handler);
        sp->handler = 0;
    }

    if (sp->fd >= 0) {
        /*
         *  Do a graceful shutdown. Read any outstanding read data to prevent resets. Then do a shutdown to
         *  send a FIN and read outstanding data. All non-blocking.
         */
        if (gracefully) {
            mprSetSocketBlockingMode(sp, 0);
            while (recv(sp->fd, buf, sizeof(buf), 0) > 0) {
                ;
            }
        }
        if (shutdown(sp->fd, shutdownFlag) == 0) {
            if (gracefully) {
                timesUp = mprGetTime(0) + MPR_TIMEOUT_LINGER;
                do {
                    if (recv(sp->fd, buf, sizeof(buf), 0) <= 0) {
                        break;
                    }
                } while (mprGetTime(0) < timesUp);
            }
        }

        closesocket(sp->fd);
        sp->fd = -1;
        mprAwakenWaitService(waitService);
    }

    if (! (sp->flags & MPR_SOCKET_LISTENER)) {
        ss = mprGetMpr(sp)->socketService;
        mprLock(ss->mutex);
        if (--ss->numClients < 0) {
            ss->numClients = 0;
        }
        mprUnlock(ss->mutex);
    }

    mprUnlock(sp->mutex);
}


/*
 *  Accept handler. May be called directly if single-threaded or on a pool thread.
 */
static void acceptHandler(void *data, int mask, bool isPoolThread)
{
    MprSocket   *sp;

    sp = (MprSocket*) data;

    sp->provider->acceptSocket(sp, 1);
}


/*
 *  Standard accept
 */
static MprSocket *acceptSocket(MprSocket *listen, bool invokeCallback)
{
    MprSocketService            *ss;
    MprSocket                   *nsp;
    struct sockaddr_storage     addrStorage;
    struct sockaddr             *addr;
    char                        host[NI_MAXHOST];
    socklen_t                   addrlen;
    int                         fd, port;

    if (listen->acceptCallback == 0) {
        return 0;
    }

    ss = mprGetMpr(listen)->socketService;

    addr = (struct sockaddr*) &addrStorage;
    addrlen = sizeof(addrStorage);

    fd = (int) accept(listen->fd, addr, &addrlen);
    if (fd < 0) {
        if (mprGetOsError() != EAGAIN) {
            mprLog(listen, 1, "socket: accept failed, errno %d", mprGetOsError());
        }
        return 0;
    }
    mprAssert(addrlen <= sizeof(addrStorage));

    /*
     *  Limit the number of simultaneous clients
     */
//  TODO - need lock here
    if (++ss->numClients >= ss->maxClients) {
        mprLog(listen, 2, "Rejecting connection, too many client connections (%d)", ss->numClients);
        closesocket(fd);
        return 0;
    }

    nsp = mprCreateSocket(ss, listen->ssl);
    if (nsp == 0) {
        closesocket(fd);
        return 0;
    }

    nsp->fd = fd;

#if !BLD_WIN_LIKE && !VXWORKS
    fcntl(fd, F_SETFD, FD_CLOEXEC);     /* Prevent children inheriting this socket */
#endif

#if UNUSED
    //  OPT XXX
    nsp->ipAddr = mprStrdup(nsp, listen->ipAddr);
    if (nsp->ipAddr == 0) {
        mprFree(nsp);
        return 0;
    }
#endif
    nsp->ipAddr = listen->ipAddr;

    nsp->acceptData = listen->acceptData;
    nsp->ioData = listen->ioData;
    nsp->ioData2 = listen->ioData2;
    nsp->port = listen->port;
    nsp->acceptCallback = listen->acceptCallback;
    nsp->flags = listen->flags;
    nsp->flags &= ~MPR_SOCKET_LISTENER;
    nsp->listenSock = listen;

    mprSetSocketBlockingMode(nsp, (nsp->flags & MPR_SOCKET_BLOCK) ? 1: 0);

    if (nsp->flags & MPR_SOCKET_NODELAY) {
        mprSetSocketNoDelay(nsp, 1);
    }

    if (getSocketName(nsp, addr, addrlen, host, sizeof(host), &port) != 0) {
        mprAssert(0);
        mprFree(nsp);
        return 0;
    }
    //  TODO - OPT
    nsp->clientIpAddr = mprStrdup(nsp, host);

    /*
     *  Call the user accept callback. We do not remember the socket handle, it is up to the callback to manage it
     *  from here on. The callback can delete the socket.
     */
    if (invokeCallback) {
        if (nsp->acceptCallback) {
            (nsp->acceptCallback)(nsp->acceptData, nsp, host, port);
        } else {
            mprFree(nsp);
            return 0;
        }
    }
    return nsp;
}


/*
 *  Read data. Return zero for EOF or no data if in non-blocking mode. Return -1 for errors. On success,
 *  return the number of bytes read. Use getEof to tell if we are EOF or just no data (in non-blocking mode).
 */
int mprReadSocket(MprSocket *sp, void *buf, int bufsize)
{
    mprAssert(sp);
    mprAssert(buf);
    mprAssert(bufsize > 0);
    mprAssert(sp->provider);

    return sp->provider->readSocket(sp, buf, bufsize);
}


/*
 *  Standard read from a socket (Non SSL)
 */
static int readSocket(MprSocket *sp, void *buf, int bufsize)
{
    struct sockaddr_storage server;
    socklen_t               len;
    int                     bytes, errCode;

    mprAssert(buf);
    mprAssert(bufsize > 0);
    mprAssert(~(sp->flags & MPR_SOCKET_CLOSED));

    mprLock(sp->mutex);

    if (sp->flags & MPR_SOCKET_EOF) {
        mprUnlock(sp->mutex);
        return 0;
    }

again:
    if (sp->flags & MPR_SOCKET_DATAGRAM) {
        len = sizeof(server);
        bytes = recvfrom(sp->fd, buf, bufsize, MSG_NOSIGNAL, (struct sockaddr*) &server, (socklen_t*) &len);
    } else {
        bytes = recv(sp->fd, buf, bufsize, MSG_NOSIGNAL);
    }

    if (bytes < 0) {
        errCode = mprGetSocketError(sp);
        if (errCode == EINTR) {
            goto again;

        } else if (errCode == EAGAIN || errCode == EWOULDBLOCK) {
            bytes = 0;                          /* No data available */

        } else if (errCode == ECONNRESET) {
            sp->flags |= MPR_SOCKET_EOF;        /* Disorderly disconnect */
            bytes = 0;

        } else {
            sp->flags |= MPR_SOCKET_EOF;        /* Some other error */
            bytes = -errCode;
        }

    } else if (bytes == 0) {                    /* EOF */
        sp->flags |= MPR_SOCKET_EOF;
    }


#if TODO && FOR_SSL
    /*
     *  If there is more buffered data to read, then ensure the handler recalls us again even if there is no
     *  more IO events
     */
    if (isBufferedData()) {
        if (sp->handler) {
            mprRecallWaitHandler(sp->handler);
        }
    }
#endif

    mprUnlock(sp->mutex);
    return bytes;
}


/*
 *  Write data. Return the number of bytes written or -1 on errors. NOTE: this routine will return with a
 *  short write if the underlying socket can't accept any more data.
 */
int mprWriteSocket(MprSocket *sp, void *buf, int bufsize)
{
    mprAssert(sp);
    mprAssert(buf);
    mprAssert(bufsize > 0);
    mprAssert(sp->provider);

    return sp->provider->writeSocket(sp, buf, bufsize);
}


/*
 *  Standard write to a socket (Non SSL)
 */
static int writeSocket(MprSocket *sp, void *buf, int bufsize)
{
    struct sockaddr     *addr;
    socklen_t           addrlen;
    int                 family, sofar, errCode, len, written;

    mprAssert(buf);
    mprAssert(bufsize >= 0);
    mprAssert((sp->flags & MPR_SOCKET_CLOSED) == 0);

    mprLock(sp->mutex);

    if (sp->flags & (MPR_SOCKET_BROADCAST | MPR_SOCKET_DATAGRAM)) {
        if (getSocketAddress(sp, sp->ipAddr, sp->port, &family, &addr, &addrlen) < 0) {
            return MPR_ERR_NOT_FOUND;
        }
    }

    if (sp->flags & MPR_SOCKET_EOF) {
        sofar = bufsize;

    } else {
        errCode = 0;
        len = bufsize;
        sofar = 0;
        while (len > 0) {
            if ((sp->flags & MPR_SOCKET_BROADCAST) || (sp->flags & MPR_SOCKET_DATAGRAM)) {
                written = sendto(sp->fd, &((char*) buf)[sofar], len, MSG_NOSIGNAL, addr, addrlen);

            } else {
                written = send(sp->fd, &((char*) buf)[sofar], len, MSG_NOSIGNAL);
            }

            if (written < 0) {
                errCode = mprGetSocketError(sp);
                if (errCode == EINTR) {
                    continue;
                } else if (errCode == EAGAIN || errCode == EWOULDBLOCK) {
                    mprUnlock(sp->mutex);
                    return sofar;
                }
                mprUnlock(sp->mutex);
                return -errCode;
            }
            len -= written;
            sofar += written;
        }
    }
    mprUnlock(sp->mutex);
    return sofar;
}


/*
 *  Write a string to the socket
 */
int mprWriteSocketString(MprSocket *sp, cchar *str)
{
    return mprWriteSocket(sp, (void*) str, (int) strlen(str));
}


int mprWriteSocketVector(MprSocket *sp, MprIOVec *iovec, int count)
{
    char    *start;
    int     total, len, i, written;

#if BLD_UNIX_LIKE
    if (sp->ssl == 0) {
        return writev(sp->fd, (const struct iovec*) iovec, count);
    } else
#endif
    {
        if (count <= 0) {
            return 0;
        }

        start = iovec[0].start;
        len = (int) iovec[0].len;
        mprAssert(len > 0);

        total = i = 0;

        while (i < count) {
            written = mprWriteSocket(sp, start, len);
            if (written < 0) {
                return written;

            } else if (written == 0) {
                break;

            } else {
                len -= written;
                start += written;
                total += written;
                if (len <= 0) {
                    i++;
                    start = iovec[i].start;
                    len = (int) iovec[i].len;
                }
            }
        }
        return total;
    }
}


#if !LINUX
static MprOffset localSendfile(MprSocket *sp, MprFile *file, MprOffset off, int len)
{
    char    buf[MPR_BUFSIZE];

    len = min(len, sizeof(buf));
    if (mprRead(file, buf, len) != len) {
        mprAssert(0);
        return MPR_ERR_CANT_READ;
    }
    return mprWriteSocket(sp, buf, len);
}
#endif


/*
 *  Write data from a file to a socket. Includes the ability to write header before and after the file data.
 *  Works even with a null "file" to just output the headers.
 */
MprOffset mprSendFileToSocket(MprFile *file, MprSocket *sock, MprOffset offset, int bytes, MprIOVec *beforeVec, 
    int beforeCount, MprIOVec *afterVec, int afterCount)
{
#if MACOSX
    struct sf_hdtr  def;
#endif
    off_t           written, off;
    int             rc, i, done, toWriteBefore, toWriteAfter, toWriteFile;

    rc = 0;

#if MACOSX
    written = bytes;
    def.hdr_cnt = beforeCount;
    def.headers = (beforeCount > 0) ? (struct iovec*) beforeVec: 0;
    def.trl_cnt = afterCount;
    def.trailers = (afterCount > 0) ? (struct iovec*) afterVec: 0;

    if (file && file->fd >= 0) {
        rc = sendfile(file->fd, sock->fd, offset, &written, &def, 0);
    } else
#else
    if (1) 
#endif
    {
        /*
         *  Either !MACOSX or no file is opened
         */
        done = written = 0;
        for (i = toWriteBefore = 0; i < beforeCount; i++) {
            toWriteBefore += (int) beforeVec[i].len;
        }
        for (i = toWriteAfter = 0; i < afterCount; i++) {
            toWriteAfter += (int) afterVec[i].len;
        }
        toWriteFile = bytes - toWriteBefore - toWriteAfter;
        mprAssert(toWriteFile >= 0);

        /*
         *  Linux sendfile does not have the integrated ability to send headers. Must do it separately here.
         *  I/O requests may return short (write fewer than requested bytes).
         */
        if (beforeCount > 0) {
            rc = mprWriteSocketVector(sock, beforeVec, beforeCount);
            if (rc > 0) {
                written += rc;
            }
            if (rc != toWriteBefore) {
                done++;
            }
        }

        if (!done && toWriteFile > 0) {
            off = offset;
#if LINUX && !__UCLIBC__
            rc = sendfile(sock->fd, file->fd, &off, toWriteFile);
#else
            rc = localSendfile(sock, file, offset, toWriteFile);
#endif
            if (rc > 0) {
                written += rc;
                if (rc != toWriteFile) {
                    done++;
                }
            }
        }
        if (!done && afterCount > 0) {
            rc = mprWriteSocketVector(sock, afterVec, afterCount);
            if (rc > 0) {
                written += rc;
            }
        }
    }

    if (rc < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return written;
        }
        return -1;
    }
    return written;
}


static int flushSocket(MprSocket *sp)
{
    return 0;
}


int mprFlushSocket(MprSocket *sp)
{
    return sp->provider->flushSocket(sp);
}


/*
 *  Return true if end of file
 */
bool mprGetSocketEof(MprSocket *sp)
{
    return ((sp->flags & MPR_SOCKET_EOF) != 0);
}


/*
 *  Set the EOF condition
 */
void mprSetSocketEof(MprSocket *sp, bool eof)
{
    if (eof) {
        sp->flags |= MPR_SOCKET_EOF;
    } else {
        sp->flags &= ~MPR_SOCKET_EOF;
    }
}


/*
 *  Define an IO callback for this socket. The callback called whenever there is an event of interest as
 *  defined by handlerMask (MPR_SOCKET_READABLE, ...)
 */
void mprSetSocketCallback(MprSocket *sp, MprSocketProc fn, void *data, void *data2, int mask, int pri)
{
    mprLock(sp->mutex);

    sp->ioCallback = fn;
    sp->ioData = data;
    sp->ioData2 = data2;
    sp->handlerPriority = pri;
    sp->handlerMask = mask;

    sp->handler = mprCreateWaitHandler(sp, sp->fd, sp->handlerMask, (MprWaitProc) ioProc, (void*) sp, sp->handlerPriority, 
        (sp->flags & MPR_SOCKET_THREAD) ? MPR_WAIT_THREAD : 0);

#if UNUSED
    mprSetSocketEventMask(sp, sp->handlerMask);
#endif

    mprUnlock(sp->mutex);
}


/*
 *  Return the O/S socket file handle
 */
int mprGetSocketFd(MprSocket *sp)
{
    return sp->fd;
}


/*
 *  Return the blocking mode of the socket
 */
bool mprGetSocketBlockingMode(MprSocket *sp)
{
    return sp->flags & MPR_SOCKET_BLOCK;
}


/*
 *  Get the socket flags
 */
int mprGetSocketFlags(MprSocket *sp)
{
    return sp->flags;
}


/*
 *  Set whether the socket blocks or not on read/write
 */
int mprSetSocketBlockingMode(MprSocket *sp, bool on)
{
    int     flag, oldMode;

    mprLock(sp->mutex);

    oldMode = sp->flags & MPR_SOCKET_BLOCK;

    sp->flags &= ~(MPR_SOCKET_BLOCK);
    if (on) {
        sp->flags |= MPR_SOCKET_BLOCK;
    }

    flag = (sp->flags & MPR_SOCKET_BLOCK) ? 0 : 1;

#if BLD_WIN_LIKE
    ioctlsocket(sp->fd, FIONBIO, (ulong*) &flag);
#elif VXWORKS
    ioctl(sp->fd, FIONBIO, (int) &flag);
#else
    flag = 0;
    if (on) {
        fcntl(sp->fd, F_SETFL, fcntl(sp->fd, F_GETFL) & ~O_NONBLOCK);
    } else {
        fcntl(sp->fd, F_SETFL, fcntl(sp->fd, F_GETFL) | O_NONBLOCK);
    }
#endif

    mprUnlock(sp->mutex);

    return oldMode;
}


/*
 *  Set the TCP delay behavior (nagle algorithm)
 */
int mprSetSocketNoDelay(MprSocket *sp, bool on)
{
    int     oldDelay;

    mprLock(sp->mutex);

    oldDelay = sp->flags & MPR_SOCKET_NODELAY;
    if (on) {
        sp->flags |= MPR_SOCKET_NODELAY;
    } else {
        sp->flags &= ~(MPR_SOCKET_NODELAY);
    }
#if BLD_WIN_LIKE
    {
        BOOL    noDelay;
        noDelay = on ? 1 : 0;
        setsockopt(sp->fd, IPPROTO_TCP, TCP_NODELAY, (FAR char*) &noDelay, sizeof(BOOL));
    }
#else
    {
        int     noDelay;
        noDelay = on ? 1 : 0;
        setsockopt(sp->fd, IPPROTO_TCP, TCP_NODELAY, (char*) &noDelay, sizeof(int));
    }
#endif /* BLD_WIN_LIKE */

    mprUnlock(sp->mutex);

    return oldDelay;
}


/*
 *  Get the port number
 */
int mprGetSocketPort(MprSocket *sp)
{
    return sp->port;
}


/*
 *  IO ready handler. May be called directly if single-threaded or on a pool thread.
 */
static void ioProc(MprSocket *sp, int mask, bool isPoolThread)
{
    mprLock(sp->mutex);
    if (sp->ioCallback == 0 || (sp->handlerMask & mask) == 0) {
        mprAssert(sp->handlerMask & mask);
        if ((sp->handlerMask & mask) == 0) {
            mprLog(sp, 0, "ioProc: callback %x, handlerMask %x mask %x", sp->ioCallback, sp->handlerMask, mask);
        }
        mprUnlock(sp->mutex);
        return;
    }
    mask &= sp->handlerMask;
    mprUnlock(sp->mutex);

    /*
     *  The callback can delete the socket
     */
    (sp->ioCallback)(sp->ioData, sp, mask, isPoolThread);
}


/*
 *  Define the events of interest. Must only be called with a locked socket.
 */
void mprSetSocketEventMask(MprSocket *sp, int handlerMask)
{
    sp->handlerMask = handlerMask;

    if (handlerMask) {
        if (sp->handler) {
            mprSetWaitInterest(sp->handler, handlerMask);

        } else {
            sp->handler = mprCreateWaitHandler(sp, sp->fd, handlerMask, (MprWaitProc) ioProc, (void*) sp, 
                sp->handlerPriority, (sp->flags & MPR_SOCKET_THREAD) ? MPR_WAIT_THREAD : 0);
        }

    } else if (sp->handler) {
        mprSetWaitInterest(sp->handler, handlerMask);
    }
}


/*
 *  Map the O/S error code to portable error codes.
 */
int mprGetSocketError(MprSocket *sp)
{
#if BLD_WIN_LIKE
    int     rc;
    switch (rc = WSAGetLastError()) {
    case WSAEINTR:
        return EINTR;

    case WSAENETDOWN:
        return ENETDOWN;

    case WSAEWOULDBLOCK:
        return EWOULDBLOCK;

    case WSAEPROCLIM:
        return EAGAIN;

    case WSAECONNRESET:
    case WSAECONNABORTED:
        return ECONNRESET;

    case WSAECONNREFUSED:
        return ECONNREFUSED;

    case WSAEADDRINUSE:
        return EADDRINUSE;
    default:
        return EINVAL;
    }
#else
    return errno;
#endif
}


#if LINUX || WIN || MACOSX || SOLARIS
/*
 *  Get a socket address from a host/port combination. If a host provides both IPv4 and IPv6 addresses, 
 *  prefer the IPv4 address.
 */
static int getSocketAddress(MprSocket *sp, cchar *host, int port, int *family, struct sockaddr **addr, socklen_t *addrlen)
{
    struct addrinfo     hints, *res;
    char                portBuf[MPR_MAX_IP_PORT];
    int                 rc;

    mprAssert(sp);
    mprAssert(host);
    mprAssert(addr);

    memset((char*) &hints, '\0', sizeof(hints));

    /*
     *  Note that IPv6 does not support broadcast, there is no 255.255.255.255 equivalent.
     *  Multicast can be used over a specific link, but the user must provide that address plus %scope_id.
     */
    if (host == 0 || strcmp(host, "") == 0) {
        host = 0;
        hints.ai_flags |= AI_PASSIVE;           /* Bind to 0.0.0.0 and :: */
    }
    hints.ai_socktype = SOCK_STREAM;

    mprItoa(portBuf, sizeof(portBuf), port, 10);

    hints.ai_family = AF_INET;
    res = 0;

    /*
     *  Try to sleuth the address to avoid duplicate address lookups. Then try IPv4 first then IPv6.
     */
    rc = -1;
    if (host == NULL || strchr(host, ':') == 0) {
        /* Looks like IPv4 */
        rc = getaddrinfo(host, portBuf, &hints, &res);
    }
    if (rc != 0) {
        hints.ai_family = AF_INET6;
        rc = getaddrinfo(host, portBuf, &hints, &res);
        if (rc != 0) {
            return MPR_ERR_CANT_OPEN;
        }
    }

    *addr = (struct sockaddr*) mprAllocObjZeroed(sp, struct sockaddr_storage);
    mprMemcpy((char*) *addr, sizeof(struct sockaddr_storage), (char*) res->ai_addr, (int) res->ai_addrlen);

    *addrlen = (int) res->ai_addrlen;
    *family = res->ai_family;

    freeaddrinfo(res);
    return 0;
}


#elif MACOSX && 0
/*
 *  Mac OS X has getaddrinfo, but it is not thread-safe. Moreover, it crashes when any second threads calls it!
 */
static int getSocketAddress(MprSocket *sp, cchar *host, int port, int *family, struct sockaddr **addr, socklen_t *addrlen)
{
    struct hostent      *hostent;
    struct sockaddr_in  *sa;
    struct sockaddr_in6 *sa6;
    int                 len, err;

    mprAssert(addr);

    mprLock(sp->service->mutex);
    len = sizeof(struct sockaddr_in);
    if ((hostent = getipnodebyname(host, AF_INET, 0, &err)) == NULL) {
        len = sizeof(struct sockaddr_in6);
        if ((hostent = getipnodebyname(host, AF_INET6, 0, &err)) == NULL) {
            return MPR_ERR_CANT_OPEN;
        }
        sa6 = (struct sockaddr_in6*) mprAllocZeroed(sp, len);
        if (sa6 == 0) {
            return MPR_ERR_NO_MEMORY;
        }
        memcpy((char*) &sa6->sin6_addr, (char*) hostent->h_addr_list[0], (size_t) hostent->h_length);
        sa6->sin6_family = hostent->h_addrtype;
        sa6->sin6_port = htons((short) (port & 0xFFFF));
        *addr = (struct sockaddr*) sa6;

    } else {
        sa = (struct sockaddr_in*) mprAllocZeroed(sp, len);
        if (sa == 0) {
            return MPR_ERR_NO_MEMORY;
        }
        memcpy((char*) &sa->sin_addr, (char*) hostent->h_addr_list[0], (size_t) hostent->h_length);
        sa->sin_family = hostent->h_addrtype;
        sa->sin_port = htons((short) (port & 0xFFFF));
        *addr = (struct sockaddr*) sa;
    }
    mprUnlock(sp->service->mutex);

    *addrlen = len;
    *family = hostent->h_addrtype;

    freehostent(hostent);
    return 0;
}


#else

static int getSocketAddress(MprSocket *sp, cchar *host, int port, int *family, struct sockaddr **addr, socklen_t *addrlen)
{
    struct sockaddr_in  *sa;

    sa = mprAllocObjZeroed(sp, struct sockaddr_in);
    if (sa == 0) {
        return MPR_ERR_NO_MEMORY;
    }

    memset((char*) sa, '\0', sizeof(struct sockaddr_in));
    sa->sin_family = AF_INET;
    sa->sin_port = htons((short) (port & 0xFFFF));

    if (strcmp(sp->ipAddr, "") != 0) {
        sa->sin_addr.s_addr = inet_addr(sp->ipAddr);

    } else {
        if ((sp->flags & MPR_SOCKET_BROADCAST) || (sp->flags & MPR_SOCKET_DATAGRAM)) {
            sa->sin_addr.s_addr = INADDR_BROADCAST;
        } else {
            sa->sin_addr.s_addr = INADDR_ANY;
        }
    }

    /*
     *  gethostbyname is not thread safe
     */
    mprLock(sp->service->mutex);
    if (sa->sin_addr.s_addr == INADDR_NONE) {
#if VXWORKS
        /*
         *  VxWorks only supports one interface and this code only supports IPv4
         */
        sa->sin_addr.s_addr = (ulong) hostGetByName((char*) host);
        if (sa->sin_addr.s_addr < 0) {
            mprUnlock(sp->service->mutex);
            mprAssert(0);
            return 0;
        }
#else
        struct hostent      *hostent;
        hostent = gethostbyname2(host, AF_INET);
        if (hostent == 0) {
            hostent = gethostbyname2(host, AF_INET6);
            if (hostent == 0) {
                mprUnlock(sp->service->mutex);
                return MPR_ERR_NOT_FOUND;
            }
        }
        memcpy((char*) &sa->sin_addr, (char*) hostent->h_addr_list[0], (size_t) hostent->h_length);
#endif
    }
    mprUnlock(sp->service->mutex);

    *addr = (struct sockaddr*) sa;
    *addrlen = sizeof(struct sockaddr_in);
    *family = sa->sin_family;

    return 0;
}
#endif


static int getSocketName(MprSocket *sp, struct sockaddr *addr, int addrlen, char *host, int hostLen, int *port)
{
#if BLD_UNIX_LIKE || WIN
    char    service[NI_MAXSERV];

    if (getnameinfo(addr, addrlen, host, hostLen, service, sizeof(service), NI_NUMERICHOST | NI_NUMERICSERV | NI_NOFQDN)) {
        return MPR_ERR_BAD_VALUE;
    }
    *port = atoi(service);

#else
    struct sockaddr_in  *sa;

#if HAVE_NTOA_R
    sa = (struct sockaddr_in*) addr;
    inet_ntoa_r(sa->sin_addr, host, hostLen);
#else
    uchar   *cp;
    sa = (struct sockaddr_in*) addr;
    cp = (uchar*) &sa->sin_addr;
    mprSprintf(host, hostLen, "%d.%d.%d.%d", cp[0], cp[1], cp[2], cp[3]);
#endif
    *port = ntohs(sa->sin_port);
#endif
    return 0;
}


/*
 *  Parse ipAddrPort and return the IP address and port components. Handles ipv4 and ipv6 addresses. When an ipAddrPort
 *  contains an ipv6 port it should be written as
 *
 *      aaaa:bbbb:cccc:dddd:eeee:ffff:gggg:hhhh:iiii
 *  or
 *      [aaaa:bbbb:cccc:dddd:eeee:ffff:gggg:hhhh:iiii]:port
 */
int mprParseIp(MprCtx ctx, cchar *ipAddrPort, char **ipAddrRef, int *port, int defaultPort)
{
    char    *ipAddr;
    char    *cp;
    int     colonCount;

    ipAddr = NULL;
    if (defaultPort < 0) {
        defaultPort = 80;
    }

    /*
     * First check if ipv6 or ipv4 address by looking for > 1 colons.
     */
    colonCount = 0;
    for (cp = (char*) ipAddrPort; ((*cp != '\0') && (colonCount < 2)) ; cp++) {
        if (*cp == ':') {
            colonCount++;
        }
    }

    if (colonCount > 1) {
        /*
         *  IPv6. If port is present, it will follow a closing bracket ']'
         */
        if ((cp = strchr(ipAddrPort, ']')) != 0) {
            cp++;
            if ((*cp) && (*cp == ':')) {
                *port = (*++cp == '*') ? -1 : atoi(cp);

                /* set ipAddr to ipv6 address without brackets */
                ipAddr = mprStrdup(ctx, ipAddrPort+1);
                cp = strchr(ipAddr, ']');
                *cp = '\0';

            } else {
                /* Handles [a:b:c:d:e:f:g:h:i] case (no port)- should not occur */
                ipAddr = mprStrdup(ctx, ipAddrPort+1);
                cp = strchr(ipAddr, ']');
                *cp = '\0';

                /* No port present, use callers default */
                *port = defaultPort;
            }
        } else {
            /* Handles a:b:c:d:e:f:g:h:i case (no port) */
            ipAddr = mprStrdup(ctx, ipAddrPort);

            /* No port present, use callers default */
            *port = defaultPort;
        }

    } else {
        /* 
         *  ipv4 
         */
        ipAddr = mprStrdup(ctx, ipAddrPort);

        if ((cp = strchr(ipAddr, ':')) != 0) {
            *cp++ = '\0';
            if (*cp == '*') {
                *port = -1;
            } else {
                *port = atoi(cp);
            }
            if (*ipAddr == '*') {
                mprFree(ipAddr);
                ipAddr = mprStrdup(ctx, "127.0.0.1");
            }

        } else {
            if (isdigit((int) *ipAddr)) {
                *port = atoi(ipAddr);
                mprFree(ipAddr);
                ipAddr = mprStrdup(ctx, "127.0.0.1");

            } else {
                /* No port present, use callers default */
                *port = defaultPort;
            }
        }
    }

    if (ipAddrRef) {
        *ipAddrRef = ipAddr;
    }

    return 0;
}


#if UNUSED
/*
 *  Reentrant replacement for gethostbyname for when getaddrinfo is not available.
 */
struct hostent *getHostByName(MprCtx ctx, cchar *name)
{
    Mpr             *mpr;
    struct hostent  *hp;
    struct hostent  *ip;
    int             count, i;

    hp = (struct hostent*) mprAllocZeroed(ctx, sizeof(struct hostent));

    mpr = mprGetMpr(ctx);

    #undef gethostbyname

    mprGlobalLock(mpr);

#if VXWORKS
{
    /*
     *  Only supports one interface
     */
    struct in_addr inaddr;

    inaddr.s_addr = (ulong) hostGetByName((char*) name);
    if (inaddr.s_addr < 0) {
        mprGlobalUnlock(mpr);
        mprAssert(0);
        return 0;
    }
    hp->h_addrtype = AF_INET;
    hp->h_length = sizeof(int);
    hp->h_name = mprStrdup(hp, name);

    hp->h_addr_list = mprAlloc(hp, 2 * sizeof(char*));
    hp->h_addr_list[0] = (char *) mprAllocZeroed(hp, sizeof(struct in_addr));
    memcpy(&hp->h_addr_list[0], &inaddr, hp->h_length);
    hp->h_addr_list[1] = 0;
}
#else
    ip = gethostbyname2(name, AF_INET);
    if (ip == 0) {
        ip = gethostbyname2(name, AF_INET6);
        if (ip == 0) {
            mprGlobalUnlock(mpr);
            return 0;
        }
    }
    hp->h_addrtype = ip->h_addrtype;
    hp->h_length = ip->h_length;
    hp->h_name = mprStrdup(hp, ip->h_name);
    hp->h_addr_list = 0;
    hp->h_aliases = 0;

    for (count = 0; ip->h_addr_list[count] != 0; ) {
        count++;
    }
    if (count > 0) {
        count++;
        hp->h_addr_list = (char**) mprAllocZeroed(hp, count * sizeof(char*));
        for (i = 0; ip->h_addr_list[i] != 0; i++) {
            memcpy(&hp->h_addr_list[i], &ip->h_addr_list[i], ip->h_length);
        }
        hp->h_addr_list[i] = 0;
    }

    for (count = 0; ip->h_aliases[count] != 0; ) {
        count++;
    }
    if (count > 0) {
        count++;
        hp->h_aliases = (char**) mprAllocZeroed(hp, count * sizeof(char*));
        for (i = 0; ip->h_aliases[i] != 0; i++) {
            hp->h_aliases[i] = mprStrdup(hp, ip->h_aliases[i]);
        }
        hp->h_aliases[i] = 0;
    }
#endif
    mprGlobalUnlock(mpr);

    return hp;
}
#endif


bool mprSocketIsSecure(MprSocket *sp)
{
    return sp->sslSocket != 0;
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
/************************************************************************/
/*
 *  End of file "../mprSocket.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../mprString.c"
 */
/************************************************************************/

/**
 *  mprString.c - String routines safe for embedded programming
 *
 *  This module provides safe replacements for the standard string library. 
 *
 *  Most routines in this file are not thread-safe. It is the callers responsibility to perform all thread 
 *  synchronization.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */




//  TODO - need a routine that supplies a lenght of bytes to copy out of str. Like:
//      int mprMemcpy(void *dest, int destMax, const void *src, int nbytes)   but adding a null.

int mprStrcpy(char *dest, int destMax, cchar *src)
{
    int     len;

    mprAssert(dest);
    mprAssert(destMax >= 0);
    mprAssert(src);
    mprAssert(src != dest);
    mprAssert(src < dest || src > &dest[destMax]);

    len = (int) strlen(src);
    if (destMax > 0 && len >= destMax && len > 0) {
        return MPR_ERR_WONT_FIT;
    }
    if (len > 0) {
        memcpy(dest, src, len);
        dest[len] = '\0';
    } else {
        *dest = '\0';
        len = 0;
    } 
    return len;
}


int mprAllocStrcpy(MprCtx ctx, char **dest, int destMax, cchar *src)
{
    int     len;

    mprAssert(dest);
    mprAssert(destMax >= 0);
    mprAssert(src);

    len = (int) strlen(src);
    if (destMax > 0 && len >= destMax) {
        mprAssert(0);
        return MPR_ERR_WONT_FIT;
    }
    if (len > 0) {
        *dest = (char*) mprAlloc(ctx, len);
        memcpy(*dest, src, len);
        (*dest)[len] = '\0';
    } else {
        *dest = (char*) mprAlloc(ctx, 1);
        *dest = '\0';
        len = 0;
    } 
    return len;
}


#if !BREW   /* TODO -- temp */

int mprMemcmp(const void *s1, int s1Len, const void *s2, int s2Len)
{
    int     len, rc;

    mprAssert(s1);
    mprAssert(s2);
    mprAssert(s1Len >= 0);
    mprAssert(s2Len >= 0);

    len = min(s1Len, s2Len);

    rc = memcmp(s1, s2, len);
    if (rc == 0) {
        if (s1Len < s2Len) {
            return -1;
        } else if (s1Len > s2Len) {
            return 1;
        }
    }
    return rc;
}

#endif


/*
 *  Supports insitu copy where src and destination overlap
 */

int mprMemcpy(void *dest, int destMax, const void *src, int nbytes)
{
    mprAssert(dest);
    mprAssert(destMax <= 0 || destMax >= nbytes);
    mprAssert(src);
    mprAssert(nbytes >= 0);

    if (destMax > 0 && nbytes > destMax) {
        mprAssert(0);
        return MPR_ERR_WONT_FIT;
    }
    if (nbytes > 0) {
        memmove(dest, src, nbytes);
        return nbytes;
    } else {
        return 0;
    }
}


int mprAllocMemcpy(MprCtx ctx, char **dest, int destMax, const void *src, int nbytes)
{
    mprAssert(dest);
    mprAssert(src);
    mprAssert(nbytes > 0);
    mprAssert(destMax <= 0 || destMax >= nbytes);

    if (destMax > 0 && nbytes > destMax) {
        mprAssert(0);
        return MPR_ERR_WONT_FIT;
    }
    if (nbytes > 0) {
        *dest = (char*) mprAlloc(ctx, nbytes);
        if (*dest == 0) {
            return MPR_ERR_NO_MEMORY;
        }
        memcpy(*dest, src, nbytes);
    } else {
        *dest = (char*) mprAlloc(ctx, 1);
    }
    return nbytes;
}


int mprCoreStrcat(MprCtx ctx, char **destp, int destMax, int existingLen, cchar *delim, cchar *src, va_list args)
{
    va_list     ap;
    char        *dest, *str, *dp;
    int         sepLen, addBytes, required;

    mprAssert(destp);
    mprAssert(src);

    //  TODO - should be <= 0
    if (destMax < 0) {
        destMax = INT_MAX;
    }

    dest = *destp;
    sepLen = (delim) ? (int) strlen(delim) : 0;

#ifdef __va_copy
    __va_copy(ap, args);
#else
    ap = args;
#endif
    addBytes = 0;
    if (existingLen < 0) {
        existingLen = (dest) ? (int) strlen(dest) : 0;
    } 
    if (existingLen > 0) {
        addBytes += sepLen;
    }
    str = (char*) src;

    while (str) {
        addBytes += (int) strlen(str);
        str = va_arg(ap, char*);
        if (str) {
            addBytes += sepLen;
        }
    }

    required = existingLen + addBytes + 1;
    if (destMax > 0 && required >= destMax) {
        return MPR_ERR_WONT_FIT;
    }

    if (ctx != 0) {
        if (dest == 0) {
            dest = (char*) mprAlloc(ctx, required);
        } else {
            dest = (char*) mprRealloc(ctx, dest, required);
        }
    } else {
        dest = (char*) *destp;
    }

    if (dest == 0) {
        return MPR_ERR_NO_MEMORY;
    }

    dp = &dest[existingLen];
    if (delim && existingLen > 0) {
        strcpy(dp, delim);
        dp += sepLen;
    }

    if (addBytes > 0) {
#ifdef __va_copy
        __va_copy(ap, args);
#else
        ap = args;
#endif
        str = (char*) src;
        while (str) {
            strcpy(dp, str);
            dp += (int) strlen(str);
            str = va_arg(ap, char*);
            if (delim && str) {
                strcpy(dp, delim);
                dp += sepLen;
            }
        }
    } else if (dest == 0) {
        dest = (char*) mprAlloc(ctx, 1);
    } 
    *dp = '\0';

    *destp = dest;
    mprAssert(dp < &dest[required]);

    return required - 1;
}


int mprStrcat(char *dest, int destMax, cchar *delim, cchar *src, ...)
{
    va_list     ap;
    int         rc;

    mprAssert(dest);
    mprAssert(src);

    va_start(ap, src);
    rc = mprCoreStrcat(0, &dest, destMax, 0, delim, src, ap);
    va_end(ap);
    return rc;
}


int mprAllocStrcat(MprCtx ctx, char **destp, int destMax, cchar *delim, cchar *src, ...)
{
    va_list     ap;
    int         rc;

    mprAssert(destp);
    mprAssert(src);

    *destp = 0;
    va_start(ap, src);
    rc = mprCoreStrcat(ctx, destp, destMax, 0, delim, src, ap);
    va_end(ap);
    return rc;
}


int mprReallocStrcat(MprCtx ctx, char **destp, int destMax, int existingLen, cchar *delim, cchar *src, ...)
{
    va_list     ap;
    int         rc;

    va_start(ap, src);
    rc = mprCoreStrcat(ctx, destp, destMax, existingLen, delim, src, ap);
    va_end(ap);
    return rc;
}


int mprStrlen(cchar *src, int max)
{
    int     len;

    len = (int) strlen(src);
    if (len >= max) {
        mprAssert(0);
        return MPR_ERR_WONT_FIT;
    }
    return len;
}


//  TODO - would be good to have a trim from only the end
char *mprStrTrim(char *str, cchar *set)
{
    int     len, i;

    if (str == 0 || set == 0) {
        return str;
    }

    i = (int) strspn(str, set);
    str += i;

    len = (int) strlen(str);
    while (len > 0 && strspn(&str[len - 1], set) > 0) {
        str[len - 1] = '\0';
        len--;
    }
    return str;
}


/*  
 *  Map a string to lower case (overwrites original string)
 */
char *mprStrLower(char *str)
{
    char    *cp;

    mprAssert(str);

    if (str == 0) {
        return 0;
    }

    for (cp = str; *cp; cp++) {
        if (isupper((int) *cp)) {
            *cp = (char) tolower((int) *cp);
        }
    }
    return str;
}


/*  
 *  Map a string to upper case (overwrites buffer)
 */
char *mprStrUpper(char *str)
{
    char    *cp;

    mprAssert(str);
    if (str == 0) {
        return 0;
    }

    for (cp = str; *cp; cp++) {
        if (islower((int) *cp)) {
            *cp = (char) toupper((int) *cp);
        }
    }
    return str;
}


/*
 *  Case sensitive string comparison.
 */
int mprStrcmp(cchar *str1, cchar *str2)
{
    int     rc;

    if (str1 == 0) {
        return -1;
    }
    if (str2 == 0) {
        return 1;
    }
    if (str1 == str2) {
        return 0;
    }

    for (rc = 0; *str1 && rc == 0; str1++, str2++) {
        rc = *str1 - *str2;
    }
    if (*str2) {
        return -1;
    }
    return rc;
}


/*
 *  Case insensitive string comparison. Stop at the end of str1.
 */
int mprStrcmpAnyCase(cchar *str1, cchar *str2)
{
    int     rc;

    if (str1 == 0) {
        return -1;
    }
    if (str2 == 0) {
        return 1;
    }
    if (str1 == str2) {
        return 0;
    }

    for (rc = 0; *str1 && *str2 && rc == 0; str1++, str2++) {
        rc = tolower((int) *str1) - tolower((int) *str2);
    }
    
    if (rc) {
        return rc;
    } else if (*str1 == '\0' && *str2 == '\0') {
        return 0;
    } else if (*str1 == '\0') {
        return -1;
    } else if (*str2 == '\0') {
        return 1;
    }

    return 0;
}


/*
 *  Case insensitive string comparison. Limited by length
 */
int mprStrcmpAnyCaseCount(cchar *str1, cchar *str2, int len)
{
    int     rc;

    if (str1 == 0 || str2 == 0) {
        return -1;
    }
    if (str1 == str2) {
        return 0;
    }

    for (rc = 0; len-- > 0 && *str1 && rc == 0; str1++, str2++) {
        rc = tolower((int) *str1) - tolower((int) *str2);
    }
    return rc;
}


/*
 *  Thread-safe wrapping of strtok. Note "str" is modifed as per strtok()
 */
char *mprStrTok(char *str, cchar *delim, char **last)
{
    char    *start, *end;
    int     i;

    start = str ? str : *last;

    if (start == 0) {
        *last = 0;
        return 0;
    }
    
    i = (int) strspn(start, delim);
    start += i;
    if (*start == '\0') {
        *last = 0;
        return 0;
    }
    end = strpbrk(start, delim);
    if (end) {
        *end++ = '\0';
        i = (int) strspn(end, delim);
        end += i;
    }
    *last = end;
    return start;
}


/*
 *  Split the buffer into word tokens
 */
char *mprGetWordTok(char *buf, int bufsize, cchar *str, cchar *delim, cchar **tok)
{
    cchar       *start, *end;
    int         i, len;

    start = str ? str : *tok;

    if (start == 0) {
        return 0;
    }
    
    i = (int) strspn(start, delim);
    start += i;
    if (*start =='\0') {
        *tok = 0;
        return 0;
    }
    end = strpbrk(start, delim);
    if (end) {
        len = min((int) (end - start), bufsize - 1);
        mprMemcpy(buf, bufsize, start, len);
        buf[len] = '\0';
    } else {
        if (mprStrcpy(buf, bufsize, start) < 0) {
            buf[bufsize - 1] = '\0';
            return 0;
        }
        buf[bufsize - 1] = '\0';
    }
    *tok = end;
    return buf;
}


/*
 *  Format a number as a string. Support radix 10 and 16.
 */
char *mprItoa(char *buf, int size, int value, int radix)
{
    char    numBuf[16];
    char    *cp, *dp, *endp;
    char    digits[] = "0123456789ABCDEF";
    int     negative;

    if (radix != 10 && radix != 16) {
        return 0;
    }

    cp = &numBuf[sizeof(numBuf)];
    *--cp = '\0';

    if (value < 0) {
        negative = 1;
        value = -value;
        size--;
    } else {
        negative = 0;
    }

    do {
        *--cp = digits[value % radix];
        value /= radix;
    } while (value > 0);

    if (negative) {
        *--cp = '-';
    }

    dp = buf;
    endp = &buf[size];
    while (dp < endp && *cp) {
        *dp++ = *cp++;
    }
    *dp++ = '\0';
    return buf;
}


/*
 *  Parse an ascii number. Supports radix 10 or 16.
 */
int mprAtoi(cchar *str, int radix)
{
    int     c, val, negative;

    mprAssert(radix == 10 || radix == 16);

    if (str == 0) {
        return 0;
    }

    val = 0;
    if (radix == 10 && *str == '-') {
        negative = 1;
        str++;
    } else {
        negative = 0;
    }

    if (radix == 10) {
        while (*str && isdigit((int) *str)) {
            val = (val * radix) + *str - '0';
            str++;
        }
    } else if (radix == 16) {
        if (*str == '0' && tolower((int) str[1]) == 'x') {
            str += 2;
        }
        while (*str) {
            c = tolower((int) *str);
            if (isdigit(c)) {
                val = (val * radix) + c - '0';
            } else if (c >= 'a' && c <= 'f') {
                val = (val * radix) + c - 'a' + 10;
            } else {
                break;
            }
            str++;
        }
    }

    return (negative) ? -val: val;
}


/*
 *  Make an argv array. Caller must free by calling mprFree(argv) to free everything.
 */
int mprMakeArgv(MprCtx ctx, cchar *program, cchar *cmd, int *argcp, char ***argvp)
{
    char        *cp, **argv, *buf, *args;
    int         size, argc;

    /*
     *  Allocate one buffer for argv and the actual args themselves
     */
    size = (int) strlen(cmd) + 1;

    buf = (char*) mprAlloc(ctx, (MPR_MAX_ARGC * sizeof(char*)) + size);
    if (buf == 0) {
        return MPR_ERR_NO_MEMORY;
    }

    args = &buf[MPR_MAX_ARGC * sizeof(char*)];
    strcpy(args, cmd);
    argv = (char**) buf;

    argc = 0;
    if (program) {
        argv[argc++] = (char*) mprStrdup(ctx, program);
    }

    for (cp = args; cp && *cp != '\0'; argc++) {
        if (argc >= MPR_MAX_ARGC) {
            mprAssert(argc < MPR_MAX_ARGC);
            mprFree(buf);
            *argvp = 0;
            if (argcp) {
                *argcp = 0;
            }
            return MPR_ERR_TOO_MANY;
        }
        while (isspace((int) *cp)) {
            cp++;
        }
        if (*cp == '\0')  {
            break;
        }
        if (*cp == '"') {
            cp++;
            argv[argc] = cp;
            while ((*cp != '\0') && (*cp != '"')) {
                cp++;
            }
        } else {
            argv[argc] = cp;
            while (*cp != '\0' && !isspace((int) *cp)) {
                cp++;
            }
        }
        if (*cp != '\0') {
            *cp++ = '\0';
        }
    }
    argv[argc] = 0;

    if (argcp) {
        *argcp = argc;
    }
    *argvp = argv;

    return argc;
}


char *mprStrnstr(cchar *str, cchar *pattern, int len)
{
    cchar   *start, *p;
    int     i;

    if (str == 0 || pattern == 0 || len == 0) {
        return 0;
    }

    while (*str && len-- > 0) {
        if (*str++ == *pattern) {
            start = str - 1;
            for (p = pattern + 1, i = len; *p && *str && i >= 0; i--) {
                if (*p++ != *str++) {
                    break;
                }
            }
            if (*p == '\0') {
                return (char*) start;
            }
        }
    }
    return 0;
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
/************************************************************************/
/*
 *  End of file "../mprString.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../mprTest.c"
 */
/************************************************************************/

/**
 *  mprTestLib.c - Embedthis Unit Test Framework Library
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



#if BLD_FEATURE_TEST

static void     adjustFailedCount(MprTestService *sp, int adj);
static void     adjustThreadCount(MprTestService *sp, int adj);
static void     buildFullNames(MprTestGroup *gp, cchar *runName);
static MprTestFailure *createFailure(MprTestGroup *gp, cchar *loc, cchar *message);
static MprTestGroup *createTestGroup(MprTestService *sp, MprTestDef *def);
static bool     filterTestGroup(MprTestGroup *gp);
static bool     filterTestCast(MprTestGroup *gp, MprTestCase *tc);
static char     *getErrorMessage(MprTestGroup *gp, char *errorBuf, int size);
static int      parseFilter(MprTestService *sp, cchar *str);
static void     runTestGroup(MprTestGroup *gp);
static void     runTestProc(MprTestGroup *gp, MprTestCase *test);
static void     runTestThread(MprList *groups, void *threadp);
static int      setLogging(Mpr *mpr, char *logSpec);

#if BLD_FEATURE_MULTITHREAD
static MprList  *copyGroups(MprTestService *sp, MprList *groups);
#endif



MprTestService *mprCreateTestService(MprCtx ctx)
{
    MprTestService      *sp;

    sp = mprAllocObjZeroed(ctx, MprTestService);
    if (sp == 0) {
        return 0;
    }

    sp->iterations = 1;
    sp->numThreads = 1;
    sp->testFilter = mprCreateList(sp);
    sp->groups = mprCreateList(sp);
    sp->start = mprGetTime(sp);

#if BLD_FEATURE_MULTITHREAD
    sp->mutex = mprCreateLock(sp);
#endif
    return sp;
}



int mprParseTestArgs(MprTestService *sp, int argc, char *argv[])
{
    Mpr         *mpr;
    cchar       *programName;
    char        *argp, *logSpec;
    int         err, i, depth, nextArg, outputVersion;

    i = 0;
    err = 0;
    outputVersion = 0;
    logSpec = "stderr:1";

    mpr = mprGetMpr(sp);
    programName = mprGetBaseName(argv[0]);

    sp->name = BLD_PRODUCT;
    sp->testLog = getenv("TEST_LOG");

    /*
     *  Save the command line
     */
    sp->commandLine[0] = '\0';
    mprStrcat(sp->commandLine, sizeof(sp->commandLine), " ", mprGetBaseName(argv[i++]), 0);
    for (; i < argc; i++) {
        mprStrcat(sp->commandLine, sizeof(sp->commandLine), " ", argv[i], 0);
    }

    for (nextArg = 1; nextArg < argc; nextArg++) {
        argp = argv[nextArg];
        if (*argp != '-') {
            break;
        }

        if (strcmp(argp, "--continue") == 0) {
            sp->continueOnFailures = 1; 

        } else if (strcmp(argp, "--depth") == 0) {
            if (nextArg >= argc) {
                err++;
            } else {
                depth = atoi(argv[++nextArg]);
                if (depth < 0 || depth > 10) {
                    mprError(sp, "Bad test depth %d, (range 0-9)", depth);
                    err++;
                } else {
                    sp->testDepth = depth;
                }
            }

        } else if (strcmp(argp, "--debug") == 0 || strcmp(argp, "-d") == 0) {
            mprSetDebugMode(mpr, 1);
            sp->debugOnFailures = 1;

        } else if (strcmp(argp, "--echo") == 0) {
            sp->echoCmdLine = 1;

        } else if (strcmp(argp, "--filter") == 0 || strcmp(argp, "-f") == 0) {
            if (nextArg >= argc) {
                err++;
            } else {
                if (parseFilter(sp, argv[++nextArg]) < 0) {
                    err++;
                }
            }

        } else if (strcmp(argp, "--iterations") == 0 || (strcmp(argp, "-i") == 0)) {
            if (nextArg >= argc) {
                err++;
            } else {
                sp->iterations = atoi(argv[++nextArg]);
            }

        } else if (strcmp(argp, "--log") == 0 || strcmp(argp, "-l") == 0) {
            if (nextArg >= argc) {
                err++;
            } else {
                setLogging(mpr, argv[++nextArg]);
            }

        } else if (strcmp(argp, "--name") == 0) {
            if (nextArg >= argc) {
                err++;
            } else {
                sp->name = argv[++nextArg];
            }

        } else if (strcmp(argp, "--poolThreads") == 0) {
            if (nextArg >= argc) {
                err++;
            } else {
                i = atoi(argv[++nextArg]);
                if (i < 0 || i > 100) {
                    mprError(sp, "%s: Bad number of pool threads (0-100)", programName);
                    return MPR_ERR_BAD_ARGS;
                }
#if BLD_FEATURE_MULTITHREAD
                sp->poolThreads = i;
#else
                if (i > 1) {
                    mprLog(sp, 0, "%s: Program built single-threaded. Ignoring threads directive", programName);
                }
#endif
            }

        } else if (strcmp(argp, "--single") == 0 || strcmp(argp, "-s") == 0) {
            sp->singleStep = 1; 

        } else if (strcmp(argp, "--testLog") == 0) {
            if (nextArg >= argc) {
                err++;
            } else {
                sp->testLog = argv[++nextArg];
            }

        } else if (strcmp(argp, "--threads") == 0 || strcmp(argp, "-t") == 0) {
            if (nextArg >= argc) {
                err++;
            } else {
                i = atoi(argv[++nextArg]);
                if (i <= 0 || i > 100) {
                    mprError(sp, "%s: Bad number of threads (1-100)", programName);
                    return MPR_ERR_BAD_ARGS;
                }
#if BLD_FEATURE_MULTITHREAD
                sp->numThreads = i;
#else
                if (i > 1) {
                    mprLog(sp, 0, "%s: Program built single-threaded. Ignoring threads directive", programName);
                }
#endif
            }

        } else if (strcmp(argp, "--verbose") == 0 || strcmp(argp, "-v") == 0) {
            sp->verbose++;

        } else if (strcmp(argp, "-vv") == 0) {
            sp->verbose += 2;

        } else if (strcmp(argp, "-vvv") == 0) {
            sp->verbose += 3;

        } else if (strcmp(argp, "--version") == 0 || strcmp(argp, "-V") == 0) {
            outputVersion++;

        } else if (strcmp(argp, "-?") == 0 || (strcmp(argp, "--help") == 0 || strcmp(argp, "--?") == 0)) {
            err++;

        } else {
            /* Ignore unknown args */
        }
    }

#if LOAD_TEST_PACKAGES
    /* Must be at least one test module to load */
    if (nextArg >= argc) {
        err++;
    }
#endif

    if (err) {
        mprErrorPrintf(mpr, 
        "usage: %s [options]\n"
        "    --continue          # Continue on errors\n"
        "    --depth number      # Zero == basic, 1 == throrough, 2 extensive\n"
        "    --debug             # Run in debug mode\n"
        "    --echo              # Echo the command line\n"
        "    --filter pattern    # Filter tests by pattern x.y.z...\n"
        "    --iterations count  # Number of iterations to run the test\n"
        "    --log logFile:level # Log to file file at verbosity level\n"
        "    --name testName     # Log to file file at verbosity level\n"
        "    --poolThreads count # Set maximum pool threads\n"
        "    --single            # Single step tests\n"
        "    --testLog path      # Define a test log\n"
        "    --threads count     # Set maximum test (worker) threads\n"
        "    --verbose           # Verbose mode\n"
        "    --version           # Output version information\n",
        programName);
        return MPR_ERR_BAD_ARGS;
    }

    if (outputVersion) {
        mprErrorPrintf(mpr, "%s: Version: %s\n", BLD_NAME, BLD_VERSION);
        mprFree(mpr);
        return MPR_ERR_BAD_ARGS;
    }

    sp->argc = argc;
    sp->argv = argv;
    sp->firstArg = nextArg;

#if LOAD_TEST_PACKAGES
    for (i = nextArg; i < argc; i++) {
        if (loadModule(sp, argv[i]) < 0) {
            return MPR_ERR_CANT_OPEN;
        }
    }
#endif
    
#if BLD_FEATURE_MULTITHREAD
    mprSetMaxPoolThreads(sp, sp->poolThreads);
#endif

    return 0;
}



static int parseFilter(MprTestService *sp, cchar *filter)
{
    char    *str, *word, *tok;

    mprAssert(filter);
    if (filter == 0 || *filter == '\0') {
        return 0;
    }

    tok = 0;
    str = mprStrdup(sp, filter);
    word = mprStrTok(str, " \t\r\n", &tok);
    while (word) {
        if (mprAddItem(sp->testFilter, mprStrdup(sp, word)) < 0) {
            return MPR_ERR_NO_MEMORY;
        }
        word = mprStrTok(0, " \t\r\n", &tok);
    }
    mprFree(str);
    return 0;
}



#if LOAD_TEST_PACKAGES
static int loadModule(MprTestService *sp, cchar *fileName)
{
    char    *cp, *base, entry[MPR_MAX_FNAME], path[MPR_MAX_FNAME];

    mprAssert(fileName && *fileName);

    base = mprStrdup(sp, mprGetBaseName(fileName));
    mprAssert(base);
    if ((cp = strrchr(base, '.')) != 0) {
        *cp = '\0';
    }
    
    if (mprLookupModule(sp, base)) {
        return 0;
    }
                
    mprSprintf(entry, sizeof(entry), "%sInit", base);

    if (fileName[0] == '/' || (*fileName && fileName[1] == ':')) {
        mprSprintf(path, sizeof(path), "%s%s", fileName, BLD_BUILD_SHOBJ);
    } else {
        mprSprintf(path, sizeof(path), "./%s%s", fileName, BLD_BUILD_SHOBJ);
    }

    if (mprLoadModule(sp, path, entry, (void*) sp) == 0) {
        mprError(sp, "Can't load module %s", path);
        return -1;
    }

    return 0;
}
#endif



int mprRunTests(MprTestService *sp)
{
#if BLD_FEATURE_MULTITHREAD
    MprThread       *tp;
#endif
    MprTestGroup    *gp;
    int             next;

    /*
     *  Build the full names for all groups
     */
    next = 0; 
    while ((gp = mprGetNextItem(sp->groups, &next)) != 0) {
        buildFullNames(gp, gp->name);
    }

    sp->activeThreadCount = sp->numThreads;

    if (sp->echoCmdLine) {
        mprPrintf(sp, "  %s\n", sp->commandLine);
    }

#if BLD_FEATURE_MULTITHREAD
{
    int i;
    /*
     *  Create worker threads for each test thread. 
     */
    for (i = 1; i < sp->numThreads; i++) {
        MprList     *lp;
        char        tName[64];

        mprSprintf(tName, sizeof(tName), "test.%d", i);

        lp = copyGroups(sp, sp->groups);
        if (lp == 0) {
            return MPR_ERR_NO_MEMORY;
        }
        
        /*
         *  Build the full names for all groups
         */
        next = 0; 
        while ((gp = mprGetNextItem(lp, &next)) != 0) {
            buildFullNames(gp, gp->name);
        }


        tp = mprCreateThread(sp, tName, (MprThreadProc) runTestThread, (void*) lp, MPR_NORMAL_PRIORITY, 0);
        if (tp == 0) {
            return MPR_ERR_NO_MEMORY;
        }

        if (mprStartThread(tp) < 0) {
            mprError(sp, "Can't start thread %d", i);
            return MPR_ERR_CANT_INITIALIZE;
        }
    }
}
#endif

    sp->start = mprGetTime(sp);

    /*
     *  Utilize the main thread for the first test worker thread.
     *  This works even if we are built without MULTITHREAD.
     */
    runTestThread(sp->groups, 0);

#if BLD_FEATURE_MULTITHREAD
    /*
     *  Wait for all the threads to complete (simple but effective)
     *  TODO -- would be good to have a join API.
     */
    while (sp->activeThreadCount > 1) {
        mprSleep(sp, 75);
    }
#endif

    return (sp->totalFailedCount == 0) ? 0 : 1;
}



#if BLD_FEATURE_MULTITHREAD
static MprList *copyGroups(MprTestService *sp, MprList *groups)
{
    MprTestGroup    *gp, *newGp;
    MprList         *lp;
    int             next;

    lp = mprCreateList(sp);
    if (lp == 0) {
        return 0;
    }

    next = 0; 
    while ((gp = mprGetNextItem(groups, &next)) != 0) {
        newGp = createTestGroup(sp, gp->def);
        if (newGp == 0) {
            mprFree(lp);
            return 0;
        }
        if (mprAddItem(lp, newGp) < 0) {
            mprFree(lp);
            return 0;
        }
    }
    return lp;
}
#endif



/*
 *  Run the test groups. One invocation per thread. Used even if not multithreaded.
 */
void runTestThread(MprList *groups, void *threadp)
{
    MprTestService  *sp;
    MprTestGroup    *gp;
    int             next, i;

    /*
     *  Get the service pointer
     */
    gp = mprGetFirstItem(groups);
    if (gp == 0) {
        return;
    }
    sp = gp->service;
    mprAssert(sp);

#if UNUSED
    /*
     *  Skew the thread priorities a bit. Under extreme load, this can cause a bit of thread starvation and cause some 
     *  tests to timeout. So we don't use this currently.
     */
    if (threadp) {
        mprSetPriority(threadp, MPR_NORMAL_PRIORITY - 5 + (rand() % 10));
    }
#endif


    for (i = 0; i < sp->iterations; i++) {
        if (sp->totalFailedCount > 0 && !sp->continueOnFailures) {
            break;
        }
        next = 0; 
        while ((gp = mprGetNextItem(groups, &next)) != 0) {
            runTestGroup(gp);
        }
    }

    if (threadp) {
        adjustThreadCount(sp, -1);
    }
}



void mprReportTestResults(MprTestService *sp)
{
    if (sp->verbose <= 1 && sp->totalFailedCount == 0) {
        if (sp->verbose) {
            mprPrintf(sp, "\n");
        }
        mprPrintf(sp, "# PASSED all tests for \"%s\"\n", sp->name);
    }

    if (sp->verbose) {
        double  elapsed;

        elapsed = ((mprGetTime(sp) - sp->start) * 1.0 / 1000.0);
        mprPrintf(sp, "\n# %d tests completed, %d test(s) failed. ", sp->totalTestCount, sp->totalFailedCount);
        mprPrintf(sp, "Elapsed time: %5.2f seconds.\n", elapsed);
    }
    mprPrintf(sp, "\n");
}



static void buildFullNames(MprTestGroup *gp, cchar *name)
{
    MprTestGroup    *np;
    char            nameBuf[MPR_MAX_STRING];
    cchar           *nameStack[MPR_TEST_MAX_STACK];
    int             tos, nextItem;

    tos = 0;

    /*
     *  Build the full name for this case
     */
    nameStack[tos++] = name;
    for (np = gp->parent; np && np != np->parent && tos < MPR_TEST_MAX_STACK;  np = np->parent) {
        nameStack[tos++] = np->name;
    }
    mprStrcpy(nameBuf, sizeof(nameBuf), nameStack[--tos]);
    while (--tos >= 0) {
        mprStrcat(nameBuf, sizeof(nameBuf), ".", nameStack[tos], 0);
    }
    mprAssert(gp->fullName == 0);
    gp->fullName = mprStrdup(gp, nameBuf);

    /*
     *  Recurse for all test case groups
     */
    nextItem = 0;
    np = mprGetNextItem(gp->groups, &nextItem);
    while (np) {
        buildFullNames(np, np->name);
        np = mprGetNextItem(gp->groups, &nextItem);
    }
}



MprTestGroup *mprAddTestGroup(MprTestService *sp, MprTestDef *def)
{
    MprTestGroup    *gp;

    gp = createTestGroup(sp, def);
    if (gp == 0) {
        return 0;
    }

    if (mprAddItem(sp->groups, gp) < 0) {
        mprFree(gp);
        return 0;
    }
    return gp;
}


static MprTestGroup *createTestGroup(MprTestService *sp, MprTestDef *def)
{
    MprTestGroup    *gp, *child;
    MprTestDef      **dp;
    MprTestCase     *tc;

    gp = mprAllocObjZeroed(sp, MprTestGroup);
    if (gp == 0) {
        return 0;
    }
    gp->service = sp;

    gp->failures = mprCreateList(sp);
    if (gp->failures == 0) {
        mprFree(gp);
        return 0;
    }

    gp->cases = mprCreateList(sp);
    if (gp->cases == 0) {
        mprFree(gp);
        return 0;
    }

    gp->groups = mprCreateList(sp);
    if (gp->groups == 0) {
        mprFree(gp);
        return 0;
    }

    gp->def = def;
    gp->name = mprStrdup(sp, def->name);
    gp->success = 1;

    for (tc = def->caseDefs; tc->proc; tc++) {
        if (mprAddItem(gp->cases, tc) < 0) {
            mprFree(gp);
            return 0;
        }
    }

    if (def->groupDefs) {
        for (dp = &def->groupDefs[0]; *dp && (*dp)->name; dp++) {
            child = createTestGroup(sp, *dp);
            if (child == 0) {
                mprFree(gp);
                return 0;
            }
            if (mprAddItem(gp->groups, child) < 0) {
                mprFree(gp);
                return 0;
            }
            child->parent = gp;
        }
    }
    return gp;
}



void mprResetTestGroup(MprTestGroup *gp)
{
    gp->condWakeFlag = 0;
    gp->cond2WakeFlag = 0;

    gp->success = 1;

#if BLD_FEATURE_MULTITHREAD
    /*
     *  If a previous test failed, a cond var may be still signalled. 
     *  So we MUST recreate.
     */
    if (gp->cond) {
        mprFree(gp->cond);
    }
    gp->cond = mprCreateCond(gp);

    if (gp->cond2) {
        mprFree(gp->cond2);
    }
    gp->cond2 = mprCreateCond(gp);

    if (gp->mutex) {
        mprFree(gp->mutex);
    }
    gp->mutex = mprCreateLock(gp);
#endif
}



static void runTestGroup(MprTestGroup *parent)
{
    MprTestService  *sp;
    MprTestGroup    *gp, *nextGroup;
    MprTestCase     *tc;
    int             count, nextItem;

    sp = parent->service;

    if (parent->def->init && (*parent->def->init)(parent) < 0) {
        parent->failedCount++;
        return;
    }

    /*
     *  Recurse over sub groups
     */
    nextItem = 0;
    gp = mprGetNextItem(parent->groups, &nextItem);
    while (gp && (parent->success || sp->continueOnFailures)) {
        nextGroup = mprGetNextItem(parent->groups, &nextItem);
        if (gp->testDepth > sp->testDepth) {
            gp = nextGroup;
            continue;
        }

        /*
         *  See if this group has been filtered for execution
         */
        if (! filterTestGroup(gp)) {
            gp = nextGroup;
            continue;
        }
        count = sp->totalFailedCount;
        if (count > 0 && !sp->continueOnFailures) {
            if (parent->def->term) {
                (*parent->def->term)(parent);
            }
            return;
        }

        /*
         *  Recurse over all tests in this group
         */
        runTestGroup(gp);

        gp->testCount++;

        if (! gp->success) {
            /*  Propagate the failures up the parent chain */
            parent->failedCount++;
            parent->success = 0;
        }
        gp = nextGroup;
    }

    /*
     *  Run test cases for this group
     */
    nextItem = 0;
    tc = mprGetNextItem(parent->cases, &nextItem);
    while (tc && (parent->success || sp->continueOnFailures)) {
        if (parent->testDepth <= sp->testDepth) {
            if (filterTestCast(parent, tc)) {
                runTestProc(parent, tc);
            }
        }
        tc = mprGetNextItem(parent->cases, &nextItem);
    }

    if (parent->def->term && (*parent->def->term)(parent) < 0) {
        parent->failedCount++;
    }
}



/*
 *  Return true if we are to run the test group
 */
static bool filterTestGroup(MprTestGroup *gp)
{
    MprTestService  *sp;
    MprList         *testFilter;
    char            *pattern;
    int             len, next;

    sp = gp->service;
    testFilter = sp->testFilter;

    if (testFilter == 0) {
        return 1;
    }

    /*
     *  See if this test has been filtered
     */
    if (mprGetListCount(testFilter) > 0) {
        next = 0;
        pattern = mprGetNextItem(testFilter, &next);
        while (pattern) {
            len = min((int) strlen(pattern), (int) strlen(gp->fullName));
            if (mprStrcmpAnyCaseCount(gp->fullName, pattern, len) == 0) {
                break;
            }
            pattern = mprGetNextItem(testFilter, &next);
        }
        if (pattern == 0) {
            return 0;
        }
    }
    return 1;
}



/*
 *  Return true if we are to run the test case
 */
static bool filterTestCast(MprTestGroup *gp, MprTestCase *tc)
{
    MprTestService  *sp;
    MprList         *testFilter;
    char            *pattern, *fullName;
    int             len, next;

    sp = gp->service;
    testFilter = sp->testFilter;

    if (testFilter == 0) {
        return 1;
    }

    /*
     *  See if this test has been filtered
     */
    if (mprGetListCount(testFilter) > 0) {
        mprAllocSprintf(gp, &fullName, -1, "%s.%s", gp->fullName, tc->name);
        next = 0;
        pattern = mprGetNextItem(testFilter, &next);
        while (pattern) {
            len = min((int) strlen(pattern), (int) strlen(fullName));
            if (mprStrcmpAnyCaseCount(fullName, pattern, len) == 0) {
                break;
            }
            pattern = mprGetNextItem(testFilter, &next);
        }
        mprFree(fullName);
        if (pattern == 0) {
            return 0;
        }
    }
    return 1;
}



static void runTestProc(MprTestGroup *gp, MprTestCase *test)
{
    MprTestService      *sp;

    if (test->proc == 0) {
        return;
    }

    sp = gp->service;

    mprResetTestGroup(gp);

    if (sp->singleStep) {
        mprPrintf(gp, "Run test %s.%s, press <ENTER>: ", gp->fullName, test->name);
        getchar();

    } else if (sp->verbose > 2) {
        mprPrintf(gp, "Run test %s.%s: \n", gp->fullName, test->name);
    }

    if (gp->skip) {
        if (sp->verbose) {
            mprPrintf(gp, "Skipping test %s.%s: \n", gp->fullName, test->name);
        }
        
    } else {
    
        /*
         *  The function is part of the enclosing MprTest group
         */
        (test->proc)(gp);
    
        mprLock(sp->mutex);
        if (gp->success) {
            if (sp->verbose == 1) {
                int count = ++sp->totalTestCount;
                if ((count % 50) == 1) {
                    mprPrintf(gp, "  ");
                }
                mprPrintf(gp, (gp->success) ? "." : "!");
                if (count > 0 && (count % 50) == 0) {
                    mprPrintf(gp, " (%d, %d)\n", count, sp->totalFailedCount);
                }
    
            } else if (sp->verbose > 1) {
                mprPrintf(gp, "# PASSED test \"%s.%s\"\n", gp->fullName, test->name);
            }
            
        } else {
            char    errorMsg[MPR_MAX_STRING];
    
            getErrorMessage(gp, errorMsg, sizeof(errorMsg));
            mprPrintf(gp, "FAILED test \"%s.%s\"\nDetails: %s\n", gp->fullName, test->name, errorMsg);
        }
    }
    mprUnlock(sp->mutex);
}



static char *getErrorMessage(MprTestGroup *gp, char *errorBuf, int size)
{
    MprTestFailure  *fp;
    char            msg[MPR_MAX_STRING];
    int             nextItem;

    mprAssert(errorBuf);

    errorBuf[0] = '\0';
    nextItem = 0;
    fp = mprGetNextItem(gp->failures, &nextItem);
    while (fp) {
        mprSprintf(msg, sizeof(msg),
            "Failure in %s\nAssertion: \"%s\"\n", fp->loc, fp->message);
        if (mprStrcat(errorBuf, size, "", msg, 0) < 0) {
            /* Overflow */
            errorBuf[size - 1] = '\0';
            break;
        }
        fp = mprGetNextItem(gp->failures, &nextItem);
    }
    return errorBuf;
}



static int addFailure(MprTestGroup *gp, cchar *loc, cchar *message)
{
    MprTestFailure  *fp;

    fp = createFailure(gp, loc, message);
    if (fp == 0) {
        mprAssert(fp);
        return MPR_ERR_NO_MEMORY;
    }
    mprAddItem(gp->failures, fp);
    return 0;
}



static MprTestFailure *createFailure(MprTestGroup *gp, cchar *loc, cchar *message)
{
    MprTestFailure  *fp;

    fp = mprAllocObj(gp, MprTestFailure);
    if (fp == 0) {
        return 0;
    }
    fp->loc = mprStrdup(fp, loc);
    fp->message = mprStrdup(fp, message);
    return fp;
}


bool assertTrue(MprTestGroup *gp, cchar *loc, bool isTrue, cchar *msg)
{
    if (! isTrue) {
        gp->success = isTrue;
    }
    if (! isTrue) {
        if (gp->service->debugOnFailures) {
            mprBreakpoint(0, 0);
        }
        addFailure(gp, loc, msg);
        gp->failedCount++;
        adjustFailedCount(gp->service, 1);
    }
    return isTrue;
}



bool mprWaitForTestToComplete(MprTestGroup *gp, int timeout)
{
    MprTestService  *sp;
    MprTime         mark;

    sp = gp->service;

    mark = mprGetTime(gp);
    if (mprGetDebugMode(gp)) {
        timeout = MAXINT;
    }

#if BLD_FEATURE_MULTITHREAD
    if (mprIsRunningEventsThread(sp)) {
        if (mprWaitForCond(gp->cond, timeout) < 0) {
            return 0;
        }
        return 1;

    } else 
#endif
    {
        /*
         *  Use a short-nap here as complete may get set after testing at the top of the loop and we may do a long wait 
         *  for an event that has already happened.
         */
        while (! gp->condWakeFlag &&  mprGetTimeRemaining(gp, mark, timeout) > 0) {
            mprServiceEvents(gp, timeout, MPR_SERVICE_ONE_THING);
        }
        
        if (gp->condWakeFlag) {
            gp->condWakeFlag = 0;
            return 1;
            
        } else {
            return 0;
        }
    }
}



void mprSignalTestComplete(MprTestGroup *gp)
{
    gp->condWakeFlag = 1;
#if BLD_FEATURE_MULTITHREAD
    if (mprIsRunningEventsThread(gp)) {
        mprSignalCond(gp->cond);
    }
#endif
}



bool mprWaitForTestToComplete2(MprTestGroup *gp, int timeout)
{
    MprTestService  *sp;
    MprTime         mark;

    sp = gp->service;

    mark = mprGetTime(gp);
#if BLD_FEATURE_MULTITHREAD
    if (mprIsRunningEventsThread(sp)) {
        if (mprWaitForCond(gp->cond2, timeout) < 0) {
            return 0;
        }
    } else 
#endif
    {
        /*
         *  Use a short-nap here as complete may get set after testing at
         *  the top of the loop and we may do a long wait for an event
         *  that has already happened.
         */
        while (! gp->cond2WakeFlag &&  mprGetTimeRemaining(gp, mark, timeout) > 0) {
            mprServiceEvents(gp, -1, MPR_SERVICE_ONE_THING);
        }
        if (gp->cond2WakeFlag == 0) {
            return 0;
        }
        gp->cond2WakeFlag = 0;
    }
    return 1;
}



void mprSignalTestComplete2(MprTestGroup *gp)
{
    gp->cond2WakeFlag = 1;
#if BLD_FEATURE_MULTITHREAD
    if (mprIsRunningEventsThread(gp)) {
        mprSignalCond(gp->cond2);
    }
#endif
}



static void adjustThreadCount(MprTestService *sp, int adj)
{
#if BLD_FEATURE_MULTITHREAD
    mprLock(sp->mutex);
    sp->activeThreadCount += adj;
    mprUnlock(sp->mutex);
#endif
}



static void adjustFailedCount(MprTestService *sp, int adj)
{
    mprLock(sp->mutex);
    sp->totalFailedCount += adj;
    mprUnlock(sp->mutex);
}



static void logHandler(MprCtx ctx, int flags, int level, cchar *msg)
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
            mprStaticPrintf(file, "%s: Error: %s\n", prefix, msg);
        } else {
            mprFprintf(file, "%s: Error: %s\n", prefix, msg);
        }

    } else if (flags & MPR_FATAL_SRC) {
        mprFprintf(file, "%s: Fatal: %s\n", prefix, msg);
        
    } else if (flags & MPR_RAW) {
        mprFprintf(file, "%s", msg);
    }
    
    if (flags & (MPR_ERROR_SRC | MPR_FATAL_SRC)) {
        mprBreakpoint();
    }
}



static int setLogging(Mpr *mpr, char *logSpec)
{
    MprFile     *file;
    char        *levelSpec;
    int         level;

    level = 0;

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
            return MPR_ERR_CANT_OPEN;
        }
    }

    mprSetLogLevel(mpr, level);
    mprSetLogHandler(mpr, logHandler, (void*) file);

    return 0;
}

#else
void __mprUnitTest() {}
#endif /* BLD_FEATURE_TEST */

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
/************************************************************************/
/*
 *  End of file "../mprTest.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../mprThread.c"
 */
/************************************************************************/

/**
 *  mprThread.c - Primitive multi-threading support for Windows
 *
 *  This module provides threading, mutex and condition variable APIs.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



#if BLD_FEATURE_MULTITHREAD


static void threadProc(MprThread *tp);
static int threadServiceDestructor(MprThreadService *ts);
static int threadDestructor(MprThread *tp);


MprThreadService *mprCreateThreadService(Mpr *mpr)
{
    MprThreadService    *ts;

    mprAssert(mpr);

    ts = mprAllocObjWithDestructor(mpr, MprThreadService, threadServiceDestructor);
    if (ts == 0) {
        return 0;
    }

    ts->mutex = mprCreateLock(mpr);
    if (ts->mutex == 0) {
        mprFree(ts);
        return 0;
    }
    ts->threads = mprCreateList(ts);
    if (ts->threads == 0) {
        mprFree(ts);
        return 0;
    }

    mpr->threadService = ts;
    ts->stackSize = MPR_DEFAULT_STACK;

    /*
     *  Don't actually create the thread. Just create a thread object for this main thread.
     */
    ts->mainThread = mprCreateThread(ts, "main", 0, 0, MPR_NORMAL_PRIORITY, 0);
    if (ts->mainThread == 0) {
        mprFree(ts);
        return 0;
    }

    if (mprAddItem(ts->threads, ts->mainThread) < 0) {
        mprFree(ts);
        return 0;
    }
    return ts;
}



static int threadServiceDestructor(MprThreadService *ts)
{
    /*  TODO - what about running threads?  */
    return 0;
}



int mprStartThreadService(MprThreadService *ts)
{
    return 0;
}


int mprStopThreadService(MprThreadService *ts, int timeout)
{
    return 0;
}



void mprSetThreadStackSize(MprCtx ctx, int size)
{
    mprGetMpr(ctx)->threadService->stackSize = size;
}



/*
 *  Return the current thread object
 */
MprThread *mprGetCurrentThread(MprCtx ctx)
{
    MprThreadService    *ts;
    MprThread           *tp;
    MprOsThread         id;
    int                 i;

    ts = mprGetMpr(ctx)->threadService;

    mprLock(ts->mutex);

    id = mprGetCurrentOsThread(ts);

    for (i = 0; i < ts->threads->length; i++) {
        tp = (MprThread*) mprGetItem(ts->threads, i);
        if (tp->osThreadID == id) {
            mprUnlock(ts->mutex);
            return tp;
        }
    }

    mprUnlock(ts->mutex);
    return 0;
}



/*
 *  Return the current thread object
 */
cchar *mprGetCurrentThreadName(MprCtx ctx)
{
    MprThread       *tp;

    tp = mprGetCurrentThread(ctx);
    if (tp == 0) {
        return 0;
    }

    return tp->name;
}



/*
 *  Return the current thread object
 */
void mprSetCurrentThreadPriority(MprCtx ctx, int pri)
{
    MprThread       *tp;

    tp = mprGetCurrentThread(ctx);
    if (tp == 0) {
        return;
    }

    mprSetThreadPriority(tp, pri);
}



/*
 *  Create a main thread
 */
MprThread *mprCreateThread(MprCtx ctx, cchar *name, MprThreadProc entry, void *data, int priority, int stackSize)
{
    MprThreadService    *ts;
    MprThread           *tp;

    ts = mprGetMpr(ctx)->threadService;
    if (ts) {
        ctx = ts;
    }

    tp = mprAllocObjWithDestructor(ctx, MprThread, threadDestructor);
    if (tp == 0) {
        return 0;
    }

    tp->osThreadID = mprGetCurrentOsThread(ctx);
    tp->data = data;
    tp->entry = entry;
    tp->name = mprStrdup(tp, name);
    tp->mutex = mprCreateLock(tp);
    tp->pid = getpid();
    tp->priority = priority;

    if (stackSize == 0) {
        tp->stackSize = ts->stackSize;
    } else {
        tp->stackSize = stackSize;
    }

#if BLD_WIN_LIKE
    tp->threadHandle = 0;
#endif

    if (ts && ts->threads) {
        mprLock(ts->mutex);
        if (mprAddItem(ts->threads, tp) < 0) {
            mprFree(tp);
            mprUnlock(ts->mutex);
            return 0;
        }
        mprUnlock(ts->mutex);
    }
    return tp;
}



/*
 *  Destroy a thread
 */
static int threadDestructor(MprThread *tp)
{
    MprThreadService    *ts;

    mprLock(tp->mutex);

    ts = mprGetMpr(tp)->threadService;
    mprRemoveItem(ts->threads, tp);

#if BLD_WIN_LIKE
    if (tp->threadHandle) {
        CloseHandle(tp->threadHandle);
    }
#endif
    return 0;
}



/*
 *  Entry thread function
 */ 
#if BLD_WIN_LIKE
static uint __stdcall threadProcWrapper(void *data) 
{
    threadProc((MprThread*) data);
    return 0;
}
#elif VXWORKS

static int threadProcWrapper(void *data) 
{
    threadProc((MprThread*) data);
    return 0;
}

#else
void *threadProcWrapper(void *data) 
{
    threadProc((MprThread*) data);
    return 0;
}

#endif



/*
 *  Thread entry
 */
static void threadProc(MprThread *tp)
{
    mprAssert(tp);

    tp->osThreadID = mprGetCurrentOsThread(tp);

#if VXWORKS
    tp->pid = tp->osThreadID;
#else
    tp->pid = getpid();
#endif

    (tp->entry)(tp->data, tp);

    mprFree(tp);
}



/*
 *  Start a thread
 */
int mprStartThread(MprThread *tp)
{
    mprLock(tp->mutex);

#if BLD_WIN_LIKE
{
    HANDLE          h;
    uint            threadId;

    h = (HANDLE) _beginthreadex(NULL, 0, threadProcWrapper, (void*) tp, 0, &threadId);
    if (h == NULL) {
        mprUnlock(tp->mutex);
        return MPR_ERR_CANT_INITIALIZE;
    }
    tp->osThreadID = (int) threadId;
    tp->threadHandle = (HANDLE) h;
}
#elif VXWORKS
{
    int     taskHandle, pri;

    taskPriorityGet(taskIdSelf(), &pri);
    taskHandle = taskSpawn(tp->name, pri, 0, tp->stackSize, (FUNCPTR) threadProcWrapper, (int) tp, 
        0, 0, 0, 0, 0, 0, 0, 0, 0);

    if (taskHandle < 0) {
        mprUnlock(tp->mutex);
        mprError(tp, "Can't create thread %s\n", tp->name);
        return MPR_ERR_CANT_INITIALIZE;
    }
}
#else /* UNIX */
{
    pthread_attr_t  attr;
    pthread_t       h;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, tp->stackSize);

    if (pthread_create(&h, &attr, threadProcWrapper, (void*) tp) != 0) { 
        mprAssert(0);
        pthread_attr_destroy(&attr);
        mprUnlock(tp->mutex);
        return MPR_ERR_CANT_CREATE;
    }
    pthread_attr_destroy(&attr);
}
#endif

    mprSetThreadPriority(tp, tp->priority);

    mprUnlock(tp->mutex);
    return 0;
}



MprOsThread mprGetCurrentOsThread()
{
#if BLD_UNIX_LIKE
    return (MprOsThread) pthread_self();
#elif BLD_WIN_LIKE
    return (MprOsThread) GetCurrentThreadId();
#elif VXWORKS
    return (MprOsThread) taskIdSelf();
#endif
}



void mprSetThreadPriority(MprThread *tp, int newPriority)
{
    int     osPri;

    mprLock(tp->mutex);

    osPri = mprMapMprPriorityToOs(newPriority);

#if BLD_WIN_LIKE
    SetThreadPriority(tp->threadHandle, osPri);
#elif VXWORKS
    taskPrioritySet(tp->osThreadID, osPri);
#else
    setpriority(PRIO_PROCESS, tp->pid, osPri);
#endif

    tp->priority = newPriority;

    mprUnlock(tp->mutex);
}


static int threadLocalDestructor(MprThreadLocal *tls)
{
#if BLD_UNIX_LIKE
    pthread_key_delete(tls->key);
#elif BLD_WIN_LIKE
    TlsFree(tls->key);
#endif
    return 0;
}


int mprSetThreadData(MprThreadLocal *tls, void *value)
{
    bool    err;
#if BLD_UNIX_LIKE
    err = pthread_setspecific(tls->key, value) != 0;
#elif BLD_WIN_LIKE
    err = TlsSetValue(tls->key, value) != 0;
#endif
    return (err) ? MPR_ERR_CANT_WRITE: 0;
}


void *mprGetThreadData(MprThreadLocal *tls)
{
#if BLD_UNIX_LIKE
    return pthread_getspecific(tls->key);
#elif BLD_WIN_LIKE
    return TlsGetValue(tls->key);
#elif VXWORKS
    /* TODO VXWORKS */
    return 0;
#endif
}


MprThreadLocal *mprCreateThreadLocal()
{
    MprThreadLocal      *tls;

    tls = mprAllocObjWithDestructorZeroed(mprGetMpr(0), MprThreadLocal, threadLocalDestructor);
    if (tls == 0) {
        return 0;
    }
#if BLD_UNIX_LIKE
    if (pthread_key_create(&tls->key, NULL) != 0) {
        mprFree(tls);
        return 0;
    }
#endif
    return tls;
}



#if BLD_WIN_LIKE
/*
 *  Map Mpr priority to Windows native priority. Windows priorities range from -15 to +15 (zero is normal). 
 *  Warning: +15 will not yield the CPU, -15 may get starved. We should be very wary going above +11.
 */

int mprMapMprPriorityToOs(int mprPriority)
{
    mprAssert(mprPriority >= 0 && mprPriority <= 100);
 
    if (mprPriority <= MPR_BACKGROUND_PRIORITY) {
        return THREAD_PRIORITY_LOWEST;
    } else if (mprPriority <= MPR_LOW_PRIORITY) {
        return THREAD_PRIORITY_BELOW_NORMAL;
    } else if (mprPriority <= MPR_NORMAL_PRIORITY) {
        return THREAD_PRIORITY_NORMAL;
    } else if (mprPriority <= MPR_HIGH_PRIORITY) {
        return THREAD_PRIORITY_ABOVE_NORMAL;
    } else {
        return THREAD_PRIORITY_HIGHEST;
    }
}



/*
 *  Map Windows priority to Mpr priority
 */ 
int mprMapOsPriorityToMpr(int nativePriority)
{
    int     priority;

    priority = (45 * nativePriority) + 50;
    if (priority < 0) {
        priority = 0;
    }
    if (priority >= 100) {
        priority = 99;
    }
    return priority;
}


#elif VXWORKS
/*
 *  Map MR priority to linux native priority. Unix priorities range from -19 to +19. Linux does -20 to +19.
 */

int mprMapMprPriorityToOs(int mprPriority)
{
    int     nativePriority;

    mprAssert(mprPriority >= 0 && mprPriority < 100);

    nativePriority = (100 - mprPriority) * 5 / 2;

    if (nativePriority < 10) {
        nativePriority = 10;
    } else if (nativePriority > 255) {
        nativePriority = 255;
    }
    return nativePriority;
}



/*
 *  Map O/S priority to Mpr priority.
 */ 
int mprMapOsPriorityToMpr(int nativePriority)
{
    int     priority;

    priority = (255 - nativePriority) * 2 / 5;
    if (priority < 0) {
        priority = 0;
    }
    if (priority >= 100) {
        priority = 99;
    }
    return priority;
}


#else /* UNIX */
/*
 *  Map MR priority to linux native priority. Unix priorities range from -19 to +19. Linux does -20 to +19. 
 */
int mprMapMprPriorityToOs(int mprPriority)
{
    mprAssert(mprPriority >= 0 && mprPriority < 100);

    if (mprPriority <= MPR_BACKGROUND_PRIORITY) {
        return 19;
    } else if (mprPriority <= MPR_LOW_PRIORITY) {
        return 10;
    } else if (mprPriority <= MPR_NORMAL_PRIORITY) {
        return 0;
    } else if (mprPriority <= MPR_HIGH_PRIORITY) {
        return -8;
    } else {
        return -19;
    }
    mprAssert(0);
    return 0;
}


/*
 *  Map O/S priority to Mpr priority.
 */ 
int mprMapOsPriorityToMpr(int nativePriority)
{
    int     priority;

    priority = (nativePriority + 19) * (100 / 40); 
    if (priority < 0) {
        priority = 0;
    }
    if (priority >= 100) {
        priority = 99;
    }
    return priority;
}

#endif /* UNIX */



#else /* BLD_FEATURE_MULTITHREAD */
void __dummyMprThread() {}

cchar *mprGetCurrentThreadName(MprCtx ctx) { return "main"; }
#endif /* BLD_FEATURE_MULTITHREAD */

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
/************************************************************************/
/*
 *  End of file "../mprThread.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../mprTime.c"
 */
/************************************************************************/

/**
 *  mprTime.c - Date and Time handling
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */




#if BLD_UNIX_LIKE
#undef localtime
#undef localtime_r
#undef gmtime
#undef gmtime_r
#undef ctime
#undef ctime_r
#undef asctime
#undef asctime_r
#endif

/*
 *  Token types ored inot the TimeToken value
 */
#define TOKEN_DAY       0x01000000
#define TOKEN_MONTH     0x02000000
#define TOKEN_ZONE      0x04000000
#define TOKEN_OFFSET    0x08000000
#define TOKEN_MASK      0xFF000000

typedef struct TimeToken {
    char    *name;
    int     value;
} TimeToken;

static TimeToken days[] = {
    { "sun",  0 | TOKEN_DAY },
    { "mon",  1 | TOKEN_DAY },
    { "tue",  2 | TOKEN_DAY },
    { "wed",  3 | TOKEN_DAY },
    { "thu",  4 | TOKEN_DAY },
    { "fri",  5 | TOKEN_DAY },
    { "sat",  6 | TOKEN_DAY },
    { 0, 0 },
};

static TimeToken fullDays[] = {
    { "sunday",     0 | TOKEN_DAY },
    { "monday",     1 | TOKEN_DAY },
    { "tuesday",    2 | TOKEN_DAY },
    { "wednesday",  3 | TOKEN_DAY },
    { "thursday",   4 | TOKEN_DAY },
    { "friday",     5 | TOKEN_DAY },
    { "saturday",   6 | TOKEN_DAY },
    { 0, 0 },
};

/*
 *  Make origin 1 to correspond to user date entries 10/28/2009
 */
static TimeToken months[] = {
    { "jan",  1 | TOKEN_MONTH },
    { "feb",  2 | TOKEN_MONTH },
    { "mar",  3 | TOKEN_MONTH },
    { "apr",  4 | TOKEN_MONTH },
    { "may",  5 | TOKEN_MONTH },
    { "jun",  6 | TOKEN_MONTH },
    { "jul",  7 | TOKEN_MONTH },
    { "aug",  8 | TOKEN_MONTH },
    { "sep",  9 | TOKEN_MONTH },
    { "oct", 10 | TOKEN_MONTH },
    { "nov", 11 | TOKEN_MONTH },
    { "dec", 12 | TOKEN_MONTH },
    { 0, 0 },
};

static TimeToken fullMonths[] = {
    { "january",    1 | TOKEN_MONTH },
    { "february",   2 | TOKEN_MONTH },
    { "march",      3 | TOKEN_MONTH },
    { "april",      4 | TOKEN_MONTH },
    { "may",        5 | TOKEN_MONTH },
    { "june",       6 | TOKEN_MONTH },
    { "july",       7 | TOKEN_MONTH },
    { "august",     8 | TOKEN_MONTH },
    { "september",  9 | TOKEN_MONTH },
    { "october",   10 | TOKEN_MONTH },
    { "november",  11 | TOKEN_MONTH },
    { "december",  12 | TOKEN_MONTH },
    { 0, 0 }
};

static TimeToken ampm[] = {
    { "am", 0 | TOKEN_OFFSET },
    { "pm", 0 | TOKEN_OFFSET },
    { 0, 0 },
};


static TimeToken zones[] = {
    { "ut",      0 | TOKEN_ZONE},
    { "utc",     0 | TOKEN_ZONE},
    { "gmt",     0 | TOKEN_ZONE},
    { "edt",  -240 | TOKEN_ZONE},
    { "est",  -300 | TOKEN_ZONE},
    { "cdt",  -300 | TOKEN_ZONE},
    { "cst",  -360 | TOKEN_ZONE},
    { "mdt",  -360 | TOKEN_ZONE},
    { "mst",  -420 | TOKEN_ZONE},
    { "pdt",  -420 | TOKEN_ZONE},
    { "pst",  -480 | TOKEN_ZONE},
    { 0, 0 },
};


static TimeToken offsets[] = {
    { "tomorrow",    86400 | TOKEN_OFFSET},
    { "yesterday",  -86400 | TOKEN_OFFSET},
    { "next week",   (86400 * 7) | TOKEN_OFFSET},
    { "last week",  -(86400 * 7) | TOKEN_OFFSET},
    { 0, 0 },
};

/*
 *  TODO FUTURE
 *
 *  August 25th, 2009
 *  25 Aug 2009
 *  Aug 25 5pm
 *  5pm August 25
 *  next saturday
 *  tomorrow
 *  next thursday at 4pm
 *  at 4pm
 *  eod
 *  tomorrow eod
 *  eod tuesday
 *  eoy
 *  eom
 *  in 5 minutes
 *  5 minutes from now
 *  5 hours before now
 *  2 hours before noon
 *  2 days from tomorrow
 */

static void validateTime(MprCtx ctx, struct tm *tm);

#if BLD_WIN_LIKE || BREW || VXWORKS
static int gettimeofday(struct timeval *tv, struct timezone *tz);
#endif

/*
 *  Initialize the time service
 */
int mprCreateTimeService(MprCtx ctx)
{
    Mpr                 *mpr;
    TimeToken           *tt;
    struct timezone     tz;
    struct timeval      tv;

    mpr = mprGetMpr(ctx);
    mpr->timeTokens = mprCreateHash(mpr, -1);
    ctx = mpr->timeTokens;

    for (tt = days; tt->name; tt++) {
        mprAddHash(mpr->timeTokens, tt->name, (void*) tt);
    }
    for (tt = fullDays; tt->name; tt++) {
        mprAddHash(mpr->timeTokens, tt->name, (void*) tt);
    }
    for (tt = months; tt->name; tt++) {
        mprAddHash(mpr->timeTokens, tt->name, (void*) tt);
    }
    for (tt = fullMonths; tt->name; tt++) {
        mprAddHash(mpr->timeTokens, tt->name, (void*) tt);
    }
    for (tt = ampm; tt->name; tt++) {
        mprAddHash(mpr->timeTokens, tt->name, (void*) tt);
    }
    for (tt = zones; tt->name; tt++) {
        mprAddHash(mpr->timeTokens, tt->name, (void*) tt);
    }
    for (tt = offsets; tt->name; tt++) {
        mprAddHash(mpr->timeTokens, tt->name, (void*) tt);
    }

    /* TODO VXWORKS */
    gettimeofday(&tv, &tz);
    mpr->timezone = -tz.tz_minuteswest;

    return 0;
}


/**
 *  Returns time in milliseconds since the epoch: 0:0:0 UTC Jan 1 1970.
 */
MprTime mprGetTime(MprCtx ctx)
{
    struct timeval  tv;

#if VXWORKS
    return tickGet();
#else
    if (gettimeofday(&tv, 0) < 0) {
        mprAssert(0);
        return 0;
    }
#endif
    return (MprTime) (((MprTime) tv.tv_sec) * 1000) + (tv.tv_usec / 1000);
}


/*
 *  Return the number of milliseconds until the given timeout has expired.
 */
MprTime mprGetTimeRemaining(MprCtx ctx, MprTime mark, uint timeout)
{
    MprTime     now, diff;

    now = mprGetTime(ctx);
    diff = (now - mark);

    if (diff < 0) {
        /*
         *  Detect time going backwards
         */
        mprAssert(diff >= 0);
        diff = 0;
    }
    return (timeout - diff);
}


/*
 *  Return the elapsed time since a timer marker
 */
MprTime mprGetElapsedTime(MprCtx ctx, MprTime mark)
{
    return mprGetTime(ctx) - mark;
}


int mprCompareTime(MprTime t1, MprTime t2)
{
    if (t1 < t2) {
        return -1;
    } else if (t1 == t2) {
        return 0;
    }
    return 1;
}


/*
 *  Make a time value interpreted using the local time value
 */
MprTime mprMakeLocalTime(MprCtx ctx, struct tm *tm)
{
    MprTime     rc;
    
    rc = mktime(tm);
    if (rc < 0) {
        return rc;
    }
    return rc * MPR_TICKS_PER_SEC;
}


/*
 *  Thread-safe wrapping of localtime
 */
struct tm *mprLocaltime(MprCtx ctx, struct tm *timep, MprTime time)
{
    time_t      when;

    when = time / MPR_TICKS_PER_SEC;

#if BLD_UNIX_LIKE
    localtime_r(&when, timep);
    return timep;
#else
    *timep = *localtime(&when);
#endif
    return timep;
}


/*
 *  Thread-safe wrapping of gmtime
 */
struct tm *mprGmtime(MprCtx ctx, struct tm *timep, MprTime time)
{
    time_t      when;

    when = time / MPR_TICKS_PER_SEC;

#if BLD_UNIX_LIKE
    gmtime_r(&when, timep);
#else
    *timep = *gmtime(&when);
#endif
    return timep;
}


/*
 *  Thread-safe wrapping of ctime. Outputs in the format: "Thu Nov 24 18:22:48 1986\n"
 */
int mprCtime(MprCtx ctx, char *buf, int bufsize, MprTime time)
{
    time_t      when;
    char        *cp;
    int         len;

    when = time / MPR_TICKS_PER_SEC;

#if BLD_UNIX_LIKE
{
    char        localBuf[80];
    cp = ctime_r(&when, localBuf);
}
#else
    cp = ctime(&when);
#endif
    if ((int) strlen(cp) >= bufsize) {
        mprStrcpy(buf, bufsize, "WONT FIT");
        mprAssert(0);
        return MPR_ERR_WONT_FIT;
    }
    len = mprStrcpy(buf, bufsize, cp);

    if (buf[len - 1] == '\n') {
        buf[len - 1] = '\0';
    }

    return 0;
}


/*
 *  Thread-safe wrapping of asctime Outputs in the format: "Thu Nov 24 18:22:48 1986\n"
 */
int mprAsctime(MprCtx ctx, char *buf, int bufsize, const struct tm *timeptr)
{
    char    *cp;

    mprAssert(buf);
    mprAssert(timeptr);

#if BLD_UNIX_LIKE
{
    char    localBuf[80];
    cp = asctime_r(timeptr, localBuf);
}
#else
    cp = asctime(timeptr);
#endif
    if ((int) strlen(cp) >= bufsize) {
        mprAssert(0);
        mprGlobalUnlock(ctx);
        return MPR_ERR_WONT_FIT;
    }
    mprStrcpy(buf, bufsize, cp);

    return (int) strlen(buf);
}


/*
 *  Wrapping of strftime
 *  Useful formats:
 *      RFC822: "%a, %d %b %Y %H:%M:%S %Z           "Fri, 07 Jan 2003 12:12:21 PDT"
 *              "%T %F                              "12:12:21 2007-01-03"
 *              "%v                                 "07-Jul-2003"
 *              "%+                                 "Fri Jan 07 12:12:21 PDT 2003"
 */
int mprStrftime(MprCtx ctx, char *buf, int bufsize, cchar *fmt, const struct tm *timeptr)
{
    struct tm       tm;
#if BLD_WIN_LIKE
    char            localFmt[128];
#endif

    if (fmt == 0) {
        fmt = "%+";
    }
    if (timeptr == 0) {
        mprLocaltime(ctx, &tm, mprGetTime(ctx));
        timeptr = &tm;
    }
#if BLD_WIN_LIKE
{
    cchar   *cp;
    char    tz[80], *sign, *dp;
    long    timezone;
    int     len;
    /*
     *  Windows does not support %T or %D or %z
     */
    dp = localFmt;
    for (cp = fmt; *cp && dp < &localFmt[sizeof(localFmt) - 9]; ) {
        if (*cp == '%') {
            *dp++ = *cp++;
            if (*cp == 'T') {
                strcpy(dp, "H:%M:%S");
                dp += 7;
                cp++;
            } else if (*cp == 'D') {
                strcpy(dp, "m/%d/%y");
                dp += 7;
                cp++;
            } else if (*cp == 'z') {
                _get_timezone(&timezone);
                sign = (timezone >= 0) ? "-": "";
                if (timezone < 0) {
                    timezone = -timezone;
                }
                timezone /= 60;
                len = mprSprintf(tz, sizeof(tz), "%s%02d%02d", sign, timezone / 60, timezone % 60);
                if (&dp[len] >= &localFmt[sizeof(localFmt) - 9]) {
                    break;
                }
                mprStrcpy(--dp, len + 1, tz);
                dp += len;
                cp++;
            } else {
                *dp++ = *cp++;
            }
        } else {
            *dp++ = *cp++;
        }
    }
    *dp = '\0';
    fmt = localFmt;
}
#endif
    return (int) strftime(buf, bufsize, fmt, timeptr);
}


/*
 *  TODO - remove this routine
 *  Thread-safe RFC822 dates (Eg: "Fri, 07 Jan 2003 12:12:21 GMT")
 *      strftime(,, "%a, %d %b %Y %T %Z)
 */
int mprRfctime(MprCtx ctx, char *buf, int bufsize, const struct tm *timep)
{
#if HAS_STRFTIME || 1
    strftime(buf, bufsize, "%a, %d %b %Y %H:%M:%S %Z", timep);
#else
    char months[12][4] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep",
        "Oct", "Nov", "Dec"
    };

    char days[7][4] = {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    };

    char    *dayp, *monthp;
    int     year;

    if (bufsize < 30) {
        return MPR_ERR_WONT_FIT;
    }
    dayp = &days[timep->tm_wday][0];
    *buf++ = *dayp++;
    *buf++ = *dayp++;
    *buf++ = *dayp++;
    *buf++ = ',';
    *buf++ = ' ';

    *buf++ = timep->tm_mday / 10 + '0';
    *buf++ = timep->tm_mday % 10 + '0';
    *buf++ = ' ';

    monthp = &months[timep->tm_mon][0];
    *buf++ = *monthp++;
    *buf++ = *monthp++;
    *buf++ = *monthp++;
    *buf++ = ' ';

    year = 1900 + timep->tm_year;
    /* This routine isn't y10k ready. */
    *buf++ = year / 1000 + '0';
    *buf++ = year % 1000 / 100 + '0';
    *buf++ = year % 100 / 10 + '0';
    *buf++ = year % 10 + '0';
    *buf++ = ' ';

    *buf++ = timep->tm_hour / 10 + '0';
    *buf++ = timep->tm_hour % 10 + '0';
    *buf++ = ':';
    *buf++ = timep->tm_min / 10 + '0';
    *buf++ = timep->tm_min % 10 + '0';
    *buf++ = ':';
    *buf++ = timep->tm_sec / 10 + '0';
    *buf++ = timep->tm_sec % 10 + '0';
    *buf++ = ' ';

    *buf++ = 'G';
    *buf++ = 'M';
    *buf++ = 'T';
    *buf++ = 0;
#endif
    return 0;
}


/* TODO
- Issues month/day ordering
- Locale on day of week / month names
- HMS ordering
- DST
*/

static int timeSep = ':';


static int lookupSym(Mpr *mpr, cchar *token, int kind)
{
    TimeToken   *tt;

    if ((tt = (TimeToken*) mprLookupHash(mpr->timeTokens, token)) == 0) {
        return -1;
    }
    if (kind != (tt->value & TOKEN_MASK)) {
        return -1;
    }
    return tt->value & ~TOKEN_MASK;
}


static int getNum(Mpr *mpr, char **token, int sep)
{
    int     num;

    if (*token == 0) {
        return 0;
    }

    num = atoi(*token);
    *token = strchr(*token, sep);
    if (*token) {
        *token += 1;
    }
    return num;
}


static int getNumOrSym(Mpr *mpr, char **token, int sep, int kind, int *isAlpah)
{
    char    *cp;
    int     num;

    mprAssert(token && *token);

    if (*token == 0) {
        return 0;
    }
    if (isalpha((int) **token)) {
        *isAlpah = 1;
        cp = strchr(*token, sep);
        if (cp) {
            *cp++ = '\0';
        }
        num = lookupSym(mpr, *token, kind);
        *token = cp;
        return num;
    }
    num = atoi(*token);
    *token = strchr(*token, sep);
    if (*token) {
        *token += 1;
    }
    *isAlpah = 0;
    return num;
}


static bool allDigits(cchar *token)
{
    cchar   *cp;

    for (cp = token; *cp; cp++) {
        if (!isdigit((int) *cp)) {
            return 0;
        }
    }
    return 1;
} 


static void swapDayMonth(struct tm *tm)
{
    int     tmp;

    tmp = tm->tm_mday;
    tm->tm_mday = tm->tm_mon;
    tm->tm_mon = tmp;
}


static int firstDay(int year, int mon, int wday)
{
    struct tm tm;

    if (wday == -1) {
        return 1;
    }

    memset(&tm, 0, sizeof (struct tm));
    tm.tm_year = year;
    tm.tm_mon = mon;
    tm.tm_mday = 1;
    mktime(&tm);

    return (1 + (wday - tm.tm_wday + 7) % 7);
}


/*
 *  Parse the a date/time string
 */ 
int mprParseTime(MprCtx ctx, MprTime *time, cchar *dateString)
{
    Mpr         *mpr;
    TimeToken   *tt;
    struct tm   tm;
    char        *str, *next, *token, *cp, *sep;
    int         kind, hour, min, negate, value, value2, value3, alpha, alpha2, alpha3, dateSep, offset, zoneOffset;

    mpr = mprGetMpr(ctx);

    zoneOffset = offset = 0;
    sep = ", \t";
    cp = 0;
    next = 0;

    /*
     *  Set these mandatory values to -1 so we can tell if they are set to valid values
     *  WARNING: all the calculations use tm_year with origin 0, not 1900. It is fixed up below.
     */
    tm.tm_isdst = tm.tm_year = tm.tm_mon = tm.tm_mday = tm.tm_hour = tm.tm_sec = tm.tm_min = tm.tm_wday = -1;

    /*
     *  Set these to the correct defaults (wday and yday are not needed and ignored)
     */
    tm.tm_min = tm.tm_sec = tm.tm_yday = 0;
    tm.tm_isdst = -1;
    zoneOffset = mpr->timezone;

    str = mprStrdup(ctx, dateString);
    mprStrLower(str);

    token = mprStrTok(str, sep, &next);

    /*
     *  Smart date parser. Can handle the following formats: TODO
     */
    while (token && *token) {

        if (allDigits(token)) {
            /*
             *  Parse either day of month or year. Priority to day of month. Format: <29> Jan <15> <2009>
             */ 
            value = atoi(token);
            if (value > 32 || (tm.tm_mday >= 0 && tm.tm_year < 0)) {
                tm.tm_year = value;
            } else if (tm.tm_mday < 0) {
                tm.tm_mday = value;
            }

        } else if ((*token == '+') || (*token == '-') ||
                    ((strncmp(token, "gmt", 3) == 0 || strncmp(token, "utc", 3) == 0) &&
                    ((cp = strchr(&token[3], '+')) != 0 || (cp = strchr(&token[3], '-')) != 0))) {
            /*
             *  Timezone. Format: [GMT|UTC][+-]NN[:]NN
             */
            if (!isalpha((int) *token)) {
                cp = token;
            }
            negate = *cp == '-' ? -1 : 1;
            cp++;
            hour = getNum(mpr, &cp, timeSep);
            if (hour >= 100) {
                hour /= 100;
            }
            min = getNum(mpr, &cp, timeSep);
            zoneOffset = negate * (hour * 60 + min);

        } else if (isalpha((int) *token)) {
            if ((tt = (TimeToken*) mprLookupHash(mpr->timeTokens, token)) != 0) {
                kind = tt->value & TOKEN_MASK;
                value = tt->value & ~TOKEN_MASK; 
                switch (kind) {

                case TOKEN_DAY:
                    tm.tm_wday = value;
                    break;

                case TOKEN_MONTH:
                    tm.tm_mon = value;
                    break;

                case TOKEN_OFFSET:
                    /* Named timezones or symbolic names like: tomorrow, yesterday, next week ... */ 
                    offset += value;
                    break;

                case TOKEN_ZONE:
                    zoneOffset = value;
                    break;

                default:
                    /* Just ignore unknown values */
                    break;
                }
            }

        } else if ((cp = strchr(token, timeSep)) != 0 && isdigit((int) token[0])) {
            /*
             *  Time:  10:52[:23]
             *  Must not parse GMT-07:30
             */
            tm.tm_hour = getNum(mpr, &token, timeSep);
            tm.tm_min = getNum(mpr, &token, timeSep);
            tm.tm_sec = getNum(mpr, &token, timeSep);

        } else {
            dateSep = '/';
            if (strchr(token, dateSep) == 0) {
                dateSep = '-';
                if (strchr(token, dateSep) == 0) {
                    dateSep = '.';
                    if (strchr(token, dateSep) == 0) {
                        dateSep = 0;
                    }
                }
            }

            if (dateSep) {
                /*
                 *  Date:  07/28/2009, 07/28/08, Jan/28/2009, Jaunuary-28-2009, 28-jan-2009
                 *  Support order: dd/mm/yy, mm/dd/yy and yyyy/mm/dd
                 *  Support separators "/", ".", "-"
                 */
                value = getNumOrSym(mpr, &token, dateSep, TOKEN_MONTH, &alpha);
                value2 = getNumOrSym(mpr, &token, dateSep, TOKEN_MONTH, &alpha2);
                value3 = getNumOrSym(mpr, &token, dateSep, TOKEN_MONTH, &alpha3);

                if (value > 31) {
                    /* yy/mm/dd */
                    tm.tm_year = value;
                    tm.tm_mon = value2;
                    tm.tm_mday = value3;

                } else if (value > 12 || alpha2) {
                    /* 
                     *  dd/mm/yy 
                     *  Can't detect 01/02/03  This will be evaluated as Jan 2 2003 below.
                     */  
                    tm.tm_mday = value;
                    tm.tm_mon = value2;
                    tm.tm_year = value3;

                } else {
                    /*
                     *  The default to parse is mm/dd/yy unless the mm value is out of range
                     */
                    tm.tm_mon = value;
                    tm.tm_mday = value2;
                    tm.tm_year = value3;
                }
            }
        }
        token = mprStrTok(NULL, sep, &next);
    }

    /*
     *  Y2K fix and rebias
     */
    if (0 <= tm.tm_year && tm.tm_year < 100) {
        if (tm.tm_year < 50) {
            tm.tm_year += 2000;
        } else {
            tm.tm_year += 1900;
        }
    }    
    if (tm.tm_year >= 1900) {
        tm.tm_year -= 1900;
    }

    /*
     *  Convert back to origin 1 for months
     */
    tm.tm_mon--;

    validateTime(mpr, &tm);
    *time = mprMakeLocalTime(ctx, &tm);
    if (*time < 0) {
        return MPR_ERR_WONT_FIT;
    }
    *time -= ((MprTime) zoneOffset - mpr->timezone) * 60 * MPR_TICKS_PER_SEC;
    if (tm.tm_isdst) {
        *time += (MprTime) (60 * 60 * MPR_TICKS_PER_SEC);
    }
    return 0;
}


static void validateTime(MprCtx ctx, struct tm *tm)
{
    struct tm   current;

    /*
     *  Fix apparent day-mon-year ordering issues. Can't fix everything!
     */
    if ((12 <= tm->tm_mon && tm->tm_mon <= 31) && 0 <= tm->tm_mday && tm->tm_mday <= 11) {
        /*
         *  Looks like day month are swapped
         */
        swapDayMonth(tm);
    }


    if (tm->tm_year >= 0 && tm->tm_mon >= 0 && tm->tm_mday >= 0 && tm->tm_hour >= 0) {
        /*  Everything defined */
        return;
    }

    /*
     *  Use current time if missing
     */
    mprLocaltime(ctx, &current, mprGetTime(ctx));

    if (tm->tm_hour < 0 && tm->tm_min < 0 && tm->tm_sec < 0) {
        tm->tm_hour = current.tm_hour;
        tm->tm_min = current.tm_min;
        tm->tm_sec = current.tm_sec;
    }

    /*
     *  Get weekday, if before today then make next week
     */
    if (tm->tm_wday >= 0 && tm->tm_year == 0 && tm->tm_mon < 0 && tm->tm_mday < 0) {
        tm->tm_mday = current.tm_mday + (tm->tm_wday - current.tm_wday + 7) % 7;
        tm->tm_mon = current.tm_mon;
        tm->tm_year = current.tm_year;
    }

    /*
     *  Get month, if before this month then make next year
     */
    if (tm->tm_mon >= 0 && tm->tm_mon <= 11 && tm->tm_mday < 0) {
        if (tm->tm_year < 0) {
            tm->tm_year = current.tm_year + (((tm->tm_mon - current.tm_mon) < 0) ? 1 : 0);
        }
        tm->tm_mday = firstDay(tm->tm_year, tm->tm_mon, tm->tm_wday);
    }

    /*
     *  Get date, if before current time then make tomorrow
     */
    if (tm->tm_hour >= 0 && tm->tm_year < 0 && tm->tm_mon < 0 && tm->tm_mday < 0) {
        tm->tm_mday = current.tm_mday + ((tm->tm_hour - current.tm_hour) < 0 ? 1 : 0);
        tm->tm_mon = current.tm_mon;
        tm->tm_year = current.tm_year;
    }

    if (tm->tm_sec < 0) {
        tm->tm_sec = current.tm_sec;
    }
    if (tm->tm_min < 0) {
        tm->tm_min = current.tm_min;
    }
    if (tm->tm_hour < 0) {
        tm->tm_hour = current.tm_hour;
    }
    if (tm->tm_year < 0) {
        tm->tm_year = current.tm_year;
    }
    if (tm->tm_mon < 0) {
        tm->tm_mon = current.tm_mon;
    }
}


/*
 *  Compatibility for windows and brew
 */
#if BLD_WIN_LIKE || BREW || VXWORKS
static int gettimeofday(struct timeval *tv, struct timezone *tz)
{
#if BLD_WIN_LIKE
    FILETIME        fileTime;
    MprTime         now;
    static int      tzOnce;

    if (NULL != tv) {
        GetSystemTimeAsFileTime(&fileTime);

        now = ((((MprTime) fileTime.dwHighDateTime) << BITS(uint)) + ((MprTime) fileTime.dwLowDateTime));

        /*
         *  Convert from 100-nanosec units to microsectonds
         */
        now /= 10;

        now -= GENESIS;
        tv->tv_sec = (long) (now / 1000000);
        tv->tv_usec = (long) (now % 1000000);
    }

    if (NULL != tz) {
        if (!tzOnce) {
            _tzset();
            tzOnce++;
        }
        tz->tz_minuteswest = _timezone / 60;
        tz->tz_dsttime = _daylight;
    }
#elif BREW
    Mpr     *mpr;
    uint    upTime;

    mpr = mprGetMpr(ctx);

    upTime = ISHELL_GetUpTimeMS(mpr->shell);

    /*
     *  TODO -- Must convert to be sec since Jan 1 1970
     */
    return (MprTime) (upTime / 1000) + upTime % 1000;
#elif VXWORKS
    struct tm       tm;
    struct timespec now;
    time_t          t;
    char            *tze, *p;
    int rc;

    if ((rc = clock_gettime(CLOCK_REALTIME, &now)) == 0) {
        tv->tv_sec  = now.tv_sec;
        tv->tv_usec = (now.tv_nsec + 500) / 1000;
        if ((tze = getenv("TIMEZONE")) != 0) {
            if ((p = strchr(tze, ':')) != 0) {
                if ((p = strchr(tze, ':')) != 0) {
                    tz->tz_minuteswest = mprAtoi(++p, 10);
                }
            }
            t = tickGet();
            tz->tz_dsttime = (localtime_r(&t, &tm) == 0) ? tm.tm_isdst : 0;
        }
    }
    return rc;
#endif
    return 0;
}
#endif

/*
*  High resolution timer
*/
#if BLD_DEBUG && UNUSED
#if BLD_UNIX_LIKE
#if MPR_CPU_IX86
inline MprTime mprGetHiResTime()
{
    MprTime  now;
    __asm__ __volatile__ ("rdtsc" : "=A" (now));
    return now;
}
#endif /* MPR_CPU_IX86 */
#elif BLD_WIN_LIKE
inline MprTime mprGetHiResTime()
{
    MprTime  now;
    QueryPerformanceCounter((LARGE_INTEGER*) &now);
    return now;
}
#endif /* BLD_WIN_LIKE */
#endif /* BLD_DEBUG */


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
/************************************************************************/
/*
 *  End of file "../mprTime.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../mprUnicode.c"
 */
/************************************************************************/

/**
 *  mprUnicode.c - Unicode 
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

#if UNUSED


/*
 *  Allocate a new (empty) unicode string
 */
MprUs *mprAllocUs(MprCtx ctx)
{
    mprAssert(ctx);

    return mprAllocObjZeroed(ctx, MprUs);
}



/*
 *  Grow the string buffer for a unicode string
 */
static int growUs(MprUs *us, int len)
{
    mprAssert(us);
    mprAssert(len >= 0);

    if (len < us->length) {
        return 0;
    }

    //  TODO - ensure slab allocation. What about a reasonable growth increment?
    us->str = mprRealloc(us, us->str, len);
    if (us->str == 0) {
        return MPR_ERR_NO_MEMORY;
    }
    us->length = len;
    return 0;
}



/*
 *  Convert a ASCII string to UTF-8/16
 */
static int memToUs(MprUs *us, const uchar *str, int len)
{
    MprUsData   *up;
    cchar       *cp;

    mprAssert(us);
    mprAssert(str);

    if (len > us->length) {
        if (growUs(us, len) < 0) {
            return MPR_ERR_NO_MEMORY;
        }
    }
    us->length = len;

#if BLD_FEATURE_UTF16
    cp = (cchar*) str;
    up = us->str;
    while (*cp) {
        *up++ = *cp++;
    }
#else
    memcpy((void*) us->str, str, len);
#endif

    return 0;
}



/*
 *  Convert a C string to a newly allocated unicode string
 */
MprUs *mprStrToUs(MprCtx ctx, cchar *str)
{
    MprUs   *us;
    int     len;

    mprAssert(ctx);
    mprAssert(str);

    us = mprAllocUs(ctx);
    if (us == 0) {
        return 0;
    }

    if (str == 0) {
        str = "";
    }

    len = strlen(str);

    if (memToUs(us, (const uchar*) str, len) < 0) {
        return 0;
    }
    
    return us;
}



/*
 *  Convert a memory buffer to a newly allocated unicode string
 */
MprUs *mprMemToUs(MprCtx ctx, const uchar *buf, int len)
{
    MprUs   *us;

    mprAssert(ctx);
    mprAssert(buf);

    us = mprAllocUs(ctx);
    if (us == 0) {
        return 0;
    }

    if (memToUs(us, buf, len) < 0) {
        return 0;
    }
    
    return us;
}



/*
 *  Convert a unicode string newly allocated C string
 */
char *mprUsToStr(MprUs *us)
{
    char    *str, *cp;

    mprAssert(us);

    str = cp = mprAlloc(us, us->length + 1);
    if (cp == 0) {
        return 0;
    }

#if BLD_FEATURE_UTF16
{
    MprUsData   *up;
    int         i;

    up = us->str;
    for (i = 0; i < us->length; i++) {
        cp[i] = up[i];
    }
}
#else
    mprStrcpy(cp, us->length, us->str);
#endif
    return str;
}



/*
 *  Copy one unicode string to another. No allocation
 */
static void copyUs(MprUs *dest, MprUs *src)
{
    mprAssert(dest);
    mprAssert(src);
    mprAssert(dest->length <= src->length);
    mprAssert(dest->str);
    mprAssert(src->str);

    memcpy(dest->str, src->str, src->length * sizeof(MprUsData));
    dest->length = src->length;
}



/*
 *  Copy one unicode string to another. Grow the destination unicode string buffer as required.
 */
int mprCopyUs(MprUs *dest, MprUs *src)
{
    mprAssert(dest);
    mprAssert(src);

    dest->length = src->length;

    if (src->length > dest->length) {
        if (growUs(dest, src->length) < 0) {
            return MPR_ERR_NO_MEMORY;
        }
    }

    copyUs(dest, src);

    return 0;
}



/*
 *  Catenate a unicode string onto another.
 */
int mprCatUs(MprUs *dest, MprUs *src)
{
    int     len;

    len = dest->length + src->length;
    if (growUs(dest, len) < 0) {
        return MPR_ERR_NO_MEMORY;
    }

    memcpy(&dest->str[dest->length], src->str, src->length * sizeof(MprUsData));
    dest->length += src->length;

    return 0;
}



/*
 *  Catenate a set of unicode string arguments onto another.
 */
int mprCatUsArgs(MprUs *dest, MprUs *src, ...)
{
    va_list     args;
    MprUs       *us;
    int         len;

    va_start(args, src);

    len = 0;
    us = src;
    for (us = src; us; ) {
        us = va_arg(args, MprUs*);
        len += us->length;
    }

    if (growUs(dest, len) < 0) {
        return MPR_ERR_NO_MEMORY;
    }

    va_start(args, src);
    for (us = src; us; ) {
        us = va_arg(args, MprUs*);
        
        memcpy(&dest->str[dest->length], src->str, src->length * sizeof(MprUsData));
        dest->length += src->length;
    }

    va_end(args);
    
    return 0;
}



/*
 *  Duplicate a unicode string by allocating a new unicode string and copying the source data.
 */
MprUs *mprDupUs(MprUs *src)
{
    MprUs   *dest;

    dest = mprAllocUs(src);
    if (dest == 0) {
        return 0;
    }

    copyUs(dest, src);

    return dest;
}




/*
 *  Copy a C string into an existing unicode string.
 */
int mprCopyStrToUs(MprUs *dest, cchar *str)
{
    mprAssert(dest);
    mprAssert(str);

    return memToUs(dest, (const uchar*) str, strlen(str));
}



/*
 *  Return the lenght of a unicoded string.
 */
int mprGetUsLength(MprUs *us)
{
    mprAssert(us);

    return us->length;
}




/*
 *  Return the index in a unicode string of a given unicode character code. Return -1 if not found.
 */
int mprContainsChar(MprUs *us, int charPat)
{
    int     i;

    mprAssert(us);

    for (i = 0; i < us->length; i++) {
        if (us->str[i] == charPat) {
            return i;
        }
    }
    return -1;
}



/*
 *  Return TRUE if a unicode string contains a given unicode string.
 */
int mprContainsUs(MprUs *us, MprUs *pat)
{
    int     i, j;

    mprAssert(us);
    mprAssert(pat);
    mprAssert(pat->str);

    if (pat == 0 || pat->str == 0) {
        return 0;
    }
    
    for (i = 0; i < us->length; i++) {
        for (j = 0; j < pat->length; j++) {
            if (us->str[i] != pat->str[j]) {
                break;
            }
        }
        if (j == pat->length) {
            return i;
        }
    }
    return -1;
}



/*
 *  Return TRUE if a unicode string contains a given unicode string after doing a case insensitive comparison.
 */
int mprContainsCaselessUs(MprUs *us, MprUs *pat)
{
    int     i, j;

    mprAssert(us);
    mprAssert(pat);
    mprAssert(pat->str);

    for (i = 0; i < us->length; i++) {
        for (j = 0; j < pat->length; j++) {
            if (tolower(us->str[i]) != tolower(pat->str[j])) {
                break;
            }
        }
        if (j == pat->length) {
            return i;
        }
    }
    return -1;
}



/*
 *  Return TRUE if a unicode string contains a given C string.
 */
int mprContainsStr(MprUs *us, cchar *pat)
{
    int     i, j, len;

    mprAssert(us);
    mprAssert(pat);

    if (pat == 0 || *pat == '\0') {
        return 0;
    }
    
    len = strlen(pat);
    
    for (i = 0; i < us->length; i++) {
        for (j = 0; j < len; j++) {
            if (us->str[i] != pat[j]) {
                break;
            }
        }
        if (j == len) {
            return i;
        }
    }
    return -1;
}



#if FUTURE
int mprContainsPattern(MprUs *us, MprRegex *pat)
{
    return 0;
}
#endif




MprUs *mprTrimUs(MprUs *us, MprUs *pat)
{
    //  TODO
    return 0;
}



int mprTruncateUs(MprUs *us, int len)
{
    mprAssert(us);

    mprAssert(us->length >= len);

    if (us->length < len) {
        return MPR_ERR_WONT_FIT;
    }

    us->length = len;
    return 0;
}



MprUs *mprSubUs(MprUs *src, int start, int len)
{
    MprUs   *dest;

    mprAssert(src);
    mprAssert(start >= 0);
    mprAssert(len > 0);
    mprAssert((start + len) <= src->length);

    if ((start + len) > src->length) {
        return 0;
    }

    dest = mprAllocUs(src);
    if (dest == 0) {
        return 0;
    }

    if (growUs(dest, len) < 0) {
        mprFree(dest);
        return 0;
    }
    memcpy(dest->str, &src->str[start], len * sizeof(MprUsData));
    dest->length = len;

    return dest;
}







void mprUsToLower(MprUs *us)
{
    int     i;

    mprAssert(us);
    mprAssert(us->str);

    for (i = 0; i < us->length; i++) {
        us->str[i] = tolower(us->str[i]);
    }
}



void mprUsToUpper(MprUs *us)
{
    int     i;

    mprAssert(us);
    mprAssert(us->str);

    for (i = 0; i < us->length; i++) {
        us->str[i] = toupper(us->str[i]);
    }
}




MprUs *mprTokenizeUs(MprUs *us, MprUs *delim, int *last)
{
    return 0;
}




int mprFormatUs(MprUs *us, int maxSize, cchar *fmt, ...)
{
    return 0;
}



int mprScanUs(MprUs *us, cchar *fmt, ...)
{
    return 0;
}



#else
void __mprDummyUnicode() {}
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
/************************************************************************/
/*
 *  End of file "../mprUnicode.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../mprUrl.c"
 */
/************************************************************************/

/**
 *  mprUrl.c - Url manipulation routines
 *
 *  Miscellaneous routines to parse and enscape URLs.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



/*
 *  Character escape/descape matching codes. Generated by charGen.
 */
static uchar charMatch[256] = {
     0, 4, 4, 4, 4, 4, 4, 4, 4, 4, 6, 4, 4, 4, 4, 4,
     4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
     4, 4, 7, 5, 6, 4, 7, 6, 7, 7, 2, 0, 4, 0, 0, 4,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 6, 7, 4, 7, 6,
     4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 6, 6, 6, 0,
     2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 6, 6, 6, 4,
     4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
     4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
     4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
     4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
     4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
     4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
     4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
     4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 
};

/*
 *  Max size of the port specification in a URL
 */
#define MAX_PORT_LEN 8

/*
 *  Parse a complete URI. This accepts full URIs with schemes (http:) and partial URLs
 */
MprUri *mprParseUri(MprCtx ctx, cchar *uri)
{
    MprUri  *up;
    char    *tok, *cp, *last_delim, *hostbuf, *urlTok;
    int     c, len, ulen, http;

    mprAssert(uri && *uri);

    up = mprAllocObj(ctx, MprUri);
    if (up == 0) {
        return 0;
    }

    /*
     *  Allocate a single buffer to hold all the cracked fields.
     */
    ulen = (int) strlen(uri);
    len = ulen * 2 + 3;

    up->originalUri = mprStrdup(up, uri);
    up->parsedUriBuf = (char*) mprAlloc(up, len * sizeof(char));

    hostbuf = &up->parsedUriBuf[ulen+1];
    strcpy(up->parsedUriBuf, uri);
    urlTok = up->parsedUriBuf;

    /*
     *  Defaults for missing URL fields
     */
    up->url = "/";
    up->scheme = "http";
    up->host = "localhost";
    up->port = 80;
    up->query = 0;
    up->ext = 0;
    up->secure = 0;

    http = 0;
    tok = 0;

    if (strncmp(urlTok, "https://", 8) == 0) {
        up->secure = 1;
        up->port = 443;
        tok = &urlTok[8];
        http++;

    } else if (strncmp(urlTok, "http://", 7) == 0) {
        tok = &urlTok[7];
        http++;
    }

    if (http) {
        up->scheme = urlTok;
        up->host = tok;
        tok[-3] = '\0';

        for (cp = tok; *cp; cp++) {
            if (*cp == '/') {
                break;
            }
            if (*cp == ':') {
                *cp++ = '\0';
                up->port = atoi(cp);
                tok = cp;
            }
        }
        if ((cp = strchr(tok, '/')) != NULL) {
            c = *cp;
            *cp = '\0';
            mprStrcpy(hostbuf, ulen + 1, up->host);
            *cp = c;
            up->host = hostbuf;
            up->url = cp;
            tok = cp;
        }

    } else {
        up->url = urlTok;
        tok = urlTok;
    }

    /*
     *  Split off the query string.
     */
    if ((cp = strchr(tok, '?')) != NULL) {
        *cp++ = '\0';
        up->query = cp;
        up->url = tok;
        tok = up->query;
    }

    /*
     *  Split off fragment identifier.
     */
    if ((cp = strchr(tok, '#')) != NULL) {
        *cp++ = '\0';
        if (*up->query == 0) {
            up->url = tok;
        }
    }

    /*
     *  FUTURE -- this logic could be improved
     */
    if ((cp = strrchr(up->url, '.')) != NULL) {
        if ((last_delim = strrchr(up->url, '/')) != NULL) {
            if (last_delim <= cp) {
                up->ext = cp + 1;
#if BLD_WIN_LIKE
                mprStrLower(up->ext);
#endif
            }
        } else {
            up->ext = cp + 1;
#if BLD_WIN_LIKE
            mprStrLower(up->ext);
#endif
        }
    } else {
        len = (int) strlen(up->url);
    }

    return up;
}



/*
 *  Format a fully qualified URI
 */
char *mprFormatUri(MprCtx ctx, cchar *scheme, cchar *host, int port, cchar *path, cchar *query)
{
    char    portBuf[16], *uri;
    cchar   *portDelim, *pathDelim, *queryDelim;
    int     defaultPort, len;

    len = 0;

    if (scheme == 0 || *scheme == '\0') {
        scheme = "http";
    }
    len += (int) strlen(scheme) + 3;                            /* Add 3 for "://" */

    defaultPort = (strcmp(scheme, "http") == 0) ? 80 : 443;

    if (host == 0 || *host == '\0') {
        host = "localhost";
    }
    len += (int) strlen(host);

    if (port != defaultPort) {
        mprItoa(portBuf, sizeof(portBuf), port, 10);
        portDelim = ":";
    } else {
        portBuf[0] = '\0';
        portDelim = "";
    }
    len += (int) strlen(portBuf) + (int) strlen(portDelim);

    if (path) {
        pathDelim = (*path == '/') ? "" :  "/";
    } else {
        pathDelim = "/";
        path = "";
    }
    len += (int) strlen(path) + (int) strlen(pathDelim);

    if (query && *query) {
        queryDelim = "?";
    } else {
        queryDelim = query = "";
    }
    len += (int) strlen(query) + (int) strlen(queryDelim);
    len += 1;                                               /* Add one for the null */

    uri = mprAlloc(ctx, len);
    if (uri == 0) {
        return 0;
    }

    if (mprAllocSprintf(ctx, &uri, len, "%s://%s%s%s%s%s%s%s", scheme, host, portDelim, portBuf,
            pathDelim, path, queryDelim, query) < 0) {
        return 0;
    }
    return uri;
}



/*
 *  Url encode by encoding special characters with hex equivalents.
 */
char *mprUrlEncode(char *buf, int len, cchar *inbuf)
{
    static cchar        hexTable[] = "0123456789abcdef";
    uchar               c;
    cchar               *ip;
    char                *op, *endp;

    mprAssert(buf);
    mprAssert(inbuf);
    mprAssert(buf != inbuf);

    ip = inbuf;
    op = buf;

    endp = &buf[len - 4];
    while ((c = (uchar) (*inbuf++)) != 0 && op < endp) {
        if (c == ' ') {
            *op++ = '+';
        } else if (charMatch[c] & MPR_HTTP_ESCAPE_URL) {
            *op++ = '%';
            *op++ = hexTable[c >> 4];
            *op++ = hexTable[c & 0xf];
        } else {
            *op++ = c;
        }
    }
    *op = '\0';
    return op;
}



/*
 *  Decode a string using URL encoding. Can work insitu (ie. buf == inbuf).
 */
char *mprUrlDecode(char *buf, int len, cchar *inbuf)
{
    cchar   *ip;
    char    *op;
    int     num, i, c;

    mprAssert(buf);
    mprAssert(inbuf);

    for (op = buf, ip = inbuf; *ip && len > 0; ip++, op++) {
        if (*ip == '+') {
            *op = ' ';

        } else if (*ip == '%' && isxdigit((int) ip[1]) && isxdigit((int) ip[2])) {
            ip++;
            num = 0;
            for (i = 0; i < 2; i++, ip++) {
                c = tolower((int) *ip);
                if (c >= 'a' && c <= 'f') {
                    num = (num * 16) + 10 + c - 'a';
                } else if (c >= '0' && c <= '9') {
                    num = (num * 16) + c - '0';
                } else {
                    /* Bad chars in URL */
                    return 0;
                }
            }
            *op = (char) num;
            ip--;

        } else {
            *op = *ip;
        }
        len--;
    }
    *op = '\0';
    return buf;
}



/*
 *  Escape a shell command
 */
//  TODO - should alloc a response buffer
char *mprEscapeCmd(char *buf, int len, cchar *cmd, int escChar)
{
    uchar   c;
    char    *op, *endp;

    mprAssert(buf);
    mprAssert(cmd);
    mprAssert(buf != cmd);

    if (escChar == 0) {
        escChar = '\\';
    }
    op = buf;
    endp = &buf[len - 3];
    while ((c = (uchar) *cmd++) != 0 && op < endp) {
#if BLD_WIN_LIKE
        if ((c == '\r' || c == '\n') && *cmd != '\0') {
            c = ' ';
            continue;
        }
#endif
        if (charMatch[c] & MPR_HTTP_ESCAPE_SHELL) {
            *op++ = escChar;
        }
        *op++ = c;
    }
    *op = '\0';
    return op;
}



/*
 *  Escape HTML to escape defined characters (prevent cross-site scripting)
 */
//  TODO - should alloc a response buffer
char *mprEscapeHtml(char *buf, int buflen, cchar *html)
{
    char    *bp, *endp;

    /*
     *  Leave room for the biggest expansion
     */
    bp = buf;
    endp = &buf[buflen - 7];
    while (*html != '\0' && bp < endp) {
        if (charMatch[(uchar) *html] & MPR_HTTP_ESCAPE_HTML) {
            if (*html == '&') {
                strcpy(bp, "&amp;");
                bp += 5;
            } else if (*html == '<') {
                strcpy(bp, "&lt;");
                bp += 4;
            } else if (*html == '>') {
                strcpy(bp, "&gt;");
                bp += 4;
            } else if (*html == '#') {
                strcpy(bp, "&#35;");
                bp += 5;
            } else if (*html == '(') {
                strcpy(bp, "&#40;");
                bp += 5;
            } else if (*html == ')') {
                strcpy(bp, "&#41;");
                bp += 5;
            } else if (*html == '"') {
                strcpy(bp, "&quot;");
                bp += 5;
            } else {
                mprAssert(0);
            }
            html++;
        } else {
            *bp++ = *html++;
        }
    }
    *bp = '\0';
    return buf;
}



/*
 *  Validate a Url. The passed in url is modified in-situ.
 *
 *  WARNING: this code will not fully validate against certain Windows 95/98/Me bugs. Don't use this code in these
 *  operating systems without modifying this code to remove "con", "nul", "aux", "clock$" and "config$" in either
 *  case from the URI. The MprFileSystem::stat() will perform these checks to determine if a file is a device file.
 */
char *mprValidateUrl(char *url)
{
    char    *sp, *dp, *xp, *dot;

    /*
     *  Remove multiple path separators and map '\\' to '/' for windows
     */
    sp = dp = url;
    while (*sp) {
#if BLD_WIN_LIKE
        if (*sp == '\\') {
            *sp = '/';
        }
#endif
        if (sp[0] == '/' && sp[1] == '/') {
            sp++;
        } else {
            *dp++ = *sp++;
        }
    }
    *dp = '\0';

    dot = strchr(url, '.');
    if (dot == 0) {
        return url;
    }


    /*
     *  Per RFC 1808, remove "./" segments
     */
    dp = dot;
    for (sp = dot; *sp; ) {
        if (*sp == '.' && sp[1] == '/' && (sp == url || sp[-1] == '/')) {
            sp += 2;
        } else {
            *dp++ = *sp++;
        }
    }
    *dp = '\0';


    /*
     *  Remove trailing "."
     */
    if ((dp == &url[1] && url[0] == '.') ||
        (dp > &url[1] && dp[-1] == '.' && dp[-2] == '/')) {
        *--dp = '\0';
    }


    /*
     *  Remove "../"
     */
    for (sp = dot; *sp; ) {
        if (*sp == '.' && sp[1] == '.' && sp[2] == '/' &&
            (sp == url || sp[-1] == '/')) {
            xp = sp + 3;
            sp -= 2;
            if (sp < url) {
                sp = url;
            } else {
                while (sp >= url && *sp != '/') {
                    sp--;
                }
                sp++;
            }
            dp = sp;
            while ((*dp++ = *xp) != 0) {
                xp++;
            }
        } else {
            sp++;
        }
    }
    *dp = '\0';


    /*
     *  Remove trailing "/.."
     */
    if (sp == &url[2] && *url == '.' && url[1] == '.') {
        *url = '\0';
    } else {
        if (sp > &url[2] && sp[-1] == '.' && sp[-2] == '.' &&
                sp[-3] == '/') {
            sp -= 4;
            if (sp < url) {
                sp = url;
            } else {
                while (sp >= url && *sp != '/') {
                    sp--;
                }
                sp++;
            }
            *sp = '\0';
        }
    }
#if BLD_WIN_LIKE
    if (*url != '\0') {
        char    *cp;
        /*
         *  There was some extra URI past the matching alias prefix portion.  Windows will ignore trailing "."
         *  and " ". We must reject here as the URL probably won't match due to the trailing character and the
         *  copyHandler will return the unprocessed content to the user. Bad.
         */
        cp = &url[strlen(url) - 1];
        while (cp >= url) {
            if (*cp == '.' || *cp == ' ') {
                *cp-- = '\0';
            } else {
                break;
            }
        }
    }
#endif
    return url;
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
/************************************************************************/
/*
 *  End of file "../mprUrl.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../mprWait.c"
 */
/************************************************************************/

/**
 *  mprWait.c - Wait for I/O service.
 *
 *  This module provides wait management for sockets and other file descriptors and allows users to create wait
 *  handlers which will be called when wait events are detected. Multiple backends (one at a time) are supported.
 *  The only current backend is mprSelectWait.c
 *
 *  This module is thread-safe.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */




static int  serviceDestructor(MprWaitService *ss);
static int  handlerDestructor(MprWaitHandler *wp);

/*
 *  Initialize the service
 */
MprWaitService *mprCreateWaitService(Mpr *mpr)
{
    MprWaitService  *ws;

    ws = mprAllocObjWithDestructorZeroed(mpr, MprWaitService, serviceDestructor);
    if (ws == 0) {
        return 0;
    }

    ws->flags = 0;
    ws->rebuildMasks = 0;
    ws->listGeneration = 0;
    ws->maskGeneration = 0;
    ws->lastMaskGeneration = -1;

    ws->list = mprCreateList(ws);

#if BLD_WIN_LIKE
    ws->socketMessage = MPR_SOCKET_MESSAGE;
#endif

#if BLD_FEATURE_MULTITHREAD
    ws->mutex = mprCreateLock(ws);
#endif

    mprInitSelectWait(ws);
    return ws;
}


/*
 *  Destroy the wait service
 */
static int serviceDestructor(MprWaitService *ws)
{
    return 0;
}


/*
 *  Start the wait service.
 */
int mprStartWaitService(MprWaitService *ws)
{
#if !BLD_FEATURE_MULTITHREAD
#if BLD_WIN_LIKE
    if (mprInitWindow(ws) < 0) {
        return MPR_ERR_CANT_OPEN;
    }
#endif
#if BLD_FEATURE_MULTITHREAD && UNUSED
    if (mprOpenWaitPort(ws) < 0) {
        return MPR_ERR_CANT_OPEN;
    }
#endif
#endif
    return 0;
}


/*
 *  Stop the wait service. Must be idempotent.
 */
int mprStopWaitService(MprWaitService *ws)
{
    return 0;
}


/*
 *  Add a new handler
 */

int mprInsertWaitHandler(MprWaitService *ws, MprWaitHandler *wp)
{
    mprAssert(ws);
    mprAssert(wp);

    if (mprGetListCount(ws->list) == FD_SETSIZE) {
        mprError(ws, "io: Too many io handlers: %d\n", FD_SETSIZE);
        return MPR_ERR_TOO_MANY;
    }

    mprLock(ws->mutex);
    if (mprAddItem(ws->list, wp) < 0) {
        mprUnlock(ws->mutex);
        return MPR_ERR_NO_MEMORY;
    }
    ws->listGeneration++;
    ws->maskGeneration++;
    mprUnlock(ws->mutex);

    mprAwakenWaitService(ws);

    return 0;
}


/*
 *  Remove a handler
 */
void mprRemoveWaitHandler(MprWaitService *ws, MprWaitHandler *wp)
{
    mprLock(ws->mutex);

    mprRemoveItem(ws->list, wp);

    ws->listGeneration++;
    ws->maskGeneration++;
    ws->rebuildMasks++;

    mprUnlock(ws->mutex);

    mprAwakenWaitService(ws);
}


/*
 *  Modify a handler
 */
int mprModifyWaitHandler(MprWaitService *ws, MprWaitHandler *wp, bool wakeUp)
{
    mprLock(ws->mutex);
    ws->maskGeneration++;
    mprUnlock(ws->mutex);

    if (wakeUp) {
        mprAwakenWaitService(ws);
    }
    return 0;
}


/*
 *  Create a handler. Priority is only observed when multi-threaded.
 */
MprWaitHandler *mprCreateWaitHandler(MprCtx ctx, int fd, int mask, MprWaitProc proc, void *data, int pri, int flags)
{
    MprWaitService  *ws;
    MprWaitHandler  *wp;

    mprAssert(fd >= 0);

    ws = mprGetMpr(ctx)->waitService;

    wp = mprAllocObjWithDestructorZeroed(ws, MprWaitHandler, handlerDestructor);
    if (wp == 0) {
        return 0;
    }

#if BLD_UNIX_LIKE
    if (fd >= FD_SETSIZE) {
        mprError(ws, "File descriptor %d exceeds max io of %d", fd, FD_SETSIZE);
    }
#endif

    if (pri == 0) {
        pri = MPR_NORMAL_PRIORITY;
    }

    wp->fd              = fd;
    wp->proc            = proc;
    wp->flags           = flags;
    wp->handlerData     = data;
    wp->disableMask     = -1;
    wp->waitService     = ws;

#if BLD_FEATURE_MULTITHREAD
    wp->priority        = pri;
    wp->threadEvent     = 0;
#endif

    wp->desiredMask = 0;

    mprInsertWaitHandler(ws, wp);
    mprSetWaitInterest(wp, mask);

    return wp;
}


/*
 *  Wait Handler Destructor. Called from mprFree.
 */
static int handlerDestructor(MprWaitHandler *wp)
{
    mprRemoveWaitHandler(wp->waitService, wp);
    return 0;
}


/*
 *  Called by the wait backend and indirectly via a pool thread event
 */
void mprInvokeWaitCallback(MprWaitHandler *wp, void *poolThread)
{
    MprWaitService      *ws;

    ws = wp->waitService;

#if BLD_FEATURE_MULTITHREAD
    if (poolThread == 0 && mprGetMaxPoolThreads(ws) > 0) {
        /*
         *  Recall the callback via the thread pool
         */
        mprStartPoolThread(wp, (MprPoolProc) mprInvokeWaitCallback, (void*) wp, MPR_REQUEST_PRIORITY);
        return;
    }
#endif

    (*wp->proc)((MprWaitHandler*) wp->handlerData, wp->presentMask, (poolThread != 0));

    /*
     *  WARNING: wp may not exist here anymore. Often socket handlers will free the socket and handler in the callback.
     */
}


void mprSetWaitCallback(MprWaitHandler *wp, MprWaitProc newProc, int mask)
{
    mprLock(wp->waitService->mutex);
    wp->proc = newProc;
    mprSetWaitInterest(wp, mask);
    mprUnlock(wp->waitService->mutex);
}


#if BLD_FEATURE_MULTITHREAD
void mprSetWaitServiceThread(MprWaitService *ws, MprThread *thread)
{
    ws->serviceThread = thread;
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
/************************************************************************/
/*
 *  End of file "../mprWait.c"
 */
/************************************************************************/



/************************************************************************/
/*
 *  Start of file "../mprXml.c"
 */
/************************************************************************/

/**
 *  mprXml.c - A simple SAX style XML parser
 *
 *  This is a recursive descent parser for XML text files. It is a one-pass simple parser that invokes a user 
 *  supplied callback for key tokens in the XML file. The user supplies a read function so that XML files can 
 *  be parsed from disk or in-memory. 
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */



#if BLD_FEATURE_XML

static int       parseNext(MprXml *xp, int state);
static MprXmlToken getToken(MprXml *xp, int state);
static int       getNextChar(MprXml *xp);
static int       scanFor(MprXml *xp, char *str);
static int       putLastChar(MprXml *xp, int c);
static void      xmlError(MprXml *xp, char *fmt, ...);
static void      trimToken(MprXml *xp);


MprXml *mprXmlOpen(MprCtx ctx, int initialSize, int maxSize)
{
    MprXml  *xp;

    xp = mprAllocObjZeroed(ctx, MprXml);
    
    xp->inBuf = mprCreateBuf(xp, MPR_XML_BUFSIZE, MPR_XML_BUFSIZE);
    xp->tokBuf = mprCreateBuf(xp, initialSize, maxSize);

    return xp;
}



void mprXmlSetParserHandler(MprXml *xp, MprXmlHandler h)
{
    mprAssert(xp);

    xp->handler = h;
}



void mprXmlSetInputStream(MprXml *xp, MprXmlInputStream s, void *arg)
{
    mprAssert(xp);

    xp->readFn = s;
    xp->inputArg = arg;
}



/*
 *  Set the parse arg
 */ 
void mprXmlSetParseArg(MprXml *xp, void *parseArg)
{
    mprAssert(xp);

    xp->parseArg = parseArg;
}



/*
 *  Set the parse arg
 */ 
void *mprXmlGetParseArg(MprXml *xp)
{
    mprAssert(xp);

    return xp->parseArg;
}



/*
 *  Parse an XML file. Return 0 for success, -1 for error.
 */ 
int mprXmlParse(MprXml *xp)
{
    mprAssert(xp);

    return parseNext(xp, MPR_XML_BEGIN);
}



/*
 *  XML recursive descent parser. Return -1 for errors, 0 for EOF and 1 if there is still more data to parse.
 */
static int parseNext(MprXml *xp, int state)
{
    MprXmlHandler   handler;
    MprXmlToken     token;
    MprBuf          *tokBuf;
    char            *tname, *aname;
    int             rc;

    mprAssert(state >= 0);

    tokBuf = xp->tokBuf;
    handler = xp->handler;
    tname = aname = 0;
    rc = 0;
    
    /*
     *  In this parse loop, the state is never assigned EOF or ERR. In such cases we always return EOF or ERR.
     */
    while (1) {

        token = getToken(xp, state);

        if (token == MPR_XMLTOK_TOO_BIG) {
            xmlError(xp, "XML token is too big");
            goto err;
        }

        switch (state) {
        case MPR_XML_BEGIN:     /* ------------------------------------------ */
            /*
             *  Expect to get an element, comment or processing instruction 
             */
            switch (token) {
            case MPR_XMLTOK_EOF:
                goto exit;

            case MPR_XMLTOK_LS:
                /*
                 *  Recurse to handle the new element, comment etc.
                 */
                rc = parseNext(xp, MPR_XML_AFTER_LS);
                if (rc < 0) {
                    goto exit;
                }
                break;

            default:
                xmlError(xp, "Syntax error");
                goto err;
            }
            break;

        case MPR_XML_AFTER_LS: /* ------------------------------------------ */
            switch (token) {
            case MPR_XMLTOK_COMMENT:
                state = MPR_XML_COMMENT;
                rc = (*handler)(xp, state, "!--", 0, mprGetBufStart(tokBuf));
                if (rc < 0) {
                    goto err;
                }
                rc = 1;
                goto exit;

            case MPR_XMLTOK_CDATA:
                state = MPR_XML_CDATA;
                rc = (*handler)(xp, state, "!--", 0, mprGetBufStart(tokBuf));
                if (rc < 0) {
                    goto err;
                }
                rc = 1;
                goto exit;

            case MPR_XMLTOK_INSTRUCTIONS:
                /* Just ignore processing instructions */
                rc = 1;
                goto exit;

            case MPR_XMLTOK_TEXT:
                state = MPR_XML_NEW_ELT;
                tname = mprStrdup(xp, mprGetBufStart(tokBuf));
                if (tname == 0) {
                    rc = MPR_ERR_NO_MEMORY;
                    goto exit;
                }
                rc = (*handler)(xp, state, tname, 0, 0);
                if (rc < 0) {
                    goto err;
                }
                break;

            default:
                xmlError(xp, "Syntax error");
                goto err;
            }
            break;

        case MPR_XML_NEW_ELT:   /* ------------------------------------------ */
            /*
             *  We have seen the opening "<element" for a new element and have not yet seen the terminating 
             *  ">" of the opening element.
             */
            switch (token) {
            case MPR_XMLTOK_TEXT:
                /*
                 *  Must be an attribute name
                 */
                aname = mprStrdup(xp, mprGetBufStart(tokBuf));
                token = getToken(xp, state);
                if (token != MPR_XMLTOK_EQ) {
                    xmlError(xp, "Missing assignment for attribute \"%s\"", aname);
                    goto err;
                }

                token = getToken(xp, state);
                if (token != MPR_XMLTOK_TEXT) {
                    xmlError(xp, "Missing value for attribute \"%s\"", aname);
                    goto err;
                }
                state = MPR_XML_NEW_ATT;
                rc = (*handler)(xp, state, tname, aname, mprGetBufStart(tokBuf));
                if (rc < 0) {
                    goto err;
                }
                state = MPR_XML_NEW_ELT;
                break;

            case MPR_XMLTOK_GR:
                /*
                 *  This is ">" the termination of the opening element
                 */
                if (*tname == '\0') {
                    xmlError(xp, "Missing element name");
                    goto err;
                }

                /*
                 *  Tell the user that the opening element is now complete
                 */
                state = MPR_XML_ELT_DEFINED;
                rc = (*handler)(xp, state, tname, 0, 0);
                if (rc < 0) {
                    goto err;
                }
                state = MPR_XML_ELT_DATA;
                break;

            case MPR_XMLTOK_SLASH_GR:
                /*
                 *  If we see a "/>" then this is a solo element
                 */
                if (*tname == '\0') {
                    xmlError(xp, "Missing element name");
                    goto err;
                }
                state = MPR_XML_SOLO_ELT_DEFINED;
                rc = (*handler)(xp, state, tname, 0, 0);
                if (rc < 0) {
                    goto err;
                }
                rc = 1;
                goto exit;
    
            default:
                xmlError(xp, "Syntax error");
                goto err;
            }
            break;

        case MPR_XML_ELT_DATA:      /* -------------------------------------- */
            /*
             *  We have seen the full opening element "<name ...>" and now await data or another element.
             */
            if (token == MPR_XMLTOK_LS) {
                /*
                 *  Recurse to handle the new element, comment etc.
                 */
                rc = parseNext(xp, MPR_XML_AFTER_LS);
                if (rc < 0) {
                    goto exit;
                }
                break;

            } else if (token == MPR_XMLTOK_LS_SLASH) {
                state = MPR_XML_END_ELT;
                break;

            } else if (token != MPR_XMLTOK_TEXT) {
                goto err;
            }
            if (mprGetBufLength(tokBuf) > 0) {
                /*
                 *  Pass the data between the element to the user
                 */
                rc = (*handler)(xp, state, tname, 0, mprGetBufStart(tokBuf));
                if (rc < 0) {
                    goto err;
                }
            }
            break;

        case MPR_XML_END_ELT:           /* -------------------------------------- */
            if (token != MPR_XMLTOK_TEXT) {
                xmlError(xp, "Missing closing element name for \"%s\"", tname);
                goto err;
            }
            /*
             *  The closing element name must match the opening element name 
             */
            if (strcmp(tname, mprGetBufStart(tokBuf)) != 0) {
                xmlError(xp, "Closing element name \"%s\" does not match on line %d"
                    "opening name \"%s\"",
                    mprGetBufStart(tokBuf), xp->lineNumber, tname);
                goto err;
            }
            rc = (*handler)(xp, state, tname, 0, 0);
            if (rc < 0) {
                goto err;
            }
            if (getToken(xp, state) != MPR_XMLTOK_GR) {
                xmlError(xp, "Syntax error");
                goto err;
            }
            return 1;

        case MPR_XML_EOF:       /* ---------------------------------------------- */
            goto exit;

        case MPR_XML_ERR:   /* ---------------------------------------------- */
        default:
            goto err;
        }
    }
    mprAssert(0);

err:
    rc = -1;

exit:
    mprFree(tname);
    mprFree(aname);

    return rc;
}



/*
 *  Lexical analyser for XML. Return the next token reading input as required. It uses a one token look ahead and 
 *  push back mechanism (LAR1 parser). Text token identifiers are left in the tokBuf parser buffer on exit. This Lex 
 *  has special cases for the states MPR_XML_ELT_DATA where we have an optimized read of element data, and 
 *  MPR_XML_AFTER_LS where we distinguish between element names, processing instructions and comments. 
 */
static MprXmlToken getToken(MprXml *xp, int state)
{
    MprBuf      *tokBuf, *inBuf;
    uchar       *cp;
    int         c, rc;

    tokBuf = xp->tokBuf;
    inBuf = xp->inBuf;

    mprAssert(state >= 0);

    if ((c = getNextChar(xp)) < 0) {
        return MPR_XMLTOK_EOF;
    }
    mprFlushBuf(tokBuf);

    /*
     *  Special case parsing for names and for element data. We do this for performance so we can return to the caller 
     *  the largest token possible.
     */
    if (state == MPR_XML_ELT_DATA) {
        /*
         *  Read all the data up to the start of the closing element "<" or the start of a sub-element.
         */
#if UNUSED
        while (isspace(c)) {
            if ((c = getNextChar(xp)) < 0) {
                return MPR_XMLTOK_EOF;
            }
        }
#endif
        if (c == '<') {
            if ((c = getNextChar(xp)) < 0) {
                return MPR_XMLTOK_EOF;
            }
            if (c == '/') {
                return MPR_XMLTOK_LS_SLASH;
            }
            putLastChar(xp, c);
            return MPR_XMLTOK_LS;
        }
        do {
            if (mprPutCharToBuf(tokBuf, c) < 0) {
                return MPR_XMLTOK_TOO_BIG;
            }
            if ((c = getNextChar(xp)) < 0) {
                return MPR_XMLTOK_EOF;
            }
        } while (c != '<');

        /*
         *  Put back the last look-ahead character
         */
        putLastChar(xp, c);

        /*
         *  If all white space, then zero the token buffer
         */
        for (cp = tokBuf->start; *cp; cp++) {
            if (!isspace(*cp)) {
                return MPR_XMLTOK_TEXT;
            }
        }
        mprFlushBuf(tokBuf);
        return MPR_XMLTOK_TEXT;
    }

    while (1) {
        switch (c) {
        case ' ':
        case '\n':
        case '\t':
        case '\r':
            break;

        case '<':
            if ((c = getNextChar(xp)) < 0) {
                return MPR_XMLTOK_EOF;
            }
            if (c == '/') {
                return MPR_XMLTOK_LS_SLASH;
            }
            putLastChar(xp, c);
            return MPR_XMLTOK_LS;
    
        case '=':
            return MPR_XMLTOK_EQ;

        case '>':
            return MPR_XMLTOK_GR;

        case '/':
            if ((c = getNextChar(xp)) < 0) {
                return MPR_XMLTOK_EOF;
            }
            if (c == '>') {
                return MPR_XMLTOK_SLASH_GR;
            }
            return MPR_XMLTOK_ERR;
        
        case '\"':
        case '\'':
            xp->quoteChar = c;
            /* Fall through */

        default:
            /*
             *  We handle element names, attribute names and attribute values 
             *  here. We do NOT handle data between elements here. Read the 
             *  token.  Stop on white space or a closing element ">"
             */
            if (xp->quoteChar) {
                if ((c = getNextChar(xp)) < 0) {
                    return MPR_XMLTOK_EOF;
                }
                while (c != xp->quoteChar) {
                    if (mprPutCharToBuf(tokBuf, c) < 0) {
                        return MPR_XMLTOK_TOO_BIG;
                    }
                    if ((c = getNextChar(xp)) < 0) {
                        return MPR_XMLTOK_EOF;
                    }
                }
                xp->quoteChar = 0;

            } else {
                while (!isspace(c) && c != '>' && c != '/' && c != '=') {
                    if (mprPutCharToBuf(tokBuf, c) < 0) {
                        return MPR_XMLTOK_TOO_BIG;
                    }
                    if ((c = getNextChar(xp)) < 0) {
                        return MPR_XMLTOK_EOF;
                    }
                }
                putLastChar(xp, c);
            }
            if (mprGetBufLength(tokBuf) <= 0) {
                return MPR_XMLTOK_ERR;
            }
            mprAddNullToBuf(tokBuf);

            if (state == MPR_XML_AFTER_LS) {
                /*
                 *  If we are just inside an element "<", then analyze what we
                 *  have to see if we have an element name, instruction or
                 *  comment. Tokbuf will hold "?" for instructions or "!--"
                 *  for comments.
                 */
                if (mprLookAtNextCharInBuf(tokBuf) == '?') {
                    /*  Just ignore processing instructions */
                    rc = scanFor(xp, "?>");
                    if (rc < 0) {
                        return MPR_XMLTOK_TOO_BIG;
                    } else if (rc == 0) {
                        return MPR_XMLTOK_ERR;
                    }
                    return MPR_XMLTOK_INSTRUCTIONS;

                } else if (mprLookAtNextCharInBuf(tokBuf) == '!') {
                    /*
                     *  First discard the comment leadin "!--" and eat leading 
                     *  white space.
                     */
                    if (strcmp((char*) tokBuf->start, "![CDATA[") == 0) {
                        mprFlushBuf(tokBuf);
#if UNUSED
                        c = mprLookAtNextCharInBuf(inBuf);
                        while (isspace(c)) {
                            if ((c = getNextChar(xp)) < 0) {
                                return MPR_XMLTOK_EOF;
                            }
                            c = mprLookAtNextCharInBuf(inBuf);
                        }
#endif
                        rc = scanFor(xp, "]]>");
                        if (rc < 0) {
                            return MPR_XMLTOK_TOO_BIG;
                        } else if (rc == 0) {
                            return MPR_XMLTOK_ERR;
                        }
                        return MPR_XMLTOK_CDATA;

                    } else {
                        mprFlushBuf(tokBuf);
#if UNUSED
                        c = mprLookAtNextCharInBuf(inBuf);
                        while (isspace(c)) {
                            if ((c = getNextChar(xp)) < 0) {
                                return MPR_XMLTOK_EOF;
                            }
                            c = mprLookAtNextCharInBuf(inBuf);
                        }
#endif
                        rc = scanFor(xp, "-->");
                        if (rc < 0) {
                            return MPR_XMLTOK_TOO_BIG;
                        } else if (rc == 0) {
                            return MPR_XMLTOK_ERR;
                        }
                        return MPR_XMLTOK_COMMENT;
                    }
                }
            }
            trimToken(xp);
            return MPR_XMLTOK_TEXT;
        }
        if ((c = getNextChar(xp)) < 0) {
            return MPR_XMLTOK_EOF;
        }
    }

    /* Should never get here */
    mprAssert(0);
    return MPR_XMLTOK_ERR;
}



/*
 *  Scan for a pattern. Eat and discard input up to the pattern. Return 1 if the pattern was found, return 0 if 
 *  not found. Return < 0 on errors.
 */
static int scanFor(MprXml *xp, char *str)
{
    MprBuf  *tokBuf;
    char    *cp;
    int     c;

    mprAssert(str);

    tokBuf = xp->tokBuf;

    while (1) {
        for (cp = str; *cp; cp++) {
            if ((c = getNextChar(xp)) < 0) {
                return 0;
            }
            if (tokBuf) {
                if (mprPutCharToBuf(tokBuf, c) < 0) {
                    return -1;
                }
            }
            if (c != *cp) {
                break;
            }
        }
        if (*cp == '\0') {
            /*
             *  Remove the pattern from the tokBuf
             */
            if (tokBuf) {
                mprAdjustBufEnd(tokBuf, -(int) strlen(str));
                trimToken(xp);
            }
            return 1;
        }
    }
}



/*
 *  Get another character. We read and buffer blocks of data if we need more data to parse.
 */
static int getNextChar(MprXml *xp)
{
    MprBuf  *inBuf;
    char    c;
    int     l;

    inBuf = xp->inBuf;
    if (mprGetBufLength(inBuf) <= 0) {
        /*
         *  Flush to reset the servp/endp pointers to the start of the buffer so we can do a maximal read 
         */
        mprFlushBuf(inBuf);
        l = (xp->readFn)(xp, xp->inputArg, mprGetBufStart(inBuf), mprGetBufSpace(inBuf));
        if (l <= 0) {
            return -1;
        }
        mprAdjustBufEnd(inBuf, l);
    }
    c = mprGetCharFromBuf(inBuf);

    if (c == '\n') {
        xp->lineNumber++;
    }
    return c;
}



/*
 *  Put back a character in the input buffer
 */
static int putLastChar(MprXml *xp, int c)
{
    if (mprInsertCharToBuf(xp->inBuf, (char) c) < 0) {
        mprAssert(0);
        return MPR_ERR_BAD_STATE;
    }
    if (c == '\n') {
        xp->lineNumber--;
    }
    return 0;
}


/*
 *  Output a parse message
 */ 
static void xmlError(MprXml *xp, char *fmt, ...)
{
    va_list     args;
    char        *buf;

    mprAssert(fmt);

    va_start(args, fmt);
    mprAllocVsprintf(xp, &buf, MPR_MAX_STRING, fmt, args);
    va_end(args);

    /*
     *  TODO need to add the failing line text and a pointer to which column
     */
    mprFree(xp->errMsg);
    mprAllocSprintf(xp, &xp->errMsg, MPR_MAX_STRING, "XML error: %s\nAt line %d\n", buf, xp->lineNumber);

    mprFree(buf);
}



/*
 *  Remove trailing whitespace in a token and ensure it is terminated with a NULL for easy parsing
 */
static void trimToken(MprXml *xp)
{
    while (isspace(mprLookAtLastCharInBuf(xp->tokBuf))) {
        mprAdjustBufEnd(xp->tokBuf, -1);
    }
    mprAddNullToBuf(xp->tokBuf);
}



cchar *mprXmlGetErrorMsg(MprXml *xp)
{
    if (xp->errMsg == 0) {
        return "";
    }
    return xp->errMsg;
}



int mprXmlGetLineNumber(MprXml *xp)
{
    return xp->lineNumber;
}




#else
void __dummyMprXml() {} 
#endif /* BLD_FEATURE_XML */

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
/************************************************************************/
/*
 *  End of file "../mprXml.c"
 */
/************************************************************************/

