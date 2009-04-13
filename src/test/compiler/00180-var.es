/*
 *	Variables declared in the class hierarchy
 */
class Shape 
{
	var a : Number
}


/*
 *	Forward reference to Circle
 */
class Ellipse extends Circle 
{
	var c

	function f() {
		a = 1
		b = 2
		c = 3
	}
}


class Circle extends Shape 
{
	var b
}

var e : Ellipse = new Ellipse
assert(e)

e.f()
assert(e.a == 1)
assert(e.b == 2)
assert(e.c == 3)
