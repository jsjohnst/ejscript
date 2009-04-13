/*
 *  ejsMemory.c - Memory class
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */
/********************************** Includes **********************************/

#include    "ejs.h"

/*********************************** Methods *********************************/

#if FUTURE
static EjsVar *getUsedMemoryProc(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, getUsedMemory(ejs));
}


static int getUsedStackProc(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, mprStackSize(ejs));
}


uint ejsGetAvailableMemory(Ejs *ejs)
{
    EjsVar          *memoryClass;
    uint            ram;

    memoryClass =  ejsFindClass(ejs, 0, "System.Memory");

    ram = ejsGetPropertyAsInteger(ejs, memoryClass, "ram");
    return ram - getUsedMemory(ejs);
}



static EjsVar *getAvailableMemoryProc(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    EjsVar          *memoryClass;
    uint            ram;

    memoryClass = ejsFindClass(ejs, 0, "System.Memory");

    ram = ejsGetPropertyAsInteger(ejs, memoryClass, "ram");
    ejsSetReturnValueToInteger(ejs, 0);
    return 0;
}



static uint getUsedMemory(Ejs *ejs)
{
    return 0;
}
#endif


static EjsVar *printStats(Ejs *ejs, EjsVar *unused, int argc, EjsVar **argv)
{
    mprPrintAllocReport(ejs, "Memory.printStats()");
    ejsPrintAllocReport(ejs);
    return 0;
}

/******************************** Initialization ******************************/

void ejsCreateMemoryType(Ejs *ejs)
{
    EjsName     qname;

    ejsCreateCoreType(ejs, ejsName(&qname, "ejs.sys", "Memory"), ejs->objectType, sizeof(EjsObject), ES_ejs_sys_Memory,
        ES_ejs_sys_Memory_NUM_CLASS_PROP, ES_ejs_sys_Memory_NUM_INSTANCE_PROP, EJS_ATTR_NATIVE | EJS_ATTR_OBJECT_HELPERS);
}


void ejsConfigureMemoryType(Ejs *ejs)
{
    EjsType         *type;

    type = ejsGetType(ejs, ES_ejs_sys_Memory);

    ejsBindMethod(ejs, type, ES_ejs_sys_Memory_printStats, (EjsNativeFunction) printStats);
#if FUTURE
    ejsBindMethod(ejs, type, ES_ejs_sys_App_NAME, (EjsNativeFunction) NAME);
#endif
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
