/*
 *	Test variable scope: public, private, internal, protected
 */
use strict

public class Shape {

	public static var publicVar : Number = 1
	private static var privateVar : Number = 2
	internal static var internalVar : Number = 3
	protected static var protectedVar : Number = 4

	public function Shape() {
		assert(publicVar == 1)
		assert(privateVar == 2)
		assert(internalVar == 3)
		assert(protectedVar == 4)
	}

	public static function hello() : Number {
		return 7
	}
}

internal class Ellipse extends Shape {
	assert(publicVar == 1)
	assert(internalVar == 3)
	assert(protectedVar == 4)
}

var a : Shape = new Shape


var x : Number

x = Shape.hello()
assert(x == 7)

assert(Shape.publicVar == 1)

//	Fail on access to Shape.internalVar
//	Fail on access to Shape.protectedVar
