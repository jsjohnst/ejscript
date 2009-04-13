//	OLD EJScript 2.0 doc
/**
 *	@file 	ejscript.java
 *	@overview 	Documentation for the EJScript 2.0 Language.
 *
 *	@description EJScript is an enhanced implementation of the ECMA-262
 *		(Javascript) language. EJScript is an interpreted, object oriented
 *		scripting language. It supports objects, classes, exceptions,
 *		statements, expressions and a powerful suite of data types. 
 *		\n\n
 *		EJScript is an enhanced version of Javascript 1.5 and it improves on
 *		the base language by implementing key elements of the Javascript 2.0
 *		proposal, such as classes and exceptions. It also implements several
 *		new language features that go beyond Javascript 2.0 such as the use of
 *		mixins.
 *		\n\n
 *		EJScript innovates and fixes several key deficiencies in the ECMA
 *		standard.  You can optionally run EJ in ECMA-STANDARD mode, but you
 *		lose the benefit of these important improvements. For example, one of
 *		these improvements changes the behavior of variable declarations
 *		inside functions to declare variables as locals instead of as globals
 *		by default.
 *		\n\n
 *		EJScript is a dynamic typed language unlike C, C++, Java or C#.
 *		Variables are typed according to the data they contain at the time and
 *		by what they can do. This is sometimes called duck-typing. EJ will
 *		dynamically convert and cast variables to the appropriate type as
 *		required. All variables, functions and classes in EJ may be treated as
 *		objects. 
 */

/*
 *	TODO: 
 *	- The following attributes are not documented: dynamic, final
 *	- Add ATsee for all classes
 */

/********************************* Copyright **********************************/
/*
 *	@copy	default
 *	
 *	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
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

/****************************** EJS Standard Variables ************************/

/**
 *	@overview Root class from which all classes, modules and objects inherit.
 *	@description The Object Class is the root class from which all EJ objects
 *		are based. EJ classes and modules are also directly or indirectly 
 *		derrived from Object. The Object class provides a foundation set of 
 *		functions and properties which are available to all objects.
 *	@see Array, Boolean, Function, Global, Number, String
 */
class Object
{
	/**
	 * 	@overview Constructor for an object
	 *	@description Create a new object.
	 *		JavaScript objects can contain data properties and functions. All
	 *		objects contain a \e toString function that by default returns
	 *		"[object name]". This function may be replaced by assigning 
	 *		a function to the toString property.  
	 *	@return Returns the constructed array object
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see baseClass, hash, mixin, prototype, toSource, toString, valueOf
	 */
	public static Object Object();

	/**
	 *	@overview Return a string representation of an object.
	 *	@description This function converts an object to a string
	 *	representation. For objects, it will return [object className]. 
	 *		Objects may override this function.
	 *	@return This function returns [object className], where
	 *		className is set to the name of the class on which the object was 
	 *		based.
	 *	@stability Prototype. 
	 *	@library Builtin. 
	 *	@see Object, baseClass, hash, mixin, prototype, toSource, valueOf
	 */ 
	public String toString();


	/**
	 *	@overview Return a string source code representation of an object.
	 *	@description This function returns a literal string for
	 *		the object and all its properties. If \a maxDepth is sufficiently
	 *		large (or zero for infinite depth), each property will be
	 *		processed recursively until all properties are rendered as object
	 *		literals. Objects may override this function. Arrays which are
	 *		based on Objects override toString and return array literals. 
	 *	@param maxDepth The depth to recurse when converting properties to
	 *		literals. If set to zero, the depth is infinite.
	 *	@param showHidden Show non-enumerable properties and functions.
	 *	@param showBase Show base class properties.
	 *	@return This function returns an object literal that can be used to 
	 *		reinstantiate an object.
	 *	@stability Prototype. 
	 *	@library Builtin. 
	 *	@see Object, baseClass, hash, mixin, prototype, toString, valueOf
	 */ 
	public String toSource(
			Number maxDepth = 0, 
			Boolean showHidden = false, 
			Boolean showBase = false);


	/**
	 *	@overview Return the value of an object
	 *	@description This function returns an equivalent primitive value for 
	 *		the object. For objects, this function returns [object className], 
	 *		where className is set to the name of the class on which the 
	 *		object was based. 
	 *	@return This function returns [object className], where
	 *		className is set to the name of the class on which the object was
	 *		based.
	 *	@stability Prototype. 
	 *	@library Builtin. 
	 *	@see Object, baseClass, hash, mixin, prototype, toSource, toString
	 */ 
	public var valueOf();


	/**
	 *	@overview Mix in a module into a class or object.
	 *	@description This routine blends the functions and properties from a
	 *		supplied module into the specified object. Modules that are
	 *		blended in this manner are known as "mixins". Mixins are used to
	 *		add horizontal functionality to classes or objects. Because
	 *		classes are also objects, mixin can be used to add new properties
	 *		and functions to a class.
	 *	@param module The module to mix in.
	 *	@stability Prototype. 
	 *	@library Builtin. 
	 *	@see Object, baseClass, hash, prototype, toSource, toString, valueOf
	 */ 
	public void mixin(Object module);


	/**
	 *	@overview Mix in a module into a class or object.
	 *	@description This function blends the functions and properties from a
	 *		supplied module into the specified object. Modules that are
	 *		blended in this manner are known as "mixins". Mixins are used to
	 *		add horizontal functionality to classes or objects. Because
	 *		classes are also objects, mixin can be used to add new properties
	 *		and functions to a class.
	 *	@param code Literal EJ code to mix in.
	 *	@stability Prototype. 
	 *	@library Builtin. 
	 *	@see Object, baseClass, hash, prototype, toSource, toString, valueOf
	 */ 
	public void mixin(String code);

	
	/**
	 *	@overview Base class name of the object
	 *	@description The name of an object's direct base class.
	 *	@return String containing the name of the base class on which the 
	 *		object was based.
	 *	@access Read only.
	 *	@stability Prototype. 
	 *	@library Builtin. 
	 *	@see Object, hash, prototype, toSource, toString, valueOf
	 */ 
	public readonly String baseClass;


	/**
	 *	@overview Base class reference for the object
	 *	@description Reference to the base class on which the object was based.
	 *	@access Read only.
	 *	@stability Prototype. 
	 *	@library Builtin. 
	 *	@see Object, hash, prototype, toSource, toString, valueOf
	 */ 
	public readonly Object prototype;


	/**
	 *	@overview Unique object hash id
	 *	@description All objects have a unique object hash. This property 
	 *		accessor returns a Number containing the object's unique hash 
	 *		identifier. 
	 *	@access Read only.
	 *	@stability Prototype. 
	 *	@library Builtin. 
	 *	@see Object, baseClass, prototype, toSource, toString, valueOf
	 */ 
	public Number hash;
}



/**
 *	@overview Boolean type class
 *	@description The Boolean class wraps the primitive Boolean type and
 *		provides a set of static functions for interacting with boolean types
 *		and converting them to string representations.
 *	@see Array, Function, Global, Number, Object, String
 */
class Boolean extends Object
{ 
	public static Boolean Boolean();
}



/**
 *	@overview Number type class
 *	@description The Number class wraps the primitive Number type and
 *		provides a set of static functions for interacting with Number types
 *		and converting them to string representations.
 *	@see Array, Boolean, Function, Global, Object, String
 */
class Number extends Object
{ 
	public void abs(Numeric a);
	public void min(Number a, Number b);
	public void max(Number a, Number b);
}


