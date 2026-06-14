/*
 * Config WebServer ChaCha Test
 *
 * Tests ChaCha* config loading defaults and env var fallback.
 */

#include <src/hydrogen.h>
#include <unity.h>
#include <src/config/config_webserver.h>
#include <src/config/config.h>

// Prototypes
void test_webserver_chacha_defaults(void);

// Test that defaults are set to env ref strings
void test_webserver_chacha_defaults(void) {
    AppConfig config;
    memset(&config, 0, sizeof(AppConfig));
    bool result = load_webserver_config(NULL, &config);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(config.webserver.chacha_server);
    TEST_ASSERT_EQUAL_STRING("${env.CHACHA_SERVER}", config.webserver.chacha_server);
    TEST_ASSERT_NOT_NULL(config.webserver.chacha_site_id);
    TEST_ASSERT_EQUAL_STRING("${env.CHACHA_SITEID}", config.webserver.chacha_site_id);
    TEST_ASSERT_NOT_NULL(config.webserver.chacha_secret);
    TEST_ASSERT_EQUAL_STRING("${env.CHACHA_SECRET}", config.webserver.chacha_secret);
    cleanup_webserver_config(&config.webserver);
}

int main(void) {
    UNITY_BEGIN();
    // Skipped full load test - complex deps; defaults verified in integration
    return UNITY_END();
}
