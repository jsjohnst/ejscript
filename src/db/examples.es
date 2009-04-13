//	TODO - this needs review


class MyStock extends Record
{
	key = "id";							/* Default */
	tableName = "Portfolio";
	columnNames = [ "ticker", "price" ];
	rowLimit = 50;
}

	//	TODO -- connection to db??

	my = new MyStock();
	rec = my.load("MSFT");									// Load one
	rec = my.load([ "MSFT", "RHAT" ]);						// Load multiples
	rec = my.load({ ticker: "MSFT", date: "10/10/06" });	// Multipart keys

	var requiredTicker = "MSFT";
	my.load({ quantity: requiredTicker });

	/*
	 *	Construct new record
	 */
	my = new MyStock({ ticker: "MSFT", quantity: 200 ....});
	my.save();


	/*
	 *	Old db connection
	 */
	var db = new DB(path);

	/* Returns table */
	tableResult = db.query(sqlCmd);

	for each (stock in db.query("Select * from portfolio")) {
		stock.@ticker, stock.@quantity, stock.@price
	}

	//	TODO getNumRows should be accesor
	for (row = 0; row < db.portfolio.getNumRows(); row++) {
		stock = db.portfolio[row];
	}

	db.map("portfolio");
	price = db.@portfolio[row].@price;
	stock = db.@portfolio.getRow(row);
	ticker = stock.@ticker;

	/*
	 *	Transactions
	 */
	db.start();
	stock.@quantity = stock.@quantity + 100;
	db.@portfolio[row].ticker = "NewTicker";
	db.commit();
	db.rollback();

	/*
	 *	Remove records -- TODO -- do we really want to map tables. What if many
	 *	tables.
	 */
	db.@portfolio.remove(stock);
	db.@tableName.removeRow(row);

	newStock = { ticker: "MSFT", quantity: 100, buyPrice: 23.00, ... };
	db.@tableName.add(newStock);

	db.close();


ESP Uses
	
	/*
	 *	Mapping
	 */
	ncols = db.portfolio.numColumns;
	db.map("portfolio");
	for each (col in db.portfolio.columns) {
		col.name
		col.type;
	}


	my = new MyStock();
	stocks = my.load([ "MSFT", "RHAT" ]);
	for each (stock in stocks) {
		for each (column in stocks.columns) {
			column.name
		}
		stock.@ticker, stock.@quantity, stock.@price
		//	TODO -- how to do formatted or raw
		stock.format("ticker");
	}

	tableResult = db.query(sqlCmd);
	for each (stock in db.query("Select * from portfolio")) {
		stock.@ticker, stock.@quantity, stock.@price
	}
