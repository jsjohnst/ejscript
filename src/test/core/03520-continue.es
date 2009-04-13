/*
 *	Break test with nested switch inside for loop
 */

for (i = 0; i < 10; i++) {

	switch (i) {
	case 9:
		continue
	}

	if (i == 9) {
		i = 99
	}
	
}
assert(i == 10)
