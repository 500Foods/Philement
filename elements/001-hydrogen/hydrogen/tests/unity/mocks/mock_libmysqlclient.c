/*
 * Mock libmysqlclient functions for unit testing
 *
 * This file provides mock implementations of libmysqlclient functions
 * to enable testing of MySQL database operations.
 */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Include the header to get type definitions
#include "mock_libmysqlclient.h"

// Include the header but undefine the macros to access real functions if needed
// (already included above)

// Undefine the macros in this file so we can declare the functions
#undef mysql_init_ptr
#undef mysql_real_connect_ptr
#undef mysql_options_ptr
#undef mysql_close_ptr
#undef mysql_ping_ptr
#undef mysql_store_result_ptr
#undef mysql_free_result_ptr
#undef mysql_query_ptr
#undef mysql_autocommit_ptr
#undef mysql_commit_ptr
#undef mysql_rollback_ptr
#undef mysql_error_ptr

// Function prototypes
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
void* mock_mysql_stmt_init(void* mysql);
int mock_mysql_stmt_prepare(void* stmt, const char* query, unsigned long length);
int mock_mysql_stmt_execute(void* stmt);
int mock_mysql_stmt_close(void* stmt);
void* mock_mysql_stmt_result_metadata(void* stmt);
int mock_mysql_stmt_store_result(void* stmt);
int mock_mysql_stmt_fetch(void* stmt);
int mock_mysql_stmt_bind_param(void* stmt, void* bind);
int mock_mysql_stmt_bind_result(void* stmt, void* bind);
const char* mock_mysql_stmt_error(void* stmt);
unsigned long long mock_mysql_stmt_affected_rows(void* stmt);
int mock_mysql_stmt_free_result(void* stmt);
unsigned int mock_mysql_stmt_field_count(void* stmt);
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
void mock_libmysqlclient_reset_all(void);
void mock_libmysqlclient_setup_fields(size_t num_fields, const char** column_names);
void mock_libmysqlclient_setup_result_data(size_t num_rows, size_t num_fields, const char** column_names, char*** row_data);

// Static variables to store mock state
static void* mock_mysql_init_result = (void*)0x12345678; // Non-NULL = success
static void* mock_mysql_real_connect_result = (void*)0x12345678; // Non-NULL = success
static int mock_mysql_options_result = 0; // 0 = success
static int mock_mysql_ping_result = 0; // 0 = success
static void* mock_mysql_store_result_result = (void*)0x87654321; // Non-NULL = success
static int mock_mysql_query_result = 0; // 0 = success
static int mock_mysql_autocommit_result = 0; // 0 = success
static int mock_mysql_commit_result = 0; // 0 = success
static int mock_mysql_rollback_result = 0; // 0 = success
static const char* mock_mysql_error_result = NULL;
static unsigned long long mock_mysql_affected_rows_result = 1;
static unsigned long long mock_mysql_num_rows_result = 2;
static unsigned int mock_mysql_num_fields_result = 3;
static void* mock_mysql_fetch_fields_result = (void*)0x12345678;
static char** mock_mysql_fetch_row_result = NULL;

// Prepared statement mock results
static void* mock_mysql_stmt_init_result = (void*)0x87654321; // Non-NULL = success
static int mock_mysql_stmt_prepare_result = 0; // 0 = success
static int mock_mysql_stmt_close_result = 0; // 0 = success

// Mock control to simulate unavailable functions
static bool mock_mysql_ping_available = true;
static bool mock_mysql_query_available = true;

// Mock data for results
static MockMYSQL_RES* mock_result_data = NULL;
static size_t mock_current_row = 0;

// Mock MYSQL_FIELD data
static MockMYSQL_FIELD mock_fields[10] = {0}; // Support up to 10 fields
static size_t mock_num_fields = 0;

// Mock implementation of mysql_init
void* mock_mysql_init(void* mysql) {
    (void)mysql; // Suppress unused parameter
    return mock_mysql_init_result;
}

