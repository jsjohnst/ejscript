/*
 *	Test basic namespace directives
 */

public namespace blue
namespace red 							//	TODO = "http://red"

blue var x = "blue"
red var x = "red"
public var x = "public"

use namespace red
assert(x == "red")

use default namespace red
