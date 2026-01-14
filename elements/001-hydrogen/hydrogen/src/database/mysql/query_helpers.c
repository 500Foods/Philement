/*
 * MySQL Database Engine - Query Helper Functions Implementation
 *
 * Implementation of helper functions for MySQL query processing.
 * These functions are non-static for better testability.
 */

#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/database/mysql/types.h>
#include <src/database/mysql/connection.h>
#include <src/database/mysql/query.h>
#include <src/database/mysql/query_helpers.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// MySQL type constants (from mysql_com.h)
#define MYSQL_TYPE_DECIMAL      0
#define MYSQL_TYPE_TINY         1
#define MYSQL_TYPE_SHORT        2
#define MYSQL_TYPE_LONG         3
#define MYSQL_TYPE_FLOAT        4
#define MYSQL_TYPE_DOUBLE       5
#define MYSQL_TYPE_LONGLONG     8
#define MYSQL_TYPE_INT24        9
#define MYSQL_TYPE_NEWDECIMAL   246

// External declarations for libmysqlclient function pointers
extern mysql_fetch_fields_t mysql_fetch_fields_ptr;

// Helper function to check if MySQL type is numeric
bool mysql_is_numeric_type(unsigned int type) {
    switch (type) {
        case MYSQL_TYPE_DECIMAL:
        case MYSQL_TYPE_TINY:
        case MYSQL_TYPE_SHORT:
        case MYSQL_TYPE_LONG:
        case MYSQL_TYPE_FLOAT:
        case MYSQL_TYPE_DOUBLE:
        case MYSQL_TYPE_LONGLONG:
        case MYSQL_TYPE_INT24:
        case MYSQL_TYPE_NEWDECIMAL:
            return true;
        default:
            return false;
    }
}

// Helper function to extract column names (non-static for testing)
char** mysql_extract_column_names(void* mysql_result, size_t column_count) {
    if (!mysql_result || column_count == 0 || !mysql_fetch_fields_ptr) {
        return NULL;
    }

    // Complete MYSQL_FIELD structure definition to match libmysqlclient
    typedef struct {
        char *name;                  /* Name of column */
        char *org_name;              /* Original column name, if an alias */
        char *table;                 /* Table of column if column was a field */
        char *org_table;             /* Org table name, if table was an alias */
        char *db;                    /* Database for table */
        char *catalog;               /* Catalog for table */
        char *def;                   /* Default value (set by mysql_list_fields) */
        unsigned long length;        /* Width of column (create length) */
        unsigned long max_length;    /* Max width for selected set */
        unsigned int name_length;
        unsigned int org_name_length;
        unsigned int table_length;
        unsigned int org_table_length;
        unsigned int db_length;
        unsigned int catalog_length;
        unsigned int def_length;
        unsigned int flags;          /* Div flags */
        unsigned int decimals;       /* Number of decimals in field */
        unsigned int charsetnr;      /* Character set */
        unsigned int type;           /* Type of field */
        void *extension;
    } MYSQL_FIELD_COMPLETE;

    const MYSQL_FIELD_COMPLETE* fields = (const MYSQL_FIELD_COMPLETE*)mysql_fetch_fields_ptr(mysql_result);
    if (!fields) {
        return NULL;
    }

    char** column_names = calloc(column_count, sizeof(char*));
    if (!column_names) {
        return NULL;
    }

    for (size_t i = 0; i < column_count; i++) {
        // Extract actual field name from MYSQL_FIELD structure
        if (fields[i].name) {
            column_names[i] = strdup(fields[i].name);
        } else {
            // Fallback for NULL field names
            char col_name[32];
            snprintf(col_name, sizeof(col_name), "col_%zu", i);
            column_names[i] = strdup(col_name);
        }

        if (!column_names[i]) {
            // Cleanup on allocation failure
            for (size_t j = 0; j < i; j++) {
                free(column_names[j]);
            }
            free(column_names);
            return NULL;
        }
    }

    return column_names;
}

