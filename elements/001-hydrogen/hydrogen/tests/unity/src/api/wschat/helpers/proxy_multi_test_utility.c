/*
 * Unity Test File: Chat Proxy Multi - Utility Functions
 * Tests the network-independent utility helpers in proxy_multi.c:
 *   - chat_proxy_multi_get_stats
 *   - chat_proxy_multi_has_queued_data
 *   - chat_proxy_multi_socket_action (guards)
 *   - chat_proxy_multi_timer_callback (guards)
 *   - chat_proxy_multi_request_writable (mocked lws)
 *   - chat_proxy_multi_has_active_streams
 *   - chat_proxy_multi_get_stream_count
 *
 * Manager-based functions that lock streams_mutex are exercised against a
 * locally constructed manager with a properly initialized mutex. No real CURL
 * or LWS handles are created; lws calls are mocked globally.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <curl/curl.h>

#include <src/api/wschat/helpers/proxy_multi.h>

// Function prototypes (declared non-static in proxy_multi.h for testing)
void chat_proxy_multi_get_stats(const MultiStreamContext* context,
                                size_t* chunks_queued, size_t* chunks_sent);
bool chat_proxy_multi_has_queued_data(const MultiStreamContext* context);
void chat_proxy_multi_request_writable(MultiStreamContext* context);
bool chat_proxy_multi_has_active_streams(const MultiStreamManager* manager);
size_t chat_proxy_multi_get_stream_count(const MultiStreamManager* manager);

// Test function prototypes
void test_get_stats_null(void);
void test_get_stats_counts(void);
void test_has_queued_data_null(void);
void test_has_queued_data_empty(void);
void test_has_queued_data_with_data(void);
void test_socket_action_null_manager(void);
void test_socket_action_uninitialized(void);
void test_timer_callback_null_manager(void);
void test_timer_callback_uninitialized(void);
void test_request_writable_null(void);
void test_request_writable_no_wsi(void);
void test_has_active_streams_null(void);
void test_has_active_streams_uninitialized(void);
void test_has_active_streams_empty(void);
void test_has_active_streams_with_stream(void);
void test_get_stream_count_null(void);
void test_get_stream_count_uninitialized(void);
void test_get_stream_count_list(void);

// Test fixtures
static MultiStreamManager manager;
static MultiStreamContext stream_a;
static MultiStreamContext stream_b;

void setUp(void) {
    memset(&manager, 0, sizeof(manager));
    pthread_mutex_init(&manager.streams_mutex, NULL);
    manager.initialized = true;
    manager.active_streams = NULL;

    memset(&stream_a, 0, sizeof(stream_a));
    memset(&stream_b, 0, sizeof(stream_b));
    chunk_queue_init(&stream_a.chunk_queue);
    chunk_queue_init(&stream_b.chunk_queue);
}

void tearDown(void) {
    chunk_queue_destroy(&stream_a.chunk_queue);
    chunk_queue_destroy(&stream_b.chunk_queue);
    pthread_mutex_destroy(&manager.streams_mutex);
}

/* ---- get_stats ---- */
void test_get_stats_null(void) {
    chat_proxy_multi_get_stats(NULL, NULL, NULL); // must not crash
    size_t queued = 0, sent = 0;
    chat_proxy_multi_get_stats(NULL, &queued, &sent);
    TEST_ASSERT_EQUAL_size_t(0, queued);
    TEST_ASSERT_EQUAL_size_t(0, sent);
}

void test_get_stats_counts(void) {
    stream_a.chunk_index = 7;
    chunk_queue_enqueue(&stream_a.chunk_queue, "{\"x\":1}", 7);
    chunk_queue_enqueue(&stream_a.chunk_queue, "{\"x\":2}", 7);

    size_t queued = 0, sent = 0;
    chat_proxy_multi_get_stats(&stream_a, &queued, &sent);
    TEST_ASSERT_EQUAL_size_t(2, queued);
    TEST_ASSERT_EQUAL_size_t(7, sent);
}

/* ---- has_queued_data ---- */
void test_has_queued_data_null(void) {
    TEST_ASSERT_FALSE(chat_proxy_multi_has_queued_data(NULL));
}

