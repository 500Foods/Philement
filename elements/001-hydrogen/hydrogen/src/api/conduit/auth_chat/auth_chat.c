/*
 * Authenticated Chat API Endpoint Implementation
 *
 * Handles single-model chat requests with JWT authentication.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/api/api_utils.h>
#include <src/api/conduit/helpers/auth_jwt_helper.h>

// Local includes
#include "auth_chat.h"
#include "../chat_common/chat_engine_cache.h"
#include "../chat_common/chat_request_builder.h"
#include "../chat_common/chat_response_parser.h"
#include "../chat_common/chat_proxy.h"
#include "../chat_common/chat_metrics.h"
#include "../conduit_service.h"

// External reference to global queue manager
extern DatabaseQueueManager* global_queue_manager;

// Handle auth_chat request
enum MHD_Result handle_auth_chat_request(struct MHD_Connection *connection,
                                          const char *url,
                                          const char *method,
                                          const char *upload_data,
                                          size_t *upload_data_size,
                                          void **con_cls) {
    (void)url;

    // Use common POST body buffering
    ApiPostBuffer *buffer = NULL;
    ApiBufferResult buf_result = api_buffer_post_data(method, upload_data, upload_data_size, con_cls, &buffer);

    if (buf_result == API_BUFFER_CONTINUE) {
        return MHD_YES;
    }
    if (buf_result == API_BUFFER_ERROR) {
        return api_send_error_and_cleanup(connection, con_cls, "Request processing error", MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    // Validate HTTP method
    if (strcmp(method, "POST") != 0) {
        api_free_post_buffer(con_cls);
        json_t *error = auth_chat_build_error_response("Method not allowed - use POST");
        enum MHD_Result ret = api_send_json_response(connection, error, MHD_HTTP_METHOD_NOT_ALLOWED);
        json_decref(error);
        return ret;
    }

    // Extract and validate JWT
    jwt_validation_result_t jwt_result = {0};
    const char *auth_header = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Authorization");

    if (!extract_and_validate_jwt(auth_header, &jwt_result)) {
        api_free_post_buffer(con_cls);
        const char *error_msg = jwt_result.claims ? "Invalid token" : "Authentication required";
        json_t *error = auth_chat_build_error_response(error_msg);
        enum MHD_Result ret = api_send_json_response(connection, error, MHD_HTTP_UNAUTHORIZED);
        json_decref(error);
        free_jwt_validation_result(&jwt_result);
        return ret;
    }

    // Validate claims
    if (!validate_jwt_claims(&jwt_result, connection)) {
        api_free_post_buffer(con_cls);
        free_jwt_validation_result(&jwt_result);
        return MHD_NO;
    }

    // Extract database from JWT claim
    const char *database = jwt_result.claims->database;
    if (!database) {
        api_free_post_buffer(con_cls);
        json_t *error = auth_chat_build_error_response("Token missing database claim");
        enum MHD_Result ret = api_send_json_response(connection, error, MHD_HTTP_FORBIDDEN);
        json_decref(error);
        free_jwt_validation_result(&jwt_result);
        return ret;
    }

    // Parse request JSON
    json_error_t json_error;
    json_t *request_json = json_loads(buffer->data, 0, &json_error);
    api_free_post_buffer(con_cls);

    if (!request_json) {
        free_jwt_validation_result(&jwt_result);
        json_t *error_response = auth_chat_build_error_response("Invalid JSON in request body");
        enum MHD_Result ret = api_send_json_response(connection, error_response, MHD_HTTP_BAD_REQUEST);
        json_decref(error_response);
        return ret;
    }

    // Parse request parameters
    char *engine_name = NULL;
    json_t *messages = NULL;
    double temperature = -1.0;
    int max_tokens = -1;
    bool stream = false;
    char *error_message = NULL;

    bool parse_ok = auth_chat_parse_request(request_json, &engine_name,
                                             &messages, &temperature, &max_tokens,
                                             &stream, &error_message);

    if (!parse_ok) {
        free_jwt_validation_result(&jwt_result);
        json_decref(request_json);
        json_t *error_response = auth_chat_build_error_response(error_message ? error_message : "Invalid request");
        free(error_message);
        enum MHD_Result ret = api_send_json_response(connection, error_response, MHD_HTTP_BAD_REQUEST);
        json_decref(error_response);
        return ret;
    }

    // Check if streaming is requested (not yet implemented)
    if (stream) {
        free(engine_name);
        free_jwt_validation_result(&jwt_result);
        json_decref(request_json);
        json_t *error_response = auth_chat_build_error_response("Streaming not yet implemented");
        enum MHD_Result ret = api_send_json_response(connection, error_response, MHD_HTTP_NOT_IMPLEMENTED);
        json_decref(error_response);
        return ret;
    }

    // Get database queue
    DatabaseQueue *db_queue = database_queue_manager_get_database(global_queue_manager, database);
    if (!db_queue) {
        free(engine_name);
        free_jwt_validation_result(&jwt_result);
        json_decref(request_json);
        json_t *error_response = auth_chat_build_error_response("Database not found");
        enum MHD_Result ret = api_send_json_response(connection, error_response, MHD_HTTP_BAD_REQUEST);
        json_decref(error_response);
        return ret;
    }

    // Get CEC
    ChatEngineCache *cec = db_queue->chat_engine_cache;
    if (!cec) {
        free(engine_name);
        free_jwt_validation_result(&jwt_result);
        json_decref(request_json);
        json_t *error_response = auth_chat_build_error_response("Chat not enabled for this database");
        enum MHD_Result ret = api_send_json_response(connection, error_response, MHD_HTTP_SERVICE_UNAVAILABLE);
        json_decref(error_response);
        return ret;
    }

    // Look up engine
    const ChatEngineConfig *engine = NULL;
    if (engine_name) {
        engine = chat_engine_cache_lookup_by_name(cec, engine_name);
    } else {
        engine = chat_engine_cache_get_default(cec);
    }

    if (!engine) {
        free(engine_name);
        free_jwt_validation_result(&jwt_result);
        json_decref(request_json);
        json_t *error_response = auth_chat_build_error_response(engine_name ? "Engine not found" : "No default engine configured");
        enum MHD_Result ret = api_send_json_response(connection, error_response, MHD_HTTP_BAD_REQUEST);
        json_decref(error_response);
        return ret;
    }

    // Check engine health
    if (!engine->is_healthy) {
        free(engine_name);
        free_jwt_validation_result(&jwt_result);
        json_decref(request_json);
        json_t *error_response = auth_chat_build_error_response("Engine is currently unavailable");
        enum MHD_Result ret = api_send_json_response(connection, error_response, MHD_HTTP_SERVICE_UNAVAILABLE);
        json_decref(error_response);
        return ret;
    }

    // Build request parameters
    ChatRequestParams params = chat_request_params_default();
    if (temperature >= 0.0) {
        params.temperature = temperature;
    } else {
        params.temperature = engine->temperature_default;
    }
    if (max_tokens > 0) {
        params.max_tokens = max_tokens;
    } else {
        params.max_tokens = engine->max_tokens;
    }
    params.stream = stream;

    // Convert messages JSON to ChatMessage list
    ChatMessage *chat_messages = NULL;
    size_t msg_count = json_array_size(messages);
    for (size_t i = 0; i < msg_count; i++) {
        json_t *msg = json_array_get(messages, i);
        if (!json_is_object(msg)) continue;

        json_t *role_obj = json_object_get(msg, "role");
        json_t *content_obj = json_object_get(msg, "content");

        if (!role_obj || !content_obj) continue;

        const char *role_str = json_string_value(role_obj);
        ChatMessageRole role = chat_message_role_from_string(role_str);

        // Handle content as string or array (multimodal)
        char *content_str = NULL;
        if (json_is_string(content_obj)) {
            content_str = strdup(json_string_value(content_obj));
        } else if (json_is_array(content_obj)) {
            // For now, convert array content to JSON string for transport
            content_str = json_dumps(content_obj, JSON_COMPACT);
        }

        if (content_str) {
            ChatMessage *new_msg = chat_message_create(role, content_str, NULL);
            chat_messages = chat_message_list_append(chat_messages, new_msg);
            free(content_str);
        }
    }

    // Build request JSON for provider
    json_t *provider_request = chat_request_build(engine, chat_messages, &params);

    // Convert to string
    char *request_body = chat_request_to_json_string(provider_request, true);
    json_decref(provider_request);
    chat_message_list_destroy(chat_messages);

    if (!request_body) {
        free(engine_name);
        free_jwt_validation_result(&jwt_result);
        json_decref(request_json);
        json_t *error_response = auth_chat_build_error_response("Failed to build request");
        enum MHD_Result ret = api_send_json_response(connection, error_response, MHD_HTTP_INTERNAL_SERVER_ERROR);
        json_decref(error_response);
        return ret;
    }

    // Send request to AI API
    ChatProxyConfig proxy_config = chat_proxy_get_default_config();
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    ChatProxyResult *proxy_result = chat_proxy_send_with_retry(engine, request_body, &proxy_config);
    free(request_body);

    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double response_time_ms = (double)(end_time.tv_sec - start_time.tv_sec) * 1000.0 +
                               (double)(end_time.tv_nsec - start_time.tv_nsec) / 1000000.0;

    // Update engine health
    bool request_success = chat_proxy_result_is_success(proxy_result);
    chat_engine_config_update_health((ChatEngineConfig*)engine, request_success, response_time_ms);

    // Record metrics
    if (request_success) {
        chat_metrics_conversation(database, engine->name);
        chat_metrics_response_time(database, engine->name, response_time_ms);
    } else {
        chat_metrics_error(database, engine->name, "proxy_error");
    }

    // Parse response
    if (!request_success) {
        free(engine_name);
        free_jwt_validation_result(&jwt_result);
        json_decref(request_json);

        const char *error_msg = proxy_result->error_message ? proxy_result->error_message : "Request failed";
        json_t *error_response = auth_chat_build_error_response(error_msg);
        chat_proxy_result_destroy(proxy_result);
        enum MHD_Result ret = api_send_json_response(connection, error_response, MHD_HTTP_BAD_GATEWAY);
        json_decref(error_response);
        return ret;
    }

    // Parse the AI response
    ChatParsedResponse *parsed = chat_response_parse(proxy_result->response_body, engine->provider);

    if (!parsed || !parsed->success) {
        free(engine_name);
        free_jwt_validation_result(&jwt_result);
        json_decref(request_json);

        const char *error_msg = parsed && parsed->error_message ? parsed->error_message : "Failed to parse response";
        json_t *error_response = auth_chat_build_error_response(error_msg);
        chat_parsed_response_destroy(parsed);
        chat_proxy_result_destroy(proxy_result);
        enum MHD_Result ret = api_send_json_response(connection, error_response, MHD_HTTP_BAD_GATEWAY);
        json_decref(error_response);
        return ret;
    }

    // Record token metrics
    if (parsed->total_tokens > 0) {
        chat_metrics_tokens(database, engine->name, "prompt", parsed->prompt_tokens);
        chat_metrics_tokens(database, engine->name, "completion", parsed->completion_tokens);
    }

    // Build success response
    json_t *response = auth_chat_build_response(
        database,
        engine->name,
        parsed->model ? parsed->model : engine->model,
        parsed->content ? parsed->content : "",
        parsed->prompt_tokens,
        parsed->completion_tokens,
        parsed->total_tokens,
        parsed->finish_reason ? parsed->finish_reason : "stop",
        response_time_ms
    );

    // Cleanup
    free(engine_name);
    free_jwt_validation_result(&jwt_result);
    json_decref(request_json);
    chat_parsed_response_destroy(parsed);
    chat_proxy_result_destroy(proxy_result);

    enum MHD_Result ret = api_send_json_response(connection, response, MHD_HTTP_OK);
    json_decref(response);
    return ret;
}

// Parse and validate request
bool auth_chat_parse_request(json_t *request_json,
                              char **engine,
                              json_t **messages,
                              double *temperature,
                              int *max_tokens,
                              bool *stream,
                              char **error_message) {
    (void)request_json;
    *engine = NULL;
    *messages = NULL;
    *temperature = -1.0;
    *max_tokens = -1;
    *stream = false;
    *error_message = NULL;

    // Extract messages (required)
    json_t *messages_obj = json_object_get(request_json, "messages");
    if (!messages_obj || !json_is_array(messages_obj)) {
        *error_message = strdup("Missing or invalid 'messages' array");
        return false;
    }
    *messages = messages_obj;

    // Extract engine (optional)
    json_t *engine_obj = json_object_get(request_json, "engine");
    if (engine_obj && json_is_string(engine_obj)) {
        *engine = strdup(json_string_value(engine_obj));
    }

    // Extract temperature (optional)
    json_t *temp_obj = json_object_get(request_json, "temperature");
    if (temp_obj && json_is_number(temp_obj)) {
        *temperature = json_number_value(temp_obj);
        if (*temperature < 0.0 || *temperature > 2.0) {
            *error_message = strdup("Temperature must be between 0.0 and 2.0");
            return false;
        }
    }

    // Extract max_tokens (optional)
    json_t *tokens_obj = json_object_get(request_json, "max_tokens");
    if (tokens_obj && json_is_integer(tokens_obj)) {
        *max_tokens = (int)json_integer_value(tokens_obj);
        if (*max_tokens < 1 || *max_tokens > 128000) {
            *error_message = strdup("max_tokens must be between 1 and 128000");
            return false;
        }
    }

    // Extract stream (optional)
    json_t *stream_obj = json_object_get(request_json, "stream");
    if (stream_obj && json_is_boolean(stream_obj)) {
        *stream = json_boolean_value(stream_obj);
    }

    return true;
}

// Build success response
json_t* auth_chat_build_response(const char *database,
                                  const char *engine,
                                  const char *model,
                                  const char *content,
                                  int prompt_tokens,
                                  int completion_tokens,
                                  int total_tokens,
                                  const char *finish_reason,
                                  double response_time_ms) {
    json_t *response = json_object();
    json_object_set_new(response, "success", json_true());
    json_object_set_new(response, "database", json_string(database));
    json_object_set_new(response, "engine", json_string(engine));
    json_object_set_new(response, "model", json_string(model));
    json_object_set_new(response, "content", json_string(content));

    json_t *tokens = json_object();
    json_object_set_new(tokens, "prompt", json_integer(prompt_tokens));
    json_object_set_new(tokens, "completion", json_integer(completion_tokens));
    json_object_set_new(tokens, "total", json_integer(total_tokens));
    json_object_set_new(response, "tokens", tokens);

    json_object_set_new(response, "finish_reason", json_string(finish_reason));
    json_object_set_new(response, "response_time_ms", json_real(response_time_ms));

    return response;
}

// Build error response
json_t* auth_chat_build_error_response(const char *error_message) {
    json_t *response = json_object();
    json_object_set_new(response, "success", json_false());
    json_object_set_new(response, "error", json_string(error_message));
    return response;
}
