/**
 *  sqliteMem.c - Memory interface from Sqlite to the MPR.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************* Includes ***********************************/

#include    "mpr.h"

#if BLD_FEATURE_SQLITE
/*********************************** Locals ***********************************/

#if BLD_FEATURE_MULTITHREAD
static MprThreadLocal   *sqliteLocal;
#endif

static MprCtx           sqliteCtx;

/************************************ Code ************************************/

void ejsSetDbMemoryContext(MprThreadLocal *tls, MprCtx ctx)
{
#if BLD_FEATURE_MULTITHREAD
    sqliteLocal = tls;
    mprSetThreadData(tls, (void*) ctx);
#else
    sqliteCtx = ctx;
#endif
}

/*
 *  Get a memory context to work with. This allows sqlite to execute without anyone explicitly calling ejsSetDbMemoryContext
 */
static inline MprCtx getCtx(MprCtx ctx)
{
    if (ctx == 0) {
        ctx = mprGetMpr(0);
        if (ctx == 0) {
            ctx = sqliteCtx = mprCreate(0, NULL, NULL);
        }
    }
    return ctx;
}


int64 sqlite3_memory_used(void) 
{
    return mprGetAllocStats(0)->bytesAllocated;
}


int64 sqlite3_memory_highwater(int resetFlag) 
{
    return mprGetAllocStats(0)->peakAllocated;
}


void *sqlite3_malloc(int size) 
{
    MprCtx      ctx;

#if BLD_FEATURE_MULTITHREAD
    if (sqliteLocal) {
        ctx = mprGetThreadData(sqliteLocal);
    } else 
#endif
        ctx = sqliteCtx;

    return mprAlloc(getCtx(ctx), size);
}


void sqlite3_free(void *ptr) 
{
    mprFree(ptr);
}


void *sqlite3_realloc(void *ptr, int size) 
{
    MprCtx      ctx;

#if BLD_FEATURE_MULTITHREAD
    if (sqliteLocal) {
        ctx = mprGetThreadData(sqliteLocal);
    } else
#endif
        ctx = sqliteCtx;

    return mprRealloc(getCtx(ctx), ptr, size);
}


int sqlite3MallocSize(void *ptr) 
{
    MprBlk      *bp;

    bp = MPR_GET_BLK(ptr);
    return MPR_GET_BLK_SIZE(bp) - MPR_ALLOC_HDR_SIZE;
}


int sqlite3_memory_alarm(void(*xCallback)(void *pArg, int64 used, int N), void *pArg, int64 iThreshold) 
{
    return 0;
}

#endif /* BLD_FEATURE_SQLITE */
