/*
 * Authenticated Chat SSE Streaming
 *
 * REST SSE streaming via MHD incremental response + chat_proxy_multi_*.
 * Reuses the multi-curl worker thread and chunk queue from the WebSocket
 * streaming path; replaces the LWS write with MHD response callbacks.
 *
 * Architecture:
 * 1. auth_chat_stream_sse() starts a multi-curl stream (worker thread pulls SSE from provider)
 * 2. A callback thread drains the chunk queue, formats SSE events, writes to a pipe
 * 3. MHD response callback reads from the pipe and streams to the HTTP client
 */

#include <src/hydrogen.h>
#include <src/api/api_utils.h>
#include <src/api/wschat/helpers/proxy_multi.h>
#include <src/api/wschat/helpers/proxy.h>
#include <src/api/wschat/helpers/resp_parser.h>
#include <src/api/wschat/auth_chat/auth_chat.h>

#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>

void *rest_sse_callback_thread(void *arg) {
    RestSseContext *ctx = (RestSseContext *)arg;
    if (!ctx || !ctx->stream_ctx) return NULL;

    while (true) {
        StreamChunkNode *node = NULL;

        while (!ctx->stream_ctx->stream_completed) {
            node = chunk_queue_dequeue(&ctx->stream_ctx->chunk_queue);
            if (node) break;
            usleep(10000);
        }

        if (!node && ctx->stream_ctx->stream_completed) {
            node = chunk_queue_dequeue(&ctx->stream_ctx->chunk_queue);
            if (!node) break;
        }

        if (!node) break;

        char *json_str = node->json_data;
        free(node);

        if (json_str) {
            char buf[8192];
            int n = 0;
            if (strlen(json_str) > 0) {
                n = snprintf(buf, sizeof(buf), "data: %s\n\n", json_str);
            }
            free(json_str);

            if (n > 0 && ctx->pipe_write >= 0) {
                size_t written = 0;
                while (written < (size_t)n) {
                    ssize_t w = write(ctx->pipe_write, buf + written, (size_t)n - written);
                    if (w <= 0) break;
                    written += (size_t)w;
                }
            }
        }
    }

    ctx->callback_done = true;
    if (ctx->pipe_write >= 0) {
        close(ctx->pipe_write);
        ctx->pipe_write = -1;
    }
    return NULL;
}

void rest_sse_cleanup(RestSseContext *ctx) {
    if (!ctx) return;
    if (ctx->cleanup_done) return;
    ctx->cleanup_done = true;

    if (ctx->callback_thread && !ctx->callback_done) {
        pthread_join(ctx->callback_thread, NULL);
    }

    if (ctx->pipe_read >= 0) close(ctx->pipe_read);
    if (ctx->pipe_write >= 0) close(ctx->pipe_write);

    if (ctx->manager && ctx->stream_ctx) {
        pthread_mutex_lock(&ctx->manager->streams_mutex);
        MultiStreamContext *sc = ctx->stream_ctx;
        if (sc->prev) {
            sc->prev->next = sc->next;
        } else {
            ctx->manager->active_streams = sc->next;
        }
        if (sc->next) {
            sc->next->prev = sc->prev;
        }
        if (sc->headers) {
            curl_slist_free_all(sc->headers);
        }
        pthread_mutex_unlock(&ctx->manager->streams_mutex);

        chunk_queue_destroy(&sc->chunk_queue);
        free(sc->request_id);
        free(sc->engine_name);
        free(sc->finish_reason);
        free(sc->request_body);
        free(sc);
    }

    free(ctx);
}

#include <errno.h>

ssize_t rest_sse_mhd_callback(void *cls, uint64_t pos,
                                      char *buf, size_t max) {
    (void)pos;
    RestSseContext *ctx = (RestSseContext *)cls;
    if (!ctx || ctx->pipe_read < 0) return MHD_CONTENT_READER_END_WITH_ERROR;

    if (ctx->callback_done) {
        ssize_t r = read(ctx->pipe_read, buf, max);
        if (r > 0) return r;
        rest_sse_cleanup(ctx);
        return MHD_CONTENT_READER_END_OF_STREAM;
    }

    ssize_t r = read(ctx->pipe_read, buf, max);
    if (r > 0) return r;

    if (ctx->callback_done) {
        rest_sse_cleanup(ctx);
        return MHD_CONTENT_READER_END_OF_STREAM;
    }

    if (r == 0 || errno == EAGAIN || errno == EWOULDBLOCK) return 0;

    rest_sse_cleanup(ctx);
    return MHD_CONTENT_READER_END_WITH_ERROR;
}

