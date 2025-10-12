/*
 * PostgreSQL Database Engine - Shared Types and Constants
 *
 * Shared header file for PostgreSQL types, constants, and function pointer declarations.
 */

#ifndef DATABASE_ENGINE_POSTGRESQL_TYPES_H
#define DATABASE_ENGINE_POSTGRESQL_TYPES_H

#include <time.h>

// Function pointer types for libpq functions
typedef void* (*PQconnectdb_t)(const char* conninfo);
typedef int (*PQstatus_t)(void* conn);
typedef char* (*PQerrorMessage_t)(void* conn);
typedef void (*PQfinish_t)(void* conn);
typedef void* (*PQexec_t)(void* conn, const char* query);
typedef int (*PQresultStatus_t)(void* res);
typedef void (*PQclear_t)(void* res);
typedef int (*PQntuples_t)(void* res);
typedef int (*PQnfields_t)(void* res);
typedef char* (*PQfname_t)(void* res, int column_number);
typedef char* (*PQgetvalue_t)(void* res, int row_number, int column_number);
typedef char* (*PQcmdTuples_t)(void* res);
typedef void (*PQreset_t)(void* conn);
typedef void* (*PQprepare_t)(void* conn, const char* stmtName, const char* query, int nParams, const char* const* paramTypes);
typedef void* (*PQexecPrepared_t)(void* conn, const char* stmtName, int nParams, const char* const* paramValues, const int* paramLengths, const int* paramFormats, int resultFormat);
typedef size_t (*PQescapeStringConn_t)(void* conn, char* to, const char* from, size_t length, int* error);
typedef int (*PQping_t)(const char* conninfo);

// Constants (defined since we can't include libpq-fe.h)
#define CONNECTION_OK 0
#define CONNECTION_BAD 1
#define PGRES_EMPTY_QUERY 0
#define PGRES_COMMAND_OK 1
#define PGRES_TUPLES_OK 2
#define PGRES_COPY_OUT 3
#define PGRES_COPY_IN 4
#define PGRES_BAD_RESPONSE 5
#define PGRES_NONFATAL_ERROR 6
#define PGRES_FATAL_ERROR 7

// Timeout utility function
bool check_timeout_expired(time_t start_time, int timeout_seconds);

#endif // DATABASE_ENGINE_POSTGRESQL_TYPES_H