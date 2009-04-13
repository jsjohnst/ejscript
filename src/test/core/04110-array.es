/*
 *	Test various array constructors
 */

var a: Array = new Array
assert(a.length == 0)

var b: Array = new Array(10)
assert(b.length == 10)

var c: Array = new Array("one", "two", "three")
assert(c.length == 3)

//	TODO - BUG for empty elements
/*
var d: Array = new Array("one", "two", "three", , "five")
assert(d.length == 5)
*/
