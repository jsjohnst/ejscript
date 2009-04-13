/*
 *	if statement with integer expressions
 */

x = 5;

if (x == 5) {
	assert(1)
} else {
	assert(0)
}

if (x === 5) {
	assert(1)
} else {
	assert(0)
}

if (x != 5) {
	assert(0)
} else {
	assert(1)
}

if (x !== 5) {
	assert(0)
} else {
	assert(1)
}

if (x < 7) {
	assert(1)
} else {
	assert(0)
}

if (x <= 5) {
	assert(1)
} else {
	assert(0)
}

if (x > 3) {
	assert(1)
} else {
	assert(0)
}

if (x >= 3) {
	assert(1)
} else {
	assert(0)
}
