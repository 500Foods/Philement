/*
 * Unity Test File: Mail Relay Configuration Tests
 * This file contains unit tests for the load_mailrelay_config function
 * from src/config/config_mail_relay.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../src/config/config_mail_relay.h"
#include "../../../../src/config/config.h"

// Forward declarations for functions being tested
bool load_mailrelay_config(json_t* root, AppConfig* config);
void cleanup_mailrelay_config(MailRelayConfig* config);
void dump_mailrelay_config(const MailRelayConfig* config);

// Forward declarations for helper functions we'll need
bool initialize_config_defaults(AppConfig* config);

// Forward declarations for test functions
void test_load_mailrelay_config_null_root(void);
void test_load_mailrelay_config_empty_json(void);
void test_load_mailrelay_config_basic_fields(void);
void test_load_mailrelay_config_queue_settings(void);
void test_cleanup_mailrelay_config_null_pointer(void);
void test_cleanup_mailrelay_config_empty_config(void);
void test_cleanup_mailrelay_config_with_data(void);
void test_dump_mailrelay_config_null_pointer(void);
void test_dump_mailrelay_config_basic(void);

// Test setup and teardown
void setUp(void) {
    // Reset any global state if needed
}

void tearDown(void) {
    // Clean up after each test
}

// ===== PARAMETER VALIDATION TESTS =====

void test_load_mailrelay_config_null_root(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    // Test with NULL root - should handle gracefully and use defaults
    bool result = load_mailrelay_config(NULL, &config);

    // Function should initialize defaults and return success even with NULL root
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(config.mail_relay.Enabled);  // Default is enabled
    TEST_ASSERT_EQUAL(587, config.mail_relay.ListenPort);  // Default port
    TEST_ASSERT_EQUAL(2, config.mail_relay.Workers);  // Default workers
    TEST_ASSERT_EQUAL(2, config.mail_relay.OutboundServerCount);  // Default servers

    cleanup_mailrelay_config(&config.mail_relay);
}

void test_load_mailrelay_config_empty_json(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();

    bool result = load_mailrelay_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(config.mail_relay.Enabled);  // Default is enabled
    TEST_ASSERT_EQUAL(587, config.mail_relay.ListenPort);  // Default port
    TEST_ASSERT_EQUAL(2, config.mail_relay.Workers);  // Default workers
    TEST_ASSERT_EQUAL(2, config.mail_relay.OutboundServerCount);  // Default servers

    json_decref(root);
    cleanup_mailrelay_config(&config.mail_relay);
}

// ===== BASIC FIELD TESTS =====

void test_load_mailrelay_config_basic_fields(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* mail_relay_section = json_object();

    // Set up basic mail relay configuration (without servers to avoid complexity)
    json_object_set(mail_relay_section, "Enabled", json_false());
    json_object_set(mail_relay_section, "ListenPort", json_integer(2525));
    json_object_set(mail_relay_section, "Workers", json_integer(4));

    json_object_set(root, "MailRelay", mail_relay_section);

    bool result = load_mailrelay_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(config.mail_relay.Enabled);
    TEST_ASSERT_EQUAL(2525, config.mail_relay.ListenPort);
    TEST_ASSERT_EQUAL(4, config.mail_relay.Workers);
    // Should have default servers when none configured
    TEST_ASSERT_EQUAL(2, config.mail_relay.OutboundServerCount);

    json_decref(root);
    cleanup_mailrelay_config(&config.mail_relay);
}

void test_load_mailrelay_config_queue_settings(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* mail_relay_section = json_object();
    json_t* queue_section = json_object();

    // Set up queue configuration
    json_object_set(queue_section, "MaxQueueSize", json_integer(500));
    json_object_set(queue_section, "RetryAttempts", json_integer(5));
    json_object_set(queue_section, "RetryDelaySeconds", json_integer(600));

    json_object_set(mail_relay_section, "Queue", queue_section);
    json_object_set(root, "MailRelay", mail_relay_section);

    bool result = load_mailrelay_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(500, config.mail_relay.Queue.MaxQueueSize);
    TEST_ASSERT_EQUAL(5, config.mail_relay.Queue.RetryAttempts);
    TEST_ASSERT_EQUAL(600, config.mail_relay.Queue.RetryDelaySeconds);

    json_decref(root);
    cleanup_mailrelay_config(&config.mail_relay);
}

// ===== CLEANUP FUNCTION TESTS =====

void test_cleanup_mailrelay_config_null_pointer(void) {
    // Test cleanup with NULL pointer - should handle gracefully
    cleanup_mailrelay_config(NULL);
    // No assertions needed - function should not crash
}

void test_cleanup_mailrelay_config_empty_config(void) {
    MailRelayConfig config = {0};

    // Test cleanup on empty/uninitialized config
    cleanup_mailrelay_config(&config);

    // Config should be zeroed out
    TEST_ASSERT_EQUAL(0, config.OutboundServerCount);
}

void test_cleanup_mailrelay_config_with_data(void) {
    MailRelayConfig config = {0};

    // Initialize with some test data
    config.Enabled = true;
    config.ListenPort = 587;
    config.Workers = 2;
    config.OutboundServerCount = 1;
    config.Servers[0].Host = strdup("smtp.example.com");
    config.Servers[0].Port = strdup("587");
    config.Servers[0].Username = strdup("test@example.com");
    config.Servers[0].Password = strdup("test-password");
    config.Servers[0].UseTLS = true;

    // Cleanup should free all allocated memory
    cleanup_mailrelay_config(&config);

    // Config should be zeroed out
    TEST_ASSERT_EQUAL(0, config.OutboundServerCount);
    TEST_ASSERT_NULL(config.Servers[0].Host);
    TEST_ASSERT_NULL(config.Servers[0].Port);
    TEST_ASSERT_NULL(config.Servers[0].Username);
    TEST_ASSERT_NULL(config.Servers[0].Password);
}

// ===== DUMP FUNCTION TESTS =====

void test_dump_mailrelay_config_null_pointer(void) {
    // Test dump with NULL pointer - should handle gracefully
    dump_mailrelay_config(NULL);
    // No assertions needed - function should not crash
}

void test_dump_mailrelay_config_basic(void) {
    MailRelayConfig config = {0};

    // Initialize with test data
    config.Enabled = true;
    config.ListenPort = 587;
    config.Workers = 2;
    config.Queue.MaxQueueSize = 1000;
    config.Queue.RetryAttempts = 3;
    config.Queue.RetryDelaySeconds = 300;
    config.OutboundServerCount = 1;
    config.Servers[0].Host = strdup("smtp.example.com");
    config.Servers[0].Port = strdup("587");
    config.Servers[0].Username = strdup("test@example.com");
    config.Servers[0].Password = strdup("test-password");
    config.Servers[0].UseTLS = true;

    // Dump should not crash and handle the data properly
    dump_mailrelay_config(&config);

    // Clean up
    cleanup_mailrelay_config(&config);
}

// ===== MAIN TEST RUNNER =====

int main(void) {
    UNITY_BEGIN();

    // Parameter validation tests
    RUN_TEST(test_load_mailrelay_config_null_root);
    RUN_TEST(test_load_mailrelay_config_empty_json);

    // Basic field tests
    RUN_TEST(test_load_mailrelay_config_basic_fields);
    RUN_TEST(test_load_mailrelay_config_queue_settings);

    // Cleanup function tests
    RUN_TEST(test_cleanup_mailrelay_config_null_pointer);
    RUN_TEST(test_cleanup_mailrelay_config_empty_config);
    RUN_TEST(test_cleanup_mailrelay_config_with_data);

    // Dump function tests
    RUN_TEST(test_dump_mailrelay_config_null_pointer);
    RUN_TEST(test_dump_mailrelay_config_basic);

    return UNITY_END();
}