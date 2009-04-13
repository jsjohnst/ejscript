/*
 *	Simple use case for Event, Dispatcher
 */

var gotEvent = undefined

function callback(e: Event): Void {
	gotEvent = e
}

public var events: Dispatcher = new Dispatcher
events.addListener(callback)
events.dispatch(new Event("Sunny"))

assert(gotEvent != undefined)
assert(gotEvent is Event)
