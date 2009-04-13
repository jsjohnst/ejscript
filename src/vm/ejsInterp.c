/*
 *  ejsInterp.c - Virtual Machine Interpreter for Ejscript.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejs.h"

/********************************** Inline Code *******************************/
/*
 *  The stack is a stack of pointers to EjsVar. The top of stack (stack.top) always points to the current top item 
 *  on the stack. To push a new value, top is incremented then the value is stored. To pop, simply copy the value at 
 *  top and decrement top ptr.
 */
#define pop(ejs)            *ejs->stack.top--
#define push(ejs, value)    (*(++(ejs->stack.top))) = ((EjsVar*) (value))
#define popString(ejs)      ((EjsString*) pop(ejs))
#define getType(frame)      ((EjsType*) getGlobalArg(frame))

//  TODO - If PUT_PROPERTY validated the type on assignment, we could eliminate testing against numProp and 
//  TODO - vp should never be zero
//  TODO - OPTIMIZE GET_PROPERTY

#define GET_PROPERTY(_ejs, _thisObj, _vp, _slotNum)                                                         \
    if (unlikely(_vp == 0 || (EjsVar*) (_vp) == (EjsVar*) ejs->nullValue)) {                                \
        goto nullException;                                                                                 \
    } else {                                                                                                \
        EjsVar *_value;                                                                                     \
        if (ejsIsObject(_vp)) {                                                                             \
            if (unlikely((_slotNum >= ((EjsObject*)(_vp))->numProp))) {                                     \
                goto slotReferenceException;                                                                \
            }                                                                                               \
            _value = ((EjsObject*)(_vp))->slots[_slotNum];                                                  \
        } else {                                                                                            \
            _value = ejsGetProperty(_ejs, (EjsVar*) (_vp), _slotNum);                                       \
        }                                                                                                   \
        mprAssert(_value);                                                                                  \
        if (unlikely(_value->isFunction)) {                                                                 \
            frame = getFunction(_ejs, _thisObj, (EjsVar*)(_vp), _slotNum, (EjsFunction*) _value, &local);   \
        } else {                                                                                            \
            push(_ejs, _value);                                                                             \
        }                                                                                                   \
    }

#define PUT_PROPERTY(_ejs, _thisObj, _vp, _slotNum)                                                         \
    if (unlikely(_vp == 0 || (EjsVar*) (_vp) == (EjsVar*) ejs->nullValue)) {                                \
        goto nullException;                                                                                 \
    } else if (_slotNum >= ((EjsObject*)(_vp))->numProp) {                                                  \
        ejsSetProperty(_ejs, (EjsVar*) (_vp), _slotNum, pop(_ejs));                                         \
    } else {                                                                                                \
        EjsVar  *_oldValue;                                                                                 \
        if (ejsIsObject(_vp)) {                                                                             \
            _oldValue = ((EjsObject*)(_vp))->slots[_slotNum];                                               \
            mprAssert(_oldValue);                                                                           \
        } else {                                                                                            \
            _oldValue = ejsGetProperty(_ejs, (EjsVar*) (_vp), _slotNum);                                    \
        }                                                                                                   \
        if (unlikely(_oldValue->isFunction)) {                                                              \
            EjsFunction *_fun = (EjsFunction*) _oldValue;                                                   \
            if (_fun->getter && _fun->nextSlot) {                                                           \
                putFunction(_ejs, (EjsVar*) (_thisObj ? _thisObj : _vp), _fun, pop(_ejs));                  \
            }                                                                                               \
        } else {                                                                                            \
            EjsVar *_value = pop(_ejs);                                                                     \
            if (ejsIsObject(_vp)) {                                                                         \
                ((EjsObject*)(_vp))->slots[_slotNum] = _value;                                              \
            } else {                                                                                        \
                ((EjsObject*)(_vp))->slots[_slotNum] = _value;                                              \
            }                                                                                               \
            ejsSetReference(_ejs, (EjsVar*) _vp, (EjsVar*) _value);                                         \
        }                                                                                                   \
    }

#if LINUX || MACOSX || LINUX || SOLARIS || VXWORKS
    #define CASE(opcode) opcode
    #define BREAK \
        if (1) { \
            opcode = getByte(frame); \
            goto *opcodeJump[traceCode(ejs, opcode)]; \
        } else
    #define CHECK \
        if (1) { \
            if (unlikely(ejs->attention)) { \
                if ((frame = payAttention(ejs)) == 0) { \
                    return; \
                } \
                local = (EjsObject*) frame->currentFunction; \
            } \
        } else
#else
    /*
     *  Traditional switch for compilers (looking at you MS) without computed goto.
     */
    #define BREAK break
    #define CHECK 
    #define CASE(opcode) case opcode
#endif

#if BLD_DEBUG
static EjsOpCode traceCode(Ejs *ejs, EjsOpCode opcode);
#define setFrameDebugName(frame, vp) (frame)->debugName = ((EjsVar*) (vp))->debugName
#else
static EjsOpCode traceCode(Ejs *ejs, EjsOpCode opcode);
// #define traceCode(ejs, opcode) opcode
#define setFrameDebugName(frame, name)
#endif

#if MPR_LITTLE_ENDIAN

#if 1
#define getByte(frame)              *(frame)->pc++

/*
 *  This is just temporary to avoid GCC warnings!
 */
static inline ushort getShort(EjsFrame *frame) {
    ushort  value, *sp = (ushort*) frame->pc;
    value = *sp;
    frame->pc = (uchar*) (sp + 1);
    return value;
}

static inline uint getWord(EjsFrame *frame) {
    uint  value, *sp = (uint*) frame->pc;
    value = *sp;
    frame->pc = (uchar*) (sp + 1);
    return value;
}

static inline uint64 getLong(EjsFrame *frame) {
    uint64  value, *sp = (uint64*) frame->pc;
    value = *sp;
    frame->pc = (uchar*) (sp + 1);
    return value;
}

#if BLD_FEATURE_FLOATING_POINT
static inline double getDoubleWord(EjsFrame *frame) {
    double  value, *sp = (double*) frame->pc;
    value = *sp;
    frame->pc = (uchar*) (sp + 1);
    return value;
}
#endif

#else

#define getByte(frame)              *(frame)->pc++
#define getShort(frame)             *((ushort*) ((frame)->pc))++
#define getWord(frame)              *((uint*) (frame)->pc)++
#define getLong(frame)              *((uint64*) (frame)->pc)++
#if BLD_FEATURE_FLOATING_POINT
#define getDoubleWord(frame)        *((double*) (frame)->pc)++
#endif

#endif

#else
//  TODO - add swapping code. NOTE: need to have in module what is the endian-ness of the module.
#endif

/******************************** Forward Declarations ************************/

static EjsFrame *callBySlot(Ejs *ejs, EjsVar *vp, int slotNum, EjsVar *thisObj, int argc, int stackAdjust);
static inline EjsFrame *createFrame(Ejs *ejs);
static EjsFrame *callFunction(Ejs *ejs, EjsFunction *fun, EjsVar *thisObj, int argc, int stackAdjust);
static void callExceptionHandler(Ejs *ejs, EjsFunction *fun, int index, int flags);
static void debug(EjsFrame *frame);
static EjsVar *evalBinaryExpr(Ejs *ejs, EjsVar *lhs, EjsOpCode opcode, EjsVar *rhs);
static EjsVar *evalUnaryExpr(Ejs *ejs, EjsVar *lhs, EjsOpCode opcode);
static EjsName getNameArg(EjsFrame *frame);
static EjsVar *getNthBase(Ejs *ejs, EjsVar *obj, int nthBase);
static EjsVar *getNthBaseFromBottom(Ejs *ejs, EjsVar *obj, int nthBase);
static inline EjsVar *getNthBlock(EjsFrame *frame, int nth);
static int getNum(EjsFrame *frame);
static char *getStringArg(EjsFrame *frame);
static EjsVar *getGlobalArg(EjsFrame *frame);
static bool handleException(Ejs *ejs);
static void handleExceptionAtThisLevel(Ejs *ejs, EjsFrame *frame);
static void makeClosure(EjsFrame *frame);
static EjsFrame *getFunction(Ejs *ejs, EjsVar *thisObj, EjsVar *owner, int slotNum, EjsFunction *fun, EjsObject **local);
static void needClosure(EjsFrame *frame, EjsBlock *block);
static EjsFrame *payAttention(Ejs *ejs);
static EjsFrame *popExceptionFrame(Ejs *ejs);
static bool popFrameAndReturn(Ejs *ejs);
static void putFunction(Ejs *ejs, EjsVar *thisObj, EjsFunction *fun, EjsVar *value);
static void storeProperty(Ejs *ejs, EjsVar *obj, EjsName *name);
static void storePropertyToScope(Ejs *ejs, EjsName *qname);
static void swap2(Ejs *ejs);
static void throwNull(Ejs *ejs);
static EjsBlock *varToBlock(Ejs *ejs, EjsVar *vp);
static void vmLoop(Ejs *ejs);

/************************************* Code ***********************************/
/*
 *  Virtual Machine byte code evaluation
 */
