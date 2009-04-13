/*
 *	Test basic array usage
 */

var a = new Array()

a[0] = 5
a[1] = "Hello World"
a[2] = true

assert(a.length == 3)
assert(a[0] == 5)
assert(a[1] == "Hello World")
assert(a[2] == true)
