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

// Unity test mocks for network-dependent fan-out calls.
#if defined(USE_MOCK_API_UTILS)
#include <unity/mocks/mock_api_utils.h>
#endif
#if defined(USE_MOCK_AUTH_SERVICE_JWT)
#include <unity/mocks/mock_auth_service_jwt.h>
#endif
#if defined(USE_MOCK_DBQUEUE)
#include <unity/mocks/mock_dbqueue.h>
#endif
#if defined(USE_MOCK_AUTH_CHAT_DEPS)
#include <unity/mocks/mock_auth_chat_deps.h>
#endif

// External reference to global queue manager
extern DatabaseQueueManager* global_queue_manager;

// Generate a simple UUID string (simplified version)
void auth_chats_generate_broadcast_id(char *buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) {
        return;
    }

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

jwt_validation_result_t auth_chats_validate_jwt_from_header(const char *auth_header) {
    jwt_validation_result_t result = {0};
    if (!auth_header || strncmp(auth_header, "Bearer ", 7) != 0) {
        result.error = JWT_ERROR_INVALID_FORMAT;
        return result;
    }
    return validate_jwt(auth_header + 7, NULL);
}

json_t *auth_chats_build_error_response(const char *error_message) {
    json_t *response = json_object();
    json_object_set_new(response, "success", json_false());
    json_object_set_new(response, "error", json_string(error_message ? error_message : "Unknown error"));
    return response;
}

bool auth_chats_parse_request(json_t *request_json,
                              json_t **engines,
                              json_t **messages,
                              size_t *engine_count,
                              double *temperature,
                              int *max_tokens,
                              char **error_message) {
    if (!request_json || !engines || !messages || !engine_count || !temperature ||
        !max_tokens || !error_message) {
        if (error_message) {
            *error_message = strdup("Invalid parse request parameters");
        }
        return false;
    }

    *engines = NULL;
    *messages = NULL;
    *engine_count = 0;
    *temperature = -1.0;
    *max_tokens = -1;
    *error_message = NULL;

    json_t *engines_obj = json_object_get(request_json, "engines");
    if (!engines_obj || !json_is_array(engines_obj)) {
        *error_message = strdup("Missing or invalid 'engines' array");
        return false;
    }

    size_t count = json_array_size(engines_obj);
    if (count == 0 || count > 10) {
        *error_message = strdup("Engines array must contain 1-10 engine names");
        return false;
    }

    json_t *messages_obj = json_object_get(request_json, "messages");
    if (!messages_obj || !json_is_array(messages_obj)) {
        *error_message = strdup("Missing or invalid 'messages' array");
        return false;
    }

    json_t *temperature_obj = json_object_get(request_json, "temperature");
    if (temperature_obj && json_is_number(temperature_obj)) {
        *temperature = json_number_value(temperature_obj);
    }

    json_t *max_tokens_obj = json_object_get(request_json, "max_tokens");
    if (max_tokens_obj && json_is_integer(max_tokens_obj)) {
        *max_tokens = (int)json_integer_value(max_tokens_obj);
    }

    *engines = engines_obj;
    *messages = messages_obj;
    *engine_count = count;
    return true;
}

ChatMessage *auth_chats_messages_json_to_list(json_t *messages) {
    if (!messages || !json_is_array(messages)) {
        return NULL;
    }

    ChatMessage *chat_messages = NULL;
    size_t message_count = json_array_size(messages);
    for (size_t i = 0; i < message_count; i++) {
        json_t *message = json_array_get(messages, i);
        if (!json_is_object(message)) {
            continue;
        }

        json_t *role_obj = json_object_get(message, "role");
        json_t *content_obj = json_object_get(message, "content");
        if (!role_obj || !content_obj) {
            continue;
        }

        char *content = NULL;
        if (json_is_string(content_obj)) {
            content = strdup(json_string_value(content_obj));
        } else if (json_is_array(content_obj)) {
            content = json_dumps(content_obj, JSON_COMPACT);
        }
        if (!content) {
            continue;
        }

        ChatMessageRole role = chat_message_role_from_string(json_string_value(role_obj));
        ChatMessage *new_message = chat_message_create(role, content, NULL);
        if (new_message) {
            chat_messages = chat_message_list_append(chat_messages, new_message);
        }
        free(content);
    }
    return chat_messages;
}

