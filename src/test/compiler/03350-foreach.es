/*
 *	Basic for/each test on arrays
 */

count = 0
str = ""
for each (x in [ "red", "blue", "yellow" ]) {
	str += x
	count++
}
assert(str == "redblueyellow")
assert(count == 3)



/*
 *	Test iteration on an array
 */	
count = 0
var last
for each (x in { one: 1, two: 2, three: 3 } ) {
	last = x
	count++
}
assert(last == 3)
assert(count == 3)


/*
 *	Test default iterator for a number
 */
count = 0
for each (i in 10)
	count++

assert(count == 10)
