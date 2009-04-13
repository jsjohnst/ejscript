/*
 *	Simple variable visibility. This does not cover all the bases. Later unit tests will do that.
 */

var outside

public var a = 1

internal var b = 2

var d = 4

assert(outside == null)
assert(a == 1)
assert(b == 2)
assert(d == 4)

function test() 
{
	assert(outside == null)
	assert(a == 1)
	assert(b == 2)
	assert(d == 4)
}

test()

assert(outside == null)
assert(a == 1)

/*
 *	TODO - add more code to test visibility harder
 */
