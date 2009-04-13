/*
 *	Test basic block scope variables inside a function 
 */

var x = 1
assert(x == 1)

function run()
{
	assert(x == undefined)

	var x = 2

	assert(x == 2)
}

assert(x == 1)
run()
assert(x == 1)
