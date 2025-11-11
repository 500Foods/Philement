/*
 * Unity Test File: Comprehensive mDNS Server Launch Tests
 * Tests for check_mdns_server_launch_readiness function with full edge case coverage
 */

// Enable mocks BEFORE including ANY headers
#define USE_MOCK_LAUNCH

// Include mock headers immediately
#include <unity/mocks/mock_launch.h>

// Include Unity framework
#include <unity.h>

// Include source headers (functions will now be mocked)
#include <src/hydrogen.h>
#include <src/launch/launch.h>
#include <src/config/config.h>
#include <src/config/config_defaults.h>
#include <src/config/config_mdns_server.h>
#include <src/mdns/mdns_server.h>

// Forward declarations for functions being tested
LaunchReadiness check_mdns_server_launch_readiness(void);

// External references
extern AppConfig* app_config;

// Forward declarations for test functions
void test_mdns_server_disabled_configuration(void);
void test_mdns_server_missing_device_id(void);
void test_mdns_server_empty_device_id(void);
void test_mdns_server_missing_friendly_name(void);
void test_mdns_server_empty_friendly_name(void);
void test_mdns_server_missing_model(void);
void test_mdns_server_empty_model(void);
void test_mdns_server_missing_manufacturer(void);
void test_mdns_server_empty_manufacturer(void);
void test_mdns_server_missing_version(void);
void test_mdns_server_empty_version(void);
void test_mdns_server_services_null_with_nonzero_count(void);
void test_mdns_server_service_missing_name(void);
void test_mdns_server_service_empty_name(void);
void test_mdns_server_service_missing_type(void);
void test_mdns_server_service_empty_type(void);
void test_mdns_server_service_port_zero(void);
void test_mdns_server_service_port_negative(void);
void test_mdns_server_service_port_too_large(void);
void test_mdns_server_service_txt_records_null_with_nonzero_count(void);
void test_mdns_server_valid_configuration_with_services(void);
void test_mdns_server_ipv6_enabled(void);
void test_mdns_server_multiple_services_first_invalid(void);

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
    
    // Enable mDNS server (defaults have it disabled)
    app_config->mdns_server.enable_ipv4 = true;
    // Now we have all required fields from defaults:
    // - device_id = "hydrogen-server"
    // - friendly_name = "Hydrogen Server"
    // - model = "Hydrogen"
    // - manufacturer = "Philement"
    // - version = "1.0.0"
}

// Test helper to cleanup configuration
static void cleanup_test_config(void) {
    if (app_config) {
        cleanup_mdns_server_config(&app_config->mdns_server);
        free(app_config);
        app_config = NULL;
    }
}

void setUp(void) {
    // Reset mocks
    mock_launch_reset_all();
    
    // Make Network subsystem appear ready so we can test configuration validation
    mock_launch_set_is_subsystem_launchable_result(true);
    mock_launch_set_add_dependency_result(true);
    mock_launch_set_get_subsystem_id_result(1);
    
    // Clean slate for each test
    cleanup_test_config();
}

void tearDown(void) {
    // Clean up after each test
    cleanup_test_config();
    
    // Reset mocks
    mock_launch_reset_all();
}

