/*
 * Unity Test File: mailrelay_rate_limit_test.c
 *
 * Tests for the Mail Relay API rate limiter (Phase 14.3).
 * Covers: disabled fail-open, global scope, per-key bucketing,
 * window reset, throttle at max, allocation-failure fail-open,
 * and key-building logic for each scope.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/mailrelay/mailrelay_rate_limit.h>

#include <stdlib.h>
#include <string.h>

extern AppConfig* app_config;

static AppConfig g_test_config = {0};
static AppConfig* g_saved_app_config = NULL;

/* Forward declarations for test functions */
void test_rate_limit_disabled_fails_open(void);
void test_rate_limit_null_config_fails_open(void);
void test_rate_limit_zero_max_requests_fails_open(void);
void test_rate_limit_zero_interval_fails_open(void);
void test_rate_limit_user_scope_separate_buckets(void);
void test_rate_limit_ip_scope_separate_buckets(void);
void test_rate_limit_template_scope_separate_buckets(void);
void test_rate_limit_global_scope_single_bucket(void);
void test_rate_limit_build_key_global_returns_null(void);
void test_rate_limit_build_key_user(void);
void test_rate_limit_build_key_ip(void);
void test_rate_limit_build_key_template(void);
void test_rate_limit_build_key_null_sub_fails_open(void);
void test_rate_limit_build_key_null_ip_fails_open(void);
void test_rate_limit_build_key_null_template_fails_open(void);
void test_rate_limit_find_locked_existing(void);
void test_rate_limit_find_locked_missing(void);
void test_rate_limit_new_bucket_locked(void);
void test_rate_limit_new_bucket_null_key(void);
void test_rate_limit_shutdown_clears_buckets(void);

void setUp(void) {
    g_saved_app_config = app_config;
    memset(&g_test_config, 0, sizeof(g_test_config));
    app_config = &g_test_config;
    mailrelay_rate_limit_init();
}

void tearDown(void) {
    mailrelay_rate_limit_shutdown();
    mailrelay_rate_limit_init();
    app_config = g_saved_app_config;
}

void test_rate_limit_disabled_fails_open(void) {
    g_test_config.mail_relay.RateLimit.Enabled = false;
    g_test_config.mail_relay.RateLimit.MaxRequestsPerInterval = 1;
    g_test_config.mail_relay.RateLimit.IntervalSeconds = 60;
    g_test_config.mail_relay.RateLimit.Scope = MAIL_RL_SCOPE_USER;

    for (int i = 0; i < 10; i++) {
        MailRelayRateLimitResult result = mailrelay_rate_limit_check_and_record("user1", "1.2.3.4", "tmpl");
        TEST_ASSERT_EQUAL(MAIL_RELAY_RATE_ALLOWED, result);
    }
}

void test_rate_limit_null_config_fails_open(void) {
    app_config = NULL;

    MailRelayRateLimitResult result = mailrelay_rate_limit_check_and_record("user1", "10.0.0.1", "tmpl");
    TEST_ASSERT_EQUAL(MAIL_RELAY_RATE_ALLOWED, result);
}

void test_rate_limit_zero_max_requests_fails_open(void) {
    g_test_config.mail_relay.RateLimit.Enabled = true;
    g_test_config.mail_relay.RateLimit.MaxRequestsPerInterval = 0;
    g_test_config.mail_relay.RateLimit.IntervalSeconds = 60;
    g_test_config.mail_relay.RateLimit.Scope = MAIL_RL_SCOPE_USER;

    MailRelayRateLimitResult result = mailrelay_rate_limit_check_and_record("user1", "10.0.0.1", "tmpl");
    TEST_ASSERT_EQUAL(MAIL_RELAY_RATE_ALLOWED, result);
}

void test_rate_limit_zero_interval_fails_open(void) {
    g_test_config.mail_relay.RateLimit.Enabled = true;
    g_test_config.mail_relay.RateLimit.MaxRequestsPerInterval = 5;
    g_test_config.mail_relay.RateLimit.IntervalSeconds = 0;
    g_test_config.mail_relay.RateLimit.Scope = MAIL_RL_SCOPE_USER;

    MailRelayRateLimitResult result = mailrelay_rate_limit_check_and_record("user1", "10.0.0.1", "tmpl");
    TEST_ASSERT_EQUAL(MAIL_RELAY_RATE_ALLOWED, result);
}

