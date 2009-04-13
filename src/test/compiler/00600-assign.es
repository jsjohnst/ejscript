/*
 *	Compound operators
 */
i = 6

i += 3
assert(i == 9)

i -= 4
assert(i == 5)

i *= 6
assert(i == 30)

i /= 3
assert(i == 10)

i %= 7
assert(i == 3)

i &= 0x1 
assert(i == 1)

i |= 0x4
assert(i == 5)

i <<= 3
assert(i == 40)

i >>= 2
assert(i == 10)

i = 15
i ^= 7
assert(i == 8)

i |= 3
assert(i == 11)

i &&= 4
assert(i == true)

i = false
i ||= true
assert(i == true)

