/*
 *	Test lazy evaluation of logical operator "||"
 */

funCalled = 0
function fun(): Boolean {
	funCalled = 1
	return true
}

i = 0
assert(0 <= i || i < 10 || fun())

assert(funCalled == 0)
