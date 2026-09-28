/*
 * Unity Test File: Chat Configuration Tests
 * This file contains unit tests for the chat configuration functions
 * from src/config/config_chat.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/config/config_chat.h>
#include <src/config/config.h>

// Forward declarations for functions being tested
bool load_chat_config(json_t *root, AppConfig *config);
void dump_chat_config(const ChatConfig *config);
void cleanup_chat_config(ChatConfig *config);
void chat_config_apply_defaults(ChatConfig *config);

// Forward declarations for test functions
void test_chat_config_apply_defaults_null_pointer(void);
void test_chat_config_apply_defaults_valid_config(void);
void test_load_chat_config_null_config(void);
void test_load_chat_config_empty_json(void);
void test_load_chat_config_all_fields(void);
void test_load_chat_config_rate_limit_disabled(void);
void test_load_chat_config_invalid_max_requests(void);
void test_load_chat_config_invalid_interval_seconds(void);
void test_load_chat_config_invalid_max_tokens(void);
void test_load_chat_config_multiple_invalid_fields(void);
void test_cleanup_chat_config_null_pointer(void);
void test_cleanup_chat_config_empty_config(void);
void test_cleanup_chat_config_with_data(void);
void test_dump_chat_config_null_pointer(void);
void test_dump_chat_config_basic(void);

// Test setup and teardown
void setUp(void) {
}

void tearDown(void) {
}

// ===== chat_config_apply_defaults TESTS =====

void test_chat_config_apply_defaults_null_pointer(void) {
    // Test with NULL pointer - should handle gracefully
    chat_config_apply_defaults(NULL);
    // No assertions needed - function should not crash
}

void test_chat_config_apply_defaults_valid_config(void) {
    ChatConfig config = {0};

    chat_config_apply_defaults(&config);

    TEST_ASSERT_FALSE(config.RateLimit.Enabled);
    TEST_ASSERT_EQUAL(60, config.RateLimit.MaxRequestsPerInterval);
    TEST_ASSERT_EQUAL(60, config.RateLimit.IntervalSeconds);
    TEST_ASSERT_EQUAL(100000, config.RateLimit.MaxTokensPerInterval);
}

// ===== load_chat_config TESTS =====

void test_load_chat_config_null_config(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();

    // Test with NULL config - should return false
    bool result = load_chat_config(root, NULL);

    TEST_ASSERT_FALSE(result);

    json_decref(root);
}

void test_load_chat_config_empty_json(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();

    bool result = load_chat_config(root, &config);

    TEST_ASSERT_TRUE(result);

    // Defaults should be applied
    TEST_ASSERT_FALSE(config.chat.RateLimit.Enabled);
    TEST_ASSERT_EQUAL(60, config.chat.RateLimit.MaxRequestsPerInterval);
    TEST_ASSERT_EQUAL(60, config.chat.RateLimit.IntervalSeconds);
    TEST_ASSERT_EQUAL(100000, config.chat.RateLimit.MaxTokensPerInterval);

    json_decref(root);
    cleanup_chat_config(&config.chat);
}

void test_load_chat_config_all_fields(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* chat_section = json_object();
    json_t* rate_limit_section = json_object();

    // Set up full chat configuration
    json_object_set(rate_limit_section, "Enabled", json_true());
    json_object_set(rate_limit_section, "MaxRequestsPerInterval", json_integer(120));
    json_object_set(rate_limit_section, "IntervalSeconds", json_integer(30));
    json_object_set(rate_limit_section, "MaxTokensPerInterval", json_integer(50000));

    json_object_set(chat_section, "RateLimit", rate_limit_section);
    json_object_set(root, "Chat", chat_section);

    bool result = load_chat_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(config.chat.RateLimit.Enabled);
    TEST_ASSERT_EQUAL(120, config.chat.RateLimit.MaxRequestsPerInterval);
    TEST_ASSERT_EQUAL(30, config.chat.RateLimit.IntervalSeconds);
    TEST_ASSERT_EQUAL(50000, config.chat.RateLimit.MaxTokensPerInterval);

    json_decref(root);
    cleanup_chat_config(&config.chat);
}

void test_load_chat_config_rate_limit_disabled(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* chat_section = json_object();
    json_t* rate_limit_section = json_object();

    // Disable rate limiting
    json_object_set(rate_limit_section, "Enabled", json_false());
    json_object_set(rate_limit_section, "MaxRequestsPerInterval", json_integer(10));
    json_object_set(rate_limit_section, "IntervalSeconds", json_integer(10));
    json_object_set(rate_limit_section, "MaxTokensPerInterval", json_integer(100));

    json_object_set(chat_section, "RateLimit", rate_limit_section);
    json_object_set(root, "Chat", chat_section);

    bool result = load_chat_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(config.chat.RateLimit.Enabled);
    TEST_ASSERT_EQUAL(10, config.chat.RateLimit.MaxRequestsPerInterval);
    TEST_ASSERT_EQUAL(10, config.chat.RateLimit.IntervalSeconds);
    TEST_ASSERT_EQUAL(100, config.chat.RateLimit.MaxTokensPerInterval);

    json_decref(root);
    cleanup_chat_config(&config.chat);
}

void test_load_chat_config_invalid_max_requests(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* chat_section = json_object();
    json_t* rate_limit_section = json_object();

    // Set invalid MaxRequestsPerInterval (< 0)
    json_object_set(rate_limit_section, "Enabled", json_true());
    json_object_set(rate_limit_section, "MaxRequestsPerInterval", json_integer(-1));
    json_object_set(rate_limit_section, "IntervalSeconds", json_integer(60));
    json_object_set(rate_limit_section, "MaxTokensPerInterval", json_integer(100000));

    json_object_set(chat_section, "RateLimit", rate_limit_section);
    json_object_set(root, "Chat", chat_section);

    bool result = load_chat_config(root, &config);

    TEST_ASSERT_FALSE(result);

    json_decref(root);
    cleanup_chat_config(&config.chat);
}

void test_load_chat_config_invalid_interval_seconds(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* chat_section = json_object();
    json_t* rate_limit_section = json_object();

    // Set invalid IntervalSeconds (<= 0)
    json_object_set(rate_limit_section, "Enabled", json_true());
    json_object_set(rate_limit_section, "MaxRequestsPerInterval", json_integer(60));
    json_object_set(rate_limit_section, "IntervalSeconds", json_integer(0));
    json_object_set(rate_limit_section, "MaxTokensPerInterval", json_integer(100000));

    json_object_set(chat_section, "RateLimit", rate_limit_section);
    json_object_set(root, "Chat", chat_section);

    bool result = load_chat_config(root, &config);

    TEST_ASSERT_FALSE(result);

    json_decref(root);
    cleanup_chat_config(&config.chat);
}

void test_load_chat_config_invalid_max_tokens(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* chat_section = json_object();
    json_t* rate_limit_section = json_object();

    // Set invalid MaxTokensPerInterval (< 0)
    json_object_set(rate_limit_section, "Enabled", json_true());
    json_object_set(rate_limit_section, "MaxRequestsPerInterval", json_integer(60));
    json_object_set(rate_limit_section, "IntervalSeconds", json_integer(60));
    json_object_set(rate_limit_section, "MaxTokensPerInterval", json_integer(-1));

    json_object_set(chat_section, "RateLimit", rate_limit_section);
    json_object_set(root, "Chat", chat_section);

    bool result = load_chat_config(root, &config);

    TEST_ASSERT_FALSE(result);

    json_decref(root);
    cleanup_chat_config(&config.chat);
}

void test_load_chat_config_multiple_invalid_fields(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* chat_section = json_object();
    json_t* rate_limit_section = json_object();

    // Set multiple invalid values
    json_object_set(rate_limit_section, "Enabled", json_true());
    json_object_set(rate_limit_section, "MaxRequestsPerInterval", json_integer(-5));
    json_object_set(rate_limit_section, "IntervalSeconds", json_integer(-10));
    json_object_set(rate_limit_section, "MaxTokensPerInterval", json_integer(-20));

    json_object_set(chat_section, "RateLimit", rate_limit_section);
    json_object_set(root, "Chat", chat_section);

    bool result = load_chat_config(root, &config);

    TEST_ASSERT_FALSE(result);

    json_decref(root);
    cleanup_chat_config(&config.chat);
}

// ===== cleanup_chat_config TESTS =====

void test_cleanup_chat_config_null_pointer(void) {
    // Test cleanup with NULL pointer - should handle gracefully
    cleanup_chat_config(NULL);
    // No assertions needed - function should not crash
}

void test_cleanup_chat_config_empty_config(void) {
    ChatConfig config = {0};

    // Test cleanup on empty/uninitialized config
    cleanup_chat_config(&config);

    // Config should be zeroed out
    TEST_ASSERT_FALSE(config.RateLimit.Enabled);
    TEST_ASSERT_EQUAL(0, config.RateLimit.MaxRequestsPerInterval);
    TEST_ASSERT_EQUAL(0, config.RateLimit.IntervalSeconds);
    TEST_ASSERT_EQUAL(0, config.RateLimit.MaxTokensPerInterval);
}

void test_cleanup_chat_config_with_data(void) {
    ChatConfig config = {0};

    // Initialize with some test data
    config.RateLimit.Enabled = true;
    config.RateLimit.MaxRequestsPerInterval = 100;
    config.RateLimit.IntervalSeconds = 30;
    config.RateLimit.MaxTokensPerInterval = 50000;

    // Cleanup should zero out all fields
    cleanup_chat_config(&config);

    TEST_ASSERT_FALSE(config.RateLimit.Enabled);
    TEST_ASSERT_EQUAL(0, config.RateLimit.MaxRequestsPerInterval);
    TEST_ASSERT_EQUAL(0, config.RateLimit.IntervalSeconds);
    TEST_ASSERT_EQUAL(0, config.RateLimit.MaxTokensPerInterval);
}

// ===== dump_chat_config TESTS =====

void test_dump_chat_config_null_pointer(void) {
    // Test dump with NULL pointer - should handle gracefully
    dump_chat_config(NULL);
    // No assertions needed - function should not crash
}

void test_dump_chat_config_basic(void) {
    ChatConfig config = {0};

    // Initialize with test data
    config.RateLimit.Enabled = true;
    config.RateLimit.MaxRequestsPerInterval = 120;
    config.RateLimit.IntervalSeconds = 30;
    config.RateLimit.MaxTokensPerInterval = 50000;

    // Dump should not crash and handle the data properly
    dump_chat_config(&config);

    // Clean up
    cleanup_chat_config(&config);
}

// ===== MAIN TEST RUNNER =====

int main(void) {
    UNITY_BEGIN();

    // chat_config_apply_defaults tests
    RUN_TEST(test_chat_config_apply_defaults_null_pointer);
    RUN_TEST(test_chat_config_apply_defaults_valid_config);

    // load_chat_config tests
    RUN_TEST(test_load_chat_config_null_config);
    RUN_TEST(test_load_chat_config_empty_json);
    RUN_TEST(test_load_chat_config_all_fields);
    RUN_TEST(test_load_chat_config_rate_limit_disabled);
    RUN_TEST(test_load_chat_config_invalid_max_requests);
    RUN_TEST(test_load_chat_config_invalid_interval_seconds);
    RUN_TEST(test_load_chat_config_invalid_max_tokens);
    RUN_TEST(test_load_chat_config_multiple_invalid_fields);

    // cleanup_chat_config tests
    RUN_TEST(test_cleanup_chat_config_null_pointer);
    RUN_TEST(test_cleanup_chat_config_empty_config);
    RUN_TEST(test_cleanup_chat_config_with_data);

    // dump_chat_config tests
    RUN_TEST(test_dump_chat_config_null_pointer);
    RUN_TEST(test_dump_chat_config_basic);

    return UNITY_END();
}
