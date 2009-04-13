/*
 *	GC.es -- Garbage collector class
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs.sys {

	/**
	 *	Garbage collector control class. Singleton class to control operation of the Ejscript garbage collector.
	 *	@spec ejs-11
	 */
	native class GC {

        use default namespace public

        /*
         *  Get the current heap usage.
         *  @return the amount of heap memory used by ejscript and the portable runtime in bytes
         */
        native static function get allocatedMemory(): Number


		/**
		 *	Test if the garbage collector is enabled. 
         *	@return True if enabled
		 *	the garbage collector. The default value is true.
		 */
		native static function get enabled(): Boolean


		/**
		 *	Enable or disable the garbage collector
         *	@param on Set to true to enable the collector.
		 */
		native static function set enabled(on: Boolean): Void


        /*
         *  Get the maximum amount of heap memory the application can consume. This is the amount of memory used by
         *  ejscript and the portable runtime. It will be less than the total application memory. If the application 
         *  exceeds this amount a MemoryError exception will be thrown and the application will go into a graceful degrate
         *  mode. In this mode, memory will be aggressively conserved and the application should immediately do an 
         *  orderly exit.
         *  @return The maximum permissable amount of heap memory (bytes) that can be consumed by ejscript
         */
		native static function get maxMemory(): Number


        /*
         *  Set the maximum amount of heap memory ejscript and the portable runtime can consume. This is not equal to total 
         *  application memory.
         *  @param limit The maximum permissable amount of heap memory (bytes) that can be consumed by ejscript.
         */
		native static function set maxMemory(limit: Number): Void


        /*
         *  Print memory consumption stats to the console (stdout)
         */
        native static function printStats(): Void


        /*
         *  Get the peak heap usage.
         *  @return the peak amount of heap memory used by ejscript and the portable runtime in bytes
         */
        native static function get peakMemory(): Number


		/**
		 *	Get the quota of work to perform before the GC will be invoked. 
         *	@return The number of work units that will trigger the GC to run. This roughly corresponds to the number
         *	of allocated objects.
		 */
		native static function get workQuota(): Number


		/**
		 *	Set the quota of work to perform before the GC will be invoked. 
         *	@param quota The number of work units that will trigger the GC to run. This roughly corresponds to the number
         *	of allocated objects.
		 */
		native static function set workQuota(quota: Number): Void


		/**
		 *	Run the garbage collector and reclaim memory allocated to objects and properties that are no longer reachable. 
		 *	When objects and properties are freed, any registered destructors will be called. The run function will run 
		 *	the garbage collector even if the @enable property is set to false. 
		 *	@param deep If set to true, will collect from all generations. The default is to collect only the youngest
		 *		geneartion of objects.
		 */
		native static function run(deep: Boolean = flase): void

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
