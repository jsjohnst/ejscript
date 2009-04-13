/*
 *	Test E4X operations on elements 
 */

var order : XML = <order id="1234" color="red">
	<customer><name>Joe Green</name>
		<address><street>410 Main</street>
			<city>New York</city><state>VT</state>
		</address>
	 </customer>
	<item level="rush" priority="low">
		<price>250</price>
		<qty>100</qty>
	</item>
	<item level="normal">
		<price>150</price>
		<qty>10</qty>
	</item>
</order>;


/*
 *	Add an element
 */
order.item[2] = <item level="slow">
		<price>9.50</price>
		<qty>99</qty>
	</item>

//	TODO BUG - work-around. using length requires type annotations on the object
// assert(order.item.length() == 3)
var o: XML = order.item
assert(o.length() == 3)

/*
 *	Add an attribute
 */
order.customer.@id = 7777
assert(order.customer.@id == 7777)


item = order.item[0]
price = item.price * item.qty
assert(price == 25000)
