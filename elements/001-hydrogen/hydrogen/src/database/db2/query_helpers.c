/*
 * DB2 Database Engine - Query Helper Functions Implementation
 */

// Standard library includes
#include <ctype.h>

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
        // Successfully retrieved column name - convert to lowercase for cross-engine consistency
        // DB2 returns uppercase by default, but we want lowercase to match other engines
        *column_name = strdup(col_name);
        if (*column_name) {
            for (char* p = *column_name; *p; p++) {
                *p = (char)tolower((unsigned char)*p);
            }
        }
        return (*column_name != NULL);
    }

    // Fallback to generic name if SQLDescribeCol failed or is unavailable
    char fallback_name[32];
    snprintf(fallback_name, sizeof(fallback_name), "col%d", col_index + 1);
    *column_name = strdup(fallback_name);
    return (*column_name != NULL);
}

bool db2_get_column_type(void* stmt_handle, int col_index, int* sql_type) {
    if (!stmt_handle || !sql_type) {
        return false;
    }

    // SQLDescribeCol parameters: handle, column, name, name_length, name_length_ptr,
    //                            data_type, column_size, decimal_digits, nullable
    int data_type = 0;
    int desc_result = SQLDescribeCol_ptr ?
        SQLDescribeCol_ptr(stmt_handle, col_index + 1, NULL, 0, NULL,
                          &data_type, NULL, NULL, NULL) : -1;

    if (desc_result == SQL_SUCCESS || desc_result == SQL_SUCCESS_WITH_INFO) {
        *sql_type = data_type;
        return true;
    }

    return false;
}

bool db2_is_numeric_type(int sql_type) {
    switch (sql_type) {
        case SQL_INTEGER:
        case SQL_SMALLINT:
        case SQL_BIGINT:
        case SQL_DECIMAL:
        case SQL_NUMERIC:
        case SQL_REAL:
        case SQL_FLOAT:
        case SQL_DOUBLE:
            return true;
        default:
            return false;
    }
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