/*
 *	Basic continue from a nested if inside a for loop
 */

for (var i = 0; i < 10; i++) {

	if (i <= 5) {
		continue
		i = 77
	}
	if (i <= 5) {
		assert(0)
	}
}
assert(i == 10)
