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

// Send a chat error response to the client. When error_code > 0 the
// payload includes "error_code" matching the conduit envelope
// (4xxx range for chat subsystem throttle/limit conditions; Phase
// 10b uses 4291/4292).
void send_chat_error(struct lws *wsi, const char* error_message,
                     const char* request_id, int error_code) {
    json_t* response = json_object();
    json_object_set_new(response, "type", json_string("chat_error"));
    if (request_id) {
        json_object_set_new(response, "id", json_string(request_id));
    }
    json_object_set_new(response, "error", json_string(error_message ? error_message : "Unknown error"));
    if (error_code > 0) {
        json_object_set_new(response, "error_code", json_integer(error_code));
    }

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
