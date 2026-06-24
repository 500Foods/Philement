/*
 * Config WebServer ChaCha Test
 *
 * Tests ChaCha* config loading defaults, literal values, and env var fallback.
 */

#include <src/hydrogen.h>
#include <unity.h>
#include <src/config/config_webserver.h>
#include <src/config/config.h>

// Forward declarations for functions being tested
bool load_webserver_config(json_t* root, AppConfig* config);
void cleanup_webserver_config(WebServerConfig* config);

// Forward declarations for test functions
void test_webserver_chacha_defaults(void);
void test_webserver_chacha_full_load(void);
void test_webserver_chacha_env_fallback(void);

// Test setup and teardown
void setUp(void) {
    // Reset any global state if needed
}

void tearDown(void) {
    // Ensure test env vars do not leak to other tests
    unsetenv("CHACHA_SERVER");
    unsetenv("CHACHA_SITEID");
    unsetenv("CHACHA_SECRET");
}

// Test that defaults are set to env ref strings when env vars are not present
void test_webserver_chacha_defaults(void) {
    // Ensure the env vars are not resolved for this test
    unsetenv("CHACHA_SERVER");
    unsetenv("CHACHA_SITEID");
    unsetenv("CHACHA_SECRET");

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

// Test that literal values from JSON are loaded correctly
void test_webserver_chacha_full_load(void) {
    AppConfig config;
    memset(&config, 0, sizeof(AppConfig));

    json_t* root = json_object();
    json_t* webserver_section = json_object();

    json_object_set(webserver_section, "ChaChaServer", json_string("https://chacha.example.com"));
    json_object_set(webserver_section, "ChaChaSiteID", json_string("site-123"));
    json_object_set(webserver_section, "ChaChaSecret", json_string("secret-456"));

    json_object_set(root, "WebServer", webserver_section);

    bool result = load_webserver_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(config.webserver.chacha_server);
    TEST_ASSERT_EQUAL_STRING("https://chacha.example.com", config.webserver.chacha_server);
    TEST_ASSERT_NOT_NULL(config.webserver.chacha_site_id);
    TEST_ASSERT_EQUAL_STRING("site-123", config.webserver.chacha_site_id);
    TEST_ASSERT_NOT_NULL(config.webserver.chacha_secret);
    TEST_ASSERT_EQUAL_STRING("secret-456", config.webserver.chacha_secret);

    json_decref(root);
    cleanup_webserver_config(&config.webserver);
}

// Test that ${env.CHACHA_*} references resolve when env vars are set
void test_webserver_chacha_env_fallback(void) {
    setenv("CHACHA_SERVER", "https://chacha.env.example.com", 1);
    setenv("CHACHA_SITEID", "env-site-789", 1);
    setenv("CHACHA_SECRET", "env-secret-abc", 1);

    AppConfig config;
    memset(&config, 0, sizeof(AppConfig));

    // NULL root forces default env-ref values to be resolved
    bool result = load_webserver_config(NULL, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(config.webserver.chacha_server);
    TEST_ASSERT_EQUAL_STRING("https://chacha.env.example.com", config.webserver.chacha_server);
    TEST_ASSERT_NOT_NULL(config.webserver.chacha_site_id);
    TEST_ASSERT_EQUAL_STRING("env-site-789", config.webserver.chacha_site_id);
    TEST_ASSERT_NOT_NULL(config.webserver.chacha_secret);
    TEST_ASSERT_EQUAL_STRING("env-secret-abc", config.webserver.chacha_secret);

    cleanup_webserver_config(&config.webserver);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_webserver_chacha_defaults);
    RUN_TEST(test_webserver_chacha_full_load);
    RUN_TEST(test_webserver_chacha_env_fallback);
    return UNITY_END();
}
