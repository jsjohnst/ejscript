/**
 *  ecCodeGen.c - Ejscript code generator
 *
 *  This module generates code for a program that is represented by an in-memory AST set of nodes.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejs.h"
#include    "ecModuleWrite.h"
#include    "ecCompiler.h"

/********************************** Defines ***********************************/

/*
 *  State level macros. Enter/Leave manage state and inheritance of state.
 */
#undef ENTER
#define ENTER(a)    if (ecEnterState(a) < 0) { return; } else

#undef LEAVE
#define LEAVE(cp)   ecLeaveState(cp)

#define SAVE_ONLEFT(cp)                                     \
    if (1) {                                                \
            cp->state->saveOnLeft = cp->state->onLeft;      \
            cp->state->onLeft = 0;                          \
    } else

#define RESTORE_ONLEFT(cp)                                  \
    cp->state->onLeft = cp->state->saveOnLeft

/***************************** Forward Declarations ***************************/

static void     addDebugInstructions(EcCompiler *cp, EcNode *np);
static void     addException(EcCompiler *cp, uint tryStart, uint tryEnd, EjsType *catchType, uint handlerStart, 
                    uint handlerEnd, int flags);
static void     addJump(EcCompiler *cp, EcNode *np, int kind);
static void     addModule(EcCompiler *cp, EjsModule *mp);
static EcCodeGen *allocCodeBuffer(EcCompiler *cp);
static void     badNode(EcCompiler *cp, EcNode *np);
static void     copyCodeBuffer(EcCompiler *cp, EcCodeGen *dest, EcCodeGen *code);
static void     createInitializer(EcCompiler *cp, EjsModule *mp);
static void     emitNamespace(EcCompiler *cp, EjsNamespace *nsp);
static void     emptyStack(EcCompiler *cp, int preserve);
static int      flushModuleBuffer(MprFile *file, EcCodeGen *code);
static void     genBinaryOp(EcCompiler *cp, EcNode *np);
static void     genBlock(EcCompiler *cp, EcNode *np);
static void     genBreak(EcCompiler *cp, EcNode *np);
static void     genBoundName(EcCompiler *cp, EcNode *np);
static void     genCall(EcCompiler *cp, EcNode *np);
static void     genCatchArg(EcCompiler *cp, EcNode *np);
static void     genClass(EcCompiler *cp, EcNode *child);
static void     genClassName(EcCompiler *cp, EjsType *type);
static int      getCodeLength(EcCompiler *cp, EcCodeGen *code);
static void     genContinue(EcCompiler *cp, EcNode *np);
static void     genDirectives(EcCompiler *cp, EcNode *np, bool resetStack);
static void     genDo(EcCompiler *cp, EcNode *np);
static void     genDot(EcCompiler *cp, EcNode *np, EcNode **rightMost);
static void     genError(EcCompiler *cp, EcNode *np, char *fmt, ...);
static void     genEndFunction(EcCompiler *cp, EcNode *np);
static void     genExpressionName(EcCompiler *cp);
static void     genExpressions(EcCompiler *cp, EcNode *np);
static void     genFor(EcCompiler *cp, EcNode *np);
static void     genForIn(EcCompiler *cp, EcNode *np);
static void     genFunction(EcCompiler *cp, EcNode *np);
static void     genHash(EcCompiler *cp, EcNode *np);
static void     genIf(EcCompiler *cp, EcNode *np);
static void     genLeftHandSide(EcCompiler *cp, EcNode *np);
static void     genLiteral(EcCompiler *cp, EcNode *np);
static void     genLogicalOp(EcCompiler *cp, EcNode *np);
static void     genModule(EcCompiler *cp, EcNode *np);
static void     genName(EcCompiler *cp, EcNode *np);
static void     genNew(EcCompiler *cp, EcNode *np);
static void     genObjectLiteral(EcCompiler *cp, EcNode *np);
static void     genProgram(EcCompiler *cp, EcNode *np);
static void     genPragmas(EcCompiler *cp, EcNode *np);
static void     genPostfixOp(EcCompiler *cp, EcNode *np);
static void     genReturn(EcCompiler *cp, EcNode *np);
static void     genSuper(EcCompiler *cp, EcNode *np);
static void     genSwitch(EcCompiler *cp, EcNode *np);
static void     genThis(EcCompiler *cp, EcNode *np);
static void     genThrow(EcCompiler *cp, EcNode *np);
static void     genTry(EcCompiler *cp, EcNode *np);
static void     genUnaryOp(EcCompiler *cp, EcNode *np);
static void     genUnboundName(EcCompiler *cp, EcNode *np);
static void     genUseNamespace(EcCompiler *cp, EcNode *np);
static void     genVar(EcCompiler *cp, EcNode *np);
static void     genVarDefinition(EcCompiler *cp, EcNode *np);
static void     genWith(EcCompiler *cp, EcNode *np);
static EcNode   *getNextNode(EcCompiler *cp, EcNode *np, int *next);
static EcNode   *getPrevNode(EcCompiler *cp, EcNode *np, int *next);
static int      mapToken(EcCompiler *cp, EjsOpCode tokenId);
static MprFile  *openModuleFile(EcCompiler *cp, const char *filename);
static void     patchJumps(EcCompiler *cp, int kind, int target);
static void     popStack(EcCompiler *cp, int count);
static void     processNode(EcCompiler *cp, EcNode *np);
static void     processModule(EcCompiler *cp, EjsModule *mp);
static void     pushStack(EcCompiler *cp, int count);
static void     resetStack(EcCompiler *cp);
static void     setCodeBuffer(EcCompiler *cp, EcCodeGen *saveCode);
static void     setFunctionCode(EcCompiler *cp, EjsFunction *fun, EcCodeGen *code);
static void     setStack(EcCompiler *cp, int count);


/************************************ Code ************************************/
/*
 *  Generate code for evaluating conditional compilation directives
 */
void ecGenConditionalCode(EcCompiler *cp, EcNode *np, EjsModule *mp)
{
    EcState         *state;

    ENTER(cp);

    state = cp->state;
    mprAssert(state);

    addModule(cp, mp);

    genDirectives(cp, np, 0);

    /*
     *  Save the expression result from the stack into ejs->frame->returnValue
     */
    ecEncodeOpcode(cp, EJS_OP_SAVE_RESULT);

    if (cp->errorCount > 0) {
        ecRemoveModule(cp, mp);
        LEAVE(cp);
        return;
    }

    createInitializer(cp, mp);
    ecRemoveModule(cp, mp);

    LEAVE(cp);
}


/*
 *  Top level for code generation. Loop through the AST nodes recursively.
 */
int ecCodeGen(EcCompiler *cp, int argc, EcNode **nodes)
{
    EjsModule   *mp;
    EcNode      *np;
    int         moduleCount, i;

    if (ecEnterState(cp) < 0) {
        return EJS_ERR;
    }

    for (i = 0; i < argc; i++) {
        np = nodes[i];

        cp->fileState = cp->state;
        cp->fileState->mode = cp->defaultMode;
        cp->fileState->lang = cp->lang;

        if (np) {
            processNode(cp, np);
        }
    }

    /*
     *  Open once if merging into a single output file
     */
    if (cp->outputFile && openModuleFile(cp, cp->outputFile) == 0) {
        return EJS_ERR;
    }

    /*
     *  Now generate code for all the modules
     */
    moduleCount = mprGetListCount(cp->modules);
    for (i = 0; i < moduleCount && !cp->error; i++) {
        mp = (EjsModule*) mprGetItem(cp->modules, i);
        if (mp->loaded) {
            continue;
        }

        /*
         *  Don't generate the default module unless it contains some real code or definitions and we have more than one module.
         */
        if (moduleCount == 1 || mp->globalProperties || mp->hasInitializer || strcmp(mp->name, EJS_DEFAULT_MODULE) != 0) {
            mp->initialized = 0;
            processModule(cp, mp);
        }
    }

    if (cp->outputFile) {
        if (flushModuleBuffer(cp->file, cp->state->code) < 0) {
            genError(cp, 0, "Can't write to module file %s", cp->outputFile);
        }
        mprFree(cp->file);
        cp->file = 0;
    }
    //  TODO - is this right?
    cp->file = 0;

    ecLeaveState(cp);

    //  TODO - should test if any global memory errors
    return (cp->fatalError) ? EJS_ERR : 0;
}


static void genArgs(EcCompiler *cp, EcNode *np)
{
    EcNode      *child;
    int         next;

    ENTER(cp);

    mprAssert(np->kind == N_ARGS);

    cp->state->needsValue = 1;

    //  TODO - is this the right order of evaluation. We are doing left to right?

    next = 0;
    while ((child = getNextNode(cp, np, &next)) && !cp->error) {
        if (child->kind == N_ASSIGN_OP) {
            child->needDup = 1;
        }
        processNode(cp, child);
        child->needDup = 0;
    }

    LEAVE(cp);
}


static void genArrayLiteral(EcCompiler *cp, EcNode *np)
{
    EcNode      *child;
    int         next;

    ENTER(cp);

    next = 0;
    while ((child = getNextNode(cp, np, &next)) != 0) {
        /* Don't propagate needsValue here. We have a new and that will take care of the residual value */
        cp->state->needsValue = 0;
        processNode(cp, child);
    }

    LEAVE(cp);
}


/*
 *  Generate an assignment expression
 */
static void genAssignOp(EcCompiler *cp, EcNode *np)
{
    EcState     *state;
    int         rc, next;

    ENTER(cp);

    state = cp->state;
    rc = 0;
    next = 0;

    mprAssert(np->kind == N_ASSIGN_OP);
    mprAssert(np->left);
    mprAssert(np->right);

    state->onLeft = 0;

    /*
     *  Dup the object on the stack so it is available for subsequent operations
     */
    if (np->needDupObj) {
        ecEncodeOpcode(cp, EJS_OP_DUP);
        pushStack(cp, 1);
    }

    /*
     *  Process the expression on the right. Leave the result on the stack.
     */
    if (np->right->kind == N_ASSIGN_OP) {
        np->right->needDup = 1;
    }

    state->needsValue = 1;
    processNode(cp, np->right);
    state->needsValue = 0;

    if (np->needDupObj) {
        /*
         *  Get the object on the top above the value
         */
        ecEncodeOpcode(cp, EJS_OP_SWAP);
    }

    /*
     *  If this expression is part of a function argument, the result must be preserved.
     */
    //  CHANGE for:    while ((x = 7) != 8)   .. need to save expression result
    if (np->needDup || state->prev->needsValue) {
        ecEncodeOpcode(cp, EJS_OP_DUP);
        pushStack(cp, 1);
    }

    /*
     *  Store to the left hand side
     *  TODO - this is bugged. Really need to complete the lvalue first, then the rhs, then store.
     */
    genLeftHandSide(cp, np->left);

    LEAVE(cp);
}


static void genBinaryOp(EcCompiler *cp, EcNode *np)
{
    EcState     *state;

    ENTER(cp);

    state = cp->state;
    state->needsValue = 1;

    mprAssert(np->kind == N_BINARY_OP);

    switch (np->tokenId) {
    case T_LOGICAL_AND:
    case T_LOGICAL_OR:
        genLogicalOp(cp, np);
        break;

    default:
        if (np->left) {
            processNode(cp, np->left);
        }
        if (np->right) {
            processNode(cp, np->right);
        }
        ecEncodeOpcode(cp, mapToken(cp, np->tokenId));
        popStack(cp, 2);
        pushStack(cp, 1);
        break;
    }

    mprAssert(state == cp->state);
    LEAVE(cp);
}


static void genBreak(EcCompiler *cp, EcNode *np)
{
    EcState     *state;

    ENTER(cp);

    state = cp->state;

    if (state->code->jumps == 0 || !(state->code->jumpKinds & EC_JUMP_BREAK)) {
        //  TODO - need line number info here
        genError(cp, np, "Illegal break statement");

    } else {
        if (state->needsStackReset) {
            emptyStack(cp, state->preserveStackCount);
        }
        ecEncodeOpcode(cp, EJS_OP_GOTO);
        addJump(cp, np, EC_JUMP_BREAK);
        ecEncodeWord(cp, 0);
    }

    LEAVE(cp);
}


static void genBlock(EcCompiler *cp, EcNode *np)
{
    Ejs             *ejs;
    EjsNamespace    *namespace;
    EcState         *state;
    EjsBlock        *block;
    EjsLookup       *lookup;
    EcNode          *child;
    int             next;

    ENTER(cp);

    state = cp->state;
    ejs = cp->ejs;
    block = (EjsBlock*) np->blockRef;

    if (block && np->createBlockObject) {
        state->prevBlockState = cp->blockState;
        cp->blockState = state;

        lookup = &np->lookup;
        if (lookup->slotNum >= 0) {
            ecEncodeOpcode(cp, EJS_OP_OPEN_BLOCK);
            ecEncodeNumber(cp, lookup->slotNum);
            ecEncodeNumber(cp, lookup->nthBlock);
        }

        /*
         *  Emit block namespaces
         */
        if (block->namespaces.length > 0) {
            for (next = 0; ((namespace = (EjsNamespace*) ejsGetNextItem(&block->namespaces, &next)) != 0); ) {
                /*
                 *  TODO - Select only the -hoisted namespaces. Need better way to do this.
                 */
                if (namespace->name[0] == '-') {
                    emitNamespace(cp, namespace);
                }
            }
        }

        state->letBlock = (EjsVar*) block;
        state->letBlockNode = np;

        next = 0;
        while ((child = getNextNode(cp, np, &next))) {
            processNode(cp, child);
        }

        if (lookup->slotNum >= 0) {
            ecEncodeOpcode(cp, EJS_OP_CLOSE_BLOCK);
        }
        cp->blockState = state->prevBlockState;
        ecAddNameConstant(cp, &np->qname);

    } else {
        next = 0;
        mprAssert(cp->state->code->stackCount == 0);

        while ((child = getNextNode(cp, np, &next))) {
            mprAssert(cp->state->code->stackCount == 0);
            processNode(cp, child);
        }
    }

    LEAVE(cp);
}


/*
 *  Block scope variable reference
 */
static void genBlockName(EcCompiler *cp, int slotNum, int nthBlock)
{
    int         code;

    mprAssert(slotNum >= 0);

#if FUTURE
    if (slotNum < 10) {
        code = (!cp->state->onLeft) ?  EJS_OP_GET_BLOCK_SLOT_0 :  EJS_OP_PUT_BLOCK_SLOT_0;
        ecEncodeOpcode(cp, code + slotNum);

    } else {
#endif

        code = (!cp->state->onLeft) ?  EJS_OP_GET_BLOCK_SLOT :  EJS_OP_PUT_BLOCK_SLOT;
        ecEncodeOpcode(cp, code);
        ecEncodeNumber(cp, slotNum);
        ecEncodeNumber(cp, nthBlock);
#if FUTURE
    }
#endif
    pushStack(cp, (cp->state->onLeft) ? -1 : 1);
}


static void genContinue(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);

    if (cp->state->code->jumps == 0 || !(cp->state->code->jumpKinds & EC_JUMP_CONTINUE)) {
        //  TODO - need line number info here
        genError(cp, np, "Illegal continue statement");

    } else {
        ecEncodeOpcode(cp, EJS_OP_GOTO);
        addJump(cp, np, EC_JUMP_CONTINUE);
        ecEncodeWord(cp, 0);
    }

    LEAVE(cp);
}


