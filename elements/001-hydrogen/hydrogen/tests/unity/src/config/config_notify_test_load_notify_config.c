/*
 * Unity Test File: Notify Configuration Tests
 * This file contains unit tests for the load_notify_config function
 * from src/config/config_notify.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/config/config_notify.h>
#include <src/config/config.h>

// Forward declarations for functions being tested
bool load_notify_config(json_t* root, AppConfig* config);
void cleanup_notify_config(NotifyConfig* config);
void dump_notify_config(const NotifyConfig* config);

// Forward declarations for test functions
void test_load_notify_config_null_root(void);
void test_load_notify_config_empty_json(void);
void test_load_notify_config_basic_fields(void);
void test_load_notify_config_smtp_fields(void);
void test_cleanup_notify_config_null_pointer(void);
void test_cleanup_notify_config_empty_config(void);
void test_cleanup_notify_config_with_data(void);
void test_dump_notify_config_null_pointer(void);
void test_dump_notify_config_basic(void);

// Test setup and teardown
void setUp(void) {
    // Reset any global state if needed
}

void tearDown(void) {
    // Clean up after each test
}

// ===== PARAMETER VALIDATION TESTS =====

void test_load_notify_config_null_root(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    // Test with NULL root - should handle gracefully and use defaults
    bool result = load_notify_config(NULL, &config);

    // Function should initialize defaults and return success even with NULL root
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(config.notify.enabled);  // Default is enabled
    TEST_ASSERT_EQUAL_STRING("none", config.notify.notifier);  // Default value
    TEST_ASSERT_EQUAL(587, config.notify.smtp.port);  // Default SMTP port

    cleanup_notify_config(&config.notify);
}

void test_load_notify_config_empty_json(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();

    bool result = load_notify_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(config.notify.enabled);  // Default is enabled
    TEST_ASSERT_EQUAL_STRING("none", config.notify.notifier);  // Default value
    TEST_ASSERT_EQUAL(587, config.notify.smtp.port);  // Default SMTP port
    TEST_ASSERT_TRUE(config.notify.smtp.use_tls);  // Default is true
    TEST_ASSERT_EQUAL(30, config.notify.smtp.timeout);  // Default timeout
    TEST_ASSERT_EQUAL(3, config.notify.smtp.max_retries);  // Default retries

    json_decref(root);
    cleanup_notify_config(&config.notify);
}

// ===== BASIC FIELD TESTS =====

void test_load_notify_config_basic_fields(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* notify_section = json_object();

    // Set up basic notify configuration
    json_object_set(notify_section, "Enabled", json_false());
    json_object_set(notify_section, "Notifier", json_string("SMTP"));

    json_object_set(root, "Notify", notify_section);

    bool result = load_notify_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(config.notify.enabled);
    TEST_ASSERT_EQUAL_STRING("SMTP", config.notify.notifier);

    json_decref(root);
    cleanup_notify_config(&config.notify);
}

// ===== SMTP FIELD TESTS =====

void test_load_notify_config_smtp_fields(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* notify_section = json_object();
    json_t* smtp_section = json_object();

    // Set up SMTP configuration
    json_object_set(smtp_section, "Host", json_string("smtp.example.com"));
    json_object_set(smtp_section, "Port", json_integer(465));
    json_object_set(smtp_section, "Username", json_string("test@example.com"));
    json_object_set(smtp_section, "Password", json_string("secret-password"));
    json_object_set(smtp_section, "UseTLS", json_false());
    json_object_set(smtp_section, "Timeout", json_integer(60));
    json_object_set(smtp_section, "MaxRetries", json_integer(5));
    json_object_set(smtp_section, "FromAddress", json_string("noreply@example.com"));

    json_object_set(notify_section, "SMTP", smtp_section);
    json_object_set(root, "Notify", notify_section);

    bool result = load_notify_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("smtp.example.com", config.notify.smtp.host);
    TEST_ASSERT_EQUAL(465, config.notify.smtp.port);
    TEST_ASSERT_EQUAL_STRING("test@example.com", config.notify.smtp.username);
    TEST_ASSERT_EQUAL_STRING("secret-password", config.notify.smtp.password);
    TEST_ASSERT_FALSE(config.notify.smtp.use_tls);
    TEST_ASSERT_EQUAL(60, config.notify.smtp.timeout);
    TEST_ASSERT_EQUAL(5, config.notify.smtp.max_retries);
    TEST_ASSERT_EQUAL_STRING("noreply@example.com", config.notify.smtp.from_address);

    json_decref(root);
    cleanup_notify_config(&config.notify);
}

// ===== CLEANUP FUNCTION TESTS =====

void test_cleanup_notify_config_null_pointer(void) {
    // Test cleanup with NULL pointer - should handle gracefully
    cleanup_notify_config(NULL);
    // No assertions needed - function should not crash
}

void test_cleanup_notify_config_empty_config(void) {
    NotifyConfig config = {0};

    // Test cleanup on empty/uninitialized config
    cleanup_notify_config(&config);

    // Config should be zeroed out
    TEST_ASSERT_FALSE(config.enabled);
    TEST_ASSERT_NULL(config.notifier);
    TEST_ASSERT_NULL(config.smtp.host);
    TEST_ASSERT_NULL(config.smtp.username);
    TEST_ASSERT_NULL(config.smtp.password);
    TEST_ASSERT_NULL(config.smtp.from_address);
}

void test_cleanup_notify_config_with_data(void) {
    NotifyConfig config = {0};

    // Initialize with some test data
    config.enabled = true;
    config.notifier = strdup("SMTP");
    config.smtp.host = strdup("smtp.example.com");
    config.smtp.username = strdup("test@example.com");
    config.smtp.password = strdup("secret-password");
    config.smtp.from_address = strdup("noreply@example.com");

    // Cleanup should free all allocated memory
    cleanup_notify_config(&config);

    // Config should be zeroed out
    TEST_ASSERT_FALSE(config.enabled);
    TEST_ASSERT_NULL(config.notifier);
    TEST_ASSERT_NULL(config.smtp.host);
    TEST_ASSERT_NULL(config.smtp.username);
    TEST_ASSERT_NULL(config.smtp.password);
    TEST_ASSERT_NULL(config.smtp.from_address);
}

// ===== DUMP FUNCTION TESTS =====

void test_dump_notify_config_null_pointer(void) {
    // Test dump with NULL pointer - should handle gracefully
    dump_notify_config(NULL);
    // No assertions needed - function should not crash
}

void test_dump_notify_config_basic(void) {
    NotifyConfig config = {0};

    // Initialize with test data
    config.enabled = true;
    config.notifier = strdup("SMTP");
    config.smtp.host = strdup("smtp.example.com");
    config.smtp.port = 587;
    config.smtp.username = strdup("test@example.com");
    config.smtp.password = strdup("secret-password");
    config.smtp.use_tls = true;
    config.smtp.timeout = 30;
    config.smtp.max_retries = 3;
    config.smtp.from_address = strdup("noreply@example.com");

    // Dump should not crash and handle the data properly
    dump_notify_config(&config);

    // Clean up
    cleanup_notify_config(&config);
}

// ===== MAIN TEST RUNNER =====

int main(void) {
    UNITY_BEGIN();

    // Parameter validation tests
    RUN_TEST(test_load_notify_config_null_root);
    RUN_TEST(test_load_notify_config_empty_json);

    // Basic field tests
    RUN_TEST(test_load_notify_config_basic_fields);
    RUN_TEST(test_load_notify_config_smtp_fields);

    // Cleanup function tests
    RUN_TEST(test_cleanup_notify_config_null_pointer);
    RUN_TEST(test_cleanup_notify_config_empty_config);
    RUN_TEST(test_cleanup_notify_config_with_data);

    // Dump function tests
    RUN_TEST(test_dump_notify_config_null_pointer);
    RUN_TEST(test_dump_notify_config_basic);

    return UNITY_END();
}