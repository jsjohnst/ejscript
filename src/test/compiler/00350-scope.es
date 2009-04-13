/*
 *	New variables in EJS mode should be made locals
 */

function fun() {
	x = 2
	assert(x == 2)
}

fun()

//	TODO - should have an ECMA variable we can test and if so, this test can run in --ecma mode
assert(global.x == undefined)