static void genDelete(EcCompiler *cp, EcNode *np)
{
    EcNode      *left;
    EjsName     *qname;

    ENTER(cp);

    mprAssert(np);

    left = np->left;
    mprAssert(left);

    switch (left->kind) {
    case N_DOT:
        processNode(cp, left->left);
        if (left->right->kind == N_QNAME) {
            ecEncodeOpcode(cp, EJS_OP_DELETE);
            qname = &left->right->qname;
            if (qname->name[0] == '.') {
                /* XML Descender */
                ecEncodeString(cp, &qname->name[1]);
            } else {
                ecEncodeString(cp, qname->name);
            }
            ecEncodeString(cp, qname->space);
            popStack(cp, 1);
        } else {
            processNode(cp, left->right);
            ecEncodeOpcode(cp, EJS_OP_DELETE_NAME_EXPR);
            popStack(cp, 2);
        }
        break;

    case N_QNAME:
        ecEncodeOpcode(cp, EJS_OP_DELETE_NAME);
        ecEncodeName(cp, &left->qname);
        break;

    default:
        mprAssert(0);
    }


    LEAVE(cp);
}


/*
 *  Global variable
 */
static void genGlobalName(EcCompiler *cp, int slotNum)
{
    int     code;

    mprAssert(slotNum >= 0);

    code = (!cp->state->onLeft) ?  EJS_OP_GET_GLOBAL_SLOT :  EJS_OP_PUT_GLOBAL_SLOT;
    ecEncodeOpcode(cp, code);

    mprAssert(slotNum < 256);
    ecEncodeByte(cp, slotNum);

    pushStack(cp, (cp->state->onLeft) ? -1 : 1);
}


/*
 *  Function local variable or argument reference
 */
static void genLocalName(EcCompiler *cp, int slotNum)
{
    int     code;

    mprAssert(slotNum >= 0);

    if (slotNum < 10) {
        code = (!cp->state->onLeft) ?  EJS_OP_GET_LOCAL_SLOT_0 :  EJS_OP_PUT_LOCAL_SLOT_0;
        ecEncodeOpcode(cp, code + slotNum);

    } else {
        code = (!cp->state->onLeft) ?  EJS_OP_GET_LOCAL_SLOT :  EJS_OP_PUT_LOCAL_SLOT;
        ecEncodeOpcode(cp, code);
        ecEncodeNumber(cp, slotNum);
    }
    pushStack(cp, (cp->state->onLeft) ? -1 : 1);
}


/*
 *  Generate code for a logical operator. Called by genBinaryOp
 *
 *  (expression OP expression)
 */
static void genLogicalOp(EcCompiler *cp, EcNode *np)
{
    EcState     *state;
    EcCodeGen   *saveCode;
    int         doneIfTrue, rightLen;

    ENTER(cp);

    state = cp->state;
    saveCode = state->code;

    mprAssert(np->kind == N_BINARY_OP);

    switch (np->tokenId) {
    case T_LOGICAL_AND:
        doneIfTrue = 0;
        break;

    case T_LOGICAL_OR:
        doneIfTrue = 1;
        break;

    default:
        doneIfTrue = 1;
        mprAssert(0);
        ecEncodeOpcode(cp, mapToken(cp, np->tokenId));
        break;
    }

    /*
     *  Process the conditional test. Put the pop for the branch here prior to the right hand side.
     */
    processNode(cp, np->left);
    ecEncodeOpcode(cp, EJS_OP_CAST_BOOLEAN);
    ecEncodeOpcode(cp, EJS_OP_DUP);
    pushStack(cp, 1);
    popStack(cp, 1);

    if (np->right) {
        state->code = allocCodeBuffer(cp);
        np->binary.rightCode = state->code;
        /*
         *  Evaluating right hand side, so we must pop the left side duped value.
         */
        ecEncodeOpcode(cp, EJS_OP_POP);
        popStack(cp, 1);
        processNode(cp, np->right);
        ecEncodeOpcode(cp, EJS_OP_CAST_BOOLEAN);
    }

    rightLen = mprGetBufLength(np->binary.rightCode->buf);

    /*
     *  Now copy the code to the output code buffer
     */
    setCodeBuffer(cp, saveCode);

    /*
     *  Jump to done if we know the result due to lazy evalation.
     */
    if (rightLen > 0 && rightLen < 0x7f && cp->optimizeLevel > 0) {
        ecEncodeOpcode(cp, (doneIfTrue) ? EJS_OP_BRANCH_TRUE_8: EJS_OP_BRANCH_FALSE_8);
        ecEncodeByte(cp, rightLen);
    } else {
        ecEncodeOpcode(cp, (doneIfTrue) ? EJS_OP_BRANCH_TRUE: EJS_OP_BRANCH_FALSE);
        ecEncodeWord(cp, rightLen);
    }

    copyCodeBuffer(cp, state->code, np->binary.rightCode);

    mprAssert(state == cp->state);

    LEAVE(cp);
}


/*
 *  Generate a property name reference based on the object already pushed.
 *  The owning object (pushed on the VM stack) may be an object or a type.
 */
static void genPropertyName(EcCompiler *cp, int slotNum)
{
    EcState     *state;
    int         code;

    mprAssert(slotNum >= 0);

    state = cp->state;

    if (slotNum < 10) {
        code = (!state->onLeft) ?  EJS_OP_GET_OBJ_SLOT_0 :  EJS_OP_PUT_OBJ_SLOT_0;
        ecEncodeOpcode(cp, code + slotNum);

    } else {
        code = (!state->onLeft) ?  EJS_OP_GET_OBJ_SLOT :  EJS_OP_PUT_OBJ_SLOT;
        ecEncodeOpcode(cp, code);
        ecEncodeNumber(cp, slotNum);
    }

    popStack(cp, 1);
    pushStack(cp, (state->onLeft) ? -1 : 1);
}


/*
 *  Generate a class property name reference
 *  The owning object (pushed on the VM stack) may be an object or a type. We must access its base class.
 */
static void genBaseClassPropertyName(EcCompiler *cp, int slotNum, int nthBase)
{
    EcState     *state;
    int         code;

    mprAssert(slotNum >= 0);

    state = cp->state;

    /* TODO - OPT. Want TYPE_PROP_0 style opcodes */
    code = (!cp->state->onLeft) ?  EJS_OP_GET_TYPE_SLOT :  EJS_OP_PUT_TYPE_SLOT;

    ecEncodeOpcode(cp, code);
    ecEncodeNumber(cp, slotNum);
    ecEncodeNumber(cp, nthBase);

    popStack(cp, 1);
    pushStack(cp, (state->onLeft) ? -1 : 1);
}


/*
 *  Generate a class property name reference
 *  The owning object (pushed on the VM stack) may be an object or a type. We must access its base class.
 */
static void genThisBaseClassPropertyName(EcCompiler *cp, EjsType *type, int slotNum)
{
    Ejs         *ejs;
    EcState     *state;
    int         code, nthBase;

    mprAssert(slotNum >= 0);
    mprAssert(type && ejsIsType(type));

    ejs = cp->ejs;
    state = cp->state;

    /*
     *  Count based up from object 
     */
    for (nthBase = 0; type->baseType; type = type->baseType) {
        nthBase++;
    }

    /* TODO - OPT. Want TYPE_PROP_0 style opcodes */
    code = (!state->onLeft) ?  EJS_OP_GET_THIS_TYPE_SLOT :  EJS_OP_PUT_THIS_TYPE_SLOT;
    ecEncodeOpcode(cp, code);
    ecEncodeNumber(cp, slotNum);
    ecEncodeNumber(cp, nthBase);

    pushStack(cp, (state->onLeft) ? -1 : 1);
}


/*
 *  Generate a class name reference or a global reference.
 *  TODO - rename function. Misleading in that it handles global
 */
static void genClassName(EcCompiler *cp, EjsType *type)
{
    Ejs         *ejs;
    int         slotNum;

    mprAssert(type);

    ejs = cp->ejs;

    if (type == (EjsType*) ejs->global) {
        ecEncodeOpcode(cp, EJS_OP_LOAD_GLOBAL);
        pushStack(cp, 1);
        return;
    }

    if (cp->bindGlobals || type->block.obj.var.native) {
        slotNum = ejsLookupProperty(ejs, ejs->global, &type->qname);
        mprAssert(slotNum >= 0);
        genGlobalName(cp, slotNum);

    } else {
        //  TODO - OPT. Need load name from global op code.
        ecEncodeOpcode(cp, EJS_OP_LOAD_GLOBAL);
        pushStack(cp, 1);
        ecEncodeOpcode(cp, EJS_OP_GET_OBJ_NAME);
        ecEncodeName(cp, &type->qname);
        popStack(cp, 1);
        pushStack(cp, 1);
    }
}


/*
 *  Generate a property reference in the current object
 */
static void genPropertyViaThis(EcCompiler *cp, int slotNum)
{
    Ejs             *ejs;
    EcState         *state;
    int             code;

    mprAssert(slotNum >= 0);

    ejs = cp->ejs;
    state = cp->state;

    /*
     *  Property in the current "this" object
     */
    if (slotNum < 10) {
        code = (!state->onLeft) ?  EJS_OP_GET_THIS_SLOT_0 :  EJS_OP_PUT_THIS_SLOT_0;
        ecEncodeOpcode(cp, code + slotNum);

    } else {
        code = (!state->onLeft) ?  EJS_OP_GET_THIS_SLOT :  EJS_OP_PUT_THIS_SLOT;
        ecEncodeOpcode(cp, code);
        ecEncodeNumber(cp, slotNum);
    }
    pushStack(cp, (cp->state->onLeft) ? -1 : 1);
}


/*
 *  Generate code for a bound name reference. We already know the slot for the property and its owning type.
 */
static void genBoundName(EcCompiler *cp, EcNode *np)
{
    Ejs         *ejs;
    EcState     *state;
    EjsLookup   *lookup;

    ENTER(cp);

    ejs = cp->ejs;
    state = cp->state;
    lookup = &np->lookup;

    mprAssert(lookup->slotNum >= 0);

    /*  TODO - refactor */

    if (lookup->obj == ejs->global) {
        /*
         *  Global variable.
         */
        //  TODO - remove slotNum test below
        if (lookup->slotNum < 0 || (!cp->bindGlobals && 
                (lookup->trait == 0 || !(lookup->trait->attributes & EJS_ATTR_BUILTIN)))) {
            lookup->slotNum = -1;
            genUnboundName(cp, np);

        } else {
            genGlobalName(cp, lookup->slotNum);
        }

    } else if (ejsIsFunction(lookup->obj) && lookup->nthBlock == 0) {
        genLocalName(cp, lookup->slotNum);

    } else if ((ejsIsBlock(lookup->obj) || ejsIsFunction(lookup->obj)) && 
            (!ejsIsType(lookup->obj) && !ejsIsInstanceBlock(lookup->obj))) {
        genBlockName(cp, lookup->slotNum, lookup->nthBlock);

    } else if (lookup->useThis) {
        if (lookup->instanceProperty) {
            /*
             *  Property being accessed via the current object "this" or an explicit object?
             */
            genPropertyViaThis(cp, lookup->slotNum);

        } else {
            genThisBaseClassPropertyName(cp, (EjsType*) lookup->obj, lookup->slotNum);
        }

    } else if (!state->currentObjectNode) {
        if (lookup->instanceProperty) {
            genBlockName(cp, lookup->slotNum, lookup->nthBlock);

        } else {
            /*
             *  Static property with no explicit object. ie. Not "obj.property". The property was found via a scope search.
             *  We ignore nthBase as we use the actual type (lookup->obj) where the property was found.
             */
            if (state->inClass && state->inFunction && state->currentFunction->staticMethod) {
                genThisBaseClassPropertyName(cp, (EjsType*) lookup->obj, lookup->slotNum);
                
            } else {
                if (state->inFunction && ejsIsA(ejs, (EjsVar*) state->currentClass, (EjsType*) lookup->obj)) {
                    genThisBaseClassPropertyName(cp, (EjsType*) lookup->obj, lookup->slotNum);
                    
                } else {
                    SAVE_ONLEFT(cp);
                    genClassName(cp, (EjsType*) lookup->obj);
                    RESTORE_ONLEFT(cp);
                    genPropertyName(cp, lookup->slotNum);
                }
            }
        }

    } else {
        /*
         *  Explicity object. ie. "obj.property". The object in a dot expression is already pushed on the stack.
         *  Determine if we can access the object itself or if we need to use the type of the object to access
         *  static properties.
         */
        if (lookup->instanceProperty) {
            genPropertyName(cp, lookup->slotNum);

        } else {
            /*
             *  Property is in the nth base class from the object already pushed on the stack (left hand side).
             */
            genBaseClassPropertyName(cp, lookup->slotNum, lookup->nthBase);
        }
    }
    LEAVE(cp);
}


static void processNodeGetValue(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);
    cp->state->needsValue = 1;
    processNode(cp, np);
    LEAVE(cp);
}


static int genCallArgs(EcCompiler *cp, EcNode *np) 
{
    if (np == 0) {
        return 0;
    }
    processNode(cp, np);
    return mprGetListCount(np->children);
}


