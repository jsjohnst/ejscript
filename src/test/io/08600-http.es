
const URL: String = "127.0.0.1/index.html"
var http: Http

//
//  Basic usage
//
http = new Http(URL)
http.get()
assert(http.code == 200)
assert(http.headers.length > 2)
assert(http.contentLength > 10)
assert(http.contentType == "text/html")
assert(http.date)
assert(http.url == "http://" + URL)
assert(http.available == http.contentLength)
assert(http.method == "GET")
assert(http.lastModified)
//
// Unset
//    print("RESPONSE ENCODING " + http.contentEncoding)
//    print("RESPONSE EXPIRES " + http.expires)
//

//
//  Url specified in connector method
//
http = new Http
http.get(URL)
assert(http.code == 200)

//
//  Url specified in via property
//
http = new Http
http.url = URL
http.get()


//
//  Test form post
//
http = new Http
http.addRequestHeader("Content-Type", "application/x-www-form-urlencoded")
http.form(URL, {name: "Michael", address: "747 Park Lane" })

//
//  Test authorization
//
http = new Http
http.setCredentials("mob", "dog")
http.get("127.0.0.1/acme/index.html")
assert(http.code == 200)

//
//  Test raw post data
//
http = new Http(URL)
http.post(null, "Just a string of data")

//
//  With follow redirects
//
http = new Http("127.0.0.1/sub")
http.get()



//
//  XML
//
http = new Http("http://127.0.0.1/xml.html")
http.get()
var xml: XML = http.readXml()
assert(xml.customer.name == "Joe Green")

//
//  Read as string
//
str = Http("http://127.0.0.1/index.html").readString()
assert(str.length > 100)
assert(str.contains("<html"))


//
//  Read as lines
//
lines = Http(URL).readLines()
assert(lines.length > 10)

//
//  Read raw
//
http = new Http(URL)
http.connect()
ba = new ByteArray(-1, true)
assert(http.available > 0)
assert(http.available == http.contentLength)
var count = http.read(ba, 0, -1)
assert(count == http.contentLength)
assert(http.available == 0)

//
//  Read in async mode via a callback
//
function callbackTest() {

    var http: Http = new Http()
    var complete: Boolean = false
    var received: Number = 0

    http.callback = function(e: Event) {
        if (e is HttpError) {
            assert(e == null)
            return
        }
        assert(e is HttpDataEvent)
        let http: Http = e.data

        buffer = new ByteArray
        count = http.read(buffer)
        if (count == 0) {
            complete = true
        }
        received += count
    }

    http.postLength = 30
    http.post(URL)
    for (i in 3) {
        http.write("012345678\r\n")
    }

    let timeout = 5 * 1000
    when = new Date
    do {
        App.serviceEvents(-1, timeout)
    } while (complete != true && when.elapsed < timeout)

    assert(received > 0)
}
callbackTest()


//
//  Using a response stream
//
http = new Http
var ts: TextStream = new TextStream(Http("http://127.0.0.1/index.html").responseStream)
assert(ts.readLines().length > 10)

/*
//
//  SSL - crashes on memory free
//
http = new Http("https://127.0.0.1/index.html")
http.get()
print("Secure: " + http.isSecure)
print(http.code)
*/


/*
 *  close to flush
 *   new Http("127.0.0.1").readStream
 */
