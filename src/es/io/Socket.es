/*
 *  Socket.es -- Socket I/O class
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs.io {

    //  TODO - more doc
    //  TODO - compare with other Socket APIs
   
    /**
     *  Client and server side TCP/IP support for IPv4 and IPv6 communications. This class supports broadcast, datagram and
     *  byte-stream socket sevices for both client and server endpoints. Aynchronous and asynchronous operation is supported.
     *  @spec ejs-11
     */
    # FUTURE
    class Socket implements Stream {

        use default namespace public

        /**
         *  Packet modes
         */
        static const Stream:    Number = 1
        static const Datagram:  Number = 2
        static const Broadcast: Number = 3


        //  TODO - better to use Events?
        /**
         *  Callback function kind
         */
        static const Accept:    Number = 1
        static const Close:     Number = 2
        static const Read:      Number = 3
        static const Write:     Number = 4


        /**
         *  Create a socket object
         */
        native function Socket()


        /**
         *  Get the local IP address bound to this socket.
         *  @returns the address in dot notation or empty string if it is not bound.
         */
        native function get address(): String 


        /**
         *  Find the number of bytes currently available on this socket for reading.
         *  @return the number of bytes.
         *  @throws IOError if the number of available bytes cannot be determined.
         */
        native function available(): Number


        /**
         *  Put the socket into async mode and define a completion callback.
         *  The callback is invoked with the signature: 
         *      function callback(e: Event): Void
         *  The following events are passed: SocketAccept, SocketClose, SocketReadData, SocketWriteData
         *  @param callback The callback function to invoke when I/O completes.
         */
        native function set callback(fn: Function): Void


        /**
         *  Close the socket. 
         *  @param graceful If true, the socket is closed gracefully after all write data has been written to the
         *  socket. Unread data is always discarded.
         */
        native function close(graceful: Boolean = 0): Void


//  TODO - should support REUSE
        /**
         *  Establish a client network connection from this socket to the supplied address and port. After a successful call to 
         *  connect() the socket may be used for sending and receiving. Async notification may be received by defining a 
         *  callback. Once connected, a Write callback event will be issued.
         *  @param address An (optional) IP address; if not supplied the system will choose one.
         *  @param port An (optional) integer port number, e.g. 8080; if not supplied the system will choose one.
         *  @throws IOError if teh connection fails. Reasons may include the socket is already bound or the host is 
         *  unknown.
         */
        native function connect(address: String = null, port: Number = 0): Void


        /**
         *  Return the end of file condition of the socket.
         *  @returns TRUE if the remote client has closed the socket.
         */
        native function get eof(): Boolean


        /**
         *  Flush all socket data. Only does something if the socket is secure (SSL). Non secure sockets do not buffer data.
         */
        native function flush(): Void 


        /**
         *  Listen on a socket for client connections. This put the socket into a server role for communcations.
         *  If accept callback has been defined, the listen call will return immediately and client connections
         *  will be notified via the callback. Otherwise, the listen call will block until a client connection
         *  is received.
         *  @param address An (optional) IP address; if not supplied the system will listen on all available interfaces.
         *      But in this case a valid port must be supplied.
         *  @param port An (optional) integer port number, e.g. 8080; if not supplied the system will listen on all
         *      available ports. But in this case, a valid address must be supplied.
         *  @return a new socket connected to the remote client.
         *  @throws ArgError if the specified IP address and port is not valid.
         *  @throws IOError for network errors.
         */
        native function listen(address: String = "", port: Number = 0): Socket


        /**
         *  Return the packet mode of the socket.
         *  @returns the curent packet mode defined via setPacketMode. Valid modes are: Broadcast, Datagram or Stream.
         */
        native function get mode(): Number


        /**
         *  Set the packet mode for the socket.
         *  @param mode Set the socket packet mode to either STREAM, DATAGRAM or BROADCAST
         *  @throws ArgError for an invalid packet mode
         */
        native function set mode(packetMode: Number)


        /**
         *  Get the port bound to this socket.
         *  @return The port number or 0 if it is not bound.
         */
        native function get port(): Number 


        /**
         *  Read data from the socket. If in async mode, the call will return whatever data is immediately available.
         *  If no data is available, then the callback defined via $setCallback will be invoked when more read data
         *  is available. If in sync mode, the call blocks until some data is available.
         *  @param buffer Destination byte array for the read data.
         *  @param offset Offset in the byte array to place the data.
         *  @param count Number of bytes to read. 
         *  @returns a count of the bytes actually read.
         *  @throws IOError if an I/O error occurs.
         */
        native function read(buffer: ByteArray, offset: Number, count: Number): Number


        /**
         *  Get the remote address bound to this socket.
         *  @return The address in dot notation or empty string if it is not bound.
         */
        native function get remoteAddress(): String 


        /**
         *  Get the number of bytes that the write side of the socket can absorb before blocking.
         *  @returns The number of bytes of space.
         */
        native function get room(): Number 


        /**
         *  Set an I/O timeout for the read and write methods.
         *  @param timeout Time in milliseconds to wait.
         */
        native function set timeout(timeout: Number): Void

        //  TODO - bug must have getter
        native function get timeout(): Number

        /*
         *  Write data to the socket. If in sync mode, the write call blocks until the underlying socket has absorbed all 
         *  the data. If in async-mode, the call accepts what ever data can be accepted immediately and returns a count of 
         *  the bytes written. If this count is less than the length of $data, then the callback defined via setCallback 
         *  will be invoked later when all the data has been written to the underlying socket.
         *
         *	Binary data is written in an optimal, platform dependent binary format. If cross-platform portability is required, 
         *	use the BinaryStream to encode the data. Data is written to the current $writePosition If the data argument is 
         *	a ByteArray, the available data from the byte array will be copied. NOTE: the data byte array will not have its 
         *	readPosition adjusted.
         *  @param data Data to write. 
         *  @returns The total number of elements that were written.
         *  @throws IOError if there is an I/O error.
         */
        native function write(...data): Void


        /*
         *  TODO
         *      Input stream
         *      output stream
         */
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
