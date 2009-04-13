/*
 *	Empty for-in loop
 */

for (i in new Array) {
}
assert(global.i == null)


/*
 *	Simple iteration over ten elements. But this will actually iterate over none as all elements are null.
 */
count = 0
for (i in new Array(10)) {
	count++
}
assert(count == 0)


/*
 *	Iterate over an object
 */
count = 0
for (i in { "one": 1, "two": 2 })
	count++
assert(count == 2)


/*
 *	For in with let
 */
count = 0
assert(global.lx == undefined)
for (let lx in [1, 2, 3, 4, 5]) {
	assert(lx == count++)
}
assert(count == 5)
assert(global.lx == undefined)


/*
 *	For/in with var
 */
count = 0
assert(x == undefined)
for (var vx in [1, 2, 3, 4, 5]) {
	assert(vx == count++)
}
assert(count == 5)
assert(vx == 4)


/*
 *	For/in with unspecified var (defaults to let)
 */
count = 0
assert(global.ux == undefined)
for (ux in [1, 2, 3, 4, 5]) {
	assert(ux == count++)
}
assert(count == 5)
assert(global.ux == undefined)


/*
 *	Break
 */
count = 0
for (x in [1, 2, 3, 4, 5]) {
	count = 11
	break
	count = 99
}
assert(count == 11)


/*
 *	Continue
 */
evenCount = 0
for (var x in [1, 2, 3, 4, 5]) {

	if ((x % 2) == 1) {
		continue
	}
	evenCount++
}
assert(x == 4)
assert(evenCount == 3)

