/*
 * SQLite Database Engine - Query Helper Functions Implementation
 */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>

// Local includes
#include "types.h"
#include "query_helpers.h"

// SQLite type constants (from sqlite3.h)
#define SQLITE_INTEGER  1
#define SQLITE_FLOAT    2
#define SQLITE_TEXT     3
#define SQLITE_BLOB     4
#define SQLITE_NULL     5

// External declarations for libsqlite3 function pointers
extern sqlite3_column_count_t sqlite3_column_count_ptr;
extern sqlite3_column_name_t sqlite3_column_name_ptr;
extern sqlite3_column_text_t sqlite3_column_text_ptr;
extern sqlite3_column_type_t sqlite3_column_type_ptr;
extern sqlite3_step_t sqlite3_step_ptr;

// Helper function to check if SQLite type is numeric
bool sqlite_is_numeric_type(int type) {
    return (type == SQLITE_INTEGER || type == SQLITE_FLOAT);
}

// Helper function to check if a string value represents a valid number
// This handles SQLite's dynamic typing where numbers might be stored as text
bool sqlite_is_numeric_value(const char* value) {
    if (!value || strlen(value) == 0) {
        return false;
    }

    // Skip leading whitespace
    const char* ptr = value;
    while (*ptr == ' ' || *ptr == '\t') ptr++;

    // Check for optional sign
    if (*ptr == '+' || *ptr == '-') ptr++;

    // Check for digits before decimal point
    bool has_digits = false;
    while (*ptr >= '0' && *ptr <= '9') {
        has_digits = true;
        ptr++;
    }

    // Check for decimal point and digits after
    if (*ptr == '.') {
        ptr++;
        while (*ptr >= '0' && *ptr <= '9') {
            has_digits = true;
            ptr++;
        }
    }

    // Check for exponential notation
    if ((*ptr == 'e' || *ptr == 'E') && has_digits) {
        ptr++;
        if (*ptr == '+' || *ptr == '-') ptr++;
        bool has_exp_digits = false;
        while (*ptr >= '0' && *ptr <= '9') {
            has_exp_digits = true;
            ptr++;
        }
        if (!has_exp_digits) {
            return false;
        }
    }

    // Skip trailing whitespace
    while (*ptr == ' ' || *ptr == '\t') ptr++;

    // Must have digits and be at end of string
    return has_digits && *ptr == '\0';
}

bool sqlite_ensure_json_buffer_capacity(char** buffer, size_t current_size,
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

void sqlite_cleanup_column_names(char** column_names, int column_count) {
    if (column_names) {
        for (int i = 0; i < column_count; i++) {
            free(column_names[i]);
        }
        free(column_names);
    }
}

char** sqlite_get_column_names(void* stmt_handle, int column_count) {
    if (column_count <= 0 || !stmt_handle) {
        return NULL;
    }
    
    char** column_names = calloc((size_t)column_count, sizeof(char*));
    if (!column_names) {
        return NULL;
    }

    for (int col = 0; col < column_count; col++) {
        const char* col_name = sqlite3_column_name_ptr ? sqlite3_column_name_ptr(stmt_handle, col) : NULL;
        column_names[col] = col_name ? strdup(col_name) : strdup("");
        if (!column_names[col]) {
            // Cleanup on failure
            for (int i = 0; i < col; i++) {
                free(column_names[i]);
            }
            free(column_names);
            return NULL;
        }
    }
    
    return column_names;
}

bool sqlite_fetch_row_data(void* stmt_handle, char** column_names, int column_count,
                           char** json_buffer, size_t* json_buffer_size, 
                           size_t* json_buffer_capacity, bool first_row) {
    if (!stmt_handle || !json_buffer || !json_buffer_size || !json_buffer_capacity) {
        return false;
    }
    
    // Add comma between rows if not first row
    if (!first_row) {
        if (!sqlite_ensure_json_buffer_capacity(json_buffer, *json_buffer_size, json_buffer_capacity, 2)) {
            return false;
        }
        strcat(*json_buffer, ",");
        (*json_buffer_size)++;
    }

    // Start JSON object for this row
    if (!sqlite_ensure_json_buffer_capacity(json_buffer, *json_buffer_size, json_buffer_capacity, 2)) {
        return false;
    }
    strcat(*json_buffer, "{");
    (*json_buffer_size)++;

    // Fetch each column
    for (int col = 0; col < column_count; col++) {
        if (col > 0) {
            if (!sqlite_ensure_json_buffer_capacity(json_buffer, *json_buffer_size, json_buffer_capacity, 2)) {
                return false;
            }
            strcat(*json_buffer, ",");
            (*json_buffer_size)++;
        }

        const char* col_name = column_names ? column_names[col] : "unknown";
        
        // Get column type to determine if we should quote the value
        int col_type = sqlite3_column_type_ptr ? sqlite3_column_type_ptr(stmt_handle, col) : SQLITE_TEXT;
        bool is_numeric = sqlite_is_numeric_type(col_type);
        
        // Check for NULL value
        if (col_type == SQLITE_NULL) {
            char col_json[256];
            snprintf(col_json, sizeof(col_json), "\"%s\":null", col_name);
            size_t col_json_len = strlen(col_json);
            if (!sqlite_ensure_json_buffer_capacity(json_buffer, *json_buffer_size, json_buffer_capacity, col_json_len + 1)) {
                return false;
            }
            strcat(*json_buffer, col_json);
            *json_buffer_size += col_json_len;
        } else {
            const unsigned char* text = sqlite3_column_text_ptr ? sqlite3_column_text_ptr(stmt_handle, col) : NULL;
            const char* value = text ? (const char*)text : "";
            
            // Calculate needed size for this column (name + value + escaping overhead)
            size_t value_len = strlen(value);
            size_t needed_size = strlen(col_name) + (value_len * 2) + 20;
            
            // Ensure buffer has enough space
            if (!sqlite_ensure_json_buffer_capacity(json_buffer, *json_buffer_size, json_buffer_capacity, needed_size)) {
                return false;
            }
            
            // Append column to JSON
            char* append_pos = *json_buffer + *json_buffer_size;
            
            if ((is_numeric || sqlite_is_numeric_value(value)) && strlen(value) > 0) {
                // Numeric type or numeric value - no quotes around value
                sprintf(append_pos, "\"%s\":%s", col_name, value);
            } else {
                // String type - escape and quote
                sprintf(append_pos, "\"%s\":\"", col_name);
                char* dst = append_pos + strlen(append_pos);
                const char* src = value;
                while (*src) {
                    if (*src == '"' || *src == '\\') {
                        *dst++ = '\\';
                        *dst++ = *src++;
                    } else if (*src == '\n') {
                        *dst++ = '\\';
                        *dst++ = 'n';
                        src++;
                    } else if (*src == '\r') {
                        *dst++ = '\\';
                        *dst++ = 'r';
                        src++;
                    } else if (*src == '\t') {
                        *dst++ = '\\';
                        *dst++ = 't';
                        src++;
                    } else {
                        *dst++ = *src++;
                    }
                }
                *dst++ = '"';
                *dst = '\0';
            }
            
            *json_buffer_size = strlen(*json_buffer);
        }
    }

    // End JSON object for this row
    if (!sqlite_ensure_json_buffer_capacity(json_buffer, *json_buffer_size, json_buffer_capacity, 2)) {
        return false;
    }
    strcat(*json_buffer, "}");
    (*json_buffer_size)++;
    
    return true;
}