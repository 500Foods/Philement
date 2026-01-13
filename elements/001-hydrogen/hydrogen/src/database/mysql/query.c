/*
  * MySQL Database Engine - Query Execution Implementation
  *
  * Implements MySQL query execution functions.
  */

// Project includes
#include <src/hydrogen.h>
#include <src/database/database.h>
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/database_params.h>

// Local includes
#include "types.h"
#include "connection.h"
#include "query.h"
#include "query_helpers.h"
#include "utils.h"

// External declarations for libmysqlclient function pointers (defined in connection.c)
extern mysql_store_result_t mysql_store_result_ptr;
extern mysql_num_rows_t mysql_num_rows_ptr;
extern mysql_num_fields_t mysql_num_fields_ptr;
extern mysql_fetch_row_t mysql_fetch_row_ptr;
extern mysql_fetch_fields_t mysql_fetch_fields_ptr;
extern mysql_free_result_t mysql_free_result_ptr;
extern mysql_error_t mysql_error_ptr;
extern mysql_query_t mysql_query_ptr;
extern mysql_affected_rows_t mysql_affected_rows_ptr;
extern mysql_stmt_execute_t mysql_stmt_execute_ptr;
extern mysql_stmt_result_metadata_t mysql_stmt_result_metadata_ptr;
extern mysql_stmt_fetch_t mysql_stmt_fetch_ptr;
extern mysql_stmt_error_t mysql_stmt_error_ptr;
extern mysql_stmt_affected_rows_t mysql_stmt_affected_rows_ptr;
extern mysql_stmt_store_result_t mysql_stmt_store_result_ptr;
extern mysql_stmt_free_result_t mysql_stmt_free_result_ptr;
extern mysql_stmt_field_count_t mysql_stmt_field_count_ptr;

/*
 * Helper Functions
 */

// Helper function to cleanup column names
void mysql_cleanup_column_names(char** column_names, size_t column_count) {
    if (column_names) {
        for (size_t i = 0; i < column_count; i++) {
            free(column_names[i]);
        }
        free(column_names);
    }
}

/*
 * Query Execution
 */

