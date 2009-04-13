/**
 *	Request.es - Request class for the Ejscript web framework
 */

module ejs.web {

    /**
     *  HTTP request information. The request objects stores parsed information for incoming HTTP requests.
     */
	final class Request {

        use default namespace public

		/**
		 *	Accept header
		 */
		native var accept: String


		/**
		 *	AcceptCharset header
		 */
		native var acceptCharset: String


		/**
		 *	AcceptEncoding header
		 */
		native var acceptEncoding: String


		/**
		 *	Authentication access control list
		 */
		native var authAcl: String


		/**
		 *	Authentication group
		 */
		native var authGroup: String


		/**
		 *	Authentication method if authorization is being used (basic or digest)
		 */
		native var authType: String


		/**
		 *	Authentication user name
		 */
		native var authUser: String


		/**
		 *	Connection header
		 */
		native var connection: String


		/**
		 *	Posted content length (header: Content-Length)
		 */
		native var contentLength: Number


		/**
		 *	Stores Client cookie state information. The cookies object will be created automatically if the Client supplied 
		 *	cookies with the current request. Cookies are used to specify the session state. If sessions are being used, 
		 *	a session cookie will be sent to and from the browser with each request. The elements are user defined.
         *	TODO - would be better if this were a hash of pre-parsed Cookie objects.
		 */
		native var cookies: Object

			
		/**
		 *	Extension portion of the URL after aliasing to a filename.
		 */
		native var extension: String


		/**
		 *	Files uploaded as part of the request. For each uploaded file, an object is created in files. The name of the 
		 *	object is given by the upload field name in the request page form. This is an array of UploadFile objects.
		 */
		native var files: Array


		/**
		 *	Store the request headers. The request array stores all the HTTP request headers that were supplied by 
         *	the client in the current request. 
		 */
		native var headers: Object


		/**
		 *	The host name header
		 */
		native var hostName: String


		/**
		 *	Request method: DELETE, GET, POST, PUT, OPTIONS, TRACE
		 */
		native var method: String


		/**
		 *	Content mime type (header: Content-Type)
		 */
		native var mimeType: String


		/**
		 *	The portion of the path after the script name if extra path processing is being used. See the ExtraPath 
         *	directive.
		 */
		native var pathInfo: String


		/**
		 *	The physical path corresponding to PATH_INFO.
		 */
		native var pathTranslated


		/**
		 *	Pragma header
		 */
		native var pragma: String


		/**
		 *	Decoded Query string (URL query string)
		 */
		native var query: String


		/**
		 *	Raw request URI before decoding.
		 */
		native var originalUri: String


		/**
		 *	Name of the referring URL
		 */
		native var referrer: String


		/**
		 *	The IP address of the Client issuing the request.
		 */
		native var remoteAddress: String


		/**
		 *	The host address of the Client issuing the request
		 */
		native var remoteHost: String


		/**
		 *	Current session ID. Index into the $sessions object
		 */
		native var sessionID: String


		/**
		 *	The decoded request URL portion after stripping the scheme, host, extra path, query and fragments
		 */
		native var url: String


		/**
		 *	Name of the Client browser software set in the HTTP_USER_AGENT header
		 */
		native var userAgent: String
	}
}
