#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/wschat/helpers/proxy_multi.h>
#include <src/api/wschat/helpers/proxy.h>
#include <string.h>

void test_chunk_queue_init(void);
void test_chunk_queue_enqueue_dequeue(void);
void test_chunk_queue_has_data_empty(void);
void test_chunk_queue_has_data_full(void);
void test_chunk_queue_get_count_empty(void);
void test_chunk_queue_get_count_after_enqueue(void);
void test_chunk_queue_dequeue_empty(void);
void test_chunk_queue_destroy(void);
void test_chunk_queue_multiple_enqueue_dequeue(void);
void test_multi_result_create_null(void);
void test_multi_result_create_valid(void);
void test_multi_result_destroy_null(void);
void test_proxy_get_default_config(void);
void test_proxy_get_streaming_config(void);
void test_proxy_result_is_success_null(void);
void test_proxy_result_is_success_ok(void);
void test_proxy_result_is_success_error(void);
void test_proxy_result_should_retry_null(void);
void test_proxy_result_should_retry_5xx(void);
void test_proxy_result_should_retry_4xx(void);
void test_proxy_result_should_retry_ok(void);
void test_proxy_result_create_destroy(void);
void test_proxy_result_destroy_null(void);

void setUp(void) {}
void tearDown(void) {}

void test_chunk_queue_init(void) {
    StreamChunkQueue queue;
    chunk_queue_init(&queue);
    TEST_ASSERT_NULL(queue.head);
    TEST_ASSERT_NULL(queue.tail);
    TEST_ASSERT_EQUAL_size_t(0, queue.count);
}

void test_chunk_queue_enqueue_dequeue(void) {
    StreamChunkQueue queue;
    chunk_queue_init(&queue);

    TEST_ASSERT_TRUE(chunk_queue_enqueue(&queue, "chunk1", 6));
    TEST_ASSERT_EQUAL_size_t(1, queue.count);

    StreamChunkNode *node = chunk_queue_dequeue(&queue);
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_EQUAL_STRING("chunk1", node->json_data);
    TEST_ASSERT_EQUAL_size_t(6, node->data_len);
    free(node->json_data);
    free(node);
    TEST_ASSERT_EQUAL_size_t(0, queue.count);
}

void test_chunk_queue_has_data_empty(void) {
    StreamChunkQueue queue;
    chunk_queue_init(&queue);
    TEST_ASSERT_FALSE(chunk_queue_has_data(&queue));
}

void test_chunk_queue_has_data_full(void) {
    StreamChunkQueue queue;
    chunk_queue_init(&queue);
    chunk_queue_enqueue(&queue, "data", 4);
    TEST_ASSERT_TRUE(chunk_queue_has_data(&queue));

    StreamChunkNode *node = chunk_queue_dequeue(&queue);
    free(node->json_data);
    free(node);
    TEST_ASSERT_FALSE(chunk_queue_has_data(&queue));
}

void test_chunk_queue_get_count_empty(void) {
    StreamChunkQueue queue;
    chunk_queue_init(&queue);
    TEST_ASSERT_EQUAL_size_t(0, chunk_queue_get_count(&queue));
}

void test_chunk_queue_get_count_after_enqueue(void) {
    StreamChunkQueue queue;
    chunk_queue_init(&queue);
    chunk_queue_enqueue(&queue, "a", 1);
    chunk_queue_enqueue(&queue, "b", 1);
    TEST_ASSERT_EQUAL_size_t(2, chunk_queue_get_count(&queue));
}

void test_chunk_queue_dequeue_empty(void) {
    StreamChunkQueue queue;
    chunk_queue_init(&queue);
    TEST_ASSERT_NULL(chunk_queue_dequeue(&queue));
}

void test_chunk_queue_destroy(void) {
    StreamChunkQueue queue;
    chunk_queue_init(&queue);
    chunk_queue_enqueue(&queue, "data", 4);
    chunk_queue_destroy(&queue);
    TEST_ASSERT_NULL(queue.head);
    TEST_ASSERT_NULL(queue.tail);
}

void test_chunk_queue_multiple_enqueue_dequeue(void) {
    StreamChunkQueue queue;
    chunk_queue_init(&queue);

    chunk_queue_enqueue(&queue, "first", 5);
    chunk_queue_enqueue(&queue, "second", 6);
    chunk_queue_enqueue(&queue, "third", 5);

    TEST_ASSERT_EQUAL_size_t(3, queue.count);

    StreamChunkNode *n1 = chunk_queue_dequeue(&queue);
    TEST_ASSERT_EQUAL_STRING("first", n1->json_data);
    free(n1->json_data); free(n1);

    StreamChunkNode *n2 = chunk_queue_dequeue(&queue);
    TEST_ASSERT_EQUAL_STRING("second", n2->json_data);
    free(n2->json_data); free(n2);

    StreamChunkNode *n3 = chunk_queue_dequeue(&queue);
    TEST_ASSERT_EQUAL_STRING("third", n3->json_data);
    free(n3->json_data); free(n3);

    TEST_ASSERT_EQUAL_size_t(0, queue.count);
}

