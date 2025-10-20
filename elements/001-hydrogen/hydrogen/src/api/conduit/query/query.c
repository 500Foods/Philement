/*
 * Conduit Query Endpoint Implementation
 *
 * REST API endpoint for executing database queries by reference.
 */

#include "../conduit_service.h"
#include "query.h"
#include "../../../../src/api/api_utils.h"

// Main query handler
int conduit_query_handler(
    struct MHD_Connection* connection,
    const char* url,
    const char* method,
    const char* upload_data,
    size_t* upload_data_size,
    void** con_cls
) {
    (void)url;
    (void)upload_data;
    (void)upload_data_size;
    (void)con_cls;
    // Only accept POST requests
    if (strcmp(method, "POST") != 0) {
        return MHD_NO;
    }

    // Parse JSON request body
    ConduitQueryRequest* request = NULL;
    ConduitQueryResponse* response = NULL;
    int ret = MHD_NO;

    // Get POST data
    json_t* post_data_json = api_extract_post_data(connection);
    if (!post_data_json) {
        // Send error response
        json_t* error_json = json_object();
        json_object_set_new(error_json, "success", json_false());
        json_object_set_new(error_json, "error", json_string("No request body provided"));
        api_send_json_response(connection, error_json, MHD_HTTP_BAD_REQUEST);
        json_decref(error_json);
        return MHD_YES;
    }

    // Convert JSON to string for parsing
    char* post_data = json_dumps(post_data_json, JSON_COMPACT);
    json_decref(post_data_json);
    if (!post_data) {
        // Send error response
        json_t* error_json = json_object();
        json_object_set_new(error_json, "success", json_false());
        json_object_set_new(error_json, "error", json_string("Failed to process request body"));
        api_send_json_response(connection, error_json, MHD_HTTP_BAD_REQUEST);
        json_decref(error_json);
        return MHD_YES;
    }

    // Parse request
    request = conduit_query_parse_request(post_data);
    free(post_data);

    if (!request) {
        // Send error response
        json_t* error_json = json_object();
        json_object_set_new(error_json, "success", json_false());
        json_object_set_new(error_json, "error", json_string("Invalid JSON request format"));
        api_send_json_response(connection, error_json, MHD_HTTP_BAD_REQUEST);
        json_decref(error_json);
        return MHD_YES;
    }

    // Execute query
    response = conduit_query_execute(request);

    // Send response
    ret = conduit_query_send_response(connection, response);

    // Cleanup
    conduit_query_free_request(request);
    conduit_query_free_response(response);

    return ret;
}

// Parse JSON request into ConduitQueryRequest structure
ConduitQueryRequest* conduit_query_parse_request(const char* json_str) {
    json_t* root;
    json_error_t error;

    root = json_loads(json_str, 0, &error);
    if (!root || !json_is_object(root)) {
        return NULL;
    }

    ConduitQueryRequest* request = (ConduitQueryRequest*)calloc(1, sizeof(ConduitQueryRequest));
    if (!request) {
        json_decref(root);
        return NULL;
    }

    // Parse query_ref
    json_t* query_ref_json = json_object_get(root, "query_ref");
    if (!query_ref_json || !json_is_integer(query_ref_json)) {
        conduit_query_free_request(request);
        json_decref(root);
        return NULL;
    }
    request->query_ref = (int)json_integer_value(query_ref_json);

    // Parse database
    json_t* database_json = json_object_get(root, "database");
    if (database_json && json_is_string(database_json)) {
        request->database = strdup(json_string_value(database_json));
    }

    // Parse params (keep reference to JSON object)
    json_t* params_json = json_object_get(root, "params");
    if (params_json && json_is_object(params_json)) {
        request->params = json_incref(params_json);  // Increase ref count
    }

    json_decref(root);
    return request;
}

// Free request structure
void conduit_query_free_request(ConduitQueryRequest* request) {
    if (!request) return;

    free(request->database);
    if (request->params) {
        json_decref(request->params);
    }
    free(request);
}

// Execute query (placeholder implementation)
ConduitQueryResponse* conduit_query_execute(const ConduitQueryRequest* request) {
    ConduitQueryResponse* response = (ConduitQueryResponse*)calloc(1, sizeof(ConduitQueryResponse));
    if (!response) {
        return NULL;
    }

    // Placeholder: return not implemented error
    response->success = false;
    response->query_ref = request->query_ref;
    response->error_message = strdup("Conduit query execution not yet implemented");
    response->execution_time_ms = 0;

    return response;
}

// Free response structure
void conduit_query_free_response(ConduitQueryResponse* response) {
    if (!response) return;

    free(response->description);
    if (response->rows) {
        json_decref(response->rows);
    }
    free(response->queue_used);
    free(response->error_message);
    free(response);
}

// Send JSON response
int conduit_query_send_response(struct MHD_Connection* connection, ConduitQueryResponse* response) {
    json_t* json_response = json_object();

    json_object_set_new(json_response, "success", response->success ? json_true() : json_false());
    json_object_set_new(json_response, "query_ref", json_integer(response->query_ref));

    if (response->description) {
        json_object_set_new(json_response, "description", json_string(response->description));
    }

    if (response->success) {
        if (response->rows) {
            json_object_set(json_response, "rows", response->rows);
        }
        json_object_set_new(json_response, "row_count", json_integer((json_int_t)response->row_count));
        json_object_set_new(json_response, "column_count", json_integer((json_int_t)response->column_count));
        json_object_set_new(json_response, "execution_time_ms", json_integer((json_int_t)response->execution_time_ms));
        if (response->queue_used) {
            json_object_set_new(json_response, "queue_used", json_string(response->queue_used));
        }
    } else {
        if (response->error_message) {
            json_object_set_new(json_response, "error", json_string(response->error_message));
        }
    }

    unsigned int status_code = response->success ? (unsigned int)MHD_HTTP_OK : (unsigned int)MHD_HTTP_INTERNAL_SERVER_ERROR;
    int ret = api_send_json_response(connection, json_response, status_code);

    json_decref(json_response);
    return ret;
}