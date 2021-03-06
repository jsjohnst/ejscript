��        �
�� db/Database.es } internal-0 ejs.db module ejs.db { 	class Database { Database         use default namespace public 		private static var _defaultDb: Database _defaultDb [ejs.db::Database,private] 		private static var _traceSql: Boolean _traceSql Boolean 		private var _name: String _name String 		private var _connection: String _connection 		native function Database(connectionString: String) -constructor- connectionString private intrinsic 		native function connect(connectionString: String): Void connect public Void 		native function close(): Void close 		native function sql(cmd: String): Array sql cmd Array         function query(cmd: String): Array {             log(cmd)             return sql(cmd) query 		function get connection(): String { 			return _connection connection 		function get name(): String { 			return _name name 		function getTables(): Array { 			let cmd: String = "SELECT name from sqlite_master WHERE type = 'table' order by NAME;" SELECT name from sqlite_master WHERE type = 'table' order by NAME; 			let grid: Array = query(cmd) grid 			let result: Array = new Array result 			for each (let row: Object in grid) { row -hoisted-3 Object -hoisted-4 				let name: String = row["name"] 				if (!name.contains("sqlite_") && !name.contains("_Ejs")) { sqlite_ _Ejs 					result.append(row["name"]) 			return result getTables  Block StopIteration iterator         function getColumns(table: String): Array {             grid =  query('PRAGMA table_info("' + table + '");') PRAGMA table_info(" ");             let names = [] names             for each (let row in grid) { -hoisted-2 getValues                 let name: String = row["name"]                 names.append(name) append             return names getColumns table         static function get defaultDatabase(): Database {             return _defaultDb defaultDatabase         static function set defaultDatabase(db: Database): Void {             _defaultDb = db         } set-defaultDatabase db         static const DatatypeToSqlite: Object = { DatatypeToSqlite             "binary":       "blob", binary blob             "boolean":      "tinyint", boolean tinyint             "date":         "date", date             "datetime":     "datetime", datetime             "decimal":      "decimal", decimal             "float":        "float", float             "integer":      "int", integer int             "number":       "decimal", number             "string":       "varchar", string varchar             "text":         "text", text             "time":         "time", time             "timestamp":    "datetime", timestamp         static const SqliteToDatatype: Object = { SqliteToDatatype             "blob":         "binary",             "tinyint":      "boolean",             "int":          "integer",             "varchar":      "string",         static const SqliteToEjs: Object = { SqliteToEjs             "blob":         String,             "date":         Date,             "datetime":     Date,             "decimal":      Number,             "integer":      Number,             "float":        Number,             "time":         Date,             "tinyint":      Boolean,             "text":         String,             "varchar":      String,         function createDatabase(name: String, options: Object = null): Void { createDatabase options         function destroyDatabase(name: String): Void { destroyDatabase         function createTable(table: String, columns: Array = null): Void {             let cmd: String             query("DROP TABLE IF EXISTS " + table + ";") DROP TABLE IF EXISTS  ;             query("CREATE TABLE " + table + "(id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL);") CREATE TABLE  (id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL);             if (columns) {                 for each (let colspec: String in columns) { colspec spec                     let spec: Array = colspec.split(":") :                     if (spec.length != 2) {                         throw "Bad column spec: " + spec Bad column spec:                      } column -hoisted-5                     let column: String = spec[0]                     let datatype: String = spec[1] datatype -hoisted-6                     addColumn(table, column.trim(), datatype.trim()) createTable columns         function renameTable(oldTable: String, newTable: String): Void {             query("ALTER TABLE " + oldTable + " RENAME TO " + newTable + ";") ALTER TABLE   RENAME TO  renameTable oldTable newTable         function destroyTable(table: String): Void { destroyTable         function addIndex(table: String, column: String, indexName: String): Void {             query("CREATE INDEX " + indexName + " ON " + table + " (" + column + ");") CREATE INDEX   ON   ( ); addIndex indexName         function removeIndex(table: String, indexName: String): Void {             query("DROP INDEX " + indexName + ";") DROP INDEX  removeIndex         function addColumn(table: String, column: String, datatype: String, options: Object = null): Void {             datatype = DatatypeToSqlite[datatype.toLower()]             if (datatype == undefined) {                 throw "Bad Ejscript column type: " + datatype Bad Ejscript column type:              }             query("ALTER TABLE " + table + " ADD " + column + " " + datatype)  ADD    addColumn         function changeColumn(table: String, column: String, datatype: String, options: Object = null): Void {             datatype = datatype.toLower()             if (DatatypeToSqlite[datatype] == undefined) {                 throw "Bad column type: " + datatype Bad column type:              throw "SQLite does not support column changes" SQLite does not support column changes changeColumn         function renameColumn(table: String, oldColumn: String, newColumn: String): Void {             query("ALTER TABLE " + table + " RENAME " + oldColumn + " TO " + newColumn + ";")  RENAME   TO  renameColumn oldColumn newColumn         function removeColumns(table: String, columns: Array): Void {             backup = "_backup_" + table _backup_ backup             keep = getColumns(table) keep             for each (column in columns) {                 if ((index = keep.indexOf(column)) < 0) { indexOf index                     throw "Column \"" + column + "\" does not exist in " + table Column " " does not exist in                  }                  keep.remove(index) remove             schema = 'PRAGMA table_info("' + table + '");' schema             grid = query(schema)             types = {} types                 types[name] = row["type"] type             columnSpec = [] columnSpec             for each (k in keep) { k -hoisted-7                 if (k == "id") { id                     columnSpec.append(k + " INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL")  INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL                 } else {                     columnSpec.append(k + " " + types[k])             cmd = "BEGIN TRANSACTION;                 DROP TABLE " + backup + ";                 INSERT INTO " + table + " SELECT " + keep + " FROM " + backup + ";                 CREATE TABLE " + table + "(" + columnSpec + ");                 DROP TABLE " + table + ";                 INSERT INTO " + backup + " SELECT " + keep + " FROM " + table + ";                 CREATE TEMPORARY TABLE " + backup + "(" + columnSpec + "); BEGIN TRANSACTION;
                CREATE TEMPORARY TABLE  ( );
                INSERT INTO   SELECT   FROM  ;
                DROP TABLE  ;
                CREATE TABLE  ;
                COMMIT;             query(cmd) removeColumns         private static function log(cmd: String): Void {             if (_traceSql) {                 print("SQL: " + cmd) SQL:  log         static function trace(on: Boolean): void {             _traceSql = on trace on Database-initializer -initializer- clone Function deep get Iterator namespaces length Number toString locale block_0007_1 -block- db/Record.es internal-1     dynamic class Record { Record         private static var  _belongsTo: Object _belongsTo [ejs.db::Record,private]         private static var  _className: String _className         private static var  _columnNames: Object _columnNames         private static var  _columnTypes: Object _columnTypes         private static var  _sqlColumnTypes: Object _sqlColumnTypes         private static var  _db: Database = undefined _db         private static var  _foreignId: String _foreignId         private static var  _keyName: String _keyName         private static var  _tableName: String _tableName         private static var  _traceSql: Boolean = false         private static var  _validations: Array _validations         private static var  _beforeFilters: Array _beforeFilters         private static var  _afterFilters: Array _afterFilters         private static var  _wrapFilters: Array _wrapFilters         private var         _keyValue: Object _keyValue         private var         _errors: Object _errors         function constructor(fields: Object = null): Void {             if (_columnNames == null) {                 getSchema()             if (fields) {                 for (field in fields) { field -hoisted-1                     if (_columnNames[field] == undefined) {                         throw new Error("Column " + field + " is not a column in the table \"" + _tableName + "\"") Column   is not a column in the table " "                     } else {                         this[field] = fields[field] constructor fields         static function beforeFilter(fn, options: Object = null): Void {             if (_beforeFilters == undefined) {                 _beforeFilters = []             _beforeFilters.append([fn, options]) beforeFilter fn         static function afterFilter(fn, options: Object = null): Void {             if (_afterFilters == undefined) {                 _afterFilters = []             _afterFilters.append([fn, options]) afterFilter         static function wrapFilter(fn, options: Object = null): Void {             if (_wrapFilters == undefined) {                 _wrapFilters = []             _wrapFilters.append([fn, options]) wrapFilter         private function runFilters(filters): Void {             if (!filters) {                 return             for each (filter in filters) { filter                 let fn = filter[0]                 let options = filter[1]                 if (options) {                     only = options.only only                 }                 fn.call(this) call runFilters filters         function save(): Boolean {             var sql: String             if (!validateModel()) {                 return false             runFilters(_beforeFilters)             if (_keyValue == null) {                 sql = "INSERT INTO " + _tableName + " (" INSERT INTO                  for (let field: String in this) {                     if (_columnNames[field]) {                         sql += field + ", " ,                  sql = sql.trim(', ')                 sql += ") VALUES(" ) VALUES(                     sql += "'" + this[field] + "', " ' ',                  sql += ")" )             } else {                 sql = "UPDATE " + _tableName + " SET " UPDATE   SET                          sql += field + " = '" + this[field] + "', "  = '                 sql += " WHERE " + _keyName + " = " +  _keyValue  WHERE   =              sql += "; SELECT last_insert_rowid();" ; SELECT last_insert_rowid();             log("save", sql) save             let result: Array = getDb().query(sql)             _keyValue = this["id"] = result[0]["last_insert_rowid()"] cast Number last_insert_rowid()             runFilters(_afterFilters)             return true         function saveUpdate(fields: Object): Boolean {             for (field in fields) {                 if (this[field] != undefined) {                     this[field] = fields[field]             return save() saveUpdate         static function belongsTo(owner, options: Object = null): Void {             if (_belongsTo == undefined) {                 _belongsTo = [owner]                 _belongsTo.append(owner) belongsTo owner         private static function createRecord(rowData: Object): Record { rec             let rec: Record = new global[_className]             rec.constructor(rec)             for (let field: String in rowData) {                 rec[field] = rowData[field]             rec.coerceTypes()             rec._keyValue = rowData[_keyName]             return rec createRecord rowData         static function find(key: Object, options: Object = null): Object {             let grid: Array = innerFind(key, options)             if (grid.length >= 1) {                 return createRecord(grid[0])             return null find key         static function findAll(options: Object = null): Array {             let grid: Array = innerFind(null, options)             for (let i = 0; i < grid.length; i++) { i                 grid[i] = createRecord(grid[i])             return grid findAll         static function findOneWhere(where: String): Object {             let grid: Array = innerFind(null, { conditions: [where]}) conditions findOneWhere where         static function findWhere(where: String, count: Number = null): Array { findWhere count         private static function includeJoin(joins: Object): String {             var cmd: String = ""             for each (join in joins) { join                 cmd ="LEFT OUTER JOIN " + join + ON + " " + prev + ".id = " + join + "." + singular(prev) + "Id" + LEFT OUTER JOIN  ON prev .id =  . singular Id                      includeJoin(join)                 break             return cmd includeJoin joins         static private function innerFind(key: Object, options: Object = null): Array {             let columns: Array             let from: String from             let conditions: String             let where: String             if (options == null) {                 options = {}             if (options.columns) {                 columns = options.columns                 let index: Number = columns.indexOf("id") -hoisted-9                 if (index >= 0) {                     columns[index] = _tableName + ".id" .id                 } else if (!columns.contains(_tableName + ".id")) {                     columns.insert(0, _tableName + ".id")                 columns =  "*" *             conditions = ""             from = ""             where = false             if (options.from) {                 from = options.from             } else if (options.include) { include                 from = _tableName + " " + includeJoins(options.include) includeJoins                 from = _tableName                 if (_belongsTo && options.joinBelongs != false) { joinBelongs                     conditions = " ON "                     for each (let owner in _belongsTo) { -hoisted-11                         from += " INNER JOIN " + owner._tableName  INNER JOIN  -hoisted-12 tname -hoisted-13                         let tname = Reflect(owner).name                         tname = tname[0].toLower() + tname.slice(1) + "Id" toLower slice                         conditions += _tableName + "." + tname + " = " + owner._tableName + "." + owner._keyName + " AND "  AND                  if (options.joins) {                     if (conditions == "") {                         conditions = " ON " parts -hoisted-14                     let parts: Array = options.joins.split(" ON ") split                     from += " " + parts[0]                     if (parts.length > 1) {                         conditions += parts[1] + " AND "             conditions = conditions.trim(" AND ")             if (options.conditions) {                 where = true                 conditions += " WHERE "                 if (options.conditions is Array) {                     for each (cond in options.conditions) { cond -hoisted-16                         conditions += cond + " " + " OR "  OR                      conditions = conditions.trim(" OR ")                 } else if (options.conditions is Object) {                     for (field in options.conditions) { -hoisted-17                         conditions += field + " = '" + options.conditions[field] + "' " + " AND " '                  conditions = conditions.trim(" AND ")                 from = from.trim(" AND ")             if (key || options.key) {                 if (!where) {                     conditions += " WHERE "                     where = true                     conditions += " AND "                 conditions += (_tableName + "." + _keyName + " = ") + ((key) ? key : options.key)             cmd = "SELECT " + columns + " FROM " + from + conditions SELECT              if (options.group) { group                 cmd += " GROUP BY " + options.group  GROUP BY              if (options.order) { order                 cmd += " ORDER BY " + options.order  ORDER BY              if (options.limit) { limit                 cmd += " LIMIT " + options.limit  LIMIT              if (options.offset) { offset                 cmd += " OFFSET " + options.offset  OFFSET              cmd += ";"             log("find", cmd)             let db = getDb()             if (db == null) {                 throw new Error("Database connection has not yet been established") Database connection has not yet been established results             let results: Array             try {                 results = db.query(cmd)             catch (e) { e -hoisted-19                 throw e             return results innerFind         static function getDb(): Database {             if (_db == null) {                 return Database.defaultDatabase                 return _db getDb         static function get columnNames(): Array {              let result: Array = []             for (name in _columnNames) {                 result.append(name)             return result columnNames         static function get columnTitles(): Array {              for each (title in _columnNames) { title                 result.append(title) columnTitles         static function getKeyName(): String {             return _keyName getKeyName         static function getNumRows(): Number {             let cmd: String = "SELECT COUNT(*) FROM " + _tableName + " WHERE " + _keyName + " <> '';" SELECT COUNT(*) FROM   <> '';             log("getNumRows", cmd) getNumRows             let grid: Array = getDb().query(cmd)             return grid[0]["COUNT(*)"] COUNT(*)         private static function getSchema(): Void {             if (_className == null) {                 throw new Error("Model is not initialized. Must call Model.setup() first") Model is not initialized. Must call Model.setup() first             if (getDb() == undefined) {                 throw new Error("Can't get schema, database connection has not yet been established") Can't get schema, database connection has not yet been established             let sql: String = 'PRAGMA table_info("' + _tableName + '");'             log("schema", sql)             let grid: Array = getDb().query(sql)             _columnNames = {}             _columnTypes = {}             _sqlColumnTypes = {}                 _columnNames[name] = name.toPascal()                 _sqlColumnTypes[name] = row.type.toLower()                 _columnTypes[name] = mapSqlTypeToEjs(row.type.toLower()) getSchema         static function getTableName(): String {             return _tableName getTableName         static function hasMany(model: Object, options: Object = null): Void {             if (_hasMany == undefined) { _hasMany                 _hasMany = [model]                 _hasMany.append(model) hasMany model         static function hasAndBelongsToMany(thing: Object, options: Object = null): Void {             belongs(thing, options) belongs             hasMany(thing, options) hasAndBelongsToMany thing         static function hasOne(model: Object, options: Object = null): Void {             if (_hasOne == undefined) { _hasOne                 _hasOne = [model]                 _hasOne.append(model) hasOne         private static function loadReference(model: Record, key: String, hasMany: Boolean = false): Object { data             let data: Array = model.innerFind(key)             if (hasMany) {                 let result: Array = new Array                 for each (item in data) { item                     result.append(model.createRecord(data[0]))                 return result                 if (data.length > 0) {                     return model.createRecord(data[0]) loadReference         private static function log(where: String, cmd: String): Void {                 print(where + " SQL: " + cmd)  SQL:          private static function logResult(data: Object): Void {             dump(data) logResult         private static function mapSqlTypeToEjs(sqlType: String): String {             sqlType = sqlType.replace(/\(.*/, "") /\(.*/             let ejsType: String = Database.SqliteToEjs[sqlType] ejsType             if (ejsType == undefined) {                 throw new Error("Unsupported SQL type: \"" + sqlType + "\"") Unsupported SQL type: "             return ejsType mapSqlTypeToEjs sqlType         static function remove(...keys): Void {             for each (let key: Object in keys) {                 let cmd: String = "DELETE FROM " + _tableName + " WHERE " + _keyName + " = " + key + ";" DELETE FROM                  log("remove", cmd)                 getDb().query(cmd) keys         static function setDb(dbase: Database): Void {             _db = dbase setDb dbase         static function setup(database: Database = null): Void {             _db = database             _className = Reflect(this).name             _tableName = pluralize(_className) pluralize             _foreignId = _className.toCamel() + "Id"             _keyName = "id" setup database         static function setTableName(name: String): Void {             _tableName = name setTableName         static function setKeyName(value: String): Void {             _keyName = value setKeyName value         static function sql(cmd: String, count: Number = null): Array {             cmd = "SELECT " + cmd + ";"             log("select", cmd) select             return getDb().query(cmd)         static function validateFormat(fields: Object, options: Object = null) {             if (_validations == null) {                 _validations = []             _validations.append([checkFormat, fields, options]) validateFormat         static function validateNumber(fields: Object, options: Object = null) {             _validations.append([checkNumber, fields, options]) validateNumber         static function validatePresence(fields: Object, options: Object = null) {             _validations.append([checkPresent, fields, options]) validatePresence         static function validateUnique(fields: Object, options: Object = null) {             _validations.append([checkUnique, fields, options]) validateUnique         static var ErrorMessages = { ErrorMessages             accepted: "must be accepted", accepted must be accepted             blank: "can't be blank", blank can't be blank             confirmation: "doesn't match confirmation", confirmation doesn't match confirmation             empty: "can't be empty", empty can't be empty             invalid: "is invalid", invalid is invalid             missing: "is missing", missing is missing             notNumber: "is not a number", notNumber is not a number             notUnique: "is not unique", notUnique is not unique             taken: "already taken", taken already taken             tooLong: "is too long", tooLong is too long             tooShort: "is too short", tooShort is too short             wrongLength: "wrong length", wrongLength wrong length             wrongFormat: "wrong format", wrongFormat wrong format         private static function checkFormat(thisObj: Object, field: String, value, options): Void {             if (! RegExp(options.format).test(value)) { format                 thisObj._errors[field] = options.message ? options.message : ErrorMessages.wrongFormat message checkFormat thisObj         private static function checkNumber(thisObj: Object, field: String, value, options): Void {             if (! RegExp(/^[0-9]+$/).test(value)) { /^[0-9]+$/                 thisObj._errors[field] = options.message ? options.message : ErrorMessages.notNumber checkNumber         private static function checkPresent(thisObj: Object, field: String, value, options): Void {             if (value == undefined) {                 thisObj._errors[field] = options.message ? options.message : ErrorMessages.missing             } else if (value.length == 0 || value.trim() == "" && thisObj._errors[field] == undefined) { trim                 thisObj._errors[field] = ErrorMessages.blank checkPresent         private static function checkUnique(thisObj: Object, field: String, value, options): Void {             if (thisObj._keyValue) {                 grid = findWhere(field + ' = "' + value + '" AND id <> ' + thisObj._keyValue)  = " " AND id <>                  grid = findWhere(field + ' = "' + value + '"')             if (grid.length > 0) {                 thisObj._errors[field] = options.message ? options.message : ErrorMessages.notUnique checkUnique         function error(field: String, msg: String): Void {             if (!field) {                 field = ""             _errors[field] = msg error msg         function validateModel(): Boolean {             _errors = {}             if (_validations) {                 for each (let validation: String in _validations) { validation -hoisted-0                     check = validation[0] check                     fields = validation[1]                     options = validation[2]                     if (fields is Array) {                         for each (let field in fields) {                             if (_errors[field]) {                                 continue                             }                             check(this, field, this[field], options)                         check(this, fields, this[fields], options)             self = Reflect(this).type self             if (self["validate"]) { validate                 self["validate"].call(this)             if (_errors.length == 0) {                 coerceTypes()             return _errors.length == 0 validateModel         function getErrors(): Array {             return _errors getErrors         function hasError(field: String = null): Boolean {             if (field) {                 return (_errors && _errors[field])             return _errors && _errors.length > 0 hasError         function getFieldType(field: String): String {             return Database.SqliteToDatatype[_sqlColumnTypes[field]] getFieldType         private function coerceTypes(): Void {             for (let field: String in this) {                 if (_columnTypes[field] == Reflect(this[field]).type) {                     continue                 let value: String = this[field]                 switch (_columnTypes[field]) {                 case Boolean:                     if (value is String) {                         this[field] = (value.toLower() == "true") true                     } else if (value is Number) {                         this[field] = (value == 1)                         this[field] = value cast Boolean                     this[field] = (this[field]) ? true : false                     break                 case Date:                     this[field] = new Date(value)                 case Number:                     this[field] = this[field] cast Number coerceTypes Record-initializer     function pluralize(name: String): String { s         var s: String = name + "s"         return s.toPascal() block_0011_18 __initializer__ ��� ����  A�  3�3$344�#� 3�=��>3�=$3�=�>4��3�=���#=�=  	�� 	F� # F� ���     
��  �<�=     �3O�#3t3�3�3�33�3:�3A�3K�3T�3]�3f�3o�3}�
3��3��3��3��\�\�3��\�\�3��\�\�3��\�\�3��\�\�3��\�\�3��\�\�3��\�\�3��\�\�3��\�\�3��\�\�3��\�\��3��R�F�3��3��\�\�3��\�\�3��\�\�3��\�\�3��\�\�3��\�\�3��\�\�3��\�\�3��\�\�3��\�\��
3��R�F�3��3��\��3��\��
3��\��
3��\��3��\��3��\��3��\��
3��\��3��\��3��\���
3��R�F�3��3��3��3��!3��#3��#3��%3��&3��)3��,3��.3��;3��<�
������
������� 	   
��  �� 
    ��    
��  �� ����  3U�a!3V�a��
��  �� ����   3^�v��� ����   3g�u��
� ���� s3p�\��3q�a��3r��� �3s�b �; ��3t�	d\���3u�	e\�	
��";�e\�

��"3v�
cd\��@ @��3y�
c�*-kk�
��  
�� 
�� 
�� 
��	 �� ����\3~�\�a \� ����
3����3����
��
 �; ��3��c\���3��bd� @ߖ3��b�36SS�
��  
��  
��  
�� �������   
3����������  3��a��3��
��   
�����
�����
�������    CZ��3��
��  
����    �3��
��  �!�  �CZ�3��3��\�a \� 3��\�a \� 3��bo3��b �; ��3��d\���3��e�H,3��\�e �@ 3��3��eF��3�� eG��3�� af& �g& �@��@ �3��EH���
��  
�!�
�� 
�� 
��	 
�� 
� �  �"�    3��"\�"a \�" b \� �3��
�"�  
�"� �#�    3��\�a \� �3��
��  �%�    %3��$\�$c \�% a \�% b \�% �3��
��  
�� 
�%� �&�    3��%\�&b \� �3��
��  
�%� �)� ����  WCZ�3��'�c$ ���3��'c�:%3��(\�(c �@ 3��(3��(\�"a \�) b \�) c �3��
��  
�� 
� � 
���,� ����  ACZ�3��*c$ ��3��*�c��:%3��+\�+c �@ 3��+\�,��3��
��  
�� 
� � 
���.�    %3��-\�"a \�. b \�. c \� �3��
��  
�.� 
�.� �;� ���� �3��/\�/a ��/�
3��/a���/�
3��/b �; ��3��0�/�
c�0 �;��0�
F+3��0\�1c \�1 a �@ 3��13��1�/�
�0�
�1 @��3��(3��2\�a \� ��2�
3��2�2�
����
3��2� ��2�
3����
��
 �; ��3��e\���3��3e\�3��2�
f�@ږ3��(3��3����3�
3��3�/�
��
 �; ��3��4h\�4%3��4�3�
h\�5 � @#3��53��5�3�
h\�) �2�
h� � @��3��(3��63��63��63��73��73��83��83��6\�93��8�/�
 \�9 �3�
 \�9 3��8�/�
 \�: �/�
 \�: a \�: 3��7a \�: 3��7a \�9 �3�
 \�9 3��6a \�: �/�
 \�: �/�
 \�: 3��6�/�
 \�: 3��6���
3��:��
�3��,/~~�����������
��  
�!� 
��  
��	  
�� 
�3�3  �<� ����!  "3��;�3��;\�<a E@ �3��
��  �<� ����"  3��<a��3��
�<�  
��  
�� �>�� G ���=     �3�=O�#3�=�>3�=�?3�=�?3�=�?3�=�@3�= �@�:R��>�3�=!�A3�="�A3�=#�B3�=$�BQR��>�3�=%�C3�='�C3�=(�D3�=)�D3�=+�E3�=,�E3�=6�F3�=H�J3�=O�K3�=V�M3�=]�O3�=��R3�=��[3�=��\3�=��^3�=��a3�=��c3�=��e3�=��f3�=��g3�=��j3�=�؈3�=��3�=���3�=�ڌ3�=���3�=��3�=��3�=�ŗ3�=���3�=��3�=���3�=���3�=���3�=���3�=���3�=���3�=���3�=���3�=���3�=���3�=��<3�=���3�=���3�=���3�=�ر3�=���3�=���\ճ\޳3�=��\��\��3�=���\�\�3�=���\��\��3�=�õ\�\�3�=���\��\��3�=���\ٶ\�3�=��\��\��3�=���\׷\ݷ3�=��\��\��3�=���\ɸ\Ҹ3�=�߸\��\��3�=���\ʹ\ֹ�3�=���R��>�<3�=��3�=���3�=���3�=���3�=���3�=���3�=���3�=���3�=���3�=����
�>�>���
�?�>���
�?�>���
�@�>���	
�@�>���

�A�>����
�A�>���
�B�>���
�B�>���
��>���
�C�>���
�D�>���
�D�>���
�E�>����I� �����CZ�3�=7�F�Z%3�=8�F( @ 3�==�GaW3�=>�Ga �; ��3�=?�G�b��:%3�=@�H��\�Ib \�I � \�I �@3�=A�I3�=B�Iab�]b�@��@ �3�=F�47||�
�J� 
�G�G  �K� ����  HCZ�3�=I�J��:%3�=J�K���@ 3�=K�(3�=L�K���;a�F�;b�G��3�=M�
�K�   
���M� ����  HCZ�3�=P�L��:%3�=Q�L���@ 3�=R�(3�=S�M���;a�F�;b�G��3�=T�
�K�   
���O� ����  HCZ�3�=W�N��:%3�=X�N���@ 3�=Y�(3�=Z�N���;a�F�;b�G��3�=[�
�K�   
���Q�> ����s3�=^�Oa�	3�=_�O�@ 3�=a�Oa��
 �; ��3�=b�PbF��3�=c�PbG��3�=d�Pd3�=e�Qd��Q ��Q�
@ 3�=y�Q3�=z�Qc]�Q @���3�=|�"%kk�
�R�   
�P�G  
�K�  
��  �Y� ���� �3�=��R3�=��RB ��3�=��RQ�@ 3�=��(3�=��S�3�=��SuZ%�   3�=��S\�T� \�% �3�=��T]�=�
 �; ��3�=��T�c�3�=��Tac\�U  �@ @ٖ3�=��Q3�=��Ua\�U&��3�=��Ua\�U �3�=��T]�=�
 �; ��3�=��Va\�V]d� \�V  �@�3�=��Q3�=��Ua\�U&��3�=��Va\�V �?�   3�=��V3�=��V\�W� \�W �3�=��T]�=�
 �; ��3�=��T�f�3�=��Waf\�W ]f� \�V  �@ @і3�=��Q3�=��Ua\�U&��3�=��Wa\�X� \�X u  �3�=��(3�=��Xa\�X �3�=��Y\�Ya.3�=��Y# �a��
��3�=��YbF�\�Z��!;]\�4��3�=��Z�3�=��Z^�fi�������������
��  
�� 
�G� 
�G� 
�G� �\� ����?3�=��[a �; ��3�=��[]b��:,3�=��\ab�]b�@ @ږ3�=��\ ��22�
�J�  
�G�G  �^� ����  ICZ�3�=��]��:%3�=��]��;a�F��@3�=��V3�=��^�a��
�3�=��
�^�   
���a�> ������h3�=��_�3��� �3�=��_bb3�=��_a �; ��3�=��`ac�bc�@�3�=��(3�=��`bF 3�=��`a��b�3�=��ab�*->>�
�a�  
�_� ��
�G� �c� ���� ?CZ�3�=��bab" ��3�=��bc�G(3�=��bcF� ��@ 3�=��cZ�
�c�  
��
�� �e� ���� PCZ�3�=��cZa" ��3�=��dF�cb�+!3�=��dbc� �bc�3�=��dc;A��@�3�=��eb�
�� 
�� 
�d�  �f� ���� F3�=��eZ\�f��;a�F��" ��3�=��bb�G(3�=��bbF� ��@ 3�=��cZ�
�f�  
�� �g� ����  ]CZ�3�=��eZ\�f��;a�F��" ��3�=��dF�dc�+!3�=��dcd� �cd�3�=��dd;A��@�3�=��ec�
�f�  
�g�
�� 
�d�  �j�> ����!�3�=��g\�
�3�=��ha �; ��3�=��h\�ic �i�
 \�) �i�
 \�i c \�i �i�
�i�
� \�i 3�=��ic! � 3�=��h�3�=��j?   @��3�=��jb�xx�
�j�  
�� 
�h�  Έ�> ����"�CZ�3�=��3�=��k3�=��k3�=��k3�=��l3�=��lbZ%3�=��l� �@ 3�=��lb��!�
j3�=��mb��!�
�3�=��md\�4��3�=��mjF(3�=��n�\�n dj�@+3�=��nd�\�n 
��3�=��odF�\�n @ @3�=��V3�=��o\�o�3�=��(3�=��o\�
�3�=��p\�
�3�=��pQ�3�=��pb��k�
3�=��pb��k�
�?�  3�=��qb��q�
"3�=��q�\�) b��q�
�q�
� �?b  3�=��V3�=��r��3�=��r�";
�b��r�
Q,"�   3�=��r\�%�3�=��s� �; ��3�=��se\�t���B   �@�3�=��s� �; ��3�=��t��� �3�=��u�F��u  ��G�u � \�i �3�=��uf�\�i � \�X ���B  \�i ���B  \�v  �@��@ 3�=��vb��j�
r3�=��wf\�
%3�=��w\�%�@ 3�=��3�=��wb��j�
\�%�x ��3�=��xe\�)�F�  �3�=��x��G)3�=��yf�G�\�v  �@ @ 3�=��(3�=��yf\�v&��3�=��yb��f�
�   3�=��z^�3�=��zf\�X �3�=��zb��f�
�EK3�=��{b��f�
��
 �; ��3�=��{f�\�) \�|  �@�3�=��3�=��|f\�|&��@T3�=��|b��f�
� EB3�=��}b��f�
�=�
 �; ��3�=��}f�\�W b��f�
�� \�~ \�v  �@Ֆ@ 3�=��Q3�=��~f\�v&��@3�=��V3�=��~e\�v&��3�=��a";�b��c�
"i3�=��g�3�=��f\�X �3�=���^�@3�=��53�=���f\�v �3�=��Q3�=���f�\�i � \�X aa@b��c�
  �@ 3�=��(3�=�\��d \�: e f �3�=���b����
3�=���c\�b����
  �@ 3�=���b����
3�=���c\Ѓb����
  �@ 3�=�ۃb����
3�=���c\��b����
  �@ 3�=���b�݄�
3�=��c\��b�݄�
  �@ 3�=��(3�=���c\� �3�=���\�cc. 3�=�ԅ#  ��3�=��hZ%3�=�����\��@ 3�=��(3�=���3�=���3�=�͇hc� ��@3�=�����3�=�����>3�=���i����������������������
���
�c�  
��
�� 
�!� 
�k� 
�f� 
�f� 
��  
��� 
�0�m 	
�^�s  
�^�t  
�t�t  
�w�w 
�{�{  
�G�}  
����  �� �����#   =3�=����Z%3�=���R�F� �@3�=��V3�=�Ӊ��Z�3�=����� ����$ ]3�=��F�Z%3�=��F(  @ 3�=��(3�=������3�=�ˊ� �; ��3�=��ab@�3�=���a�>ARR�
��  
��G  ͌� ����% ]3�=��F�Z%3�=��F(  @ 3�=��(3�=������3�=��� �; ��3�=���ab@�3�=���a�>ARR�
��  
���G  ��� ����&   3�=�������� ����'  V3�=�ߍ\Ŏ� \�X � \ێ �3�=��\��a. 3�=���#  �a��
��3�=�bF�\���
��  
�� ߖ�> ����( �3�=����Z%3�=�̐��\���@ 3�=�ߑ#  ��:%3�=�����\��@ 3�=��(3�=���\�� \� �3�=���\�2a. 3�=���#  �a��
��3�=�ɔ� �3�=��� �	3�=���� �
3�=��b �; ��3�=��c\���3�=���d ��d�3�=�ەc��3 �u  ��
d�3�=���c��3 �u  �0 ��	d�@���3�=�������
��  
�� 
��  
�� ��� ����)   3�=�������� ����*  TCZ�3�=������
�:%3�=�Ƙ��;a�F�����
@3�=��V3�=�����
a� �3�=��
���  
��ɚ� ����+  -CZ�3�=���ab���
3�=���ab* �3�=��
ݚ�  
����� ����,  TCZ�3�=���ٛ�
�:%3�=����;a�F��ٛ�
@3�=��V3�=���ٛ�
a� �3�=��
���  
�����> ����-�CQ�3�=���abΈ ��3�=�֝cF3�=���� �3�=���d �; ��3�=�ΞeadF��a �@�3�=���e�@,3�=��V3�=���d�F)3�=�ҟadF��a ��@ 3�=��cZ�?B\\�
���   
�c� 
���
��� 
��	 
ɞ�  �<�> ����.  )3�=��;�3�=�ߠa\�� b E@ �3�=��
�f�  
�� ��> ���/  3�=�ԡa@�3�=��
���  ��> ����0 Y3�=���a[�\�
��3�=���R�F� a��3�=���b�:%3�=����\��a \�I �@ 3�=�Τb�
���  
��� �1� ����1e3�=���a �; ��3�=��\˦� \�X � \�X b \� �3�=�ئ\�1c. 3�=���#  �c��
@���3�=��\\�
���  
�c�G 
�� �� ����2  3�=�ڧa��3�=��
���   ��� ����3  fCZ�3�=���a�3�=�ڨ]�� �3�=��������3�=���� �\�i �3�=���\�4��3�=��
���  ��� ����4  3�=�ڪa��3�=��
��  ܫ� ����5  3�=���a��3�=��
��  �� ����6  @CZ�3�=���\��a \� �3�=�ݬ\��a. 3�=���#  �a��
��
��  
�g��<� ����7  3�=��<a��3�=��
�<�  ��� ���� 8  VCZ�3�=����Z%3�=������@ 3�=��(3�=�Į���;�=�F�;a�G�;b�H��3�=��
�J�  
����� ���� 9  VCZ�3�=����Z%3�=������@ 3�=��(3�=�����;�>�F�;a�G�;b�H��3�=��
�J�  
��Ǳ� ���� :  VCZ�3�=����Z%3�=������@ 3�=��(3�=������;�?�F�;a�G�;b�H��3�=��
�J�  
���� ���� ;  0CZ�3�=������;�@�F�;a�G�;b�H��3�=��
�J�  
��
������< ���> ����=  L3�=�Ǻd���  �c��*3�=���d��� d��� @�<�ʹ a��E�
b�@ �3�=��
���  
�G� 
��  
��  ���> ����>  J3�=���[�� �c��*3�=���d��� d��� @�<�ٶ a��E�
b�@ �3�=��
���  
�G� 
��  
��  ���> ����?  �3�=���c�:%*3�=���d��� d��� @�<��� a��E�
b�@N3�=���c��= F%";�c��  �\�
%";�a��E�
b��:%""3�=����<��� a��E�
b�@ �3�=��
���  
�G� 
��  
��  ���> ����@  �3�=���a��E�
(3�=���b\�� c \�� a��E�
   ����
@%3�=��V3�=���b\�� c \�I   ����
3�=�����
��= F)*3�=���d��� d��� @�<��� a��E�
b�@ �3�=��
���  
�G� 
��  
��  ���  A  53�=���a�3�=���\�
�@ 3�=��(3�=���bva��3�=��
�G�  
��� ��� ����B �3�=���� �3�=�����   3�=���� �; ��3�=���aF�����
3�=���aG���J�
3�=���aH����
3�=����J�
�E\3�=����J�
��
 �; ��3�=���vb�3�=���?����@ 3�=���3�=���]b]b���
���
@��@(3�=��I3�=���]�J�
]�J�
���
���
?.����@ 3�=��(3�=���]�� ����
3�=������
\���3�=������
\���]�Q @ 3�=���v�F%3�=���F @ 3�=���v�F%������,/���
����  
�G�G  ��� ����C   
3�=���v���� ����D  <CZ�3�=���a3�=���v";�va�"�@ 3�=���v";�v�F)"�
�G� ��� ����E  13�=��F�Z%3�=��F( @ 3�=���R�F� �
a���
�G�  ���> ����F �3�=���]�=�
 �; ��3�=����	a�]a��� %3�=���?����@ 3�=��Q3�=���]a��3�=����	a�3�=���;�&�   3�=���b�E3�=���b$ �\��%]a�@43�=���b�E3�=���bG%]a�@3�=��I3�=���b�!]a�3�=��3�=���]a�^@Q]a�3�=���?\   @3�=���;�
& 3�=����
�b]a�3�=���?.   @3�=���;�&3�=���]a��!]a�3�=���?    �?������3�=�����
�G��  
��G 
�E�>  
�E�> �� ����q 3�=���a\�� �3�=���b ��
��  
��� 