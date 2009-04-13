/*
 *	Exception in finally
 */

state = 1

function one() {

	try {
		assert(state == 1)
		state = 2
		throw new Error("Hello World")
		assert(0)
	}

	catch {
		assert(state == 2)
		state = 3
	}

	finally {
		assert(state == 3)
		state = 4

		throw "Go no further"
		assert(0)
	}

	assert(0)
}

try {
	assert(state == 1)
	one()
	assert(0)
}

catch {
	assert(state == 4)
	state = 5
}

assert(state == 5)
