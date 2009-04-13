/*
 *	Basic break from a nested if inside a for loop
 */

for (var i = 0; i < 10; i++) {
	if (i >= 5) {
		break
	}

}
assert(i == 5)
