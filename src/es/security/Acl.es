/*
 *	Acl.es - Access control list
 */


module ejs {

	use default namespace "ejs.security"

	/**
	 *	An Acl, or access control list, object stores a list of access control entries (ACE).
	 *	There is one ACL per securable entity, e.g. a file or directory. There can be no more then one positive 
	 *	and one negative ACE per ACL. A positive ACE is one that grants permissions; a negative ACE is one that 
	 *	denies them. <p>If there is no ACE that references a particular access entity (e.g a person or login) in 
	 *	an ACL then that entity has no permissions on the securable entity.</p> <p>If there are two ACE entries for 
	 *	an access entity, one of which grants permission and the other denies it; access is denied.</p> 
	 *	<p>Individual permissions override group permissions. For example, if a group ACE grants permission but an 
	 *	individual ACE denies it for a particular access entity then access is denied.</p>
	 */
	class Acl extends Object {

		/**
		 *	The ACL constructor. ACL's have an owner and a name.
		 *	@param caller The named entity or group that will own this ACL.
		 *	@param name The name of the ACL.
		 *	@throws ejs.exceptions.SecurityError if the caller does not have
		 *	permission to modify this ACL.
		 */
		public function Acl(owner : Object, name : String)
		{ 
		}

		/**
		 *	Add an ACE to this list; if the ACE is already in the list this method does
		 *	nothing.
		 *	@param caller The named entity or group that owns this ACL.
		 *	@param newAce The new ACE to add.
		 *	@throws ejs.exceptions.SecurityError if the caller does not have
		 *	permission to modify this ACL.
		 */
		public function add(caller : Object, newAce : Ace) : void
		{ 
		}

		/**
		 *	Determine whether an entity has certain permissions or not.
		 *	@param accessor The named entity or group to check against.
		 *	@param permissions The permissions object to check against.
		 *	@returns True if the entity/group does have those permissions.
		 *	@throws ejs.exceptions.SecurityError if the caller does not have
		 *	permission to read this ACL.
		 */
		public function check(accessor : Object, permissions : Permission) : Boolean
		{
			return false
		}

		/**
		 *	Get the name of this ACL.
		 *	@returns The string name.
		 *	@throws ejs.exceptions.SecurityError if the caller does not have
		 *	permission to access this ACL.
		 */
		public function get name() : String
		{
			return null
		}

		/**
		 *	Get the list of permission objects for a group in this ACL. All access
		 *	control entries are searched.
		 *	@param group The group to search on.
		 *	@returns An array containing references to all the permissions objects
		 *	for the group.
		 *	@throws ejs.exceptions.SecurityError if the caller does not have
		 *	permission to access this ACL.
		 */
		public function getPermissions(group : Group) : Array
		{
			return null
		}

		/**
		 *	Remove an ACE from this list; if the ACE isn't present this method does
		 *	nothing.
		 *	@param caller The named entity or group that owns this ACL.
		 *	@param ace The ACE to remove.
		 *	@throws ejs.exceptions.SecurityError if the caller does not have
		 *	permission to modify this ACL.
		 */
		public function remove(caller : Object, ace : Ace) : void
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
