/*
 *	Default namespace definitions
 */

var x = "outside"
assert(x == "outside")

{
	use default namespace "embedthis.com" 
	var x = "inside"

	use namespace "embedthis.com"
	assert(x == "inside")
}

assert(x == "outside")

use namespace "embedthis.com"
assert(x == "inside")
