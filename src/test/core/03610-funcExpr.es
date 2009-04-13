/*
 *	Function expression with local variables
 */

var fun = function run() : Number {
	var localVar: Number = 3

	assert(localVar == 3)

	localVar = 1

	assert(localVar == 1)
	return localVar
}


assert(fun() == 1)

assert(global.localVar == undefined)
