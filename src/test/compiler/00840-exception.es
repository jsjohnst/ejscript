/*
 *	Test throw of primitive types
 */

state = 1
try {
	assert(state == 1)

	throw (1 + 2)

	state = 2
}

catch (e: Number) {
	assert(e == 3)
	assert(state = 1)
	state = 3
}

assert(state == 3)


/*
 *	Now with a string
 */
state = 1
try {
	assert(state == 1)

	throw "Big throw"

	state = 2
}

catch (e: String) {
	assert(e == "Big throw")
	assert(state = 1)
	state = 3
}

assert(state == 3)
