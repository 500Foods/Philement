/*
 * Mock libmysqlclient functions for unit testing
 *
 * This file provides mock implementations of libmysqlclient functions
 * to enable testing of MySQL database operations.
 */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Include the header but undefine the macros to access real functions if needed
#include "mock_libmysqlclient.h"

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
void mock_libmysqlclient_reset_all(void);

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

// Mock control to simulate unavailable functions
static bool mock_mysql_ping_available = true;
static bool mock_mysql_query_available = true;

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
    mock_mysql_ping_available = true;
    mock_mysql_query_available = true;
}