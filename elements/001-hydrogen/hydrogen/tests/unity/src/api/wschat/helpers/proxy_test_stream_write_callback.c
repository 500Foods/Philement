/*
 * Unity Test File: Chat Proxy - Streaming Write Callback
 * Tests chat_proxy_stream_write_callback() in proxy.c.
 *
 * The callback parses incoming SSE bytes into lines, buffering partial lines
 * until a newline arrives, then parses each complete line into a ChatStreamChunk
 * and invokes the supplied chunk_callback. It also tracks bytes/chunks and the
 * first-data-arrival marker.
 *
 * NOTE: StreamContext is a private struct defined in proxy.c; its layout is
 * reproduced here so the exposed callback can be exercised directly.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/wschat/helpers/proxy.h>
#include <src/api/wschat/helpers/resp_parser.h>

// Mirror of StreamContext in proxy.c (must stay in sync with that definition)
typedef struct {
    ChatProxyStreamChunkCallback chunk_callback;
    void* user_data;
    char* line_buffer;
    size_t line_buffer_len;
    size_t line_buffer_capacity;
    size_t bytes_received;
    size_t chunks_processed;
    bool first_data_logged;
} StreamContext;

// Function prototype (declared non-static in proxy.h for testing)
size_t chat_proxy_stream_write_callback(const void* contents, size_t size, size_t nmemb, void* userp);

// Test function prototypes
void test_stream_write_callback_single_line(void);
void test_stream_write_callback_split_across_calls(void);
void test_stream_write_callback_multiple_lines(void);
void test_stream_write_callback_empty_data(void);

// Test fixtures
static StreamContext ctx;
static int g_chunk_calls;
static char g_last_content[256];

static void test_chunk_cb(const ChatStreamChunk* chunk, void* user_data) {
    (void)user_data;
    g_chunk_calls++;
    if (chunk && chunk->content) {
        strncpy(g_last_content, chunk->content, sizeof(g_last_content) - 1);
        g_last_content[sizeof(g_last_content) - 1] = '\0';
    }
}

void setUp(void) {
    memset(&ctx, 0, sizeof(ctx));
    ctx.chunk_callback = test_chunk_cb;
    ctx.user_data = NULL;
    ctx.line_buffer_capacity = 64;
    ctx.line_buffer = (char*)malloc(ctx.line_buffer_capacity);
    TEST_ASSERT_NOT_NULL(ctx.line_buffer);
    ctx.line_buffer[0] = '\0';
    g_chunk_calls = 0;
    g_last_content[0] = '\0';
}

void tearDown(void) {
    free(ctx.line_buffer);
    ctx.line_buffer = NULL;
}

void test_stream_write_callback_single_line(void) {
    // A complete SSE line (OpenAI-style content delta) ending in newline
    const char* sse = "data: {\"choices\":[{\"delta\":{\"content\":\"Hello\"}}]}\n";
    size_t written = chat_proxy_stream_write_callback(sse, 1, strlen(sse), &ctx);
    TEST_ASSERT_EQUAL_size_t(strlen(sse), written);
    TEST_ASSERT_EQUAL_size_t(strlen(sse), ctx.bytes_received);
    TEST_ASSERT_TRUE(ctx.first_data_logged);
    TEST_ASSERT_EQUAL_size_t(1, g_chunk_calls);
    TEST_ASSERT_EQUAL_STRING("Hello", g_last_content);
    // line buffer reset after newline
    TEST_ASSERT_EQUAL_size_t(0, ctx.line_buffer_len);
}

void test_stream_write_callback_split_across_calls(void) {
    const char* part1 = "data: {\"choices\":[{\"delta\":{\"content\":\"Hi";
    const char* part2 = " there\"}}]}\n";

    size_t w1 = chat_proxy_stream_write_callback(part1, 1, strlen(part1), &ctx);
    TEST_ASSERT_EQUAL_size_t(strlen(part1), w1);
    // No newline yet -> callback not invoked, data buffered
    TEST_ASSERT_EQUAL_size_t(0, g_chunk_calls);
    TEST_ASSERT_EQUAL_size_t(strlen(part1), ctx.line_buffer_len);

    size_t w2 = chat_proxy_stream_write_callback(part2, 1, strlen(part2), &ctx);
    TEST_ASSERT_EQUAL_size_t(strlen(part2), w2);
    TEST_ASSERT_EQUAL_size_t(1, g_chunk_calls);
    TEST_ASSERT_EQUAL_STRING("Hi there", g_last_content);
}

void test_stream_write_callback_multiple_lines(void) {
    const char* sse =
        "data: {\"choices\":[{\"delta\":{\"content\":\"one\"}}]}\n"
        "data: {\"choices\":[{\"delta\":{\"content\":\"two\"}}]}\n";
    size_t written = chat_proxy_stream_write_callback(sse, 1, strlen(sse), &ctx);
    TEST_ASSERT_EQUAL_size_t(strlen(sse), written);
    TEST_ASSERT_EQUAL_size_t(2, g_chunk_calls);
    TEST_ASSERT_EQUAL_STRING("two", g_last_content);
}

void test_stream_write_callback_empty_data(void) {
    // Zero bytes must be tolerated
    size_t written = chat_proxy_stream_write_callback("", 1, 0, &ctx);
    TEST_ASSERT_EQUAL_size_t(0, written);
    TEST_ASSERT_EQUAL_size_t(0, g_chunk_calls);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_stream_write_callback_single_line);
    RUN_TEST(test_stream_write_callback_split_across_calls);
    RUN_TEST(test_stream_write_callback_multiple_lines);
    RUN_TEST(test_stream_write_callback_empty_data);

    return UNITY_END();
}
