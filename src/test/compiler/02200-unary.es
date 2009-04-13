/*
 *	Unary not
 */

var x = 0
assert(!x)

x = 1
assert(!x == false)

x = 5
assert(x)
assert(false == !x)

y = !x
assert(!y)