static void vmLoop(Ejs *ejs)
{
    EjsFrame        *frame;
    EjsFunction     *fun;
    EjsString       *nameVar;
    EjsType         *type;
    EjsVar          *v1, *v2, *result, *vp, *vobj;
    EjsObject       *global, *local, *thisObj, *obj;
    EjsVar          **stack;
    EjsName         qname;
    EjsLookup       lookup;
    char            *str;
    uchar           *pc;
    int             i, argc, offset, slotNum, count, opcode, nthBase;

#if LINUX || MACOSX || LINUX || SOLARIS || VXWORKS 
    /*
     *  Direct threading computed goto processing. Include computed goto jump table.
     */
    static void *opcodeJump[] = {
        &&EJS_OP_ADD,
        &&EJS_OP_ADD_NAMESPACE,
        &&EJS_OP_ADD_NAMESPACE_REF,
        &&EJS_OP_AND,
        &&EJS_OP_BRANCH_EQ,
        &&EJS_OP_BRANCH_STRICTLY_EQ,
        &&EJS_OP_BRANCH_FALSE,
        &&EJS_OP_BRANCH_GE,
        &&EJS_OP_BRANCH_GT,
        &&EJS_OP_BRANCH_LE,
        &&EJS_OP_BRANCH_LT,
        &&EJS_OP_BRANCH_NE,
        &&EJS_OP_BRANCH_STRICTLY_NE,
        &&EJS_OP_BRANCH_NULL,
        &&EJS_OP_BRANCH_NOT_ZERO,
        &&EJS_OP_BRANCH_TRUE,
        &&EJS_OP_BRANCH_UNDEFINED,
        &&EJS_OP_BRANCH_ZERO,
        &&EJS_OP_BRANCH_FALSE_8,
        &&EJS_OP_BRANCH_TRUE_8,
        &&EJS_OP_BREAKPOINT,

        &&EJS_OP_CALL,
        &&EJS_OP_CALL_GLOBAL_SLOT,
        &&EJS_OP_CALL_OBJ_SLOT,
        &&EJS_OP_CALL_THIS_SLOT,
        &&EJS_OP_CALL_BLOCK_SLOT,
        &&EJS_OP_CALL_OBJ_INSTANCE_SLOT,
        &&EJS_OP_CALL_OBJ_STATIC_SLOT,
        &&EJS_OP_CALL_THIS_STATIC_SLOT,
        &&EJS_OP_CALL_OBJ_NAME,
        &&EJS_OP_CALL_SCOPED_NAME,
        &&EJS_OP_CALL_CONSTRUCTOR,
        &&EJS_OP_CALL_NEXT_CONSTRUCTOR,

        &&EJS_OP_CAST,
        &&EJS_OP_CAST_BOOLEAN,
        &&EJS_OP_CLOSE_BLOCK,
        &&EJS_OP_CLOSE_WITH,
        &&EJS_OP_COMPARE_EQ,
        &&EJS_OP_COMPARE_STRICTLY_EQ,
        &&EJS_OP_COMPARE_FALSE,
        &&EJS_OP_COMPARE_GE,
        &&EJS_OP_COMPARE_GT,
        &&EJS_OP_COMPARE_LE,
        &&EJS_OP_COMPARE_LT,
        &&EJS_OP_COMPARE_NE,
        &&EJS_OP_COMPARE_STRICTLY_NE,
        &&EJS_OP_COMPARE_NULL,
        &&EJS_OP_COMPARE_NOT_ZERO,
        &&EJS_OP_COMPARE_TRUE,
        &&EJS_OP_COMPARE_UNDEFINED,
        &&EJS_OP_COMPARE_ZERO,
        &&EJS_OP_DEBUG,
        &&EJS_OP_DEFINE_CLASS,
        &&EJS_OP_DEFINE_FUNCTION,
        &&EJS_OP_DEFINE_GLOBAL_FUNCTION,
        &&EJS_OP_DELETE_NAME_EXPR,
        &&EJS_OP_DELETE,
        &&EJS_OP_DELETE_NAME,
        &&EJS_OP_DIV,
        &&EJS_OP_DUP,
        &&EJS_OP_DUP2,
        &&EJS_OP_END_CODE,
        &&EJS_OP_END_EXCEPTION,
        &&EJS_OP_GOTO,
        &&EJS_OP_GOTO_8,
        &&EJS_OP_INC,
        &&EJS_OP_INIT_DEFAULT_ARGS,
        &&EJS_OP_INIT_DEFAULT_ARGS_8,
        &&EJS_OP_INST_OF,
        &&EJS_OP_IS_A,
        &&EJS_OP_LOAD_0,
        &&EJS_OP_LOAD_1,
        &&EJS_OP_LOAD_2,
        &&EJS_OP_LOAD_3,
        &&EJS_OP_LOAD_4,
        &&EJS_OP_LOAD_5,
        &&EJS_OP_LOAD_6,
        &&EJS_OP_LOAD_7,
        &&EJS_OP_LOAD_8,
        &&EJS_OP_LOAD_9,
        &&EJS_OP_LOAD_DOUBLE,
        &&EJS_OP_LOAD_FALSE,
        &&EJS_OP_LOAD_GLOBAL,
        &&EJS_OP_LOAD_INT_16,
        &&EJS_OP_LOAD_INT_32,
        &&EJS_OP_LOAD_INT_64,
        &&EJS_OP_LOAD_INT_8,
        &&EJS_OP_LOAD_M1,
        &&EJS_OP_LOAD_NAME,
        &&EJS_OP_LOAD_NAMESPACE,
        &&EJS_OP_LOAD_NULL,
        &&EJS_OP_LOAD_REGEXP,
        &&EJS_OP_LOAD_STRING,
        &&EJS_OP_LOAD_THIS,
        &&EJS_OP_LOAD_TRUE,
        &&EJS_OP_LOAD_UNDEFINED,
        &&EJS_OP_LOAD_XML,
        &&EJS_OP_GET_LOCAL_SLOT_0,
        &&EJS_OP_GET_LOCAL_SLOT_1,
        &&EJS_OP_GET_LOCAL_SLOT_2,
        &&EJS_OP_GET_LOCAL_SLOT_3,
        &&EJS_OP_GET_LOCAL_SLOT_4,
        &&EJS_OP_GET_LOCAL_SLOT_5,
        &&EJS_OP_GET_LOCAL_SLOT_6,
        &&EJS_OP_GET_LOCAL_SLOT_7,
        &&EJS_OP_GET_LOCAL_SLOT_8,
        &&EJS_OP_GET_LOCAL_SLOT_9,
        &&EJS_OP_GET_OBJ_SLOT_0,
        &&EJS_OP_GET_OBJ_SLOT_1,
        &&EJS_OP_GET_OBJ_SLOT_2,
        &&EJS_OP_GET_OBJ_SLOT_3,
        &&EJS_OP_GET_OBJ_SLOT_4,
        &&EJS_OP_GET_OBJ_SLOT_5,
        &&EJS_OP_GET_OBJ_SLOT_6,
        &&EJS_OP_GET_OBJ_SLOT_7,
        &&EJS_OP_GET_OBJ_SLOT_8,
        &&EJS_OP_GET_OBJ_SLOT_9,
        &&EJS_OP_GET_THIS_SLOT_0,
        &&EJS_OP_GET_THIS_SLOT_1,
        &&EJS_OP_GET_THIS_SLOT_2,
        &&EJS_OP_GET_THIS_SLOT_3,
        &&EJS_OP_GET_THIS_SLOT_4,
        &&EJS_OP_GET_THIS_SLOT_5,
        &&EJS_OP_GET_THIS_SLOT_6,
        &&EJS_OP_GET_THIS_SLOT_7,
        &&EJS_OP_GET_THIS_SLOT_8,
        &&EJS_OP_GET_THIS_SLOT_9,
        &&EJS_OP_GET_SCOPED_NAME,
        &&EJS_OP_GET_OBJ_NAME,
        &&EJS_OP_GET_OBJ_NAME_EXPR,
        &&EJS_OP_GET_BLOCK_SLOT,
        &&EJS_OP_GET_GLOBAL_SLOT,
        &&EJS_OP_GET_LOCAL_SLOT,
        &&EJS_OP_GET_OBJ_SLOT,
        &&EJS_OP_GET_THIS_SLOT,
        &&EJS_OP_GET_TYPE_SLOT,
        &&EJS_OP_GET_THIS_TYPE_SLOT,
        &&EJS_OP_IN,
        &&EJS_OP_LIKE,
        &&EJS_OP_LOGICAL_NOT,
        &&EJS_OP_MUL,
        &&EJS_OP_NEG,
        &&EJS_OP_NEW,
        &&EJS_OP_NEW_ARRAY,
        &&EJS_OP_NEW_OBJECT,
        &&EJS_OP_NOP,
        &&EJS_OP_NOT,
        &&EJS_OP_OPEN_BLOCK,
        &&EJS_OP_OPEN_WITH,
        &&EJS_OP_OR,
        &&EJS_OP_POP,
        &&EJS_OP_POP_ITEMS,
        &&EJS_OP_PUSH_CATCH_ARG,
        &&EJS_OP_PUSH_RESULT,
        &&EJS_OP_PUT_LOCAL_SLOT_0,
        &&EJS_OP_PUT_LOCAL_SLOT_1,
        &&EJS_OP_PUT_LOCAL_SLOT_2,
        &&EJS_OP_PUT_LOCAL_SLOT_3,
        &&EJS_OP_PUT_LOCAL_SLOT_4,
        &&EJS_OP_PUT_LOCAL_SLOT_5,
        &&EJS_OP_PUT_LOCAL_SLOT_6,
        &&EJS_OP_PUT_LOCAL_SLOT_7,
        &&EJS_OP_PUT_LOCAL_SLOT_8,
        &&EJS_OP_PUT_LOCAL_SLOT_9,
        &&EJS_OP_PUT_OBJ_SLOT_0,
        &&EJS_OP_PUT_OBJ_SLOT_1,
        &&EJS_OP_PUT_OBJ_SLOT_2,
        &&EJS_OP_PUT_OBJ_SLOT_3,
        &&EJS_OP_PUT_OBJ_SLOT_4,
        &&EJS_OP_PUT_OBJ_SLOT_5,
        &&EJS_OP_PUT_OBJ_SLOT_6,
        &&EJS_OP_PUT_OBJ_SLOT_7,
        &&EJS_OP_PUT_OBJ_SLOT_8,
        &&EJS_OP_PUT_OBJ_SLOT_9,
        &&EJS_OP_PUT_THIS_SLOT_0,
        &&EJS_OP_PUT_THIS_SLOT_1,
        &&EJS_OP_PUT_THIS_SLOT_2,
        &&EJS_OP_PUT_THIS_SLOT_3,
        &&EJS_OP_PUT_THIS_SLOT_4,
        &&EJS_OP_PUT_THIS_SLOT_5,
        &&EJS_OP_PUT_THIS_SLOT_6,
        &&EJS_OP_PUT_THIS_SLOT_7,
        &&EJS_OP_PUT_THIS_SLOT_8,
        &&EJS_OP_PUT_THIS_SLOT_9,
        &&EJS_OP_PUT_OBJ_NAME_EXPR,
        &&EJS_OP_PUT_SCOPED_NAME_EXPR,
        &&EJS_OP_PUT_OBJ_NAME,
        &&EJS_OP_PUT_SCOPED_NAME,
        &&EJS_OP_PUT_BLOCK_SLOT,
        &&EJS_OP_PUT_GLOBAL_SLOT,
        &&EJS_OP_PUT_LOCAL_SLOT,
        &&EJS_OP_PUT_OBJ_SLOT,
        &&EJS_OP_PUT_THIS_SLOT,
        &&EJS_OP_PUT_TYPE_SLOT,
        &&EJS_OP_PUT_THIS_TYPE_SLOT,
        &&EJS_OP_REM,
        &&EJS_OP_RETURN,
        &&EJS_OP_RETURN_VALUE,
        &&EJS_OP_SAVE_RESULT,
        &&EJS_OP_SHL,
        &&EJS_OP_SHR,
        &&EJS_OP_SUB,
        &&EJS_OP_SUPER,
        &&EJS_OP_SWAP,
        &&EJS_OP_THROW,
        &&EJS_OP_TYPE_OF,
        &&EJS_OP_USHR,
        &&EJS_OP_XOR,
    };
#endif

    mprAssert(ejs);
    mprAssert(!mprHasAllocError(ejs));

    frame = ejs->frame;
    stack = ejs->stack.top;
    global = (EjsObject*) ejs->global;
    local = (EjsObject*) frame->currentFunction;
    thisObj = global;
    pc = frame->pc;

    /*
     *  Just to keep the compiler happy
     */
    slotNum = -1;
    vp = 0;

#if LINUX || MACOSX || LINUX || SOLARIS || VXWORKS 
    /*
     *  Direct threading computed goto processing. Include computed goto jump table.
     */
    BREAK;

#else
    /*
     *  Traditional switch for compilers (looking at you MS) without computed goto.
     */
    while (1) {
        opcode = (EjsOpCode) getByte(frame);
        traceCode(ejs, opcode);
        switch (opcode) {
#endif
        /*
         *  Symbolic source code debug information
         *      Debug <fileName> <lineNumber> <sourceLine>
         */
        CASE (EJS_OP_DEBUG):
            debug(frame);
            BREAK;

        /*
         *  End of a code block. Used to mark the end of a script. Saves testing end of code block in VM loop.
         *      EndCode
         */
        CASE (EJS_OP_END_CODE):
            /*
             *  The "ejs" command needs to preserve the current ejs->result for interactive sessions.
             */
            if (ejs->result == 0) {
                ejs->result = frame->returnValue = ejs->undefinedValue;
            }
            popFrameAndReturn(ejs);
            return;

        /*
         *  Return from a function with a result
         *      ReturnValue
         *      Stack before (top)  [value]
         *      Stack after         []
         *      Frame return value set
         */
        CASE (EJS_OP_RETURN_VALUE):
            frame->returnValue = pop(ejs);
            ejs->result = frame->returnValue;
            if (popFrameAndReturn(ejs)) {
                return;
            }
            frame = ejs->frame;
            local = (EjsObject*) frame->currentFunction;
            CHECK; BREAK;

        /*
         *  Return from a function without a result
         *      Return
         */
        CASE (EJS_OP_RETURN):
            frame->returnValue = 0;
            ejs->result = ejs->undefinedValue;
            if (popFrameAndReturn(ejs)) {
                return;
            }
            frame = ejs->frame;
            local = (EjsObject*) frame->currentFunction;
            CHECK; BREAK;

        /*
         *  Load the catch argument
         *      PushCatchArg
         *      Stack before (top)  []
         *      Stack after         [catchArg]
         */
        CASE (EJS_OP_PUSH_CATCH_ARG):
            /*
             *  The exception argument has been pushed on the stack before the catch block
             */
            push(ejs, frame->exceptionArg);
            BREAK;

        /*
         *  Push the function call result
         *      PushResult
         *      Stack before (top)  []
         *      Stack after         [result]
         */
        CASE (EJS_OP_PUSH_RESULT):
            push(ejs, frame->returnValue);
            BREAK;

        /*
         *  Save the top of stack and store in the interpreter result register
         *      SaveResult
         *      Stack before (top)  [value]
         *      Stack after         []
         */
        CASE (EJS_OP_SAVE_RESULT):
            ejs->result = frame->returnValue = pop(ejs);
            BREAK;


        /*
         *  Load Constants -----------------------------------------------
         */

        /*
         *  Load a signed 8 bit integer constant
         *      LoadInt.8           <int8>
         *      Stack before (top)  []
         *      Stack after         [Number]
         */
        CASE (EJS_OP_LOAD_INT_8):
            push(ejs, ejsCreateNumber(ejs, getByte(frame)));
            CHECK; BREAK;

        /*
         *  Load a signed 16 bit integer constant
         *      LoadInt.16          <int16>
         *      Stack before (top)  []
         *      Stack after         [Number]
         */
        CASE (EJS_OP_LOAD_INT_16):
            push(ejs, ejsCreateNumber(ejs, getShort(frame)));
            CHECK; BREAK;

        /*
         *  Load a signed 32 bit integer constant
         *      LoadInt.32          <int32>
         *      Stack before (top)  []
         *      Stack after         [Number]
         */
        CASE (EJS_OP_LOAD_INT_32):
            push(ejs, ejsCreateNumber(ejs, getWord(frame)));
            CHECK; BREAK;

        /*
         *  Load a signed 64 bit integer constant
         *      LoadInt.64          <int64>
         *      Stack before (top)  []
         *      Stack after         [Number]
         */
        CASE (EJS_OP_LOAD_INT_64):
        push(ejs, ejsCreateNumber(ejs, (MprNumber) getLong(frame)));
        CHECK; BREAK;

        /*
         *  Load a float constant
         *      LoadDouble          <double>
         *      Stack before (top)  []
         *      Stack after         [Double]
         */
        CASE (EJS_OP_LOAD_DOUBLE):
#if BLD_FEATURE_FLOATING_POINT
            push(ejs, ejsCreateNumber(ejs, getDoubleWord(frame)));
#endif
            CHECK; BREAK;

        /*
         *  Load integer constant between 0 and 9
         *      Load0, Load1, ... Load9
         *      Stack before (top)  []
         *      Stack after         [Number]
         */
        CASE (EJS_OP_LOAD_0):
        CASE (EJS_OP_LOAD_1):
        CASE (EJS_OP_LOAD_2):
        CASE (EJS_OP_LOAD_3):
        CASE (EJS_OP_LOAD_4):
        CASE (EJS_OP_LOAD_5):
        CASE (EJS_OP_LOAD_6):
        CASE (EJS_OP_LOAD_7):
        CASE (EJS_OP_LOAD_8):
        CASE (EJS_OP_LOAD_9):
            push(ejs, ejsCreateNumber(ejs, opcode - EJS_OP_LOAD_0));
            CHECK; BREAK;

        /*
         *  Load the -1 integer constant
         *      LoadMinusOne
         *      Stack before (top)  []
         *      Stack after         [Number]
         */
        CASE (EJS_OP_LOAD_M1):
            push(ejs, ejsCreateNumber(ejs, -1));
            BREAK;

        /*
         *  Load a string constant (TODO - Unicode)
         *      LoadString          <string>
         *      Stack before (top)  []
         *      Stack after         [String]
         */
        CASE (EJS_OP_LOAD_STRING):
            str = getStringArg(frame);
            push(ejs, ejsCreateString(ejs, str));
            CHECK; BREAK;

        /*
         *  Load a namespace constant
         *      LoadNamespace       <UriString>
         *      Stack before (top)  []
         *      Stack after         [Namespace]
         */
        CASE (EJS_OP_LOAD_NAMESPACE):
            /*
             *  TODO - This is the namespace URI. missing the name itself.  TODO Unicode.
             */
            str = getStringArg(frame);
            push(ejs, ejsCreateNamespace(ejs, str, str));
            CHECK; BREAK;


        /*
         *  Load an XML constant
         *      LoadXML             <xmlString>
         *      Stack before (top)  []
         *      Stack after         [XML]
         */
        CASE (EJS_OP_LOAD_XML):
#if BLD_FEATURE_EJS_E4X
            v1 = (EjsVar*) ejsCreateXML(ejs, 0, 0, 0, 0);
            str = getStringArg(frame);
            ejsLoadXMLString(ejs, (EjsXML*) v1, str);
            push(ejs, v1);
#endif
            CHECK; BREAK;

        /*
         *  Load a Regexp constant
         *      LoadRegExp
         *      Stack before (top)  []
         *      Stack after         [RegExp]
         */
        CASE (EJS_OP_LOAD_REGEXP):
            str = getStringArg(frame);
#if BLD_FEATURE_REGEXP
            v1 = (EjsVar*) ejsCreateRegExp(ejs, str);
            push(ejs, v1);
#else
            ejsThrowReferenceError(ejs, "No regular expression support");
#endif
            CHECK; BREAK;

        /*
         *  Load a null constant
         *      LoadNull
         *      Stack before (top)  []
         *      Stack after         [Null]
         */
        CASE (EJS_OP_LOAD_NULL):
            push(ejs, ejs->nullValue);
            BREAK;

        /*
         *  Load a void / undefined constant
         *      LoadUndefined
         *      Stack before (top)  []
         *      Stack after         [undefined]
         */
        CASE (EJS_OP_LOAD_UNDEFINED):                           //  TODO - rename LOAD_UNDEFINED
            push(ejs, ejs->undefinedValue);
            BREAK;

        /*
         *  Load the "this" value
         *      LoadThis
         *      Stack before (top)  []
         *      Stack after         [this]
         */
        CASE (EJS_OP_LOAD_THIS):
            mprAssert(frame->thisObj);
            push(ejs, frame->thisObj);
            BREAK;

        /*
         *  Load the "global" value
         *      LoadGlobal
         *      Stack before (top)  []
         *      Stack after         [global]
         */
        CASE (EJS_OP_LOAD_GLOBAL):
            push(ejs, ejs->global);
            BREAK;

        /*
         *  Load the "true" value
         *      LoadTrue
         *      Stack before (top)  []
         *      Stack after         [true]
         */
        CASE (EJS_OP_LOAD_TRUE):
            push(ejs, ejs->trueValue);
            BREAK;

        /*
         *  Load the "false" value
         *      LoadFalse
         *      Stack before (top)  []
         *      Stack after         [false]
         */
        CASE (EJS_OP_LOAD_FALSE):
            push(ejs, ejs->falseValue);
            BREAK;


        /*
         *  Push a global variable by slot number
         *      GetGlobalSlot       <slot>
         *      Stack before (top)  []
         *      Stack after         [PropRef]
         */
        CASE (EJS_OP_GET_GLOBAL_SLOT):
            slotNum = getByte(frame);
            GET_PROPERTY(ejs, NULL, global, slotNum);
            CHECK; BREAK;

        /*
         *  Push a local variable by slot number
         *      GetLocalSlot        <slot>
         *      Stack before (top)  []
         *      Stack after         [PropRef]
         */
        CASE (EJS_OP_GET_LOCAL_SLOT):
            slotNum = getByte(frame);
            GET_PROPERTY(ejs, NULL, local, slotNum);
            CHECK; BREAK;

        /*
         *  Push a local variable in slot 0-9
         *      GetLocalSlot0, GetLocalSlot1, ... GetLocalSlot9
         *      Stack before (top)  []
         *      Stack after         [PropRef]
         */
        CASE (EJS_OP_GET_LOCAL_SLOT_0):
        CASE (EJS_OP_GET_LOCAL_SLOT_1):
        CASE (EJS_OP_GET_LOCAL_SLOT_2):
        CASE (EJS_OP_GET_LOCAL_SLOT_3):
        CASE (EJS_OP_GET_LOCAL_SLOT_4):
        CASE (EJS_OP_GET_LOCAL_SLOT_5):
        CASE (EJS_OP_GET_LOCAL_SLOT_6):
        CASE (EJS_OP_GET_LOCAL_SLOT_7):
        CASE (EJS_OP_GET_LOCAL_SLOT_8):
        CASE (EJS_OP_GET_LOCAL_SLOT_9):
            slotNum = opcode - EJS_OP_GET_LOCAL_SLOT_0;
            GET_PROPERTY(ejs, NULL, local, slotNum);
            CHECK; BREAK;

        /*
         *  Push a block scoped variable by slot number
         *      GetBlockSlot        <slot> <nthBlock>
         *      Stack before (top)  []
         *      Stack after         [value]
         */
        CASE (EJS_OP_GET_BLOCK_SLOT):
            slotNum = getByte(frame);
            obj = (EjsObject*) getNthBlock(frame, getByte(frame));
            GET_PROPERTY(ejs, NULL, obj, slotNum);
            CHECK; BREAK;

        /*
         *  Push a property in thisObj by slot number
         *      GetThisSlot         <slot>
         *      Stack before (top)  []
         *      Stack after         [value]
         */
        CASE (EJS_OP_GET_THIS_SLOT):
            slotNum = getByte(frame);
            GET_PROPERTY(ejs, NULL, frame->thisObj, slotNum);
            CHECK; BREAK;

        /*
         *  Push a property in slot 0-9
         *      GetThisSlot0, GetThisSlot1,  ... GetThisSlot9
         *      Stack before (top)  []
         *      Stack after         [value]
         */
        CASE (EJS_OP_GET_THIS_SLOT_0):
        CASE (EJS_OP_GET_THIS_SLOT_1):
        CASE (EJS_OP_GET_THIS_SLOT_2):
        CASE (EJS_OP_GET_THIS_SLOT_3):
        CASE (EJS_OP_GET_THIS_SLOT_4):
        CASE (EJS_OP_GET_THIS_SLOT_5):
        CASE (EJS_OP_GET_THIS_SLOT_6):
        CASE (EJS_OP_GET_THIS_SLOT_7):
        CASE (EJS_OP_GET_THIS_SLOT_8):
        CASE (EJS_OP_GET_THIS_SLOT_9):
            slotNum = opcode - EJS_OP_GET_THIS_SLOT_0;
            GET_PROPERTY(ejs, NULL, frame->thisObj, slotNum);
            CHECK; BREAK;

        /*
         *  Push a property in an object by slot number
         *      GetObjSlot          <slot>
         *      Stack before (top)  []
         *      Stack after         [value]
         */
        CASE (EJS_OP_GET_OBJ_SLOT):
            slotNum = getByte(frame);
            obj = (EjsObject*) pop(ejs);
            GET_PROPERTY(ejs, NULL, obj, slotNum);
            CHECK; BREAK;

        /*
         *  Push a property in an object from slot 0-9
         *      GetObjSlot0, GetObjSlot1, ... GetObjSlot9
         *      Stack before (top)  []
         *      Stack after         [value]
         */
        CASE (EJS_OP_GET_OBJ_SLOT_0):
        CASE (EJS_OP_GET_OBJ_SLOT_1):
        CASE (EJS_OP_GET_OBJ_SLOT_2):
        CASE (EJS_OP_GET_OBJ_SLOT_3):
        CASE (EJS_OP_GET_OBJ_SLOT_4):
        CASE (EJS_OP_GET_OBJ_SLOT_5):
        CASE (EJS_OP_GET_OBJ_SLOT_6):
        CASE (EJS_OP_GET_OBJ_SLOT_7):
        CASE (EJS_OP_GET_OBJ_SLOT_8):
        CASE (EJS_OP_GET_OBJ_SLOT_9):
            vp = pop(ejs);
            slotNum = opcode - EJS_OP_GET_OBJ_SLOT_0;
            GET_PROPERTY(ejs, NULL, vp, slotNum);
            CHECK; BREAK;


        /*
         *  Push a variable from a type by slot number
         *      GetTypeSlot         <slot> <nthBase>
         *      Stack before (top)  [objRef]
         *      Stack after         [value]
         */
        CASE (EJS_OP_GET_TYPE_SLOT):
            slotNum = getByte(frame);
            obj = (EjsObject*) pop(ejs);
            vp = getNthBase(ejs, (EjsVar*) obj, getNum(frame));
            GET_PROPERTY(ejs, (EjsVar*) obj, vp, slotNum);
            CHECK; BREAK;

        /*
         *  Push a type variable by slot number from this. NthBase counts from Object up rather than "this" down.
         *      GetThisTypeSlot     <slot> <nthBaseFromBottom>
         *      Stack before (top)  []
         *      Stack after         [value]
         */
        CASE (EJS_OP_GET_THIS_TYPE_SLOT):
            slotNum = getNum(frame);
            type = (EjsType*) getNthBaseFromBottom(ejs, frame->thisObj, getNum(frame));
            if (type == 0) {
                ejsThrowReferenceError(ejs, "Bad base class reference");
            } else {
                GET_PROPERTY(ejs, frame->thisObj, type, slotNum);
            }
            CHECK; BREAK;

        /*
         *  Push a variable by an unqualified name
         *      GetScopedName       <qname>
         *      Stack before (top)  []
         *      Stack after         [value]
         */
        CASE (EJS_OP_GET_SCOPED_NAME):
            qname = getNameArg(frame);
            result = ejsGetVarByName(ejs, 0, &qname, 1, &lookup);
            if (result) {
                if (ejsIsFunction(result)) {
                    GET_PROPERTY(ejs, NULL, lookup.obj, lookup.slotNum);
                } else {
                    push(ejs, result);
                }
            } else {
                push(ejs, ejs->nullValue);
            }
            CHECK; BREAK;
                
        /*
         *  Get a property by property name
         *      GetObjName          <qname>
         *      Stack before (top)  [obj]
         *      Stack after         [result]
         */
        CASE (EJS_OP_GET_OBJ_NAME):
            qname = getNameArg(frame);
            vp = pop(ejs);
            result = ejsGetVarByName(ejs, vp, &qname, 1, &lookup);
            if (result) {
                if (ejsIsFunction(result)) {
                    GET_PROPERTY(ejs, vp, lookup.obj, lookup.slotNum);
                } else {
                    push(ejs, result);
                }
            } else {
                push(ejs, ejs->nullValue);
            }
            CHECK; BREAK;

        /*
         *  Get a property by property name expression
         *      GetObjNameExpr
         *      Stack before (top)  [propName]
         *                          [obj]
         *      Stack after         [value]
         */
        CASE (EJS_OP_GET_OBJ_NAME_EXPR):
            v1 = pop(ejs);
            vp = pop(ejs);
            if (vp->type->numericIndicies && ejsIsNumber(v1)) {
                push(ejs, ejsGetProperty(ejs, vp, ejsGetInt(v1)));
                CHECK; BREAK;
            }
            nameVar = ejsToString(ejs, v1);
            if (nameVar) {
                qname.name = nameVar->value;
                qname.space = "";
                result = ejsGetVarByName(ejs, vp, &qname, 1, &lookup);
                if (result) {
                    if (ejsIsFunction(result)) {
                        GET_PROPERTY(ejs, vp, lookup.obj, lookup.slotNum);
                    } else {
                        push(ejs, result);
                    }
                } else {
                    push(ejs, ejs->nullValue);
                }
            }
            CHECK; BREAK;

        /*
         *  Store -------------------------------
         */

        /*
         *  Pop a global variable by slot number
         *      Stack before (top)  [value]
         *      Stack after         []
         *      PutGlobalSlot       <slot>
         */
        CASE (EJS_OP_PUT_GLOBAL_SLOT):
            slotNum = getByte(frame);
            mprAssert(slotNum < 256);
            PUT_PROPERTY(ejs, NULL, global, slotNum);
            CHECK; BREAK;

        /*
         *  Pop a local variable by slot number
         *      Stack before (top)  [value]
         *      Stack after         []
         *      PutLocalSlot        <slot>
         */
        CASE (EJS_OP_PUT_LOCAL_SLOT):
            slotNum = getByte(frame);
            PUT_PROPERTY(ejs, NULL, local, slotNum);
            CHECK; BREAK;

        /*
         *  Pop a local variable from slot 0-9
         *      PutLocalSlot0, PutLocalSlot1, ... PutLocalSlot9
         *      Stack before (top)  [value]
         *      Stack after         []
         */
        CASE (EJS_OP_PUT_LOCAL_SLOT_0):
        CASE (EJS_OP_PUT_LOCAL_SLOT_1):
        CASE (EJS_OP_PUT_LOCAL_SLOT_2):
        CASE (EJS_OP_PUT_LOCAL_SLOT_3):
        CASE (EJS_OP_PUT_LOCAL_SLOT_4):
        CASE (EJS_OP_PUT_LOCAL_SLOT_5):
        CASE (EJS_OP_PUT_LOCAL_SLOT_6):
        CASE (EJS_OP_PUT_LOCAL_SLOT_7):
        CASE (EJS_OP_PUT_LOCAL_SLOT_8):
        CASE (EJS_OP_PUT_LOCAL_SLOT_9):
            slotNum = opcode - EJS_OP_PUT_LOCAL_SLOT_0;
            PUT_PROPERTY(ejs, NULL, local, slotNum);
            CHECK; BREAK;

        /*
         *  Pop a block variable by slot number
         *      PutBlockSlot        <slot> <nthBlock>
         *      Stack before (top)  [value]
         *      Stack after         []
         */
        CASE (EJS_OP_PUT_BLOCK_SLOT):
            slotNum = getByte(frame);
            obj = (EjsObject*) getNthBlock(frame, getNum(frame));
            PUT_PROPERTY(ejs, NULL, obj, slotNum);
            CHECK; BREAK;

#if FUTURE
        /*
         *  Pop a block variable from slot 0-9
         *      PutBlockSlot0, PutBlockSlot1, ... PutBlockSlot9 <nthBlock>
         *      Stack before (top)  [value]
         *      Stack after         []
         */
        CASE (EJS_OP_PUT_BLOCK_SLOT_0):
        CASE (EJS_OP_PUT_BLOCK_SLOT_1):
        CASE (EJS_OP_PUT_BLOCK_SLOT_2):
        CASE (EJS_OP_PUT_BLOCK_SLOT_3):
        CASE (EJS_OP_PUT_BLOCK_SLOT_4):
        CASE (EJS_OP_PUT_BLOCK_SLOT_5):
        CASE (EJS_OP_PUT_BLOCK_SLOT_6):
        CASE (EJS_OP_PUT_BLOCK_SLOT_7):
        CASE (EJS_OP_PUT_BLOCK_SLOT_8):
        CASE (EJS_OP_PUT_BLOCK_SLOT_9):
            slotNum = opcode - EJS_OP_PUT_BLOCK_SLOT_0;
            obj = (EjsObject*) getNthBlock(frame, getNum(frame));
            PUT_PROPERTY(ejs, NULL, obj, slotNum);
            CHECK; BREAK;
#endif

        /*
         *  Store a property by slot number
         *      PutThisSlot         <slot>
         *      Stack before (top)  [value]
         *      Stack after         []
         */
        CASE (EJS_OP_PUT_THIS_SLOT):
            slotNum = getByte(frame);
            PUT_PROPERTY(ejs, NULL, frame->thisObj, slotNum);
            CHECK; BREAK;

        /*
         *  Store a property to slot 0-9
         *      PutThisSlot0, PutThisSlot1, ... PutThisSlot9,
         *      Stack before (top)  [value]
         *      Stack after         []
         */
        CASE (EJS_OP_PUT_THIS_SLOT_0):
        CASE (EJS_OP_PUT_THIS_SLOT_1):
        CASE (EJS_OP_PUT_THIS_SLOT_2):
        CASE (EJS_OP_PUT_THIS_SLOT_3):
        CASE (EJS_OP_PUT_THIS_SLOT_4):
        CASE (EJS_OP_PUT_THIS_SLOT_5):
        CASE (EJS_OP_PUT_THIS_SLOT_6):
        CASE (EJS_OP_PUT_THIS_SLOT_7):
        CASE (EJS_OP_PUT_THIS_SLOT_8):
        CASE (EJS_OP_PUT_THIS_SLOT_9):
            slotNum = opcode - EJS_OP_PUT_THIS_SLOT_0;
            PUT_PROPERTY(ejs, NULL, frame->thisObj, slotNum);
            CHECK; BREAK;

        /*
         *  Store a property by slot number
         *      PutObjSlot          <slot>
         *      Stack before (top)  [obj]
         *                          [value]
         *      Stack after         []
         */
        CASE (EJS_OP_PUT_OBJ_SLOT):
            slotNum = getByte(frame);
            vp = pop(ejs);
            PUT_PROPERTY(ejs, NULL, vp, slotNum);
            CHECK; BREAK;

        /*
         *  Store a property to slot 0-9
         *      PutObjSlot0, PutObjSlot1, ... PutObjSlot9
         *      Stack before (top)  [obj]
         *                          [value]
         *      Stack after         []
         */
        CASE (EJS_OP_PUT_OBJ_SLOT_0):
        CASE (EJS_OP_PUT_OBJ_SLOT_1):
        CASE (EJS_OP_PUT_OBJ_SLOT_2):
        CASE (EJS_OP_PUT_OBJ_SLOT_3):
        CASE (EJS_OP_PUT_OBJ_SLOT_4):
        CASE (EJS_OP_PUT_OBJ_SLOT_5):
        CASE (EJS_OP_PUT_OBJ_SLOT_6):
        CASE (EJS_OP_PUT_OBJ_SLOT_7):
        CASE (EJS_OP_PUT_OBJ_SLOT_8):
        CASE (EJS_OP_PUT_OBJ_SLOT_9):
            slotNum = opcode - EJS_OP_PUT_OBJ_SLOT_0;
            vp = pop(ejs);
            PUT_PROPERTY(ejs, NULL, vp, slotNum);
            CHECK; BREAK;

        /*
         *  Pop a variable by an unqualified name
         *      PutScopedName       <qname>
         *      Stack before (top)  [value]
         *      Stack after         []
         */
        CASE (EJS_OP_PUT_SCOPED_NAME):
            qname = getNameArg(frame);
            storePropertyToScope(ejs, &qname);
            CHECK; BREAK;

        /*
         *  Pop a variable by an unqualified property name expr
         *      PutNameExpr
         *      Stack before (top)  [name]
         *                          [value]
         *      Stack after         []
         */
        //  TODO - this op code looks like its operands are swapped on the stack.
        CASE (EJS_OP_PUT_SCOPED_NAME_EXPR):
            swap2(ejs);
            nameVar = popString(ejs);
            nameVar = ejsToString(ejs, (EjsVar*) nameVar);
            if (nameVar) {
                //  TODO - BUG. Must strdup the name
                qname.name = nameVar->value;
                qname.space = 0;
                storePropertyToScope(ejs, &qname);
            }
            CHECK; BREAK;

        /*
         *  Pop a property by property name to an object
         *      PutObjName
         *      Stack before (top)  [value]
         *                          [objRef]
         *      Stack after         []
         */
        CASE (EJS_OP_PUT_OBJ_NAME):
            qname = getNameArg(frame);
            storeProperty(ejs, pop(ejs), &qname);
            CHECK; BREAK;

        /*
         *  Pop a property by property name expression to an object
         *      PutObjNameExpr
         *      Stack before (top)  [nameExpr]
         *                          [objRef]
         *                          [value]
         *      Stack after         []
         */
        CASE (EJS_OP_PUT_OBJ_NAME_EXPR):
            v1 = pop(ejs);
            vp = pop(ejs);
            if (vp->type->numericIndicies && ejsIsNumber(v1)) {
                ejsSetProperty(ejs, vp, ejsGetInt(v1), pop(ejs));
            } else {
                nameVar = ejsToString(ejs, v1);
                if (nameVar) {
                    //  TODO BUG - not freeing old property name if it was alloced.
                    qname.name = mprStrdup(vp, nameVar->value);
                    qname.space = EJS_PUBLIC_NAMESPACE;
                    storeProperty(ejs, vp, &qname);
                }
            }
            CHECK; BREAK;

        /*
         *  Pop a type variable by slot number
         *      PutTypeSlot         <slot> <nthBase>
         *      Stack before (top)  [obj]
         *                          [value]
         *      Stack after         []
         */
        CASE (EJS_OP_PUT_TYPE_SLOT):
            slotNum = getNum(frame);
            vobj = pop(ejs);
            vp = getNthBase(ejs, vobj, getNum(frame));
            PUT_PROPERTY(ejs, vobj, vp, slotNum);
            CHECK; BREAK;

        /*
         *  Pop a variable to a slot in the nthBase class of the current "this" object
         *      PutThisTypeSlot     <slot> <nthBase>
         *      Stack before (top)  [value]
         *      Stack after         []
         */
        CASE (EJS_OP_PUT_THIS_TYPE_SLOT):
            //  TODO - CLEANUP
            // v1 = pop(ejs);
            slotNum = getNum(frame);
            type = (EjsType*) getNthBaseFromBottom(ejs, frame->thisObj, getNum(frame));
            if (type == 0) {
                ejsThrowReferenceError(ejs, "Bad base class reference");
            } else {
                PUT_PROPERTY(ejs, frame->thisObj, (EjsVar*) type, slotNum);
                // ejsSetProperty(ejs, (EjsVar*) type, slotNum, v1);
            }
            CHECK; BREAK;


        /*
         *  Function calling and return
         */

        /*
         *  Call a function by reference
         *      Stack before (top)  [args]
         *                          [function]
         *                          [obj]
         *      Stack after         []
         */
        CASE (EJS_OP_CALL):
            argc = getNum(frame);
            vp = ejs->stack.top[-argc - 1];
            fun = (EjsFunction*) ejs->stack.top[-argc];
            frame = callFunction(ejs, fun, vp, argc, 2);
            if (frame) {
                local = (EjsObject*) frame->currentFunction;
            }
            CHECK; BREAK;

        /*
         *  Call a global function by slot on the given type
         *      CallGlobalSlot      <slot> <argc>
         *      Stack before (top)  [args]
         *      Stack after         []
         */
        CASE (EJS_OP_CALL_GLOBAL_SLOT):
            slotNum = getNum(frame);
            argc = getNum(frame);
            frame = callFunction(ejs, (EjsFunction*) global->slots[slotNum], ejs->global, argc, 0);
            if (frame) {
                local = (EjsObject*) frame->currentFunction;
            }
            CHECK; BREAK;

        /*
         *  Call a function by slot number on the pushed object
         *      CallObjSlot         <slot> <argc>
         *      Stack before (top)  [args]
         *                          [obj]
         *      Stack after         []
         */
        CASE (EJS_OP_CALL_OBJ_SLOT):
            slotNum = getNum(frame);
            argc = getNum(frame);
            vp = ejs->stack.top[-argc];
            if (vp == 0 || vp == ejs->nullValue) {
                throwNull(ejs);
            } else {
                frame = callFunction(ejs, (EjsFunction*) vp->type->block.obj.slots[slotNum], vp, argc, 1);
                if (frame) {
                    local = (EjsObject*) frame->currentFunction;
                }
            }
            CHECK; BREAK;

        /*
         *  Call a function by slot number on the current this object.
         *      CallThisSlot        <slot> <argc>
         *      Stack before (top)  [args]
         *      Stack after         []
         */
        CASE (EJS_OP_CALL_THIS_SLOT):
            slotNum = getNum(frame);
            argc = getNum(frame);
            if (ejsIsFunction(&frame->function)) {
                obj = (EjsObject*) frame->function.owner;
            } else {
                obj = (EjsObject*) frame->thisObj->type;
                mprAssert(obj != (EjsObject*) ejs->typeType);
            }
            mprAssert(ejsIsObject(obj));
            mprAssert(!ejsIsFunction(obj));
            frame = callFunction(ejs, (EjsFunction*) obj->slots[slotNum], frame->thisObj, argc, 0);
            if (frame) {
                local = (EjsObject*) frame->currentFunction;
            }
            CHECK; BREAK;

        /*
         *  Call a function by slot number on the nth enclosing block
         *      CallBlockSlot        <slot> <nthBlock> <argc>
         *      Stack before (top)  [args]
         *      Stack after         []
         */
        CASE (EJS_OP_CALL_BLOCK_SLOT):
            slotNum = getNum(frame);
            obj = (EjsObject*) getNthBlock(frame, getNum(frame));
            argc = getNum(frame);
            frame = callFunction(ejs, (EjsFunction*) obj->slots[slotNum], frame->thisObj, argc, 0);
            if (frame) {
                local = (EjsObject*) frame->currentFunction;
            }
            CHECK; BREAK;

        /*
         *  Call a function by slot number on an object.
         *      CallObjInstanceSlot <slot> <argc>
         *      Stack before (top)  [args]
         *                          [obj]
         *      Stack after         []
         */
        CASE (EJS_OP_CALL_OBJ_INSTANCE_SLOT):
            slotNum = getNum(frame);
            argc = getNum(frame);
            vp = ejs->stack.top[-argc];
            if (vp == 0 || vp == ejs->nullValue) {
                throwNull(ejs);
            } else {
                frame = callBySlot(ejs, vp, slotNum, vp, argc, 1);
                if (frame) {
                    local = (EjsObject*) frame->currentFunction;
                }
            }
            CHECK; BREAK;

        /*
         *  Call a static function by slot number on the pushed object
         *      CallObjStaticSlot   <slot> <nthBase> <argc>
         *      Stack before (top)  [args]
         *                          [obj]
         *      Stack after         []
         */
        CASE (EJS_OP_CALL_OBJ_STATIC_SLOT):
            slotNum = getNum(frame);
            nthBase = getNum(frame);
            argc = getNum(frame);
            vp = ejs->stack.top[-argc];
            if (vp == 0 || vp == ejs->nullValue) {
                throwNull(ejs);
            } else {
                type = (EjsType*) getNthBase(ejs, vp, nthBase);
                frame = callFunction(ejs, (EjsFunction*) type->block.obj.slots[slotNum], (EjsVar*) type, argc, 1);
                if (frame) {
                    local = (EjsObject*) frame->currentFunction;
                }
            }
            CHECK; BREAK;

        /*
         *  Call a static function by slot number on the nth base class of the current "this" object
         *      CallThisStaticSlot  <slot> <nthBase> <argc>
         *      Stack before (top)  [args]
         *      Stack after         []
         */
        CASE (EJS_OP_CALL_THIS_STATIC_SLOT):
            slotNum = getNum(frame);
            nthBase = getNum(frame);
            argc = getNum(frame);
            type = (EjsType*) getNthBase(ejs, frame->thisObj, nthBase);
            frame = callFunction(ejs, (EjsFunction*) type->block.obj.slots[slotNum], (EjsVar*) type, argc, 0);
            if (frame) {
                local = (EjsObject*) frame->currentFunction;
            }
            CHECK; BREAK;

        /*
         *  Call a function by name on the pushed object
         *      CallObjName         <qname> <argc>
         *      Stack before (top)  [args]
         *                          [obj]
         *      Stack after         []
         */
        CASE (EJS_OP_CALL_OBJ_NAME):
            qname = getNameArg(frame);
            argc = getNum(frame);
            vp = ejs->stack.top[-argc];
            //  TODO - what about undefined?
            if (vp == 0 /* || vp == ejs->nullValue */) {
                throwNull(ejs);
                CHECK; BREAK;
            }
            slotNum = ejsLookupVar(ejs, (EjsVar*) vp, &qname, 1, &lookup);
            if (slotNum < 0) {
                ejsThrowReferenceError(ejs, "Can't find function \"%s\"", qname.name);
            } else {
                frame = callBySlot(ejs, (EjsVar*) lookup.obj, slotNum, vp, argc, 1);
                if (frame) {
                    local = (EjsObject*) frame->currentFunction;
                }
            }
            CHECK; BREAK;

        /*
         *  Call a function by name in the current scope. Use existing "this" object if defined.
         *      CallName            <qname> <argc>
         *      Stack before (top)  [args]
         *      Stack after         []
         */
        CASE (EJS_OP_CALL_SCOPED_NAME):
            qname = getNameArg(frame);
            argc = getNum(frame);
            slotNum = ejsLookupScope(ejs, &qname, 1, &lookup);
            if (slotNum < 0) {
                ejsThrowReferenceError(ejs, "Can't find method %s", qname.name);
                CHECK; BREAK;
            }
            /*
             *  Calculate the "this" to use for the function. If required function is a method in the current "this" object
             *  use the current thisObj. If the lookup.obj is a type, then use it. Otherwise global.
             */
            if (ejsIsA(ejs, frame->thisObj, (EjsType*) lookup.obj)) {
                v1 = frame->thisObj;
            } else if (ejsIsType(lookup.obj)) {
                v1 = lookup.obj;
            } else {
                v1 = ejs->global;
            }
            frame = callBySlot(ejs, lookup.obj, lookup.slotNum, v1, argc, 0);
            if (frame) {
                local = (EjsObject*) frame->currentFunction;
            }
            CHECK; BREAK;

        /*
         *  Call a constructor
         *      CallConstructor     <argc>
         *      Stack before (top)  [args]
         *                          [obj]
         *      Stack after         [obj]
         */
        CASE (EJS_OP_CALL_CONSTRUCTOR):
            argc = getNum(frame);
            vp = ejs->stack.top[-argc];
            if (vp == 0 || vp == ejs->nullValue) {
                throwNull(ejs);
                CHECK; BREAK;
            }
            type = vp->type;
            mprAssert(type);
            if (type && type->hasConstructor) {
                mprAssert(type->baseType);
                //  Constructor is always at slot 0 (offset by base properties)
                slotNum = type->block.numInherited;
                frame = callBySlot(ejs, (EjsVar*) type, slotNum, vp, argc, 0);
                if (frame) {
                    local = (EjsObject*) frame->currentFunction;
                }
            }
            CHECK; BREAK;

        /*
         *  Call the next constructor
         *      CallNextConstructor <argc>
         *      Stack before (top)  [args]
         *      Stack after         []
         */
        CASE (EJS_OP_CALL_NEXT_CONSTRUCTOR):
            argc = getNum(frame);
            fun = &frame->function;
            type = (EjsType*) fun->owner;
            mprAssert(type);
            type = type->baseType;
            if (type) {
                mprAssert(type->hasConstructor);
                slotNum = type->block.numInherited;
                vp = frame->thisObj;
                frame = callBySlot(ejs, (EjsVar*) type, slotNum, vp, argc, 0);
                if (frame) {
                    local = (EjsObject*) frame->currentFunction;
                }
            } else {
                mprAssert(0);
            }
            CHECK; BREAK;

        /*
         *  Add namespace to the set of open namespaces for the current block
         *      AddNamespace <string>
         */
        CASE (EJS_OP_ADD_NAMESPACE):
            /*
             *  TODO this is the namespace URI, missing the name. TODO - unicode.
             */
            str = getStringArg(frame);
            ejsAddNamespaceToBlock(ejs, &frame->function.block, ejsCreateNamespace(ejs, str, str));
            CHECK; BREAK;

        /*
         *  Add namespace reference to the set of open namespaces for the current block. (use namespace).
         *      Stack before (top)  [namespace]
         *      Stack after         []
         *      AddNamespaceRef
         */
        CASE (EJS_OP_ADD_NAMESPACE_REF):
            ejsAddNamespaceToBlock(ejs, &frame->function.block, (EjsNamespace*) pop(ejs));
            CHECK; BREAK;

        /*
         *  Push a new scope block on the scope chain
         *      OpenBlock <slotNum> <nthBlock>
         */
        CASE (EJS_OP_OPEN_BLOCK):
            slotNum = getNum(frame);
            vp = getNthBlock(frame, getNum(frame));
            v1 = ejsGetProperty(ejs, vp, slotNum);
            if (v1 == 0 || !ejsIsBlock(v1)) {
                ejsThrowReferenceError(ejs, "Reference is not a block");
                CHECK; BREAK;
            }
            frame = ejsPushFrame(ejs, (EjsBlock*) v1);
            CHECK; BREAK;

        /*
         *  Add a new scope block from the stack onto on the scope chain
         *      OpenWith
         */
        CASE (EJS_OP_OPEN_WITH):
            frame = ejsPushFrame(ejs, varToBlock(ejs, pop(ejs)));
            CHECK; BREAK;

        /*
         *  Pop the top scope block off the scope chain
         *      CloseBlock
         *      CloseWith
         *  TODO - merge these
         */
        CASE (EJS_OP_CLOSE_BLOCK):
        CASE (EJS_OP_CLOSE_WITH):
            frame = ejsPopFrame(ejs);
            CHECK; BREAK;

        /*
         *  Define a class and initialize by calling any static initializer.
         *      DefineClass <type>
         */
        CASE (EJS_OP_DEFINE_CLASS):
            type = getType(frame);
            if (type == 0 || !ejsIsType(type)) {
                ejsThrowReferenceError(ejs, "Reference is not a class");
            } else {
                needClosure(frame, (EjsBlock*) type);
                if (type && type->hasStaticInitializer) {
                    mprAssert(type->baseType);
                    //  Initializer is always immediately after the constructor (if present)
                    slotNum = type->block.numInherited;
                    if (type->hasConstructor) {
                        slotNum++;
                    }
                    frame = callBySlot(ejs, (EjsVar*) type, slotNum, ejs->global, 0, 0);
                    if (frame) {
                        local = (EjsObject*) frame->currentFunction;
                    }
                }
            }
            ejs->result = (EjsVar*) type;
            CHECK; BREAK;

        /*
         *  Define a function. This is used for non-method functions.
         *      DefineFunction <slot> <nthBlock>
         */
        CASE (EJS_OP_DEFINE_FUNCTION):
            slotNum = getNum(frame);
            vp = getNthBlock(frame, getNum(frame));
            mprAssert(vp != ejs->global);
            fun = (EjsFunction*) ejsGetProperty(ejs, vp, slotNum);
            if (fun == 0 || !ejsIsFunction(fun)) {
                ejsThrowReferenceError(ejs, "Reference is not a function");
            } else {
                if (fun->fullScope) {
                    needClosure(frame, (EjsBlock*) fun);
                    fun->thisObj = frame->thisObj;
                }
            }
            CHECK; BREAK;

        /*
         *  Define a global function
         *      DefineGlobalFunction <Global>
         */
        CASE (EJS_OP_DEFINE_GLOBAL_FUNCTION):
            fun = (EjsFunction*) getGlobalArg(frame);
            if (fun == 0 || !ejsIsFunction(fun)) {
                ejsThrowReferenceError(ejs, "Global reference is not a function");
            } else {
                /* 
                 *  Add or 1 because function (a=a) is not binding RHS correctly
                 */
                if (fun->fullScope || 1) {
                    needClosure(frame, (EjsBlock*) fun);
                }
            }
            CHECK; BREAK;

        /*
         *  Exception Handling --------------------------------------------
         */

        /*
         *  End of an exception block
         *      EndException
         */
        CASE (EJS_OP_END_EXCEPTION):
            if (frame->inCatch || frame->inFinally) {
                frame = popExceptionFrame(ejs);
                frame->pc = frame->endException;
            }
            CHECK; BREAK;

        /*
         *  Throw an exception
         *      Stack before (top)  [exceptionObj]
         *      Stack after         []
         *      Throw
         */
        CASE (EJS_OP_THROW):
            ejs->exception = pop(ejs);
            ejs->attention = 1;
            CHECK; BREAK;

        /*
         *  Stack management ----------------------------------------------
         */

        /*
         *  Pop one item off the stack
         *      Pop
         *      Stack before (top)  [value]
         *      Stack after         []
         */
        CASE (EJS_OP_POP):
            ejs->result = pop(ejs);
            BREAK;

        /*
         *  Pop N items off the stack
         *      PopItems            <count>
         *      Stack before (top)  [value]
         *                          [...]
         *      Stack after         []
         */
        CASE (EJS_OP_POP_ITEMS):
            ejs->stack.top -= getByte(frame);
            ejs->result = ejs->stack.top[1];
            BREAK;

        /*
         *  Duplicate one item on the stack
         *      Stack before (top)  [value]
         *      Stack after         [value]
         *                          [value]
         */
        CASE (EJS_OP_DUP):
            v1 = ejs->stack.top[0];
            push(ejs, v1);
            BREAK;

        /*
         *  Duplicate two items on the stack
         *      Dup2
         *      Stack before (top)  [value1]
         *                          [value2]
         *      Stack after         [value1]
         *                          [value2]
         *                          [value1]
         *                          [value2]
         */
        CASE (EJS_OP_DUP2):
            v1 = ejs->stack.top[-1];
            push(ejs, v1);
            v1 = ejs->stack.top[0];
            push(ejs, v1);
            BREAK;

        /*
         *  Swap the top two items on the stack
         *      Swap
         *      Stack before (top)  [value1]
         *                          [value2]
         *      Stack after         [value2]
         *                          [value1]
         */
        CASE (EJS_OP_SWAP):
            swap2(ejs);
            BREAK;

        /*
         *  Branching
         */

        /*
         *  Default function argument initialization. Computed goto for functions with more than 256 parameters.
         *      InitDefault         <tableSize>
         */
        CASE (EJS_OP_INIT_DEFAULT_ARGS): {
            int tableSize, numNonDefault;
            /*
             *  Use the argc value for the current function. Compare with the number of default args.
             */
            tableSize = (char) getByte(frame);
            fun = &frame->function;
            numNonDefault = fun->numArgs - fun->numDefault;
            mprAssert(numNonDefault >= 0);
            offset = frame->argc - numNonDefault;
            if (offset < 0 || offset > tableSize) {
                offset = tableSize;
            }
            //  TODO - ENDIAN
            frame->pc += ((uint*) frame->pc)[offset];
            CHECK; BREAK;
        }

        /*
         *  Default function argument initialization. Computed goto for functions with less than 256 parameters.
         *      InitDefault.8       <tableSize.8>
         */
        CASE (EJS_OP_INIT_DEFAULT_ARGS_8): {
            int tableSize, numNonDefault;

            tableSize = (char) getByte(frame);
            fun = &frame->function;
            numNonDefault = fun->numArgs - fun->numDefault;
            mprAssert(numNonDefault >= 0);
            offset = frame->argc - numNonDefault;
            if (offset < 0 || offset > tableSize) {
                offset = tableSize;
            }
            frame->pc += frame->pc[offset];
            CHECK; BREAK;
        }

        /*
         *  Unconditional branch to a new location
         *      Goto                <offset.32>
         */
        CASE (EJS_OP_GOTO):
            offset = getWord(frame);
            frame->pc = &frame->pc[offset];
            BREAK;

        /*
         *  Unconditional branch to a new location (8 bit)
         *      Goto.8              <offset.8>
         */
        CASE (EJS_OP_GOTO_8):
            offset = (char) getByte(frame);
            frame->pc = &frame->pc[offset];
            BREAK;

        /*
         *  Branch to offset if false
         *      BranchFalse
         *      Stack before (top)  [boolValue]
         *      Stack after         []
         */
        CASE (EJS_OP_BRANCH_FALSE):
            offset = getWord(frame);
            goto commonBoolBranchCode;

        /*
         *  Branch to offset if true
         *      BranchTrue
         *      Stack before (top)  [boolValue]
         *      Stack after         []
         */
        CASE (EJS_OP_BRANCH_TRUE):
            offset = getWord(frame);
            goto commonBoolBranchCode;

        /*
         *  Branch to offset if false (8 bit)
         *      BranchFalse.8
         *      Stack before (top)  [boolValue]
         *      Stack after         []
         */
        CASE (EJS_OP_BRANCH_FALSE_8):
            opcode = (EjsOpCode) (opcode - EJS_OP_BRANCH_TRUE_8 + EJS_OP_BRANCH_TRUE);
            offset = (char) getByte(frame);
            goto commonBoolBranchCode;

        /*
         *  Branch to offset if true (8 bit)
         *      BranchTrue.8
         *      Stack before (top)  [boolValue]
         *      Stack after         []
         */
        CASE (EJS_OP_BRANCH_TRUE_8):
            /* We want sign extension here */
            opcode = (EjsOpCode) (opcode - EJS_OP_BRANCH_TRUE_8 + EJS_OP_BRANCH_TRUE);
            offset = (char) getByte(frame);

        /*
         *  Common boolean branch code
         */
        commonBoolBranchCode:
            v1 = pop(ejs);
            if (v1 == 0 || !ejsIsBoolean(v1)) {
                v1 = ejsCastVar(ejs, v1, ejs->booleanType);
                if (ejs->exception) {
                    CHECK; BREAK;
                }
            }
            if (!ejsIsBoolean(v1)) {
                ejsThrowTypeError(ejs, "Result of a comparision must be boolean");
                CHECK; BREAK;
            }
            if (opcode == EJS_OP_BRANCH_TRUE) {
                if (((EjsBoolean*) v1)->value) {
                    frame->pc = &frame->pc[offset];
                }
            } else {
                if (((EjsBoolean*) v1)->value == 0) {
                    frame->pc = &frame->pc[offset];
                }
            }
            BREAK;

        /*
         *  Branch to offset if [value1] == null
         *      BranchNull
         *      Stack before (top)  [boolValue]
         *      Stack after         []
         */
        CASE (EJS_OP_BRANCH_NULL):
            push(ejs, ejs->nullValue);
            offset = getWord(frame);
            goto commonBranchCode;

        /*
         *  Branch to offset if [value1] == undefined
         *      BranchUndefined
         *      Stack before (top)  [boolValue]
         *      Stack after         []
         */
        CASE (EJS_OP_BRANCH_UNDEFINED):
            push(ejs, ejs->undefinedValue);
            offset = getWord(frame);
            goto commonBranchCode;

        /*
         *  Branch to offset if [tos] value is zero
         *      BranchZero
         *      Stack before (top)  [boolValue]
         *      Stack after         []
         */
        CASE (EJS_OP_BRANCH_ZERO):
            /* Fall through */

        /*
         *  Branch to offset if [tos] value is not zero
         *      BranchNotZero
         *      Stack before (top)  [boolValue]
         *      Stack after         []
         */
        CASE (EJS_OP_BRANCH_NOT_ZERO):
            //  TODO - need a pre-created zero value
            push(ejs, ejsCreateNumber(ejs, 0));
            offset = getWord(frame);
            goto commonBranchCode;

        /*
         *  Branch to offset if [value1] == [value2]
         *      BranchEQ
         *      Stack before (top)  [value1]
         *      Stack before (top)  [value2]
         *      Stack after         []
         */
        CASE (EJS_OP_BRANCH_EQ):

        /*
         *  Branch to offset if [value1] === [value2]
         *      BranchStrictlyEQ
         *      Stack before (top)  [value1]
         *      Stack after         [value2]
         */
        CASE (EJS_OP_BRANCH_STRICTLY_EQ):

        /*
         *  Branch to offset if [value1] != [value2]
         *      BranchNotEqual
         *      Stack before (top)  [value1]
         *                          [value2]
         *      Stack after         []
         */
        CASE (EJS_OP_BRANCH_NE):

        /*
         *  Branch to offset if [value1] !== [value2]
         *      BranchStrictlyNE
         *      Stack before (top)  [value1]
         *                          [value2]
         *      Stack after         []
         */
        CASE (EJS_OP_BRANCH_STRICTLY_NE):

        /*
         *  Branch to offset if [value1] <= [value2]
         *      BranchLE
         *      Stack before (top)  [value1]
         *                          [value2]
         *      Stack after         []
         */
        CASE (EJS_OP_BRANCH_LE):

        /*
         *  Branch to offset if [value1] < [value2]
         *      BranchLT
         *      Stack before (top)  [value1]
         *                          [value2]
         *      Stack after         []
         */
        CASE (EJS_OP_BRANCH_LT):

        /*
         *  Branch to offset if [value1] >= [value2]
         *      BranchGE
         *      Stack before (top)  [value1]
         *                          [value2]
         *      Stack after         []
         */
        CASE (EJS_OP_BRANCH_GE):

        /*
         *  Branch to offset if [value1] > [value2]
         *      BranchGT
         *      Stack before (top)  [value1]
         *                          [value2]
         *      Stack after         []
         */
        CASE (EJS_OP_BRANCH_GT):
            offset = getWord(frame);
            goto commonBranchCode;

        /*
         *  Handle all branches here. We convert to a compare opcode and pass to the type to handle.
         */
        commonBranchCode:
            opcode = (EjsOpCode) (opcode - EJS_OP_BRANCH_EQ + EJS_OP_COMPARE_EQ);
            v2 = pop(ejs);
            v1 = pop(ejs);
            result = evalBinaryExpr(ejs, v1, opcode, v2);
            if (!ejsIsBoolean(result)) {
                ejsThrowTypeError(ejs, "Result of a comparision must be boolean");
                CHECK;
            } else {
                if (((EjsBoolean*) result)->value) {
                    frame->pc = &frame->pc[offset];
                }
            }
            BREAK;

        /*
         *  Compare if [value1] == true
         *      CompareTrue
         *      Stack before (top)  [value]
         *      Stack after         [boolean]
         */
        CASE (EJS_OP_COMPARE_TRUE):

        /*
         *  Compare if ![value1]
         *      CompareNotTrue
         *      Stack before (top)  [value]
         *      Stack after         [boolean]
         */
        CASE (EJS_OP_COMPARE_FALSE):
            v1 = pop(ejs);
            result = evalUnaryExpr(ejs, v1, opcode);
            push(ejs, result);
            CHECK; BREAK;

        /*
         *  Compare if [value1] == NULL
         *      CompareNull
         *      Stack before (top)  [value]
         *      Stack after         [boolean]
         */
        CASE (EJS_OP_COMPARE_NULL):
            push(ejs, ejs->nullValue);
            goto binaryExpression;

        /*
         *  Compare if [item] == undefined
         *      CompareUndefined
         *      Stack before (top)  [value]
         *      Stack after         [boolean]
         */
        CASE (EJS_OP_COMPARE_UNDEFINED):
            push(ejs, ejs->undefinedValue);
            goto binaryExpression;

        /*
         *  Compare if [item] value is zero
         *      CompareZero
         *      Stack before (top)  [value]
         *      Stack after         [boolean]
         */
        CASE (EJS_OP_COMPARE_ZERO):
            push(ejs, ejsCreateNumber(ejs, 0));
            goto binaryExpression;

        /*
         *  Compare if [tos] value is not zero
         *      CompareZero
         *      Stack before (top)  [value]
         *      Stack after         [boolean]
         */
        CASE (EJS_OP_COMPARE_NOT_ZERO):
            push(ejs, ejsCreateNumber(ejs, 0));
            goto binaryExpression;

        /*
         *  Compare if [value1] == [item2]
         *      CompareEQ
         *      Stack before (top)  [value1]
         *                          [value2]
         *      Stack after         [boolean]
         */
        CASE (EJS_OP_COMPARE_EQ):

        /*
         *  Compare if [value1] === [item2]
         *      CompareStrictlyEQ
         *      Stack before (top)  [value1]
         *                          [value2]
         *      Stack after         [boolean]
         */
        CASE (EJS_OP_COMPARE_STRICTLY_EQ):

        /*
         *  Compare if [value1] != [item2]
         *      CompareNE
         *      Stack before (top)  [value1]
         *                          [value2]
         *      Stack after         [boolean]
         */
        CASE (EJS_OP_COMPARE_NE):

        /*
         *  Compare if [value1] !== [item2]
         *      CompareStrictlyNE
         *      Stack before (top)  [value1]
         *                          [value2]
         *      Stack after         [boolean]
         */
        CASE (EJS_OP_COMPARE_STRICTLY_NE):

        /*
         *  Compare if [value1] <= [item2]
         *      CompareLE
         *      Stack before (top)  [value1]
         *                          [value2]
         *      Stack after         [boolean]
         */
        CASE (EJS_OP_COMPARE_LE):

        /*
         *  Compare if [value1] < [item2]
         *      CompareStrictlyLT
         *      Stack before (top)  [value1]
         *                          [value2]
         *      Stack after         [boolean]
         */
        CASE (EJS_OP_COMPARE_LT):

        /*
         *  Compare if [value1] >= [item2]
         *      CompareGE
         *      Stack before (top)  [value1]
         *                          [value2]
         *      Stack after         [boolean]
         */
        CASE (EJS_OP_COMPARE_GE):

        /*
         *  Compare if [value1] > [item2]
         *      CompareGT
         *      Stack before (top)  [value1]
         *                          [value2]
         *      Stack after         [boolean]
         */
        CASE (EJS_OP_COMPARE_GT):

        /*
         *  Binary expressions
         *      Stack before (top)  [right]
         *                          [left]
         *      Stack after         [boolean]
         */
        CASE (EJS_OP_ADD):
        CASE (EJS_OP_SUB):
        CASE (EJS_OP_MUL):
        CASE (EJS_OP_DIV):
        CASE (EJS_OP_REM):
        CASE (EJS_OP_SHL):
        CASE (EJS_OP_SHR):
        CASE (EJS_OP_USHR):
        CASE (EJS_OP_AND):
        CASE (EJS_OP_OR):
        CASE (EJS_OP_XOR):
        binaryExpression:
            v2 = pop(ejs);
            v1 = pop(ejs);
            ejs->result = evalBinaryExpr(ejs, v1, opcode, v2);
            push(ejs, ejs->result);
            CHECK; BREAK;

        /*
         *  Unary operators
         */

        /*
         *  Negate a value
         *      Neg
         *      Stack before (top)  [value]
         *      Stack after         [result]
         */
        CASE (EJS_OP_NEG):
            v1 = pop(ejs);
            result = evalUnaryExpr(ejs, v1, opcode);
            push(ejs, result);
            CHECK; BREAK;

        /*
         *  Logical not (~value)
         *      LogicalNot
         *      Stack before (top)  [value]
         *      Stack after         [result]
         */
        CASE (EJS_OP_LOGICAL_NOT):
            v1 = pop(ejs);
            v1 = ejsCastVar(ejs, v1, ejs->booleanType);
            result = evalUnaryExpr(ejs, v1, opcode);
            push(ejs, result);
            CHECK; BREAK;

        /*
         *  Bitwise not (!value)
         *      BitwiseNot
         *      Stack before (top)  [value]
         *      Stack after         [result]
         */
        CASE (EJS_OP_NOT):
            v1 = pop(ejs);
            result = evalUnaryExpr(ejs, v1, opcode);
            push(ejs, result);
            CHECK; BREAK;

        /*
         *  Increment a stack variable
         *      Inc                 <increment>
         *      Stack before (top)  [value]
         *      Stack after         [result]
         */
        CASE (EJS_OP_INC):
            v1 = pop(ejs);
            count = (char) getByte(frame);
            result = evalBinaryExpr(ejs, v1, EJS_OP_ADD, (EjsVar*) ejsCreateNumber(ejs, count));
            push(ejs, result);
            CHECK; BREAK;

        /*
         *  Object creation
         */

        /*
         *  Create a new object
         *      New
         *      Stack before (top)  [type]
         *      Stack after         [obj]
         */
        CASE (EJS_OP_NEW):
            type = (EjsType*) pop(ejs);
            if (type == 0 || (EjsVar*) type == (EjsVar*) ejs->undefinedValue || !ejsIsType(type)) {
                ejsThrowReferenceError(ejs, "Can't locate type");
            } else {
                v1 = ejsCreateVar(ejs, type, 0);
                push(ejs, v1);
                ejs->result = v1;
            }
            CHECK; BREAK;

        /*
         *  Create a new object literal
         *      NewObject           <type> <argc.
         *      Stack before (top)  [args]
         *      Stack after         []
         */
        CASE (EJS_OP_NEW_OBJECT):
            //  TODO - what values need to be tested for success
            type = getType(frame);
            argc = getNum(frame);
            vp = (EjsVar*) ejsCreateObject(ejs, type, 0);
            for (i = 1 - (argc * 2); i <= 0; ) {
                nameVar = (EjsString*) ejs->stack.top[i++];
                v1 = ejs->stack.top[i++];
                if (v1 && nameVar) {
                    if (ejsIsFunction(v1)) {
                        fun = (EjsFunction*) v1;
                        if (fun->literalGetter) {
                            fun->getter = 1;
                        }
                    }
                    nameVar = ejsToString(ejs, (EjsVar*) nameVar);
                    ejsName(&qname, "", mprStrdup(vp, nameVar->value));
                    ejsSetPropertyByName(ejs, vp, &qname, v1);
                }
            }
            ejs->stack.top -= (argc * 2);
            push(ejs, vp);
            CHECK; BREAK;

        /*
         *  Reference the super class
         *      Super
         *      Stack before (top)  [obj]
         *      Stack after         [type]
         */
        CASE (EJS_OP_SUPER):
            //  TODO - not implemented
            mprAssert(0);
#if OLD && TODO
            vp = pop(ejs);
            if (ejsIsType(obj)) {
                push(ejs, ((EjsType*) obj)->baseType);
            } else {
                push(ejs, obj->type->baseType);
            }
            frame = callConstructors(ejs, type, obj, 0);
            if (frame) {
                local = (EjsObject*) frame->currentFunction;
            }
            CHECK; BREAK;
#endif

        /*
         *  Delete a property
         *      Delete              <qname>
         *      Stack before (top)  [obj]
         *      Stack after         []
         */
        CASE (EJS_OP_DELETE):
            vp = pop(ejs);
            qname = getNameArg(frame);
            slotNum = ejsDeletePropertyByName(ejs, vp, &qname);
            if (slotNum < 0) {
                ejsThrowReferenceError(ejs, "Property \"%s\" does not exist", qname.name);
            }
            CHECK; BREAK;

        /*
         *  Delete an object property by name expression
         *      DeleteyNameExpr
         *      Stack before (top)  [obj]
         *                          [nameValue]
         *      Stack after         []
         */
        CASE (EJS_OP_DELETE_NAME_EXPR):
            nameVar = ejsToString(ejs, pop(ejs));
            vp = pop(ejs);
            ejsName(&qname, "", nameVar->value);
            slotNum = ejsLookupVar(ejs, vp, &qname, 1, &lookup);
            if (slotNum < 0) {
                ejsThrowReferenceError(ejs, "Property \"%s\" does not exist", qname.name);
            } else {
                slotNum = ejsDeleteProperty(ejs, vp, slotNum);
            }
            CHECK; BREAK;

        /*
         *  Delete an object property from the current scope
         *      DeleteName          <name>
         *      Stack before (top)  [obj]
         *      Stack after         []
         */
        CASE (EJS_OP_DELETE_NAME):
            qname = getNameArg(frame);
            slotNum = ejsLookupScope(ejs, &qname, 1, &lookup);
            if (slotNum < 0) {
                ejsThrowReferenceError(ejs, "Property \"%s\" does not exist", qname.name);
            } else {
                ejsDeleteProperty(ejs, lookup.obj, slotNum);
            }
            CHECK; BREAK;


        /*
         *  No operation. Does nothing.
         *      Nop
         */
        CASE (EJS_OP_NOP):
            BREAK;

        /*
         *  Check if object is an instance of a given type. TODO - this is currently just the same as IS_A
         *      InstanceOf          <type>
         *      Stack before (top)  [type]
         *                          [obj]
         *      Stack after         [boolean]
         */
        CASE (EJS_OP_INST_OF):
            type = (EjsType*) pop(ejs);
            v1 = pop(ejs);
            push(ejs, ejsCreateBoolean(ejs, ejsIsA(ejs, v1, type)));
            BREAK;

        /*
         *  Get the type of an object.
         *      TypeOf              <obj>
         *      Stack before (top)  [obj]
         *      Stack after         [string]
         */
        CASE (EJS_OP_TYPE_OF):
            v1 = pop(ejs);
            push(ejs, ejsGetTypeOf(ejs, v1));
            BREAK;

        /*
         *  Check if object is a given type. TODO - Like should be implemented differently.
         *      IsA, Like
         *      Stack before (top)  [type]
         *                          [obj]
         *      Stack after         [boolean]
         */
        CASE (EJS_OP_IS_A):
        CASE (EJS_OP_LIKE):
            type = (EjsType*) pop(ejs);
            v1 = pop(ejs);
            push(ejs, ejsCreateBoolean(ejs, ejsIsA(ejs, v1, type)));
            BREAK;

        /*
         *  Cast an object to the given the type. Throw if not castable.
         *      Cast
         *      Stack before (top)  [type]
         *                          [obj]
         *      Stack after         [result]
         */
        CASE (EJS_OP_CAST):
            type = (EjsType*) pop(ejs);
            if (!ejsIsType(type)) {
                ejsThrowTypeError(ejs, "Not a type");
            } else {
                v1 = pop(ejs);
                push(ejs, ejsCastVar(ejs, v1, type));
            }
            CHECK; BREAK;

        /*
         *  Cast to a boolean type
         *      CastBoolean
         *      Stack before (top)  [value]
         *      Stack after         [result]
         */
        CASE (EJS_OP_CAST_BOOLEAN):
            v1 = ejsCastVar(ejs, pop(ejs), ejs->booleanType);
            push(ejs, v1);
            CHECK; BREAK;

        /*
         *  Test if a given name is the name of a property "in" an object
         *      Cast
         *      Stack before (top)  [obj]
         *                          [name]
         *      Stack after         [result]
         */
        CASE (EJS_OP_IN):
            v1 = pop(ejs);
            nameVar = ejsToString(ejs, pop(ejs));
            if (nameVar == 0) {
                ejsThrowTypeError(ejs, "Can't convert to a name");
            } else {
                ejsName(&qname, "", nameVar->value);                        //  Don't consult namespaces
                slotNum = ejsLookupProperty(ejs, v1, &qname);
                if (slotNum < 0) {
                    slotNum = ejsLookupVar(ejs, v1, &qname, 1, &lookup);
                    if (slotNum < 0 && ejsIsType(v1)) {
                        slotNum = ejsLookupVar(ejs, (EjsVar*) ((EjsType*) v1)->instanceBlock, &qname, 1, &lookup);
                    }
                }
                push(ejs, ejsCreateBoolean(ejs, slotNum >= 0));
            }
            CHECK; BREAK;

        /*
         *  Unimplemented op codes
         */
        CASE (EJS_OP_BREAKPOINT):
            mprAssert(0);
            BREAK;

        /*
         *  Create a new array literal
         *      NewArray            <type> <argc.
         *      Stack before (top)  [args]
         *      Stack after         []
         */
        CASE (EJS_OP_NEW_ARRAY):
            mprAssert(0);
            CHECK; BREAK;

        /*
         *  Push a qualified name constant (TODO - Not implemented)
         *      LoadName            <qname>
         *      Stack before (top)  []
         *      Stack after         [String]
         */
        CASE (EJS_OP_LOAD_NAME):
            mprAssert(0);
            qname = getNameArg(frame);
            push(ejs, ejs->undefinedValue);
            CHECK; BREAK;

        nullException:
            ejsThrowReferenceError(ejs, "Object reference is null");
            if ((frame = payAttention(ejs)) == 0) {
                return;
            }
            local = (EjsObject*) frame->currentFunction;
            BREAK;

        slotReferenceException:
            /*
             *  Callers must setup slotNum
             */
            ejsThrowReferenceError(ejs, "Property at slot \"%d\" is not found", slotNum);
            if ((frame = payAttention(ejs)) == 0) {
                return;
            }
            local = (EjsObject*) frame->currentFunction;
            BREAK;

#if !LINUX && !MACOSX && !LINUX && !SOLARIS && !VXWORKS
        }
        if (ejs->attention && (frame = payAttention(ejs)) == 0) {
            return;
        }
        local = (EjsObject*) frame->currentFunction;
    }
#endif
}



