/*
 *	Logger.es - Log file control class
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs.sys {

	/**
	 *	Logger objects provide a convenient and consistent method to capture and store logging information from 
	 *	applications. The verbosity and scope of the logging can be changed at start-up time. Logs can be sent
	 *	to various output and storage facilities (console, disk files, etc.) 
	 *
	 *	A logger may have a "parent" logger in order to create hierarchies of loggers for better logging control 
	 *	and granularity. For example, a logger can be created for each class in a package with all such loggers 
	 *	having a single parent. Loggers can send log messages to their parent and inherit their parent's log level. 
	 *	This allows for easier control of verbosity and scope of logging.  
 	 *
	 *	A logger may have a "filter", an arbitrary function, that returns true or false depending on whether a 
	 *	specific message should be logged or not. 
	 *	@spec ejs-11
	 */
    # FUTURE
	namespace BIG_SPACE

    # FUTURE
	class Logger {

        use default namespace public

		/**
		 *	Logging level for inherit level from parent.
		 */
		static const Inherit: Number = -1


		/**
		 *	Logging level for no logging.
		 */
		static const Off: Number = 0


		/**
		 *	Logging level for most serious errors.
		 */
		static const Error: Number = 1


		/**
		 *	Logging level for warnings.
		 */
		static const Warn: Number = 2


		/**
		 *	Logging level for informational messages.
		 */
		static const Info: Number = 3


		/**
		 *	Logging level for configuration output.
		 */
		static const Config: Number = 4


		/**
		 *	Logging level to output all messages.
		 */
		static const All: Number = 5


		/**
		 *	Do not output messages to any device.
		 */
		static const None: Number = 0


		/**
		 *	Output messages to the console.
		 */
		static const Console: Number = 0x1


		/**
		 *	Output messages to a file.
		 */
		static const LogFile: Number = 0x2


		/**
		 *	Output messages to an O/S event log.
		 */
		static const EventLog: Number = 0x4


		/**
		 *	Output messages to a in-memory store.
		 */
		static const MemLog: Number = 0x8


		/**
 		 * 	Logger constructor.
		 *	The Logger constructor can create different types of loggers based on the three (optional) arguments. 
		 *	The logging level can be set in the constructor and also changed at run-time. Where the logger output 
		 *	goes (e.g. console or file) is statically set. A logger may have a parent logger to provide hierarchical 
		 *	mapping of loggers to the code structure.
		 *	@param name Loggers are typically named after the namespace package or class they are associated with.
		 *	@param level Optional enumerated integer specifying the verbosity.
		 *	@param output Optional output device(s) to send messages to.
		 *	@param parent Optional parent logger.
		 *	@example:
		 *		var log = new Logger("name", 5, LogFile)
		 *		log(2, "message")
		 */
		native function Logger(name: String, level: Number = 0, output: Number = LogFile, parent: Logger = null)


		/**
		 *	Get the filter function for a logger.
		 *	@return The filter function.
		 */
		native function get filter(): Function


		/**
		 *	Set the filter function for this logger. The filter function is called with the following signature:
		 *
		 *		function filter(log: Logger, level: Number, msg: String): Boolean
		 *
		 *	@param function The filter function must return true or false.
		 */
		native function set filter(filter: Function): void


		/**
		 *	Get the verbosity setting (level) of this logger.
		 *	@return The level.
		 */
		native function get level(): Number


		/**
		 *	Set the output level of this logger. (And all child loggers who have their logging level set to Inherit.)
		 *	@param level The next logging level (verbosity).
		 */
		native function set level(level: Number): void


		/**
		 *	Get the name of this logger.
		 *	@return The string name.
		 */
		native function get name(): String


		/**
		 *	Set the name for this logger.
		 *	@param name An optional string name.
		 */
		native function set name(name: String): void


		/**
		 *	Get the devices this logger sends messages to.
		 *	@return The different devices OR'd together.
		 */
		native function get output(): Number


		/**
		 *	Set the output devices for this logger.
		 *	@param name Logically OR'd list of devices.
		 */
		native function set output(ouput: Number): void


		/**
		 *	Get the parent of this logger.
		 *	@return The parent logger.
		 */
		native function get parent(): Logger


		/**
		 *	Set the parent logger for this logger.
		 *	@param parent A logger.
		 */
		native function set parent(parent: Logger): void


		/**
		 *	Record a message via a logger. The message level will be compared to the logger setting to determine 
		 *	whether it will be output to the devices or not. Also, if the logger has a filter function set that 
		 *	may filter the message out before logging.
		 *	@param level The level of the message.
		 *	@param msg The string message to log.
		 */
		native function log(level: Number, msg: String): void


		/**
		 *	Convenience method to record a configuration change via a logger.
		 *	@param msg The string message to log.
		 */
		native function config(msg: String): void


		/**
		 *	Convenience method to record an error via a logger.
		 *	@param msg The string message to log.
		 */
		native function error(msg: String): void


		/**
		 *	Convenience method to record an informational message via a logger.
		 *	@param msg The string message to log.
		 */
		native function info(msg: String): void


		/**
		 *	Convenience method to record a warning via a logger.
		 *	@param msg The string message to log.
		 */
		native function warn(msg: String): void
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
