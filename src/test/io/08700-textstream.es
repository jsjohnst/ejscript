/*
 *  Text stream
 *
 *  TODO - encoding
 */

var b: ByteArray
var t: TextStream 

//
//  Write 
//
b = new ByteArray(1000)
t = new TextStream(b)
t.writeLine("line 1", "line 2", "line 3")
assert(b == "line 1\r\nline 2\r\nline 3\r\n" || b == "line 1\nline 2\nline 3\n")

//
//  Read
//
b = new ByteArray(1000)
t = new TextStream(b)
b.write("Line one\nLine 2\nLine 3") 
dest = new ByteArray(1000)
count = t.read(dest)
assert(count == 22)
assert(dest.readString() == "Line one\nLine 2\nLine 3") 


b = new ByteArray(1000)
t = new TextStream(b)
b.write("Line one\nLine two\nLine three") 
a = t.readLines()
if (File.newline.length > 1) {
	assert(a.length == 1)
	assert(a == "Line one\nLine two\nLine three") 
} else {
	assert(a.length == 3)
	assert(a == "Line one,Line two,Line three")
}
