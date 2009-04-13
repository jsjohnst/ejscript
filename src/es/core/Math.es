/*
 *	Math.es -- Math class 
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs {

	use default namespace intrinsic

	/**
	 *	The Math class provides a set of static methods for performing common arithmetic, exponential and 
	 *	trigonometric functions. It also provides commonly used constants such as PI. See also the Number class.
	 *	Depending on the method and the supplied argument, return values may be real numbers, NaN (not a number) 
	 *	or positive or negative infinity.
	 */
    # FUTURE
	class Math extends Object 
	{

		/**
		 *	The ratio of the circumference to the diameter of a circle.
		 */
		native static const PI: Number = 3.141592653589793


		/**
		 *	Base of natural logarithms (Euler's number).
		 */
		native static const E: Number = 2.718281828459045


		/**
		 *	Natural log of 10.
		 */
		native static const LN10: Number = 2.302585092994046


		/**
		 *	Natural log of 2.
		 */
		native static const LN2: Number = 0.6931471805599453


		/**
		 *	Base 2 log of e.
		 */
		native static const LOG2E: Number = 1.4426950408889634


		/**
		 *	Base 10 log of e.
		 */
		native static const LOG10E: Number = 0.4342944819032518


		/**
		 *	Reciprocal of the square root of 2.
		 */
		native static const SQRT1_2: Number = 0.7071067811865476


		/**
		 *	Square root of 2.
		 */
		native static const SQRT2: Number = 1.4142135623730951


        //  TODO - doc
        native static function abs(value: Number): FloatNumber 

		/**
		 *	Calculates the arc cosine of an angle (in radians).
		 *	@param angle In radians 
		 *	@return The arc cosine of the argument 
		 */
		native static function acos(angle: Number): FloatNumber 
		

		/**
		 *	Calculates the arc sine of an angle (in	radians).
		 *	@param oper The operand.
		 *	@return The arc sine of the argument 
		 */
		native static function asin(oper: Number): FloatNumber 
		

		/**
		 *	Calculates the arc tangent of an angle (in radians).
		 *	@param oper The operand.
		 *	@return The arc tanget of the argument 
		 */
		native static function atan(oper: Number): FloatNumber 
		

		//	TODO - what does this fn really do
		/**
		 *	Calculates the arc tangent of an angle (in radians).
		 *	@param x the x operand.
		 *	@param y the y operand.
		 *	@return The arc tanget of the argument 
		 */
		native static function atan2(y: Number, x: Number): FloatNumber 
		

		/**
		 *	Return the smallest integer greater then this number.
		 *	@return The ceiling
		 */
		native function ceil(oper: Number): Number


		/**
		 *	Calculates the cosine of an angle (in radians).
		 *	@param angle In radians 
		 *	@return The cosine of the argument 
		 */
		native static function cos(angle: Number): FloatNumber 
		
        native static function exp(angle: Number): FloatNumber 

		/**
		 *	Returns the largest integer smaller then the argument.
		 *	@param oper The operand.
		 *	@return The floor
		 */
		native function floor(oper: Number): Number


		/**
		 *	Calculates the natural log (ln) of a number.
		 *	@param oper The operand.
		 *	@return The natural log of the argument
		 */
		native static function ln(oper: Number): FloatNumber 
		
		
		/**
		 *	Calculates the log (base 10) of a number.
		 *	@param oper The operand.
		 *	@return The base 10 log of the argument
		 */
		native static function log(oper: Number): FloatNumber 
		

		/**
		 *	Returns the greater of the number or the argument.
		 *	@param x First number to compare
		 *	@param y Second number to compare
		 *	@return A number
		 */
		native function max(x: Number, y: Number): Number


		/**
		 *	Returns the lessor of the number or the argument.
		 *	@param x First number to compare
		 *	@param y Second number to compare
		 *	@return A number
		 */
		native function min(x: Number, y: Number): Number


		/**
		 *	Returns a number which is equal to this number raised to the power of the argument.
		 *	@param num The number to raise to the power
		 *	@param pow The exponent to raise @num to
		 *	@return A number
		 */
		native function power(num: Number, pow: Number): Number


		/**
		 *	Returns a number which is equal to this number raised to the power of the argument.
		 *	Invokes @power.
		 *	@param limit The number to compare to
		 *	@return A number
		 */
		# ECMA
		native function pow(num: Number, pow: Number): Number


		/**
		 *	Generates a random number (a FloatNumber) inclusively between 0.0 and 1.0.
		 *	@return A random number
		 *	BUG: ECMA returns double
		 */
		native static function random(): FloatNumber 


		/**
		 *	Round this number down to the closes integral value.
		 *	@return A rounded number
		 */
		native function round(): Number


		/**
		 *	Calculates the sine of an angle (in radians).
		 *	@param angle In radians 
		 *	@return The sine of the argument 
		 */
		native static function sin(angle: Number): FloatNumber 
		

		/**
		 *	Calculates the square root of a number.
		 *	@param oper The operand.
		 *	@return The square root of the argument
		 */
		native static function sqrt(oper: Number): FloatNumber 
		

		/**
		 *	Calculates the tangent of an angle (in radians).
		 *	@param angle In radians 
		 *	@return The tangent of the argument 
		 */
		native static function tan(angle: Number): FloatNumber 
	}
}

/*
 *	@copy	default
 *	
 *	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *	Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Embedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.embedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Embedthis 
 *	Software at http://www.embedthis.com 
 *	
 *	@end
 */
