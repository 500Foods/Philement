/*
 * WebSocket Chat Send Helpers
 *
 * Builds and sends chat-related WebSocket messages to clients.
 */

// Project includes
#include <src/hydrogen.h>

// Local includes
#include "websocket_server_chat_internal.h"
#include "websocket_server_message.h"

// Send a chat error response to the client
void send_chat_error(struct lws *wsi, const char* error_message, const char* request_id) {
    json_t* response = json_object();
    json_object_set_new(response, "type", json_string("chat_error"));
    if (request_id) {
        json_object_set_new(response, "id", json_string(request_id));
    }
    json_object_set_new(response, "error", json_string(error_message ? error_message : "Unknown error"));

    ws_write_json_response(wsi, response);
    json_decref(response);
}

// Send a chat completion/done response to the client
void send_chat_done(struct lws *wsi, const char* request_id, const char* content,
                    const char* model, const char* finish_reason,
                    int prompt_tokens, int completion_tokens, int total_tokens,
                    double response_time_ms, json_t* raw_response) {
    json_t* response = json_object();
    json_object_set_new(response, "type", json_string("chat_done"));
    if (request_id) {
        json_object_set_new(response, "id", json_string(request_id));
    }

    json_t* result = json_object();
    json_object_set_new(result, "content", json_string(content ? content : ""));
    if (model) json_object_set_new(result, "model", json_string(model));
    if (finish_reason) json_object_set_new(result, "finish_reason", json_string(finish_reason));

    json_t* tokens = json_object();
    json_object_set_new(tokens, "prompt", json_integer(prompt_tokens));
    json_object_set_new(tokens, "completion", json_integer(completion_tokens));
    json_object_set_new(tokens, "total", json_integer(total_tokens));
    json_object_set_new(result, "tokens", tokens);

    json_object_set_new(result, "response_time_ms", json_real(response_time_ms));

    if (raw_response) {
        json_t* retrieval = json_object_get(raw_response, "retrieval");
        if (retrieval) {
            log_this(SR_WEBSOCKET_CHAT, "Raw response contains retrieval data", LOG_LEVEL_DEBUG, 0);
        }
        json_object_set(result, "raw_provider_response", raw_response);
    }

    json_object_set_new(response, "result", result);

    ws_write_json_response(wsi, response);
    json_decref(response);
}

// Send a chat chunk response to the client
void send_chat_chunk(struct lws *wsi, const char* request_id, const char* content,
                     const char* reasoning_content, const char* model, int index,
                     const char* finish_reason) {
    json_t* response = json_object();
    json_object_set_new(response, "type", json_string("chat_chunk"));
    if (request_id) {
        json_object_set_new(response, "id", json_string(request_id));
    }

    json_t* chunk = json_object();
    json_object_set_new(chunk, "content", json_string(content ? content : ""));
    if (reasoning_content) {
        json_object_set_new(chunk, "reasoning_content", json_string(reasoning_content));
    }
    if (model) json_object_set_new(chunk, "model", json_string(model));
    json_object_set_new(chunk, "index", json_integer(index));
    if (finish_reason) json_object_set_new(chunk, "finish_reason", json_string(finish_reason));
    json_object_set_new(response, "chunk", chunk);

    ws_write_json_response(wsi, response);
    json_decref(response);
}

// Build and send a chat chunk directly to the WebSocket from a stream context
void send_stream_chunk(StreamContext* ctx, const char* content,
                       const char* reasoning_content, const char* model,
                       int index, const char* finish_reason) {
    if (!ctx || !ctx->wsi) return;

    if (ctx->connection_valid && !*ctx->connection_valid) {
        return;
    }

    json_t* response = json_object();
    json_object_set_new(response, "type", json_string("chat_chunk"));
    if (ctx->request_id) {
        json_object_set_new(response, "id", json_string(ctx->request_id));
    }

    json_t* chunk = json_object();
    json_object_set_new(chunk, "content", json_string(content ? content : ""));
    if (reasoning_content) {
        json_object_set_new(chunk, "reasoning_content", json_string(reasoning_content));
    }
    if (model) json_object_set_new(chunk, "model", json_string(model));
    json_object_set_new(chunk, "index", json_integer(index));
    if (finish_reason) json_object_set_new(chunk, "finish_reason", json_string(finish_reason));
    json_object_set_new(response, "chunk", chunk);

    char* json_str = json_dumps(response, JSON_COMPACT);
    json_decref(response);

    if (json_str) {
        int result = ws_write_raw_data(ctx->wsi, json_str, strlen(json_str));
        if (result != 0) {
            if (ctx->connection_valid) {
                log_this(SR_WEBSOCKET_CHAT, "Write failed, stopping stream", LOG_LEVEL_DEBUG, 0);
            }
        }
        free(json_str);
    }
}

// Build and send a chat done message directly to the WebSocket from a stream context
void send_stream_done(StreamContext* ctx, const char* finish_reason,
                      int prompt_tokens, int completion_tokens, int total_tokens,
                      double response_time_ms, json_t* raw_response) {
    if (!ctx || !ctx->wsi) return;

    if (ctx->connection_valid && !*ctx->connection_valid) {
        return;
    }

    json_t* response = json_object();
    json_object_set_new(response, "type", json_string("chat_done"));
    if (ctx->request_id) {
        json_object_set_new(response, "id", json_string(ctx->request_id));
    }

    json_t* result = json_object();
    json_object_set_new(result, "content", json_string(""));
    if (ctx->model) json_object_set_new(result, "model", json_string(ctx->model));
    if (finish_reason) json_object_set_new(result, "finish_reason", json_string(finish_reason));

    json_t* tokens = json_object();
    json_object_set_new(tokens, "prompt", json_integer(prompt_tokens));
    json_object_set_new(tokens, "completion", json_integer(completion_tokens));
    json_object_set_new(tokens, "total", json_integer(total_tokens));
    json_object_set_new(result, "tokens", tokens);

    json_object_set_new(result, "response_time_ms", json_real(response_time_ms));

    if (raw_response) {
        json_object_set(result, "raw_provider_response", raw_response);
    }

    json_object_set_new(response, "result", result);

    char* json_str = json_dumps(response, JSON_COMPACT);
    json_decref(response);

    if (json_str) {
        ws_write_raw_data(ctx->wsi, json_str, strlen(json_str));
        free(json_str);
    }
}
