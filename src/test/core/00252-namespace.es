/*
 *	Test namespaces explicit variable qualification
 */

public namespace blue
namespace red 							//	TODO = "http://red"

blue var x = "blue"
red var x = "red"
public var x = "public"

assert(blue::x == "blue")
assert(red::x == "red")
assert(x == "public")
