/*
 *	Reflect.es - Reflection API and class
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs {

	use default namespace intrinsic

	/*
	 *	Usage:
	 *
	 *	name      = Reflect(obj).name
	 *	type      = Reflect(obj).type
	 *	typeName  = Reflect(obj).typeName
     *
     *  TODO namespace
	 */

    /**
     *  Simple reflection class
     */
    native final class Reflect {
        use default namespace public

        native private var obj: Object

        /**
         *  Create a new reflection object.
         *  @param object to reflect upon
         */
        native function Reflect(o: Object)

        /**
         *  Get the name of the object
         *  @return a string describing the name of the object
         */
        native function get name(): String

        /**
         *  Get the type of the object
         *  @return the type object for object being examined.
         */
        native function get type(): Type

        /**
         *  Get the type name of the object
         *  @return a string containing the type name object for object being examined.
         */
        native function get typeName(): String

        # FUTURE
        native function get baseClass(): Type
    }

    /**
     *  Return the name of a type. This is a fixed version of the standard "typeof" operator. It returns the real
     *  Ejscript underlying type. 
     *  This is implemented as a wrapper around Reflect(o).typeName.
     *  @param o Object or value to examine. 
     *  @return A string type name. If the object to examine is a type object, then return the name of the base type.
     *      If the object is Object, then return null.
     *  @spec ejs-11
     */
    function typeOf(o): String {
        return Reflect(o).typeName()
    }


/*
 *  ES4 reflection proposal
 
	# ECMA
	intrinsic function typeOf(e: *): Type

	interface Type {
		function canConvertTo(t: Type): Boolean
		function isSubtypeOf(t: Type): Boolean
	}

	interface Field {
		function namespace() : String
		function name(): String
		function type(): Type
	}

	type FieldIterator = iterator::IteratorType.<Field>

	interface NominalType extends Type {
		function name(): String
		function namespace(): String
		function superTypes(): IteratorType.<ClassType>
		function publicMembers(): FieldIterator
		function publicStaticMembers(): FieldIterator
	}

	interface InterfaceType extends NominalType {
		function implementedBy():IteratorType.<ClassType>
	}

	type TypeIterator = iterator::IteratorType.<Type>
	type ValueIterator = iterator::IteratorType.<*>

	interface ClassType extends NominalType {
		function construct(typeArgs: TypeIterator, valArgs: ValueIterator): Object
	}

	interface UnionType extends Type {
		function members(): TypeIterator
		function construct(typeArgs: TypeIterator, valArgs: ValueIterator): *
	}

	interface FieldValue {
		function namespace() : String
		function name(): String
		function value(): *
	}

	type FieldValueIterator = iterator::IteratorType.<FieldValue>

	interface RecordType extends Type {
		function fields(): FieldIterator
		function construct(typeArgs: TypeIterator, valArgs: FieldValueIterator): Object
	}

	interface FunctionType extends Type {
		function hasBoundThis(): Boolean
		function returnType(): Type
		function argTypes(): TypeIterator
		function construct(typeArgs: TypeIterator, valArgs: ValueIterator): *
		function apply(typeArgs: TypeIterator, thisArg: Object?, valArgs: ValueIterator): *
	}
*/
}

/*
 *	@copy	default
 *	
 *	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *	Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Embedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.embedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Embedthis 
 *	Software at http://www.embedthis.com 
 *	
 *	@end
 */
