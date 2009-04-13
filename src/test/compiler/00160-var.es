/*
 *	Multiple definitions in a single var
 */

var a: Number, b: string, c: double

//	TODO - this is a known bug. Numerics should initialze to zero
assert(a == null)
assert(b == null)
assert(c == null)
