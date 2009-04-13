/*
 *	Nested if statement
 */

if (true) {
	if (false) {
		assert(0)
	} else {
		assert(1)
	}
} else {
	if (false) {
		assert(0)
	} else {
		assert(1)
	}
}
