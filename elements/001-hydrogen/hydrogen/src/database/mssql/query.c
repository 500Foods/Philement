// MSSQL Database Engine - Query Execution Implementation
// Implements MSSQL query execution functions.

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

// External declarations for ODBC function pointers (defined in connection.c)
extern SQLAllocHandle_t mssql_SQLAllocHandle_ptr;
extern SQLExecDirect_t mssql_SQLExecDirect_ptr;
extern SQLExecute_t mssql_SQLExecute_ptr;
extern SQLFreeHandle_t mssql_SQLFreeHandle_ptr;
extern SQLFreeStmt_t mssql_SQLFreeStmt_ptr;
extern SQLGetDiagRec_t mssql_SQLGetDiagRec_ptr;
extern SQLPrepare_t mssql_SQLPrepare_ptr;
extern SQLBindParameter_t mssql_SQLBindParameter_ptr;
extern SQLEndTran_t mssql_SQLEndTran_ptr;

// Helper function to trim trailing whitespace from strings (MSSQL-specific)
char* mssql_trim_trailing_whitespace(char* str) {
    if (!str) return NULL;

    // Find the end of the string, Move backwards from the end, removing whitespace
    char* end = str + strlen(str) - 1;
    while (end >= str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        *end = '\0';
        end--;
    }

    return str;
}

// Helper function to format MSSQL datetime strings to standard format (MSSQL-specific)
char* mssql_format_datetime_string(char* str) {
    if (!str) return NULL;

    // MSSQL datetime format: "2023-12-25 14:30:00.000000"
    // Standard format: "2023-12-25 14:30:00" (remove decimal point and microseconds)

    size_t len = strlen(str);
    // Look for the pattern YYYY-MM-DD HH:MM:SS. (19 chars + decimal), Truncate at the decimal point to get "YYYY-MM-DD HH:MM:SS"
    if (len >= 20 && str[10] == ' ' && str[19] == '.') {
        str[19] = '\0';
    }

    return str;
}

