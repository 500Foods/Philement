/*
 * Unity Test File: mDNS Server Configuration Validation Tests
 * Tests for validate_mdns_server_configuration function
 * This function was extracted from check_mdns_server_launch_readiness for better testability
 */

// Include Unity framework
#include <unity.h>

// Include source headers
#include <src/hydrogen.h>
#include <src/launch/launch.h>
#include <src/config/config.h>
#include <src/config/config_defaults.h>
#include <src/config/config_mdns_server.h>
#include <src/mdns/mdns_server.h>

// Forward declarations for functions being tested
bool validate_mdns_server_configuration(const char*** messages, size_t* count, size_t* capacity);

// External references
extern AppConfig* app_config;

// Forward declarations for test functions
void test_validate_mdns_server_configuration_disabled_both_protocols(void);
void test_validate_mdns_server_configuration_null_config(void);
void test_validate_mdns_server_configuration_missing_device_id(void);
void test_validate_mdns_server_configuration_empty_device_id(void);
void test_validate_mdns_server_configuration_missing_friendly_name(void);
void test_validate_mdns_server_configuration_empty_friendly_name(void);
void test_validate_mdns_server_configuration_missing_model(void);
void test_validate_mdns_server_configuration_empty_model(void);
void test_validate_mdns_server_configuration_missing_manufacturer(void);
void test_validate_mdns_server_configuration_empty_manufacturer(void);
void test_validate_mdns_server_configuration_missing_version(void);
void test_validate_mdns_server_configuration_empty_version(void);
void test_validate_mdns_server_configuration_services_null_nonzero_count(void);
void test_validate_mdns_server_configuration_service_missing_name(void);
void test_validate_mdns_server_configuration_service_empty_name(void);
void test_validate_mdns_server_configuration_service_missing_type(void);
void test_validate_mdns_server_configuration_service_empty_type(void);
void test_validate_mdns_server_configuration_service_port_zero(void);
void test_validate_mdns_server_configuration_service_port_negative(void);
void test_validate_mdns_server_configuration_service_port_too_large(void);
void test_validate_mdns_server_configuration_service_txt_null_nonzero_count(void);
void test_validate_mdns_server_configuration_valid_ipv4_only(void);
void test_validate_mdns_server_configuration_valid_ipv6_only(void);
void test_validate_mdns_server_configuration_valid_with_services(void);

void setUp(void) {
    // Clean up any existing config
    if (app_config) {
        cleanup_application_config();
        app_config = NULL;
    }
}

void tearDown(void) {
    // Clean up after each test
    if (app_config) {
        cleanup_application_config();
        app_config = NULL;
    }
}

// Test 1: Both IPv4 and IPv6 disabled
void test_validate_mdns_server_configuration_disabled_both_protocols(void) {
    app_config = calloc(1, sizeof(AppConfig));
    initialize_config_defaults(app_config);
    app_config->mdns_server.enable_ipv4 = false;
    app_config->mdns_server.enable_ipv6 = false;
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_mdns_server_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_FALSE(result);
    free_launch_messages(messages, count);
}

// Test 2: Null config pointer
void test_validate_mdns_server_configuration_null_config(void) {
    app_config = NULL;
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_mdns_server_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_FALSE(result);
    free_launch_messages(messages, count);
}

// Test 3: Missing device ID
void test_validate_mdns_server_configuration_missing_device_id(void) {
    app_config = calloc(1, sizeof(AppConfig));
    initialize_config_defaults(app_config);
    app_config->mdns_server.enable_ipv4 = true;
    free(app_config->mdns_server.device_id);
    app_config->mdns_server.device_id = NULL;
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_mdns_server_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_FALSE(result);
    free_launch_messages(messages, count);
}

// Test 4: Empty device ID
void test_validate_mdns_server_configuration_empty_device_id(void) {
    app_config = calloc(1, sizeof(AppConfig));
    initialize_config_defaults(app_config);
    app_config->mdns_server.enable_ipv4 = true;
    free(app_config->mdns_server.device_id);
    app_config->mdns_server.device_id = strdup("");
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_mdns_server_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_FALSE(result);
    free_launch_messages(messages, count);
}

// Test 5: Missing friendly name
void test_validate_mdns_server_configuration_missing_friendly_name(void) {
    app_config = calloc(1, sizeof(AppConfig));
    initialize_config_defaults(app_config);
    app_config->mdns_server.enable_ipv4 = true;
    free(app_config->mdns_server.friendly_name);
    app_config->mdns_server.friendly_name = NULL;
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_mdns_server_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_FALSE(result);
    free_launch_messages(messages, count);
}

