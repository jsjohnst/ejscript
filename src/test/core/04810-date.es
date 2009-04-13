/*
 *  Test date parsing
 */

/*
 *  TODO BUG when after 11:00 pm
assert(Date.parseDate("August 25 2008").date == 25)
assert(Date.parseDate("August 25, 2008").date == 25)
*/

assert((Date.parseDate("1/1/08")).year == 2008)
assert((Date.parseDate("1/1/08")).month == 0)
assert((Date.parseDate("1/1/08")).date == 1)
assert((Date.parseDate("15 Jul 2008 15:50:28")).date == 15)
assert(Date.parseDate("Mon Jan 30 2008 14:30:00").year == 2008)
assert(Date.parseDate("Tuesday, 09-Nov-99 10:12:40").year == 1999)
assert(Date.parseDate("14-06-80").year == 1980)
assert(Date.parseDate("14-06-10").year == 2010)
assert(Date.parseDate("30 Jun 2008 14:30:00").date == 30)
assert(Date.parseDate("2008 30 Jun 14:30:00").date == 30)
assert(Date.parseDate("14:30 2008 30 Jun").date == 30)
//BUG assert(Date.parseDate("14:30").hours == 14)
assert(Date.parseDate("12/18/08 12:00:00 GMT").year == 2008)
assert(Date.parseDate("Tue Jul 15 2008 10:53:23 GMT-0700 (PDT)").date == 15)
assert(Date.parseDate("07/15/08").year == 2008)
assert(Date.parseDate("07/15/2008").year == 2008)
assert(Date.parseDate("2008-07-15 10:53:23").year == 2008)
assert(Date.parseDate("1/2/08").month == 0)
// BUG assert(Date.parseDate("10:53:23").hours == 10)
