/*
 *	XMLList.es - XMLList class

 *	Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */

module ejs {

	use default namespace intrinsic

	/**
	 *	The XMLList class, and the entire XML API, is based on ECMA-357, ECMAScript for XML (E4X). An XMLList object 
	 *	stores a list of XML objects. If the list has only one element, it functions similar to an XML object.
	 *	@spec ecma-357
	 */
	native final class XMLList extends Object {

        use default namespace public

		/**
		 *	XML Constructor. Create an empty XML object.
		 *	@return An XML node object.
		 */
		native function XMLList() 


		# ASC
		native function addNamespace(ns: Namespace): XML


		/**
		 *	Append a child to this XML object.
		 *	@param child The child to add.
		 *	@return This object with the added child.
		 */
		native function appendChild(child: XML): XML


		/**
		 *	Get an XMLList containing all of the attributes of this object with the given name.
		 *	@param name The name to search on.
		 *	@return An XMLList with all the attributes (zero or more).
		 */
		native function attribute(name: String): XMLList


		/**
		 *	Get an XMLList containing all of the attributes of this object.
		 *	@return An XMLList with all the attributes (zero or more).
		 */
		native function attributes(): XMLList
		

		/**
		 *	Get an XMLList containing the list of children (properties) in this XML object with the given name.
		 *	@param name The name to search on.
		 *	@return An XMLList with all the children names (zero or more).
		 */
		native function child(name: String): XMLList
		

		/**
		 *	Get the position of this object within the context of its parent.
		 *	@return A number with the zero-based position.
		 */
		native function childIndex(): Number
		

		/**
		 *	Get an XMLList containing the children (properties) of this object in order.
		 *	@return An XMLList with all the properties.
		 */
		native function children(): XMLList
		

		/**
		 *	Get an XMLList containing the properties of this object that are
		 *	comments.
		 *	@return An XMLList with all the comment properties.
		 */
		native function comments(): XMLList
		

		/**
		 *	Compare an XML object to another one or an XMLList with only one XML object in it. If the 
		 *	comparison operator is true, then one object is said to contain the other.
		 *	@return True if this object contains the argument.
		 */
		native function contains(obj: Object): Boolean
		

		/**
		 *	Deep copy an XML object. The new object has its parent set to null.
		 *	@return Then new XML object.
		 */
		native function copy(): XML
		

		/**
		 *	Get the defaults settings for an XML object. The settings include boolean values for: ignoring comments, 
		 *	ignoring processing instruction, ignoring white space and pretty printing and indenting. See ECMA-357
		 *	for details.
		 *	@return Get the settings for this XML object.
		 */
		native function defaultSettings(): Object
		

		/**
		 *	Get all the descendants (that have the same name) of this XML object. The optional argument defaults 
		 *	to getting all descendants.
		 *	@param name The (optional) name to search on.
		 *	@return The list of descendants.
		 */
		native function descendants(name: String = "*"): Object
		

		/**
		 *	Get all the children of this XML node that are elements having the
		 *	given name. The optional argument defaults to getting all elements.
		 *	@param name The (optional) name to search on.
		 *	@return The list of elements.
		 */
		native function elements(name: String = "*"): XMLList
		

		/**
		 *	Get an iterator for this node to be used by "for (v in node)"
		 *	@param deep Follow the prototype chain. Only implemented in ECMA compliance mode..
		 *	@return An iterator object.
		 *	@spec ecma-3
		 */
		override iterator native function get(deep: Boolean = false): Iterator


		/**
		 *	Get an iterator for this node to be used by "for each (v in node)"
		 *	@param deep Follow the prototype chain. Only implemented in ECMA compliance mode..
		 *	@return An iterator object.
		 *	@spec ecma-3
		 */
		override iterator native function getValues(deep: Boolean = false): Iterator

		/**
		 *	Determine whether this XML object has complex content. If the object has child elements it is 
		 *	considered complex.
		 *	@return True if this object has complex content.
		 */
		native function hasComplexContent(): Boolean
		

		/**
		 *	Determine whether this object has its own property of the given name.
		 *	@param prop The property to look for.
		 *	@return True if this object does have that property.
		 */
		# ECMA
		override intrinsic native function hasOwnProperty(name: String): Boolean
		

		/**
		 *	Determine whether this XML object has simple content. If the object
		 *	is a text node, an attribute node or an XML element that has no
		 *	children it is considered simple.
		 *	@return True if this object has simple content.
		 */
		native function hasSimpleContent(): Boolean
		

		# ASC
		/**
		 *	Return the namespace in scope
		 *	@return Array of namespaces
		 */
		native function inScopeNamespaces(): Array


		/**
		 *	Insert a child object into an XML object immediately after a specified marker object. If the marker 
		 *	object is null then the new object is inserted at the beginning. If the marker object is not found 
		 *	then the insertion is not made.
		 *	TODO - if marker is null, should it insert at the beginning or end?
		 *	@param marker Insert the new element after this one.
		 *	@param child Child element to be inserted.
		 *	@return This XML object - modified or not.
		 */
		native function insertChildAfter(marker: Object, child: Object): XML
		

		/**
		 *	Insert a child object into an XML object immediately before a specified marker object. If the marker 
		 *	object is null then the new object is inserted at the end. If the marker object is not found then the
		 *	insertion is not made.
		 *	TODO - if marker is null, should it insert at the beginning?
		 *	@param marker Insert the new element before this one.
		 *	@param child Child element to be inserted.
		 *	@return This XML object - modified or not.
		 */
		native function insertChildBefore(marker: Object, child: Object): XML
		

		/**
		 *	Return the length of an XML object; the length is defined as 1.
		 *	@return 1.
		 */
		override native function length(): Number
		

		/**
		 *	Get the local name portion of the complete name of this XML object.
		 *	@return The local name.
		 */
		native function localName(): String
		

		/**
		 *	Get the qualified name of this XML object.
		 *	@return The qualified name.
		 */
		native function name(): String
		

		# ASC
		native function namespace(prefix: String = null): Object


		# ASC
		native function namespaceDeclarations(): Array


		/**
		 *	Get the kind of node this XML object is.
		 *	@return The node kind.
		 */
		native function nodeKind(): String
		

		/**
		 *	Merge adjacent text nodes into one text node and eliminate empty text nodes.
		 *	@return This XML object.
		 */
		native function normalize(): XML
		

		/**
		 *	Get the parent of this XML object.
		 *	@return The parent.
		 */
		native function parent(): XML
		

		/**
		 *	Insert a child object into an XML object immediately before all existing properties.
		 *	@param child Child element to be inserted.
		 *	@return This XML object - modified or not.
		 */
		native function prependChild(child: Object): XML
		

		/**
		 *	Get all the children of this XML node that are processing instructions having the given name. 
		 *	The optional argument defaults to getting all processing instruction nodes.
		 *	@param name The (optional) name to search on.
		 *	@return The list of processing instruction nodes.
		 */
		native function processingInstructions(name: String = "*"): XMLList
		

		/**
		 *	Test a property to see if it will be included in an enumeration over the XML object.
		 *	@param property The property to test.
		 *	@return True if the property will be enumerated.
		 */
		# ECMA
		override intrinsic native function propertyIsEnumerable(property: Object): Boolean
		

		/**
		 *	Change the value of a property. If the property is not found, nothing happens.
		 *	@param property The property to change.
		 *	@param value The new value.
		 *	@return True if the property will be enumerated.
		 */
		native function replace(property: Object, value: Object): void

		
		/**
		 *	Replace all the current properties of this XML object with a new set. The argument can be 
		 *	another XML object or an XMLList.
		 *	@param properties The new property or properties.
		 *	@return This XML object.
		 */
		native function setChildren(properties: Object): XML
		

		/**
		 *	Set the local name portion of the complete name of this XML object.
		 *	@param The local name.
		 */
		native function setLocalName(name: String): void
		

		/**
		 *	Set the qualified name of this XML object.
		 *	@param The qualified name.
		 */
		native function setName(name: String): void
		

		/**
		 *	Get the settings for this XML object. The settings include boolean values for: ignoring comments, 
		 *	ignoring processing instruction, ignoring white space and pretty printing and indenting. See ECMA-357
		 *	for details.
		 *	@return Get the settings for this XML object.
		 */
		native function settings(): Object
		

		/**
		 *	Configure the settings for this XML object.
		 *	@param settings A "settings" object previously returned from a call to the "settings" method.
		 */
		native function setSettings(settings: Object): void

		
		/**
		 *	Get all the properties of this XML node that are text nodes having the given name. The optional 
		*	argument defaults to getting all text nodes.
		 *	@param name The (optional) name to search on.
		 *	@return The list of text nodes.
		 */
		native function text(name: String = "*"): XMLList
		

		/**
		 *	Provides a string representation of the XML object.
		 *	@return A string with the encoding.
		 */
		override native function toString(): String 
		

		/**
		 *	Provides an XML-encoded version of the XML object that includes the tags.
		 *	@return A string with the encoding.
		 */
		native function toXMLString(): String 
		

		/**
		 *	Return this XML object.
		 *	@return This object.
		 */
		# ECMA
		override intrinsic function valueOf(): XML {
			return this
		}

		/*
		   XML
		   		.prototype
		   		.ignoreComments
		   		.ignoreProcessingInstructions
		   		.ignoreWhitespace
		   		.prettyPrinting
		   		.prettyIndent
		   		.settings()
		   		.setSettings(settings)
		   		.defaultSettings()

			   function domNode()
			   function domNodeList()
			   function xpath(XPathExpression)
		   XMLList
			   function domNode()
			   function domNodeList()
			   function xpath(XPathExpression)
		 */
	}
}


/*
 *	@copy	default
 *	
 *	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
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

