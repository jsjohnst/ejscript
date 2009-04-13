/*
 *	Function expression with arguments
 */

fun = function (x: Number, y: String): Number {
	assert(x == 1)
	assert(y == "point")

	x = 3
	return x
}

assert(fun(1, "point") == 3)
