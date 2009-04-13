/*
 *	Super with args
 */
 
public class Shape
{
	var shapeConstructorRan

	public function Shape(a, b, c)
	{
		assert((a + b + c) == 6)
		shapeConstructorRan = true
	}
}


public class Ellipse extends Shape 
{
	var ellipseConstructorRan

	public function Ellipse(a, b, c)
	{
		assert((a + b + c) == 6)
		super(a, b, c)
		ellipseConstructorRan = true
	}
}


public class Circle extends Ellipse
{
	var circleConstructorRan
	
	public function Circle(a, b, c)
	{
		assert((a + b + c) == 6)
		super(a, b, c)
		circleConstructorRan = true
		assert(circleConstructorRan)
	}
}

var circle: Circle = new Circle(1, 2, 3)

assert(circle != null)
assert(circle.circleConstructorRan == true)
assert(circle.ellipseConstructorRan)
assert(circle.shapeConstructorRan)
