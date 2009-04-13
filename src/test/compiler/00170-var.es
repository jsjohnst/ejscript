/*
 *	Variable scoping
 */

var globalVar = 1

class Shape {

	static var classVar = 3

	var instanceVar = 4

	function run(argVar: Number)
	{
		var localVar

		assert(globalVar == 1)
		assert(classVar == 3)
		assert(argVar == 55)
		assert(localVar == 66)
		assert(instanceVar == 4)

		globalVar = 11
		classVar = 33
		instanceVar = 44
		argVar = 55
		localVar = 66

		assert(argVar == 55)
		assert(localVar == 66)
		assert(instanceVar == 4)
	}

	assert(globalVar == 1)
	assert(classVar == 3)
}
