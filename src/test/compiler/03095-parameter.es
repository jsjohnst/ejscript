/*
 *	Test function parameter validation and conversions
 */

/*
 *	Test ...rest args
 */
function testRest(a, b, c, ...items)
{
	assert(a is Number)
	assert(b is String)
	assert(c is boolean)
	assert(items == "4,5,6")
	assert(Reflect(items).typeName == "Array")
	assert(items is Array)
	assert(items.length == 3)
}
testRest(1, "Hello World", true, 4, 5, 6)


/*
 *	Test parameter type conversion
 */
function testWithTypes(a: Number, b: String, c: Boolean) 
{
	assert(a is Number)
	assert(b is String)
	assert(c is Boolean)
}

testWithTypes(1, "77", 1 == 7)
testWithTypes("1", 77, undefined)
testWithTypes(true, true, undefined)
