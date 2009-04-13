/*
 *	Constructor initializers
 */
class Shape {

	var total: Number = 0

	function Shape(a, b, c) 
	{
		total += a
		total += b
		total += c
	}
}

class Circle extends Shape {
	function Circle() :
		super(1, 2, 3)
	{
	}
}

var c = new Circle

assert(c.total == 6)
