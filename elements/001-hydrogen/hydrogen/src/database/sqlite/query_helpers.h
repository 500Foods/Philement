/*
 * SQLite Database Engine - Query Helper Functions
 *
 * Internal helper functions for query execution.
 * These functions are extracted for better testability and code reuse.
 */

#ifndef DATABASE_ENGINE_SQLITE_QUERY_HELPERS_H
#define DATABASE_ENGINE_SQLITE_QUERY_HELPERS_H

#include <stdbool.h>
#include <stddef.h>

/**
 * Check if SQLite type is numeric
 *
 * @param type SQLite column type (SQLITE_INTEGER, SQLITE_FLOAT, etc.)
 * @return true if type is numeric, false otherwise
 */
bool sqlite_is_numeric_type(int type);

/**
 * Check if a string value represents a valid number
 * This handles SQLite's dynamic typing where numbers might be stored as text
 *
 * @param value The string value to check
 * @return true if the value represents a valid number, false otherwise
 */
bool sqlite_is_numeric_value(const char* value);

/**
 * Ensure JSON buffer has enough capacity, reallocating if necessary
 *
 * @param buffer Pointer to buffer pointer (may be reallocated)
 * @param current_size Current used size of buffer
 * @param capacity Pointer to current capacity (will be updated if reallocated)
 * @param needed_size Minimum capacity needed
 * @return true if buffer has enough capacity (possibly after reallocation), false on allocation failure
 */
bool sqlite_ensure_json_buffer_capacity(char** buffer, size_t current_size,
                                        size_t* capacity, size_t needed_size);

/**
 * Cleanup column names array
 *
 * @param column_names Array of column name strings
 * @param column_count Number of columns
 */
void sqlite_cleanup_column_names(char** column_names, int column_count);

/**
 * Fetch and format a single row of data
 *
 * @param stmt_handle SQLite statement handle
 * @param column_names Array of column names
 * @param column_count Number of columns
 * @param json_buffer Pointer to JSON buffer pointer
 * @param json_buffer_size Pointer to current buffer size
 * @param json_buffer_capacity Pointer to current buffer capacity
 * @param first_row Whether this is the first row
 * @return true on success, false on allocation failure
 */
bool sqlite_fetch_row_data(void* stmt_handle, char** column_names, int column_count,
                           char** json_buffer, size_t* json_buffer_size, 
                           size_t* json_buffer_capacity, bool first_row);

/**
 * Get column names from prepared statement
 *
 * @param stmt_handle SQLite statement handle
 * @param column_count Number of columns
 * @return Array of column names (caller must free), or NULL on failure
 */
char** sqlite_get_column_names(void* stmt_handle, int column_count);

#endif // DATABASE_ENGINE_SQLITE_QUERY_HELPERS_H