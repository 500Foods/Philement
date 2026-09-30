/*
 * MSSQL Database Engine - Type Definitions Header
 *
 * Header file for MSSQL engine type definitions.
 */

#ifndef DATABASE_ENGINE_MSSQL_TYPES_H
#define DATABASE_ENGINE_MSSQL_TYPES_H

#include <src/database/database.h>

// Function pointer types for unixODBC functions (same ODBC API shape as DB2)
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
typedef int (*SQLPrepare_t)(void*, unsigned char*, int);
typedef int (*SQLExecute_t)(void*);
typedef int (*SQLFreeStmt_t)(void*, int);
typedef int (*SQLDescribeCol_t)(void*, int, unsigned char*, int, short*, int*, int*, short*, short*);
typedef int (*SQLBindParameter_t)(void*, unsigned short, short, short, short, unsigned long, short, void*, long, long*);

// Additional function pointers for connection and error handling
typedef int (*SQLDriverConnect_t)(void*, void*, unsigned char*, short, unsigned char*, short, short*, unsigned short);
typedef int (*SQLGetDiagRec_t)(short, void*, short, unsigned char*, long*, unsigned char*, short, short*);
typedef int (*SQLSetConnectAttr_t)(void*, int, long, int);
typedef int (*SQLSetEnvAttr_t)(void*, int, long*, int);
typedef int (*SQLCancel_t)(void*);

// MSSQL function pointers (loaded dynamically or mocked) - mssql_ prefix avoids collision with DB2
extern SQLAllocHandle_t mssql_SQLAllocHandle_ptr;
extern SQLConnect_t mssql_SQLConnect_ptr;
extern SQLExecDirect_t mssql_SQLExecDirect_ptr;
extern SQLFetch_t mssql_SQLFetch_ptr;
extern SQLGetData_t mssql_SQLGetData_ptr;
extern SQLNumResultCols_t mssql_SQLNumResultCols_ptr;
extern SQLRowCount_t mssql_SQLRowCount_ptr;
extern SQLFreeHandle_t mssql_SQLFreeHandle_ptr;
extern SQLDisconnect_t mssql_SQLDisconnect_ptr;
extern SQLEndTran_t mssql_SQLEndTran_ptr;
extern SQLPrepare_t mssql_SQLPrepare_ptr;
extern SQLExecute_t mssql_SQLExecute_ptr;
extern SQLFreeStmt_t mssql_SQLFreeStmt_ptr;
extern SQLDescribeCol_t mssql_SQLDescribeCol_ptr;
extern SQLBindParameter_t mssql_SQLBindParameter_ptr;
extern SQLDriverConnect_t mssql_SQLDriverConnect_ptr;
extern SQLGetDiagRec_t mssql_SQLGetDiagRec_ptr;
extern SQLSetConnectAttr_t mssql_SQLSetConnectAttr_ptr;
extern SQLSetEnvAttr_t mssql_SQLSetEnvAttr_ptr;
extern SQLCancel_t mssql_SQLCancel_ptr;

// ODBC constants (same ODBC API shape as DB2)
#define SQL_HANDLE_ENV 1
#define SQL_HANDLE_DBC 2
#define SQL_HANDLE_STMT 3
#define SQL_SUCCESS 0
#define SQL_SUCCESS_WITH_INFO 1
#define SQL_ATTR_ODBC_VERSION 20
#define SQL_OV_ODBC3 3
#define SQL_IS_INTEGER -6
#define SQL_IS_UINTEGER -5
#define SQL_COMMIT 0
#define SQL_ROLLBACK 1
#define SQL_CLOSE 0
#define SQL_NTS -3
#define SQL_NULL_DATA -1
#define SQL_C_CHAR 1
// SQL data types for column type detection
#define SQL_INTEGER 4
#define SQL_SMALLINT 5
#define SQL_BIGINT -5
#define SQL_DECIMAL 3
#define SQL_NUMERIC 2
#define SQL_REAL 7
#define SQL_FLOAT 6
#define SQL_DOUBLE 8
#define SQL_CHAR 1
#define SQL_VARCHAR 12
#define SQL_LONGVARCHAR -1
// Auto-commit control
#define SQL_ATTR_AUTOCOMMIT 102
#define SQL_AUTOCOMMIT_OFF 0
#define SQL_AUTOCOMMIT_ON 1

// Parameter binding constants
#define SQL_PARAM_INPUT 1
#define SQL_C_LONG 4
#define SQL_C_DOUBLE 8
#define SQL_C_SHORT 5

// Date/time SQL types
#define SQL_TYPE_DATE 91
#define SQL_TYPE_TIME 92
#define SQL_TYPE_TIMESTAMP 93
#define SQL_C_TYPE_DATE 91
#define SQL_C_TYPE_TIME 92
#define SQL_C_TYPE_TIMESTAMP 93

// MSSQL-specific ODBC constants for Driver 18
#define SQL_COPT_SS_BASE 1090000000     // SQL Server-specific connect attributes
#define SQL_COPT_SS_ENCRYPT 3
#define SQL_COPT_SS_TRUST_SERVER_CERTIFICATE 2

// Current catalog/database
#define SQL_ATTR_CURRENT_CATALOG 109

// Row array size for batch fetching
#define SQL_ATTR_ROW_ARRAY_SIZE 27

// SQL_NO_DATA (no more rows / no data retrieved)
#define SQL_NO_DATA 100

// Type definitions
typedef void* SQLPOINTER;

// Date/time structure definitions
typedef struct {
    short year;
    unsigned short month;
    unsigned short day;
} SQL_DATE_STRUCT;

typedef struct {
    unsigned short hour;
    unsigned short minute;
    unsigned short second;
} SQL_TIME_STRUCT;

typedef struct {
    short year;
    unsigned short month;
    unsigned short day;
    unsigned short hour;
    unsigned short minute;
    unsigned short second;
    unsigned long fraction;
} SQL_TIMESTAMP_STRUCT;

// Prepared statement cache structure
typedef struct PreparedStatementCache {
    char** names;
    size_t count;
    size_t capacity;
    pthread_mutex_t lock;
} PreparedStatementCache;

// MSSQL-specific connection structure (odbcHandle/odbcConn are void* as in DB2)
typedef struct MSSQLConnection {
    void* environment;  // SQLHENV
    void* connection;   // SQLHDBC
    PreparedStatementCache* prepared_statements;
    void* active_stmt;   // SQLHSTMT for the currently in-flight query, or NULL
    pthread_mutex_t active_stmt_lock;  // Protects active_stmt read/write
} MSSQLConnection;

#endif // DATABASE_ENGINE_MSSQL_TYPES_H
