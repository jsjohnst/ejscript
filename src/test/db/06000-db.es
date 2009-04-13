/*
 *	Unit test for ejs.db
 *
 *	Must leave the database with the same content when the test started.
 *
 *	TODO - test sql()
 */

use module ejs.db

let db = Database.defaultDatabase = new Database("db/test.sdb")
assert(db.name  == "test.sdb")
assert(db.connection == "db/test.sdb")

public dynamic class Trade implements Record {
	setup()
}

var rec: Trade

assert(Trade.columnNames == "id,date,stockId,quantity,price")
assert(Trade.columnTitles == "Id,Date,StockId,Quantity,Price")

/*
 *	Find a row by key
 */
rec = Trade.find(1)

assert(rec is Object)
count = 0
for (f in rec) {
    count++
}
assert(count == 5)
assert(rec.id == 1)
assert(rec.stockId == 1)
assert(rec.quantity == 100)
assert(rec.price == 23)


/*
 *  Find by conditions
 */
rec = Trade.find(null, { conditions: {quantity: 100}})
assert(rec is Object)
assert(rec.quantity == 100)


/*
 *  Find with order, group, limit and offset
 */
rec = Trade.findAll({order: "price DESC", limit: 2, offset: 1, conditions: {quantity: 100}})
assert(rec is Array)
assert(rec.length == 2)
assert(rec[0].quantity == 100)


/*
 *	Update a field and save
 */
rec = Trade.find(2)
rec.quantity = 200 + rec.quantity
rec.save()
rec = Trade.find(1)
rec = Trade.find(2)
assert(rec.quantity == 300)
rec.quantity = rec.quantity - 200
rec.save()


/*
 *	Create a new row
 */
assert(Trade.getNumRows() == 3)
rec = new Trade
rec.stockId = 1
rec.quantity = 999
rec.price = 1000
rec.save()
assert(Trade.getNumRows() == 4)

//	TODO - need some way to get the ID of rec
/* Get the ID of the row just added */
grid = Trade.findAll()
id = grid[grid.length - 1].id


/*
 *	Remove the last row
 */
Trade.remove(id)
assert(Trade.getNumRows() == 3)

/*
 *	Find all matching rows
 */
grid = Trade.findAll({conditions: ["quantity = 100"]})
assert(grid.length == 3)
rec = grid[0]
assert(rec.quantity == 100)
assert(rec.stockId == 1)
assert(rec.price == 23)


/*
 *	Select
 */
var trades = Trade.findWhere("price > 10")
assert(trades.length == 3)

trade = Trade.findOneWhere("price > 10")
assert(trade is Object)
assert(trade.price == 23)
assert(trade.id == 1)


/*
 *	Raw SQL
 */
trades = db.query("SELECT * FROM Trades WHERE id >= 1;")
assert(trades.length == 3)


/*
 *	TODO FUTURE

rec = new Trade({ stockId: 2, date: "10/10/06", quantity: 1, price: 10.50})
rec = Trade.find("MSFT", "RHAT")

dynamic class Stock extends Record
{
	hasOne("Invoice");
	belongsTo("Invoice");
	hasAndBelongsToMany("Categories");

	belongsTo({ className: "Order", foreignKey: "order_id", });
}
*/

db.close()
