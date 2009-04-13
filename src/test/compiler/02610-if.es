/*
 *	if statement with floating expressions
 */

var x : double = 5.0

if (x == 5.0) {
	assert(1)
} else {
	assert(0)
}

if (x === 5.0) {
	assert(1)
} else {
	assert(0)
}

if (x != 5.0) {
	assert(0)
} else {
	assert(1)
}

if (x !== 5.0) {
	assert(0)
} else {
	assert(1)
}

if (x < 7.0) {
	assert(1)
} else {
	assert(0)
}

if (x <= 5.0) {
	assert(1)
} else {
	assert(0)
}

if (x > 3.0) {
	assert(1)
} else {
	assert(0)
}

if (x >= 3.0) {
	assert(1)
} else {
	assert(0)
}
