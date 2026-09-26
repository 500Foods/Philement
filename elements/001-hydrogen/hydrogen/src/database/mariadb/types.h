/*
 * MariaDB Database Engine Type Definitions Header
 *
 * Header file for MariaDB engine type definitions.
 */

#ifndef DATABASE_ENGINE_MARIADB_TYPES_H
#define DATABASE_ENGINE_MARIADB_TYPES_H

#include <src/database/database.h>

// Function pointer types for libmariadb functions
typedef void* (*mariadb_init_t)(void*);
typedef void* (*mariadb_real_connect_t)(void*, const char*, const char*, const char*, const char*, unsigned int, const char*, unsigned long);
typedef int (*mariadb_query_t)(void*, const char*);
typedef void* (*mariadb_store_result_t)(void*);
typedef unsigned long long (*mariadb_num_rows_t)(void*);
typedef unsigned int (*mariadb_num_fields_t)(void*);
typedef void* (*mariadb_fetch_row_t)(void*);
typedef void* (*mariadb_fetch_fields_t)(void*);  // Actually MYSQL_FIELD* but we use void*
typedef void (*mariadb_free_result_t)(void*);
typedef const char* (*mariadb_error_t)(void*);
typedef void (*mariadb_close_t)(void*);
typedef int (*mariadb_options_t)(void*, int, const void*);
typedef int (*mariadb_ping_t)(void*);
typedef int (*mariadb_autocommit_t)(void*, int);
typedef int (*mariadb_commit_t)(void*);
typedef int (*mariadb_rollback_t)(void*);
typedef unsigned long long (*mariadb_affected_rows_t)(void*);
typedef void* (*mariadb_stmt_init_t)(void*);
typedef int (*mariadb_stmt_prepare_t)(void*, const char*, unsigned long);
typedef int (*mariadb_stmt_execute_t)(void*);
typedef int (*mariadb_stmt_close_t)(void*);
typedef void* (*mariadb_stmt_result_metadata_t)(void*);
typedef int (*mariadb_stmt_fetch_t)(void*);
typedef int (*mariadb_stmt_bind_param_t)(void*, void*);
typedef int (*mariadb_stmt_bind_result_t)(void*, void*);
typedef const char* (*mariadb_stmt_error_t)(void*);
typedef unsigned long long (*mariadb_stmt_affected_rows_t)(void*);
typedef int (*mariadb_stmt_store_result_t)(void*);
typedef int (*mariadb_stmt_free_result_t)(void*);
typedef unsigned int (*mariadb_stmt_field_count_t)(void*);
typedef int (*mariadb_kill_t)(void*, unsigned long);
typedef unsigned long (*mariadb_thread_id_t)(void*);

// MariaDB function pointers (loaded dynamically)
extern mariadb_init_t mariadb_init_ptr;
extern mariadb_real_connect_t mariadb_real_connect_ptr;
extern mariadb_query_t mariadb_query_ptr;
extern mariadb_store_result_t mariadb_store_result_ptr;
extern mariadb_num_rows_t mariadb_num_rows_ptr;
extern mariadb_num_fields_t mariadb_num_fields_ptr;
extern mariadb_fetch_row_t mariadb_fetch_row_ptr;
extern mariadb_fetch_fields_t mariadb_fetch_fields_ptr;
extern mariadb_free_result_t mariadb_free_result_ptr;
extern mariadb_error_t mariadb_error_ptr;
extern mariadb_close_t mariadb_close_ptr;
extern mariadb_options_t mariadb_options_ptr;
extern mariadb_ping_t mariadb_ping_ptr;
extern mariadb_autocommit_t mariadb_autocommit_ptr;
extern mariadb_commit_t mariadb_commit_ptr;
extern mariadb_rollback_t mariadb_rollback_ptr;
extern mariadb_affected_rows_t mariadb_affected_rows_ptr;
extern mariadb_stmt_init_t mariadb_stmt_init_ptr;
extern mariadb_stmt_prepare_t mariadb_stmt_prepare_ptr;
extern mariadb_stmt_execute_t mariadb_stmt_execute_ptr;
extern mariadb_stmt_close_t mariadb_stmt_close_ptr;
extern mariadb_stmt_result_metadata_t mariadb_stmt_result_metadata_ptr;
extern mariadb_stmt_fetch_t mariadb_stmt_fetch_ptr;
extern mariadb_stmt_bind_param_t mariadb_stmt_bind_param_ptr;
extern mariadb_stmt_bind_result_t mariadb_stmt_bind_result_ptr;
extern mariadb_stmt_error_t mariadb_stmt_error_ptr;
extern mariadb_stmt_affected_rows_t mariadb_stmt_affected_rows_ptr;
extern mariadb_stmt_store_result_t mariadb_stmt_store_result_ptr;
extern mariadb_stmt_free_result_t mariadb_stmt_free_result_ptr;
extern mariadb_stmt_field_count_t mariadb_stmt_field_count_ptr;
extern mariadb_kill_t mariadb_kill_ptr;
extern mariadb_thread_id_t mariadb_thread_id_ptr;

// Library handle (declared in connection.c)

// MYSQL_OPT_RECONNECT comes from <mysql.h> (enum enum_mysql_option) in
// translation units that include it (query.c). For other TUs (connection.c,
// etc.) that don't pull in mysql.h, provide a numeric fallback that matches
// the historical MariaDB Connector/C value. The enum value and the #define
// agree by convention; do not change one without the other.
#ifndef MYSQL_OPT_RECONNECT
#define MYSQL_OPT_RECONNECT 20
#endif

// Prepared statement cache structure
typedef struct PreparedStatementCache {
    char** names;
    size_t count;
    size_t capacity;
    pthread_mutex_t lock;
} PreparedStatementCache;

// MariaDB-specific connection structure
typedef struct MariadbConnection {
    void* connection;  // MYSQL* loaded dynamically
    bool reconnect;
    PreparedStatementCache* prepared_statements;
} MariadbConnection;

#endif // DATABASE_ENGINE_MARIADB_TYPES_H
