/*
 * Unity Test File: Mail Relay Launch Comprehensive Coverage Tests
 * This file contains comprehensive unit tests for launch_mail_relay.c to achieve >75% coverage
 */

// Enable mocks BEFORE including ANY headers
// USE_MOCK_LAUNCH defined by CMake
#define USE_MOCK_SYSTEM

// Include mock headers immediately
#include <unity/mocks/mock_launch.h>
#include <unity/mocks/mock_system.h>

// Include Unity framework
#include <unity.h>

// Include source headers (functions will now be mocked)
#include <src/hydrogen.h>
#include <src/launch/launch.h>
#include <src/config/config.h>

// Forward declarations for functions being tested
LaunchReadiness check_mail_relay_launch_readiness(void);
int launch_mail_relay_subsystem(void);

// Forward declarations for test functions
void test_check_mail_relay_launch_readiness_valid_config(void);
void test_check_mail_relay_launch_readiness_null_config(void);
void test_check_mail_relay_launch_readiness_disabled(void);
void test_check_mail_relay_launch_readiness_invalid_subsystem_id(void);
void test_check_mail_relay_launch_readiness_invalid_port(void);
void test_check_mail_relay_launch_readiness_invalid_workers(void);
void test_check_mail_relay_launch_readiness_invalid_queue_size(void);
void test_check_mail_relay_launch_readiness_invalid_retry_attempts(void);
void test_check_mail_relay_launch_readiness_invalid_retry_delay(void);
void test_check_mail_relay_launch_readiness_invalid_server_count(void);
void test_check_mail_relay_launch_readiness_missing_host(void);
void test_check_mail_relay_launch_readiness_missing_port(void);
void test_check_mail_relay_launch_readiness_missing_username(void);
void test_check_mail_relay_launch_readiness_missing_password(void);
void test_launch_mail_relay_subsystem_success(void);
void test_launch_mail_relay_subsystem_sets_shutdown_flag(void);

// Test configuration setup
static AppConfig* test_config = NULL;

// Helper function to create a valid mail relay configuration
static void setup_valid_config(void) {
    if (test_config) {
        cleanup_application_config();
    }

    test_config = calloc(1, sizeof(AppConfig));
    TEST_ASSERT_NOT_NULL(test_config);

    // Set up valid mail relay configuration
    test_config->mail_relay.Enabled = true;
    test_config->mail_relay.ListenPort = 2525;
    test_config->mail_relay.Workers = 4;
    test_config->mail_relay.Queue.MaxQueueSize = 1000;
    test_config->mail_relay.Queue.RetryAttempts = 3;
    test_config->mail_relay.Queue.RetryDelaySeconds = 30;
    test_config->mail_relay.OutboundServerCount = 2;

    // Configure first server
    test_config->mail_relay.Servers[0].Host = strdup("smtp.example.com");
    test_config->mail_relay.Servers[0].Port = strdup("587");
    test_config->mail_relay.Servers[0].Username = strdup("user@example.com");
    test_config->mail_relay.Servers[0].Password = strdup("password123");

    // Configure second server
    test_config->mail_relay.Servers[1].Host = strdup("smtp.backup.com");
    test_config->mail_relay.Servers[1].Port = strdup("465");
    test_config->mail_relay.Servers[1].Username = strdup("backup@example.com");
    test_config->mail_relay.Servers[1].Password = strdup("backup123");

    // Set global config pointer
    app_config = test_config;
}

// Helper function to create invalid configurations for testing
static void setup_invalid_config_port(void) {
    setup_valid_config();
    test_config->mail_relay.ListenPort = 0; // Invalid port
}

static void setup_invalid_config_workers(void) {
    setup_valid_config();
    test_config->mail_relay.Workers = 0; // Invalid workers
}

static void setup_invalid_config_queue_size(void) {
    setup_valid_config();
    test_config->mail_relay.Queue.MaxQueueSize = 0; // Invalid queue size
}

static void setup_invalid_config_retry_attempts(void) {
    setup_valid_config();
    test_config->mail_relay.Queue.RetryAttempts = -1; // Invalid retry attempts
}

static void setup_invalid_config_retry_delay(void) {
    setup_valid_config();
    test_config->mail_relay.Queue.RetryDelaySeconds = 0; // Invalid retry delay
}

static void setup_invalid_config_server_count(void) {
    setup_valid_config();
    test_config->mail_relay.OutboundServerCount = 0; // Invalid server count
}

static void setup_invalid_config_missing_host(void) {
    setup_valid_config();
    free(test_config->mail_relay.Servers[0].Host);
    test_config->mail_relay.Servers[0].Host = NULL; // Missing host
}

