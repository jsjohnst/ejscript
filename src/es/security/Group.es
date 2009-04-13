/*
 *	Group.es - Group access entities
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs {

	use default namespace "ejs.security"

	/**
	 * 	A group represents one or more access entities that own and access system resources.
	 *	A group may contain individual access entities or other groups. An access entity can appear in any number 
	 *	of groups and a group can be in any number of access control entries (ACE). There is one group associated 
	 *	with each ACE.
	 */
	class Group extends Object {

		/**
		 *	Add a member to this group; can be a named entity or another group.
		 *	@param newMember The new member to add.
		 *	@throws ejs.exceptions.SecurityError if the calling object does not
		 *	have authorization to add members to this group.
		 */
		public function add(newMember : Object) : void
		{ 
		}

		/**
		 *	Test to see if an entity is a member of this group.
		 *	@param testMember The string or group to test against.
		 *	@throws ejs.exceptions.SecurityError if the calling object does not
		 *	have authorization to access this group.
		 */
		public function contains(testMember : Object) : void
		{ 
		}

		/**
		 *	Create a new group and optionally add a member to it; the member 
		 *	can be a named entity or another group.
		 *	@param initialMember The first member.
		 *	@throws ejs.exceptions.SecurityError if the calling object does not
		 *	have authorization to add members to this group.
		 */
		public function Group(initialMember : Object = null)
		{ 
		}

		/**
		 *	Remove a member from this group; if the passed in entity isn't a
		 *	member of the group this method does nothing.
		 *	@param removeMember The new member to add.
		 *	@throws ejs.exceptions.SecurityError if the calling object does not
		 *	have authorization to remove members from this group.
		 */
		public function remove(removeMember : Object) : void
		{ 
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
