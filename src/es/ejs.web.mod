��        '��� ejs.db web/Controller.es } internal-0 ejs.web module ejs.web {     use module ejs.db     namespace action = "action"                 /* Namespace for all action methods */ action Namespace     var view: View view View     class Controller { Controller         use default namespace "ejs.web"         public var actionName:  String  actionName public String         public var originalActionName:  String  originalActionName         public /* synchronized */ var application: Object application Object         public var appUrl:      String appUrl         public var controllerName: String controllerName         public var config:      Object config         public var flash:       Object flash         public var home:        String home         public var host:        Host host Host         public var params:      Object params         public var request:     Request request Request         public var response:    Response response Response         public /* synchronized */ var session: Object session         private var isApp:      Boolean isApp [ejs.web::Controller,private] Boolean         private var rendered:   Boolean rendered         private var redirected: Boolean redirected         private var events:     Dispatcher events Dispatcher         private var _afterFilters: Array _afterFilters Array         private var _beforeFilters: Array _beforeFilters         private var _wrapFilters: Array _wrapFilters         function initialize(isApp: Boolean, appDir: String, appUrl: String, session: Session, host: Host, request: Request,                  response: Response): Void {             this.isApp = isApp             this.home = appDir             this.appUrl = appUrl             this.session = session             this.host = host             this.request = request             this.response = response             if (isApp) {                 config = deserialize("{ " + File.getString(appDir + "/config/config.ecf") + " }") {  /config/config.ecf  }                 config.database = deserialize("{ " + File.getString(appDir + "/config/database.ecf") + " }") /config/database.ecf database                  config.view = deserialize("{ " + File.getString(appDir + "/config/view.ecf") + " }") /config/view.ecf                 let adapter: String = config.database[config.app.mode].adapter adapter -hoisted-8 app mode                 let dbname: String = config.database[config.app.mode].database dbname -hoisted-9                 if (adapter != "" && dbname != "") {                     Database.defaultDatabase = new Database(appDir + "/" + dbname) Database /                     if (config.database[config.app.mode].trace) { trace                         Record.trace(true) Record                         Database.trace(true)             }             rendered = false             redirected = false             params = new Object             let name: String = Reflect(this).name name private             controllerName = name.trim("Controller")         } initialize intrinsic appDir Block Void         native function cache(enable: Boolean = true): Void cache enable         native function createSession(timeout: Number): Void createSession timeout Number         native function destroySession(): Void destroySession         native function discardOutput(): Void discardOutput         function beforeFilter(fn, options: Object = null): Void {             if (_beforeFilters == null) {                 _beforeFilters = []             _beforeFilters.append([fn, options]) beforeFilter fn options         function afterFilter(fn, options: Object = null): Void {             if (_afterFilters == null) {                 _afterFilters = []             _afterFilters.append([fn, options]) afterFilter         function wrapFilter(fn, options: Object = null): Void {             if (_wrapFilters == null) {                 _wrapFilters = []             _wrapFilters.append([fn, options]) wrapFilter         private function runFilters(filters: Array): Void {             if (!filters) {                 return             for each (filter in filters) { filter -hoisted-1 -hoisted-2                 let fn = filter[0]                 let options = filter[1] -hoisted-3                 if (options) {                     only = options.only only                     if (only) {                         if (only is String && actionName != only) {                             continue                         if (only is Array && !only.contains(actionName)) { contains                     }                      except = options.except except                     if (except) {                         if (except is String && actionName == except) {                         if (except is Array && except.contains(actionName)) {                 }                 fn.call(this) call runFilters filters StopIteration iterator         function doAction(act: String): Void {             if (act == "") {                 act = "index" index             actionName = act             use namespace action             if (this[actionName] == undefined) {                 originalActionName = actionName                 actionName = "missing" missing             flash = session["__flash__"] __flash__             if (flash == "" || flash == undefined) {                 flash = {}             } else {                 session["__flash__"] = undefined                 lastFlash = flash.clone() lastFlash             runFilters(_beforeFilters)             if (!redirected) {                 try {                     this[actionName]()                 } catch (e) { e                     reportError(Http.ServerError, "Error in action: " + actionName, e) Error in action:                      rendered = true                     return                 if (!rendered) {                     renderView()                 runFilters(_afterFilters)                 if (Record.db) { db                     Record.db.close() close             if (lastFlash) {                 for (item in flash) { item                     for each (old in lastFlash) { old -hoisted-4 getValues                         if (hashcode(flash[item]) == hashcode(old)) {                             delete flash[item]             if (flash && flash.length > 0) {                 session["__flash__"] = flash doAction act         native function sendError(code: Number, msg: String): Void sendError code msg         function escapeHtml(s: String): String {             return s.replace(/&/g,'&amp;').replace(/\>/g,'&gt;').replace(/</g,'&lt;').replace(/"/g,'&quot;') /&/g &amp; /\>/g &gt; replace /</g &lt; /"/g &quot; escapeHtml s 		function html(...args): String {             result = "" result 			for (let s: String in args) { 				result += escapeHtml(s)             return resul resul html args         function inform(msg: String): Void {             flash["inform"] = msg inform         function error(msg: String): Void {             flash["error"] = msg error         native function keepAlive(on: Boolean): Void keepAlive on         native function loadView(path: String = null): Void loadView path         function makeUrl(action: String, id: String = null, options: Object = null): String { cname             let cname : String              if (options && options["url"]) { url                 return options.url             if (action.startsWith("/")) {                 return action             if (options == null || options["controller"] == null) { controller                 cname = controllerName                 cname = options["controller"]             let url: String = appUrl.trim("/")             if (url != "") {                 url = "/" + url             url += "/" + cname + "/" + action             if (id && id != "" && id != "undefined" && id != null) { undefined                 url += "?id=" + id ?id=             return url makeUrl id         native function redirectUrl(url: String, code: Number = 302): Void redirectUrl         function redirect(action: String, id: String = null, options: Object = null): Void {             redirectUrl(makeUrl(action, id, options))             redirected = true redirect         function render(...args): Void {              rendered = true             write(args) render         function renderFile(filename: String): Void {              let file: File = new File(filename) file File             try {                 file.open(File.Read)                 while (true) {                     writeRaw(file.read(4096))                 file.close()             } catch (e: Error) { Error                 reportError(Http.ServerError, "Can't read file: " + filename, e) Can't read file:              write(File.getString(filename)) renderFile filename ejs.io         function renderRaw(...args): Void {             writeRaw(args) renderRaw         function renderView(viewName: String = null): Void {             if (rendered) {                 throw new Error("renderView invoked but render has already been called") renderView invoked but render has already been called             if (viewName == null) {                 viewName = actionName gotError             let gotError = false                 let name = Reflect(this).name                 let viewClass: String = name.trim("Controller") + "_" + viewName + "View" viewClass trim _                 if (global[viewClass] == undefined) {                     loadView(viewName)                 view = new global[viewClass](this) -hoisted-5                 if (e.code == undefined) {                     e.code = Http.ServerError                 if (extension(request.url) == ".ejs") { .ejs                     reportError(e.code, "Can't load page: " + request.url, e) Can't load page:                  } else {                     reportError(e.code, "Can't load view: " + viewName + ".ejs" + " for " + request.url, e) Can't load view:   for                  gotError = true             if (!gotError) {                     for (let n: String in this) { n -hoisted-7 get                         view[n] = this[n]                     }                     view.render()                 } catch (e: Error) {                     reportError(Http.ServerError, 'Error rendering: "' + viewName + '.ejs".', e) Error rendering: " .ejs".                 } catch (msg) {                     reportError(Http.ServerError, 'Error rendering: "' + viewName + '.ejs". ' + msg) .ejs".  renderView viewName         private function reportError(code: Number, msg: String, e: Object = null): Void {             if (code <= 0) {                 code = Http.ServerError             if (e) {                 e = e.toString().replace(/.*Error Exception: /, "") /.*Error Exception: /             if (host.logErrors) {                 if (e) {                     msg += "\r\n" + e 
                 msg = "<h1>Ejscript error for \"" + request.url + "\"</h1>\r\n<h2>" + msg + "</h2>\r\n" <h1>Ejscript error for " "</h1>
<h2> </h2>
                     msg += "<pre>" + escapeHtml(e) + "</pre>\r\n" <pre> </pre>
                     'use <b>"EjsErrors log"</b> in the config file.</p>\r\n'                 msg += '<p>To prevent errors being displayed in the "browser, ' +  <p>To prevent errors being displayed in the "browser,  use <b>"EjsErrors log"</b> in the config file.</p>
             sendError(code, msg) reportError         native function setCookie(name: String, value: String, lifetime: Number, path: String, secure: Boolean = false): Void setCookie value lifetime secure         native function setHeader(key: String, value: String, allowMultiple: Boolean = false): Void setHeader key allowMultiple         native function setHttpCode(code: Number): Void setHttpCode         function statusMessage(...args) { statusMessage         native function setMimeType(format: String): Void setMimeType format         function unescapeHtml(s: String): String {             return replace(/&amp/g,'&;').replace(/&gt/g,'>').replace(/&lt/g,'<').replace(/&quot/g,'"') /&amp/g &; /&gt/g > /&lt/g < /&quot/g " unescapeHtml         function warn(msg: String): Void {             flash["warning"] = msg warning warn         native function write(...args): Void write         function writeHtml(...args): Void {             write(html(args)) writeHtml         native function writeRaw(...args): Void writeRaw         action function missing(): Void {             render("<h1>Missing Action</h1>") <h1>Missing Action</h1>             render("<h3>Action: \"" + originalActionName + "\" could not be found for controller \"" +                  controllerName + "\".</h3>") <h3>Action: " " could not be found for controller " ".</h3> clone Function deep Iterator namespaces length toString locale ejs.events     class _SoloController extends Controller { _SoloController block_0005_1 -block- web/Cookie.es internal-1     class Cookie { Cookie         var name: String         var value: String         var domain: String domain         var path: String block_0005_61 web/Host.es internal-2 	final class Host {         use default namespace public 		native var documentRoot: String documentRoot 		native var name: String 		native var protocol: String protocol 		native var isVirtualHost: Boolean isVirtualHost 		native var isNamedVirtualHost: Boolean isNamedVirtualHost 		native var software: String software         native var logErrors: Boolean logErrors block_0005_63 web/Request.es internal-3 	final class Request { 		native var accept: String accept 		native var acceptCharset: String acceptCharset 		native var acceptEncoding: String acceptEncoding 		native var authAcl: String authAcl 		native var authGroup: String authGroup 		native var authType: String authType 		native var authUser: String authUser 		native var connection: String connection 		native var contentLength: Number contentLength 		native var cookies: Object cookies 		native var extension: String extension 		native var files: Array files 		native var headers: Object headers 		native var hostName: String hostName 		native var method: String method 		native var mimeType: String mimeType 		native var pathInfo: String pathInfo 		native var pathTranslated pathTranslated 		native var pragma: String pragma 		native var query: String query 		native var originalUri: String originalUri 		native var referrer: String referrer 		native var remoteAddress: String remoteAddress 		native var remoteHost: String remoteHost 		native var sessionID: String sessionID 		native var url: String 		native var userAgent: String userAgent block_0005_65 web/Response.es internal-4 	final class Response { 		native var code: Number 		native var filename: String 		native var headers: Array block_0005_67 web/Session.es internal-5 sessions     var sessions = []     dynamic class Session { Session block_0005_69 web/UploadFile.es internal-6 	class UploadFile { UploadFile 		native var clientFilename: String clientFilename 		native var contentType: String contentType 		var name: String 		native var size: Number size block_0005_71 web/View.es internal-7 	dynamic class View {         var controller: Controller         private var currentModel: Record currentModel [ejs.web::View,private]         private var config: Object         function View(controller: Controller) {             this.controller = controller             this.config = controller.config -constructor- 		public function render(): Void {}         function aform(action: String, record: Object = null, options: Object = null): Void {             if (record == null) {                 record = new Model Model             currentModel = record             formErrors(record)             options = setOptions("aform", options) aform             if (options.method == null) {                 options.method = "POST" POST             if (action == null) {                 action = "update" update connector             let connector = getConnector("aform", options)             options.url = makeUrl(action, record.id, options)             connector.aform(record, options.url, options) record 		function alink(text: String, action: String = null, options: Object = null): Void {                 action = text.split(" ")[0].toLower()   toLower             options = setOptions("alink", options) alink             let connector = getConnector("alink", options)             options.url = makeUrl(action, options.id, options)             connector.alink(text, options.url, options) 		} text 		function button(value: String, buttonName: String = null, options: Object = null): Void {             options = setOptions("button", options) button             if (buttonName == null) {                 buttonName = value.toLower()             let connector = getConnector("button", options)             connector.button(value, buttonName, options) buttonName 		function buttonLink(text: String, action: String, options: Object = null): Void {             options = setOptions("buttonLink", options) buttonLink             let connector = getConnector("buttonLink", options)             connector.buttonLink(text, makeUrl(action, "", options), options) 		function chart(initialData: Array, options: Object = null): Void {             let connector = getConnector("chart", options) chart             connector.chart(initialData, options) initialData 		function checkbox(field: String, choice: String, options: Object = null): Void {             options = setOptions(field, options)             let value = getValue(currentModel, field, options)             let connector = getConnector("checkbox", options) checkbox             connector.checkbox(options.fieldName, value, choice, options) fieldName field choice         function endform(): Void {             let connector = getConnector("endform", null) endform             connector.endform()             currentModel = undefined         function form(action: String, record: Object = null, options: Object = null): Void {             options = setOptions("form", options) form             let connector = getConnector("form", options)             connector.form(record, options.url, options)         function image(image: String, options: Object = null): Void {             let connector = getConnector("image", options) image             connector.image(image, options)         function input(field: String, options: Object = null): Void {             datatype = currentModel.getFieldType(field) datatype             switch (datatype) {             case "binary": binary             case "date": date             case "datetime": datetime             case "decimal": decimal             case "float": float             case "integer": integer             case "number": number             case "string": string             case "time": time             case "timestamp": timestamp                 text(field, options)                 break             case "text":                 textarea(field, options)             case "boolean": boolean                 checkbox(field, "true", options) true             default:                 throw "input control: Unknown field type: " + datatype + " for field " + field input control: Unknown field type:   for field  input         function label(text: String, options: Object = null): Void {             options = setOptions("label", options) label             let connector = getConnector("label", options)             connector.label(text, options) 		function link(text: String, action: String = null, options: Object = null): Void {             options = setOptions("link", options) link             let connector = getConnector("link", options)             connector.link(text, makeUrl(action, options.id, options), options) 		function extlink(text: String, url: String, options: Object = null): Void {             let connector = getConnector("extlink", options) extlink             connector.extlink(text, controller.appUrl + url, options) 		function list(field: String, choices: Object = null, options: Object = null): Void {             if (choices == null) {                 modelTypeName = field.replace(/\s/, "").toPascal() /\s/ toPascal modelTypeName                 modelTypeName = modelTypeName.replace(/Id$/, "") /Id$/                 if (global[modelTypeName] == undefined) {                     throw new Error("Can't find model to create list data: " + modelTypeName) Can't find model to create list data:                  choices = global[modelTypeName].findAll() findAll             let connector = getConnector("list", options) list             connector.list(options.fieldName, choices, value, options) choices 		function mail(nameText: String, address: String, options: Object = null): Void  {             let connector = getConnector("mail", options) mail             connector.mail(nameText, address, options) nameText address 		function progress(initialData: Object, options: Object = null): Void {             let connector = getConnector("progress", options) progress             connector.progress(initialData, options) 		function radio(field: String, choices: Object, options: Object = null): Void {             let connector = getConnector("radio", options) radio             connector.radio(options.fieldName, value, choices, options) 		function script(url: Object, options: Object = null): Void {             let connector = getConnector("script", options) script             if (url is Array) {                 for each (u in url) { u                     connector.script(controller.appUrl + "/" + u, options)                 connector.script(controller.appUrl + "/" + url, options) 		function status(initialData: Object, options: Object = null): Void {             let connector = getConnector("status", options) status             connector.status(initialData, options) 		function stylesheet(url: Object, options: Object = null): Void {             let connector = getConnector("stylesheet", options) stylesheet                     connector.stylesheet(controller.appUrl + "/" + u, options)                 connector.stylesheet(controller.appUrl + "/" + url, options) 		function table(data: Array, options: Object = null): Void {             options = setOptions("table", options) table             let connector = getConnector("table", options)             if (controller.params.filter) {                 filter(data)             if (controller.params.sort) { sort                 sort(data)             connector.table(data, options) data         function tabs(initialData: Array, options: Object = null): Void {             let connector = getConnector("tabs", options) tabs             connector.tabs(initialData, options)         function text(field: String, options: Object = null): Void {             let connector = getConnector("text", options)             connector.text(options.fieldName, value, options)         function textarea(field: String, options: Object = null): Void {             let connector = getConnector("textarea", options) textarea             connector.textarea(options.fieldName, value, options)         function tree(initialData: XML, options: Object = null): Void {             let connector = getConnector("tree", options) tree             connector.tree(initialData, options) XML 		function flash(kinds = null, options: Object = null): Void {             options = setOptions("flash", options)             let cflash = controller.flash cflash             if (cflash == null || cflash.length == 0) { msgs             let msgs: Object             if (kinds is String) {                 msgs = {}                 msgs[kinds] = cflash[kinds]             } else if (kinds is Array) {                 for each (kind in kinds) { kind                     msgs[kind] = cflash[kind]                 msgs = cflash             for (kind in msgs) { -hoisted-6                 let msg: String = msgs[kind]                 if (msg && msg != "") {                     let connector = getConnector("flash", options)                     options.style = "flash flash" + kind.toPascal() flash flash style                     connector.flash(kind, msg, options) kinds         private function formErrors(model): Void {             if (!model) { errors             let errors = model.getErrors() getErrors             if (errors) {                 write('<div class="formError"><h2>The ' + Reflect(model).name.toLower() + ' has ' +                      ((errors.length > 1) ? 'prevent' : 'prevents') + '  it being saved.</h2>\r\n')                     errors.length + (errors.length > 1 ? ' errors' : ' error') + ' that ' + <div class="formError"><h2>The   has   errors  error  that  prevent prevents   it being saved.</h2>
                 write('    <p>There were problems with the following fields:</p>\r\n')     <p>There were problems with the following fields:</p>
                 write('    <ul>\r\n')     <ul>
                 for (e in errors) {                     write('        <li>' + e.toPascal() + ' ' + errors[e] + '</li>\r\n')         <li> </li>
                 write('    </ul>\r\n')     </ul>
                 write('</div>\r\n') </div>
 formErrors model         function createSession(timeout: Number): Void {             controller.createSession(timeoout) timeoout         function destroySession(): Void {             controller.destroySession()             return controller.html(args)             return controller.makeUrl(action, id, options)         function redirectUrl(url: String, code: Number = 302): Void {             controller.redirectUrl(url, code)         function setCookie(name: String, value: String, lifetime: Number, path: String, secure: Boolean = false): Void {             controller.setCookie(name, value, lifetime, path, secure)         function setHeader(key: String, value: String, allowMultiple: Boolean = false): Void {             controller.setHeader(key, value, allowMultiple)         function setHttpCode(code: Number): Void {             controller.setHttpCode(code)         function setMimeType(format: String): Void {             controller.setMimeType(format); 		function write(...args): Void {             controller.write(args) 		function writeHtml(...args): Void { 			controller.write(html(args))         function writeRaw(...args): Void {             controller.writeRaw(args)         function d(...args): Void {             write('<pre>\r\n') <pre>
             for each (var e: Object in args) {                 write(serialize(e) + "\r\n")             write('</pre>\r\n') d         private function addHelper(fun: Function, overwrite: Boolean = false): Void {             let name: String = Reflect(fun).name             if (this[name] && !overwrite) {                 throw new Error('Helper ' + name + ' already exists') Helper   already exists             this[name] = fun addHelper fun overwrite 		private function getConnector(kind: String, options: Object) { connectorName             var connectorName: String             if (options && options["connector"]) {                 connectorName = options["connector"]                 connectorName =  config.view.connectors[kind] connectors             if (connectorName == undefined || connectorName == null) {                 connectorName =  config.view.connectors["rest"] rest                 if (connectorName == undefined || connectorName == null) {                     connectorName = "html"                 config.view.connectors[kind] = connectorName             let name: String = (connectorName + "Connector").toPascal() Connector                 return new global[name]                 throw new Error("Undefined view connector: " + name) Undefined view connector:  getConnector         private function setOptions(field: String, options: Object): Object {             if (options == null) {                 options = {}             if (options.fieldName == null) {                 if (currentModel) {                     options.fieldName = Reflect(currentModel).name.toCamel() + '.' + field toCamel .                     options.fieldName = field;             if (options.id == null) {                 if (currentModel) {                      if (currentModel.id) {                         options.id = field + '_' + currentModel.id             if (options.style == null) {                 options.style = field             if (currentModel && currentModel.hasError(field)) {                 options.style += " fieldError"  fieldError             return options setOptions         function getValue(model: Object, field: String, options: Object): String {             let value             if (model && field) {                 value = model[field]             if (value == null || value == undefined) {                 if (options.value) {                     value = options.value                 value = ""             if (options.render != undefined && options.render is Function) {                 return options.render(value, model, field).toString() typeName             let typeName = Reflect(value).typeName             let fmt = config.view.formats[typeName] fmt formats             if (fmt == undefined || fmt == null || fmt == "") {                 return value.toString()             switch (typeName) {             case "Date": Date                 return new Date(value).format(fmt)             case "Number":                 return fmt.format(value)             return value.toString() getValue         function date(fmt: String): String {             return function (data: String): String {                 return new Date(data).format(fmt) --fun_4958--         function currency(fmt: String): String {                 return fmt.format(data) --fun_4992-- currency         function number(fmt: String): String { --fun_5022--         private static const htmlOptions: Object = {  htmlOptions             background: "", color: "", id: "", height: "", method: "", size: "",  background color height             style: "class", visible: "", width: "", class visible width 		function getOptions(options: Object): String { 			if (!options) {                 return ''             let result: String = ""             for (let option: String in options) { option mapped                 let mapped = htmlOptions[option]                 if (mapped) {                     if (mapped == "") {                         mapped = option                     result += ' ' +  mapped + '="' + options[option] + '"' ="             return result + " " getOptions         private function sort(data: Array) { tmp             let tmp = data[0]             data[0] = data[1]             data[1] = tmp         private function filter(data: Array) {             pattern = controller.params.filter.toLower() pattern             for (let i = 0; i < data.length; i++) { i found                 let found: Boolean = false                 for each (f in data[i]) { f                     if (f.toString().toLower().indexOf(pattern) >= 0) { indexOf                         found = true                 if (!found) {                     data.remove(i, i)                     i-- View-initializer -initializer-     internal class Model implements Record {         setup()         function Model(fields: Object = null) {             constructor(fields) fields Model-initializer Record-initializer _belongsTo [ejs.db::Record,private] _className _columnNames _columnTypes _sqlColumnTypes _db _foreignId _keyName _tableName _traceSql _validations constructor save sql saveUpdate belongsTo owner createRecord rowData rec find grid findOneWhere where findWhere count includeJoin joins cmd join innerFind columns from conditions results -hoisted-11 -hoisted-12 tname -hoisted-13 parts -hoisted-14 cond -hoisted-16 -hoisted-17 -hoisted-19 getDb columnNames columnTitles title getKeyName getNumRows getSchema row getTableName hasMany hasAndBelongsToMany thing hasOne loadReference log logResult mapSqlTypeToEjs sqlType ejsType remove keys setDb dbase setup setTableName setKeyName validateFormat validateNumber validatePresence validateUnique ErrorMessages checkFormat thisObj checkNumber checkPresent checkUnique validateModel validation -hoisted-0 hasError getFieldType coerceTypes _keyValue _errors block_0004_73 web/connectors/HtmlConnector.es internal-8 	class HtmlConnector { HtmlConnector 		function aform(record: Object, url: String, options: Object): Void {             if (options.id == undefined) {                 options.id = "form"             onsubmit = "" onsubmit             if (options.condition) { condition                 onsubmit += options.condition + ' && '  &&              if (options.confirm) { confirm                 onsubmit += 'confirm("' + options.confirm + '"); && ' confirm(" "); &&              onsubmit = '$.ajax({ ' +                 'type: "' + options.method + '", '                 'url: "' + url + '", ' +  $.ajax({  url: " ",  type: "             if (options.query) {                 onsubmit += 'data: ' + options.query + ', ' data:  ,                  onsubmit += 'data: $("#' + options.id + '").serialize(), ' data: $("# ").serialize(),              if (options.update) {                 if (options.success) { success                         options.success + '; }, '                     onsubmit += 'success: function(data) { $("#' + options.update + '").html(data).hide("slow"); ' +  success: function(data) { $("# ").html(data).hide("slow");  ; },                      onsubmit += 'success: function(data) { $("#' + options.update + '").html(data).hide("slow"); }, ' ").html(data).hide("slow"); },              } else if (options.success) {                 onsubmit += 'success: function(data) { ' + options.success + '; } ' success: function(data) {  ; }              if (options.error) {                 onsubmit += 'error: function(data) { ' + options.error + '; }, ' error: function(data) {              onsubmit += '}); return false;' }); return false;             write('<form action="' + "/User/list" + '"' + getOptions(options) + "onsubmit='" + onsubmit + "' >") <form action=" /User/list onsubmit=' ' > 		function alink(text: String, url: String, options: Object): Void {                 options.id = "alink"             onclick = "" onclick                 onclick += options.condition + ' && '                 onclick += 'confirm("' + options.confirm + '"); && '             onclick = '$.ajax({ ' +                 'data: ' + options.query + ', '                     onclick += 'success: function(data) { $("#' + options.update + '").html(data); ' +  ").html(data);                      onclick += 'success: function(data) { $("#' + options.update + '").html(data); }, ' ").html(data); },                  onclick += 'success: function(data) { ' + options.success + '; } '                 onclick += 'error: function(data) { ' + options.error + '; }, '             onclick += '}); return false;'             write('<a href="' + options.url + '"' + getOptions(options) + "onclick='" + onclick + "' >" + text + '</a>') <a href=" onclick=' </a> 		function button(value: String, buttonName: String, options: Object): Void {             write('<input name="' + buttonName + '" type="submit" value="' + value + '"' + getOptions(options) + ' />') <input name=" " type="submit" value="  /> 		function buttonLink(text: String, url: String, options: Object): Void { 			write('<a href="' + url + '"><button>' + text + '</button></a>') "><button> </button></a> 		function chart(data: Array, options: Object): Void {             throw 'HtmlConnector control "chart" not implemented.' HtmlConnector control "chart" not implemented. 		function checkbox(name: String, value: String, submitValue: String, options: Object): Void { checked             let checked = (value == submitValue) ? ' checked="yes" ' : ''  checked="yes"              write('<input name="' + name + '" type="checkbox" "' + getOptions(options) + checked +                  '" value="' + submitValue + '" />') " type="checkbox" " " value=" " /> submitValue 		function endform(): Void {             write('</form>') </form> 		function flash(kind: String, msg: String, options: Object): Void {             write('<div' + getOptions(options) + '>' + msg + '</div>') <div </div>             if (kind == "inform") {                 write('<script>$(document).ready(function() { <script>$(document).ready(function() {
                        $("div.flashInform").animate({opacity: 1.0}, 2000).hide("slow");});
                    </script> 		function form(record: Object, url: String, options: Object): Void {             write('<form action="' + url + '"' + getOptions(options) + '>')         function image(src: String, options: Object): Void { 			write('<img src="' + src + '"' + getOptions(options) + '/>') <img src=" /> src         function label(text: String, options: Object): Void {             write('<span ' + getOptions(options) + ' type="' + getTextKind(options) + '">' +  text + '</span>') <span   type=" "> </span> 		function link(text: String, url: String, options: Object): Void { 			write('<a href="' + url + '"' + getOptions(options) + '>' + text + '</a>') 		function extlink(text: String, url: String, options: Object): Void { 		function list(name: String, choices: Object, defaultValue: String, options: Object): Void {             write('<select name="' + name + '" ' + getOptions(options) + '>') <select name=" "              let isSelected: Boolean isSelected             let i = 0             for each (choice in choices) {                 if (choice is Array) {                     isSelected = (choice[0] == defaultValue) ? 'selected="yes"' : '' selected="yes"                     write('  <option value="' + choice[0] + '"' + isSelected + '>' + choice[1] + '</option>')   <option value=" </option>                     if (choice && choice.id) {                         for (field in choice) {                             isSelected = (choice.id == defaultValue) ? 'selected="yes"' : ''                             if (field != "id") {                                 write('  <option value="' + choice.id + '"' + isSelected + '>' + choice[field] + '</option>')                     } else {                         isSelected = (i == defaultValue) ? 'selected="yes"' : ''                         write('  <option value="' + i + '"' + isSelected + '>' + choice + '</option>')                 i++             write('</select>') </select> defaultValue 		function mail(name: String, address: String, options: Object): Void  { 			write('<a href="mailto:' + address + '" ' + getOptions(options) + '>' + name + '</a>') <a href="mailto: 		function progress(data: Array, options: Object): Void {             write('<p>' + data + '%</p>') <p> %</p>         function radio(name: String, selected: String, choices: Object, options: Object): Void {             let checked: String             if (choices is Array) {                 for each (v in choices) { v                     checked = (v == selected) ? "checked" : ""                     write(v + ' <input type="radio" name="' + name + '"' + getOptions(options) +                          ' value="' + v + '" ' + checked + ' />\r\n')  <input type="radio" name="  value="  />
                 for (item in choices) {                     checked = (choices[item] == selected) ? "checked" : ""                     write(item + ' <input type="radio" name="' + name + '"' + getOptions(options) +                          ' value="' + choices[item] + '" ' + checked + ' />\r\n') selected 		function script(url: String, options: Object): Void {             write('<script src="' + url + '" type="text/javascript"></script>\r\n') <script src=" " type="text/javascript"></script>
 		function status(data: Array, options: Object): Void {             write('<p>' + data + '</p>\r\n') </p>
 		function stylesheet(url: String, options: Object): Void {             write('<link rel="stylesheet" type="text/css" href="' + url + '" />\r\n') <link rel="stylesheet" type="text/css" href=" " />
 		function tabs(tabs: Array, options: Object): Void {             write('<div class="menu">') <div class="menu">             write('<ul>') <ul>             for each (t in tabs) { t                 for (name in t) {                     let url = t[name]                     write('<li><a href="' + url + '">' + name + '</a></li>\r\n') <li><a href=" </a></li>
             write('</ul>') </ul>             write('</div>') 		function table(data: Array, options: Object = {}): Void { 			if (data == null || data.length == 0) { 				write("<p>No Data</p>") <p>No Data</p> 				return             if (options.title) {                 write('    <h2 class="ejs tableHead">' + options.title + '</h2>')     <h2 class="ejs tableHead"> </h2> 			write('<table ' + getOptions(options) + '>') <table  			write('  <thead class="ejs">')   <thead class="ejs"> 			write('  <tr>')   <tr>             let line: Object = data[0] line             let columns: Object = options["columns"]             if (columns) {                 for (name in columns) { column                     let column = columns[name]                     if (line[name] == undefined && column.render == undefined) {                         throw new Error("Can't find column \"" + name + "\" in data set: " + serialize(line)) Can't find column " " in data set:                          columns[name] = null                 for (let name in columns) {                     if (name == null) {                         continue header                     let header: String                     if (columns[name].header == undefined) {                         header = name.toPascal()                         header = columns[name].header                     let width = (columns[name].width) ? ' width="' + columns[name].width + '"' : ''  width="                     write('    <th class="ejs"' + width + '>' + header + '</th>')     <th class="ejs" </th>                 columns = {}                 for (let name in line) {                     if (name == "id" && !options.showId) { showId                     write('    <th class="ejs">' + name.toPascal() + '</th>')     <th class="ejs">                     columns[name] = {} 			write("  </tr>\r\n</thead>")   </tr>
</thead>             let row: Number = 0 			for each (let r: Object in data) { r 				write('  <tr class="ejs">')   <tr class="ejs">                 let url: String = null                 if (options.click) { click                     url = view.makeUrl(options.click, r.id, options) -hoisted-15                 let style = (row % 2) ? "oddRow" : "evenRow" oddRow evenRow 				for (name in columns) {                     let cellStyle: String cellStyle -hoisted-18                     if (column.style) {                         cellStyle = style + " " + column.style                         cellStyle = style                     data = view.getValue(r, name, { render: column.render } )                     if (url) {                         write('    <td class="ejs ' + cellStyle + '"><a href="' + url + '">' + data + '</a></td>')     <td class="ejs  "><a href=" </a></td>                         write('    <td class="ejs ' + cellStyle + '">' + data + '</td>') </td> 				}                 row++ 				write('  </tr>')   </tr> 			} 			write('</table>') </table>         function text(name: String, value: String, options: Object): Void {             write('<input name="' + name + '" ' + getOptions(options) + ' type="' + getTextKind(options) +                  '" value="' + value + '" />')         function textarea(name: String, value: String, options: Object): Void {             numCols = options.numCols numCols             if (numCols == undefined) {                 numCols = 60             numRows = options.numRows numRows             if (numRows == undefined) {                 numRows = 10             write('<textarea name="' + name + '" type="' + getTextKind(options) + '" ' + getOptions(options) +                  ' cols="' + numCols + '" rows="' + numRows + '">' + value + '</textarea>') <textarea name=" " type="  cols=" " rows=" </textarea>         function tree(data: Array, options: Object): Void {             throw 'HtmlConnector control "tree" not implemented.' HtmlConnector control "tree" not implemented.         private function getTextKind(options): String {             var kind: String             if (options.password) { password                 kind = "password"             } else if (options.hidden) { hidden                 kind = "hidden"                 kind = "text"             return kind getTextKind [ejs.web::HtmlConnector,private] 		private function getOptions(options: Object): String {             return view.getOptions(options)         private function write(str: String): Void {             view.write(str) str block_0005_135 web/connectors/GoogleConnector.es internal-9 	class GoogleConnector { GoogleConnector         private var nextId: Number = 0 nextId [ejs.web::GoogleConnector,private]         private function scriptHeader(kind: String, id: String): Void {             write('<script type="text/javascript" src="http://www.google.com/jsapi"></script>') <script type="text/javascript" src="http://www.google.com/jsapi"></script>             write('<script type="text/javascript">') <script type="text/javascript">             write('  google.load("visualization", "1", {packages:["' + kind + '"]});')   google.load("visualization", "1", {packages:[" "]});             write('  google.setOnLoadCallback(' + 'draw_' + id + ');')   google.setOnLoadCallback( draw_ ); scriptHeader 		function table(grid: Array, options: Object): Void {             var id: String = "GoogleTable_" + nextId++ GoogleTable_ 			if (grid == null || grid.length == 0) {             let columns: Array = options["columns"]             scriptHeader("table", id)             write('  function ' + 'draw_' + id + '() {')   function  () { 			write('    var data = new google.visualization.DataTable();')     var data = new google.visualization.DataTable();             let firstLine: Object = grid[0] firstLine                 if (columns[0] != "id") {                     columns.insert(0, "id")                 for (let i = 0; i < columns.length; ) {                     if (firstLine[columns[i]]) {                         i++                         columns.remove(i, i)                 columns = []                 for (let name in firstLine) {                     columns.append(name)             for each (name in columns) { -hoisted-10                 write('    data.addColumn("string", "' + name.toPascal() + '");')     data.addColumn("string", " "); 			write('    data.addRows(' + grid.length + ');')     data.addRows( 			for (let row: Object in grid) { col                 let col: Number = 0                 for each (name in columns) {                     write('    data.setValue(' + row + ', ' + col + ', "' + grid[row][name] + '");')     data.setValue( , "                     col++             write('    var table = new google.visualization.Table(document.getElementById("' + id + '"));')     var table = new google.visualization.Table(document.getElementById(" "));             let goptions = getOptions(options, {  goptions                 height: null,                  page: null, page                 pageSize: null, pageSize                 showRowNumber: null, showRowNumber                 sort: null,                 title: null,                 width: null,              write('    table.draw(data, ' + serialize(goptions) + ');')     table.draw(data,              if (options.click) {                 write('    google.visualization.events.addListener(table, "select", function() {')     google.visualization.events.addListener(table, "select", function() {                 write('        var row = table.getSelection()[0].row;')         var row = table.getSelection()[0].row;                 write('        window.location = "' + view.makeUrl(options.click, "", options) + '?id=" + ' +          window.location = " ?id=" +                      'data.getValue(row, 0);') data.getValue(row, 0);                 write('    });')     });             write('  }')   }             write('</script>') </script>             write('<div id="' + id + '"></div>') <div id=" "></div> 		function chart(grid: Array, options: Object): Void {             var id: String = "GoogleChart_" + nextId++ GoogleChart_             scriptHeader("piechart", id) piechart 			let firstLine: Object = grid[0]             let col: Number = 0             let dataType: String = "string" dataType 			for (let name: String in firstLine) {                 if  (columns && columns.contains(name)) {                     write('    data.addColumn("' + dataType + '", "' + name.toPascal() + '");')     data.addColumn(" ", "                     if (col >= 2) {                         break                     dataType = "number" col2                 let col2: Number = 0 				for (let name2: String in grid[row]) { name2                     if  (columns && columns.contains(name2)) {                         if (col2 == 0) {                             write('    data.setValue(' + row + ', ' + col2 + ', "' + grid[row][name2] + '");')                         } else if (col2 == 1) {                             write('    data.setValue(' + row + ', ' + col2 + ', ' + grid[row][name2] + ');')                         }                         col2++             write('    var chart = new google.visualization.PieChart(document.getElementById("' + id + '"));')     var chart = new google.visualization.PieChart(document.getElementById("             let goptions = getOptions(options, { width: 400, height: 400, is3D: true, title: null }) is3D             write('    chart.draw(data, ' + serialize(goptions) + ');')     chart.draw(data,          private function getOptions(options: Object, defaults: Object): Object {             var result: Object = {}             for (let word: String in defaults) { word                 if (options[word]) {                     result[word] = options[word]                 } else if (defaults[word]) {                     result[word] = defaults[word]             return result defaults block_0005_195 __initializer__ ���� ���� 
 ��  3�'3/3@3	VY�R��'3�3�4�'3��b4��'#� 3�c#�c'3�c/3�c
�c4��'#� 3�e9�e'3�e/3�e�e4�'#� 3�h��h'3�h/3�h
�h4�'#� 3�q:�q'3�q/3�q
�q4�'#� 3�r�r'3�r/3�r
�r��R��r'3�r�r4��'#� 3�s(�s'3�s/3�s
�s4��'#� 3�t��t'3�t/3�t@3�t�t4�'3�t���4���t#� 3�����'3��/3��@3����4��'#�	 3�����'3��/3����4��
'#=�c  	�d 	�h 	�p 	�r 	�r 	�t 	�� 	�� 	��	 	
�'����rv�
�' s�'�'  ) �' ���� �3|�3~�a]�3�b]�3��c]�3��d]�3��e]�3��f]�
3��g]�3��a  3��\��Rb\�  � \� ?��3��\��Rb\�  � \� ?�z���3��\��Rb\�  � \� ?�z���3��z���z����� ��� �3��z���z����� ��� �3��i\�,";�j\�,"^3��R���b\� j R��� 3��z���z����� ��� "3��R��^7 3��R��^" @ @ @ 3��3��Q�3��Q�3��� ��3��]�� �3��h\�&���3��
��  
�� 
�� 
��  
��  
��  
��  
�� 
�� 
�� 	�'    
�� �'    
��  �'     �' 	    �'  
  DCZ�3���Z%3�����@ 3��3�����;a�F�;b�G��3��
��   
���'    DCZ�3���Z%3�����@ 3��3�����;a�F�;b�G��3��
��   
���'    DCZ�3���Z%3�����@ 3��3�����;a�F�;b�G��3��
��   
���%� �����3��a�	3���@ 3��a �; ��3�� bF��3�� bG��3�� d�   3�� d��! ��!�3��!�!�\3��!�!��E";	�u�!�,"3��"?����@ 3��"�!��E";��!�u�" ��"3��"?e���@ @ 3��#3��#d��# ��#�3��#�#�[3��#�#��E";	�u�#�%"3��"?���@ 3��$�#��E";��#�u�" �"3��"?����@ @ @ 3��%3��%c]�% ?������3��"���
�%�  
��  
��  
��   �1' �����3��&a\�%3��&\�&�@ 3��3��&a�3��&R��'3��']u��:%3��'u�3��'\�(�@ 3��3��(�\�(��3��({\�%";�{�:%"3��)� �@$3��)3��)�:�\�(�3��){  ���*�3��3��*�3��*���   3��*3��*]u� @-3��+��3��+�S�% \�,u b3��,^�3��,�>3��,��3��- @ 3��%3��-�3��-R����-�3��-R����-��.  @ @ 3��.�*�Q3��.{ �; ��3��.�*��/�% �; ��3��/{d�C�eC�%3��0{d7@ @ؖ@��@ 3��0{";�{�F)"3��0{�\�(�@ �3����������������
�1�  
�+�  
�.�   
�/�/  �1'    
�1�  
�1� �3' ����  73��2a[�3\�3�[�3\�3�3��[�3\�3�3 �[�3\�3�3 ��
�3�  �4' ����D3��3\���4�3��4a �; ��3��4�4�b� ��4�@�3��4�4��77�
�4�  
�3� �5'    3��5a{\�5��3��
�1�  �6'    3��5a{\�6��3��
�1�  �6'    
�6�  �7'    
�7� �=' ���� �CZ�Z�3��83��8c";�c\�8�"3��8c��8��@ 3��9a\� �
3��9a�@ 3��9cZ%";	�c\�:�Z%"
3��:y�@3��)3��:c\�:��3��3��:x\�&��3��;e\�,3��;\�e �@ 3��3��;e\�d \� a  �3��<b";�b\�,"";�b\�<,"";�bZ,"3��<e\�<b  �@ 3��<e�
��  
�=�
��
�8� 
�8� �='    
�8�  
�1��?'    )CZ�Z�3��>abc�3��?^��3��
��  
�=�
���@' ���@  3��?^�3��?a%�3��
�4�  �C'  �3��?^�3��@�R�a�3��A3��Ab�R�	 )3��A^3��AbS 6�'@�3��%3��Bb @3��B��3��B�S�% \�Ca c>3��3��C�Ra �%�3��YYv>
�C�  
�@� �
�+� >�D' ���@  3��?^�3��Da'�3��
�4�  �P' ����
�CZ�3��E�3��E��\�E�3���@ 3��3��?^�3��FaZ%
3��Fu�@ 3��3��GQ�3��A3��G]�� �3��Gc\��H �\�H a \� �3��H�3d��:%3��Ha@ 3��%3��I�3d��]R��'?�   3��B��3��If��1��:%3��J�S�% f��1�@ 3��J�
�b�\�J%3��Jf��1�\�K�
� f@(3��K3��Kf��1�\�La \�J \�L �
� f3��%3��L^�3���>3��Mb��   3��*3��M]�M�% �; ��3��M]h�R��'h�@�3��N3��NR��' @C3��N��3��N�S�% \�Oa \�O i>3��O��3��P�S�% \�Oa \�P j >@ �3��Z���>���������>����
�Q� 
�F�  
��  
�H�  
�+�I >
�M�M 
�+� >
�1� 	 �X� ����  �CZ�3��QaF*3��R�S�% �@ 3��Rc3��Rc �[�S\��3���@ 3��S}q3��Sc3��Sb\�Tc  �@ @h3��)3��T\�T�
� \�U b \�U �3��Sc3��Ub\�Uc� \�U  �@ 3��%3��U3��Vb\�W3��U\�W  3��V�3��3��Wab�3��
�1�  
�1� 
�+��Y'    
��  
�Y� 
�Y� 
�7� 
�Y��Z'    
�Z�  
�Y� 
�Z��Z'     
�1�  �[' ���@ !  �3��
�4�  �[' "   
�\�  �]' ����#  83��\[�]\�]�3��[�]\�]�3 �[�]\�]�3 �[�]\�]�3 ��
�3�  �^'  $  3��^a{\�^��3��
�1�  �^' ���@%   
�4�  �_' ���@&  3��_a�%�3��
�4�  �_' ���@'   
�4�  �(�  (   =3��`\�`3��`3��a3��`\�av \�b 3��ay \�b �3��
��  
�� 
�� 
�� 
�� 
�� 
�� 
�� 
�� �'
�� 	
�� 
�'
�� �'
�� 
�� 
�� 
�	� 
�	� �
�
� 
�
� 
�� �c'  �') �c'   
��c  
�Y�c 
�d�c 
�7�c �'��  
�e� 
��
�f�
�f�
�g�
�g�
�g��'��  
�h� 
�i�
�i�
�i�
�j�
�j�
�j�
�k�
�k�
�k�	
�l�

�l�
�l�
�l�
�m�
�m�
�m�
�n� 
�n�
�n�
�o�
�o�
�o�
�p�
�p�
�8�
�p��'��  
�1� 
�C�
�l�
�m�
�r' z �r'��   �s'   
�s� 
�t�
�C�
�� 
�t��'�� < ��w ����   3�t,�va]�3�t-�va�� ]��3�t.�
�:�   ����     �3�t�'3�t�u3�t �u3�t%�v3�t+�v3�t4�w3�tC�w3�t_�|3�tv�3�t�ق3�t���3�t�3�t���3�t�׊3�t�ތ3�t���3�t���3�t���3�t���3�t��3�t���3�t��3�t���3�t���3�t��3�t���3�t�ҫ3�t�Ǯ3�t���3�t���3�t���3�t�̴3�t���3�t���3�t���3�t��33�t��73�t���3�t��=3�t���3�t���3�t���3�t���3�t���3�t���3�t���3�t���3�t���3�t���3�t���3�t���3�t���3�t���3�t���3�t���3�t���\��\�\��\�\�=\�\��\�\�m\�\�t\�3�t���\�\��\��\�\��\��	3�t���R��'�83�t���3�t���3�t�����@�     ��y' ���� �CZ�Z�3�tD�xbZ%3�tE�xR��x�t� �@ 3�tF�3�tG�yb�3�tH�yb"3�tI�y\�yc3��3�tJ�yc��m�Z%3�tK�z\�zc��m�@ 3�tM�zaZ%3�tN�z\�{�@ 3�tO�3�tP�{\�yc2��3�tQ�{ab��=�c&�c��8�3�tR�|dbc��8�c�y �3�tS�
��  
�|�
��
�{�  �~' ����	 �CZ�Z�3�t`�zbZ%3�ta�}a\�}�F��}  ��@ 3�tb�3�tc�}\�~c3��3�td�yc��m�Z%3�te�z\�zc��m�@ 3�tf�3�tg�~\�~c2��3�th�~bc��=�c&�c��8�3�ti�dac��8�c�~ �3�tj�
��  
��
��
�{�  ��' ����
 dCZ�Z�3�twˀ\��c3��3�tx��bZ%3�ty��a$ ��@ 3�tz�3�t{ف\��c2��3�t|��dabc�� �3�t}�
�Y�  
΂�
��
�{�  �' ���� LCZ�3�t���\�c3��3�t���\�c2��3�t���dab\�c&�c� �3�t��
��  
�� 
��
�{�  ��' ���� 1CZ�3�t�Å\��b2��3�t���cab�� �3�t��
���  
��
�{�  È' ���� XCZ�3�t���ac3��3�t�Ƈvac4��3�t���\Èc2��3�t�̈ec����dbcÈ �3�t��
���  
��� 
��
�Y�  
�{�  ��' ����  43�t�Љ\��Z2��3�t���a��  3�t����:��3�t��
�{�   �' ���� �CZ�Z�3�t��yb�3�t��yb"3�t���\�c3��3�t��yc��m�Z%3�t��z\�zc��m�@ 3�t��zaZ%3�t��z\�{�@ 3�t��3�t��\�c2��3�t��{ab��=�c&�c��8�3�t���dbc��8�c� �3�t��
��  
�|�
��
�{�  ߍ' ���� 1CZ�3�t���\ߍb2��3�t��cabߍ �3�t��
ߍ�  
��
�{�  ��' ����  �CZ�3�t�׎vaE�����3�t������3�t���;\ӏ&@3�t�ڏ;\�&@3�t���;\��&@3�t���;\��&@3�t�;\ܐ&@3�t��;\��&@3�t���;\��&@3�t���;\Ñ&@3�t�ʑ;\�&@3�t��;\��&3�t���ab3�t���?~   @3�t�˒;\�&3�t��ab3�t���?S   @3�t���;\��&3�t���a\�b3�t���?#   3�t��3�t���\۔��� \�� a ̖�3�t��
���  
����' ���� CCZ�3�t�֕\��b3��3�t���\��b2��3�t�ʖcab�� �3�t��
��  
��
�{�  ��' ���� CZ�Z�3�t��zbZ%3�t��}a\�}�F��}  ��@ 3�t��3�t�ʗ\��c3��3�t���\��c2��3�t���dabc��=�c&�c�� �3�t��
��  
��
��
�{�  ��' ���� 5CZ�3�t�ٙ\��c2��3�t���daunb c�� �3�t��
��  
�8� 
��
�{�  ��' ���� �CZ�Z�3�t���ac3��3�t���bZ%�   3�t�ޛa[��\����� �����3�t������[��\��3 �����3�t����3�����:%3�t�����\����� �@ 3�t��%3�t�Þ�3������  ��@ 3�t��3�t�Ƈvac4��3�t���\��c2��3�t�ğec����bdc�� �3�t��
���  
���
��
�Y�  
�{�  ��' ���� 2CZ�3�t��\��c2��3�t���dabc�� �3�t��
ݡ�  
�� 
��
�{�  ��' ���� 1CZ�3�t���\��b2��3�t���cab�� �3�t��
���  
��
�{�  ��' ���� XCZ�3�t���ac3��3�t�Ƈvac4��3�t���\��c2��3�t�Ťec����dbc�� �3�t��
���  
��� 
��
�Y�  
�{�  ��' ����}CZ�3�t�̥\��b2��3�t���a�E03�t���a �; ��3�t�צcun\� d b�� @��@3�t��)3�t���cun\� a b�� �3�t��47SS�
�8�  
��
�{�  
զ�   �' ���� 1CZ�3�t���\�b2��3�t���cab� �3�t��
���  
��
�{�  ��' ����}CZ�3�t��\��b2��3�t���a�E03�t���a �; ��3�t���cun\� d b�� @��@3�t��)3�t���cun\� a b�� �3�t��47SS�
�8�  
��
�{�  
զ�   ì' ���� �CZ�3�t���\ìb3��3�t�ɬ\ìb2��3�t���ut���3�t���a;@ 3�t�ͭut����3�t���a:@ 3�t��3�t���cabì �3�t��
®�  
��
�{�  ˯' ���� 1CZ�3�t���\˯b2��3�t�Яcab˯ �3�t��
���  
��
�{�  �' ���� UCZ�3�t���ab3��3�t�Ƈvab4��3�t�ư\�b2��3�t���db����cb� �3�t��
���  
��
�Y�  
�{�  Ų' ���� WCZ�3�t���ab3��3�t�Ƈvab4��3�t���\Ųb2��3�t�βdb����cbŲ �3�t��
���  
��
�Y�  
�{�  ��' ����  1CZ�3�t�س\��b2��3�t���cab�� �3�t��
���  �
��
�{�  �' ����!�CZ�Z�3�t���\�b3��3�t���uq�3�t��cZ%";	�c��b F%"
3�t���@ 3�t��3�t���3�t�ɶa�E3�t��� �3�t���ca�da�@T3�t���a�E53�t��� �3�t�۷a�/�% �; ��3�t���ce�de�@�@3�t��)3�t���c�3�t�׸d �; ��3�t���dg��3�t���h";�h\�,"@3�t�ع\�b2��3�t���\ߺg��  � b���3�t��ighb� @ @���3�t������������
���  
��
��  
��� 
���/  
����  
�1�M 
�{�  ���u ����"�3�t��a�
3�t���@ 3�t��3�t���a��  ��3�t���b  3�t�Ҽ3�t���3�t���3�t�Ҽ\��a�� �}  � \�� 3�t���b��b  b��b G)\��@\�� \�� 3�t���b��b G)\��@\�� \ÿ -3�t�ܿ\��-3�t���\��-3�t���b�M�% �; ��3�t���\��c��  � \�} bc� \�� -@Ӗ3�t��%3�t���\��-3�t���\��-@ �3�t�������
���   
���  
�+�  �' ����#  3�t���u����3�t��
��  �'  $   3�t���u �3�t���4' ����%  3�t���ua��
�4�  �=' ����&  CZ�Z�3�t���uabc��
��  
�=�
���='  '  CS.�3�t���uab�3�t��
�8�  
�1��?'  (  "CZ�Z�3�t��>abc&�'�3�t��
��  
�=�
���Y'  )  CQ�3�t���uabcde�3�t��
��  
�Y� 
�Y� 
�7� 
�Y��Z'  *  CQ�3�t���uabc�3�t��
�Z�  
�Y� 
�Z��Z'  +  3�t���ua �3�t��
�1�  �['  ,  3�t���ua"�3�t��
�\�  �^' ���@-  3�t���ua%�3�t��
�4�  �_' ���@.  3�t���ua%�%�3�t��
�4�  �_' ���@/  3�t���ua'�3�t��
�4�  ��' ���@0V3�t���\��-3�t���a �; ��3�t���bH�\�T -@�3�t��3�t���\�U-�3�t�� 88�
�4�  
�+� ���u  1 bCQ�3�t���a�� �3�t���]c�";�b�"3�t�����\��c \�� �@ 3�t��3�t���a]c��3�t��
���  
���
�� ���u ���� 2�3�t���3�t���b";�b\�{�"3�t���b\�{��@3�t��)3�t���w������ a��3�t���c�:%";�cZ%"]3�t���w������ \����3�t���c�:%";�cZ%"3�t���\�4�@ 3�t��%3�t���cw������ a�@ 3�t��3�t���c\�� ��  ��3�t��A3�t����3d�� �@3�t��B��3�t�����\��d �>Z�3�t������>
���  
�� 
��� 
�� 
�+�/ >���u ����3  �3�t���bZ%3�t���� �@ 3�t���b����Z%K3�t���v'3�t���v�� ��  �\�� a b����@3�t��K3�t���ab����@ 3�t���b��=�Z%;3�t���v.3�t���v��=�3�t���a\�H v��=� b��=�@ @ @ 3�t���b���Z%3�t���ab���@ 3�t���v";�vaD�"3�t���b���\�� b���@ 3�t���b�
���  
�� ��' ����4 �3�t���3�t���a";�b"3�t���ab��@ 3�t���dZ%";�d�:%"#3�t���c��Y�3�t���c��Y��@ @ 3�t���dZ%";�d�:%"3�t���\��@ 3�t���c��@��:,";�c��@��E"3�t���cdab�@���b  ��@ 3�t��3�t���d�� �3�t���w������ e��3�t���f�:%";�fZ%";�f\�%""3�t���d�b  ��@ 3�t��3�t���e3�t���;\��&3�t����
�df��@3�t���;\�&3�t���fd�\ �Ŗ3�t���d�b  ��
���  
��� 
�� 
�Y�  
���  
���  �' ����5 3�t���5 3�t���3�t��b�
���  ��� ����  3�t����
�a� ��
®�  ��' ����6 3�t���5 3�t���3�t��b�
���  ��� ����  3�t���� a��
®�  ��' ����7 3�t���5 3�t���3�t��b�
���  ��� ����  3�t���� a��
®�  
���u���8��' ����9�3�t���a�3�t���\��@ 3�t��3�t���\��3�t���a �; ��3�t����8c��3�t���dA3�t���d\�%3�t���c�@ 3�t��N3�t���b\�}d \�� ac� \�]  �@ @��3�t���b\�} �;>���
��  
�4� 
��� 
���   ���u   : .3�t���aF��3�t���aG�aF�3�t���baG��3�t��
®�  
���  ��u ���� ;�3�t���ut����}  �����3�t���F�ba�+�   3�t���Q�3�t���ab��/�% �; ��3�t���d�b  ��}  ������ �F(3�t���^�@ @Ɩ3�t���c�3�t���abb3�t���b;A���@ 3�t���b;A��?a����3�t��MP���
®�  
���  
��� 
���   
�:'  �'
�u�u �T
��u �x�t�
 I�T�x�w ���  G  CZ�3�t���a�3�t��
��� ����  H   3�t���R��x�t3  3�t������'  !  �y' ����  �3��(��c��=��:%3��)��\�c��=�@ 3��*�3��+��\�����3��,��c����#3��-�����c����\��  ����@ 3��/��c����(3��0�����\��c���� \��  ����@ 3��1�3��2��3��4��3��3��3��2��\��3��3��\�� b \�� 3��4��\�� c��m� \�� 3��2������3��6��c��n�'3��7�����\��c��n� \��  ����@,3��8�)3��9�����\��c��=� \��  ����3��<��c��{��   3��=��c�ځ�\3��?�3��>�����3��?�3��>��\��c��{� \�� 3��?�c�ځ� \ƃ  3��>������@,3��@�K3��Ã���\��c��{� \  ����@93��C�c�ځ�(3��D�����\��c�ځ� \��  ����@ 3��F��c��6�'3��G�����\�c��6� \ƃ  ����@ 3��H�3��I�����\�� ����3��Kɇ\��\Ɉ \�] c� \Ԉ ��� \߈  �3��L�
�|�  
�8� 
�� �~' ����  �3��^��c��=��:%3��_��\�~c��=�@ 3��`�3��a͉\����3��b��c����#3��c���c����\��  ���@ 3��e��c����(3��f����\��c���� \��  ���@ 3��g�3��h�3��j��3��i��3��h�\��3��i��\�� b \�� 3��j��\�� c��m� \�� 3��h����3��l��c��n�3��m��\��c��n� \�� �@ 3��p��c��{��   3��q��c�ځ�\3��s�3��r����3��s�3��r��\��c��{� \�� 3��s�c�ځ� \ƃ  3��r�����@,3��t�K3��u����\��c��{� \��  ���@93��w�c�ځ�(3��x����\��c�ځ� \��  ���@ 3��z��c��6�'3��{����\�c��6� \ƃ  ���@ 3��|�3��}ӎ��\�� ���3����\��c��8� \�] c� \�� �� \߈ a \��  �3����
��  
�8� 
�� ��'    13���ސ\֑b \� a \�] c� \��  �3����
�Y�  
΂� 
�� �'    '3���ʒ\��b \�� a \��  �3����
��  
�8� 
�� ��'  	  3���ޓ\����3����
®�  
�� È'  
 g3�����bc%\��@\��3�����3�����3�����\֑a \�� d� e 3�����\�� c \Ǘ  �3����
��  
�Y� 
̗� 
�� 
���  ��'     3�����\�� �3�����'    L3���ߘ\��c� \�] b \��  3�����a\�5%3���֙\�� @ �3����
���  
�1� 
�� �'    )3�����\��b \�] c� \�]  �3����
�|�  
�8� 
�� ߍ'    *3�����\ĝa \�] b� \ϝ  �3����
ҝ�  
�� ��'    63�����\��b� \�� b� \�� a \��  �3����
��  
�� ��'    03����\��b \�] c� \�] a \��  �3����
��  
�8� 
�� ��'    03����\��b \�] c� \�] a \��  �3����
��  
�8� 
�� ��' �����3���ա\��a \�� d� \�]  3�����3����F�3�����b �; ��3�����g�EF3���̣gF�c%\��@\��3�����\��gF� \�] e \�] gG� \��  ?�   3����K3�����g";�g��= "s3����g�M�% �; ��3�����g��= c%\��@\��3�����h\�=,+3�����\��g��=  \�] e \�] gh� \��  @ @��@D3�����3���¨fc%\��@\��3�����\��f \�] e \�] g \��  3����%3�����f;A��?�����3����3�����\�� �3���������DG���
��  
��� 
��� 
�� 
٢� 
���  
����  
���M  ��'    13�����\�b \�� c� \�] a \��  �3����
��  
�� 
�� ��'     3�����\ܬa \�  �3����
®�  
�� ��'  �3���ǭ3����c�E�   3�����c �; ��3�����fb%\��@\��3�����3���د3�����f\�� a \�] d� 3���د\�� f \�� e \°  @��?�   3����)3���Ȱc �; ��3����ch�b%\��@\��3�����3�����3�����h\�� a \�] d� 3�����\�� ch� \�� e \°  @���3����*-��������
��  
�� 
��� 
�� 
��� 
���I  
�.�M  ��'     3�����\��a \��  �3����
�8�  
�� �'     3����\ܬa \��  �3����
®�  
�� ��'     3����\��a \�  �3����
�8�  
�� ˯' �����3�����\ʷ 3���ݷ\�� 3�����a �; ��3�����c�M�% �; ��3���øcd��3����\��e \�� d \ȹ  @͖@��3����3���Թ\� 3�����\�� �3����FIxx�/2{{�
˯�  
�� 
���  
��   
�8�/  ì' �����	C� �3���ͺaZ%";�a�F%"3�����\�� 3������@ 3�����b����3���ϻ\��b���� \��  @ 3����3���Ƽ\��b� \�]  3�����\�� 3�����\ɽ 3���нaF��3�����b\����3�����dT  3���̾d �; ��3�����df��3�����cf��:%";
�g��@ �:%"/3�������\��f \�� cH� �3�����Zdf�@ @��3�����d �; ��3�����hZ%3�����?����@ 3����N3�����3�����dh���� �:%3�����h��  ��@3�����3�����dh���� �3����N3�����dh���� \��dh����  \�] @\��3�����\��j \�] i \��  ?2����?�   3����)3������ �3�����c �; ��3������\�=%";
�b�����"3�����?����@ 3����N3�����\�����  � \��  3������ d��@��3����3�����\�� 3�����F�3�����a �; ��3�����\�� 3�����Z�3�����b����%3�����R��'b�������=�b&��@ 3����%3�����eH�\��@\���3�����d �; ��3������Z%3�����?����@ 3����N3�����d���3�����3�������� 3������\�} ���  �@3�����3�������3����N3�����R��'��\�@���@ �4��3������)3�����\��� \�� � \�� a \��  @(3�����3�����\��� \�� a \��  ?�����3�����3�����e;A��3�����\�� ?1����3�����3�����\�� �3����������������������	�	����	�	�
®�  
��
��� 
��� 
��� 
��I  
����  
��M  
��� 
��� 	 
���  
���� 
�8�� 
���  
���  
����  
���� �'    X3�����3�����3�����\֑a \�� c� \�� c� 3�����\�� b \Ǘ  �3����
��  
�Y� 
�� Ų' ����  �3�����c��������3���������:%3�����V<����@ 3����3�����c��������3���������:%3�����V
����@ 3����3�����3�����3�����\��a \�� c� \�� c� 3�����\�� ��� \�� ��� \�� b \��  �3����
��  
�Y� 
�� ��'    3�����\����3����
®�  
�� ���� ���� k3�����3�����a��� 3�����\���@63�����a��� 3�����\���@3����)3�����\��3�����b�
��   
��� ���� ����  3�����R��'a9��
��  �^�� ����   3�����R��'a-�3����
���  ��'�  ���w ���     F������    W3����\��
3����\��
3����\��a \�� 
3����\��\�� b \�� 
�3���
���  
�=� ì' �����3����\��u;A� �3����aZ%";�a�F%"3����\��
3�� ���@ 3��!��3��"��b\����3��$��\ìc3��&��\��\�� c \�� 
3��'��\��
3��)��aF��3��*��dn3��+��dF�\�=,3��,��dF\�=@ 3��.��F�gd�+73��/��edg��3��0��g;A��@3��1��3��2��dgg@�@63��5�)3��6�����3��7��e �; ��3��8��di@�3��<��d �; ��
3��=��\���
��  � \�� 
@ٖ3��>��3��?��\��a� \�� 
3��A��a �; ��3��B��F�3��C��d �; ��3��D��\��� \�� � \�� a���� \�� 
3��E���;A��@��@��3��G�3��I��\��c \�� 
3��K��b3��L��\��Z3��M��\��Z3��N��\��Z3��O��\��Z3��P��\��Z3��Q��\��Z3��R��\��Z�	�3��K���3��U��\��fH� \�� 
3��W��b����e3��X��\��
3��Y��\��
3��Z��\��R��'b����\�b&� \�� 3��[��\�� 
3��\��\��
@ 3��]�3��_��\��
3��`��\��
3��b��\��c \�� 
�3��c���������������������
���  
�� 
�=� 
��� 
��� 
���  
����  
��  
��� 
 
���� 
���� 
���  ��' �����3��l��\��u;A� �3��n��aZ%";�a�F%"3��o��\��
3��p���@ 3��q��3��s��b\����3��u��\��c3��w��\��\�� c \�� 
3��x��\��
3��z��aF��3��{��F�3��}��\Ñ�3��~��e �; ��3����d";�di
�"g3�����\��g \�� i � \�� 
3�����f;A��3�����fH(3�����?    @ 3����N3�����\���@ ?w����3�����3�����\��a� \�� 
3�����a �; ��
3�����F�3�����a�
� �; ��3�����d";	�d�
�"�   3������F%/3�����\���
 \�� � \�� a�
��� \�� 
@>3������G%/3�����\���
 \�� � \�� a�
��� \�� 
@ 3�����3������;A��@ ?@����?����3����3�����\��c \�� 
3���΁b\��S�\��S�\��^\��Z�	��3�����\��hH� \�� 
3�����\��
3�����\��
3�����\��c \�� 
�3�������������������
���  
�� 
�=� 
��� 
��� 
��� 
��� 
���  
�� 
���� 

���� 
���� ���� ����	m3����� �3�����b �; ��3�����ad�3����ad�cd�@3�����bd�3���ąbd�cd�@ @��3�����c�aa�
��  
��� 
�4� 
���  �^�� ����
  3�����R��'a-�3����
���  
����  