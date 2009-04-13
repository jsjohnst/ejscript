/**
 *  Record.es -- Record class
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 *
 *  Issues:
 *      Types. How do we convert and where
 *      Pluralize. Get class name, table name ... and convert from one to the other
 */

module ejs.db {

    /**
     *  Database record class. A record instance corresponds to a row in the database. This class provides a low level 
     *  Object Relational Mapping (ORM) between the database and Ejscript objects. This class provides methods to create,
     *  read, update and delete rows in the database. When read or initialized object properties are dynamically created 
     *  in the Record instance for each column in the database table. Users should subclass the Record class for each 
     *  database table to manage.
     */
    dynamic class Record {

        use default namespace public

        /*
         *  Trace sql statements
         */
        private static var  _belongsTo: Object
        private static var  _className: String
        private static var  _columnNames: Object
        private static var  _columnTypes: Object
        private static var  _sqlColumnTypes: Object
        private static var  _db: Database = undefined
        private static var  _foreignId: String
        private static var  _keyName: String
        private static var  _tableName: String
        private static var  _traceSql: Boolean = false
        private static var  _validations: Array

        private static var  _beforeFilters: Array
        private static var  _afterFilters: Array
        private static var  _wrapFilters: Array

        private var         _keyValue: Object
        private var         _errors: Object

        /************************************ Instance Methods ********************************/
        /**
         *  Construct a new record instance. This is really a constructor function, because the Record class is implemented 
         *  by user models, no constructor will be invoked when a new user model is instantiated. The record may be 
         *  initialized by optionally supplying field data. However, the record will not be written to the database 
         *  until $save is called. To read data from the database into the record, use one of the $find methods.
         *  @param fields An optional object set of field names and values may be supplied to initialize the recrod.
         */
        function constructor(fields: Object = null): Void {
            if (_columnNames == null) {
                getSchema()
            }

            //  TODO - need to validate the fields here and only create valid coluns - also need to get the types

            if (fields) {
                for (field in fields) {
                    if (_columnNames[field] == undefined) {
                        throw new Error("Column " + field + " is not a column in the table \"" + _tableName + "\"")
                    } else {
                        this[field] = fields[field]
                    }
                }
            }
        }

        static function beforeFilter(fn, options: Object = null): Void {
            if (_beforeFilters == undefined) {
                _beforeFilters = []
            }
            _beforeFilters.append([fn, options])
        }

        static function afterFilter(fn, options: Object = null): Void {
            if (_afterFilters == undefined) {
                _afterFilters = []
            }
            _afterFilters.append([fn, options])
        }

        static function wrapFilter(fn, options: Object = null): Void {
            if (_wrapFilters == undefined) {
                _wrapFilters = []
            }
            _wrapFilters.append([fn, options])
        }

        private function runFilters(filters): Void {
            if (!filters) {
                return
            }
            for each (filter in filters) {
                let fn = filter[0]
                let options = filter[1]
                if (options) {
                    only = options.only
/*
                    if (only) {
                        if (only is String && actionName != only) {
                            continue
                        }
                        if (only is Array && !only.contains(actionName)) {
                            continue
                        }
                    } 
                    except = options.except
                    if (except) {
                        if (except is String && actionName == except) {
                            continue
                        }
                        if (except is Array && except.contains(actionName)) {
                            continue
                        }
                    }
*/
                }
                fn.call(this)
            }
        }

        /**
         *  Save the record to the database.
         *  @returns True if the record is validated and successfully written to the database
         *  @throws IOError Throws exception on sql errors
         */
        //  TODO - need to know the ID that was saved? - what happens on db errors?
        function save(): Boolean {
            var sql: String

            if (!validateModel()) {
                return false
            }

            runFilters(_beforeFilters)
            
            if (_keyValue == null) {
                sql = "INSERT INTO " + _tableName + " ("
                for (let field: String in this) {
                    if (_columnNames[field]) {
                        sql += field + ", "
                    }
                }
                sql = sql.trim(', ')
                sql += ") VALUES("
                for (let field: String in this) {
                    sql += "'" + this[field] + "', "
                }
                sql = sql.trim(', ')
                sql += ")"

            } else {
                sql = "UPDATE " + _tableName + " SET "
                for (let field: String in this) {
                    if (_columnNames[field]) {
                        sql += field + " = '" + this[field] + "', "
                    }
                }
                sql = sql.trim(', ')
                sql += " WHERE " + _keyName + " = " +  _keyValue
            }
            sql += "; SELECT last_insert_rowid();"

            log("save", sql)
            //  TODO - what should be done on errors?
            let result: Array = getDb().query(sql)
            _keyValue = this["id"] = result[0]["last_insert_rowid()"] cast Number

            runFilters(_afterFilters)

            return true
        }


        /**
         *  Update a record based on the supplied fields and values.
         *  @param fields Hash of field/value pairs to use for the record update.
         *  @returns True if the record is successfully updated. Returns false if validation fails and the record is not 
         *      saved.
         *  @throws IOError on database SQL errors
         */
        function saveUpdate(fields: Object): Boolean {
            for (field in fields) {
                if (this[field] != undefined) {
                    this[field] = fields[field]
                }
            }
            return save()
        }


        /************************************ Static Methods ********************************/
        //  TODO - options are ignored
        /**
         *  Define a belonging reference to another model class. When a model belongs to another, it has a foreign key
         *  reference to another class.
         *  @param owner Referenced model class that logically owns this model.
         *  @param options Optional options hash
         *  @option className 
         *  @option foreignKey Key name for the foreign key
         *  @option conditions SQL conditions for the relationship to be satisfied
         */
        static function belongsTo(owner, options: Object = null): Void {
            if (_belongsTo == undefined) {
                _belongsTo = [owner]
            } else {
                _belongsTo.append(owner)
            }
        }


        /*
         *  Create a new record instance and apply the row data
         *  Process a sql result and add properties for each field in the row
         */
        private static function createRecord(rowData: Object): Record {
            let rec: Record = new global[_className]
            rec.constructor(rec)
            for (let field: String in rowData) {
                rec[field] = rowData[field]
            }
            rec.coerceTypes()
            rec._keyValue = rowData[_keyName]
            return rec
        }


        /**
         *  Find a record. Find and return a record identified by its primary key if supplied or by the specified options. 
         *  If more than one record matches, return the first matching record.
         *  @param key Key Optional key value. Set to null if selecting via the options 
         *  @param options Optional search option values
         *  @returns a model record or null if the record cannot be found.
         *  @throws IOError on internal SQL errors
         *
         *  @option columns List of columns to retrieve
         *  @option conditions { field: value, ...}   or [ "SQL condition", "id == 23", ...]
         *  @option from Low level from clause (TODO not fully implemented)
         *  @option keys [set of matching key values]
         *  @option order ORDER BY clause
         *  @option group GROUP BY clause
         *  @option limit LIMIT count
         *  @option offset OFFSET count
         *  @option include { table1: { table2: { table3: {}}}}
         *  @option joins Low level join statement "LEFT JOIN vists on stockId = visits.id"
         *  @option joinBelongs Automatically join all belongsTo models. Defaults to true.
         *
         *  FUTURE
         *  @option readonly
         *  @option lock
         */
        static function find(key: Object, options: Object = null): Object {
            let grid: Array = innerFind(key, options)
            if (grid.length >= 1) {
                return createRecord(grid[0])
            } 
            return null
        }


        /**
         *  Find all the matching records
         *  @param options Optional set of options. See $find for list of possible options.
         *  @returns An array of model records. The array may be empty if no matching records are found
         *  @throws IOError on internal SQL errors
         */
        static function findAll(options: Object = null): Array {
            let grid: Array = innerFind(null, options)
            for (let i = 0; i < grid.length; i++) {
                grid[i] = createRecord(grid[i])
            }
            return grid
        }


        /**
         *  Find the first record matching a condition. Select a record using a given SQL where clause.
         *  @param where SQL WHERE clause to use when selecting rows.
         *  @returns a model record or null if the record cannot be found.
         *  @throws IOError on internal SQL errors
         *  @example
         *      rec = findOneWhere("cost < 200")
         */
        static function findOneWhere(where: String): Object {
            let grid: Array = innerFind(null, { conditions: [where]})
            if (grid.length >= 1) {
                return createRecord(grid[0])
            } 
            return null
        }


        /**
         *  Find records matching a condition. Select a set of records using a given SQL where clause
         *  @param whereClause SQL WHERE clause to use when selecting rows.
         *  @returns An array of objects. Each object represents a matching row with fields for each column.
         *  @example
         *      list = findWhere("cost < 200")
         */
        static function findWhere(where: String, count: Number = null): Array {
            let grid: Array = innerFind(null, { conditions: [where]})
            for (let i = 0; i < grid.length; i++) {
                grid[i] = createRecord(grid[i])
            }
            return grid
        }


        //  TODO
        private static function includeJoin(joins: Object): String {
            var cmd: String = ""
            for each (join in joins) {
                cmd ="LEFT OUTER JOIN " + join + ON + " " + prev + ".id = " + join + "." + singular(prev) + "Id" +
                     includeJoin(join)
                break
            }
            return cmd
        }


        /*
         *  Common find implementation. See find/findAll for doc.
         */
        static private function innerFind(key: Object, options: Object = null): Array {
            let cmd: String
            let columns: Array
            let from: String
            let conditions: String
            let where: String

            //  TODO - BUG options: Object = {} doesn't work in args above
            if (options == null) {
                options = {}
            }

            if (options.columns) {
                columns = options.columns
                /*
                 *  Qualify "id" so it won't clash when doing joins. If the "click" option is specified, 
                 *  must have ID in the data
                 */
                let index: Number = columns.indexOf("id")
                if (index >= 0) {
                    columns[index] = _tableName + ".id"
                } else if (!columns.contains(_tableName + ".id")) {
                    columns.insert(0, _tableName + ".id")
                }
            } else {
                columns =  "*"
            }

            conditions = ""
            from = ""
            where = false

            if (options.from) {
                from = options.from

            } else if (options.include) {
                //  TODO - incomplete
                from = _tableName + " " + includeJoins(options.include)

            } else {
                from = _tableName
                
                /*
                 *  Join other belonging models if a key has not been provided
                 */
                if (_belongsTo && options.joinBelongs != false) {
                    conditions = " ON "
                    for each (let owner in _belongsTo) {
                        from += " INNER JOIN " + owner._tableName
                    }
                    for each (let owner in _belongsTo) {
                        let tname = Reflect(owner).name
                        tname = tname[0].toLower() + tname.slice(1) + "Id"
                        conditions += _tableName + "." + tname + " = " + owner._tableName + "." + owner._keyName + " AND "
                    }
                }
                if (options.joins) {
                    if (conditions == "") {
                        conditions = " ON "
                    }
                    let parts: Array = options.joins.split(" ON ")
                    from += " " + parts[0]
                    if (parts.length > 1) {
                        conditions += parts[1] + " AND "
                    }
                }
            }
            conditions = conditions.trim(" AND ")

            if (options.conditions) {
                where = true
                conditions += " WHERE "
                if (options.conditions is Array) {
                    for each (cond in options.conditions) {
                        conditions += cond + " " + " OR "
                    }
                    conditions = conditions.trim(" OR ")

                } else if (options.conditions is Object) {
                    for (field in options.conditions) {
                        conditions += field + " = '" + options.conditions[field] + "' " + " AND "
                    }
                }
                conditions = conditions.trim(" AND ")

            } else {
                from = from.trim(" AND ")
            }

            if (key || options.key) {
                if (!where) {
                    conditions += " WHERE "
                    where = true
                } else {
                    conditions += " AND "
                }
                conditions += (_tableName + "." + _keyName + " = ") + ((key) ? key : options.key)
            }

            cmd = "SELECT " + columns + " FROM " + from + conditions
            if (options.group) {
                cmd += " GROUP BY " + options.group
            }
            if (options.order) {
                cmd += " ORDER BY " + options.order
            }
            if (options.limit) {
                cmd += " LIMIT " + options.limit
            }
            if (options.offset) {
                cmd += " OFFSET " + options.offset
            }

            cmd += ";"
            log("find", cmd)

            let db = getDb()
            if (db == null) {
                throw new Error("Database connection has not yet been established")
            }

            let results: Array
            try {
                results = db.query(cmd)
            }
            catch (e) {
                throw e
            }

            // dump(results)

/*
            for each (rec in results) {
                //  TODO - when we have catchalls, these should be loaded only on demand access
                if (_belongsTo) {
                    for each (modelType in _belongsTo) {
                        fieldName = modelType._className.toCamel() + "Id"
                        rec[fieldName] = loadReference(modelType, rec[modelType._foreignId])
                    }
                }
                if (hasOne) {
                    for each (modelType in hasOne) {
                        fieldName = modelType._className.toCamel() + "Id"
                        rec[fieldName] = loadReference(modelType, rec[modelType._foreignId])
                    }
                }
                if (_hasMany) {
                    for each (modelType in _hasMany) {
                        fieldName = modelType._className.toCamel() + "Id"
                        rec[fieldName] = loadReference(modelType, rec[modelType._foreignId], true)
                    }
                }
            }
*/
            return results
        }


        /**
         *  Get the database connection for this record class
         *  @returns Database instance object created via new $Database
         */
        static function getDb(): Database {
            if (_db == null) {
                return Database.defaultDatabase
            } else {
                return _db
            }
        }


        /**
         *  Return the column names for the table
         *  @returns an array containing the names of the database columns. This corresponds to the set of properties
         *      that will be created when a row is read using $find.
         */
        static function get columnNames(): Array { 
            if (_columnNames == null) {
                getSchema()
            }
            let result: Array = []
            for (name in _columnNames) {
                result.append(name)
            }
            return result
        }


        /**
         *  Return the column names for the record
         *  @returns an array containing the Pascal case names of the database columns. The names have the first letter
         *      capitalized. 
         */
        static function get columnTitles(): Array { 
            if (_columnNames == null) {
                getSchema()
            }
            let result: Array = []
            for each (title in _columnNames) {
                result.append(title)
            }
            return result
        }


        /**
         *  Get the key name for this record
         */
        static function getKeyName(): String {
            return _keyName
        }


        //  TODO - should this be a get numRows?
        /**
         *  Return the number of rows in the table
         */
        static function getNumRows(): Number {
            let cmd: String = "SELECT COUNT(*) FROM " + _tableName + " WHERE " + _keyName + " <> '';"
            log("getNumRows", cmd)
            let grid: Array = getDb().query(cmd)
            return grid[0]["COUNT(*)"]
        }


        /*
         *  Read the table schema
         */
        private static function getSchema(): Void {
            if (_className == null) {
                throw new Error("Model is not initialized. Must call Model.setup() first")
            }
            if (getDb() == undefined) {
                throw new Error("Can't get schema, database connection has not yet been established")
            }

            let sql: String = 'PRAGMA table_info("' + _tableName + '");'
            log("schema", sql)

            let grid: Array = getDb().query(sql)
            _columnNames = {}
            _columnTypes = {}
            _sqlColumnTypes = {}
            for each (let row in grid) {
                let name: String = row["name"]
                _columnNames[name] = name.toPascal()
                _sqlColumnTypes[name] = row.type.toLower()
                _columnTypes[name] = mapSqlTypeToEjs(row.type.toLower())
                //  Others "dflt_value", "notnull" "pk"
            }
        }


        /**
         *  Get the associated name for this record
         *  @returns the database table name backing this record class. Normally this is simply a  pluralized class name. 
         */
        static function getTableName(): String {
            return _tableName
        }


        //  TODO - options are ignored
        /**
         *  Define a containment relationship to another model class. When using "hasMany" on another model, it means 
         *  that other model has a foreign key reference to this class and this class can "contain" many instances of 
         *  the other.
         *  @param model Model class that is contained by this class. 
         *  @option things Model object that is posessed by this. 
         *  @option through String Class name which mediates the many to many relationship
         *  @option foreignKey Key name for the foreign key
         */
        static function hasMany(model: Object, options: Object = null): Void {
            //  TODO - incomplete
            if (_hasMany == undefined) {
                _hasMany = [model]
            } else {
                _hasMany.append(model)
            }
        }


        /**
         *  Define a containment relationship to another model class. When using "hasAndBelongsToMany" on another model, it 
         *  means that other models have a foreign key reference to this class and this class can "contain" many instances 
         *  of the other models.
         *  @option thing Model 
         *  @option foreignKey Key name for the foreign key
         *  @option through String Class name which mediates the many to many relationship
         *  @option joinTable
         */
        static function hasAndBelongsToMany(thing: Object, options: Object = null): Void {
            belongs(thing, options)
            hasMany(thing, options)
        }


        //  TODO - options are ignored
        /**
         *  Define a containment relationship to another model class. When using "hasOne" on another model, 
         *  it means that other model has a foreign key reference to this class and this class can "contain" 
         *  only one instance of the other.
         *  @param model Model class that is contained by this class. 
         *  @option thing Model that is posessed by this.
         *  @option foreignKey Key name for the foreign key
         *  @option as String 
         */
        static function hasOne(model: Object, options: Object = null): Void {
            //  TODO - incomplete
            if (_hasOne == undefined) {
                _hasOne = [model]
            } else {
                _hasOne.append(model)
            }
        }


        private static function loadReference(model: Record, key: String, hasMany: Boolean = false): Object {
            let data: Array = model.innerFind(key)
            if (hasMany) {
                let result: Array = new Array
                for each (item in data) {
                    result.append(model.createRecord(data[0]))
                }
                return result
                
            } else {
                if (data.length > 0) {
                    return model.createRecord(data[0])
                }
            }
            return null
        }


        //  TODO - should log be pushed into database.query -- yes
        private static function log(where: String, cmd: String): Void {
            if (_traceSql) {
                print(where + " SQL: " + cmd)
            }
        }


        //  TODO - unused
        private static function logResult(data: Object): Void {
            dump(data)
        }


        private static function mapSqlTypeToEjs(sqlType: String): String {
            sqlType = sqlType.replace(/\(.*/, "")
            let ejsType: String = Database.SqliteToEjs[sqlType]
            if (ejsType == undefined) {
                throw new Error("Unsupported SQL type: \"" + sqlType + "\"")
            }
            return ejsType
        }


        //  TODO - need to have an instance method to remove a record from the database
        /**
         *  Remove records from the database
         *  @param keys Set of keys identifying the records to remove
         */
        static function remove(...keys): Void {
            for each (let key: Object in keys) {
                let cmd: String = "DELETE FROM " + _tableName + " WHERE " + _keyName + " = " + key + ";"
                log("remove", cmd)
                getDb().query(cmd)
            }
        }


        /**
         *  Set the database connection for this record class
         *  @param database Database instance object created via new $Database
         */
        static function setDb(dbase: Database): Void {
            _db = dbase
        }


        /**
         *  Initialize the model
         *  @param database Reference to the database object.
         */
        static function setup(database: Database = null): Void {
            _db = database
            _className = Reflect(this).name
            _tableName = pluralize(_className)
            _foreignId = _className.toCamel() + "Id"
            _keyName = "id"
        }


        /**
         *  Set the associated table name for this record
         *  @param name Name of the database table to backup the record class.
         */
        static function setTableName(name: String): Void {
            _tableName = name
        }


        /**
         *  Set the key name for this record
         */
        static function setKeyName(value: String): Void {
            _keyName = value
        }


        /**
         *  Run an SQL statement and return selected records.
         *  @param sql SQL command to issue
         *  @returns An array of objects. Each object represents a matching row with fields for each column.
         */
        static function sql(cmd: String, count: Number = null): Array {
            cmd = "SELECT " + cmd + ";"
            log("select", cmd)
            return getDb().query(cmd)
        }


        /**
         *  Trace SQL statements. Control whether trace is enabled for the actual SQL statements issued against the database.
         *  @param on If true, display each SQL statement to the log
         */
        static function trace(on: Boolean): void {
            _traceSql = on
        }


        static function validateFormat(fields: Object, options: Object = null) {
            if (_validations == null) {
                _validations = []
            }
            _validations.append([checkFormat, fields, options])
        }


        static function validateNumber(fields: Object, options: Object = null) {
            if (_validations == null) {
                _validations = []
            }
            _validations.append([checkNumber, fields, options])
        }


        static function validatePresence(fields: Object, options: Object = null) {
            if (_validations == null) {
                _validations = []
            }
            _validations.append([checkPresent, fields, options])
        }


        static function validateUnique(fields: Object, options: Object = null) {
            _validations.append([checkUnique, fields, options])
        }


        static var ErrorMessages = {
            accepted: "must be accepted",
            blank: "can't be blank",
            confirmation: "doesn't match confirmation",
            empty: "can't be empty",
            invalid: "is invalid",
            missing: "is missing",
            notNumber: "is not a number",
            notUnique: "is not unique",
            taken: "already taken",
            tooLong: "is too long",
            tooShort: "is too short",
            wrongLength: "wrong length",
            wrongFormat: "wrong format",
        }


        private static function checkFormat(thisObj: Object, field: String, value, options): Void {
            if (! RegExp(options.format).test(value)) {
                thisObj._errors[field] = options.message ? options.message : ErrorMessages.wrongFormat
            }
        }


        private static function checkNumber(thisObj: Object, field: String, value, options): Void {
            if (! RegExp(/^[0-9]+$/).test(value)) {
                thisObj._errors[field] = options.message ? options.message : ErrorMessages.notNumber
            }
        }


        private static function checkPresent(thisObj: Object, field: String, value, options): Void {
            if (value == undefined) {
                thisObj._errors[field] = options.message ? options.message : ErrorMessages.missing
            } else if (value.length == 0 || value.trim() == "" && thisObj._errors[field] == undefined) {
                thisObj._errors[field] = ErrorMessages.blank
            }
        }


        private static function checkUnique(thisObj: Object, field: String, value, options): Void {
            if (thisObj._keyValue) {
                grid = findWhere(field + ' = "' + value + '" AND id <> ' + thisObj._keyValue)
            } else {
                grid = findWhere(field + ' = "' + value + '"')
            }
            if (grid.length > 0) {
                thisObj._errors[field] = options.message ? options.message : ErrorMessages.notUnique
            }
        }

        function error(field: String, msg: String): Void {
            if (!field) {
                field = ""
            }
            _errors[field] = msg
        }

        function validateModel(): Boolean {
            _errors = {}
            if (_validations) {
                for each (let validation: String in _validations) {
                    check = validation[0]
                    fields = validation[1]
                    options = validation[2]
                    if (fields is Array) {
                        for each (let field in fields) {
                            if (_errors[field]) {
                                continue
                            }
                            check(this, field, this[field], options)
                        }
                    } else {
                        check(this, fields, this[fields], options)
                    }
                }
            }
            self = Reflect(this).type
            if (self["validate"]) {
                self["validate"].call(this)
            }
            if (_errors.length == 0) {
                coerceTypes()
            }
            return _errors.length == 0
        }


        function getErrors(): Array {
            return _errors
        }


        function hasError(field: String = null): Boolean {
            if (field) {
                return (_errors && _errors[field])
            }
            return _errors && _errors.length > 0
        }


        function getFieldType(field: String): String {
            if (_columnNames == null) {
                getSchema()
            }
            return Database.SqliteToDatatype[_sqlColumnTypes[field]]
        }


        private function coerceTypes(): Void {
            for (let field: String in this) {
                if (_columnTypes[field] == Reflect(this[field]).type) {
                    continue
                }
                let value: String = this[field]
                switch (_columnTypes[field]) {
                case Boolean:
                    if (value is String) {
                        this[field] = (value.toLower() == "true")
                    } else if (value is Number) {
                        this[field] = (value == 1)
                    } else {
                        this[field] = value cast Boolean
                    }
                    this[field] = (this[field]) ? true : false
                    break

                case Date:
                    this[field] = new Date(value)
                    break

                case Number:
                    this[field] = this[field] cast Number
                    break
                }
            }
        }

/*
        static function selectCount(sql: String, count: Number): Object
        static function removeAll(): Void {}
        static function average(column: String): Number {}
        static function maximum(column: String): Number {}
        static function minimum(column: String): Number {}
        static function sum(column: String): Number {}
        static function count(column: String): Number {}
*/

    }


    /*
     *  Internal class to demand load model references and collections
     *
FUTURE
    class DemandLoader {
        var cached: Record
        var hasMany: Boolean
        var key: Number
        var loaded: Boolean
        var model: Record


        function DemandLoader(model: Record, key: Number, hasMany: Boolean = false) {
            this.model = model
            this.key = key
            this.hasMany = hasMany
        }


        function load(): Record {
            if (!loaded) {
                data = this.innerFind(key)
                if (hasMany) {
                    cached = new Array
                    for each (item in data) {
                        cached.append(model.createRecord(data[0]))
                    }
                } else {
                    if (data.length > 0) {
                        cached = model.createRecord(data[0])
                    } else {
                        cached = null
                    }
                }
            }
            return cached
        }


        function reload(): Record {
            loaded = false
            return load
        }
    }
*/

    //  TODO - need real pluralize() singularize()
    function pluralize(name: String): String {
        var s: String = name + "s"
        return s.toPascal()
    }

}


/*
 *  @copy   default
 *  
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *  
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire 
 *  a commercial license from Embedthis Software. You agree to be fully bound 
 *  by the terms of either license. Consult the LICENSE.TXT distributed with 
 *  this software for full details.
 *  
 *  This software is open source; you can redistribute it and/or modify it 
 *  under the terms of the GNU General Public License as published by the 
 *  Free Software Foundation; either version 2 of the License, or (at your 
 *  option) any later version. See the GNU General Public License for more 
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *  
 *  This program is distributed WITHOUT ANY WARRANTY; without even the 
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *  
 *  This GPL license does NOT permit incorporating this software into 
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses 
 *  for this software and support services are available from Embedthis 
 *  Software at http://www.embedthis.com 
 *  
 *  @end
 */
