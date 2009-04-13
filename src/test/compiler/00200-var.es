/*
 *	Test forward references. This tests the ejs loader fixup mechanism
 */

/* 
 *	Forward base type reference to y
 */
class X extends Y {

 	/*
 	 *	Forward arg type and return type reference to y
 	 */
	function my(arg: Y) : Y
	{
		print("in My")
		return arg.z
	}
}

class Y {
	var filler
	var z : Number = 3
}


var y : Y = new Y

var x = new X

assert(1)


/*
 *	TODO - should test circular references
 */