// Test 6: Empty friendly name
void test_validate_mdns_server_configuration_empty_friendly_name(void) {
    app_config = calloc(1, sizeof(AppConfig));
    initialize_config_defaults(app_config);
    app_config->mdns_server.enable_ipv4 = true;
    free(app_config->mdns_server.friendly_name);
    app_config->mdns_server.friendly_name = strdup("");
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_mdns_server_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_FALSE(result);
    free_launch_messages(messages, count);
}

// Test 7: Missing model
void test_validate_mdns_server_configuration_missing_model(void) {
    app_config = calloc(1, sizeof(AppConfig));
    initialize_config_defaults(app_config);
    app_config->mdns_server.enable_ipv4 = true;
    free(app_config->mdns_server.model);
    app_config->mdns_server.model = NULL;
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_mdns_server_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_FALSE(result);
    free_launch_messages(messages, count);
}

// Test 8: Empty model
void test_validate_mdns_server_configuration_empty_model(void) {
    app_config = calloc(1, sizeof(AppConfig));
    initialize_config_defaults(app_config);
    app_config->mdns_server.enable_ipv4 = true;
    free(app_config->mdns_server.model);
    app_config->mdns_server.model = strdup("");
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_mdns_server_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_FALSE(result);
    free_launch_messages(messages, count);
}

// Test 9: Missing manufacturer
void test_validate_mdns_server_configuration_missing_manufacturer(void) {
    app_config = calloc(1, sizeof(AppConfig));
    initialize_config_defaults(app_config);
    app_config->mdns_server.enable_ipv4 = true;
    free(app_config->mdns_server.manufacturer);
    app_config->mdns_server.manufacturer = NULL;
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_mdns_server_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_FALSE(result);
    free_launch_messages(messages, count);
}

// Test 10: Empty manufacturer
void test_validate_mdns_server_configuration_empty_manufacturer(void) {
    app_config = calloc(1, sizeof(AppConfig));
    initialize_config_defaults(app_config);
    app_config->mdns_server.enable_ipv4 = true;
    free(app_config->mdns_server.manufacturer);
    app_config->mdns_server.manufacturer = strdup("");
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_mdns_server_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_FALSE(result);
    free_launch_messages(messages, count);
}

// Test 11: Missing version
void test_validate_mdns_server_configuration_missing_version(void) {
    app_config = calloc(1, sizeof(AppConfig));
    initialize_config_defaults(app_config);
    app_config->mdns_server.enable_ipv4 = true;
    free(app_config->mdns_server.version);
    app_config->mdns_server.version = NULL;
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_mdns_server_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_FALSE(result);
    free_launch_messages(messages, count);
}

// Test 12: Empty version
void test_validate_mdns_server_configuration_empty_version(void) {
    app_config = calloc(1, sizeof(AppConfig));
    initialize_config_defaults(app_config);
    app_config->mdns_server.enable_ipv4 = true;
    free(app_config->mdns_server.version);
    app_config->mdns_server.version = strdup("");
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_mdns_server_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_FALSE(result);
    free_launch_messages(messages, count);
}

// Test 13: Services count non-zero but array NULL
void test_validate_mdns_server_configuration_services_null_nonzero_count(void) {
    app_config = calloc(1, sizeof(AppConfig));
    initialize_config_defaults(app_config);
    app_config->mdns_server.enable_ipv4 = true;
    app_config->mdns_server.num_services = 1;
    app_config->mdns_server.services = NULL;
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_mdns_server_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_FALSE(result);
    free_launch_messages(messages, count);
}

// Test 14: Service with missing name
void test_validate_mdns_server_configuration_service_missing_name(void) {
    app_config = calloc(1, sizeof(AppConfig));
    initialize_config_defaults(app_config);
    app_config->mdns_server.enable_ipv4 = true;
    app_config->mdns_server.num_services = 1;
    app_config->mdns_server.services = calloc(1, sizeof(mdns_server_service_t));
    app_config->mdns_server.services[0].name = NULL;
    app_config->mdns_server.services[0].type = strdup("_http._tcp");
    app_config->mdns_server.services[0].port = 8080;
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_mdns_server_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_FALSE(result);
    free_launch_messages(messages, count);
}

