/*
 *  This provoked an issue where the "i++" was storing to global and not to the block where the "i" variable
 *  was actually found. BUG was looping forever.
 */
function fun() {
	for (i = 1; i < 4; i++) {
		// print(i)
	}
}
fun()
