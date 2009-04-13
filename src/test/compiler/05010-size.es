/*
 *	Test large number of function args
 */

function fun(a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,a16,a17,a18,a19,a20,a21,a22,a23,a24,a25,
	a26,a27,a28,a29,a30,a31,a32,a33,a34,a35,a36,a37,a38,a39,a40,a41,a42,a43,a44,a45,a46,a47,a48,a49) 
{
	assert(a0 == 0)
	assert(a1 == 1)
	assert(a2 == 2)
	assert(a3 == 3)
	assert(a4 == 4)
	assert(a5 == 5)
	assert(a6 == 6)
	assert(a7 == 7)
	assert(a8 == 8)
	assert(a9 == 9)
	assert(a10 == 10)
	assert(a11 == 11)
	assert(a12 == 12)
	assert(a13 == 13)
	assert(a14 == 14)
	assert(a15 == 15)
	assert(a16 == 16)
	assert(a17 == 17)
	assert(a18 == 18)
	assert(a19 == 19)
	assert(a20 == 20)
	assert(a21 == 21)
	assert(a22 == 22)
	assert(a23 == 23)
	assert(a24 == 24)
	assert(a25 == 25)
	assert(a26 == 26)
	assert(a27 == 27)
	assert(a28 == 28)
	assert(a29 == 29)
	assert(a30 == 30)
	assert(a31 == 31)
	assert(a32 == 32)
	assert(a33 == 33)
	assert(a34 == 34)
	assert(a35 == 35)
	assert(a36 == 36)
	assert(a37 == 37)
	assert(a38 == 38)
	assert(a39 == 39)
	assert(a40 == 40)
	assert(a41 == 41)
	assert(a42 == 42)
	assert(a43 == 43)
	assert(a44 == 44)
	assert(a45 == 45)
	assert(a46 == 46)
	assert(a47 == 47)
	assert(a48 == 48)
	assert(a49 == 49)

	return 77
}

assert(fun(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 
	26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49) == 77);
