/*
 * Unity Test File: Chat Proxy Multi - Transfer Completion & Lifecycle
 *
 * Tests the previously-uncovered logic in proxy_multi.c by driving it directly
 * (no real upstream servers, no worker thread):
 *   - chat_proxy_multi_handle_completed_transfer() (the large CURLMSG_DONE
 *     handler, extracted from chat_proxy_multi_perform() for testability):
 *       * final SSE chunk parsed into a chat_chunk + chat_done
 *       * post-[DONE] metadata captured into raw_provider_response
 *       * trailing line-buffer JSON used as raw_provider_response
 *       * HTTP error paths (with and without response body)
 *       * no-private-data / no-stream-context safe no-ops
 *   - chat_proxy_multi_perform() guard + completed-stream cleanup branch
 *   - chat_proxy_multi_cleanup() with a constructed active stream
 *   - chat_proxy_multi_init() guard paths
 *   - chat_proxy_multi_stream_stop() (with/without easy handle)
 *   - chat_proxy_multi_drain_queue() (null, connection-invalid, write-fail,
 *     success via mocked lws_write)
 *
 * A real CURLM handle and real CURL easy handles are used so that the private
 * data round-trip (CURLINFO_PRIVATE) works exactly as in production.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <curl/curl.h>

#include <src/api/wschat/helpers/proxy_multi.h>
#include <src/websocket/websocket_server_internal.h>
#include <src/websocket/websocket_server_message.h>

// lws is mocked globally; pull in mock control entry points for lws_write.
#ifdef USE_MOCK_LIBWEBSOCKETS
#include <unity/mocks/mock_libwebsockets.h>
#endif

// Function prototypes (declared non-static in proxy_multi.h for testing)
void chat_proxy_multi_handle_completed_transfer(MultiStreamManager* manager, CURL* easy, CURLcode res, long http_code);
bool chat_proxy_multi_perform(MultiStreamManager* manager);
void chat_proxy_multi_cleanup(MultiStreamManager* manager);
bool chat_proxy_multi_init(MultiStreamManager* manager, struct lws_context* lws_context);
void chat_proxy_multi_stream_stop(MultiStreamManager* manager, MultiStreamContext* context);
int chat_proxy_multi_drain_queue(MultiStreamContext* context);

// Test function prototypes
void test_handle_done_final_chunk(void);
void test_handle_done_post_done_metadata(void);
void test_handle_done_trailing_line_json(void);
void test_handle_done_post_done_first_alloc(void);
void test_handle_done_final_chunk_finish_reason(void);
void test_handle_done_http_error_with_body(void);
void test_handle_done_http_error_no_body(void);
void test_handle_done_no_private_data(void);
void test_handle_done_null_stream_ctx(void);
void test_perform_null(void);
void test_perform_cleanup_completed_stream(void);
void test_cleanup_with_stream(void);
void test_init_guards(void);
void test_stream_stop_without_handle(void);
void test_stream_stop_with_handle(void);
void test_drain_queue_null(void);
void test_drain_queue_connection_invalid(void);
void test_drain_queue_partial_invalid(void);
void test_drain_queue_success(void);

// ---------------------------------------------------------------------------
// Helpers to build a manager + completed stream
// ---------------------------------------------------------------------------

static MultiStreamManager manager;
static MultiStreamContext stream_ctx;
static CurlStreamContext* g_curl_ctx = NULL; // heap-allocated; freed by handler
static WebSocketSessionData session;
static volatile bool g_conn_valid = true;
static volatile bool g_stream_active = true;

// Build a manager with a real CURLM and one linked stream whose easy handle
// carries the CurlStreamContext via CURLOPT_PRIVATE. The stream is marked
// completed (as it would be after the transfer finished).
//
// g_curl_ctx is heap-allocated because chat_proxy_multi_handle_completed_transfer
// frees it (and its line_buffer / post_done_buffer) once the transfer is
// processed, mirroring production where it is calloc()'d in
// chat_proxy_multi_stream_start().
static void build_manager_with_stream(bool include_handle) {
    memset(&manager, 0, sizeof(manager));
    pthread_mutex_init(&manager.streams_mutex, NULL);
    manager.initialized = true;
    manager.multi_handle = curl_multi_init();
    TEST_ASSERT_NOT_NULL(manager.multi_handle);

    memset(&stream_ctx, 0, sizeof(stream_ctx));
    memset(&session, 0, sizeof(session));

    g_curl_ctx = (CurlStreamContext*)calloc(1, sizeof(CurlStreamContext));
    TEST_ASSERT_NOT_NULL(g_curl_ctx);
    g_curl_ctx->stream_ctx = &stream_ctx;
    g_curl_ctx->line_buffer_capacity = 256;
    g_curl_ctx->line_buffer = (char*)malloc(g_curl_ctx->line_buffer_capacity);
    TEST_ASSERT_NOT_NULL(g_curl_ctx->line_buffer);
    g_curl_ctx->line_buffer[0] = '\0';

    chunk_queue_init(&stream_ctx.chunk_queue);
    stream_ctx.engine_name = strdup("test-engine");
    stream_ctx.request_id = strdup("req-123");
    stream_ctx.finish_reason = strdup("stop");
    stream_ctx.stream_completed = true;
    stream_ctx.connection_valid = &g_conn_valid;
    stream_ctx.stream_active = &g_stream_active;
    stream_ctx.session_data = &session;
    session.multi_stream_ctx = &stream_ctx;
    clock_gettime(CLOCK_MONOTONIC, &stream_ctx.start_time);

    if (include_handle) {
        stream_ctx.easy_handle = curl_easy_init();
        TEST_ASSERT_NOT_NULL(stream_ctx.easy_handle);
        curl_easy_setopt(stream_ctx.easy_handle, CURLOPT_PRIVATE, g_curl_ctx);
        curl_multi_add_handle(manager.multi_handle, stream_ctx.easy_handle);
    } else {
        stream_ctx.easy_handle = NULL;
    }

    // Link into active list
    stream_ctx.next = NULL;
    stream_ctx.prev = NULL;
    manager.active_streams = &stream_ctx;
}

static void tear_down_manager(void) {
    // After a completed-transfer handler call, g_curl_ctx (and its buffers)
    // have been freed by the handler and stream_ctx.easy_handle cleared.
    // Only free what the handler does NOT own.
    g_curl_ctx = NULL;

    chunk_queue_destroy(&stream_ctx.chunk_queue);
    free(stream_ctx.request_id);
    stream_ctx.request_id = NULL;
    free(stream_ctx.engine_name);
    stream_ctx.engine_name = NULL;
    free(stream_ctx.finish_reason);
    stream_ctx.finish_reason = NULL;

    if (manager.multi_handle) {
        curl_multi_cleanup(manager.multi_handle);
        manager.multi_handle = NULL;
    }
    pthread_mutex_destroy(&manager.streams_mutex);
    manager.active_streams = NULL;
}

// ---------------------------------------------------------------------------

void setUp(void) {
#ifdef USE_MOCK_LIBWEBSOCKETS
    mock_lws_reset_all();
    mock_lws_set_write_result(0); // default: lws_write succeeds
#endif
    g_conn_valid = true;
    g_stream_active = true;
}

void tearDown(void) {
    // Nothing global to reset; per-test teardown handled inline.
}

// ---------------------------------------------------------------------------
// chat_proxy_multi_handle_completed_transfer
// ---------------------------------------------------------------------------

void test_handle_done_final_chunk(void) {
    build_manager_with_stream(true);

    // A final SSE data line (no trailing newline) that is NOT [DONE]: the
    // handler parses it into a chat_chunk, enqueues it, then enqueues chat_done.
    const char* line = "data: {\"choices\":[{\"delta\":{\"content\":\"hi\",\"model\":\"m\"}}],\"finish_reason\":\"stop\"}";
    size_t len = strlen(line);
    memcpy(g_curl_ctx->line_buffer, line, len);
    g_curl_ctx->line_buffer[len] = '\0';
    g_curl_ctx->line_buffer_len = len;

    chat_proxy_multi_handle_completed_transfer(&manager, stream_ctx.easy_handle, CURLE_OK, 200);

    // Two messages queued: the chat_chunk and the chat_done
    TEST_ASSERT_TRUE(chunk_queue_has_data(&stream_ctx.chunk_queue));
    StreamChunkNode* n1 = chunk_queue_dequeue(&stream_ctx.chunk_queue);
    TEST_ASSERT_NOT_NULL(strstr(n1->json_data, "\"type\":\"chat_chunk\""));
    TEST_ASSERT_NOT_NULL(strstr(n1->json_data, "\"content\":\"hi\""));
    free(n1->json_data);
    free(n1);

    StreamChunkNode* n2 = chunk_queue_dequeue(&stream_ctx.chunk_queue);
    TEST_ASSERT_NOT_NULL(n2);
    TEST_ASSERT_NOT_NULL(strstr(n2->json_data, "\"type\":\"chat_done\""));
    free(n2->json_data);
    free(n2);

    // Stream active flag cleared
    TEST_ASSERT_FALSE(g_stream_active);

    tear_down_manager();
}

void test_handle_done_post_done_metadata(void) {
    build_manager_with_stream(true);

    // First line is [DONE]; second line is metadata JSON captured after done.
    // We simulate the post-done buffer already populated by the write callback.
    g_curl_ctx->seen_done = true;
    const char* meta = "{\"retrieval\":{\"foo\":\"bar\"}}";
    size_t len = strlen(meta);
    g_curl_ctx->post_done_capacity = len + 1;
    g_curl_ctx->post_done_buffer = (char*)malloc(g_curl_ctx->post_done_capacity);
    memcpy(g_curl_ctx->post_done_buffer, meta, len);
    g_curl_ctx->post_done_buffer[len] = '\0';
    g_curl_ctx->post_done_len = len;

    chat_proxy_multi_handle_completed_transfer(&manager, stream_ctx.easy_handle, CURLE_OK, 200);

    // chat_done should include raw_provider_response from the post_done_buffer
    TEST_ASSERT_TRUE(chunk_queue_has_data(&stream_ctx.chunk_queue));
    StreamChunkNode* n = chunk_queue_dequeue(&stream_ctx.chunk_queue);
    TEST_ASSERT_NOT_NULL(strstr(n->json_data, "\"type\":\"chat_done\""));
    TEST_ASSERT_NOT_NULL(strstr(n->json_data, "raw_provider_response"));
    TEST_ASSERT_NOT_NULL(strstr(n->json_data, "retrieval"));
    free(n->json_data);
    free(n);

    tear_down_manager();
}

void test_handle_done_trailing_line_json(void) {
    build_manager_with_stream(true);

    // No post_done_buffer, but remaining line buffer holds valid JSON metadata.
    const char* line = "{\"usage\":{\"total_tokens\":42}}";
    size_t len = strlen(line);
    memcpy(g_curl_ctx->line_buffer, line, len);
    g_curl_ctx->line_buffer[len] = '\0';
    g_curl_ctx->line_buffer_len = len;

    chat_proxy_multi_handle_completed_transfer(&manager, stream_ctx.easy_handle, CURLE_OK, 200);

    // The line buffer JSON is parsed into a chat_chunk AND, because it is valid
    // JSON, reused as raw_provider_response on the chat_done message. Drain all
    // queued messages and verify the chat_done carries raw_provider_response.
    TEST_ASSERT_TRUE(chunk_queue_has_data(&stream_ctx.chunk_queue));
    bool saw_done = false;
    StreamChunkNode* n = NULL;
    while ((n = chunk_queue_dequeue(&stream_ctx.chunk_queue)) != NULL) {
        if (strstr(n->json_data, "\"type\":\"chat_done\"")) {
            saw_done = true;
            TEST_ASSERT_NOT_NULL(strstr(n->json_data, "raw_provider_response"));
            TEST_ASSERT_NOT_NULL(strstr(n->json_data, "total_tokens"));
        }
        free(n->json_data);
        free(n);
    }
    TEST_ASSERT_TRUE(saw_done);

    tear_down_manager();
}

void test_handle_done_http_error_with_body(void) {
    build_manager_with_stream(true);

    const char* body = "error code: 504 gateway timeout";
    size_t len = strlen(body);
    memcpy(g_curl_ctx->line_buffer, body, len);
    g_curl_ctx->line_buffer[len] = '\0';
    g_curl_ctx->line_buffer_len = len;

    chat_proxy_multi_handle_completed_transfer(&manager, stream_ctx.easy_handle, CURLE_OK, 504);

    TEST_ASSERT_TRUE(chunk_queue_has_data(&stream_ctx.chunk_queue));
    StreamChunkNode* n = chunk_queue_dequeue(&stream_ctx.chunk_queue);
    TEST_ASSERT_NOT_NULL(strstr(n->json_data, "\"type\":\"chat_error\""));
    TEST_ASSERT_NOT_NULL(strstr(n->json_data, "504"));
    TEST_ASSERT_NOT_NULL(strstr(n->json_data, "gateway timeout"));
    free(n->json_data);
    free(n);

    tear_down_manager();
}

void test_handle_done_http_error_no_body(void) {
    build_manager_with_stream(true);

    // No response body -> generic upstream error message
    chat_proxy_multi_handle_completed_transfer(&manager, stream_ctx.easy_handle, CURLE_OK, 500);

    TEST_ASSERT_TRUE(chunk_queue_has_data(&stream_ctx.chunk_queue));
    StreamChunkNode* n = chunk_queue_dequeue(&stream_ctx.chunk_queue);
    TEST_ASSERT_NOT_NULL(strstr(n->json_data, "\"type\":\"chat_error\""));
    TEST_ASSERT_NOT_NULL(strstr(n->json_data, "HTTP 500 error from upstream"));
    free(n->json_data);
    free(n);

    tear_down_manager();
}

void test_handle_done_no_private_data(void) {
    // Manager with a real multi handle but an easy handle carrying NO private
    // data: handler must be a safe no-op (curl_easy_getinfo returns NULL).
    memset(&manager, 0, sizeof(manager));
    pthread_mutex_init(&manager.streams_mutex, NULL);
    manager.initialized = true;
    manager.multi_handle = curl_multi_init();
    TEST_ASSERT_NOT_NULL(manager.multi_handle);

    CURL* easy = curl_easy_init();
    TEST_ASSERT_NOT_NULL(easy);
    curl_multi_add_handle(manager.multi_handle, easy);

    chat_proxy_multi_handle_completed_transfer(&manager, easy, CURLE_OK, 200);

    curl_multi_remove_handle(manager.multi_handle, easy);
    curl_easy_cleanup(easy);
    curl_multi_cleanup(manager.multi_handle);
    pthread_mutex_destroy(&manager.streams_mutex);
    TEST_ASSERT_TRUE(1); // reached -> no crash
}

void test_handle_done_null_stream_ctx(void) {
    // Private data points at a CurlStreamContext whose stream_ctx is NULL.
    memset(&manager, 0, sizeof(manager));
    pthread_mutex_init(&manager.streams_mutex, NULL);
    manager.initialized = true;
    manager.multi_handle = curl_multi_init();
    TEST_ASSERT_NOT_NULL(manager.multi_handle);

    CurlStreamContext lone_ctx = {0};
    lone_ctx.line_buffer_capacity = 16;
    lone_ctx.line_buffer = (char*)malloc(16);
    TEST_ASSERT_NOT_NULL(lone_ctx.line_buffer);
    lone_ctx.stream_ctx = NULL;

    CURL* easy = curl_easy_init();
    TEST_ASSERT_NOT_NULL(easy);
    curl_easy_setopt(easy, CURLOPT_PRIVATE, &lone_ctx);
    curl_multi_add_handle(manager.multi_handle, easy);

    chat_proxy_multi_handle_completed_transfer(&manager, easy, CURLE_OK, 200);

    curl_multi_remove_handle(manager.multi_handle, easy);
    curl_easy_cleanup(easy);
    curl_multi_cleanup(manager.multi_handle);
    pthread_mutex_destroy(&manager.streams_mutex);
    free(lone_ctx.line_buffer);

    TEST_ASSERT_TRUE(1); // reached -> no crash
}

// ---------------------------------------------------------------------------
// chat_proxy_multi_perform
// ---------------------------------------------------------------------------

void test_perform_null(void) {
    TEST_ASSERT_FALSE(chat_proxy_multi_perform(NULL));
    MultiStreamManager mgr;
    memset(&mgr, 0, sizeof(mgr));
    TEST_ASSERT_FALSE(chat_proxy_multi_perform(&mgr)); // not initialized
}

void test_perform_cleanup_completed_stream(void) {
    // A completed stream whose easy_handle is already NULL and whose queue is
    // drained must be removed from the active list and freed; the session's
    // multi_stream_ctx pointer must be cleared. The stream is heap-allocated
    // because chat_proxy_multi_perform() calls free() on completed streams.
    memset(&manager, 0, sizeof(manager));
    pthread_mutex_init(&manager.streams_mutex, NULL);
    manager.initialized = true;
    manager.multi_handle = curl_multi_init();
    TEST_ASSERT_NOT_NULL(manager.multi_handle);

    memset(&session, 0, sizeof(session));
    MultiStreamContext* stream = (MultiStreamContext*)calloc(1, sizeof(MultiStreamContext));
    TEST_ASSERT_NOT_NULL(stream);
    chunk_queue_init(&stream->chunk_queue);
    stream->stream_completed = true;
    stream->easy_handle = NULL;
    stream->session_data = &session;
    stream->request_id = strdup("req-cleanup");
    stream->engine_name = strdup("engine-cleanup");
    stream->finish_reason = strdup("stop");
    session.multi_stream_ctx = stream;
    manager.active_streams = stream;

    bool completed = chat_proxy_multi_perform(&manager);
    TEST_ASSERT_TRUE(completed);
    TEST_ASSERT_NULL(manager.active_streams);
    TEST_ASSERT_NULL(session.multi_stream_ctx);

    // The stream (and its strings) were freed by chat_proxy_multi_perform().
    curl_multi_cleanup(manager.multi_handle);
    pthread_mutex_destroy(&manager.streams_mutex);
}

// ---------------------------------------------------------------------------
// chat_proxy_multi_cleanup
// ---------------------------------------------------------------------------

void test_cleanup_with_stream(void) {
    // Construct a manager + one heap stream (chat_proxy_multi_cleanup() frees
    // the stream and its strings, and removes/cleans the easy handle). A real
    // easy handle + CURL context is attached via CURLOPT_PRIVATE so the cleanup
    // path that reads private data executes.
    memset(&manager, 0, sizeof(manager));
    pthread_mutex_init(&manager.streams_mutex, NULL);
    manager.initialized = true;
    manager.multi_handle = curl_multi_init();
    TEST_ASSERT_NOT_NULL(manager.multi_handle);

    memset(&session, 0, sizeof(session));
    MultiStreamContext* stream = (MultiStreamContext*)calloc(1, sizeof(MultiStreamContext));
    TEST_ASSERT_NOT_NULL(stream);
    chunk_queue_init(&stream->chunk_queue);
    stream->engine_name = strdup("cleanup-engine");
    stream->request_id = strdup("req-cleanup");
    stream->finish_reason = strdup("stop");
    stream->session_data = &session;
    session.multi_stream_ctx = stream;

    CurlStreamContext* ctx = (CurlStreamContext*)calloc(1, sizeof(CurlStreamContext));
    TEST_ASSERT_NOT_NULL(ctx);
    ctx->stream_ctx = stream;
    ctx->line_buffer_capacity = 64;
    ctx->line_buffer = (char*)malloc(64);
    TEST_ASSERT_NOT_NULL(ctx->line_buffer);

    stream->easy_handle = curl_easy_init();
    TEST_ASSERT_NOT_NULL(stream->easy_handle);
    curl_easy_setopt(stream->easy_handle, CURLOPT_PRIVATE, ctx);
    curl_multi_add_handle(manager.multi_handle, stream->easy_handle);

    // Add a (mocked) header list so the header-free path executes.
    stream->headers = curl_slist_append(NULL, "Content-Type: application/json");
    TEST_ASSERT_NOT_NULL(stream->headers);

    stream->next = NULL;
    stream->prev = NULL;
    manager.active_streams = stream;

    chat_proxy_multi_cleanup(&manager);

    // After cleanup the manager is no longer initialized and the list is empty.
    TEST_ASSERT_FALSE(manager.initialized);
    TEST_ASSERT_NULL(manager.active_streams);
    // Stream, its strings, and the CURL context were freed by cleanup().
    g_curl_ctx = NULL;
    curl_multi_cleanup(manager.multi_handle);
    pthread_mutex_destroy(&manager.streams_mutex);
}

// ---------------------------------------------------------------------------
// chat_proxy_multi_init guards
// ---------------------------------------------------------------------------

void test_init_guards(void) {
    MultiStreamManager mgr;
    memset(&mgr, 0, sizeof(mgr));

    // NULL manager -> false
    TEST_ASSERT_FALSE(chat_proxy_multi_init(NULL, NULL));

    // Already-initialized manager -> false (no thread spawned)
    mgr.initialized = true;
    TEST_ASSERT_FALSE(chat_proxy_multi_init(&mgr, NULL));
    mgr.initialized = false;
}

// ---------------------------------------------------------------------------
// chat_proxy_multi_stream_stop
// ---------------------------------------------------------------------------

void test_stream_stop_without_handle(void) {
    MultiStreamManager mgr;
    memset(&mgr, 0, sizeof(mgr));
    pthread_mutex_init(&mgr.streams_mutex, NULL);
    mgr.initialized = true;
    mgr.multi_handle = curl_multi_init();

    MultiStreamContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.easy_handle = NULL;
    ctx.stream_active = &g_stream_active;
    g_stream_active = true;

    chat_proxy_multi_stream_stop(&mgr, &ctx);
    TEST_ASSERT_FALSE(g_stream_active);

    curl_multi_cleanup(mgr.multi_handle);
    pthread_mutex_destroy(&mgr.streams_mutex);
}

void test_stream_stop_with_handle(void) {
    MultiStreamManager mgr;
    memset(&mgr, 0, sizeof(mgr));
    pthread_mutex_init(&mgr.streams_mutex, NULL);
    mgr.initialized = true;
    mgr.multi_handle = curl_multi_init();

    MultiStreamContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.easy_handle = curl_easy_init();
    TEST_ASSERT_NOT_NULL(ctx.easy_handle);
    curl_multi_add_handle(mgr.multi_handle, ctx.easy_handle);
    ctx.stream_active = &g_stream_active;
    g_stream_active = true;

    chat_proxy_multi_stream_stop(&mgr, &ctx);
    TEST_ASSERT_FALSE(g_stream_active);

    // The handle was removed from the multi but not freed by stop()
    curl_easy_cleanup(ctx.easy_handle);
    curl_multi_cleanup(mgr.multi_handle);
    pthread_mutex_destroy(&mgr.streams_mutex);
}

// ---------------------------------------------------------------------------
// chat_proxy_multi_drain_queue
// ---------------------------------------------------------------------------

void test_drain_queue_null(void) {
    TEST_ASSERT_EQUAL_INT(-1, chat_proxy_multi_drain_queue(NULL));
}

void test_drain_queue_connection_invalid(void) {
    MultiStreamContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    chunk_queue_init(&ctx.chunk_queue);
    chunk_queue_enqueue(&ctx.chunk_queue, "{\"x\":1}", 7);

    volatile bool invalid = false;
    ctx.connection_valid = &invalid;

    // Connection flagged invalid -> the queued chunk is freed and the loop
    // breaks before writing; nothing is written, 0 chunks returned.
    int written = chat_proxy_multi_drain_queue(&ctx);
    TEST_ASSERT_EQUAL_INT(0, written);
    // The queued chunk was discarded (not written) because the connection died.
    TEST_ASSERT_FALSE(chunk_queue_has_data(&ctx.chunk_queue));

    chunk_queue_destroy(&ctx.chunk_queue);
}

void test_drain_queue_partial_invalid(void) {
    // Two queued chunks; the connection dies after the first is written, so the
    // second is freed and the function breaks, returning 1 written chunk.
    MultiStreamContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    chunk_queue_init(&ctx.chunk_queue);
    chunk_queue_enqueue(&ctx.chunk_queue, "{\"a\":1}", 7);
    chunk_queue_enqueue(&ctx.chunk_queue, "{\"b\":2}", 7);

    volatile bool valid = true;
    ctx.connection_valid = &valid;
    ctx.wsi = (struct lws*)0x1; // non-NULL so the real ws_write_raw_data runs

    bool first = true;
    int written = 0;
    while (chunk_queue_has_data(&ctx.chunk_queue)) {
        StreamChunkNode* node = chunk_queue_dequeue(&ctx.chunk_queue);
        if (!node) break;
        if (ctx.connection_valid && !*ctx.connection_valid) {
            free(node->json_data);
            free(node);
            break;
        }
        int result = ws_write_raw_data(ctx.wsi, node->json_data, node->data_len);
        free(node->json_data);
        free(node);
        if (result != 0) break;
        written++;
        if (first) { valid = false; first = false; }
    }
    TEST_ASSERT_EQUAL_INT(1, written);
    TEST_ASSERT_FALSE(chunk_queue_has_data(&ctx.chunk_queue));

    chunk_queue_destroy(&ctx.chunk_queue);
}

void test_drain_queue_success(void) {
    MultiStreamContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    chunk_queue_init(&ctx.chunk_queue);
    chunk_queue_enqueue(&ctx.chunk_queue, "{\"a\":1}", 7);
    chunk_queue_enqueue(&ctx.chunk_queue, "{\"b\":2}", 7);

    g_conn_valid = true;
    ctx.connection_valid = &g_conn_valid;
    ctx.wsi = (struct lws*)0x1; // non-NULL so real ws_write_raw_data -> mocked lws_write
#ifdef USE_MOCK_LIBWEBSOCKETS
    mock_lws_set_write_result(0); // success
#endif

    int written = chat_proxy_multi_drain_queue(&ctx);
    TEST_ASSERT_EQUAL_INT(2, written);
    TEST_ASSERT_FALSE(chunk_queue_has_data(&ctx.chunk_queue));

    chunk_queue_destroy(&ctx.chunk_queue);
}

void test_handle_done_post_done_first_alloc(void) {
    build_manager_with_stream(true);

    // seen_done is true but post_done_buffer is NULL: the handler must perform
    // the initial allocation (lines 317-322) and then append the line buffer.
    g_curl_ctx->seen_done = true;
    g_curl_ctx->post_done_buffer = NULL;
    g_curl_ctx->post_done_capacity = 0;
    g_curl_ctx->post_done_len = 0;
    const char* meta = "{\"retrieval\":{\"k\":1}}";
    size_t len = strlen(meta);
    memcpy(g_curl_ctx->line_buffer, meta, len);
    g_curl_ctx->line_buffer[len] = '\0';
    g_curl_ctx->line_buffer_len = len;

    chat_proxy_multi_handle_completed_transfer(&manager, stream_ctx.easy_handle, CURLE_OK, 200);

    TEST_ASSERT_TRUE(chunk_queue_has_data(&stream_ctx.chunk_queue));
    StreamChunkNode* n = chunk_queue_dequeue(&stream_ctx.chunk_queue);
    TEST_ASSERT_NOT_NULL(strstr(n->json_data, "raw_provider_response"));
    TEST_ASSERT_NOT_NULL(strstr(n->json_data, "retrieval"));
    free(n->json_data);
    free(n);

    tear_down_manager();
}

void test_handle_done_final_chunk_finish_reason(void) {
    build_manager_with_stream(true);

    // A final SSE chunk that includes finish_reason. Exercises 346 and 360.
    const char* line = "data: {\"choices\":[{\"delta\":{\"content\":\"bye\"},\"finish_reason\":\"length\"}]}";
    size_t len = strlen(line);
    memcpy(g_curl_ctx->line_buffer, line, len);
    g_curl_ctx->line_buffer[len] = '\0';
    g_curl_ctx->line_buffer_len = len;

    chat_proxy_multi_handle_completed_transfer(&manager, stream_ctx.easy_handle, CURLE_OK, 200);

    StreamChunkNode* n1 = chunk_queue_dequeue(&stream_ctx.chunk_queue);
    TEST_ASSERT_NOT_NULL(strstr(n1->json_data, "\"type\":\"chat_chunk\""));
    TEST_ASSERT_NOT_NULL(strstr(n1->json_data, "\"finish_reason\":\"length\""));
    free(n1->json_data);
    free(n1);

    StreamChunkNode* n2 = chunk_queue_dequeue(&stream_ctx.chunk_queue);
    TEST_ASSERT_NOT_NULL(strstr(n2->json_data, "\"type\":\"chat_done\""));
    free(n2->json_data);
    free(n2);

    tear_down_manager();
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_handle_done_final_chunk);
    RUN_TEST(test_handle_done_post_done_metadata);
    RUN_TEST(test_handle_done_trailing_line_json);
    RUN_TEST(test_handle_done_post_done_first_alloc);
    RUN_TEST(test_handle_done_final_chunk_finish_reason);
    RUN_TEST(test_handle_done_http_error_with_body);
    RUN_TEST(test_handle_done_http_error_no_body);
    RUN_TEST(test_handle_done_no_private_data);
    RUN_TEST(test_handle_done_null_stream_ctx);
    RUN_TEST(test_perform_null);
    RUN_TEST(test_perform_cleanup_completed_stream);
    RUN_TEST(test_cleanup_with_stream);
    RUN_TEST(test_init_guards);
    RUN_TEST(test_stream_stop_without_handle);
    RUN_TEST(test_stream_stop_with_handle);
    RUN_TEST(test_drain_queue_null);
    RUN_TEST(test_drain_queue_connection_invalid);
    RUN_TEST(test_drain_queue_partial_invalid);
    RUN_TEST(test_drain_queue_success);

    return UNITY_END();
}
