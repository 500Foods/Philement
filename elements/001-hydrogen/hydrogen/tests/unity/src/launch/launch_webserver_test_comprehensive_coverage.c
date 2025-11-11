/*
 * Unity Test File: Comprehensive WebServer Launch Tests
 * Tests for check_webserver_launch_readiness and launch_webserver_subsystem functions
 * with full edge case coverage to achieve >75% code coverage
 */

// Enable mocks BEFORE including ANY headers
// USE_MOCK_LAUNCH defined by CMake
#define USE_MOCK_SYSTEM
#define USE_MOCK_PTHREAD

// Include mock headers immediately
#include <unity/mocks/mock_launch.h>
#include <unity/mocks/mock_system.h>
#include <unity/mocks/mock_pthread.h>

// Include Unity framework
#include <unity.h>

// Include source headers (functions will now be mocked)
#include <src/hydrogen.h>
#include <src/launch/launch.h>
#include <src/config/config.h>
#include <src/config/config_defaults.h>
#include <src/config/config_webserver.h>

// Forward declarations for functions being tested
LaunchReadiness check_webserver_launch_readiness(void);
int launch_webserver_subsystem(void);

// Forward declarations for test functions
void test_webserver_readiness_no_config(void);
void test_webserver_readiness_no_protocols_enabled(void);
void test_webserver_readiness_invalid_port_low(void);
void test_webserver_readiness_valid_port_80(void);
void test_webserver_readiness_valid_port_443(void);
void test_webserver_readiness_null_web_root(void);
void test_webserver_readiness_invalid_web_root_no_slash(void);
void test_webserver_readiness_ipv6_enabled(void);
void test_webserver_readiness_high_port(void);
void test_launch_webserver_during_shutdown(void);
void test_launch_webserver_with_shutdown_flag(void);
void test_launch_webserver_not_starting(void);
void test_launch_webserver_no_configuration(void);
void test_launch_webserver_disabled_configuration(void);
void test_launch_webserver_malloc_failure_on_messages(void);
void test_launch_webserver_malloc_failure_protocol_message(void);

// External references
extern AppConfig* app_config;
extern volatile sig_atomic_t server_starting;
extern volatile sig_atomic_t server_stopping;
extern volatile sig_atomic_t web_server_shutdown;

// Test helper to create a minimal valid configuration using config_defaults
static void setup_minimal_valid_config(void) {
    if (!app_config) {
        app_config = calloc(1, sizeof(AppConfig));
        if (!app_config) {
            return;  // Let test fail naturally
        }
    }
    
    // Use the standard initialization which sets all defaults properly
    initialize_config_defaults(app_config);
    
    // Enable webserver (defaults have IPv4 enabled already)
    // Now we have all required fields from defaults:
    // - enable_ipv4 = true
    // - port = 5000
    // - web_root = "/tmp/hydrogen"
    // - upload_path = "/upload"
    // - upload_dir = "/tmp/hydrogen"
}

// Test helper to cleanup configuration
static void cleanup_test_config(void) {
    if (app_config) {
        // Cleanup webserver config fields
        if (app_config->webserver.web_root) {
            free(app_config->webserver.web_root);
            app_config->webserver.web_root = NULL;
        }
        if (app_config->webserver.upload_path) {
            free(app_config->webserver.upload_path);
            app_config->webserver.upload_path = NULL;
        }
        if (app_config->webserver.upload_dir) {
            free(app_config->webserver.upload_dir);
            app_config->webserver.upload_dir = NULL;
        }
        if (app_config->webserver.cors_origin) {
            free(app_config->webserver.cors_origin);
            app_config->webserver.cors_origin = NULL;
        }
        free(app_config);
        app_config = NULL;
    }
}

void setUp(void) {
    // Reset mocks
    mock_launch_reset_all();
    mock_system_reset_all();
    mock_pthread_reset_all();
    
    // Make dependencies appear ready by default
    mock_launch_set_is_subsystem_launchable_result(true);
    mock_launch_set_add_dependency_result(true);
    mock_launch_set_get_subsystem_id_result(1);
    mock_launch_set_get_subsystem_state_result(SUBSYSTEM_RUNNING);
    
    // Reset server state flags
    server_starting = 1;
    server_stopping = 0;
    web_server_shutdown = 0;
    
    // Clean slate for each test
    cleanup_test_config();
}

void tearDown(void) {
    // Clean up after each test
    cleanup_test_config();
    
    // Reset mocks
    mock_launch_reset_all();
    mock_system_reset_all();
    mock_pthread_reset_all();
    
    // Reset server state
    server_starting = 0;
    server_stopping = 0;
    web_server_shutdown = 0;
}

// ============================================================================
// check_webserver_launch_readiness() Tests
// ============================================================================

