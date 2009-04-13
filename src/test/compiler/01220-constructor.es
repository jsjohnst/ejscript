/*
 *	Constructor test with args
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
	
	public function Circle(x, y)
	{
		circleConstructorRan = x + y
	}
}

var circle: Circle = new Circle(2, 3)
assert(circle != null)
assert(circle.shapeConstructorRan)
assert(circle.ellipseConstructorRan)
assert(circle.circleConstructorRan == 5)
