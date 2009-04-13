/*
 *	Test throw
 */

state = 1
try {
	assert(state == 1)

	throw new Error("Hello World")

	state = 2
}


catch {
	assert(state = 1)
	state = 3
}

assert(state == 3)
