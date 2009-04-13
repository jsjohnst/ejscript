/*
 *	Assignment of logical operators
 */

funCalled = 0
function fun(): Boolean {
	funCalled = 1
	return true
}

i = 3

x = i && 4
assert(x == true)

i = 0
funCalled = 0
x = i || i != 0 || fun()
assert(x == true)
assert(funCalled)


/* 
 *	Test lazy evaluation
 */
i = 0
funCalled = 0
x = i || i == 0 || fun()
assert(x == true)
assert(! funCalled)

i = 0
funCalled = 0
x = i == 0 && i < 4 && "Hello World" &&  fun()
assert(x == true)
assert(funCalled)

i = 0
funCalled = 0
x = i == 0 && i < 4 && false &&  fun()
assert(x == false)
assert(! funCalled)