static void genCallSequence(EcCompiler *cp, EcNode *np)
{
    Ejs             *ejs;
    EjsType         *type;
    EcNode          *left, *right;
    EcState         *state;
    EjsFunction     *fun;
    EjsLookup       *lookup;
    int             fast, argc, staticMethod;    
        
    ejs = cp->ejs;
    state = cp->state;
    left = np->left;
    right = np->right;
    lookup = &np->left->lookup;

    if (lookup->slotNum < 0 /* TODO || lookup->instanceProperty */) {
        /*
         *  Unbound or Function expression or instance variable containing a function. Can't use fast path op codes below.
         */
        if (left->kind == N_QNAME) {
            argc = genCallArgs(cp, right);
            ecEncodeOpcode(cp, EJS_OP_CALL_SCOPED_NAME);
            ecEncodeName(cp, &np->qname);
            
        } else if (left->kind == N_DOT && left->right->kind == N_QNAME) {
            processNodeGetValue(cp, left->left);
            argc = genCallArgs(cp, right);
            ecEncodeOpcode(cp, EJS_OP_CALL_OBJ_NAME);
            ecEncodeName(cp, &np->qname);
            popStack(cp, 1);
            
        } else {
            //  TODO - is this right. What should this be and where does it come from?
            ecEncodeOpcode(cp, EJS_OP_LOAD_GLOBAL);
            processNodeGetValue(cp, left);
            argc = genCallArgs(cp, right);
            ecEncodeOpcode(cp, EJS_OP_CALL);
            popStack(cp, 1);
        }
        ecEncodeNumber(cp, argc); 
        popStack(cp, argc);
        return;
    }
        
    fun = (EjsFunction*) lookup->ref;
    staticMethod = (ejsIsFunction(fun) && fun->staticMethod);
        
    /*
     *  Use fast opcodes when the call sequence is bindable and either:
     *      expression.name()
     *      name
     */
    fast = (left->kind == N_DOT && left->right->kind == N_QNAME) || left->kind == N_QNAME;      
        
    if (!fast) {
        /*
         *  Resolve a reference to a function expression
         */
        processNodeGetValue(cp, left);
        argc = genCallArgs(cp, right);
        ecEncodeOpcode(cp, EJS_OP_CALL);
        popStack(cp, 1);
        ecEncodeNumber(cp, argc); 
        popStack(cp, argc);
        return;
    }
    
    if (staticMethod) {
        mprAssert(ejsIsType(lookup->obj));
        if (state->currentClass && state->inFunction && 
                ejsIsA(ejs, (EjsVar*) state->currentClass, (EjsType*) lookup->originalObj)) {
            /*
             *  Calling a static method from within a subclass. So we can use "this"
             */
            argc = genCallArgs(cp, right);
            ecEncodeOpcode(cp, EJS_OP_CALL_THIS_STATIC_SLOT);
            ecEncodeNumber(cp, lookup->slotNum);
            /*
             *  If searching the scope chain (i.e. without a qualifying obj.property), and if the current class is not the 
             *  original object (i.e.  SomeClass.property), then see how far back on the inheritance chain we must go.
             */
            if (lookup->originalObj != lookup->obj) {
                for (type = state->currentClass; type != (EjsType*) lookup->originalObj; type = type->baseType) {
                    lookup->nthBase++;
                }
            }
            if (!state->currentFunction->staticMethod) {
                /*
                 *  If calling from within an instance function, need to step over the instance also
                 */
                lookup->nthBase++;
            }
            ecEncodeNumber(cp, lookup->nthBase);
            
        } else if (left->kind == N_DOT && left->right->kind == N_QNAME) {
            /*
             *  Calling a static method with an explicit object or expression. Call via the object.
             */
            processNode(cp, left->left);
            argc = genCallArgs(cp, right);
            ecEncodeOpcode(cp, EJS_OP_CALL_OBJ_STATIC_SLOT);
            ecEncodeNumber(cp, lookup->slotNum);
            if (lookup->ownerIsType) {
                lookup->nthBase--;
            }
            ecEncodeNumber(cp, lookup->nthBase);
            popStack(cp, 1);
            
        } else {
            /*
             *  Foreign static method. Call directly on the correct class type object.
             */
            genClassName(cp, (EjsType*) lookup->obj);
            argc = genCallArgs(cp, right);
            ecEncodeOpcode(cp, EJS_OP_CALL_OBJ_STATIC_SLOT);
            ecEncodeNumber(cp, lookup->slotNum);
            ecEncodeNumber(cp, 0);
            popStack(cp, 1);
        }
        
    } else {
        /*
         *  Instance function or type being invoked as a constructor (e.g. Reflect(obj))
         */
        if (left->kind == N_DOT && left->right->kind == N_QNAME) {
            if (left->left->kind == N_THIS) {
                lookup->useThis = 1;
            }
        }
        
        if (lookup->useThis && !lookup->instanceProperty) {
            argc = genCallArgs(cp, right);
            ecEncodeOpcode(cp, EJS_OP_CALL_THIS_SLOT);
            ecEncodeNumber(cp, lookup->slotNum);
            
        } else if (lookup->obj == ejs->global) {
            argc = genCallArgs(cp, right);
            ecEncodeOpcode(cp, EJS_OP_CALL_GLOBAL_SLOT);
            ecEncodeNumber(cp, lookup->slotNum);
            
        } else if (lookup->instanceProperty && left->left) {
            processNodeGetValue(cp, left->left);
            argc = genCallArgs(cp, right);
            ecEncodeOpcode(cp, EJS_OP_CALL_OBJ_INSTANCE_SLOT);
            ecEncodeNumber(cp, lookup->slotNum);
            popStack(cp, 1);
            
        } else if (ejsIsType(lookup->obj) || ejsIsInstanceBlock(lookup->obj)) {
            if (left->kind == N_DOT && left->right->kind == N_QNAME) {
                processNodeGetValue(cp, left->left);
                argc = genCallArgs(cp, right);
                ecEncodeOpcode(cp, EJS_OP_CALL_OBJ_SLOT);
                ecEncodeNumber(cp, lookup->slotNum);
                popStack(cp, 1);
                
            } else {
                processNodeGetValue(cp, left);
                argc = genCallArgs(cp, right);
                ecEncodeOpcode(cp, EJS_OP_CALL);
                popStack(cp, 1);
            }
            
        } else if (ejsIsBlock(lookup->obj)) {
            argc = genCallArgs(cp, right);
            ecEncodeOpcode(cp, EJS_OP_CALL_BLOCK_SLOT);
            ecEncodeNumber(cp, lookup->slotNum);
            ecEncodeNumber(cp, lookup->nthBlock);


        } else {
            //  TODO - should this not be OP_CALL
            mprAssert(0);
            processNodeGetValue(cp, left->left);
            argc = genCallArgs(cp, right);
            ecEncodeOpcode(cp, EJS_OP_CALL_OBJ_SLOT);
            ecEncodeNumber(cp, lookup->slotNum);
            popStack(cp, 1);
        }
    }
    ecEncodeNumber(cp, argc); 
    popStack(cp, argc);
}


/*
 *  Code generation for function calls
 */
static void genCall(EcCompiler *cp, EcNode *np)
{
    Ejs             *ejs;
    EcNode          *left, *right;
    EcState         *state;
    EjsFunction     *fun;
    int             argc, hasResult;

    ENTER(cp);

    ejs = cp->ejs;
    state = cp->state;
    left = np->left;
    right = np->right;
    fun = (EjsFunction*) np->lookup.ref;    
    
    if (left->kind == N_NEW && !left->newExpr.callConstructors) {
        processNode(cp, left);
        LEAVE(cp);
        return;
    }

    if (left->kind == N_NEW) {
        processNode(cp, left);
        argc = genCallArgs(cp, right);
        ecEncodeOpcode(cp, EJS_OP_CALL_CONSTRUCTOR);
        ecEncodeNumber(cp, argc);
        popStack(cp, argc);
        LEAVE(cp);
        return;
    }
    
    genCallSequence(cp, np);


    /*
     *  Process the function return value. Call by ref has a this pointer plus method reference plus args
     */
    hasResult = 0;
    if (fun && ejsIsFunction(fun)) {
        if (fun->resultType && fun->resultType != ejs->voidType) {
            hasResult = 1;

        } else if (fun->hasReturn) {
            /*
             *  Untyped function, but it has a return stmt.
             *  We don't do data flow to make sure all return cases have returns (sorry).
             */
            hasResult = 1;
        }
        if (state->needsValue && !hasResult) {
            genError(cp, np, "Function call does not return a value.");
        }
    }

    if (state->needsValue) {
        ecEncodeOpcode(cp, EJS_OP_PUSH_RESULT);
        pushStack(cp, 1);
    }

    LEAVE(cp);
}


static void genCatchArg(EcCompiler *cp, EcNode *np)
{
    ecEncodeOpcode(cp, EJS_OP_PUSH_CATCH_ARG);
    pushStack(cp, 1);
}


/*
 *  Process a class node.
 */
static void genClass(EcCompiler *cp, EcNode *np)
{
    Ejs             *ejs;
    EjsType         *type, *baseType;
    EjsFunction     *constructor;
    EjsEx           *ex;
    EcCodeGen       *code;
    EcState         *state;
    EcNode          *constructorNode;
    EjsName         qname;
    MprBuf          *codeBuf;
    uchar           *buf;
    int             next, len, initializerLen, constructorLen, slotNum;

    ENTER(cp);

    mprAssert(np->kind == N_CLASS);

    ejs = cp->ejs;
    state = cp->state;
    type = (EjsType*) np->klass.ref;
    mprAssert(type);

    state->inClass = 1;

    /*
     *  Op code to define the class. This goes into the module code buffer. DefineClass will capture the current scope
     *  including the internal namespace for this file.
     *  OPT See above todo
     */
    ecEncodeOpcode(cp, EJS_OP_DEFINE_CLASS);
    ecEncodeGlobal(cp, (EjsVar*) type, &type->qname);

    state->letBlock = (EjsVar*) type;
    state->varBlock = (EjsVar*) type;
    state->currentClass = type;
    state->currentClassNode = np;

    constructorNode = np->klass.constructor;

    /*
     *  Create code buffers to hold the static and instance level initialization code. The AST module will always
     *  create a constructor node for us if there is instance level initialization code. We currently put the class
     *  initialization code in the constructor. Static variable initialization code will go into the current
     *  module buffer (cp->currentModule) and will be run when the module is loaded. BUG - CLASS INITIALIZATION ORDERING.
     */
    mprAssert(state->code == state->currentModule->code);

    /*
     *  Create a code buffer for static initialization code and set it as the default buffer
     */
    state->code = state->staticCodeBuf = allocCodeBuffer(cp);

    if (type->hasConstructor) {
        state->instanceCodeBuf = allocCodeBuffer(cp);
    }

    /*
     *  The current code buffer is the module code buffer. genVar will redirect to the instanceCodeBuf as required.
     */
    if (!type->isInterface) {
        mprAssert(np->left->kind == N_DIRECTIVES);
        processNode(cp, np->left);
    }

    if (type->hasStaticInitializer) {
        /*
         *  Create the static initializer
         */
        ecEncodeOpcode(cp, EJS_OP_RETURN);
        setFunctionCode(cp, np->klass.initializer, state->staticCodeBuf);
    }

    if (type->hasConstructor) {

        mprAssert(constructorNode);
        mprAssert(state->instanceCodeBuf);
        code = state->code = state->instanceCodeBuf;
        codeBuf = code->buf;

        constructor = state->currentFunction = constructorNode->function.functionVar;

        mprAssert(constructor);

        state->currentFunctionName = constructorNode->qname.name;

        if (constructorNode->function.isDefaultConstructor) {
            /*
             *  Generate the default constructor. Append the default constructor instructions after any initialization code.
             */
            baseType = type->baseType;
            if (baseType && baseType->hasConstructor) {
                ecEncodeOpcode(cp, EJS_OP_CALL_NEXT_CONSTRUCTOR);
                ecEncodeNumber(cp, 0);
            }
            ecEncodeOpcode(cp, EJS_OP_RETURN);
            setFunctionCode(cp, constructor, code);

            //  TODO - comment as to why we have this here?
            ecAddConstant(cp, EJS_PUBLIC_NAMESPACE);
            ecAddConstant(cp, EJS_CONSTRUCTOR_NAMESPACE);

        } else if (type->hasInitializer) {

            /*
             *  Inject initializer code into the pre-existing constructor code. It is injected before any constructor code.
             */
            //  TODO - make sure super is being called for non-default constructors

            initializerLen = mprGetBufLength(codeBuf);
            mprAssert(initializerLen >= 0);
            constructorLen = constructor->body.code.codeLen;
            mprAssert(constructorLen >= 0);

            len = initializerLen + constructorLen;
            if (len > 0) {
                buf = (uchar*) mprAlloc(state, len);
                if (buf == 0) {
                    genError(cp, np, "Can't allocate code buffer");
                    LEAVE(cp);
                }

                mprMemcpy((char*) buf, initializerLen, mprGetBufStart(codeBuf), initializerLen);
                if (constructorLen) {
                    mprMemcpy((char*) &buf[initializerLen], constructorLen, (char*) constructor->body.code.byteCode, 
                        constructorLen);
                }
                ejsSetFunctionCode(constructor, buf, len);

                /*
                 *  Define any try/catch blocks encountered
                 */
                next = 0;
                while ((ex = (EjsEx*) mprGetNextItem(code->exceptions, &next)) != 0) {
                    ejsAddException(constructor, ex->tryStart, ex->tryEnd, ex->catchType, ex->handlerStart, ex->handlerEnd, 
                        ex->flags, -1);
                }
            }
        }
    }

    /*
     *  Add extra constants
     */
    ecAddNameConstant(cp, &np->qname);

    if (type->hasStaticInitializer) {
        slotNum = type->block.numInherited;
        if (type->hasConstructor) {
            slotNum++;
        }
        qname = ejsGetPropertyName(ejs, (EjsVar*) type, slotNum);
        ecAddNameConstant(cp, &qname);
    }
    if (type->baseType) {
        ecAddNameConstant(cp, &type->baseType->qname);
    }

    /*
     *  Emit any properties implemented via another class (there is no Node for these)
     */
    ecAddBlockConstants(cp, (EjsBlock*) type);
    if (type->instanceBlock) {
        ecAddBlockConstants(cp, (EjsBlock*) type->instanceBlock);
    }

#if BLD_FEATURE_EJS_DOC
    if (cp->ejs->flags & EJS_FLAG_DOC) {
        ecAddDocConstant(cp, np->lookup.trait, ejs->global, np->lookup.slotNum);
    }
#endif

    LEAVE(cp);
}


static void genDirectives(EcCompiler *cp, EcNode *np, bool resetStack)
{
    EcState     *lastDirectiveState;
    EcNode      *child;
    int         next, lastKind;

    ENTER(cp);

    //  TODO - directiveState appears to be not used
    lastDirectiveState = cp->directiveState;
    mprAssert(cp->state->code->stackCount == 0);

    lastKind = -1;
    next = 0;
    while ((child = getNextNode(cp, np, &next)) && !cp->error) {

        lastKind = child->kind;
        cp->directiveState = cp->state;

        processNode(cp, child);

        if (resetStack) {
            emptyStack(cp, 0);
        }
    }

    cp->directiveState = lastDirectiveState;

    LEAVE(cp);
}


/*
 *  Handle property dereferencing via "." and "[". This routine generates code for bound properties where we know
 *  the slot offsets and also for unbound references. Return the right most node in right.
 */
static void genDot(EcCompiler *cp, EcNode *np, EcNode **rightMost)
{
    EcState     *state;
    EcNode      *left, *right;

    ENTER(cp);

    state = cp->state;
    state->onLeft = 0;
    left = np->left;
    right = np->right;

    /*
     *  Process the left of the dot and leave an object reference on the stack
     */
    switch (left->kind) {
    case N_DOT:
    case N_EXPRESSIONS:
    case N_LITERAL:
    case N_THIS:
    case N_REF:
    case N_QNAME:
    case N_CALL:
        state->needsValue = 1;
        processNode(cp, left);
        state->needsValue = state->prev->needsValue;
        break;

    case N_ARRAY_LITERAL:
        processNode(cp, left);
        break;

    default:
        mprAssert(0);
    }

    state->currentObjectNode = np->left;

    //  TODO - is this being used anymore
    if (np->needThis) {
        mprAssert(0);
        ecEncodeOpcode(cp, EJS_OP_DUP);
        pushStack(cp, 1);
        np->needThis = 0;
    }

    /*
     *  Process the right
     */
    switch (right->kind) {
    case N_CALL:
        state->needsValue = state->prev->needsValue;
        genCall(cp, right);
        state->needsValue = 0;
        break;

    case N_QNAME:
        state->onLeft = state->prev->onLeft;
        genName(cp, right);
        break;

    default:
    case N_DOT:            
    case N_EXPRESSIONS:
        state->currentObjectNode = 0;
        processNode(cp, np->right);
        state->onLeft = state->prev->onLeft;
        genExpressionName(cp);
        break;

    case N_LITERAL:
        genLiteral(cp, right);
        state->onLeft = state->prev->onLeft;
        genExpressionName(cp);
        break;

    case N_OBJECT_LITERAL:
        genObjectLiteral(cp, right);
        state->onLeft = state->prev->onLeft;
        genExpressionName(cp);
        break;

    case N_SUPER:
        ecEncodeOpcode(cp, EJS_OP_SUPER);
        break;
    }

    if (rightMost) {
        *rightMost = right;
    }

    LEAVE(cp);
}


