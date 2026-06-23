/*
 * Internal WebSocket Chat Handler Definitions
 *
 * Exposes chat helper functions and structures for use within the chat module
 * and for unit testing. These are not part of the public WebSocket API.
 */

#ifndef WEBSOCKET_SERVER_CHAT_INTERNAL_H
#define WEBSOCKET_SERVER_CHAT_INTERNAL_H

// Project includes
#include <src/hydrogen.h>

// Local includes
#include "websocket_server_chat.h"
#include "websocket_server_internal.h"

// Chat common includes
#include <src/api/wschat/helpers/engine_cache.h>
#include <src/api/wschat/helpers/proxy.h>
#include <src/api/wschat/helpers/proxy_multi.h>
#include <src/api/wschat/helpers/req_builder.h>
#include <src/api/wschat/helpers/resp_parser.h>

// ============================================================================
// Non-streaming Response Helper
// ============================================================================

int send_chat_proxy_result(struct lws *wsi, const char* request_id,
                           const ChatEngineConfig* engine,
                           ChatProxyResult* proxy_result,
                           struct timespec start_time,
                           size_t message_count);

// ============================================================================
// Message Conversion Helper
// ============================================================================

ChatMessage* convert_json_messages_to_chat_messages(json_t *messages);

// ============================================================================
// Streaming Context
// ============================================================================

typedef struct {
    struct lws *wsi;
    char *request_id;
    char *model;
    int chunk_index;
    bool stream_completed;
    char *finish_reason;
    bool first_chunk_logged;
    struct timespec start_time;
    volatile bool *connection_valid;
    volatile bool *stream_active;
    MultiStreamContext *multi_stream_ctx;
} StreamContext;

// ============================================================================
// Send Helpers
// ============================================================================

void send_chat_error(struct lws *wsi, const char* error_message, const char* request_id);

void send_chat_done(struct lws *wsi, const char* request_id, const char* content,
                    const char* model, const char* finish_reason,
                    int prompt_tokens, int completion_tokens, int total_tokens,
                    double response_time_ms, json_t* raw_response);

void send_chat_chunk(struct lws *wsi, const char* request_id, const char* content,
                     const char* reasoning_content, const char* model, int index,
                     const char* finish_reason);

void send_stream_chunk(StreamContext* ctx, const char* content,
                       const char* reasoning_content, const char* model,
                       int index, const char* finish_reason);

void send_stream_done(StreamContext* ctx, const char* finish_reason,
                      int prompt_tokens, int completion_tokens, int total_tokens,
                      double response_time_ms, json_t* raw_response);

// ============================================================================
// Stream Callback
// ============================================================================

void stream_chunk_callback(const ChatStreamChunk* chunk, void* user_data);

#endif // WEBSOCKET_SERVER_CHAT_INTERNAL_H