// Helper function to build JSON from MySQL result (non-static for testing)
bool mysql_build_json_from_result(void* mysql_result, size_t row_count, size_t column_count,
                                  char** column_names, char** json_buffer) {
    if (!mysql_result || row_count == 0 || column_count == 0 || !json_buffer) {
        if (json_buffer) {
            *json_buffer = strdup("[]");
        }
        return json_buffer && *json_buffer;
    }

    // Get field types to determine numeric vs string columns
    typedef struct {
        char *name;
        char *org_name;
        char *table;
        char *org_table;
        char *db;
        char *catalog;
        char *def;
        unsigned long length;
        unsigned long max_length;
        unsigned int name_length;
        unsigned int org_name_length;
        unsigned int table_length;
        unsigned int org_table_length;
        unsigned int db_length;
        unsigned int catalog_length;
        unsigned int def_length;
        unsigned int flags;
        unsigned int decimals;
        unsigned int charsetnr;
        unsigned int type;           /* Type of field - CRITICAL for numeric detection */
        void *extension;
    } MYSQL_FIELD_COMPLETE;

    const MYSQL_FIELD_COMPLETE* fields = mysql_fetch_fields_ptr ? (const MYSQL_FIELD_COMPLETE*)mysql_fetch_fields_ptr(mysql_result) : NULL;

    size_t json_size = 1024 * row_count;
    *json_buffer = calloc(1, json_size);
    if (!*json_buffer) {
        return false;
    }

    strcpy(*json_buffer, "[");
    for (size_t row = 0; row < row_count; row++) {
        if (row > 0) strcat(*json_buffer, ",");

        // MYSQL_ROW is char** (array of strings)
        char** row_data = (char**)mysql_fetch_row_ptr(mysql_result);
        if (row_data) {
            strcat(*json_buffer, "{");
            for (size_t col = 0; col < column_count; col++) {
                if (col > 0) strcat(*json_buffer, ",");

                // Build column JSON
                const char* col_name = column_names ? column_names[col] : "unknown";

                // Check for NULL value
                if (row_data[col] == NULL) {
                    // Ensure buffer has room for null value
                    size_t needed = strlen(col_name) + 10; // "name":null plus comma
                    if (strlen(*json_buffer) + needed >= json_size) {
                        json_size = json_size * 2 + needed;
                        char* new_json = realloc(*json_buffer, json_size);
                        if (!new_json) break;
                        *json_buffer = new_json;
                    }
                    char* pos = *json_buffer + strlen(*json_buffer);
                    snprintf(pos, json_size - strlen(*json_buffer), "\"%s\":null", col_name);
                } else {
                    // Check if column is numeric type
                    bool is_numeric = fields && mysql_is_numeric_type(fields[col].type);

                    if (is_numeric) {
                        // Numeric types - no quotes around value (JSON number)
                        size_t needed = strlen(col_name) + strlen(row_data[col]) + 10;
                        if (strlen(*json_buffer) + needed >= json_size) {
                            json_size = json_size * 2 + needed;
                            char* new_json = realloc(*json_buffer, json_size);
                            if (!new_json) break;
                            *json_buffer = new_json;
                        }
                        char* pos = *json_buffer + strlen(*json_buffer);
                        snprintf(pos, json_size - strlen(*json_buffer), "\"%s\":%s", col_name, row_data[col]);
                    } else {
                        // String types - escape and quote the value
                        // For large strings (like migration SQL), use dynamic allocation for escaped data
                        size_t escaped_size = (row_data[col] ? strlen(row_data[col]) * 2 : 0) + 1;
                        char* escaped_data = calloc(1, escaped_size);
                        if (!escaped_data) {
                            size_t needed = strlen(col_name) + 10;
                            if (strlen(*json_buffer) + needed >= json_size) {
                                json_size = json_size * 2 + needed;
                                char* new_json = realloc(*json_buffer, json_size);
                                if (!new_json) break;
                                *json_buffer = new_json;
                            }
                            char* pos = *json_buffer + strlen(*json_buffer);
                            snprintf(pos, json_size - strlen(*json_buffer), "\"%s\":null", col_name);
                        } else {
                            mysql_json_escape_string(row_data[col], escaped_data, escaped_size);
                            size_t needed = strlen(col_name) + strlen(escaped_data) + 10;
                            if (strlen(*json_buffer) + needed >= json_size) {
                                json_size = json_size * 2 + needed;
                                char* new_json = realloc(*json_buffer, json_size);
                                if (!new_json) {
                                    free(escaped_data);
                                    break;
                                }
                                *json_buffer = new_json;
                            }
                            char* pos = *json_buffer + strlen(*json_buffer);
                            snprintf(pos, json_size - strlen(*json_buffer), "\"%s\":\"%s\"", col_name, escaped_data);
                            free(escaped_data);
                        }
                    }
                }
            }
            strcat(*json_buffer, "}");
        }
    }
    strcat(*json_buffer, "]");

    return true;
}

