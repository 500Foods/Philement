/*
 * Authenticated Chat API Endpoint Implementation
 *
 * Handles single-model chat requests with JWT authentication.
 * Phase 7: Integrated with chat storage for conversation persistence.
 */

// Project includes
#include <src/hydrogen.h>
#include <src/api/api_utils.h>
#include <src/api/conduit/helpers/auth_jwt_helper.h>

// Local includes
#include "auth_chat.h"
#include "../helpers/engine_cache.h"
#include "../helpers/req_builder.h"
#include "../helpers/resp_parser.h"
#include "../helpers/proxy.h"
#include "../helpers/metrics.h"
#include "../helpers/storage.h"
#include "../helpers/context_hashing.h"
#include "../helpers/lru_cache.h"
#include <src/api/conduit/conduit_service.h>

// External reference to global queue manager
extern DatabaseQueueManager* global_queue_manager;

// Logging source for context hashing
static const char* SR_AUTH_CHAT = "AUTH_CHAT";

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
    char **context_hashes = NULL;
    size_t context_hash_count = 0;
    double temperature = -1.0;
    int max_tokens = -1;
    bool stream = false;
    char *error_message = NULL;

    bool parse_ok = auth_chat_parse_request(request_json, &engine_name,
                                             &messages, &context_hashes,
                                             &context_hash_count,
                                             &temperature, &max_tokens,
                                             &stream, &error_message);

    if (!parse_ok) {
        free_jwt_validation_result(&jwt_result);
        json_decref(request_json);
        chat_context_free_hash_array(context_hashes, context_hash_count);
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
        chat_context_free_hash_array(context_hashes, context_hash_count);
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
        chat_context_free_hash_array(context_hashes, context_hash_count);
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
        chat_context_free_hash_array(context_hashes, context_hash_count);
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
        chat_context_free_hash_array(context_hashes, context_hash_count);
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
        chat_context_free_hash_array(context_hashes, context_hash_count);
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
            // Convert array content to JSON string
            char *content_json = json_dumps(content_obj, JSON_COMPACT);
            if (content_json) {
                // Check if content contains media:hash references
                if (strstr(content_json, "media:") != NULL) {
                    // Resolve media references
                    char *resolved_json = NULL;
                    char *error_msg = NULL;
                    if (chat_storage_resolve_media_in_content(database, content_json, &resolved_json, &error_msg)) {
                        free(content_json);
                        content_str = resolved_json;
                    } else {
                        // Failed to resolve media, keep original content
                        content_str = content_json;
                        if (error_msg) {
                            log_this(SR_AUTH_CHAT, "Media resolution failed: %s", LOG_LEVEL_ALERT, 1, error_msg);
                            free(error_msg);
                        }
                    }
                } else {
                    content_str = content_json;
                }
            }
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

    // Validate payload size
    size_t request_size = strlen(request_body);
    size_t max_payload_bytes = (size_t)engine->max_payload_mb * 1024 * 1024;
    if (max_payload_bytes > 0 && request_size > max_payload_bytes) {
        free(request_body);
        free(engine_name);
        free_jwt_validation_result(&jwt_result);
        json_decref(request_json);
        char error_buf[256];
        snprintf(error_buf, sizeof(error_buf), "Request payload size (%zu bytes) exceeds engine limit (%d MB)",
                 request_size, engine->max_payload_mb);
        json_t *error_response = auth_chat_build_error_response(error_buf);
        enum MHD_Result ret = api_send_json_response(connection, error_response, MHD_HTTP_REQUEST_ENTITY_TOO_LARGE);
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

    static int chat_counter = 0;
    int convos_id = ++chat_counter;
    char convos_ref[64];
    snprintf(convos_ref, sizeof(convos_ref), "chat.%d.%lld", convos_id, (long long)time(NULL));

    // Phase 8: Context Hashing - Use client-provided hashes when available
    size_t messages_count = (messages && json_is_array(messages)) ? json_array_size(messages) : 0;
    double bandwidth_saved_percent = 0.0;
    size_t bandwidth_saved_bytes = 0;
    size_t hashes_used = 0;
    size_t hashes_missed = 0;

    const char** segment_hashes = NULL;
    size_t segment_hash_count = 0;

    // Allocate array for segment hashes (messages + response)
    size_t max_hashes = messages_count + 1;
    segment_hashes = calloc(max_hashes, sizeof(char*));
    if (!segment_hashes) {
        free(engine_name);
        free_jwt_validation_result(&jwt_result);
        json_decref(request_json);
        chat_context_free_hash_array(context_hashes, context_hash_count);
        chat_parsed_response_destroy(parsed);
        chat_proxy_result_destroy(proxy_result);
        json_t *error_response = auth_chat_build_error_response("Memory allocation failed");
        enum MHD_Result ret = api_send_json_response(connection, error_response, MHD_HTTP_INTERNAL_SERVER_ERROR);
        json_decref(error_response);
        return ret;
    }

    // Process messages: use provided context_hashes or store new segments
    if (messages_count > 0) {
        for (size_t i = 0; i < messages_count && segment_hash_count < max_hashes; i++) {
            json_t* msg = json_array_get(messages, i);
            if (!json_is_object(msg)) continue;

            const json_t* role_obj = json_object_get(msg, "role");
            const json_t* content_obj = json_object_get(msg, "content");
            if (!role_obj || !content_obj) continue;

            char* content_str = NULL;
            if (json_is_string(content_obj)) {
                content_str = strdup(json_string_value(content_obj));
            } else if (json_is_array(content_obj)) {
                content_str = json_dumps(content_obj, JSON_COMPACT);
            }

            if (!content_str) {
                continue;
            }

            // Phase 8: Check if we have a matching context hash from client
            char* hash = NULL;
            if (context_hashes && i < context_hash_count) {
                // Verify the provided hash exists in storage
                if (chat_storage_segment_exists(database, context_hashes[i])) {
                    hash = strdup(context_hashes[i]);
                    hashes_used++;
                    size_t content_len = strlen(content_str);
                    if (content_len > 54) {  // Hash overhead is ~54 bytes
                        bandwidth_saved_bytes += content_len - 54;
                    }
                    log_this(SR_AUTH_CHAT, "Using client-provided context hash for message %zu", LOG_LEVEL_DEBUG, 1, i);
                } else {
                    hashes_missed++;
                    log_this(SR_AUTH_CHAT, "Client-provided hash not found for message %zu, storing new segment", LOG_LEVEL_ALERT, 1, i);
                }
            }

            // If no valid context hash, store as new segment
            if (!hash) {
                hash = chat_storage_store_segment(database, content_str, strlen(content_str));
            }

            if (hash) {
                segment_hashes[segment_hash_count++] = hash;
            }
            free(content_str);
        }
    }

    // Store response as new segment
    if (parsed->content) {
        char* resp_hash = chat_storage_store_segment(database, parsed->content, strlen(parsed->content));
        if (resp_hash && segment_hash_count < max_hashes) {
            segment_hashes[segment_hash_count++] = resp_hash;
        }
    }

    // Calculate bandwidth savings percentage
    if (messages_count > 0 && bandwidth_saved_bytes > 0) {
        size_t total_original_size = 0;
        for (size_t i = 0; i < messages_count; i++) {
            json_t* msg = json_array_get(messages, i);
            if (json_is_object(msg)) {
                char* msg_str = json_dumps(msg, JSON_COMPACT);
                if (msg_str) {
                    total_original_size += strlen(msg_str);
                    free(msg_str);
                }
            }
        }
        if (total_original_size > 0) {
            bandwidth_saved_percent = ((double)bandwidth_saved_bytes / (double)total_original_size) * 100.0;
        }
    }

    chat_storage_store_chat(database, convos_ref,
                            segment_hashes, segment_hash_count,
                            engine->name,
                            parsed->model ? parsed->model : engine->model,
                            parsed->prompt_tokens,
                            parsed->completion_tokens,
                            0.0,
                            jwt_result.claims ? jwt_result.claims->user_id : 0,
                            (int)response_time_ms,
                            NULL);

    for (size_t i = 0; i < segment_hash_count; i++) {
        if (segment_hashes[i]) chat_storage_free_hash((char*)segment_hashes[i]);
    }
    free(segment_hashes);

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
        response_time_ms,
        convos_id,
        convos_ref
    );

    // Phase 8: Add context hashing stats to response if hashes were used
    if (context_hashes && context_hash_count > 0) {
        json_t *context_stats = json_object();
        json_object_set_new(context_stats, "hashes_used", json_integer((json_int_t)hashes_used));
        json_object_set_new(context_stats, "hashes_missed", json_integer((json_int_t)hashes_missed));
        json_object_set_new(context_stats, "bandwidth_saved_bytes", json_integer((json_int_t)bandwidth_saved_bytes));
        json_object_set_new(context_stats, "bandwidth_saved_percent", json_real(bandwidth_saved_percent));

        // Phase 9: Add cache statistics
        uint64_t cache_hits = 0, cache_misses = 0;
        double cache_hit_ratio = 0.0;
        if (chat_storage_cache_get_stats(database, &cache_hits, &cache_misses, &cache_hit_ratio)) {
            json_object_set_new(context_stats, "cache_hits", json_integer((json_int_t)cache_hits));
            json_object_set_new(context_stats, "cache_misses", json_integer((json_int_t)cache_misses));
            json_object_set_new(context_stats, "cache_hit_ratio", json_real(cache_hit_ratio));
        }

        json_object_set_new(response, "context_hashing", context_stats);
    }

    // Cleanup
    free(engine_name);
    free_jwt_validation_result(&jwt_result);
    json_decref(request_json);
    chat_context_free_hash_array(context_hashes, context_hash_count);
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
                              char ***context_hashes,
                              size_t *context_hash_count,
                              double *temperature,
                              int *max_tokens,
                              bool *stream,
                              char **error_message) {
    (void)request_json;
    *engine = NULL;
    *messages = NULL;
    *context_hashes = NULL;
    *context_hash_count = 0;
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

    // Extract context_hashes (optional - for bandwidth optimization)
    *context_hashes = chat_context_parse_request_hashes(request_json, error_message);
    if (*error_message) {
        return false;  // Parse error occurred
    }
    if (*context_hashes) {
        // Count valid hashes
        size_t count = 0;
        while ((*context_hashes)[count]) {
            count++;
        }
        *context_hash_count = count;
    }

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
                                  double response_time_ms,
                                  int convos_id,
                                  const char *convos_ref) {
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
    json_object_set_new(response, "convos_id", json_integer(convos_id));
    json_object_set_new(response, "convos_ref", json_string(convos_ref));

    return response;
}

// Build error response
json_t* auth_chat_build_error_response(const char *error_message) {
    json_t *response = json_object();
    json_object_set_new(response, "success", json_false());
    json_object_set_new(response, "error", json_string(error_message));
    return response;
}
