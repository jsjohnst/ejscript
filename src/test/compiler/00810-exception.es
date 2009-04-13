/*
 *	Nested exception handling
 */

state = 0

function one() {
	try {
		state = 1
		two()

		assert(0)
		state = 99
	}
	catch {
		assert(state == 3)
		state = 4
	}
	finally {
		assert(state == 4)
		state = 5
	}
}


function two() {

	assert(state == 1)
	state = 2

	three()

	/* Should never get here */
	assert(0)
}


function three() {

	assert(state == 2)
	state = 3

	throw "Go no further"

	state = 99
}

assert(state == 0)
one()
assert(state == 5)
