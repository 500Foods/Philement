/*
 * Cap-Protected Conduit Query API endpoint implementation.
 *
 * Mirrors the public conduit/query flow but:
 *   1. Requires a Cap token + site id in the request body.
 *   2. Verifies the token with the configured ChaCha siteverify endpoint.
 *   3. Looks up only protected query cache entries (query_type_a28 = 11).
 *   4. Rejects the request with a CAP_VERIFY_* error before any query runs.
 */

#include <src/hydrogen.h>
#include <src/api/api_utils.h>

#include <src/api/conduit/helpers/cap_verify.h>

#include <src/database/database_cache.h>
#include <src/database/database_params.h>
#include <src/database/database_pending.h>
#include <src/database/database_queue_select.h>
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/database.h>

#include "../conduit_service.h"
#include "../conduit_helpers.h"
#include "cap_query.h"

// Error codes returned in the JSON error response
#define CAP_ERROR_TOKEN_MISSING "CAP_TOKEN_MISSING"
#define CAP_ERROR_TOKEN_INVALID "CAP_TOKEN_INVALID"
#define CAP_ERROR_VERIFY_FAILED "CAP_VERIFY_FAILED"
#define CAP_ERROR_STATEMENT_TYPE_NOT_ALLOWED "CAP_STATEMENT_TYPE_NOT_ALLOWED"

// HTTP status used for all Cap verification failures
#define CAP_VERIFY_HTTP_STATUS MHD_HTTP_BAD_REQUEST

/**
 * Send a Cap verification error response and clean up the request JSON.
 */
static enum MHD_Result send_cap_verify_error(struct MHD_Connection *connection,
                                             json_t* request_json,
                                             const char* error_code,
                                             const char* error_detail) {
    json_t* response = create_validation_error_response(error_code, error_detail);
    enum MHD_Result result = api_send_json_response(connection, response, CAP_VERIFY_HTTP_STATUS);
    if (request_json) {
        json_decref(request_json);
    }
    return result;
}

/**
 * Select the execution queue for a cap-protected query.
 * Type 11 queries are always forced onto the slow queue.
 */
static enum MHD_Result handle_cap_queue_selection(struct MHD_Connection *connection,
                                                  const char* database,
                                                  int query_ref,
                                                  const QueryCacheEntry* cache_entry,
                                                  ParameterList* param_list,
                                                  char* converted_sql,
                                                  TypedParameter** ordered_params,
                                                  DatabaseQueue** selected_queue) {
    const char* queue_type = (cache_entry && cache_entry->query_type == 11)
                                 ? "slow"
                                 : (cache_entry ? cache_entry->queue_type : "slow");
    *selected_queue = select_query_queue(database, queue_type);
    if (!*selected_queue) {
        free(converted_sql);
        free_parameter_list(param_list);
        if (ordered_params) free(ordered_params);
        json_t *error_response = create_processing_error_response("No suitable queue available", database, query_ref);
        api_send_json_response(connection, error_response, MHD_HTTP_SERVICE_UNAVAILABLE);
        return MHD_NO;
    }
    return MHD_YES;
}

/**
 * Handle the result of api_buffer_post_data.
 */
static enum MHD_Result handle_cap_buffer_result(struct MHD_Connection *connection,
                                                ApiBufferResult buf_result,
                                                void **con_cls) {
    switch (buf_result) {
        case API_BUFFER_CONTINUE:
            return MHD_YES;

        case API_BUFFER_ERROR:
            return api_send_error_and_cleanup(connection, con_cls,
                "Request processing error", MHD_HTTP_INTERNAL_SERVER_ERROR);

        case API_BUFFER_METHOD_ERROR:
            return api_send_error_and_cleanup(connection, con_cls,
                "Method not allowed - use GET or POST", MHD_HTTP_METHOD_NOT_ALLOWED);

        case API_BUFFER_COMPLETE:
            return MHD_YES;

        default:
            return api_send_error_and_cleanup(connection, con_cls,
                "Unknown buffer result", MHD_HTTP_INTERNAL_SERVER_ERROR);
    }
}