void test_has_queued_data_empty(void) {
    TEST_ASSERT_FALSE(chat_proxy_multi_has_queued_data(&stream_a));
}

void test_has_queued_data_with_data(void) {
    chunk_queue_enqueue(&stream_a.chunk_queue, "data", 4);
    TEST_ASSERT_TRUE(chat_proxy_multi_has_queued_data(&stream_a));
}

/* ---- socket_action (guard-only; multi_handle is NULL in our manager) ---- */
void test_socket_action_null_manager(void) {
    chat_proxy_multi_socket_action(NULL, 5, CURL_POLL_IN); // safe no-op
    TEST_ASSERT_TRUE(1);
}

void test_socket_action_uninitialized(void) {
    manager.initialized = false;
    chat_proxy_multi_socket_action(&manager, 5, CURL_POLL_IN); // safe no-op
    manager.initialized = true;
    TEST_ASSERT_TRUE(1);
}

/* ---- timer_callback (guard-only) ---- */
void test_timer_callback_null_manager(void) {
    chat_proxy_multi_timer_callback(NULL, 100); // safe no-op
    TEST_ASSERT_TRUE(1);
}

void test_timer_callback_uninitialized(void) {
    manager.initialized = false;
    chat_proxy_multi_timer_callback(&manager, 100); // safe no-op
    manager.initialized = true;
    TEST_ASSERT_TRUE(1);
}

/* ---- request_writable (mocked lws) ---- */
void test_request_writable_null(void) {
    chat_proxy_multi_request_writable(NULL); // safe no-op
    TEST_ASSERT_TRUE(1);
}

void test_request_writable_no_wsi(void) {
    stream_a.wsi = NULL;
    chat_proxy_multi_request_writable(&stream_a); // skip when wsi is NULL
    TEST_ASSERT_TRUE(1);
}

/* ---- has_active_streams ---- */
void test_has_active_streams_null(void) {
    TEST_ASSERT_FALSE(chat_proxy_multi_has_active_streams(NULL));
}

void test_has_active_streams_uninitialized(void) {
    manager.initialized = false;
    TEST_ASSERT_FALSE(chat_proxy_multi_has_active_streams(&manager));
    manager.initialized = true;
}

void test_has_active_streams_empty(void) {
    TEST_ASSERT_FALSE(chat_proxy_multi_has_active_streams(&manager));
}

void test_has_active_streams_with_stream(void) {
    manager.active_streams = &stream_a;
    TEST_ASSERT_TRUE(chat_proxy_multi_has_active_streams(&manager));
}

/* ---- get_stream_count ---- */
void test_get_stream_count_null(void) {
    TEST_ASSERT_EQUAL_size_t(0, chat_proxy_multi_get_stream_count(NULL));
}

void test_get_stream_count_uninitialized(void) {
    manager.initialized = false;
    TEST_ASSERT_EQUAL_size_t(0, chat_proxy_multi_get_stream_count(&manager));
    manager.initialized = true;
}

void test_get_stream_count_list(void) {
    manager.active_streams = &stream_a;
    stream_a.next = &stream_b;
    TEST_ASSERT_EQUAL_size_t(2, chat_proxy_multi_get_stream_count(&manager));
    stream_a.next = NULL;
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_get_stats_null);
    RUN_TEST(test_get_stats_counts);
    RUN_TEST(test_has_queued_data_null);
    RUN_TEST(test_has_queued_data_empty);
    RUN_TEST(test_has_queued_data_with_data);
    RUN_TEST(test_socket_action_null_manager);
    RUN_TEST(test_socket_action_uninitialized);
    RUN_TEST(test_timer_callback_null_manager);
    RUN_TEST(test_timer_callback_uninitialized);
    RUN_TEST(test_request_writable_null);
    RUN_TEST(test_request_writable_no_wsi);
    RUN_TEST(test_has_active_streams_null);
    RUN_TEST(test_has_active_streams_uninitialized);
    RUN_TEST(test_has_active_streams_empty);
    RUN_TEST(test_has_active_streams_with_stream);
    RUN_TEST(test_get_stream_count_null);
    RUN_TEST(test_get_stream_count_uninitialized);
    RUN_TEST(test_get_stream_count_list);

    return UNITY_END();
}
