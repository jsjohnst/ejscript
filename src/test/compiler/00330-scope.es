/*
 *	Multilevel block scope test
 */

/* TODO ZZZ
assert(x == undefined)

assert(x == undefined)
var x = 1
assert(x == 1)

class My {
	var x = 2

	function fun() {
		assert(x == undefined)
		var x = 3
		assert(x == 3)

		if (true) {
print(x)
			assert(x == undefined)
			let x = 4
print(x)
			assert(x == 4)

			if (true) {
				assert(x == undefined)
				let x = 6
				assert(x == 6)
			}
			assert(x == 4)
		}
		assert(x == 3)
	}
}
assert(x == 1)

var c: My = new My
assert(x == 1)

c.fun()
assert(x == 1)

assert(x == undefined)
*/
