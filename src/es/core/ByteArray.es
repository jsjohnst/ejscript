/*
 *	ByteArray.es - ByteArray class
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs {

	use strict
	use default namespace intrinsic

	/**
	 *	ByteArrays provide a growable, integer indexed, in-memory store for bytes. ByteArrays are a powerful data 
     *	type that can be used as a simple array to store and encode data as bytes or it can be used as a Stream 
     *	implementing the Stream interface.
     *
     *	When used as a simple byte array, the ByteArray class offers a low level set of methods to insert and 
     *	extract bytes. The index operator [] can be used to access individual bytes and the copyIn and copyOut methods 
     *	can be used to get and put blocks of data. In this mode, the read and write position properties are ignored. 
     *	Accesses to the byte array are from index zero up to the size defined by the length property. When constructed, 
     *	the ByteArray can be designated as growable, in which case the initial size will grow as required to accomodate 
     *	data and the length property will be updated accordingly.
     *
     *  When used as a Stream, the byte array offers various read and write methods which store data at the location 
     *  specified by the write position property and they read data from the read position. The available method 
     *  indicates how much data is available between the read and write position pointers. The flush method will 
     *  reset the pointers to the start of the array. The length property is unchanged in behavior from when used as 
     *  a simple byte array and it specifies the overall storage capacity of the byte array. As numeric values are 
     *  read or written, they will be encoded according to the value of the endian property which can be set to 
     *  either LittleEndian or BigEndian. When used with for/in, ByteArrays will iterate or enumerate over the 
     *  available data between the read and write pointers.
     *
     *  In Stream mode ByteArrays can be configured with input and output callbacks to provide or consume data to other 
     *  streams or components. These callbacks will automatically be invoked as required when the various read/write 
     *  methods are called.
	 */
	native final class ByteArray implements Stream {

        use default namespace public

		/**
		 *	Numeric byte order constants used for the endian property
	 	 */
		static const LittleEndian: Number 	= 0
		static const BigEndian: Number 		= 1


		/**
		 *	Create a new array. This will set the default encoding.
		 *	@param size The initial size of the byte array. If not supplied a system default buffer size will be used.
         *	@param growable Set to true to automatically grow the array as required to fit written data. If growable 
         *	    is false, then some writes may return "short". ie. not be able to accomodate all written data.
		 */
		native function ByteArray(size: Number = -1, growable: Boolean = false)


		/**
		 *	Get the number of bytes that are currently available for reading from the current read position.
		 *	@returns The number of available bytes of data.
		 */
		native function get available(): Number 


		/**
		 *	Close the byte array
		 */
		function close(graceful: Boolean = false): Void
            flush()


		/**
		 *	Compress the array using zlib
		 */
		# FUTURE
		native function compress(): Void


        /**
		 *	Copy data into the array. Data is written at the $destOffset index.
		 *	@param destOffset Index in the destination byte array to copy the data to
		 *	@param src Byte array containing the data elements to copy
		 *	@param srcOffset Location in the source buffer from which to copy the data. Defaults to the start.
		 *	@param count Number of bytes to copy. Set to -1 to read all available data.
		 *	@return the number of bytes written into the array
         */
        native function copyIn(destOffset: Number, src: ByteArray, srcOffset: Number = 0, count: Number = -1): Void


		/**
		 *	Copy data from the array. Data is copied from the $srcOffset pointer.
		 *	@param dest Destination byte array
		 *	@param destOffset Location in the destination array to copy the data. Defaults to the start.
		 *	@param count Number of bytes to read. Set to -1 to read all available data.
		 *	@returns the count of bytes read. Returns 0 on end of file.
		 *	@throws IOError if an I/O error occurs.
		 */
		native function copyOut(srcOffset: Number, dest: ByteArray, destOffset: Number = 0, count: Number = -1): Number


		/**
		 *	Determine if the system is using little endian byte ordering
		 *	@return An endian encoding constant. Either LittleEndian or BigEndian
	 	 */
		native function get endian(): Number


		/**
		 *	Set the system encoding to little or big endian.
		 *	@param value Set to true for little endian encoding or false for big endian.
	 	 */
		native function set endian(value: Number): Void


		/**	
		 *	Flush the the byte array and reset the read and write position pointers. This may invoke the output callback
		 *	to send the data if the output callback is defined.
		 */
		native function flush(): Void


		/**
		 *	Get an iterator for this array to be used by "for (v in array)". This will return array indicies for 
		 *	read data in the array.
		 *	@param deep Follow the prototype chain. Only implemented in ECMA compliance mode..
		 *	@return An iterator object.
		 *	@spec ecma-3
		 */
		override iterator native function get(deep: Boolean = false): Iterator


		/**
		 *	Get an iterator for this array to be used by "for each (v in array)". This will return read data in the array.
		 *	@param deep Follow the prototype chain. Only implemented in ECMA compliance mode..
		 *	@return An iterator object.
		 *	@spec ecma-3
		 */
		override iterator native function getValues(deep: Boolean = false): Iterator


		/**
		 *	Input callback function when read data is required. The input callback should write to the supplied buffer.
		 *	@param callback Function to call to supply read data. The function is called with the following signature:
		 *		function inputCallback(buffer: ByteArray): Void
		 */
		native function set input(value: Function): Void
		//	TODO - bug. Can't have setter without a getter.
		native function get input(): Function


		/**
		 *	Get the length of an array. This is not the amount of read or write data, but is the size of the total array storage.
		 *	@return The size of the byte array.
		 */
		override native function get length(): Number


		/**
		 *	Define an output function to process (output) data. The output callback should read from the supplied buffer.
		 *	@param callback Function to invoke when the byte array is full or flush() is called.
		 *		function outputCallback(buffer: ByteArray): Number
		 */
		native function set output(callback: Function): Void
		//	TODO - bug. Can't have setter without a getter.
		native function get output(): Function


		/**
		 *	Read data from the array into another byte array. Data is read from the current read $position pointer.
		 *	@param buffer Destination byte array
		 *	@param offset Location in the destination buffer to copy the data. Defaults to the write position. If the offset is
         *	    < 0, then the write position will be updated.
		 *	@param count Number of bytes to read. Set to -1 to read all available data.
		 *	@returns the count of bytes read. Returns 0 on end of file.
		 *	@throws IOError if an I/O error occurs.
		 */
		native function read(buffer: ByteArray, offset: Number = -1, count: Number = -1): Number


		/**
		 *	Read a boolean from the array. Data is read from the current read $position pointer.
		 *	@returns a boolean
		 *	@throws IOError if an I/O error occurs or a premature end of file.
		 */
		native function readBoolean(): Boolean


		/**
		 *	Read a byte from the array. Data is read from the current read $position pointer.
		 *	@returns a byte
		 *	@throws IOError if an I/O error occurs or a premature end of file.
		 */
		native function readByte(): Number


		/**
		 *	Read a date from the array or a premature end of file. Data is read from the current read $position pointer.
		 *	@returns a date
		 *	@throws IOError if an I/O error occurs.
		 */
		native function readDate(): Date


		/**
		 *	Read a double from the array. The data will be decoded according to the encoding property.
		 *	Data is read from the current read $position pointer.
		 *	@returns a double
		 *	@throws IOError if an I/O error occurs or a premature end of file.
		 */
		native function readDouble(): Date


		/**
		 *	Read an 32-bit integer from the array. The data will be decoded according to the encoding property.
		 *	Data is read from the current read $position pointer.
		 *	@returns an integer
		 *	@throws IOError if an I/O error occurs or a premature end of file.
		 */
		native function readInteger(): Number


		/**
		 *	Read a 64-bit long from the array.The data will be decoded according to the encoding property.
		 *	Data is read from the current read $position pointer.
		 *	@returns a long
		 *	@throws IOError if an I/O error occurs or a premature end of file.
		 */
		native function readLong(): Number


		/**
		 *	Return the current read position offset
		 *	@return the read offset
		 */
		native function get readPosition(): Number


		/**
		 *	Set the current read position offset
		 *	@param position The new read position
		 */
		native function set readPosition(position: Number): Void


		/**
		 *	Read a 16-bit short integer from the array.The data will be decoded according to the encoding property.
		 *	Data is read from the current read $position pointer.
		 *	@returns a short int
		 *	@throws IOError if an I/O error occurs or a premature end of file.
		 */
		native function readShort(): Number


		/**
		 *	Read a data from the array as a string. Read data from the read position to a string up to the write position,
         *	    but not more than count characters.
		 *	@param count of bytes to read. If -1, convert the data up to the write position.
		 *	@returns a string
		 *	@throws IOError if an I/O error occurs or a premature end of file.
		 */
		native function readString(count: Number = -1): String


		/**
		 *	Read an XML document from the array. Data is read from the current read $position pointer.
		 *	@returns an XML document
		 *	@throws IOError if an I/O error occurs or a premature end of file.
		 */
		native function readXML(): XML


        /**
         *  Reset the read and write position pointers if there is no available data.
         */
        native function reset(): Void


		/**
		 *	Get the number of data bytes that the array can store from the write position till the end of the array.
		 *	@returns The number of data bytes that can be written.
		 */
		native function get room(): Number 


		/**
		 *	Convert the data in the byte array between the read and write positions to a string.
		 *	@return A string
		 *	@spec ecma-3
		 */
		override native function toString(locale: String = null): String 


		/**
		 *	Uncompress the array using zlib
		 */
		# FUTURE
		native function uncompress(): Void


		/**
		 *	Write data to the array. Binary data is written in an optimal, platform dependent binary format. If cross-platform
         *	portability is required, use the BinaryStream to encode the data. Data is written to the current $writePosition 
         *	If the data argument is itself a ByteArray, the available data from the byte array will be copied. NOTE: the 
         *	data byte array will not have its readPosition adjusted.
		 *	@param data Data elements to write
		 *	@return the number of bytes written into the array
		 */
		native function write(...data): Number


		/**
		 *	Write a byte to the array. Data is written to the current write $position pointer which is then incremented.
		 *	@param data Data to write
		 */
		native function writeByte(data: Number): Void


		/**
		 *	Write a short to the array. Data is written to the current write $position pointer which is then incremented.
		 *	@param data Data to write
		 */
		native function writeShort(data: Number): Void


		/**
		 *	Write a double to the array. Data is written to the current write $position pointer which is then incremented.
		 *	@param data Data to write
		 */
		native function writeDouble(data: Number): Void


		/**
		 *	Write a 32-bit integer to the array. Data is written to the current write $position pointer which is then incremented.
		 *	@param data Data to write
		 */
		native function writeInteger(data: Number): Void


		/**
		 *	Write a 64 bit long integer to the array. Data is written to the current write $position pointer which is 
		 *	then incremented.
		 *	@param data Data to write
		 */
		native function writeLong(data: Number): Void


		/**
		 *	Get the current write position offset.
		 *	@return the write position
		 */
		native function get writePosition(): Number


		/**
		 *	Set the current write position offset.
		 *	@param position the new write  position
		 */
		native function set writePosition(position: Number): Void
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
