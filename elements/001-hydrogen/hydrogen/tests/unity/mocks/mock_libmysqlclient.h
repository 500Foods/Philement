/*
 * Mock libmysqlclient functions for unit testing
 *
 * This file provides mock implementations of libmysqlclient functions
 * to enable testing of MySQL database operations.
 */

#ifndef MOCK_LIBMYSQLCLIENT_H
#define MOCK_LIBMYSQLCLIENT_H

#include <stddef.h>
#include <stdbool.h>

// Mock data structures (always available for internal use)
typedef struct {
    char* name;
    char* org_name;
    char* table;
    char* org_table;
    char* db;
    char* catalog;
    char* def;
    unsigned long length;
    unsigned long max_length;
    unsigned int name_length;
    unsigned int org_name_length;
    unsigned int table_length;
    unsigned int org_table_length;
    unsigned int db_length;
    unsigned int catalog_length;
    unsigned int def_length;
    unsigned int flags;
    unsigned int decimals;
    unsigned int charsetnr;
    unsigned int type;
    void* extension;
} MockMYSQL_FIELD;

typedef struct {
    size_t num_rows;
    size_t num_fields;
    MockMYSQL_FIELD* fields;
    char*** rows;
    size_t current_row;
} MockMYSQL_RES;

// Mock function declarations - these will override the real ones when USE_MOCK_LIBMYSQLCLIENT is defined
// Control function declarations (always available)
void mock_libmysqlclient_set_mysql_init_result(void* result);
void mock_libmysqlclient_set_mysql_real_connect_result(void* result);
void mock_libmysqlclient_set_mysql_options_result(int result);
void mock_libmysqlclient_set_mysql_ping_result(int result);
void mock_libmysqlclient_set_mysql_store_result_result(void* result);
void mock_libmysqlclient_set_mysql_query_result(int result);
void mock_libmysqlclient_set_mysql_autocommit_result(int result);
void mock_libmysqlclient_set_mysql_commit_result(int result);
void mock_libmysqlclient_set_mysql_rollback_result(int result);
void mock_libmysqlclient_set_mysql_error_result(const char* error);
void mock_libmysqlclient_set_mysql_ping_available(bool available);
void mock_libmysqlclient_set_mysql_query_available(bool available);
void mock_libmysqlclient_set_mysql_affected_rows_result(unsigned long long result);
void mock_libmysqlclient_set_mysql_num_rows_result(unsigned long long result);
void mock_libmysqlclient_set_mysql_num_fields_result(unsigned int result);
void mock_libmysqlclient_set_mysql_fetch_fields_result(void* result);
void mock_libmysqlclient_set_mysql_fetch_row_result(char** result);
void mock_libmysqlclient_set_mysql_stmt_init_result(void* result);
void mock_libmysqlclient_set_mysql_stmt_prepare_result(int result);
void mock_libmysqlclient_set_mysql_stmt_close_result(int result);
void mock_libmysqlclient_reset_all(void);
void mock_libmysqlclient_setup_fields(size_t num_fields, const char** column_names);
void mock_libmysqlclient_setup_result_data(size_t num_rows, size_t num_fields, const char** column_names, char*** row_data);
void mock_libmysqlclient_set_field_type(size_t field_index, unsigned int field_type);

#ifdef USE_MOCK_LIBMYSQLCLIENT

// Mock function prototypes
void* mock_mysql_init(void* mysql);
void* mock_mysql_real_connect(void* mysql, const char* host, const char* user, const char* passwd,
                              const char* db, unsigned int port, const char* unix_socket, unsigned long client_flag);
int mock_mysql_options(void* mysql, int option, const void* arg);
void mock_mysql_close(void* mysql);
int mock_mysql_ping(void* mysql);
void* mock_mysql_store_result(void* mysql);
void mock_mysql_free_result(void* result);
int mock_mysql_query(void* mysql, const char* query);
int mock_mysql_autocommit(void* mysql, int mode);
int mock_mysql_commit(void* mysql);
int mock_mysql_rollback(void* mysql);
const char* mock_mysql_error(void* mysql);
unsigned long long mock_mysql_affected_rows(void* mysql);
unsigned long long mock_mysql_num_rows(void* result);
unsigned int mock_mysql_num_fields(void* result);
void* mock_mysql_fetch_fields(void* result);
void* mock_mysql_fetch_row(void* result);

// Prepared statement mocks
void* mock_mysql_stmt_init(void* mysql);
int mock_mysql_stmt_prepare(void* stmt, const char* query, unsigned long length);
int mock_mysql_stmt_execute(void* stmt);
int mock_mysql_stmt_close(void* stmt);
void* mock_mysql_stmt_result_metadata(void* stmt);
int mock_mysql_stmt_store_result(void* stmt);
int mock_mysql_stmt_fetch(void* stmt);
int mock_mysql_stmt_bind_result(void* stmt, void* bind);
const char* mock_mysql_stmt_error(void* stmt);
unsigned long long mock_mysql_stmt_affected_rows(void* stmt);
int mock_mysql_stmt_free_result(void* stmt);
unsigned int mock_mysql_stmt_field_count(void* stmt);


#endif // USE_MOCK_LIBMYSQLCLIENT

#endif // MOCK_LIBMYSQLCLIENT_H