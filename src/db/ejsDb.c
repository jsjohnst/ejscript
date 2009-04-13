/*
 *  ejsDb.c -- Database class
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 *
 *  This file is currently hard-code for SQLite. In the future, it will be split into
 *  a generic adapter top-end and a set of database adapters.
 *
 *  Todo:
 *      - should handle SQLITE_BUSY for multiuser access. Need to set the default timeout
 */

/********************************** Includes **********************************/

#include    "ejs.h"

#include    "sqlite3.h"
#include    "ejs.db.slots.h"

#if BLD_FEATURE_EJS_DB

/*********************************** Locals ***********************************/

typedef struct EjsDb
{
    EjsObject       obj;
    sqlite3         *sdb;               /* Sqlite handle */
    MprHeap         *arena;             /* Memory context arena */
    MprThreadLocal  *tls;               /* Thread local data for setting Sqlite memory context */
} EjsDb;


/********************************** Forwards **********************************/

static int dbDestructor(EjsDb **db);

/************************************ Code ************************************/
/*
 *  DB Constructor and also used for constructor for sub classes.
 *
 *  function DB(connectionString: String)
 */
static EjsVar *dbConstructor(Ejs *ejs, EjsDb *db, int argc, EjsVar **argv)
{
    sqlite3     *sdb;
    EjsDb       **dbp;
    char        *path;

    path = ejsGetString(argv[0]);    
    
    /*
     *  Create a memory context for use by sqlite. This is a virtual paged memory region.
     *  TODO - this is not ideal for long running applications.
     */
    db->arena = mprAllocArena(ejs, "sqlite", EJS_MAX_DB_MEM, 0, 0);
    if (db->arena == 0) {
        return 0;
    }
    
    /*
     *  Create a destructor object so we can cleanup and close the database. Must create after the arena so it will be
     *  invoked before the arena is freed. 
     */
    if ((dbp = mprAllocObject(ejs, 1, (MprDestructor) dbDestructor)) == 0) {
        ejsThrowMemoryError(ejs);
        return 0;
    }
    *dbp = db;
    
    db->tls = mprCreateThreadLocal(db->arena);
    if (db->tls == 0) {
        return 0;
    }
    ejsSetDbMemoryContext(db->tls, db->arena);

    sdb = 0;
    if (sqlite3_open(path, &sdb /* TODO remove , SQLITE_OPEN_READWRITE, 0 */) != SQLITE_OK) {
        ejsThrowIOError(ejs, "Can't open database %s", path);
        return 0;
    }
    db->sdb = sdb;

    sqlite3_busy_timeout(sdb, 15000);

    /*
     *  Query or change the count-changes flag. Normally, when the count-changes flag is not set, INSERT, UPDATE and
     *  DELETE statements return no data. When count-changes is set, each of these commands returns a single row of
     *  data consisting of one integer value - the number of rows inserted, modified or deleted by the command. The
     *  returned change count does not include any insertions, modifications or deletions performed by triggers.
     */
//  sqlite3_exec(sdb, "PRAGMA count_changes = OFF", NULL, NULL, NULL);

    ejsSetProperty(ejs, (EjsVar*) db, ES_ejs_db_Database__connection, (EjsVar*) ejsCreateString(ejs, path));
    ejsSetProperty(ejs, (EjsVar*) db, ES_ejs_db_Database__name, (EjsVar*) ejsCreateString(ejs, mprGetBaseName(path)));
    
    return 0;
}


static int dbDestructor(EjsDb **dbp)
{
    EjsDb       *db;

    db = *dbp;

    if (db->sdb) {
        ejsSetDbMemoryContext(db->tls, db->arena);
        sqlite3_close(db->sdb);
        db->sdb = 0;
    }
    return 0;
}


/*
 *  function close(): Void
 */
static int closeDb(Ejs *ejs, EjsDb *db, int argc, EjsVar **argv)
{
    mprAssert(ejs);
    mprAssert(db);

    if (db->sdb) {
        ejsSetDbMemoryContext(db->tls, db->arena);
        sqlite3_close(db->sdb);
        db->sdb = 0;
    }
    return 0;
}


