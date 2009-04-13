/*
 *  Test function capture closures
 */

class Shape {
    var x = 7
    public function fun(): Number {
        //  Testing that the scope is captured
        return x
    }
}

class MyShape extends Shape { }

s = new MyShape
x = s.fun
assert(x() == 7)


/*
 *  Similar test, but testing if the args accessible
 */

class View {
    function fun(fmt: String) {
        assert(fmt == "fmt")
        return function (data: String) {
            assert(fmt == "fmt")
            return data.toUpper()
        }
    }
}

v = new View
x = v.fun("fmt")
assert(x("abc") == "ABC")
