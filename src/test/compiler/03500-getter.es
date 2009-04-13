/*
 *	Simple getter and setter declaration
 */

class Shape {
	private var _width: Number = 7

	function get width(): Number 
	{
		return _width
	}

	function set width(value: Number)
	{
		_width = value
	}
}

s = new Shape

assert(s.width == 7)

s.width = 2

assert(s.width == 2)