/*
 *  Attend to unusual circumstances. Memory allocation errors, exceptions and forced exits all set the attention flag.
 */
static EjsFrame *payAttention(Ejs *ejs)
{
    ejs->attention = 0;

    if (ejs->gc.required) {
        ejsCollectGarbage(ejs, EJS_GC_SMART);
    }
    if (mprHasAllocError(ejs)) {
        ejsThrowMemoryError(ejs);
        ejs->attention = 1;
        return ejs->frame;
    }
    if (ejs->exception) {
        if (!handleException(ejs)) {
            ejs->attention = 1;
            return 0;
        }
    }
    if (ejs->exception) {
        /*
         *  Unhandled exception
         */
        popFrameAndReturn(ejs);
        ejs->attention = 1;
        return 0;
    }
    if (ejs->frame->depth > EJS_MAX_RECURSION) {
        ejsThrowInternalError(ejs, "Recursion limit exceeded");
        return 0;
    }
    return ejs->frame;
}



/*
 *  Run the module initializer
 */
EjsVar *ejsRunInitializer(Ejs *ejs, EjsModule *mp)
{
    EjsModule   *dp;
    EjsVar      *result;
    int         next;

    if (mp->initialized || !mp->hasInitializer) {
        mp->initialized = 1;
        return ejs->nullValue;
    }
    mp->initialized = 1;

    if (mp->dependencies) {
        for (next = 0; (dp = (EjsModule*) mprGetNextItem(mp->dependencies, &next)) != 0;) {
            if (dp->hasInitializer && !dp->initialized) {
                if (ejsRunInitializer(ejs, dp) == 0) {
                    mprAssert(ejs->exception);
                    return 0;
                }
            }
        }
    }

    mprLog(ejs, 6, "Running initializer for module %s", mp->name);

    result = ejsRunFunction(ejs, mp->initializer, ejs->global, 0, NULL);

    ejsMakeTransient(ejs, (EjsVar*) mp->initializer);
    
    return result;
}