// Mock implementation of mysql_real_connect
void* mock_mysql_real_connect(void* mysql, const char* host, const char* user, const char* passwd,
                             const char* db, unsigned int port, const char* unix_socket, unsigned long client_flag) {
    (void)mysql; // Suppress unused parameter
    (void)host; // Suppress unused parameter
    (void)user; // Suppress unused parameter
    (void)passwd; // Suppress unused parameter
    (void)db; // Suppress unused parameter
    (void)port; // Suppress unused parameter
    (void)unix_socket; // Suppress unused parameter
    (void)client_flag; // Suppress unused parameter
    return mock_mysql_real_connect_result;
}

// Mock implementation of mysql_options
int mock_mysql_options(void* mysql, int option, const void* arg) {
    (void)mysql; // Suppress unused parameter
    (void)option; // Suppress unused parameter
    (void)arg; // Suppress unused parameter
    return mock_mysql_options_result;
}

// Mock implementation of mysql_close
void mock_mysql_close(void* mysql) {
    (void)mysql; // Suppress unused parameter
    // Do nothing in mock
}

// Mock implementation of mysql_ping
int mock_mysql_ping(void* mysql) {
    (void)mysql; // Suppress unused parameter
    if (!mock_mysql_ping_available) {
        return 1; // Return failure to simulate unavailability
    }
    return mock_mysql_ping_result;
}

// Mock implementation of mysql_store_result
void* mock_mysql_store_result(void* mysql) {
    (void)mysql; // Suppress unused parameter
    return mock_mysql_store_result_result;
}

// Mock implementation of mysql_free_result
void mock_mysql_free_result(void* result) {
    (void)result; // Suppress unused parameter
    // Do nothing in mock
}

// Mock implementation of mysql_query
int mock_mysql_query(void* mysql, const char* query) {
    (void)mysql;  // Suppress unused parameter
    (void)query; // Suppress unused parameter
    if (!mock_mysql_query_available) {
        return 1; // Return failure to simulate unavailability
    }
    return mock_mysql_query_result;
}

// Mock implementation of mysql_autocommit
int mock_mysql_autocommit(void* mysql, int mode) {
    (void)mysql;  // Suppress unused parameter
    (void)mode;   // Suppress unused parameter
    return mock_mysql_autocommit_result;
}

// Mock implementation of mysql_commit
int mock_mysql_commit(void* mysql) {
    (void)mysql; // Suppress unused parameter
    return mock_mysql_commit_result;
}

// Mock implementation of mysql_rollback
int mock_mysql_rollback(void* mysql) {
    (void)mysql; // Suppress unused parameter
    return mock_mysql_rollback_result;
}

// Mock implementation of mysql_error
const char* mock_mysql_error(void* mysql) {
    (void)mysql; // Suppress unused parameter
    return mock_mysql_error_result ? mock_mysql_error_result : "";
}

// Mock implementation of mysql_affected_rows
unsigned long long mock_mysql_affected_rows(void* mysql) {
    (void)mysql; // Suppress unused parameter
    return mock_mysql_affected_rows_result;
}

// Mock implementation of mysql_num_rows
unsigned long long mock_mysql_num_rows(void* result) {
    (void)result; // Suppress unused parameter
    return mock_mysql_num_rows_result;
}

// Mock implementation of mysql_num_fields
unsigned int mock_mysql_num_fields(void* result) {
    (void)result; // Suppress unused parameter
    return mock_mysql_num_fields_result;
}

// Mock implementation of mysql_fetch_fields
void* mock_mysql_fetch_fields(void* result) {
    (void)result; // Suppress unused parameter
    return mock_fields; // Return the mock fields array
}

// Mock implementation of mysql_fetch_row
void* mock_mysql_fetch_row(void* result) {
    (void)result; // Suppress unused parameter
    if (mock_result_data && mock_current_row < mock_result_data->num_rows) {
        return mock_result_data->rows[mock_current_row++];
    }
    return NULL;
}

// Prepared statement mocks
void* mock_mysql_stmt_init(void* mysql) {
    (void)mysql; // Suppress unused parameter
    return mock_mysql_stmt_init_result;
}

int mock_mysql_stmt_prepare(void* stmt, const char* query, unsigned long length) {
    (void)stmt; // Suppress unused parameter
    (void)query; // Suppress unused parameter
    (void)length; // Suppress unused parameter
    return mock_mysql_stmt_prepare_result;
}

int mock_mysql_stmt_execute(void* stmt) {
    (void)stmt; // Suppress unused parameter
    return 0; // Success
}