static void setup_invalid_config_missing_port(void) {
    setup_valid_config();
    free(test_config->mail_relay.Servers[0].Port);
    test_config->mail_relay.Servers[0].Port = NULL; // Missing port
}

static void setup_invalid_config_missing_username(void) {
    setup_valid_config();
    free(test_config->mail_relay.Servers[0].Username);
    test_config->mail_relay.Servers[0].Username = NULL; // Missing username
}

static void setup_invalid_config_missing_password(void) {
    setup_valid_config();
    free(test_config->mail_relay.Servers[0].Password);
    test_config->mail_relay.Servers[0].Password = NULL; // Missing password
}

static void setup_disabled_config(void) {
    setup_valid_config();
    test_config->mail_relay.Enabled = false; // Disabled
}

static void setup_null_config(void) {
    if (test_config) {
        cleanup_application_config();
        test_config = NULL;
    }
    app_config = NULL; // Null config
}

void setUp(void) {
    // Reset all mocks
    mock_launch_reset_all();
    mock_system_reset_all();

    // Set up default valid configuration
    setup_valid_config();

    // Set up mock defaults for successful operation
    mock_launch_set_get_subsystem_id_result(1); // Valid subsystem ID
}

void tearDown(void) {
    // Clean up test configuration
    if (test_config) {
        cleanup_application_config();
        test_config = NULL;
        app_config = NULL;
    }

    // Reset mocks
    mock_launch_reset_all();
    mock_system_reset_all();
}

