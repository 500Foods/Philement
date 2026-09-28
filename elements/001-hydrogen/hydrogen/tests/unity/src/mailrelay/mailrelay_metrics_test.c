/*
 * Unity Test File: Mail Relay Prometheus Metrics
 *
 * Verifies mailrelay_metrics_generate_prometheus() produces correct
 * Prometheus-format output for various runtime states.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mailrelay/mailrelay.h>
#include <src/mailrelay/mailrelay_internal.h>
#include <src/mailrelay/mailrelay_metrics.h>

#include <string.h>

/* Forward declarations for test functions */
void test_metrics_not_enabled_emits_enabled_gauge_zero(void);
void test_metrics_initialized_counters(void);
void test_metrics_capacity_query_mode(void);
void test_metrics_format_contains_help_type(void);
void test_metrics_contains_counter_values(void);
void test_metrics_contains_last_success_failure(void);

void setUp(void) {
    mailrelay_shutdown();
}

void tearDown(void) {
    mailrelay_shutdown();
}

void test_metrics_not_enabled_emits_enabled_gauge_zero(void) {
    char buffer[4096];
    size_t len = mailrelay_metrics_generate_prometheus(buffer, sizeof(buffer));
    TEST_ASSERT_GREATER_THAN(0, len);
    if (len < sizeof(buffer)) {
        buffer[len] = '\0';
    } else {
        buffer[sizeof(buffer) - 1] = '\0';
    }

    TEST_ASSERT_NOT_NULL(strstr(buffer, "hydrogen_mailrelay_enabled 0"));
}

void test_metrics_initialized_counters(void) {
    TEST_ASSERT_TRUE(mailrelay_init());

    pthread_mutex_lock(&mailrelay_runtime->mutex);
    mailrelay_runtime->queued_count = 10;
    mailrelay_runtime->sent_count = 7;
    mailrelay_runtime->failed_count = 2;
    mailrelay_runtime->sending_count = 1;
    mailrelay_runtime->retrying_count = 3;
    mailrelay_runtime->permanent_failures_count = 2;
    mailrelay_runtime->last_success_at = 1720612800;
    mailrelay_runtime->last_failure_at = 1720612900;
    pthread_mutex_unlock(&mailrelay_runtime->mutex);

    char buffer[8192];
    size_t len = mailrelay_metrics_generate_prometheus(buffer, sizeof(buffer));
    TEST_ASSERT_GREATER_THAN(0, len);
    if (len < sizeof(buffer)) {
        buffer[len] = '\0';
    } else {
        buffer[sizeof(buffer) - 1] = '\0';
    }

    TEST_ASSERT_NOT_NULL(strstr(buffer, "hydrogen_mailrelay_queued_total 10.000"));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "hydrogen_mailrelay_sent_total 7.000"));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "hydrogen_mailrelay_failed_total 2.000"));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "hydrogen_mailrelay_sending 1.000"));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "hydrogen_mailrelay_retrying 3.000"));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "hydrogen_mailrelay_permanent_failures_total 2.000"));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "hydrogen_mailrelay_last_success 1720612800.000"));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "hydrogen_mailrelay_last_failure 1720612900.000"));
}

void test_metrics_capacity_query_mode(void) {
    size_t needed = mailrelay_metrics_generate_prometheus(NULL, 0);
    TEST_ASSERT_GREATER_THAN(0, needed);
}

void test_metrics_format_contains_help_type(void) {
    char buffer[8192];
    size_t len = mailrelay_metrics_generate_prometheus(buffer, sizeof(buffer));
    TEST_ASSERT_GREATER_THAN(0, len);
    if (len < sizeof(buffer)) {
        buffer[len] = '\0';
    } else {
        buffer[sizeof(buffer) - 1] = '\0';
    }

    char* p = strstr(buffer, "# HELP hydrogen_mailrelay_enabled");
    if (!p) {
        /* Fallback: check by direct byte comparison */
        TEST_ASSERT_EQUAL_MEMORY("# HELP", buffer, 6);
    }
    TEST_ASSERT_NOT_NULL(p);
}

void test_metrics_contains_counter_values(void) {
    TEST_ASSERT_TRUE(mailrelay_init());

    char buffer[8192];
    size_t len = mailrelay_metrics_generate_prometheus(buffer, sizeof(buffer));
    TEST_ASSERT_GREATER_THAN(0, len);
    if (len < sizeof(buffer)) {
        buffer[len] = '\0';
    } else {
        buffer[sizeof(buffer) - 1] = '\0';
    }

    TEST_ASSERT_NOT_NULL(strstr(buffer, "hydrogen_mailrelay_worker_count"));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "hydrogen_mailrelay_queue_depth"));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "hydrogen_mailrelay_sent_total"));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "hydrogen_mailrelay_failed_total"));
}

void test_metrics_contains_last_success_failure(void) {
    TEST_ASSERT_TRUE(mailrelay_init());

    pthread_mutex_lock(&mailrelay_runtime->mutex);
    mailrelay_runtime->last_success_at = 1700000000;
    mailrelay_runtime->last_failure_at = 1700000100;
    pthread_mutex_unlock(&mailrelay_runtime->mutex);

    char buffer[8192];
    size_t len = mailrelay_metrics_generate_prometheus(buffer, sizeof(buffer));
    TEST_ASSERT_GREATER_THAN(0, len);
    if (len < sizeof(buffer)) {
        buffer[len] = '\0';
    } else {
        buffer[sizeof(buffer) - 1] = '\0';
    }

    TEST_ASSERT_NOT_NULL(strstr(buffer, "hydrogen_mailrelay_last_success 1700000000.000"));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "hydrogen_mailrelay_last_failure 1700000100.000"));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_metrics_not_enabled_emits_enabled_gauge_zero);
    RUN_TEST(test_metrics_initialized_counters);
    RUN_TEST(test_metrics_capacity_query_mode);
    RUN_TEST(test_metrics_format_contains_help_type);
    RUN_TEST(test_metrics_contains_counter_values);
    RUN_TEST(test_metrics_contains_last_success_failure);

    return UNITY_END();
}