/*
 *  Run all initializers for all modules
 */
int ejsRun(Ejs *ejs)
{
    EjsModule   *mp;
    int         next;

    for (next = 0; (mp = (EjsModule*) mprGetNextItem(ejs->modules, &next)) != 0;) {
        if (ejsRunInitializer(ejs, mp) == 0) {
            mprAssert(ejs->exception);
            return EJS_ERR;
        }
    }
    return 0;
}


/*
 *  Run a function with the given parameters
 */
EjsVar *ejsRunFunction(Ejs *ejs, EjsFunction *fun, EjsVar *thisObj, int argc, EjsVar **argv)
{
    EjsFrame    *frame, *prev;
    int         i;
    
    mprAssert(ejs);
    mprAssert(fun);
    mprAssert(ejsIsFunction(fun));

    if (fun->thisObj) {
        /*
         *  Closure value of thisObj
         */
        thisObj = fun->thisObj;
    }

    if (ejsIsNativeFunction(fun)) {
        mprAssert(fun->body.proc);
        ejs->result = (fun->body.proc)(ejs, thisObj, argc, argv);

    } else {
        mprAssert(fun->body.code.byteCode);
        mprAssert(fun->body.code.codeLen > 0);
        
        for (i = 0; i < argc; i++) {
            push(ejs, argv[i]);
        }
        
        /*
         *  This will setup to call the function, but not interpret any byte codes yet.
         */
        prev = ejs->frame;
        frame = callFunction(ejs, fun, thisObj, argc, 0);
        if (ejs->exception == 0) {
            frame->returnFrame = 1;
            vmLoop(ejs);
            mprAssert(ejs->frame == prev);
        }
    }
    return (ejs->exception) ? 0 : ejs->result;
}


