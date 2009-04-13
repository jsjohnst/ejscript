/*
 *	Test defaulting function parameters
 */
function sum0()
{
	return 0
}

function sum1(a)
{
	return a
}

function sum3(a, b, c)
{
	return a + b + c
}

function sum6(a, b, c, d = 14, e = 15, f = 16)
{
	return a + b + c + d + e + f
}

assert(sum0() == 0)
assert(sum1(1) == 1)
assert(sum3(1, 2, 3) == 6)
assert(sum6(1, 2, 3, 4, 5, 6) == 21)
assert(sum6(1, 2, 3, 4) == 41)
assert(sum6(1, 2, 3) == 51)
