/*
 *	Test finally code is executed without exceptions being thrown.
 */

state = 0

try {
	assert(state == 0)
	state = 1
}

catch {
	assert(0)
	state = 2
}

finally {
	assert(state == 1)
	state = 2
}

assert(state == 2)
