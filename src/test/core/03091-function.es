/*
 *	Function default args
 */

function add(x: Number, y: Number = 0): Number
{
	return x + y
}

assert(add(0, 50) == 50)
assert(add(50, 0) == 50)
assert(add(50) == 50)
