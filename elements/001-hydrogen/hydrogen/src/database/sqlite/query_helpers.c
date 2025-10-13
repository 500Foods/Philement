/*
 * SQLite Database Engine - Query Helper Functions Implementation
 */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>

// Local includes
#include "types.h"
#include "query_helpers.h"

// External declarations for libsqlite3 function pointers
extern sqlite3_column_count_t sqlite3_column_count_ptr;
extern sqlite3_column_name_t sqlite3_column_name_ptr;
extern sqlite3_column_text_t sqlite3_column_text_ptr;
extern sqlite3_column_type_t sqlite3_column_type_ptr;
extern sqlite3_step_t sqlite3_step_ptr;

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

        // Build column JSON
        char col_json[1024];
        const char* col_name = column_names ? column_names[col] : "unknown";
        
        // Check for NULL value
        if (sqlite3_column_type_ptr && sqlite3_column_type_ptr(stmt_handle, col) == SQLITE_NULL) {
            snprintf(col_json, sizeof(col_json), "\"%s\":null", col_name);
        } else {
            const unsigned char* text = sqlite3_column_text_ptr ? sqlite3_column_text_ptr(stmt_handle, col) : NULL;
            const char* value = text ? (const char*)text : "";
            snprintf(col_json, sizeof(col_json), "\"%s\":\"%s\"", col_name, value);
        }

        size_t col_json_len = strlen(col_json);
        if (!sqlite_ensure_json_buffer_capacity(json_buffer, *json_buffer_size, json_buffer_capacity, col_json_len + 1)) {
            return false;
        }
        strcat(*json_buffer, col_json);
        *json_buffer_size += col_json_len;
    }

    // End JSON object for this row
    if (!sqlite_ensure_json_buffer_capacity(json_buffer, *json_buffer_size, json_buffer_capacity, 2)) {
        return false;
    }
    strcat(*json_buffer, "}");
    (*json_buffer_size)++;
    
    return true;
}