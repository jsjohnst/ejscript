/*
 *	Test with
 */

class Shape {
	var x
	var y

	function Shape(x, y) {
		this.x = x
		this.y = y
	}
}


/*
 *	unbound access
 */
s = new Shape(1, 2)
with (s) {
	assert(x == 1)
}


/*
 *	Bound access
 */
var s: Shape = new Shape(1, 2)
with (s) {
	assert(x == 1)
}

