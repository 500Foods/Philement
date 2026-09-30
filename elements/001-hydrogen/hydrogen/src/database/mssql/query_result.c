/*
 * MSSQL query results: column names, one row of JSON, and the QueryResult.
 */

#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/database/database_params.h>

#include "types.h"
#include "query.h"
#include "query_helpers.h"

extern SQLFetch_t mssql_SQLFetch_ptr;
extern SQLGetData_t mssql_SQLGetData_ptr;
extern SQLNumResultCols_t mssql_SQLNumResultCols_ptr;
extern SQLRowCount_t mssql_SQLRowCount_ptr;

// Helper function to cleanup column names (non-static for testing)
void mssql_cleanup_column_names(char** column_names, int column_count) {
    if (column_names) {
        for (int i = 0; i < column_count; i++) {
            free(column_names[i]);
        }
        free(column_names);
    }
}

// Helper function to get column names (non-static for testing)
char** mssql_get_column_names(void* stmt_handle, int column_count) {
    if (column_count <= 0) {
        return NULL;
    }

    char** column_names = calloc((size_t)column_count, sizeof(char*));
    if (!column_names) {
        return NULL;
    }

    for (int col = 0; col < column_count; col++) {
        if (!mssql_get_column_name(stmt_handle, col, &column_names[col])) {
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

// Helper function to fetch and format a single row (non-static for testing)
bool mssql_fetch_row_data(void* stmt_handle, char** column_names, int column_count, char** json_buffer, size_t* json_buffer_size, size_t* json_buffer_capacity, bool first_row) {
    if (!stmt_handle || !json_buffer || !json_buffer_size || !json_buffer_capacity) {
        return false;
    }

    // Add comma between rows if not first row
    if (!first_row) {
        if (!mssql_ensure_json_buffer_capacity(json_buffer, *json_buffer_size, json_buffer_capacity, 2)) {
            return false;
        }
        strcat(*json_buffer, ",");
        (*json_buffer_size)++;
    }

    // Start JSON object for this row
    if (!mssql_ensure_json_buffer_capacity(json_buffer, *json_buffer_size, json_buffer_capacity, 2)) {
        return false;
    }
    strcat(*json_buffer, "{");
    (*json_buffer_size)++;

    // Fetch each column
    for (int col = 0; col < column_count; col++) {
        if (col > 0) {
            if (!mssql_ensure_json_buffer_capacity(json_buffer, *json_buffer_size, json_buffer_capacity, 2)) {
                return false;
            }
            strcat(*json_buffer, ",");
            (*json_buffer_size)++;
        }

        // Get column type to determine if we should quote the value
        int sql_type = 0;
        bool got_type = mssql_get_column_type(stmt_handle, col, &sql_type);
        bool is_numeric = got_type && mssql_is_numeric_type(sql_type);
        bool is_datetime = got_type && (sql_type == SQL_TYPE_DATE || sql_type == SQL_TYPE_TIME || sql_type == SQL_TYPE_TIMESTAMP);

        /* FreeTDS rejects SQLGetData(NULL, 0) and does not report a length.
         * Read SQL_C_CHAR in chunks so nvarchar migration SQL and INSERT
         * OUTPUT columns come back whole. 256 bytes truncates them. */
        char* col_data = NULL;
        bool is_null = false;
        size_t col_cap = 4096;
        size_t col_used = 0;
        const size_t col_max = 32u * 1024u * 1024u;

        if (!mssql_SQLGetData_ptr) {
            return false;
        }

        col_data = calloc(1, col_cap);
        if (!col_data) {
            return false;
        }

        for (;;) {
            long ind = 0;
            size_t space;
            size_t room;
            size_t visible;
            size_t got;
            short get_rc;
            bool need_more;

            if (col_cap - col_used < 2) {
                size_t new_cap = col_cap * 2;
                char* grown;

                if (new_cap < col_cap || new_cap > col_max) {
                    free(col_data);
                    return false;
                }
                grown = realloc(col_data, new_cap);
                if (!grown) {
                    free(col_data);
                    return false;
                }
                memset(grown + col_used, 0, new_cap - col_used);
                col_data = grown;
                col_cap = new_cap;
            }

            space = col_cap - col_used;
            get_rc = mssql_SQLGetData_ptr(stmt_handle, col + 1, SQL_C_CHAR,
                                          col_data + col_used, (long)space, &ind);
            if (get_rc == SQL_NO_DATA) {
                break;
            }
            if (get_rc != SQL_SUCCESS && get_rc != SQL_SUCCESS_WITH_INFO) {
                free(col_data);
                return false;
            }
            if (ind == SQL_NULL_DATA) {
                free(col_data);
                col_data = NULL;
                is_null = true;
                break;
            }

            room = space - 1;
            visible = 0;
            while (visible < room && col_data[col_used + visible] != '\0') {
                visible++;
            }

            /* SUCCESS_WITH_INFO with an unknown or oversized indicator is
             * truncation (FreeTDS SQLSTATE 01004). A later call continues
             * at column_text_sqlgetdatapos. SUCCESS means this chunk is
             * the end, even when the mock's indicator is larger than the
             * bytes it copied. */
            need_more = (get_rc == SQL_SUCCESS_WITH_INFO &&
                         (ind < 0 || (size_t)ind >= space));
            got = visible;
            if (!need_more && ind >= 0 && (size_t)ind < got) {
                got = (size_t)ind;
            }
            if (need_more && got == 0) {
                free(col_data);
                return false;
            }
            if (got > col_max - col_used) {
                free(col_data);
                return false;
            }
            col_used += got;
            if (!need_more) {
                break;
            }
        }

        if (col_data) {
            col_data[col_used] = '\0';
        }

        if (is_null || col_data) {
            size_t actual_data_len = col_data ? strlen(col_data) : 0;
            size_t needed_json_space = strlen(column_names[col]) + (actual_data_len * 2) + 20;

            // Ensure we have enough capacity
            if (!mssql_ensure_json_buffer_capacity(json_buffer, *json_buffer_size, json_buffer_capacity, needed_json_space)) {
                free(col_data);
                return false;
            }

            // Build JSON for this column
            char* current_pos = *json_buffer + *json_buffer_size;

            if (is_null) {
                int written = snprintf(current_pos, needed_json_space, "\"%s\":null", column_names[col]);
                // Check for truncation - snprintf returns what WOULD be written, not what WAS written
                if (written >= (int)needed_json_space) {
                    written = (int)needed_json_space - 1; // Actual bytes written (excluding null terminator)
                }
                *json_buffer_size += (size_t)written;
            } else if (is_numeric) {
                // Numeric types - no quotes around value
                int written = snprintf(current_pos, needed_json_space, "\"%s\":%s", column_names[col], col_data);
                // Check for truncation - snprintf returns what WOULD be written, not what WAS written
                if (written >= (int)needed_json_space) {
                    written = (int)needed_json_space - 1; // Actual bytes written (excluding null terminator)
                }
                *json_buffer_size += (size_t)written;
            } else {
                // String types - apply MSSQL-specific formatting, trim trailing whitespace, then quote and escape the value
                // For large strings (like migration SQL), use dynamic allocation for escaped data

                // Apply datetime/timestamp formatting for MSSQL
                if (is_datetime) {
                    // Check column name to determine formatting (since MSSQL uses DATETIME2 for both DATETIME and TIMESTAMP columns)
                    if (strstr(column_names[col], "datetime") != NULL) {
                        mssql_format_datetime_string(col_data);
                    } else if (strstr(column_names[col], "timestamp") != NULL) {
                        mssql_format_timestamp_string(col_data);
                    } else {
                        // Fallback to SQL type
                        if (sql_type == SQL_TYPE_TIMESTAMP) {
                            mssql_format_timestamp_string(col_data);
                        } else if (sql_type == SQL_TYPE_DATE || sql_type == SQL_TYPE_TIME) {
                            mssql_format_datetime_string(col_data);
                        }
                    }
                }

                // Trim trailing whitespace (MSSQL-specific)
                mssql_trim_trailing_whitespace(col_data);

                size_t escaped_size = (col_data ? strlen(col_data) * 2 : 0) + 1;
                char* escaped_data = calloc(1, escaped_size);
                if (!escaped_data) {
                    free(col_data);
                    return false;
                }

                database_json_escape_string(col_data, escaped_data, escaped_size);
                int written = snprintf(current_pos, needed_json_space, "\"%s\":\"%s\"", column_names[col], escaped_data);
                // Check for truncation - snprintf returns what WOULD be written, not what WAS written
                if (written >= (int)needed_json_space) {
                    written = (int)needed_json_space - 1; // Actual bytes written (excluding null terminator)
                }
                *json_buffer_size += (size_t)written;

                free(escaped_data);
            }
        } else {
            return false;
        }

        free(col_data);
    }

    // End JSON object for this row
    if (!mssql_ensure_json_buffer_capacity(json_buffer, *json_buffer_size, json_buffer_capacity, 2)) {
        return false;
    }
    strcat(*json_buffer, "}");
    (*json_buffer_size)++;

    return true;
}

// Helper function to process query results (non-static for testing)
bool mssql_process_query_results(void* stmt_handle, const char* designator, struct timespec start_time, QueryResult** result) {
    if (!stmt_handle || !designator || !result) {
        return false;
    }

    // Create result structure
    QueryResult* db_result = calloc(1, sizeof(QueryResult));
    if (!db_result) {
        return false;
    }

    db_result->success = true;

    /* A failed or missing column count used to leave column_count at 0.
     * Bootstrap then treated the SELECT as an empty table and dropped it. */
    int column_count = 0;
    int num_rc;

    if (!mssql_SQLNumResultCols_ptr) {
        log_this(designator, "MSSQL SQLNumResultCols unavailable", LOG_LEVEL_ERROR, 0);
        free(db_result);
        return false;
    }
    num_rc = mssql_SQLNumResultCols_ptr(stmt_handle, &column_count);
    if (num_rc != SQL_SUCCESS && num_rc != SQL_SUCCESS_WITH_INFO) {
        log_this(designator, "MSSQL SQLNumResultCols failed: %d", LOG_LEVEL_ERROR, 1, num_rc);
        free(db_result);
        return false;
    }
    if (column_count < 0) {
        column_count = 0;
    }
    db_result->column_count = (size_t)column_count;

    // Get column names using helper function
    char** column_names = mssql_get_column_names(stmt_handle, column_count);
    if (column_count > 0 && !column_names) {
        free(db_result);
        return false;
    }

    /* SQLRowCount writes a SQLLEN. On a SELECT FreeTDS stores -1 (unknown).
     * Keep that as affected_rows -1; a 4-byte int would spill into column_count. */
    long sql_row_count = 0;
    if (mssql_SQLRowCount_ptr) {
        int row_rc = mssql_SQLRowCount_ptr(stmt_handle, &sql_row_count);
        if (row_rc == SQL_SUCCESS || row_rc == SQL_SUCCESS_WITH_INFO) {
            if (sql_row_count < 0) {
                db_result->affected_rows = -1;
            } else if (sql_row_count > 2147483647L) {
                db_result->affected_rows = 2147483647;
            } else {
                db_result->affected_rows = (int)sql_row_count;
            }
        }
    }

    // Fetch all result rows - only if there are result columns
    size_t row_count = 0;
    char* json_buffer = NULL;
    size_t json_buffer_size = 0;
    size_t json_buffer_capacity = 1024;

    // Check if this statement returns result columns (not DDL statements)
    if (column_count > 0) {
        json_buffer = calloc(1, json_buffer_capacity);
        if (!json_buffer) {
            // Cleanup column names using helper
            mssql_cleanup_column_names(column_names, column_count);
            free(db_result);
            return false;
        }

        // Start JSON array
        strcpy(json_buffer, "[");
        json_buffer_size = 1;

        if (!mssql_SQLFetch_ptr) {
            log_this(designator, "MSSQL SQLFetch unavailable", LOG_LEVEL_ERROR, 0);
            free(json_buffer);
            mssql_cleanup_column_names(column_names, column_count);
            free(db_result);
            return false;
        }

        for (;;) {
            int fetch_rc = mssql_SQLFetch_ptr(stmt_handle);
            /* SQL_NO_DATA ends the cursor. Any other failure must not be
             * reported as a successful empty result: bootstrap drops the
             * FROM table when row_count is 0. */
            if (fetch_rc == SQL_NO_DATA) {
                break;
            }
            if (fetch_rc != SQL_SUCCESS && fetch_rc != SQL_SUCCESS_WITH_INFO) {
                log_this(designator, "MSSQL SQLFetch failed: %d", LOG_LEVEL_ERROR, 1, fetch_rc);
                free(json_buffer);
                mssql_cleanup_column_names(column_names, column_count);
                free(db_result);
                return false;
            }
            bool first_row = (row_count == 0);
            if (!mssql_fetch_row_data(stmt_handle, column_names, column_count,
                                      &json_buffer, &json_buffer_size, &json_buffer_capacity, first_row)) {
                free(json_buffer);
                mssql_cleanup_column_names(column_names, column_count);
                free(db_result);
                return false;
            }
            row_count++;
        }
    } else {
        // DDL statement or statement with no result columns - create empty JSON array
        json_buffer = strdup("[]");
        if (!json_buffer) {
            mssql_cleanup_column_names(column_names, column_count);
            free(db_result);
            return false;
        }
        json_buffer_size = 2; // Length of "[]"
        json_buffer_capacity = 3; // CRITICAL FIX: Update capacity to match actual allocation (2 chars + null terminator)
    }

    // End JSON array - only for queries with result columns. For DDL statements, "[]" is already complete, don't append anything
    if (column_count > 0) {
        if (!mssql_ensure_json_buffer_capacity(&json_buffer, json_buffer_size, &json_buffer_capacity, 2)) {
            free(json_buffer);
            mssql_cleanup_column_names(column_names, column_count);
            free(db_result);
            return false;
        }
        strcat(json_buffer, "]");
    }

    db_result->row_count = row_count;
    db_result->data_json = json_buffer;

    // End timing after all result processing is complete
    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    db_result->execution_time_ms = (end_time.tv_sec - start_time.tv_sec) * 1000000 +
                                   (end_time.tv_nsec - start_time.tv_nsec) / 1000;

    log_this(designator, "MSSQL query results: %zu columns, %zu rows, %d affected", LOG_LEVEL_TRACE, 3,
        db_result->column_count, db_result->row_count, db_result->affected_rows);

    // Clean up column names using helper
    mssql_cleanup_column_names(column_names, column_count);

    *result = db_result;
    return true;
}
