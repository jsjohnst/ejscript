/*
 *	Function recursion
 */

function add(x: Number, limit: Number): Number
{
	if (x < limit) {
		return x + add(x + 1, limit)
	}
	return x
}

x = add(0, 50)
assert(x == 1275)