/**
 *	@overview String type class
 *	@description The String class wraps the primitive String type and provides
 *		a set of static functions for interacting with String types. EJ 
 *		represents strings internally as primitive types and string 
 *		capability is built into the language. Strings may contain binary
 *		data.
 *	@remarks The returned value from this classes constructor will be a
 *		primitive function representing the supplied function string.
 *	@see Array, Boolean, Function, Global, Number, Object
 */
class String extends Object
{ 
	/**
	 *	@overview Calculate the length of a string
	 *	@description Determine the length of the supplied string.
	 *	@remarks This function will be deprecated when String.length is
	 *		supported.
	 *	@param str String to measure.
	 *	@return Returns a Number containing the number of bytes in \a str.
	 *		function literal string.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see strstr
	 */
	public static Number strlen(String str);


	/**
	 *	@overview Find a substring in a string
	 *	@description Determine the length of the supplied string.
	 *	@remarks This function will be deprecated when String.subst is
	 *		supported.
	 *	@param str String to measure.
	 *	@param pat String pattern to search for
	 *	@return Returns a new substring that starts with the pattern or
	 *		undefined if the pattern is not found.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see strlen
	 */
	public static String strstr(String str, String pat);
}


/**
 *	@overview Class to hold an array of objects.
 *	@description The Array Class manages indexed arrays. It provides a 
 *		growable array that can hold an arbitrary set of objects. Objects do
 *		not need to be of the same class.
 *		Arrays store objects as propeties with a zero-based numeric index.
 *		Arrays can be spares with gaps between elements.
 *	@see Boolean, Function, Global, Number, Object, String
 */
class Array extends Object
{ 
	/**
	 *	@overview Constructor for an Array object
	 *	@description Create an Array object. An empty array object will 
	 *		be created with \a size initial elements.  These elements will be
	 *		named "0", "1", "2" etc. The value of these elements will
	 *		initially be the @e undefined value. 
	 *		\n\n
	 *		The array object will have a length property defined that will be
	 *		set to the number of elements currently in the array. The array
	 *		will grow on demand as new elements are added to the array.
	 *		\n\n
	 *		Arrays inherit all the standard properties and functions from EJS
	 *		Objects.
	 *	@param size Optional argument specifying the initial number of 
	 *		elements in the array.
	 *	@return Returns the constructed array object
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see Object
	 */
	public static Array Array(Number size = 0);


	/**
	 *	@overview Constructor for an Array object
	 *	@description Create an Array object using the supplied elements.
	 *		The array object will set its length property to the number
	 *		of elements supplied.
	 *	@param elt1 Element one
	 *	@param elt2 Element two
	 *	@return Returns the constructed array object
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see Object
	 */
	public static Array Array(var elt1, var elt2, ...);


	/**
	 *	@overview Length of the Array
	 *	@description The length property is set to the size of the array. The
	 *		size is determined by the index of the greatest array element.
	 *	@remarks This property is not enumerable if the Array is used in a
	 *		for-in loop.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see Array
	 */
	public Number length;
}



/**
 *	@overview Class to hold all global variables and functions.
 *	@description The Global Class is a singleton class that is used to create
 *		an instance of the global variable. It holds all publicaly accessible
 *		global variables (properties) and functions.
 *	@remarks The global object is automatically created by EJ for each
 *		interpreter instance. The Global class is a singleton and should not
 *		be instantiated by users.
 *	@see Array, Boolean, Function, Global, Number, Object, String
 */
class Global extends Object
{
	/** 
	 *	@overview Boolean false value.
	 *	@description Global property set to the Boolean false value. 
	 *	@remarks Only present if floating point enabled via 
	 *		BLD_FEATURE_FLOATING_POINT.
	 *	@stability Prototype. 
	 *	@library Builtin. 
	 *	@see Infinity, Nan, null, undefined, true
	 */
	public static const Boolean false;
	
	
	/**
	 *	@overview Floating number set to Infinity
	 *	@remarks Only present if floating point enabled via 
	 *		BLD_FEATURE_FLOATING_POINT.
	 *	@stability Prototype. 
	 *	@library Builtin. 
	 *	@see false, Nan, null, undefined, true
	 */
	public static const Float Infinity;
	
	
	/** 
	 *	@overview Floating variable set to "Not a number"
	 *	@remarks Only present if floating point enabled via 
	 *		BLD_FEATURE_FLOATING_POINT.
	 *	@stability Prototype. 
	 *	@library Builtin. 
	 *	@see false, Infinity, null, undefined, true
	 */
	public static const Float Nan;
	
	
	/** 
	 *	@overview Null variable value
	 *	@description Special value variables have if they are defined but 
	 *		have not been assigned a value.
	 *	@stability Prototype. 
	 *	@library Builtin. 
	 *	@see false, Infinity, Nan, undefined, true
	 */
	public static const var null;
	
	
	/** 
	 *	@overview Undefined variable value
	 *	@description Value returned when testing for an object property that 
	 *		has not been defined 
	 *	@stability Prototype. 
	 *	@library Builtin. 
	 *	@see false, Infinity, Nan, null, true
	 */
	public static const var undefined;
	
	
	/** 
	 *	@overview Boolean true value.
	 *	@description Global property set to the Boolean true value. 
	 *	@stability Prototype. 
	 *	@library Builtin. 
	 *	@see false, Infinity, Nan, null, undefined
	 */
	public static const Boolean true;
	

	/**
	 *	@overview Assert a condition is true
	 *	@description This call tests if a condition is true by testing to see
	 *		if the supplied expression is true. If the expression is false,
	 *		the interpreter will throw an exception.
	 *	@param condition JavaScript expression evaluating or castable to 
	 *		a boolean result.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see eval, exit, print, println, trace
	 */
	public static void assert(Boolean condition);


	/**
	 *	@overview Evaluate a script
	 *	@description This call evaluates the given JavaScript \a script in the
	 *		current context. It provides a feature to dynamically modify the
	 *		code executed by the interpreter. It is also useful to evaluate
	 *		the value of complex expressions as the call will return the value
	 *		of the last expression evaluated.  
	 *		\n\n 
	 *		The script is executed with the current local and global
	 *		variables.  No new local variable stack frame is created.
	 *	@param script JavaScript to execute
	 *	@return Returns the value of the last expression evaluated.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see assert, exit, print, println, trace
	 */
	public static void eval(String script);


	/**
	 *	@overview Exit a script
	 *	@description This call immediately exits the current script.
	 *		Useful when you immediately want to exit a script.
	 *		This call is identical to System.exit.
	 *	@param status Numeric status code. This code is retrievable via the
	 *	ejsGetExitCode API.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see assert, print, println, trace, System.exit
	 */
	public static void exit(Number status);


	/**
	 *	@overview Include an JavaScript file with local context
	 *	@description This call includes a JavaScript at the point of the
	 *		include statement. The effect is as though the text from the
	 *		included file were pasted into the original file at the point of 
	 *		the include statement. The script executes with the context of the
	 *		original file and uses its local and global variables that exist
	 *		at the time of execution of the statement.
	 *	@param path Path name of file to be included.
	 *	@return Returns the value of last expression executed in the included
	 *		script
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see eval, includeGlobal
	 */
	public static void include(String path, ...);


	/**
	 *	@overview Include an JavaScript file with global context
	 *	@description This call includes a JavaScript and executes it with
	 *		global scope. The effect is as though the text from the
	 *		included file were run outside all functions and classes.
	 *		The script executes with the context of the
	 *		original file and uses its global variables that exist
	 *		at the time of execution of the statement.
	 *	@param path Path name of file to be included.
	 *	@return Returns the value of last expression executed in the included
	 *		script
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see eval, include
	 */
	public static void includeGlobal(String path, ...);


