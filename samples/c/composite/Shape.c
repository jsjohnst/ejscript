/*
 *  Shape.c - Native implementation for the Shape class. Includes the loadable entry point for the Sample module.
 *
 *  This sample provides the implementation for the Shape class. It defines the native C functions which are bound 
 *  to the JavaScript methods for the Shape class. It uses a native C structure for Shape instance state. 
 *
 *  The Shape properties are NOT stored as discreate JavaScript properties. Rather they are stored as native C data
 *  types in the instance state structure. This provides the most compact memory organization and is much smaller
 *  than using Script classes or standard native classes. The trade-off, however, is more code, so if you are only 
 *  going to create a small number of object instances, then scripted classes or normal native classes are a probably
 *  a better choice. But if you will be creating lots of instances or if your class has a very large number of 
 *  properties, then composite classes are ideal.
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

/*********************************** Locals ***********************************/
/*
 *  Native class for the Shape instance state.
 */

typedef struct Shape {
    EjsVar      var;                /* Logically extends Object */
    int         x;                  /* x property */
    int         y;                  /* y property */
    int         height;             /* height property */
    int         width;              /* width property */
} Shape;

/*********************************** Helpers **********************************/
/*
 *  Create a new Shape instance. Use by the VM to create instances.
 *
 *  @param ejs VM handle.
 *  @param type Shape type class object from which to create an instance.
 *  @param size Number of extra slots to allocate (ignored).
 */
Shape *createVar(Ejs *ejs, EjsType *type, int size)
{
    return (Shape*) ejsAllocVar(ejs, type, 0);
}


/*
 *  Copy all properties of the instance including all references. If deep is true and if some properties are 
 *  reference types, then recursively copy all their properties and so on.
 *
 *  @param ejs VM handle.
 *  @param sp is set to the object instance to clone. 
 *  @return The newly copied object
 */
static EjsVar *cloneVar(Ejs *ejs, Shape *sp, bool deep)
{
    Shape   *newShape;

    newShape = (Shape*) ejsCreateVar(ejs, sp->var.type, 0);
    if (newShape == 0) {
        ejsThrowMemoryError(ejs);
        return 0;
    }

    newShape->x = sp->x;
    newShape->y = sp->y;
    newShape->height = sp->height;
    newShape->width = sp->width;

    return (EjsVar*) newShape;
}


/*
 *  Get a property from the object
 *
 *  @param ejs VM handle.
 *  @param sp is set to the object instance.
 *  @param slotNum Slot number of the property to retrieve. The VM maps the property names to slots.
 *  @return the property value
 */
static EjsVar *getProperty(Ejs *ejs, Shape *sp, int slotNum)
{
    mprAssert(sp);

    switch (slotNum) {
    case ES_Sample_Shape_x:
        return (EjsVar*) ejsCreateNumber(ejs, sp->x);

    case ES_Sample_Shape_y:
        return (EjsVar*) ejsCreateNumber(ejs, sp->x);

    case ES_Sample_Shape_height:
        return (EjsVar*) ejsCreateNumber(ejs, sp->height);

    case ES_Sample_Shape_width:
        return (EjsVar*) ejsCreateNumber(ejs, sp->width);

    default:
        ejsThrowReferenceError(ejs, "Bad slot reference");
    }
    return 0;
}


/*
 *  Return the number of instance properties.
 *
 *  @param ejs VM handle.
 *  @param sp is set to the object instance.
 *  @return The number of properties
 */
static int getPropertyCount(Ejs *ejs, Shape *sp)
{
    /*
     *  The slot file computes this for us
     */
    return ES_Sample_Shape_NUM_INSTANCE_PROP;
}


/*
 *  Get a property name
 *
 *  @param ejs VM handle.
 *  @param sp is set to the object instance. 
 *  @param slotNum Slot number of the property to retrieve. The VM maps the property names to slots.
 *  @return The qualified property name.
 */
static EjsName getPropertyName(Ejs *ejs, Shape *sp, int slotNum)
{
    /*
     *  To be thread-safe, we must have a different qname structure for each property name.
     */
    switch (slotNum) {
        case ES_Sample_Shape_x: {
            static EjsName qname;
            qname.name = "y";
            qname.space = 0;
            return qname;
        }

        case ES_Sample_Shape_y: {
            static EjsName qname;
            qname.space = 0;
            return qname;
        }

        case ES_Sample_Shape_height: {
            static EjsName qname;
            qname.name = "height";
            qname.space = 0;
            return qname;
         }

        case ES_Sample_Shape_width: {
            static EjsName qname;
            qname.name = "width";
            qname.space = 0;
            return qname;
        }

        default: {
            static EjsName qname;
            qname.name = 0;
            qname.space = 0;
            ejsThrowReferenceError(ejs, "Bad slot reference");
            return qname;
        }
    }
}


