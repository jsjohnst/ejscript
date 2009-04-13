/*
 *	Test use of this
 */
class MyClass {

	public var x: Number = 1

	function fun() : Object {
		assert(x == 1)
		assert(this.x == 1)

		this.x = 3

		assert(x == 3)
		assert(this.x == 3)

		return this
	}

}

var my: MyClass = new MyClass

returnValue = my.fun()
assert(returnValue == my)