	/**
	 *	@overview Print the arguments to the standard output
	 *	@description This call evaluates the arguments, converts the result 
	 *		to strings and prints the result to the standard output.
	 *		Arguments are converted to strings using the normal JavaScript
	 *		conversion rules. Objects will have their @e toString function
	 *		called to get a string equivalent of their value.  
	 *	@param msg String to print
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see assert, assert, exit, println, printv, trace
	 */
	public static void print(String msg, ...);


	/**
	 *	@overview Print the arguments to the standard output with a new line
	 *	@description This call evaluates the arguments, converts the result 
	 *		to strings, appends a new line character and prints the result to
	 *		the standard output.  Arguments are converted to strings using the
	 *		normal JavaScript conversion rules. Objects will have their @e
	 *		toString function called to get a string equivalent of their value.
	 *	@param var String to print
	 *	@return Returns the value of the last expression evaluated.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see assert, exit, print, printv, trace
	 */
	public static void println(String var, ...);


	/**
	 *	@overview Print the value of variables with a new line.
	 *	@description Print variables with their values. The printv function
	 *		differs from print or println in that it will print the name of
	 *		the variables and their values followed by a newline. E.g.
	 *
	 *		@code
	 *	printv(x, y);
	 *		@endcode
	 *		will output
	 *		@code
	 *	x = 4, y = 5
	 *		@endcode
	 *		use.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see assert, exit, print, println, trace
	 */
	public static void printv(var variable, ...);


	/**
	 *	@overview Output trace to the debug log
	 *	@description This call outputs the given message to the debug log.  
	 *		An optional numeric trace log level between 0 and 9 may be given
	 *		as the first argument (not shown).
	 *	@param msg Message to log
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see assert, exit, print, println
	 */
	public static void trace(String msg, ...);


	/**
	 *	@overview Sort the properties in an object
	 *	@description This function sorts the properties in an object using
	 *		simple lexical ordering.
	 *	@param object Object whoes properties are to be sorted.
	 *	@param order Order is 1, then the properties are sorted in ascending
	 *		order, otherwise, the sort order is descending.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see assert, exit, print, println, trace
	 */
	public static void sort(Object object, Number order = 0);


	/**
	 *	@overview Sort the object properties in an object
	 *	@description This function sorts the objects in an object according
	 *		to a specified property.
	 *	@param object Object containing the objects to sort.
	 *	@param order Order is 1, then the properties are sorted in ascending
	 *		order, otherwise, the sort order is descending.
	 *	@param propertyName Name of the property in the referenced objects to
	 *		compare.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see assert, exit, print, println, trace, typeof
	 */
	public static void sort(Object object, Number order = 0, 
		String propertyName);


	/**
	 *	@overview Determine the type of a variable
	 *	@description Determine the type of a variable. 
	 *	@param v Variable to examine.
	 *	@return Returns a string containing the variables type. Possible types
	 *		are:
	 *		@li @ref undefined "undefined"
	 *		@li @ref Object "object"
	 *		@li @ref Boolean "boolean"
	 *		@li function
	 *		@li @ref Number "number"
	 *		@li @ref String "string"
	 *		@li pointer
	 *	@remarks Note that lower case names are returned for class names.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see assert, exit, print, println, sort, trace
	 */
	public static String typeof(var v);
}


/**
 *	@overview Class to create dynamic functions
 *	@description This class exists to allow functions to be created from
 *		arbitrary scripts.
 *	@remarks The returned value from this classes constructor will be a
 *		primitive function representing the supplied function string.
 *	@see Array, Boolean, Global, Number, Object, String
 */
class Function extends Object
{
	/**
	 *	@overview Create a function from a string
	 *	@description Creates a function from the supplied string. The string
	 *		must be of the format "function NAME(ARGS) { BODY }".
	 *	@param functionListeral String containing the function source script.
	 *	@return Returns a primitive function data type representing the supplied
	 *		function literal string.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see Object, eval, include, includeGlobal
	 */
	public static function Function(String functionListeral);
}


/**
 *	@overview Base class for error exception objects.
 *	@description The Error clase is the base class for all error classes that
 *		are used to throw exceptions.
 *	@see AssertError, EvalError, InternalError, IOError, MemoryError,
 *		RangeError, ReferenceError, SyntaxError, TypeError
 */
class System.Error extends Object
{
	/**
	 *	@overview Name of the error 
	 *	@description Error class name
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see message, stack
	 */
	public String name;

	/**
	 *	@overview Exception error message
	 *	@description  Some desc
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see name, stack
	 */
	public String message;

	/**
	 *	@overview EJ stack exception backtrace
	 *	@description  Some desc
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see message, name
	 */
	public String stack;

	/**
	 *	@overview Length of the Array
	 *	@description The length property is set to the size of the array. The
	 *		size is determined by the index of the greatest array element.
	 *	@remarks This property is not enumerable if the Array is used in a
	 *		for-in loop.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see Array
	 */
	public static Error Error(message);
}


/**
 *	@overview Script assert error class 
 *	@description An instance of the AssertError class is thrown for 
 *		program assertion errors. The assert function may throw AssertErrors
 *		if the assert condition is not satisfied.
 *	@see Error, EvalError, InternalError, IOError, MemoryError,
 *		RangeError, ReferenceError, SyntaxError, TypeError
 */
class System.AssertError extends Error 
{ 
	public static AssertError AssertError();
}


/**
 *	@overview Script evaluation error class 
 *	@description An instance of the EvalError class is thrown for 
 *		script evaluation errors.
 *	@see AssertError, Error, InternalError, IOError, MemoryError,
 *		RangeError, ReferenceError, SyntaxError, TypeError
 */
class System.EvalError extends Error 
{
	public static EvalError EvalError();
}


/**
 *	@overview Internal script evaluation error class 
 *	@description An instance of the InternalError class is thrown for 
 *		script evaluation errors.
 *	@see AssertError, Error, EvalError, IOError, MemoryError,
 *		RangeError, ReferenceError, SyntaxError, TypeError
 */
class System.InternalError extends Error { }


/**
 *	@overview IO error class 
 *	@description An instance of the IOError class should be thrown when 
 *		I/O or network errors occur.
 *	@see AssertError, Error, EvalError, InternalError, MemoryError,
 *		RangeError, ReferenceError, SyntaxError, TypeError
 */
class System.IOError extends Error { }


/**
 *	@overview Memory error class 
 *	@description An instance of the MemoryError class is thrown when 
 *		a memory allocation error occurs.
 *	@see AssertError, Error, EvalError, InternalError, IOError, 
 *		RangeError, ReferenceError, SyntaxError, TypeError
 */
class System.MemoryError extends Error { }


/**
 *	@overview Numeric range error class 
 *	@description An instance of the RangeError class is thrown when 
 *		a numeric value is outside the acceptable range.
 *	@see AssertError, Error, EvalError, InternalError, IOError, MemoryError,
 *		ReferenceError, SyntaxError, TypeError
 */
class System.RangeError extends Error { }


/**
 *	@overview Object or property reference error class 
 *	@description An instance of the ReferenceError class is thrown when 
 *		an object or property reference cannot be resolved.
 *	@see AssertError, Error, EvalError, InternalError, IOError, MemoryError,
 *		RangeError, SyntaxError, TypeError
 */
class System.ReferenceError extends Error { }


/**
 *	@overview Script syntax error class 
 *	@description An instance of the SyntaxError class is thrown when 
 *		a script syntax error is encountered.
 *	@see AssertError, Error, EvalError, InternalError, IOError, MemoryError,
 *		RangeError, ReferenceError, TypeError
 */
