/*
 *	Type.es -- Type class. Base class for all type objects.
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs {

	use default namespace intrinsic

	/*
		WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
		WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
		WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
		WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING

		Must not define properties and methods of this type. They cannot be inherited by types.
	 */

	/**
	 *    Base class for all type objects.
	 */
	native final class Type {

        use default namespace public

		/**
		 *	Get the prototype object for the type. The prototype object provides the template of instance properties 
		 *	shared by all Objects.
		 */
		# TODO || ECMA
		native function get prototype(): Type


		/**
		 *	Get the base class for this type.
		 *	@spec ejs-11
		 */
		# FUTURE
		native function get baseClass(): Type


		/**
		 *	Mix in a type into a dynamic type or object. This routine blends the functions and properties from 
		 *	a supplied mix-type into the specified target type. Mix-types that are blended in this manner are 
		 *	known as "mixins". Mixins are used to add horizontal functionality to types at run-time. The target 
		 *	type must be declared as dynamic.
		 *	@param type module The module to mix in.
		 *	@return Returns "this".
		 *	@throws StateError if the mixin fails due to the target type not being declared as dynamic.
		 *	@throws TypeError if the mixin fails due to a property clash between the mixin and target types.
		 *	@spec ejs-11
		 */ 
		# FUTURE
		native function mixin(mixType: Type): Object


		/**
		 *	Get the type (class)  name
		 *	@returns the class name
		 *	@spec ejs-11
		 */
		# FUTURE
		native function get name(): String


		/**
		 *	Get the class name.
		 *	Same as name()
		 *	@returns the class name
		 */
		# ECMA || FUTURE
		function getClass(): String {
			return name()
		}


		/**
		 *	Seal a type. Once an type is sealed, further attempts to create or delete properties will fail, or calls 
		 *	to mixin will throw an exception. 
		 *	@spec ejs-11
		 */
		# FUTURE
		override native function seal() : void
	}


	/*
	 *	TODO - should rename class Type into class Class
	 */
	# ECMA || FUTURE
	var Class = Type
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
