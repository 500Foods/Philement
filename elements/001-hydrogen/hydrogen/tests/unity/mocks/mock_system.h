/*
 * Mock system functions for unit testing
 *
 * This file provides mock implementations of system functions
 * like malloc, gethostname, etc. to enable testing of error conditions.
 */

#ifndef MOCK_SYSTEM_H
#define MOCK_SYSTEM_H

#include <stddef.h>
#include <unistd.h>

// Mock function declarations - these will override the real ones when USE_MOCK_SYSTEM is defined
#ifdef USE_MOCK_SYSTEM

// Override system functions with our mocks
#define malloc mock_malloc
#define free mock_free
#define strdup mock_strdup
#define gethostname mock_gethostname

// Always declare mock function prototypes for the .c file
void *mock_malloc(size_t size);
void mock_free(void *ptr);
char *mock_strdup(const char *s);
int mock_gethostname(char *name, size_t len);

// Mock control functions for tests
void mock_system_set_malloc_failure(int should_fail);
void mock_system_set_gethostname_failure(int should_fail);
void mock_system_set_gethostname_result(const char *result);
void mock_system_reset_all(void);

#endif // USE_MOCK_SYSTEM

#endif // MOCK_SYSTEM_H