ChatRequestParams auth_chats_resolve_request_params(const ChatEngineConfig *engine,
                                                     double temperature,
                                                     int max_tokens) {
    ChatRequestParams params = chat_request_params_default();
    if (!engine) {
        return params;
    }
    params.temperature = temperature >= 0.0 ? temperature : engine->temperature_default;
    params.max_tokens = max_tokens > 0 ? max_tokens : engine->max_tokens;
    return params;
}

size_t auth_chats_build_multi_requests(ChatEngineCache *cache,
                                       json_t *engines,
                                       const ChatMessage *messages,
                                       double temperature,
                                       int max_tokens,
                                       ChatMultiRequest *requests) {
    if (!cache || !engines || !json_is_array(engines) || !requests) {
        return 0;
    }

    size_t valid_requests = 0;
    size_t engine_count = json_array_size(engines);
    for (size_t i = 0; i < engine_count; i++) {
        json_t *engine_name_obj = json_array_get(engines, i);
        if (!json_is_string(engine_name_obj)) {
            continue;
        }

        const ChatEngineConfig *engine = chat_engine_cache_lookup_by_name(
            cache, json_string_value(engine_name_obj));
        if (!engine || !engine->is_healthy) {
            continue;
        }

        ChatRequestParams params = auth_chats_resolve_request_params(engine, temperature, max_tokens);
        json_t *provider_request = chat_request_build(engine, messages, &params);
        char *request_body = chat_request_to_json_string(provider_request, true);
        json_decref(provider_request);
        if (!request_body) {
            continue;
        }

        char *engine_name = strdup(engine->name);
        if (!engine_name) {
            free(request_body);
            continue;
        }
        requests[valid_requests].engine = engine;
        requests[valid_requests].request_json = request_body;
        requests[valid_requests].engine_name = engine_name;
        valid_requests++;
    }
    return valid_requests;
}

void auth_chats_free_multi_requests(ChatMultiRequest *requests, size_t request_count) {
    if (!requests) {
        return;
    }
    for (size_t i = 0; i < request_count; i++) {
        free((char *)requests[i].request_json);
        free(requests[i].engine_name);
    }
    free(requests);
}

char **auth_chats_copy_engine_names(const ChatMultiRequest *requests, size_t request_count) {
    if (!requests || request_count == 0) {
        return NULL;
    }
    char **engine_names = calloc(request_count, sizeof(char *));
    if (!engine_names) {
        return NULL;
    }
    for (size_t i = 0; i < request_count; i++) {
        if (requests[i].engine_name) {
            engine_names[i] = strdup(requests[i].engine_name);
        }
    }
    return engine_names;
}

void auth_chats_free_engine_names(char **engine_names, size_t engine_count) {
    if (!engine_names) {
        return;
    }
    for (size_t i = 0; i < engine_count; i++) {
        free(engine_names[i]);
    }
    free(engine_names);
}