static void genEndFunction(EcCompiler *cp, EcNode *np)
{
#if UNUSED
    EcNode          *funNode;
#endif
    EjsFunction     *fun;

    ENTER(cp);

    mprAssert(np);

    fun = cp->state->currentFunction;
#if UNUSED
    funNode = np->endfunction.function;
#endif
    
    if (cp->lastOpcode != EJS_OP_RETURN_VALUE && cp->lastOpcode != EJS_OP_RETURN) {
        /*
         *  Ensure code cannot run off the end of a method.
         *  TODO OPT - must do a better job of basic block analysis and check if all paths out of a function have a return.
         */
        if (fun->resultType == 0) {
#if UNUSED
            if (funNode->function.noBlock) {
                /* Single expression function without a block. Expression is already on the stack */
                ecEncodeOpcode(cp, EJS_OP_RETURN_VALUE);
            } else 
#endif
            if (fun->hasReturn) {
                //  TODO - OPT. Should be able to avoid this somehow. We put it here now to ensure that all
                //  paths out of the function terminate with a return.
                ecEncodeOpcode(cp, EJS_OP_LOAD_NULL);
                ecEncodeOpcode(cp, EJS_OP_RETURN_VALUE);
            } else {
                ecEncodeOpcode(cp, EJS_OP_RETURN);
            }

        } else if (fun->resultType == cp->ejs->voidType) {
            ecEncodeOpcode(cp, EJS_OP_RETURN);

        } else {
#if UNUSED
            if (funNode->function.noBlock) {
                /* Single expression function without a block. Expression is already on the stack */
                ecEncodeOpcode(cp, EJS_OP_RETURN_VALUE);
            } else {
#endif
                //  TODO - OPT. Should be able to avoid this somehow.
                ecEncodeOpcode(cp, EJS_OP_LOAD_NULL);
                ecEncodeOpcode(cp, EJS_OP_RETURN_VALUE);
#if UNUSED
            }
#endif
        }

        addDebugInstructions(cp, np);
    }

    LEAVE(cp);
}


/*
 *  Generate code for a name reference by expression.
 */
static void genExpressionName(EcCompiler *cp)
{
    int         code;

    ENTER(cp);

    /*
     *  Unqualified property name (requires obj on stack)
     */
    code = (!cp->state->onLeft) ? EJS_OP_GET_OBJ_NAME_EXPR :  EJS_OP_PUT_OBJ_NAME_EXPR;
    ecEncodeOpcode(cp, code);

    /* store: -3, load: -1 */

    popStack(cp, 2);
    pushStack(cp, (cp->state->onLeft) ? -1 : 1);

    LEAVE(cp);
}


static void genExpressions(EcCompiler *cp, EcNode *np)
{
    EcNode      *child;
    int         next;

    mprAssert(np->kind == N_EXPRESSIONS);

    ENTER(cp);

    next = 0;
    while ((child = getNextNode(cp, np, &next)) != 0) {
        processNode(cp, child);
    }

    LEAVE(cp);
}


/*
 *  This handles "do { ... } while" constructs.
 *
 *  do {
 *       body
 *  } while (conditional)
 *
 *  Labels:
 *      topOfLoop:
 *          body
 *      continueLabel:
 *          conditional
 *          bxx topOfLoop
 *      endLoop:
 */
static void genDo(EcCompiler *cp, EcNode *np)
{
    EcCodeGen   *outerBlock, *code;
    EcState     *state;
    int         condLen, bodyLen, len, condShortJump, continueLabel, breakLabel;

    ENTER(cp);

    state = cp->state;

    mprAssert(np->kind == N_DO);

    outerBlock = state->code;
    code = state->code = allocCodeBuffer(cp);

    ecStartBreakableStatement(cp, EC_JUMP_BREAK | EC_JUMP_CONTINUE);

    if (np->forLoop.body) {
        np->forLoop.bodyCode = state->code = allocCodeBuffer(cp);
        setStack(cp, 0);
        processNode(cp, np->forLoop.body);
        emptyStack(cp, 0);
    }

    if (np->forLoop.cond) {
        np->forLoop.condCode = state->code = allocCodeBuffer(cp);
        processNode(cp, np->forLoop.cond);
        /* Leaves one item on the stack */
    }

    /*
     *  Get the lengths of code blocks
     */
    condLen = bodyLen = 0;

    if (np->forLoop.condCode) {
        condLen = mprGetBufLength(np->forLoop.condCode->buf);
    }
    if (np->forLoop.bodyCode) {
        bodyLen = mprGetBufLength(np->forLoop.bodyCode->buf);
    }

    /*
     *  Now that we know the body length, we can calculate the jump back to the top.
     */
    //  TODO - need defines for 0x75, 2 and 5
    condShortJump = 0;
    len = bodyLen + condLen;
    if (len > 0) {
        if (len < 0x7f && cp->optimizeLevel > 0) {
            condShortJump = 1;
            condLen += 2;

        } else {
            condLen += 5;
        }
    }

    setCodeBuffer(cp, code);
    continueLabel = mprGetBufLength(cp->state->code->buf);

    /*
     *  Add the body
     */
    if (np->forLoop.bodyCode) {
        copyCodeBuffer(cp, state->code, np->forLoop.bodyCode);
    }


    /*
     *  Copy the conditional code and add condition jump to the end of the for loop, then copy the body code.
     */
    if (np->forLoop.condCode) {
        copyCodeBuffer(cp, state->code, np->forLoop.condCode);
        setStack(cp, 1);
        len = bodyLen + condLen;
        if (condShortJump) {
            ecEncodeOpcode(cp, EJS_OP_BRANCH_TRUE_8);
            ecEncodeByte(cp, -len);
        } else {
            ecEncodeOpcode(cp, EJS_OP_BRANCH_TRUE);
            ecEncodeWord(cp, -len);
        }
        popStack(cp, 1);
    }

    breakLabel = mprGetBufLength(cp->state->code->buf);
    patchJumps(cp, EC_JUMP_BREAK, breakLabel);
    patchJumps(cp, EC_JUMP_CONTINUE, continueLabel);

    copyCodeBuffer(cp, outerBlock, state->code);

    LEAVE(cp);
}


/*
 *  This handles "for" and while" constructs but not "for .. in"
 *
 *  for (initializer; conditional; perLoop) { body }
 *
 *  Labels:
 *          initializer
 *      topOfLoop:
 *          conditional
 *          bxx endLoop
 *      topOfBody:
 *          body
 *      continueLabel:
 *          perLoop
 *      endIteration:
 *          goto topOfLoop
 *      endLoop:
 *
 *  TODO OPT - Rotate loop
 */
static void genFor(EcCompiler *cp, EcNode *np)
{
    EcCodeGen   *outerBlock, *code;
    EcState     *state;
    int         condLen, bodyLen, perLoopLen, len, condShortJump, perLoopShortJump, continueLabel, breakLabel;

    ENTER(cp);

    mprAssert(np->kind == N_FOR);

    state = cp->state;
    outerBlock = state->code;
    code = state->code = allocCodeBuffer(cp);

    /*
     *  initializer is outside the loop
     */
    if (np->forLoop.initializer) {
        processNode(cp, np->forLoop.initializer);
        emptyStack(cp, 0);
    }

    /*
     *  For conditional
     */
    ecStartBreakableStatement(cp, EC_JUMP_BREAK | EC_JUMP_CONTINUE);

    if (np->forLoop.cond) {
        np->forLoop.condCode = state->code = allocCodeBuffer(cp);
        processNode(cp, np->forLoop.cond);
        /* Leaves one item on the stack */
    }

    if (np->forLoop.body) {
        np->forLoop.bodyCode = state->code = allocCodeBuffer(cp);
        setStack(cp, 0);
        processNode(cp, np->forLoop.body);
        emptyStack(cp, 0);
    }

    /*
     *  Per loop iteration
     */
    if (np->forLoop.perLoop) {
        np->forLoop.perLoopCode = state->code = allocCodeBuffer(cp);
        processNode(cp, np->forLoop.perLoop);
        emptyStack(cp, 0);
    }

    /*
     *  Get the lengths of code blocks
     */
    perLoopLen = condLen = bodyLen = 0;

    if (np->forLoop.condCode) {
        condLen = mprGetBufLength(np->forLoop.condCode->buf);
    }
    if (np->forLoop.bodyCode) {
        bodyLen = mprGetBufLength(np->forLoop.bodyCode->buf);
    }
    if (np->forLoop.perLoopCode) {
        perLoopLen = mprGetBufLength(np->forLoop.perLoopCode->buf);
    }

    /*
     *  Now that we know the body length, we can calculate the jump at the top. This is the shorter of
     *  the two jumps as it does not span the conditional code, so we optimize it first incase the saving
     *  of 3 bytes allows us to also optimize the branch back to the top. Subtract 5 to the test with 0x7f to
     *  account for the worst-case jump at the bottom back to the top
     *  TODO - need defines for 2 & 5 & 0x7f.
     */
    condShortJump = 0;
    if (condLen > 0) {
        len = bodyLen + perLoopLen;
        if (len < (0x7f - 5) && cp->optimizeLevel > 0) {
            condShortJump = 1;
            condLen += 2;

        } else {
            condLen += 5;
        }
    }

    /*
     *  Calculate the jump back to the top of the loop (per-iteration jump). Subtract 5 to account for the worst case
     *  where the per loop jump is a long jump.
     */
    len = condLen + bodyLen + perLoopLen;
    if (len < (0x7f - 5) && cp->optimizeLevel > 0) {
        perLoopShortJump = 1;
        perLoopLen += 2;
    } else {
        perLoopShortJump = 0;
        perLoopLen += 5;
    }

    /*
     *  Copy the conditional code and add condition jump to the end of the for loop, then copy the body code.
     */
    setCodeBuffer(cp, code);
    if (np->forLoop.condCode) {
        copyCodeBuffer(cp, state->code, np->forLoop.condCode);
        setStack(cp, 1);
        len = bodyLen + perLoopLen;
        if (condShortJump) {
            ecEncodeOpcode(cp, EJS_OP_BRANCH_FALSE_8);
            ecEncodeByte(cp, len);
        } else {
            ecEncodeOpcode(cp, EJS_OP_BRANCH_FALSE);
            ecEncodeWord(cp, len);
        }
        popStack(cp, 1);
    }

    /*
     *  Add the body and per loop code
     */
    if (np->forLoop.bodyCode) {
        copyCodeBuffer(cp, state->code, np->forLoop.bodyCode);
    }
    continueLabel = mprGetBufLength(state->code->buf);
    if (np->forLoop.perLoopCode) {
        copyCodeBuffer(cp, state->code, np->forLoop.perLoopCode);
    }

    /*
     *  Add the per-loop jump back to the top of the loop
     */
    len = condLen + bodyLen + perLoopLen;
    if (perLoopShortJump) {
        ecEncodeOpcode(cp, EJS_OP_GOTO_8);
        ecEncodeByte(cp, -len);
    } else {
        ecEncodeOpcode(cp, EJS_OP_GOTO);
        ecEncodeWord(cp, -len);
    }

    breakLabel = mprGetBufLength(state->code->buf);
    patchJumps(cp, EC_JUMP_BREAK, breakLabel);
    patchJumps(cp, EC_JUMP_CONTINUE, continueLabel);

    copyCodeBuffer(cp, outerBlock, state->code);

    LEAVE(cp);
}


/*
 *  This routine is a little atypical in that it hand-crafts an exception block.
 */
static void genForIn(EcCompiler *cp, EcNode *np)
{
    EcCodeGen   *outerBlock, *code;
    EcState     *state;
    int         len, breakLabel, tryStart, tryEnd, handlerStart;

    ENTER(cp);

    mprAssert(cp->state->code->stackCount == 0);
    mprAssert(np->kind == N_FOR_IN);

    state = cp->state;
    outerBlock = state->code;
    code = state->code = allocCodeBuffer(cp);

    ecStartBreakableStatement(cp, EC_JUMP_BREAK | EC_JUMP_CONTINUE);

    processNode(cp, np->forInLoop.iterVar);

    /*
     *  Consider:
     *      for (i in obj.get())
     *          body
     *
     *  Now process the obj.get()
     */
    np->forInLoop.initCode = state->code = allocCodeBuffer(cp);

    mprAssert(cp->state->code->stackCount == 0);

    processNode(cp, np->forInLoop.iterGet);
    ecEncodeOpcode(cp, EJS_OP_PUSH_RESULT);
    pushStack(cp, 1);
    
    mprAssert(cp->state->code->stackCount == 1);

    /*
     *  Process the iter.next()
     */
    np->forInLoop.bodyCode = state->code = allocCodeBuffer(cp);

    /*
     *  Dup the iterator reference each time round the loop as iter.next() will consume the object.
     *  TODO - OPT. Consider having a CALL op code that does not consume the object.
     */
    ecEncodeOpcode(cp, EJS_OP_DUP);
    pushStack(cp, 1);

    /*
     *  Emit code to invoke the iterator
     */
    tryStart = getCodeLength(cp, np->forInLoop.bodyCode);

    ecEncodeOpcode(cp, EJS_OP_CALL_OBJ_SLOT);
    ecEncodeNumber(cp, np->forInLoop.iterNext->lookup.slotNum);
    ecEncodeNumber(cp, 0);
    popStack(cp, 1);
    
    if (np->forInLoop.each) {
        /*
         *  getObjName
         */
    }
    tryEnd = getCodeLength(cp, np->forInLoop.bodyCode);

    /*
     *  Save the result of the iter.next() call
     */
    ecEncodeOpcode(cp, EJS_OP_PUSH_RESULT);
    pushStack(cp, 1);
    genLeftHandSide(cp, np->forInLoop.iterVar->left);

    if (np->forInLoop.iterVar->kind == N_VAR_DEFINITION && np->forInLoop.iterVar->def.varKind == KIND_LET) {
        ecAddConstant(cp, np->forInLoop.iterVar->left->qname.name);
        ecAddConstant(cp, np->forInLoop.iterVar->left->qname.space);
    }

    /*
     *  Now the loop body. Must hide the pushed iterator on the stack as genDirectives will clear the stack.
     */
    if (np->forInLoop.body) {
        state->code->stackCount--;
        mprAssert(state->code->stackCount == 0);
        processNode(cp, np->forInLoop.body);
        state->code->stackCount++;
    }
    emptyStack(cp, 1);

    len = getCodeLength(cp, np->forInLoop.bodyCode);
    if (len < (0x7f - 5)) {
        ecEncodeOpcode(cp, EJS_OP_GOTO_8);
        len += 2;
        ecEncodeByte(cp, -len);
    } else {
        ecEncodeOpcode(cp, EJS_OP_GOTO);
        len += 5;
        ecEncodeWord(cp, -len);
    }

    /*
     *  Create exception catch block around iter.next() to catch the StopIteration exception.
     *  Note: we have a zero length handler (noop)
     */
    handlerStart = ecGetCodeOffset(cp);
    addException(cp, tryStart, tryEnd, cp->ejs->stopIterationType, handlerStart, handlerStart,
        EJS_EX_CATCH | EJS_EX_ITERATION);

    emptyStack(cp, 0);

    /*
     *  Patch break/continue statements
     */
    breakLabel = mprGetBufLength(state->code->buf);
    patchJumps(cp, EC_JUMP_BREAK, breakLabel);
    patchJumps(cp, EC_JUMP_CONTINUE, 0);

    /*
     *  Copy the code fragments to the outer code buffer
     */
    setCodeBuffer(cp, code);
    copyCodeBuffer(cp, state->code, np->forInLoop.initCode);
    copyCodeBuffer(cp, state->code, np->forInLoop.bodyCode);

    copyCodeBuffer(cp, outerBlock, state->code);

    LEAVE(cp);
}


/*
 *  Generate code for default parameters. Native classes must handle this themselves. We
 *  generate the code for all default parameters in sequence with a computed goto at the front.
 */
