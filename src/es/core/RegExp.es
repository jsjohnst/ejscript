/*
 *	Regex.es -- Regular expression class.
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs {

	use default namespace intrinsic

    //  # Config.RegularExpressions - TODO 
	/**
	 *	Regular expressions per ECMA-262. Ecmascript 4 will be supported in the future. The following special 
	 *	characters are supported:
     *	<ul>
	 *		<li>\ - Reverse whether a character is treated	literally or not.</li>
	 *		<li>^ - Match to the start of input. If matching multiline, match starting after a line break.</li>
	 *		<li>$ - Match to the end of input. If matching multiline, match before after a line break.</li>
	 *		<li>~~ - Match the preceding item zero or more	times.</li>
	 *		<li>+ - Match the preceding item one or more	times.</li>
	 *		<li>? - Match the preceding item zero or one times.</li>
	 *		<li>(mem) - Match inside the parenthesis (i.e. "mem") and store the match.</li>
	 *		<li>(?:nomem) - Match "nomem" and do not store the match.</li>
	 *		<li>oper(?=need) - Match "oper" only if it is  followed by "need".</li>
	 *		<li>oper(?!not) - Match "oper" only if it is not followed by "not".</li>
	 *		<li>either|or - Match "either" or "or".</li>
	 *		<li>{int} - Match exactly int occurences of the preceding item.</li>
	 *		<li>{int,} - Match at least int occurences of the preceding item.</li>
	 *		<li>{int1,int2} - Match at least int1 occurences of the preceding item but no more then int2.</li>
	 *		<li>[pqr] - Match any one of the enclosed characters. Use a hyphen to specify a range of characters.</li>
	 *		<li>[^pqr] - Match anything except the characters in brackets.</li>
	 *		<li>[\b] - Match a backspace.</li>
	 *		<li>\b - Match a word boundary.</li>
	 *		<li>\B - Match a non-word boundary.</li>
	 *		<li>\cQ - Match a control string, e.g. Control-Q</li>
	 *		<li>\d - Match a digit.</li>
	 *		<li>\D - Match any non-digit character.</li>
	 *		<li>\f - Match a form feed.</li>
	 *		<li>\n - Match a line feed.</li>
	 *		<li>\r - Match a carriage return.</li>
	 *		<li>\s - Match a single white space.</li>
	 *		<li>\S - Match a non-white space.</li>
	 *		<li>\t - Match a tab.</li>
	 *		<li>\v - Match a vertical tab.</li>
	 *		<li>\w - Match any alphanumeric character.</li>
	 *		<li>\W - Match any non-word character.</li>
	 *		<li>\int - A reference back int matches.</li>
	 *		<li>\0 - Match a null character.</li>
	 *		<li>\xYY - Match the character code YY.</li>
	 *		<li>\xYYYY - Match the character code YYYY.</li>
     *	</ul>
	 */
	native final class RegExp {

        use default namespace public

		/**
		 *	Create a regular expression object that can be used to process strings.
		 *	@param pattern The pattern to associated with this regular expression.
		 *	@param flags "g" for global match, "i" to ignore case, "m" match over multiple lines, "y" for sticky match.
		 */
		native function RegExp(pattern: String, flags: String = null)


		/**
		 *	Get the integer index of the end of the last match plus one. This is the index to start the next match for
         *  global patterns.
		 *	@return Match end plus one or -1 if there is no last match.
		 */
		native function get lastIndex(): Number


		/**
		 *	Set the integer index of the end of the last match plus one. This is the index to start the next match for
         *  global patterns.
		 *	@return Match end plus one or -1 if there is no last match.
		 */
		native function set lastIndex(value: Number): Void


		/**
		 *	Match this regular expression against a string. By default, the matching starts at the beginning 
		 *	of the string.
		 *	@param str String to match.
		 *	@param start Optional starting index for matching.
		 *	@return Array of results, empty array if no matches.
		 *	@spec ejs-11 Adds start argument.
		 */
		native function exec(str: String, start: Number = 0): Array


		/**
		 *	Get the global flag this regular expression is using. If the global flag is set, the regular expression 
		 *	will search through the entire input string looking for matches.
		 *	@return The global flag, true if set, false otherwise.
		 *	@spec ejs-11
		 */
		native function get global(): Boolean


		/**
		 *	Get the case flag this regular expression is using. If the ignore case flag is set, the regular expression 
		 *	is case insensitive.
		 *	@return The case flag, true if set, false otherwise.
		 *	@spec ejs-11
		 */
		native function get ignoreCase(): Boolean


		/**
		 *	Get the multiline flag this regular expression is using. If the multiline flag is set, the regular 
		 *	expression will search through carriage return and new line characters in the input.
		 *	@return The multiline flag, true if set, false otherwise.
		 */
		native function get multiline(): Boolean


		/**
		 *	Get the regular expression source pattern is using to match with.
		 *	@return The pattern string
		 */
		native function get source(): String


		/**
		 *	Get the substring that was last matched.
		 *	@return The matched string or null if there were no matches.
		 *	@spec ejs-11
		 */
		native function get matched(): String


		/**
		 *	Replace all the matches. This call replaces all matching substrings with the corresponding array element.
		 *	If the array element is not a string, it is converted to a string before replacement.
		 *	@param str String to match and replace.
		 *	@return A string with zero, one or more substitutions in it.
		 *	@spec ejs-11
		 */
		function replace(str: String, replacement: Object): String {
            return str(this, replacement)
        }


		/**
		 *	Split the target string into substrings around the matching sections.
		 *	@param String to split.
		 *	@return Array containing the matching substrings
		 *	@spec ejs-11
		 */
		function split(target: String): Array {
            return target.split(this)
        }


		/**
		 *	Get the integer index of the start of the last match.
		 *	@return Match start.
		 *	@spec ejs-11
		 */
		native function get start(): Number


		/**
		 *	Get the sticky flag this regular expression is using. If the sticky flag is set, the regular expression 
		 *	contained the character flag "y".
		 *	@return The sticky flag, true if set, false otherwise.
		 *	@spec ejs-11
		 */
		native function get sticky(): Boolean


		/**
		 *	Test whether this regular expression will match against a string.
		 *	@param str String to search.
		 *	@return True if there is a match, false otherwise.
		 *	@spec ejs-11
		 */
		native function test(str: String): Boolean


		/**
		 *	Convert the regular expression to a string
		 *	@param locale Locale 
		 *	@returns a string representation of the regular expression.
		 */
		override native function toString(locale: String = null): String
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
