/*
 * Logging
 * 
 */

#ifndef LOGGING_H
#define LOGGING_H

#include "../globals.h"
#include <stdbool.h>

/*
 * Primary logging function - use this for all logging needs
 *
 * This is the ONLY function that should be used for logging throughout the codebase.
 * It handles:
 * - Checking if logging is initialized
 * - Applying logging filters based on configuration
 * - Routing to appropriate outputs based on configuration settings
 * - Thread safety and synchronization
 * - Defensive checking that num_args matches the number of format specifiers
 *
 * Logging destinations (console, database, file) are controlled by configuration
 * settings rather than parameters. This ensures consistent logging behavior
 * across the application based on centralized configuration.
 */
void log_this(const char* subsystem, const char* format, int priority, int num_args, ...);

/**
 * Begin a group of log messages that should be output consecutively
 * 
 * This function acquires the logging mutex to ensure that multiple log messages
 * can be written without being interleaved with messages from other threads.
 * Must be paired with log_group_end().
 */
void log_group_begin(void);

/**
 * End a group of log messages
 * 
 * This function releases the logging mutex acquired by log_group_begin().
 * Must be called after log_group_begin() to prevent deadlocks.
 */
void log_group_end(void);

/*
 * Get all log messages matching a subsystem
 * Returns a newly allocated string containing matching messages
 * or NULL if no matches or on error
 * Caller must free the returned string
 * 
 * @param subsystem The subsystem to match (e.g., "Config-Dump")
 * @return Newly allocated string with matching messages or NULL
 */
char* log_get_messages(const char* subsystem);

/*
 * Get the last N log messages
 * Returns a newly allocated string containing the messages
 * or NULL if no messages or on error
 * Caller must free the returned string
 * 
 * @param count Number of messages to retrieve
 * @return Newly allocated string with messages or NULL
 */
char* log_get_last_n(size_t count);

#endif // LOGGING_H
