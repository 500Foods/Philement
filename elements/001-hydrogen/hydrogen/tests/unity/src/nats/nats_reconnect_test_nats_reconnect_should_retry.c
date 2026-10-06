/*
 * Unity Test File: nats_reconnect_should_retry
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/config/config_nats.h>
#include <src/nats/nats.h>

#include <string.h>

static AppConfig cfg;
static AppConfig *saved_config;

void test_nats_reconnect_should_retry_forever(void);
void test_nats_reconnect_should_retry_once(void);
void test_nats_reconnect_should_retry_without_config(void);

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

void test_nats_reconnect_should_retry_forever(void) {
    cfg.nats.Reconnect.MaxRetries = -1;
    TEST_ASSERT_TRUE(nats_reconnect_should_retry(1));
    TEST_ASSERT_TRUE(nats_reconnect_should_retry(100));
    TEST_ASSERT_FALSE(nats_reconnect_should_retry(0));
}

void test_nats_reconnect_should_retry_once(void) {
    cfg.nats.Reconnect.MaxRetries = 0;
    TEST_ASSERT_FALSE(nats_reconnect_should_retry(1));

    cfg.nats.Reconnect.MaxRetries = 1;
    TEST_ASSERT_TRUE(nats_reconnect_should_retry(1));
    TEST_ASSERT_FALSE(nats_reconnect_should_retry(2));
}

void test_nats_reconnect_should_retry_without_config(void) {
    app_config = NULL;
    TEST_ASSERT_FALSE(nats_reconnect_should_retry(1));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_nats_reconnect_should_retry_forever);
    RUN_TEST(test_nats_reconnect_should_retry_once);
    RUN_TEST(test_nats_reconnect_should_retry_without_config);
    return UNITY_END();
}
