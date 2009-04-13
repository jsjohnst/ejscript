/*
 *	Test switch statement break
 */

function test(x) {

	var total = 0

	switch (x) {

	case 1:
		assert(x == 1)
		total += 1
		break

	case 2:
		assert(x <= 2)
		total += 1
		break

	case 3:
		assert(x <= 3)
		total += 1
		break

	default:
		total += 1
		break
	}

	return total
}

assert(test(1) == 1)
assert(test(2) == 1)
assert(test(3) == 1)
assert(test(4) == 1)
assert(test(99) == 1)
