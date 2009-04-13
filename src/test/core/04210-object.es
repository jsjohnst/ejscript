/*
 *	Object literals
 */

o = { }
assert(o != null)
assert(o.length == 0)
assert(o == '[object Object]')

/*
 *	Literal with simple elements
 */
o = { one: 1, two: 2, three: 3, four: "Four" }
assert(o.one == 1)
assert(o.two == 2)
assert(o.three == 3)
assert(o.four == "Four")

/*
 *	Using getter/setter functions
 */
var x = 7
o = { 
	one: 1, 
	get fun () { return x; },
	set fun (value) { x = value; }
}
assert(o.fun == 7)
o.fun = 23
assert(o.fun == 23)

/*
 *	Nested literals
 */
o = {
	name: "Peter", 
	details: { location: "Australia", age: 99 }
}
assert(o.length == 2)
assert(o.name == "Peter")
assert(o.details == "[object Object]")
assert(serialize(o.details) == "{
  location: \"Australia\",
  age: 99,
}")


/*
 *	Literals with non-name names
 */
o = { 
	one : 1, 
	"two" : 2,
	3 : 3
}
assert(o.length == 3)
assert(o.one == 1)
assert(o.two == 2)
assert(o["3"] == 3)


/*
 *	Test deserialization
 */
user = { 
	name: "Peter",
	age: 27,
	color: "blue",
	hobby: {
		name: "Sailing",
	},
	profession: "coder",
}

s = serialize(user)
assert(s == '{
  name: "Peter",
  age: 27,
  color: "blue",
  hobby: {
    name: "Sailing",
  },
  profession: "coder",
}')

o = deserialize(s)
assert(o.name == "Peter")
assert(o.age == 27)
assert(o.color == "blue")
assert(o.hobby.name == "Sailing")
assert(o.profession == "coder")

for (key in user) {
	assert(key in o)
}