static void genDefaultParameterCode(EcCompiler *cp, EcNode *np, EjsFunction *fun)
{
    EcNode          *parameters, *child;
    EcState         *state;
    EcCodeGen       **buffers, *saveCode;
    int             next, len, needLongJump, count, firstDefault;

    state = cp->state;
    saveCode = state->code;

    parameters = np->function.parameters;
    mprAssert(parameters);

    count = mprGetListCount(parameters->children);
    buffers = (EcCodeGen**) mprAllocZeroed(state, count * sizeof(EcCodeGen*));

    for (next = 0; (child = getNextNode(cp, parameters, &next)) && !cp->error; ) {
        mprAssert(child->kind == N_VAR_DEFINITION);

        if (child->left->kind == N_ASSIGN_OP) {
            buffers[next - 1] = state->code = allocCodeBuffer(cp);
            genAssignOp(cp, child->left);
        }
    }

    firstDefault = fun->numArgs - fun->numDefault;
    mprAssert(firstDefault >= 0);
    needLongJump = cp->optimizeLevel > 0 ? 0 : 1;

    /*
     *  Compute the worst case jump size. Start with 4 because the table is always one larger than the
     *  number of default args.
     */
    len = 4;
    for (next = firstDefault; next < count; next++) {
        len = mprGetBufLength(buffers[next]->buf) + 4;
        if (len >= 0x7f) {
            needLongJump = 1;
            break;
        }
    }

    setCodeBuffer(cp, saveCode);

    /*
     *  This is a jump table where each parameter initialization segments falls through to the next one.
     *  We have one more entry in the table to jump over the entire computed jump section.
     */
    ecEncodeOpcode(cp, (needLongJump) ? EJS_OP_INIT_DEFAULT_ARGS: EJS_OP_INIT_DEFAULT_ARGS_8);
    ecEncodeNumber(cp, fun->numDefault + 1);

    len = (fun->numDefault + 1) * ((needLongJump) ? 4 : 1);

    for (next = firstDefault; next < count; next++) {
        if (needLongJump) {
            ecEncodeWord(cp, len);
        } else {
            ecEncodeByte(cp, len);
        }
        len += mprGetBufLength(buffers[next]->buf);
    }
    /*
     *  Add one more jump to jump over the entire jump table
     */
    if (needLongJump) {
        ecEncodeWord(cp, len);
    } else {
        ecEncodeByte(cp, len);
    }

    /*
     *  Now copy all the initialization code
     */
    for (next = firstDefault; next < count; next++) {
        copyCodeBuffer(cp, state->code, buffers[next]);
    }
    mprFree(buffers);
}


static void genFunction(EcCompiler *cp, EcNode *np)
{
    Ejs             *ejs;
    EjsEx           *ex;
    EcState         *state;
    EcCodeGen       *code;
    EjsFunction     *fun;
    EjsType         *baseType;
    EjsName         qname;
    EjsTrait        *trait;
    EjsLookup       *lookup;
    int             i;

    ENTER(cp);

    mprAssert(np->kind == N_FUNCTION);

    ejs = cp->ejs;
    state = cp->state;
    mprAssert(state);

    mprAssert(np->function.functionVar);
    fun = np->function.functionVar;

    state->inFunction = 1;
    state->inMethod = state->inMethod || np->function.isMethod;
    state->blockIsMethod = np->function.isMethod;
    state->currentFunctionName = np->qname.name;
    state->currentFunction = fun;

    /*
     *  Capture the scope chain by the defineFunction op code. Emit this into the existing code buffer. Only do if not a method.
     *  Native methods don't get to see the scope chain directly.
     */
    if (!(np->attributes & EJS_ATTR_NATIVE) && fun->fullScope) {
        if (!np->function.isMethod) {
            lookup = &np->lookup;
            if (lookup->slotNum >= 0) {
                if (lookup->obj == ejs->global) {
                    ecEncodeOpcode(cp, EJS_OP_DEFINE_GLOBAL_FUNCTION);
                    ecEncodeGlobal(cp, (EjsVar*) lookup->ref, &np->qname);
                } else {
                    ecEncodeOpcode(cp, EJS_OP_DEFINE_FUNCTION);
                    ecEncodeNumber(cp, lookup->slotNum);
                    ecEncodeNumber(cp, lookup->nthBlock);
                }

            } else {
                mprAssert(0);
            }
        }
    }

    code = state->code = allocCodeBuffer(cp);
    addDebugInstructions(cp, np);

    /*
     *  Generate code for any parameter default initialization.
     *  Native classes must do default parameter initialization themselves.
     */
    if (fun->numDefault > 0 && !(np->attributes & EJS_ATTR_NATIVE)) {
        genDefaultParameterCode(cp, np, fun);
    }

    if (np->function.constructorSettings) {
        genDirectives(cp, np->function.constructorSettings, 1);
    }

    state->letBlock = (EjsVar*) fun;
    state->varBlock = (EjsVar*) fun;

    if (np->function.isConstructor) {
        /*
         *  Function is a constructor. Call any default constructors if required.
         *  Should this be before or after default variable initialization?
         */
        mprAssert(state->currentClass);
        baseType = state->currentClass->baseType;
        if (!state->currentClass->callsSuper && baseType && baseType->hasConstructor && !(np->attributes & EJS_ATTR_NATIVE)) {
            ecEncodeOpcode(cp, EJS_OP_CALL_NEXT_CONSTRUCTOR);
            ecEncodeNumber(cp, 0);
        }
    }

    /*
     *  May be no body for native functions
     */
    if (np->function.body) {
        mprAssert(np->function.body->kind == N_DIRECTIVES);
        processNode(cp, np->function.body);
    }

    if (cp->errorCount > 0) {
        LEAVE(cp);
        return;
    }

    setFunctionCode(cp, fun, code);

    /*
     *  Add string constants
     */
    ecAddNameConstant(cp, &np->qname);

    for (i = 0; i < fun->block.obj.numProp; i++) {
        qname = ejsGetPropertyName(ejs, (EjsVar*) fun, i);
        ecAddNameConstant(cp, &qname);
        trait = ejsGetPropertyTrait(ejs, (EjsVar*) fun, i);
        if (trait && trait->type) {
            ecAddNameConstant(cp, &trait->type->qname);
        }
    }
    if (fun->resultType) {
        ecAddNameConstant(cp, &fun->resultType->qname);
    }
    for (i = 0; i < fun->body.code.numHandlers; i++) {
        ex = fun->body.code.handlers[i];
        if (ex && ex->catchType) {
            ecAddNameConstant(cp, &ex->catchType->qname);
        }
    }

#if BLD_FEATURE_EJS_DOC
    if (cp->ejs->flags & EJS_FLAG_DOC) {
        ecAddDocConstant(cp, 0, fun->owner, fun->slotNum);
    }
#endif

    LEAVE(cp);
}


static void genHash(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);

    if (!np->hash.disabled) {
        processNode(cp, np->hash.body);
    }
    LEAVE(cp);
}


static void genIf(EcCompiler *cp, EcNode *np)
{
    EcCodeGen   *saveCode;
    EcState     *state;
    int         thenLen, elseLen;

    ENTER(cp);

    mprAssert(np->kind == N_IF);

    state = cp->state;
    saveCode = state->code;

    /*
     *  Process the conditional. Put the popStack for the branch here so the stack is correct for the "then" and "else" blocks.
     */
    state->needsValue = 1;
    processNode(cp, np->tenary.cond);
    state->needsValue = 0;
    popStack(cp, 1);

    /*
     *  Process the "then" block.
     */
    np->tenary.thenCode = state->code = allocCodeBuffer(cp);
    resetStack(cp);
    processNode(cp, np->tenary.thenBlock);
    if (state->prev->needsValue) {
        /* Part of a tenary expression */
        if (state->code->stackCount != 1) {
            genError(cp, np, "Then expression does not evaluate to a value. Check if operands are void");
        }
        emptyStack(cp, 1);
    }

    /*
     *  Else block (optional)
     */
    if (np->tenary.elseBlock) {
        np->tenary.elseCode = state->code = allocCodeBuffer(cp);
        resetStack(cp);
        processNode(cp, np->tenary.elseBlock);
        if (state->prev->needsValue) {
            if (state->code->stackCount < 1) {
                genError(cp, np, "Else expression does not evaluate to a value. Check if operands are void");
            }
            emptyStack(cp, 1);
        }
    }

    /*
     *  Calculate jump lengths. Then length will vary depending on if the jump at the end of the "then" block
     *  can jump over the "else" block with a short jump.
     */
    elseLen = (np->tenary.elseCode) ? mprGetBufLength(np->tenary.elseCode->buf) : 0;
    thenLen = mprGetBufLength(np->tenary.thenCode->buf);
    thenLen += (elseLen < 0x7f && cp->optimizeLevel > 0) ? 2 : 5;

    /*
     *  Now copy the basic blocks into the output code buffer, starting with the jump around the "then" code.
     */
    setCodeBuffer(cp, saveCode);

    if (thenLen < 0x7f && cp->optimizeLevel > 0) {
        ecEncodeOpcode(cp, EJS_OP_BRANCH_FALSE_8);
        ecEncodeByte(cp, thenLen);
    } else {
        //  TODO - BFALSE_16
        ecEncodeOpcode(cp, EJS_OP_BRANCH_FALSE);
        ecEncodeWord(cp, thenLen);
    }

    /*
     *  Copy the then code
     */
    copyCodeBuffer(cp, state->code, np->tenary.thenCode);

    /*
     *  Create the jump to the end of the if statement
     */
    if (elseLen < 0x7f && cp->optimizeLevel > 0) {
        ecEncodeOpcode(cp, EJS_OP_GOTO_8);
        ecEncodeByte(cp, elseLen);
    } else {
        //  TODO - BFALSE_16
        ecEncodeOpcode(cp, EJS_OP_GOTO);
        ecEncodeWord(cp, elseLen);
    }

    if (np->tenary.elseCode) {
        copyCodeBuffer(cp, state->code, np->tenary.elseCode);
    }

    if (state->prev->needsValue) {
        pushStack(cp, 1);
    }

    LEAVE(cp);
}


/*
 *  Expect data on the stack already to assign
 */
static void genLeftHandSide(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);

    mprAssert(cp);
    mprAssert(np);

    cp->state->onLeft = 1;

    switch (np->kind) {
    case N_DOT:
    case N_QNAME:
    case N_SUPER:
    case N_EXPRESSIONS:
        processNode(cp, np);
        break;

    case N_CALL:
    default:
        mprAssert(0);
    }

    LEAVE(cp);
}


static void genLiteral(EcCompiler *cp, EcNode *np)
{
    EjsNamespace    *nsp;
    EjsBoolean      *bp;
    EjsNumber       *ip;
    int64           n;
    int             id;

    ENTER(cp);

    /*
     *  Map Numbers to the configured real type
     */
    id = np->literal.var->type->id;

    switch (id) {
    case ES_Boolean:
        bp = (EjsBoolean*) np->literal.var;
        if (bp->value) {
            ecEncodeOpcode(cp, EJS_OP_LOAD_TRUE);
        } else {
            ecEncodeOpcode(cp, EJS_OP_LOAD_FALSE);
        }
        break;

    case ES_Number:
        /*
         *  These are signed values
         */
        ip = (EjsNumber*) np->literal.var;
#if BLD_FEATURE_FLOATING_POINT
        n = (int64) floor(ip->value);
        if (ip->value != floor(ip->value)) {
            ecEncodeOpcode(cp, EJS_OP_LOAD_DOUBLE);
            ecEncodeDouble(cp, ip->value);
        } else
#else
        n = (int64) ip->value;
#endif
        if (0 <= n && n <= 9) {
            ecEncodeOpcode(cp, EJS_OP_LOAD_0 + (int) n);

        } else if ((n & 0x7F) == n) {
            ecEncodeOpcode(cp, EJS_OP_LOAD_INT_8);
            ecEncodeByte(cp, (char) n);

        } else if ((n & 0x7FFF) == n) {
            ecEncodeOpcode(cp, EJS_OP_LOAD_INT_16);
            ecEncodeShort(cp, (short) n);

        } else if ((n & 0x7FFFFFFF) == n) {
            ecEncodeOpcode(cp, EJS_OP_LOAD_INT_32);
            ecEncodeWord(cp, (int) n);

        } else {
            ecEncodeOpcode(cp, EJS_OP_LOAD_INT_64);
            ecEncodeLong(cp, (int64) n);
        }
        break;

    case ES_Namespace:
        ecEncodeOpcode(cp, EJS_OP_LOAD_NAMESPACE);
        nsp = (EjsNamespace*) np->literal.var;
        ecEncodeString(cp, nsp->uri);
        break;

    case ES_Null:
        ecEncodeOpcode(cp, EJS_OP_LOAD_NULL);
        break;

    case ES_String:
        ecEncodeOpcode(cp, EJS_OP_LOAD_STRING);
        ecEncodeString(cp, ((EjsString*) np->literal.var)->value);
        break;

    case ES_XML:
        ecEncodeOpcode(cp, EJS_OP_LOAD_XML);
        ecEncodeString(cp, mprGetBufStart(np->literal.data));
        break;

    case ES_RegExp:
        ecEncodeOpcode(cp, EJS_OP_LOAD_REGEXP);
        ecEncodeString(cp, ((EjsRegExp*) np->literal.var)->pattern);
        break;

    case ES_Void:
        ecEncodeOpcode(cp, EJS_OP_LOAD_UNDEFINED);
        break;

    default:
        mprAssert(0);
        break;
    }
    pushStack(cp, 1);

    LEAVE(cp);
}


/*
 *  Generate code for name reference. This routine handles both loads and stores.
 */
static void genName(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);

    mprAssert(np->kind == N_QNAME || np->kind == N_USE_NAMESPACE);

    if (np->needThis) {
        mprAssert(0);
        if (np->lookup.useThis) {
            ecEncodeOpcode(cp, EJS_OP_LOAD_THIS);

        } else if (np->lookup.obj == cp->ejs->global){
            ecEncodeOpcode(cp, EJS_OP_LOAD_GLOBAL);

        } else if (cp->state->currentObjectNode) {
            ecEncodeOpcode(cp, EJS_OP_DUP);

        } else {
            /*
             *  Unbound function
             */
            ecEncodeOpcode(cp, EJS_OP_LOAD_GLOBAL);
        }
        pushStack(cp, 1);
        np->needThis = 0;
    }

    if (np->lookup.slotNum >= 0) {
        genBoundName(cp, np);

    } else {
        genUnboundName(cp, np);
    }

    LEAVE(cp);
}


static void genNew(EcCompiler *cp, EcNode *np)
{
    EcState     *state;
    int         argc;

    ENTER(cp);

    mprAssert(np->kind == N_NEW);

    state = cp->state;
    argc = 0;

    /*
     *  Process the type reference to instantiate
     */
    processNode(cp, np->left);
    ecEncodeOpcode(cp, EJS_OP_NEW);
    popStack(cp, 1);
    pushStack(cp, 1);

    LEAVE(cp);
}


static void genObjectLiteral(EcCompiler *cp, EcNode *np)
{
    EcNode      *child, *typeNode;
    EjsType     *type;
    int         next, argc;

    ENTER(cp);

    /*
     *  Push all the args
     */
    next = 0;
    while ((child = getNextNode(cp, np, &next)) != 0) {
        processNode(cp, child);
    }
    argc = next;

    ecEncodeOpcode(cp, EJS_OP_NEW_OBJECT);
    typeNode = np->objectLiteral.typeNode;
    type = (EjsType*) typeNode->lookup.ref;
    //  TODO - should we encode the <type> or push it on the stack?
    ecEncodeGlobal(cp, (EjsVar*) type, (type) ? &type->qname: 0);
    ecEncodeNumber(cp, argc);
    pushStack(cp, 1);
    popStack(cp, argc * 2);

    LEAVE(cp);
}


