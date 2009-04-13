/*
 *	String.es -- String class
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs {

	use default namespace intrinsic

	/**
	 *	Each String object represents a single immutable linear sequence of characters. Strings have operators 
	 *	for: comparison, concatenation, copying, searching, conversion, matching, replacement, and, subsetting.
	 */
	native final class String {

        use default namespace public

		/**
		 *	String constructor. This can take two forms:
		 *	<ul>
		 *		<li>String()</li>
		 *		<li>String(str: String)</li>
		 *	</ul>
		 *	@param The args can be either empty or a string. If a non-string arg is supplied, the VM will automatically
         *	    cast to a string.
		 */
		native function String(...str)


		/**
		 *	Do a case sensitive comparison between this string and another.
		 *	@param The string to compare against
		 *	@return -1 if less than, zero if equal and 1 if greater than.
		 *	@spec ejs-11
		 */
		native function caseCompare(compare: String): Number


		/**
		 *	Return the character at a given position in the string
		 *	@returns a new string containing the selected character.
		 *	@throws RangeException
		 *	@spec ecma-3
		 */
		native function charAt(index: Number): String


		/**
		 *	Get a character code. 
		 *	@param The index of the character to retrieve
		 *	@return Return the character code at the specified index. If the index is -1, get the last character.
		 *	@throws OutOfBoundsError If the index is less then -1 or greater then or equal to the size of string.
		 *	@spec ecma-3
		 */
		native function charCodeAt(index: Number = 0): Number


		/**
		 *	Concatenate strings and returns a new string. 
		 *	@param args Strings to append to this string
		 *	@return Return a new string.
		 *	@spec ecma-3
		 */
		native function concat(...args): String


        //  TODO - change to (String | RegExp)
		/**
		 *	Check if a string contains a pattern.
		 *	@param pattern The pattern can be either a string or regular expression.
		 *	@return Returns true if the pattern is found.
		 *	@spec ejs-11
		 */
		native function contains(pattern: Object): Boolean


		/**
		 *	Determine if this string ends with a given string
		 *	@param test The string to test with
		 *	@return True if the string matches.
		 *	@spec ejs-11
		 */
		native function endsWith(test: String): Boolean


		/**
		 *	Format arguments as a string. Use the string as a format specifier.
		 *	@param args Array containing the data to format. 
		 *	@return -1 if less than, zero if equal and 1 if greater than.
		 *	@example
		 *		"%5.3f".format(num)
         *  \n\n
		 *		"%s Arg1 %d, arg2 %d".format("Hello World", 1, 2)
		 *	@spec ejs-11
		 */
		native function format(...args): String


		/**
		 *	Create a string from the character code arguments
		 *	@param codes Character codes from which to create the string
		 *	@returns a new string
		 *	@spec ecma-3
		 */
		native static function fromCharCode(...codes): String
		

		/**
		 *	Get an iterator for this array to be used by "for (v in string)"
		 *	@param deep Follow the prototype chain. Not used.
		 *	@return An iterator object.
		 *	@spec ecma-3
		 */
		override iterator native function get(deep: Boolean = false): Iterator


		/**
		 *	Get an iterator for this array to be used by "for each (v in string)"
		 *	@param deep Follow the prototype chain. Not used.
		 *	@return An iterator object.
		 *	@spec ecma-3
		 */
		override iterator native function getValues(deep: Boolean = false): Iterator


		/**
		 *	Search for an item using strict equality "===". This call searches from the start of the string for 
		 *	the specified element. 
		 *	@param pattern The string to search for.
		 *	@param startIndex Where in the array (zero based) to start searching for the object.
		 *	@return The items index into the array if found, otherwise -1.
		 *	@throws OutOfBoundsError If the starting index is greater than or equal to the size of the array or less then 0.
		 *	@spec ecma-3
		 */
		native function indexOf(pattern: String, startIndex: Number = 0): Number


		/**
		 *	If there is at least one character in the string and all characters are digits return true.
		 *	@return False otherwise
		 *	@spec ejs-11
		 */
		native function get isDigit(): Boolean


        //  TODO - need isAlphaNum
		/**
		 *	If there is at least one character in the string and all characters are alphabetic return true.
		 *	@return False otherwise
		 *	@spec ejs-11
		 */
		native function get isAlpha(): Boolean


		/**
		 *	If there is at least one character in the string that can be upper or lower case and all characters 
		 *	are lower case return true.
		 *	@return False otherwise
		 *	@spec ejs-11
		 */
		native function get isLower(): Boolean


		/**
		 *	If there is at least one character in the string and all characters are white space return true.
		 *	@return False otherwise
		 *	@spec ejs-11
		 */
		native function get isSpace(): Boolean


		/**
		 *	If there is at least one character in the string that can be upper or lower case and all characters are upper 
		 *	case return true.
		 *	@return False otherwise
		 *	@spec ejs-11
		 */
		native function get isUpper(): Boolean


		/**
		 *	Search right to left for a substring starting at a given index.
		 *	@param pattern The string to search for
		 *	@param location The integer starting to start the search or a range to search in.
		 *	@throws OutOfBoundsError If the index is less then -1 or greater then or equal to the size of string.
		 *	@return Return the starting index of the last match found.
		 *	@spec ecma-3
		 */
		native function lastIndexOf(pattern: String, location: Number = -1): Number


		/**
		 *	Get the length of a string. 
		 *	@return Return the length of the string in characters.
		 *	@spec ecma-3
		 */
		override native function get length(): Number


		/**
		 *	Compare another string with the this string object
		 *	@returns zero if the strings are equal, -1 if this string is lexically before @str and 1 if after.
		 *	@spec ecma-3
		 */
		# ECMA
		function localeCompare(str: String): Number {
			if (this < string) {
				return -1
			} else if (string == this) {
				return 0
			} else {
				return 1
			}
		}


        //  # Config.RegularExpressions - TODO
        //  TODO - should this allow a string?
		/**
		 *	Match the a regular expression pattern against a string.
		 *	@param pattern The regular expression to search for
		 *	@return Returns an array of matching substrings.
		 *	@spec ecma-3, ejs-11 allows pattern to be a string
		 */
		native function match(pattern: RegExp): Array


		/**
		 *	Parse the current string object as a JSON string object. The @filter is an optional filter with the 
		 *	following signature:
		 *		function filter(key: String, value: String): Boolean
		 *	@param filter Function to call for each element of objects and arrays.
		 *	@returns an object representing the JSON string.
		 *	@spec ecma-3
		 */
		# ECMA
		native function parseJSON(filter: Function): Object


		/**
		 *	Copy the string into a new string and lower case the first letter if there is one. If the first non-white 
		 *	character is not a character or if it is already lower there is no change.
		 *	@return A new String
		 *	@spec ejs-11
		 */
		native function toCamel(): String


		/**
		 *	Copy the string into a new string and capitalize the first letter if there is one. If the first non-white 
		 *	character is not a character or if it is already capitalized there is no change.
		 *	@return A new String
		 *	@spec ejs-11
		 */
		native function toPascal(): String


		/**
		 *	Create a new string with all nonprintable characters replaced with unicode hexadecimal equivalents (e.g. \uNNNN).
		 *	@return The new string
		 *	@spec ejs-11
		 */
		native function printable(): String


		/**
		 *	Wrap a string in double quotes.
		 *	@return The new string
		 *	@spec ecma-3
		 */
		native function quote(): String


		/**
		 *	Remove characters from a string. Remove the elements from @start to @end inclusive. 
		 *	@param start Numeric index of the first element to remove. Negative indicies measure from the end of the string.
         *	-1 is the last character element.
		 *	@param end Numeric index of one past the last element to remove
		 *	@return A new string with the characters removed
		 *	@spec ejs-11
		 */
		native function remove(start: Number, end: Number = -1): String


		/**
		 *	Search and replace. Search for the given pattern which can be either a string or a regular expression 
		 *	and replace it with the replace text.
		 *	@param pattern The regular expression pattern to search for
		 *	@param replacement The string to replace the match with or a function to generate the replacement text
		 *	@return Returns a new string.
		 *	@spec ejs-11
		 */
		native function replace(pattern: Object, replacement: String): String


		/**
		 *	Reverse a string. 
		 *	@return Returns a new string with the order of all characters reversed.
		 *	@spec ejs-11
		 */
		native function reverse(): String

	
		/**
		 *	Search for a pattern.
		 *	@param pattern Regular expression pattern to search for in the string.
		 *	@return Return the starting index of the pattern in the string.
		 *	@spec ecma-3
		 */
		native function search(pattern: Object): Number


		/**
		 *	Extract a substring.
		 *	@param start The position of the first character to slice.
		 *	@param end The position one after the last character. Negative indicies are measured from the end of the string.
		 *	@throws OutOfBoundsError If the range boundaries exceed the string limits.
		 *	@spec ecma-3
		 */	
		native function slice(start: Number, end: Number = -1, step: Number = 1): String


		/**
		 *	Split a string into an array of substrings. Tokenizes a string using a specified delimiter.
		 *	@param delimiter String or regular expression object separating the tokens.
		 *	@param limit At most limit strings are extracted. If limit is set to -1, then unlimited strings are extracted.
		 *	@return Returns an array of strings.
		 *	@spec ecma-3
		 */
		native function split(delimiter: Object, limit: Number = -1): Array


		/**
		 *	Tests if this string starts with the string specified in the argument.
		 *	@param test String to compare against
		 *	@return True if it does, false if it doesn't
		 *	@spec ejs-11
		 */
		native function startsWith(test: String): Boolean


		/**
		 *	Extract a substring. Similar to substring, but utilizes a length.
		 *	@param startIndex Integer location to start copying
		 *	@param length Number of characters to copy
		 *	@return Returns a new string
		 *	@throws OutOfBoundsError If the starting index and/or the length exceed the string's limits.
		 *	@spec ecma-3
		 */
		# ECMA
		native function substr(startIndex: Number, length: Number = -1): String


		/**
		 *	Extract a substring. Similar to slice but only allows positive indicies.
		 *	@param startIndex Integer location to start copying
		 *	@param end Postitive index of one past the last character to extract.
		 *	@return Returns a new string
		 *	@throws OutOfBoundsError If the starting index and/or the length exceed the string's limits.
		 *	@spec ecma-3
		 */
		native function substring(startIndex: Number, end: Number = -1): String


		/**
		 *	Replication. Replicate the string N times.
		 *	@param str The number of times to copy the string
		 *	@return A new String with the copies concatenated together
		 *	@spec ejs-11
		 */
		function times(times: Number): String {
			var s: String = ""

			for (i in times) {
				s += this
			}
			return s
		}


		/**
		 *	Serialize the string as a JSON string.
		 *	@returns a string containing the string serialized as a JSON string.
		 *	@spec ecma-3
		 */ 
		# ECMA
		native function toJSONString(pretty: Boolean = false): String


		//	TODO. Should this be the reverse?   for (s in "%s %s %s".tokenize(value))
        //  that would be more consistent with format()
		/**
		 *	Scan the input and tokenize according to a string format specifier.
		 *	@param format Tokenizing format specifier
		 *	@returns array containing the tokenized elements
		 *	@example
		 *		for (s in string.tokenize("%s %s %s")) {
		 *			print(s)
		 *		}
		 *	@spec ejs-11
		 */
		native function tokenize(format: String): Array


		/**
		 *	Convert the string to lower case.
		 *	@return Returns a new lower case version of the string.
		 *	@spec ejs-11
		 */
		native function toLower(locale: String = null): String


		/**
		 *	This function converts the string to a localized lower case string representation. 
		 *	@returns a lower case localized string representation of the string. 
		 *	@spec ecma-3
		 */ 
		# ECMA
		function toLocaleLower(): String {
			//	TODO should be getting from App.Locale not from date
			return toLowerCase(Date.LOCAL)
		}


		/**
		 *	This function converts the string to a localized string representation. 
		 *	@returns a localized string representation of the string. 
		 *	@spec ecma-3
		 */ 
		# ECMA
		override function toLocaleString(): String {
			return toString()
		}


		/**
		 *	This function converts an object to a string representation. Types typically override this to provide 
		 *	the best string representation.
		 *	@returns a string representation of the object. For Objects "[object className]" will be returned, 
		 *	where className is set to the name of the class on which the object was based.
		 *	@spec ecma-3
		 */ 
		override native function toString(locale: String = null): String


		/**
		 *	Convert the string to upper case.
		 *	@return Returns a new upper case version of the string.
		 *	@spec ejs-11
		 */
		native function toUpper(locale: String = null): String


		/**
		 *	Convert the string to localized upper case string
		 *	@return Returns a new localized upper case version of the string.
		 *	@spec ecma-3
		 */
		# ECMA
		function toLocaleUpperCase(locale: String = null): String {
			//	TODO should be getting from App.Locale not from date
			return toUpper(Date.LOCAL)
		}


//  TODO - great if this could take a regexp
		/**
		 *	Returns a trimmed copy of the string. Normally used to trim white space, but can be used to trim any 
		 *	substring from the start or end of the string.
		 *	@param str May be set to a substring to trim from the string. If not set, it defaults to any white space.
		 *	@return Returns a (possibly) modified copy of the string
		 *	@spec ecma-3
		 */
		native function trim(str: String = null): String


		/**
		 *	Return the value of the object
		 *	@returns this object.
		 *	@spec ecma-3
		 */ 
		# ECMA
		override function valueOf(): String {
			return this
		}


		/*
		 *	Overloaded operators
		 */

		/**
		 *	String subtraction. Remove the first occurance of str.
		 *	@param str The string to remove from this string
		 *	@return Return a new string.
		 *	@spec ejs-11
		 */
		function - (str: String): String {
			var i: Number = indexOf(str)
			return remove(i, i + str.length)
		}

		
		//	TODO - delegate to localeCompare
		/**
		 *	Compare strings. 
		 *	@param str The string to compare to this string
		 *	@return -1 if less than, zero if equal and 1 if greater than.
		 *	@spec ejs-11
		 */
		# DOC_ONLY
		function < (str: String): Number {
			return localeCompare(str) < 0
		}


		/**
		 *	Compare strings.
		 *	@param str The string to compare to this string
		 *	@return -1 if less than, zero if equal and 1 if greater than.
		 *	@spec ejs-11
		 */
		# DOC_ONLY
		function > (str: String): Number {
			return localeCompare(str) > 0
		}


		/**
		 *	Compare strings.
		 *	@param str The string to compare to this string
		 *	@return -1 if less than, zero if equal and 1 if greater than.
		 *	@spec ejs-11
		 */
		# DOC_ONLY
		function == (str: String): Number {
			return localeCompare(str) == 0
		}


		/**
		 *	Format arguments as a string. Use the string as a format specifier.
		 *	@param arg The arguments to format. Pass an array if multiple arguments are required.
		 *	@return -1 if less than, zero if equal and 1 if greater than.
		 *	@example
		 *		"%5.3f" % num
         *  <br/>
		 *		"Arg1 %d, arg2 %d" % [1, 2]
		 *	@spec ejs-11
		 */
		function % (obj: Object): String {
			return format(obj)
		}
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