class System.SyntaxError extends Error { }


/**
 *	@overview Wronge data type error class 
 *	@description An instance of the TypeError class is thrown when 
 *		an expression encounteres data of the wrong type that cannot be cast
 *		or converted to the required type. EJ is a duck-typing language and in
 *		most cases, it will automatically convert data to the required type.
 *	@see AssertError, Error, EvalError, InternalError, IOError, MemoryError,
 *		RangeError, ReferenceError, SyntaxError
 */
class System.TypeError extends Error { }


/**
 *	@overview System Object Model (SOM) class
 *	@description The System class is a singleton class which acts as the
 *		namespace for the EJ System Object Model (SOM). It contains static
 *		functions and classes useful for EJ programs.
 *	@see App, Arguments, AssertError, Callee, Debug, Error, EvalError, Event,
 *		EventTarget, FileInfo, FileSystem, GC, InternalError, IOError,
 *		Listener, Local, Log, Memory, MemoryError, RangeError, ReferenceError,
 *		SyntaxError, Timer, TimerService, TypeError, XML
 */
class System extends Object
{
	/**
	 *	@overview Exit a script
	 *	@description This call immediately exits the current script.
	 *		This call is useful when you immediately want to exit a script.
	 *		This API is identical to Global.exit.
	 *	@param status Numeric status code. This code is retrievable via the
	 *	ejsGetExitCode API.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see assert, print, println, trace
	 */
	public static void exit(Number status);


	/**
	 * 	@overview Get the date
	 *	@description This function returns the date in an operating system
	 *		standard (and dependent) manner. 
	 *	@remarks This function will be deprecated when the Date class is fully
	 *		supported.
	 *	@return For BREW, return the date in seconds since Jan 6, 1980 GMT.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see exit, time
	 */
	public static Number date();


	/**
	 * 	@overview Sleep and suspend execution.
	 *	@description Sleep for the given number of milliseconds.
	 *	@param msec Number of milliseconds to sleep.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see exit, time
	 */
	public static void sleep(Number msec);


	/**
	 * 	@overview Time of day
	 *	@description This function returns the time of day as milliseconds.
	 *	@remarks On systems with 32-bit CPUs, this number may wrap if the 
	 *		application runs for more than 49 days.
	 *	days.
	 *	@return Number of milliseconds since 12:00 AM on the day the system
	 *		was booted.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see exit, time
	 */
	public static Number time();
}


/**
 *	@overview Application configuration class
 *	@description Singleton class holding the application's name, title and 
 *		version.
 *	@see System.Debug, System.Memory
 */
class System.App extends Object
{
	/**
	 *	@overview Application name
	 *	@description Single word, lower case name for the application
	 *	@remarks E.g. turbofan
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see title, version
	 */
	public static const String name;


	/**
	 *	@overview Application title name
	 *	@description Multi word, Camel Case name for the application
	 *	@remarks E.g. Acme Technologies Turbo Fan
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see name, version
	 */
	public static const String title;

	/**
	 *	@overview Application version 
	 *	@description Application version string. The format is typically
	 *		Major.Minor.Patch
	 *	@remarks E.g. 1.1.2
	 *	@stability Prototype
	 *	@library Builtin
	 */
	public static const String version;

	/**
	 *	@overview Command line arguments 
	 *	@description Application command line arguments.
	 *	@stability Prototype
	 *	@library Builtin
	 */
	public static const Array args;
}


/**
 *	@overview Debug configuration class
 *	@description Singleton class containing the application's debug
 *	configuration.
 *	@see System.App, System.Memory
 */
class System.Debug extends Object
{
	/**
	 *	@overview Current debug mode
	 *	@description Application debug mode. This property is read-write.
	 *		Setting debugMode to true will put the application in debug mode. 
	 *		When debugMode is enabled, EJ will typically suspend timeouts and
	 *		will take other actions to make debugging easier.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see breakpoint
	 */
	public static Boolean debugMode;


	/**
	 *	@overview Break to the debugger
	 *	@description Suspend execution and break to the debugger.
	 *	@param expressionResult Value to pass to the debugger breakpoint
	 *		routine.
	 *		literals. If set to zero, the depth is infinite.
	 *	@stability Prototype. 
	 *	@library Builtin. 
	 *	@see debugMode
	 */ 
	private void breakpoint(Boolean expressionResult);
}


/**
 *	@overview Garbage Collector class
 *	@description Singleton class to control the operation of the EJ Garbage
 *		Collector.
 *	@see System.Debug, System.Memory
 */
class System.GC extends Object
{
	/**
	 *	@overview Debug Level
	 *	@description Debug level for the EJ garbage collector. Zero is the
	 *		least verbose and level 5 is the most verbose.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see enableDemandCollect, enable, enableIdleCollect, run, workQuota
	 */
	public static Number debugLevel;


	/**
	 *	@overview Control on-demand garbage collection
	 *	@description Controls whether garbage collection occurs when an
	 *		allocation request cannot be immediately satisfied from garbage
	 *		collector memory free lists. If set to true, the garbage collector
	 *		will run if a request cannot be satisfied. The default value is
	 *		true.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see debugLevel, enable, enableIdleCollect, run, workQuota
	 */
	public static Boolean enableDemandCollect;


	/**
	 *	@overview Enable the Garbage Collector
	 *	@description Enables or disables the garbage collector. Setting to
	 *	false will disable the garbage collector. The default value is true.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see debugLevel, enableDemandCollect, enableIdleCollect, run, workQuota
	 */
	public static Boolean enable;


	/**
	 *	@overview Control idle time running of the garbage collector
	 *	@description Controls whether garbage collection occurs when EJ
	 *		is idle. If set to true, and the application has been idle and is
	 *		likely to remain idle for a short time, the garbage collector will
	 *		run. The default value is true.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see debugLevel, enableDemandCollect, enable, run, workQuota
	 */
	public static Boolean enableIdleCollect;


	/**
	 *	@overview Run the garbage collector
	 *	@description Run the garbage collector and reclaim all memory
	 *		allocated to objects and properties that are no longer reachable.
	 *		When objects and properties are freed, any registered destructors
	 *		will be called. The run function will run the garbage collector even
	 *		if the \ref enable property is set to fales.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see debugLevel, enableDemandCollect, enable, enableIdleCollect, 
	 *		workQuota
	 */
	public static void	run();


	/**
	 *	@overview Allocated memory
	 *	@description Memory allocated by EJ for objects and properties. This
	 *		includes memory currently in use and also memory that has been
	 *		freed but is still retained by the garbage collector for future
	 *		use.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see debugLevel, enableDemandCollect, enable, enableIdleCollect, run
	 */
	public static Number workQuota;
}


/**
 *	@overview Memory Allocation Statistics class
 *	@description Singleton class to monitor and report on memory allocation by
 *		the application.
 *	@see System.Debug, System.GC
 */
class System.Memory extends Object
{
	/**
	 *	@overview Print memory stats
	 *	@description Prints EJ memory statistics to the debug log
	 *	@stability Prototype
	 *	@param leakStats If true, printStats will print a leaked memory
	 *		report.
	 *	@library Builtin
	 *	@see allocatedObjectMemory, availableAppMemory, ejRedline, 
	 *		maxEjMemory, peakEjMemory, peakStack, systemRam, usedAppMemory,
	 *		usedEjMemory, usedObjectMemory
	 */
	public static void printStats(Boolean leakStats = 0);


