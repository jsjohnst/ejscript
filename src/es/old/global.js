/*
 *	@file 	global.js
 *	@brief 	Misc global functions
 *	@copy 	Copyright (c) Embedthis Software LLC, 2005-2009. All Rights Reserved.
 */


function min(a, b) 
{
	if (a < b) {
		return a;
	} else {
		return b;
	}
}


function max(a, b) 
{
	if (a > b) {
		return a;
	} else {
		return b;
	}
}

function abs(a)
{
	if (a < 0) {
		return -a;
	}
	return a;
}