#if UNUSED
static int commitDb(Ejs *ejs, EjsVar *db, int argc, EjsVar **argv)
{
    mprAssert(ejs);
    mprAssert(db);

    return 0;
}


static int rollbackDb(Ejs *ejs, EjsVar *db, int argc, EjsVar **argv)
{
    mprAssert(ejs);
    mprAssert(db);

    return 0;
}
#endif


/*
 *  function sql(cmd: String): Array
 *
 *  Will support multiple sql cmds but will only return one result table.
 */
static EjsVar *sql(Ejs *ejs, EjsDb *db, int argc, EjsVar **argv)
{
    sqlite3         *sdb;
    sqlite3_stmt    *stmt;
    EjsArray        *result;
    EjsVar          *row, *svalue;
    EjsName         qname;
    cchar           *tail, *colName, *cmd, *value;
    int             i, ncol, rc, retries, rowNum;

    mprAssert(ejs);
    mprAssert(db);

    cmd = ejsGetString(argv[0]);
    
    ejsSetDbMemoryContext(db->tls, db->arena);

    rc = SQLITE_OK;
    retries = 0;
    sdb = db->sdb;

    if (sdb == 0) {
        ejsThrowIOError(ejs, "Database is closed");
        return 0;
    }
    mprAssert(sdb);

    result = ejsCreateArray(ejs, 0);
    if (result == 0) {
        return 0;
    }

    while (cmd && *cmd && (rc == SQLITE_OK || (rc == SQLITE_SCHEMA && ++retries < 2))) {

        stmt = 0;
        rc = sqlite3_prepare_v2(sdb, cmd, -1, &stmt, &tail);
        if (rc != SQLITE_OK) {
            continue;
        }
        if (stmt == 0) {
            /* Comment or white space */
            cmd = tail;
            continue;
        }

        ncol = sqlite3_column_count(stmt);

        for (rowNum = 0; ; rowNum++) {

            rc = sqlite3_step(stmt);

            if (rc == SQLITE_ROW) {

                row = (EjsVar*) ejsCreateSimpleObject(ejs);
                if (row == 0) {
                    sqlite3_finalize(stmt);
                    return 0;
                }
                if (ejsSetProperty(ejs, (EjsVar*) result, rowNum, row) < 0) {
                    /* TODO rc */
                }
                for (i = 0; i < ncol; i++) {
                    colName = sqlite3_column_name(stmt, i);
                    value = (cchar*) sqlite3_column_text(stmt, i);
                    ejsName(&qname, EJS_EMPTY_NAMESPACE, mprStrdup(row, colName));
                    if (ejsLookupProperty(ejs, row, &qname) < 0) {
                        svalue = (EjsVar*) ejsCreateString(ejs, mprStrdup(row, value));
                        if (ejsSetPropertyByName(ejs, row, &qname, svalue) < 0) {
                            /* TODO */
                        }
                    }
                }
            } else {
                rc = sqlite3_finalize(stmt);
                stmt = 0;

                if (rc != SQLITE_SCHEMA) {
                    //  TODO - what is this?
                    retries = 0;
                    for (cmd = tail; isspace(*cmd); cmd++) {
                        ;
                    }
                }
                break;
            }
#if UNUSED
            /* TODO -- should we be doing this */
            cmd = tail;
#endif
        }
    }

    if (stmt) {
        rc = sqlite3_finalize(stmt);
    }

    if (rc != SQLITE_OK) {
        if (rc == sqlite3_errcode(sdb)) {
            ejsThrowIOError(ejs, "SQL error: %s", sqlite3_errmsg(sdb));
        } else {
            ejsThrowIOError(ejs, "Unspecified SQL error");
        }
        return 0;
    }

    return (EjsVar*) result;
}


#if UNUSED
/*
 *  Save database changes
 *
 *  function save(): Void
 */