	/**
	 *	@overview Allocated object memory
	 *	@description Memory allocated by EJ for objects and properties. This
	 *		includes memory currently in use and also memory that has been
	 *		freed but is still retained by the garbage collector for future
	 *		use.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see availableAppMemory, ejRedline, 
	 *		maxEjMemory, peakEjMemory, peakStack, printStats, systemRam,
	 *		usedAppMemory, usedEjMemory, usedObjectMemory
	 */
	public static readonly Number allocatedObjectMemory;


	/**
	 *	@overview Available memory for the application
	 *	@description Total memory available for use by the application.
	 *		This is the maximum amount of memory the application may ever
	 *		request from the operating system.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see allocatedObjectMemory, ejRedline, 
	 *		maxEjMemory, peakEjMemory, peakStack, printStats, systemRam,
	 *		usedAppMemory, usedEjMemory, usedObjectMemory
	 */
	public static readonly Number availableAppMemory;


	/**
	 *	@overview Memory redline for EJ
	 *	@description Defined memory limit for EJ. When the redline limit is
	 *		exceeded, a \ref MemoryError exception will be thrown and EJ will
	 *		go into graceful degrade mode. Subsequent memory allocations up to
	 *		the \ref maxEjMemory limit will succeed allowing a graceful recovery
	 *		or exit of the application. 
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see allocatedObjectMemory, availableAppMemory, 
	 *		maxEjMemory, peakEjMemory, printStats, systemRam, usedAppMemory,
	 *		usedEjMemory, usedObjectMemory
	 */
	public static Number ejRedline;


	/**
	 *	@overview Maximum memory
	 *	@description Maximum memory that EJ may use. This property defines the
	 *		upper limit for memory usage by EJ and the MPR layer. If this 
	 *		limit is reached, subsequent memory allocations will fail and 
	 *		a \ref MemoryError exception will be thrown. Setting it to zero
	 *		will allow unlimited memory allocations up to the system imposed
	 *		maximum.
	 *		\n\n
	 *		If \ref ejRedline is defined, a \ref MemoryError exception
	 *		will be thrown when the \ref ejRedline is exceeded.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see allocatedObjectMemory, availableAppMemory, ejRedline, 
	 *		peakEjMemory, peakStack, printStats, systemRam, usedAppMemory,
	 *		usedEjMemory, usedObjectMemory
	 */
	public static Number maxEjMemory;


	/**
	 *	@overview Peak memory allocated by EJ
	 *	@description Peak memory ever allocated by the application. It includes 
	 *		all memory allocated by EJ and by the cross-platform layer.
	 *		This statistic is the maximum value ever attained by \ref
	 *		usedEjMemory.  
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see allocatedObjectMemory, availableAppMemory, ejRedline, 
	 *		maxEjMemory, peakStack, printStats, systemRam, usedAppMemory,
	 *		usedEjMemory, usedObjectMemory
	public static readonly Number peakEjMemory;


	/**
	 *	@overview Peak application stack size
	 *	@description Peak stack size every used by the application. 
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see allocatedObjectMemory, availableAppMemory, ejRedline, 
	 *		maxEjMemory, peakEjMemory, printStats, systemRam, usedAppMemory,
	 *		usedEjMemory, usedObjectMemory
	 */
	/* public static Number getUsedStack(); */
	public static readonly Number peakStack;



	/**
	 *	@overview System RAM
	 *	@description Total system memory. This is the total amount of RAM
	 *		installed in the system.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see allocatedObjectMemory, availableAppMemory, ejRedline, 
	 *		maxEjMemory, peakEjMemory, peakStack, printStats, usedAppMemory,
	 *		usedEjMemory, usedObjectMemory
	 */
	public static Number systemRam;


	/**
	 *	@overview Used application memory
	 *	@description Amount of memory currently in-use by the application.
	 *		This includes all memory used by EJ and any other parts of the
	 *		application. It is measured by the O/S.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see allocatedObjectMemory, availableAppMemory, ejRedline, 
	 *		maxEjMemory, peakEjMemory, peakStack, printStats, systemRam,
	 *		usedEjMemory, usedObjectMemory
	 */
	/* public static Number getUsedMemory(); */
	public static readonly Number usedAppMemory;


	/**
	 *	@overview Memory allocated by the application
	 *	@description Total memory allocated by the application in the EJ and
	 *		MPR subsystems. It includes all memory allocated by EJ for objects
	 *		and properties and memory allocated by the MPR (cross-platform 
	 *		layer). This statistic includes the \ref allocatedObjectMemory
	 *		value.  
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see allocatedObjectMemory, availableAppMemory, ejRedline, 
	 *		maxEjMemory, peakEjMemory, peakStack, printStats, usedAppMemory,
	 *		usedEjMemory, usedObjectMemory
	 */
	public static readonly Number usedEjMemory;


	/**
	 *	@overview Memory in-use by EJ
	 *	@description Memory currently in-use by EJ for objects and properties. 
	 *		This does not include memory allocated but not in-use. It thus
	 *		represents the current memory requirements for EJ.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see allocatedMemory, debugLevel, demandCollect, enable, idleCollect,
	 *		maxMemory, mprMemory, peakMprMemory, peakStack, printStats, run, 
	 *		workQuota
	 */
	public static readonly Number usedObjectMemory;


}


/**
 *	@overview Debug logging control class
 *	@description Singleton class to control logging of debug trace.
 *	@see System.Debug, print, println, printv, trace
 */
class System.Log extends Object
{
	/**
	 *	@overview Set the debug log filename.
	 *	@description Set the debug log to write to the specified \a path.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see System.Debug.debugMode, System.Debug.breakpoint 
	 */
	public static void setLog(String path);
}


/**
 *	@overview Event class
 *	@description Event information to use with System.EventTarget. EJ includes
 *		a powerful multipoint publish and subscribe event mechanism. 
 *	@see System.EventTarget, System.Listener
 */
class System.Event extends Object
{
	/**
	 *	@overview Constructor for Event
	 *	@description Create a new Event object.
	 *	@param arg Arbitrary object to associate with the event.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see System.Listener, System.EventTarget, timeStamp, type, arg
	 */
	public static Event Event(Object arg);


	/**
	 *	@overview Time the event was created
	 *	@description The Event constructure will automatically set the 
	 *		timeStamp to the current time returned by \ref time. 
	 *	@param arg Arbitrary object to associate with the event.
	 *	@remarks This property will be converted to a Date type in the future. 
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see Event, type, arg
	 */
	public Number timeStamp;


	/**
	 *	@overview Event class type
	 *	@description String name of the Event class. Events are typically
	 *		subclases of the Event class. By examining the \a type properity,
	 *		the type of exception can be determined.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see Event, timeStamp, arg
	 */
	public String type;


	/**
	 *	@overview User argument associated with the Event
	 *	@description When Events are created, the constructor takes an
	 *		arbitrary object reference. This is stored in the \a arg.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see Event, timeStamp
	 */
	public Object arg;
}


/**
 *	@overview Event listener class
 *	@description Event listeners are used to bind callback functions to
 *		event targets. 
 *	@see System.Event, System.EventTarget
 */
