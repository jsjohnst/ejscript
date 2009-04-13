/*
 *	Function with return expressions
 */

function add(x: Number, y: Number): Number
{
	if (x == 1) {
		return x + y
	}
	if (x == 2) {
		return (x + y)
	}
	if (x == 3) {
		return x + (y)
	}
	return 0
}

assert(add(1, 2) == 3)
assert(add(2, 2) == 4)
assert(add(3, 2) == 5)
