/*
 *	Three types of global variable access
 */

/*
 *	Unbound by name
 */
x = 1
assert(x == 1)

/*
 *	Unbound by expression
 */
x = new Object
assert(x)

x[1 + 1] = 2
assert(x[2] == 2)


var y: Number
assert(y == null)

/*
 *	Bound by slot
 */
y = 3
assert(y == 3)


/*
 *	Now the loads
 */
a = x
assert(a != null)
a = x[1 + 1]
assert(a == 2)
a = y
assert(a == 3)
