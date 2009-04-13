/**
 *  Controller.es -- Ejscript Controller class as part of the MVC framework. Also contains control helpers for views.
 */

module ejs.web {

    use module ejs.db

    namespace action = "action"                 /* Namespace for all action methods */

    //  TODO - call currentView
    /**
     *  Current view in the web framework
     */
    var view: View

    /**
     *  Web framework controller. Part of the Ejscript web MVC framework.
     */
    class Controller {

        /*
         *  Define properties and functions (by default) in the ejs.web namespace so that user controller variables 
         *  don't clash.
         */
        use default namespace "ejs.web"


        //  TODO - why are these public?
        /**
         *  Name of the action being run
         */
        public var actionName:  String 
        public var originalActionName:  String 

        /**
         *  Stores application global data. The application array provides a means to store persistent information 
         *  to be shared across all clients using an application. Objects and variables stored in the application 
         *  array will live until either explicitly deleted or the web server exits. The application array does not 
         *  persist over system reboots. The elements are User defined.
         *  NOTE: Not yet implemented.
         */
        public /* synchronized */ var application: Object

        /**
         *  Base URL for the application. Does not include the URL scheme or host name portion.
         */
        public var appUrl:      String

        /**
         *   Lower case controller name 
         */
        public var controllerName: String

        /**
         *  Web application configuration. This is initialized from the config *.ecf files.
         */
        public var config:      Object

        //  TODO - needs work - names are not same gramatical types
        /**
         *  Flash messages to display on the next screen
         *      "inform"        Informational / postitive feedback (note)
         *      "message"       Neutral feedback (reminders, suggestions...)
         *      "warning"       Negative feedback (Warnings and errors)
         *      "error"         Negative errors (Warnings and errors)
         */
        public var flash:       Object

        /**
         *  Physical home directory of the application
         */
        public var home:        String

        /**
         *  Host object
         */
        public var host:        Host

        /**
         *  Form parameters. 
         */
        public var params:      Object

        /**
         *  The request object stores details of the incoming client's request
         */
        public var request:     Request

        /**
         *  The response object stores details of the response going back to the client.
         */
        public var response:    Response

        /**
         *  Stores session state information. The session array will be created automatically if SessionAutoCreate 
         *  is defined or if a session is started via the useSession() or createSession() functions. Sessions are 
         *  shared among requests that come from a single client. This may mean that multiple requests access the 
         *  same session concurrently. Ejscript ensures that such accesses are serialized. The elements are user defined.
         */
        public /* synchronized */ var session: Object

        private var isApp:      Boolean
        private var rendered:   Boolean
        private var redirected: Boolean
        private var events:     Dispatcher

        private var _afterFilters: Array
        private var _beforeFilters: Array
        private var _wrapFilters: Array


        /**
         *  Controller initialization. This is specially hand-crafted by the hosting web server so that it runs
         *  before the sub-classing constructors.
         *  @param appDir Set to the top level directory containing the application
         *  @param appUrl URL that points to the application
         *  @param session Session state object
         *  @param host Host object
         *  @param request Request object
         *  @param response Response object
         */
        function initialize(isApp: Boolean, appDir: String, appUrl: String, session: Session, host: Host, request: Request, 
                response: Response): Void {

            this.isApp = isApp
            this.home = appDir
            this.appUrl = appUrl
            this.session = session
            this.host = host
            this.request = request
            this.response = response

            /*
             *  Load application configuration. 
             */
            if (isApp) {
                config = deserialize("{ " + File.getString(appDir + "/config/config.ecf") + " }")
                config.database = deserialize("{ " + File.getString(appDir + "/config/database.ecf") + " }")
                config.view = deserialize("{ " + File.getString(appDir + "/config/view.ecf") + " }")

                let adapter: String = config.database[config.app.mode].adapter
                let dbname: String = config.database[config.app.mode].database

                if (adapter != "" && dbname != "") {
                    Database.defaultDatabase = new Database(appDir + "/" + dbname)

                    //  TODO BUGGED - must push down into database. This is not doing per model.
                    if (config.database[config.app.mode].trace) {
                        Record.trace(true)
                        Database.trace(true)
                    }
                }
            }

            // events = new Dispatcher
            rendered = false
            redirected = false
            //  TODO - better to do these lazily
            params = new Object
            let name: String = Reflect(this).name
            controllerName = name.trim("Controller")
        }


