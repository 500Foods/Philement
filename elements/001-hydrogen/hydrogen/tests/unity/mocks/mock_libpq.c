/*
 * Mock libpq functions for unit testing
 *
 * This file provides mock implementations of libpq functions
 * to enable testing of PostgreSQL database operations.
 */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

// Constants
#define CONNECTION_OK 0
#define CONNECTION_BAD 1
#define PGRES_COMMAND_OK 1
#define PGRES_TUPLES_OK 2
#define PGRES_FATAL_ERROR 7

// Include the header but undefine the macros to access real functions if needed
#include "mock_libpq.h"

// Undefine the macros in this file so we can declare the functions
#undef PQconnectdb_ptr
#undef PQstatus_ptr
#undef PQerrorMessage_ptr
#undef PQfinish_ptr
#undef PQexec_ptr
#undef PQresultStatus_ptr
#undef PQclear_ptr
#undef PQntuples_ptr
#undef PQnfields_ptr
#undef PQfname_ptr
#undef PQgetvalue_ptr
#undef PQcmdTuples_ptr
#undef PQreset_ptr
#undef PQprepare_ptr
#undef PQescapeStringConn_ptr
#undef PQping_ptr
#undef check_timeout_expired

// Function prototypes
void mock_libpq_initialize(void);
void* mock_PQconnectdb(const char* conninfo);
int mock_PQstatus(void* conn);
char* mock_PQerrorMessage(void* conn);
void mock_PQfinish(void* conn);
void* mock_PQexec(void* conn, const char* query);
int mock_PQresultStatus(const void* res);
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

// Mock control functions
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

// Static variables to store mock state
static void* mock_PQconnectdb_result = (void*)0x12345678;
static int mock_PQstatus_result = CONNECTION_OK;
static char* mock_PQerrorMessage_result = NULL;
static void* mock_PQexec_result = NULL;
static int mock_PQresultStatus_result = PGRES_COMMAND_OK;
static int mock_PQntuples_result = 1;
static int mock_PQnfields_result = 1;
static char* mock_PQfname_result = NULL;
static char* mock_PQgetvalue_result = NULL;
static char* mock_PQcmdTuples_result = NULL;
static int mock_PQping_result = 0; // PING_OK
static bool mock_check_timeout_expired_result = false;
static bool mock_check_timeout_expired_use_mock = false;


void mock_libpq_reset_all(void) {
    mock_PQconnectdb_result = (void*)0x12345678;
    mock_PQstatus_result = CONNECTION_OK;
    if (mock_PQerrorMessage_result) free(mock_PQerrorMessage_result);
    mock_PQerrorMessage_result = strdup("");
    mock_PQexec_result = NULL;
    mock_PQresultStatus_result = PGRES_COMMAND_OK;
    mock_PQntuples_result = 1;
    mock_PQnfields_result = 1;
    if (mock_PQfname_result) free(mock_PQfname_result);
    mock_PQfname_result = strdup("test_column");
    if (mock_PQgetvalue_result) free(mock_PQgetvalue_result);
    mock_PQgetvalue_result = strdup("1");
    if (mock_PQcmdTuples_result) free(mock_PQcmdTuples_result);
    mock_PQcmdTuples_result = strdup("1");
    mock_PQping_result = 0; // PING_OK
    mock_check_timeout_expired_result = false;
    mock_check_timeout_expired_use_mock = false;
}

// Initialize mock strings (called once at startup)
void mock_libpq_initialize(void) {
    if (!mock_PQerrorMessage_result) mock_PQerrorMessage_result = strdup("");
    if (!mock_PQfname_result) mock_PQfname_result = strdup("test_column");
    if (!mock_PQgetvalue_result) mock_PQgetvalue_result = strdup("1");
    if (!mock_PQcmdTuples_result) mock_PQcmdTuples_result = strdup("1");
}

// Mock implementations
void* mock_PQconnectdb(const char* conninfo) {
    (void)conninfo; // Suppress unused parameter
    return mock_PQconnectdb_result;
}

int mock_PQstatus(void* conn) {
    (void)conn; // Suppress unused parameter
    return mock_PQstatus_result;
}

char* mock_PQerrorMessage(void* conn) {
    (void)conn; // Suppress unused parameter
    return mock_PQerrorMessage_result;
}

void mock_PQfinish(void* conn) {
    (void)conn; // Suppress unused parameter
    // Do nothing for mock
}

