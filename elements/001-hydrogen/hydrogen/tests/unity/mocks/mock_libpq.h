/*
 * Mock libpq functions for unit testing
 *
 * This file provides mock implementations of libpq functions
 * to enable testing of PostgreSQL database operations.
 */

#ifndef MOCK_LIBPQ_H
#define MOCK_LIBPQ_H

#include <stddef.h>
#include <stdbool.h>
#include <time.h>

// Mock function declarations - these will override the real ones when USE_MOCK_LIBPQ is defined
#ifdef USE_MOCK_LIBPQ

// Mock function prototypes
void* mock_PQexec(void* conn, const char* query);
int mock_PQresultStatus(void* res);
void mock_PQclear(void* res);
bool mock_check_timeout_expired(time_t start_time, int timeout_seconds);

// Mock control functions for tests
void mock_libpq_set_PQexec_result(void* result);
void mock_libpq_set_PQresultStatus_result(int status);
void mock_libpq_set_check_timeout_expired_result(bool result);
void mock_libpq_set_check_timeout_expired_use_mock(bool use_mock);
void mock_libpq_reset_all(void);

#endif // USE_MOCK_LIBPQ

#endif // MOCK_LIBPQ_H