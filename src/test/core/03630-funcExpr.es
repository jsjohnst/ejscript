/*
 *	Function expression recursion
 */

var add = function addFun(x: Number, limit: Number): Number
{
	if (x < limit) {
		return x + addFun(x + 1, limit)
	}
	return x
}

x = add(0, 50)
assert(x == 1275)
