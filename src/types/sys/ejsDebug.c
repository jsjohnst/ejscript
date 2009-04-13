/*
 *  ejsDebug.c - System.Debug class
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */
/********************************** Includes **********************************/

#include    "ejs.h"

#if UNUSED
/************************************ Methods *********************************/
/*
 *  function bool isDebugMode()
 *  MOB -- convert to accessor
 */

static int isDebugMode(EjsFiber *fp, EjsVar *thisObj, int argc, EjsVar **argv)
{
    ejsTrace(fp, "isDebugMode()\n");
    ejsSetReturnValueToInteger(fp, mprGetDebugMode(fp));
    return 0;
}

/******************************** Initialization ******************************/

int ejsDefineDebugClass(EjsFiber *fp)
{
    EjsVar  *systemDebugClass;

    systemDebugClass =  ejsDefineClass(fp, "System.Debug", "Object", 0);
    if (systemDebugClass == 0) {
        return MPR_ERR_CANT_INITIALIZE;
    }

    /*
     *  Define the class methods
     */
    ejsDefineCMethod(fp, systemDebugClass, "isDebugMode", isDebugMode,
        0);

    return ejsObjHasErrors(systemDebugClass) ? MPR_ERR_CANT_INITIALIZE : 0;
}
#else
void __dummyEjsDebug() {}
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
