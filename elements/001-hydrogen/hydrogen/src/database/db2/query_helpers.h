/*
 * DB2 Database Engine - Query Helper Functions
 *
 * Internal helper functions for query execution.
 * These functions are extracted for better testability and code reuse.
 */

#ifndef DATABASE_ENGINE_DB2_QUERY_HELPERS_H
#define DATABASE_ENGINE_DB2_QUERY_HELPERS_H

#include <stdbool.h>
#include <stddef.h>

/**
 * Retrieve column name from statement handle with fallback to generic name
 *
 * @param stmt_handle DB2 statement handle
 * @param col_index Column index (0-based)
 * @param column_name Output buffer for column name (caller must free)
 * @return true if successful (either from SQLDescribeCol or fallback), false on allocation failure
 */
bool db2_get_column_name(void* stmt_handle, int col_index, char** column_name);

/**
 * Ensure JSON buffer has enough capacity, reallocating if necessary
 *
 * @param buffer Pointer to buffer pointer (may be reallocated)
 * @param current_size Current used size of buffer
 * @param capacity Pointer to current capacity (will be updated if reallocated)
 * @param needed_size Minimum capacity needed
 * @return true if buffer has enough capacity (possibly after reallocation), false on allocation failure
 */
bool db2_ensure_json_buffer_capacity(char** buffer, size_t current_size, 
                                     size_t* capacity, size_t needed_size);

/**
 * Escape special characters in string for JSON output
 *
 * @param input Source string to escape
 * @param output Output buffer for escaped string
 * @param output_size Size of output buffer
 * @return Number of characters written (excluding null terminator), or -1 if buffer too small
 */
int db2_json_escape_string(const char* input, char* output, size_t output_size);

#endif // DATABASE_ENGINE_DB2_QUERY_HELPERS_H