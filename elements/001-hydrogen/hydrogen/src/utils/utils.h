/*
 * Core Utilities for High-Precision 3D Printing
 * 
 * This header serves as a facade for the utility subsystem,
 * providing access to all utility functions through a single include.
 * 
 * The implementation has been split into focused modules:
 * - Queue Operations (utils_queue)
 * - System Status (utils_status)
 * - Time Operations (utils_time)
 * - Logging Functions (utils_logging)
 * 
 * Note: Thread Management has been elevated to a full subsystem
 */

#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdarg.h>

// Include all utility module headers
#include "../threads/threads.h"  // Thread management subsystem
#include "utils_queue.h"    // Local include since we're in utils/
#include "../status/status.h"  // System status subsystem
#include "utils_time.h"     // Local include since we're in utils/
#include "utils_logging.h"  // Local include since we're in utils/
#include "utils_dependency.h"  // Local include since we're in utils/

/**
 * @brief Format a number with thousands separators
 * @param n The number to format
 * @param formatted Buffer to store the formatted string
 * @param size Size of the buffer
 * @return The formatted string (same as formatted parameter)
 * @note Thread-safe as long as different buffers are used
 */
char* format_number_with_commas(size_t n, char* formatted, size_t size);

/**
 * @brief Add a formatted message to a message array
 * @param messages Array of message strings
 * @param max_messages Maximum number of messages the array can hold
 * @param count Pointer to current message count
 * @param format Printf-style format string
 * @param ... Additional arguments for format string
 * @return true if message was added, false if array is full
 * @note Automatically maintains NULL termination of message array
 */
bool add_message_to_array(const char** messages, int max_messages, int* count, const char* format, ...);

#endif // UTILS_H