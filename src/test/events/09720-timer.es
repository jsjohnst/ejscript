/*
 *	Timer
 */

var gotTimer = undefined

/*
 *	Timer callback. Stop timer after one event
 */
function callback(e: TimerEvent): Void {
	gotTimer = e
	e.data.stop()
}

/*
 *	Set timer for 1/10 sec
 */
var t: Timer  = new Timer(100, callback)

/*
 *	Wait for timer
 */
for (let i in 100) {
	if (gotTimer) {
		break
	}
	App.sleep(100)
}

assert(gotTimer)
assert(gotTimer is TimerEvent)
