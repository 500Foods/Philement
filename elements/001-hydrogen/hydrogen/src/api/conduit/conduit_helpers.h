/*
 * Conduit Service Helper Functions
 * 
 * Common helper functions for all Conduit Service endpoints.
 */

#ifndef CONDUIT_HELPERS_H
#define CONDUIT_HELPERS_H

// Standard includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Third-party libraries
#include <microhttpd.h>
#include <jansson.h>

// API subsystem includes
#include <src/api/api_utils.h>

// Database subsystem includes for type definitions
#include <src/database/database.h>
#include <src/database/database_cache.h>
#include <src/database/database_params.h>
#include <src/database/database_pending.h>
#include <src/database/database_queue_select.h>
#include <src/database/dbqueue/dbqueue.h>

// Function declarations for common helper functions

// Generate unique query ID
char* generate_query_id(void);

// Validate HTTP method - only POST is allowed
bool validate_http_method(const char* method);

// Parse request data from either POST JSON body or GET query parameters
json_t* parse_request_data(struct MHD_Connection* connection, const char* method,
                          const char* upload_data, const size_t* upload_data_size);

// Extract and validate required fields from request JSON
bool extract_request_fields(json_t* request_json, int* query_ref, const char** database, json_t** params);

// Lookup database queue from global queue manager
DatabaseQueue* lookup_database_queue(const char* database);

// Lookup query cache entry from database queue
QueryCacheEntry* lookup_query_cache_entry(DatabaseQueue* db_queue, int query_ref);

// Lookup database queue and query cache entry
bool lookup_database_and_query(DatabaseQueue** db_queue, QueryCacheEntry** cache_entry,
                                const char* database, int query_ref);

// Lookup database queue and public query cache entry (query_type_a28 = 10)
bool lookup_database_and_public_query(DatabaseQueue** db_queue, QueryCacheEntry** cache_entry,
                                       const char* database, int query_ref);

// Parse and convert parameters
bool process_parameters(json_t* params_json, ParameterList** param_list,
                        const char* sql_template, DatabaseEngineType engine_type,
                        char** converted_sql, TypedParameter*** ordered_params, size_t* param_count);

// Generate parameter validation messages
char* generate_parameter_messages(const char* sql_template, json_t* params_json);

// Select optimal queue for query execution
DatabaseQueue* select_query_queue(const char* database, const char* queue_type);

// Prepare and submit database query
bool prepare_and_submit_query(DatabaseQueue* selected_queue, const char* query_id,
                               const char* sql_template, TypedParameter** ordered_params,
                               size_t param_count, const QueryCacheEntry* cache_entry);

// Wait for query result
const QueryResult* wait_for_query_result(PendingQueryResult* pending);

// Parse query result data into JSON
json_t* parse_query_result_data(const QueryResult* result);

// Build success response JSON
json_t* build_success_response(int query_ref, const QueryCacheEntry* cache_entry,
                              const QueryResult* result, const DatabaseQueue* selected_queue, const char* message);

// Build error response JSON
json_t* build_error_response(int query_ref, const char* database, const QueryCacheEntry* cache_entry,
                            const PendingQueryResult* pending, const QueryResult* result, const char* message);

// Build invalid queryref response JSON
json_t* build_invalid_queryref_response(int query_ref, const char* database, const char* message);

// Build response JSON
json_t* build_response_json(int query_ref, const char* database, const QueryCacheEntry* cache_entry,
                            const DatabaseQueue* selected_queue, PendingQueryResult* pending, const char* message);

// Determine HTTP status code based on result
unsigned int determine_http_status(const PendingQueryResult* pending, const QueryResult* result);

// Error response creation functions
json_t* create_validation_error_response(const char* error_msg, const char* error_detail);
json_t* create_lookup_error_response(const char* error_msg, const char* database, int query_ref, bool include_query_ref, const char* message);
json_t* create_processing_error_response(const char* error_msg, const char* database, int query_ref);

// Request handling helper functions
enum MHD_Result handle_method_validation(struct MHD_Connection *connection, const char* method);

enum MHD_Result handle_request_parsing_with_buffer(struct MHD_Connection *connection, ApiPostBuffer* buffer, json_t** request_json);

enum MHD_Result handle_request_parsing(struct MHD_Connection *connection, const char* method,
                                         const char* upload_data, const size_t* upload_data_size,
                                         json_t** request_json);

enum MHD_Result handle_field_extraction(struct MHD_Connection *connection, json_t* request_json,
                                         int* query_ref, const char** database, json_t** params_json);

enum MHD_Result handle_database_lookup(struct MHD_Connection *connection, const char* database,
                                        int query_ref, DatabaseQueue** db_queue, QueryCacheEntry** cache_entry,
                                        bool* query_not_found, bool require_public);

enum MHD_Result handle_parameter_processing(struct MHD_Connection *connection, json_t* params_json,
                                             const DatabaseQueue* db_queue, const QueryCacheEntry* cache_entry,
                                             const char* database, int query_ref,
                                             ParameterList** param_list, char** converted_sql,
                                             TypedParameter*** ordered_params, size_t* param_count);

enum MHD_Result handle_queue_selection(struct MHD_Connection *connection, const char* database,
                                         int query_ref, const QueryCacheEntry* cache_entry,
                                         ParameterList* param_list, char* converted_sql,
                                         TypedParameter** ordered_params,
                                         DatabaseQueue** selected_queue);

enum MHD_Result handle_query_id_generation(struct MHD_Connection *connection, const char* database,
                                            int query_ref, ParameterList* param_list, char* converted_sql,
                                            TypedParameter** ordered_params, char** query_id);

enum MHD_Result handle_pending_registration(struct MHD_Connection *connection, const char* database,
                                             int query_ref, char* query_id, ParameterList* param_list,
                                             char* converted_sql, TypedParameter** ordered_params,
                                             const QueryCacheEntry* cache_entry, PendingQueryResult** pending);

enum MHD_Result handle_query_submission(struct MHD_Connection *connection, const char* database,
                                         int query_ref, DatabaseQueue* selected_queue, char* query_id,
                                         char* converted_sql, ParameterList* param_list,
                                         TypedParameter** ordered_params, size_t param_count,
                                         const QueryCacheEntry* cache_entry);

enum MHD_Result handle_response_building(struct MHD_Connection *connection, int query_ref,
                                          const char* database, const QueryCacheEntry* cache_entry,
                                          const DatabaseQueue* selected_queue, PendingQueryResult* pending,
                                          char* query_id, char* converted_sql, ParameterList* param_list,
                                          TypedParameter** ordered_params, const char* message);

#endif /* CONDUIT_HELPERS_H */