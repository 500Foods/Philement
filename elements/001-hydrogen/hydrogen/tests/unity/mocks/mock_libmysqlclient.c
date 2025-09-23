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
#undef mysql_query_ptr
#undef mysql_autocommit_ptr
#undef mysql_commit_ptr
#undef mysql_rollback_ptr
#undef mysql_error_ptr

// Function prototypes
int mock_mysql_query(void* mysql, const char* query);
int mock_mysql_autocommit(void* mysql, int mode);
int mock_mysql_commit(void* mysql);
int mock_mysql_rollback(void* mysql);
const char* mock_mysql_error(void* mysql);
void mock_libmysqlclient_set_mysql_query_result(int result);
void mock_libmysqlclient_set_mysql_autocommit_result(int result);
void mock_libmysqlclient_set_mysql_commit_result(int result);
void mock_libmysqlclient_set_mysql_rollback_result(int result);
void mock_libmysqlclient_set_mysql_error_result(const char* error);
void mock_libmysqlclient_reset_all(void);

// Static variables to store mock state
static int mock_mysql_query_result = 0; // 0 = success
static int mock_mysql_autocommit_result = 0; // 0 = success
static int mock_mysql_commit_result = 0; // 0 = success
static int mock_mysql_rollback_result = 0; // 0 = success
static const char* mock_mysql_error_result = NULL;

// Mock implementation of mysql_query
int mock_mysql_query(void* mysql, const char* query) {
    (void)mysql;  // Suppress unused parameter
    (void)query; // Suppress unused parameter
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

void mock_libmysqlclient_reset_all(void) {
    mock_mysql_query_result = 0;
    mock_mysql_autocommit_result = 0;
    mock_mysql_commit_result = 0;
    mock_mysql_rollback_result = 0;
    mock_mysql_error_result = NULL;
}