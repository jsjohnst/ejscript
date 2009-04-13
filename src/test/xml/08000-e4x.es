/*
 *	E4X literals
 */

var solo: XML = <html/>


var noType = <html/>


var nextLine  = \
	<html/>


var dual: XML = <html></html>


var withAttributes = <html id="0"></html>


var simple = <order id="1234" color="red">
	<customer><name>Joe Green</name>
		<address><street>410 Main</street>
			<city>York</city><state>VT</state>
		</address>
	 </customer>
	<item level="rush" priority="low">
		<price>2.50</price>
		<qty>30</qty>
	</item>
</order>


var withElementList : XML = <order id="1234" color="red">
	<customer><name>Joe Green</name>
		<address><street>410 Main</street>
			<city>York</city><state>VT</state>
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
</order>
