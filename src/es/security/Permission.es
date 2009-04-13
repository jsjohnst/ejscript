/*
 *	Permission.es - Permissions associated with Groups
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs {

	use default namespace "ejs.security"

	/**
	 * 	The Permission object stores a set of permissions for a group by an access control entry (ACE).
	 * 	Permissions have a name and a set of boolean flags; one each for read, write, execute, "other" 
	 *	and owner. "Other" is available for general use; the owner permission means the group is the owner of 
	 *	this securable entity and can do any operation on it including reading and changing its security settings.
	 */
	class Permission extends Object {

		/**
		 *	The "read" permission.
		 */
		public static const READ : Number			= 1

		/**
		 *	The "write" permission.
		 */
		public static const WRITE : Number			= 2

		/**
		 *	The "other" permission.
		 */
		public static const OTHER : Number			= 4

		/**
		 *	The "owner" permission.
		 */
		public static const OWNER : Number			= 8

		/**
		 *	The "execute" permission.
		 */
		public static const EXECUTE : Number		= 16

		/**
		 *	Get the permission flags for this object.
		 *	@return An integer with the encoded permissions.
		 *	@throws ejs.exceptions.SecurityError if the calling object is not the
		 *	owner.
		 */
		public function get permissions() : Number 
		{
			return 0
		}

		/**
		 *	Permission object constructor.
		 *	@param name The string name of the permission.
		 *	@param permissions The access rights to grant or deny.
		 */
		public function Permission(name : String, permissions : Number)
		{ 
		}

		/**
		 *	Set the permission flags for this object.
		 *	@return An integer with the encoded permissions.
		 *	@throws ejs.exceptions.SecurityError if the calling object is not the
		 *	owner.
		 */
		public function setPermissions(permissions : Number) : void
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
