/*
 * Query Execution Helper Functions
 *
 * Helper functions for query execution logic that can be shared across
 * different conduit endpoints (queries, auth_queries, alt_queries).
 */

#include <src/hydrogen.h>

// Standard includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Third-party libraries
#include <jansson.h>

// Project includes
#include <src/api/conduit/conduit_helpers.h>
#include <src/api/conduit/conduit_service.h>
#include <src/logging/logging.h>

/**
 * @brief Process and validate query parameters
 *
 * Handles parameter type validation, missing parameter checking, parameter processing,
 * and unused parameter warnings. Returns a consolidated message string.
 *
 * @param params JSON parameter object
 * @param cache_entry Query cache entry with SQL template
 * @param db_queue Database queue for engine type
 * @param param_list Output parameter list
 * @param converted_sql Output converted SQL
 * @param ordered_params Output ordered parameters
 * @param param_count Output parameter count
 * @return Consolidated message string (caller must free) or NULL on success
 */
char* process_query_parameters(json_t* params, const QueryCacheEntry* cache_entry,
                              const DatabaseQueue* db_queue, ParameterList** param_list,
                              char** converted_sql, TypedParameter*** ordered_params,
                              size_t* param_count) {
    char* message = NULL;

    // (1) Validate parameter types
    char* type_error = validate_parameter_types_simple(params);
    if (type_error) {
        return type_error;  // Return error directly
    }

    // (2) Check for missing parameters
    ParameterList* temp_param_list = NULL;
    if (params && json_is_object(params)) {
        char* params_str = json_dumps(params, JSON_COMPACT);
        if (params_str) {
            temp_param_list = parse_typed_parameters(params_str, NULL);
            free(params_str);
        }
    }
    if (!temp_param_list) {
        temp_param_list = calloc(1, sizeof(ParameterList));
    }

    char* missing_error = check_missing_parameters_simple(cache_entry->sql_template, temp_param_list);
    if (missing_error) {
        // Clean up temp list
        if (temp_param_list) {
            if (temp_param_list->params) {
                for (size_t i = 0; i < temp_param_list->count; i++) {
                    if (temp_param_list->params[i]) {
                        free(temp_param_list->params[i]->name);
                        free(temp_param_list->params[i]);
                    }
                }
                free(temp_param_list->params);
            }
            free(temp_param_list);
        }
        return missing_error;  // Return error directly
    }

    // (3) Process parameters
    if (!process_parameters(params, param_list, cache_entry->sql_template,
                            db_queue->engine_type, converted_sql, ordered_params, param_count)) {
        // Clean up temp list
        if (temp_param_list) {
            if (temp_param_list->params) {
                for (size_t i = 0; i < temp_param_list->count; i++) {
                    if (temp_param_list->params[i]) {
                        free(temp_param_list->params[i]->name);
                        free(temp_param_list->params[i]);
                    }
                }
                free(temp_param_list->params);
            }
            free(temp_param_list);
        }
        return strdup("Parameter processing failed");
    }

    // (4) Check for unused parameters (warning only)
    char* unused_warning = check_unused_parameters_simple(cache_entry->sql_template, *param_list);
    if (unused_warning) {
        log_this(SR_API, "%s: Query has unused parameters: %s", LOG_LEVEL_ALERT, 2,
                 conduit_service_name(), unused_warning);
        message = unused_warning;
    }

    // Clean up temp list
    if (temp_param_list) {
        if (temp_param_list->params) {
            for (size_t i = 0; i < temp_param_list->count; i++) {
                if (temp_param_list->params[i]) {
                    free(temp_param_list->params[i]->name);
                    free(temp_param_list->params[i]);
                }
            }
            free(temp_param_list->params);
        }
        free(temp_param_list);
    }

    // Generate parameter validation messages
    char* validation_message = generate_parameter_messages(cache_entry->sql_template, params);
    if (validation_message) {
        if (message) {
            // Combine the messages
            size_t combined_length = strlen(message) + strlen(validation_message) + 3; // +3 for " | " and null terminator
            char* combined_message = malloc(combined_length);
            if (combined_message) {
                snprintf(combined_message, combined_length, "%s | %s", message, validation_message);
                free(message);
                free(validation_message);
                message = combined_message;
            }
        } else {
            message = validation_message;
        }
    }

    return message;  // NULL on success, error message on failure
}

