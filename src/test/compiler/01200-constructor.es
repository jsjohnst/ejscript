/*
 *	Basic constructor test
 */
 
public class Shape
{
	var constructorRan

	public function Shape()
	{
		constructorRan = true
	}
}


var shape: Shape = new Shape

assert(shape != null)
assert(shape.constructorRan)