// Test 15: Service with empty name
void test_validate_mdns_server_configuration_service_empty_name(void) {
    app_config = calloc(1, sizeof(AppConfig));
    initialize_config_defaults(app_config);
    app_config->mdns_server.enable_ipv4 = true;
    app_config->mdns_server.num_services = 1;
    app_config->mdns_server.services = calloc(1, sizeof(mdns_server_service_t));
    app_config->mdns_server.services[0].name = strdup("");
    app_config->mdns_server.services[0].type = strdup("_http._tcp");
    app_config->mdns_server.services[0].port = 8080;
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_mdns_server_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_FALSE(result);
    free_launch_messages(messages, count);
}

// Test 16: Service with missing type
void test_validate_mdns_server_configuration_service_missing_type(void) {
    app_config = calloc(1, sizeof(AppConfig));
    initialize_config_defaults(app_config);
    app_config->mdns_server.enable_ipv4 = true;
    app_config->mdns_server.num_services = 1;
    app_config->mdns_server.services = calloc(1, sizeof(mdns_server_service_t));
    app_config->mdns_server.services[0].name = strdup("Web Server");
    app_config->mdns_server.services[0].type = NULL;
    app_config->mdns_server.services[0].port = 8080;
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_mdns_server_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_FALSE(result);
    free_launch_messages(messages, count);
}

// Test 17: Service with empty type
void test_validate_mdns_server_configuration_service_empty_type(void) {
    app_config = calloc(1, sizeof(AppConfig));
    initialize_config_defaults(app_config);
    app_config->mdns_server.enable_ipv4 = true;
    app_config->mdns_server.num_services = 1;
    app_config->mdns_server.services = calloc(1, sizeof(mdns_server_service_t));
    app_config->mdns_server.services[0].name = strdup("Web Server");
    app_config->mdns_server.services[0].type = strdup("");
    app_config->mdns_server.services[0].port = 8080;
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_mdns_server_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_FALSE(result);
    free_launch_messages(messages, count);
}

// Test 18: Service with port 0
void test_validate_mdns_server_configuration_service_port_zero(void) {
    app_config = calloc(1, sizeof(AppConfig));
    initialize_config_defaults(app_config);
    app_config->mdns_server.enable_ipv4 = true;
    app_config->mdns_server.num_services = 1;
    app_config->mdns_server.services = calloc(1, sizeof(mdns_server_service_t));
    app_config->mdns_server.services[0].name = strdup("Web Server");
    app_config->mdns_server.services[0].type = strdup("_http._tcp");
    app_config->mdns_server.services[0].port = 0;
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_mdns_server_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_FALSE(result);
    free_launch_messages(messages, count);
}

// Test 19: Service with negative port
void test_validate_mdns_server_configuration_service_port_negative(void) {
    app_config = calloc(1, sizeof(AppConfig));
    initialize_config_defaults(app_config);
    app_config->mdns_server.enable_ipv4 = true;
    app_config->mdns_server.num_services = 1;
    app_config->mdns_server.services = calloc(1, sizeof(mdns_server_service_t));
    app_config->mdns_server.services[0].name = strdup("Web Server");
    app_config->mdns_server.services[0].type = strdup("_http._tcp");
    app_config->mdns_server.services[0].port = -1;
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_mdns_server_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_FALSE(result);
    free_launch_messages(messages, count);
}

// Test 20: Service with port > 65535
void test_validate_mdns_server_configuration_service_port_too_large(void) {
    app_config = calloc(1, sizeof(AppConfig));
    initialize_config_defaults(app_config);
    app_config->mdns_server.enable_ipv4 = true;
    app_config->mdns_server.num_services = 1;
    app_config->mdns_server.services = calloc(1, sizeof(mdns_server_service_t));
    app_config->mdns_server.services[0].name = strdup("Web Server");
    app_config->mdns_server.services[0].type = strdup("_http._tcp");
    app_config->mdns_server.services[0].port = 65536;
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_mdns_server_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_FALSE(result);
    free_launch_messages(messages, count);
}

// Test 21: Service with TXT records count non-zero but array NULL
void test_validate_mdns_server_configuration_service_txt_null_nonzero_count(void) {
    app_config = calloc(1, sizeof(AppConfig));
    initialize_config_defaults(app_config);
    app_config->mdns_server.enable_ipv4 = true;
    app_config->mdns_server.num_services = 1;
    app_config->mdns_server.services = calloc(1, sizeof(mdns_server_service_t));
    app_config->mdns_server.services[0].name = strdup("Web Server");
    app_config->mdns_server.services[0].type = strdup("_http._tcp");
    app_config->mdns_server.services[0].port = 8080;
    app_config->mdns_server.services[0].num_txt_records = 1;
    app_config->mdns_server.services[0].txt_records = NULL;
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_mdns_server_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_FALSE(result);
    free_launch_messages(messages, count);
}

