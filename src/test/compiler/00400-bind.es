/*
 *	Variable binding
 */

class X extends Y {

	function my(arg: Y) : Y
	{
		return arg.z
	}
}


class Y {
	var filler
	var z : Number = 3
}
