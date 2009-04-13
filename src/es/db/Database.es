/**
 *	Database.es -- Database class
 *
 *	Copyright (c) All Rights Reserved. See details at the end of the file.
 */

module ejs.db {

	// use strict

    /**
     *  SQLite database support
     */
	class Database {

        use default namespace public

		private static var _defaultDb: Database
		private static var _traceSql: Boolean

		private var _name: String
		private var _connection: String

		/*
         *  TODO FUTURE DOC AND USAGE
         *
		 *	Initialize a database connection using the supplied database connection string
		 *	@param connectionString Connection string stipulating how to connect to the database. The format is one of the 
		 *	following forms:
		 *		<ul>
		 *			<li>adapter://host/database/username/password</li>
		 *			<li>filename</li>
		 *		</ul>
		 *		Where adapter specifies the kind of database. Sqlite is currently the only supported adapter.
		 *		For sqlite connection strings, the abbreviated form is permitted where a filename is supplied and the 
		 *		connection string is assumed to be: <pre>sqlite://localhost/filename</pre>
		 */
		/**
		 *	Initialize a SQLite database connection using the supplied database connection string.
		 *	@param connectionString Connection string stipulating how to connect to the database. The format is one of the 
		 *	following forms:
		 *		<ul>
		 *			<li>filename</li>
		 *		</ul>
         *  In the future, connection strings will support other databases using a format a format like:
		 *		<ul>
		 *			<li>adapter://host/database/username/password</li>
		 *			<li>filename</li>
		 *		</ul>
		 */
		native function Database(connectionString: String)


		/**
		 *	Reconnect to the database using a new connection string
		 *	@param connectionString See Database() for information about connection string formats.
		 */
		native function connect(connectionString: String): Void


		/**
		 *	Close the database connection. Database connections should be closed when no longer needed rather than waiting
		 *	for the garbage collector to automatically close the connection when disposing the database instance.
		 */
		native function close(): Void


		/**
		 *	Execute a SQL command on the database. This is a low level SQL command interface that bypasses logging.
         *	    Use @query instead.
		 *	@param sql SQL command string
		 *	@returns An array of row results where each row is represented by an Object hash containing the column names and
		 *		values
		 */
		native function sql(cmd: String): Array


		/**
		 *	Execute a SQL command on the database.
		 *	@param sql SQL command string
		 *	@returns An array of row results where each row is represented by an Object hash containing the column names and
		 *		values
         */
        function query(cmd: String): Array {
            log(cmd)
            return sql(cmd)
        }


		/**
		 *	Get the database connection string
		 */
		function get connection(): String {
			return _connection
		}


        /**
         *  Get the name of the database.
         *  @returns the database name defined via the connection string
         */
		function get name(): String {
			return _name
		}


        /**
         *  Return list of tables in a database
         *  @returns an array containing list of table names present in the currently opened database.
         */
		function getTables(): Array {
			let cmd: String = "SELECT name from sqlite_master WHERE type = 'table' order by NAME;"
			let grid: Array = query(cmd)
			let result: Array = new Array
			for each (let row: Object in grid) {
				let name: String = row["name"]
				if (!name.contains("sqlite_") && !name.contains("_Ejs")) {
					result.append(row["name"])
				}
			}
			return result
		}


        function getColumns(table: String): Array {
            grid =  query('PRAGMA table_info("' + table + '");')
            let names = []
            for each (let row in grid) {
                let name: String = row["name"]
                names.append(name)
            }
            return names
        }


        /**
         *  Get the default database for the application.
         *  @returns the default database defined via the $defaultDatabase setter method
         */
        static function get defaultDatabase(): Database {
            return _defaultDb
        }


        /**
         *  Set the default database for the application.
         *  @param the default database to define
         */
        static function set defaultDatabase(db: Database): Void {
            _defaultDb = db
        }


        /*
         *  Map independent types to SQLite types
         */
        static const DatatypeToSqlite: Object = {
            "binary":       "blob",
            "boolean":      "tinyint",
            "date":         "date",
            "datetime":     "datetime",
            "decimal":      "decimal",
            "float":        "float",
            "integer":      "int",
            "number":       "decimal",
            "string":       "varchar",
            "text":         "text",
            "time":         "time",
            "timestamp":    "datetime",
        }


        /*
         *  Map independent types to SQLite types
         */
        static const SqliteToDatatype: Object = {
            "blob":         "binary",
            "tinyint":      "boolean",
            "date":         "date",
            "datetime":     "datetime",
            "decimal":      "decimal",
            "float":        "float",
            "int":          "integer",
            "varchar":      "string",
            "text":         "text",
            "time":         "time",
        }


        /*
         *  Map SQLite types to Ejscript native types
         */
        static const SqliteToEjs: Object = {
            "blob":         String,
            "date":         Date,
            "datetime":     Date,
            "decimal":      Number,
            "integer":      Number,
            "float":        Number,
            "time":         Date,
            "tinyint":      Boolean,
            "text":         String,
            "varchar":      String,
        }

        /*
         *  Map Ejscript native types back to SQLite types
         *  INCOMPLETE and INCORRECT
        static const EejsToDatatype: Object = {
            "string":       "varchar",
            "number":       "decimal",
            "date":         "datetime",
            "bytearray":    "Blob",
            "boolean":      "tinyint",
        }
         */

        function createDatabase(name: String, options: Object = null): Void {
            //  Nothing to do for sqlite
        }


