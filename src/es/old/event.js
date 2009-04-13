/*
 *	@file 	event.js
 *	@brief 	Event class
 *	@copy 	Copyright (c) Embedthis Software LLC, 2009-2009. All Rights Reserved.
 *
 *	Usage:
 *		listener = new System.Listener(obj, method);
 *		eventTarget.addListener(eventName, listener);
 *
 *	To fire events:
 *		eventTarget.fire(eventName, new System.Event("My Event"));
 */

/******************************************************************************/

package System {


class Event 
{
	var	type : String;
	var	timeStamp : Date;
	var	arg : Object;

	public function Event(arg : Object)			// MOB - arg should be optional
	{
		timeStamp = time();
		type = "default";
		this.arg = arg;
	}
}



class Listener
{
	var		method;

	function Listener(func : Object)	// MOB -- what type should method be
	{
		if (arguments.length == 1) {
			method = this["onEvent"];
		} else {
			method = func;
		}
	}
}


class EventTarget
{
	private var	events : Object;		/* Hash of a event names */

	public function EventTarget()
	{
		events = new Object();
	}


	public synchronized function addListener(eventName : String, listener : Listener) 
	{
		var eventListeners : Object;

		eventListeners = events[eventName];

		if (eventListeners == undefined) {
			eventListeners = events[eventName] = new Array();
		}
		/* MOB OPT -- array search */
		/* MOB use let */
		for (var i = 0; i < eventListeners.length; i++) {
			var l : Listener = eventListeners[i];
			if (l == listener) {
				return;
			}
		}
		/* MOB -- Array append */
		eventListeners[eventListeners.length] = listener;
	}


	public synchronized function removeListener(eventName : String, listener : Listener)
	{
		var listeners : Object;

		listeners = events[eventName];

		if (listeners == undefined) {
			return;
		}

		for (var i = 0; i < listeners.length; i++) {
			var l = listeners[i];
			if (l == listener) {
				// MOB -- want listeners.splice here
				// listeners.splice(i, 1);
				for (var j = i; j < (listeners.length - 1); j++) {
					listeners[j] = listeners[j + 1];
				}
				delete listeners[listeners.length - 1];
				i = listeners.length;
			}
		}
	}


	public synchronized function fire(eventName : String, event : Event)
	{
		var listeners : Object;

		listeners = events[eventName];
	
		if (listeners == undefined) {
			throw new Error("EventTarget.fire: unknown eventName " + eventName);
		}

		for (var i = listeners.length - 1; i >= 0; i--) {
			var listener = listeners[i];
			listener.method(listener, event);
		}
	}
}


} /* package System */