enum MHD_Result handle_conduit_cap_query_request(
    struct MHD_Connection *connection,
    const char *url,
    const char *method,
    const char *upload_data,
    // cppcheck-suppress[constParameterPointer] - upload_data_size parameter must match libmicrohttpd callback signature
    size_t *upload_data_size,
    void **con_cls
) {
    (void)url;

    // Buffer POST body (handles both GET and POST)
    ApiPostBuffer *buffer = NULL;
    ApiBufferResult buf_result = api_buffer_post_data(method, upload_data, upload_data_size, con_cls, &buffer);

    enum MHD_Result buffer_result = handle_cap_buffer_result(connection, buf_result, con_cls);
    if (buffer_result != MHD_YES) {
        return buffer_result;
    }

    log_this(SR_API, "%s: Processing cap-protected query request", LOG_LEVEL_TRACE, 1, conduit_service_name());

    // Validate HTTP method (GET and POST are allowed)
    enum MHD_Result result = handle_method_validation(connection, method);
    if (result != MHD_YES) {
        api_free_post_buffer(con_cls);
        return MHD_YES;
    }

    // Parse request data
    json_t *request_json = NULL;
    result = handle_request_parsing_with_buffer(connection, buffer, &request_json);
    api_free_post_buffer(con_cls);
    if (result != MHD_YES) {
        return MHD_YES;
    }

    // Extract standard conduit fields
    int query_ref = 0;
    const char* database = NULL;
    json_t* params_json = NULL;
    result = handle_field_extraction(connection, request_json, &query_ref, &database, &params_json);
    if (result != MHD_YES) {
        json_decref(request_json);
        return MHD_YES;
    }

    // Extract Cap-specific fields
    const char* chacha_token = NULL;
    const char* chacha_site = NULL;
    json_t* token_json = json_object_get(request_json, "chachaToken");
    json_t* site_json = json_object_get(request_json, "chachaSite");
    if (json_is_string(token_json)) {
        chacha_token = json_string_value(token_json);
    }
    if (json_is_string(site_json)) {
        chacha_site = json_string_value(site_json);
    }

    if (!chacha_token || !*chacha_token) {
        return send_cap_verify_error(connection, request_json,
                                     CAP_ERROR_TOKEN_MISSING,
                                     "Missing required field: chachaToken");
    }

    // Verify the Cap token
    char verify_error[256] = {0};
    cap_verify_result_t verify_result = cap_verify_token(chacha_token, chacha_site, verify_error, sizeof(verify_error));
    bool cap_fallback = false;
    if (verify_result == CAP_VERIFY_HARD_FAIL) {
        log_this(SR_API, "%s: Cap verification failed: %s", LOG_LEVEL_DEBUG, 2,
                 conduit_service_name(), verify_error[0] ? verify_error : "unknown");

        // Distinguish an explicitly rejected token from other verification failures
        const char* error_code = CAP_ERROR_VERIFY_FAILED;
        if (strstr(verify_error, "token rejected")) {
            error_code = CAP_ERROR_TOKEN_INVALID;
        }

        return send_cap_verify_error(connection, request_json,
                                     error_code,
                                     verify_error[0] ? verify_error : "Cap token verification failed");
    }
    if (verify_result == CAP_VERIFY_FALLBACK) {
        log_this(SR_API, "%s: Cap verification transient failure - accepting with fallback flag: %s",
                 LOG_LEVEL_DEBUG, 2, conduit_service_name(),
                 verify_error[0] ? verify_error : "unknown");
        cap_fallback = true;
    }

    log_this(SR_API, "%s: Cap verification %s", LOG_LEVEL_TRACE, 1, conduit_service_name(),
             cap_fallback ? "accepted via fallback" : "succeeded");

    // Lookup protected database queue and query cache entry (type 11)
    DatabaseQueue* db_queue = NULL;
    QueryCacheEntry* cache_entry = NULL;
    bool query_not_found = false;
    result = handle_database_lookup(connection, database, query_ref, &db_queue, &cache_entry,
                                    &query_not_found, 11);
    if (result != MHD_YES) {
        json_decref(request_json);
        return MHD_YES;
    }

    if (!db_queue) {
        json_decref(request_json);
        return MHD_YES;
    }

    if (query_not_found) {
        json_t* response = build_invalid_queryref_response(query_ref, database, NULL);
        enum MHD_Result http_result = api_send_json_response(connection, response, MHD_HTTP_OK);
        json_decref(request_json);
        return http_result;
    }

    log_this(SR_API, "%s: Protected database and query lookup successful", LOG_LEVEL_TRACE, 1, conduit_service_name());

    // Enforce statement-type restrictions for protected queries
    if (cache_entry && !query_statement_type_allowed(cache_entry->query_type, cache_entry->sql_template)) {
        return send_cap_verify_error(connection, request_json,
                                     CAP_ERROR_STATEMENT_TYPE_NOT_ALLOWED,
                                     "Protected query statement type not allowed");
    }

    // From here the flow is identical to the public query endpoint
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
        return MHD_YES;
    }

    DatabaseQueue* selected_queue;
    result = handle_cap_queue_selection(connection, database, query_ref, cache_entry,
                                        param_list, converted_sql, ordered_params, &selected_queue);
    if (result != MHD_YES) {
        json_decref(request_json);
        return MHD_YES;
    }

    char* query_id;
    result = handle_query_id_generation(connection, database, query_ref, param_list,
                                        converted_sql, ordered_params, &query_id);
    if (result != MHD_YES) {
        json_decref(request_json);
        return MHD_YES;
    }

    PendingQueryResult* pending;
    result = handle_pending_registration(connection, database, query_ref, query_id,
                                         param_list, converted_sql, ordered_params,
                                         cache_entry, &pending);
    if (result != MHD_YES) {
        json_decref(request_json);
        return MHD_YES;
    }

    result = handle_query_submission(connection, database, query_ref, selected_queue, query_id,
                                     converted_sql, param_list, ordered_params, param_count, cache_entry);
    if (result != MHD_YES) {
        json_decref(request_json);
        return MHD_YES;
    }

    result = handle_response_building(connection, query_ref, database, cache_entry,
                                      selected_queue, pending, query_id, converted_sql,
                                      param_list, ordered_params, message, cap_fallback);
    json_decref(request_json);

    log_this(SR_API, "%s: Cap-protected query request processing completed", LOG_LEVEL_TRACE, 1, conduit_service_name());

    return result;
}
