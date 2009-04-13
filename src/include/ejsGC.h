/*
 *  ejsGC.h - Garbage collection for the Ejscript virtual machine.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

#ifndef _h_EJS_GC
#define _h_EJS_GC 1

/********************************** Includes **********************************/

#ifdef __cplusplus
extern "C" {
#endif

/********************************** Defines ***********************************/
#if !DOXYGEN
/*
 *  Forward declare types
 */
struct Ejs;
#endif

/*
 *  Default GC tune factors
 *  TODO -- move to ejsTune.h
 */
#define EJS_MIN_TIME_FOR_GC         300     /* Need 1/3 sec for GC */
#define EJS_GC_MIN_WORK_QUOTA       50      /* Min to stop thrashing */
    
/** 
 * Magic number when allocated 
 */
#define EJS_GC_IN_USE       0xe801e2ec
#define EJS_GC_FREE         0xe701e3ea

/*
 *  Object generations
 */
#define EJS_GEN_NEW         0           /* New objects */
#define EJS_GEN_YOUNG       1           /* Young objects, survived one collection */
#define EJS_GEN_OLD         2           /* Longer lived application objects, survived multiple collections */
#define EJS_GEN_ETERNAL     3           /* Builtin objects that live forever */
#define EJS_MAX_GEN         4           /* Number of generations for object allocation */

/*
 *  Collection modes
 */
#define EJS_GC_QUICK        0           /* Only collect the newest garbage */
#define EJS_GC_SMART        1           /* Collect from generations that have activity */
#define EJS_GC_ALL          2           /* Collect all garbage */

/*
 *  Per generation structure
 */
typedef struct EjsGen
{
    struct EjsVar   *next;              /* Queue of objects in this generation */
    struct EjsVar   **roots;            /* Cross generational roots for this generation */
    struct EjsVar   **nextRoot;         /* Reference to the next free slot in roots */
    struct EjsVar   **lastRoot;         /* Reference to the last +1 slot in roots */
    struct EjsVar   **peakRoot;         /* Peak root usage */
    int             rootCount;          /* Convenience count of root objects in use */
    int             inUse;              /* Count of objects marked as being used */
    uint            newlyCreated;       /* Count of new objects since last sweep of this generation */
    uint            totalReclaimed;     /* Total blocks reclaimed on sweeps */
    uint            totalSweeps;        /* Total sweeps */
} EjsGen;



/*
 *  Pool of free objects of a given type. Each type maintains a free pool for faster allocations.
 *  Types in the pool have a weak reference and may be reclaimed.
 */
typedef struct EjsPool
{
    struct EjsVar   *next;              /* Next free in pool */
    struct EjsType  *type;              /* Type corresponding to this pool */
    int             allocated;          /* Count of instances created */
    int             peakAllocated;      /* High water mark for allocated */
    int             count;              /* Count in pool */
    int             peakCount;          /* High water mark for count */
    int             reuse;              /* Count of reuses */
} EjsPool;



/*
 *  Garbage collector control
 */
typedef struct EjsGC {

    EjsGen      generations[EJS_MAX_GEN];

    EjsPool     *pools;                 /* Object pools */
    int         numPools;               /* Count of object pools */
    
    uint        allocGeneration;        /* Current generation accepting objects */
    uint        collectGeneration;      /* Current generation doing GC */
    uint        markGenRef;             /* Generation to mark objects */
    uint        firstGlobal;            /* First global slots to examine */

    bool        collecting;             /* Running garbage collection */
    bool        enabled;                /* GC is enabled */
    bool        required;               /* GC is now required */
    bool        enableDemandCollect;    /* Enable GC on demand */
    bool        enableIdleCollect;      /* Enable GC at idle time */

    int         degraded;               /* Have exceeded redlineMemory */
    int         overflow;               /* Cross generational overflow - must do full gc */
    int         workQuota;              /* Quota of work before GC */
    int         workDone;               /* Count of allocations */

    uint        allocatedTypes;         /* Count of types allocated */
    uint        peakAllocatedTypes;     /* Peak allocated types */ 
    uint        allocatedObjects;       /* Count of objects allocated */
    uint        peakAllocatedObjects;   /* Peak allocated */ 
    uint        peakMemory;             /* Peak memory usage */
    uint        totalAllocated;         /* Total count of allocation calls */
    uint        totalReclaimed;         /* Total blocks reclaimed on sweeps */
    uint        totalOverflows;         /* Total overflows  */
    uint        totalRedlines;          /* Total times redline limit exceeded */
    uint        totalSweeps;            /* Total sweeps */

#if BLD_DEBUG
    int         indent;                 /* Indent formatting */
#endif

} EjsGC;

#if FUTURE
#define ejsSetReference(ejs, obj, value) \
    if ((value) && ((struct EjsVar*) (obj))->generation < ((struct EjsVar*) (value))->linkTo) { \
        ((struct EjsVar*) (value))->linkTo = ((struct EjsVar*) (obj))->generation; \
        ejsSetCrossGenRef(ejs, ((struct EjsVar*)(value))->generation, ((struct EjsVar*) (obj))); \
    }
#else
//DDD
extern void ejsSetReference(struct Ejs *ejs, struct EjsVar *obj, struct EjsVar *value);
#endif

extern void ejsSetCrossGenRef(struct Ejs *ejs, int generation, struct EjsVar *obj);
extern int  ejsSetGeneration(struct Ejs *ejs, int generation);

/******************************** Internal API ********************************/

extern void     ejsAnalyzeGlobal(struct Ejs *ejs);
extern int      ejsCreateGCService(struct Ejs *ejs);
//DDD
extern int      ejsIsTimeForGC(struct Ejs *ejs, int timeTillNextEvent);
//DDD
extern void     ejsCollectGarbage(struct Ejs *ejs, int mode);
extern void     ejsEnableGC(struct Ejs *ejs, bool on);
extern void     ejsTraceMark(struct Ejs *ejs, struct EjsVar *vp);
extern void     ejsGracefulDegrade(struct Ejs *ejs);
//DDD
extern void     ejsPrintAllocReport(struct Ejs *ejs);
//DDD
extern void     ejsMakePermanent(struct Ejs *ejs, struct EjsVar *vp);
//DDD
extern void     ejsMakeTransient(struct Ejs *ejs, struct EjsVar *vp);

#if UNUSED
/*
 *  Could possibly make these routines public
 */
extern int      ejsSetGCMaxMemory(struct Ejs *ejs, uint maxMemory);
#endif

/********************************* Public API *********************************/

#ifdef __cplusplus
}
#endif
#endif /* _h_EJS_GC */

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
