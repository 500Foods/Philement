/*
 * Mock libdb2 functions for unit testing
 *
 * This file provides mock implementations of libdb2 functions
 * to enable testing of DB2 database operations.
 */

#ifndef MOCK_LIBDB2_H
#define MOCK_LIBDB2_H

#include <stddef.h>
#include <stdbool.h>

// Mock function declarations - these will override the real ones when USE_MOCK_LIBDB2 is defined
#ifdef USE_MOCK_LIBDB2

// Mock function prototypes
int mock_SQLEndTran(int handleType, void* handle, int completionType);

// Mock control functions for tests
void mock_libdb2_set_SQLEndTran_result(int result);
void mock_libdb2_reset_all(void);

#endif // USE_MOCK_LIBDB2

#endif // MOCK_LIBDB2_H