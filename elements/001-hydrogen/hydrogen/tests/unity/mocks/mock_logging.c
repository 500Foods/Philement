/*
 * Mock logging functions implementation for unit testing
 *
 * This file provides mock implementations of logging functions
 * to enable unit testing without system dependencies.
 */

#include "mock_logging.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unity.h>

// Include the real logging header but undefine the function to access real implementation if needed
#include <src/logging/logging.h>

// Undefine the macro in this file so we can call the real function if needed
#undef log_this

// Function prototype for the real log_this function
void real_log_this(const char* subsystem, const char* format, int priority, int num_args, ...);

// Mock state variables
static int mock_call_count = 0;
static char mock_last_subsystem[256] = {0};
static char mock_last_message[1024] = {0};
static int mock_last_priority = 0;
static int mock_expected_calls = 0;

// Mock implementation of log_this
void mock_log_this(const char* subsystem, const char* format, int priority, int num_args, ...) {
    // Debug output to see what's being called
    fprintf(stderr, "MOCK_LOG_DEBUG: MOCK FUNCTION CALLED! subsystem='%s', format='%s', priority=%d, num_args=%d, call_count=%d\n",
            subsystem ? subsystem : "NULL",
            format ? format : "NULL",
            priority, num_args, mock_call_count);

    // Store call information
    mock_call_count++;
    strncpy(mock_last_subsystem, subsystem, sizeof(mock_last_subsystem) - 1);
    mock_last_priority = priority;

    // Format the message
    va_list args;
    va_start(args, num_args);
    vsnprintf(mock_last_message, sizeof(mock_last_message) - 1, format, args);
    va_end(args);

    // Print to stderr for debugging
    fprintf(stderr, "MOCK_LOG: [%s] %s (priority: %d, calls: %d)\n", subsystem, mock_last_message, priority, mock_call_count);
}

// Reset all mock state
void mock_logging_reset_all(void) {
    mock_call_count = 0;
    mock_last_subsystem[0] = '\0';
    mock_last_message[0] = '\0';
    mock_last_priority = 0;
    mock_expected_calls = 0;
}

// Get call count
int mock_logging_get_call_count(void) {
    return mock_call_count;
}

// Get last subsystem
const char* mock_logging_get_last_subsystem(void) {
    return mock_last_subsystem;
}

// Get last message
const char* mock_logging_get_last_message(void) {
    return mock_last_message;
}

// Get last priority
int mock_logging_get_last_priority(void) {
    return mock_last_priority;
}

// Set expected number of calls
void mock_logging_set_expected_calls(int count) {
    mock_expected_calls = count;
}