// Test 1: mDNS server disabled in configuration (both IPv4 and IPv6 off)
void test_mdns_server_disabled_configuration(void) {
    setup_minimal_valid_config();
    app_config->mdns_server.enable_ipv4 = false;
    app_config->mdns_server.enable_ipv6 = false;
    
    LaunchReadiness result = check_mdns_server_launch_readiness();
    
    TEST_ASSERT_NOT_NULL(result.subsystem);
    TEST_ASSERT_EQUAL_STRING(SR_MDNS_SERVER, result.subsystem);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 2: Missing device ID
void test_mdns_server_missing_device_id(void) {
    setup_minimal_valid_config();
    free(app_config->mdns_server.device_id);
    app_config->mdns_server.device_id = NULL;
    
    LaunchReadiness result = check_mdns_server_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 3: Empty device ID
void test_mdns_server_empty_device_id(void) {
    setup_minimal_valid_config();
    free(app_config->mdns_server.device_id);
    app_config->mdns_server.device_id = strdup("");
    
    LaunchReadiness result = check_mdns_server_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 4: Missing friendly name
void test_mdns_server_missing_friendly_name(void) {
    setup_minimal_valid_config();
    free(app_config->mdns_server.friendly_name);
    app_config->mdns_server.friendly_name = NULL;
    
    LaunchReadiness result = check_mdns_server_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 5: Empty friendly name
void test_mdns_server_empty_friendly_name(void) {
    setup_minimal_valid_config();
    free(app_config->mdns_server.friendly_name);
    app_config->mdns_server.friendly_name = strdup("");
    
    LaunchReadiness result = check_mdns_server_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 6: Missing model
void test_mdns_server_missing_model(void) {
    setup_minimal_valid_config();
    free(app_config->mdns_server.model);
    app_config->mdns_server.model = NULL;
    
    LaunchReadiness result = check_mdns_server_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 7: Empty model
void test_mdns_server_empty_model(void) {
    setup_minimal_valid_config();
    free(app_config->mdns_server.model);
    app_config->mdns_server.model = strdup("");
    
    LaunchReadiness result = check_mdns_server_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 8: Missing manufacturer
void test_mdns_server_missing_manufacturer(void) {
    setup_minimal_valid_config();
    free(app_config->mdns_server.manufacturer);
    app_config->mdns_server.manufacturer = NULL;
    
    LaunchReadiness result = check_mdns_server_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 9: Empty manufacturer
void test_mdns_server_empty_manufacturer(void) {
    setup_minimal_valid_config();
    free(app_config->mdns_server.manufacturer);
    app_config->mdns_server.manufacturer = strdup("");
    
    LaunchReadiness result = check_mdns_server_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 10: Missing version
void test_mdns_server_missing_version(void) {
    setup_minimal_valid_config();
    free(app_config->mdns_server.version);
    app_config->mdns_server.version = NULL;
    
    LaunchReadiness result = check_mdns_server_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 11: Empty version
void test_mdns_server_empty_version(void) {
    setup_minimal_valid_config();
    free(app_config->mdns_server.version);
    app_config->mdns_server.version = strdup("");
    
    LaunchReadiness result = check_mdns_server_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 12: Services count non-zero but array is NULL
void test_mdns_server_services_null_with_nonzero_count(void) {
    setup_minimal_valid_config();
    app_config->mdns_server.num_services = 1;
    app_config->mdns_server.services = NULL;
    
    LaunchReadiness result = check_mdns_server_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 13: Service with missing name
void test_mdns_server_service_missing_name(void) {
    setup_minimal_valid_config();
    app_config->mdns_server.num_services = 1;
    app_config->mdns_server.services = calloc(1, sizeof(mdns_server_service_t));
    app_config->mdns_server.services[0].name = NULL;
    app_config->mdns_server.services[0].type = strdup("_http._tcp");
    app_config->mdns_server.services[0].port = 8080;
    
    LaunchReadiness result = check_mdns_server_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 14: Service with empty name
void test_mdns_server_service_empty_name(void) {
    setup_minimal_valid_config();
    app_config->mdns_server.num_services = 1;
    app_config->mdns_server.services = calloc(1, sizeof(mdns_server_service_t));
    app_config->mdns_server.services[0].name = strdup("");
    app_config->mdns_server.services[0].type = strdup("_http._tcp");
    app_config->mdns_server.services[0].port = 8080;
    
    LaunchReadiness result = check_mdns_server_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 15: Service with missing type
void test_mdns_server_service_missing_type(void) {
    setup_minimal_valid_config();
    app_config->mdns_server.num_services = 1;
    app_config->mdns_server.services = calloc(1, sizeof(mdns_server_service_t));
    app_config->mdns_server.services[0].name = strdup("Web Server");
    app_config->mdns_server.services[0].type = NULL;
    app_config->mdns_server.services[0].port = 8080;
    
    LaunchReadiness result = check_mdns_server_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 16: Service with empty type
void test_mdns_server_service_empty_type(void) {
    setup_minimal_valid_config();
    app_config->mdns_server.num_services = 1;
    app_config->mdns_server.services = calloc(1, sizeof(mdns_server_service_t));
    app_config->mdns_server.services[0].name = strdup("Web Server");
    app_config->mdns_server.services[0].type = strdup("");
    app_config->mdns_server.services[0].port = 8080;
    
    LaunchReadiness result = check_mdns_server_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 17: Service with port 0
void test_mdns_server_service_port_zero(void) {
    setup_minimal_valid_config();
    app_config->mdns_server.num_services = 1;
    app_config->mdns_server.services = calloc(1, sizeof(mdns_server_service_t));
    app_config->mdns_server.services[0].name = strdup("Web Server");
    app_config->mdns_server.services[0].type = strdup("_http._tcp");
    app_config->mdns_server.services[0].port = 0;
    
    LaunchReadiness result = check_mdns_server_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 18: Service with negative port
void test_mdns_server_service_port_negative(void) {
    setup_minimal_valid_config();
    app_config->mdns_server.num_services = 1;
    app_config->mdns_server.services = calloc(1, sizeof(mdns_server_service_t));
    app_config->mdns_server.services[0].name = strdup("Web Server");
    app_config->mdns_server.services[0].type = strdup("_http._tcp");
    app_config->mdns_server.services[0].port = -1;
    
    LaunchReadiness result = check_mdns_server_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 19: Service with port greater than 65535
void test_mdns_server_service_port_too_large(void) {
    setup_minimal_valid_config();
    app_config->mdns_server.num_services = 1;
    app_config->mdns_server.services = calloc(1, sizeof(mdns_server_service_t));
    app_config->mdns_server.services[0].name = strdup("Web Server");
    app_config->mdns_server.services[0].type = strdup("_http._tcp");
    app_config->mdns_server.services[0].port = 65536;
    
    LaunchReadiness result = check_mdns_server_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 20: Service with TXT records count non-zero but array is NULL
void test_mdns_server_service_txt_records_null_with_nonzero_count(void) {
    setup_minimal_valid_config();
    app_config->mdns_server.num_services = 1;
    app_config->mdns_server.services = calloc(1, sizeof(mdns_server_service_t));
    app_config->mdns_server.services[0].name = strdup("Web Server");
    app_config->mdns_server.services[0].type = strdup("_http._tcp");
    app_config->mdns_server.services[0].port = 8080;
    app_config->mdns_server.services[0].num_txt_records = 1;
    app_config->mdns_server.services[0].txt_records = NULL;
    
    LaunchReadiness result = check_mdns_server_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 21: Valid configuration with services
void test_mdns_server_valid_configuration_with_services(void) {
    setup_minimal_valid_config();
    app_config->mdns_server.num_services = 2;
    app_config->mdns_server.services = calloc(2, sizeof(mdns_server_service_t));
    
    // First service
    app_config->mdns_server.services[0].name = strdup("Web Server");
    app_config->mdns_server.services[0].type = strdup("_http._tcp");
    app_config->mdns_server.services[0].port = 8080;
    app_config->mdns_server.services[0].num_txt_records = 1;
    app_config->mdns_server.services[0].txt_records = calloc(1, sizeof(char*));
    app_config->mdns_server.services[0].txt_records[0] = strdup("path=/api");
    
    // Second service
    app_config->mdns_server.services[1].name = strdup("WebSocket Server");
    app_config->mdns_server.services[1].type = strdup("_ws._tcp");
    app_config->mdns_server.services[1].port = 8081;
    app_config->mdns_server.services[1].num_txt_records = 0;
    app_config->mdns_server.services[1].txt_records = NULL;
    
    LaunchReadiness result = check_mdns_server_launch_readiness();
    
    TEST_ASSERT_NOT_NULL(result.subsystem);
    TEST_ASSERT_EQUAL_STRING(SR_MDNS_SERVER, result.subsystem);
    TEST_ASSERT_NOT_NULL(result.messages);
    // Note: readiness depends on network subsystem state
}

// Test 22: IPv6 enabled configuration
void test_mdns_server_ipv6_enabled(void) {
    setup_minimal_valid_config();
    app_config->mdns_server.enable_ipv6 = true;
    
    LaunchReadiness result = check_mdns_server_launch_readiness();
    
    TEST_ASSERT_NOT_NULL(result.subsystem);
    TEST_ASSERT_EQUAL_STRING(SR_MDNS_SERVER, result.subsystem);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test 23: Multiple services with mixed validity to check that we validate all
void test_mdns_server_multiple_services_first_invalid(void) {
    setup_minimal_valid_config();
    app_config->mdns_server.num_services = 2;
    app_config->mdns_server.services = calloc(2, sizeof(mdns_server_service_t));
    
    // First service - invalid (missing name)
    app_config->mdns_server.services[0].name = NULL;
    app_config->mdns_server.services[0].type = strdup("_http._tcp");
    app_config->mdns_server.services[0].port = 8080;
    
    // Second service - valid
    app_config->mdns_server.services[1].name = strdup("WebSocket Server");
    app_config->mdns_server.services[1].type = strdup("_ws._tcp");
    app_config->mdns_server.services[1].port = 8081;
    
    LaunchReadiness result = check_mdns_server_launch_readiness();
    
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

int main(void) {
    UNITY_BEGIN();
    
    // Configuration validation tests
    RUN_TEST(test_mdns_server_disabled_configuration);
    RUN_TEST(test_mdns_server_missing_device_id);
    RUN_TEST(test_mdns_server_empty_device_id);
    RUN_TEST(test_mdns_server_missing_friendly_name);
    RUN_TEST(test_mdns_server_empty_friendly_name);
    RUN_TEST(test_mdns_server_missing_model);
    RUN_TEST(test_mdns_server_empty_model);
    RUN_TEST(test_mdns_server_missing_manufacturer);
    RUN_TEST(test_mdns_server_empty_manufacturer);
    RUN_TEST(test_mdns_server_missing_version);
    RUN_TEST(test_mdns_server_empty_version);
    
    // Service validation tests
    RUN_TEST(test_mdns_server_services_null_with_nonzero_count);
    RUN_TEST(test_mdns_server_service_missing_name);
    RUN_TEST(test_mdns_server_service_empty_name);
    RUN_TEST(test_mdns_server_service_missing_type);
    RUN_TEST(test_mdns_server_service_empty_type);
    RUN_TEST(test_mdns_server_service_port_zero);
    RUN_TEST(test_mdns_server_service_port_negative);
    RUN_TEST(test_mdns_server_service_port_too_large);
    RUN_TEST(test_mdns_server_service_txt_records_null_with_nonzero_count);
    
    // Valid configuration tests
    RUN_TEST(test_mdns_server_valid_configuration_with_services);
    RUN_TEST(test_mdns_server_ipv6_enabled);
    RUN_TEST(test_mdns_server_multiple_services_first_invalid);
    
    return UNITY_END();
}