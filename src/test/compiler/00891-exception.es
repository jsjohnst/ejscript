/*
 *	Test exception nested inside an if 
 */

var counter = 0

assert(counter == 0)

if (true) {
	assert(counter++ == 0)
	try {
		throw "My Exception"
		assert(0)

	} catch {
		assert(counter++ == 1)

	} finally {
		assert(counter++ == 2)
	}
	assert(counter++ == 3)
}

assert(counter == 4)
