/*
 *	Function expression inside a class
 */

public var outerFun = function (x: Number, y): Number {
	return x + y
}

class Shape {

	public var innerFun = function (x: Number, y): Number {
		return x + y
	}
}

var s : Shape = new Shape

assert(s)
s.innerFun(1, 2)

assert(s.innerFun(1, 2) == 3)

outerFun(1, 2)
assert(outerFun(1, 2) == 3)
