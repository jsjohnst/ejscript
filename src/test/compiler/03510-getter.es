/*
 *	Getter and setter access via typed and untyped vars
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

var s2: Shape = new Shape
assert(s2.width == 7)
s2.width = 3
assert(s2.width == 3)
assert(s.width == 7)

