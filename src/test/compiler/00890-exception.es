/*
 *	Test exception nested inside a switch
 */

var counter = 0

assert(counter == 0)

switch(1) {
case 1:
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