int mock_mysql_stmt_close(void* stmt) {
    (void)stmt; // Suppress unused parameter
    return mock_mysql_stmt_close_result;
}

void* mock_mysql_stmt_result_metadata(void* stmt) {
    (void)stmt; // Suppress unused parameter
    return mock_mysql_store_result_result;
}

int mock_mysql_stmt_store_result(void* stmt) {
    (void)stmt; // Suppress unused parameter
    return 0; // Success
}

int mock_mysql_stmt_fetch(void* stmt) {
    (void)stmt; // Suppress unused parameter
    if (mock_result_data && mock_current_row < mock_result_data->num_rows) {
        mock_current_row++;
        return 0; // Success
    }
    return 1; // No more rows
}

int mock_mysql_stmt_bind_param(void* stmt, void* bind) {
    (void)stmt; // Suppress unused parameter
    (void)bind; // Suppress unused parameter
    return 0; // Success
}

int mock_mysql_stmt_bind_result(void* stmt, void* bind) {
    (void)stmt; // Suppress unused parameter
    (void)bind; // Suppress unused parameter
    return 0; // Success
}

const char* mock_mysql_stmt_error(void* stmt) {
    (void)stmt; // Suppress unused parameter
    return mock_mysql_error_result ? mock_mysql_error_result : "";
}

unsigned long long mock_mysql_stmt_affected_rows(void* stmt) {
    (void)stmt; // Suppress unused parameter
    return mock_mysql_affected_rows_result;
}

int mock_mysql_stmt_free_result(void* stmt) {
    (void)stmt; // Suppress unused parameter
    return 0; // Success
}

unsigned int mock_mysql_stmt_field_count(void* stmt) {
    (void)stmt; // Suppress unused parameter
    return mock_mysql_num_fields_result;
}

// Mock control functions
void mock_libmysqlclient_set_mysql_init_result(void* result) {
    mock_mysql_init_result = result;
}

void mock_libmysqlclient_set_mysql_real_connect_result(void* result) {
    mock_mysql_real_connect_result = result;
}

void mock_libmysqlclient_set_mysql_options_result(int result) {
    mock_mysql_options_result = result;
}

void mock_libmysqlclient_set_mysql_ping_result(int result) {
    mock_mysql_ping_result = result;
}

void mock_libmysqlclient_set_mysql_store_result_result(void* result) {
    mock_mysql_store_result_result = result;
}

void mock_libmysqlclient_set_mysql_query_result(int result) {
    mock_mysql_query_result = result;
}

void mock_libmysqlclient_set_mysql_autocommit_result(int result) {
    mock_mysql_autocommit_result = result;
}

void mock_libmysqlclient_set_mysql_commit_result(int result) {
    mock_mysql_commit_result = result;
}

void mock_libmysqlclient_set_mysql_rollback_result(int result) {
    mock_mysql_rollback_result = result;
}

void mock_libmysqlclient_set_mysql_error_result(const char* error) {
    mock_mysql_error_result = error;
}

void mock_libmysqlclient_set_mysql_ping_available(bool available) {
    mock_mysql_ping_available = available;
}

void mock_libmysqlclient_set_mysql_query_available(bool available) {
    mock_mysql_query_available = available;
}

void mock_libmysqlclient_set_mysql_affected_rows_result(unsigned long long result) {
    mock_mysql_affected_rows_result = result;
}

void mock_libmysqlclient_set_mysql_num_rows_result(unsigned long long result) {
    mock_mysql_num_rows_result = result;
}

void mock_libmysqlclient_set_mysql_num_fields_result(unsigned int result) {
    mock_mysql_num_fields_result = result;
}

void mock_libmysqlclient_set_mysql_fetch_fields_result(void* result) {
    mock_mysql_fetch_fields_result = result;
}

void mock_libmysqlclient_set_mysql_fetch_row_result(char** result) {
    mock_mysql_fetch_row_result = result;
}

void mock_libmysqlclient_set_mysql_stmt_init_result(void* result) {
    mock_mysql_stmt_init_result = result;
}

void mock_libmysqlclient_set_mysql_stmt_prepare_result(int result) {
    mock_mysql_stmt_prepare_result = result;
}

void mock_libmysqlclient_set_mysql_stmt_close_result(int result) {
    mock_mysql_stmt_close_result = result;
}

