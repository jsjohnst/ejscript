/*
 *	Nested switch
 */

/* NOT YET
x = 1

switch (x) {

case 1:
	day = "mon"
	switch (day) {
	case "mon":
		weather = "rainy"
		break

	case "tue":
		weather = "sunny"
		break

	default:
		weather = "cloudy"
		break
	}

case 2:
	temp = 95
	//	Fall through

default:
	humidity = 100
}

assert(day == "mon")
assert(weather == "rainy")
assert(temp == 95)
assert(humidity == 100)

*/
