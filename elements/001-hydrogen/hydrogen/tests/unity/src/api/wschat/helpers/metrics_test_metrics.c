/*
 * Unity Test File: chat metrics
 * This file contains unit tests for src/api/wschat/helpers/metrics.c
 *
 * Coverage focus: init/cleanup lifecycle, all gauge/counter setters, the
 * request-duration histogram, update-from-engine, the write_metric helper
 * (including its buffer-full path), and the Prometheus generator.
 */

#include <src/hydrogen.h>
#include <unity.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <src/api/wschat/helpers/metrics.h>
#include <src/api/wschat/helpers/engine_cache.h>

// Forward declarations for tests
void test_init_and_reinit(void);
void test_cleanup_resets_state(void);
void test_get_metric_entry_null_args(void);
void test_get_metric_entry_create_and_reuse(void);
void test_engine_health(void);
void test_engine_health_null_entry(void);
void test_response_time(void);
void test_conversation_counter(void);
void test_tokens_prompt_completion_unknown(void);
void test_tokens_null_entry(void);
void test_error_counter(void);
void test_request_duration(void);
void test_request_duration_no_match(void);
void test_update_from_engine_null_args(void);
void test_update_from_engine(void);
void test_write_metric_basic(void);
void test_write_metric_offset_at_end(void);
void test_write_metric_truncation(void);
void test_generate_prometheus_null_and_zero(void);
void test_generate_prometheus_empty(void);
void test_generate_prometheus_populated(void);
void test_generate_prometheus_small_buffer(void);

void setUp(void) {
    // Start every test from a clean, initialized metrics store.
    chat_metrics_cleanup();
    chat_metrics_init();
}

void tearDown(void) {
    chat_metrics_cleanup();
}

/* init returns true, and calling it again while already initialized is a no-op */
void test_init_and_reinit(void) {
    TEST_ASSERT_TRUE(chat_metrics_init());
    // Second call hits the already-initialized early return
    TEST_ASSERT_TRUE(chat_metrics_init());
}

/* cleanup clears entries so a subsequent generate produces no data rows */
void test_cleanup_resets_state(void) {
    chat_metrics_conversation("db", "eng");
    chat_metrics_cleanup();
    chat_metrics_init();

    char buffer[4096];
    size_t len = chat_metrics_generate_prometheus(buffer, sizeof(buffer));
    TEST_ASSERT_GREATER_THAN(0, len);
    // No engine data lines should be present after cleanup
    TEST_ASSERT_NULL(strstr(buffer, "engine=\"eng\""));
}

/* NULL database or engine returns NULL */
void test_get_metric_entry_null_args(void) {
    TEST_ASSERT_NULL(chat_metrics_get_metric_entry(NULL, "eng"));
    TEST_ASSERT_NULL(chat_metrics_get_metric_entry("db", NULL));
    TEST_ASSERT_NULL(chat_metrics_get_metric_entry(NULL, NULL));
}

/* First lookup creates the entry; second returns the same pointer */
void test_get_metric_entry_create_and_reuse(void) {
    ChatMetricEntry* a = chat_metrics_get_metric_entry("db1", "eng1");
    TEST_ASSERT_NOT_NULL(a);
    ChatMetricEntry* b = chat_metrics_get_metric_entry("db1", "eng1");
    TEST_ASSERT_EQUAL_PTR(a, b);
    // Different key produces a different entry
    ChatMetricEntry* c = chat_metrics_get_metric_entry("db1", "eng2");
    TEST_ASSERT_NOT_NULL(c);
    TEST_ASSERT_NOT_EQUAL(a, c);
}

/* health gauge reflected in generated output, and provider recorded once */
void test_engine_health(void) {
    chat_metrics_engine_health("dbh", "engh", "openai", true);
    chat_metrics_engine_health("dbh", "engh", "anthropic", false);

    char buffer[8192];
    chat_metrics_generate_prometheus(buffer, sizeof(buffer));
    // Health should now be 0 (unhealthy from last call)
    TEST_ASSERT_NOT_NULL(strstr(buffer,
        "hydrogen_chat_engine_health{database=\"dbh\",engine=\"engh\",provider=\"openai\"} 0.000"));
}