json_t *auth_chats_build_response(const char *database,
                                  const char *broadcast_id,
                                  size_t engines_requested,
                                  char **engine_names,
                                  const ChatMultiResult *multi_result) {
    json_t *response = json_object();
    json_object_set_new(response, "success", json_true());
    json_object_set_new(response, "database", json_string(database));
    json_object_set_new(response, "broadcast_id", json_string(broadcast_id));
    json_object_set_new(response, "engines_requested", json_integer((json_int_t)engines_requested));
    json_object_set_new(response, "engines_succeeded",
                        json_integer((json_int_t)(multi_result ? multi_result->success_count : 0)));
    json_object_set_new(response, "engines_failed",
                        json_integer((json_int_t)(multi_result ? multi_result->failure_count : 0)));

    json_t *results = json_array();
    if (multi_result) {
        for (size_t i = 0; i < multi_result->count; i++) {
            ChatProxyResult *proxy_result = multi_result->results[i];
            const char *engine_name = engine_names && engine_names[i] ? engine_names[i] : "unknown";
            json_t *result = json_object();
            json_object_set_new(result, "engine", json_string(engine_name));

            if (chat_proxy_result_is_success(proxy_result)) {
                json_object_set_new(result, "success", json_true());
                ChatParsedResponse *parsed = chat_response_parse(
                    proxy_result->response_body, CEC_PROVIDER_OPENAI);
                if (parsed && parsed->success) {
                    json_object_set_new(result, "model",
                                        json_string(parsed->model ? parsed->model : engine_name));
                    json_object_set_new(result, "content",
                                        json_string(parsed->content ? parsed->content : ""));
                    json_t *tokens = json_object();
                    json_object_set_new(tokens, "prompt", json_integer(parsed->prompt_tokens));
                    json_object_set_new(tokens, "completion", json_integer(parsed->completion_tokens));
                    json_object_set_new(tokens, "total", json_integer(parsed->total_tokens));
                    json_object_set_new(result, "tokens", tokens);
                    json_object_set_new(result, "finish_reason",
                                        json_string(parsed->finish_reason ? parsed->finish_reason : "stop"));
                    chat_metrics_conversation(database, engine_name);
                    chat_metrics_tokens(database, engine_name, "prompt", parsed->prompt_tokens);
                    chat_metrics_tokens(database, engine_name, "completion", parsed->completion_tokens);
                } else {
                    json_object_set_new(result, "content", json_string(""));
                    json_object_set_new(result, "error", json_string("Failed to parse response"));
                }
                chat_parsed_response_destroy(parsed);
            } else {
                json_object_set_new(result, "success", json_false());
                const char *error = proxy_result && proxy_result->error_message
                    ? proxy_result->error_message : "Request failed";
                json_object_set_new(result, "error", json_string(error));
                chat_metrics_error(database, engine_name, "proxy_error");
            }
            json_object_set_new(result, "response_time_ms",
                                json_real(proxy_result ? proxy_result->total_time_ms : 0.0));
            json_array_append_new(results, result);
        }
    }
    json_object_set_new(response, "results", results);

    json_t *timing = json_object();
    json_object_set_new(timing, "total_time_ms",
                        json_real(multi_result ? multi_result->total_time_ms : 0.0));
    json_object_set_new(response, "timing", timing);
    return response;
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
        json_t *error = auth_chats_build_error_response("Method not allowed - use POST");
        enum MHD_Result ret = api_send_json_response(connection, error, MHD_HTTP_METHOD_NOT_ALLOWED);
        // api_send_json_response takes ownership and decrefs internally
        return ret;
    }

    // Extract and validate JWT
    const char *auth_header = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Authorization");
    jwt_validation_result_t jwt_result = auth_chats_validate_jwt_from_header(auth_header);
    if (!jwt_result.valid) {
        api_free_post_buffer(con_cls);
        const char *error_msg = jwt_result.claims ? "Invalid token" : "Authentication required";
        json_t *error = auth_chats_build_error_response(error_msg);
        enum MHD_Result ret = api_send_json_response(connection, error, MHD_HTTP_UNAUTHORIZED);
        // api_send_json_response takes ownership and decrefs internally
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
        json_t *error = auth_chats_build_error_response("Token missing database claim");
        enum MHD_Result ret = api_send_json_response(connection, error, MHD_HTTP_FORBIDDEN);
        // api_send_json_response takes ownership and decrefs internally
        free_jwt_validation_result(&jwt_result);
        return ret;
    }

    // Parse request JSON
    json_error_t json_error;
    json_t *request_json = json_loads(buffer->data, 0, &json_error);
    api_free_post_buffer(con_cls);

    if (!request_json) {
        free_jwt_validation_result(&jwt_result);
        json_t *error = auth_chats_build_error_response("Invalid JSON in request body");
        enum MHD_Result ret = api_send_json_response(connection, error, MHD_HTTP_BAD_REQUEST);
        // api_send_json_response takes ownership and decrefs internally
        return ret;
    }

    json_t *engines_array = NULL;
    json_t *messages = NULL;
    size_t engine_count = 0;
    double temperature = -1.0;
    int max_tokens = -1;
    char *parse_error = NULL;
    if (!auth_chats_parse_request(request_json, &engines_array, &messages, &engine_count,
                                  &temperature, &max_tokens, &parse_error)) {
        free_jwt_validation_result(&jwt_result);
        json_decref(request_json);
        json_t *error = auth_chats_build_error_response(parse_error);
        free(parse_error);
        enum MHD_Result ret = api_send_json_response(connection, error, MHD_HTTP_BAD_REQUEST);
        return ret;
    }

    // Get database queue
    DatabaseQueue *db_queue = database_queue_manager_get_database(global_queue_manager, database);
    if (!db_queue) {
        free_jwt_validation_result(&jwt_result);
        json_decref(request_json);
        json_t *error = auth_chats_build_error_response("Database not found");
        enum MHD_Result ret = api_send_json_response(connection, error, MHD_HTTP_BAD_REQUEST);
        // api_send_json_response takes ownership and decrefs internally
        return ret;
    }

    // Get CEC
    ChatEngineCache *cec = db_queue->chat_engine_cache;
    if (!cec) {
        free_jwt_validation_result(&jwt_result);
        json_decref(request_json);
        json_t *error = auth_chats_build_error_response("Chat not enabled for this database");
        enum MHD_Result ret = api_send_json_response(connection, error, MHD_HTTP_SERVICE_UNAVAILABLE);
        // api_send_json_response takes ownership and decrefs internally
        return ret;
    }

    // Build multi-request array
    ChatMultiRequest *multi_requests = (ChatMultiRequest*)calloc(engine_count, sizeof(ChatMultiRequest));
    if (!multi_requests) {
        free_jwt_validation_result(&jwt_result);
        json_decref(request_json);
        json_t *error = auth_chats_build_error_response("Failed to allocate request array");
        enum MHD_Result ret = api_send_json_response(connection, error, MHD_HTTP_INTERNAL_SERVER_ERROR);
        // api_send_json_response takes ownership and decrefs internally
        return ret;
    }

    ChatMessage *chat_messages = auth_chats_messages_json_to_list(messages);
    size_t valid_requests = auth_chats_build_multi_requests(
        cec, engines_array, chat_messages, temperature, max_tokens, multi_requests);
    chat_message_list_destroy(chat_messages);

    if (valid_requests == 0) {
        auth_chats_free_multi_requests(multi_requests, valid_requests);
        free_jwt_validation_result(&jwt_result);
        json_decref(request_json);
        json_t *error = auth_chats_build_error_response("No valid or healthy engines found");
        enum MHD_Result ret = api_send_json_response(connection, error, MHD_HTTP_BAD_REQUEST);
        // api_send_json_response takes ownership and decrefs internally
        return ret;
    }

    // Send requests in parallel
    ChatProxyConfig proxy_config = chat_proxy_get_default_config();
    ChatMultiResult *multi_result = chat_proxy_send_multi(multi_requests, valid_requests, &proxy_config);

    // Save engine names before freeing multi_requests
    char **engine_names = auth_chats_copy_engine_names(multi_requests, valid_requests);
    if (!engine_names) {
        auth_chats_free_multi_requests(multi_requests, valid_requests);
        chat_multi_result_destroy(multi_result);
        free_jwt_validation_result(&jwt_result);
        json_decref(request_json);
        json_t *error = auth_chats_build_error_response("Failed to allocate engine names array");
        enum MHD_Result ret = api_send_json_response(connection, error, MHD_HTTP_INTERNAL_SERVER_ERROR);
        // api_send_json_response takes ownership and decrefs internally
        return ret;
    }
    auth_chats_free_multi_requests(multi_requests, valid_requests);

    // Generate broadcast ID
    char broadcast_id[37];
    auth_chats_generate_broadcast_id(broadcast_id, sizeof(broadcast_id));

    json_t *response = auth_chats_build_response(
        database, broadcast_id, engine_count, engine_names, multi_result);

    // Cleanup
    auth_chats_free_engine_names(engine_names, valid_requests);
    chat_multi_result_destroy(multi_result);
    free_jwt_validation_result(&jwt_result);
    json_decref(request_json);

    enum MHD_Result ret = api_send_json_response(connection, response, MHD_HTTP_OK);
    // api_send_json_response takes ownership and decrefs internally
    return ret;
}
