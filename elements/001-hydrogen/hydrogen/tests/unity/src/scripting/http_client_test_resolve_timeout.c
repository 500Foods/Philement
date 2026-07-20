/*
 * Unity Test File: http_client_test_resolve_timeout.c
 *
 * Tests resolve_timeout in src/scripting/http_client.c. It maps a
 * caller-supplied timeout to an effective seconds value: a positive
 * caller value wins; otherwise the config default
 * (app_config->scripting.DefaultHTTPTimeout) is used, falling back to a
 * hard-coded 30 s when config is absent or zero.
 *
 * No network activity. Pure logic over the global app_config.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <string.h>

#include <src/scripting/http_client.h>

// Mock app_config: zeroed, scripting disabled by default.
static AppConfig mock_app_config_storage = {0};

// Forward declarations
void test_resolve_timeout_positive_request_wins(void);
void test_resolve_timeout_zero_request_uses_hardcoded_default(void);
void test_resolve_timeout_negative_request_uses_hardcoded_default(void);
void test_resolve_timeout_config_default_used_when_present(void);
void test_resolve_timeout_null_app_config_uses_hardcoded_default(void);

void setUp(void) {
    memset(&mock_app_config_storage, 0, sizeof(mock_app_config_storage));
    app_config = &mock_app_config_storage;
}

void tearDown(void) {
    app_config = NULL;
}

// A positive caller value is returned verbatim (line 131).
void test_resolve_timeout_positive_request_wins(void) {
    TEST_ASSERT_EQUAL(42, resolve_timeout(42));
    TEST_ASSERT_EQUAL(1, resolve_timeout(1));
    TEST_ASSERT_EQUAL(120, resolve_timeout(120));
}

// A zero request with config present but zeroed falls back to 30.
void test_resolve_timeout_zero_request_uses_hardcoded_default(void) {
    mock_app_config_storage.scripting.DefaultHTTPTimeout = 0;
    TEST_ASSERT_EQUAL(30L, resolve_timeout(0));
}

// A negative request also falls back to 30.
void test_resolve_timeout_negative_request_uses_hardcoded_default(void) {
    mock_app_config_storage.scripting.DefaultHTTPTimeout = 0;
    TEST_ASSERT_EQUAL(30L, resolve_timeout(-5));
}

// A non-positive request uses the configured default when it is > 0.
void test_resolve_timeout_config_default_used_when_present(void) {
    mock_app_config_storage.scripting.DefaultHTTPTimeout = 15;
    TEST_ASSERT_EQUAL(15L, resolve_timeout(0));
    TEST_ASSERT_EQUAL(15L, resolve_timeout(-1));
}

// Without any app_config, the hard-coded 30 s floor applies.
void test_resolve_timeout_null_app_config_uses_hardcoded_default(void) {
    app_config = NULL;
    TEST_ASSERT_EQUAL(30L, resolve_timeout(0));
    TEST_ASSERT_EQUAL(30L, resolve_timeout(-10));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_resolve_timeout_positive_request_wins);
    RUN_TEST(test_resolve_timeout_zero_request_uses_hardcoded_default);
    RUN_TEST(test_resolve_timeout_negative_request_uses_hardcoded_default);
    RUN_TEST(test_resolve_timeout_config_default_used_when_present);
    RUN_TEST(test_resolve_timeout_null_app_config_uses_hardcoded_default);

    return UNITY_END();
}
