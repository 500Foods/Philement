/*
 * Mock logging functions for unit testing
 *
 * This file provides mock implementations of logging functions
 * to enable unit testing without system dependencies.
 */

#ifndef MOCK_LOGGING_H
#define MOCK_LOGGING_H

#include <stdarg.h>

// Mock function declarations - these will override the real ones when USE_MOCK_LOGGING is defined
#ifdef USE_MOCK_LOGGING

// Undefine the function prototypes from logging.h to prevent conflicts
#undef log_this
#undef cleanup_log_buffer

// Override logging functions with our mocks
#define log_this mock_log_this
#define cleanup_log_buffer mock_cleanup_log_buffer

// Debug output to verify the define is working
// #pragma message("USE_MOCK_LOGGING is defined - mock logging is active")

#endif // USE_MOCK_LOGGING

#ifndef USE_MOCK_LOGGING
// #pragma message("USE_MOCK_LOGGING is NOT defined - using real logging")
#endif

// Mock function prototypes (always available)
void mock_log_this(const char* subsystem, const char* format, int priority, int num_args, ...);
void mock_cleanup_log_buffer(void);

// Mock control functions (always available)
void mock_logging_reset_all(void);
int mock_logging_get_call_count(void);
const char* mock_logging_get_last_subsystem(void);
const char* mock_logging_get_last_message(void);
int mock_logging_get_last_priority(void);
void mock_logging_set_expected_calls(int count);

#endif // MOCK_LOGGING_H