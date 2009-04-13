/*
 *	Multiple catch blocks
 */

state = 0

try {
	state = 1
	throw 77
}
catch (e: string) {
	assert(0)
}
catch (e: Number) {
	assert(state == 1)
	state = 3
}
catch (e: *) {
	assert(0)
}
assert(state == 3)
