/*
 *	Simple property reference. Unbound.
 */

var o: Object

assert(o == null)

o = new Object()
assert(o != null)

/*
 *	NOTE: Compiler can't bind to o.b as we don't know it's type or location.
 */
o.b = 2
assert(o.b == 2)
