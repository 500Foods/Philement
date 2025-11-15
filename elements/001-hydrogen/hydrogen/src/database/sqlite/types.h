/*
 * SQLite Database Engine - Type Definitions Header
 *
 * Header file for SQLite engine type definitions.
 */

#ifndef SQLITE_TYPES_H
#define SQLITE_TYPES_H

#include <src/database/database.h>

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
typedef int (*sqlite3_load_extension_t)(void*, const char*, const char*, char**);
typedef int (*sqlite3_db_config_t)(void*, int, ...);
typedef void (*sqlite3_free_t)(void*);

// SQLite function pointers (loaded dynamically)
// Defined in connection.c

// Library handle (declared in connection.c)

// Constants (defined since we can't include sqlite3.h)
#define SQLITE_OK 0
#define SQLITE_ROW 100
#define SQLITE_DONE 101
#define SQLITE_NULL 5
#define SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION 1005

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
    void* crypto_handle;  // Handle for crypto.so library
} SQLiteConnection;

#endif // SQLITE_TYPES_H