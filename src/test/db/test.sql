DROP TABLE IF EXISTS History;
DROP TABLE IF EXISTS Portfolio;
DROP TABLE IF EXISTS Stocks;
DROP TABLE IF EXISTS Trades;

CREATE TABLE "History" (
	"id" INTEGER  NOT NULL PRIMARY KEY AUTOINCREMENT,
	"date" INTEGER DEFAULT 'CURRENT_TIMESTAMP' NULL,
	"stockId" INTEGER  NULL,
	"price" INTEGER  NULL
);

CREATE TABLE "Portfolio" (
	"id" INTEGER  PRIMARY KEY NOT NULL,
	"stockId" INTEGER  NULL,
	"quantity" INTEGER  NULL,
	"buyPrice" INTEGER  NULL
);

CREATE TABLE "Stocks" (
	"id" INTEGER  PRIMARY KEY NOT NULL,
	"ticker" VARCHAR(16)  NOT NULL,
	"name" VARCHAR(80)  NULL
);

CREATE TABLE "Trades" (
	"id" INTEGER  PRIMARY KEY AUTOINCREMENT NOT NULL,
	"date" DATE DEFAULT '2006-05-02 04:46:19' NULL,
	"stockId" INTEGER  NULL,
	"quantity" INTEGER  NULL,
	"price" INTEGER  NULL
);


-- comment line
INSERT INTO Stocks VALUES('1', 'MSFT', 'Microsoft Corp');
INSERT INTO Stocks VALUES('2', 'RHAT', 'Red Hat Corp');

INSERT INTO Trades (id, date, stockId, quantity, price) VALUES('1', '2008-06-30 13:30:00', '1', '100', '23.00');
INSERT INTO Trades (id, date, stockId, quantity, price) VALUES('2', '2008-06-30 14:30:00', '1', '100', '24.00');
INSERT INTO Trades (id, date, stockId, quantity, price) VALUES('3', '2008-06-30 15:30:00', '1', '100', '25.00');
