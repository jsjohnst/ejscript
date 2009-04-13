/*
 *	Tests for the App class
 */

use namespace "ejs.sys"

/*
 *	dir -- Test the App dir contains at least a "/"
 */
assert(App.dir.search("/") >= 0)
assert(App.dir.search("%") < 0)

/*
 *	args
 */ 
assert(App.args != "")
assert(App.args.length > 0)

/*
 *	sleep
 */
d = new Date
App.sleep(100)
elapsed = new Date - d

/*
 *	TODO - when we have floor, use the following
 *	assert(((elapsed + 50) / 10).floor * 10 == 200)
 */