class System.Listener extends Object
{
	/**
	 *	@overview Listener constructor
	 *	@description Create a Listener object that can be used with the
	 *		EventTarget.addListener function to subscribe for event
	 *		notifications. The Listener constructor takes a callback
	 *		object and function name as arguments which will be called when an
	 *		event notification is sent. This occurs in response to 
	 *		EventTarget.fire being called.
	 *		\n\n
	 *		The callback signature must have the following form:
	 *		@code
	 *	function callback(System.Listener listener, System.Event event)
	 *	{
	 *	}
	 *		@endcode
	 *		The callback function determines what action to take in response to
	 *		the event by examining event.name, event.timeStamp, and
	 *		other Event specific properties. Typically a subclass of Event is
	 *		provided to callbacks rather than an Event.
	 *	@param obj Object on which to invoke the \a function when event
	 *		notifications are received.
	 *	@param func String containing the function name which will be invoked
	 *		for event notifications.
	 *	@remark In the future, \a func will be changed to be a function
	 *		reference and the \a obj parameter will be deleted.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see System.Event, System.EventTarget, addListener, removeListener, 
	 *		fire
	 */
	public Listener Listener(Object obj = this, String func = "onEvent");
}


/**
 *	@overview Event target class
 *	@description The EventTarget class manages firing events and issuing event
 *		notifications to registered listeners.
 *	@see System.Event, System.Listener
 */
class System.EventTarget extends Object
{
	public static EventTarget EventTarget();

	/**
	 *	@overview Add a listener
	 *	@description Add a listener to the event target. Once added, a
	 *		listener will be subscribed for all event notifications issued to
	 *		the given \a eventName.
	 *	@param eventName Name of events to which to subscribe
	 *	@param listener Listener instance to add. Listeners are created via
	 *		System.Listener.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see fire, removeListener
	 */
	public void addListener(String eventName, System.Listener listener);


	/**
	 *	@overview Fire an event notification
	 *	@description Fires an event notification to all subscribed listeners.
	 *		The Listener callback is called synchronously by the fire function.
	 *	@param eventName Name of events to which to subscribe
	 *	@param event Event class instance to send to the listeners.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see Event, fire, removeListener
	 */
	public void fire(String eventName, System.Event event);


	/**
	 *	@overview Remove a listener
	 *	@description Remove a listener from the event target. The
	 *		removeListener should be given the same eventName and listener
	 *		instance that was supplied to addListner.
	 *	@stability Prototype
	 *	@param eventName Event name used when adding the listener.
	 *	@param listener Listener instance used when adding the listener.
	 *	@library Builtin
	 *	@see addListener, fire
	 */
	public void removeListener(String eventName, System.Listener listener);
}


/**
 *	@overview Timer class
 *	@description Timers manage the execution of scripts at some point in the
 *		future. Timers can be started, stopped and rescheduled. Timers may run
 *		repeatedly or be configured to run only once.
 *		\n\n
 *		Timers are scheduled with a granularity of 1 millisecond, but most
 *		operating systems are not capable of supporting this granularity and
 *		make only best efforts to schedule events at the desired time.
 *	@see System.TimerService, time
 */
class System.Timer extends Object
{
	/**
	 *	@overview Constructor for Timer
	 *	@description Construct a Timer object. The timer is will not be called
	 *		until \ref start is called.
	 *	@param id Unique String ID identifying the timer.
	 *	@param period Time period in milliseconds between invocations of the
	 *		specified \a func.
	 *	@param obj Reference to the callback object in which to invoke the 
	 *		\a func.
	 *	@param func String name of the callback function to invoke when the
	 *		timer is due.
	 *	@remarks The \a obj parameter will be removed in the future and the \a
	 *		func parameter will be converted to a function reference.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see Timer, due, id, period, reschedule, runOnce, start, stop
	 */
	public static Timer(String id, Number period, Object obj, String func);


	/**
	 *	@overview Time when the time is due.
	 *	@description System time when the timer is due. This is computed using
	 *		\ref time as the time in milliseconds after 12:00 AM on the day
	 *		the system was booted.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see Timer, due, id, period, reschedule, runOnce, start, stop
	 */
	public Number due;


	/**
	 *	@overview Enable a timer
	 *	@description Enable a timer to run.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see Timer, due, id, period, reschedule, runOnce, start, stop
	 */
	public Boolean enabled;


	/**
	 *	@overview Unique ID for the timer
	 *	@description The caller of Timer must supply a unique string ID for
	 *		each timer. This ID is used internally to index the timers.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see Timer, due, enabled, period, reschedule, runOnce, start, stop
	 */
	public String id;


	/**
	 *	@overview Period interval between timer invocations
	 *	@description Period of time in milliseconds between timer invocations. 
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see Timer, due, enabled, id, reschedule, runOnce, start, stop
	 */
	public Number period;


	/**
	 *	@overview Run a timer only once.
	 *	@description Timers normally keep running and are called every period
	 *		milliseconds.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see Timer, due, enabled, id, period, reschedule, start, stop
	 */
	public Boolean runOnce;


	/**
	 *	@overview Reschedule a timer
	 *	@description Reschedule a timer by changing its period. This function is
	 *		deprecated. To reschedule a timer, set its period.
	 *	@param period New time in milliseconds between timer invocations.
	 *	@stability Deprecated
	 *	@library Builtin
	 *	@see Timer, due, enabled, id, period, start, stop
	 */
	public void reschedule(Number period);

	/**
	 *	@overview Start a timer
	 *	@description Start a timer running. Once started, the callback
	 *		function will be invoked every \ref period milliseconds.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see Timer, due, id, period, reschedule, script, stop
	 */
	public void start();


	/**
	 *	@overview Stop a timer
	 *	@description Stop a timer running. Once stopped a timer can be
	 *		restarted by calling \ref start.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see Timer, due, id, period, reschedule, script, start
	 */
	public void stop();
}


/**
 *	@overview Timer service
 *	@description The TimerService class manages all timers created in EJ. The
 *		TimerService is an internal singleton class and instantiated by the EJ
 *		runtime. 
 *	@see Timer
 */
class System.TimerService extends Object
{
	public static TimerService TimerService();
	/* Internal to EJS */
}


/**
 *	@overview XML class
 *	@description The XML class is part of the EJ E4X implementation. It
 *		provides the ability to load, parse and save XML documents. When
 *		loaded, the XML nodes are converted into EJ objects and properties for
 *		easy and efficient access.
 *	\n\n
 *		Once an XML document is loaded, or an XML document is created, the
 *		XML tags are mapped to EJ objects and XML attributes are mapped to 
 *		properties with "@" prefixed to their names. For example, the XML
 *		document:
 *	@code
 *		var xmlInput = <user>
 *			<customer id="45">
 *				<name>William Wallace</name>
 *			</customer>
 *		<user>
 *	@endcode
 *		Will map to the EJ objects:
 *	@code
 *		var xml = new XML(xmlInput);
 *		var name = xml.customer.name;
 *		var id = xml.customer.@id;
 *	@endcode
 *		The XML properties are all functions so as to not clash with actual XML
 *		node properties.
 *	@see System.FileSystem
 */
class XML extends Object
{
	/**
	 *	@overview XML Constructor
	 *	@description Create an empty XML object.
	 *	@stability Prototype
	 *	@return Returns an XML node object
	 *	@library Builtin
	 *	@see getList, load, name, save, setText, toString, text, valueOf
	 */
	public static XML XML();


	/**
	 *	@overview XML Constructor
	 *	@description Create an XML object based on an XML literal. 
	 *	@code
	 *		var xml = new XML( <user>Peter</user> );
	 *	@endcode
	 *	@stability Prototype
	 *	@return Returns an XML node object containing the XML literal.
	 *	@remarks This constructor is not yet supported.
	 *	@library Builtin
	 *	@see getList, load, name, save, setText, toString, text, valueOf
	 */
	public static XML XML(XML xmlLiteral);


