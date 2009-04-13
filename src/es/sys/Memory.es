/*
 *	Memory.es -- Memory statistics
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs.sys {

	/**
	 *	Singleton class to monitor and report on memory allocation by the application for a given interpreter.
	 *	@spec ejs-11
	 */
	native class Memory {

        use default namespace public

		/**
		 *	Memory redline. When the memory redline limit is exceeded, a $MemoryError exception will be thrown and 
		 *	the interpreter will go into graceful degrade mode. Subsequent memory allocations up to the $maxMemory 
		 *	limit will succeed allowing a graceful recovery or exit of the application. 
		 */
        #FUTURE
		native static function get redline(): Number
        #FUTURE
		native static function set redline(value: Number): Void


		/**
		 *	Maximum memory that may be used. This defines the upper limit for memory usage by the application. If 
		 *	this limit is reached, subsequent memory allocations will fail and a $MemoryError exception will be 
		 *	thrown. Setting it to zero will allow unlimited memory allocations up to the system imposed maximum.
		 *	If $redline is defined and non-zero, $MemoryError exception will be thrown when the $redline is exceeded.
		 */
        #FUTURE
		native static function get maxMemory(): Number
        #FUTURE
		native static function set maxMemory(value: Number): Void


		/**
		 *	Available memory for the application. This is the maximum amount of memory the application may ever 
		 *	request from the operating system. This is the maximum number that $consumedMemrory will ever return.
		 */
        #FUTURE
		native static function get availableMemory(): Number


		/**
		 *	Total memory consumed by the application. This includes memory currently in use and also memory that 
		 *	has been freed but is still retained by the garbage collector for future use.
		 */
        #FUTURE
		native static function get consumedMemory(): Number


		/**
		 *	Peak memory ever used by the application. This statistic is the maximum value ever attained by $usedMemory. 
		 */
        #FUTURE
		native static function get peakMemory(): Number
		

		/**
		 *	Peak stack size ever used by the application. 
		 */
        #FUTURE
		native static function get peakStack(): Number
		

		/**
		 *	System RAM. This is the total amount of RAM installed in the system.
		 */
        #FUTURE
		native static function get systemRam(): Number
		

		/**
		 *	Total memory used to host the application. This includes all memory used by the application and 
		 *	the interpreter. It is measured by the O/S.
		 */
        #FUTURE
		native static function get totalMemory(): Number
		

		/**
		 *	Memory currently in-use by the application for objects. This does not include memory allocated but not 
		 *	in-use (see $consumedMemory). It thus represents the current memory requirements.
		 */
        #FUTURE
		native static function get usedMemory(): Number


		/**
		 *	Prints memory statistics to the debug log
		 */
		native static function printStats(): void
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
