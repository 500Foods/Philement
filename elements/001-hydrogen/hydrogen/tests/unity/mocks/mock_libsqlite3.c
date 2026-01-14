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
int mock_sqlite3_prepare_v2(void* db, const char* sql, int nByte, void** ppStmt, const char** pzTail);
int mock_sqlite3_finalize(void* stmt);
int mock_sqlite3_step(void* stmt);
int mock_sqlite3_column_count(void* stmt);
const char* mock_sqlite3_column_name(void* stmt, int col);
const unsigned char* mock_sqlite3_column_text(void* stmt, int col);
int mock_sqlite3_column_type(void* stmt, int col);
int mock_sqlite3_changes(void* db);
int mock_sqlite3_reset(void* stmt);
int mock_sqlite3_bind_int(void* stmt, int col, int value);
int mock_sqlite3_bind_double(void* stmt, int col, double value);
int mock_sqlite3_bind_text(void* stmt, int col, const char* text, int nBytes, void(*destructor)(void*));
int mock_sqlite3_bind_null(void* stmt, int col);
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
void mock_libsqlite3_set_sqlite3_bind_int_result(int result);
void mock_libsqlite3_set_sqlite3_bind_double_result(int result);
void mock_libsqlite3_set_sqlite3_bind_text_result(int result);
void mock_libsqlite3_set_sqlite3_bind_null_result(int result);
void mock_libsqlite3_reset_all(void);

