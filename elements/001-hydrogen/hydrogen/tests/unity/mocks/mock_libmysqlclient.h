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

// Mock function declarations - these will override the real ones when USE_MOCK_LIBMYSQLCLIENT is defined
#ifdef USE_MOCK_LIBMYSQLCLIENT

// Mock function prototypes
int mock_mysql_query(void* mysql, const char* query);
int mock_mysql_autocommit(void* mysql, int mode);
int mock_mysql_commit(void* mysql);
int mock_mysql_rollback(void* mysql);
const char* mock_mysql_error(void* mysql);

// Mock control functions for tests
void mock_libmysqlclient_set_mysql_query_result(int result);
void mock_libmysqlclient_set_mysql_autocommit_result(int result);
void mock_libmysqlclient_set_mysql_commit_result(int result);
void mock_libmysqlclient_set_mysql_rollback_result(int result);
void mock_libmysqlclient_set_mysql_error_result(const char* error);
void mock_libmysqlclient_reset_all(void);

#endif // USE_MOCK_LIBMYSQLCLIENT

#endif // MOCK_LIBMYSQLCLIENT_H