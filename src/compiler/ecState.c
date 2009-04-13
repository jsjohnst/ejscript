/**
 *  ecState.c - Manage state for the parser
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejs.h"
#include    "ecCompiler.h"

/************************************ Code ************************************/

/*
 *  Push the state onto the stack
 */
int ecPushState(EcCompiler *cp, EcState *newState)
{
    EcState     *prev;

    prev = cp->state;
    if (prev) {
        /*
         *  Copy inherited fields.
         *  TODO - could we just use structure assignment?
         */
        newState->inClass = prev->inClass;
        newState->inFunction = prev->inFunction;
        newState->inMethod = prev->inMethod;
        newState->blockIsMethod = prev->blockIsMethod;

        newState->stateLevel = prev->stateLevel;
        newState->currentClass = prev->currentClass;
        newState->currentClassNode = prev->currentClassNode;
        newState->currentClassName = prev->currentClassName;
        newState->currentModule = prev->currentModule;
        newState->currentFunction = prev->currentFunction;
        newState->currentFunctionName = prev->currentFunctionName;
        newState->currentFunctionNode = prev->currentFunctionNode;
        newState->topVarBlockNode = prev->topVarBlockNode;
        newState->currentObjectNode = prev->currentObjectNode;
        newState->onLeft = prev->onLeft;
        newState->needsValue = prev->needsValue;
        newState->needsStackReset = prev->needsStackReset;
        newState->preserveStackCount = prev->preserveStackCount;
        newState->code = prev->code;
        newState->varBlock = prev->varBlock;
        newState->optimizedLetBlock = prev->optimizedLetBlock;
        newState->letBlock = prev->letBlock;
        newState->letBlockNode = prev->letBlockNode;
        newState->conditional = prev->conditional;
        newState->instanceCode = prev->instanceCode;
        newState->instanceCodeBuf = prev->instanceCodeBuf;
        newState->staticCodeBuf = prev->staticCodeBuf;
        newState->mode = prev->mode;
        newState->lang = prev->lang;
        newState->inheritedTraits = prev->inheritedTraits;
        newState->disabled = prev->disabled;
        newState->inHashExpression = prev->inHashExpression;
        newState->inSettings = prev->inSettings;
        newState->noin = prev->noin;
        newState->blockNestCount = prev->blockNestCount;
        newState->namespace = prev->namespace;
        newState->defaultNamespace = prev->defaultNamespace;
        newState->breakState = prev->breakState;

        /*
         *  TODO refactor these
         */
        newState->inInterface = prev->inInterface;

    } else {
        newState->lang = cp->lang;
        newState->mode = cp->defaultMode;
    }

    newState->prev = prev;
    newState->stateLevel++;

    cp->state = newState;

    return 0;
}



/*
 *  Pop the state. Clear out old notes and put onto the state free list.
 */
EcState *ecPopState(EcCompiler *cp)
{
    EcState *prev, *state;

    state = cp->state;
    mprAssert(state);

    prev = state->prev;
    mprFree(state);

    return prev;
}



/*
 *  Enter a new level. For the parser, this is a new production rule.
 *  For the ASP processor or code generator, it is a new AST node.
 *  Push old state and setup a new production state
 */
int ecEnterState(EcCompiler *cp)
{
    EcState *state;

    //  TODO - keep state free list for speed
    state = mprAllocObjZeroed(cp, EcState);
    if (state == 0) {
        mprAssert(state);
        //  TBD -- convenience function for this.
        cp->memError = 1;
        cp->error = 1;
        cp->fatalError = 1;
        /* Memory erorrs are reported globally */
        return MPR_ERR_NO_MEMORY;
    }

    if (ecPushState(cp, state) < 0) {
        cp->memError = 1;
        cp->error = 1;
        cp->fatalError = 1;
        return MPR_ERR_NO_MEMORY;
    }

    return 0;
}



/*
 *  Leave a level. Pop the state and pass back the current node.
 */
EcNode *ecLeaveStateWithResult(EcCompiler *cp, EcNode *np)
{
    /*
     *  Steal the result from the current state and pass back to be owned by the previous state.
     */
    if (cp->state->prev) {
        mprStealBlock(cp->state->prev, np);
    } else {
        mprAssert(cp->state);
        mprStealBlock(cp, np);
    }

    cp->state = ecPopState(cp);

    if (cp->fatalError || cp->error) {
        return 0;
    }

    return np;
}


/*
 *  Leave a level. Pop the state and pass back the current node.
 */
void ecLeaveState(EcCompiler *cp)
{
    cp->state = ecPopState(cp);
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
