/**
 *	Host.es - Host class for the Ejscript web framework
 */

module ejs.web {

	/**
	 *	Web server host information. The server array stores information that typically does not vary from request to request. 
	 *	For a given virtual server, these data items will be constant across all requests.
	 */
	final class Host {

        use default namespace public


		/**
		 *	Home directory for the web documents
		 */
		native var documentRoot: String


		/**
		 *	Fully qualified name of the server. Should be of the form (http://name[:port])
		 */
		native var name: String


		/**
		 *	Host protocol (http or https)
		 */
		native var protocol: String


		/**
		 *	Set if the host is a virtual host
		 */	
		native var isVirtualHost: Boolean


		/**
		 *	Set if the host is a named virtual host
		 */
		native var isNamedVirtualHost: Boolean


		/**
		 *	Server software description
		 */
		native var software: String


        /**
         *  Log errors to the application log
         */
        native var logErrors: Boolean
	}
}