/*
 *  Run a function by slot.
 */
EjsVar *ejsRunFunctionBySlot(Ejs *ejs, EjsVar *obj, int slotNum, int argc, EjsVar **argv)
{
    EjsFunction     *fun;

    if (obj == 0) {
        mprAssert(0);
        return 0;
    }

    if (obj == ejs->global) {
        fun = (EjsFunction*) ejsGetProperty(ejs, obj, slotNum);
    } else {
        fun = (EjsFunction*) ejsGetProperty(ejs, ejsIsType(obj) ? obj : (EjsVar*) obj->type, slotNum);
    }
    if (fun == 0) {
        return 0;
    }
    return ejsRunFunction(ejs, fun, obj, argc, argv);
}


/*
 *  Validate the args. This routine handles ...rest args and parameter type checking and casts. Returns the new argc 
 *  or < 0 on errors.
 */
static int validateArgs(Ejs *ejs, EjsFunction *fun, int argc, EjsVar **argv)
{
    EjsTrait        *trait;
    EjsVar          *newArg;
    EjsArray        *rest;
    int             nonDefault, i, limit, numRest;

    nonDefault = fun->numArgs - fun->numDefault;

    if (argc < nonDefault) {
        if (!fun->rest || argc != (fun->numArgs - 1)) {
            if (fun->lang < EJS_SPEC_FIXED) {
                /*
                 *  Create undefined values for missing args
                 */
                for (i = argc; i < nonDefault; i++) {
                    push(ejs, ejs->undefinedValue);
                }
                argc = nonDefault;

            } else {
                /*
                 *  We are currently allowing too few arguments and supply "undefined" for the missing parameters
                 */
                ejsThrowArgError(ejs, "Insufficient actual parameters. Call requires %d parameter(s).", nonDefault);
                return EJS_ERR;
            }
        }
    }

    if ((uint) argc > fun->numArgs && !fun->rest) {
        /*
         *  TODO - Array match functions to Array.every take 3 parameters. Yet most users want to be able to supply fewer
         *  actual parameters and ignore the others. So let's just discard the args here.
         */
        if (1 || fun->lang < EJS_SPEC_FIXED) {
            /*
             *  TODO - currently enabled for all language spec levels.
             *  Discard excess arguments
             */
            ejs->stack.top -=  (argc - fun->numArgs);
            argc = fun->numArgs;

        } else {
            /*
             *  We are currently allowing too many arguments.
             */
            ejsThrowArgError(ejs, "Too many actual parameters. Call accepts at most %d parameter(s).", fun->numArgs);
            return EJS_ERR;
        }
    }

    /*
     *  Handle rest "..." args
     */
    if (fun->rest) {
        numRest = argc - fun->numArgs + 1;
        rest = ejsCreateArray(ejs, numRest);
        if (rest == 0) {
            return EJS_ERR;
        }
        for (i = numRest - 1; i >= 0; i--) {
            ejsSetProperty(ejs, (EjsVar*) rest, i, pop(ejs));
        }
        argc = argc - numRest + 1;
        push(ejs, rest);
    }

    if (fun->block.numTraits == 0) {
        return argc;
    }

    mprAssert((uint) fun->block.numTraits >= fun->numArgs);

    /*
     *  Cast args to the right types
     */
    limit = min((uint) argc, fun->numArgs);
    for (i = 0; i < limit; i++) {
        trait = ejsGetTrait((EjsBlock*) fun, i);
        if (trait->type && !ejsIsA(ejs, argv[i], trait->type) && argv[i] != ejs->nullValue) {
            newArg = ejsCastVar(ejs, argv[i], trait->type);
            if (newArg == 0) {
                mprAssert(ejs->exception);
                return EJS_ERR;
            }
            argv[i] = newArg;
        }
    }

    return argc;
}


