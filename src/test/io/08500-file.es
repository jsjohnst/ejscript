/*
 *	Length in bytes of file.dat
 */
const TestLength = 500
const TestFile: String = "io/file.dat"

//
//	copy, remove
//
name = "temp.dat"
newName = new File(name)
newName.remove()
assert(!newName.exists())

from = new File(TestFile)
from.copy(name)

assert(newName.exists)
assert(newName.length == TestLength)
newName.remove()
assert(!newName.exists())


//
//	createTempFile
//
var file: File = File.createTempFile("/tmp")
assert(file.exists)
file.remove()
assert(!file.exists)


// BUG - not implemented
//	freeSpace
//  assert(freeSpace("/") > 0)
//

//
//	getFiles
//	WINBUG
//	f = new File("io") 
//	dirs = f.getFiles()
//	print(dirs)
//	print(Reflect(dirs).typeName)
//	assert(dirs is Array)
//	assert(dirs.length > 0)
//	assert(dirs.find(function (e) { return e == TestFile; }) == TestFile)


//
//	makeDir, removeDir
//
f = new File("tempDir")
f.removeDir()
assert(!f.exists())
f.makeDir()
assert(f.exists())
assert(f.isDir)

f.removeDir()
assert(!f.exists())


//
//	relativePath
//
assert((new File("a.b")).relativePath == "a.b")
assert((new File("./a.b")).relativePath == "a.b")
//	WINBUG result is /tmp/a.b
// assert((new File("/tmp/a.b")).relativePath.indexOf("../") >= 0)


//
//	Test basic file attributes
//
f = new File(TestFile)
assert(f.exists)
assert(f.isRegular)
assert(!f.isDir)
assert(!f.isOpen)
assert(f.mode == File.Closed)
assert(f.created > 0)
assert(f.absolutePath.indexOf("/") >= 0 || f.absolutePath.indexOf("\\") >= 0)
assert(f.relativePath == "io/file.dat" || f.relativePath == "io\\file.dat")
assert(f.unixPath == "io/file.dat")
assert(f.name == "io/file.dat" || f.name == "io\\file.dat")
assert(f.basename == "file.dat")
assert(f.dirname == "io")
assert(f.extension == ".dat")
assert(f.parent == ".")
assert(f.length == TestLength)
assert(f.lastAccess is Date )
assert(f.lastAccess > 0)
assert(f.permissions = 0644)

//BUG	assert(f.available == 0)

if (f.hasDriveSpecs) {
	assert(f.name[0].isAlpha)
}


//
//	Low level reading
//
f = new File(TestFile)
f.open(File.Read)
assert(f.isOpen)
assert(f.mode == File.Open | File.Read)
bytes = f.readBytes(1024)
assert(bytes.length == TestLength)
assert(bytes.available == TestLength)


//
//	Test seek (use open above)
//
f.position = 0
again = f.readBytes(1024)
assert(again.length == bytes.length)
assert(again.available == bytes.length)

//
//	Low level writing
//	
f = new File("temp.dat")
f.open(File.Create | File.Write | File.Truncate, 0644)
f.write("0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789\r\n")
f.flush()
assert(f.length == 102)
f.close()
assert(!f.isOpen)
assert(f.exists)
assert(f.permissions = 0644)
f.permissions = 777
assert(f.permissions = 0777)
f.remove()
assert(!f.exists)

//
//	Create new file, write then read contents
//
f = new File("temp.dat")
f.open(File.Write | File.Truncate | File.Create)
for (i in 256) {
	f[i] = i
}
f.close()
assert(!f.isOpen)
assert(f.mode == File.Closed)
f.open()
for (i in 256) {
	assert(f[i] == i)
}
f.close()


//
//	Read via direct indexing 
//
f = new File("temp.dat")
f.open(File.Read)
count = 0
for (i in f) {
	assert(f[i] == i)
	count++
}
assert(count == 256)
f.close()

//
//	Read entire file as a string
//
str = File.getString(TestFile)
assert(str.search("123456789") >= 0)
assert(str.length == 500)


//
// 	Read entire file a byte array
//
bytes = File.getBytes(TestFile)
assert(bytes.length == 500)
assert(bytes.readString(5) == "12345")



//
//	Read entire file as lines
//
lines = File.getLines(TestFile)
for each (l in lines) {
	assert(l.search("12345") == 0);
}

//
//	put to create entire file
//
File.put("temp.dat", 0644, "Hello World\r\n", new Date, "\r\n")
f = new File("temp.dat")
assert(f.exists)
f.remove()
assert(!f.exists)
f.close()


//
//	Stream access
//
f = File.openFileStream(TestFile)
bytes = f.readBytes(1024)
assert(bytes.length == TestLength)
assert(bytes.available == TestLength)
f.close()

//
//	Test delimiter
//
f = new File(TestFile)
assert(f.pathDelimiter == "/" || f.pathDelimiter == "\\")
f.close()


//
//	Read binary data
//
bs = File.openBinaryStream(TestFile)
assert(bs.readString(5) == "12345")
assert(String.fromCharCode(bs.readByte()) == "6")
assert(String.fromCharCode(bs.readByte()) == "7")
assert(String.fromCharCode(bs.readByte()) == "8")
assert(String.fromCharCode(bs.readByte()) == "9")
assert(String.fromCharCode(bs.readByte()) == " ")

data = new ByteArray(500)
assert(bs.read(data, 0, data.length) == 490)
assert((data.readString(-1)).length == 490)


//
//	Write binary data
//
bs = File.openBinaryStream("temp.dat", File.Write | File.Create | File.Truncate)
bs.write("Hello world")
bs.flush()
bs.close()
f = new File("temp.dat")
assert(f.length == 11)
f.close()

//
//	endian encodings. TODO - this is not really testing much
//
bs = File.openBinaryStream("temp.dat", File.Write | File.Create | File.Truncate)
bs.endian = BinaryStream.BigEndian
for (i in 1024) {
	bs.writeInteger(i)
}
bs.close()
f = new File("temp.dat")
assert(f.length == 4096 || f.length == 8192)
f.close()


//	BUG - not implemented
//	ts = File.openTextStream(TestFile, File.Read)
//	ts.encoder = new UnicodeEncoder
//	lines = ts.readLines(5)
//	ts.close()
//

//
//	Write lines to a stream
//
ts = File.openTextStream("temp.dat", File.Write | File.Create | File.Truncate)
ts.writeLine("Hello World")
ts.writeLine("Sunny\nDay")
ts.close()


//
//	Layered stream access
//
var s: TextStream = new TextStream(File.openFileStream("temp.dat"))
assert(s.readLine() == "Hello World")

if (File.newline.length > 1) {
	assert(s.readLine() == "Sunny\nDay")
} else {
	assert(s.readLine() == "Sunny")
	assert(s.readLine() == "Day")
}
s.close()
rm("temp.dat")


//  BUG - not implemented
//	File.put
//
//  xml = File.getXml(TestFile)