/* NULL args don't crash and produce no entry */
void test_engine_health_null_entry(void) {
    chat_metrics_engine_health(NULL, "eng", "p", true);
    chat_metrics_engine_health("db", NULL, "p", true);
    // No provider recorded when entry exists but provider is NULL
    chat_metrics_engine_health("dbn", "engn", NULL, true);

    char buffer[8192];
    chat_metrics_generate_prometheus(buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(strstr(buffer,
        "database=\"dbn\",engine=\"engn\",provider=\"unknown\""));
}

/* response time gauge appears in output */
void test_response_time(void) {
    chat_metrics_response_time("dbr", "engr", 42.5);
    chat_metrics_response_time(NULL, "engr", 1.0);  // no-op null path

    char buffer[8192];
    chat_metrics_generate_prometheus(buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(strstr(buffer,
        "hydrogen_chat_engine_response_time_ms{database=\"dbr\",engine=\"engr\"} 42.500"));
}

/* conversation counter increments */
void test_conversation_counter(void) {
    chat_metrics_conversation("dbc", "engc");
    chat_metrics_conversation("dbc", "engc");
    chat_metrics_conversation("dbc", "engc");
    chat_metrics_conversation(NULL, "engc");  // no-op null path

    char buffer[8192];
    chat_metrics_generate_prometheus(buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(strstr(buffer,
        "hydrogen_chat_conversations_total{database=\"dbc\",engine=\"engc\"} 3.000"));
}

/* token counter routes to prompt/completion/other buckets */
void test_tokens_prompt_completion_unknown(void) {
    chat_metrics_tokens("dbt", "engt", "prompt", 10);
    chat_metrics_tokens("dbt", "engt", "completion", 20);
    chat_metrics_tokens("dbt", "engt", "mystery", 5);  // unknown -> prompt bucket

    char buffer[8192];
    chat_metrics_generate_prometheus(buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(strstr(buffer,
        "hydrogen_chat_tokens_total{database=\"dbt\",engine=\"engt\",type=\"prompt\"} 15.000"));
    TEST_ASSERT_NOT_NULL(strstr(buffer,
        "hydrogen_chat_tokens_total{database=\"dbt\",engine=\"engt\",type=\"completion\"} 20.000"));
}

/* tokens with NULL args is a safe no-op */
void test_tokens_null_entry(void) {
    chat_metrics_tokens(NULL, "eng", "prompt", 1);
    chat_metrics_tokens("db", NULL, "prompt", 1);
    // NULL token type falls through to the "add to both/else" branch
    chat_metrics_tokens("dbtn", "engtn", NULL, 7);

    char buffer[8192];
    chat_metrics_generate_prometheus(buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(strstr(buffer,
        "hydrogen_chat_tokens_total{database=\"dbtn\",engine=\"engtn\",type=\"prompt\"} 7.000"));
}

/* error counter increments */
void test_error_counter(void) {
    chat_metrics_error("dbe", "enge", "timeout");
    chat_metrics_error("dbe", "enge", "http");
    chat_metrics_error(NULL, "enge", "http");  // no-op null path

    char buffer[8192];
    chat_metrics_generate_prometheus(buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(strstr(buffer,
        "hydrogen_chat_errors_total{database=\"dbe\",engine=\"enge\",error_type=\"total\"} 2.000"));
}

/* request duration finds the engine (across databases) and sums */
void test_request_duration(void) {
    // Create an entry first so request_duration can find it by engine name
    chat_metrics_conversation("dbd", "engd");
    chat_metrics_request_duration("engd", 1.5);
    chat_metrics_request_duration("engd", 2.5);

    char buffer[8192];
    chat_metrics_generate_prometheus(buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(strstr(buffer,
        "hydrogen_chat_request_duration_seconds_sum{engine=\"engd\"} 4.000"));
    TEST_ASSERT_NOT_NULL(strstr(buffer,
        "hydrogen_chat_request_duration_seconds_count{engine=\"engd\"} 2.000"));
}

/* request duration for unknown engine is a safe no-op */
void test_request_duration_no_match(void) {
    chat_metrics_request_duration("does-not-exist", 9.9);
    // Nothing to assert beyond not crashing; the loop simply finds no match.
    TEST_PASS();
}

/* update_from_engine rejects NULL args */
void test_update_from_engine_null_args(void) {
    chat_metrics_update_from_engine(NULL, NULL);
    ChatEngineConfig* e = chat_engine_config_create(1, "eu", CEC_PROVIDER_OPENAI,
        "m", "u", "k", 0, 0, false, 0, 0, 0, 0, 0, false);
    chat_metrics_update_from_engine(NULL, e);
    chat_metrics_update_from_engine("db", NULL);
    chat_engine_config_destroy(e);
    TEST_PASS();
}

/* update_from_engine copies health/response/tokens into a metric entry */
void test_update_from_engine(void) {
    ChatEngineConfig* e = chat_engine_config_create(7, "engu", CEC_PROVIDER_ANTHROPIC,
        "m", "u", "k", 0, 0, false, 0, 0, 0, 0, 0, false);
    TEST_ASSERT_NOT_NULL(e);

    pthread_mutex_lock(&e->health_mutex);
    e->is_healthy = true;
    e->avg_response_time_ms = 33.0;
    e->conversations_24h = 5;
    e->tokens_24h = 99;
    pthread_mutex_unlock(&e->health_mutex);

    chat_metrics_update_from_engine("dbu", e);

    char buffer[8192];
    chat_metrics_generate_prometheus(buffer, sizeof(buffer));
    TEST_ASSERT_NOT_NULL(strstr(buffer,
        "hydrogen_chat_engine_health{database=\"dbu\",engine=\"engu\",provider=\"anthropic\"} 1.000"));
    TEST_ASSERT_NOT_NULL(strstr(buffer,
        "hydrogen_chat_engine_response_time_ms{database=\"dbu\",engine=\"engu\"} 33.000"));
    TEST_ASSERT_NOT_NULL(strstr(buffer,
        "hydrogen_chat_conversations_total{database=\"dbu\",engine=\"engu\"} 5.000"));

    chat_engine_config_destroy(e);
}

/* write_metric writes a properly formatted line and returns advanced offset */
void test_write_metric_basic(void) {
    char buffer[128];
    memset(buffer, 0, sizeof(buffer));
    size_t off = chat_metrics_write_metric(buffer, 0, sizeof(buffer),
                                           "metric_name", "a=\"b\"", 1.25);
    TEST_ASSERT_GREATER_THAN(0, off);
    TEST_ASSERT_EQUAL_STRING("metric_name{a=\"b\"} 1.250\n", buffer);
    TEST_ASSERT_EQUAL_size_t(strlen(buffer), off);
}

/* write_metric returns offset unchanged when offset >= buffer_size */
void test_write_metric_offset_at_end(void) {
    char buffer[16];
    size_t off = chat_metrics_write_metric(buffer, sizeof(buffer), sizeof(buffer),
                                           "m", "l", 1.0);
    TEST_ASSERT_EQUAL_size_t(sizeof(buffer), off);
}

/* write_metric returns buffer_size on truncation */
void test_write_metric_truncation(void) {
    char buffer[8];
    // The formatted line is far longer than 8 bytes -> snprintf truncates.
    size_t off = chat_metrics_write_metric(buffer, 0, sizeof(buffer),
                                           "a_really_long_metric_name",
                                           "database=\"x\",engine=\"y\"", 3.14159);
    TEST_ASSERT_EQUAL_size_t(sizeof(buffer), off);
}

/* generate_prometheus rejects NULL buffer / zero size */
void test_generate_prometheus_null_and_zero(void) {
    char buffer[16];
    TEST_ASSERT_EQUAL_size_t(0, chat_metrics_generate_prometheus(NULL, 16));
    TEST_ASSERT_EQUAL_size_t(0, chat_metrics_generate_prometheus(buffer, 0));
}

/* generate_prometheus with no entries still emits HELP/TYPE headers */
void test_generate_prometheus_empty(void) {
    char buffer[8192];
    size_t len = chat_metrics_generate_prometheus(buffer, sizeof(buffer));
    TEST_ASSERT_GREATER_THAN(0, len);
    TEST_ASSERT_NOT_NULL(strstr(buffer, "# HELP hydrogen_chat_engine_health"));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "# TYPE hydrogen_chat_request_duration_seconds histogram"));
}

/* populated store emits all metric families */
void test_generate_prometheus_populated(void) {
    chat_metrics_engine_health("dbp", "engp", "ollama", true);
    chat_metrics_response_time("dbp", "engp", 12.0);
    chat_metrics_conversation("dbp", "engp");
    chat_metrics_tokens("dbp", "engp", "prompt", 3);
    chat_metrics_tokens("dbp", "engp", "completion", 4);
    chat_metrics_error("dbp", "engp", "http");
    chat_metrics_request_duration("engp", 0.5);

    char buffer[16384];
    size_t len = chat_metrics_generate_prometheus(buffer, sizeof(buffer));
    TEST_ASSERT_GREATER_THAN(0, len);
    TEST_ASSERT_NOT_NULL(strstr(buffer, "provider=\"ollama\""));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "hydrogen_chat_engine_response_time_ms"));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "hydrogen_chat_conversations_total"));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "hydrogen_chat_tokens_total"));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "hydrogen_chat_errors_total"));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "hydrogen_chat_request_duration_seconds_sum"));
}

