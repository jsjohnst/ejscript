/**
 *	Array.es - Array class
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs {

	use default namespace intrinsic

	/*
	 *	TODO 
	 *		- ECMA should be dynamic
	 *		- removeByElement(e) - remove an array element by reference
	 */

	/**
	 *	Arrays provide a growable, integer indexed, in-memory store for objects. An array can be treated as a 
	 *	stack (FIFO or LIFO) or a list (ordered). Insertions can be done at the beginning or end of the stack or at an 
	 *	indexed location within a list.
	 *	@spec ecma-262
	 */
	dynamic native class Array {

        use default namespace public

		/**
		 *	Create a new array.
		 *	@param values Initialization values. The constructor allows three forms:
		 *	<ul>
		 *		<li>Array()</li>
		 *		<li>Array(size: Number)</li>
		 *		<li>Array(elt: Object, ...args)</li>
		 *	</ul>
		 *	@spec ecma-3
		 */
		native function Array(...values)


		/**
		 *	Append an item to the array.
		 *	@param obj Object to add to the array 
		 *	@return The array itself.
		 *	@spec ejs-11
		 */
		native function append(obj: Object): Array


		/**
		 *	Clear an array. Remove all elements of the array.
		 *	@spec ejs-11
		 */
		native function clear() : Void


		/**
		 *	Clone the array and all its elements.
		 *	@param deep If true, do a deep copy where all object references are also copied, and so on, recursively.
		 *	@spec ejs-11
		 */
		override native function clone(deep: Boolean = true) : Array


		/**
		 *	Compact an array. Remove all null elements.
		 *	@spec ejs-11
		 */
		native function compact() : Array


		/**
		 *	Concatenate the supplied elements with the array to create a new array. If any arguments specify an 
		 *	array, their elements are concatenated. This is a one level deep copy.
		 *	@param args A variable length set of values of any data type.
		 *	@return A new array of the combined elements.
		 *	@spec ecma-3
		 */
		native function concat(...args): Array 


		/**
		 *	See if the array contains an element using strict equality "===". This call searches from the start of the 
         *	array for the specified element.  
		 *	@param element The object to search for.
		 *	@return True if the element is found. Otherwise false.
		 *	@throws OutOfBoundsError If the starting index is greater than or equal to the size of the array or less then 0.
		 *	@spec ejs-11
		 */
		function contains(element: Object): Boolean {
            if (indexOf(element) >= 0) {
                return true
            } else {
                return false
            }
        }


		/**
		 *	Determine if all elements match.
		 *	Iterate over every element in the array and determine if the matching function is true for all elements. 
		 *	This method is lazy and will cease iterating when an unsuccessful match is found. The checker is called 
		 *	with the following signature:
		 *		function match(element: Object, elementIndex: Number, arr: Array): Boolean
		 *	@param match Matching function
		 *	@return True if the match function always returns true.
		 *	@spec ecma-3
		 */
		function every(match: Function): Boolean {
			for (let i: Number in this) {
				if (!match(this[i], i, this)) {
					return false
				}
			}
			return true
		}


		 //	TODO - how does this compare with JS1.8 reduce?
		/**
		 *	Find all matching elements. ECMA function doing the Same as findAll.
		 *	Iterate over all elements in the object and find all elements for which the matching function is true.
		 *	The match is called with the following signature:
		 *		function match(element: Object, elementIndex: Number, arr: Array): Boolean
		 *	This method is identical to the @findAll method.
		 *	@param match Matching function
		 *	@return Returns a new array containing all matching elements.
		 *	@spec ecma-3
		 */
		function filter(match: Function): Array {
			return findAll(match)
		}


		 //	TODO - should this return the index rather than the element?
		/**
		 *	Find the first matching element.
		 *	Iterate over elements in the object and select the first element for which the matching function is true.
		 *	The matching function is called with the following signature:
		 *		function match(element: Object, elementIndex: Number, arr: Array): Boolean
		 *	@param match Matching function
		 *	@return True when the match function returns true.
		 *	@spec ejs-11
		 */
		function find(match: Function): Object {
			var result: Array = new Array
			for (let i: Number in this) {
				if (match(this[i], i, this)) {
					return this[i]
				}
			}
			return result
		}


		/**
		 *	Find all matching elements.
		 *	Iterate over all elements in the object and find all elements for which the matching function is true.
		 *	The matching function is called with the following signature:
		 *		function match(element: Object, elementIndex: Number, arr: Array): Boolean
		 *	@param match Matching function
		 *	@return Returns an array containing all matching elements.
		 *	@spec ejs-11
		 */
		function findAll(match: Function): Array {
			var result: Array = new Array
			for (let i: Number in this) {
				if (match(this[i], i, this)) {
					result.append(this[i])
				}
			}
			return result
		}


		/**
		 *	Transform all elements.
		 *	Iterate over the elements in the array and transform all elements by applying the transform function. 
		 *	The matching function is called with the following signature:
		 *		function match(element: Object, elementIndex: Number, arr: Array): Boolean
		 *	This method is identical to the @transform method.
		 *	@param modifier Transforming function
		 *	@return Returns a new array of transformed elements.
		 *	@spec ecma-3
		 */
		function forEach(modifier: Function, thisObj: Object = null): Void {
			transform(modifier)
		}


		/**
		 *	Get an iterator for this array to be used by "for (v in array)"
		 *	@param deep Follow the prototype chain. Only implemented in ECMA compliance mode..
		 *	@return An iterator object.
		 *	@spec ecma-3
		 */
		override iterator native function get(deep: Boolean = false): Iterator


		/**
		 *	Get an iterator for this array to be used by "for each (v in array)"
		 *	@param deep Follow the prototype chain. Only implemented in ECMA compliance mode..
		 *	@return An iterator object.
		 *	@spec ecma-3
		 */
		override iterator native function getValues(deep: Boolean = false): Iterator


		/**
		 *	Search for an item using strict equality "===". This call searches from the start of the array for 
		 *	the specified element.  
		 *	@param element The object to search for.
		 *	@param startIndex Where in the array (zero based) to start searching for the object.
		 *	@return The items index into the array if found, otherwise -1.
		 *	@throws OutOfBoundsError If the starting index is greater than or equal to the size of the array or less then 0.
		 *	@spec ecma-3
		 */
		native function indexOf(element: Object, startIndex: Number = 0): Number


		/**
		 *	Insert elements. Insert elements at the specified position. The insertion occurs before the element at the 
		 *		specified position. Negative indicies will measure from the end so that -1 will specify the last element.  
		 *	@param pos Where in the array to insert the objects.
		 *	@param ...args Arguments are aggregated and passed to the method in an array.
		 *	@throws OutOfBoundsError If the starting index is greater than or equal to the size of the array or less then 0.
		 *	@return The original array.
		 *	@spec ejs-11
		 */
		native function insert(pos: Number, ...args): Array


		/**
		 *	Convert the array into a string.
		 *	Joins the elements in the array into a single string by converting each element to a string and then 
		 *	concatenating the strings inserting a separator between each.
		 *	@param sep Element separator.
		 *	@return A string.
		 *	@spec ecma-3
		 */
		native function join(sep: String = undefined): String


		/**
		 *	Find an item searching from the end of the arry.
		 *	Search for an item using strict equality "===". This call searches from the end of the array for the given 
         *	element.
		 *	@param element The object to search for.
		 *	@param startIndex Where in the array (zero based) to start searching for the object.
		 *	@return The items index into the array if found, otherwise -1.
		 *	@throws OutOfBoundsError If the starting index is greater than or equal to the size of the array or less then 0.
		 *	@spec ecma-3
		 */
		native function lastIndexOf(element: Object, fromIndex: Number = 0): Number


		/**
		 *	Get the length of an array.
		 *	@return The number of items in the array
		 */
		override native function get length(): Number


		/**
		 *	Set the length of an array. The array will be truncated if required. If the new length is greater then 
		 *	the old length, new elements will be created as required and set to undefined. If the new length is less
		 *	than 0 the length is set to 0.
		 *	@param value The new length
		 */
		native function set length(value: Number): Void


		/**
		 *	Call the mapper on each array element in increasing index order and return a new array with the values returned 
		 *	from the mapper. The mapper function is called with the following signature:
		 *		function mapper(element: Object, elementIndex: Number, arr: Array): Object
		 *	@param mapper Transforming function
		 *	@spec ecma-3
		 */
		function map(mapper: Function): Array {
			//	BUG TODO return clone().transform(mapper)
			var result: Array  = clone()
			result.transform(mapper)
			return result
		}


		/**
		 *	Remove and return the last value in the array.
		 *	@return The last element in the array. Returns undefined if the array is empty
		 *	@spec ecma-3
		 */
		native function pop(): Object 


		/**
		 *	Append items to the end of the array.
		 *	@param items Items to add to the array.
		 *	@return The new length of the array.
		 *	@spec ecma-3
		 */
		native function push(...items): Number 


		/**
		 *	Find non-matching elements. Iterate over all elements in the array and select all elements for which 
		 *	the filter function returns false. The matching function is called with the following signature:
		 *
		 *		function match(element: Object, elementIndex: Number, arr: Array): Boolean
		 *
		 *	@param match Matching function
		 *	@return A new array of non-matching elements.
		 *	@spec ejs-11
		 */
		function reject(match: Function): Array {
			var result: Array = new Array
			for (let i: Number in this) {
				if (!match(this[i], i, this)) {
					result.append(this[i])
				}
			}
			return result
		}


		/*
		 *	Remove elements. Remove the elements from $start to $end inclusive. The elements are removed and not just set 
		 *	to undefined as the delete operator will do. Indicies are renumbered. NOTE: this routine delegates to splice.
		 *	@param start Numeric index of the first element to remove. Negative indices measure from the end of the array.
		 *	-1 is the last element.
		 *	@param end Numeric index of the last element to remove
		 *	@spec ejs-11
		 */
		function remove(start: Number, end: Number = -1): Void {
			if (start < 0) {
				start += length
			}
			if (end < 0) {
				end += length
			}
			splice(start, end - start + 1)
		}


		/**
		 *	Reverse the order of the objects in the array. The elements are reversed in the original array.
		 *	@return A reference to the array.
		 *	@spec ecma-3
		 */
		native function reverse(): Array 


		/**
		 *	Remove and return the first element in the array.
		 *	@returns the previous first element in the array.
		 *	@spec ecma-3
		 */
		native function shift(): Object 


		 //	TODOO - with multimethods, it would be nice to do:
		 //		native function slice(range: Range): Array 
		/**
		 *	Create a new array subset by taking a slice from an array.
		 *	@param start The array index at which to begin. Negative indicies will measure from the end so that -1 will 
		 *		specify the last element.  
		 *	@param end One beyond the index of the last element to extract. The array index at which to begin. If 
		 *		end is negative, it is measured from the end of the array.  
		 *	@param step Slice every step (nth) element. The step value may be negative.
		 *	@return A new array that is a subset of the original array.
		 *	@throws OutOfBoundsError If the start or end are equal to or greater than the length of the array, or, if the 
		 *		start is greater than the end, or, either start or end is negative.
		 *	@spec ecma-3
		 */
		native function slice(start: Number, end: Number = -1, step: Number = 1): Array 


		/**
		 *	Determine if some elements match.
		 *	Iterate over all element in the array and determine if the matching function is true for at least one element. 
		 *	This method is lazy and will cease iterating when a successful match is found.
		 *	This method is identical to the ECMA @some method. The match function is called with the following signature:
		 *		function match(element: Object, elementIndex: Number, arr: Array): Boolean
		 *	@param match Matching function
		 *	@return True if the match function ever is true.
		 *	@spec ecma-3
		 */
		function some(match: Function): Boolean {
			var result: Array = new Array
			for (let i: Number in this) {
				if (match(this[i], i, this)) {
					return true
				}
			}
			return false
		}


		 //	TODO: need to use Comparator instead of Function
		/**
		 *	Sort the array using the supplied compare function
		 *	@param compare Function to use to compare. A null comparator will use a text compare
		 *	@param order If order is >= 0, then an ascending order is used. Otherwise descending.
		 *	@return the sorted array reference
		 *		type Comparator = (function (*,*): AnyNumber | undefined)
		 *	@spec emca-4, ejs-11 Added the order argument.
		 */
		native function sort(compare: Function = null, order: Number = 1): Array 


		/**
		 *	Insert, remove or replace array elements. Splice modifies an array in place. 
		 *	@param start The array index at which the insertion or deleteion will begin. Negative indicies will measure 
         *	    from the end so that -1 will specify the last element.  
		 *	@param deleteCount Number of elements to delete. If omitted, splice will delete all elements from the 
         *	    start argument to the end.  
		 *	@param values The array elements to add.
		 *	@return Array containing the removed elements.
		 *	@throws OutOfBoundsError If the start is equal to or greater than the length of the array.
		 *	@spec ecma-3
		 */
		native function splice(start: Number, deleteCount: Number, ...values): Array 


		# ECMA || FUTURE
		native function toJSONString(array: Array, pretty: Boolean = false): String 


		# ECMA || FUTURE
		override native function toLocaleString(): String 


		/**
		 *	Convert the array to a single string each member of the array has toString called on it and the resulting 
		 *	strings are concatenated.
		 *	@return A string
		 *	@spec ecma-3
		 */
		override native function toString(locale: String = null): String 


		/**
		 *	Transform all elements.
		 *	Iterate over all elements in the object and transform the elements by applying the transform function. 
		 *	This modifies the object elements in-situ. The transform function is called with the following signature:
		 *		function mapper(element: Object, elementIndex: Number, arr: Array): Object
		 *	@param transform Transforming function
		 *	@spec ejs-11
		 */
		function transform(mapper: Function): Void {
			for (let i: Number in this) {
				this[i] = mapper(this[i], i, this);
			}
		}


		/**
		 *	Remove duplicate elements and make the array unique. Duplicates are detected by using "==" (ie. content 
		 *		equality, not strict equality).
		 *	@return The original array with unique elements
		 *	@spec ejs-11
		 */
		native function unique(): Array


		/**
		 *	Insert the items at the front of the array.
		 *	@param items to insert
		 *	@return Returns the array reference
		 *	@spec ecma-3
		 */
		function unshift(...items): Object {
			return insert(0, items)
		}


		/**
		 *	Array intersection. Return the elements that appear in both arrays. 
		 *	@param arr The array to join.
		 *	@return A new array.
		 *	@spec ejs-11
		 */
		# DOC_ONLY
		native function & (arr: Array): Array


		/**
		 *	Append. Appends elements to an array.
		 *	@param elements The array to add append.
		 *	@return The modified original array.
		 *	@spec ejs-11
		 */
		# DOC_ONLY
		native function << (elements: Array): Array


		/**
		 *	Array subtraction. Remove any items that appear in the supplied array.
		 *	@param arr The array to remove.
		 *	@return A new array.
		 *	@spec ejs-11
		 */
		# DOC_ONLY
		native function - (arr: Array): Array


		/**
		 *	Array union. Return the union of two arrays. 
		 *	@param arr The array to join.
		 *	@return A new array
		 *	@spec ejs-11
		 */
		# DOC_ONLY
		native function | (arr: Array): Array


		/**
		 *	Concatenate two arrays. 
		 *	@param arr The array to add.
		 *	@return A new array.
		 *	@spec ejs-11
		 */
		# DOC_ONLY
		native function + (arr: Array): Array
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
