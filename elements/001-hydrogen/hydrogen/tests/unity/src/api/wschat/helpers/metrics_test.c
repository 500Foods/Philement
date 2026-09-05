#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/wschat/helpers/metrics.h>

void test_metrics_get_entry_null(void);
void test_metrics_get_entry_create(void);
void test_metrics_get_entry_lookup_existing(void);
void test_metrics_response_time(void);
void test_metrics_conversation(void);
void test_metrics_tokens_prompt(void);
void test_metrics_tokens_completion(void);
void test_metrics_tokens_unknown(void);
void test_metrics_error(void);
void test_metrics_generate_prometheus_empty(void);
void test_metrics_generate_prometheus_with_data(void);
void test_metrics_write_metric_normal(void);
void test_metrics_write_metric_buffer_full(void);
void test_metrics_write_metric_offset_beyond_buffer(void);
void test_metrics_get_stats(void);

void setUp(void) {}
void tearDown(void) {}

void test_metrics_get_entry_null(void) {
    TEST_ASSERT_NULL(chat_metrics_get_metric_entry(NULL, "engine"));
    TEST_ASSERT_NULL(chat_metrics_get_metric_entry("db", NULL));
}

void test_metrics_get_entry_create(void) {
    ChatMetricEntry *entry = chat_metrics_get_metric_entry("testdb1", "gpt4");
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_EQUAL_STRING("testdb1", entry->database);
    TEST_ASSERT_EQUAL_STRING("gpt4", entry->engine);
    TEST_ASSERT_EQUAL_DOUBLE(1.0, entry->health);
    TEST_ASSERT_EQUAL_UINT64(0, entry->conversations_total);
}

void test_metrics_get_entry_lookup_existing(void) {
    ChatMetricEntry *entry1 = chat_metrics_get_metric_entry("testdb2", "claude");
    TEST_ASSERT_NOT_NULL(entry1);
    ChatMetricEntry *entry2 = chat_metrics_get_metric_entry("testdb2", "claude");
    TEST_ASSERT_NOT_NULL(entry2);
    TEST_ASSERT_EQUAL_PTR(entry1, entry2);
}

void test_metrics_response_time(void) {
    chat_metrics_response_time("testdb3", "gpt4", 125.5);
    ChatMetricEntry *entry = chat_metrics_get_metric_entry("testdb3", "gpt4");
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_EQUAL_DOUBLE(125.5, entry->response_time_ms);
}

void test_metrics_conversation(void) {
    chat_metrics_conversation("testdb4", "gpt4");
    chat_metrics_conversation("testdb4", "gpt4");
    ChatMetricEntry *entry = chat_metrics_get_metric_entry("testdb4", "gpt4");
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_EQUAL_UINT64(2, entry->conversations_total);
}

void test_metrics_tokens_prompt(void) {
    chat_metrics_tokens("testdb5", "gpt4", "prompt", 100);
    chat_metrics_tokens("testdb5", "gpt4", "prompt", 50);
    ChatMetricEntry *entry = chat_metrics_get_metric_entry("testdb5", "gpt4");
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_EQUAL_UINT64(150, entry->tokens_prompt_total);
}

void test_metrics_tokens_completion(void) {
    chat_metrics_tokens("testdb6", "gpt4", "completion", 200);
    ChatMetricEntry *entry = chat_metrics_get_metric_entry("testdb6", "gpt4");
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_EQUAL_UINT64(200, entry->tokens_completion_total);
}

void test_metrics_tokens_unknown(void) {
    chat_metrics_tokens("testdb7", "gpt4", "unknown", 75);
    ChatMetricEntry *entry = chat_metrics_get_metric_entry("testdb7", "gpt4");
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_EQUAL_UINT64(75, entry->tokens_prompt_total);
}

void test_metrics_error(void) {
    chat_metrics_error("testdb8", "gpt4", "timeout");
    chat_metrics_error("testdb8", "gpt4", "http");
    ChatMetricEntry *entry = chat_metrics_get_metric_entry("testdb8", "gpt4");
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_EQUAL_UINT64(2, entry->errors_total);
}