/*
 *  Call a function by slot number
 */
static EjsFrame *callBySlot(Ejs *ejs, EjsVar *vp, int slotNum, EjsVar *thisObj, int argc, int stackAdjust)
{
    EjsFunction     *fun;

    fun = (EjsFunction*) ejsGetProperty(ejs, vp, slotNum);
    if (fun == 0) {
        ejsThrowInternalError(ejs, "Can't find function in slot %d", slotNum);
        return ejs->frame;
    }

    if (! ejsIsFunction(fun)) {
        ejsThrowArgError(ejs, "Variable is not a function");
        return ejs->frame;
    }

    callFunction(ejs, fun, thisObj, argc, stackAdjust);

    /*
     *  Return the current (new) frame as a convenience
     */
    return ejs->frame;
}


#if UNUSED
/*
 *  Call all constructors. Recursively build up frames for each base class constructor.
 */
static EjsFrame *callConstructors(Ejs *ejs, EjsType *type, EjsVar *thisObj, int argc)
{
    EjsType     *baseType;
    int         slotNum;

    mprAssert(type);
    mprAssert(ejsIsType(type));
    mprAssert(thisObj);

    baseType = type->baseType;

    /*
     *  Setup the top most constructor first. This will then call the next level and so on.
     *  ie. deepest constructor runs first.
     */
    if (type->hasConstructor) {
        /* 
         *  For bound types, the constructor is always the first slot (above the inherited properties) 
         */
        slotNum = type->block.numInherited;
        callBySlot(ejs, (EjsVar*) type, slotNum, thisObj, argc, 0);

    } else {
        slotNum = ejsLookupProperty(ejs, (EjsVar*) type, &type->qname);
        if (slotNum >= 0) {
            callBySlot(ejs, (EjsVar*) type, slotNum, thisObj, argc, 0);
        }
    }
    if (baseType && baseType->baseType && !type->callsSuper) {
        /* 
         *  Don't pass on args. Only call default constructors 
         */
        callConstructors(ejs, baseType, thisObj, 0);
    }
    return ejs->frame;
}
#endif


/*
 *  Call a function aka pushFunctionFrame. Supports both native and scripted functions. If native, the function is fully 
 *  invoked here. If scripted, a new frame is created and the pc adjusted to point to the new function.
 */
static EjsFrame *callFunction(Ejs *ejs, EjsFunction *fun, EjsVar *thisObj, int argc, int stackAdjust)
{
    EjsFrame        *frame, *saveFrame;
    EjsName         qname;
    EjsObject       *obj;
    EjsType         *type;
    EjsVar          **argv, *vp;
    int             numLocals, i, slotNum;

    mprAssert(fun);

    if (!ejsIsFunction(fun)) {
        /* 
         *  Handle calling a constructor to create a new instance 
         */
        if ((EjsVar*) fun == (EjsVar*) ejs->undefinedValue) {
            ejsThrowReferenceError(ejs, "Function is undefined");

        } else if (ejsIsType(fun)) {
            type = (EjsType*) fun;
            vp = ejsCreateVar(ejs, type, 0);

            if (type->hasConstructor) {
                /*
                 *  Constructor is always at slot 0 (offset by base properties)
                 */
                slotNum = type->block.numInherited;
                fun = (EjsFunction*) ejsGetProperty(ejs, (EjsVar*) type, slotNum);
                if (ejsIsNativeFunction(fun)) {
                    callFunction(ejs, fun, vp, argc, 0);
                    mprAssert(ejs->frame->returnValue);
                } else {
                    //  TODO - remove saveFrame
                    saveFrame = ejs->frame;
                    callFunction(ejs, fun, vp, argc, 0);
                    vmLoop(ejs);
                    mprAssert(ejs->frame);
                    mprAssert(ejs->frame == saveFrame);
                    ejs->frame = saveFrame;
                    ejs->frame->returnValue = vp;
                }
            } else {
                ejs->frame->returnValue = vp;
            }

        } else if (!ejsIsType(fun)) {
            ejsThrowReferenceError(ejs, "Reference is not a function");
        }
        return ejs->frame;
    }

    mprAssert(ejsIsFunction(fun));

    if (fun->thisObj) {
        /*
         *  Function has previously extracted the this value
         */
        thisObj = fun->thisObj;

    } else if (fun->staticMethod) {
        /*
         *  For static methods, must find the right base class to use as "this"
         */
        slotNum = fun->slotNum;
        type = ejsIsType(thisObj) ? (EjsType*) thisObj: thisObj->type;
        while (type) {
            /*
             *  Use baseType->numTraits rather than numInherited because implemented traits are not accessed 
             *  via the base type.
             */
            if (slotNum >= type->baseType->block.numTraits) {
                break;
            }
            type = type->baseType;
        }
        thisObj = (EjsVar*) type;
    }

    /*
     *  Validate the args. Cast to the right type, handle rest args and return with argc adjusted
     */
    argv = &ejs->stack.top[1 - argc];
    if ((argc = validateArgs(ejs, fun, argc, argv)) < 0) {
        return ejs->frame;
    }
    mprAssert(argc <= (int) fun->numArgs);

    if (ejsIsNativeFunction(fun)) {
        if (fun->body.proc == 0) {
            qname = ejsGetPropertyName(ejs, fun->owner, fun->slotNum);
            ejsThrowInternalError(ejs, "Native function \"%s\" is not defined", qname.name);
            return ejs->frame;
        }
        ejs->result = ejs->frame->returnValue = (fun->body.proc)(ejs, thisObj, argc, argv);
        ejs->stack.top = ejs->stack.top - argc - stackAdjust;

    } else {
        frame = createFrame(ejs);
        frame->function = *fun; 
        frame->function.block.obj.var.isFrame = 1;
        frame->templateBlock = (EjsBlock*) fun;
        frame->prevStackTop -= (argc + stackAdjust);
        frame->currentFunction = (EjsFunction*) &frame->function;
        frame->caller = frame->prev;
        frame->thisObj = thisObj;
        frame->code = &fun->body.code;
        frame->pc = fun->body.code.byteCode;
        setFrameDebugName(frame, fun);
        
        /* TODO - use the simpler approach in the assert */
        frame->function.block.obj.slots = (EjsVar**) ((char*) frame + MPR_ALLOC_ALIGN(sizeof(EjsFrame)));
        mprAssert(frame->function.block.obj.slots == ejs->stack.top + 1);
        mprAssert(frame->function.block.obj.slots == frame->stackBase);
        
        obj = &frame->function.block.obj;

        mprAssert(ejs->stack.top + 1 == obj->slots);
        
        /*
         *  Allow some padding to speed up creation of dynamic locals. TODO - is this worth doing?
         */
        obj->capacity += EJS_NUM_PROP;
        ejs->stack.top += obj->capacity;

        mprAssert(obj->numProp <= obj->capacity);
        mprAssert(&obj->slots[obj->capacity] == (ejs->stack.top + 1));
        
        memset(obj->slots, 0, obj->capacity * sizeof(EjsVar*));
        
        if (argc > 0) {
            frame->argc = argc;
            if ((uint) argc < (fun->numArgs - fun->numDefault) || (uint) argc > fun->numArgs) {
                ejsThrowArgError(ejs, "Incorrect number of arguments");
                return ejs->frame;
            }
            for (i = 0; i < argc; i++) {
                obj->slots[i] = argv[i];
            }
        }

        /*
         *  Initialize local vars. TODO - do this just to pickup namespaces initialization values. Loader defines those.
         *  TODO - remove this requirement.
         */
        numLocals = (obj->numProp - argc);
        if (numLocals > 0) {
            mprAssert(numLocals <= obj->numProp);
            for (i = 0; i < numLocals; i++) {
                obj->slots[i + argc] = fun->block.obj.slots[i + argc];
            }
        }
        
#if BLD_DEBUG
        frame->debugScopeChain = frame->function.block.scopeChain;
#endif
    }
    return ejs->frame;
}


/*
 *  Push a frame for lexical blocks. Used by the compiler and the VM for with and local blocks.
 */
EjsFrame *ejsPushFrame(Ejs *ejs, EjsBlock *block)
{
    EjsFrame    *frame, *prev;
    int         numProp, i;

    prev = ejs->frame;
    frame = createFrame(ejs);

    frame->function.block = *block;
    frame->templateBlock = block;

    frame->function.block.obj.var.isFrame = 1;

    setFrameDebugName(frame, block);

    if (likely(prev)) {
        /*
         *  Don't change the return caller, code or pc
         */
        frame->caller = prev->caller;
        frame->currentFunction = prev->currentFunction;
        frame->code = prev->code;
        if (prev->code) {
            frame->pc = prev->pc;
        }
        frame->exceptionArg = prev->exceptionArg;
    }

    /*
     *  Initialize properties
     */
    numProp = block->obj.numProp;
    if (numProp > 0) {
        frame->function.block.obj.slots = ejs->stack.top + 1;
        ejs->stack.top += numProp;  
        for (i = 0; i < numProp; i++) {
            frame->function.block.obj.slots[i] = block->obj.slots[i];
        }
    }
    frame->function.block.scopeChain = &prev->function.block;
    
#if BLD_DEBUG
    frame->debugScopeChain = frame->function.block.scopeChain;
#endif

    return frame;
}


/*
 *  Allocate a new frame for exceptions. We don't create new slots[] or locals. The exception frame reuses the parent slots.
 */
EjsFrame *ejsPushExceptionFrame(Ejs *ejs)
{
    EjsFrame    *frame, *prev;

    prev = ejs->frame;

    frame = createFrame(ejs);

    //  TOOD - not setting up the frame->function.block??
    
    mprAssert(prev);
    if (prev) {
        frame->function = prev->function;
        frame->templateBlock = prev->templateBlock;
        frame->code = prev->code;
        frame->currentFunction = prev->currentFunction;
        frame->caller = prev->caller;
        frame->thisObj = prev->thisObj;
    }
#if BLD_DEBUG
    frame->debugScopeChain = frame->function.block.scopeChain;
#endif

    return frame;
}


/*
 *  Pop a block frame and return to the previous frame. 
 */
EjsFrame *ejsPopFrame(Ejs *ejs)
{
    EjsFrame    *frame;

    frame = ejs->frame;
    mprAssert(frame);

    if (frame->needClosure.length > 0) {
        makeClosure(frame);
    }

    mprFreeChildren(frame);

    ejs->stack.top = frame->prevStackTop;
    ejs->frame = frame->prev;
    if (ejs->frame) {
        ejs->frame->pc = frame->pc;
    }
        
    return ejs->frame;
}


/*
 *  Pop a frame and return to the caller. This in-effect does a return from a function. This adjusts the stack and pops 
 *  off any actual arguments
 */
static bool popFrameAndReturn(Ejs *ejs)
{
    EjsFrame    *frame, *caller;

    frame = ejs->frame;
    mprAssert(frame);
    
    if (frame->needClosure.length > 0) {
        makeClosure(frame);
    }

    ejs->stack.top = frame->prevStackTop;

    if ((caller = frame->caller) != 0) {
        caller->returnValue = frame->returnValue;

        if (frame->function.getter) {
            mprFreeChildren(frame);
            push(ejs, frame->returnValue);
        
        } else {
            mprFreeChildren(frame);
        }
    } else {
        mprFreeChildren(frame);
    }
    if ((ejs->frame = caller) == 0) {
        return 1;
    }
    return frame->returnFrame;
}


