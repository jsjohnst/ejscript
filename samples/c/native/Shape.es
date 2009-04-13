/*
 *  Shape.es - Sample module with Shape class
 */

module Sample
{
	class Shape 
	{
		var x: num
		var y: num
		var height: num
		var width: num

        /*
         *  Native constructor function must have its actual implementation be supplied by a C function
         */
		native function Shape(height: num, width: num)

        /*
         *  Native function must have its actual implementation be supplied by a C function
         */
		public native function area(): num

        /*
         *  Script function with no native counterpart.
         */
		public function moveTo(x: num, y: num): void {
            this.x = x
            this.y = y
        }
	}
}
