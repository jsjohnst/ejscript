/*
 *	Event.es -- Event class
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs.events {

	/**
	 *	The Event class encapsulates information pertaining to a system or application event. Applications typically
	 *	subclass Event to add custom event data if required. Events are initiated via the EventTarget class and are
	 *	routed to listening functions via a system event queue.
	 *
	 *	Example:
	 *	
	 *		class UpdateEvent extends Event {
	 *		}
	 *
	 *		obj.events.dispatch(new UpdateEvent())
	 */

	class Event {

        use default namespace public

        /**
         *  Low priority constant for use with the Event() constructor method.
         */
		static const	PRI_LOW: Number		= 25;


        /**
         *  Normal priority constant for use with the Event() constructor method.
         */
		static const	PRI_NORMAL: Number	= 50;


        /**
         *  High priority constant for use with the Event() constructor method.
         */
		static const	PRI_HIGH: Number	= 75;


		/**
		 *	Whether the event will bubble up to the listeners parent
		 */
		var bubbles: Boolean


		/**
		 *	Event data associated with the Event. When Events are created, the constructor optionally takes an arbitrary 
		 *	object data reference.
		 */
		var data: Object


		/**
		 *	Time the event was created. The Event constructor will automatically set the timestamp to the current time.  
		 */
		var timestamp: Date


		/**
		 *	Event priority. Priorities are 0-99. Zero is the highest priority and 50 is normal. Use the priority 
		 *	symbolic constants PRI_LOW, PRI_NORMAL or PRI_HIGH.
		 */
		var priority: Number


		/**
		 *	Constructor for Event. Create a new Event object.
		 *	@param data Arbitrary object to associate with the event.
		 *	@param bubbles Bubble the event to the listener's parent if true. Not currently implemented.
		 *	@param priority Event priority.
		 */
		function Event(data: Object = null, bubbles: Boolean = false, priority: Number = PRI_NORMAL) {
			this.timestamp = new Date
			this.data = data
			this.priority = priority
			this.bubbles = bubbles
		}

		//	TODO - BUG - not overriding native method
		override function toString(): String {
			return "[Event: " +  Reflect(this).typeName + "]"
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
