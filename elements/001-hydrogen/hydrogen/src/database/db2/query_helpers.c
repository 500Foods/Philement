/*
 * DB2 Database Engine - Query Helper Functions Implementation
 */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>

// Local includes
#include "types.h"
#include "query_helpers.h"

// External declarations for libdb2 function pointers
extern SQLDescribeCol_t SQLDescribeCol_ptr;

bool db2_get_column_name(void* stmt_handle, int col_index, char** column_name) {
    if (!stmt_handle || !column_name) {
        return false;
    }

    // Try to get the actual column name
    char col_name[256] = {0};
    short col_name_len = 0;
    int desc_result = SQLDescribeCol_ptr ? 
        SQLDescribeCol_ptr(stmt_handle, col_index + 1, (unsigned char*)col_name, 
                          sizeof(col_name), &col_name_len, NULL, NULL, NULL, NULL) : -1;

    if (desc_result == SQL_SUCCESS || desc_result == SQL_SUCCESS_WITH_INFO) {
        // Successfully retrieved column name
        *column_name = strdup(col_name);
        return (*column_name != NULL);
    }

    // Fallback to generic name if SQLDescribeCol failed or is unavailable
    char fallback_name[32];
    snprintf(fallback_name, sizeof(fallback_name), "col%d", col_index + 1);
    *column_name = strdup(fallback_name);
    return (*column_name != NULL);
}

bool db2_ensure_json_buffer_capacity(char** buffer, size_t current_size,
                                     size_t* capacity, size_t needed_size) {
    if (!buffer || !capacity) {
        return false;
    }

    // Already have enough capacity
    if (current_size + needed_size < *capacity) {
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

int db2_json_escape_string(const char* input, char* output, size_t output_size) {
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