	/**
	 *	@overview XML Constructor
	 *	@description Create an XML object based an XML string
	 *	@code
	 *		var xml = new XML("<user>Peter</user>");
	 *	@endcode
	 *	@param xmlString String containing XML document to load.
	 *	@stability Prototype
	 *	@return Returns an XML node object containing the XML string literal.
	 *	@library Builtin
	 *	@see XML getList, load, name, save, setText, toString, text, valueOf
	 */
	public static XML XML(String xmlString);


	/**
	 *	@overview XML Constructor
	 *	@description Create an XML object and load the specified XML document
	 *		file.
	 *	@code
	 *		var xml = new XML(fileName);
	 *	@endcode
	 *	@param fileName Filename containing the XML document to load.
	 *	@stability Prototype
	 *	@return Returns an XML node object containing the XML document.
	 *	@library Builtin
	 *	@see XML getList, load, name, save, setText, toString, text, valueOf
	 */
	public static XML XML(String fileName);


	/**
	 *	@overview Get a list of matching XML nodes
	 *	@description Get a list of XML nodes matching a given tag name. The
	 *		tag name may be omitted or set to the empty string in which case
	 *		all nodes will be returned.
	 *	@remarks This function will be removed when the E4X query operators are
	 *		supported.
	 *	@stability Prototype
	 *	@return Array of matching XML nodes.
	 *	@library Builtin
	 *	@see XML, load, name, save, setText, toString, text, valueOf
	 */
	public Array getList(String tagName = "");


	/**
	 *	@overview Load an XML document
	 *	@description The XML document specified by \a fileName is loaded into
	 *		\a this object. The loading overlays any existing properties.
	 *		Typically, the load function should be called on a cleanly
	 *		constructed XML object.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see XML, getList, name, save, setText, toString, text, valueOf
	 */
	public void load(String fileName);


	/**
	 *	@overview Get the XML tag name
	 *	@description Get the XML tag name associated with an XML node.
	 *	@remarks This function will be removed when the E4X convention of simple
	 *		dot access to text data is supported.
	 *	@stability Prototype
	 *	@return Returns a string containing the XML tag name.
	 *	@library Builtin
	 *	@see XML, getList, load, save, setText, toString, text, valueOf
	 */
	public String name();


	/**
	 *	@overview Save an XML document
	 *	@description The XML document is saved to the \a fileName.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see XML, getList, name, setText, toString, text, valueOf
	 */
	public void save(String fileName);


	/**
	 *	@overview Set the text data for an XML node.
	 *	@description Sets the inter-node text for an XML node.
	 *	@remarks This function will be removed when the E4X convention of simple
	 *		dot access to text data is supported.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see XML, getList, name, toString, text, valueOf
	 */
	public String setText();

	/**
	 *	@overview Get the text data for an XML node.
	 *	@description Create an XML object.
	 *	@remarks This function will be removed when the E4X convention of simple
	 *		dot access to text data is supported.
	 *	@stability Prototype
	 *	@return Returns an XML node object
	 *	@library Builtin
	 *	@see XML, getList, load, name, save, setText, toString, text, valueOf
	 */
	public String text();


	/**
	 *	@overview Convert an XML node to a string.
	 *	@description Convert an XML node and its children into a string
	 *		representation.
	 *	@stability Prototype
	 *	@return Returns an XML node object
	 *	@library Builtin
	 *	@see XML, getList, load, name, save, setText, text, valueOf
	 */
	public override String toString();
}


/*
	class XMLList extends Object
	{
	}
*/


/**
 *	@overview File information class
 *	@description The FileInfo class stores information about a files state and
 *		attributes.
 *	@see System.FileSystem
 */
class System.FileInfo extends Object
{
	/**
	 *	@overview When the file was created.
	 *	@description Date and time when the file was created. This an
	 *		operating system dependent date consistent with the units returned
	 *		by the \ref date function.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see getList, load, name, save, setText, toString, text, valueOf
	 */
	public Number created;						// Change to Date


	/**
	 *	@overview File size
	 *	@description Length of the file in bytes.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see getList, load, name, save, setText, toString, text, valueOf
	 */
	public Number length;


	/**
	 *	@overview Is the file a directory
	 *	@description Set to true if the file is a directory.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see getList, load, name, save, setText, toString, text, valueOf
	 */
	public Boolean isDir;
}


/**
 *	@overview File system class
 *	@description The FileSystem provides a suite of static functions for
 *		interacting with the local file system.
 *	@see System.FileInfo, XML
 */
class System.FileSystem extends Object
{
	/**
	 *	@overview Can the file be accessed.
	 *	@description Return true if the specified file exists and can be
	 *		accessed.
	 *	@param path Filename path to examine.
	 *	@return Returns true if the file can be accessed
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see FileSystem, access, basename, copy, dirname, dirList, freeSpace,
	 *		getFileInfo, mkdir, readFile, remove, rename, rmdir, writeFile
	 */
	public static Boolean access(String path);


	/**
	 *	@overview Get the basename of a file
	 *	@description Returns the basename portion of a file name. The base
	 *		name portion is the trailing portion without any directory elements.
	 *	@param path Filename path to examine.
	 *	@return Returns a string containing the base name.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see FileSystem, access, copy, dirname, dirList, freeSpace,
	 *		getFileInfo, mkdir, readFile, remove, rename, rmdir, writeFile
	 */
	public static void basename(String path);


	/**
	 *	@overview Copy a file
	 *	@description Copy a file. If the destination file already exists, the
	 *		old copy will be overwritten as part of the copy operation.
	 *	@stability Prototype
	 *	@param fromPath Original file to copy.
	 *	@param toPath New destination file path name.
	 *	@library Builtin
	 *	@see FileSystem, access, dirname, dirList, freeSpace,
	 *		getFileInfo, mkdir, readFile, remove, rename, rmdir, writeFile
	 */
	public static void copy(String fromPath, String toPath);

	/**
	 *	@overview Get a list of files in a directory
	 *	@description Returns a directory listing of the files in a directory.
	 *		name portion is the trailing portion without any directory elements.
	 *	@param path Filename path to examine.
	 *	@param enumDirs If set to true, then dirList will include
	 *		subdirectories in the returned list of files.
	 *	@return Returns an Array of strings containing the filenames in the 
	 *		directory.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see FileSystem, access, copy, dirname, freeSpace,
	 *		getFileInfo, mkdir, readFile, remove, rename, rmdir, writeFile
	 */
	public static Array dirList(String path, Boolean enumDirs = false);


	/**
	 *	@overview Get the directory name portion of a file
	 *	@description Returns the directory portion of a file name. The dirname
	 *		name portion is the leading portion including all directory 
	 *		elements.
	 *	@stability Prototype
	 *	@param path Filename path to examine.
	 *	@return Returns a string containing the directory name.
	 *	@library Builtin
	 *	@see FileSystem, access, copy, dirList, freeSpace,
	 *		getFileInfo, mkdir, readFile, remove, rename, rmdir, writeFile
	 */
	public static void dirname(String path);


	/**
	 *	@overview Return the free space in the file system
	 *	@description Returns the number of bytes of free space in the 
	 *		file system.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see FileSystem, access, copy, dirname, dirList, 
	 *		getFileInfo, mkdir, readFile, remove, rename, rmdir, writeFile
	 */
	public static readonly Number freeSpace;


	/**
	 *	@overview Get information about a file
	 *	@description Returns a FileInfo object describing the file.
	 *	@stability Prototype
	 *	@param path Filename path to examine.
	 *	@return Returns a FileInfo object.
	 *	@exception Throws IOError exception if the file cannot be accessed.
	 *	@library Builtin
	 *	@see FileSystem, access, copy, dirname, dirList, freeSpace,
	 *		mkdir, readFile, remove, rename, rmdir, writeFile
	 */
	public static FileInfo getFileInfo(String path);