// Helper function to calculate JSON buffer size (non-static for testing)
size_t mysql_calculate_json_buffer_size(size_t row_count, size_t column_count __attribute__((unused))) {
    // Estimate: 1024 bytes per row as a reasonable default
    return 1024 * row_count;
}

// Helper function to validate query parameters (non-static for testing)
bool mysql_validate_query_parameters(const DatabaseHandle* connection, const QueryRequest* request, QueryResult** result) {
    if (!connection || !request || !result || connection->engine_type != DB_ENGINE_MYSQL) {
        return false;
    }
    return true;
}

// Helper function to execute query statement (non-static for testing)
bool mysql_execute_query_statement(void* mysql_connection, const char* sql_template, const char* designator) {
    if (!mysql_query_ptr) {
        log_this(designator, "MySQL query function not available", LOG_LEVEL_ERROR, 0);
        return false;
    }

    if (mysql_query_ptr(mysql_connection, sql_template) != 0) {
        log_this(designator, "MySQL query execution failed", LOG_LEVEL_TRACE, 0);
        if (mysql_error_ptr) {
            const char* error_msg = mysql_error_ptr(mysql_connection);
            if (error_msg && strlen(error_msg) > 0) {
                log_this(designator, "MySQL query error: %s", LOG_LEVEL_TRACE, 1, error_msg);
            }
        }
        return false;
    }
    return true;
}

// Helper function to store query result (non-static for testing)
void* mysql_store_query_result(void* mysql_connection, const char* designator) {
    if (!mysql_store_result_ptr) {
        return NULL;
    }

    void* mysql_result = mysql_store_result_ptr(mysql_connection);
    if (!mysql_result) {
        log_this(designator, "MySQL execute_query: No result set returned", LOG_LEVEL_TRACE, 0);
    }
    return mysql_result;
}

