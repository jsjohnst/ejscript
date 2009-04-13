/*
 *	Namespace.es -- Namespace class
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 *
 *	NOTE: this is only partially implemented.
 */

module ejs {

	use default namespace intrinsic

	/**
	 *	Namespaces are used to qualify names into discrete spaces
	 */
	native final class Namespace {

        use default namespace public

		/**
		 *	Name of the namespace
		 */
		native var name: String

		/**
		 *	Unique URI defining the string used to qualify names which use the namespace
		 */
		native var uri: String

		/*
		 *	Constructor. Valid forms:
		 *		Namespace()
		 *		Namespace(uriValue: String)
		 *		Namespace(prefixValue: String, uriValue: String)
         *   native function Namespace(...args);
		 */
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
