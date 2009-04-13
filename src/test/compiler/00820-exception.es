/*
 *	Exception handling with no finally block
 */

state = 0

try {
	state = 1
	throw new Error("Go no further")
	state = 2
}
catch (e: *) {
	assert(state == 1)
	state = 3
}

assert(state == 3)
