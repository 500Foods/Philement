/*
 * Unity Test File: Chat Proxy Multi - Write Callback
 * Tests multi_stream_write_callback() in proxy_mc.c.
 *
 * The callback parses SSE bytes for the curl_multi streaming path, splitting
 * on newlines, buffering partial lines, parsing each complete line into a
 * ChatStreamChunk, building a chat_chunk JSON and enqueuing it onto the
 * stream's chunk queue. After [DONE] it accumulates subsequent lines into a
 * post-done buffer instead of enqueuing them.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/wschat/helpers/proxy_multi.h>
#include <src/api/wschat/helpers/resp_parser.h>

// Function prototype (declared non-static in proxy_multi.h for testing)
size_t multi_stream_write_callback(const void* contents, size_t size, size_t nmemb, void* userp);

// Test function prototypes
void test_multi_write_callback_enqueues_chunk(void);
void test_multi_write_callback_buffers_partial_line(void);
void test_multi_write_callback_connection_invalid(void);
void test_multi_write_callback_post_done_accumulates(void);
void test_multi_write_callback_grows_line_buffer(void);
void test_multi_write_callback_null_context(void);

// Test fixtures
static MultiStreamContext stream_ctx;
static CurlStreamContext curl_ctx;
static volatile bool g_conn_valid = true;

void setUp(void) {
    memset(&stream_ctx, 0, sizeof(stream_ctx));
    memset(&curl_ctx, 0, sizeof(curl_ctx));
    curl_ctx.stream_ctx = &stream_ctx;
    stream_ctx.connection_valid = &g_conn_valid;
    stream_ctx.chunk_index = 0;
    chunk_queue_init(&stream_ctx.chunk_queue);
    curl_ctx.line_buffer_capacity = 64;
    curl_ctx.line_buffer = (char*)malloc(curl_ctx.line_buffer_capacity);
    TEST_ASSERT_NOT_NULL(curl_ctx.line_buffer);
    curl_ctx.line_buffer[0] = '\0';
    g_conn_valid = true;
}

void tearDown(void) {
    chunk_queue_destroy(&stream_ctx.chunk_queue);
    free(curl_ctx.line_buffer);
    curl_ctx.line_buffer = NULL;
    free(curl_ctx.post_done_buffer);
    curl_ctx.post_done_buffer = NULL;
}

void test_multi_write_callback_enqueues_chunk(void) {
    const char* sse = "data: {\"choices\":[{\"delta\":{\"content\":\"Hi\"}}]}\n";
    size_t written = multi_stream_write_callback(sse, 1, strlen(sse), &curl_ctx);
    TEST_ASSERT_EQUAL_size_t(strlen(sse), written);
    TEST_ASSERT_EQUAL_size_t(1, curl_ctx.chunks_processed);
    // A chunk was queued
    TEST_ASSERT_TRUE(chunk_queue_has_data(&stream_ctx.chunk_queue));
    // Drain and verify it parses to expected JSON containing the content
    StreamChunkNode* node = chunk_queue_dequeue(&stream_ctx.chunk_queue);
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_NOT_NULL(strstr(node->json_data, "\"content\":\"Hi\""));
    TEST_ASSERT_NOT_NULL(strstr(node->json_data, "\"type\":\"chat_chunk\""));
    free(node->json_data);
    free(node);
}

void test_multi_write_callback_buffers_partial_line(void) {
    const char* part = "data: {\"choices\":[{\"delta\":{\"content\":\"par";
    size_t written = multi_stream_write_callback(part, 1, strlen(part), &curl_ctx);
    TEST_ASSERT_EQUAL_size_t(strlen(part), written);
    // No newline -> no chunk enqueued, data buffered
    TEST_ASSERT_EQUAL_size_t(0, curl_ctx.chunks_processed);
    TEST_ASSERT_FALSE(chunk_queue_has_data(&stream_ctx.chunk_queue));
    TEST_ASSERT_EQUAL_size_t(strlen(part), curl_ctx.line_buffer_len);

    const char* rest = "tial\"}}]}\n";
    written = multi_stream_write_callback(rest, 1, strlen(rest), &curl_ctx);
    TEST_ASSERT_EQUAL_size_t(strlen(rest), written);
    TEST_ASSERT_EQUAL_size_t(1, curl_ctx.chunks_processed);
    TEST_ASSERT_TRUE(chunk_queue_has_data(&stream_ctx.chunk_queue));
}

void test_multi_write_callback_connection_invalid(void) {
    g_conn_valid = false;
    const char* sse = "data: {\"choices\":[{\"delta\":{\"content\":\"x\"}}]}\n";
    size_t written = multi_stream_write_callback(sse, 1, strlen(sse), &curl_ctx);
    TEST_ASSERT_EQUAL_size_t(0, written);
    TEST_ASSERT_EQUAL_size_t(0, curl_ctx.chunks_processed);
}

void test_multi_write_callback_post_done_accumulates(void) {
    // First feed a [DONE] line so seen_done becomes true
    const char* done = "data: [DONE]\n";
    size_t w = multi_stream_write_callback(done, 1, strlen(done), &curl_ctx);
    TEST_ASSERT_EQUAL_size_t(strlen(done), w);
    TEST_ASSERT_TRUE(curl_ctx.seen_done);

    // Subsequent line should be captured into post_done_buffer, not enqueued
    const char* meta = "{\"retrieval\":\"some-data\"}\n";
    w = multi_stream_write_callback(meta, 1, strlen(meta), &curl_ctx);
    TEST_ASSERT_EQUAL_size_t(strlen(meta), w);
    TEST_ASSERT_NOT_NULL(curl_ctx.post_done_buffer);
    TEST_ASSERT_NOT_NULL(strstr(curl_ctx.post_done_buffer, "retrieval"));
}

void test_multi_write_callback_grows_line_buffer(void) {
    // Feed a very long single line exceeding the 64-byte initial capacity
    char* big = (char*)malloc(4096);
    TEST_ASSERT_NOT_NULL(big);
    int n = snprintf(big, 4096, "data: {\"choices\":[{\"delta\":{\"content\":\"");
    for (int i = 0; i < 3000; i++) big[n + i] = 'a';
    int off = n + 3000;
    big[off++] = '"';
    big[off++] = '}';
    big[off++] = ']';
    big[off++] = '}';
    big[off++] = '\n';
    big[off] = '\0';

    size_t written = multi_stream_write_callback(big, 1, strlen(big), &curl_ctx);
    TEST_ASSERT_EQUAL_size_t(strlen(big), written);
    // The oversized line forces the line buffer to grow beyond its initial capacity
    TEST_ASSERT_TRUE(curl_ctx.line_buffer_capacity >= strlen(big));
    free(big);
}

void test_multi_write_callback_null_context(void) {
    size_t written = multi_stream_write_callback("x", 1, 1, NULL);
    TEST_ASSERT_EQUAL_size_t(0, written);

    written = multi_stream_write_callback("x", 1, 1, &(CurlStreamContext){0});
    TEST_ASSERT_EQUAL_size_t(0, written);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_multi_write_callback_enqueues_chunk);
    RUN_TEST(test_multi_write_callback_buffers_partial_line);
    RUN_TEST(test_multi_write_callback_connection_invalid);
    RUN_TEST(test_multi_write_callback_post_done_accumulates);
    RUN_TEST(test_multi_write_callback_grows_line_buffer);
    RUN_TEST(test_multi_write_callback_null_context);

    return UNITY_END();
}
