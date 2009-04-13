/*
 *	Number.es - Number class
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs {

	use default namespace intrinsic

//        static const MAX_VALUE : double         = 1.7976931348623157e+308;  /* INFORMATIVE */
//        static const MIN_VALUE : double         = 5e-324;                   /* INFORMATIVE */
//        static const NaN : double               = 0.0 / 0.0;
//        static const NEGATIVE_INFINITY : double = -1.0 / 0.0;
//        static const POSITIVE_INFINITY : double = 1.0 / 0.0;
//        // 15.8.1 Value Properties of the Math Object.  These are {DD,DE,RO}.
//        static const E: double = 2.7182818284590452354;   /* Approximately */
//        static const LN10: double = 2.302585092994046;    /* Approximately */
//        static const LN2: double = 0.6931471805599453;    /* Approximately */
//        static const LOG2E: double = 1.4426950408889634;  /* Approximately */
//        static const LOG10E: double = 0.4342944819032518; /* Approximately */
//        static const PI: double = 3.1415926535897932;     /* Approximately */
//        static const SQRT1_2: double = 0.7071067811865476;/* Approximately */
//        static const SQRT2: double = 1.4142135623730951;  /* Approximately */

	/**
	 *	The Number type is used by all numeric values in Ejscript. Depending on how Ejscript is configured, the underlying
	 *	number representation may be based on either an int, long, int64 or double data type. If the underlying type is
	 *	integral (not double) then some of these routines will be mapped onto 
	 */
	native final class Number {

        use default namespace public

		/**
		 *	Number constructor.
		 *	@param value. Value to use in creating the Number object. If the value cannot be converted to a number, 
         *	    the value will ba NaN (or 0 if using integer numerics).
         *	@spec ecma-3
		 */
		native function Number(...value)


		/**
		 *	Return the maximim value this number type can assume. Alias for MaxValue.
		 *	@return An object of the appropriate number with its value set to the maximum value allowed.
		 */
		# ECMA
		static const MAX_VALUE: Number = MaxValue


		/**
		 *	Return the minimum value this number type can assume. Alias for MinValue.
		 *	@return An object of the appropriate number with its value set to the minimum value allowed.
		 */
		# ECMA
		static const MIN_VALUE: Number = MinValue


		/**
		 *	Return a unique value which is less than or equal then any value which the number can assume. 
		 *	@return A number with its value set to -Infinity. If the numeric type is integral, then return zero.
		 */
		# ECMA
		static const NEGATIVE_INFINITY: Number = NegativeInfinity


		/**
		 *	Return a unique value which is greater then any value which the number can assume. 
		 *	@return A number with its value set to Infinity. If the numeric type is integral, then return MaxValue.
		 */
		# ECMA
		static const POSITIVE_INFINITY: Number = Infinity


		/**
		 *	Return the maximim value this number type can assume.
		 *	@return A number with its value set to the maximum value allowed.
		 *	@spec ejs-11
		 */
		native static const MaxValue: Number


		/**
		 *	Return the minimum value this number type can assume.
		 *	@return A number with its value set to the minimum value allowed.
		 *	@spec ejs-11
		 */
		native static const MinValue: Number


		/**
		 *	The ceil function computes the smallest integral number that is greater or equal to the number value. 
		 *	@return A number rounded up to the next integral value.
		 *	@spec ejs-11
		 */
		native function get ceil(): Number 


		/**
		 *	Compuete the largest integral number that is  smaller than the number value.
		 *	@return A number rounded down to the closest integral value.
		 *	@spec ejs-11
		 */
		native function get floor(): Number


		/**
		 *	Returns true if the number is not Infinity or NegativeInfinity.
		 *	@return A boolean
		 *	@spec ejs-11
		 */
		native function get isFinite(): Boolean


		/**
		 *	Returns true if the number is equal to the NaN value. If the numeric type is integral, this will 
         *	    always return false.
		 *	@return A boolean
		 *	@spec ejs-11
		 */
		native function get isNaN(): Boolean


		/**
		 *	Compute the integral number that is closest to this number. Returns the closest integer value of this number 
		 *	closest to the number. ie. round up or down to the closest integer.
		 *	@return A integral number.
		 *	@spec ejs-11
		 */
		native function get round(): Number


		/**
		 *	Returns the number formatted as a string in scientific notation with one digit before the decimal point 
		 *	and the argument number of digits after it.
		 *	@param fractionDigits The number of digits in the fraction.
		 *	@return A string representing the number.
		 */


		/**
		 *	Returns the number formatted as a string with the specified number of digits after the decimal point.
		 *	@param fractionDigits The number of digits in the fraction.
		 *	@return A string
		 */
		native function toFixed(fractionDigits: Number = 0): String


		/**
		 *	Returns the number formatted as a string in either fixed or exponential notation with argument number of digits.
		 *	@param numDigits The number of digits in the result
		 *	@return A string
		 *	@spec ejs-11
		 */
		native function toPrecision(numDigits: Number = SOME_DEFAULT): String


		/**
		 *	Returns the absolute value of a number (which is equal to its magnitude).
		 *	@return the absolute value.
		 *	@spec ejs-11
		 */
		native function get abs(): Number


		/**
		 *	Convert this number to a byte sized integral number. Numbers are rounded and truncated as necessary.
		 *	@return A byte
		 *	@spec ejs-11
		 */
        # FUTURE
		function get byte(): Number {
			return integer(8)
		}


		/**
		 *	Convert this number to an integral value. Floating point numbers are converted to integral values 
         *	    using truncation.
		 *	@size Size in bits of the value
		 *	@return An integral number
		 *	@spec ejs-11
		 */
		# FUTURE
		native function get integer(size: Number = 32): Number


		/**
		 *	Return an iterator that can be used to iterate a given number of times. This is used in for/in statements.
		 *	@param deep Ignored
		 *	@return an iterator
		 *	@example
		 *		for (i in 5) 
		 *			print(i)
		 *	@spec ejs-11
		 */
		override iterator native function get(deep: Boolean = false): Iterator


		/**
		 *	Return an iterator that can be used to iterate a given number of times. This is used in for/each statements.
		 *	@param deep Ignored
		 *	@return an iterator
		 *	@example
		 *		for each (i in 5) 
		 *			print(i)
		 *	@spec ejs-11
		 */
		override iterator native function getValues(deep: Boolean = false): Iterator


		/**
		 *	Returns the greater of the number or the argument.
		 *	@param other The number to compare to
		 *	@return A number
		 *	@spec ejs-11
		 */
		function max(other: Number): Number {
			return this > other ? this : other
		}


		/**
		 *	Returns the lessor of the number or the argument.
		 *	@param other The number to compare to
		 *	@return A number
		 *	@spec ejs-11
		 */
		function min(other: Number): Number {
			return this < other ? this : other
		}


		/**
		 *	Returns a number which is equal to this number raised to the power of the argument.
		 *	@param limit The number to compare to
		 *	@return A number
		 *	@spec ejs-11
		 */
		function power(power: Number): Number {
			var result: Number = this
			for (i in power) {
				result *= result
			}
			return result
		}

		/*
			Operators: /.  for truncating division
		 */
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
