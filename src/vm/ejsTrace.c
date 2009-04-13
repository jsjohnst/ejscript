#if FUTURE
/*
 *  @file       ejsTrace.c
 *  @brief      JIT Tracing
 *  @overview 
 *  @remarks 
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/*
 *  Next
 *  - Need to have stackDepth counter and do auto pop like ecCodeGen does       
 *  - Need register allocation
 *  - Need guards 
 *
 *  Questions
 *      How to handle exceptions
 *          - Throwing routine must handle the stack for compiled code
 */

/********************************** Includes **********************************/

#include    "ejs.h"


//  NEED to age spots down every so often
#define EJS_JIT_HOT_SPOT        5
#define EJS_JIT_HASH            4099


typedef struct EjsRecord {
    int         pc;                                 /* Should this be a ptr ? */
    int         opcode;

    union {
        struct {
            EjsName     qname;
            int         slotNum;
            int         nthBase;
        } name;

        struct {
            EjsVar      value;
        } literalGetter;
    };

    struct EjsTrace     *trace;                     /* Owning trace */

} EjsRecord;


typedef struct EjsTrace {
    int             origin;                         //  TODO - shoudl this be a pointer?
    MprList         *records;                       /* Trace records */


    struct EjsTrace *parent;
    struct EjsTrace *forw;
    struct EjsTrace *back;
} EjsTrace;



typedef struct EjsJit {
    MprList         *traces;
    EjsTrace        *currentTrace;
    EjsTrace        preCompile;                     /*  Traces to be compiled */
    EjsTrace        postCompile;                    /*  Compiled traces */
    char            hotSpotHash[EJS_JIT_HASH]       /* Hash of PC locations */
} EjsJit;


/*********************************** Forward **********************************/

/************************************* Code ***********************************/
/*
 */
EjsJit *ejsCreateJit(Ejs *ejs)
{
    EjsJit  *jit;

    jit = mprAllocObjZeroed(ctx, EjsJit);
    if (jit == 0) {
        return 0;
    }
    jit->traces = mprCreateList(jit);
    
    return jit;
}



static EjsTrace *createTrace(EjsJit *jit)
{
    EjsTrace    *trace;

    trace = mprAllocObjZeroed(jit, EjsTrace);
    if (trace == 0) {
        return 0;
    }
    return trace;
}



void queueTrace(EjsTrace *head, EjsTrace *trace)
{
    trace = head->forw;

    //  TODO - how to do this lock free??
    trace->forw = head;
    trace->back = head->back;
    trace->back->forw = trace;
    head->back = trace;
}



EjsTrace *dequeTrace(EjsTrace *head)
{
    EjsTrace    *trace;

    //  TODO - how to do this lock free??
    trace = head->forw;
    head->forw = trace->forw;
    head->forw->back = head;
}



static int createTraceRecord(EjsTrace *trace, int origin)
{
    EjsRecord       *rec;
    
    rec = mprAllocObjZeroed(trace, EjsRecord);
    if (rec == 0) {
        return 0;
    }
    rec->origin = origin;
    return rec;
}



/*
 *  Need to age down the temp of all spots
 */
int getSpotTemp(EjsJit *jit, int spot)
{
    return hotSpotHash[(spot >> 4) % EJS_JIT_HASH];
}



void heatSpot(EjsJit *jit, int spot)
{
    return hotSpotHash[(spot >> 4) % EJS_JIT_HASH]++;
}



void removeSpot(EjsJit *jit, int spot)
{
    return hotSpotHash[(spot >> 4) % EJS_JIT_HASH] = 0;
}



/*
 *  TODO Do we need to make sure this is a backward branch?
 */
void ejsTraceBranch(Ejs *ejs, int pc)
{
    EjsJit      *jit;
    EjsTrace    *trace;
    EjsRecord   *rec;

    jit = ejs->jit;

    trace = lookupTrace(jit, pc);
    if (trace == 0) {
        temp = getLocationTemp(jit, pc);
        if (temp < EJS_JIT_HOT_SPOT) {
            heatLocation(jit, pc);
            return;
        }
        trace = createTrace(jit);
        if (trace == 0) {
            return;
        }
    }

    if (trace->pc != pc) {
        /* Must wait until the first trace is compiled */
        return;
    }

    if (trace->pc == pc) {
        /* Trace is complete */
        queueTrace(&jit->preCompile, trace);
        removeSpot(pc);
        ejsCompileTraces(ejs);
    }
}



EjsTraceRecord *createRecord(Ejs *ejs, int pc)
{
    EjsTrace    *trace;
    EjsRecord   *record;

    if ((trace = jit->currentTrace) == 0) {
        return 0;
    }

    rec = createTraceRecord(trace, pc);
    if (rec == 0) {
        return 0;
    }
    rec->trace = trace;

    if (mprAddItem(trace->records, rec) < 0) {
        mprFree(rec);
        ejs->jit->currentTrace = 0;
        mprFree(trace);
    }

    return rec;
}



/*
 *  TODO what about nthBase
 */
void ejsTraceLoadProp(Ejs *ejs, int pc, int opcode, int slotNum, EjsName qname)
{
    EjsTraceRecord  *rec;

    if ((rec = createRecord(ejs, pc)) == 0) {
        return;
    }

    rec->opcode = opcode;
    rec->name.slotNum = slotNum;
    rec->name.qname = qname;
    rec->name.nthBase = nthBase;
}



/*
 *  TODO - how to encode all iteral values
 */
void ejsTraceLoadLiteral(Ejs *ejs, int pc, int opcode, int value)
{
    EjsTraceRecord  *rec;

    if ((rec = createRecord(ejs, pc)) == 0) {
        return;
    }
    rec->opcode = opcode;
    rec->literalGetter.value = value;
}



void ejsCompileTraces(Ejs *ejs)
{
    EjsJit          *jit;
    EjsTrace        *trace;
    EjsTraceRecord  *rec;
    int             next;

    //  TODO - Can we do this lock free?
    while ((trace = dequeueTrace(&jit->preCompile)) != 0) {
        for (next = -1; rec = mprGetPrevItem(trace->records, &next)) != 0; ) {
            switch (rec->opcode) {
            case EJS_OP_LOAD_INT_8:
                /*
                 *  push(ejs, ejsCreateInteger(ejs, getByte(frame)));
                 */
                encodeInvoke(ejs, EJS_FN_CREATE_INTEGER, rec->value, 1);
                encodePushAccum(ejs, );
                break;

            default:
                interpretRecord(ejs, rec);
            }
        }
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
#endif
void __dummyEjsTrace() {}
