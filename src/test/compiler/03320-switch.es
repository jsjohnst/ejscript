/*
 *	Switch with strings
 */

function test(x) {

	switch (x) {

	case "one":
		x = "one-match"
		break

	default:
		x = "any-match"
		break
	}
	return x
}

assert(test("one") == "one-match")
assert(test("any") == "any-match")
