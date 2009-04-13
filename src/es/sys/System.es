/*
 *	System.es - System class
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs.sys {

	/**
	 *	System is a utility class providing methods to interact with the operating system.
	 *	@spec ejs-11
	 */
	native class System {

        use default namespace public

        public static const Bufsize: Number = 1024

		/**
		 *	Get the system hostname
		 *	@param fullyQualified If true, return a hostname including a domain portion
		 *	@return A host name string
		 */
		native static function get hostname(fullyQualified: Boolean = true): String


		/**
		 *	Execute a command/program using the default operating system shell.
		 *	@param command Command or program to execute
		 *	@return a text stream connected to the programs standard output.
         *  @throws IOError if the command exits with non-zero status. 
		 */
		native static function run(cmd: String): String

		native static function runx(cmd: String): Void

        # FUTURE
		native static function cmd(cmd: String): Stream
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
