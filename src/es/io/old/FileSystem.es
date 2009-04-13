/*
 *	FileSystem.es -- File system class
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs {

	use default namespace "ejs.io"

	/**
	 *	FileSystem class provides a simple, high level abstraction for a file system. It offers static methods to create, 
	 *	destroy and manage directories and files.
	 *	@spec ejs-11
	 */
	class FileSystem extends Object {

        use default namespace public

		/**
		 *	Copy a file. If the destination file already exists, the old copy will be overwritten as part of the copy operation.
		 *	@param fromPath Original file to copy.
		 *	@param toPath New destination file path name.
		 *	@throws IOError if the copy is not successful.
		 */
		native static function copy(fromPath: String, toPath: String): void
		

		/*
		 *	Create a temporary file. Creates a new, uniquely named temporary file.
		 *	@param directory Directory in which to create the temp file.
		 *	@returns a closed File object after creating an empty temporary file.
		 */
		native static function createTempFile(directory: String = null): File


		/**
		 *	Does a file exist. Return true if the specified file exists and can be accessed.
		 *	@param path Filename path to examine.
		 *	@return True if the file can be accessed
		 */
		native static function exists(path: String): Boolean


		/**
		 *	Return the free space in the file system.
		 *	@return The number of 1M blocks (1024 * 1024 bytes) of free space in the file system.
		 */
		native static function freeSpace(path: String = null): Number
		

		/**
		 *	Get a list of files in a directory. The returned array contains the base file name portion only.
		 *	@param path Directory path to enumerate.
		 *	@param enumDirs If set to true, then dirList will include sub-directories in the returned list of files.
		 *	@return An Array of strings containing the filenames in the directory.
		 *	TODO - good to add ability to do a regexp on the path or a filter function
		 *	TODO - good to add ** to go recursively to any depth
		 */
		native static function getDirs(path: String, enumDirs: Boolean = false): Array 

	
		/**
		 *	Make a new directory. Makes a new directory and all required intervening directories. If the directory 
		 *	already exists, the function returns without throwing an exception.
		 *	@param path Filename path to use.
		 *	@throws IOError if the directory cannot be created.
		 */
		native static function makeDir(path: String, permissions: Number = 0755): void
		

		/**
		 *	Rename a file. If the new file name exists it is removed before the rename.
		 *	@param from Original file name.
		 *	@param to New file name.
		 *	@throws IOError if the original file does not exist or cannot be renamed.
		 */
		native static function rename(fromFile: String, toFile: String): void
		

		/**
		 *	Remove a file from the file system.
		 *	@param path Filename path to delete.
		 *	@param quiet Don't complain if the file does not exist.
		 *	@throws IOError if the file exists and cannot be removed.
		 */
		native static function remove(path: String): void


		/**
		 *	Removes a directory. 
		 *	@param path Filename path to remove.
		 *	@throws IOError if the directory exists and cannot be removed.
		 *	TODO - recursive remove?
		 */
		native static function removeDir(path: String, recursive: Boolean = false): void
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