bool mysql_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result) {
    if (!connection || !request || !result || connection->engine_type != DB_ENGINE_MYSQL) {
        const char* designator = connection ? (connection->designator ? connection->designator : SR_DATABASE) : SR_DATABASE;
        log_this(designator, "MySQL execute_query: Invalid parameters", LOG_LEVEL_ERROR, 0);
        return false;
    }

    const char* designator = connection->designator ? connection->designator : SR_DATABASE;
    log_this(designator, "mysql_execute_query: ENTER - connection=%p, request=%p, result=%p", LOG_LEVEL_TRACE, 3, (void*)connection, (void*)request, (void*)result);

    log_this(designator, "mysql_execute_query: Parameters validated, proceeding", LOG_LEVEL_TRACE, 0);

    // cppcheck-suppress constVariablePointer
    // Justification: MySQL API requires non-const MYSQL* connection handle
    const MySQLConnection* mysql_conn = (const MySQLConnection*)connection->connection_handle;
    if (!mysql_conn || !mysql_conn->connection) {
        log_this(designator, "MySQL execute_query: Invalid connection handle", LOG_LEVEL_ERROR, 0);
        return false;
    }

    log_this(designator, "MySQL execute_query: Executing query: %s", LOG_LEVEL_TRACE, 1, request->sql_template);

    // Execute query
    if (mysql_query_ptr(mysql_conn->connection, request->sql_template) != 0) {
        log_this(designator, "MySQL query execution failed", LOG_LEVEL_TRACE, 0);
        if (mysql_error_ptr) {
            const char* error_msg = mysql_error_ptr(mysql_conn->connection);
            if (error_msg && strlen(error_msg) > 0) {
                log_this(designator, "MySQL query error: %s", LOG_LEVEL_TRACE, 1, error_msg);
            }
        }
        return false;
    }

    // Store result
    void* mysql_result = NULL;
    if (mysql_store_result_ptr) {
        mysql_result = mysql_store_result_ptr(mysql_conn->connection);
        if (!mysql_result) {
            log_this(designator, "MySQL execute_query: No result set returned", LOG_LEVEL_DEBUG, 0);
        }
    }

    // Create result structure
    QueryResult* db_result = calloc(1, sizeof(QueryResult));
    if (!db_result) {
        if (mysql_result && mysql_free_result_ptr) {
            mysql_free_result_ptr(mysql_result);
        }
        return false;
    }

    db_result->success = true;
    // db_result->execution_time_ms is now set by the engine abstraction layer

    // Get result metadata
    if (mysql_result) {
        if (mysql_num_rows_ptr && mysql_num_fields_ptr) {
            db_result->row_count = (size_t)mysql_num_rows_ptr(mysql_result);
            db_result->column_count = (size_t)mysql_num_fields_ptr(mysql_result);

            // Complete MYSQL_FIELD structure definition to match libmysqlclient
            // This ensures correct memory alignment and array indexing
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
            
            // Get fields for type checking (declared at broader scope for use in row loop)
            const MYSQL_FIELD_COMPLETE* fields = NULL;
            if (mysql_fetch_fields_ptr) {
                fields = (const MYSQL_FIELD_COMPLETE*)mysql_fetch_fields_ptr(mysql_result);
            }

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
                // Increase buffer size to handle large query templates (migration SQL can be 10KB+)
                size_t json_size = 16384 * db_result->row_count; // Larger estimate for query templates
                db_result->data_json = calloc(1, json_size);
                if (db_result->data_json) {
                    strcpy(db_result->data_json, "[");
                    for (size_t row = 0; row < db_result->row_count; row++) {
                        if (row > 0) strcat(db_result->data_json, ",");

                        // Track start of this row for debug logging
                        size_t row_start_pos = strlen(db_result->data_json);

                        // MYSQL_ROW is char** (array of strings)
                        char** row_data = (char**)mysql_fetch_row_ptr(mysql_result);
                        if (row_data) {
                            strcat(db_result->data_json, "{");
                            for (size_t col = 0; col < db_result->column_count; col++) {
                                if (col > 0) strcat(db_result->data_json, ",");
                                
                                // Get column type to determine if we should quote the value
                                bool is_numeric = false;
                                if (fields && mysql_fetch_fields_ptr) {
                                    is_numeric = mysql_is_numeric_type(fields[col].type);
                                }
                                
                                const char* col_name = db_result->column_names ? db_result->column_names[col] : "unknown";
                                const char* value = row_data[col];
                                
                                // Calculate needed size for this column (name + value + escaping overhead)
                                size_t value_len = value ? strlen(value) : 0;
                                size_t needed_size = strlen(col_name) + (value_len * 2) + 20; // Extra for escaping
                                
                                // Ensure buffer has enough space
                                size_t current_len = strlen(db_result->data_json);
                                if (current_len + needed_size > json_size) {
                                    json_size = (current_len + needed_size) * 2;
                                    char* new_json = realloc(db_result->data_json, json_size);
                                    if (!new_json) continue; // Skip column on allocation failure
                                    db_result->data_json = new_json;
                                }
                                
                                // Append column to JSON
                                char* append_pos = db_result->data_json + current_len;
                                
                                if (value == NULL) {
                                    // NULL value
                                    sprintf(append_pos, "\"%s\":null", col_name);
                                } else if (is_numeric && strlen(value) > 0) {
                                    // Numeric type - no quotes around value
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
                            }
                            strcat(db_result->data_json, "}");
                            
                            // Debug: Log first row JSON for diagnosis
                            if (row == 0) {
                                size_t row_len = strlen(db_result->data_json) - row_start_pos;
                                char* row_json = strndup(db_result->data_json + row_start_pos, row_len);
                                if (row_json) {
                                    log_this(designator, "MySQL first row JSON: %s", LOG_LEVEL_DEBUG, 1, row_json);
                                    free(row_json);
                                }
                            }
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
    } else {
        // No result set (e.g., INSERT, UPDATE, DELETE)
        db_result->row_count = 0;
        db_result->column_count = 0;
        db_result->data_json = strdup("[]");
        // Get affected rows from mysql_affected_rows
        if (mysql_affected_rows_ptr) {
            // mysql_affected_rows returns unsigned long long, cast to size_t safely
            unsigned long long affected = mysql_affected_rows_ptr(mysql_conn->connection);
            // Suppress conversion warning - we're explicitly handling the potential truncation
            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wconversion"
            db_result->affected_rows = (size_t)affected;
            #pragma GCC diagnostic pop
        } else {
            db_result->affected_rows = 0;
        }
    }

    *result = db_result;

    log_this(designator, "MySQL execute_query: Query completed successfully", LOG_LEVEL_DEBUG, 0);
    return true;
}

bool mysql_execute_prepared(DatabaseHandle* connection, const PreparedStatement* stmt, QueryRequest* request, QueryResult** result) {
    if (!connection || !stmt || !request || !result || connection->engine_type != DB_ENGINE_MYSQL) {
        const char* designator = connection ? (connection->designator ? connection->designator : SR_DATABASE) : SR_DATABASE;
        log_this(designator, "MySQL execute_prepared: Invalid parameters", LOG_LEVEL_ERROR, 0);
        return false;
    }

    const char* designator = connection->designator ? connection->designator : SR_DATABASE;
    log_this(designator, "mysql_execute_prepared: ENTER - connection=%p, stmt=%p, request=%p, result=%p", LOG_LEVEL_TRACE, 4, (void*)connection, (void*)stmt, (void*)request, (void*)result);

    // cppcheck-suppress constVariablePointer
    // Justification: MySQL API requires non-const MYSQL* connection handle
    MySQLConnection* mysql_conn = (MySQLConnection*)connection->connection_handle;
    if (!mysql_conn || !mysql_conn->connection) {
        log_this(designator, "MySQL execute_prepared: Invalid connection handle", LOG_LEVEL_ERROR, 0);
        return false;
    }

    // Get the prepared statement handle
    void* stmt_handle = stmt->engine_specific_handle;
    if (!stmt_handle) {
        // Statement had no executable SQL (e.g., only comments after macro processing)
        // Return successful empty result instead of error
        log_this(designator, "MySQL prepared statement: No executable SQL (statement was not actionable)", LOG_LEVEL_DEBUG, 0);
        
        QueryResult* db_result = calloc(1, sizeof(QueryResult));
        if (!db_result) {
            return false;
        }
        
        db_result->success = true;
        db_result->row_count = 0;
        db_result->column_count = 0;
        db_result->affected_rows = 0;
        db_result->execution_time_ms = 0;
        db_result->data_json = strdup("[]");
        
        *result = db_result;
        return true;
    }

    // Check if required functions are available
    if (!mysql_stmt_execute_ptr) {
        log_this(designator, "MySQL execute_prepared: mysql_stmt_execute function not available", LOG_LEVEL_ERROR, 0);
        return false;
    }

    log_this(designator, "MySQL execute_prepared: Executing prepared statement", LOG_LEVEL_TRACE, 0);

    // Execute the prepared statement
    if (mysql_stmt_execute_ptr(stmt_handle) != 0) {
        log_this(designator, "MySQL prepared statement execution failed", LOG_LEVEL_ERROR, 0);
        if (mysql_stmt_error_ptr) {
            const char* error_msg = mysql_stmt_error_ptr(stmt_handle);
            if (error_msg && strlen(error_msg) > 0) {
                log_this(designator, "MySQL prepared statement error: %s", LOG_LEVEL_ERROR, 1, error_msg);
            }
        }
        return false;
    }

    // Create result structure
    QueryResult* db_result = calloc(1, sizeof(QueryResult));
    if (!db_result) {
        return false;
    }

    db_result->success = true;
    // db_result->execution_time_ms is now set by the engine abstraction layer

    // Get result metadata
    void* mysql_result = NULL;
    if (mysql_stmt_result_metadata_ptr) {
        mysql_result = mysql_stmt_result_metadata_ptr(stmt_handle);
    }

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

        // Complete MYSQL_FIELD structure definition to match libmysqlclient
        // This ensures correct memory alignment and array indexing
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
        
        // Get fields for type checking (declared at broader scope for use in row loop)
        const MYSQL_FIELD_COMPLETE* fields = NULL;
        if (mysql_fetch_fields_ptr) {
            fields = (const MYSQL_FIELD_COMPLETE*)mysql_fetch_fields_ptr(mysql_result);
        }

        // Extract column names
        if (column_count > 0 && fields) {
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
            free(db_result->column_names);
            free(db_result);
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
                free(db_result->column_names);
                free(db_result);
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
            free(db_result->column_names);
            free(db_result);
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
            free(db_result->column_names);
            free(db_result);
            return false;
        }

        // Fetch all rows into JSON
        size_t json_size = 16384; // Larger initial size for query templates
        db_result->data_json = calloc(1, json_size);
        if (db_result->data_json) {
            strcpy(db_result->data_json, "[");
            size_t row_count = 0;

            while (mysql_stmt_fetch_ptr && mysql_stmt_fetch_ptr(stmt_handle) == 0) {
                if (row_count > 0) strcat(db_result->data_json, ",");

                strcat(db_result->data_json, "{");
                for (size_t col = 0; col < column_count; col++) {
                    if (col > 0) strcat(db_result->data_json, ",");
                    
                    // Get column type to determine if we should quote the value
                    bool is_numeric = false;
                    if (fields && mysql_fetch_fields_ptr) {
                        is_numeric = mysql_is_numeric_type(fields[col].type);
                    }
                    
                    const char* col_name = db_result->column_names ? db_result->column_names[col] : "unknown";
                    const char* value = col_buffers[col];
                    
                    // Calculate needed size for this column (name + value + escaping overhead)
                    size_t value_len = col_is_null[col] ? 0 : strlen(value);
                    size_t needed_size = strlen(col_name) + (value_len * 2) + 20;
                    
                    // Ensure buffer has enough space
                    size_t current_len = strlen(db_result->data_json);
                    if (current_len + needed_size > json_size) {
                        json_size = (current_len + needed_size) * 2;
                        char* new_json = realloc(db_result->data_json, json_size);
                        if (!new_json) break;
                        db_result->data_json = new_json;
                    }
                    
                    // Append column to JSON
                    char* append_pos = db_result->data_json + current_len;
                    
                    if (col_is_null[col]) {
                        sprintf(append_pos, "\"%s\":null", col_name);
                    } else if (is_numeric && strlen(value) > 0) {
                        // Numeric type - no quotes around value
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

    *result = db_result;

    log_this(designator, "MySQL execute_prepared: Query completed successfully", LOG_LEVEL_TRACE, 0);
    return true;
}