/*
 *  Pop an exception frame and return to the previous frame. This in-effect does a return from a catch/finally block.
 */
static EjsFrame *popExceptionFrame(Ejs *ejs)
{
    EjsFrame        *frame, *prev;

    frame = ejs->frame;
    mprAssert(frame);

    if (frame->needClosure.length > 0) {
        makeClosure(frame);
    }

    prev = frame->prev;

    if (prev) {
        prev->returnValue = frame->returnValue;

        if (ejs->exception == 0) {
            ejs->exception = prev->saveException;
            prev->saveException = 0;
            if (ejs->exception) {
                ejs->attention = 1;
            }
        }
    }
    ejs->stack.top = frame->prevStackTop;

    mprFreeChildren(frame);

    /*
     *  Update the current frame. If the current frame is a function, this is actually a return.
     */
    ejs->frame = prev;

    return prev;
}


/*
 *  Allocate a new frame. Frames are created for functions and blocks.
 */
static inline EjsFrame *createFrame(Ejs *ejs)
{
    EjsFrame    *frame;
    int         size;

    size = MPR_ALLOC_ALIGN(MPR_ALLOC_HDR_SIZE + sizeof(EjsFrame));

    /*
     *  Allocate the frame off the eval stack.
     */
    frame = (EjsFrame*) (((char*) (ejs->stack.top + 1)) + MPR_ALLOC_HDR_SIZE);

    /*
     *  Initialize the memory header. This allows "frame" to be used as a memory context for mprAlloc
     */
    mprInitBlock(ejs, frame, size);
    
    if (likely(ejs->frame)) {
        frame->depth = ejs->frame->depth + 1;
    }
    frame->prev = ejs->frame;
    frame->ejs = ejs;
    frame->prevStackTop = ejs->stack.top;
    frame->thisObj = ejs->global;

    ejs->frame = frame;
    ejs->stack.top += (size / sizeof(EjsVar*));
    frame->stackBase = ejs->stack.top + 1;

    frame->function.block.obj.var.isFrame = 1;

    /*
     *  TODO OPT - bit fields
     */
    frame->function.block.obj.var.generation = 0;
    frame->function.block.obj.var.rootLinks = 0;
    frame->function.block.obj.var.refLinks = 0;
    frame->function.block.obj.var.builtin = 0;
    frame->function.block.obj.var.dynamic = 0;
    frame->function.block.obj.var.hasGetterSetter = 0;
    frame->function.block.obj.var.isFunction = 0;
    frame->function.block.obj.var.isObject = 0;
    frame->function.block.obj.var.isInstanceBlock = 0;
    frame->function.block.obj.var.isType = 0;
    frame->function.block.obj.var.isFrame = 0;
    frame->function.block.obj.var.hidden = 0;
    frame->function.block.obj.var.marked = 0;
    frame->function.block.obj.var.native = 0;
    frame->function.block.obj.var.nativeProc = 0;
    frame->function.block.obj.var.permanent = 0;
    frame->function.block.obj.var.survived = 0;
    frame->function.block.obj.var.visited = 0;
    
    return frame;
}


/*
 *  Called for catch and finally blocks
 */
static void callExceptionHandler(Ejs *ejs, EjsFunction *fun, int index, int flags)
{
    EjsFrame    *frame;
    EjsEx       *ex, *thisEx;
    EjsCode     *code;
    uint        handlerEnd;
    int         i;

    mprAssert(fun);

    frame = ejs->frame;
    code = &fun->body.code;
    mprAssert(0 <= index && index < code->numHandlers);
    ex = code->handlers[index];
    mprAssert(ex);

    if (flags & EJS_EX_ITERATION) {
        /*
         *  Empty handler is a special case for iteration. We simply do a break to the handler location
         *  which targets the end of the for/in loop.
         */
        ejs->frame->pc = &frame->code->byteCode[ex->handlerStart];
        ejs->exception = 0;
        return;
    }

    /*
     *  Allocate a new frame in which to execute the handler
     */
    frame = ejsPushExceptionFrame(ejs);
    if (frame == 0) {
        /*  Exception will continue to bubble up */
        return;
    }

    /*
     *  Move the PC outside of the try region. If this is a catch block, this allows the finally block to still
     *  be found. But if this is processing a finally block, the scanning for a matching handler will be forced
     *  to bubble up.
     */
    frame->pc = &frame->code->byteCode[ex->handlerStart];

    if (flags & EJS_EX_CATCH) {
        mprAssert(frame->inFinally == 0);
        frame->inCatch = 1;
        frame->ex = ex;
        frame->exceptionArg = ejs->exception;
        // push(ejs, ejs->exception);

    } else {
        mprAssert(flags & EJS_EX_FINALLY);
        mprAssert(frame->inCatch == 0);
        frame->inFinally = 1;
        frame->ex = ex;

        /*
         *  Mask the exception while processing the finally block
         */
        mprAssert(frame->saveException == 0);
        frame->prev->saveException = ejs->exception;
        ejs->attention = 1;
    }

    /*
     *  Adjust the PC in the original frame to be outside the try block so that we don't come back to here if the
     *  catch block does handle it.
     */
    frame->prev->pc = &frame->prev->code->byteCode[ex->handlerEnd - 1];
    ejs->exception = 0;

    /*
     *  Find the end of the exception block.
     *  TODO OPT - how could the compiler (simply) speed this up?
     */
    handlerEnd = 0;
    for (i = index; i < code->numHandlers; i++) {
        thisEx = code->handlers[i];
        if (ex->tryEnd == thisEx->tryEnd) {
            if (thisEx->handlerEnd > handlerEnd) {
                handlerEnd = thisEx->handlerEnd;
            }
        }
    }
    mprAssert(handlerEnd > 0);
    frame->prev->endException = &code->byteCode[handlerEnd];
}


/*
 *  Search for an exception handler at this level to process the exception. Return true if the exception is handled.
 */
static void handleExceptionAtThisLevel(Ejs *ejs, EjsFrame *frame)
{
    EjsFunction *fun;
    EjsCode     *code;
    EjsEx       *ex;
    uint        pc;
    int         i;

    ex = 0;

    code = frame->code;
    if (code == 0 || code->numHandlers == 0) {
        return;
    }

    /*
     *  The PC is always one advanced from the throwing instruction. ie. the PC has probably advanced
     *  past the instruction and now points to the next instruction. So reverse by one.
     */
    pc = (uint) (frame->pc - code->byteCode - 1);
    mprAssert(pc >= 0);

    if (frame->inCatch == 0 && frame->inFinally == 0) {
        /*
         *  Normal exception in a try block. NOTE: the catch will jump or fall through to the finally block code.
         *  ie. We won't come here again for the finally code unless there is an exception in the catch block.
         */
        for (i = 0; i < code->numHandlers; i++) {
            ex = code->handlers[i];
            mprAssert(ex);
            if (ex->tryStart <= pc && pc < ex->tryEnd && ex->flags & EJS_EX_CATCH) {
                //  TODO - temp comparison to voidType
                if (ex->catchType == ejs->voidType || ejsIsA(ejs, (EjsVar*) ejs->exception, ex->catchType)) {
                    fun = frame->currentFunction;
                    callExceptionHandler(ejs, fun, i, ex->flags);
                    return;
                }
            }
        }
        /*
         *  No catch handler at this level. Bubble up. But first invoke any finally handler -- see below.
         */

    } else {
        /*
         *  Exception in a catch block or in a finally block. If in a catch block, must first run the finally
         *  block before bubbling up. If in a finally block, we are done and upper levels will handle. We can be
         *  in a finally block and inFinally == 0. This happens because catch blocks jump or fall through directly
         *  into finally blocks (fast). But we need to check here if we are in the finally block explicitly.
         */
        for (i = 0; i < code->numHandlers; i++) {
            ex = code->handlers[i];
            mprAssert(ex);
            if (ex->handlerStart <= pc && pc < ex->handlerEnd && ex->flags & EJS_EX_FINALLY) {
                frame->inFinally = 1;
                break;
            }
        }
        if (frame->inFinally) {
            /*
             *  If falling through from a catch code block into a finally code block, we must push the outer blocks's
             *  frame pc to be outside the tryStart to finallyStart region. This prevents this try block from
             *  handling this exception again.
             */
            frame->prev->pc = &frame->prev->code->byteCode[ex->handlerEnd - 1];
            return;
        }
    }

    /*
     *  Either exception in the catch handler or exception not handled by this frame. Before returning up the stack
     *  to find upper handler to process the exception, we must first invoke the finally handler at this level if
     *  one exists.
     */
    mprAssert(!frame->inFinally);
    for (i = 0; i < code->numHandlers; i++) {
        ex = code->handlers[i];
        mprAssert(ex);
        /*
         *  Go from try start to finally start incase there is an exception in a catch block.
         */
        if (ex->tryStart <= pc && pc < ex->handlerStart && ex->flags & EJS_EX_FINALLY) {
            /*
             *  Clear any old catch frame
             */
            if (frame->inCatch) {
                frame = popExceptionFrame(ejs);
            }
            /*
             *  Create a finally block. Only case here is for exceptions in the try or catch regions.
             */
            fun = frame->currentFunction;
            callExceptionHandler(ejs, fun, i, EJS_EX_FINALLY);
            break;
        }
    }
}


/*
 *  Process and exception. Bubble up the exception until we find an exception handler for it.
 */
static bool handleException(Ejs *ejs)
{
    EjsFrame    *frame;

    /*
     *  Check at each level for a handler to process the exception.
     */
    for (frame = ejs->frame; frame; frame = popExceptionFrame(ejs)) {
        handleExceptionAtThisLevel(ejs, frame);
        if (ejs->exception == 0) {
            return 1;
        }
        if (frame->returnFrame) {
            popExceptionFrame(ejs);
            return 0;
        }
    }
    return 0;
}


typedef struct OperMap {
    int         opcode;
    cchar       *name;
} OperMap;

static OperMap operMap[] = {
        { EJS_OP_MUL,           "*"     },
        { EJS_OP_DIV,           "/"     },
        { EJS_OP_REM,           "%"     },
        { EJS_OP_COMPARE_LT,    "<"     },
        { EJS_OP_COMPARE_GT,    ">"     },
        { EJS_OP_COMPARE_LE,    "<="    },
        { EJS_OP_COMPARE_GE,    ">="    },
        { 0,                    0       },
};


static int lookupOverloadedOperator(Ejs *ejs, EjsOpCode opcode, EjsVar *lhs)
{
    EjsName     qname;
    int         i;

    for (i = 0; operMap[i].opcode; i++) {
        if (operMap[i].opcode == opcode) {
            ejsName(&qname, "", operMap[i].name);
            break;
        }
    }
    return ejsLookupProperty(ejs, (EjsVar*) lhs->type, &qname);
}


/*
 *  Evaluate a binary expression.
 *  TODO -- simplify and move back inline into eval loop.
 */
static EjsVar *evalBinaryExpr(Ejs *ejs, EjsVar *lhs, EjsOpCode opcode, EjsVar *rhs)
{
    EjsVar      *result;
    int         slotNum;

    if (lhs == 0) {
        lhs = ejs->undefinedValue;
    }
    if (rhs == 0) {
        rhs = ejs->undefinedValue;
    }

    result = ejsInvokeOperator(ejs, lhs, opcode, rhs);

    if (result == 0 && ejs->exception == 0) {
        slotNum = lookupOverloadedOperator(ejs, opcode, lhs);
        if (slotNum >= 0) {
            result = ejsRunFunctionBySlot(ejs, lhs, slotNum, 1, &rhs);
        }
    }
    return result;
}


/*
 *  Evaluate a unary expression.
 *  TODO -- once simplified, move back inline into eval loop.
 */
static EjsVar *evalUnaryExpr(Ejs *ejs, EjsVar *lhs, EjsOpCode opcode)
{
    return ejsInvokeOperator(ejs, lhs, opcode, 0);
}


int ejsInitStack(Ejs *ejs)
{
    EjsStack    *stack;

    /*
     *  Allocate the stack
     */
    stack = &ejs->stack;
    stack->size = MPR_PAGE_ALIGN(EJS_STACK_MAX, mprGetPageSize(ejs));

    /*
     *  This will allocate memory virtually for systems with virutal memory. Otherwise, it will just use malloc.
     *  TODO - should create a guard page to catch stack overflow.
     */
    stack->bottom = mprMapAlloc(stack->size, MPR_MAP_READ | MPR_MAP_WRITE);
    if (stack->bottom == 0) {
        mprSetAllocError(ejs);
        return EJS_ERR;
    }
    stack->top = &stack->bottom[-1];

    return 0;
}


#if FUTURE
/*
 *  Grow the operand evaluation stack.
 *  Return a negative error code on memory allocation errors or if the stack grows too big.
 */
int ejsGrowStack(Ejs *ejs, int incr)
{
    EjsStack *sp;
    EjsFrame *frame;
    EjsVar **bottom;
    int i, size, moveBy;

    sp = ejs->stack;
    sp->ejs = ejs;

    incr = max(incr, EJS_STACK_INC);

    if (sp->bottom) {
        /*
         *  Grow an existing stack
         */
        size = sp->size + (sizeof(EjsVar*) * incr);
        bottom = (EjsVar**) mprRealloc(sp, sp->bottom, size);
        //  TODO - do we really need zeroed?
        memset(&bottom[sp->size], 0, (size - sp->size) * sizeof(EjsVar*));
        moveBy = (int) ((char*) bottom - (char*) sp->bottom);
        sp->top = (EjsVar**) ((char*) sp->top + moveBy);
        sp->bottom = bottom;

        /*
         *  Adjust all the argv pointers. TODO REFACTOR!!!!
         */
        for (frame = ejs->frame; frame; frame = frame->prev) {
            if (frame->argv) {
                frame->argv = (EjsVar**) ((char*) frame->argv + moveBy);
            }
            frame->prevStackTop = (EjsVar**) ((char*) frame->prevStackTop + moveBy);
        }

    } else {
        /*
         *  Allocate a stack
         */
        if (sp->top >= &sp->bottom[EJS_STACK_MAX]) {
            return MPR_ERR_NO_MEMORY;
        }
        size = (sizeof(EjsVar*) * incr);
        sp->bottom = (EjsVar**) mprAlloc(sp, size);
        /*
         *  Push always begins with an increment of sp->top. Initially, sp_bottom points to the first (future) element.
         */
        sp->top = &sp->bottom[-1];
    }

    if (sp->bottom == 0) {
        return MPR_ERR_NO_MEMORY;
    }

    sp->end = &sp->bottom[size / sizeof(EjsVar*)];
    sp->size = size;

    //  TODO - opt memset
    for (i = 1; i <= incr; i++) {
        sp->top[i] = 0;
    }

    return 0;
}
#endif


/*
 *  Exit the script
 */
void ejsExit(Ejs *ejs, int status)
{
    //  TODO - should pass status back
    ejs->flags |= EJS_FLAG_EXIT;
}


/*
 *  Get an encoded number. Relies on correct byte code and some form of validation.
 */
static int getNum(EjsFrame *frame)
{
    uint t, c;

    c = (uint) *frame->pc++;
    t = c & 0x7f;

    if (c & 0x80) {
        c = (uint) *frame->pc++;
        t |= ((c & 0x7f) << 7);

        if (c & 0x80) {
            c = (uint) *frame->pc++;
            t |= ((c & 0x7f) << 14);

            if (c & 0x80) {
                c = (uint) *frame->pc++;
                t |= ((c & 0x7f) << 21);

                if (c & 0x80) {
                    c = (uint) *frame->pc++;
                    t |= ((c & 0x7f) << 28);
                }
            }
        }
    }
    return (int) t;
}


static EjsName getNameArg(EjsFrame *frame)
{
    EjsName qname;

    qname.name = getStringArg(frame);
    qname.space = getStringArg(frame);

    return qname;
}


/*
 *  Get an interned string. String constants are stored as token offsets into the constant pool. The pool
 *  contains null terminated UTF-8 strings.
 */
static char *getStringArg(EjsFrame *frame)
{
    int number;

    number = getNum(frame);
    if (number < 0) {
        ejsThrowInternalError(frame->ejs, "Bad instruction token");
        return 0;
    }

    mprAssert(frame->code->constants);
    return &frame->code->constants->pool[number];
}


static EjsVar *getGlobalArg(EjsFrame *frame)
{
    Ejs         *ejs;
    EjsVar      *vp;
    EjsName     qname;
    int         t, slotNum;

    ejs = frame->ejs;

    t = getNum(frame);
    if (t < 0) {
        return 0;
    }

    slotNum = -1;
    qname.name = 0;
    qname.space = 0;
    vp = 0;

    /*
     *  TODO - OPT. Could this encoding be optimized?
     */
    switch (t & EJS_ENCODE_GLOBAL_MASK) {
    default:
        mprAssert(0);
        return 0;

    case EJS_ENCODE_GLOBAL_NOREF:
        return 0;

    case EJS_ENCODE_GLOBAL_SLOT:
        slotNum = t >> 2;
        if (0 <= slotNum && slotNum < ejsGetPropertyCount(ejs, ejs->global)) {
            vp = ejsGetProperty(ejs, ejs->global, slotNum);
        }
        break;

    case EJS_ENCODE_GLOBAL_NAME:
        qname.name = &frame->code->constants->pool[t >> 2];
        if (qname.name == 0) {
            mprAssert(0);
            return 0;
        }
        qname.space = getStringArg(frame);
        if (qname.space == 0) {
            return 0;
        }
        if (qname.name) {
            vp = ejsGetPropertyByName(ejs, ejs->global, &qname);
        }
        break;
    }
    return vp;
}


