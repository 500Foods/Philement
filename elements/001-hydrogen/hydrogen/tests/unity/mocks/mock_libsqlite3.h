/*
 * Mock libsqlite3 functions for unit testing
 *
 * This file provides mock implementations of libsqlite3 functions
 * to enable testing of SQLite database operations.
 */

#ifndef MOCK_LIBSQLITE3_H
#define MOCK_LIBSQLITE3_H

#include <stddef.h>
#include <stdbool.h>

// Mock function declarations - these will override the real ones when USE_MOCK_LIBSQLITE3 is defined
#ifdef USE_MOCK_LIBSQLITE3

// Mock function prototypes
int mock_sqlite3_open(const char* filename, void** ppDb);
int mock_sqlite3_close(void* db);
int mock_sqlite3_exec(void* db, const char* sql, int (*callback)(void*, int, char**, char**), void* arg, char** errmsg);
int mock_sqlite3_extended_result_codes(void* db, int onoff);
void mock_sqlite3_free(void* ptr);
const char* mock_sqlite3_errmsg(void* db);

// Mock control functions for tests
void mock_libsqlite3_set_sqlite3_open_result(int result);
void mock_libsqlite3_set_sqlite3_close_result(int result);
void mock_libsqlite3_set_sqlite3_exec_result(int result);
void mock_libsqlite3_set_sqlite3_errmsg_result(const char* errmsg);
void mock_libsqlite3_reset_all(void);

#endif // USE_MOCK_LIBSQLITE3

#endif // MOCK_LIBSQLITE3_H