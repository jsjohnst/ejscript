/*
 *	Break test with nested switch inside for loop
 */

for (i = 0; i < 10; i++) {

	filler = i
	filler = i

	switch (i) {
	case 1:
		break
	
	case 2:
		break
	
	default:
		break
	}
	
	filler = i
	filler = i
}
assert(i == 10)
