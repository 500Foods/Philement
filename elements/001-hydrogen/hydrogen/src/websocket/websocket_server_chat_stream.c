/*
 * WebSocket Chat Stream Callback
 *
 * Legacy streaming chunk callback implementation. Kept for compatibility and
 * reference; the current multi_curl implementation handles chunks internally.
 */

// Project includes
#include <src/hydrogen.h>

// Local includes
#include "websocket_server_chat_internal.h"

// Streaming chunk callback
void stream_chunk_callback(const ChatStreamChunk* chunk, void* user_data) {
    StreamContext* ctx = (StreamContext*)user_data;
    if (!chunk || !ctx || !ctx->wsi) return;

    if (ctx->connection_valid && !*ctx->connection_valid) {
        return;
    }

    if (chunk->is_done) {
        ctx->stream_completed = true;
        if (chunk->finish_reason) {
            ctx->finish_reason = strdup(chunk->finish_reason);
        } else {
            ctx->finish_reason = strdup("stop");
        }

        struct timespec end_time;
        clock_gettime(CLOCK_MONOTONIC, &end_time);
        double elapsed_ms = (double)(end_time.tv_sec - ctx->start_time.tv_sec) * 1000.0 +
                           (double)(end_time.tv_nsec - ctx->start_time.tv_nsec) / 1000000.0;

        send_stream_done(ctx,
                        ctx->finish_reason ? ctx->finish_reason : "stop",
                        0, 0, 0,
                        elapsed_ms, NULL);

        log_this(SR_WEBSOCKET_CHAT, "Stream complete: %d chunks, %.0fms, finish=%s",
                 LOG_LEVEL_STATE, 3,
                 ctx->chunk_index,
                 elapsed_ms,
                 ctx->finish_reason ? ctx->finish_reason : "stop");

        if (ctx->stream_active) {
            *ctx->stream_active = false;
        }

        free(ctx->finish_reason);
        free(ctx->request_id);
        free(ctx->model);
        free(ctx);
    } else {
        if (!ctx->first_chunk_logged) {
            ctx->first_chunk_logged = true;
            struct timespec first_chunk_time;
            clock_gettime(CLOCK_MONOTONIC, &first_chunk_time);
            double ttfb_ms = (double)(first_chunk_time.tv_sec - ctx->start_time.tv_sec) * 1000.0 +
                            (double)(first_chunk_time.tv_nsec - ctx->start_time.tv_nsec) / 1000000.0;
            log_this(SR_WEBSOCKET_CHAT, "Initial response received from %s (%.0fms TTFB)",
                     LOG_LEVEL_DEBUG, 1, ctx->model ? ctx->model : "unknown", ttfb_ms);
        }

        const char* model = chunk->model ? chunk->model : ctx->model;
        if (chunk->model) {
            free(ctx->model);
            ctx->model = strdup(chunk->model);
        }

        const char* content = chunk->content ? chunk->content : "";
        const char* reasoning_content = chunk->reasoning_content ? chunk->reasoning_content : NULL;
        send_stream_chunk(ctx, content, reasoning_content, model, ctx->chunk_index, NULL);
        ctx->chunk_index++;
    }
}
