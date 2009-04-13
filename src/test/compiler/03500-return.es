/*
 *	Test return
 */

function withReturn(x: Number, y: Number): Number
{
	//	Should fail without this return
	return x + y
}


function withoutReturn(x: Number, y: Number)
{
	return 5
}


/*
 *	Ensure we can call function and discard the return value 
 */
assert(withReturn(1, 2) == 3)

withoutReturn(1, 2)
withoutReturn(1, 2)
withoutReturn(1, 2)

assert(withReturn(1, 2) == 3)
assert(withReturn(1, 2) == 3)
