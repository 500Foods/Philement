/*
 * Query Execution Helper Functions Header
 *
 * Header file for query execution helper functions.
 */

#ifndef QUERY_EXECUTION_HELPERS_H
#define QUERY_EXECUTION_HELPERS_H

// Standard includes
#include <stdbool.h>
#include <stdlib.h>

// Third-party libraries
#include <jansson.h>

// Project includes
#include <src/database/database.h>
#include <src/database/database_cache.h>
#include <src/database/database_params.h>
#include <src/database/database_pending.h>
#include <src/database/dbqueue/dbqueue.h>

// Function declarations

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
                              size_t* param_count);

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
                                                      TypedParameter** ordered_params, char* message);

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
                                           TypedParameter** ordered_params, char* message);

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
                                                               TypedParameter** ordered_params, char* message);

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
                                     char* message);

#endif /* QUERY_EXECUTION_HELPERS_H */