bool mssql_normalize_iso8601_timestamp(const char* input, char* output, size_t output_size) {
    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int second = 0;
    int written;

    if (!input || !output || output_size < 20) {
        return false;
    }
    if (sscanf(input, "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &minute, &second) != 6) {
        return false;
    }
    if (year < 1 || month < 1 || month > 12 || day < 1 || day > 31 ||
        hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 60) {
        return false;
    }
    written = snprintf(output, output_size, "%04d-%02d-%02d %02d:%02d:%02d",
                       year, month, day, hour, minute, second);
    return written == 19;
}

void mssql_complete_standalone_statement(DatabaseHandle* connection, bool success) {
    MSSQLConnection* mssql_conn;
    int end_result;

    if (!connection || connection->current_transaction || !mssql_SQLEndTran_ptr) {
        return;
    }
    mssql_conn = (MSSQLConnection*)connection->connection_handle;
    if (!mssql_conn || !mssql_conn->connection) {
        return;
    }
    end_result = mssql_SQLEndTran_ptr(SQL_HANDLE_DBC, mssql_conn->connection,
                                success ? SQL_COMMIT : SQL_ROLLBACK);
    if (end_result != SQL_SUCCESS && end_result != SQL_SUCCESS_WITH_INFO) {
        const char* designator = connection->designator ? connection->designator : SR_DATABASE;
        log_this(designator, "MSSQL standalone %s failed: %d", LOG_LEVEL_ERROR, 2,
                 success ? "commit" : "rollback", end_result);
    }
}

// Helper function to format MSSQL timestamp strings to standard format (MSSQL-specific)
char* mssql_format_timestamp_string(char* str) {
    if (!str) return NULL;

    // MSSQL timestamp format: "2023-12-25 14:30:00.000032"
    // Standard format: "2023-12-25 14:30:00.000" (truncate to milliseconds)

    size_t len = strlen(str);
    if (len >= 23 && str[10] == ' ' && str[19] == '.') {
        // Truncate after 3 decimal places (milliseconds)
        if (len > 23) {
            str[23] = '\0';
        }
    }

    return str;
}


// Bind one TypedParameter via SQLBindParameter (1-based index).
// INTEGER/BOOLEAN/FLOAT use native C types; STRING/TEXT as CHAR/LONGVARCHAR;
// DATE/TIME/DATETIME/TIMESTAMP parse ISO strings into SQL_*_STRUCT buffers stored in bound_values for cleanup after execute.

bool mssql_bind_single_parameter(void* stmt_handle, unsigned short param_index, TypedParameter* param,
                                       void** bound_values, long* str_len_indicators, const char* designator) {
    if (!stmt_handle || !param || !bound_values || !str_len_indicators || !designator) {
        return false;
    }

    if (!mssql_SQLBindParameter_ptr) {
        log_this(designator, "SQLBindParameter function not available", LOG_LEVEL_ERROR, 0);
        return false;
    }

    int bind_result = SQL_SUCCESS;

    log_this(designator, "Binding parameter %u: name=%s, type=%d", LOG_LEVEL_TRACE, 3,
             (unsigned int)param_index, param->name, param->type);

    if (param->is_null) {
        bound_values[param_index - 1] = NULL;
        str_len_indicators[param_index - 1] = SQL_NULL_DATA;
        log_this(designator, "Binding NULL parameter %u: name=%s", LOG_LEVEL_TRACE, 2,
                 (unsigned int)param_index, param->name);
        bind_result = mssql_SQLBindParameter_ptr(stmt_handle, param_index, SQL_PARAM_INPUT,
                                           SQL_C_CHAR, SQL_VARCHAR, 1, 0,
                                           NULL, 0,
                                           &str_len_indicators[param_index - 1]);
        return bind_result == SQL_SUCCESS || bind_result == SQL_SUCCESS_WITH_INFO;
    }

    switch (param->type) {
        case PARAM_TYPE_INTEGER: {
            bound_values[param_index - 1] = malloc(sizeof(int));
            if (!bound_values[param_index - 1]) return false;
            *(int*)bound_values[param_index - 1] = (int)param->value.int_value;
            str_len_indicators[param_index - 1] = 0;
            log_this(designator, "Binding INTEGER parameter %u: value=%d", LOG_LEVEL_TRACE, 2,
                     (unsigned int)param_index, (int)param->value.int_value);
            bind_result = mssql_SQLBindParameter_ptr(stmt_handle, param_index, SQL_PARAM_INPUT,
                                               SQL_C_LONG, SQL_INTEGER, 0, 0,
                                               bound_values[param_index - 1], 0,
                                               &str_len_indicators[param_index - 1]);
            break;
        }
        case PARAM_TYPE_STRING: {
            char normalized[32];
            const char* bind_str = param->value.string_value ? param->value.string_value : "";
            size_t str_len;
            if (param->value.string_value &&
                mssql_normalize_iso8601_timestamp(param->value.string_value, normalized, sizeof(normalized))) {
                bind_str = normalized;
            }
            str_len = strlen(bind_str);
            bound_values[param_index - 1] = strdup(bind_str);
            if (!bound_values[param_index - 1]) return false;
            str_len_indicators[param_index - 1] = (long)str_len;
            log_this(designator, "Binding STRING parameter %u: value='%s', len=%zu", LOG_LEVEL_TRACE, 3,
                     (unsigned int)param_index, (char*)bound_values[param_index - 1], str_len);
            bind_result = mssql_SQLBindParameter_ptr(stmt_handle, param_index, SQL_PARAM_INPUT,
                                               SQL_C_CHAR, SQL_CHAR, str_len > 0 ? str_len : 1, 0,
                                               bound_values[param_index - 1], (long)(str_len + 1),
                                               &str_len_indicators[param_index - 1]);
            break;
        }
        case PARAM_TYPE_BOOLEAN: {
            bound_values[param_index - 1] = malloc(sizeof(short));
            if (!bound_values[param_index - 1]) return false;
            *(short*)bound_values[param_index - 1] = param->value.bool_value ? 1 : 0;
            str_len_indicators[param_index - 1] = 0;
            log_this(designator, "Binding BOOLEAN parameter %u: value=%d", LOG_LEVEL_TRACE, 2,
                     (unsigned int)param_index, param->value.bool_value ? 1 : 0);
            bind_result = mssql_SQLBindParameter_ptr(stmt_handle, param_index, SQL_PARAM_INPUT,
                                               SQL_C_SHORT, SQL_SMALLINT, 0, 0,
                                               bound_values[param_index - 1], 0,
                                               &str_len_indicators[param_index - 1]);
            break;
        }
        case PARAM_TYPE_FLOAT: {
            bound_values[param_index - 1] = malloc(sizeof(double));
            if (!bound_values[param_index - 1]) return false;
            *(double*)bound_values[param_index - 1] = param->value.float_value;
            str_len_indicators[param_index - 1] = 0;
            log_this(designator, "Binding FLOAT parameter %u: value=%f", LOG_LEVEL_TRACE, 2,
                     (unsigned int)param_index, param->value.float_value);
            bind_result = mssql_SQLBindParameter_ptr(stmt_handle, param_index, SQL_PARAM_INPUT,
                                               SQL_C_DOUBLE, SQL_DOUBLE, 0, 0,
                                               bound_values[param_index - 1], 0,
                                               &str_len_indicators[param_index - 1]);
            break;
        }
        case PARAM_TYPE_TEXT: {
            size_t text_len = param->value.text_value ? strlen(param->value.text_value) : 0;
            bound_values[param_index - 1] = param->value.text_value ? strdup(param->value.text_value) : strdup("");
            if (!bound_values[param_index - 1]) return false;
            str_len_indicators[param_index - 1] = (long)text_len;
            log_this(designator, "Binding TEXT parameter %u: len=%zu", LOG_LEVEL_TRACE, 2,
                     (unsigned int)param_index, text_len);
            bind_result = mssql_SQLBindParameter_ptr(stmt_handle, param_index, SQL_PARAM_INPUT,
                                               SQL_C_CHAR, SQL_LONGVARCHAR, text_len > 0 ? text_len : 1, 0,
                                               bound_values[param_index - 1], (long)(text_len + 1),
                                               &str_len_indicators[param_index - 1]);
            break;
        }
        case PARAM_TYPE_DATE: {
            // DATE_STRUCT: year, month, day
            SQL_DATE_STRUCT* date_struct = malloc(sizeof(SQL_DATE_STRUCT));
            if (!date_struct) return false;

            // Parse date string (YYYY-MM-DD format)
            const char* date_value = param->value.date_value ? param->value.date_value : "1970-01-01";
            int year = 0, month = 0, day = 0;
            if (sscanf(date_value, "%d-%d-%d", &year, &month, &day) != 3) {
                log_this(designator, "Invalid DATE format (expected YYYY-MM-DD): %s", LOG_LEVEL_ERROR, 1, date_value);
                free(date_struct);
                return false;
            }

            date_struct->year = (short)year;
            date_struct->month = (unsigned short)month;
            date_struct->day = (unsigned short)day;

            bound_values[param_index - 1] = date_struct;
            str_len_indicators[param_index - 1] = 0;
            log_this(designator, "Binding DATE parameter %u: %04d-%02d-%02d", LOG_LEVEL_TRACE, 4,
                     (unsigned int)param_index, year, month, day);
            bind_result = mssql_SQLBindParameter_ptr(stmt_handle, param_index, SQL_PARAM_INPUT,
                                               SQL_C_TYPE_DATE, SQL_TYPE_DATE, 0, 0,
                                               bound_values[param_index - 1], 0,
                                               &str_len_indicators[param_index - 1]);
            break;
        }
        case PARAM_TYPE_TIME: {
            // TIME_STRUCT: hour, minute, second
            SQL_TIME_STRUCT* time_struct = malloc(sizeof(SQL_TIME_STRUCT));
            if (!time_struct) return false;

            // Parse time string (HH:MM:SS format)
            const char* time_value = param->value.time_value ? param->value.time_value : "00:00:00";
            int hour = 0, minute = 0, second = 0;
            if (sscanf(time_value, "%d:%d:%d", &hour, &minute, &second) != 3) {
                log_this(designator, "Invalid TIME format (expected HH:MM:SS): %s", LOG_LEVEL_ERROR, 1, time_value);
                free(time_struct);
                return false;
            }

            time_struct->hour = (unsigned short)hour;
            time_struct->minute = (unsigned short)minute;
            time_struct->second = (unsigned short)second;

            bound_values[param_index - 1] = time_struct;
            str_len_indicators[param_index - 1] = 0;
            log_this(designator, "Binding TIME parameter %u: %02d:%02d:%02d", LOG_LEVEL_TRACE, 4,
                     (unsigned int)param_index, hour, minute, second);
            bind_result = mssql_SQLBindParameter_ptr(stmt_handle, param_index, SQL_PARAM_INPUT,
                                               SQL_C_TYPE_TIME, SQL_TYPE_TIME, 0, 0,
                                               bound_values[param_index - 1], 0,
                                               &str_len_indicators[param_index - 1]);
            break;
        }
        case PARAM_TYPE_DATETIME: {
            // Validate DATETIME format (YYYY-MM-DD HH:MM:SS)
            const char* datetime_value = param->value.datetime_value ? param->value.datetime_value : "";
            char normalized[32];
            int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
            if (mssql_normalize_iso8601_timestamp(datetime_value, normalized, sizeof(normalized))) {
                datetime_value = normalized;
            } else if (sscanf(datetime_value, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second) != 6) {
                log_this(designator, "Invalid DATETIME format (expected YYYY-MM-DD HH:MM:SS): %s", LOG_LEVEL_ERROR, 1, datetime_value);
                return false;
            }

            // Bind as string for ODBC compatibility
            size_t str_len = strlen(datetime_value);
            bound_values[param_index - 1] = strdup(datetime_value);
            if (!bound_values[param_index - 1]) return false;
            str_len_indicators[param_index - 1] = (long)str_len;
            log_this(designator, "Binding DATETIME parameter %u as string: value='%s', len=%zu", LOG_LEVEL_TRACE, 3,
                      (unsigned int)param_index, (char*)bound_values[param_index - 1], str_len);
            bind_result = mssql_SQLBindParameter_ptr(stmt_handle, param_index, SQL_PARAM_INPUT,
                                                SQL_C_CHAR, SQL_TYPE_TIMESTAMP, str_len > 0 ? str_len : 1, 0,
                                                bound_values[param_index - 1], (long)(str_len + 1),
                                                &str_len_indicators[param_index - 1]);
            break;
        }
        case PARAM_TYPE_TIMESTAMP: {
            // Validate TIMESTAMP format (YYYY-MM-DD HH:MM:SS.FFF)
            const char* timestamp_value = param->value.timestamp_value ? param->value.timestamp_value : "";
            char normalized[32];
            int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
            float fraction = 0.0f;
            if (mssql_normalize_iso8601_timestamp(timestamp_value, normalized, sizeof(normalized))) {
                timestamp_value = normalized;
            } else if (sscanf(timestamp_value, "%d-%d-%d %d:%d:%d.%f", &year, &month, &day, &hour, &minute, &second, &fraction) != 7) {
                log_this(designator, "Invalid TIMESTAMP format (expected YYYY-MM-DD HH:MM:SS.FFF): %s", LOG_LEVEL_ERROR, 1, timestamp_value);
                return false;
            }

            // Bind as string for ODBC compatibility
            size_t str_len = strlen(timestamp_value);
            bound_values[param_index - 1] = strdup(timestamp_value);
            if (!bound_values[param_index - 1]) return false;
            str_len_indicators[param_index - 1] = (long)str_len;
            log_this(designator, "Binding TIMESTAMP parameter %u as string: value='%s', len=%zu", LOG_LEVEL_TRACE, 3,
                      (unsigned int)param_index, (char*)bound_values[param_index - 1], str_len);
            bind_result = mssql_SQLBindParameter_ptr(stmt_handle, param_index, SQL_PARAM_INPUT,
                                                SQL_C_CHAR, SQL_TYPE_TIMESTAMP, str_len > 0 ? str_len : 1, 0,
                                                bound_values[param_index - 1], (long)(str_len + 1),
                                                &str_len_indicators[param_index - 1]);
            break;
        }
    }

    if (bind_result != SQL_SUCCESS && bind_result != SQL_SUCCESS_WITH_INFO) {
        log_this(designator, "Failed to bind parameter %u (type %d) - result: %d", LOG_LEVEL_ERROR, 3,
                 (unsigned int)param_index, param->type, bind_result);
        return false;
    }

    log_this(designator, "Successfully bound parameter %u", LOG_LEVEL_TRACE, 1, (unsigned int)param_index);
    return true;
}

// Helper function to cleanup bound values
void mssql_cleanup_bound_values(void** bound_values, size_t count) {
    if (bound_values) {
        for (size_t i = 0; i < count; i++) {
            free(bound_values[i]);
        }
        free(bound_values);
    }
}

// Query Execution Functions
bool mssql_execute_query(DatabaseHandle* connection, QueryRequest* request, QueryResult** result) {
    if (!connection || !request || !result || connection->engine_type != DB_ENGINE_MSSQL) {
        const char* designator = connection ? (connection->designator ? connection->designator : SR_DATABASE) : SR_DATABASE;
        log_this(designator, "MSSQL execute_query: Invalid parameters", LOG_LEVEL_ERROR, 0);
        return false;
    }

    const char* designator = connection->designator ? connection->designator : SR_DATABASE;
    log_this(designator, "mssql_execute_query: ENTER - connection=%p, request=%p, result=%p", LOG_LEVEL_TRACE, 3, (void*)connection, (void*)request, (void*)result);
    log_this(designator, "mssql_execute_query: Parameters validated, proceeding", LOG_LEVEL_TRACE, 0);

    MSSQLConnection* mssql_conn = (MSSQLConnection*)connection->connection_handle;
    if (!mssql_conn || !mssql_conn->connection) {
        log_this(designator, "MSSQL execute_query: Invalid connection handle", LOG_LEVEL_ERROR, 0);
        return false;
    }

    /* Serialize ODBC use of this connection handle (CLI is not free-threaded). */
    MutexResult conn_lock = MUTEX_LOCK(&connection->connection_lock, designator);
    if (conn_lock != MUTEX_SUCCESS) {
        log_this(designator, "MSSQL execute_query: Failed to lock connection", LOG_LEVEL_ERROR, 0);
        return false;
    }

    /* Allocate statement handle */
    void* stmt_handle = NULL;
    if (mssql_SQLAllocHandle_ptr(SQL_HANDLE_STMT, mssql_conn->connection, &stmt_handle) != SQL_SUCCESS) {
        log_this(designator, "MSSQL execute_query: Failed to allocate statement handle", LOG_LEVEL_ERROR, 0);
        mutex_unlock(&connection->connection_lock);
        return false;
    }

    /* Register the statement with the watchdog so it can be cancelled */
    mssql_active_stmt_set(connection, stmt_handle);

    /* Start timing before query execution */
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    /* Variables for parameter binding */
    ParameterList* param_list = NULL;
    TypedParameter** ordered_params = NULL;
    size_t param_count = 0;
    char* positional_sql = NULL;
    void** bound_values = NULL;
    long* str_len_indicators = NULL;
    int exec_result = -1;

    /* Check if we have parameters to bind */
    bool has_params = request->parameters_json && strlen(request->parameters_json) > 2; /* More than "{}" */

    if (has_params && mssql_SQLPrepare_ptr && mssql_SQLBindParameter_ptr) {
        /* Parse parameters */
        log_this(designator, "MSSQL execute_query: Parsing parameters: %s", LOG_LEVEL_TRACE, 1, request->parameters_json);
        param_list = parse_typed_parameters(request->parameters_json, designator);

        if (param_list && param_list->count > 0) {
            /* Convert named parameters to positional */
            positional_sql = convert_named_to_positional(
                request->sql_template, param_list, DB_ENGINE_MSSQL,
                &ordered_params, &param_count, designator
            );

            if (!positional_sql) {
                log_this(designator, "MSSQL execute_query: Failed to convert named to positional parameters", LOG_LEVEL_ERROR, 0);
                free_parameter_list(param_list);
                mssql_active_stmt_clear(connection, stmt_handle);
                mssql_SQLFreeHandle_ptr(SQL_HANDLE_STMT, stmt_handle);
                mutex_unlock(&connection->connection_lock);
                return false;
            }

            log_this(designator, "MSSQL execute_query: Converted SQL: %s", LOG_LEVEL_TRACE, 1, positional_sql);
            log_this(designator, "MSSQL execute_query: Parameter count: %zu", LOG_LEVEL_TRACE, 1, param_count);

            // If there are no actual placeholders in the SQL, use direct execution
            // (SQL Server's SQLPrepare fails on some statements like INSERT...WITH...SELECT)
            if (param_count > 0) {
                // Prepare the statement
                int prepare_result = mssql_SQLPrepare_ptr(stmt_handle, (unsigned char*)positional_sql, SQL_NTS);
                if (prepare_result != SQL_SUCCESS && prepare_result != SQL_SUCCESS_WITH_INFO) {
                    log_this(designator, "MSSQL execute_query: SQLPrepare failed with result %d", LOG_LEVEL_ERROR, 1, prepare_result);
                    free(positional_sql);
                    free(ordered_params);
                    free_parameter_list(param_list);
                    mssql_active_stmt_clear(connection, stmt_handle);
                    mssql_SQLFreeHandle_ptr(SQL_HANDLE_STMT, stmt_handle);
                    mutex_unlock(&connection->connection_lock);
                    return false;
                }

                // Allocate arrays for bound values and indicators
                bound_values = calloc(param_count, sizeof(void*));
                str_len_indicators = calloc(param_count, sizeof(long));
                if (!bound_values || !str_len_indicators) {
                    log_this(designator, "MSSQL execute_query: Failed to allocate binding arrays", LOG_LEVEL_ERROR, 0);
                    free(bound_values);
                    free(str_len_indicators);
                    free(positional_sql);
                    free(ordered_params);
                    free_parameter_list(param_list);
                    mssql_active_stmt_clear(connection, stmt_handle);
                    mssql_SQLFreeHandle_ptr(SQL_HANDLE_STMT, stmt_handle);
                    mutex_unlock(&connection->connection_lock);
                    return false;
                }

                // Bind each parameter
                for (size_t i = 0; i < param_count; i++) {
                    if (!mssql_bind_single_parameter(stmt_handle, (unsigned short)(i + 1), ordered_params[i],
                                                     bound_values, str_len_indicators, designator)) {
                        log_this(designator, "MSSQL execute_query: Failed to bind parameter %zu", LOG_LEVEL_ERROR, 1, i + 1);
                        mssql_cleanup_bound_values(bound_values, i);
                        free(str_len_indicators);
                        free(positional_sql);
                        free(ordered_params);
                        free_parameter_list(param_list);
                        mssql_active_stmt_clear(connection, stmt_handle);
                        mssql_SQLFreeHandle_ptr(SQL_HANDLE_STMT, stmt_handle);
                        mutex_unlock(&connection->connection_lock);
                        return false;
                    }
                }

                // Execute the prepared statement
                exec_result = mssql_SQLExecute_ptr(stmt_handle);

                free(positional_sql);
                free(ordered_params);
                free_parameter_list(param_list);
                positional_sql = NULL;
                ordered_params = NULL;
                param_list = NULL;
            } else {
                // No placeholders in SQL - use direct execution
                log_this(designator, "MSSQL execute_query: No placeholders, using SQLExecDirect", LOG_LEVEL_TRACE, 0);
                exec_result = mssql_SQLExecDirect_ptr(stmt_handle, (char*)positional_sql, SQL_NTS);
                free(positional_sql);
                free(ordered_params);
                free_parameter_list(param_list);
                positional_sql = NULL;
                ordered_params = NULL;
                param_list = NULL;
            }
        } else {
            // No actual parameters or parsing failed
            if (param_list) {
                free_parameter_list(param_list);
            } else {  // has_params is always true
                // Parameter parsing failed when parameters were expected - this is an error
                log_this(designator, "MSSQL execute_query: Failed to parse required parameters", LOG_LEVEL_ERROR, 0);
                mssql_active_stmt_clear(connection, stmt_handle);
                mssql_SQLFreeHandle_ptr(SQL_HANDLE_STMT, stmt_handle);
                mutex_unlock(&connection->connection_lock);
                return false;
            }
            exec_result = mssql_SQLExecDirect_ptr(stmt_handle, (char*)request->sql_template, SQL_NTS);
        }
    } else {
        // No parameters, use direct execution
        exec_result = mssql_SQLExecDirect_ptr(stmt_handle, (char*)request->sql_template, SQL_NTS);
    }

    if (exec_result != SQL_SUCCESS && exec_result != SQL_SUCCESS_WITH_INFO && exec_result != SQL_NO_DATA) {
        // Get detailed error information
        unsigned char sql_state[6] = {0};
        long int native_error = 0;
        unsigned char error_msg[1024] = {0};
        short msg_len = 0;

        // Get the first error diagnostic
        int diag_result = mssql_SQLGetDiagRec_ptr ? mssql_SQLGetDiagRec_ptr(SQL_HANDLE_STMT, stmt_handle, 1,
            sql_state, &native_error, error_msg, (short)sizeof(error_msg), &msg_len) : -1;

        char* error_message = NULL;
        if (diag_result == SQL_SUCCESS || diag_result == SQL_SUCCESS_WITH_INFO) {
            char *msg = (char*)error_msg;
            while (*msg) {
                if (*msg == '\n') {
                    *msg = ' ';
                }
                msg++;
            }
            error_message = strdup((char*)error_msg);
            log_this(designator, "MSSQL query execution failed - MESSAGE: %s", LOG_LEVEL_TRACE, 1, (char*)error_msg);
            log_this(designator, "MSSQL query execution failed - SQLSTATE: %s, Native Error: %ld", LOG_LEVEL_TRACE, 2, (char*)sql_state, (long int)native_error);
            log_this(designator, "MSSQL query execution failed - STATEMENT:\n%s", LOG_LEVEL_TRACE, 1, request->sql_template);

        } else {
            error_message = strdup("MSSQL query execution failed (could not get error details)");
            log_this(designator, "MSSQL query execution failed - result: %d (could not get error details)", LOG_LEVEL_TRACE, 1, exec_result);
        }

        // Create error result
        QueryResult* error_result = calloc(1, sizeof(QueryResult));
        if (error_result) {
            error_result->success = false;

             // Classify by SQLSTATE prefix: 08 = connection_exception, 40 = transaction_rollback (deadlock/serialization),
             // 57 = operator_intervention (query cancelled or statement_timeout). These are transient and worth retrying. Everything else is a real error.

            DatabaseErrorClass err_class = DB_ERR_OTHER;
            if (sql_state[0] == '0' && sql_state[1] == '8') {
                err_class = DB_ERR_TRANSPORT;
            } else if (sql_state[0] == '4' && sql_state[1] == '0') {
                err_class = DB_ERR_TRANSPORT;
            } else if (sql_state[0] == '5' && sql_state[1] == '7') {
                err_class = DB_ERR_TIMEOUT;
            }
            error_result->error_class = err_class;
            error_result->error_message = error_message;
            error_result->row_count = 0;
            error_result->column_count = 0;
            error_result->data_json = strdup("[]");
            error_result->execution_time_ms = 0;
            error_result->affected_rows = 0;
            *result = error_result;
        } else {
            free(error_message);
        }

        if (bound_values) {
            mssql_cleanup_bound_values(bound_values, param_count);
            bound_values = NULL;
        }
        free(str_len_indicators);
        str_len_indicators = NULL;
        mssql_complete_standalone_statement(connection, false);
        mssql_active_stmt_clear(connection, stmt_handle);
        mssql_SQLFreeHandle_ptr(SQL_HANDLE_STMT, stmt_handle);
        mutex_unlock(&connection->connection_lock);
        return false;
    }

    // Process query results using helper function
    bool process_result = mssql_process_query_results(stmt_handle, designator, start_time, result);
    if (bound_values) {
        mssql_cleanup_bound_values(bound_values, param_count);
        bound_values = NULL;
    }
    free(str_len_indicators);
    str_len_indicators = NULL;
    mssql_complete_standalone_statement(connection, process_result);

    // Clean up statement handle
    mssql_active_stmt_clear(connection, stmt_handle);
    mssql_SQLFreeHandle_ptr(SQL_HANDLE_STMT, stmt_handle);
    mutex_unlock(&connection->connection_lock);

    if (process_result) {
        log_this(designator, "MSSQL execute_query: Query completed successfully", LOG_LEVEL_DEBUG, 0);
    }

    return process_result;
}

// cppcheck-suppress constParameterPointer
// Justification: Database engine interface requires non-const QueryRequest* parameter
bool mssql_execute_prepared(DatabaseHandle* connection, const PreparedStatement* stmt, QueryRequest* request, QueryResult** result) {
    if (!connection || !stmt || !request || !result || connection->engine_type != DB_ENGINE_MSSQL) {
        return false;
    }

    const char* designator = connection->designator ? connection->designator : SR_DATABASE;

    const MSSQLConnection* mssql_conn = (const MSSQLConnection*)connection->connection_handle;
    if (!mssql_conn || !mssql_conn->connection) {
        return false;
    }

    // Get the prepared statement handle
    void* stmt_handle = stmt->engine_specific_handle;
    if (!stmt_handle) {
        // Statement had no executable SQL (e.g., only comments after macro processing)
        // Return successful empty result instead of error
        log_this(designator, "MSSQL prepared statement: No executable SQL (statement was not actionable)", LOG_LEVEL_DEBUG, 0);

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

    // Check if SQLExecute is available
    if (!mssql_SQLExecute_ptr) {
        log_this(designator, "MSSQL prepared statement execution: SQLExecute function not available", LOG_LEVEL_ERROR, 0);
        return false;
    }

    log_this(designator, "MSSQL prepared statement execution: Executing prepared statement", LOG_LEVEL_TRACE, 0);

    // Start timing before query execution
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    // Execute the prepared statement
    int exec_result = mssql_SQLExecute_ptr(stmt_handle);
    if (exec_result != SQL_SUCCESS && exec_result != SQL_SUCCESS_WITH_INFO && exec_result != SQL_NO_DATA) {
        // Get detailed error information
        unsigned char sql_state[6] = {0};
        long int native_error = 0;
        unsigned char error_msg[1024] = {0};
        short msg_len = 0;

        // Get the first error diagnostic
        int diag_result = mssql_SQLGetDiagRec_ptr ? mssql_SQLGetDiagRec_ptr(SQL_HANDLE_STMT, stmt_handle, 1,
            sql_state, &native_error, error_msg, (short)sizeof(error_msg), &msg_len) : -1;

        if (diag_result == SQL_SUCCESS || diag_result == SQL_SUCCESS_WITH_INFO) {
            char *msg = (char*)error_msg;
            while (*msg) {
                if (*msg == '\n') {
                    *msg = ' ';
                }
            msg++;
        }
            log_this(designator, "MSSQL prepared statement execution failed - MESSAGE: %s", LOG_LEVEL_ERROR, 1, (char*)error_msg);
            log_this(designator, "MSSQL prepared statement execution failed - SQLSTATE: %s, Native Error: %ld", LOG_LEVEL_ERROR, 2, (char*)sql_state, (long int)native_error);
        } else {
            log_this(designator, "MSSQL prepared statement execution failed - result: %d (could not get error details)", LOG_LEVEL_ERROR, 1, exec_result);
        }

        /* A failed SQLExecute leaves the statement active. FreeTDS then
         * rejects SQLEndTran until the cursor is closed, so a reverse
         * migration cannot roll back. SQL_CLOSE keeps the prepared plan. */
        if (mssql_SQLFreeStmt_ptr) {
            mssql_SQLFreeStmt_ptr(stmt_handle, SQL_CLOSE);
        }
        mssql_complete_standalone_statement(connection, false);
        return false;
    }

    // Process query results using helper function
    bool process_result = mssql_process_query_results(stmt_handle, designator, start_time, result);
    mssql_complete_standalone_statement(connection, process_result);

    if (process_result) {
        log_this(designator, "MSSQL prepared statement execution: Query completed successfully", LOG_LEVEL_TRACE, 0);
    }

    return process_result;
}
