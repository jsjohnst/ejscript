/**
 *  ejsFunction.c - Function class
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejs.h"

/******************************************************************************/
/*
 *  Cast the operand to the specified type
 *
 *  intrinsic function cast(type: Type) : Object
 */

static EjsVar *castFunction(Ejs *ejs, EjsFunction *vp, EjsType *type)
{
    switch (type->id) {
    case ES_String:
        return (EjsVar*) ejsCreateString(ejs, "[function Function]");

    case ES_Number:
        return (EjsVar*) ejs->nanValue;

    case ES_Boolean:
        return (EjsVar*) ejs->trueValue;
            
    default:
        ejsThrowTypeError(ejs, "Can't cast type \"%s\"", type->qname.name);
        return 0;
    }
    return 0;
}


//  TODO need deep copy. This just does a shallow copy for now.

static EjsFunction *cloneFunctionVar(Ejs *ejs, EjsFunction *src, bool deep)
{
    EjsFunction     *dest;

    dest = (EjsFunction*) ejsCopyBlock(ejs, &src->block, deep);
    if (dest == 0) {
        return 0;
    }
    dest->body.code = src->body.code;
    dest->resultType = src->resultType;
    dest->thisObj = src->thisObj;
    dest->owner = src->owner;
    dest->slotNum = src->slotNum;
    dest->numArgs = src->numArgs;
    dest->numDefault = src->numDefault;
    dest->nextSlot = src->nextSlot;
    /*
     *  TODO OPT - bit fields
     */
    dest->getter = src->getter;
    dest->setter = src->setter;
    dest->staticMethod = src->staticMethod;
    dest->constructor = src->constructor;
    dest->hasReturn = src->hasReturn;
    dest->isInitializer = src->isInitializer;
    dest->literalGetter = src->literalGetter;
    dest->override = src->override;
    dest->rest = src->rest;
    dest->fullScope = src->fullScope;
    dest->lang = src->lang;

    return dest;
}


static void markFunctionVar(Ejs *ejs, EjsVar *parent, EjsFunction *fun)
{
    ejsMarkBlock(ejs, parent, (EjsBlock*) fun);

    if (fun->owner) {
        ejsMarkVar(ejs, parent, fun->owner);
    }
    if (fun->thisObj) {
        ejsMarkVar(ejs, parent, fun->thisObj);
    }
}


static EjsVar *applyFunction(Ejs *ejs, EjsFunction *fun, int argc, EjsVar **argv)
{
    EjsArray        *args;
    EjsVar          *save, *result;
    EjsFrame        *frame;
    
    mprAssert(argc == 2);
    args = (EjsArray*) argv[1];
    mprAssert(ejsIsArray(args));

    frame = ejs->frame;

    //  TODO - need to protect fun from GC while here
    save = fun->thisObj;
    fun->thisObj = 0;

    result =  ejsRunFunction(ejs, fun, argv[0], args->length, args->data);

    mprAssert(frame == ejs->frame);

    fun->thisObj = save;
    return result;
}


static EjsVar *callFunctionMethod(Ejs *ejs, EjsFunction *fun, int argc, EjsVar **argv)
{
    return applyFunction(ejs, fun, argc, argv);
}

/*************************************************************************************************************/
/*
 *  Create a script function. This defines the method traits. It does not create a  method slot. ResultType may
 *  be null to indicate untyped. NOTE: untyped functions may return a result at their descretion.
 */

//TODO - numExceptions doesn't belong here. Should be incremented via AddException
//TODO - rather than EjsConst, should be a module reference?

EjsFunction *ejsCreateFunction(Ejs *ejs, const uchar *byteCode, int codeLen, int numArgs, int numExceptions, 
        EjsType *resultType, int attributes, EjsConst *constants, EjsBlock *scopeChain, int lang)
{
    EjsFunction     *fun;
    EjsCode         *code;

    fun = (EjsFunction*) ejsCreateVar(ejs, ejs->functionType, 0);
    if (fun == 0) {
        return 0;
    }

    fun->block.obj.var.isFunction = 1;

    fun->numArgs = numArgs;
    fun->resultType = resultType;
    fun->slotNum = -1;
    fun->nextSlot = -1;
    fun->numArgs = numArgs;
    fun->resultType = resultType;
    fun->block.scopeChain = scopeChain;
    fun->lang = lang;

    /*
     *  When functions are in object literals, we dely setting .getter until the object is actually created.
     *  This enables reading the function without running the getter in the VM.
     */
    if (attributes & EJS_ATTR_LITERAL_GETTER) {
        fun->literalGetter = 1;

    } else if (attributes & EJS_ATTR_GETTER) {
        fun->getter = 1;
    }
    if (attributes & EJS_ATTR_SETTER) {
        fun->setter = 1;
    }
    if (attributes & EJS_ATTR_CONSTRUCTOR) {
        fun->constructor = 1;
    }
    if (attributes & EJS_ATTR_REST) {
        fun->rest = 1;
    }
    if (attributes & EJS_ATTR_STATIC) {
        fun->staticMethod = 1;
    }
    if (attributes & EJS_ATTR_OVERRIDE) {
        fun->override = 1;
    }
    if (attributes & EJS_ATTR_NATIVE) {
        fun->block.obj.var.nativeProc = 1;
    }
    if (attributes & EJS_ATTR_FULL_SCOPE) {
        fun->fullScope = 1;
    }
    if (attributes & EJS_ATTR_HAS_RETURN) {
        fun->hasReturn = 1;
    }

    code = &fun->body.code;
    code->codeLen = codeLen;
    code->byteCode = (uchar*) byteCode;
    code->numHandlers = numExceptions;
    code->constants = constants;
    code->finallyIndex = -1;

    return fun;
}


