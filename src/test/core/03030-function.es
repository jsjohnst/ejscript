/*
 *	Function declaration with arguments
 */

function run(x: Number, y: string): Number
{
	assert(x == 1)
	assert(y == "point")

	x = 3
	return x
}

assert(run(1, "point") == 3)
