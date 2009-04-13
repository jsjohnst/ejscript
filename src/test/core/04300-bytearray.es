/*
 *	ByteArray tests. This tests storage and retrieval of primitive types and basic read/write positioning.
 */

Size = 200

var b: ByteArray

//
//	Test basic construction
//
b = new ByteArray(Size)
assert(b != null)
assert(b.length == 200)
assert(b.readPosition == 0)
assert(b.writePosition == 0)
assert(b.available == 0)
assert(b.room == Size)

//
//  Test low level access. 
//  Write some bytes and consume one. Should still be able to access via direct indexing[]
//
b = new ByteArray(Size)
b.writeByte(110)
b.writeByte(111)
b.writeByte(112)
assert(b == "nop")
b.readByte()
assert(b.readPosition == 1)
assert(b == "op")
assert(b[0] == 110)
assert(String.fromCharCode(b[0]) == "n")

//
//  Test enumeration. Should enumerate over the data between read/write positions only
//
b = new ByteArray(Size)
b.write("Hello World")
b.readByte()
s = ""
for each (f in b) {
    s = s.concat(String.fromCharCode(f))
}
assert(s == "ello World")

//
//	Basic read / write
//
b = new ByteArray(Size)
b.write("Sunny")
assert(b.readPosition == 0)
assert(b.writePosition == 5)
b.write(" Day")
assert(b.writePosition == 9)
s = b.readString()
assert(s == "Sunny Day")
assert(b.readPosition == b.writePosition)


//
//	flush
//
b = new ByteArray(Size)
assert(b.available == 0)
b.flush()
assert(b.available == 0)
b.write("Hello")
assert(b.available == 5)
b.flush()
assert(b.available == 0)
assert(b.length == Size)


//
//	Indexing
//
b = new ByteArray(Size)
b.write("Sunny")
assert(b[0] == 83)
assert(b[1] == 117)


//
//	Iteration
//
b = new ByteArray(Size)
b.write("Sunny")
count = 0
for each (i in b) {
	count++
}
assert(count == 5)



//
//	readPosition
//
b = new ByteArray(Size)
b.write("Sunny Day")
b.readPosition += 6
assert(b.readString() == "Day")


//
//	read
//
b = new ByteArray(Size)
b.writeByte(1)
b.writeByte(2)
b.writeByte(3)
check  = new ByteArray
b.read(check, 0, 3)
assert(check[0] == 1)
assert(check[1] == 2)
assert(check[2] == 3)


//
//	Read and write lots of bytes
//
size = 4096
b = new ByteArray
for (i in size) {
	b.writeByte(i)
}
check = new ByteArray(b.available)
b.read(check)
assert(check.available == size)
for (i in Size) {
	assert(check[i] == i);
	assert(check.readByte() == i);
}


//
//	Test low level read/write primitives
//
b = new ByteArray
b.writeByte(1)
b.writeShort(2)
b.writeInteger(3)
b.writeLong(4)
if (Config.Floating) {
	b.writeDouble(5.1234)
}
assert(b.readByte() == 1)
assert(b.readShort() == 2)
assert(b.readInteger() == 3)
assert(b.readLong() == 4)
if (Config.Floating) {
    assert(b.readDouble() == 5.1234)
}

//
//  Test simple indexing
//
b = new ByteArray(Size)
b.write("Sunny Day")
assert(b[2] == 110)

//
//  Test writeXml
//
b = new ByteArray(Size)
b.write(<order><item>shirt</item><price>23.00</price></order>)
xml = b.readXML()
assert(xml.item == "shirt")

//
//  Test output functions
//
b = new ByteArray(Size)
var saveData
b.output = function(ba: ByteArray) {
    saveData = ba.readString()
}
b.write("Hello World")
b.flush()
assert(saveData == "Hello World")

//
//  Test input functions
//
b = new ByteArray(Size)
b.input = function(ba) {
    ba.write("Some Data")
}
assert(b.readString() == "Some Data")


//
//  Test copyin/copyout
//
source = new ByteArray(Size)
source.write("Hello")
dest = new ByteArray(Size)
b.copyIn(0, source)
assert(String.fromCharCode(b[0], b[1], b[2]) == 'Hel')
b.writePosition = source.available
assert(b == "Hello")

dest = new ByteArray(1024)
source.copyOut(0, dest, 0, 4)
assert(String.fromCharCode(dest[0], dest[1], dest[2]) == 'Hel')
dest.writePosition = 4
assert(dest == "Hell")
