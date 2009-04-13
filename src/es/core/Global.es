/*
 *	Global.es -- Global variables, namespaces and functions.
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/*
	TODO missing:

	Classes: Vector, Map, EncodingError
	Types: EnumerableId, Numeric, Strings, Booleans, FieldIterator, FieldValueIterator, TypeIterator, InterfaceIterator
	Interfaces: Field, FieldValue, Type, NominalType, InterfaceType, ClassType, AnyType, NullType, UndefinedType,
		UnionType, RecordType, FunctionType, ArrayType
	Functions: get(), set()

	function isIntegral(n: Number): Boolean
	function copySign(x: Number, y:Number): Number
	function sign(n: Number): Number					// Return -1 for <0,  1 for >= 0 for NaN
	function isInt(n: Number): Boolean					// integral & 32 bits
	function isUint(n: Number): Boolean					// integral & 32 bits and +ve
	function toInt(n: Number): Number
	function toUint(n: Number): Number
    function get(obj:Object!, name:string) : *;
    function set(obj:Object!, name:string, val:*) : void;
    function eval(cmd: String)
 */

/*
 *	Notes:
 *		native means supplied as a builtin (native C/Java)
 *		intrinsic implies ReadOnly, DontDelete, DontEnum
 */

module ejs {

	use strict

    /**
     *  The public namespace used to make entities visible accross modules.
     */
	public namespace public

    /**
     *  The internal namespace used to make entities visible within a single module only.
     */
	public namespace internal

    /**
     *  The intrinsic namespace used for entities that are part of and intrinsic to, the Ejscript platform.
     */
	public namespace intrinsic

    /**
     *  The iterator namespace used to defined iterators.
     */
	public namespace iterator

    /**
     *  The CONFIG namespace used to defined conditional compilation directives.
     */
	public namespace CONFIG

	use default namespace intrinsic
	use namespace iterator

	use namespace "ejs.sys"

    //  TODO - refactor and reduce these
	/** 
     *  Conditional compilation constant. Used to disable compilation of certain elements.
     */  
	const TODO: Boolean = false

	/** 
     *  Conditional compilation constant. Used to disable compilation of certain elements.
     */  
	const FUTURE: Boolean = false

	/** 
     *  Conditional compilation constant. Used to disable compilation of certain elements.
     */  
	const ASC: Boolean = false

	/** 
     *  Conditional compilation constant. Used to enable the compilation of elements only for creating the API documentation.
     */  
	const DOC_ONLY: Boolean = false

	/** 
     *  Conditional compilation constant. Used to deprecate elements.
     */  
	const DEPRECATED: Boolean = false

    //  TODO - remove. Should be using Config.RegularExpressions
	/** 
     *  Conditional compilation constant. Used to deprecate elements.
     */  
	const REGEXP: Boolean = true

	/*
		FUTURE - ECMA

		type AnyNumber = (Number);
		type AnyBoolean = Boolean
		type AnyType = Boolean
		type AnyString = String
		type FloatNumber = (Number)
        var AnyNumber: Type = Number
	 */


	/**
	 *	Alias for the Boolean type
	 *	@spec ecma4
	 */
	native const boolean: Type = Boolean


	/**
	 *	Alias for the Number type
	 *	@spec ecma4
	 */
	native const double: Type = Number


	/**
	 *	Alias for the Number type
	 *	@spec ejs-11
	 */
	native const num: Type = Number


	/**
	 *	Alias for the String type
	 *	@spec ecma4
	 */
	native const string: Type = String


	/**
	 *	Boolean false value.
 	 */
	native const false: Boolean


	/**
	 *	Global variable space reference. The global variable references an object which is the global variable space. 
     *	This is useful when guaranteed access to a global variable is required. e.g. global.someName.
 	 */
	native var global: Object


	/**
	 *	Null value. The null value is returned when testing a nullable variable that has not yet had a 
	 *	value assigned or one that has had null explicitly assigned.
	 */
	native const null: Null


	/**
	 *	Infinity value.
 	 */
	native const Infinity: Number


	/**
	 *	Negative infinity value.
 	 */
	native const NegativeInfinity: Number


	/**
	 *	Invalid numeric value. If the numeric type is set to an integral type, the value is zero.
	 */
	native const NaN: Number


	/**
	 *	StopIteration used by iterators
	 */
    iterator native final class StopIteration {}


	/**
	 *	True value.
	 */
	native const true: Boolean


	/**
	 *	Undefined variable value. The undefined value is returned when testing
	 *	for a property that has not been defined. 
	 */
	native const undefined: Void


	/**
	 *	void type value. 
	 *	This is an alias for Void.
	 *	@spec ejs-11
	 */
	native const void: Type = Void


	/**
	 *	Assert a condition is true. This call tests if a condition is true by testing to see if the supplied 
	 *	expression is true. If the expression is false, the interpreter will throw an exception.
	 *	@param condition JavaScript expression evaluating or castable to a Boolean result.
	 *	@return True if the condition is.
	 *	@spec ejs-11
	 */
	native function assert(condition: Boolean): Boolean


    /*
     *  Convenient way to trap to the debugger
     */
    native function breakpoint(): Void


