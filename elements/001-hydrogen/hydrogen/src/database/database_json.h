/*
 * Database JSON Utilities - Shared JSON formatting functions
 *
 * Provides consistent JSON formatting across all database engines.
 */

#ifndef DATABASE_JSON_H
#define DATABASE_JSON_H

#include <stdbool.h>
#include <stddef.h>

// JSON escaping function - shared across all engines
int database_json_escape_string(const char* input, char* output, size_t output_size);

// JSON value formatting functions
bool database_json_format_value(const char* column_name, const char* value, bool is_numeric, bool is_null,
                               char* output, size_t output_size, size_t* written);

// JSON buffer management
bool database_json_ensure_buffer_capacity(char** buffer, size_t current_size,
                                         size_t* capacity, size_t needed_size);

#endif // DATABASE_JSON_H