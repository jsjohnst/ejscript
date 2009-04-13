/*
 *  File.es -- File I/O class. Do file I/O and manage files.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs.io {

    /**
     *  The File class provides a foundation of I/O services to interact with physical files.
     *  Each File object represents a single file, a named path to data stored in non-volatile memory. A File object 
     *  provides methods for creating, opening, reading, writing and deleting a file, and for accessing and modifying 
     *  information about the file.
     *  @spec ejs-11
     */
    native class File implements Stream {

        use default namespace public

        /**
         *  File access mode representing a Closed file.
         */
        static const Closed: Number     = 0x0

        /**
         *  File access mode representing an Opened file.
         */
        static const Open: Number       = 0x1

        /**
         *  File access mode representing an opened file for read access.
         */
        static const Read: Number       = 0x2

        /**
         *  File access mode representing an opened file for write access
         */
        static const Write: Number      = 0x4   

        /**
         *  File access mode representing a file opened for append write access.
         */
        static const Append: Number     = 0x8

        /**
         *  File access mode where the file will be re-created when opened.
         */
        static const Create: Number     = 0x10

        /**
         *  File access mode where the file will be truncated when opened.
         */
        static const Truncate: Number   = 0x20


        /**
         *  Create a new File object and set the file object's path.
         *  @param path the name of the file to associate with this file object.
         */
        native function File(path: String)


		//	TODO - need isAbsolute and isRelative functions
        /**
         *  Return an absolute path name for the file.
         *  @return a string containing an absolute file name relative to the file system's root directory. 
         *  The file name is canonicalized such that multiple directory separators and ".." segments are removed.
         *  @see relativePath
         */
        native function get absolutePath(): String

        /**
         *  Get the base name of a file. Returns the base name portion of a file name. The base name portion is the 
         *  trailing portion without any directory elements.
         *  @return A string containing the base name portion of the file name.
         */
        native function get basename(): String

        /**
         *  Close the input stream and free up all associated resources.
         *  @param graceful if true, then close the file gracefully after writing all pending data.
         */
        native function close(graceful: Boolean = true): void 

        /**
         *  Copy a file. If the destination file already exists, the old copy will be overwritten as part of the copy 
		 *		operation.
         *  @param toPath New destination file path name.
         *  @throws IOError if the copy is not successful.
         */
        native function copy(toPath: String): void
        
        /*
         *  Create a temporary file. Creates a new, uniquely named temporary file.
         *  @param directory Directory in which to create the temp file.
         *  @returns a closed File object after creating an empty temporary file.
         */
        native static function createTempFile(directory: String = null): File

        /**
         *  Get when the file was created.
         *  @return A date object with the date and time or null if the method fails.
         */
        native function get created(): Date 

        /**
         *  Get the directory name portion of a file. The dirname name portion is the leading portion including all 
         *  directory elements and excluding the base name. On some systems, it will include a drive specifier.
         *  @return A string containing the directory name portion of the file name.
         */
        native function get dirname(): String

        /**
         *  Test to see if this file exists.
         *  @return True if the file exists.
         */
        native function get exists(): Boolean 

        /**
         *  Get the file extension portion of the file name.
         *  @return String containing the file extension
         */
        native function get extension(): String 

        /**
         *  Flush the stream and the underlying file data. Will block while flushing. Note: may complete before
         *  the data is actually written to disk.
         */
        native function flush(): void 

        /**
         *  Return the free space in the file system.
         *  @return The number of 1M blocks (1024 * 1024 bytes) of free space in the file system.
         */
        native function freeSpace(path: String = null): Number
        
        /**
         *  Get an iterator for this file to be used by "for (v in files)". Return the seek positions for each byte.
         *  @param deep Follow the prototype chain. Only implemented in ECMA compliance mode..
         *  @return An iterator object.
         *  @spec ejs-11
         */
        override iterator native function get(deep: Boolean = false): Iterator

        //  TODO - good to add ability to do a regexp on the path or a filter function
        //  TODO - good to add ** to go recursively to any depth
        /**
         *  Get a list of files in a directory. The returned array contains the base file name portion only.
         *  @param path Directory path to enumerate.
         *  @param enumDirs If set to true, then dirList will include sub-directories in the returned list of files.
         *  @return An Array of strings containing the filenames in the directory.
         */
        native function getFiles(enumDirs: Boolean = false): Array 

        /**
         *  Get an iterator for this file to be used by "for each (v in obj)". Return each byte of the file in turn.
         *  @param deep Follow the prototype chain. Only implemented in ECMA compliance mode..
         *  @return An iterator object.
         *  @spec ejs-11
         */
        override iterator native function getValues(deep: Boolean = false): Iterator

        /**
         *  Get the file contents as an array of lines. Each line is a string. This is a static method that opens the file, 
         *  reads the contents and closes the file.
         *  @param path the name of the file to read.
         *  @return An array of strings.
         *  @throws IOError if the file cannot be read
         */
        native static function getBytes(path: String): ByteArray 

        //  TODO - missing a way to get lines for an already open file
        /**
         *  Get the file contents as an array of lines. Each line is a string. This is a static method that 
         *  opens the file, reads the contents and closes the file.
         *  @param path the name of the file to read.
         *  @return An array of strings.
         *  @throws IOError if the file cannot be read
         */
        native static function getLines(path: String, encoding: String = App.UTF_8): Array 


        /**
         *  Get the file contents as a string. This is a static method that opens the file, reads the contents and 
         *  closes the file.
         *  @param path the name of the file to read.
         *  @return A string.
         *  @throws IOError if the file cannot be read
         */
        native static function getString(path: String, encoding: String = App.UTF_8): String 


        /**
         *  Get the file contents as an XML object.  This is a static method that opens the file, reads the contents 
         *  and closes the file.
         *  @param path the name of the file to read.
         *  @return An XML object
         *  @throws IOError if the file cannot be read
         */
        native static function getXML(path: String): XML 


        /**
         *  Determine if the file path has a drive spec (C:) in it's name. Only relevant on Windows like systems.
         *  @return True if the file path has a drive spec
         */
        native function get hasDriveSpec(): Boolean 


        /**
         *  Determine if the file name is a directory
         *  @return true if the file is a directory
         */
        native function get isDir(): Boolean


        /**
         *  Determine if the file is currently open for reading or writing
         *  @return true if the file is currently open via @open or @create
         */
        native function get isOpen(): Boolean


        /**
         *  Determine if the file name is a regular file
         *  @return true if the file is a regular file and not a directory
         */
        native function get isRegular(): Boolean


        /**
         *  Get when the file was last accessed.
         *  @return A date object with the date and time or null if the method fails.
         */
        native function get lastAccess(): Date 


        /**
         *  Get the length of the file associated with this File object.
         *  @return The number of bytes in the file or -1 if length determination failed.
         */
        override native function get length(): Number 


        /**
         *  Set the length of the file associated with this File object.
         *  @param value the new length of the file
         *  @throws IOError if the file's length cannot be changed
         */
        # FUTURE
        intrinsic native function set length(value: Number): Void 


        /**
         *  Make a new directory. Makes a new directory and all required intervening directories. If the directory 
         *  already exists, the function returns without throwing an exception.
         *  @param path Filename path to use.
         *  @throws IOError if the directory cannot be created.
         */
        native function makeDir(permissions: Number = 0755): void
        

        /**
         *  Get the file access mode.
         *  @return file access mode with values ored from: Read, Write, Append, Create, Open, Truncate 
         */ 
        native function get mode(): Number


        /**
         *  Get when the file was created or last modified.
         *  @return A date object with the date and time or null if the method fails.
         */
        native function get modified(): Date 


        /**
         *  Get the name of the file associated with this File object.
         *  @return The name of the file or null if there is no associated file.
         */
        native function get name(): String 


        //  TODO - this needs a path arg to define the file system
        /**
         *  Return the new line characters
         *  @return the new line delimiting characters Usually "\n" or "\r\n".
         */
        native static function get newline(): String 


        /**
         *  Set the new line characters
         *  @param terminator the new line termination characters Usually "\n" or "\r\n"
         */
        native static function set newline(terminator: String): Void


        /**
         *  Open a file using the current file name. This method requires a file instance.
         *  @param name The (optional) name of the file to create.
         *  @param mode optional file access mode with values ored from: Read, Write, Append, Create, Open, Truncate.
         *  Defaults to Read.
         *  @param permissions optional permissions. Defaults to the App.permissions.
         *  @throws IOError if the path or file cannot be created.
         */
        native function open(mode: Number = Read, permissions: Number = 0644): void


        /**
         *  Open a file and return a Stream object. This is a static method.
         *  @param filename The name of the file to create.
         *  @param mode optional file access mode with values ored from: Read, Write, Append, Create, Open, Truncate. Defaults 
         *  to Read.
         *  @param permissions optional permissions. Defaults to the App.permissions.
         *  @return a File object which implements the Stream interface.
         *  @throws IOError if the path or file cannot be created.
         */
        static function openFileStream(filename: String, mode: Number = Read, permissions: Number = 0644): File {
            var file: File

            file = new File(filename)
            file.open(mode, permissions)
            return file
        }


        /**
         *  Open a file and return a TextStream object. This is a static method.
         *  @param filename The name of the file to create.
         *  @param mode optional file access mode with values ored from: Read, Write, Append, Create, Open, Truncate. 
         *      Defaults to Read.
         *  @param permissions optional permissions. Defaults to the App.permissions.
         *  @return a TextStream object which implements the Stream interface.
         *  @throws IOError if the path or file cannot be created.
         */
        static function openTextStream(filename: String, mode: Number = Read, permissions: Number = 0644): TextStream {
            var file: File = new File(filename)
            file.open(mode, permissions)
            return new TextStream(file)
        }


        /**
         *  Open a file and return a BinaryStream object. This is a static method.
         *  @param filename The name of the file to create.
         *  @param mode optional file access mode with values ored from: Read, Write, Append, Create, Open, Truncate. 
         *      Defaults to Read.
         *  @param permissions optional permissions. Defaults to the App.permissions.
         *  @return a BinaryStream object which implements the Stream interface.
         *  @throws IOError if the path or file cannot be created.
         */
        static function openBinaryStream(filename: String, mode: Number = Read, permissions: Number = 0644): BinaryStream {
            var file: File = new File(filename)
            file.open(mode, permissions)
            return new BinaryStream(file)
        }


        //  TODO - this needs a path arg to define the file system
        /**
         *  Return the path segment delimiter
         *  @return the path segment delimiter. Usually "/" or "\\"
         */
        native static function get pathDelimiter(): String 


        //  TODO - remove this API. Should be defined internally by the file system.
        /**
         *  Set the path segment delimiter
         *  @param delim the new path segment delimiter. Usually "/" or "\\"
         */
        native static function set pathDelimiter(delim: String): Void 


        /**
         *  Get the parent directory of the absolute path of the file.
         *  @return the parent directory
         */
        native function get parent(): String


        /**
         *  Get the file security permissions.
         *  @return the file permission mask. This is a POSIX file creation mask.
         */
        native function get permissions(): Number


        /**
         *  Set the file security permissions. 
         *  @return the file permission mask. This is a POSIX file creation mask.
         */
        native function set permissions(mask: Number): void


        /**
         *  Get the current I/O position in the file.
         *  @returns the current read / write position in the file.
         *  @throws IOError if the seek failed.
         */
        native function get position(): Number


        /**
         *  Seek to a new location in the file and set the File marker to a new read/write position.
         *  @param number Location in the file to seek to. Set the position to zero to reset the position to the beginning 
         *  of the file. Set the position to a negative number to seek relative to the end of the file (-1 positions 
         *  at the end of file).
         *  @throws IOError if the seek failed.
         */
        native function set position(value: Number): void


//  TODO - having permissings in the middle doesn't work well
        /**
         *  Put the file contents.  This is a static method that opens the file, writes the contents and closes the file.
         *  @param path the name of the file to write.
         *  @param data to write to the file.
         *  @param permissions optional permissions. Defaults to the App.permissions.
         *  @return An XML object
         *  @throws IOError if the file cannot be written
         */
        native static function put(path: String, permissions: Number, ...args): void 


        /**
         *  Return a relative path name for the file.
         *  @return a string containing a file path name relative to the application's current working directory..
         */
        native function get relativePath()


        /**
         *  Read a block of data from a file into a byte array. This will advance the read position.
         *  @param buffer Destination byte array for the read data.
         *  @param offset Offset in the byte array to place the data.
         *  @param count Number of bytes to read. 
         *  @return A count of the bytes actually read.
         *  @throws IOError if the file could not be read.
         */
        native function read(buffer: ByteArray, offset: Number = 0, count: Number = -1): Number


        /**
         *  Read data bytes from a file and return a byte array containing the data.
         *  @param count Number of bytes to read. 
         *  @return A byte array containing the read data
         *  @throws IOError if the file could not be read.
         */
        native function readBytes(count: Number): ByteArray

      //    TODO - need read Lines
        //  TODO - need readString();

        /**
         *  Delete the file associated with the File object. If the file is opened, it is first closed.
         *  @throws IOError if the file exists and could not be deleted.
         */
        native function remove(): void 


        //  TODO - recursive remove?
        /**
         *  Removes a directory. 
         *  @param path Filename path to remove.
         *  @throws IOError if the directory exists and cannot be removed.
         */
        native function removeDir(recursive: Boolean = false): void


        /**
         *  Rename a file. If the new file name exists it is removed before the rename.
         *  @param to New file name.
         *  @throws IOError if the original file does not exist or cannot be renamed.
         */
        native function rename(toFile: String): void
        

        /**
         *  Put the file stream into async mode and define a completion callback. When in async mode, I/O will be 
         *  non-blocking and the callback function will be invoked when the there is read data available or when 
         *  the write side can accept more data.
         *  @param callback Callback function to invoke when the pending I/O is complete. The callback is invoked 
         *  with the signature: function callback(e: Event): void.  Where e.data == stream.
         */
        native function setCallback(callback: Function): void


        /**
         *  Reduce the size of the file. 
         *  @param size The new maximum size of the file.If the truncate argument is greater than or equal to the 
         *  current file size nothing happens.
         *  @throws IOError if the truncate failed.
         */
        # FUTURE
        native function truncate(size: Number): void


        /**
         *  Return an absolute unix style path name for the file. Used on Windows when using cygwin to return
         *  a cygwin styled path without drive specifiers.
         *  @returns a string containing an absolute file name relative to the cygwin file system's root directory. The file 
         *  name is canonicalized such that drive specifiers, multiple directory separators and ".." segments are removed.
         *  @see absolutePath, relativePath
         */
        native function get unixPath(): String


        /**
         *  Write data to the file. If the stream is in sync mode, the write call blocks until the underlying stream or 
         *  endpoint absorbes all the data. If in async-mode, the call accepts whatever data can be accepted immediately 
         *  and returns a count of the elements that have been written.
         *  @param items The data argument can be ByteArrays, strings or Numbers. All other types will call serialize
         *  first before writing. Note that numbers will not be written in a cross platform manner. If that is required, use
         *  the BinaryStream class to write Numbers.
         *  @returns the number of bytes written.  
         *  @throws IOError if the file could not be written.
         */
        native function write(...items): Number
    }
}

/*
 *  @copy   default
 *  
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *  
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire 
 *  a commercial license from Embedthis Software. You agree to be fully bound 
 *  by the terms of either license. Consult the LICENSE.TXT distributed with 
 *  this software for full details.
 *  
 *  This software is open source; you can redistribute it and/or modify it 
 *  under the terms of the GNU General Public License as published by the 
 *  Free Software Foundation; either version 2 of the License, or (at your 
 *  option) any later version. See the GNU General Public License for more 
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *  
 *  This program is distributed WITHOUT ANY WARRANTY; without even the 
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *  
 *  This GPL license does NOT permit incorporating this software into 
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses 
 *  for this software and support services are available from Embedthis 
 *  Software at http://www.embedthis.com 
 *  
 *  @end
 */