        /** 
         *  Add a cache-control header to direct the browser to not cache the response.
         */
        native function cache(enable: Boolean = true): Void


        /**
         *  Enable session control. This enables session state management for this request and other requests 
         *  from the browser. If a session has not already been created, this call creates a session and sets 
         *  the @sessionID property in the request object. If a session already exists, this call has no effect. 
         *  A cookie containing a session ID is automatically created and sent to the client on the first response 
         *  after creating the session. If SessionAutoCreate is defined in the configuration file, then sessions 
         *  will automatically be created for every web request and your Ejscript web pages do not need to call 
         *  createSession. Multiple requests may be sent from a client's browser at the same time. Ejscript will 
         *  ensure that accesses to the sesssion object are correctly serialized. 
         *  @param timeout Optional timeout for the session in seconds. If ommitted the default timeout is used.
         */
        native function createSession(timeout: Number): Void


        /**
         *  Destroy a session. This call destroys the session state store that is being used for the current client. If no 
         *  session exists, this call has no effect.
         */
        native function destroySession(): Void


        /**
         *  Discard all prior output
         */
        native function discardOutput(): Void


        function beforeFilter(fn, options: Object = null): Void {
            if (_beforeFilters == null) {
                _beforeFilters = []
            }
            _beforeFilters.append([fn, options])
        }

        function afterFilter(fn, options: Object = null): Void {
            if (_afterFilters == null) {
                _afterFilters = []
            }
            _afterFilters.append([fn, options])
        }

        function wrapFilter(fn, options: Object = null): Void {
            if (_wrapFilters == null) {
                _wrapFilters = []
            }
            _wrapFilters.append([fn, options])
        }

        private function runFilters(filters: Array): Void {
            if (!filters) {
                return
            }
            for each (filter in filters) {
                let fn = filter[0]
                let options = filter[1]
                if (options) {
                    only = options.only
                    if (only) {
                        if (only is String && actionName != only) {
                            continue
                        }
                        if (only is Array && !only.contains(actionName)) {
                            continue
                        }
                    } 
                    except = options.except
                    if (except) {
                        if (except is String && actionName == except) {
                            continue
                        }
                        if (except is Array && except.contains(actionName)) {
                            continue
                        }
                    }
                }
                fn.call(this)
            }
        }

        /**
         *  Invoke the named action. Internal use only. Called from ejsWeb.c.
         *  @param act Action name to invoke
         */
        function doAction(act: String): Void {
            if (act == "") {
                act = "index"
            }
            actionName = act

            use namespace action
            if (this[actionName] == undefined) {
                originalActionName = actionName
                actionName = "missing"
            }

            flash = session["__flash__"]
            if (flash == "" || flash == undefined) {
                flash = {}
            } else {
                session["__flash__"] = undefined
                lastFlash = flash.clone()
            }

            runFilters(_beforeFilters)

            if (!redirected) {
                try {
                    this[actionName]()

                } catch (e) {
                    reportError(Http.ServerError, "Error in action: " + actionName, e)
                    //  TODO BUG - return not taken
                    rendered = true
                    return
                }

                if (!rendered) {
                    renderView()
                }

                runFilters(_afterFilters)

                //  TODO - temp
                if (Record.db) {
                    Record.db.close()
                }
            }
            
            if (lastFlash) {
                for (item in flash) {
                    for each (old in lastFlash) {
                        if (hashcode(flash[item]) == hashcode(old)) {
                            delete flash[item]
                        }
                    }
                }
            }
            if (flash && flash.length > 0) {
                session["__flash__"] = flash
            }
            // Memory.printStats()
        }


        /**
         *  Send an error response back to the client. This calls discard.
         *  @param msg Message to display
         */
        native function sendError(code: Number, msg: String): Void


