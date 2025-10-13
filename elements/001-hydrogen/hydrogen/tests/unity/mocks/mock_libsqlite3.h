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

// Mock function prototypes (always declared)
int mock_sqlite3_open(const char* filename, void** ppDb);
int mock_sqlite3_close(void* db);
int mock_sqlite3_exec(void* db, const char* sql, int (*callback)(void*, int, char**, char**), void* arg, char** errmsg);
int mock_sqlite3_extended_result_codes(void* db, int onoff);
void mock_sqlite3_free(void* ptr);
const char* mock_sqlite3_errmsg(void* db);
int mock_sqlite3_prepare_v2(void* db, const char* sql, int nByte, void** ppStmt, const char** pzTail);
int mock_sqlite3_finalize(void* stmt);
int mock_sqlite3_step(void* stmt);
int mock_sqlite3_column_count(void* stmt);
const char* mock_sqlite3_column_name(void* stmt, int col);
const unsigned char* mock_sqlite3_column_text(void* stmt, int col);
int mock_sqlite3_column_type(void* stmt, int col);
int mock_sqlite3_changes(void* db);
int mock_sqlite3_reset(void* stmt);

// Mock control functions for tests
void mock_libsqlite3_set_sqlite3_open_result(int result);
void mock_libsqlite3_set_sqlite3_close_result(int result);
void mock_libsqlite3_set_sqlite3_exec_result(int result);
void mock_libsqlite3_set_sqlite3_exec_callback_calls(int count);
void mock_libsqlite3_set_sqlite3_errmsg_result(const char* errmsg);
void mock_libsqlite3_set_sqlite3_prepare_v2_result(int result);
void mock_libsqlite3_set_sqlite3_prepare_v2_output_handle(void* handle);
void mock_libsqlite3_set_sqlite3_finalize_result(int result);
void mock_libsqlite3_set_sqlite3_step_result(int result);
void mock_libsqlite3_set_sqlite3_step_row_count(int count);
void mock_libsqlite3_set_sqlite3_column_count_result(int count);
void mock_libsqlite3_set_sqlite3_column_name_result(const char* name);
void mock_libsqlite3_set_sqlite3_column_text_result(const unsigned char* text);
void mock_libsqlite3_set_sqlite3_column_type_result(int type);
void mock_libsqlite3_set_sqlite3_changes_result(int changes);
void mock_libsqlite3_set_sqlite3_reset_result(int result);
void mock_libsqlite3_reset_all(void);

#endif // MOCK_LIBSQLITE3_H