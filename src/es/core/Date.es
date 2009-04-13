/*
 *	Date.es -- Date class
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs {

	use default namespace intrinsic

	/**
	 *	General purpose class for representing and working with dates, times, time spans, time zones and locales.
	 */
	native final class Date {

        use default namespace public

		/**
		 *	Construct a new date object. Permissible constructor forms:
         *	<ul>
		 *		<li>Date()</li>
		 *		<li>Date(milliseconds)</li>
		 *		<li>Date(dateString)</li>
		 *		<li>Date(year, month, date)</li>
		 *		<li>Date(year, month, date, hour, minute, second)</li>
         *  </ul>
		 */
        native function Date(...args)


		/**
		 *	Return the day of the week in local time.
		 *	@return The integer day of the week (0 - 6, where 0 is Sunday)
		 */
		native function get day(): Number 

//  TODO - why are we missing the setters?

		/**
		 *	Return the day of the year in local time.
		 *	@return The integer day of the year (0 - 366)
		 *	@spec ejs-11
		 */
		native function get dayOfYear(): Number 


		 //	TODO not a great name but matches getDate()
		/**
		 *	Return the day of the month.
		 *	@return Returns the day of the year (1-31)
		 */
		native function get date(): Number 


		/**
		 *	Time in milliseconds since the date object was constructed
		 */
		native function get elapsed(): Number


		/**
		 *	Return the year as four digits.
		 *	@return The year
		 */
		native function get fullYear(): Number 


		/**
		 *	Set the year as four digits according to the locale
		 *	@param year Year to set.
		 */
		native function set fullYear(year: Number): void


		/**
		 *	Return a formatted date string of the date. This corresponds to the C language strftime conventionts. 
		 *	The format specifiers are:
		 *
		 *	<ul>
		 *	<li>%A    national representation of the full weekday name.</li>
		 *	<li>%a    national representation of the abbreviated weekday name.</li>
		 *	<li>%B    national representation of the full month name.</li>
		 *	<li>%b    national representation of the abbreviated month name.</li>
		 *	<li>%C    (year / 100) as decimal number; single digits are preceded by a zero.</li>
		 *	<li>%c    national representation of time and date.</li>
		 *	<li>%D    is equivalent to ``%m/%d/%y''.</li>
		 *	<li>%d    the day of the month as a decimal number (01-31).</li>
		 *	<li>%E* %O* POSIX locale extensions.  The sequences %Ec %EC %Ex %EX %Ey %EY %Od %Oe %OH %OI %Om %OM %OS %Ou
		 *		   %OU %OV %Ow %OW %Oy are supposed to provide alternate representations. Additionly %OB implemented
		 *		   to represent alternative months names (used standalone,
		 *		   without day mentioned).</li>
		 *	<li>%e    the day of month as a decimal number (1-31); single digits are preceded by a blank.</li>
		 *	<li>%F    is equivalent to ``%Y-%m-%d''.</li>
		 *	<li>%G    a year as a decimal number with century.  This year is the one that contains the greater part of
		 *	       the week (Monday as the first day of the week).</li>
		 *	<li>%g    the same year as in ``%G'', but as a decimal number without century (00-99).</li>
		 *	<li>%H    the hour (24-hour clock) as a decimal number (00-23).</li>
		 *	<li>%h    the same as %b.</li>
		 *	<li>%I    the hour (12-hour clock) as a decimal number (01-12).</li>
		 *	<li>%j    the day of the year as a decimal number (001-366).</li>
		 *	<li>%k    the hour (24-hour clock) as a decimal number (0-23); single digits are preceded by a blank.</li>
		 *	<li>%l    the hour (12-hour clock) as a decimal number (1-12); single digits are preceded by a blank.</li>
		 *	<li>%M    the minute as a decimal number (00-59).</li>
		 *	<li>%m    the month as a decimal number (01-12).</li>
		 *	<li>%m    the month as a decimal number (01-12).</li>
		 *	<li>%n    a newline.</li>
		 *	<li>%O*   the same as %E*.</li>
		 *	<li>%p    national representation of either "ante meridiem" or "post meridiem" as appropriate.</li>
		 *	<li>%R    is equivalent to ``%H:%M''.</li>
		 *	<li>%r    is equivalent to ``%I:%M:%S %p''.</li>
		 *	<li>%S    the second as a decimal number (00-60).</li>
		 *	<li>%s    the number of seconds since the Epoch, UTC (see mktime(3)).</li>
		 *	<li>%T    is equivalent to ``%H:%M:%S''.</li>
		 *	<li>%t    a tab.</li>
		 *	<li>%U    the week number of the year (Sunday as the first day of the week) as a decimal number (00-53).</li>
		 *	<li>%u    the weekday (Monday as the first day of the week) as a decimal number (1-7).</li>
		 *	<li>%V    the week number of the year (Monday as the first day of the week) as a decimal
		 *		   number (01-53).  If the week containing January 1 has four or more days in the new year, then it
		 *		   is week 1; otherwise it is the last week of the previous year, and the next week is week 1.</li>
		 *	<li>%v    is equivalent to ``%e-%b-%Y''.</li>
		 *	<li>%W    the week number of the year (Monday as the first day of the week) as a decimal number (00-53).</li>
		 *	<li>%w    the weekday (Sunday as the first day of the week) as a decimal number (0-6).</li>
		 *	<li>%X    national representation of the time.</li>
		 *	<li>%x    national representation of the date.</li>
		 *	<li>%Y    the year with century as a decimal number.</li>
		 *	<li>%y    the year without century as a decimal number (00-99).</li>
		 *	<li>%Z    the time zone name.</li>
		 *	<li>%z    the time zone offset from UTC; a leading plus sign stands for east of UTC, a minus
		 *		   sign for west of UTC, hours and minutes follow with two digits each and no delimiter between them
		 *		   (common form for RFC 822 date headers).</li>
		 *	<li>%+    national representation of the date and time (the format is similar to that produced by date(1)).</li>
		 *	<li>%%    Literal percent.</li>
		 *	</ul>
		 *
		 *	@param layout Format layout.
		 *	@return string representation of the date.
		 *	@spec ejs-11
		 */
		native function format(layout: String): String 


		/**
		 *	Return the day of the week in local time.
		 *	@return The integer day of the week (0 - 6, where 0 is Sunday)
		 */
		# ECMA
		native function getDay(): Number 


		/**
		 *	Return the day of the month.
		 *	@return Returns the day of the year (1-31)
		 */
		# ECMA
		native function getDate(): Number 


		/**
		 *	Return the year as four digits.
		 *	@return The integer year
		 */
		# ECMA
		native function getFullYear(): Number 


		/**
		 *	Return the hour (0 - 23) in local time.
		 *	@return The integer hour of the day
		 */
		# ECMA
		native function getHours(): Number 


		/**
		 *	Return the millisecond (0 - 999) in local time.
		 *	@return The number of milliseconds as an integer
		 */
		# ECMA
		native function getMilliseconds(): Number 


		/**
		 *	Return the minute (0 - 59) in local time.
		 *	@return The number of minutes as an integer
		 */
		# ECMA
		native function getMinutes(): Number 


		/**
		 *	Return the month (1 - 12) in local time.
		 *	@return The month number as an integer
		 */
		# ECMA
		native function getMonth(): Number 


		/**
		 *	Return the second (0 - 59) in local time.
		 *	@return The number of seconds as an integer
		 */
		# ECMA
		native function getSeconds(): Number 


		/**
		 *	Return the number of milliseconds since midnight, January 1st, 1970.
		 *	@return The number of milliseconds as a long
		 */
		# ECMA
		static function getTime(): Number {
			return time
		}


		/**
		 *	Return the month (1 - 12) in UTC time.
		 *	@return The month number as an integer
		 */
		# ECMA
		native function getUTCMonth(): Number 


		/**
		 *	Return the number of minutes between the local computer time and Coordinated Universal Time.
		 *	@return The number of minutes as an integer
		 */
		# ECMA
		native function getTimezoneOffset(): Number


		/**
		 *	Return the current hour (0 - 23) in local time.
		 *	@return The integer hour of the day
         *	TODO - should this be hour? for consistency with day
		 */
		native function get hours(): Number 


		/**
		 *	Set the current hour (0 - 59) according to the locale
		 *	@param The hour as an integer
		 */
		native function set hours(hour: Number): void


		/**
		 *	Return the current millisecond (0 - 999) in local time.
		 *	@return The number of milliseconds as an integer
		 */
		native function get milliseconds(): Number 


		/**
		 *	Set the current millisecond (0 - 999) according to the locale
		 *	@param The millisecond as an integer
		 */
		native function set milliseconds(ms: Number): void


		/**
		 *	Return the current minute (0 - 59) in local time.
		 *	@return The number of minutes as an integer
		 */
		native function get minutes(): Number 


		/**
		 *	Set the current minute (0 - 59) according to the locale
		 *	@param The minute as an integer
		 */
		native function set minutes(min: Number): void


		/**
		 *	Return the current month (0 - 11) in local time.
		 *	@return The month number as an integer
		 */
		native function get month(): Number 


		/**
		 *	Set the current month (0 - 11) according to the locale
		 *	@param The month as an integer
		 */
		native function set month(month: Number): void



		/**
		 *	Time in nanoseconds since the date object was constructed
		 */
        # ECMA
		function nanoAge(): Number {
            return elapsed() * 1000
        }


		//    TODO - could take an arg for N days forward or backward
		/**
		 *	Return a new Date object that is one day greater than this one.
		 *  @param inc Increment in days to add (or subtract if negative)
		 *	@return A Date object
		 *	@spec ejs-11
		 */
		native function nextDay(inc: Number = 1): Date


		/**
		 *	Return the current time as milliseconds since Jan 1 1970.
		 */
		static native function now(): Number


		/**
		 *	Return a new Date object by parsing the argument string.
		 *	@param arg The string to parse
		 *	@param defaultDate Default date to use to fill out missing items in the date string.
		 *	@return Return a new Date.
		 *	@spec ejs-11
		 */
		static native function parseDate(arg: String, defaultDate: Date = undefined): Date


		/**
		 *	Return a new Date object by parsing the argument string.
		 *	@param arg The string to parse
		 *	@param defaultDate Default date to use to fill out missing items in the date string.
		 *	@return Return a new date number.
		 */
		# ECMA
		static function parse(arg: String, defaultDate: Number = undefined): Number {
            var d: Date = parseDate(arg, defaultDate)
            return d.time
        }


		/**
		 *	Return the current second (0 - 59) in local time.
		 *	@return The number of seconds as an integer
		 */
		native function get seconds(): Number 


		/**
		 *	Set the current second (0 - 59) according to the locale
		 *	@param The second as an integer
		 */
		native function set seconds(sec: Number): void


		/**
		 *	Set the current year as four digits according to the locale
		 */
		# ECMA
		native function setFullYear(year: Number): void


		/**
		 *	Set the current hour (0 - 59) according to the locale
		 *	@param The hour as an integer
		 */
		# ECMA
		native function setHours(hour: Number): void


		/**
		 *	Set the current millisecond (0 - 999) according to the locale
		 *	@param The millisecond as an integer
		 */
		# ECMA
		native function setMilliseconds(ms: Number): void


		/**
		 *	Set the current minute (0 - 59) according to the locale
		 *	@param The minute as an integer
		 */
		# ECMA
		native function setMinutes(min: Number): void


		/**
		 *	Set the current month (0 - 11) according to the locale
		 *	@param The month as an integer
		 */
		# ECMA
		native function setMonth(month: Number): void


		/**
		 *	Set the current second (0 - 59) according to the locale
		 *	@param The second as an integer
		 */
		# ECMA
		native function setSeconds(sec: Number, milli: Number = getMilliseconds()): void


		/**
		 *	Set the number of milliseconds since midnight, January 1st, 1970.
		 *	@param The millisecond as a long
		 */
		# ECMA
		native function setTime(ms: Number): void


		/**
		 *	Return the number of milliseconds since midnight, January 1st, 1970 and the current date object.
		 *	@return The number of milliseconds as a long
		 */
		native static function get time(): Number 


		/**
		 *	Set the number of milliseconds since midnight, January 1st, 1970.
		 *	@return The number of milliseconds as a long
		 */
		native static function set time(value: Number): Number 


		/**
		 *	Return a localized string containing the date portion excluding the time portion of the date according to
		 *	the specified locale.
		 *	Sample format: "Fri, 15 Dec 2006 GMT-0800"
		 *	@return A string representing the date portion.
		 */
		# ECMA
		native function toDateString(locale: String = null): String 


		/**
		 *	Return an ISO formatted date string.
		 *	Sample format: "2006-12-15T23:45:09.33-08:00"
		 *	@return An ISO formatted string representing the date.
		 */
		# ECMA
		native function toISOString(): String 


		/**
		 *	Return a JSON encoded date string.
		 *	@return An JSON formatted string representing the date.
		 */
		# ECMA
		native function toJSONString(pretty: Boolean = false): String 


		/**
		 *	Return a localized string containing the date portion excluding the time portion of the date according to
		 *	the current locale.
		 *	Sample format: "Fri, 15 Dec 2006 GMT-0800"
		 *	@return A string representing the date portion.
		 */
		# ECMA
		native function toLocaleDateString(): String 


		/**
		 *	Return a localized string containing the date.
		 *	Sample format: "Fri, 15 Dec 2006 23:45:09 GMT-0800"
		 *	@return A string representing the date.
		 */
		# ECMA
		override native function toLocaleString(): String 


		/**
		 *	Return a string containing the time portion of the date according to the current locale.
		 *	Sample format: "23:45:09 GMT-0800"
		 *	@return A string representing the time.
		 */
		# ECMA
		native function toLocaleTimeString(): String 


		/**
		 *	Return a string containing the date according to the locale.
		 *	Sample format: "Fri, 15 Dec 2006 23:45:09 GMT-0800"
		 *	@return A string representing the date.
		 */
		override native function toString(locale: String = null): String 


		/**
		 *	Return a string containing the time portion of the date according to the locale.
		 *	Sample format: "23:45:09 GMT-0800"
		 *	@return A string representing the time.
		 */
		# ECMA
		native function toTimeString(): String 


		/**
		 *	Return a string containing the date according to the locale.
		 *	Sample format: "Sat, 16 Dec 2006 08:06:21 GMT"
		 *	@return A string representing the date.
		 */
		# ECMA
		native function toUTCString(): String 


		/**
		 *	Construct a new date object interpreting its arguments in UTC rather than local time.
		 *	@param year Year
		 *	@param month Month of year
		 *	@param day Day of month
		 *	@param hours Hour of day
		 *	@param minutes Minute of hour
		 *	@param seconds Secods of minute
		 *	@param milliseconds Milliseconds of second
		 */
		# ECMA
        native function UTC(year: Number, month: Number, day: Number = NOARG, hours: Number = NOARG, 
			minutes: Number = NOARG, seconds: Number = NOARG, milliseconds: Number = NOARG): Date


//  TODO - should this be 0-11?
		/**
		 *	Return the current month (1 - 12) in UTC time.
		 *	@return The month number as an integer
		 */
		# ECMA
		native function get UTCmonth(): Number 


		/**
		 *	Return the value of the object
		 *	@returns this object.
		 */ 
		# ECMA
		override function valueOf(): String {
			return getTime()
		}

		/**
		 *	Return the current year as two digits.
		 *	@return The integer year
		 */
		native function get year(): Number 


		/**
		 *	Set the current year as two digits according to the locale
		 *	@param year Year to set.
		 */
		native function set year(year: Number): void



		/**
		 *	Date difference. Return a new Date that is the difference of the two dates.
		 *	@param The operand date
		 *	@return Return a new Date.
		 */
		# TODO
		native function -(date: Date): Date


/*	TODO - ECMA needs these

		Would be nice to be able to do:

		var d: Date 
		d.locale = "en_us"
		d.locale = "UTC"
		when = d.month

		native function getUTCDate(): String
		native function getUTCFullYear(): String
		native function getUTCMonth(): String
		native function getUTCDay(): String
		native function getUTCHours(): String
		native function getUTCMinutes(): String
		native function getUTCSeconds(): String
		native function getUTCMilliSeconds(): String
		native function setUTCDate(): void
		native function setUTCFullYear(): void
		native function setUTCMonth(): void
		native function setUTCDay(): void
		native function setUTCHours(): void
		native function setUTCMinutes(): void
		native function setUTCSeconds(): void
		native function setUTCMilliSeconds(): void
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
