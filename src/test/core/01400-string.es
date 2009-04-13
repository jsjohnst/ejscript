/*
 *	String methods 
 */
/*BUG
	localeCompare
	match
	parseJSON

 */

/*
 *	Test constructor
 */
s = new String
assert(s == "")
assert(s.length == 0)
assert(Reflect(s).typeName == "String")

s = new String(97);
assert(s == "97")
assert(s.length == 2)
assert(Reflect(s).typeName == "String")



/*
 *	Character index
 */
s = "abcdef"
assert(s[2] == "c")


/*
 *	caseCompare
 */
s = "abcdef"
assert(s.caseCompare("abcdef") == 0)
assert(s.caseCompare("ABCdef") == -1)


/*
 *	charAt
 */
s = "abcdef"
assert(s.charAt(2) == "c")


/*
 *	charCodeAt
 */
s = "abcdef"
assert(s.charCodeAt(2) == 99)


/*
 *	concat
 */
s = "abc"
assert(s.concat("def", "ghi") == "abcdefghi")


/*
 *	contains
 */
s = "abcdef"
assert(s.contains("cd"))
assert(!s.contains("defg"))


/*
 *	endsWith
 */
s = "abcdef"
assert(s.endsWith("def"))
assert(!s.endsWith("defg"))
assert(!s.endsWith("abc"))


/*
 *	format
 */
assert("".format() == "")
assert("Sunny Day".format() == "Sunny Day")
assert("%s".format("Cloudy Day") == "Cloudy Day")
assert("%,d".format(1000) == "1,000")
assert("%10s".format("Day") == "       Day")

if (Config.Floating) {
    assert("%g".format(1234.3) == "1234.3")
    assert("%e".format(1234.3) == "1.234300e+03")
    assert("%f".format(1234.3) == "1234.300000")
    assert("Multi %s output %d.".format("word", 1234) == "Multi word output 1234.")
}


/*
 *	fromCharCode
 */
a = String.fromCharCode(97, 98, 99)
assert(a == "abc")
assert(String.fromCharCode(97, 98, 99) == "abc")


/*
 *	length
 */
s = "abcdef"
assert(s.length == 6)



/*
 *	get
 */
count = 0
for (i in "abc") {
	assert(i == count++)
}


/*
 *	getValues
 */
count = 0
for each (i in "abc") {
	assert(i == "abc"[count++])
}


/*
 *	indexOf
 */
s = "abcdefdefdef"
assert(s.indexOf("def") == 3)
assert(s.indexOf("def", 4) == 6)


/*
 *	isDigit, isAlpha, isLower, isSpace, isUpper
 */
assert("0".isDigit)
assert(!"a".isDigit)

assert("a".isAlpha)
assert(!"0".isAlpha)
assert("A".isAlpha)
assert(!"-".isAlpha)

assert("l".isLower)
assert(!"L".isLower)

assert(" ".isSpace)
assert("\t".isSpace)
assert("\r".isSpace)
assert("\n".isSpace)

assert("L".isUpper)
assert(!"l".isUpper)


/*
 *	lastIndexOf
 */
s = "abcdefdefdef"
assert(s.lastIndexOf("def") == 9)


/*
 *	printable
 */
a = "\u0001abcd"
assert("abc\u0001\r\n".printable() == "abc\\u0001\\u000D\\u000A")


/*
 *	quote
 */
assert("abc".quote() == '"abc"')


/*
 *	replace
 */
assert("abcdef".replace("cd", "CD") == "abCDef")
assert("abcdef".replace("cd", "CDXXYYZZ") == "abCDXXYYZZef")
assert("abcdef".replace("cd", "") == "abef")

if (Config.RegularExpressions) {
    assert("abcdef".replace(/c/, "$$") == "ab$def")
}


/*
 *	reverse
 */
assert("abcdef".reverse() == "fedcba")


/*
 *	search
 */
assert("abcdef".search("ef") == 4)


/*
 *	slice
 */
s = "abcdef"
assert(s.slice(1) == "bcdef")
assert(s.slice(1, 2) == "b")
assert(s.slice(1, -2) == "bcd")
assert(s.slice(-3) == "def")
assert(s.slice(-2, -1) == "e")
assert(s.slice(0, -4) == "ab")


/*
 *	split
 */
a = "abc"
assert(a.split("") == "a,b,c")
assert(a.split(" ") == "abc")
a = "a b  c"
assert(a.split(" ") == "a,b, c")


/*
 *	startsWith
 */
s = "abcdef"
assert(s.startsWith("abc"))
assert(!s.startsWith("defg"))
assert(!s.startsWith("def"))


/*
 *	substr - Deprecated
 */
if (ECMA) {
	s = "abcdef"
	assert(s.substr(1) == "bcdef")
	assert(s.substr(1,2) == "bc")
	assert(s.substr(1,4) == "bcde")
}


/*
 *	substring
 */
s = "abcdef"
assert(s.substring(1) == "bcdef")
assert(s.substring(2,4) == "cd")
assert(s.substring(1, -1) == "bcdef")
assert(s.substring(1,-2) == "bcdef")


/*
 *	toLower
 */
assert("ABcdEF".toLower() == "abcdef")


/*
 *	toPascal
 */
assert("sunnyDay".toPascal() == "SunnyDay")
assert("SunnyDay".toPascal() == "SunnyDay")


/*
 *	toUpper
 */
assert("ABcdEF".toUpper() == "ABCDEF")


/*
 *	tokenize
 */
s = "one two 3"
a = s.tokenize("%s %s %d")
assert(a.length == 3)
assert(Reflect(a[0]).typeName == "String")
assert(a[0] == "one")
assert(Reflect(a[1]).typeName == "String")
assert(a[1] == "two")
assert(Reflect(a[2]).typeName == "Number")
assert(a[2] == "3")


/*
 *	trim
 */
assert("abcdef".trim() == "abcdef")
assert(" abcdef".trim() == "abcdef")
assert("  abcdef".trim() == "abcdef")
assert("abcdef ".trim() == "abcdef")
assert("abcdef  ".trim() == "abcdef")
assert("  abcdef  ".trim() == "abcdef")

assert("abcdef".trim(" ") == "abcdef")
assert(" abcdef".trim(" ") == "abcdef")
assert("  abcdef".trim(" ") == "abcdef")
assert("abcdef ".trim(" ") == "abcdef")
assert("abcdef  ".trim(" ") == "abcdef")
assert("  abcdef  ".trim(" ") == "abcdef")
assert("  abcdef  ".trim("  ") == "abcdef")
assert("   abcdef   ".trim("  ") == " abcdef ")

assert("  abcdef   ".trim().quote() == '"abcdef"')

/*
 *  Times
 */
assert(" ".times(5) == "     ")

/*
 *	Operators
 */

/*
 *	operator: +
 */
a = "abc"
b = "def"
c = a + b
assert(c == "abcdef")

/*
 *	operator: -
 */
a = "abcdef"
b = "def"
c = a - b
assert((a - b) == "abc")


/*
 *	operator: <
 */
a = "abc"
b = "def"
assert(a < b)


/*
 *	operator: %
 */
assert("Error %,d %s" % [1024, "Some Message"] == "Error 1,024 Some Message")