static EjsVar *save(Ejs *ejs, EjsDb *db, int argc, EjsVar **argv)
{
    EjsArray    *ap;
    char        **data, *error;
    int         rc, rowCount, i;

    ejsSetDbMemoryContext(db->tls, db->arena);

    ap = ejsCreateArray(ejs, 0);

    rc = sqlite3_exec(db->sdb,
      "SELECT name FROM sqlite_master "
      "WHERE type IN ('table','view') AND name NOT LIKE 'sqlite_%'"
      "UNION ALL "
      "SELECT name FROM sqlite_temp_master "
      "WHERE type IN ('table','view') "
      "ORDER BY 1",
      0,  &error);

    if (error) {
        ejsThrowIOError(ejs, "%s", error);
        sqlite3_free(error);
    }
    if (rc == SQLITE_OK){
        for (i = 1; i <= rowCount; i++) {
            ejsSetProperty(ejs, (EjsVar*) ap, i - 1, (EjsVar*) ejsCreateString(ejs, data[i]));
        }
    }
    sqlite3_free_table(data);
    return (EjsVar*) ap;
}
#endif


#if UNUSED
/*
 *  Return an array of table names in the database.
 *  function get tables(): Array
 */
static EjsVar *tables(Ejs *ejs, EjsDb *db, int argc, EjsVar **argv)
{
    EjsArray    *ap;
    char        **data, *error;
    int         rc, rowCount, i;

    ejsSetDbMemoryContext(db->tls, db->arena);

    ap = ejsCreateArray(ejs, 0);

    rc = sqlite3_get_table(db->sdb,
      "SELECT name FROM sqlite_master "
      "WHERE type IN ('table','view') AND name NOT LIKE 'sqlite_%'"
      "UNION ALL "
      "SELECT name FROM sqlite_temp_master "
      "WHERE type IN ('table','view') "
      "ORDER BY 1",
      &data, &rowCount, 0, &error);

    if (error) {
        ejsThrowIOError(ejs, "%s", error);
        sqlite3_free(error);
    }
    if (rc == SQLITE_OK){
        for (i = 1; i <= rowCount; i++) {
            ejsSetProperty(ejs, (EjsVar*) ap, i - 1, (EjsVar*) ejsCreateString(ejs, data[i]));
        }
    }
    sqlite3_free_table(data);
    return (EjsVar*) ap;
}
#endif


/****************************** Type Helpers *******************************/
/*
 *  Called by the garbage colllector
 */
static EjsVar *finalizeDb(Ejs *ejs, EjsDb *db)
{
    if (db->sdb) {
        ejsSetDbMemoryContext(db->tls, db->arena);
        closeDb(ejs, db, 0, 0);
        db->sdb = 0;
    }
    return 0;
}

/*********************************** Factory *******************************/

void ejsConfigureDbTypes(Ejs *ejs)
{
    EjsType     *type;
    EjsName     qname;

    ejsName(&qname, "ejs.db", "Database");
    type = (EjsType*) ejsGetPropertyByName(ejs, ejs->global, &qname);
    if (type == 0 || !ejsIsType(type)) {
        ejs->hasError = 1;
        return;
    }

    type->instanceSize = sizeof(EjsDb);
    type->helpers->finalizeVar = (EjsFinalizeVarHelper) finalizeDb;

    ejsBindMethod(ejs, type, ES_ejs_db_Database_Database, (EjsNativeFunction) dbConstructor);
    ejsBindMethod(ejs, type, ES_ejs_db_Database_close, (EjsNativeFunction) closeDb);
    ejsBindMethod(ejs, type, ES_ejs_db_Database_sql, (EjsNativeFunction) sql);

#if UNUSED
    ejsSetAccessors(ejs, type, ES_ejs_db_Database_tables, (EjsNativeFunction) tables, -1, 0);
    ejsBindMethod(ejs, type, ES_ejs_db_Database_start, startDb);
    ejsBindMethod(ejs, type, ES_ejs_db_Database_commit, commitDb);
    ejsBindMethod(ejs, type, ES_ejs_db_Database_rollback, rollbackDb);
#endif
}


/*
 *  Loadable module interface
 */
MprModule *ejs_dbModuleInit(Ejs *ejs)
{
    MprModule   *module;

    module = mprCreateModule(ejs, "db", BLD_VERSION, 0, 0, 0);
    if (module == 0) {
        return 0;
    }

    ejsSetGeneration(ejs, EJS_GEN_ETERNAL);
    ejsConfigureDbTypes(ejs);
    ejsSetGeneration(ejs, EJS_GEN_OLD);

    return module;
}


#endif /* BLD_FEATURE_EJS_DB */


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
