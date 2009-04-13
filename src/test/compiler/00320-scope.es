/*
 *	Test block scope variables inside a class
 */

class My {
	static var x = 1
	assert(x == 1)

	if (true) {
        /*
         *  BUG --nobind will find x == 1 as it will do a search by scope
         *
		 *   assert(x == undefined)
         */

		let x = 2

		assert(x == 2)
	}

	assert(x == 1)
}
