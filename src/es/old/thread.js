package system 
{

class Thread 
{
	public static current : Thread;

	public main : Thread;
	public status : String;
	public alive : bool;
	public priority : int;

	public exit() : void;
	public kill(thread : Thread) : void;
	public list() : Array;
	public join() : void;
	public start(/*OPT*/ thread : Thread) : void;
	public stop(/*OPT*/ thread : Thread) : void;

	//	Need to be able to sleep on events and wake up
	public wait(Event?) : void;
	public yield() : void;
}


class Condition
{
	private mutex : Mutex;

	public Condition(/*OPT*/ mutex : Mutex);
	public signal() : void;
	public wait() : void;				// Wakes up with mutex locked 
}


class Mutex
{
	public lock() : void;
	public unlock() : void;
	public trylock(timeout : int) : void;
}
	
} /* package system */