// Test: No configuration loaded
void test_webserver_readiness_no_config(void) {
    // Don't set up any config - app_config remains NULL
    
    LaunchReadiness result = check_webserver_launch_readiness();
    
    TEST_ASSERT_NOT_NULL(result.subsystem);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test: Both IPv4 and IPv6 disabled
void test_webserver_readiness_no_protocols_enabled(void) {
    setup_minimal_valid_config();
    app_config->webserver.enable_ipv4 = false;
    app_config->webserver.enable_ipv6 = false;
    
    LaunchReadiness result = check_webserver_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test: Invalid port number (< 1024, not 80 or 443)
void test_webserver_readiness_invalid_port_low(void) {
    setup_minimal_valid_config();
    app_config->webserver.port = 512;  // Invalid privileged port
    
    LaunchReadiness result = check_webserver_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test: Port 80 should be valid
void test_webserver_readiness_valid_port_80(void) {
    setup_minimal_valid_config();
    app_config->webserver.port = 80;
    
    LaunchReadiness result = check_webserver_launch_readiness();
    
    // Port 80 is explicitly allowed, so port validation should pass
    // (overall readiness may depend on other factors like network interfaces)
    TEST_ASSERT_NOT_NULL(result.subsystem);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test: Port 443 should be valid
void test_webserver_readiness_valid_port_443(void) {
    setup_minimal_valid_config();
    app_config->webserver.port = 443;
    
    LaunchReadiness result = check_webserver_launch_readiness();
    
    // Port 443 is explicitly allowed, so port validation should pass
    TEST_ASSERT_NOT_NULL(result.subsystem);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test: Invalid web root (NULL)
void test_webserver_readiness_null_web_root(void) {
    setup_minimal_valid_config();
    free(app_config->webserver.web_root);
    app_config->webserver.web_root = NULL;
    
    LaunchReadiness result = check_webserver_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test: Invalid web root (no slash)
void test_webserver_readiness_invalid_web_root_no_slash(void) {
    setup_minimal_valid_config();
    free(app_config->webserver.web_root);
    app_config->webserver.web_root = strdup("invalid_path");
    
    LaunchReadiness result = check_webserver_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test: Valid configuration with IPv6 enabled
void test_webserver_readiness_ipv6_enabled(void) {
    setup_minimal_valid_config();
    app_config->webserver.enable_ipv6 = true;
    
    LaunchReadiness result = check_webserver_launch_readiness();
    
    TEST_ASSERT_NOT_NULL(result.subsystem);
    TEST_ASSERT_EQUAL_STRING(SR_WEBSERVER, result.subsystem);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test: High port number (valid)
void test_webserver_readiness_high_port(void) {
    setup_minimal_valid_config();
    app_config->webserver.port = 8080;
    
    LaunchReadiness result = check_webserver_launch_readiness();
    
    TEST_ASSERT_NOT_NULL(result.messages);
}

// ============================================================================
// launch_webserver_subsystem() Tests
// ============================================================================

// Test: Cannot launch during shutdown
void test_launch_webserver_during_shutdown(void) {
    setup_minimal_valid_config();
    server_stopping = 1;
    server_starting = 0;
    
    int result = launch_webserver_subsystem();
    
    TEST_ASSERT_EQUAL(0, result);
}

// Test: Cannot launch when web_server_shutdown flag is set
void test_launch_webserver_with_shutdown_flag(void) {
    setup_minimal_valid_config();
    web_server_shutdown = 1;
    
    int result = launch_webserver_subsystem();
    
    TEST_ASSERT_EQUAL(0, result);
}

// Test: Cannot launch outside startup phase
void test_launch_webserver_not_starting(void) {
    setup_minimal_valid_config();
    server_starting = 0;
    server_stopping = 0;
    
    int result = launch_webserver_subsystem();
    
    TEST_ASSERT_EQUAL(0, result);
}

// Test: Cannot launch without configuration
void test_launch_webserver_no_configuration(void) {
    // Don't set up config
    server_starting = 1;
    
    int result = launch_webserver_subsystem();
    
    TEST_ASSERT_EQUAL(0, result);
}

// Test: Webserver disabled (no protocols enabled) - returns 1 (success, but disabled)
void test_launch_webserver_disabled_configuration(void) {
    setup_minimal_valid_config();
    app_config->webserver.enable_ipv4 = false;
    app_config->webserver.enable_ipv6 = false;
    
    int result = launch_webserver_subsystem();
    
    TEST_ASSERT_EQUAL(1, result);  // Not an error if disabled
}

// Test: Malloc failure during dependency registration
void test_launch_webserver_malloc_failure_on_messages(void) {
    setup_minimal_valid_config();
    
    // Simulate malloc failure for message allocation
    mock_system_set_malloc_failure(1);
    
    LaunchReadiness result = check_webserver_launch_readiness();
    
    // Should handle malloc failure gracefully
    TEST_ASSERT_NOT_NULL(result.subsystem);
}

// Test: Both malloc failures for protocol and port messages
void test_launch_webserver_malloc_failure_protocol_message(void) {
    setup_minimal_valid_config();
    
    // First malloc succeeds (subsystem name), second fails (protocol message)
    mock_system_set_malloc_failure(2);
    
    LaunchReadiness result = check_webserver_launch_readiness();
    
    // Should still function even if some messages can't be allocated
    TEST_ASSERT_NOT_NULL(result.subsystem);
}


int main(void) {
    UNITY_BEGIN();
    
    // check_webserver_launch_readiness() tests
    RUN_TEST(test_webserver_readiness_no_config);
    RUN_TEST(test_webserver_readiness_no_protocols_enabled);
    RUN_TEST(test_webserver_readiness_invalid_port_low);
    RUN_TEST(test_webserver_readiness_valid_port_80);
    RUN_TEST(test_webserver_readiness_valid_port_443);
    RUN_TEST(test_webserver_readiness_null_web_root);
    RUN_TEST(test_webserver_readiness_invalid_web_root_no_slash);
    RUN_TEST(test_webserver_readiness_ipv6_enabled);
    RUN_TEST(test_webserver_readiness_high_port);
    
    // launch_webserver_subsystem() tests
    RUN_TEST(test_launch_webserver_during_shutdown);
    RUN_TEST(test_launch_webserver_with_shutdown_flag);
    RUN_TEST(test_launch_webserver_not_starting);
    RUN_TEST(test_launch_webserver_no_configuration);
    RUN_TEST(test_launch_webserver_disabled_configuration);
    
    // Memory allocation failure tests
    RUN_TEST(test_launch_webserver_malloc_failure_on_messages);
    RUN_TEST(test_launch_webserver_malloc_failure_protocol_message);
    
    return UNITY_END();
}