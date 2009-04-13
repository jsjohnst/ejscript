/*
 *	Function arguments
 */

function fun(a, b)
{
	return "" + a + b
}

/*
 *  In ecma mode, the "b" arg is set to undefined
 *
 *      assert(fun(1) == "1undefined")
 */
assert(fun(1,2) == "12")


function funTyped(a: Number, b: Number)
{
	return "" + a + b
}
assert(funTyped(1,0) == "10")
assert(funTyped(1,2) == "12")

/*
 *  Ejscript catches this in !ECMA mode
 *
 *  assert(funTyped(1,2,3,4) == "12")
 */
