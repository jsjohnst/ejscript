/*
 *	For without initializer
 */

i = 0;
for (; i < 10; i = i + 1) {
	assert(i < 10)
}
assert(i == 10)
