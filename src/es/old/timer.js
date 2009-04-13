/*
 *	@file 	timer.js
 *	@brief 	Timer class
 *	@copy 	Copyright (c) Embedthis Software LLC, 2005-2009. All Rights Reserved.
 *
 *	Usage:
 *		timer = new Timer(method, period, [arg]);
 *		timer.start();
 */


package System
{


class Timer
{
	var		id : String;
	var		period : int;
	var		due : int64;
	var		runOnce : bool;					// Run timer just once
	var		method : Object;				// Callback method


	public function Timer(id : String, func : Object, period: int, 
		[arg : Object])
	{
		this.id = id;
		this.period = period;
		due = time() + period;

		method = func;
	}
	

	public function reschedule(period : int)
	{
		/* MOB -- must update the timer service somehow */
		this.period = period;
	}


	public function run(now)
	{
		/*
		 *	Run the timer
		 */
		try {
			method(this);
		}
		catch (error) {
			trace("Timer exception: " + error);
		}

		if (runOnce) {
			timerService.removeTimer(this);

		} else {
			due = now + this.period;
		}
	}



	public function start()
	{
		timerService.addTimer(this);
	}



	public function stop()
	{
		timerService.removeTimer(this);
	}

}



class TimerService
{
	var		timers;
	var		nextDue;

	public function TimerService() 
	{
		timers = new Object();
		nextDue = 0;
		global.timerService = this;
	}

	internal function addTimer(timer)
	{
		timers[timer.id] = timer;
	}

	internal function removeTimer(timer)
	{
		try {
			delete timers[timer.id];
		}
		catch {}
	}

	internal function getIdleTime()
	{
		return nextDue - time();
	}

	internal function runTimers()
	{
		var		now = time();

		nextDue = 2147483647; 		/* MOB -- MATH.MAX_INT; */

		for each (var timer in timers)
		{
			if (timer.due < now) {
				timer.run(now);
			}
		}
		for each (var timer in timers)
		{
			if (timer.due < nextDue) {
				nextDue = timer.due;
			}
		}
		// println("runTimers leaving with " + (nextDue - now));
		return nextDue - time();
	}
}


} /* package System */