/*
 *  Lookup a property by name. This is optionally implemented by native types and could be further optimized by 
 *  hashing these properties. Note: the compiler binds most references to typed properties, so this routine
 *  should not be called often.
 *
 *  @param ejs VM handle.
 *  @param sp is set to the object instance. 
 *  @param name Property name to look for.
 */
static int lookupProperty(Ejs *ejs, Shape *sp, EjsName *qname)
{
    if (strcmp(qname->name, "x") == 0) {
        return ES_Sample_Shape_x;

    } else if (strcmp(qname->name, "y") == 0) {
        return ES_Sample_Shape_y;

    } else if (strcmp(qname->name, "height") == 0) {
        return ES_Sample_Shape_height;

    } else if (strcmp(qname->name, "width") == 0) {
        return ES_Sample_Shape_width;

    } else {
        ejsThrowReferenceError(ejs, "Can't find property %s", qname->name);
        return EJS_ERR;
    }
}


/*
 *  Update the value of the property at slotNum with the given value
 *
 *  @param ejs VM handle.
 *  @param lhs Left hand side object.
 *  @param slotNum Slot number of the property to update. The VM maps the property names to slots.
 *  @param value Value to write to the property.
 */ 
static int setProperty(Ejs *ejs, Shape *sp, int slotNum, EjsVar *value)
{
    mprAssert(sp);

    switch (slotNum) {
    case ES_Sample_Shape_x:
        sp->x = ejsGetInt(value);
        break;

    case ES_Sample_Shape_y:
        sp->y = ejsGetInt(value);
        break;

    case ES_Sample_Shape_height:
        sp->height = ejsGetInt(value);
        break;

    case ES_Sample_Shape_width:
        sp->width = ejsGetInt(value);
        break;

    default:
        ejsThrowReferenceError(ejs, "Bad slot reference");
        return EJS_ERR;
    }
    return slotNum;
}


#if UNUSED
/*
 *  These methods are not required for this sample, but are included to demonstrate their use.
 */

/*
 *  Cast the instance to another type. 
 *
 *  @param ejs VM handle.
 *  @param sp is set to the object instance.
 *  @returns the function result or 0 if an exception is thrown.
 */
static EjsVar *castVar(Ejs *ejs, Shape *sp, EjsType *type)
{
    return (EjsVar*) ejsCreateString(ejs, "[object Shape]");
}


/*
 *  Delete the property at the specified slot.
 *
 *  @param ejs VM handle.
 *  @param sp is set to the object instance.
 *  @returns 0 if the delete is successful. Otherwise a negative error code.
 */
static int deleteProperty(Ejs *ejs, Shape *sp, const char *prop)
{
    ejsThrowTypeError(ejs, "Can't delete properties for this type");
    return EJS_ERR;
}


/*
 *  Delete an instance property by name.
 */
static int deletePropertyByName(Ejs *ejs, EjsObject *obj, EjsName *qname)
{
    ejsThrowTypeError(ejs, "Can't delete properties for this type");
    return EJS_ERR;
}


/*
 *  Free allocated memory before returning the object to the pool. 
 *
 *  @param ejs VM handle.
 *  @param sp is set to the object instance.
 */
static int finalizeVar(Ejs *ejs, Shape *sp)
{
    /*  Free any object state here before an object is garbage collected */
    return 0;
}


/*
 *  Destroy the instance. The default implementation is to just call the GC ejsFreeVar routine which will 
 *  either free the memory or return it to a type specific pool, ready for reuse
 */
static void destroyVar(Ejs *ejs, EjsVar *obj)
{
    ejsFreeVar(ejs, obj);
}


int defineProperty(Ejs *ejs, EjsVar *vp, int slotNum, EjsName *name, EjsType *propType, int attributes, EjsVar *value)
{
    ejsThrowReferenceError(ejs, "Can't define properties in this sample");
    return EJS_ERR;
}


/*
 *  Implement byte code operators.
 *
 *  @param ejs VM handle.
 *  @param lhs Left hand side object.
 *  @param opCode Bytecode opcode to invoke.
 *  @param rhs Right hand side object.
 */
static EjsVar *invokeOperator(Ejs *ejs, Shape *lhs, int opCode, Shape *rhs)
{
    switch (opCode) {

    case EJS_OP_COMPARE_EQ: case EJS_OP_COMPARE_STRICTLY_EQ:
    case EJS_OP_COMPARE_LE: case EJS_OP_COMPARE_GE:
        return (EjsVar*) ejsCreateBoolean(ejs, lhs == rhs);

    case EJS_OP_COMPARE_NE: case EJS_OP_COMPARE_STRICTLY_NE:
    case EJS_OP_COMPARE_LT: case EJS_OP_COMPARE_GT:
        return (EjsVar*) ejsCreateBoolean(ejs, !(lhs == rhs));

    default:
        /*
         *  Pass to the standard Object helpers to implement other op codes
         */
        return (ejs->defaultHelpers->invokeOperator)(ejs, (EjsVar*) lhs, opCode, (EjsVar*) rhs);
    }
    return 0;
}


