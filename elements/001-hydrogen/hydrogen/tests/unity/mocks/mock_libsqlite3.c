/*
 * Mock libsqlite3 functions for unit testing
 *
 * This file provides mock implementations of libsqlite3 functions
 * to enable testing of SQLite database operations.
 */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Include the header but undefine the macros to access real functions if needed
#include "mock_libsqlite3.h"

// Undefine the macros in this file so we can declare the functions
#undef sqlite3_open_ptr
#undef sqlite3_close_ptr
#undef sqlite3_exec_ptr
#undef sqlite3_extended_result_codes_ptr
#undef sqlite3_free_ptr
#undef sqlite3_errmsg_ptr

// Function prototypes
int mock_sqlite3_open(const char* filename, void** ppDb);
int mock_sqlite3_close(void* db);
int mock_sqlite3_exec(void* db, const char* sql, int (*callback)(void*, int, char**, char**), void* arg, char** errmsg);
int mock_sqlite3_extended_result_codes(void* db, int onoff);
void mock_sqlite3_free(void* ptr);
const char* mock_sqlite3_errmsg(void* db);
void mock_libsqlite3_set_sqlite3_open_result(int result);
void mock_libsqlite3_set_sqlite3_close_result(int result);
void mock_libsqlite3_set_sqlite3_exec_result(int result);
void mock_libsqlite3_set_sqlite3_errmsg_result(const char* errmsg);
void mock_libsqlite3_reset_all(void);

// Static variables to store mock state
static int mock_sqlite3_open_result = 0; // 0 = SQLITE_OK
static int mock_sqlite3_close_result = 0; // 0 = SQLITE_OK
static int mock_sqlite3_exec_result = 0; // 0 = SQLITE_OK
static const char* mock_sqlite3_errmsg_result = NULL;

// Mock implementation of sqlite3_open
int mock_sqlite3_open(const char* filename, void** ppDb) {
    (void)filename; // Suppress unused parameter

    if (ppDb) {
        *ppDb = (void*)0x12345678; // Mock database handle
    }

    return mock_sqlite3_open_result;
}

// Mock implementation of sqlite3_close
int mock_sqlite3_close(void* db) {
    (void)db; // Suppress unused parameter
    return mock_sqlite3_close_result;
}

// Mock implementation of sqlite3_exec
int mock_sqlite3_exec(void* db, const char* sql, int (*callback)(void*, int, char**, char**), void* arg, char** errmsg) {
    (void)db;      // Suppress unused parameter
    (void)sql;     // Suppress unused parameter
    (void)callback; // Suppress unused parameter
    (void)arg;     // Suppress unused parameter

    if (errmsg && mock_sqlite3_errmsg_result) {
        *errmsg = strdup(mock_sqlite3_errmsg_result);
    } else if (errmsg) {
        *errmsg = NULL;
    }

    return mock_sqlite3_exec_result;
}

// Mock implementation of sqlite3_extended_result_codes
int mock_sqlite3_extended_result_codes(void* db, int onoff) {
    (void)db;    // Suppress unused parameter
    (void)onoff; // Suppress unused parameter
    return 0; // SQLITE_OK
}

// Mock implementation of sqlite3_free
void mock_sqlite3_free(void* ptr) {
    (void)ptr; // Suppress unused parameter
    // Do nothing in mock
}

// Mock implementation of sqlite3_errmsg
const char* mock_sqlite3_errmsg(void* db) {
    (void)db; // Suppress unused parameter
    return mock_sqlite3_errmsg_result ? mock_sqlite3_errmsg_result : "mock error";
}

// Mock control functions
void mock_libsqlite3_set_sqlite3_open_result(int result) {
    mock_sqlite3_open_result = result;
}

void mock_libsqlite3_set_sqlite3_close_result(int result) {
    mock_sqlite3_close_result = result;
}

void mock_libsqlite3_set_sqlite3_exec_result(int result) {
    mock_sqlite3_exec_result = result;
}

void mock_libsqlite3_set_sqlite3_errmsg_result(const char* errmsg) {
    mock_sqlite3_errmsg_result = errmsg;
}

void mock_libsqlite3_reset_all(void) {
    mock_sqlite3_open_result = 0; // SQLITE_OK
    mock_sqlite3_close_result = 0; // SQLITE_OK
    mock_sqlite3_exec_result = 0; // SQLITE_OK
    mock_sqlite3_errmsg_result = NULL;
}