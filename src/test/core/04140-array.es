/*
 *	Test array methods and operators
 */


/*
 *	toString
 */
a = [1,2,3,4,]
assert(a == "1,2,3,4")
a = []
assert(a.toString() == '')


/* 
 *	clone
 */
a = [ "one", "two", "three" ]
assert(a.length == 3)
b = a.clone(true)
assert(b.length == 3)
assert(b is Array)
b.length = 2
assert(b.length == 2)
assert(b == "one,two")


/*
 *	clone of nested object
 */
a = [ "one", [ "two", "three"], "four" ]
assert(a.length == 3)
b = a.clone(true)
assert(b.length == 3)
assert(b is Array)
assert(b == "one,two,three,four")


/*
 *	constructors
 */
a = new Array
assert(a.length == 0)
a = new Array()
assert(a.length == 0)
a = new Array(10)
assert(a.length == 10)
a = new Array(1, 2, 3, 4)
assert(a.length == 4)


/*
 *	append
 */
a = [ 1 ]
a.append(2)
assert(a == "1,2")


/*
 *	clear
 */
a = [ 1,,2 ]
a.clear()
assert(a.length == 0)

/*
 *	compact
 */
a = [ 1,,,4,,, ]
a[20] = 7
delete a[20]
assert(a.compact() == "1,4")
assert(a.length == 2)


/*
 *	every
 */
function smallNumber(element): Boolean {
	return element < 10
}
assert([1,2,3,4].every(smallNumber))
assert(![55,2,100].every(smallNumber))
assert([1, 2].every(function (e, i, a) { return e < 10; }) == true)


/*
 *	indexOf
 */
a = [ 1, 2, 3, 4, false, "4", "9", Object ]
assert(a.indexOf(Object) == 7)


/*
 *	lastIndexOf
 */
a = [ 1, 2, 1, 3, 4, false, "4", "9", Object, 2 ]
assert(a.lastIndexOf(1) == 2)


/*
 *	insert
 */
a = [ 1, 2, 3 ]
assert(a.insert(3, 7, 8, 9) == "1,2,3,7,8,9")
a = [1,2,4]
assert(a.insert(-1, 3) == "1,2,3,4")


/*
 *	join
 */
a = [ 1, 2, 3 ]
assert(a.join(", ") == "1, 2, 3")


/*
 *	map
 */
a = [ 1, 2, 3 ]
b = a.map(function (e) { return e + 10; })

/*
 *	push
 */
a = [ 1, 2, 3 ]
assert(a.push(7, 8, 9) == 6)


/*
 *	pop
 */
a = []
a.push(1)
a.push(3)
a.pop()
a.push(2)
assert(a == "1,2")

/*
 *	reject
 */
a = [ 1, 2, 3, 4, 5, 6 ]
b = a.reject(function (e) { return e % 2 == 0; })
assert(b == "1,3,5")


/*
 *	Remove
 */
a = [1,2,3,4,5]
a.remove(-1)
assert(a == "1,2,3,4")

a = [1,2,3,4,5]
a.remove(0,1)
assert(a == "3,4,5")

a = [1,2,3,4,5]
a.remove(-2, -1)
assert(a == "1,2,3")


/*
 *	reverse
 */
assert([].reverse() == "")
assert([1].reverse() == "1")
assert([1, 2].reverse() == "2,1")
assert([1, 2, 3].reverse() == "3,2,1")
assert([1, 2, 3, 4].reverse() == "4,3,2,1")
assert([1, 2, 3, 4, 5].reverse() == "5,4,3,2,1")


/*
 *	shift
 */
a = [ 1, 2, 3, 4, 5, 6 ]
assert(a.shift() == 1)
assert(a == "2,3,4,5,6")
a = []
a.shift()
[].shift()


/*
 *	slice
 */
a = [ 1, 2, 3, 4, 5, 6 ]
assert(a.slice(1) == "2,3,4,5,6")
assert(a.slice(1,2) == "2")
assert(a.slice(-2) == "5,6")
assert(a.slice(0, -1, 2) == "1,3,5")
assert(a.length == 6)


/*
 *	sort
 */
a = [ 6, 2, 5, 1, 0, 4, 3 ]
assert(a.sort() == "0,1,2,3,4,5,6")
a = [ "def", "xyz", "abc", "ABC", "mno" ]
assert(a.sort() == "ABC,abc,def,mno,xyz")
assert(a.reverse() == "xyz,mno,def,abc,ABC")


/*
 *	splice
 */
a = [ 1, 2, 3, 4, 5]
assert(a.splice(2, 3) == "3,4,5")
assert(a == "1,2")

a = [ 1, 2, 3, 4, 6]
assert(a.splice(-1, 0, 5) == "")
assert(a == "1,2,3,4,5,6")

a = [ 1, 2, 3, 4, 5]
assert(a.splice(-2, 0, "3.5") == "")
assert(a == "1,2,3,3.5,4,5")


/*
 *	Splice to add and delete 
 */
a = [ 1, 2, 3, 4, 5]
assert(a.splice(2, 3, "three", "four") == "3,4,5")



/*
 *	transform
 */
a = [ 1, 2, 3, 4 ]
a.transform(function (e) { return e + 10; })
assert(a == "11,12,13,14")


/*
 *	unique
 */
a = [ 1, 2, 1, "blue", "red", "blue", 1, 4 ]
assert(a.unique() == "1,2,blue,red,4")


/*
 *	unshift
 */
a = [ 1, 2, 3, 4, 5 ]
a.unshift(0)
assert(a == "0,1,2,3,4,5")




/*
 *	Test get(), getValues() for enumeration and iteration
 */
a = ["one", "two", "three"]
count = 0
for (i in a) {
	assert(i == count);
	count++
}
assert(count == 3)

count = 0
for each (i in a) {
	assert(i == a[count]);
	count++
}


/*
 *	delete
 */
assert(a.length == 3)
delete a[0]
assert(a.length == 3)
assert(a[0] == undefined)


/*
 *	serialize
 */
user = { name: "Peter", age: 23 }
inner  = ["a", "b", "c"]
src = [ 1, 2, [ 3, 4], inner, user]
x = serialize(src)
copy = deserialize(x)
assert(src.type == copy.type)
assert(" " + src == " " + copy)
assert(src.length == copy.length)



/*
 *	Operators: +
 */
a = [ 1, 2, 3]
b = [ 3, 4, 5]

c = a + b
assert(c == "1,2,3,3,4,5")

a << b
assert(a == "1,2,3,3,4,5")

c = a | b
assert(c == "1,2,3,4,5")

a = [ 1, 2, 3]
b = [ 3, 4, 5]
c = a & b
assert(c == "3")

a = [1,2,3,4,5,6,7]
assert((a - [0,1,2]) == "3,4,5,6,7")