// Static variables to store mock state
static int mock_sqlite3_open_result = 0; // 0 = SQLITE_OK
static int mock_sqlite3_close_result = 0; // 0 = SQLITE_OK
static int mock_sqlite3_exec_result = 0; // 0 = SQLITE_OK
static int mock_sqlite3_exec_callback_calls = 0;
static const char* mock_sqlite3_errmsg_result = NULL;
static int mock_sqlite3_prepare_v2_result = 0; // 0 = SQLITE_OK
static void* mock_sqlite3_prepare_v2_output_handle = NULL;
static int mock_sqlite3_finalize_result = 0; // 0 = SQLITE_OK
static int mock_sqlite3_step_result = 101; // 101 = SQLITE_DONE
static int mock_sqlite3_step_row_count = 0;
static int mock_sqlite3_step_current_row = 0;
static int mock_sqlite3_column_count_result = 0;
static const char* mock_sqlite3_column_name_result = "column";
static const unsigned char* mock_sqlite3_column_text_result = (const unsigned char*)"value";
static int mock_sqlite3_column_type_result = 1; // SQLITE_INTEGER
static int mock_sqlite3_changes_result = 0;
static int mock_sqlite3_reset_result = 0; // 0 = SQLITE_OK
static int mock_sqlite3_bind_int_result = 0; // 0 = SQLITE_OK
static int mock_sqlite3_bind_double_result = 0; // 0 = SQLITE_OK
static int mock_sqlite3_bind_text_result = 0; // 0 = SQLITE_OK
static int mock_sqlite3_bind_null_result = 0; // 0 = SQLITE_OK

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

    if (errmsg && mock_sqlite3_errmsg_result && mock_sqlite3_exec_result != 0) {
        *errmsg = strdup(mock_sqlite3_errmsg_result);
    } else if (errmsg) {
        *errmsg = NULL;
    }

    // If callback is provided and we have configured callback calls, invoke it
    if (callback && mock_sqlite3_exec_callback_calls > 0) {
        for (int i = 0; i < mock_sqlite3_exec_callback_calls; i++) {
            const char* argv_const[] = {"1", "test_value", NULL};
            const char* col_names_const[] = {"id", "name", NULL};
            // Cast away const for callback signature compatibility
            char* argv[3];
            char* col_names[3];
            argv[0] = (char*)argv_const[0];
            argv[1] = (char*)argv_const[1];
            argv[2] = (char*)argv_const[2];
            col_names[0] = (char*)col_names_const[0];
            col_names[1] = (char*)col_names_const[1];
            col_names[2] = (char*)col_names_const[2];
            callback(arg, 2, argv, col_names);
        }
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

// Mock implementation of sqlite3_prepare_v2
int mock_sqlite3_prepare_v2(void* db, const char* sql, int nByte, void** ppStmt, const char** pzTail) {
    (void)db;    // Suppress unused parameter
    (void)sql;   // Suppress unused parameter
    (void)nByte; // Suppress unused parameter
    (void)pzTail; // Suppress unused parameter

    if (ppStmt && mock_sqlite3_prepare_v2_result == 0) {
        // On success, set the output handle
        *ppStmt = mock_sqlite3_prepare_v2_output_handle ? mock_sqlite3_prepare_v2_output_handle : (void*)0x87654321;
    } else if (ppStmt) {
        *ppStmt = NULL;
    }

    return mock_sqlite3_prepare_v2_result;
}

// Mock implementation of sqlite3_finalize
int mock_sqlite3_finalize(void* stmt) {
    (void)stmt; // Suppress unused parameter
    return mock_sqlite3_finalize_result;
}

// Mock implementation of sqlite3_step
int mock_sqlite3_step(void* stmt) {
    (void)stmt; // Suppress unused parameter
    
    // If we have rows to return
    if (mock_sqlite3_step_current_row < mock_sqlite3_step_row_count) {
        mock_sqlite3_step_current_row++;
        return 100; // SQLITE_ROW
    }
    
    // Reset for next call
    mock_sqlite3_step_current_row = 0;
    return mock_sqlite3_step_result;
}

// Mock implementation of sqlite3_column_count
int mock_sqlite3_column_count(void* stmt) {
    (void)stmt; // Suppress unused parameter
    return mock_sqlite3_column_count_result;
}

// Mock implementation of sqlite3_column_name
const char* mock_sqlite3_column_name(void* stmt, int col) {
    (void)stmt; // Suppress unused parameter
    (void)col;  // Suppress unused parameter
    return mock_sqlite3_column_name_result;
}

// Mock implementation of sqlite3_column_text
const unsigned char* mock_sqlite3_column_text(void* stmt, int col) {
    (void)stmt; // Suppress unused parameter
    (void)col;  // Suppress unused parameter
    return mock_sqlite3_column_text_result;
}

// Mock implementation of sqlite3_column_type
int mock_sqlite3_column_type(void* stmt, int col) {
    (void)stmt; // Suppress unused parameter
    (void)col;  // Suppress unused parameter
    return mock_sqlite3_column_type_result;
}

// Mock implementation of sqlite3_changes
int mock_sqlite3_changes(void* db) {
    (void)db; // Suppress unused parameter
    return mock_sqlite3_changes_result;
}

// Mock implementation of sqlite3_reset
int mock_sqlite3_reset(void* stmt) {
    (void)stmt; // Suppress unused parameter
    mock_sqlite3_step_current_row = 0; // Reset step counter
    return mock_sqlite3_reset_result;
}

// Mock implementation of sqlite3_bind_int
int mock_sqlite3_bind_int(void* stmt, int col, int value) {
    (void)stmt; // Suppress unused parameter
    (void)col;  // Suppress unused parameter
    (void)value; // Suppress unused parameter
    return mock_sqlite3_bind_int_result;
}

// Mock implementation of sqlite3_bind_double
int mock_sqlite3_bind_double(void* stmt, int col, double value) {
    (void)stmt; // Suppress unused parameter
    (void)col;  // Suppress unused parameter
    (void)value; // Suppress unused parameter
    return mock_sqlite3_bind_double_result;
}

// Mock implementation of sqlite3_bind_text
int mock_sqlite3_bind_text(void* stmt, int col, const char* text, int nBytes, void(*destructor)(void*)) {
    (void)stmt;      // Suppress unused parameter
    (void)col;       // Suppress unused parameter
    (void)text;      // Suppress unused parameter
    (void)nBytes;    // Suppress unused parameter
    (void)destructor; // Suppress unused parameter
    return mock_sqlite3_bind_text_result;
}

// Mock implementation of sqlite3_bind_null
int mock_sqlite3_bind_null(void* stmt, int col) {
    (void)stmt; // Suppress unused parameter
    (void)col;  // Suppress unused parameter
    return mock_sqlite3_bind_null_result;
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

void mock_libsqlite3_set_sqlite3_exec_callback_calls(int count) {
    mock_sqlite3_exec_callback_calls = count;
}

void mock_libsqlite3_set_sqlite3_errmsg_result(const char* errmsg) {
    mock_sqlite3_errmsg_result = errmsg;
}

void mock_libsqlite3_set_sqlite3_prepare_v2_result(int result) {
    mock_sqlite3_prepare_v2_result = result;
}

void mock_libsqlite3_set_sqlite3_prepare_v2_output_handle(void* handle) {
    mock_sqlite3_prepare_v2_output_handle = handle;
}

void mock_libsqlite3_set_sqlite3_finalize_result(int result) {
    mock_sqlite3_finalize_result = result;
}

void mock_libsqlite3_set_sqlite3_step_result(int result) {
    mock_sqlite3_step_result = result;
}

void mock_libsqlite3_set_sqlite3_step_row_count(int count) {
    mock_sqlite3_step_row_count = count;
    mock_sqlite3_step_current_row = 0;
}

void mock_libsqlite3_set_sqlite3_column_count_result(int count) {
    mock_sqlite3_column_count_result = count;
}

void mock_libsqlite3_set_sqlite3_column_name_result(const char* name) {
    mock_sqlite3_column_name_result = name;
}

void mock_libsqlite3_set_sqlite3_column_text_result(const unsigned char* text) {
    mock_sqlite3_column_text_result = text;
}

void mock_libsqlite3_set_sqlite3_column_type_result(int type) {
    mock_sqlite3_column_type_result = type;
}

void mock_libsqlite3_set_sqlite3_changes_result(int changes) {
    mock_sqlite3_changes_result = changes;
}

void mock_libsqlite3_set_sqlite3_reset_result(int result) {
    mock_sqlite3_reset_result = result;
}

void mock_libsqlite3_set_sqlite3_bind_int_result(int result) {
    mock_sqlite3_bind_int_result = result;
}

void mock_libsqlite3_set_sqlite3_bind_double_result(int result) {
    mock_sqlite3_bind_double_result = result;
}

void mock_libsqlite3_set_sqlite3_bind_text_result(int result) {
    mock_sqlite3_bind_text_result = result;
}

void mock_libsqlite3_set_sqlite3_bind_null_result(int result) {
    mock_sqlite3_bind_null_result = result;
}

void mock_libsqlite3_reset_all(void) {
    mock_sqlite3_open_result = 0; // SQLITE_OK
    mock_sqlite3_close_result = 0; // SQLITE_OK
    mock_sqlite3_exec_result = 0; // SQLITE_OK
    mock_sqlite3_exec_callback_calls = 0;
    mock_sqlite3_errmsg_result = NULL;
    mock_sqlite3_prepare_v2_result = 0; // SQLITE_OK
    mock_sqlite3_prepare_v2_output_handle = NULL;
    mock_sqlite3_finalize_result = 0; // SQLITE_OK
    mock_sqlite3_step_result = 101; // SQLITE_DONE
    mock_sqlite3_step_row_count = 0;
    mock_sqlite3_step_current_row = 0;
    mock_sqlite3_column_count_result = 0;
    mock_sqlite3_column_name_result = "column";
    mock_sqlite3_column_text_result = (const unsigned char*)"value";
    mock_sqlite3_column_type_result = 1; // SQLITE_INTEGER
    mock_sqlite3_changes_result = 0;
    mock_sqlite3_reset_result = 0; // SQLITE_OK
    mock_sqlite3_bind_int_result = 0; // SQLITE_OK
    mock_sqlite3_bind_double_result = 0; // SQLITE_OK
    mock_sqlite3_bind_text_result = 0; // SQLITE_OK
    mock_sqlite3_bind_null_result = 0; // SQLITE_OK
}