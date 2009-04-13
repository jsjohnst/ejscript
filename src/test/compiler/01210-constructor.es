/*
 *	Constructor and base class constructor test
 */
 
public class Shape
{
	var shapeConstructorRan

	public function Shape()
	{
		shapeConstructorRan = true
	}
}


public class Ellipse extends Shape
{
	var ellipseConstructorRan

	public function Ellipse()
	{
		ellipseConstructorRan = true
	}
}


public class Circle extends Ellipse
{
	var circleConstructorRan
	
	public function Circle()
	{
		circleConstructorRan = true
	}
}

var circle: Circle = new Circle()
assert(circle != null)
assert(circle.shapeConstructorRan)
assert(circle.ellipseConstructorRan)
assert(circle.circleConstructorRan)