/*
 *  Mark the non-native properties for the garbage collector
 */
static void markVar(Ejs *ejs, EjsVar *parent, EjsVar *sp)
{
    /*
     *  Call ejsMarkVar on all contained Ejscript properties or objects.
     */
}


/*
 *  Update a property's name (hash name)
 *
 *  @param ejs VM handle.
 *  @param sp is set to the object instance. 
 *  @param sloNum Property slot number to set.
 *  @param name Property name to set.
 */
static EjsVar *setPropertyName(Ejs *ejs, Shape *sp, int slotNum, EjsName *name)
{
    ejsThrowReferenceError(ejs, "Can't define property names for this type");
    return 0;
}

#endif

/******************************************************************************/
/*
 *  The constructor's job is to initialize a bare object instance
 *
 *  function Constructor(height: num, width: num
 */
static EjsVar *constructor(Ejs *ejs, Shape *sp, int argc, EjsVar **argv)
{
    mprAssert(sp);
    mprAssert(argc == 2);

    mprLog(ejs, 1, "Shape()\n");

    sp->x = 0;
    sp->y = 0;
    sp->height = ejsGetInt(argv[0]);
    sp->width = ejsGetInt(argv[0]);

    return (EjsVar*) sp;
}


/*
 *  Compute the area of the shape
 *
 *  function area() num
 */
static EjsVar *area(Ejs *ejs, Shape *sp, int argc, EjsVar **argv)
{
    return (EjsVar*) ejsCreateNumber(ejs, sp->height *sp->width);
}


/******************************************************************************/
/*
 *  Create the Shape class
 */

EjsType *ejsDefineShapeType(Ejs *ejs)
{
    EjsTypeHelpers  *helpers;
    EjsType         *type;
    EjsName         qname;

    /*
     *  Get the Shape class object. This will be created from the mod file for us. But we need to set the object
     *  instance size.
     */
    type = (EjsType*) ejsGetPropertyByName(ejs, ejs->global, ejsName(&qname, EJS_PUBLIC_NAMESPACE, "Shape"));
    if (type == 0) {
        mprError(ejs, "Can't find property %s:%s", qname.space, qname.name);
        return 0;
    }
    type->instanceSize = sizeof(Shape);

    /*
     *  Define the helper functions.
     */
    helpers = type->helpers;
    helpers->cloneVar               = (EjsCloneVarHelper) cloneVar;
    helpers->createVar              = (EjsCreateVarHelper) createVar;
    helpers->getProperty            = (EjsGetPropertyHelper) getProperty;
    helpers->getPropertyCount       = (EjsGetPropertyCountHelper) getPropertyCount;
    helpers->getPropertyName        = (EjsGetPropertyNameHelper) getPropertyName;
    helpers->lookupProperty         = (EjsLookupPropertyHelper) lookupProperty;
    helpers->setProperty            = (EjsSetPropertyHelper) setProperty;

#if UNUSED
    /*
     *  Other possible helpers. For this sample, the default helpers are sufficient. Override these if required 
     *  in your native class.
     */
    helpers->castVar                = (EjsCastVarHelper) castVar;
    helpers->defineProperty         = (EjsDefinePropertyHelper) defineProperty;
    helpers->destroyVar             = (EjsDestroyVarHelper) destroyVar;
    helpers->deleteProperty         = (EjsDeletePropertyHelper) deleteProperty;
    helpers->deletePropertyByName   = (EjsDeletePropertyByNameHelper) deletePropertyByName;
    helpers->finalizeVar            = (EjsFinalizeVarHelper) finalizeVar;
    helpers->invokeOperator         = (EjsInvokeOperatorHelper) invokeOperator;
    helpers->markVar                = (EjsMarkVarHelper) markVar;
    helpers->setPropertyName        = (EjsSetPropertyNameHelper) setPropertyName;
#endif

    /*
     *  Bind the C functions to the JavaScript functions. We use the slot definitions generated
     *  by ejsmod from Shape.es.
     */
    ejsBindMethod(ejs, type, ES_Sample_Shape_Shape,  (EjsNativeFunction) constructor);
    ejsBindMethod(ejs, type, ES_Sample_Shape_area, (EjsNativeFunction) area);

    return type;
}


/******************************************************************************/
/*
 *  Shape loadable module entry point. This will be called by the Ejscript loader 
 *  after the Shape.mod file is loaded and before Shape initializers are run. 
 *
 *  Module entry points be named [NAME]ModuleInit where "[NAME]" is the name of 
 *  the module starting with a lower case letter and  with any "." characters 
 *  converted to underscores.
 */
MprModule *sampleModuleInit(Ejs *ejs)
{
    MprModule   *module;
    EjsType     *type;
    EjsName     qname;

    mprLog(ejs, 1, "Loading Sample module");
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

#endif /* !BLD_FEATURE_STATIC */

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
