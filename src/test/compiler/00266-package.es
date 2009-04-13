/*
 *	Declare namespace package variables with various visibilities
 */

var defaultInternal = 0

{
	use default namespace "Embedthis"

	public var a = 1

	internal var b = 2

	var c = 3
}

assert(a == 1)
assert(b == 2)

use namespace "Embedthis"
assert(c == 3)
