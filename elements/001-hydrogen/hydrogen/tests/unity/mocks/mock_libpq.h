/*
 * Mock libpq functions for unit testing
 *
 * This file provides mock implementations of libpq functions
 * to enable testing of PostgreSQL database operations.
 */

#ifndef MOCK_LIBPQ_H
#define MOCK_LIBPQ_H

#include <stddef.h>
#include <stdbool.h>
#include <time.h>

// Mock function declarations - these will override the real ones when USE_MOCK_LIBPQ is defined
#ifdef USE_MOCK_LIBPQ

// Mock function prototypes - must match libpq function signatures
void* mock_PQconnectdb(const char* conninfo);
int mock_PQstatus(void* conn);
char* mock_PQerrorMessage(void* conn);
void mock_PQfinish(void* conn);
void* mock_PQexec(void* conn, const char* query);
int mock_PQresultStatus(void* res);
void mock_PQclear(void* res);
int mock_PQntuples(void* res);
int mock_PQnfields(void* res);
char* mock_PQfname(void* res, int column_number);
char* mock_PQgetvalue(void* res, int row_number, int column_number);
char* mock_PQcmdTuples(void* res);
void mock_PQreset(void* conn);
void* mock_PQprepare(void* conn, const char* stmtName, const char* query, int nParams, const char* const* paramTypes);
size_t mock_PQescapeStringConn(void* conn, char* to, const char* from, size_t length, int* error);
int mock_PQping(const char* conninfo);
bool mock_check_timeout_expired(time_t start_time, int timeout_seconds);

// Mock control functions for tests
void mock_libpq_initialize(void);
void mock_libpq_set_PQconnectdb_result(void* result);
void mock_libpq_set_PQstatus_result(int status);
void mock_libpq_set_PQerrorMessage_result(const char* message);
void mock_libpq_set_PQexec_result(void* result);
void mock_libpq_set_PQresultStatus_result(int status);
void mock_libpq_set_PQntuples_result(int tuples);
void mock_libpq_set_PQnfields_result(int fields);
void mock_libpq_set_PQfname_result(const char* name);
void mock_libpq_set_PQgetvalue_result(const char* value);
void mock_libpq_set_PQcmdTuples_result(const char* tuples);
void mock_libpq_set_PQping_result(int result);
void mock_libpq_set_check_timeout_expired_result(bool result);
void mock_libpq_set_check_timeout_expired_use_mock(bool use_mock);
void mock_libpq_reset_all(void);

#endif // USE_MOCK_LIBPQ

#endif // MOCK_LIBPQ_H