        function destroyDatabase(name: String): Void {
            //  TODO
        }


        //  TODO - should these types be type objects not strings?
        function createTable(table: String, columns: Array = null): Void {
            let cmd: String

            query("DROP TABLE IF EXISTS " + table + ";")
            query("CREATE TABLE " + table + "(id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL);")

            if (columns) {
                //  TODO - BUG should not need when null/undefined have iterators
                for each (let colspec: String in columns) {
                    //  TODO - destructuring assignment would be good here
                    let spec: Array = colspec.split(":")
                    if (spec.length != 2) {
                        throw "Bad column spec: " + spec
                    }
                    let column: String = spec[0]
                    let datatype: String = spec[1]
                    addColumn(table, column.trim(), datatype.trim())
                }
            }
        }


        function renameTable(oldTable: String, newTable: String): Void {
            query("ALTER TABLE " + oldTable + " RENAME TO " + newTable + ";")
        }


        function destroyTable(table: String): Void {
            query("DROP TABLE IF EXISTS " + table + ";")
        }


        function addIndex(table: String, column: String, indexName: String): Void {
            query("CREATE INDEX " + indexName + " ON " + table + " (" + column + ");")
        }


        function removeIndex(table: String, indexName: String): Void {
            query("DROP INDEX " + indexName + ";")
        }


        //  TODO - should these types be type objects not strings?
        function addColumn(table: String, column: String, datatype: String, options: Object = null): Void {
            datatype = DatatypeToSqlite[datatype.toLower()]
            if (datatype == undefined) {
                throw "Bad Ejscript column type: " + datatype
            }
            query("ALTER TABLE " + table + " ADD " + column + " " + datatype)
        }


        //  TODO - should these types be type objects not strings?
        function changeColumn(table: String, column: String, datatype: String, options: Object = null): Void {
            datatype = datatype.toLower()
            if (DatatypeToSqlite[datatype] == undefined) {
                throw "Bad column type: " + datatype
            }
            /*
                query("ALTER TABLE " + table + " CHANGE " + column + " " + datatype)
            */
            throw "SQLite does not support column changes"
        }


        function renameColumn(table: String, oldColumn: String, newColumn: String): Void {
            query("ALTER TABLE " + table + " RENAME " + oldColumn + " TO " + newColumn + ";")
        }


        function removeColumns(table: String, columns: Array): Void {
            /* NORMAL SQL
             * for each (column in columns)
             *   query("ALTER TABLE " + table + " DROP " + column + ";")
             */

            /*
             *  This is a dumb SQLite work around because it doesn't have drop column
             */
            backup = "_backup_" + table
            keep = getColumns(table)
            for each (column in columns) {
                if ((index = keep.indexOf(column)) < 0) {
                    throw "Column \"" + column + "\" does not exist in " + table
                } 
                keep.remove(index)
            }

            //  TODO - good to have a utility routine for this
            schema = 'PRAGMA table_info("' + table + '");'
            grid = query(schema)
            types = {}
            for each (let row in grid) {
                let name: String = row["name"]
                types[name] = row["type"]
            }

            columnSpec = []
            for each (k in keep) {
                if (k == "id") {
                    columnSpec.append(k + " INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL")
                } else {
                    columnSpec.append(k + " " + types[k])
                }
            }

            cmd = "BEGIN TRANSACTION;
                CREATE TEMPORARY TABLE " + backup + "(" + columnSpec + ");
                INSERT INTO " + backup + " SELECT " + keep + " FROM " + table + ";
                DROP TABLE " + table + ";
                CREATE TABLE " + table + "(" + columnSpec + ");
                INSERT INTO " + table + " SELECT " + keep + " FROM " + backup + ";
                DROP TABLE " + backup + ";
                COMMIT;"
            query(cmd)
        }


        private static function log(cmd: String): Void {
            if (_traceSql) {
                print("SQL: " + cmd)
            }
        }


        //  TODO - should this be static or instance
        /**
         *  Trace SQL statements. Control whether trace is enabled for the actual SQL statements issued against the database.
         *  @param on If true, display each SQL statement to the log
         */
        static function trace(on: Boolean): void {
            _traceSql = on
        }


/*
 *		FUTURE
		function map(): Void {}
		function map(table: String, ...): Void {}
		function map(tables: Array): Void {}

		function transaction(code: Function): Void {
			startTransaction()
			try {
				code()
			}
			catch (e: Error) {
				rollback();
			}
			finally {
				endTransaction()
			}
		}

		function startTransaction(): Void {}
		function rollback(): Void {}
		function commit(): Void {}
*/
	}
}


/*
 *	@copy	default
 *	
 *	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *	Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *	
 *	This software is distributed under commercial and open source licenses.
 *	You may use the GPL open source license described below or you may acquire 
 *	a commercial license from Embedthis Software. You agree to be fully bound 
 *	by the terms of either license. Consult the LICENSE.TXT distributed with 
 *	this software for full details.
 *	
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version. See the GNU General Public License for more 
 *	details at: http://www.embedthis.com/downloads/gplLicense.html
 *	
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	
 *	This GPL license does NOT permit incorporating this software into 
 *	proprietary programs. If you are unable to comply with the GPL, you must
 *	acquire a commercial license to use this software. Commercial licenses 
 *	for this software and support services are available from Embedthis 
 *	Software at http://www.embedthis.com 
 *	
 *	@end
 */
