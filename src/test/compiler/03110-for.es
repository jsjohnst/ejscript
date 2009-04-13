/*
 *	Loop with a short body
 */

for (i = 0; i < 10; i = i + 1) {
	assert(i < 10)
}
assert(i == 10)
