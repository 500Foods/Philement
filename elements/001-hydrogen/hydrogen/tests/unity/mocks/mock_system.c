/*
 * Mock system functions for unit testing
 *
 * This file provides mock implementations of system functions
 * like malloc, gethostname, etc. to enable testing of error conditions.
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Include the header but undefine the macros to access real functions
#include "mock_system.h"

// Undefine the macros in this file so we can call the real functions
#undef malloc
#undef free
#undef strdup
#undef gethostname

// Function prototypes
void *mock_malloc(size_t size);
void mock_free(void *ptr);
char *mock_strdup(const char *s);
int mock_gethostname(char *name, size_t len);
void mock_system_set_malloc_failure(int should_fail);
void mock_system_set_gethostname_failure(int should_fail);
void mock_system_set_gethostname_result(const char *result);
void mock_system_reset_all(void);

// Static variables to store mock state
static int mock_malloc_should_fail = 0;
static int mock_gethostname_should_fail = 0;
static const char *mock_gethostname_result = NULL;

// Mock implementation of malloc
void *mock_malloc(size_t size) {
    if (mock_malloc_should_fail) {
        return NULL;
    }
    // Now we can call the real malloc since we undefined the macro
    return malloc(size);
}

// Mock implementation of free
void mock_free(void *ptr) {
    // Call the real free function
    free(ptr);
}

// Mock implementation of strdup
char *mock_strdup(const char *s) {
    if (mock_malloc_should_fail) {
        return NULL;
    }
    // Call the real strdup function
    return strdup(s);
}

// Mock implementation of gethostname
int mock_gethostname(char *name, size_t len) {
    if (mock_gethostname_should_fail) {
        return -1;
    }

    if (mock_gethostname_result) {
        size_t result_len = strlen(mock_gethostname_result);
        if (result_len >= len) {
            return -1; // Buffer too small
        }
        strcpy(name, mock_gethostname_result);
        return 0;
    }

    // Default behavior - return "testhost"
    if (len > 9) {
        strcpy(name, "testhost");
        return 0;
    }
    return -1;
}

// Mock control functions
void mock_system_set_malloc_failure(int should_fail) {
    mock_malloc_should_fail = should_fail;
}

void mock_system_set_gethostname_failure(int should_fail) {
    mock_gethostname_should_fail = should_fail;
}

void mock_system_set_gethostname_result(const char *result) {
    mock_gethostname_result = result;
}

void mock_system_reset_all(void) {
    mock_malloc_should_fail = 0;
    mock_gethostname_should_fail = 0;
    mock_gethostname_result = NULL;
}