	/**
	 *	@overview Make a new directory
	 *	@description Makes a new directory and all required intervening
	 *		directories. If the directory already exists, the function returns
	 *		without throwing an exception.
	 *	@param path Filename path to examine.
	 *	@exception Throws IOError exception if the directory cannot be made.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see FileSystem, access, copy, dirname, dirList, freeSpace,
	 *		getFileInfo, readFile, remove, rename, rmdir, writeFile
	 */
	public static void mkdir(String path);


	/**
	 *	@overview Read a file
	 *	@description Reads the entire contents of the specified file. As EJ
	 *		strings may contain binary data, readFile will happily read binary
	 *		files.
	 *	@stability Prototype
	 *	@param path Filename path to examine.
	 *	@return Returns a string containing the files data.
	 *	@exception Throws IOError exception if the file cannot be opened or
	 *		cannot be read.
	 *	@library Builtin
	 *	@see FileSystem, access, copy, dirname, dirList, freeSpace,
	 *		getFileInfo, mkdir, remove, rename, rmdir, writeFile
	 */
	public static String readFile(String path);


	/**
	 *	@overview Remove a file
	 *	@description Delete a file from the file system.
	 *	@param path Filename path to examine.
	 *	@exception Throws IOError exception if the file does not exist or
	 *		cannot be removed.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see FileSystem, access, copy, dirname, dirList, freeSpace,
	 *		getFileInfo, mkdir, readFile, rename, rmdir, writeFile
	 */
	public static void remove(String path);


	/**
	 *	@overview Rename a file
	 *	@description Renames a file. If the new file name exists it is
	 *		removed before the rename.
	 *	@param from Old file name.
	 *	@param to New file name.
	 *	@exception Throws IOError exception if the original file does not 
	 *		exist or cannot be renamed.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see FileSystem, access, copy, dirname, dirList, freeSpace,
	 *		getFileInfo, mkdir, readFile, remove, rmdir, writeFile
	 */
	public static void rename(String from, String to);


	/**
	 *	@overview Remove a directory
	 *	@description Removes a directory. 
	 *	@stability Prototype
	 *	@param path Filename path to examine.
	 *	@return Returns a string containing the base name.
	 *	@exception Throws IOError exception if the directory cannot be removed.
	 *	@library Builtin
	 *	@see FileSystem, access, copy, dirname, dirList, freeSpace,
	 *		getFileInfo, mkdir, readFile, remove, rename, writeFile
	 */
	public static void rmdir(String path);


	/**
	 *	@overview Write a file
	 *	@description Writes data to the specified file. As EJ
	 *		strings may contain binary data, writeFile will happily write
	 *		binary strings to files.
	 *	@stability Prototype
	 *	@param path Filename path to write.
	 *	@param data Data to write.
	 *	@exception Throws IOError exception if the file cannot be created or
	 *		cannot be written.
	 *	@library Builtin
	 *	@see FileSystem, access, copy, dirname, dirList, freeSpace,
	 *		getFileInfo, mkdir, remove, rename, rmdir
	 */
	public static void writeFile(String path, String data);
}


/**
 *	@overview Object with properties for all global variables and classes
 *	@description The global array stores all global variables and classes and
 *		provides a consistent way to enumerate and explicitly access global
 *		variables.
 *
 *		You can access global variables by simply using the name of the 
 *		property. E.g.
 *		@code
 *	var x = true;
 *		@endcode
 *		Alternatively, you can use the global qualifier. E.g.
 *		@code
 *	var x = global.true;
 *		@endcode
 *		This is useful if a global variable is hidden by a local variable of
 *		the same name.
 *
 *	@elements The following properties are defined in the global object:
 *		@li @ref Global.false "false" - False boolean constant
 *		@li @ref Global.Infinity "Infinity" - Infinity constant
 *		@li @ref Global.local "local" - Set of local variables
 *		@li @ref Global.global "global" - Set of global variables
 *		@li @ref Global.Nan "Nan" - Not a number constant
 *		@li @ref Global.null "Null" - Null constant
 *		@li @ref Global.system "system" - Instance of the System class
 *		@li @ref Global.true "true" - True boolean constant
 *		@li @ref Global.undefined "undefined" - Undefined variable constant
 *
 *		The global object also contains references via the Global class to all
 *		defined classes.
 */
Global global;


/**
 *	@overview Method callee class
 *	@description The Callee class provides information about the formal
 *		arguments of a function.
 *	@see Arguments, Local
 */
class System.Callee extends Object
{
	/**
	 *	@overview Callee Constructor 
	 *	@description Create a new Callee object. The callee object stores the
	 *	number of parameters in a functions formal declaration.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see length
	 */
	public static Callee Callee();


	/**
	 *	@overview Length of parameters
	 *	@description Number of parameters in a functions formal declaration.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see Callee
	 */
	public Number length;
}


/**
 *	@overview Method arguments class
 *	@description The Arguments class is an array that provides access to the 
 *		actual arguments provided to a function. The array length is set to
 *		the number of arguments and each arguments is stored with an index
 *		equal to its ordinal position in the function call. It also stores a
 *		reference to the \ref Callee object which stores the number of
 *		parameters in the function's formal declartion.
 *	@see Arguments, Local
 */
class System.Arguments extends Array
{
	/**
	 *	@overview Arguments Constructor 
	 *	@description Create a new Arguments object. The Arguments object 
	 *		stores copies of the actual parameters supplied to a function. It
	 *		also stores the number of arguments in the \ref length property.
	 *	number of parameters in a functions formal declaration.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see Callee, length
	 */
	public static Arguments Arguments();


	/**
	 *	@overview Length of parameters
	 *	@description Number of parameters in a functions formal declaration.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see Arguments, Callee
	 */
	public Number length;


	/**
	 *	@overview Callee object
	 *	@description Reference to the callee object for the function.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see Arguments, length
	 */
	public Callee callee;
}


/**
 *	@overview Local variable block class
 *	@description The Local class contains reference to all the local variables
 *		defined in a function. It also provides a reference to the Arguments
 *		object.
 *	@see Arguments, Callee
 */
class System.Local extends Object
{
	/**
	 *	@overview Local constructor
	 *	@description Create a Local object. The local object stores all the
	 *		local variables for a function. 
	 *	@stability Prototype
	 *	@return Returns a Local object.
	 *	@library Builtin
	 *	@see arguments, this
	 */
	public static Local Local();

	/**
	 *	@overview Reference to this object. 
	 *	@description Reference to the object containing the function
	 *		that is executing.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see arguments, Local
	 */
	public Object this;

	/**
	 *	@overview Function Arguments Constructor 
	 *	@description Create a new Arguments object. The Arguments object 
	 *		stores copies of the actual parameters supplied to a function. It
	 *		also stores the number of arguments in the length property.
	 *	number of parameters in a functions formal declaration.
	 *	@stability Prototype
	 *	@library Builtin
	 *	@see Callee, length
	 */
	public Arguments arguments;
}


/**
 *	@overview Store all local variables
 *	@description The local array stores all local variables and provides a 
 *		consistent means to enumerate and explicitly access local variables.
 *	@elements The following elements are defined in the local array:
 *		@li	@ref global - Point to the global variables array.
 *		@li	this - When inside a function, if an object function has been 
 *		called. \a this will be set to point to the object.
 */
System.Local local;

/**
 *	@overview System class instance
 *	@description The system variable is a singleton reference to the System
 *		class instance.
 *	@see System
 */
System system;
