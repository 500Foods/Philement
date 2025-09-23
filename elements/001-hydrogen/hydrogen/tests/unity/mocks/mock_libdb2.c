/*
 * Mock libdb2 functions for unit testing
 *
 * This file provides mock implementations of libdb2 functions
 * to enable testing of DB2 database operations.
 */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Include the header but undefine the macros to access real functions if needed
#include "mock_libdb2.h"

// Undefine the macros in this file so we can declare the functions
#undef SQLEndTran_ptr

// Function prototypes
int mock_SQLEndTran(int handleType, void* handle, int completionType);
void mock_libdb2_set_SQLEndTran_result(int result);
void mock_libdb2_reset_all(void);

// Static variables to store mock state
static int mock_SQLEndTran_result = 0; // 0 = SQL_SUCCESS

// Mock implementation of SQLEndTran
int mock_SQLEndTran(int handleType, void* handle, int completionType) {
    (void)handleType;  // Suppress unused parameter
    (void)handle;      // Suppress unused parameter
    (void)completionType; // Suppress unused parameter
    return mock_SQLEndTran_result;
}

// Mock control functions
void mock_libdb2_set_SQLEndTran_result(int result) {
    mock_SQLEndTran_result = result;
}

void mock_libdb2_reset_all(void) {
    mock_SQLEndTran_result = 0; // SQL_SUCCESS
}