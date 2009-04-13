/*
 *	Long for loop. Branches can't be optimized.
 */

for (i = 0; i < 10; i = i + 1) {
	global; global; global; global; global; global; global; global; global;
	global; global; global; global; global; global; global; global; global;
	global; global; global; global; global; global; global; global; global;
	global; global; global; global; global; global; global; global; global; 
	global; global; global; global; global; global; global; global; global; 
	global; global; global; global; global; global; global; global; global; 
	global; global; global; global; global; global; global; global; global; 
	global; global; global; global; global; global; global; global; global; 
	global; global; global; global; global; global; global; global; global; 
	global; global; global; global; global; global; global; global; global; 
}
assert(i == 10)
