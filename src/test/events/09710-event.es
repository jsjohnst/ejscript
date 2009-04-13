/*
 *	Event, Dispatcher in a class with custom event
 */

use namespace "ejs.sys"
use namespace "ejs.events"

class KeyboardEvent extends Event {

	public function KeyboardEvent(data: Object = null, bubbles: Boolean = false, priority: Number = PRI_NORMAL)
	{
		super(data, bubbles, priority)
	}
}

class Shape {
	public var events: Dispatcher = new Dispatcher
}

var gotEvent = undefined

function eventCallback(e: Event): Void {
	gotEvent = e
}

var s: Shape = new Shape
s.events.addListener(eventCallback, KeyboardEvent)
s.events.dispatch(new KeyboardEvent("Sunny", true, 99))

assert(gotEvent)
assert(gotEvent is KeyboardEvent)
assert(gotEvent.bubbles == true)
assert(gotEvent.priority == 99)
assert(gotEvent.data == "Sunny")
