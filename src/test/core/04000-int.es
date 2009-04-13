/*
 *	Integer constants
 */

i = 0
assert(i == 0)

i = 1
assert(i == (0+1))

i = 0x7f
assert(i == 127)

i = 0x80
assert(i == 128)

i = 0xFF
assert(i == 255)

i = 0x100
assert(i == 256)

i = 0x7FFF
assert(i == 32767)

i = 0x8000
assert(i == 32768)

i = 0x8001
assert(i == 32769)

i = 0x7FFFFFFF
assert(i == 2147483647)

if (Config.numberType == "int") {
	i = 0x80000000
	assert(i == -2147483648)
}
