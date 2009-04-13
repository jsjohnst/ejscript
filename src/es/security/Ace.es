/*
 *	Ace.es - Access control entry
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs {

	use default namespace "ejs.security"

	/**
	 *	An access control entry, object references a group and associates a set of permissions with that group.
	 *	An ACE can be either positive or negative: a positive ACE is one that grants permissions; a negative ACE 
	 *	is one that denies them. There are one or more ACEs in each ACL. <p>An ACE has a permission object 
	 *	associated with it that contains the permissions. The permissions can be changed by directly modifying 
	 *	that object.</p>
	 */

	class Ace extends Object {

		/**
		 *	Access control entry (ACE) object constructor.
		 *	@param group The group associated with this ACE.
		 *	@param permissions The permission object to add.
		 *	@param negative True to make this ACE a negative one, i.e. denies
		 *	the associated permissions.
		 */
		public function Ace(group : Group, permissions : Permission, negative :
			Boolean = false)
		{ 
		}

		/**
		 *	Determine whether an entity has certain permissions or not.
		 *	@param permissions The permissions object to check against.
		 *	@returns True if the entity/group associated with this ACE does 
		 *	have those permissions.
		 *	@throws ejs.exceptions.SecurityError if the caller does not have
		 *	permission to read this ACE.
		 */
		public function check(permissions : Permission) : Boolean
		{
			return false
		}

		/**
		 *	Get the group associated with this ACE.
		 *	@returns A group object.
		 *	@throws ejs.exceptions.SecurityError if the caller does not have
		 *	permission to access this ACE.
		 */
		public function get group() : Group
		{
			return null
		}

		/**
		 *	Get the permissions in the ACE.
		 *	@returns A permission object.
		 *	@throws ejs.exceptions.SecurityError if the caller does not have
		 *	permission to access this ACE.
		 */
		public function get permissions() : Permission
		{
			return null
		}

		/**
		 *	Determine whether this is a "positive" or "negative" ACE.
		 *	@returns True if it is negative.
		 *	@throws ejs.exceptions.SecurityError if the caller does not have
		 *	permission to access this ACE.
		 */
		public function get negative() : Boolean
		{
			return false
		}

		/**
		 *	Replace the permissions with a different set.
		 *	@param permission The permission object to use.
		 *	@throws ejs.exceptions.SecurityError if the caller does not have
		 *	permission to modify this ACE.
		 */
		public function replacePermissions(permissions : Permission) : void
		{ 
		}

		/**
		 *	Make this a "negative" ACE; if it is already negative this method
		 *	has no effect.
		 *	@throws ejs.exceptions.SecurityError if the caller does not have
		 *	permission to modify this ACE.
		 */
		public function setNegative() : void
		{ 
		}

		/**
		 *	Set the group associated with this ACE.
		 *	@param group The group to associate.
		 *	@throws ejs.exceptions.SecurityError if the caller does not have
		 *	permission to modify this ACE.
		 */
		public function setGroup(group : Object) : void
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
