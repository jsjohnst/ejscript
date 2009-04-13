/*
 *	Some binary operators: cast, in, is, instanceof, like
 */
class Shape {}
class Rectangle extends Shape {}
class Square extends Rectangle {}
class Circle extends Shape {}

/*
 *	Test "is"
 */
assert(new Square is Square)
assert(new Square is Rectangle)
assert(new Square is Shape)
assert(new Square is Shape)
assert(new Circle is Shape)
assert(!(new Circle is Rectangle))

user = { 
	name: "Julie",
	age: 33,
	eyes: "blue"
}


/*
 *	Test instanceof. TODO - currently just an alias for "is"
 */
o = new Square
assert(o instanceof Square)
assert(o instanceof Shape)


/*
 *	Test "like"
 *	TODO: currently "like" is just an alias for "is"
 */
o = new Square
assert(o like Square)
assert(o like Rectangle)
assert(o like Shape)


/*
 *	Test "in"
 */
assert("name" in user)
assert(!("address" in user))


/*
 *	Test "cast"
 */
x = 7 cast String
assert(x is String)
assert(7 is Number)

x = "12345" cast Number
assert(x is Number)
assert(x == 12345)
