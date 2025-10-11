/*
 * Core Utilities
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
#include <src/threads/threads.h>  // Thread management subsystem
#include "utils_queue.h"    // Local include since we're in utils/
#include <src/status/status.h>  // System status subsystem
#include "utils_time.h"     // Local include since we're in utils/
#include "utils_logging.h"  // Local include since we're in utils/
#include "utils_dependency.h"  // Local include since we're in utils/
#include "utils_hash.h"     // Local include since we're in utils/

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
 * @brief Format a double with thousands separators
 * @param value The double value to format
 * @param decimals Number of decimal places (use -1 for no decimal formatting)
 * @param formatted Buffer to store the formatted string
 * @param size Size of the buffer
 * @return The formatted string (same as formatted parameter)
 * @note Thread-safe as long as different buffers are used
 * @note Only adds commas to the integer part, decimal part remains unchanged
 */
char* format_double_with_commas(double value, int decimals, char* formatted, size_t size);

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

/**
 * @brief Store program arguments for later retrieval
 * @param argc Argument count from main()
 * @param argv Argument vector from main()
 * @note Called by main() to preserve original command line arguments
 */
void store_program_args(int argc, char* argv[]);

/**
 * @brief Get stored program arguments
 * @return Pointer to stored argv array from main()
 * @note Used by restart functionality to preserve original command line arguments
 */
char** get_program_args(void);

#endif // UTILS_H
