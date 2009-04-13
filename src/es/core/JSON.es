/*
 *	JSON.es -- JSON class
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs {

	use default namespace intrinsic

	final class JSON {

        use default namespace public

		/**
         *  Parse a string JSON representation and return an object equivalent
         *  @param data JSON string data to parse
         *  @param filter The optional filter parameter is a function that can filter and transform the results. It 
         *      receives each of the keys and values, and its return value is used instead of the original value. If 
         *      it returns what it received, then the structure is not modified. If it returns undefined then the 
         *      member is deleted. NOTE: the filter function is not supported
         *  @return An object representing the JSON string.
		 */
		static function parse(data: String, filter: Function = null): Object {
            return deserialize(data)
        }

		/**
         *  Convert an object into a string JSON representation
         *  @param obj Object to stringify
         *  @param replacer an optional parameter that determines how object values are stringified for objects without a 
         *      toJSON method. It can be a function or an array.
         *  @param indent an optional parameter that specifies the indentation of nested structures. If it is omitted, 
         *      the text will be packed without extra whitespace. If it is a number, it will specify the number of spaces 
         *      to indent at each level. If it is a string (such as '\t' or '&nbsp;'), it contains the characters used to 
         *      indent at each level.
         *  @return A JSON string representing the object
		 */
		static function stringify(obj: Object, replacer: Object = null, indent: Number = 0): String {
            return serialize(obj)
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
