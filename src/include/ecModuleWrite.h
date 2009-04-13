/**
 *  ecModuleWrite.h - Module creation and writing routines.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#ifndef _h_EC_MODULE_WRITE
#define _h_EC_MODULE_WRITE

#include    "ejs.h"
#include    "ecCompiler.h"

#ifdef __cplusplus
extern "C" {
#endif

/********************************* Prototypes *********************************/
/*
 *  Module file creation routines.
 */
extern void     ecAddFunctionConstants(EcCompiler *cp, EjsFunction *fun);
extern void     ecAddBlockConstants(EcCompiler *cp, EjsBlock *block);
extern int      ecAddConstant(EcCompiler *cp, cchar *str);
extern int      ecAddNameConstant(EcCompiler *cp, EjsName *qname);
extern int      ecAddDocConstant(EcCompiler *cp, EjsTrait *trait, EjsVar *block, int slotNum);
extern int      ecAddModuleConstant(EcCompiler *cp, EjsModule *up, cchar *str);
extern int      ecCreateModuleHeader(EcCompiler *cp, int version, int seq);
extern int      ecCreateModuleSection(EcCompiler *cp);


/*
 *  Encoding emitter routines
 */
extern int      ecEncodeBlock(EcCompiler *cp, uchar *buf, int len);
extern int      ecEncodeByte(EcCompiler *cp, int value);
extern int      ecEncodeLong(EcCompiler *cp, int64 value);
extern int      ecEncodeName(EcCompiler *cp, EjsName *qname);
extern int      ecEncodeNumber(EcCompiler *cp, uint number);
extern int      ecEncodeOpcode(EcCompiler *cp, int value);
extern int      ecEncodeShort(EcCompiler *cp, int value);
extern int      ecEncodeString(EcCompiler *cp, cchar *str);
extern int      ecEncodeGlobal(EcCompiler *cp, EjsVar *obj, EjsName *qname);
extern int      ecEncodeWord(EcCompiler *cp, int value);
#if BLD_FEATURE_FLOATING_POINT
extern int      ecEncodeDouble(EcCompiler *cp, double value);
#endif
extern int      ecEncodeLong(EcCompiler *cp, int64 value);
extern int      ecEncodeByteAtPos(EcCompiler *cp, uchar *pos, int value);
extern int      ecEncodeWordAtPos(EcCompiler *cp, uchar *pos, int value);
extern void     ecCopyCode(EcCompiler *cp, uchar *pos, int size, int dist);
extern uint     ecGetCodeOffset(EcCompiler *cp);
extern int      ecGetCodeLen(EcCompiler *cp, uchar *mark);
extern void     ecAdjustCodeLength(EcCompiler *cp, int adj);

//  TODO - need decimal, Double

#ifdef __cplusplus
}
#endif
#endif /* _h_EC_MODULE_WRITE */

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
