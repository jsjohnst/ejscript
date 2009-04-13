/*
 *	Basic exception handling
 */

state = 0

try {
	state = 1
	throw new Error
	state = 2
}
catch {
	assert(state == 1)
	state = 3
}
finally {
	assert(state == 3)
	state = 4
}

assert(state == 4)
