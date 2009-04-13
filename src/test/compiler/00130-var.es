/*
 *	Bindable Property lookup
 */

class MyClass {
	public var b: Number
}

var a : MyClass
assert(a == null)

a = new MyClass
assert(a)

/*
 *	We know b's location and type so we can bind to it.
 */
a.b = 2
assert(a.b == 2)