void* mock_PQexec(void* conn, const char* query) {
    (void)conn;  // Suppress unused parameter
    (void)query; // Suppress unused parameter
    return mock_PQexec_result;
}

int mock_PQresultStatus(const void* res) {
    if (res == NULL) {
        return PGRES_FATAL_ERROR; // If res is NULL, return error
    }
    return mock_PQresultStatus_result;
}

void mock_PQclear(void* res) {
    (void)res; // Suppress unused parameter
    // Do nothing for mock
}

int mock_PQntuples(void* res) {
    (void)res; // Suppress unused parameter
    return mock_PQntuples_result;
}

int mock_PQnfields(void* res) {
    (void)res; // Suppress unused parameter
    return mock_PQnfields_result;
}

char* mock_PQfname(void* res, int column_number) {
    (void)res; // Suppress unused parameter
    (void)column_number; // Suppress unused parameter
    return mock_PQfname_result;
}

char* mock_PQgetvalue(void* res, int row_number, int column_number) {
    (void)res; // Suppress unused parameter
    (void)row_number; // Suppress unused parameter
    (void)column_number; // Suppress unused parameter
    return mock_PQgetvalue_result;
}

char* mock_PQcmdTuples(void* res) {
    (void)res; // Suppress unused parameter
    return mock_PQcmdTuples_result;
}

void mock_PQreset(void* conn) {
    (void)conn; // Suppress unused parameter
    // Do nothing for mock
}

void* mock_PQprepare(void* conn, const char* stmtName, const char* query, int nParams, const char* const* paramTypes) {
    (void)conn; // Suppress unused parameter
    (void)stmtName; // Suppress unused parameter
    (void)query; // Suppress unused parameter
    (void)nParams; // Suppress unused parameter
    (void)paramTypes; // Suppress unused parameter
    return (void*)0x87654321; // Mock prepared statement handle
}

size_t mock_PQescapeStringConn(void* conn, char* to, const char* from, size_t length, int* error) {
    (void)conn; // Suppress unused parameter
    (void)to; // Suppress unused parameter
    (void)from; // Suppress unused parameter
    (void)length; // Suppress unused parameter
    if (error) *error = 0;
    return 0;
}

int mock_PQping(const char* conninfo) {
    (void)conninfo; // Suppress unused parameter
    return mock_PQping_result;
}

bool mock_check_timeout_expired(time_t start_time, int timeout_seconds) {
    if (mock_check_timeout_expired_use_mock) {
        return mock_check_timeout_expired_result;
    } else {
        // Use real logic
        return (time(NULL) - start_time) >= timeout_seconds;
    }
}

// Mock control functions
void mock_libpq_set_PQconnectdb_result(void* result) {
    mock_PQconnectdb_result = result;
}

void mock_libpq_set_PQstatus_result(int status) {
    mock_PQstatus_result = status;
}

void mock_libpq_set_PQerrorMessage_result(const char* message) {
    if (mock_PQerrorMessage_result) free(mock_PQerrorMessage_result);
    mock_PQerrorMessage_result = strdup(message ? message : "");
}

void mock_libpq_set_PQexec_result(void* result) {
    mock_PQexec_result = result;
}

void mock_libpq_set_PQresultStatus_result(int status) {
    mock_PQresultStatus_result = status;
}

void mock_libpq_set_PQntuples_result(int tuples) {
    mock_PQntuples_result = tuples;
}

void mock_libpq_set_PQnfields_result(int fields) {
    mock_PQnfields_result = fields;
}

void mock_libpq_set_PQfname_result(const char* name) {
    if (mock_PQfname_result) free(mock_PQfname_result);
    mock_PQfname_result = strdup(name ? name : "test_column");
}

void mock_libpq_set_PQgetvalue_result(const char* value) {
    if (mock_PQgetvalue_result) free(mock_PQgetvalue_result);
    mock_PQgetvalue_result = strdup(value ? value : "1");
}

void mock_libpq_set_PQcmdTuples_result(const char* tuples) {
    if (mock_PQcmdTuples_result) free(mock_PQcmdTuples_result);
    mock_PQcmdTuples_result = strdup(tuples ? tuples : "1");
}

void mock_libpq_set_PQping_result(int result) {
    mock_PQping_result = result;
}

void mock_libpq_set_check_timeout_expired_result(bool result) {
    mock_check_timeout_expired_result = result;
}

void mock_libpq_set_check_timeout_expired_use_mock(bool use_mock) {
    mock_check_timeout_expired_use_mock = use_mock;
}