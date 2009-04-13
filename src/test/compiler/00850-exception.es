/*
 *	Test re-throw
 */

state = 1

function two() {

	assert(state == 2)

	try {
		assert(state == 2)
		state = 3

		/*
		 *	Throw first error
		 */
		throw new Error("Hello World")

		/* Should not get here */
		state = 99
	}

	catch {
		assert(state = 3)
		state = 4

		/*
		 *	Rethrow the exception
		 */
		throw new Error("Rethrow")

		/* Should not get here */
		state = 98
	}

	finally {
		assert(state == 4)
		state = 5
	}
}


function one() {

	assert(state == 1)

	try {
		state = 2
		two()

		/* Should not get here */
		state = 99
	}
	catch {
		assert(state == 5)
		state = 6
	}
	finally {
		assert(state == 6)
		state = 7
	}

	assert(state == 7)
	state = 8
}

assert(state == 1)
one()
assert(state == 8)