// Helper function to process query result (non-static for testing)
bool mysql_process_query_result(void* mysql_result, QueryResult* db_result, const char* designator) {
    if (!mysql_result) {
        // No result set (e.g., INSERT, UPDATE, DELETE)
        db_result->row_count = 0;
        db_result->column_count = 0;
        db_result->data_json = strdup("[]");
        // Note: affected_rows is handled by the caller since we don't have access to mysql_conn here
        db_result->affected_rows = 0;
        return true;
    }

    // Get field types for numeric detection
    typedef struct {
        char *name;
        char *org_name;
        char *table;
        char *org_table;
        char *db;
        char *catalog;
        char *def;
        unsigned long length;
        unsigned long max_length;
        unsigned int name_length;
        unsigned int org_name_length;
        unsigned int table_length;
        unsigned int org_table_length;
        unsigned int db_length;
        unsigned int catalog_length;
        unsigned int def_length;
        unsigned int flags;
        unsigned int decimals;
        unsigned int charsetnr;
        unsigned int type;              /* Type of field - CRITICAL for numeric detection */
        void *extension;
    } MYSQL_FIELD_COMPLETE;

    const MYSQL_FIELD_COMPLETE* fields = mysql_fetch_fields_ptr ? (const MYSQL_FIELD_COMPLETE*)mysql_fetch_fields_ptr(mysql_result) : NULL;

    // Process result metadata and data
    if (mysql_num_rows_ptr && mysql_num_fields_ptr) {
        db_result->row_count = (size_t)mysql_num_rows_ptr(mysql_result);
        db_result->column_count = (size_t)mysql_num_fields_ptr(mysql_result);

        // Extract column names
        if (db_result->column_count > 0 && fields) {
            db_result->column_names = calloc(db_result->column_count, sizeof(char*));
            if (db_result->column_names) {
                for (size_t i = 0; i < db_result->column_count; i++) {
                    // Extract actual field name from MYSQL_FIELD structure
                    if (fields[i].name) {
                        db_result->column_names[i] = strdup(fields[i].name);
                    } else {
                        // Fallback for NULL field names
                        char col_name[32];
                        snprintf(col_name, sizeof(col_name), "col_%zu", i);
                        db_result->column_names[i] = strdup(col_name);
                    }
                }
            }
        }

        // Convert result to JSON
        if (db_result->row_count > 0 && db_result->column_count > 0 && mysql_fetch_row_ptr) {
            size_t json_size = 1024 * db_result->row_count;
            db_result->data_json = calloc(1, json_size);
            if (db_result->data_json) {
                strcpy(db_result->data_json, "[");
                for (size_t row = 0; row < db_result->row_count; row++) {
                    if (row > 0) strcat(db_result->data_json, ",");

                    // MYSQL_ROW is char** (array of strings)
                    char** row_data = (char**)mysql_fetch_row_ptr(mysql_result);
                    if (row_data) {
                        strcat(db_result->data_json, "{");
                        for (size_t col = 0; col < db_result->column_count; col++) {
                            if (col > 0) strcat(db_result->data_json, ",");

                            // Build column JSON
                            const char* col_name = db_result->column_names ? db_result->column_names[col] : "unknown";

                            // Check for NULL value
                            if (row_data[col] == NULL) {
                                // Ensure buffer has room for null value
                                size_t needed = strlen(col_name) + 10; // "name":null plus comma
                                if (strlen(db_result->data_json) + needed >= json_size) {
                                    json_size = json_size * 2 + needed;
                                    char* new_json = realloc(db_result->data_json, json_size);
                                    if (!new_json) break;
                                    db_result->data_json = new_json;
                                }
                                char* pos = db_result->data_json + strlen(db_result->data_json);
                                snprintf(pos, json_size - strlen(db_result->data_json), "\"%s\":null", col_name);
                            } else {
                                // Check if column is numeric type
                                bool is_numeric = fields && mysql_is_numeric_type(fields[col].type);

                                if (is_numeric) {
                                    // Numeric types - no quotes around value (JSON number)
                                    size_t needed = strlen(col_name) + strlen(row_data[col]) + 10;
                                    if (strlen(db_result->data_json) + needed >= json_size) {
                                        json_size = json_size * 2 + needed;
                                        char* new_json = realloc(db_result->data_json, json_size);
                                        if (!new_json) break;
                                        db_result->data_json = new_json;
                                    }
                                    char* pos = db_result->data_json + strlen(db_result->data_json);
                                    snprintf(pos, json_size - strlen(db_result->data_json), "\"%s\":%s", col_name, row_data[col]);
                                } else {
                                    // String types - escape and quote the value
                                    // For large strings (like migration SQL), use dynamic allocation for escaped data
                                    size_t escaped_size = (row_data[col] ? strlen(row_data[col]) * 2 : 0) + 1;
                                    char* escaped_data = calloc(1, escaped_size);
                                    if (!escaped_data) {
                                        size_t needed = strlen(col_name) + 10;
                                        if (strlen(db_result->data_json) + needed >= json_size) {
                                            json_size = json_size * 2 + needed;
                                            char* new_json = realloc(db_result->data_json, json_size);
                                            if (!new_json) break;
                                            db_result->data_json = new_json;
                                        }
                                        char* pos = db_result->data_json + strlen(db_result->data_json);
                                        snprintf(pos, json_size - strlen(db_result->data_json), "\"%s\":null", col_name);
                                    } else {
                                        mysql_json_escape_string(row_data[col], escaped_data, escaped_size);
                                        size_t needed = strlen(col_name) + strlen(escaped_data) + 10;
                                        if (strlen(db_result->data_json) + needed >= json_size) {
                                            json_size = json_size * 2 + needed;
                                            char* new_json = realloc(db_result->data_json, json_size);
                                            if (!new_json) {
                                                free(escaped_data);
                                                break;
                                            }
                                            db_result->data_json = new_json;
                                        }
                                        char* pos = db_result->data_json + strlen(db_result->data_json);
                                        snprintf(pos, json_size - strlen(db_result->data_json), "\"%s\":\"%s\"", col_name, escaped_data);
                                        free(escaped_data);
                                    }
                                }
                            }
                        }
                        strcat(db_result->data_json, "}");
                    }
                }
                strcat(db_result->data_json, "]");

                log_this(designator, "MySQL execute_query: Generated result JSON", LOG_LEVEL_TRACE, 0);
            }
        } else {
            db_result->data_json = strdup("[]");
            log_this(designator, "MySQL execute_query: Query returned no data", LOG_LEVEL_TRACE, 0);
        }
    }

    // Free result
    if (mysql_free_result_ptr) {
        mysql_free_result_ptr(mysql_result);
    }

    return true;
}

