/*
 * Unity Test File: nats_reconnect_delay_seconds
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/config/config_nats.h>
#include <src/nats/nats.h>

#include <string.h>

static AppConfig cfg;
static AppConfig *saved_config;

void test_nats_reconnect_delay_seconds_ladder(void);
void test_nats_reconnect_delay_seconds_without_config(void);

void setUp(void) {
    saved_config = app_config;
    memset(&cfg, 0, sizeof(cfg));
    nats_config_apply_defaults(&cfg.nats);
    app_config = &cfg;
}

void tearDown(void) {
    cleanup_nats_config(&cfg.nats);
    app_config = saved_config;
}

void test_nats_reconnect_delay_seconds_ladder(void) {
    TEST_ASSERT_EQUAL(30, nats_reconnect_delay_seconds(1));
    TEST_ASSERT_EQUAL(60, nats_reconnect_delay_seconds(2));
    TEST_ASSERT_EQUAL(120, nats_reconnect_delay_seconds(3));
    TEST_ASSERT_EQUAL(240, nats_reconnect_delay_seconds(4));
    TEST_ASSERT_EQUAL(480, nats_reconnect_delay_seconds(5));
    TEST_ASSERT_EQUAL(480, nats_reconnect_delay_seconds(6));
}

void test_nats_reconnect_delay_seconds_without_config(void) {
    app_config = NULL;
    TEST_ASSERT_EQUAL(30, nats_reconnect_delay_seconds(1));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_nats_reconnect_delay_seconds_ladder);
    RUN_TEST(test_nats_reconnect_delay_seconds_without_config);
    return UNITY_END();
}