// Test 22: Valid configuration with IPv4 only
void test_validate_mdns_server_configuration_valid_ipv4_only(void) {
    app_config = calloc(1, sizeof(AppConfig));
    initialize_config_defaults(app_config);
    app_config->mdns_server.enable_ipv4 = true;
    app_config->mdns_server.enable_ipv6 = false;
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_mdns_server_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_TRUE(result);
    free_launch_messages(messages, count);
}

// Test 23: Valid configuration with IPv6 only
void test_validate_mdns_server_configuration_valid_ipv6_only(void) {
    app_config = calloc(1, sizeof(AppConfig));
    initialize_config_defaults(app_config);
    app_config->mdns_server.enable_ipv4 = false;
    app_config->mdns_server.enable_ipv6 = true;
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_mdns_server_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_TRUE(result);
    free_launch_messages(messages, count);
}

// Test 24: Valid configuration with services
void test_validate_mdns_server_configuration_valid_with_services(void) {
    app_config = calloc(1, sizeof(AppConfig));
    initialize_config_defaults(app_config);
    app_config->mdns_server.enable_ipv4 = true;
    
    // Add valid services
    app_config->mdns_server.num_services = 2;
    app_config->mdns_server.services = calloc(2, sizeof(mdns_server_service_t));
    
    app_config->mdns_server.services[0].name = strdup("Web Server");
    app_config->mdns_server.services[0].type = strdup("_http._tcp");
    app_config->mdns_server.services[0].port = 8080;
    app_config->mdns_server.services[0].num_txt_records = 1;
    app_config->mdns_server.services[0].txt_records = calloc(1, sizeof(char*));
    app_config->mdns_server.services[0].txt_records[0] = strdup("path=/api");
    
    app_config->mdns_server.services[1].name = strdup("WebSocket");
    app_config->mdns_server.services[1].type = strdup("_ws._tcp");
    app_config->mdns_server.services[1].port = 8081;
    app_config->mdns_server.services[1].num_txt_records = 0;
    app_config->mdns_server.services[1].txt_records = NULL;
    
    const char** messages = NULL;
    size_t count = 0;
    size_t capacity = 0;
    
    bool result = validate_mdns_server_configuration(&messages, &count, &capacity);
    
    TEST_ASSERT_TRUE(result);
    free_launch_messages(messages, count);
}

int main(void) {
    UNITY_BEGIN();
    
    // Configuration state tests
    RUN_TEST(test_validate_mdns_server_configuration_disabled_both_protocols);
    RUN_TEST(test_validate_mdns_server_configuration_null_config);
    
    // Required field validation tests
    RUN_TEST(test_validate_mdns_server_configuration_missing_device_id);
    RUN_TEST(test_validate_mdns_server_configuration_empty_device_id);
    RUN_TEST(test_validate_mdns_server_configuration_missing_friendly_name);
    RUN_TEST(test_validate_mdns_server_configuration_empty_friendly_name);
    RUN_TEST(test_validate_mdns_server_configuration_missing_model);
    RUN_TEST(test_validate_mdns_server_configuration_empty_model);
    RUN_TEST(test_validate_mdns_server_configuration_missing_manufacturer);
    RUN_TEST(test_validate_mdns_server_configuration_empty_manufacturer);
    RUN_TEST(test_validate_mdns_server_configuration_missing_version);
    RUN_TEST(test_validate_mdns_server_configuration_empty_version);
    
    // Service array validation tests
    RUN_TEST(test_validate_mdns_server_configuration_services_null_nonzero_count);
    RUN_TEST(test_validate_mdns_server_configuration_service_missing_name);
    RUN_TEST(test_validate_mdns_server_configuration_service_empty_name);
    RUN_TEST(test_validate_mdns_server_configuration_service_missing_type);
    RUN_TEST(test_validate_mdns_server_configuration_service_empty_type);
    RUN_TEST(test_validate_mdns_server_configuration_service_port_zero);
    RUN_TEST(test_validate_mdns_server_configuration_service_port_negative);
    RUN_TEST(test_validate_mdns_server_configuration_service_port_too_large);
    RUN_TEST(test_validate_mdns_server_configuration_service_txt_null_nonzero_count);
    
    // Valid configuration tests
    RUN_TEST(test_validate_mdns_server_configuration_valid_ipv4_only);
    RUN_TEST(test_validate_mdns_server_configuration_valid_ipv6_only);
    RUN_TEST(test_validate_mdns_server_configuration_valid_with_services);
    
    return UNITY_END();
}