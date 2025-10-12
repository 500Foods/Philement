/*
 * SQLite Database Engine - Type Definitions Header
 *
 * Header file for SQLite engine type definitions.
 */

#ifndef SQLITE_TYPES_H
#define SQLITE_TYPES_H

#include "../database.h"

// Function pointer types for libsqlite3 functions
typedef int (*sqlite3_open_t)(const char*, void**);
typedef int (*sqlite3_close_t)(void*);
typedef int (*sqlite3_exec_t)(void*, const char*, int (*)(void*,int,char**,char**), void*, char**);
typedef int (*sqlite3_prepare_v2_t)(void*, const char*, int, void**, const char**);
typedef int (*sqlite3_step_t)(void*);
typedef int (*sqlite3_finalize_t)(void*);
typedef int (*sqlite3_column_count_t)(void*);
typedef const char* (*sqlite3_column_name_t)(void*, int);
typedef const unsigned char* (*sqlite3_column_text_t)(void*, int);
typedef int (*sqlite3_column_type_t)(void*, int);
typedef int (*sqlite3_changes_t)(void*);
typedef int (*sqlite3_reset_t)(void*);
typedef int (*sqlite3_clear_bindings_t)(void*);
typedef int (*sqlite3_bind_text_t)(void*, int, const char*, int, void (*)(void*));
typedef int (*sqlite3_bind_int_t)(void*, int, int);
typedef int (*sqlite3_bind_double_t)(void*, int, double);
typedef int (*sqlite3_bind_null_t)(void*, int);
typedef const char* (*sqlite3_errmsg_t)(void*);
typedef int (*sqlite3_extended_result_codes_t)(void*, int);
typedef void (*sqlite3_free_t)(void*);

// SQLite function pointers (loaded dynamically)
extern sqlite3_open_t sqlite3_open_ptr;
extern sqlite3_close_t sqlite3_close_ptr;
extern sqlite3_exec_t sqlite3_exec_ptr;
extern sqlite3_prepare_v2_t sqlite3_prepare_v2_ptr;
extern sqlite3_step_t sqlite3_step_ptr;
extern sqlite3_finalize_t sqlite3_finalize_ptr;
extern sqlite3_column_count_t sqlite3_column_count_ptr;
extern sqlite3_column_name_t sqlite3_column_name_ptr;
extern sqlite3_column_text_t sqlite3_column_text_ptr;
extern sqlite3_column_type_t sqlite3_column_type_ptr;
extern sqlite3_changes_t sqlite3_changes_ptr;
extern sqlite3_reset_t sqlite3_reset_ptr;
extern sqlite3_clear_bindings_t sqlite3_clear_bindings_ptr;
extern sqlite3_bind_text_t sqlite3_bind_text_ptr;
extern sqlite3_bind_int_t sqlite3_bind_int_ptr;
extern sqlite3_bind_double_t sqlite3_bind_double_ptr;
extern sqlite3_bind_null_t sqlite3_bind_null_ptr;
extern sqlite3_errmsg_t sqlite3_errmsg_ptr;
extern sqlite3_extended_result_codes_t sqlite3_extended_result_codes_ptr;

// Library handle (declared in connection.c)

// Constants (defined since we can't include sqlite3.h)
#define SQLITE_OK 0
#define SQLITE_ROW 100
#define SQLITE_DONE 101
#define SQLITE_NULL 5

// Prepared statement cache structure
typedef struct PreparedStatementCache {
    char** names;
    size_t count;
    size_t capacity;
    pthread_mutex_t lock;
} PreparedStatementCache;

// SQLite-specific connection structure
typedef struct SQLiteConnection {
    void* db;  // sqlite3* loaded dynamically
    char* db_path;
    PreparedStatementCache* prepared_statements;
} SQLiteConnection;

#endif // SQLITE_TYPES_H