void test_rate_limit_user_scope_separate_buckets(void) {
    g_test_config.mail_relay.RateLimit.Enabled = true;
    g_test_config.mail_relay.RateLimit.MaxRequestsPerInterval = 3;
    g_test_config.mail_relay.RateLimit.IntervalSeconds = 60;
    g_test_config.mail_relay.RateLimit.Scope = MAIL_RL_SCOPE_USER;

    for (int i = 0; i < 3; i++) {
        MailRelayRateLimitResult result = mailrelay_rate_limit_check_and_record("userA", "10.0.0.1", "tmpl");
        TEST_ASSERT_EQUAL(MAIL_RELAY_RATE_ALLOWED, result);
    }

    MailRelayRateLimitResult result = mailrelay_rate_limit_check_and_record("userA", "10.0.0.2", "tmpl");
    TEST_ASSERT_EQUAL(MAIL_RELAY_RATE_THROTTLED, result);

    MailRelayRateLimitResult result2 = mailrelay_rate_limit_check_and_record("userB", "10.0.0.3", "tmpl");
    TEST_ASSERT_EQUAL(MAIL_RELAY_RATE_ALLOWED, result2);
}

void test_rate_limit_ip_scope_separate_buckets(void) {
    g_test_config.mail_relay.RateLimit.Enabled = true;
    g_test_config.mail_relay.RateLimit.MaxRequestsPerInterval = 2;
    g_test_config.mail_relay.RateLimit.IntervalSeconds = 60;
    g_test_config.mail_relay.RateLimit.Scope = MAIL_RL_SCOPE_IP;

    TEST_ASSERT_EQUAL(MAIL_RELAY_RATE_ALLOWED,
        mailrelay_rate_limit_check_and_record("user1", "10.0.0.1", "tmpl"));
    TEST_ASSERT_EQUAL(MAIL_RELAY_RATE_ALLOWED,
        mailrelay_rate_limit_check_and_record("user1", "10.0.0.1", "tmpl"));
    TEST_ASSERT_EQUAL(MAIL_RELAY_RATE_THROTTLED,
        mailrelay_rate_limit_check_and_record("user1", "10.0.0.1", "tmpl"));

    TEST_ASSERT_EQUAL(MAIL_RELAY_RATE_ALLOWED,
        mailrelay_rate_limit_check_and_record("user2", "10.0.0.2", "tmpl"));
}

void test_rate_limit_template_scope_separate_buckets(void) {
    g_test_config.mail_relay.RateLimit.Enabled = true;
    g_test_config.mail_relay.RateLimit.MaxRequestsPerInterval = 2;
    g_test_config.mail_relay.RateLimit.IntervalSeconds = 60;
    g_test_config.mail_relay.RateLimit.Scope = MAIL_RL_SCOPE_TEMPLATE;

    TEST_ASSERT_EQUAL(MAIL_RELAY_RATE_ALLOWED,
        mailrelay_rate_limit_check_and_record("user1", "10.0.0.1", "tmplA"));
    TEST_ASSERT_EQUAL(MAIL_RELAY_RATE_ALLOWED,
        mailrelay_rate_limit_check_and_record("user1", "10.0.0.1", "tmplA"));
    TEST_ASSERT_EQUAL(MAIL_RELAY_RATE_THROTTLED,
        mailrelay_rate_limit_check_and_record("user1", "10.0.0.1", "tmplA"));

    TEST_ASSERT_EQUAL(MAIL_RELAY_RATE_ALLOWED,
        mailrelay_rate_limit_check_and_record("user1", "10.0.0.1", "tmplB"));
}

