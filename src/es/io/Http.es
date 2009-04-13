/**
 *  Http.es -- HTTP client side communications
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs.io {

    // TODO - more doc
    /**
     *  The Http object represents a Hypertext Transfer Protocol version 1.1 client connection. It is used to issue 
     *  HTTP requests and capture responses. It supports the HTTP/1.1 standard including methods for GET, POST, 
     *  PUT, DELETE, OPTIONS, and TRACE. It also supports Keep-Alive and SSL connections. 
     *  @spec ejs-11
     */
    native class Http implements Stream {

        use default namespace public

        /** HTTP status code */     static const Continue           : Number    = 100
        /** HTTP status code */     static const Ok                 : Number    = 200
        /** HTTP status code */     static const Created            : Number    = 201
        /** HTTP status code */     static const Accepted           : Number    = 202
        /** HTTP status code */     static const NotAuthoritative   : Number    = 203
        /** HTTP status code */     static const NoContent          : Number    = 204
        /** HTTP status code */     static const Reset              : Number    = 205
        /** HTTP status code */     static const Partial            : Number    = 206
        /** HTTP status code */     static const MultipleChoice     : Number    = 300
        /** HTTP status code */     static const MovedPermanently   : Number    = 301
        /** HTTP status code */     static const MovedTemporarily   : Number    = 302
        /** HTTP status code */     static const SeeOther           : Number    = 303
        /** HTTP status code */     static const NotModified        : Number    = 304
        /** HTTP status code */     static const UseProxy           : Number    = 305
        /** HTTP status code */     static const BadRequest         : Number    = 400
        /** HTTP status code */     static const Unauthorized       : Number    = 401
        /** HTTP status code */     static const PaymentRequired    : Number    = 402
        /** HTTP status code */     static const Forbidden          : Number    = 403
        /** HTTP status code */     static const NotFound           : Number    = 404
        /** HTTP status code */     static const BadMethod          : Number    = 405
        /** HTTP status code */     static const NotAccepted        : Number    = 406
        /** HTTP status code */     static const ProxyAuth          : Number    = 407
        /** HTTP status code */     static const ClientTimeout      : Number    = 408
        /** HTTP status code */     static const Conflict           : Number    = 409
        /** HTTP status code */     static const Gone               : Number    = 410
        /** HTTP status code */     static const LengthRequired     : Number    = 411
        /** HTTP status code */     static const PrecondFailed      : Number    = 412
        /** HTTP status code */     static const EntityTooLarge     : Number    = 413
        /** HTTP status code */     static const ReqTooLong         : Number    = 414
        /** HTTP status code */     static const UnsupportedType    : Number    = 415
        /** HTTP status code */     static const ServerError        : Number    = 500
        /** HTTP status code */     static const NotImplemented     : Number    = 501
        /** HTTP status code */     static const BadGateway         : Number    = 502
        /** HTTP status code */     static const Unavailable        : Number    = 503
        /** HTTP status code */     static const GatewayTimeout     : Number    = 504
        /** HTTP status code */     static const Version            : Number    = 505

        /**
         *  Create an Http object. The object is initialized with the URI, but no connection is done until one of the
         *  connection methods is called.
         *  @param uri The (optional) URI to initialize with.
         *  @throws IOError if the URI is malformed.
         */
        native function Http(uri: String = null)


        /**
         *  Add a request header. Must be done before the Http request is issued. 
         *  @param key The header keyword for the request, e.g. "accept".
         *  @param value The value to associate with the header, e.g. "yes"
         *  @param overwrite If true, overwrite existing headers of the same key name.
         */
        native function addRequestHeader(key: String, value: String, overwrite: Boolean = true): Void


        /**
         *  Get the number of data bytes that are currently available on this stream for reading.
         *  @returns The number of available bytes.
         */
        native function get available(): Number 


        /**
         *  Define a request callback and put the Http object into async mode. The specified function will be invoked as 
         *  response data is received on request completion and on any errors. After the callback has been called, the
         *  After response data will be discarded. The callback function may call $available to see what data is ready for
         *  reading or may simply call $read to retrieve available data. In this mode, calls to $connect, $get, $post, $del,
         *  $head, $options or $trace  will not block. 
         *  @param cb Callback function to invoke when the HTTP request is complete and some response data is
         *      available to read. The callback is invoked with the signature: 
         *  <pre>
         *      function callback(e: Event): Void
         *  </pre>
         *  Where e.data == http. The even arg may be either a HttpDataEvent Issued when there is response data to read or 
         *  on end of request ($available == 0). The HttpError event will be passed  on any request processing errors. Does 
         *  not include remote server errors.
         */
        native function set callback(cb: Function): Void


        //  TODO - bug require getter
        /** @hide */
        native function get callback(): Function


        /**
         *  Close the connection to the server and free up all associated resources.
         *  @param graceful if true, then close the socket gracefully after writing all pending data.
         */
        native function close(graceful: Boolean = true): Void 


        /**
         *  Issue a HTTP request for the current method and uri. This function will block until the the request completes or 
         *  fails, unless a callback function has been defined. If a callback has been specified, this function will initiate 
         *  the request and return immediately. The HTTP method should be defined via the $method property and uri via the $uri
         *  property.
         *  @throws IOError if the request was cannot be issued to the remote server.
         */
        native function connect(): Void


        /**
         *  Get the name of the file holding the certificate for this HTTP object. This method applies only if secure
         *      communications are in use and if setCertificationFile has been called. 
         *  @return The file name.
         */
        native function get certificateFile(): String


        /**
         *  Set or reset the file name where this Https object holds its certificate. If defined and secure 
         *  communications are used, then the client connection will supply this certification during its 
         *  connection request.
         *  @param certFile The name of the certificate file.
         *  @throws IOError if the file does not exist.
         */
        native function set certificateFile(certFile: String): Void


        /**
         *  Get the response code from a Http message, e.g. 200.
         *  @return The integer value of the response code or -1 if not known.
         */
        native function get code(): Number


        /**
         *  Get the response code message from a Http message, e.g. OK.
         *  @return The string message returned with the HTTP response status line.
         */
        native function get codeString(): String


        /**
         *  Get the value of the content encoding.
         *  @return A string with the content type or null if not known.
         */
        native function get contentEncoding(): String


        /**
         *  Get the response content length.
         *  @return The integer value or -1 if not known.
         */
        native function get contentLength(): Number


        /**
         *  Get the response content type. Call content() to get the resonse content.
         *  @return A string with the content type or null if not known.
         */
        native function get contentType(): String


        /**
         *  Get the value of the content date header field.
         *  @return The date the request was served.
         */
        native function get date(): Date


        /**
         *  Issue a DELETE request for the current uri. This function will block until the the request completes or 
         *  fails, unless a callback function has been defined. If a callback has been specified, this function will initiate 
         *  the request and return immediately.
         *  @param uri The uri to delete. This overrides any previously defined uri for the Http object.
         *  @throws IOError if the request was cannot be issued to the remote server.
         */
        native function del(uri: String = null): Void


        /**
         *  Get the value of the expires header field.
         *  @return The date the content expires
         */
        native function get expires(): Date


        /**
         *  Flush any buffered data
         */
        function flush(): Void {
        }

        /**
         *  Get whether redirects should be automatically followed by this Http object.
         *  @return True if redirects are automatically followed.
         */
        native function get followRedirects(): Boolean


        /**
         *  Eanble or disable following redirects from the connection remove server. Default is true.
         *  @param flag Set to true to follow redirects.
         */
        native function set followRedirects(flag: Boolean): Void


        /**
         *  Issue a POST request with form data the current uri. This function will block until the the request completes or 
         *  fails, unless a callback function has been defined. If a callback has been specified, this function will initiate 
         *  the request and return immediately.
         *  @param uri Optional request uri. If non-null, this overrides any previously defined uri for the Http object.
         *  @param postData Optional object hash of key value pairs to use as the post data. These are www-url-encoded and
         *      the content mime type is set to "application/x-www-form-urlencoded".
         *  @throws IOError if the request was cannot be issued to the remote server.
         */
        native function form(uri: String, postData: Object): Void


        /**
         *  Issue a GET request for the current uri. This function will block until the the request completes or fails, unless 
         *  a callback function has been defined. If a callback has been specified, this function will initiate the request and 
         *  return immediately.
         *  @param uri The uri to delete. This overrides any previously defined uri for the Http object.
         *  @throws IOError if the request was cannot be issued to the remote server.
         */
        native function get(uri: String = null): Void


        /**
         *  Issue a HEAD request for the current uri. This function will block until the the request completes or fails, unless 
         *  a callback function has been defined. If a callback has been specified, this function will initiate the request and 
         *  return immediately.
         *  @param uri The request uri. This overrides any previously defined uri for the Http object.
         *  @throws IOError if the request was cannot be issued to the remote server.
         */
        native function head(uri: String = null): Void


        /**
         *  Get the value of any response header field.
         *  @return The header field value as a string or null if not known.
         */
        native function header(key: String): String


        /**
         *  Get all the response headers
         *  @return An object hash filled with all the headers.
         */
        native function get headers(): Object


        /**
         *  Get whether the connection is utilizing SSL.
         *  @return True if the connection is using SSL.
         */
        native function get isSecure(): Boolean


        /**
         *  Get the filename of the private key for this Https object.
         *  @return The file name.
         */
        native function get keyFile(): String


        /**
         *  Set or reset the filename where this Https object holds its private key.
         *  @param keyFile The name of the key file.
         *  @throws IOError if the file does not exist.
         */
        native function set keyFile(keyFile: String): Void


        /**
         *  Get the value of the response's last modified header field.
         *  @return The number of milliseconds since January 1, 1970 GMT or -1 if not known.
         */
        native function get lastModified(): Date


        /**
         *  Get the request method for this Http object.
         *  @return The request string or null if not set.
         */
        native function get method(): String


        /**
         *  Set or reset the Http object's request method. Default method is GET.
         *  @param name The method name as a string.
         *  @throws IOError if the request is not GET, POST, HEAD, OPTIONS, PUT, DELETE or TRACE.
         */
        native function set method(name: String)


        /**
         *  Issue a POST request for the current uri. This function will block until the the request completes or 
         *  fails, unless a callback function has been defined. If a callback has been specified, this function will initiate 
         *  the request and return immediately. NOTE: the posted data is NOT URL encoded. If you want to post data to a form,
         *  consider using the $form method.
         *  @param uri Optional request uri. If non-null, this overrides any previously defined uri for the Http object.
         *  @param data Optional data to send with the post request. 
         *  @param data Data objects to send with the post request. Data is written raw and is not encoded or converted. 
         *      However, post intelligently handles arrays such that, each element of the array will be written. If posting
         *      to a form, consider the $form method. 
         *  @throws IOError if the request was cannot be issued to the remote server.
         */
        native function post(uri: String, ...data): Void


        /**
         *  Set the post request length. This sets the Content-Length header value and is used when using $write to 
         *  emit the post data.
         *  @param length The length of post data that will be written via writePostData
         */
        native function set postLength(length: Number): Void

        //  TODO BUG - requires setter
        native function get postLength(): Number

                                                       
        /**
         *  Set the post data to send with a post request.
         *  @param items An object hash of key value pairs to use as the post data
         */
        native function set postData(items: Object): Void


        /**
         *  Issue a PUT request for the current uri. This function will block until the the request completes or 
         *  fails, unless a callback function has been defined. If a callback has been specified, this function will initiate 
         *  the request and return immediately.
         *  @param uri The uri to delete. This overrides any previously defined uri for the Http object.
         *  @param data Data objects to write to the request stream. Data is written raw and is not encoded or converted. 
         *      However, put intelligently handles arrays such that, each element of the array will be written. If encoding 
         *      of put data is required, use the BinaryStream filter. 
         *  @param putData Optional object hash of key value pairs to use as the post data.
         *  @throws IOError if the request was cannot be issued to the remote server.
         */
        native function put(uri: String, ...putData): Void


        /**
         *  Read a block of response content data from the connection. This will automatically call $connect if required.
         *  @param buffer Destination byte array for the read data.
         *  @param offset Offset in the byte array to place the data. If offset is not supplied, data is appended at the 
         *      current byte array write position.
         *  @param count Number of bytes to read. 
         *  @returns a count of the bytes actually read. Returns zero if no data is available. Returns -1 when the end of 
         *      response data has been reached.
         *  @throws IOError if an I/O error occurs.
         */
        native function read(buffer: ByteArray, offset: Number = 0, count: Number = -1): Number


        /**
         *  Read the request response as a string. This will automatically call $connect if required.
         *  @param count of bytes to read. Returns the entire response contents if count is -1.
         *  @returns a string of $count characters beginning at the start of the response data.
         *  @throws IOError if an I/O error occurs.
         */
        native function readString(count: Number = -1): String


        /**
         *  Read the request response as an array of lines. This will automatically call $connect if required.
         *  @param count of linese to read. Returns the entire response contents if count is -1.
         *  @returns a string containing count lines of data starting with the first line of response data
         *  @throws IOError if an I/O error occurs.
         */
        native function readLines(count: Number = -1): Array


        /**
         *  Read the request response as an XML document. This will automatically call $connect if required.
         *  @returns the response content as an XML object 
         *  @throws IOError if an I/O error occurs.
         */
        native function readXml(): XML


        /**
         *  Return a stream connected to the request post/put data stream. This will automatically call $connect if required.
         *  @return A stream to write the request post or put data.
         */
        native function get requestStream(): Stream


        /**
         *  Return the response data. This is an alias for $readString(). This will automatically call $connect if required.
         *  @returns the response as a string of characters.
         *  @throws IOError if an I/O error occurs.
         */
        function get response(): String {
            return readString()
        }


        /**
         *  Return a stream connected to the response data. This will automatically call $connect if required.
         *  @return A stream to read the response data.
         */
        native function get responseStream(): Stream


        /**
         *  Set the user credentials to use if the request requires authentication.
         */
        native function setCredentials(username: String, password: String): Void


        /**
         *  Get the request timeout. 
         *  @returns Number of milliseconds for requests to block while attempting the request.
         */
        native function get timeout(): Number


        /**
         *  Set the request timeout.
         *  @param timeout Number of milliseconds to block while attempting requests. -1 means no timeout.
         */
        native function set timeout(timeout: Number): Void


        /**
         *  Issue a HTTP get request for the current uri. This function will block until the the request completes or 
         *  fails, unless a callback function has been defined. If a callback has been specified, this function will initiate 
         *  the request and return immediately.
         *  @param uri Request uri. If not null, this will override any previously defined uri for the Http object. 
         *  @param filename File to upload.
         *  @throws IOError if the request was cannot be issued to the remote server.
         */
        native function upload(uri: String, filename: String): Void


        /**
         *  Get the URI for this Http object.
         *  @return The current uri string.
         */
        native function get uri(): String


        /**
         *  Set or reset the Http object's URI. If the Http object is already connected the connection is first closed.
         *  @param uri The URI as a string.
         *  @throws IOError if the URI is malformed.
         */
        native function set uri(uriString: String): Void


        /**
         *  Write post data to the request stream. The http object must be in async mode by previously defining a $callback.
         *  The write call blocks while writing data. The Http.postLength should normally be set prior to writing any data to 
         *  define the post data Content-length header value. If it is not defined, the content-length header will not be 
         *  defined and the remote server will have to close the underling HTTP connection at the completion of the request. 
         *  This will prevent HTTP keep-alive for subsequent requests. This call will automatically call $connect if required.
         *  @param data Data objects to write to the request stream. Data is written raw and is not encoded or converted. 
         *      However, write intelligently handles arrays such that, each element of the array will be written. If encoding 
         *      of write data is required, use the BinaryStream filter. 
         *  @throws StateError if the Http method is not set to POST.
         *  @throws IOError if more post data is written than specified via the setPostDataLength function.
         */
        native function write(...data): Void
    }


    /**
     *  Data event issued to the callback function.
     */
    class HttpDataEvent extends Event {
    }


    /**
     *  Error event issued to the callback function if any errors occur during an Http request.
     */
    class HttpErrorEvent extends Event {
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
