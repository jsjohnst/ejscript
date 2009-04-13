/*
 *	Object.es -- Object class. Base class for all types.
 *
 *	Copyright (c) All Rights Reserved. See details at the end of this file.
 *

	TODO Missing:
	- function hasOwnProperty(name: EnumerableId): Boolean
	- function isPrototypeOf(value: Object): Boolean
	- function propertyIsEnumerable(name: EnumerableId): Boolean
	- function __define_Poperty__(name: EnumerableId, value, enumerable: Boolean = undefined, 
		removable: Boolean = undefined, writable: Boolean = undefined): Boolean
 */
module ejs {

	use default namespace intrinsic

	/**
	 *	The Object Class is the root class from which all objects are based. It provides a foundation set of functions 
	 *	and properties which are available to all objects. It provides for: copying objects, evaluating equality of 
	 *	objects, providing information about base classes, serialization and deserialization and iteration. 
	 */
	dynamic native class Object implements Iterable {

        use default namespace public

		/**
		 *	@spec ecma-3
		 */
		# ECMA && FUTURE
		native function __defineProperty__(name, value, xenumerable = undefined, removable = undefined, writable = undefined)


		/**
		 *	Clone the array and all its elements.
		 *	@param deep If true, do a deep copy where all object references are also copied, and so on, recursively.
		 *	@spec ejs-11
		 */
		native function clone(deep: Boolean = true) : Array


		/**
		 *	Get an iterator for this object to be used by "for (v in obj)"
		 *	@param deep Follow the prototype chain. Only implemented in ECMA compliance mode..
		 *	@return An iterator object.
		 *	@spec ejs-11
		 */
		iterator native function get(deep: Boolean = false, namespaces: Array = null): Iterator


		/**
		 *	Get an iterator for this object to be used by "for each (v in obj)"
		 *	@param deep Follow the prototype chain. Only implemented in ECMA compliance mode..
		 *	@return An iterator object.
		 *	@spec ejs-11
		 */
		iterator native function getValues(deep: Boolean = false, namespaces: Array = null): Iterator


		/**
		 *	Check if an object has a property.
		 *	@param property Name of property to check for.
		 *	@returns true if the object contains the specified property.
		 *	Arg type should be: 
    	 *			type EnumerableId = (int|uint|string|Name)
		 *	@spec ecma-3
		 */
		# ECMA && FUTURE
		native function hasOwnProperty(property: Object): Boolean


		/**
		 *	Is this object a prototype of the nominated argument object.
		 *	@param obj Target object to use in test.
		 *	@returns true if this is a prototype of obj.
		 *	@spec ecma-3
		 */
		# ECMA && FUTURE
		native function isPrototypeOf(obj: Object): Boolean


		/**
		 *	The length of the object.
		 *	@return Returns the most natural size or length for the object. For types based on Object, the number of 
		 *	properties will be returned. For Arrays, the number of elements will be returned. For some types, 
		 *	the size property may be writable. For null objects the length is 0; for undefined objects the length is -1.
		 *	BUG: ECMA specifies to return 1 always
		 */
		native function get length(): Number 


	    //	TODO - this is in-flux in the spec. Better place is in the reflection API.
		/**
		 *	Test and optionally set the enumerability flag for a property.
		 *	@param property Name of property to test.
		 *	@param flag Enumerability flag. Set to true, false or undefined. Use undefined to test the current value.
		 *	@returns true if this is a prototype of obj.
		 *	@spec ecma-3
		 */
		# ECMA
		native function propertyIsEnumerable(property: String, flag: Object = undefined): Boolean


		/**
		 *	Seal a dynamic object. Once an object is sealed, further attempts to create or delete properties will fail 
		 *	and will throw an exception. 
		 *	@spec ejs-11
		 */
		# FUTURE
		native function seal() : Void


		/**
		 *	This function converts an object to a localized string representation. 
		 *	@returns a localized string representation of the object. 
		 *	@spec ecma-3
		 */ 
		# ECMA
		native function toLocaleString(): String


		/**
		 *	Convert an object to a source code representationstring. This function returns a literal string for the object 
		 *		and all its properties. It works recursively and handles nested objects, arrays and other core types. 
		 *	@return This function returns an object literal string.
		 *	@throws TypeError If the object could not be converted to a string.
		 *	@spec ecma-3
		 */ 
		# ECMA
		function toSource(): String {
			return serialize()
		}


		/**
		 *	This function converts an object to a string representation. Types typically override this to provide 
		 *	the best string representation.
		 *	@returns a string representation of the object. For Objects "[object className]" will be returned, 
		 *	where className is set to the name of the class on which the object was based.
		 *	@spec ecma-3
		 */ 
		native function toString(locale: String = null): String


		/**
		 *	Return the value of the object
		 *	@returns this object.
		 *	@spec ecma-3
		 */ 
		# ECMA
		function valueOf(): String {
			return this
		}
	}
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
