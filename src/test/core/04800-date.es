/*
 *  Test date formatting
 */

/*
d = new Date
print(d.format('%m/%d/%y %H:%M:%S'))
print(d.format('%b %d %Y %T'))
print(d.format('%d %b %Y %T'))
print(d.format('%+'))
print(d.format('%v'))
*/

/*
 * Property accessors 
 */
d = new Date
assert(0 <= d.day && d.day <= 6)
assert(0 <= d.dayOfYear && d.dayOfYear <= 366)
assert(0 <= d.date && d.date <= 31)
assert(1970 <= d.fullYear && d.fullYear <= 2100)
assert(0 <= d.hours && d.hours <= 23)
assert(0 <= d.minutes && d.minutes <= 59)
assert(0 <= d.seconds && d.seconds <= 59)
assert(0 <= d.milliseconds && d.milliseconds <= 999)
assert(d.time is Number)

/*
 *  Formatting
 */
if (Config.RegularExpressions) {
    assert(d.format('%d').replace(/^0/,'') == d.date)
}


/*
 *  Misc
 */
assert(d.elapsed >= 0)

/*
nanoAge
d.time
d - d
*/
