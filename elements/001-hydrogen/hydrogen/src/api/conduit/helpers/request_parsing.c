/*
 * Conduit Service Request Parsing Helper Functions
 * 
 * Functions for parsing and validating request data.
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
#include "../conduit_helpers.h"
#include "../conduit_service.h"

// Enable mock database queue functions for unit testing
#ifdef USE_MOCK_DBQUEUE
#include <unity/mocks/mock_dbqueue.h>
#endif

// Enable mock system functions for unit testing
#ifdef USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>
#endif

// Validate HTTP method - only POST is allowed
bool validate_http_method(const char* method) {
    if (!method) return false;
    return strcmp(method, "POST") == 0;
}

// Parse request data from either POST JSON body or GET query parameters
json_t* parse_request_data(struct MHD_Connection* connection, const char* method,
                          const char* upload_data, const size_t* upload_data_size) {
    json_t* request_json = NULL;
    json_error_t json_error;

    if (method && strcmp(method, "POST") == 0) {
        // POST request - parse JSON body
        if (!upload_data || !upload_data_size || *upload_data_size == 0) {
            return NULL; // Missing request body
        }

        request_json = json_loads(upload_data, 0, &json_error);
        if (!request_json) {
            log_this(SR_API, "Failed to parse JSON in conduit query: %s", LOG_LEVEL_ERROR, 1, json_error.text);
            return NULL; // Invalid JSON
        }
    } else {
        // GET request - parse query parameters
        request_json = json_object();

        // Extract query parameters
        const char* query_ref_str = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "query_ref");
        const char* database = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "database");
        const char* params_json = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "params");

        if (query_ref_str) json_object_set_new(request_json, "query_ref", json_integer(atoll(query_ref_str)));
        if (database) json_object_set_new(request_json, "database", json_string(database));
        if (params_json) {
            json_t* params_parsed = json_loads(params_json, 0, &json_error);
            if (params_parsed) {
                json_object_set_new(request_json, "params", params_parsed);
            } else {
                json_decref(request_json);
                return NULL; // Invalid params JSON
            }
        }
    }

    return request_json;
}

// Extract and validate required fields from request JSON
bool extract_request_fields(json_t* request_json, int* query_ref, const char** database, json_t** params) {
    json_t* query_ref_json = json_object_get(request_json, "query_ref");
    json_t* database_json = json_object_get(request_json, "database");
    *params = json_object_get(request_json, "params");

    if (!query_ref_json || !json_is_integer(query_ref_json)) {
        return false;
    }

    if (!database_json || !json_is_string(database_json)) {
        return false;
    }

    *query_ref = (int)json_integer_value(query_ref_json);
    *database = json_string_value(database_json);
    return true;
}

// Request handling helper functions
enum MHD_Result handle_method_validation(struct MHD_Connection *connection, const char* method) {
    if (!validate_http_method(method)) {
        json_t *error_response = create_validation_error_response(
            "Method not allowed",
            "Only POST requests are supported"
        );

        api_send_json_response(connection, error_response, MHD_HTTP_METHOD_NOT_ALLOWED);
        json_decref(error_response);
        return MHD_NO; // Response sent - processing complete
    }
    return MHD_YES; // Continue processing
}

enum MHD_Result handle_request_parsing_with_buffer(struct MHD_Connection *connection, ApiPostBuffer* buffer, json_t** request_json) {
    const char* method = (buffer->http_method == 'P') ? "POST" : "GET";
    *request_json = parse_request_data(connection, method, buffer->data, &buffer->size);
    if (!*request_json) {
        // Determine specific error based on method and data
        const char* error_msg = "Invalid JSON";
        const char* error_detail = "Request body contains invalid JSON";
        unsigned int http_status = MHD_HTTP_BAD_REQUEST;

        if (strcmp(method, "POST") == 0 && (!buffer->data || buffer->size == 0)) {
            error_msg = "Missing request body";
            error_detail = "POST requests must include a JSON body";
        }

        json_t *error_response = create_validation_error_response(error_msg, error_detail);

        api_send_json_response(connection, error_response, http_status);
        json_decref(error_response);
        return MHD_NO; // Response sent - processing complete
    }
    return MHD_YES; // Continue processing
}

enum MHD_Result handle_request_parsing(struct MHD_Connection *connection, const char* method,
                                         const char* upload_data, const size_t* upload_data_size,
                                         json_t** request_json) {
    *request_json = parse_request_data(connection, method, upload_data, upload_data_size);
    if (!*request_json) {
        // Determine specific error based on method and data
        const char* error_msg = "Invalid JSON";
        const char* error_detail = "Request body contains invalid JSON";
        unsigned int http_status = MHD_HTTP_BAD_REQUEST;

        if (method && strcmp(method, "POST") == 0 && (!upload_data || !upload_data_size || *upload_data_size == 0)) {
            error_msg = "Missing request body";
            error_detail = "POST requests must include a JSON body";
        }

        json_t *error_response = create_validation_error_response(error_msg, error_detail);

        api_send_json_response(connection, error_response, http_status);
        json_decref(error_response);
        return MHD_NO; // Response sent - processing complete
    }
    return MHD_YES; // Continue processing
}

enum MHD_Result handle_field_extraction(struct MHD_Connection *connection, json_t* request_json,
                                         int* query_ref, const char** database, json_t** params_json) {
    if (!extract_request_fields(request_json, query_ref, database, params_json)) {
        const char* error_msg = !json_object_get(request_json, "query_ref") || !json_is_integer(json_object_get(request_json, "query_ref"))
                                   ? "Missing or invalid query_ref" : "Missing or invalid database";
        const char* error_detail = !json_object_get(request_json, "query_ref") || !json_is_integer(json_object_get(request_json, "query_ref"))
                                      ? "query_ref must be an integer" : "database must be a string";

        json_t *error_response = create_validation_error_response(error_msg, error_detail);

        api_send_json_response(connection, error_response, MHD_HTTP_BAD_REQUEST);
        json_decref(error_response);
        return MHD_NO; // Response sent - processing complete
    }
    return MHD_YES; // Continue processing
}