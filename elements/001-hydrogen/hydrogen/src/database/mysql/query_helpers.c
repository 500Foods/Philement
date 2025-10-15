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

// External declarations for libmysqlclient function pointers
extern mysql_fetch_fields_t mysql_fetch_fields_ptr;

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
                char col_json[1024];
                const char* col_name = column_names ? column_names[col] : "unknown";

                // Check for NULL value
                if (row_data[col] == NULL) {
                    snprintf(col_json, sizeof(col_json), "\"%s\":null", col_name);
                } else {
                    // Escape the data for JSON
                    char escaped_data[512] = {0};
                    // Call the existing mysql_json_escape_string function directly
                    mysql_json_escape_string(row_data[col], escaped_data, sizeof(escaped_data));
                    snprintf(col_json, sizeof(col_json), "\"%s\":\"%s\"", col_name, escaped_data);
                }

                strcat(*json_buffer, col_json);
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
        log_this(designator, "MySQL query execution failed", LOG_LEVEL_ERROR, 0);
        if (mysql_error_ptr) {
            const char* error_msg = mysql_error_ptr(mysql_connection);
            if (error_msg && strlen(error_msg) > 0) {
                log_this(designator, "MySQL query error: %s", LOG_LEVEL_ERROR, 1, error_msg);
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

    // Process result metadata and data
    if (mysql_num_rows_ptr && mysql_num_fields_ptr) {
        db_result->row_count = (size_t)mysql_num_rows_ptr(mysql_result);
        db_result->column_count = (size_t)mysql_num_fields_ptr(mysql_result);

        // Extract column names
        if (db_result->column_count > 0 && mysql_fetch_fields_ptr) {
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
            if (fields) {
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
                            char col_json[1024];
                            const char* col_name = db_result->column_names ? db_result->column_names[col] : "unknown";

                            // Check for NULL value
                            if (row_data[col] == NULL) {
                                snprintf(col_json, sizeof(col_json), "\"%s\":null", col_name);
                            } else {
                                // Escape the data for JSON
                                char escaped_data[512] = {0};
                                mysql_json_escape_string(row_data[col], escaped_data, sizeof(escaped_data));
                                snprintf(col_json, sizeof(col_json), "\"%s\":\"%s\"", col_name, escaped_data);
                            }

                            strcat(db_result->data_json, col_json);
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

        // Extract column names
        if (column_count > 0 && mysql_fetch_fields_ptr) {
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
        long* col_lengths = calloc(column_count, sizeof(long));
        char* col_is_null = calloc(column_count, sizeof(char));

        if (!col_buffers || !col_lengths || !col_is_null) {
            free(col_buffers);
            free(col_lengths);
            free(col_is_null);
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
                if (mysql_result && mysql_free_result_ptr) {
                    mysql_free_result_ptr(mysql_result);
                }
                return false;
            }
        }

        // Create MYSQL_BIND structures for result binding
        typedef struct {
            unsigned long length;
            char is_null;
            char error;
            void* buffer;
            unsigned long buffer_length;
            unsigned int buffer_type;  // enum_field_types
        } MYSQL_BIND_WRAPPER;

        MYSQL_BIND_WRAPPER* bind = calloc(column_count, sizeof(MYSQL_BIND_WRAPPER));
        if (!bind) {
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

        // Bind result columns
        for (size_t i = 0; i < column_count; i++) {
            bind[i].buffer_type = 253; // MYSQL_TYPE_STRING
            bind[i].buffer = col_buffers[i];
            bind[i].buffer_length = MAX_COL_SIZE;
            bind[i].length = (unsigned long)col_lengths[i];
            bind[i].is_null = col_is_null[i];
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
                        char escaped_data[MAX_COL_SIZE];
                        mysql_json_escape_string(col_buffers[col], escaped_data, sizeof(escaped_data));
                        snprintf(col_json, sizeof(col_json), "\"%s\":\"%s\"", col_name, escaped_data);
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