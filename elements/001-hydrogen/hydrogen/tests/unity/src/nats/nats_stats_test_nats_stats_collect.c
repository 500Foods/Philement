/*
 * Unity Test File: nats_stats_collect
 *
 * Counters and the status snapshot. This test does not start the
 * retry thread and does not dial.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/nats/nats.h>
#include <src/nats/nats_internal.h>
#include <src/nats/nats_stats.h>

#include <string.h>

static AppConfig cfg;
static AppConfig *saved_config;

void test_nats_stats_collect_null(void);
void test_nats_stats_collect_reset(void);
void test_nats_stats_collect_increments(void);
void test_nats_stats_collect_snapshot(void);
void test_nats_stats_collect_null_config(void);

void setUp(void) {
    saved_config = app_config;
    memset(&cfg, 0, sizeof(cfg));
    app_config = &cfg;
    nats_link_set(NATS_LINK_DOWN);
    nats_registry_reset();
    nats_stats_reset();
}

void tearDown(void) {
    nats_link_set(NATS_LINK_DOWN);
    nats_registry_reset();
    nats_stats_reset();
    app_config = saved_config;
}

void test_nats_stats_collect_null(void) {
    nats_stats_collect(NULL);
}

void test_nats_stats_collect_reset(void) {
    NatsMetrics metrics;

    nats_stats_inc_published();
    nats_stats_inc_received();
    nats_stats_inc_reconnects();
    nats_stats_reset();
    nats_stats_collect(&metrics);
    TEST_ASSERT_EQUAL_INT(0, (int)metrics.published);
    TEST_ASSERT_EQUAL_INT(0, (int)metrics.received);
    TEST_ASSERT_EQUAL_INT(0, (int)metrics.reconnects);
}

void test_nats_stats_collect_increments(void) {
    NatsMetrics metrics;

    nats_stats_inc_published();
    nats_stats_inc_published();
    nats_stats_inc_received();
    nats_stats_inc_reconnects();
    nats_stats_collect(&metrics);
    TEST_ASSERT_EQUAL_INT(2, (int)metrics.published);
    TEST_ASSERT_EQUAL_INT(1, (int)metrics.received);
    TEST_ASSERT_EQUAL_INT(1, (int)metrics.reconnects);
}

void test_nats_stats_collect_snapshot(void) {
    NatsMetrics metrics;

    cfg.nats.Enabled = true;
    cfg.nats.Presence.Enabled = true;
    nats_link_set(NATS_LINK_DEGRADED);
    nats_stats_collect(&metrics);
    TEST_ASSERT_TRUE(metrics.enabled);
    TEST_ASSERT_TRUE(metrics.presence);
    TEST_ASSERT_FALSE(metrics.singleton);
    TEST_ASSERT_EQUAL_INT(NATS_LINK_DEGRADED, metrics.link);
    TEST_ASSERT_EQUAL_INT(0, metrics.peers);
    TEST_ASSERT_EQUAL_INT(0, metrics.alive);
    TEST_ASSERT_EQUAL_STRING("none", metrics.self);
}

void test_nats_stats_collect_null_config(void) {
    NatsMetrics metrics;

    app_config = NULL;
    nats_stats_collect(&metrics);
    TEST_ASSERT_FALSE(metrics.enabled);
    TEST_ASSERT_FALSE(metrics.presence);
    TEST_ASSERT_EQUAL_STRING("none", metrics.self);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_nats_stats_collect_null);
    RUN_TEST(test_nats_stats_collect_reset);
    RUN_TEST(test_nats_stats_collect_increments);
    RUN_TEST(test_nats_stats_collect_snapshot);
    RUN_TEST(test_nats_stats_collect_null_config);
    return UNITY_END();
}
