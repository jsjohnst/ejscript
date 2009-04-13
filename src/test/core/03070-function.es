/*
 *  Test calling functions with various scopes
 */

/*
 *  Global functions
 */
var funDone = false
function fun(a): Number {
    return a
}
assert(fun(1) == 1)
assert(global.fun(1) == 1)


/*
 *  Setup some classes
 */
class Shape {

    static var x = 2
    static function sfun(arg) {
        assert(this == Shape)
        return arg
    }
}
assert(Shape.x == 2)


class Circle extends Shape {

    static function cfun(arg) {
        x = 7
        assert(this == Circle)
        return sfun(arg)
    }

    function ifun(arg) {
        assert(this == circ)
        arg = cfun(arg)
        return sfun(arg)
    }
}

var circ : Circle = new Circle

assert(			Circle.cfun(1)                      == 1)
assert(			Shape.sfun(2)                       == 2)
assert(			Circle.sfun(3)                      == 3)
assert(			circ.sfun(4)                        == 4)
assert(			circ.cfun(5)                        == 5)
assert(			circ.ifun(6)                        == 6)
assert(			global.circ.ifun(7)                 == 7)
assert(			global.circ["if" + "un"](8)         == 8)
assert(			Shape["sf" + "un"](9)               == 9)

var fn = Shape.sfun
assert(         fn(10)                              == 10)

/*
 *  These will be call by name because instance is untyped
 */
var instance = circ
instance.cfun(1) == 1
instance.sfun(2) == 2
instance.ifun(3) == 3

assert(Shape.x == 7)