void rest_sse_free_cls(void *cls) {
    RestSseContext *ctx = (RestSseContext *)cls;
    rest_sse_cleanup(ctx);
}

enum MHD_Result auth_chat_stream_sse(struct MHD_Connection *connection,
                                      const ChatEngineConfig *engine,
                                      const char *request_body,
                                      jwt_validation_result_t *jwt_result,
                                      const char *database) {
    (void)database;
    (void)jwt_result;

    if (!connection || !engine || !request_body) {
        return MHD_NO;
    }

    MultiStreamManager *manager = chat_proxy_get_multi_manager();
    if (!manager || !manager->initialized) {
        json_t *error = auth_chat_build_error_response("Streaming subsystem unavailable");
        return api_send_json_response(connection, error, MHD_HTTP_SERVICE_UNAVAILABLE);
    }

    int pipefd[2];
    if (pipe(pipefd) != 0) {
        json_t *error = auth_chat_build_error_response("Failed to create pipe");
        return api_send_json_response(connection, error, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    fcntl(pipefd[0], F_SETFL, O_NONBLOCK);
    fcntl(pipefd[1], F_SETFL, O_NONBLOCK);

    RestSseContext *ctx = calloc(1, sizeof(RestSseContext));
    if (!ctx) {
        close(pipefd[0]);
        close(pipefd[1]);
        json_t *error = auth_chat_build_error_response("Failed to allocate stream context");
        return api_send_json_response(connection, error, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    ctx->manager = manager;
    ctx->pipe_read = pipefd[0];
    ctx->pipe_write = pipefd[1];
    ctx->callback_done = false;
    ctx->cleanup_done = false;
    ctx->connection_valid = true;
    ctx->stream_active = true;

    ctx->stream_ctx = chat_proxy_multi_stream_start(
        manager,
        engine,
        request_body,
        NULL,
        NULL,
        &ctx->connection_valid,
        &ctx->stream_active,
        NULL,
        NULL,
        NULL,
        NULL
    );

    if (!ctx->stream_ctx) {
        close(pipefd[0]);
        close(pipefd[1]);
        free(ctx);
        json_t *error = auth_chat_build_error_response("Failed to start stream");
        return api_send_json_response(connection, error, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    if (pthread_create(&ctx->callback_thread, NULL, rest_sse_callback_thread, ctx) != 0) {
        chat_proxy_multi_stream_stop(manager, ctx->stream_ctx);
        close(pipefd[0]);
        close(pipefd[1]);
        free(ctx);
        json_t *error = auth_chat_build_error_response("Failed to start callback thread");
        return api_send_json_response(connection, error, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    struct MHD_Response *response = MHD_create_response_from_callback(
        MHD_SIZE_UNKNOWN,
        4096,
        rest_sse_mhd_callback,
        ctx,
        rest_sse_free_cls
    );

    if (!response) {
        ctx->callback_done = true;
        if (ctx->stream_ctx) {
            ctx->stream_ctx->stream_completed = true;
        }
        if (ctx->pipe_write >= 0) {
            close(ctx->pipe_write);
            ctx->pipe_write = -1;
        }
        pthread_join(ctx->callback_thread, NULL);
        ctx->callback_thread = 0;
        chat_proxy_multi_stream_stop(manager, ctx->stream_ctx);
        rest_sse_cleanup(ctx);
        json_t *error = auth_chat_build_error_response("Failed to create response");
        return api_send_json_response(connection, error, MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    MHD_add_response_header(response, "Content-Type", "text/event-stream");
    MHD_add_response_header(response, "Cache-Control", "no-cache");
    MHD_add_response_header(response, "Connection", "keep-alive");
    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
    MHD_add_response_header(response, "Access-Control-Allow-Headers", "Authorization, Content-Type");

    enum MHD_Result ret = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);

    return ret;
}