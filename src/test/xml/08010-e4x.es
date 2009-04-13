/*
 *	Test E4X get access
 */

var order : XML = <order id="1234" color="red">
	<customer><name>Joe Green</name>
		<address><street>410 Main</street>
			<city>New York</city><state>VT</state>
		</address>
	 </customer>
	<item level="rush" priority="low">
		<price>2.50</price>
		<qty>30</qty>
	</item>
	<item level="normal">
		<price>1.50</price>
		<qty>10</qty>
	</item>
</order>;


/*
 *	TODO - does this just copy a reference or do a deep copy?
 */
var all = order
assert(all.customer.name != null)
assert(all.customer.name != undefined)
assert(all.customer.name == 'Joe Green')


/*
 *	Test element extraction
 */
var address = order.customer.address
// assert(address.length() == ??)


/*
 *	Extract a text value
 */
var city = order.customer.address.city
assert(city == "New York")


/*
 *	Extract an attribute value
 */
var level = order.item[0].@level
assert(level == "rush")


/*
	Get get non-existant element, attribute, array element
 */
