use namespace "ejs.db"

var db: DB  = new DB("market.sdb")
var stocks: Record

stocks = db.query("select * from Stocks where ID = 1;")

println("ROWS " + stocks.length)

for each (var record in stocks) {
	for (field in record) {
		print(field + " " + record[field])
	}
	print(record)
}

db.close()