void test_metrics_generate_prometheus_empty(void) {
    char buffer[65536];
    size_t written = chat_metrics_generate_prometheus(buffer, sizeof(buffer));
    TEST_ASSERT(written > 0);
    TEST_ASSERT(strstr(buffer, "# TYPE hydrogen_chat_engine_health gauge") != NULL);
    TEST_ASSERT(strstr(buffer, "# TYPE hydrogen_chat_conversations_total counter") != NULL);
    TEST_ASSERT(strstr(buffer, "# TYPE hydrogen_chat_tokens_total counter") != NULL);
    TEST_ASSERT(strstr(buffer, "# TYPE hydrogen_chat_errors_total counter") != NULL);
    TEST_ASSERT(strstr(buffer, "# TYPE hydrogen_chat_request_duration_seconds histogram") != NULL);
}

void test_metrics_generate_prometheus_with_data(void) {
    chat_metrics_response_time("testdb9", "gpt4", 42.0);
    chat_metrics_conversation("testdb9", "gpt4");

    char buffer[8192];
    size_t written = chat_metrics_generate_prometheus(buffer, sizeof(buffer));
    TEST_ASSERT(written > 0);
    TEST_ASSERT(strstr(buffer, "database=\"testdb9\"") != NULL);
    TEST_ASSERT(strstr(buffer, "engine=\"gpt4\"") != NULL);
    TEST_ASSERT(strstr(buffer, "1.000") != NULL || strstr(buffer, "1.00") != NULL);
}

void test_metrics_write_metric_normal(void) {
    char buffer[256];
    size_t result = chat_metrics_write_metric(buffer, 0, sizeof(buffer),
                                               "test_metric", "label=\"value\"", 42.5);
    TEST_ASSERT(result > 0);
    TEST_ASSERT(strstr(buffer, "test_metric{label=\"value\"} 42.500") != NULL);
}

void test_metrics_write_metric_buffer_full(void) {
    char buffer[16];
    size_t result = chat_metrics_write_metric(buffer, 0, sizeof(buffer),
                                               "test_metric", "label=\"value\"", 42.5);
    TEST_ASSERT_EQUAL_size_t(sizeof(buffer), result);
}

void test_metrics_write_metric_offset_beyond_buffer(void) {
    char buffer[256];
    size_t result = chat_metrics_write_metric(buffer, 256, sizeof(buffer),
                                               "test_metric", "label=\"value\"", 42.5);
    TEST_ASSERT_EQUAL_size_t(256, result);
}

void test_metrics_get_stats(void) {
    chat_metrics_response_time("testdb_stats", "gpt4", 100.0);
    chat_metrics_conversation("testdb_stats", "gpt4");
    ChatMetricEntry *entry = chat_metrics_get_metric_entry("testdb_stats", "gpt4");
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_EQUAL_UINT64(1, entry->conversations_total);
    TEST_ASSERT_EQUAL_DOUBLE(100.0, entry->response_time_ms);
    TEST_ASSERT_TRUE(chat_metrics_get_metric_entry(NULL, "gpt4") == NULL);
    TEST_ASSERT_TRUE(chat_metrics_get_metric_entry("testdb_stats", NULL) == NULL);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_metrics_get_entry_null);
    RUN_TEST(test_metrics_get_entry_create);
    RUN_TEST(test_metrics_get_entry_lookup_existing);
    RUN_TEST(test_metrics_response_time);
    RUN_TEST(test_metrics_conversation);
    RUN_TEST(test_metrics_tokens_prompt);
    RUN_TEST(test_metrics_tokens_completion);
    RUN_TEST(test_metrics_tokens_unknown);
    RUN_TEST(test_metrics_error);
    RUN_TEST(test_metrics_generate_prometheus_empty);
    RUN_TEST(test_metrics_generate_prometheus_with_data);
    RUN_TEST(test_metrics_write_metric_normal);
    RUN_TEST(test_metrics_write_metric_buffer_full);
    RUN_TEST(test_metrics_write_metric_offset_beyond_buffer);
    RUN_TEST(test_metrics_get_stats);
    return UNITY_END();
}
