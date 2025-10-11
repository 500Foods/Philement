/*
 * Unity Test File: mDNS Client Configuration Tests
 * This file contains unit tests for the load_mdns_client_config function
 * from src/config/config_mdns_client.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/config/config_mdns_client.h>
#include <src/config/config.h>

// Forward declarations for functions being tested
bool load_mdns_client_config(json_t* root, AppConfig* config);
void cleanup_mdns_client_config(MDNSClientConfig* config);
void dump_mdns_client_config(const MDNSClientConfig* config);

// Forward declarations for test functions
void test_load_mdns_client_config_null_root(void);
void test_load_mdns_client_config_empty_json(void);
void test_load_mdns_client_config_basic_fields(void);
void test_load_mdns_client_config_health_check(void);
void test_load_mdns_client_config_service_types(void);
void test_cleanup_mdns_client_config_null_pointer(void);
void test_cleanup_mdns_client_config_empty_config(void);
void test_cleanup_mdns_client_config_with_data(void);
void test_dump_mdns_client_config_null_pointer(void);
void test_dump_mdns_client_config_basic(void);

// Test setup and teardown
void setUp(void) {
    // Reset any global state if needed
}

void tearDown(void) {
    // Clean up after each test
}

// ===== PARAMETER VALIDATION TESTS =====

void test_load_mdns_client_config_null_root(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    // Test with NULL root - should handle gracefully and use defaults
    bool result = load_mdns_client_config(NULL, &config);

    // Function should initialize defaults and return success even with NULL root
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(config.mdns_client.enable_ipv4);  // Default is enabled
    TEST_ASSERT_FALSE(config.mdns_client.enable_ipv6);  // Default is disabled
    TEST_ASSERT_EQUAL(30, config.mdns_client.scan_interval);  // Default value
    TEST_ASSERT_EQUAL(100, config.mdns_client.max_services);  // Default value

    cleanup_mdns_client_config(&config.mdns_client);
}

void test_load_mdns_client_config_empty_json(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();

    bool result = load_mdns_client_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(config.mdns_client.enable_ipv4);  // Default is enabled
    TEST_ASSERT_FALSE(config.mdns_client.enable_ipv6);  // Default is disabled
    TEST_ASSERT_EQUAL(30, config.mdns_client.scan_interval);  // Default value
    TEST_ASSERT_EQUAL(100, config.mdns_client.max_services);  // Default value
    TEST_ASSERT_EQUAL(3, config.mdns_client.retry_count);  // Default value
    TEST_ASSERT_TRUE(config.mdns_client.health_check_enabled);  // Default is enabled
    TEST_ASSERT_EQUAL(60, config.mdns_client.health_check_interval);  // Default value

    json_decref(root);
    cleanup_mdns_client_config(&config.mdns_client);
}

// ===== BASIC FIELD TESTS =====

void test_load_mdns_client_config_basic_fields(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* mdns_section = json_object();

    // Set up basic mDNS client configuration
    json_object_set(mdns_section, "EnableIPv4", json_false());
    json_object_set(mdns_section, "EnableIPv6", json_true());
    json_object_set(mdns_section, "ScanIntervalMs", json_integer(60));
    json_object_set(mdns_section, "MaxServices", json_integer(50));
    json_object_set(mdns_section, "RetryCount", json_integer(5));

    json_object_set(root, "mDNSClient", mdns_section);

    bool result = load_mdns_client_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(config.mdns_client.enable_ipv4);
    TEST_ASSERT_TRUE(config.mdns_client.enable_ipv6);
    TEST_ASSERT_EQUAL(60, config.mdns_client.scan_interval);
    TEST_ASSERT_EQUAL(50, config.mdns_client.max_services);
    TEST_ASSERT_EQUAL(5, config.mdns_client.retry_count);

    json_decref(root);
    cleanup_mdns_client_config(&config.mdns_client);
}

// ===== HEALTH CHECK TESTS =====

void test_load_mdns_client_config_health_check(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* mdns_section = json_object();
    json_t* health_section = json_object();

    // Set up health check configuration
    json_object_set(health_section, "Enabled", json_false());
    json_object_set(health_section, "IntervalMs", json_integer(120));

    json_object_set(mdns_section, "HealthCheck", health_section);
    json_object_set(root, "mDNSClient", mdns_section);

    bool result = load_mdns_client_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(config.mdns_client.health_check_enabled);
    TEST_ASSERT_EQUAL(120, config.mdns_client.health_check_interval);

    json_decref(root);
    cleanup_mdns_client_config(&config.mdns_client);
}

// ===== SERVICE TYPES TESTS =====

void test_load_mdns_client_config_service_types(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* mdns_section = json_object();
    json_t* service_types_array = json_array();
    json_t* service1 = json_object();
    json_t* service2 = json_object();

    // Set up service types
    json_object_set(service1, "Type", json_string("_http._tcp.local"));
    json_object_set(service1, "Required", json_true());
    json_object_set(service1, "AutoConnect", json_false());

    json_object_set(service2, "Type", json_string("_ssh._tcp.local"));
    json_object_set(service2, "Required", json_false());
    json_object_set(service2, "AutoConnect", json_true());

    json_array_append(service_types_array, service1);
    json_array_append(service_types_array, service2);

    json_object_set(mdns_section, "ServiceTypes", service_types_array);
    json_object_set(root, "mDNSClient", mdns_section);

    // Also set the ServiceTypes at the root level for the PROCESS_SECTION to find it
    json_object_set(root, "mDNSClient.ServiceTypes", service_types_array);

    bool result = load_mdns_client_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(2, config.mdns_client.num_service_types);
    TEST_ASSERT_NOT_NULL(config.mdns_client.service_types);

    // Check first service
    TEST_ASSERT_EQUAL_STRING("_http._tcp.local", config.mdns_client.service_types[0].type);
    TEST_ASSERT_TRUE(config.mdns_client.service_types[0].required);
    TEST_ASSERT_FALSE(config.mdns_client.service_types[0].auto_connect);

    // Check second service
    TEST_ASSERT_EQUAL_STRING("_ssh._tcp.local", config.mdns_client.service_types[1].type);
    TEST_ASSERT_FALSE(config.mdns_client.service_types[1].required);
    TEST_ASSERT_TRUE(config.mdns_client.service_types[1].auto_connect);

    json_decref(root);
    cleanup_mdns_client_config(&config.mdns_client);
}

// ===== CLEANUP FUNCTION TESTS =====

void test_cleanup_mdns_client_config_null_pointer(void) {
    // Test cleanup with NULL pointer - should handle gracefully
    cleanup_mdns_client_config(NULL);
    // No assertions needed - function should not crash
}

void test_cleanup_mdns_client_config_empty_config(void) {
    MDNSClientConfig config = {0};

    // Test cleanup on empty/uninitialized config
    cleanup_mdns_client_config(&config);

    // Config should be zeroed out
    TEST_ASSERT_FALSE(config.enable_ipv4);
    TEST_ASSERT_NULL(config.service_types);
    TEST_ASSERT_EQUAL(0, config.num_service_types);
}

void test_cleanup_mdns_client_config_with_data(void) {
    MDNSClientConfig config = {0};

    // Initialize with some test data
    config.enable_ipv4 = true;
    config.scan_interval = 60;
    config.num_service_types = 2;
    config.service_types = calloc(2, sizeof(MDNSServiceType));
    if (config.service_types) {
        config.service_types[0].type = strdup("_http._tcp.local");
        config.service_types[0].required = true;
        config.service_types[1].type = strdup("_ssh._tcp.local");
        config.service_types[1].required = false;
    }

    // Cleanup should free all allocated memory
    cleanup_mdns_client_config(&config);

    // Config should be zeroed out
    TEST_ASSERT_FALSE(config.enable_ipv4);
    TEST_ASSERT_NULL(config.service_types);
    TEST_ASSERT_EQUAL(0, config.num_service_types);
}

// ===== DUMP FUNCTION TESTS =====

void test_dump_mdns_client_config_null_pointer(void) {
    // Test dump with NULL pointer - should handle gracefully
    dump_mdns_client_config(NULL);
    // No assertions needed - function should not crash
}

void test_dump_mdns_client_config_basic(void) {
    MDNSClientConfig config = {0};

    // Initialize with test data
    config.enable_ipv4 = true;
    config.enable_ipv6 = false;
    config.scan_interval = 60;
    config.max_services = 50;
    config.retry_count = 5;
    config.health_check_enabled = true;
    config.health_check_interval = 120;
    config.num_service_types = 1;
    config.service_types = calloc(1, sizeof(MDNSServiceType));
    if (config.service_types) {
        config.service_types[0].type = strdup("_http._tcp.local");
        config.service_types[0].required = true;
        config.service_types[0].auto_connect = false;
    }

    // Dump should not crash and handle the data properly
    dump_mdns_client_config(&config);

    // Clean up
    cleanup_mdns_client_config(&config);
}

// ===== MAIN TEST RUNNER =====

int main(void) {
    UNITY_BEGIN();

    // Parameter validation tests
    RUN_TEST(test_load_mdns_client_config_null_root);
    RUN_TEST(test_load_mdns_client_config_empty_json);

    // Basic field tests
    RUN_TEST(test_load_mdns_client_config_basic_fields);
    RUN_TEST(test_load_mdns_client_config_health_check);
    RUN_TEST(test_load_mdns_client_config_service_types);

    // Cleanup function tests
    RUN_TEST(test_cleanup_mdns_client_config_null_pointer);
    RUN_TEST(test_cleanup_mdns_client_config_empty_config);
    RUN_TEST(test_cleanup_mdns_client_config_with_data);

    // Dump function tests
    RUN_TEST(test_dump_mdns_client_config_null_pointer);
    RUN_TEST(test_dump_mdns_client_config_basic);

    return UNITY_END();
}