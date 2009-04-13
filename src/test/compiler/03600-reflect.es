/*
 *	Test reflection
 */

assert(Reflect(Object).name == "Object")

class Shape {}
assert(Reflect(Shape).name == "Shape")
assert(Reflect(Shape).typeName == "Object")

o = new Object
assert(Reflect(o).type == Object)
assert(Reflect(Reflect(o).type).name == "Object")

class Circle extends Shape {}
c = new Circle
assert(Reflect(Reflect(Reflect(c).type).type).name == "Shape")
assert(Reflect(Reflect(Reflect(Reflect(c).type).type).type).name == "Object")


function fun() {
}
assert(Reflect(fun).name == "fun")
