/*
 * Database JSON Utilities - Shared JSON formatting functions implementation
 */

#include <src/hydrogen.h>
#include <string.h>
#include <stdlib.h>
#include "database_json.h"

// JSON string escaping - escapes quotes, backslashes, and control characters
int database_json_escape_string(const char* input, char* output, size_t output_size) {
    if (!input || !output || output_size < 2) {
        return -1;
    }

    const char* src = input;
    char* dst = output;
    size_t available = output_size - 1; // Reserve space for null terminator

    while (*src && available > 0) {
        if (*src == '"' || *src == '\\') {
            if (available < 2) break;
            *dst++ = '\\';
            *dst++ = (*src == '"') ? '"' : '\\';
            available -= 2;
        } else if (*src == '\n') {
            if (available < 2) break;
            *dst++ = '\\';
            *dst++ = 'n';
            available -= 2;
        } else if (*src == '\r') {
            if (available < 2) break;
            *dst++ = '\\';
            *dst++ = 'r';
            available -= 2;
        } else if (*src == '\t') {
            if (available < 2) break;
            *dst++ = '\\';
            *dst++ = 't';
            available -= 2;
        } else {
            *dst++ = *src;
            available--;
        }
        src++;
    }

    *dst = '\0';

    // Return -1 if we couldn't fit everything
    if (*src != '\0') {
        return -1;
    }

    return (int)(dst - output);
}

// Format a single JSON value with proper typing
bool database_json_format_value(const char* column_name, const char* value, bool is_numeric, bool is_null,
                                char* output, size_t output_size, size_t* written) {
    if (!column_name || !output || !written) {
        return false;
    }

    *written = 0;

    // Check if column_name is empty - if so, format just the value without column
    bool has_column = (strlen(column_name) > 0);

    if (is_null) {
        // NULL value
        int result;
        if (has_column) {
            result = snprintf(output, output_size, "\"%s\":null", column_name);
        } else {
            result = snprintf(output, output_size, "null");
        }
        if (result < 0 || (size_t)result >= output_size) {
            return false;
        }
        *written = (size_t)result;
        return true;
    }

    if (is_numeric && value && strlen(value) > 0) {
        // Numeric type - no quotes around value
        int result;
        if (has_column) {
            result = snprintf(output, output_size, "\"%s\":%s", column_name, value);
        } else {
            result = snprintf(output, output_size, "%s", value);
        }
        if (result < 0 || (size_t)result >= output_size) {
            return false;
        }
        *written = (size_t)result;
        return true;
    }

    // String type - escape and quote the value
    if (!value) {
        value = "";
    }

    // Calculate space needed for escaped content
    size_t escaped_size = strlen(value) * 2 + 1;
    char* escaped_value = calloc(1, escaped_size);
    if (!escaped_value) {
        return false;
    }

    int escape_result = database_json_escape_string(value, escaped_value, escaped_size);
    if (escape_result < 0) {
        free(escaped_value);
        return false;
    }

    int result;
    if (has_column) {
        result = snprintf(output, output_size, "\"%s\":\"%s\"", column_name, escaped_value);
    } else {
        result = snprintf(output, output_size, "\"%s\"", escaped_value);
    }
    free(escaped_value);

    if (result < 0 || (size_t)result >= output_size) {
        return false;
    }

    *written = (size_t)result;
    return true;
}

// Ensure JSON buffer has enough capacity
bool database_json_ensure_buffer_capacity(char** buffer, size_t current_size,
                                         size_t* capacity, size_t needed_size) {
    if (!buffer || !capacity) {
        return false;
    }

    // Already have enough capacity
    if (current_size + needed_size <= *capacity) {
        return true;
    }

    // Calculate new capacity (at least double, or enough for needed size + some headroom)
    size_t new_capacity = *capacity * 2;
    if (new_capacity < current_size + needed_size + 1024) {
        new_capacity = current_size + needed_size + 1024;
    }

    // Reallocate buffer
    char* new_buffer = realloc(*buffer, new_capacity);
    if (!new_buffer) {
        return false;
    }

    *buffer = new_buffer;
    *capacity = new_capacity;
    return true;
}