void test_multi_result_create_null(void) {
    /* count=0 is a degenerate case; just verify it doesn't crash */
    ChatMultiResult *mr = chat_multi_result_create(0);
    TEST_ASSERT_NOT_NULL(mr);
    TEST_ASSERT_EQUAL_size_t(0, mr->count);
    free(mr->results);
    free(mr);
}

void test_multi_result_create_valid(void) {
    ChatMultiResult *mr = chat_multi_result_create(2);
    TEST_ASSERT_NOT_NULL(mr);
    TEST_ASSERT_EQUAL_size_t(2, mr->count);
    TEST_ASSERT_NULL(mr->results[0]);
    TEST_ASSERT_NULL(mr->results[1]);

    /* Clean up */
    free(mr->results);
    free(mr);
}

void test_multi_result_destroy_null(void) {
    chat_multi_result_destroy(NULL);
}

void test_proxy_get_default_config(void) {
    ChatProxyConfig config = chat_proxy_get_default_config();
    TEST_ASSERT_EQUAL_INT(10, config.connect_timeout_seconds);
    TEST_ASSERT_EQUAL_INT(60, config.request_timeout_seconds);
    TEST_ASSERT_EQUAL_INT(2, config.max_retries);
    TEST_ASSERT_TRUE(config.verify_ssl);
}

void test_proxy_get_streaming_config(void) {
    ChatProxyConfig config = chat_proxy_get_streaming_config();
    TEST_ASSERT_EQUAL_INT(10, config.connect_timeout_seconds);
    TEST_ASSERT_EQUAL_INT(600, config.request_timeout_seconds);
    TEST_ASSERT_EQUAL_INT(0, config.max_retries);
    TEST_ASSERT_TRUE(config.verify_ssl);
}

void test_proxy_result_is_success_null(void) {
    TEST_ASSERT_FALSE(chat_proxy_result_is_success(NULL));
}

void test_proxy_result_is_success_ok(void) {
    ChatProxyResult result;
    memset(&result, 0, sizeof(result));
    result.code = CHAT_PROXY_OK;
    result.http_status = 200;
    TEST_ASSERT_TRUE(chat_proxy_result_is_success(&result));
}

void test_proxy_result_is_success_error(void) {
    ChatProxyResult result;
    memset(&result, 0, sizeof(result));
    result.code = CHAT_PROXY_ERROR_HTTP_5XX;
    TEST_ASSERT_FALSE(chat_proxy_result_is_success(&result));
}

void test_proxy_result_should_retry_null(void) {
    TEST_ASSERT_FALSE(chat_proxy_result_should_retry(NULL));
}

void test_proxy_result_should_retry_5xx(void) {
    ChatProxyResult result;
    memset(&result, 0, sizeof(result));
    result.code = CHAT_PROXY_ERROR_HTTP_5XX;
    result.http_status = 503;
    TEST_ASSERT_TRUE(chat_proxy_result_should_retry(&result));
}

void test_proxy_result_should_retry_4xx(void) {
    ChatProxyResult result;
    memset(&result, 0, sizeof(result));
    result.code = CHAT_PROXY_ERROR_HTTP_4XX;
    TEST_ASSERT_FALSE(chat_proxy_result_should_retry(&result));
}

void test_proxy_result_should_retry_ok(void) {
    ChatProxyResult result;
    memset(&result, 0, sizeof(result));
    result.code = CHAT_PROXY_OK;
    TEST_ASSERT_FALSE(chat_proxy_result_should_retry(&result));
}

void test_proxy_result_create_destroy(void) {
    ChatProxyResult *result = chat_proxy_result_create();
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(CHAT_PROXY_ERROR_UNKNOWN, result->code);
    TEST_ASSERT_NULL(result->response_body);
    TEST_ASSERT_NULL(result->error_message);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, result->total_time_ms);
    chat_proxy_result_destroy(result);
}

void test_proxy_result_destroy_null(void) {
    chat_proxy_result_destroy(NULL);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_chunk_queue_init);
    RUN_TEST(test_chunk_queue_enqueue_dequeue);
    RUN_TEST(test_chunk_queue_has_data_empty);
    RUN_TEST(test_chunk_queue_has_data_full);
    RUN_TEST(test_chunk_queue_get_count_empty);
    RUN_TEST(test_chunk_queue_get_count_after_enqueue);
    RUN_TEST(test_chunk_queue_dequeue_empty);
    RUN_TEST(test_chunk_queue_destroy);
    RUN_TEST(test_chunk_queue_multiple_enqueue_dequeue);
    RUN_TEST(test_multi_result_create_null);
    RUN_TEST(test_multi_result_create_valid);
    RUN_TEST(test_multi_result_destroy_null);
    RUN_TEST(test_proxy_get_default_config);
    RUN_TEST(test_proxy_get_streaming_config);
    RUN_TEST(test_proxy_result_is_success_null);
    RUN_TEST(test_proxy_result_is_success_ok);
    RUN_TEST(test_proxy_result_is_success_error);
    RUN_TEST(test_proxy_result_should_retry_null);
    RUN_TEST(test_proxy_result_should_retry_5xx);
    RUN_TEST(test_proxy_result_should_retry_4xx);
    RUN_TEST(test_proxy_result_should_retry_ok);
    RUN_TEST(test_proxy_result_create_destroy);
    RUN_TEST(test_proxy_result_destroy_null);
    return UNITY_END();
}
