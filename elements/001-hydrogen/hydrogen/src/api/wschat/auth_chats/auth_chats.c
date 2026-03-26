/*
 * Authenticated Multi-Model Chat API Endpoint Implementation
 *
 * Handles broadcast chat requests to multiple AI models simultaneously.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/api/api_utils.h>
#include <src/api/conduit/helpers/auth_jwt_helper.h>

// Local includes
#include "auth_chats.h"
#include "../helpers/engine_cache.h"
#include "../helpers/req_builder.h"
#include "../helpers/resp_parser.h"
#include "../helpers/proxy.h"
#include "../helpers/metrics.h"
#include <src/api/conduit/conduit_service.h>

// External reference to global queue manager
extern DatabaseQueueManager* global_queue_manager;

// Generate a simple UUID string (simplified version)
static void generate_broadcast_id(char *buffer, size_t buffer_size) {
    const char *hex = "0123456789abcdef";
    size_t idx = 0;
    for (int i = 0; i < 36 && idx < buffer_size - 1; i++) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            buffer[idx++] = '-';
        } else {
            buffer[idx++] = hex[rand() % 16];
        }
    }
    buffer[idx] = '\0';
}

// Handle auth_chats request
enum MHD_Result handle_auth_chats_request(struct MHD_Connection *connection,
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
        json_t *error = json_object();
        json_object_set_new(error, "success", json_false());
        json_object_set_new(error, "error", json_string("Method not allowed - use POST"));
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
        json_t *error = json_object();
        json_object_set_new(error, "success", json_false());
        json_object_set_new(error, "error", json_string(error_msg));
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
        json_t *error = json_object();
        json_object_set_new(error, "success", json_false());
        json_object_set_new(error, "error", json_string("Token missing database claim"));
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
        json_t *error = json_object();
        json_object_set_new(error, "success", json_false());
        json_object_set_new(error, "error", json_string("Invalid JSON in request body"));
        enum MHD_Result ret = api_send_json_response(connection, error, MHD_HTTP_BAD_REQUEST);
        json_decref(error);
        return ret;
    }

    // Extract engines array (required)
    json_t *engines_array = json_object_get(request_json, "engines");
    if (!engines_array || !json_is_array(engines_array)) {
        free_jwt_validation_result(&jwt_result);
        json_decref(request_json);
        json_t *error = json_object();
        json_object_set_new(error, "success", json_false());
        json_object_set_new(error, "error", json_string("Missing or invalid 'engines' array"));
        enum MHD_Result ret = api_send_json_response(connection, error, MHD_HTTP_BAD_REQUEST);
        json_decref(error);
        return ret;
    }

    size_t engine_count = json_array_size(engines_array);
    if (engine_count == 0 || engine_count > 10) {
        free_jwt_validation_result(&jwt_result);
        json_decref(request_json);
        json_t *error = json_object();
        json_object_set_new(error, "success", json_false());
        json_object_set_new(error, "error", json_string("Engines array must contain 1-10 engine names"));
        enum MHD_Result ret = api_send_json_response(connection, error, MHD_HTTP_BAD_REQUEST);
        json_decref(error);
        return ret;
    }

    // Extract messages (required)
    json_t *messages = json_object_get(request_json, "messages");
    if (!messages || !json_is_array(messages)) {
        free_jwt_validation_result(&jwt_result);
        json_decref(request_json);
        json_t *error = json_object();
        json_object_set_new(error, "success", json_false());
        json_object_set_new(error, "error", json_string("Missing or invalid 'messages' array"));
        enum MHD_Result ret = api_send_json_response(connection, error, MHD_HTTP_BAD_REQUEST);
        json_decref(error);
        return ret;
    }

    // Get database queue
    DatabaseQueue *db_queue = database_queue_manager_get_database(global_queue_manager, database);
    if (!db_queue) {
        free_jwt_validation_result(&jwt_result);
        json_decref(request_json);
        json_t *error = json_object();
        json_object_set_new(error, "success", json_false());
        json_object_set_new(error, "error", json_string("Database not found"));
        enum MHD_Result ret = api_send_json_response(connection, error, MHD_HTTP_BAD_REQUEST);
        json_decref(error);
        return ret;
    }

    // Get CEC
    ChatEngineCache *cec = db_queue->chat_engine_cache;
    if (!cec) {
        free_jwt_validation_result(&jwt_result);
        json_decref(request_json);
        json_t *error = json_object();
        json_object_set_new(error, "success", json_false());
        json_object_set_new(error, "error", json_string("Chat not enabled for this database"));
        enum MHD_Result ret = api_send_json_response(connection, error, MHD_HTTP_SERVICE_UNAVAILABLE);
        json_decref(error);
        return ret;
    }

    // Build multi-request array
    ChatMultiRequest *multi_requests = (ChatMultiRequest*)calloc(engine_count, sizeof(ChatMultiRequest));
    if (!multi_requests) {
        free_jwt_validation_result(&jwt_result);
        json_decref(request_json);
        json_t *error = json_object();
        json_object_set_new(error, "success", json_false());
        json_object_set_new(error, "error", json_string("Failed to allocate request array"));
        enum MHD_Result ret = api_send_json_response(connection, error, MHD_HTTP_INTERNAL_SERVER_ERROR);
        json_decref(error);
        return ret;
    }

    // Extract optional parameters
    double temperature = -1.0;
    int max_tokens = -1;

    json_t *temp_obj = json_object_get(request_json, "temperature");
    if (temp_obj && json_is_number(temp_obj)) {
        temperature = json_number_value(temp_obj);
    }

    json_t *tokens_obj = json_object_get(request_json, "max_tokens");
    if (tokens_obj && json_is_integer(tokens_obj)) {
        max_tokens = (int)json_integer_value(tokens_obj);
    }

    // Convert messages JSON to ChatMessage list once (shared across all engines)
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

        char *content_str = NULL;
        if (json_is_string(content_obj)) {
            content_str = strdup(json_string_value(content_obj));
        } else if (json_is_array(content_obj)) {
            content_str = json_dumps(content_obj, JSON_COMPACT);
        }

        if (content_str) {
            ChatMessage *new_msg = chat_message_create(role, content_str, NULL);
            chat_messages = chat_message_list_append(chat_messages, new_msg);
            free(content_str);
        }
    }

    // Lookup engines and build requests
    size_t valid_requests = 0;
    for (size_t i = 0; i < engine_count; i++) {
        json_t *engine_name_obj = json_array_get(engines_array, i);
        if (!json_is_string(engine_name_obj)) continue;

        const char *engine_name = json_string_value(engine_name_obj);
        const ChatEngineConfig *engine = chat_engine_cache_lookup_by_name(cec, engine_name);

        if (!engine || !engine->is_healthy) {
            continue;
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

        // Build request JSON for provider
        json_t *provider_request = chat_request_build(engine, chat_messages, &params);
        const char *request_body = chat_request_to_json_string(provider_request, true);
        json_decref(provider_request);

        if (request_body) {
            multi_requests[valid_requests].engine = engine;
            multi_requests[valid_requests].request_json = request_body;
            multi_requests[valid_requests].engine_name = strdup(engine->name);
            valid_requests++;
        }
    }

    chat_message_list_destroy(chat_messages);

    if (valid_requests == 0) {
        free(multi_requests);
        free_jwt_validation_result(&jwt_result);
        json_decref(request_json);
        json_t *error = json_object();
        json_object_set_new(error, "success", json_false());
        json_object_set_new(error, "error", json_string("No valid or healthy engines found"));
        enum MHD_Result ret = api_send_json_response(connection, error, MHD_HTTP_BAD_REQUEST);
        json_decref(error);
        return ret;
    }

    // Send requests in parallel
    ChatProxyConfig proxy_config = chat_proxy_get_default_config();
    ChatMultiResult *multi_result = chat_proxy_send_multi(multi_requests, valid_requests, &proxy_config);

    // Save engine names before freeing multi_requests
    char **engine_names = (char**)calloc(valid_requests, sizeof(char*));
    if (!engine_names) {
        // Cleanup request bodies on allocation failure
        for (size_t i = 0; i < valid_requests; i++) {
            free((char*)multi_requests[i].request_json);
            free(multi_requests[i].engine_name);
        }
        free(multi_requests);
        free_jwt_validation_result(&jwt_result);
        json_decref(request_json);
        json_t *error = json_object();
        json_object_set_new(error, "success", json_false());
        json_object_set_new(error, "error", json_string("Failed to allocate engine names array"));
        enum MHD_Result ret = api_send_json_response(connection, error, MHD_HTTP_INTERNAL_SERVER_ERROR);
        json_decref(error);
        return ret;
    }
    for (size_t i = 0; i < valid_requests; i++) {
        engine_names[i] = multi_requests[i].engine_name ? strdup(multi_requests[i].engine_name) : NULL;
    }

    // Cleanup request bodies
    for (size_t i = 0; i < valid_requests; i++) {
        free((char*)multi_requests[i].request_json);
        free(multi_requests[i].engine_name);
    }
    free(multi_requests);

    // Generate broadcast ID
    char broadcast_id[37];
    generate_broadcast_id(broadcast_id, sizeof(broadcast_id));

    // Build response
    json_t *response = json_object();
    json_object_set_new(response, "success", json_true());
    json_object_set_new(response, "database", json_string(database));
    json_object_set_new(response, "broadcast_id", json_string(broadcast_id));
    json_object_set_new(response, "engines_requested", json_integer((json_int_t)engine_count));
    json_object_set_new(response, "engines_succeeded", json_integer((json_int_t)(multi_result ? multi_result->success_count : 0)));
    json_object_set_new(response, "engines_failed", json_integer((json_int_t)(multi_result ? multi_result->failure_count : 0)));

    // Build results array
    json_t *results_array = json_array();
    if (multi_result) {
        for (size_t i = 0; i < multi_result->count; i++) {
            ChatProxyResult *proxy_result = multi_result->results[i];
            json_t *result_obj = json_object();

            // Get engine name from saved context
            const char *engine_name = engine_names[i] ? engine_names[i] : "unknown";

            json_object_set_new(result_obj, "engine", json_string(engine_name));

            if (chat_proxy_result_is_success(proxy_result)) {
                json_object_set_new(result_obj, "success", json_true());

                // Parse response
                ChatParsedResponse *parsed = chat_response_parse(proxy_result->response_body, CEC_PROVIDER_OPENAI);
                if (parsed && parsed->success) {
                    json_object_set_new(result_obj, "model", json_string(parsed->model ? parsed->model : engine_name));
                    json_object_set_new(result_obj, "content", json_string(parsed->content ? parsed->content : ""));

                    json_t *tokens = json_object();
                    json_object_set_new(tokens, "prompt", json_integer(parsed->prompt_tokens));
                    json_object_set_new(tokens, "completion", json_integer(parsed->completion_tokens));
                    json_object_set_new(tokens, "total", json_integer(parsed->total_tokens));
                    json_object_set_new(result_obj, "tokens", tokens);

                    json_object_set_new(result_obj, "finish_reason", json_string(parsed->finish_reason ? parsed->finish_reason : "stop"));

                    // Record metrics
                    chat_metrics_conversation(database, engine_name);
                    chat_metrics_tokens(database, engine_name, "prompt", parsed->prompt_tokens);
                    chat_metrics_tokens(database, engine_name, "completion", parsed->completion_tokens);
                } else {
                    json_object_set_new(result_obj, "content", json_string(""));
                    json_object_set_new(result_obj, "error", json_string("Failed to parse response"));
                }
                chat_parsed_response_destroy(parsed);

                json_object_set_new(result_obj, "response_time_ms", json_real(proxy_result->total_time_ms));
            } else {
                json_object_set_new(result_obj, "success", json_false());
                const char *error_msg = proxy_result->error_message ? proxy_result->error_message : "Request failed";
                json_object_set_new(result_obj, "error", json_string(error_msg));
                json_object_set_new(result_obj, "response_time_ms", json_real(proxy_result->total_time_ms));

                // Record error metric
                chat_metrics_error(database, engine_name, "proxy_error");
            }

            json_array_append_new(results_array, result_obj);
        }
    }
    json_object_set_new(response, "results", results_array);

    // Add timing
    json_t *timing = json_object();
    if (multi_result) {
        json_object_set_new(timing, "total_time_ms", json_real(multi_result->total_time_ms));
    } else {
        json_object_set_new(timing, "total_time_ms", json_real(0.0));
    }
    json_object_set_new(response, "timing", timing);

    // Cleanup
    for (size_t i = 0; i < valid_requests; i++) {
        free(engine_names[i]);
    }
    free(engine_names);
    chat_multi_result_destroy(multi_result);
    free_jwt_validation_result(&jwt_result);
    json_decref(request_json);

    enum MHD_Result ret = api_send_json_response(connection, response, MHD_HTTP_OK);
    json_decref(response);
    return ret;
}