static void genField(EcCompiler *cp, EcNode *np)
{
    EcNode      *fieldName;

    fieldName = np->field.fieldName;

    if (fieldName->kind == N_QNAME) {
        ecEncodeOpcode(cp, EJS_OP_LOAD_STRING);
        ecEncodeString(cp, np->field.fieldName->qname.name);
        pushStack(cp, 1);

    } else if (fieldName->kind == N_LITERAL) {
        genLiteral(cp, fieldName);

    } else {
        mprAssert(0);
        processNode(cp, fieldName);
    }

    if (np->field.fieldKind == FIELD_KIND_VALUE) {
        processNode(cp, np->field.expr);
    } else {
        processNode(cp, np->field.fieldName);
    }
}


static void genPostfixOp(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);

    /*
     *  Dup before inc
     */
    processNode(cp, np->left);
    ecEncodeOpcode(cp, EJS_OP_DUP);
    ecEncodeOpcode(cp, EJS_OP_INC);
    ecEncodeByte(cp, (np->tokenId == T_PLUS_PLUS) ? 1 : -1);
    genLeftHandSide(cp, np->left);

    pushStack(cp, 1);

    LEAVE(cp);
}


static void genProgram(EcCompiler *cp, EcNode *np)
{
    Ejs         *ejs;
    EcNode      *child;
    int         next;

    ENTER(cp);

    ejs = cp->ejs;

    next = 0;
    while ((child = getNextNode(cp, np, &next)) && !cp->error) {

        switch (child->kind) {
        case N_MODULE:
            genModule(cp, child);
            break;

        case N_DIRECTIVES:
            genDirectives(cp, child, 1);
            break;

        default:
            badNode(cp, np);
        }
    }

    LEAVE(cp);
}


static void genPragmas(EcCompiler *cp, EcNode *np)
{
    EcNode  *child;
    int     next;

    next = 0;
    while ((child = getNextNode(cp, np, &next))) {
        processNode(cp, child);
    }
}


/*
 *  Generate code for function returns
 */
static void genReturn(EcCompiler *cp, EcNode *np)
{
    EjsFunction     *fun;

    ENTER(cp);

    if (np->left) {
        cp->state->needsValue = 1;
        processNode(cp, np->left);
        cp->state->needsValue = 0;
        
        fun = cp->state->currentFunction;
        ecEncodeOpcode(cp, EJS_OP_RETURN_VALUE);
        popStack(cp, 1);

    } else {
        /*
         *  return;
         */
        ecEncodeOpcode(cp, EJS_OP_RETURN);
    }
    emptyStack(cp, 0);

    LEAVE(cp);
}


/*
 *  Load the super pointer. Super function calls (super()) are handled via N_CALL.
 */
static void genSuper(EcCompiler *cp, EcNode *np)
{
    int     argc;

    ENTER(cp);

    mprAssert(np->kind == N_SUPER);

    argc = mprGetListCount(np->left->children);
    if (argc > 0) {
        processNode(cp, np->left);
    }
    ecEncodeOpcode(cp, EJS_OP_CALL_NEXT_CONSTRUCTOR);
    ecEncodeNumber(cp, argc);

    popStack(cp, argc);

    LEAVE(cp);
}


static void genSwitch(EcCompiler *cp, EcNode *np)
{
    EcNode      *caseItem, *elements;
    EcCodeGen   *code, *outerBlock;
    EcState     *state;
    int         next, len, nextCaseLen, nextCodeLen, totalLen;

    ENTER(cp);

    state = cp->state;

    outerBlock = state->code;
    code = state->code = allocCodeBuffer(cp);

    ecStartBreakableStatement(cp, EC_JUMP_BREAK);

    /*
     *  Generate code for the switch (expression)
     */
    processNode(cp, np->left);

    /*
     *  Generate the code for each case label expression and case statements.
     *  next set to one to skip the switch expression.
     */
    elements = np->right;
    mprAssert(elements->kind == N_CASE_ELEMENTS);

    next = 0;
    while ((caseItem = getNextNode(cp, elements, &next)) && !cp->error) {

        /*
         *  Allocate a buffer for the case expression and generate that code
         */
        mprAssert(caseItem->kind == N_CASE_LABEL);
        if (caseItem->caseLabel.kind == EC_SWITCH_KIND_CASE) {
            caseItem->caseLabel.expressionCode = state->code = allocCodeBuffer(cp);


            /*
             *  Dup the switch expression value to preserve it for later cases.
             *  OPT - don't need to preserve for default cases or if this is the last case
             */
            addDebugInstructions(cp, caseItem->caseLabel.expression);
            ecEncodeOpcode(cp, EJS_OP_DUP);

            setStack(cp, 0);

            mprAssert(caseItem->caseLabel.expression);
            processNode(cp, caseItem->caseLabel.expression);
        }

        /*
         *  Generate code for the case directives themselves.
         */
        caseItem->code = state->code = allocCodeBuffer(cp);
        mprAssert(caseItem->left->kind == N_DIRECTIVES);

        setStack(cp, 0);
        processNode(cp, caseItem->left);
    }

    /*
     *  Calculate jump lengths. Start from the last case and work backwards.
     */
    nextCaseLen = 0;
    nextCodeLen = 0;
    totalLen = 0;

    next = -1;
    while ((caseItem = getPrevNode(cp, elements, &next)) && !cp->error) {

        if (caseItem->kind != N_CASE_LABEL) {
            break;
        }

        /*
         *  CODE jump
         *  Jump to the code block of the next case. In the last block, we just fall out the bottom.
         */
        caseItem->caseLabel.nextCaseCode = nextCodeLen;
        if (nextCodeLen > 0) {
            len = (caseItem->caseLabel.nextCaseCode < 0x7f && cp->optimizeLevel > 0) ? 2 : 5;
            nextCodeLen += len;
            nextCaseLen += len;
            totalLen += len;
        }

        /*
         *  CASE jump
         *  Jump to the next case expression evaluation.
         */
        len = getCodeLength(cp, caseItem->code);
        nextCodeLen += len;
        nextCaseLen += len;
        totalLen += len;

        caseItem->jumpLength = nextCaseLen;
        nextCodeLen = 0;

        if (caseItem->caseLabel.kind == EC_SWITCH_KIND_CASE) {
            /*
             *  Jump to the next case expression test. Increment the length depending on whether we are using a
             *  goto_8 (2 bytes) or goto (4 bytes). Add one for the CMPEQ instruction (3 vs 6)
             */
            len = (caseItem->jumpLength < 0x7f && cp->optimizeLevel > 0) ? 3 : 6;
            nextCodeLen += len;
            totalLen += len;

            if (caseItem->caseLabel.expressionCode) {
                len = getCodeLength(cp, caseItem->caseLabel.expressionCode);
                nextCodeLen += len;
                totalLen += len;
            }
        }
        nextCaseLen = 0;
    }

    /*
     *  Now copy the basic blocks into the output code buffer.
     */
    setCodeBuffer(cp, code);

    next = 0;
    while ((caseItem = getNextNode(cp, elements, &next)) && !cp->error) {

        if (caseItem->caseLabel.expressionCode) {
            copyCodeBuffer(cp, state->code, caseItem->caseLabel.expressionCode);
        }

        /*
         *  Encode the jump to the next case
         */
        if (caseItem->caseLabel.kind == EC_SWITCH_KIND_CASE) {
            setStack(cp, 2);
            ecEncodeOpcode(cp, EJS_OP_COMPARE_STRICTLY_EQ);
            popStack(cp, 2);

            if (caseItem->jumpLength < 0x7f && cp->optimizeLevel > 0) {
                ecEncodeOpcode(cp, EJS_OP_BRANCH_FALSE_8);
                ecEncodeByte(cp, caseItem->jumpLength);
            } else {
                //  TODO - BFALSE_16
                ecEncodeOpcode(cp, EJS_OP_BRANCH_FALSE);
                ecEncodeWord(cp, caseItem->jumpLength);
            }
        }

        mprAssert(caseItem->code);
        copyCodeBuffer(cp, state->code, caseItem->code);

        /*
         *  Encode the jump to the next case's code. Last case/default block may have zero length jump.
         */
        if (caseItem->caseLabel.nextCaseCode > 0) {
            if (caseItem->caseLabel.nextCaseCode < 0x7f && cp->optimizeLevel > 0) {
                ecEncodeOpcode(cp, EJS_OP_GOTO_8);
                ecEncodeByte(cp, caseItem->caseLabel.nextCaseCode);
            } else {
                //  TODO - GOTO_16
                ecEncodeOpcode(cp, EJS_OP_GOTO);
                ecEncodeWord(cp, caseItem->caseLabel.nextCaseCode);
            }
        }
    }

    totalLen = mprGetBufLength(state->code->buf);
    patchJumps(cp, EC_JUMP_BREAK, totalLen);

    /*
     *  Pop the switch value
     */
    ecEncodeOpcode(cp, EJS_OP_POP);

    copyCodeBuffer(cp, outerBlock, state->code);

    LEAVE(cp);
}


/*
 *  Load the this pointer.
 */
static void genThis(EcCompiler *cp, EcNode *np)
{
    EcState     *state;

    ENTER(cp);

    state = cp->state;

    switch (np->thisNode.thisKind) {
    case N_THIS_GENERATOR:
        //  TODO
        break;

    case N_THIS_CALLEE:
        //  TODO
        break;

    case N_THIS_TYPE:
        genClassName(cp, state->currentClass);
        break;

    case N_THIS_FUNCTION:
        genName(cp, state->currentFunctionNode);
        break;

    default:
        ecEncodeOpcode(cp, EJS_OP_LOAD_THIS);
        pushStack(cp, 1);
    }

    LEAVE(cp);
}


/*
 *
 */
static void genThrow(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);

    cp->state->needsValue = 1;
    processNode(cp, np->left);
    ecEncodeOpcode(cp, EJS_OP_THROW);
    popStack(cp, 1);

    LEAVE(cp);
}


/*
 *  Try, catch, finally.
 */
static void genTry(EcCompiler *cp, EcNode *np)
{
    EjsFunction *fun;
    EcNode      *child, *arg, *assignOp;
    EcCodeGen   *saveCode;
    EcState     *state;
    EjsType     *catchType;
    uint        tryStart, tryEnd, handlerStart, handlerEnd;
    int         next, len;

    ENTER(cp);

    state = cp->state;
    fun = state->currentFunction;
    mprAssert(fun);

    /*
     *  Switch to a new code buffer for the try block
     */
    saveCode = state->code;
    mprAssert(saveCode);
    np->exception.tryBlock->code = state->code = allocCodeBuffer(cp);

    addDebugInstructions(cp, np);

    /*
     *  Process the try block. Will add a goto into either the finally block or if no finally block,
     *  to after the last catch.
     */
    processNode(cp, np->exception.tryBlock);

    if (np->exception.catchClauses) {
        next = 0;
        while ((child = getNextNode(cp, np->exception.catchClauses, &next)) && !cp->error) {
            child->code = state->code = allocCodeBuffer(cp);
            mprAssert(child->left);

            processNode(cp, child->left);
            if (np->exception.finallyBlock == 0) {
                ecEncodeOpcode(cp, EJS_OP_END_EXCEPTION);
            }
            /* Add jumps below */
        }
    }

    if (np->exception.finallyBlock) {
        np->exception.finallyBlock->code = state->code = allocCodeBuffer(cp);
        processNode(cp, np->exception.finallyBlock);
        ecEncodeOpcode(cp, EJS_OP_END_EXCEPTION);
    }

    /*
     *  Calculate jump lengths for the catch block into a finally block. Start from the last catch block and work backwards.
     */
    len = 0;
    if (np->exception.catchClauses) {
        next = -1;
        while ((child = getPrevNode(cp, np->exception.catchClauses, &next)) && !cp->error) {
            child->jumpLength = len;
            if (child->jumpLength > 0 && np->exception.finallyBlock) {
                /*
                 *  Add jumps if there is a finally block. Otherwise, we use and end_ecception instruction
                 *  Increment the length depending on whether we are using a goto_8 (2 bytes) or goto (4 bytes)
                 */
                len += (child->jumpLength < 0x7f && cp->optimizeLevel > 0) ? 2 : 5;
            }
            len += getCodeLength(cp, child->code);
        }
    }

    /*
     *  Now copy the code. First the try block. Restore the primary code buffer and copy try/catch/finally
     *  code blocks into the code buffer.
     */
    setCodeBuffer(cp, saveCode);

    tryStart = ecGetCodeOffset(cp);


    /*
     *  Copy the try code and add a jump
     */
    copyCodeBuffer(cp, state->code, np->exception.tryBlock->code);

    if (len < 0x7f && cp->optimizeLevel > 0) {
        ecEncodeOpcode(cp, EJS_OP_GOTO_8);
        ecEncodeByte(cp, len);
    } else {
        ecEncodeOpcode(cp, EJS_OP_GOTO);
        ecEncodeWord(cp, len);
    }
    tryEnd = ecGetCodeOffset(cp);


    /*
     *  Now the copy the catch blocks and add jumps
     */
    if (np->exception.catchClauses) {
        next = 0;
        while ((child = getNextNode(cp, np->exception.catchClauses, &next)) && !cp->error) {
            handlerStart = ecGetCodeOffset(cp);
            copyCodeBuffer(cp, state->code, child->code);
            if (child->jumpLength > 0 && np->exception.finallyBlock) {
                if (child->jumpLength < 0x7f && cp->optimizeLevel > 0) {
                    ecEncodeOpcode(cp, EJS_OP_GOTO_8);
                    ecEncodeByte(cp, child->jumpLength);
                } else {
                    //  TODO - GOTO_16
                    ecEncodeOpcode(cp, EJS_OP_GOTO);
                    ecEncodeWord(cp, child->jumpLength);
                }
            }
            handlerEnd = ecGetCodeOffset(cp);

            /*
             *  Create exception handler record
             */
            catchType = 0;
            arg = 0;
            if (child->catchBlock.arg && child->catchBlock.arg->left) {
                assignOp = child->catchBlock.arg->left;
                arg = assignOp->left;
            }
            if (arg && arg->typeNode && ejsIsType(arg->typeNode->lookup.ref)) {
                catchType = (EjsType*) arg->typeNode->lookup.ref;
            }
            if (catchType == 0) {
                catchType = cp->ejs->voidType;
            }
            ecAddNameConstant(cp, &catchType->qname);
            addException(cp, tryStart, tryEnd, catchType, handlerStart, handlerEnd, EJS_EX_CATCH);
        }
    }


    /*
     *  Finally, the finally block
     */
    if (np->exception.finallyBlock) {
        handlerStart = ecGetCodeOffset(cp);
        copyCodeBuffer(cp, state->code, np->exception.finallyBlock->code);
        handlerEnd = ecGetCodeOffset(cp);
        addException(cp, tryStart, tryEnd, cp->ejs->voidType, handlerStart, handlerEnd, EJS_EX_FINALLY);
    }

    LEAVE(cp);
}