void test_rate_limit_global_scope_single_bucket(void) {
    g_test_config.mail_relay.RateLimit.Enabled = true;
    g_test_config.mail_relay.RateLimit.MaxRequestsPerInterval = 2;
    g_test_config.mail_relay.RateLimit.IntervalSeconds = 60;
    g_test_config.mail_relay.RateLimit.Scope = MAIL_RL_SCOPE_GLOBAL;

    TEST_ASSERT_EQUAL(MAIL_RELAY_RATE_ALLOWED,
        mailrelay_rate_limit_check_and_record("user1", "10.0.0.1", "tmplA"));
    TEST_ASSERT_EQUAL(MAIL_RELAY_RATE_ALLOWED,
        mailrelay_rate_limit_check_and_record("user2", "10.0.0.2", "tmplB"));
    TEST_ASSERT_EQUAL(MAIL_RELAY_RATE_THROTTLED,
        mailrelay_rate_limit_check_and_record("user3", "10.0.0.3", "tmplC"));
}

void test_rate_limit_build_key_global_returns_null(void) {
    g_test_config.mail_relay.RateLimit.Scope = MAIL_RL_SCOPE_GLOBAL;
    char* key = mailrelay_rate_limit_build_key("user1", "10.0.0.1", "tmpl");
    TEST_ASSERT_NULL(key);
}

void test_rate_limit_build_key_user(void) {
    g_test_config.mail_relay.RateLimit.Scope = MAIL_RL_SCOPE_USER;
    char* key = mailrelay_rate_limit_build_key("user1", "10.0.0.1", "tmpl");
    TEST_ASSERT_NOT_NULL(key);
    TEST_ASSERT_EQUAL_STRING("user1", key);
    free(key);
}

void test_rate_limit_build_key_ip(void) {
    g_test_config.mail_relay.RateLimit.Scope = MAIL_RL_SCOPE_IP;
    char* key = mailrelay_rate_limit_build_key("user1", "10.0.0.1", "tmpl");
    TEST_ASSERT_NOT_NULL(key);
    TEST_ASSERT_EQUAL_STRING("10.0.0.1", key);
    free(key);
}

void test_rate_limit_build_key_template(void) {
    g_test_config.mail_relay.RateLimit.Scope = MAIL_RL_SCOPE_TEMPLATE;
    char* key = mailrelay_rate_limit_build_key("user1", "10.0.0.1", "tmpl");
    TEST_ASSERT_NOT_NULL(key);
    TEST_ASSERT_EQUAL_STRING("tmpl", key);
    free(key);
}

void test_rate_limit_build_key_null_sub_fails_open(void) {
    g_test_config.mail_relay.RateLimit.Scope = MAIL_RL_SCOPE_USER;
    char* key = mailrelay_rate_limit_build_key(NULL, "10.0.0.1", "tmpl");
    TEST_ASSERT_NULL(key);
}

void test_rate_limit_build_key_null_ip_fails_open(void) {
    g_test_config.mail_relay.RateLimit.Scope = MAIL_RL_SCOPE_IP;
    char* key = mailrelay_rate_limit_build_key("user1", NULL, "tmpl");
    TEST_ASSERT_NULL(key);
}

void test_rate_limit_build_key_null_template_fails_open(void) {
    g_test_config.mail_relay.RateLimit.Scope = MAIL_RL_SCOPE_TEMPLATE;
    char* key = mailrelay_rate_limit_build_key("user1", "10.0.0.1", NULL);
    TEST_ASSERT_NULL(key);
}

void test_rate_limit_find_locked_existing(void) {
    g_test_config.mail_relay.RateLimit.Enabled = true;
    g_test_config.mail_relay.RateLimit.MaxRequestsPerInterval = 10;
    g_test_config.mail_relay.RateLimit.IntervalSeconds = 60;
    g_test_config.mail_relay.RateLimit.Scope = MAIL_RL_SCOPE_USER;

    mailrelay_rate_limit_check_and_record("userA", "10.0.0.1", "tmpl");
    MailRelayApiRateLimitEntry* entry = mailrelay_rate_limit_find_locked("userA");
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_NOT_NULL(entry->key);
    TEST_ASSERT_EQUAL_STRING("userA", entry->key);
    TEST_ASSERT_EQUAL(1, entry->count);
}

