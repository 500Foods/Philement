/*
 * Conduit Query API endpoint implementation for the Hydrogen Project.
 * Executes pre-defined database queries by reference.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/api/api_utils.h>

// Database subsystem includes
#include <src/database/database_cache.h>
#include <src/database/database_params.h>
#include <src/database/database_pending.h>
#include <src/database/database_queue_select.h>
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/database.h>

// Local includes
#include "../conduit_service.h"
#include "../conduit_helpers.h"
#include "query.h"

// Enable mock database queue functions for unit testing
#ifdef USE_MOCK_DBQUEUE
#include <unity/mocks/mock_dbqueue.h>
#endif

// Enable mock system functions for unit testing
#ifdef USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>
#endif

// Enable mock for generate_query_id
#ifdef USE_MOCK_GENERATE_QUERY_ID
extern char* mock_generate_query_id(void);
#endif

// Enable mock for select_query_queue
#ifdef USE_MOCK_SELECT_QUERY_QUEUE
extern DatabaseQueue* mock_select_query_queue(const char* database, const char* queue_type);
#endif

/**
 * Handle the /api/conduit/query endpoint.
 * Executes pre-defined database queries by reference with typed parameters.
 *
 * Supports both GET (query parameters) and POST (JSON body) methods.
 *
 * Implementation:
 * 1. Parse request parameters (GET query string or POST JSON body)
 * 2. Lookup query in Query Table Cache (QTC)
 * 3. Parse and validate typed parameters
 * 4. Convert named parameters to positional format
 * 5. Select optimal database queue (DQM)
 * 6. Submit query and wait for result
 * 7. Return JSON response with results
 */
enum MHD_Result handle_conduit_query_request(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *upload_data,
    // cppcheck-suppress[constParameterPointer] - upload_data_size parameter must match libmicrohttpd callback signature
    size_t *upload_data_size,
    void **con_cls
   ) {
    (void)url;              // Unused parameter

    // Use common POST body buffering (handles both GET and POST)
    ApiPostBuffer *buffer = NULL;
    ApiBufferResult buf_result = api_buffer_post_data(method, upload_data, upload_data_size, con_cls, &buffer);
    
    switch (buf_result) {
        case API_BUFFER_CONTINUE:
            // More data expected for POST, continue receiving
            return MHD_YES;
            
        case API_BUFFER_ERROR:
            // Error occurred during buffering
            return api_send_error_and_cleanup(connection, con_cls,
                "Request processing error", MHD_HTTP_INTERNAL_SERVER_ERROR);
            
        case API_BUFFER_METHOD_ERROR:
            // Unsupported HTTP method
            return api_send_error_and_cleanup(connection, con_cls,
                "Method not allowed - use GET or POST", MHD_HTTP_METHOD_NOT_ALLOWED);
            
        case API_BUFFER_COMPLETE:
            // All data received (or GET request), continue with processing
            break;
    }

    log_this(SR_API, "%s: Processing conduit query request", LOG_LEVEL_TRACE, 1, conduit_service_name());

    // Step 1: Validate HTTP method (GET and POST are allowed)
    enum MHD_Result result = handle_method_validation(connection, method);
    if (result != MHD_YES) {
        api_free_post_buffer(con_cls);
        return result;
    }

    // Step 2: Parse request data from buffer (handles GET query params and POST JSON body)
    json_t *request_json = NULL;
    result = handle_request_parsing_with_buffer(connection, buffer, &request_json);
    
    // Free the buffer now that we've parsed the data
    api_free_post_buffer(con_cls);
    
    if (result != MHD_YES) return result;

    log_this(SR_API, "%s: Request data parsed successfully", LOG_LEVEL_TRACE, 1, conduit_service_name());

    // Step 3: Extract and validate required fields
    int query_ref = 0;
    const char* database = NULL;
    json_t* params_json = NULL;
    result = handle_field_extraction(connection, request_json, &query_ref, &database, &params_json);
    if (result != MHD_YES) {
        json_decref(request_json);
        return result;
    }

    log_this(SR_API, "%s: Request fields extracted: query_ref=%d, database=%s", LOG_LEVEL_TRACE, 3, conduit_service_name(), query_ref, database);

    // Step 4: Lookup database queue and query cache entry
    DatabaseQueue* db_queue = NULL;
    QueryCacheEntry* cache_entry = NULL;
    bool query_not_found = false;
    result = handle_database_lookup(connection, database, query_ref, &db_queue, &cache_entry, &query_not_found, true);
    if (result != MHD_YES) {
        json_decref(request_json);
        return result;
    }

    // Check if database lookup failed (error response already sent)
    if (!db_queue) {
        json_decref(request_json);
        return MHD_YES;
    }

    // Handle invalid queryref case
    if (query_not_found) {
        json_t* response = build_invalid_queryref_response(query_ref, database, NULL);
        enum MHD_Result http_result = api_send_json_response(connection, response, MHD_HTTP_OK);
        json_decref(response);
        json_decref(request_json);
        return http_result;
    }

    log_this(SR_API, "%s: Database and query lookup successful", LOG_LEVEL_TRACE, 1, conduit_service_name());

    // Step 5: Parse and convert parameters
    ParameterList* param_list;
    char* converted_sql;
    TypedParameter** ordered_params;
    size_t param_count = 0;
    char* message = NULL;
    result = handle_parameter_processing(connection, params_json, db_queue, cache_entry,
                                        database, query_ref, &param_list, &converted_sql,
                                        &ordered_params, &param_count, &message);
    if (result != MHD_YES || !converted_sql) {
        json_decref(request_json);
        return result;
    }

    // Generate parameter validation messages
    // Temporarily disabled due to crashes - messages handled in handle_parameter_processing

    // Step 6: Select optimal queue
    DatabaseQueue* selected_queue;
    result = handle_queue_selection(connection, database, query_ref, cache_entry,
                                    param_list, converted_sql, ordered_params, &selected_queue);
    if (result != MHD_YES) {
        json_decref(request_json);
        return result;
    }

    // Step 7: Generate unique query ID
    char* query_id;
    result = handle_query_id_generation(connection, database, query_ref, param_list,
                                        converted_sql, ordered_params, &query_id);
    if (result != MHD_YES) {
        json_decref(request_json);
        return result;
    }

    // Step 8: Register pending result
    PendingQueryResult* pending;
    result = handle_pending_registration(connection, database, query_ref, query_id,
                                        param_list, converted_sql, ordered_params,
                                        cache_entry, &pending);
    if (result != MHD_YES) {
        json_decref(request_json);
        return result;
    }

    // Step 9: Prepare and submit query
    result = handle_query_submission(connection, database, query_ref, selected_queue, query_id,
                                    converted_sql, param_list, ordered_params, param_count, cache_entry);
    if (result != MHD_YES) {
        json_decref(request_json);
        return result;
    }

    // Step 10: Wait for result and build response
    result = handle_response_building(connection, query_ref, database, cache_entry,
                                      selected_queue, pending, query_id, converted_sql,
                                      param_list, ordered_params, message);
    json_decref(request_json);

    // Clean up message
    if (message) free(message);

    log_this(SR_API, "%s: Conduit query request processing completed", LOG_LEVEL_TRACE, 1, conduit_service_name());

    return result;
}