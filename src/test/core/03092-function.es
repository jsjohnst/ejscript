/*
 *	Various function invocation styles. Will use different op codes
 */


/*
 *	Global call (CALL_BY_SLOT_FROM_GLOBAL)
 */
function add(x: Number, y: Number): Number
{
	return x + y
}
assert(add(1, 2) == 3)


/*
 *	Call on a class (CALL_STATIC_BY_SLOT_FROM_OBJ)
 */
class Shape {
	static function render() {
		return 77
	}
}
assert(Shape.render() == 77)



/*
 *	Call an instance method (CALL_BY_SLOT_FROM_OBJ)
 */
class Circle {
	function render() {
		return 88
	}
}
var c: Circle = new Circle
assert(c.render() == 88)



/*
 *	Call by name (CALL_BY_NAME_FROM_OBJ)
 */

var f: Function = add
assert(f(1, 2) == 3)


/*
 *	Call inside a class (CALL_BY_SLOT_FROM_THIS)
 */
class Square {
	function place() {
		return origin()
	}
	function origin() {
		return 23
	}
}
var s: Square = new Square
assert(s.place() == 23)
