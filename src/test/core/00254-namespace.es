/*
 *	Test namespaces qualification of types, functions and vars
 */
use strict

/*
 *	TODO FIX. Change this to something printable just so we can do doc generation.
 */
// public namespace DEBUG = "http://www.embedthis.com/ns/debug"
public namespace DEBUG = "debug_space"

DEBUG class Shape {
	DEBUG static function size(): Number {
		return 77
	}
}

use namespace DEBUG

assert(Shape.size() ==77)
