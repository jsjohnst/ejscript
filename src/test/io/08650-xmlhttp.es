/*
 *  Test XMLHttp - Can only be run with a web server on 127.0.0.1 and an xml.html document at the root
 */

const URL: String = "127.0.0.1/xml.html"
var xp: XMLHttp = new XMLHttp
var changeCount: Number = 0

xp.onreadystatechange = function() {
    changeCount++
    assert(0 <= xp.readyState && xp.readyState <= 4)
}

xp.open("GET", URL)

assert(xp.readyState == XMLHttp.Loaded)
assert(xp.status == 200)
assert(xp.statusText == "OK")
assert(xp.responseText.length > 100)
assert(Reflect(xp.responseXML).type == XML)
assert(xp.responseXML.length() == 2)
assert(xp.getAllResponseHeaders().contains("DATE"))

// print("State " + xp.readyState)
// print("Status " + xp.status)
// print("StatusText " + xp.statusText)
// print("Response " + xp.responseText)
// print("Headers " + xp.getAllResponseHeaders())
// print("Response " + xp.responseText)
// print("XML " + xp.responseXML)
