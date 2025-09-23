/*
 * Mock libpq functions for unit testing
 *
 * This file provides mock implementations of libpq functions
 * to enable testing of PostgreSQL database operations.
 */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

// Constants
#define PGRES_COMMAND_OK 1
#define PGRES_FATAL_ERROR 7

// Include the header but undefine the macros to access real functions if needed
#include "mock_libpq.h"

// Undefine the macros in this file so we can declare the functions
#undef PQexec_ptr
#undef PQresultStatus_ptr
#undef PQclear_ptr
#undef check_timeout_expired

// Function prototypes
void* mock_PQexec(void* conn, const char* query);
int mock_PQresultStatus(void* res);
void mock_PQclear(void* res);
bool mock_check_timeout_expired(time_t start_time, int timeout_seconds);
void mock_libpq_set_PQexec_result(void* result);
void mock_libpq_set_PQresultStatus_result(int status);
void mock_libpq_set_check_timeout_expired_result(bool result);
void mock_libpq_set_check_timeout_expired_use_mock(bool use_mock);
void mock_libpq_reset_all(void);

// Static variables to store mock state
static void* mock_PQexec_result = NULL;
static int mock_PQresultStatus_result = 1; // PGRES_COMMAND_OK by default
static bool mock_check_timeout_expired_result = false;
static bool mock_check_timeout_expired_use_mock = false;

// Mock implementation of PQexec
void* mock_PQexec(void* conn, const char* query) {
    (void)conn;  // Suppress unused parameter
    (void)query; // Suppress unused parameter
    return mock_PQexec_result;
}

// Mock implementation of PQresultStatus
// cppcheck-suppress constParameterPointer
int mock_PQresultStatus(void* res) {
    if (res == NULL) {
        return PGRES_FATAL_ERROR; // If res is NULL, return error
    }
    return mock_PQresultStatus_result;
}

// Mock implementation of PQclear
void mock_PQclear(void* res) {
    (void)res; // Suppress unused parameter
    // Do nothing for mock
}

// Mock implementation of check_timeout_expired
bool mock_check_timeout_expired(time_t start_time, int timeout_seconds) {
    if (mock_check_timeout_expired_use_mock) {
        return mock_check_timeout_expired_result;
    } else {
        // Use real logic
        return (time(NULL) - start_time) >= timeout_seconds;
    }
}

// Mock control functions
void mock_libpq_set_PQexec_result(void* result) {
    mock_PQexec_result = result;
}

void mock_libpq_set_PQresultStatus_result(int status) {
    mock_PQresultStatus_result = status;
}

void mock_libpq_set_check_timeout_expired_result(bool result) {
    mock_check_timeout_expired_result = result;
}

void mock_libpq_set_check_timeout_expired_use_mock(bool use_mock) {
    mock_check_timeout_expired_use_mock = use_mock;
}

void mock_libpq_reset_all(void) {
    mock_PQexec_result = NULL;
    mock_PQresultStatus_result = 1; // PGRES_COMMAND_OK
    mock_check_timeout_expired_result = false;
    mock_check_timeout_expired_use_mock = false;
}