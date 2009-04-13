/*
 *	if statement with function expressions as operands
 */

function fun()
{
	return true
}

if (fun()) {
	assert(1)
} else {
	assert(0)
}