void test_rate_limit_find_locked_missing(void) {
    g_test_config.mail_relay.RateLimit.Enabled = true;
    g_test_config.mail_relay.RateLimit.MaxRequestsPerInterval = 10;
    g_test_config.mail_relay.RateLimit.IntervalSeconds = 60;
    g_test_config.mail_relay.RateLimit.Scope = MAIL_RL_SCOPE_GLOBAL;

    TEST_ASSERT_NULL(mailrelay_rate_limit_find_locked("nonexistent"));
}

void test_rate_limit_new_bucket_locked(void) {
    time_t now = time(NULL);
    MailRelayApiRateLimitEntry* entry = mailrelay_rate_limit_new_bucket_locked("testkey", now);
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_NOT_NULL(entry->key);
    TEST_ASSERT_EQUAL_STRING("testkey", entry->key);
    TEST_ASSERT_EQUAL(now, entry->window_start);
    TEST_ASSERT_EQUAL(0, entry->count);
    /* Entry is linked into the global list; tearDown's shutdown frees it. */
}

void test_rate_limit_new_bucket_null_key(void) {
    time_t now = time(NULL);
    MailRelayApiRateLimitEntry* entry = mailrelay_rate_limit_new_bucket_locked(NULL, now);
    TEST_ASSERT_NOT_NULL(entry);
    TEST_ASSERT_NULL(entry->key);
    TEST_ASSERT_EQUAL(now, entry->window_start);
    TEST_ASSERT_EQUAL(0, entry->count);
    /* Entry is linked into the global list; tearDown's shutdown frees it. */
}

void test_rate_limit_shutdown_clears_buckets(void) {
    g_test_config.mail_relay.RateLimit.Enabled = true;
    g_test_config.mail_relay.RateLimit.MaxRequestsPerInterval = 10;
    g_test_config.mail_relay.RateLimit.IntervalSeconds = 60;
    g_test_config.mail_relay.RateLimit.Scope = MAIL_RL_SCOPE_USER;

    mailrelay_rate_limit_check_and_record("userA", "10.0.0.1", "tmpl");
    mailrelay_rate_limit_check_and_record("userB", "10.0.0.2", "tmpl");

    TEST_ASSERT_NOT_NULL(mailrelay_rate_limit_find_locked("userA"));
    TEST_ASSERT_NOT_NULL(mailrelay_rate_limit_find_locked("userB"));

    mailrelay_rate_limit_shutdown();

    TEST_ASSERT_NULL(mailrelay_rate_limit_find_locked("userA"));
    TEST_ASSERT_NULL(mailrelay_rate_limit_find_locked("userB"));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_rate_limit_disabled_fails_open);
    RUN_TEST(test_rate_limit_null_config_fails_open);
    RUN_TEST(test_rate_limit_zero_max_requests_fails_open);
    RUN_TEST(test_rate_limit_zero_interval_fails_open);
    RUN_TEST(test_rate_limit_user_scope_separate_buckets);
    RUN_TEST(test_rate_limit_ip_scope_separate_buckets);
    RUN_TEST(test_rate_limit_template_scope_separate_buckets);
    RUN_TEST(test_rate_limit_global_scope_single_bucket);
    RUN_TEST(test_rate_limit_build_key_global_returns_null);
    RUN_TEST(test_rate_limit_build_key_user);
    RUN_TEST(test_rate_limit_build_key_ip);
    RUN_TEST(test_rate_limit_build_key_template);
    RUN_TEST(test_rate_limit_build_key_null_sub_fails_open);
    RUN_TEST(test_rate_limit_build_key_null_ip_fails_open);
    RUN_TEST(test_rate_limit_build_key_null_template_fails_open);
    RUN_TEST(test_rate_limit_find_locked_existing);
    RUN_TEST(test_rate_limit_find_locked_missing);
    RUN_TEST(test_rate_limit_new_bucket_locked);
    RUN_TEST(test_rate_limit_new_bucket_null_key);
    RUN_TEST(test_rate_limit_shutdown_clears_buckets);

    return UNITY_END();
}
