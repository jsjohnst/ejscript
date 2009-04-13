/*
 *	Test namespace scope
 */

public namespace blue
namespace red 							//	TODO = "http://red"

blue var x = "blue"
red var x = "red"
public var x = "public"

assert(x == "public")

{
	use namespace red
	assert(x == "red")
}
{
	use namespace blue
	assert(x == "blue")
}

/*
 *	Now test ordering. Pragmas only take effect from the point in the block onwards
 */

use namespace blue
assert(x == "blue")
use namespace red
assert(x == "red")