void ejsSetNextFunction(EjsFunction *fun, int nextSlot)
{
    fun->nextSlot = nextSlot;
}


void ejsSetFunctionLocation(EjsFunction *fun, EjsVar *obj, int slotNum)
{
    mprAssert(fun);
    mprAssert(obj);

    fun->owner = obj;
    fun->slotNum = slotNum;
}


EjsEx *ejsAddException(EjsFunction *fun, uint tryStart, uint tryEnd, EjsType *catchType, uint handlerStart,
        uint handlerEnd, int flags, int preferredIndex)
{
    EjsEx           *exception;
    EjsCode         *code;
    int             size;

    mprAssert(fun);

    code = &fun->body.code;

    exception = mprAllocObjZeroed(fun, EjsEx);
    if (exception == 0) {
        mprAssert(0);
        return 0;
    }

    exception->flags = flags;
    exception->tryStart = tryStart;
    exception->tryEnd = tryEnd;
    exception->catchType = catchType;
    exception->handlerStart = handlerStart;
    exception->handlerEnd = handlerEnd;

    if (preferredIndex < 0) {
        preferredIndex = code->numHandlers++;
    }

    if (preferredIndex >= code->sizeHandlers) {
        size = code->sizeHandlers + EJS_EX_INC;
        code->handlers = (EjsEx**) mprRealloc(fun, code->handlers, size * sizeof(EjsEx));
        if (code->handlers == 0) {
            mprAssert(0);
            return 0;
        }
        memset(&code->handlers[code->sizeHandlers], 0, EJS_EX_INC * sizeof(EjsEx)); 
        code->sizeHandlers = size;
    }

    if (flags & EJS_EX_FINALLY) {
        code->finallyIndex = preferredIndex;
    }

    code->handlers[preferredIndex] = exception;

    return exception;
}


void ejsOffsetExceptions(EjsFunction *fun, int offset)
{
    EjsEx           *ex;
    int             i;

    mprAssert(fun);

    for (i = 0; i < fun->body.code.numHandlers; i++) {
        ex = fun->body.code.handlers[i];
        ex->tryStart += offset;
        ex->tryEnd += offset;
        ex->handlerStart += offset;
        ex->handlerEnd += offset;
    }
}


/*
 *  Set the byte code for a script function
 */
int ejsSetFunctionCode(EjsFunction *fun, uchar *byteCode, int len)
{
    mprAssert(fun);
    mprAssert(byteCode);
    mprAssert(len >= 0);

    byteCode = (uchar*) mprMemdup(fun, byteCode, len);
    if (byteCode == 0) {
        return EJS_ERR;
    }

    fun->body.code.codeLen = len;
    mprFree(fun->body.code.byteCode);
    fun->body.code.byteCode = (uchar*) byteCode;

    return 0;
}


EjsFunction *ejsCopyFunction(Ejs *ejs, EjsFunction *src)
{
    return cloneFunctionVar(ejs, src, 0);
}


void ejsCreateFunctionType(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;

    type = ejsCreateCoreType(ejs, ejsName(&qname, EJS_INTRINSIC_NAMESPACE, "Function"), ejs->objectType, sizeof(EjsFunction), 
        ES_Function, ES_Function_NUM_CLASS_PROP, ES_Function_NUM_INSTANCE_PROP, 
        EJS_ATTR_OBJECT | EJS_ATTR_NATIVE | EJS_ATTR_DYNAMIC_INSTANCE | EJS_ATTR_BLOCK_HELPERS);
    ejs->functionType = type;

    /*
     *  Define the helper functions.
     */
    type->helpers->castVar   = (EjsCastVarHelper) castFunction;
    type->helpers->cloneVar  = (EjsCloneVarHelper) cloneFunctionVar;
    type->helpers->markVar   = (EjsMarkVarHelper) markFunctionVar;

#if UNUSED
    type->skipScope = 1;
#endif
}


void ejsConfigureFunctionType(Ejs *ejs)
{
    EjsType     *type;

    type = ejs->functionType;
    ejsBindMethod(ejs, type, ES_Function_apply, (EjsNativeFunction) applyFunction);
    ejsBindMethod(ejs, type, ES_Function_call, (EjsNativeFunction) callFunctionMethod);
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
