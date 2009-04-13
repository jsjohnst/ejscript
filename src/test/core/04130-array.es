/*
 *	Test array literals
 */

var a: Array = [ "hello", "there" ]
assert(a.length == 2)

var b: Array = [ "hello", "there", [ "cruel", "world" ] ]
assert(b.length == 3)

var c: Array = [ "red", , "green", ,  ]
assert(c[0] == "red")
assert(c[1] == null)
assert(c[2] == "green")

/* 
 *	BUG
 *		assert(c[3] == null)
 *		assert(c[4] == null)
 *		assert(c.length == 5)
 */
