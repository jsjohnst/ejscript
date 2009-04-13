/**
 *	Response.es - Response object for the Ejscript web framework.
 */

module ejs.web {

    /**
     *  HTTP response class. The Http response object stores information about HTTP responses.
     */
	final class Response {

        use default namespace public

		/**
		 *	HTTP response code
		 */
		native var code: Number


		/**
		 *	Response content length. TODO -- Makes no sense as this 
		 */
        # FUTURE
		native var contentLength: Number


		/**
		 *	Cookies to send to the client. These are currently stored in headers[]
		 */
        # FUTURE
		native var cookies: Object


		/**
		 *	Unique response tag - not generated  yet
		 */
        # FUTURE
		native var etag: String


		/**
		 *	Filename for the Script name
		 */
		native var filename: String


		/**
		 *	Reponse headers
		 */
		native var headers: Array


		/**
		 *	Response content mime type
		 */
		native var mimeType: String
	}
}
