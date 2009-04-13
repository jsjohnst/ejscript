/*
 *	Test finally code is executed when catch throws an exception
 */

state = 0

try {
	try {
		assert(state == 0)
		state = 1

		throw "Go no further"
		assert(0)
	}

	catch {
		assert(state == 1)
		state = 2
		throw new Error("Catch throws")
		assert(0)
	}

	finally {
		assert(state == 2)
		state = 3
	}
}

catch {
	assert(state == 3)
	state = 4
}

finally {
	assert(state == 4)
	state = 5
}

assert(state == 5)