// Helper function to process prepared statement result (non-static for testing)
bool mysql_process_prepared_result(void* mysql_result, QueryResult* db_result, void* stmt_handle, const char* designator) {
    // Set success flag (similar to mysql_process_direct_result)
    db_result->success = true;
    
    if (mysql_result) {
        // This is a SELECT query with results
        if (mysql_stmt_store_result_ptr) {
            mysql_stmt_store_result_ptr(stmt_handle);
        }

        // Get column count
        unsigned int column_count = 0;
        if (mysql_stmt_field_count_ptr) {
            column_count = mysql_stmt_field_count_ptr(stmt_handle);
        } else if (mysql_num_fields_ptr) {
            column_count = mysql_num_fields_ptr(mysql_result);
        }
        db_result->column_count = (size_t)column_count;

        // Get field types and extract column names
        typedef struct {
            char *name;
            char *org_name;
            char *table;
            char *org_table;
            char *db;
            char *catalog;
            char *def;
            unsigned long length;
            unsigned long max_length;
            unsigned int name_length;
            unsigned int org_name_length;
            unsigned int table_length;
            unsigned int org_table_length;
            unsigned int db_length;
            unsigned int catalog_length;
            unsigned int def_length;
            unsigned int flags;
            unsigned int decimals;
            unsigned int charsetnr;
            unsigned int type;              /* Type of field - CRITICAL for numeric detection */
            void *extension;
        } MYSQL_FIELD_COMPLETE;

        const MYSQL_FIELD_COMPLETE* fields = NULL;
        if (column_count > 0 && mysql_fetch_fields_ptr) {
            fields = (const MYSQL_FIELD_COMPLETE*)mysql_fetch_fields_ptr(mysql_result);
            if (fields) {
                db_result->column_names = calloc(db_result->column_count, sizeof(char*));
                if (db_result->column_names) {
                    for (size_t i = 0; i < db_result->column_count; i++) {
                        if (fields[i].name) {
                            db_result->column_names[i] = strdup(fields[i].name);
                        } else {
                            char col_name[32];
                            snprintf(col_name, sizeof(col_name), "col_%zu", i);
                            db_result->column_names[i] = strdup(col_name);
                        }
                    }
                }
            }
        }

        // Allocate buffer for fetching rows - using simple string buffers
        #define MAX_COL_SIZE 4096
        char** col_buffers = calloc(column_count, sizeof(char*));
        unsigned long* col_lengths = calloc(column_count, sizeof(unsigned long));
        char* col_is_null = calloc(column_count, sizeof(char));
        unsigned long* col_errors = calloc(column_count, sizeof(unsigned long));

        if (!col_buffers || !col_lengths || !col_is_null || !col_errors) {
            free(col_buffers);
            free(col_lengths);
            free(col_is_null);
            free(col_errors);
            if (mysql_result && mysql_free_result_ptr) {
                mysql_free_result_ptr(mysql_result);
            }
            return false;
        }

        // Allocate column buffers
        for (size_t i = 0; i < column_count; i++) {
            col_buffers[i] = calloc(1, MAX_COL_SIZE);
            if (!col_buffers[i]) {
                for (size_t j = 0; j < i; j++) {
                    free(col_buffers[j]);
                }
                free(col_buffers);
                free(col_lengths);
                free(col_is_null);
                free(col_errors);
                if (mysql_result && mysql_free_result_ptr) {
                    mysql_free_result_ptr(mysql_result);
                }
                return false;
            }
        }

        // Create MYSQL_BIND structures for result binding
        // Must match the MYSQL_BIND structure defined in query.c
        typedef struct {
            unsigned long* length;
            char* is_null;
            void* buffer;
            unsigned long* error;
            unsigned char* row_ptr;
            void (*store_param_func)(void*, void*, unsigned char**, unsigned char**);
            void (*fetch_result)(void*, unsigned int, unsigned char**);
            void (*skip_result)(void*, unsigned int, unsigned char**);
            unsigned long buffer_length;
            unsigned long offset;
            unsigned long length_value;
            unsigned int param_number;
            unsigned int pack_length;
            unsigned int buffer_type;  // MySQL type constant
            char error_value;
            char is_unsigned;
            char long_data_used;
            char is_null_value;
            void* extension;
        } MYSQL_BIND_COMPLETE;

        MYSQL_BIND_COMPLETE* bind = calloc(column_count, sizeof(MYSQL_BIND_COMPLETE));
        if (!bind) {
            for (size_t i = 0; i < column_count; i++) {
                free(col_buffers[i]);
            }
            free(col_buffers);
            free(col_lengths);
            free(col_is_null);
            free(col_errors);
            if (mysql_result && mysql_free_result_ptr) {
                mysql_free_result_ptr(mysql_result);
            }
            return false;
        }

        // Bind result columns
        for (size_t i = 0; i < column_count; i++) {
            // Initialize all fields to zero/NULL
            memset(&bind[i], 0, sizeof(MYSQL_BIND_COMPLETE));
            
            // Set required fields for result binding
            bind[i].buffer_type = 253; // MYSQL_TYPE_STRING
            bind[i].buffer = col_buffers[i];
            bind[i].buffer_length = MAX_COL_SIZE;
            bind[i].length = &col_lengths[i];     // Point to length indicator array
            bind[i].is_null = &col_is_null[i];    // Point to is_null array element
            bind[i].error = &col_errors[i];       // Point to error indicator array
        }

        if (mysql_stmt_bind_result_ptr && mysql_stmt_bind_result_ptr(stmt_handle, bind) != 0) {
            log_this(designator, "MySQL prepared statement bind result failed", LOG_LEVEL_ERROR, 0);
            free(bind);
            for (size_t i = 0; i < column_count; i++) {
                free(col_buffers[i]);
            }
            free(col_buffers);
            free(col_lengths);
            free(col_is_null);
            if (mysql_result && mysql_free_result_ptr) {
                mysql_free_result_ptr(mysql_result);
            }
            return false;
        }

        // Fetch all rows into JSON
        size_t json_size = 8192;
        db_result->data_json = calloc(1, json_size);
        if (db_result->data_json) {
            strcpy(db_result->data_json, "[");
            size_t row_count = 0;

            while (mysql_stmt_fetch_ptr && mysql_stmt_fetch_ptr(stmt_handle) == 0) {
                if (row_count > 0) strcat(db_result->data_json, ",");

                // Expand JSON buffer if needed
                if (strlen(db_result->data_json) + 4096 > json_size) {
                    json_size *= 2;
                    char* new_json = realloc(db_result->data_json, json_size);
                    if (!new_json) break;
                    db_result->data_json = new_json;
                }

                strcat(db_result->data_json, "{");
                for (size_t col = 0; col < column_count; col++) {
                    if (col > 0) strcat(db_result->data_json, ",");

                    char col_json[MAX_COL_SIZE + 256];
                    const char* col_name = db_result->column_names ? db_result->column_names[col] : "unknown";

                    if (col_is_null[col]) {
                        snprintf(col_json, sizeof(col_json), "\"%s\":null", col_name);
                    } else {
                        // Check if column is numeric type
                        bool is_numeric = fields && mysql_is_numeric_type(fields[col].type);

                        if (is_numeric) {
                            // Numeric types - no quotes around value (JSON number)
                            snprintf(col_json, sizeof(col_json), "\"%s\":%s", col_name, col_buffers[col]);
                        } else {
                            // String types - escape and quote the value
                            char escaped_data[MAX_COL_SIZE];
                            mysql_json_escape_string(col_buffers[col], escaped_data, sizeof(escaped_data));
                            snprintf(col_json, sizeof(col_json), "\"%s\":\"%s\"", col_name, escaped_data);
                        }
                    }

                    strcat(db_result->data_json, col_json);
                }
                strcat(db_result->data_json, "}");
                row_count++;
            }
            strcat(db_result->data_json, "]");
            db_result->row_count = row_count;
        }

        // Cleanup
        free(bind);
        for (size_t i = 0; i < column_count; i++) {
            free(col_buffers[i]);
        }
        free(col_buffers);
        free(col_lengths);
        free(col_is_null);
        free(col_errors);

        if (mysql_stmt_free_result_ptr) {
            mysql_stmt_free_result_ptr(stmt_handle);
        }
        if (mysql_result && mysql_free_result_ptr) {
            mysql_free_result_ptr(mysql_result);
        }
    } else {
        // No result set (e.g., INSERT, UPDATE, DELETE)
        db_result->row_count = 0;
        db_result->column_count = 0;
        db_result->data_json = strdup("[]");

        // Get affected rows
        if (mysql_stmt_affected_rows_ptr) {
            unsigned long long affected = mysql_stmt_affected_rows_ptr(stmt_handle);
            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wconversion"
            db_result->affected_rows = (size_t)affected;
            #pragma GCC diagnostic pop
        } else {
            db_result->affected_rows = 0;
        }
    }

    return true;
}