/**
 * @brief Select query queue with error handling
 *
 * Selects the appropriate queue for query execution and handles selection failures.
 *
 * @param database Database name
 * @param cache_entry Query cache entry
 * @param converted_sql SQL to free on error
 * @param param_list Parameters to free on error
 * @param ordered_params Parameters to free on error
 * @param message Message to free on error
 * @return Selected queue or NULL on error (caller handles cleanup)
 */
DatabaseQueue* select_query_queue_with_error_handling(const char* database, const QueryCacheEntry* cache_entry,
                                                      char* converted_sql, ParameterList* param_list,
                                                      TypedParameter** ordered_params, char* message) {
    DatabaseQueue *selected_queue = select_query_queue(database, cache_entry->queue_type);
    if (!selected_queue) {
        // Clean up on error
        free(converted_sql);
        free_parameter_list(param_list);
        if (ordered_params) free(ordered_params);
        if (message) free(message);
    }
    return selected_queue;
}

/**
 * @brief Generate query ID with error handling
 *
 * Generates a unique query ID and handles generation failures.
 *
 * @param converted_sql SQL to free on error
 * @param param_list Parameters to free on error
 * @param ordered_params Parameters to free on error
 * @param message Message to free on error
 * @return Query ID or NULL on error (caller handles cleanup)
 */
char* generate_query_id_with_error_handling(char* converted_sql, ParameterList* param_list,
                                           TypedParameter** ordered_params, char* message) {
    char *query_id = generate_query_id();
    if (!query_id) {
        // Clean up on error
        free(converted_sql);
        free_parameter_list(param_list);
        if (ordered_params) free(ordered_params);
        if (message) free(message);
    }
    return query_id;
}

/**
 * @brief Register pending result with error handling
 *
 * Registers a pending result for the query and handles registration failures.
 *
 * @param query_id Query ID (will be freed on error)
 * @param cache_entry Query cache entry
 * @param converted_sql SQL to free on error
 * @param param_list Parameters to free on error
 * @param ordered_params Parameters to free on error
 * @param message Message to free on error
 * @return Pending result or NULL on error (caller handles cleanup)
 */
PendingQueryResult* register_pending_result_with_error_handling(char* query_id, const QueryCacheEntry* cache_entry,
                                                               char* converted_sql, ParameterList* param_list,
                                                               TypedParameter** ordered_params, char* message) {
    PendingResultManager *pending_mgr = get_pending_result_manager();
    PendingQueryResult *pending = pending_result_register(pending_mgr, query_id, cache_entry->timeout_seconds, NULL);
    if (!pending) {
        // Clean up on error
        free(query_id);
        free(converted_sql);
        free_parameter_list(param_list);
        if (ordered_params) free(ordered_params);
        if (message) free(message);
    }
    return pending;
}

/**
 * @brief Submit query with error handling
 *
 * Submits the query for execution and handles submission failures.
 *
 * @param selected_queue Queue to submit to
 * @param query_id Query ID (will be freed on error)
 * @param cache_entry Query cache entry
 * @param ordered_params Parameters to submit
 * @param param_count Parameter count
 * @param converted_sql SQL to free on error
 * @param param_list Parameters to free on error
 * @param message Message to free on error
 * @return true on success, false on error (caller handles cleanup)
 */
bool submit_query_with_error_handling(DatabaseQueue* selected_queue, char* query_id,
                                     const QueryCacheEntry* cache_entry, TypedParameter** ordered_params,
                                     size_t param_count, char* converted_sql, ParameterList* param_list,
                                     char* message) {
    if (!prepare_and_submit_query(selected_queue, query_id, cache_entry->sql_template,
                                  ordered_params, param_count, cache_entry)) {
        // Clean up on error
        free(query_id);
        free(converted_sql);
        free_parameter_list(param_list);
        if (ordered_params) free(ordered_params);
        if (message) free(message);
        return false;
    }
    return true;
}