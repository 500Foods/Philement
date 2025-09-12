/*
 * DB2 Database Engine - Type Definitions Header
 *
 * Header file for DB2 engine type definitions.
 */

#ifndef DATABASE_ENGINE_DB2_TYPES_H
#define DATABASE_ENGINE_DB2_TYPES_H

#include "../database.h"

// Function pointer types for libdb2 functions
typedef int (*SQLAllocHandle_t)(int, void*, void**);
typedef int (*SQLConnect_t)(void*, char*, int, char*, int, char*, int);
typedef int (*SQLExecDirect_t)(void*, char*, int);
typedef int (*SQLFetch_t)(void*);
typedef int (*SQLGetData_t)(void*, int, int, void*, int, int*);
typedef int (*SQLNumResultCols_t)(void*, int*);
typedef int (*SQLRowCount_t)(void*, int*);
typedef int (*SQLFreeHandle_t)(int, void*);
typedef int (*SQLDisconnect_t)(void*);
typedef int (*SQLEndTran_t)(int, void*, int);
typedef int (*SQLPrepare_t)(void*, char*, int);
typedef int (*SQLExecute_t)(void*);
typedef int (*SQLFreeStmt_t)(void*, int);

// DB2 function pointers (loaded dynamically)
extern SQLAllocHandle_t SQLAllocHandle_ptr;
extern SQLConnect_t SQLConnect_ptr;
extern SQLExecDirect_t SQLExecDirect_ptr;
extern SQLFetch_t SQLFetch_ptr;
extern SQLGetData_t SQLGetData_ptr;
extern SQLNumResultCols_t SQLNumResultCols_ptr;
extern SQLRowCount_t SQLRowCount_ptr;
extern SQLFreeHandle_t SQLFreeHandle_ptr;
extern SQLDisconnect_t SQLDisconnect_ptr;
extern SQLEndTran_t SQLEndTran_ptr;
extern SQLPrepare_t SQLPrepare_ptr;
extern SQLExecute_t SQLExecute_ptr;
extern SQLFreeStmt_t SQLFreeStmt_ptr;

// Library handle
extern void* libdb2_handle;
extern pthread_mutex_t libdb2_mutex;

// Constants (defined since we can't include sql.h)
#define SQL_HANDLE_ENV 1
#define SQL_HANDLE_DBC 2
#define SQL_HANDLE_STMT 3
#define SQL_SUCCESS 0
#define SQL_COMMIT 0
#define SQL_ROLLBACK 1
#define SQL_CLOSE 0
#define SQL_NTS -3

// Prepared statement cache structure
typedef struct PreparedStatementCache {
    char** names;
    size_t count;
    size_t capacity;
    pthread_mutex_t lock;
} PreparedStatementCache;

// DB2-specific connection structure
typedef struct DB2Connection {
    void* environment;  // SQLHENV
    void* connection;   // SQLHDBC
    PreparedStatementCache* prepared_statements;
} DB2Connection;

#endif // DATABASE_ENGINE_DB2_TYPES_H