// New streamlined helper for processing prepared statement results
// This consolidates the repetitive MYSQL_BIND result binding and fetching code
bool mysql_process_prepared_stmt_result(void* stmt, QueryResult* result, const char* designator) {
    if (!stmt || !result) {
        return false;
    }

    // Get result metadata
    void* mysql_result = NULL;
    if (mysql_stmt_result_metadata_ptr) {
        mysql_result = mysql_stmt_result_metadata_ptr(stmt);
    }

    // Use existing mysql_process_prepared_result helper
    return mysql_process_prepared_result(mysql_result, result, stmt, designator);
}

// New streamlined helper for processing direct query results
// This consolidates the direct execution result processing code
bool mysql_process_direct_result(void* mysql_conn, void* mysql_result, QueryResult* result, const char* designator) {
    if (!result) {
        return false;
    }

    result->success = true;

    if (!mysql_result) {
        // No result set (INSERT/UPDATE/DELETE)
        result->row_count = 0;
        result->column_count = 0;
        result->data_json = strdup("[]");
        
        if (mysql_affected_rows_ptr && mysql_conn) {
            unsigned long long affected = mysql_affected_rows_ptr(mysql_conn);
            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wconversion"
            result->affected_rows = (size_t)affected;
            #pragma GCC diagnostic pop
        } else {
            result->affected_rows = 0;
        }
        return true;
    }

    // Use existing mysql_process_query_result helper
    return mysql_process_query_result(mysql_result, result, designator);
}