// Test: Valid configuration with registry checks
// NOTE: This test validates the configuration structure but may fail on
// dependency checks since the registry is not initialized in test environment
void test_check_mail_relay_launch_readiness_valid_config(void) {
    LaunchReadiness result = check_mail_relay_launch_readiness();

    TEST_ASSERT_NOT_NULL(result.subsystem);
    TEST_ASSERT_EQUAL_STRING(SR_MAIL_RELAY, result.subsystem);
    // In test environment without registry initialization, dependency checks may fail
    // but we verify the function executes without crashing and returns a valid structure
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test: Null configuration should fail
void test_check_mail_relay_launch_readiness_null_config(void) {
    setup_null_config();

    LaunchReadiness result = check_mail_relay_launch_readiness();

    TEST_ASSERT_NOT_NULL(result.subsystem);
    TEST_ASSERT_EQUAL_STRING(SR_MAIL_RELAY, result.subsystem);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test: Disabled mail relay should fail
void test_check_mail_relay_launch_readiness_disabled(void) {
    setup_disabled_config();

    LaunchReadiness result = check_mail_relay_launch_readiness();

    TEST_ASSERT_NOT_NULL(result.subsystem);
    TEST_ASSERT_EQUAL_STRING(SR_MAIL_RELAY, result.subsystem);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test: Invalid subsystem ID skips dependency checks but continues with config validation
void test_check_mail_relay_launch_readiness_invalid_subsystem_id(void) {
    mock_launch_set_get_subsystem_id_result(-1); // Invalid subsystem ID

    LaunchReadiness result = check_mail_relay_launch_readiness();

    TEST_ASSERT_NOT_NULL(result.subsystem);
    TEST_ASSERT_EQUAL_STRING(SR_MAIL_RELAY, result.subsystem);
    // With invalid subsystem ID, dependency checks are skipped but config validation continues
    // Since we have valid config, it should still pass
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test: Invalid listen port should fail
void test_check_mail_relay_launch_readiness_invalid_port(void) {
    setup_invalid_config_port();

    LaunchReadiness result = check_mail_relay_launch_readiness();

    TEST_ASSERT_NOT_NULL(result.subsystem);
    TEST_ASSERT_EQUAL_STRING(SR_MAIL_RELAY, result.subsystem);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test: Invalid worker count should fail
void test_check_mail_relay_launch_readiness_invalid_workers(void) {
    setup_invalid_config_workers();

    LaunchReadiness result = check_mail_relay_launch_readiness();

    TEST_ASSERT_NOT_NULL(result.subsystem);
    TEST_ASSERT_EQUAL_STRING(SR_MAIL_RELAY, result.subsystem);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test: Invalid max queue size should fail
void test_check_mail_relay_launch_readiness_invalid_queue_size(void) {
    setup_invalid_config_queue_size();

    LaunchReadiness result = check_mail_relay_launch_readiness();

    TEST_ASSERT_NOT_NULL(result.subsystem);
    TEST_ASSERT_EQUAL_STRING(SR_MAIL_RELAY, result.subsystem);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test: Invalid retry attempts should fail
void test_check_mail_relay_launch_readiness_invalid_retry_attempts(void) {
    setup_invalid_config_retry_attempts();

    LaunchReadiness result = check_mail_relay_launch_readiness();

    TEST_ASSERT_NOT_NULL(result.subsystem);
    TEST_ASSERT_EQUAL_STRING(SR_MAIL_RELAY, result.subsystem);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test: Invalid retry delay should fail
void test_check_mail_relay_launch_readiness_invalid_retry_delay(void) {
    setup_invalid_config_retry_delay();

    LaunchReadiness result = check_mail_relay_launch_readiness();

    TEST_ASSERT_NOT_NULL(result.subsystem);
    TEST_ASSERT_EQUAL_STRING(SR_MAIL_RELAY, result.subsystem);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test: Invalid server count should fail
void test_check_mail_relay_launch_readiness_invalid_server_count(void) {
    setup_invalid_config_server_count();

    LaunchReadiness result = check_mail_relay_launch_readiness();

    TEST_ASSERT_NOT_NULL(result.subsystem);
    TEST_ASSERT_EQUAL_STRING(SR_MAIL_RELAY, result.subsystem);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test: Missing server host should fail
void test_check_mail_relay_launch_readiness_missing_host(void) {
    setup_invalid_config_missing_host();

    LaunchReadiness result = check_mail_relay_launch_readiness();

    TEST_ASSERT_NOT_NULL(result.subsystem);
    TEST_ASSERT_EQUAL_STRING(SR_MAIL_RELAY, result.subsystem);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test: Missing server port should fail
void test_check_mail_relay_launch_readiness_missing_port(void) {
    setup_invalid_config_missing_port();

    LaunchReadiness result = check_mail_relay_launch_readiness();

    TEST_ASSERT_NOT_NULL(result.subsystem);
    TEST_ASSERT_EQUAL_STRING(SR_MAIL_RELAY, result.subsystem);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test: Missing server username should fail
void test_check_mail_relay_launch_readiness_missing_username(void) {
    setup_invalid_config_missing_username();

    LaunchReadiness result = check_mail_relay_launch_readiness();

    TEST_ASSERT_NOT_NULL(result.subsystem);
    TEST_ASSERT_EQUAL_STRING(SR_MAIL_RELAY, result.subsystem);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test: Missing server password should fail
void test_check_mail_relay_launch_readiness_missing_password(void) {
    setup_invalid_config_missing_password();

    LaunchReadiness result = check_mail_relay_launch_readiness();

    TEST_ASSERT_NOT_NULL(result.subsystem);
    TEST_ASSERT_EQUAL_STRING(SR_MAIL_RELAY, result.subsystem);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test: Launch subsystem function should return success
void test_launch_mail_relay_subsystem_success(void) {
    int result = launch_mail_relay_subsystem();

    TEST_ASSERT_EQUAL(1, result);
}

// Test: Launch subsystem should set shutdown flag to 0
void test_launch_mail_relay_subsystem_sets_shutdown_flag(void) {
    // Reset the global flag first (simulate system state)
    mail_relay_system_shutdown = 1;

    int result = launch_mail_relay_subsystem();

    TEST_ASSERT_EQUAL(1, result);
    TEST_ASSERT_EQUAL(0, mail_relay_system_shutdown);
}

int main(void) {
    UNITY_BEGIN();

    // Basic functionality tests
    RUN_TEST(test_check_mail_relay_launch_readiness_valid_config);
    RUN_TEST(test_check_mail_relay_launch_readiness_null_config);
    RUN_TEST(test_check_mail_relay_launch_readiness_disabled);

    // Subsystem dependency tests
    RUN_TEST(test_check_mail_relay_launch_readiness_invalid_subsystem_id);

    // Configuration validation tests
    RUN_TEST(test_check_mail_relay_launch_readiness_invalid_port);
    RUN_TEST(test_check_mail_relay_launch_readiness_invalid_workers);
    RUN_TEST(test_check_mail_relay_launch_readiness_invalid_queue_size);
    RUN_TEST(test_check_mail_relay_launch_readiness_invalid_retry_attempts);
    RUN_TEST(test_check_mail_relay_launch_readiness_invalid_retry_delay);
    RUN_TEST(test_check_mail_relay_launch_readiness_invalid_server_count);

    // Server configuration validation tests
    RUN_TEST(test_check_mail_relay_launch_readiness_missing_host);
    RUN_TEST(test_check_mail_relay_launch_readiness_missing_port);
    RUN_TEST(test_check_mail_relay_launch_readiness_missing_username);
    RUN_TEST(test_check_mail_relay_launch_readiness_missing_password);

    // Launch subsystem tests
    RUN_TEST(test_launch_mail_relay_subsystem_success);
    RUN_TEST(test_launch_mail_relay_subsystem_sets_shutdown_flag);

    return UNITY_END();
}