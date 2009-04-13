/*
 *	Test switch statement without break
 */

function test(x) {

	var total = 0

	/*
	 *	All cases are fall through. Keep a "total" of the number of cases executed
 	 */
	switch (x) {

	case 1:
		assert(x == 1)
		total += 1

	case 2:
		assert(x <= 2)
		total += 1

	case 3:
		assert(x <= 3)
		total += 1

	default:
		total += 1
	}

	return total
}

assert(test(1) == 4)
assert(test(2) == 3)
assert(test(3) == 2)
assert(test(4) == 1)
assert(test(99) == 1)
