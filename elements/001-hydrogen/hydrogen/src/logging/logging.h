/*
 * Logging
 * 
 */

#ifndef LOGGING_H
#define LOGGING_H

#include <src/globals.h>
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

/*
 * Check if the current thread is currently in a logging operation
 * This is used to prevent circular dependencies between logging and mutex operations
 *
 * @return true if currently in a logging operation, false otherwise
 */
bool log_is_in_logging_operation(void);

/*
 * Initialize a message slot in the rolling buffer
 * Internal function exposed for unit testing
 *
 * @param index The slot index to initialize
 * @return true if successful, false on allocation failure
 */
bool init_message_slot(size_t index);

/*
 * Add a message to the rolling buffer
 * Internal function exposed for unit testing
 *
 * @param message The message to add
 */
void add_to_buffer(const char* message);

/*
 * Get fallback priority label for when config is unavailable
 * Internal function exposed for unit testing
 *
 * @param priority The priority level
 * @return The priority label string
 */
const char* get_fallback_priority_label(int priority);

/*
 * Count format specifiers in a format string
 * Internal function exposed for unit testing
 *
 * @param format The format string to analyze
 * @return Number of format specifiers found
 */
size_t count_format_specifiers(const char* format);

// Forward declarations for static functions
void free_logging_flag(void *ptr);
void init_tls_keys(void);
bool* get_logging_operation_flag(void);
void set_logging_operation_flag(bool val);
bool* get_mutex_operation_flag(void);
void set_mutex_operation_flag(bool val);
bool* get_log_group_flag(void);
void set_log_group_flag(bool val);
void console_log(const char* subsystem, int priority, const char* message, unsigned long current_count);

#endif // LOGGING_H