    /**
     *  Replace the base type of a type with an exact clone. 
     *  @param klass Class in which to replace the base class.
     *  @spec ejs-11
     */
	native function cloneBase(klass: Type): Void


	/**
	 *	Convert a string into an object. This will parse a string that has been encoded via serialize. It may contain
	 *	nested objects and arrays. This is a static method.
	 *	@param name The string containing the object encoding.
	 *	@return The fully constructed object or undefined if it could not be reconstructed.
	 *	@throws IOError If the object could not be reconstructed from the string.
	 *	@spec ejs-11
	 */
	native function deserialize(obj: String): Object


	# ECMA || FUTURE
	native function decodeURI(str: String): String


	# ECMA || FUTURE
	native function decodeURIComponent(str: String): String


    /**
     *  Dump the contents of objects. Used for debugging, this routine serializes the objects and prints to the standard
     *  output.
     *  @param args Variable number of arguments of any type
     */
	function dump(...args): Void {
		for each (var e: Object in args) {
			print(serialize(e))
		}
	}


	/**
	 *	Print the arguments to the standard error with a new line appended. This call evaluates the arguments, 
	 *	converts the result to strings and prints the result to the standard error. Arguments are converted to 
	 *	strings using the normal JavaScript conversion rules. Objects will have their @toString function called 
	 *	to get a string equivalent of their value. This output is currently vectored to the application log.
	 *	If the log is redirected to a log file, then print output is also.
	 *	@param args Variables to print
	 *	@spec ejs-11
	 */
	native function eprint(...args): void


	# ECMA || FUTURE
	native function escape(str: String): String


	# ECMA || FUTURE
	native function encodeURI(str: String): String


	# ECMA || FUTURE
	native function encodeURIComponent(str: String): String


	/**
	 *	Evaluate a script. Only present in the compiler executable: ec.
	 *	@param script Script to evaluate
	 *	@returns the the script expression value.
	 */
	# FUTURE
	native function eval(script: String): Object


	//	TODO - move this to System/App/Debug and use "platform" (internal use only) namespace
	/**
	 *	Format the current call stack. Used for debugging and when creating exception objects.
	 *	@spec ejs-11
	 */
	native function formatStack(): String


    /**
     *	Get the object's Unique hash id. All objects have a unique object hash. 
     *	@return This property accessor returns a long containing the object's unique hash identifier. 
     *	@spec ecma-3
     */ 
    native function hashcode(o: Object): Number


	# ECMA || FUTURE
	native function isNaN(str: String): Number


	# ECMA || FUTURE
	native function isFinite(str: String): Number


    /**
     *  Load a script or module
     *  @param file path name to load. File will be interpreted relative to EJSPATH if it is not found as an absolute or
     *      relative file name.
     */
	native function load(file: String): Void


	/**
	 *	Print the arguments to the standard output with a new line appended. This call evaluates the arguments, 
	 *	converts the result to strings and prints the result to the standard output. Arguments are converted to 
	 *	strings using the normal JavaScript conversion rules. Objects will have their @toString function called 
	 *	to get a string equivalent of their value. This output is currently vectored to the application log.
	 *	If the log is redirected to a log file, then print output is also.
	 *	@param args Variables to print
	 *	@spec ejs-11
	 */
	native function print(...args): void


	//	TODO - Config.Debug is failing:  # Config.Debug
	/**
	 *	Print variables for debugging.
	 *	@param args Variables to print
	 *	@spec ejs-11
	 */
	native function printv(...args): void


	/**
	 *	Parse a string and convert to a primitive type
	 */
	native function parse(input: String, preferredType: Type = null): Object


	//	TODO - need to document these
	# ECMA || FUTURE
	native function parseInt(str: String, radix: Number = 10): Number


	# ECMA || FUTURE
	native function parseFloat(str: String): Number


	/**
	 *	Encode an object as a string. This function returns a literal string for the object and all its properties. 
	 *	If @maxDepth is sufficiently large (or zero for infinite depth), each property will be processed recursively 
	 *	until all properties are rendered. 
	 *	@param maxDepth The depth to recurse when converting properties to literals. If set to zero, the depth is infinite.
	 *	@param all Encode non-enumerable and class fixture properties and functions.
	 *	@param base Encode base class properties.
	 *	@return This function returns an object literal that can be used to reinstantiate an object.
	 *	@throws TypeError If the object could not be converted to a string.
	 *	@spec ejs-11
	 */ 
	native function serialize(obj: Object, maxDepth: Number = 0, all: Boolean = false, base: Boolean = false): String

	/*
     *  TODO - remove
	 *	Determine the type of a variable. 
	 *	@param o Variable to examine.
	 *	@return Returns a string containing the arguments type. Possible types are:
	 *		@li $undefined "undefined"
	 *		@li $Object "object"
	 *		@li $Boolean "boolean"
	 *		@li $Function "function"
	 *		@li Number "number"
	 *		@li String "string"
	 *	@remarks Note that lower case names are returned for class names.
	 *	@spec ejs-11
	native function typeof(o: Object): String
	 */


    //  TODO - temp only
    function printHash(name: String, o: Object): Void {
        print("%20s %X" % [name, hashcode(o)])
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
