/*
 *	Test scope of function argument lookups.
 */

var a = 1

function fun(a = a)
{
	assert(a == 1)
	a = 2
}

fun()
assert(a == 1)
