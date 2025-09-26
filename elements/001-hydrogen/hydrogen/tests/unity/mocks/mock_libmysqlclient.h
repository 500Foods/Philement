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

// Mock control functions for tests
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

#endif // USE_MOCK_LIBMYSQLCLIENT

#endif // MOCK_LIBMYSQLCLIENT_H