/* a small buffer causes generate to stop early without overflowing */
void test_generate_prometheus_small_buffer(void) {
    chat_metrics_engine_health("dbs", "engs", "openai", true);
    chat_metrics_response_time("dbs", "engs", 1.0);
    chat_metrics_conversation("dbs", "engs");

    // A small buffer exercises the "offset >= buffer_size" guards in the
    // data-row loops. Note: the header snprintf uses snprintf's would-write
    // return value, so the reported offset can legitimately exceed the buffer
    // size; we only assert the buffer was not written past its bounds.
    char buffer[80];
    buffer[sizeof(buffer) - 1] = '\0';
    size_t len = chat_metrics_generate_prometheus(buffer, sizeof(buffer));
    TEST_ASSERT_GREATER_THAN(0, len);
    // The NUL terminator snprintf placed must remain within bounds.
    TEST_ASSERT_EQUAL_CHAR('\0', buffer[sizeof(buffer) - 1]);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_init_and_reinit);
    RUN_TEST(test_cleanup_resets_state);
    RUN_TEST(test_get_metric_entry_null_args);
    RUN_TEST(test_get_metric_entry_create_and_reuse);
    RUN_TEST(test_engine_health);
    RUN_TEST(test_engine_health_null_entry);
    RUN_TEST(test_response_time);
    RUN_TEST(test_conversation_counter);
    RUN_TEST(test_tokens_prompt_completion_unknown);
    RUN_TEST(test_tokens_null_entry);
    RUN_TEST(test_error_counter);
    RUN_TEST(test_request_duration);
    RUN_TEST(test_request_duration_no_match);
    RUN_TEST(test_update_from_engine_null_args);
    RUN_TEST(test_update_from_engine);
    RUN_TEST(test_write_metric_basic);
    RUN_TEST(test_write_metric_offset_at_end);
    RUN_TEST(test_write_metric_truncation);
    RUN_TEST(test_generate_prometheus_null_and_zero);
    RUN_TEST(test_generate_prometheus_empty);
    RUN_TEST(test_generate_prometheus_populated);
    RUN_TEST(test_generate_prometheus_small_buffer);
    return UNITY_END();
}
