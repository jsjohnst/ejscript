/*
 *	Function declaration inside a class
 */

class Shape {

	function add(x: Number, y): Number
	{
		return x + y
	}
}

var s : Shape = new Shape

assert(s)
s.add(1, 2)

assert(s.add(1, 2) == 3)