static void genUnaryOp(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);

    mprAssert(np->kind == N_UNARY_OP);
    mprAssert(np->left);

    switch (np->tokenId) {
    case T_DELETE:
        genDelete(cp, np);
        break;

    case T_LOGICAL_NOT:
        processNode(cp, np->left);
        ecEncodeOpcode(cp, EJS_OP_LOGICAL_NOT);
        break;

    case T_PLUS:
        /* Just ignore the plus */
        processNode(cp, np->left);
        break;

    case T_PLUS_PLUS:
        processNode(cp, np->left);
        ecEncodeOpcode(cp, EJS_OP_INC);
        ecEncodeByte(cp, 1);
        ecEncodeOpcode(cp, EJS_OP_DUP);
        pushStack(cp, 1);
        genLeftHandSide(cp, np->left);
        break;

    case T_MINUS:
        processNode(cp, np->left);
        ecEncodeOpcode(cp, EJS_OP_NEG);
        break;

    case T_MINUS_MINUS:
        processNode(cp, np->left);
        ecEncodeOpcode(cp, EJS_OP_INC);
        ecEncodeByte(cp, -1);
        ecEncodeOpcode(cp, EJS_OP_DUP);
        pushStack(cp, 1);
        genLeftHandSide(cp, np->left);
        break;

    case T_TILDE:
        /* Bitwise not */
        processNode(cp, np->left);
        ecEncodeOpcode(cp, EJS_OP_NOT);
        break;

    case T_TYPEOF:
        processNode(cp, np->left);
        ecEncodeOpcode(cp, EJS_OP_TYPE_OF);
        break;

    case T_VOID:
        /* Ignore the node and just push a void */
        ecEncodeOpcode(cp, EJS_OP_LOAD_UNDEFINED);
        pushStack(cp, 1);
        break;
    }

//  ecEncodeOpcode(cp, mapToken(cp, np->tokenId));

    LEAVE(cp);
}


/*
 *  Generate code for an unbound name reference. We don't know the slot
 */
static void genUnboundName(EcCompiler *cp, EcNode *np)
{
    Ejs         *ejs;
    EcState     *state;
    EjsVar      *owner;
    EjsLookup   *lookup;
    int         code;

    ENTER(cp);

    ejs = cp->ejs;
    state = cp->state;

    mprAssert(np->lookup.slotNum < 0 || cp->nobind);

    lookup = &np->lookup;
    owner = lookup->obj;

    if (state->currentObjectNode) {
        if (np->needThis) {
            mprAssert(0);
            ecEncodeOpcode(cp, EJS_OP_DUP);
            pushStack(cp, 1);
            np->needThis = 0;
        }
        /*
         *  Qualified property name (requires obj on stack)
         *  Store: -2, load: 0
         */
        code = (!state->onLeft) ?  EJS_OP_GET_OBJ_NAME :  EJS_OP_PUT_OBJ_NAME;
        ecEncodeOpcode(cp, code);
        ecEncodeName(cp, &np->qname);

        popStack(cp, 1);
        pushStack(cp, (state->onLeft) ? -1 : 1);

    } else if (owner == ejs->global) {
        //  TODO - should optimize
        if (np->needThis) {
            mprAssert(0);
            ecEncodeOpcode(cp, EJS_OP_LOAD_GLOBAL);
            pushStack(cp, 1);
            np->needThis = 0;
        }

        ecEncodeOpcode(cp, EJS_OP_LOAD_GLOBAL);
        pushStack(cp, 1);

        code = (!state->onLeft) ?  EJS_OP_GET_OBJ_NAME :  EJS_OP_PUT_OBJ_NAME;
        ecEncodeOpcode(cp, code);
        ecEncodeName(cp, &np->qname);

        /*
         *  Store: -2, load: 0
         */
        popStack(cp, 1);
        pushStack(cp, (state->onLeft) ? -1 : 1);

    } else if (lookup->useThis) {

        ecEncodeOpcode(cp, EJS_OP_LOAD_THIS);
        pushStack(cp, 1);
        if (np->needThis) {
            mprAssert(0);
            ecEncodeOpcode(cp, EJS_OP_DUP);
            pushStack(cp, 1);
            np->needThis = 0;
        }

        code = (!state->onLeft) ?  EJS_OP_GET_OBJ_NAME :  EJS_OP_PUT_OBJ_NAME;
        ecEncodeOpcode(cp, code);
        ecEncodeName(cp, &np->qname);

        /*
         *  Store: -2, load: 0
         */
        popStack(cp, 1);
        pushStack(cp, (state->onLeft) ? -1 : 1);


    } else if (owner && ejsIsType(owner)) {

        SAVE_ONLEFT(cp);
        genClassName(cp, (EjsType*) owner);
        RESTORE_ONLEFT(cp);

        if (np->needThis) {
            mprAssert(0);
            ecEncodeOpcode(cp, EJS_OP_DUP);
            pushStack(cp, 1);
            np->needThis = 0;
        }
        //  TODO - should be able to load by slot and not by name
        code = (!state->onLeft) ?  EJS_OP_GET_OBJ_NAME :  EJS_OP_PUT_OBJ_NAME;
        ecEncodeOpcode(cp, code);
        ecEncodeName(cp, &np->qname);

        /*
         *  Store: -2, load: 0
         */
        popStack(cp, 1);
        pushStack(cp, (state->onLeft) ? -1 : 1);

    } else {
        /*
         *  Unqualified name
         */
        if (np->needThis) {
            mprAssert(0);
            //  TODO - Is this right in all cases?
            ecEncodeOpcode(cp, EJS_OP_LOAD_THIS);
            pushStack(cp, 1);
            np->needThis = 0;
        }
        code = (!state->onLeft) ?  EJS_OP_GET_SCOPED_NAME :  EJS_OP_PUT_SCOPED_NAME;
        ecEncodeOpcode(cp, code);
        ecEncodeName(cp, &np->qname);

        /*
         *  Store: -1, load: 1
         */
        pushStack(cp, (state->onLeft) ? -1 : 1);
    }

    LEAVE(cp);
}


static void genModule(EcCompiler *cp, EcNode *np)
{    
    ENTER(cp);

    mprAssert(np->kind == N_MODULE);

    addModule(cp, np->module.ref);
    genBlock(cp, np->left);

    LEAVE(cp);
}


static void genUseModule(EcCompiler *cp, EcNode *np)
{
    EcNode      *child;
    Ejs         *ejs;
    int         next;

    ENTER(cp);

    ejs = cp->ejs;

    mprAssert(np->kind == N_USE_MODULE);

    next = 0;
    while ((child = getNextNode(cp, np, &next))) {
        processNode(cp, child);
    }

    LEAVE(cp);
}


static void genUseNamespace(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);

    mprAssert(np->kind == N_USE_NAMESPACE);

    /*
     *  Load the namespace reference. NOTE: use default space; will not add a namespace to the set of open spaces.
     */
    if (np->useNamespace.isLiteral) {
        ecEncodeOpcode(cp, EJS_OP_ADD_NAMESPACE);
        ecEncodeString(cp, np->qname.name);

    } else {
        genName(cp, np);
        ecEncodeOpcode(cp, EJS_OP_ADD_NAMESPACE_REF);
        popStack(cp, 1);
    }

    LEAVE(cp);
}


static void genVar(EcCompiler *cp, EcNode *np)
{
    EcState     *state;

    ENTER(cp);

    mprAssert(np->kind == N_QNAME);

    state = cp->state;

    /*
     *  Add string constants
     */
    ecAddNameConstant(cp, &np->qname);

    if (np->lookup.trait && np->lookup.trait->type) {
        ecAddConstant(cp, np->lookup.trait->type->qname.name);
    }

#if BLD_FEATURE_EJS_DOC
    if (cp->ejs->flags & EJS_FLAG_DOC) {
        ecAddDocConstant(cp, np->lookup.trait, np->lookup.obj, np->lookup.slotNum);
    }
#endif

    LEAVE(cp);
}


static void genVarDefinition(EcCompiler *cp, EcNode *np)
{
    EcState     *state;
    EcNode      *child, *var;
    int         next, varKind;

    ENTER(cp);

    mprAssert(np->kind == N_VAR_DEFINITION);

    state = cp->state;
    varKind = np->def.varKind;

    for (next = 0; (child = getNextNode(cp, np, &next)) != 0; ) {

        var = (child->kind == N_ASSIGN_OP) ? child->left : child;
        mprAssert(var->kind == N_QNAME);

        genVar(cp, var);

        if (child->kind == N_ASSIGN_OP) {
            /*
             *  Class level variable initializations must go into the instance code buffer.
             */
            if (var->name.instanceVar) {
                state->instanceCode = 1;
                mprAssert(state->instanceCodeBuf);
                state->code = state->instanceCodeBuf;
            }
            genAssignOp(cp, child);

        } else {
            addDebugInstructions(cp, var);
        }
    }

    LEAVE(cp);
}


static void genWith(EcCompiler *cp, EcNode *np)
{
    ENTER(cp);

    processNode(cp, np->with.object);
    ecEncodeOpcode(cp, EJS_OP_OPEN_WITH);
    popStack(cp, 1);
    processNode(cp, np->with.statement);
    ecEncodeOpcode(cp, EJS_OP_CLOSE_WITH);

    LEAVE(cp);
}


/********************************* Support Code *******************************/
/*
 *  Create the module file.
 */

static MprFile *openModuleFile(EcCompiler *cp, const char *filename)
{
    EcState     *state;
    MprTime     now;
    cchar       *sp;
    int         seq;

    mprAssert(cp);
    mprAssert(filename && *filename);

    state = cp->state;

    if (cp->noout) {
        return 0;
    }

    if ((cp->file = mprOpen(cp, filename,  O_CREAT | O_WRONLY | O_TRUNC | O_BINARY, 0664)) == 0) {
        genError(cp, 0, "Can't create %s", filename);
        return 0;
    }

    now = mprGetTime(cp);
    seq = (int) now + (int) (now >> 32);
    for (sp = filename; *sp; sp++) {
        seq += *sp;
    }

    /*
     *  Create a module header once per file instead of per-module in the file
     */
    state->code = allocCodeBuffer(cp);
    if (ecCreateModuleHeader(cp, EJS_MODULE_VERSION, seq) < 0) {
        genError(cp, 0, "Can't write module file header");
        return 0;
    }

    return cp->file;
}


/*
 *  Create a new code buffer
 */
static EcCodeGen *allocCodeBuffer(EcCompiler *cp)
{
    EcState     *state;
    EcCodeGen   *code;

    mprAssert(cp);

    state = cp->state;
    mprAssert(state);

    /*
     *  TODO - should modularize CodeBuffer and have createCodeBuffer()
     */
    code = mprAllocObjZeroed(state, EcCodeGen);
    if (code == 0) {
        cp->fatalError = 1;
        return 0;
    }
    code->buf = mprCreateBuf(cp, EC_CODE_BUFSIZE, 0);
    if (code->buf == 0) {
        mprAssert(0);
        cp->fatalError = 1;
        return 0;
    }

    code->exceptions = mprCreateList(code);
    if (code->exceptions == 0) {
        mprAssert(0);
        return 0;
    }

    /*
     *  Jumps are fully processed before the state is freed
     */
    code->jumps = mprCreateList(code);
    if (code->jumps == 0) {
        mprAssert(0);
        return 0;
    }

    /*
     *  Inherit the allowable jump kinds and stack level
     */
    if (state->code) {
        code->jumpKinds = state->code->jumpKinds;
        // TODO - is this the best policy to inherit the stack count?
        code->stackCount = state->code->stackCount;
    }

    return code;
}


static int getCodeLength(EcCompiler *cp, EcCodeGen *code)
{
    return mprGetBufLength(code->buf);
}


static void copyCodeBuffer(EcCompiler *cp, EcCodeGen *dest, EcCodeGen *src)
{
    EjsEx           *exception;
    EcJump          *jump;
    EcState         *state;
    uint            baseOffset;
    int             len, next;

    state = cp->state;
    mprAssert(state);
    mprAssert(dest != src);

    len = getCodeLength(cp, src);
    if (len <= 0) {
        return;
    }

    /*
     *  Copy the code
     */
    baseOffset = mprGetBufLength(dest->buf);
    if (mprPutBlockToBuf(dest->buf, mprGetBufStart(src->buf), len) != len) {
        mprAssert(0);
        return;
    }

    /*
     *  Copy and fix the jump offset of jump patch records. jump->offset starts out being relative to the current code src.
     *  We add the original length of dest to make it absolute to the new dest buffer.
     */
    if (src->jumps) {
        if (src->jumps != dest->jumps) {
            next = 0;
            while ((jump = (EcJump*) mprGetNextItem(src->jumps, &next)) != 0) {
                jump->offset += baseOffset;
                mprAddItem(dest->jumps, jump);
                mprStealBlock(dest->jumps, jump);
            }
        }
    }

    /*
     *  Copy and fix exception target addresses
     */
    if (src->exceptions) {
        next = 0;
        while ((exception = (EjsEx*) mprGetNextItem(src->exceptions, &next)) != 0) {
            exception->tryStart += baseOffset;
            exception->tryEnd += baseOffset;
            exception->handlerStart += baseOffset;
            exception->handlerEnd += baseOffset;
            mprAddItem(dest->exceptions, exception);
        }
    }
}


/*
 *  Patch jump addresses a code buffer. Kind is the kind of jump (break | continue)
 */
static void patchJumps(EcCompiler *cp, int kind, int target)
{
    EcJump      *jump;
    EcCodeGen   *code;
    uchar       *pos;
    int         next;
    int         offset;

    code = cp->state->code;
    mprAssert(code);

rescan:
    next = 0;
    while ((jump = (EcJump*) mprGetNextItem(code->jumps, &next)) != 0) {
        if (jump->kind == kind) {
            //  TODO - define for (4).
            //  Branch and continue jumps are always word jumps
            offset = target - jump->offset - 4;
            //  TODO sanity only
            mprAssert(-10000 < offset && offset < 10000);
            mprAssert(jump->offset < mprGetBufLength(code->buf));
            pos = (uchar*) mprGetBufStart(code->buf) + jump->offset;
            mprLog(cp, 7, "Patch 0x%x at address %d", offset, jump->offset);
            ecEncodeWordAtPos(cp, pos, offset);

            mprRemoveItem(code->jumps, jump);
            goto rescan;
        }
    }
}


/*
 *  Write the module contents
 */
static int flushModuleBuffer(MprFile *file, EcCodeGen *code)
{
    int         len;

    len = mprGetBufLength(code->buf);

    if (len > 0) {
        if (mprWrite(file, mprGetBufStart(code->buf), len) != len) {
            return EJS_ERR;
        }
        mprFlushBuf(code->buf);
    }

    return 0;
}


/*
 *  Create the module initializer
 */
static void createInitializer(EcCompiler *cp, EjsModule *mp)
{
    Ejs             *ejs;
    EjsFunction     *fun;
    EcState         *state;
    EcCodeGen       *code;
    int             len;

    ENTER(cp);

    ejs = cp->ejs;
    state = cp->state;
    mprAssert(state);

    /*
     *  Note: if hasInitializer is false, we may still have some code in the buffer if --debug is used.
     *  We can safely just ignore this debug code.
     */
    if (! mp->hasInitializer) {
        LEAVE(cp);
        return;
    }
    mprAssert(mprGetBufLength(mp->code->buf) > 0);

    if (cp->errorCount > 0) {
        LEAVE(cp);
        return;
    }

    if (cp->noout && !cp->run) {
        LEAVE(cp);
        return;
    }

    /*
     *  TODO - refactor all this code buffer code. Pretty messy.
     */
    state->code = mp->code;
    cp->directiveState = state;

    code = cp->state->code;
    len = mprGetBufLength(code->buf);
    mprAssert(len > 0);

    ecEncodeOpcode(cp, EJS_OP_END_CODE);

    /*
     *  Extract the initialization code
     */
    fun = state->currentFunction = mp->initializer;
    if (fun) {
        setFunctionCode(cp, fun, code);
    }

    LEAVE(cp);
}