        /**
         *  Transform a string to be safe for output into an HTML web page. It does this by changing the
         *  ">", "<" and '"' characters into their ampersand HTML equivalents.
         *  @param s input string
         *  @returns a transformed HTML escaped string
         */
        function escapeHtml(s: String): String {
            return s.replace(/&/g,'&amp;').replace(/\>/g,'&gt;').replace(/</g,'&lt;').replace(/"/g,'&quot;')
        }


		/** 
		 *	HTML encode the arguments
         *  @param args Variable arguments that will be converted to safe html
         *  @return A string containing the encoded arguments catenated together
		 */
		function html(...args): String {
            result = ""
			for (let s: String in args) {
				result += escapeHtml(s)
			}
            return resul
		}


        /**
         *  Send a positive notification to the user. This is just a convenience instead of setting flash["inform"]
         *  @param msg Message to display
         */
        function inform(msg: String): Void {
            flash["inform"] = msg
        }


        function error(msg: String): Void {
            flash["error"] = msg
        }


        /**
         *  Control whether the HTTP connection is kept alive after this request
         *  @parm on Set to true to enable keep alive.
         */
        native function keepAlive(on: Boolean): Void


        /**
         *  Load a view. If path is not supplied, use the default view for the current action.
         *  @param viewName Name of the view to load. It may be a fully qualified path name or a filename relative to the 
         *      views directory for the current action, but without the '.ejs' extension.
         */
        native function loadView(path: String = null): Void


        /**
         *  Make a URL suitable for invoking actions. This routine will construct a URL Based on a supplied action name, 
         *  model id and options that may contain an optional controller name. This is a convenience routine remove from 
         *  applications the burden of building URLs that correctly use action and controller names.
         *  @params action The action name to invoke in the URL. If the name starts with "/", it is assumed to be 
         *      a controller name and it is used by itself.
         *  @params id The model record ID to select via the URL. Defaults to null.
         *  @params options The options string
         *  @return A string URL.
         *  @options url An override url to use. All other args are ignored.
         *  @options controller The name of the controller to use in the URL.
         */
        function makeUrl(action: String, id: String = null, options: Object = null): String {
            let cname : String 
            if (options && options["url"]) {
                return options.url
            }
            if (action.startsWith("/")) {
                return action
            }
            if (options == null || options["controller"] == null) {
                cname = controllerName
            } else {
                cname = options["controller"]
            }
            let url: String = appUrl.trim("/")
            if (url != "") {
                url = "/" + url
            }
            url += "/" + cname + "/" + action
            if (id && id != "" && id != "undefined" && id != null) {
                url += "?id=" + id
            }
            return url
        }


        /**
         *  Redirect the client to a new URL. This call redirects the client's browser to a new location specified 
         *  by the @url.  Optionally, a redirection code may be provided. Normally this code is set to be the HTTP 
         *  code 302 which means a temporary redirect. A 301, permanent redirect code may be explicitly set.
         *  @param url Url to redirect the client to
         *  @param code Optional HTTP redirection code
         */
        native function redirectUrl(url: String, code: Number = 302): Void


        //  TODO - doc incomplete
        /**
         *  Redirect to the given action
         *  Options: id controller
         */
        function redirect(action: String, id: String = null, options: Object = null): Void {
            redirectUrl(makeUrl(action, id, options))
            // rendered = true
            redirected = true
        }


        /**
         *  Render the raw arguments back to the client. The args are converted to strings.
         */
        function render(...args): Void { 
            rendered = true
            write(args)
        }


        /**
         *  Render a file's contents. 
         */
        function renderFile(filename: String): Void { 
            rendered = true
            let file: File = new File(filename)
            try {
                file.open(File.Read)
                while (true) {
                    writeRaw(file.read(4096))
                }
                file.close()
            } catch (e: Error) {
                reportError(Http.ServerError, "Can't read file: " + filename, e)
            }
            write(File.getString(filename))
        }


        # FUTURE
        function renderPartial(): void {
        }


        /**
         *  Render raw data
         */
        function renderRaw(...args): Void {
            rendered = true
            writeRaw(args)
        }


        # FUTURE
        function renderXml(): Void {}

        # FUTURE
        function renderJSON(): Void {}


        /**
         *  Render a view template
         */
        function renderView(viewName: String = null): Void {

            if (rendered) {
                throw new Error("renderView invoked but render has already been called")
                return
            }
            rendered = true

            if (viewName == null) {
                viewName = actionName
            }
            
            //  TODO - compiler bug work around
            let gotError = false
            try {
                let name = Reflect(this).name
                let viewClass: String = name.trim("Controller") + "_" + viewName + "View"
                if (global[viewClass] == undefined) {
                    loadView(viewName)
                }
                view = new global[viewClass](this)

            } catch (e: Error) {
                if (e.code == undefined) {
                    e.code = Http.ServerError
                }
                if (extension(request.url) == ".ejs") {
                    reportError(e.code, "Can't load page: " + request.url, e)
                } else {
                    reportError(e.code, "Can't load view: " + viewName + ".ejs" + " for " + request.url, e)
                }
                //  TODO - bug this return is not returning
                gotError = true
                return
            }

            if (!gotError) {
                try {
                    for (let n: String in this) {
                        view[n] = this[n]
                    }
                    view.render()
                } catch (e: Error) {
                    reportError(Http.ServerError, 'Error rendering: "' + viewName + '.ejs".', e)
                } catch (msg) {
                    reportError(Http.ServerError, 'Error rendering: "' + viewName + '.ejs". ' + msg)
                }
            }
        }


        /**
         *  Report errors back to the client
         *  @param msg Message to send to the client
         *  @param e Optional Error exception object to include in the message
         */
        private function reportError(code: Number, msg: String, e: Object = null): Void {
            if (code <= 0) {
                code = Http.ServerError
            }
            if (e) {
                e = e.toString().replace(/.*Error Exception: /, "")
            }
            if (host.logErrors) {
                if (e) {
                    msg += "\r\n" + e
                }
            } else {
                msg = "<h1>Ejscript error for \"" + request.url + "\"</h1>\r\n<h2>" + msg + "</h2>\r\n"
                if (e) {
                    msg += "<pre>" + escapeHtml(e) + "</pre>\r\n"
                }
                msg += '<p>To prevent errors being displayed in the "browser, ' + 
                    'use <b>"EjsErrors log"</b> in the config file.</p>\r\n'
            }
            sendError(code, msg)
        }


        //  TODO - reconsider arg order
        /**
         *  Define a cookie header to include in the reponse
         */
        native function setCookie(name: String, value: String, lifetime: Number, path: String, secure: Boolean = false): Void


        /**
         *  of the format "keyword: value". If a header has already been defined and \a allowMultiple is false, 
         *  the header will be overwritten. If \a allowMultiple is true, the new header will be appended to the 
         *  response headers and the existing header will also be output. NOTE: case does not matter in the header keyword.
         *  @param header Header string
         *  @param allowMultiple If false, overwrite existing headers with the same keyword. If true, all headers are output.
         */
        native function setHeader(key: String, value: String, allowMultiple: Boolean = false): Void


        /**
         *  Set the HTTP response status code
         *  @param code HTTP status code to define
         */
        native function setHttpCode(code: Number): Void


        /**
         *  Send a status message to any registered status message view controls
         */
        function statusMessage(...args) {
            //  TODO 
        }


        /**
         *  Set the response body mime type
         *  @param format Mime type for the response. For example "text/plain".
         */
        native function setMimeType(format: String): Void


        /**
         *  Transform an escaped string into its original contents. This reverses the transformation done by $escapeHtml.
         *  It does this by changing &quot, &gt, &lt back into ", < and >.
         *  @param s input string
         *  @returns a transformed string
         */
        function unescapeHtml(s: String): String {
            return replace(/&amp/g,'&;').replace(/&gt/g,'>').replace(/&lt/g,'<').replace(/&quot/g,'"')
        }


        /**
         *  Send a warning message back to the client for display in the flash area. This is just a convenience instead of
         *  setting flash["warn"]
         *  @param msg Message to display
         */
        function warn(msg: String): Void {
            flash["warning"] = msg
        }


        /**
         *  Write text to the client. This call writes the arguments back to the client's browser. The arguments 
         *  are converted to strings before writing back to the client. Text written using write, will be buffered 
         *  up to a configurable maximum. This allows text to be written prior to setting HTTP headers with setHeader.
         *  @param args Text or objects to write to the client
         */
        native function write(...args): Void


        /**
         *  Send text back to the client which must first be HTML escaped
         *  @param args Objects to emit
         */
        function writeHtml(...args): Void {
            write(html(args))
        }


        //ZZZ - comment
        //  TODO - omit the new line
        native function writeRaw(...args): Void


        /**
         *  Missing action method. This method will be called if the requested action routine does not exist.
         */
        action function missing(): Void {
            //  TODO - should this (or not) send a non 200 code
            render("<h1>Missing Action</h1>")
            //  TODO - could still try to serve the view
            render("<h3>Action: \"" + originalActionName + "\" could not be found for controller \"" + 
                controllerName + "\".</h3>")
        }
    }

    //  TODO - need to be able to suppress doc somehow
    class _SoloController extends Controller {
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
