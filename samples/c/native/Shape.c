/*
 *  Shape.c - Native implementation for the Shape class. Includes the loadable entry point for the Sample module.
 *
 *  This sample provides the implementation for the Shape class. It defines the native C functions which
 *  are bound to the JavaScript methods for the Shape class.
 *
 *  The properties in the Shape class are real JavaScript properties and are accessible as discrete JavaScript
 *  objects. See the composite class if you need a more compact representation for properties.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "ejs.h"

/*
 *  If you would like to use this sample in a static program, remove this test
 *  and manually invoke the SampleModuleInit function in your main program.
 */
#if !BLD_FEATURE_STATIC

/*
 *  Indent so that genDepend won't warn first time when this file doesn't exist
 */
    #include   "Sample.slots.h"

/******************************************************************************/
/*
 *  The constructor's job is to initialize a bare object instance
 *
 *  function Constructor(height: num, width: num)
 */
static EjsVar *constructor(Ejs *ejs, EjsVar *sp, int argc, EjsVar **argv)
{
    mprAssert(sp);
    mprAssert(argc == 2);

    mprLog(ejs, 1, "Shape()");

    ejsSetProperty(ejs, sp, ES_Sample_Shape_x, (EjsVar*) ejs->zeroValue);
    ejsSetProperty(ejs, sp, ES_Sample_Shape_y, (EjsVar*) ejs->zeroValue);
    ejsSetProperty(ejs, sp, ES_Sample_Shape_height, argv[0]);
    ejsSetProperty(ejs, sp, ES_Sample_Shape_width, argv[1]);

    return (EjsVar*) sp;
}


/*
 *  Compute the area of the shape
 *
 *  function area(): Number
 */
static EjsVar *area(Ejs *ejs, EjsVar *sp, int argc, EjsVar **argv)
{
    int     height, width;

    mprAssert(argc == 0);

    mprLog(ejs, 1, "Shape.area()");

    height = ejsGetInt(ejsGetProperty(ejs, sp, ES_Sample_Shape_height));
    width = ejsGetInt(ejsGetProperty(ejs, sp, ES_Sample_Shape_width));

    return (EjsVar*) ejsCreateNumber(ejs, height * width);
    return 0;
}


/******************************************************************************/
/*
 *  Shape loadable module entry point. This will be called by the Ejscript loader 
 *  after the Shape.mod file is loaded and before Shape initializers are run. 
 *
 *  Module entry points be named [NAME]ModuleInit where "[NAME]" is the name of 
 *  the module with the first letter mapped to lower case and with any "." characters 
 *  converted to underscores.
 */
MprModule *SampleModuleInit(Ejs *ejs)
{
    MprModule   *module;
    EjsType     *type;
    EjsName     qname;

    mprLog(ejs, 1, "Loading Sample module\n");
    module = mprCreateModule(ejs, "sample", BLD_VERSION, 0, 0, 0);
    if (module == 0) {
        return 0;
    }

    /*
     *  Get the Shape class object. This will be created from the mod file for us.
     */
    type = (EjsType*) ejsGetPropertyByName(ejs, ejs->global, ejsName(&qname, "Sample", "Shape"));
    if (type == 0) {
        mprError(ejs, "Can't find type %s", qname.name);
        return 0;
    }

    /*
     *  Bind the C functions to the JavaScript functions. We use the slot definitions generated
     *  by ejsmod from Shape.es.
     */
    ejsBindMethod(ejs, type, ES_Sample_Shape_Shape, (EjsNativeFunction) constructor);
    ejsBindMethod(ejs, type, ES_Sample_Shape_area, (EjsNativeFunction) area);

    return module;
}

#endif /* !BLD_FEATURE_STAITC */
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