void mock_libmysqlclient_reset_all(void) {
    mock_mysql_init_result = (void*)0x12345678;
    mock_mysql_real_connect_result = (void*)0x12345678;
    mock_mysql_options_result = 0;
    mock_mysql_ping_result = 0;
    mock_mysql_store_result_result = (void*)0x87654321;
    mock_mysql_query_result = 0;
    mock_mysql_autocommit_result = 0;
    mock_mysql_commit_result = 0;
    mock_mysql_rollback_result = 0;
    mock_mysql_error_result = NULL;
    mock_mysql_affected_rows_result = 1;
    mock_mysql_num_rows_result = 2;
    mock_mysql_num_fields_result = 3;
    mock_mysql_fetch_fields_result = (void*)0x12345678;
    mock_mysql_fetch_row_result = NULL;
    mock_mysql_ping_available = true;
    mock_mysql_query_available = true;

    // Reset prepared statement mocks
    mock_mysql_stmt_init_result = (void*)0x87654321;
    mock_mysql_stmt_prepare_result = 0;
    mock_mysql_stmt_close_result = 0;

    // Clear mock fields
    memset(mock_fields, 0, sizeof(mock_fields));
    mock_num_fields = 0;

    // Clean up mock data
    if (mock_result_data) {
        // Free fields
        if (mock_result_data->fields) {
            for (size_t i = 0; i < mock_result_data->num_fields; i++) {
                free(mock_result_data->fields[i].name);
            }
            free(mock_result_data->fields);
        }
        // Free rows
        if (mock_result_data->rows) {
            for (size_t i = 0; i < mock_result_data->num_rows; i++) {
                if (mock_result_data->rows[i]) {
                    for (size_t j = 0; j < mock_result_data->num_fields; j++) {
                        free(mock_result_data->rows[i][j]);
                    }
                    free(mock_result_data->rows[i]);
                }
            }
            free(mock_result_data->rows);
        }
        free(mock_result_data);
        mock_result_data = NULL;
    }
    mock_current_row = 0;
}

void mock_libmysqlclient_setup_fields(size_t num_fields, const char** column_names) {
    mock_num_fields = num_fields < 10 ? num_fields : 10; // Limit to array size

    for (size_t i = 0; i < mock_num_fields; i++) {
        if (column_names && column_names[i]) {
            mock_fields[i].name = (char*)column_names[i]; // Note: not copying, just pointing
        } else {
            mock_fields[i].name = NULL;
        }
        // Default to VARCHAR/STRING type (253) for string escaping tests
        mock_fields[i].type = 253;
    }
}

void mock_libmysqlclient_set_field_type(size_t field_index, unsigned int field_type) {
    if (field_index < 10) {
        mock_fields[field_index].type = field_type;
    }
}

void mock_libmysqlclient_setup_result_data(size_t num_rows, size_t num_fields, const char** column_names, char*** row_data) {
    // Clean up existing data
    if (mock_result_data) {
        mock_libmysqlclient_reset_all();
    }

    mock_result_data = calloc(1, sizeof(MockMYSQL_RES));
    if (!mock_result_data) return;

    mock_result_data->num_rows = num_rows;
    mock_result_data->num_fields = num_fields;
    mock_result_data->current_row = 0;

    // Set up fields
    if (num_fields > 0 && column_names) {
        mock_result_data->fields = calloc(num_fields, sizeof(MockMYSQL_FIELD));
        if (mock_result_data->fields) {
            for (size_t i = 0; i < num_fields; i++) {
                mock_result_data->fields[i].name = column_names[i] ? strdup(column_names[i]) : NULL;
            }
        }
    }

    // Set up rows
    if (num_rows > 0 && num_fields > 0 && row_data) {
        mock_result_data->rows = calloc(num_rows, sizeof(char**));
        if (mock_result_data->rows) {
            for (size_t i = 0; i < num_rows; i++) {
                if (row_data[i]) {
                    mock_result_data->rows[i] = calloc(num_fields, sizeof(char*));
                    if (mock_result_data->rows[i]) {
                        for (size_t j = 0; j < num_fields; j++) {
                            mock_result_data->rows[i][j] = row_data[i][j] ? strdup(row_data[i][j]) : NULL;
                        }
                    }
                }
            }
        }
    }
}