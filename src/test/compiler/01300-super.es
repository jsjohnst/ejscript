/*
 *	Super method
 */
 
public class Ellipse 
{
	var ellipseConstructorRan = 0

	public function Ellipse()
	{
		ellipseConstructorRan++
	}
}


public class Circle extends Ellipse
{
	var circleConstructorRan = 0
	
	public function Circle(arg)
	{
		super()
		circleConstructorRan++
	}
}

var circle: Circle = new Circle("top")

assert(circle != null)
assert(circle.circleConstructorRan == 1)
assert(circle.ellipseConstructorRan == 1)