static void genError(EcCompiler *cp, EcNode *np, char *fmt, ...)
{
    va_list     arg;
    char        *msg;

    va_start(arg, fmt);

    if (mprAllocVsprintf(cp, &msg, 0, fmt, arg) < 0) {
        msg = "Memory allocation error";
    }

    cp->errorCount++;
    cp->error = 1;
    cp->noout = 1;

    if (np) {
        ecReportError(cp, "error", np->filename, np->lineNumber, np->currentLine, np->column, msg);

    } else {
        ecReportError(cp, "error", 0, 0, 0, 0, msg);
    }

    mprFree(msg);
    va_end(arg);
}


//  TODO - move to ecAst.c (rename ecBadAst)
static void badNode(EcCompiler *cp, EcNode *np)
{
    cp->fatalError = 1;
    cp->errorCount++;

    //  TODO - set cp->hasError or some other such
    mprError(cp, "Unsupported language feature\nUnknown AST node kind %d", np->kind);
}


//  TODO - move to ecNode.c
static EcNode *getNextNode(EcCompiler *cp, EcNode *np, int *next)
{
    if (cp->error) {
        return 0;
    }
    return (EcNode*) mprGetNextItem(np->children, next);
}


static EcNode *getPrevNode(EcCompiler *cp, EcNode *np, int *next)
{
    if (cp->fatalError || cp->error) {
        return 0;
    }
    return (EcNode*) mprGetPrevItem(np->children, next);
}


/*
 *  Map a lexical token to an op code
 */
//  TODO - faster to have a lookup table

static int mapToken(EcCompiler *cp, EjsOpCode tokenId)
{
    int     cond;

    cond = cp->state->conditional;

    /*
        TODO OPT BNULL, BZ, BNZ, BVOID
        ASSIGN variants
     */
    switch (tokenId) {
    case T_BIT_AND:
        return EJS_OP_AND;

    case T_BIT_OR:
        return EJS_OP_OR;

    case T_BIT_XOR:
        return EJS_OP_XOR;

    case T_DIV:
        return EJS_OP_DIV;

    case T_EQ:
        return (cond) ? EJS_OP_BRANCH_EQ : EJS_OP_COMPARE_EQ;

    case T_NE:
        return (cond) ? EJS_OP_BRANCH_NE : EJS_OP_COMPARE_NE;

    case T_GT:
        return (cond) ? EJS_OP_BRANCH_GT : EJS_OP_COMPARE_GT;

    case T_GE:
        return (cond) ? EJS_OP_BRANCH_GE : EJS_OP_COMPARE_GE;

    case T_LT:
        return (cond) ? EJS_OP_BRANCH_LT : EJS_OP_COMPARE_LT;

    case T_LE:
        return (cond) ? EJS_OP_BRANCH_LE : EJS_OP_COMPARE_LE;

    case T_STRICT_EQ:
        return (cond) ? EJS_OP_BRANCH_STRICTLY_EQ : EJS_OP_COMPARE_STRICTLY_EQ;

    case T_STRICT_NE:
        return (cond) ? EJS_OP_BRANCH_STRICTLY_NE : EJS_OP_COMPARE_STRICTLY_NE;

    case T_LSH:
        return EJS_OP_SHL;

    case T_LOGICAL_NOT:
        return EJS_OP_NOT;

    case T_MINUS:
        return EJS_OP_SUB;

    case T_MOD:
        return EJS_OP_REM;

    case T_MUL:
        return EJS_OP_MUL;

    case T_PLUS:
        return EJS_OP_ADD;

    case T_RSH:
        return EJS_OP_SHR;

    case T_RSH_ZERO:
        return EJS_OP_USHR;

    case T_IS:
        return EJS_OP_IS_A;

    case T_INSTANCEOF:
        return EJS_OP_INST_OF;

    case T_LIKE:
        return EJS_OP_LIKE;

    case T_CAST:
        return EJS_OP_CAST;

    case T_IN:
        return EJS_OP_IN;

    default:
        mprAssert(0);
        return -1;
    }
}


static void addDebugInstructions(EcCompiler *cp, EcNode *np)
{
    if (!cp->debug || cp->state->code == 0) {
        return;
    }

    if (np->lineNumber != cp->currentLineNumber) {
        ecEncodeOpcode(cp, EJS_OP_DEBUG);
        ecEncodeString(cp, np->filename);
        ecEncodeNumber(cp, np->lineNumber);
        ecEncodeString(cp, np->currentLine);
        cp->currentLineNumber = np->lineNumber;
    }
}


static void processNode(EcCompiler *cp, EcNode *np)
{
    EcState     *state;

    ENTER(cp);

    state = cp->state;

    mprAssert(np->parent || np->kind == N_PROGRAM || np->kind == N_MODULE);

    if (np->kind != N_TRY && np->kind != N_END_FUNCTION && np->kind != N_HASH) {
        addDebugInstructions(cp, np);
    }

    switch (np->kind) {

    case N_ARGS:
        state->needsValue = 1;
        genArgs(cp, np);
        break;

    case N_ARRAY_LITERAL:
        genArrayLiteral(cp, np);
        break;

    case N_ASSIGN_OP:
        genAssignOp(cp, np);
        break;

    case N_BINARY_OP:
        genBinaryOp(cp, np);
        break;

    case N_BLOCK:
        genBlock(cp, np);
        break;

    case N_BREAK:
        genBreak(cp, np);
        break;

    case N_CALL:
        genCall(cp, np);
        break;

    case N_CLASS:
        genClass(cp, np);
        break;

    case N_CATCH_ARG:
        genCatchArg(cp, np);
        break;

    case N_CONTINUE:
        genContinue(cp, np);
        break;

    case N_DIRECTIVES:
        genDirectives(cp, np, 1);
        break;

    case N_DO:
        genDo(cp, np);
        break;

    case N_DOT:
        genDot(cp, np, 0);
        break;

    case N_END_FUNCTION:
        genEndFunction(cp, np);
        break;

    case N_EXPRESSIONS:
        genExpressions(cp, np);
        break;

    case N_FOR:
        genFor(cp, np);
        break;

    case N_FOR_IN:
        genForIn(cp, np);
        break;

    case N_FUNCTION:
        genFunction(cp, np);
        break;

    case N_HASH:
        genHash(cp, np);
        break;

    case N_IF:
        genIf(cp, np);
        break;

    case N_LITERAL:
        genLiteral(cp, np);
        break;

    case N_OBJECT_LITERAL:
        genObjectLiteral(cp, np);
        break;

    case N_FIELD:
        genField(cp, np);
        break;

    case N_QNAME:
        genName(cp, np);
        break;

    case N_NEW:
        genNew(cp, np);
        break;

    case N_NOP:
        break;

    case N_POSTFIX_OP:
        genPostfixOp(cp, np);
        break;

    case N_PRAGMA:
        break;

    case N_PRAGMAS:
        genPragmas(cp, np);
        break;

    case N_PROGRAM:
        genProgram(cp, np);
        break;

    case N_REF:
        break;

    case N_RETURN:
        genReturn(cp, np);
        break;

    case N_SUPER:
        genSuper(cp, np);
        break;

    case N_SWITCH:
        genSwitch(cp, np);
        break;

    case N_THIS:
        genThis(cp, np);
        break;

    case N_THROW:
        genThrow(cp, np);
        break;

    case N_TRY:
        genTry(cp, np);
        break;

    case N_UNARY_OP:
        genUnaryOp(cp, np);
        break;

    case N_USE_NAMESPACE:
        genUseNamespace(cp, np);
        break;

    case N_VAR_DEFINITION:
        genVarDefinition(cp, np);
        break;

    case N_MODULE:
        genModule(cp, np);
        break;

    case N_USE_MODULE:
        genUseModule(cp, np);
        break;

    case N_WITH:
        genWith(cp, np);
        break;

    default:
        mprAssert(0);
        badNode(cp, np);
    }

    mprAssert(state == cp->state);
    LEAVE(cp);
}


/*
 *  Oputput one module.
 */
static void processModule(EcCompiler *cp, EjsModule *mp)
{
    Ejs         *ejs;
    EcState     *state;
    EcCodeGen   *code;
    char        *path;

    ENTER(cp);

    ejs = cp->ejs;
    state = cp->state;
    state->currentModule = mp;

    createInitializer(cp, mp);

    if (cp->noout) {
        return;
    }

    if (! cp->outputFile) {
        mprAllocSprintf(cp->state, &path, 0, "%s%s", mp->name,  EJS_MODULE_EXT);

        if ((mp->file = openModuleFile(cp, path)) == 0) {
            mprFree(path);
            LEAVE(cp);
            return;
        }
        mprFree(path);

    } else {
        mp->file = cp->file;
    }
    mprAssert(mp->code);
    mprAssert(mp->file);

    code = state->code;

    if (mp->hasInitializer) {
        ecAddConstant(cp, EJS_INITIALIZER_NAME);
        ecAddConstant(cp, EJS_INTRINSIC_NAMESPACE);
    }

    if (ecCreateModuleSection(cp) < 0) {
        genError(cp, 0, "Can't write module sections");
        LEAVE(cp);
        return;
    }

    if (flushModuleBuffer(mp->file, code) < 0) {
        genError(cp, 0, "Can't write to module file %s", mp->name);
        LEAVE(cp);
        return;
    }

    if (! cp->outputFile) {
        mprFree(mp->file);
        mp->file = 0;
        mp->code = 0;

    } else {
        //  TODO - API for this
        mprFree(mp->code);
        mp->code = 0;
    }
    mp->file = 0;
}


/*
 *  Keep a list of modules potentially containing generated code and declarations.
 */
static void addModule(EcCompiler *cp, EjsModule *mp)
{
    EjsModule       *module;
    Ejs             *ejs;
    EcState         *state;
    int             count, i;

    mprAssert(cp);

    state = cp->state;
    ejs = cp->ejs;

    if (mp->code == 0 || cp->interactive) {
        mprFree(mp->code);
        //  TODO - this means we must free mp->code at the end of code generation manually
        mp->code = state->code = allocCodeBuffer(cp);
        mprStealBlock(mp, mp->code);
    }
    mp->loaded = 0;

    state->code = mp->code;

    mprAssert(mp->code);
    mprAssert(mp->code->buf);

    state->currentModule = mp;
    state->varBlock = ejs->global;
    state->letBlock = ejs->global;

    mprAssert(mp->initializer);
    state->currentFunction = mp->initializer;

    /*
     *  Merge means aggregate dependent input modules with the output
     */
    if (mp->dependencies && !cp->merge) {
        count = mprGetListCount(mp->dependencies);
        for (i = 0; i < count; i++) {
            module = (EjsModule*) mprGetItem(mp->dependencies, i);
            ecAddConstant(cp, module->name);
        }
    }
}


//  TODO - temp
static int level = 7;

static void pushStack(EcCompiler *cp, int count)
{
    EcCodeGen       *code;

    code = cp->state->code;

    mprAssert(code);
    mprAssert(count == -1 || count == 1);

    mprAssert(code->stackCount >= 0);
    code->stackCount += count;
    mprAssert(code->stackCount >= 0);

    mprLog(cp, level, "Stack %d, after push %d\n", code->stackCount, count);
}


static void popStack(EcCompiler *cp, int count)
{
    EcCodeGen       *code;

    code = cp->state->code;

    mprAssert(code);
    mprAssert(code->stackCount >= 0);

    code->stackCount -= count;
    mprAssert(code->stackCount >= 0);

    mprLog(cp, level, "Stack %d, after pop %d\n", code->stackCount, count);
}


static void resetStack(EcCompiler *cp)
{
    EcCodeGen       *code;

    code = cp->state->code;

    mprAssert(code);

    code->stackCount = 0;
}


static void setStack(EcCompiler *cp, int count)
{
    EcCodeGen       *code;

    code = cp->state->code;

    mprAssert(code);

    code->stackCount = count;
}


static void emptyStack(EcCompiler *cp, int preserve)
{
    EcCodeGen       *code;
    int             count;

    code = cp->state->code;

    mprAssert(code);
    count = code->stackCount - preserve;

    mprAssert(count >= 0);

    if (count <= 0) {
        return;
    }
    if (count == 1) {
        ecEncodeOpcode(cp, EJS_OP_POP);
    } else {
        ecEncodeOpcode(cp, EJS_OP_POP_ITEMS);
        ecEncodeNumber(cp, count);
    }
    code->stackCount = 0;

    mprLog(cp, level, "Stack %d, after empty\n", code->stackCount);

}


/*
 *  Set the default code buffer
 */
static void setCodeBuffer(EcCompiler *cp, EcCodeGen *saveCode)
{
    cp->state->code = saveCode;
    mprLog(cp, level, "Stack %d, after restore code buffer\n", cp->state->code->stackCount);
}


static void addException(EcCompiler *cp, uint tryStart, uint tryEnd, EjsType *catchType, uint handlerStart, uint handlerEnd, 
    int flags)
{
    EcCodeGen       *code;
    EcState         *state;
    EjsEx           *exception;

    state = cp->state;
    mprAssert(state);

    code = state->code;
    mprAssert(code);

    exception = mprAllocObjZeroed(cp, EjsEx);
    if (exception == 0) {
        mprAssert(0);
        return;
    }

    exception->tryStart = tryStart;
    exception->tryEnd = tryEnd;
    exception->catchType = catchType;
    exception->handlerStart = handlerStart;
    exception->handlerEnd = handlerEnd;
    exception->flags = flags;

    mprAddItem(code->exceptions, exception);
}


static void addJump(EcCompiler *cp, EcNode *np, int kind)
{
    EcJump      *jump;

    ENTER(cp);

    //  TODO - better ctx. Can we use jumps?
    jump = mprAllocObjZeroed(cp, EcJump);
    mprAssert(jump);

    jump->kind = kind;
    jump->node = np;
    jump->offset = ecGetCodeOffset(cp);

    mprAddItem(cp->state->code->jumps, jump);

    LEAVE(cp);
}


static void setFunctionCode(EcCompiler *cp, EjsFunction *fun, EcCodeGen *code)
{
    EjsEx       *ex;
    int         next, len;

    /*
     *  Define any try/catch blocks encountered
     */
    next = 0;
    while ((ex = (EjsEx*) mprGetNextItem(code->exceptions, &next)) != 0) {
        ejsAddException(fun, ex->tryStart, ex->tryEnd, ex->catchType, ex->handlerStart, ex->handlerEnd, ex->flags, -1);
    }

    /*
     *  Define the code for the function
     */
    len = mprGetBufLength(code->buf);
    mprAssert(len >= 0);

    if (len > 0) {
        ejsSetFunctionCode(fun, (uchar*) mprGetBufStart(code->buf), len);
    }
}


static void emitNamespace(EcCompiler *cp, EjsNamespace *nsp)
{
    ecEncodeOpcode(cp, EJS_OP_ADD_NAMESPACE);
    ecEncodeString(cp, nsp->uri);
}


/*
 *  Aggregate the allowable kinds of jumps
 */
void ecStartBreakableStatement(EcCompiler *cp, int kinds)
{
    EcState     *state;

    mprAssert(cp);

    state = cp->state;
    state->code->jumpKinds |= kinds;
    state->breakState = state;
}

//  TODO - Reorg this entire file

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