/*
 *  Get a function reference. This binds the "this" value for method extraction. Also handles getters.
 */
static EjsFrame *getFunction(Ejs *ejs, EjsVar *thisObj, EjsVar *obj, int slotNum, EjsFunction *fun, EjsObject **local)
{
    EjsFrame    *frame;

    frame = ejs->frame;

    if (fun->getter) {
        callFunction(ejs, fun, (thisObj) ? thisObj : obj, 0, 0);
        if (ejsIsNativeFunction(fun)) {
            push(ejs, frame->returnValue);
        }

    } else if (slotNum == fun->slotNum && !fun->block.obj.var.nativeProc) {
        if (obj == fun->owner || (ejsIsType(obj) && ejsIsType(fun->owner) && ejsIsA(ejs, obj, (EjsType*) fun->owner))) {
            
            /*
             *  Bind "this" for the function
             */
            if (thisObj == 0) {
                if (obj == (EjsVar*) &frame->function) {
                    thisObj = frame->thisObj;

                } else if (fun->thisObj) {
                    thisObj = fun->thisObj;

                } else {
                    thisObj = obj;
                }
            }

            fun = ejsCopyFunction(ejs, fun);
            if (fun == 0) {
                *local = (EjsObject*) frame->currentFunction;
                return ejs->frame;
            }
            fun->thisObj = thisObj;

            if (fun->fullScope) {
                needClosure(frame, (EjsBlock*) fun);
            }
        }
        push(ejs, fun);

    } else {
        push(ejs, fun);
    }

    if (unlikely(ejs->attention)) {
        payAttention(ejs);
    }

    if (ejs->frame) {
        *local = (EjsObject*) ejs->frame->currentFunction;
    }
    return ejs->frame;
}


/*
 *  Handle setters. Setters, if present, are chained off the getter.
 */
static void putFunction(Ejs *ejs, EjsVar *thisObj, EjsFunction *fun, EjsVar *value)
{
    mprAssert(fun->getter && fun->nextSlot);
    fun = (EjsFunction*) ejsGetProperty(ejs, fun->owner, fun->nextSlot);
    mprAssert(fun && ejsIsFunction(fun) && fun->setter);
    ejsRunFunction(ejs, fun, thisObj, 1, &value);
}


/*
 *  Store a property by name somewhere in the current scope chain. Will create properties if the given name does not
 *  already exist.
 */
static void storePropertyToScope(Ejs *ejs, EjsName *qname)
{
    EjsFunction     *fun;
    EjsFrame        *frame;
    EjsVar          *value, **vpp, *obj;
    EjsObject       *block;
    EjsLookup       lookup;
    int             slotNum;

    mprAssert(qname);

    frame = ejs->frame;

    slotNum = ejsLookupScope(ejs, qname, 1, &lookup);

    if (slotNum >= 0) {
        obj = lookup.obj;

        slotNum = ejsLookupScope(ejs, qname, 1, &lookup);

        fun = (EjsFunction*) ejsGetProperty(ejs, obj, slotNum);
        if (fun && ejsIsFunction(fun) && (fun->getter || fun->setter)) {
            /*
             *  Handle setters. Setters, if present, are chained off the getter.
             */
            if (fun->getter && fun->nextSlot) {
                fun = (EjsFunction*) ejsGetProperty(ejs, fun->owner, fun->nextSlot);
                mprAssert(fun && ejsIsFunction(fun) && fun->setter);
            }
            callFunction(ejs, fun, obj, 1, 0);
            ejs->attention = 1;
            return;
        }

    } else if (frame->currentFunction->lang & EJS_SPEC_FIXED && frame->caller) {
        /*
         *  Create a new local property instead of a global
         */
        obj = (EjsVar*) &frame->function.block;
        block = &frame->function.block.obj;
        
        if (block->numProp >= block->capacity) {
            /*
             *  Copy up the evaluation stack to make room for the var
             */
            vpp = &block->slots[block->capacity];
            for (vpp = ejs->stack.top; vpp >= &block->slots[block->capacity]; vpp--) {
                vpp[EJS_NUM_PROP] = vpp[0];
            }
            block->capacity += EJS_NUM_PROP;
            ejs->stack.top += EJS_NUM_PROP;
        }

    } else {
        obj = ejs->global;
    }

    value = pop(ejs);
    ejs->result = value;

    slotNum = ejsSetProperty(ejs, obj, slotNum, value);
    if (slotNum >= 0) {
        ejsSetPropertyName(ejs, obj, slotNum, qname);
    }
}


/*
 *  Store a property by name in the given object. Will create if the property does not already exist.
 */
static void storeProperty(Ejs *ejs, EjsVar *obj, EjsName *qname)
{
    EjsFrame        *frame;
    EjsFunction     *fun;
    EjsLookup       lookup;
    EjsVar          *value, *origObj;
    int             slotNum;

    mprAssert(qname);
    mprAssert(qname->name);
    mprAssert(obj);

    frame = ejs->frame;
    slotNum = -1;
    origObj = obj;

    ejs->result = value = pop(ejs);

    if (obj->type->helpers->setPropertyByName) {
        /*
         *  If a setPropertyByName helper is defined, defer to it. Allows types like XML to not offer slot based APIs.
         */
        slotNum = (*obj->type->helpers->setPropertyByName)(ejs, obj, qname, value);
        if (slotNum >= 0) {
            ejsSetReference(ejs, obj, value);
            return;
        }
    }

    slotNum = ejsLookupVar(ejs, obj, qname, 1, &lookup);

    if (slotNum >= 0) {
        obj = lookup.obj;

        /*
         *  Handle setters. Setters, if present, are chained off the getter.
         */
        if (obj->hasGetterSetter) {
            fun = (EjsFunction*) ejsGetProperty(ejs, obj, slotNum);
            if (ejsIsFunction(fun) && (fun->getter || fun->setter)) {
                if (fun->getter && fun->nextSlot) {
                    fun = (EjsFunction*) ejsGetProperty(ejs, fun->owner, fun->nextSlot);
                    mprAssert(fun && ejsIsFunction(fun) && fun->setter);
                }
                push(ejs, value);
                callFunction(ejs, fun, origObj, 1, 0);
                ejs->attention = 1;
                return;
            }
        }
    }

    slotNum = ejsSetProperty(ejs, obj, slotNum, value);
    if (slotNum >= 0) {
        /* TODO - OPT. This can be improved. Don't always need to set the name */
        ejsSetPropertyName(ejs, obj, slotNum, qname);
    }
}


static void needClosure(EjsFrame *frame, EjsBlock *block)
{
    block->scopeChain = (EjsBlock*) frame;
    ejsAddItem(frame, &frame->needClosure, block);
}


static void makeClosure(EjsFrame *frame)
{
    EjsBlock    *block, *closure, *b;
    int         next;

    mprLog(frame, 7, "Making closure for %d blocks", frame->needClosure.length);

    closure = 0;
    next = 0;
    while ((block = ejsGetNextItem(&frame->needClosure, &next)) != 0) {
        /*
         *  Search the existing scope chain to find this frame. Then replace it.
         */
        for (b = block; b; b = b->scopeChain) {
            if (b->scopeChain == (EjsBlock*) frame) {
                if (closure == 0) {
                    /*
                     *  TODO - OPT. If returning from a frame (always), we could steal the namespace list. 
                     *  So ejsCloneVar may not be the fastest way to do this.
                     */
                    closure = (EjsBlock*) ejsCloneVar(frame->ejs, (EjsVar*) frame, 0);
                }
                b->scopeChain = closure;
                ejsSetReference(frame->ejs, (EjsVar*) b, (EjsVar*) closure);
                break;
            }
        }

        /*
         *  Pass these blocks needing closures up to the previous block as it will need to patch/replace itself in the
         *  closure chain when it returns.
         */
        if (frame->prev) {
            /*
             *  TODO BUG. This is processing the block for unrelated call-chain functions. Should only do lexical scope.
             *  ie. the top level frame and any exception blocks.
             */
            ejsAddItem(frame->prev, &frame->prev->needClosure, block);
        }
    }
}


static void swap2(Ejs *ejs)
{
    EjsVar      *tmp;

    tmp = ejs->stack.top[0];
    ejs->stack.top[0] = ejs->stack.top[-1];
    ejs->stack.top[-1] = tmp;
}


static void debug(EjsFrame *frame)
{
    int lastLine;

    lastLine = frame->lineNumber;

    frame->fileName = getStringArg(frame);
    frame->lineNumber = getNum(frame);
    frame->currentLine = getStringArg(frame);
}


static void throwNull(Ejs *ejs)
{
    ejsThrowReferenceError(ejs, "Object reference is null");
}


static EjsVar *getNthBase(Ejs *ejs, EjsVar *obj, int nthBase)
{
    EjsType     *type;

    if (obj) {
        if (ejsIsType(obj) || obj == ejs->global) {
            type = (EjsType*) obj;
        } else {
            type = obj->type;
            nthBase--;
        }
        for (; type && nthBase > 0; type = type->baseType) {
            nthBase--;
        }
        obj = (EjsVar*) type;
    }
    return obj;
}


static EjsVar *getNthBaseFromBottom(Ejs *ejs, EjsVar *obj, int nthBase)
{
    EjsType     *type, *tp;
    int         count;

    if (obj) {
        if (ejsIsType(obj) || obj == ejs->global) {
            type = (EjsType*) obj;
        } else {
            type = obj->type;
        }
        for (count = 0, tp = type->baseType
             ; tp; tp = tp->baseType) {
            count++;
        }
        nthBase = count - nthBase;
        for (; type && nthBase > 0; type = type->baseType) {
            nthBase--;
        }
        obj = (EjsVar*) type;
    }
    return obj;
}


static inline EjsVar *getNthBlock(EjsFrame *frame, int nth)
{
    EjsBlock    *block;

    mprAssert(frame);
    mprAssert(nth >= 0);

    block = &frame->function.block;

    while (block && --nth >= 0) {
        block = block->scopeChain;
    }
    mprAssert(block != frame->ejs->globalBlock);
    return (EjsVar*) block;
}


static EjsBlock *varToBlock(Ejs *ejs, EjsVar *vp)
{
    EjsBlock    *block;
    
    block = ejsCreateBlock(ejs, "", 0);
    if (block == 0) {
        return 0;
    }
    memcpy((void*) block, vp, vp->type->instanceSize);
    return block;
}


/*
 *  Enter a mesage into the log file
 */
void ejsLog(Ejs *ejs, const char *fmt, ...)
{
    va_list     args;
    char        buf[MPR_MAX_LOG_STRING];
    int         len;

    va_start(args, fmt);
    len = mprVsprintf(buf, sizeof(buf) - 1, fmt, args);
    va_end(args);

    mprLog(ejs, 0, buf, len);

    va_end(args);
}


void ejsShowStack(Ejs *ejs)
{
    char    *stack;
    
    stack = ejsFormatStack(ejs);
    mprLog(ejs, 7, "Stack\n%s", stack);
    mprFree(stack);
}


#if BLD_DEBUG || 1
static char *opcodes[] = {
    "Add",
    "AddNamespace",
    "AddNamespaceRef",
    "And",
    "BranchEQ",
    "BranchStrictlyEQ",
    "BranchFalse",
    "BranchGE",
    "BranchGT",
    "BranchLE",
    "BranchLT",
    "BranchNE",
    "BranchStrictlyNE",
    "BranchNull",
    "BranchNotZero",
    "BranchTrue",
    "BranchUndefined",
    "BranchZero",
    "BranchFalse.8",
    "BranchTrue.8",
    "Breakpoint",
    "Call",
    "CallGlobalSlot",
    "CallObjSlot",
    "CallThisSlot",
    "CallBlockSlot",
    "CallObjInstanceSlot",
    "CallObjStaticSlot",
    "CallThisStaticSlot",
    "CallName",
    "CallScopedName",
    "CallConstructor",
    "CallNextConstructor",
    "Cast",
    "CastBoolean",
    "CloseBlock",
    "CloseWith",
    "CompareEQ",
    "CompareStrictlyEQ",
    "CompareFalse",
    "CompareGE",
    "CompareGT",
    "CompareLE",
    "CompareLT",
    "CompareNE",
    "CompareStrictlyNE",
    "CompareNull",
    "CompareNotZero",
    "CompareTrue",
    "CompareUndefined",
    "CompareZero",
    "Debug",
    "DefineClass",
    "DefineFunction",
    "DefineGlobalFunction",
    "DeleteNameExp",
    "Delete",
    "DeleteName",
    "Div",
    "Dup",
    "Dup2",
    "EndCode",
    "EndException",
    "Goto",
    "Goto.8",
    "Inc",
    "InitDefaultArgs",
    "InitDefaultArgs.8",
    "InstOf",
    "IsA",
    "Load0",
    "Load1",
    "Load2",
    "Load3",
    "Load4",
    "Load5",
    "Load6",
    "Load7",
    "Load8",
    "Load9",
    "LoadDouble",
    "LoadFalse",
    "LoadGlobal",
    "LoadInt.16",
    "LoadInt.32",
    "LoadInt.64",
    "LoadInt.8",
    "LoadM1",
    "LoadName",
    "LoadNamespace",
    "LoadNull",
    "LoadRegexp",
    "LoadString",
    "LoadThis",
    "LoadTrue",
    "LoadUndefined",
    "LoadXML",
    "GetLocalSlot_0",
    "GetLocalSlot_1",
    "GetLocalSlot_2",
    "GetLocalSlot_3",
    "GetLocalSlot_4",
    "GetLocalSlot_5",
    "GetLocalSlot_6",
    "GetLocalSlot_7",
    "GetLocalSlot_8",
    "GetLocalSlot_9",
    "GetObjSlot_0",
    "GetObjSlot_1",
    "GetObjSlot_2",
    "GetObjSlot_3",
    "GetObjSlot_4",
    "GetObjSlot_5",
    "GetObjSlot_6",
    "GetObjSlot_7",
    "GetObjSlot_8",
    "GetObjSlot_9",
    "GetThisSlot_0",
    "GetThisSlot_1",
    "GetThisSlot_2",
    "GetThisSlot_3",
    "GetThisSlot_4",
    "GetThisSlot_5",
    "GetThisSlot_6",
    "GetThisSlot_7",
    "GetThisSlot_8",
    "GetThisSlot_9",
    "GetName",
    "GetObjName",
    "GetObjNameExpr",
    "GetBlockSlot",
    "GetGlobalSlot",
    "GetLocalSlot",
    "GetObjSlot",
    "GetThisSlot",
    "GetTypeSlot",
    "GetThisTypeSlot",
    "In",
    "Like",
    "LogicalNot",
    "Mul",
    "Neg",
    "New",
    "NewArray",
    "NewObject",
    "Nop",
    "Not",
    "OpenBlock",
    "OpenWith",
    "Or",
    "Pop",
    "PopItems",
    "PushCatchArg",
    "PushResult",
    "PutLocalSlot_0",
    "PutLocalSlot_1",
    "PutLocalSlot_2",
    "PutLocalSlot_3",
    "PutLocalSlot_4",
    "PutLocalSlot_5",
    "PutLocalSlot_6",
    "PutLocalSlot_7",
    "PutLocalSlot_8",
    "PutLocalSlot_9",
    "PutObjSlot_0",
    "PutObjSlot_1",
    "PutObjSlot_2",
    "PutObjSlot_3",
    "PutObjSlot_4",
    "PutObjSlot_5",
    "PutObjSlot_6",
    "PutObjSlot_7",
    "PutObjSlot_8",
    "PutObjSlot_9",
    "PutThisSlot_0",
    "PutThisSlot_1",
    "PutThisSlot_2",
    "PutThisSlot_3",
    "PutThisSlot_4",
    "PutThisSlot_5",
    "PutThisSlot_6",
    "PutThisSlot_7",
    "PutThisSlot_8",
    "PutThisSlot_9",
    "PutObjNameExpr",
    "PutScopedNameExpr",
    "PutObjName",
    "PutScopedName",
    "PutBlockSlot",
    "PutGlobalSlot",
    "PutLocalSlot",
    "PutObjSlot",
    "PutThisSlot",
    "PutTypeSlot",
    "PutThisTypeSlot",
    "Rem",
    "Return",
    "ReturnValue",
    "SaveResult",
    "Shl",
    "Shr",
    "Sub",
    "Super",
    "Swap",
    "Throw",
    "Ushr",
    "Xor",
    "Ext",
    0
};


static EjsOpCode traceCode(Ejs *ejs, EjsOpCode opcode)
{
    EjsFrame    *frame;
    int         len;
    static int  once = 0;
    static int  stop = 1;

    frame = ejs->frame;

    if (ejs->initialized && opcode != EJS_OP_DEBUG) {
        //  TODO - should strip '\n' in the compiler
        if (frame->currentLine) {
            len = (int) strlen(frame->currentLine) - 1;
            if (frame->currentLine[len] == '\n') {
                ((char*) frame->currentLine)[len] = '\0';
            }
        }
        //  TODO - compiler should strip '\n' from currentLine and we should explicitly add it here
        mprLog(ejs, 6, "%04d: [%d] %02x: %-35s # %s:%d %s",
            (uint) (frame->pc - frame->code->byteCode) - 1, (int) (ejs->stack.top - frame->stackBase + 1),
            (uchar) opcode, opcodes[opcode], frame->fileName, frame->lineNumber, frame->currentLine);
        if (stop && once++ == 0) {
             mprSleep(ejs, 0);
        }
    }
    return opcode;
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
