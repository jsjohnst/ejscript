/*
 *	Function declaration with local variables
 */

function run() : Number
{
	var localVar: Number = 3

	assert(localVar == 3)

	localVar = 1

	assert(localVar == 1)
	return localVar
}


assert(run() == 1)

assert(global.localVar == undefined)
