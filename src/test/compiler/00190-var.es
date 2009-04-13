/*
 *	Integrated test of variable access. This tests static and instance properties in various 
 *	contexts: global, local, class.
 *
 *	This does not fully cover the cases where properties and object references are untyped.
 */

var globalVar = 3

assert(globalVar == 3)

class Shape 
{
	var pos : String = "shapePos"
	public static var count : String = "shapeCount"

	function shapeMethod() {
		pos = "shapePosUpdate"
		count = "shapeCountUpdate"
	}
}


class Ellipse extends Shape
{
	var radius : String = "ellipseRadius"
	public static var len : String = "ellipseLength"

	function ellipseMethod() {
		pos = "ellipsePosUpdate"
		count = "ellipseCountUpdate"
		radius = "ellipseRadiusUpdate"
		len = "ellipseLengthUpdate"
	}
}

var s: Shape = new Shape()

/*
 *	Initial non-updated values
 */
assert(s != null)
assert(s.pos == "shapePos")
assert(s.count == "shapeCount")
assert(Shape.count == "shapeCount")

/*
 *	After method, values should be updated
 */
s.shapeMethod()
assert(s.pos == "shapePosUpdate")
assert(s.count == "shapeCountUpdate")
assert(Shape.count == "shapeCountUpdate")

/*
 *	Test expressions
 */
var name: String = "count"

assert(s["pos"] == "shapePosUpdate")
assert(s["count"] == "shapeCountUpdate")
assert(s[name] == "shapeCountUpdate")
assert(Shape["count"] == "shapeCountUpdate")
assert(Shape[name] == "shapeCountUpdate")


/* 
 *	Now test a derrived class
*/
var e : Ellipse = new Ellipse
assert(e != null)

/*
 *	Before running shapeMethod, e.pos has a non-updated value
 */
assert(e.pos == "shapePos")
assert(e.count == "shapeCountUpdate")
assert(Shape.count == "shapeCountUpdate")
e.shapeMethod()
assert(e.pos == "shapePosUpdate")
assert(e.count == "shapeCountUpdate")
assert(e.radius == "ellipseRadius")
assert(Shape.count == "shapeCountUpdate")
assert(Ellipse.count == "shapeCountUpdate")
assert(Ellipse.len == "ellipseLength")

e.ellipseMethod()
assert(e.pos == "ellipsePosUpdate")
assert(e.count == "ellipseCountUpdate")
assert(e.radius == "ellipseRadiusUpdate")
assert(e.len == "ellipseLengthUpdate")
assert(Shape.count == "ellipseCountUpdate")
assert(Ellipse.count == "ellipseCountUpdate")
assert(Ellipse.len == "ellipseLengthUpdate")

assert(e["pos"] == "ellipsePosUpdate")
assert(e["count"] == "ellipseCountUpdate")
assert(e[name] == "ellipseCountUpdate")
assert(Ellipse["count"] == "ellipseCountUpdate")
assert(Ellipse[name] == "ellipseCountUpdate")
