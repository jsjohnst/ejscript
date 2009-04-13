/*
 *	Invoke a function expression in various scopes
 */

var globalFn = function (s: string)
{
	return "globalFunction"
}

class MyClass {

	var classFn = function (s: string) {
		return "classFunction"
	}

    var callback: Function
    function test() {
        return callback()
    }
}


var my: MyClass = new MyClass
assert(my)

globalFn("global")
assert(globalFn("") == "globalFunction")

my.classFn("class")
assert(my.classFn("") == "classFunction")


//
//  Test calling via instance variable from inside the class
//
my.callback = function () {
    return 77
}
assert(my.test() == 77)
