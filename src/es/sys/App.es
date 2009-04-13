/*
 *	App.es -- Application configuration and control. (Really controlling the interpreter's environment)
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs.sys {

    //  TODO - confusion with the Application class
	/**
	 *	Application configuration class. This is a singleton class which exposes methods to interrogate and control
     *	the applications environment.
	 *	@spec ejs-11
	 */
	native class App {

        use default namespace public

		//	TODO - what about other supported encodings? These should be PascalCase
		static const UTF_8: Number = 1
		static const UTF_16: Number = 2

		/**
		 *	Application command line arguments.
         *	@returns an array containing each of the arguments. If the ejs command is invoked as "ejs script arg1 arg2", then
         *	args[0] will be "script", args[1] will be "arg1" etc.
		 */
		native static function get args(): Array


		//	TODO - change name to exeDir. Confusion with workingDir
		/**
		 *	Return the directory containing the application executable
		 *	@return a string containing the directory path for the application executable
		 */
		native static function get dir(): String


		/**
		 *	Get an environment variable.
		 *	@param name The name of the environment variable to retrieve.
		 *	@return The value of the environment variable or null if not found.
		 */
		native static function getEnv(name: String): String
		

		/**
		 *	Set the standard error stream.
		 *	@param stream The output stream.
		 */
        # FUTURE
		native static function set errorStream(stream: Stream): void
		

		/**
		 *	Gracefully stop the program and exit the interpreter.
		 *	@param status The optional exit code to provide the environment.
		 */
		native static function exit(status: Number = 0): void


		/**
		 *	Set the standard input stream.
		 *	@param stream The input stream.
		 */
        # FUTURE
		native static function set inputStream(stream: Stream): void
		

		//	TODO - need to be able to test what modules are loaded and get a list of loaded modules. Perhaps a Module class.
		/**
		 *	Load an Ejscript module into the interpreter.
		 *	@param The name of the module.
		 *	@return False if could not find the module or it was already loaded or some other error, true otherwise.
		 */
        # FUTURE
		native static function loadModule(path: String): Boolean


		/**	
		 *	Get the current language locale for this application
		 */
        # FUTURE
		native static function get locale(): String


		//	TODO - move to a Locale class
		/**	
		 *	Set the current language locale
		 */
        # FUTURE
		native static function set locale(locale: String): void


		/**
		 *	Application name. 
         *	@returns a single word, lower case name for the application.
		 */
		static function get name(): String {
			return Config.Product
		}


		//	TODO need a better name than noexit, TODO could add a max delay option.
		/**
		 *	Control whether an application will exit when global scripts have completed. Setting this to true will cause
		 *	the application to continue servicing events until the $exit method is explicitly called. The default application 
		 *	setting of noexit is false.
		 *	@param exit If true, the application will exit when the last script completes.
		 */
		native static function noexit(exit: Boolean = true): void


		/**
		 *	Get the default permissions to use when creating files
		 */
        # FUTURE
		native static function get permissions(): Number


		/**
		 *	Set the default permissions to use when creating files
		 *	@param value New umask to use for creating new files
		 */
        # FUTURE
		native static function set permissions(value: Number = 0664): void


		/**
		 *	Set the standard output stream.
		 *	@param stream The output stream.
		 */
        # FUTURE
		native static function set outputStream(stream: Stream): void
		

		/**
		 *	Service events
         *	@param count Count of events to service. Defaults to unlimited.
         *	@param timeout Timeout to block waiting for an event in milliseconds before returning. If an event occurs, the
         *	    call returns immediately.
		 */
        native static function serviceEvents(count: Number = -1, timeout: Number = -1): Void


		/**
		 *	Set an environment variable.
		 *	@param env The name of the environment variable to set.
		 *	@param value The new value.
		 *	@return True if the environment variable was successfully set.
		 */
        # FUTURE
		native static function setEnv(name: String, value: String): Boolean


		/**
		 *	Sleep the application for the given number of milliseconds
		 *	@param delay Time in milliseconds to sleep. Set to -1 to sleep forever.
		 */
		native static function sleep(delay: Number = -1): Void


		/**
		 *	Application title name. Multi word, Camel Case name for the application.
         *	@returns the name of the application suitable for printing.
		 */
		static static function get title(): String {
			return Config.Title
		}


		/**
		 *	Application version string. 
         *	@returns a version string of the format Major.Minor.Patch. For example: 1.1.2
		 */
		static static function get version(): String {
			return Config.Version
		}


		/**
		 *	Get the application's Working directory
		 *	@return the path to the working directory
		 */
		native static function get workingDir(): String


		/**
		 *	Set the application's Working directory
		 *	@param The path to the new working directory
		 */
		native static function set workingDir(value: String): Void
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
