/*
 *	Was causing stack underflow due to extraneous pop
 */
var i: Number = 0
if (false)
	++i;
