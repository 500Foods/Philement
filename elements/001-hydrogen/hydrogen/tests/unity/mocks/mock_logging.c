/*
 * Mock logging functions implementation for unit testing
 *
 * This file provides mock implementations of logging functions
 * to enable unit testing without system dependencies.
 *
 * ==================================================================================
 * IMPORTANT: Adding New Mock Functions
 * ==================================================================================
 *
 * When src/logging/logging.c adds a NEW PUBLIC FUNCTION, you must update this mock:
 *
 * STEP 1: Add Mock Implementation (in this file)
 *   - Create a mock function: void mock_your_function_name(...)
 *   - Keep it simple - usually just logs that it was called
 *   - Add any state tracking if needed for tests
 *
 * STEP 2: Add Function Prototype (in this file)
 *   - Below in the "Mock function prototypes" section around line 35
 *   - Add: void mock_your_function_name(...);
 *
 * STEP 3: Update mock_logging.h
 *   - In the USE_MOCK_LOGGING section, add:
 *     a) undef your_function_name
 *     b) define your_function_name mock_your_function_name
 *   - In the "Mock function prototypes" section, add the prototype again
 *
 * STEP 4: Protect Real Implementation in src/logging/logging.c
 *   - Before the real function, add an ifdef/undef block with prototype
 *   - This prevents the preprocessor from renaming the real function
 *   - See cleanup_log_buffer() in logging.c for the exact pattern
 *
 * WHY: The build system passes -Dlog_this=mock_log_this as compiler flags during
 * Unity builds. Without proper protection, preprocessor would rename BOTH the mock
 * and real implementations to the same name, causing duplicate symbol linker errors.
 *
 * EXAMPLE: See mock_cleanup_log_buffer() below at line 32 for reference.
 * ==================================================================================
 */

#include "mock_logging.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
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

// History buffer so tests can assert that a message was logged at any
// point during the test, not only as the final message.
//
// Worker-pool and queue code log heavily from multiple threads (idle
// poll MUTEX REL every 1ms per worker). A small non-ring buffer drops
// interesting markers under suite load; a non-locked buffer races.
// Ring of 512 + mutex keeps concurrent writers coherent and retains
// recent history even when call_count >> capacity.
#define MOCK_LOG_HISTORY_SIZE 512
static char mock_history[MOCK_LOG_HISTORY_SIZE][1024];
static size_t mock_history_count = 0;
static size_t mock_history_start = 0;
static pthread_mutex_t mock_log_mutex = PTHREAD_MUTEX_INITIALIZER;

// Mock implementation of cleanup_log_buffer
void mock_cleanup_log_buffer(void) {
    // Mock implementation does nothing - just tracks that it was called
    fprintf(stderr, "MOCK_LOG_DEBUG: mock_cleanup_log_buffer() called\n");
}

// Mock implementation of log_this
void mock_log_this(const char* subsystem, const char* format, int priority, int num_args, ...) {
    char formatted[1024];
    formatted[0] = '\0';

    // Format outside the lock so we hold it only for state updates.
    va_list args;
    va_start(args, num_args);
    if (format) {
        vsnprintf(formatted, sizeof(formatted) - 1, format, args);
        formatted[sizeof(formatted) - 1] = '\0';
    }
    va_end(args);

    pthread_mutex_lock(&mock_log_mutex);

    // Debug output to see what's being called
    fprintf(stderr, "MOCK_LOG_DEBUG: MOCK FUNCTION CALLED! subsystem='%s', format='%s', priority=%d, num_args=%d, call_count=%d\n",
            subsystem ? subsystem : "NULL",
            format ? format : "NULL",
            priority, num_args, mock_call_count);

    // Store call information
    mock_call_count++;
    if (subsystem) {
        strncpy(mock_last_subsystem, subsystem, sizeof(mock_last_subsystem) - 1);
        mock_last_subsystem[sizeof(mock_last_subsystem) - 1] = '\0';
    } else {
        mock_last_subsystem[0] = '\0';
    }
    mock_last_priority = priority;
    strncpy(mock_last_message, formatted, sizeof(mock_last_message) - 1);
    mock_last_message[sizeof(mock_last_message) - 1] = '\0';

    // Ring-buffer history: once full, overwrite oldest entries.
    size_t slot;
    if (mock_history_count < MOCK_LOG_HISTORY_SIZE) {
        slot = (mock_history_start + mock_history_count) % MOCK_LOG_HISTORY_SIZE;
        mock_history_count++;
    } else {
        slot = mock_history_start;
        mock_history_start = (mock_history_start + 1) % MOCK_LOG_HISTORY_SIZE;
    }
    strncpy(mock_history[slot], formatted, sizeof(mock_history[0]) - 1);
    mock_history[slot][sizeof(mock_history[0]) - 1] = '\0';

    // Print to stderr for debugging
    fprintf(stderr, "MOCK_LOG: [%s] %s (priority: %d, calls: %d)\n",
            subsystem ? subsystem : "(null)", formatted, priority, mock_call_count);

    pthread_mutex_unlock(&mock_log_mutex);
}

// Reset all mock state
void mock_logging_reset_all(void) {
    pthread_mutex_lock(&mock_log_mutex);
    mock_call_count = 0;
    mock_last_subsystem[0] = '\0';
    mock_last_message[0] = '\0';
    mock_last_priority = 0;
    mock_expected_calls = 0;
    mock_history_count = 0;
    mock_history_start = 0;
    for (size_t i = 0; i < MOCK_LOG_HISTORY_SIZE; i++) {
        mock_history[i][0] = '\0';
    }
    pthread_mutex_unlock(&mock_log_mutex);
}

// Get call count
int mock_logging_get_call_count(void) {
    pthread_mutex_lock(&mock_log_mutex);
    int count = mock_call_count;
    pthread_mutex_unlock(&mock_log_mutex);
    return count;
}

// Get last subsystem
const char* mock_logging_get_last_subsystem(void) {
    return mock_last_subsystem;
}

// Get last message
const char* mock_logging_get_last_message(void) {
    return mock_last_message;
}

// Search the full log history for a substring
bool mock_logging_message_contains(const char* substring) {
    if (!substring) {
        return false;
    }
    pthread_mutex_lock(&mock_log_mutex);
    bool found = false;
    for (size_t i = 0; i < mock_history_count; i++) {
        size_t idx = (mock_history_start + i) % MOCK_LOG_HISTORY_SIZE;
        if (strstr(mock_history[idx], substring) != NULL) {
            found = true;
            break;
        }
    }
    pthread_mutex_unlock(&mock_log_mutex);
    return found;
}

// Get last priority
int mock_logging_get_last_priority(void) {
    pthread_mutex_lock(&mock_log_mutex);
    int priority = mock_last_priority;
    pthread_mutex_unlock(&mock_log_mutex);
    return priority;
}

// Set expected number of calls
void mock_logging_set_expected_calls(int count) {
    pthread_mutex_lock(&mock_log_mutex);
    mock_expected_calls = count;
    pthread_mutex_unlock(&mock_log_mutex);
}