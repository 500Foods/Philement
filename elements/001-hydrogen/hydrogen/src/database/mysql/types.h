/*
 * MySQL Database Engine - Type Definitions Header
 *
 * Header file for MySQL engine type definitions.
 */

#ifndef DATABASE_ENGINE_MYSQL_TYPES_H
#define DATABASE_ENGINE_MYSQL_TYPES_H

#include <src/database/database.h>

// Function pointer types for libmysqlclient functions
typedef void* (*mysql_init_t)(void*);
typedef void* (*mysql_real_connect_t)(void*, const char*, const char*, const char*, const char*, unsigned int, const char*, unsigned long);
typedef int (*mysql_query_t)(void*, const char*);
typedef void* (*mysql_store_result_t)(void*);
typedef unsigned long long (*mysql_num_rows_t)(void*);
typedef unsigned int (*mysql_num_fields_t)(void*);
typedef void* (*mysql_fetch_row_t)(void*);
typedef void* (*mysql_fetch_fields_t)(void*);  // Actually MYSQL_FIELD* but we use void*
typedef void (*mysql_free_result_t)(void*);
typedef const char* (*mysql_error_t)(void*);
typedef void (*mysql_close_t)(void*);
typedef int (*mysql_options_t)(void*, int, const void*);
typedef int (*mysql_ping_t)(void*);
typedef int (*mysql_autocommit_t)(void*, int);
typedef int (*mysql_commit_t)(void*);
typedef int (*mysql_rollback_t)(void*);
typedef unsigned long long (*mysql_affected_rows_t)(void*);
typedef void* (*mysql_stmt_init_t)(void*);
typedef int (*mysql_stmt_prepare_t)(void*, const char*, unsigned long);
typedef int (*mysql_stmt_execute_t)(void*);
typedef int (*mysql_stmt_close_t)(void*);
typedef void* (*mysql_stmt_result_metadata_t)(void*);
typedef int (*mysql_stmt_fetch_t)(void*);
typedef int (*mysql_stmt_bind_param_t)(void*, void*);
typedef int (*mysql_stmt_bind_result_t)(void*, void*);
typedef const char* (*mysql_stmt_error_t)(void*);
typedef unsigned long long (*mysql_stmt_affected_rows_t)(void*);
typedef int (*mysql_stmt_store_result_t)(void*);
typedef int (*mysql_stmt_free_result_t)(void*);
typedef unsigned int (*mysql_stmt_field_count_t)(void*);

// MySQL function pointers (loaded dynamically)
extern mysql_init_t mysql_init_ptr;
extern mysql_real_connect_t mysql_real_connect_ptr;
extern mysql_query_t mysql_query_ptr;
extern mysql_store_result_t mysql_store_result_ptr;
extern mysql_num_rows_t mysql_num_rows_ptr;
extern mysql_num_fields_t mysql_num_fields_ptr;
extern mysql_fetch_row_t mysql_fetch_row_ptr;
extern mysql_fetch_fields_t mysql_fetch_fields_ptr;
extern mysql_free_result_t mysql_free_result_ptr;
extern mysql_error_t mysql_error_ptr;
extern mysql_close_t mysql_close_ptr;
extern mysql_options_t mysql_options_ptr;
extern mysql_ping_t mysql_ping_ptr;
extern mysql_autocommit_t mysql_autocommit_ptr;
extern mysql_commit_t mysql_commit_ptr;
extern mysql_rollback_t mysql_rollback_ptr;
extern mysql_affected_rows_t mysql_affected_rows_ptr;
extern mysql_stmt_init_t mysql_stmt_init_ptr;
extern mysql_stmt_prepare_t mysql_stmt_prepare_ptr;
extern mysql_stmt_execute_t mysql_stmt_execute_ptr;
extern mysql_stmt_close_t mysql_stmt_close_ptr;
extern mysql_stmt_result_metadata_t mysql_stmt_result_metadata_ptr;
extern mysql_stmt_fetch_t mysql_stmt_fetch_ptr;
extern mysql_stmt_bind_param_t mysql_stmt_bind_param_ptr;
extern mysql_stmt_bind_result_t mysql_stmt_bind_result_ptr;
extern mysql_stmt_error_t mysql_stmt_error_ptr;
extern mysql_stmt_affected_rows_t mysql_stmt_affected_rows_ptr;
extern mysql_stmt_store_result_t mysql_stmt_store_result_ptr;
extern mysql_stmt_free_result_t mysql_stmt_free_result_ptr;
extern mysql_stmt_field_count_t mysql_stmt_field_count_ptr;

// Library handle (declared in connection.c)

// Constants (defined since we can't include mysql.h)
#define MYSQL_OPT_RECONNECT 20

// Prepared statement cache structure
typedef struct PreparedStatementCache {
    char** names;
    size_t count;
    size_t capacity;
    pthread_mutex_t lock;
} PreparedStatementCache;

// MySQL-specific connection structure
typedef struct MySQLConnection {
    void* connection;  // MYSQL* loaded dynamically
    bool reconnect;
    PreparedStatementCache* prepared_statements;
} MySQLConnection;

#endif // DATABASE_ENGINE_MYSQL_TYPES_H