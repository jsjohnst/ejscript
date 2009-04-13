/*
 *	For without conditional and per-loop
 */

for (i = 0; ; ) {
	assert(i <= 10)
	if (++i >= 10) {
		break
	}
}
assert(i == 10)
