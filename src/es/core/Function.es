/*
 *	Function.es -- Function class
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs {

	use default namespace intrinsic

	/**
	 *	The Function type is used to represent closures, function expressions and class methods. It contains a 
	 *	reference to the code to execute, the execution scope and possibly a bound "this" reference.
	 */
	native final class Function {

        use default namespace public

		/**
		 *	Invoke the function on another object.
		 *	@param thisObject The object to set as the "this" object when the function is called.
		 *	@param args Array of actual parameters to the function.
		 *	@return Any object returned as a result of applying the function
		 *	@throws ReferenceError If the function cannot be applied to this object.
		 */
		native function apply(thisObject: Object, args: Array): Object 

		/**
		 *	Invoke the function on another object.
		 *	@param thisObject The object to set as the "this" object when the function is called.
		 *	@param args Array of actual parameters to the function.
		 *	@return Any object returned as a result of applying the function
		 *	@throws ReferenceError If the function cannot be applied to this object.
		 */
		native function call(thisObject: Object, ...args): Object 

		/**
		 *	Return the bound object representing the "this" object. Functions carry both a lexical scoping and 
		 *	the owning "this" object.
		 *	@return An object
		 */
        # FUTURE
		native function get boundThis(): Object

		//	TODO - ecma has more than this
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
