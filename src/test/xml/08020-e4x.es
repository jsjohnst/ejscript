/*
 *	Test basic update E4X access
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


order.customer.name = "Peter"
assert(order.customer.name == "Peter")

var address: XML = order.customer.address
assert(address.length() == 1)
var city = order.customer.address.city
assert(city == "New York")
