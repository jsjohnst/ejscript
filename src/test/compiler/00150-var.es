/*
 *	Three types of property variable access in global space
 */

var a : Object
a = new Object
assert(a)

/*
 *	Unbound by name
 */
a.b = 1
assert(a.b == 1)

/*
 *	Unbound by expression
 */
a[1 + 1] = 2
assert(a[2] == 2)


class MyClass {
	var a: Number
	var aa: Number
	var b: String
	var c: Number
}
var my : MyClass
my = new MyClass
assert(my)
assert(my.a == null)

/*
 *	Bound by slot
 */
my.a = 3
assert(my.a == 3)

my.aa = 4
assert(my.aa == 4)

my.b = 5
assert(my.b == 5)

my.c = "Hello World"
assert(my.c == "Hello World")

/*
 *	Now for some stores
 */

x = a.b
assert(x == 1)

x = a[1 + 1]
assert(x == 2)

x = my.a
assert(x == 3)

x = my.b
assert(x == 5)
