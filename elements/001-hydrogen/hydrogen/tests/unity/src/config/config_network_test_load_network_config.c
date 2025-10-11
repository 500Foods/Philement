/*
 * Unity Test File: Network Configuration Tests
 * This file contains unit tests for the load_network_config function
 * from src/config/config_network.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/config/config_network.h>
#include <src/config/config.h>

// Forward declarations for functions being tested
bool load_network_config(json_t* root, AppConfig* config);
int config_network_init(NetworkConfig* config);
void cleanup_network_config(NetworkConfig* config);
void dump_network_config(const NetworkConfig* config);
int config_network_add_reserved_port(NetworkConfig* config, int port);
int config_network_is_port_reserved(const NetworkConfig* config, int port);
const NetworkLimits* get_network_limits(void);

// Forward declarations for test functions
void test_load_network_config_null_root(void);
void test_load_network_config_empty_json(void);
void test_load_network_config_basic_fields(void);
void test_config_network_init_null_pointer(void);
void test_config_network_init_basic(void);
void test_cleanup_network_config_null_pointer(void);
void test_cleanup_network_config_empty_config(void);
void test_cleanup_network_config_with_data(void);
void test_dump_network_config_null_pointer(void);
void test_dump_network_config_basic(void);
void test_config_network_add_reserved_port(void);
void test_config_network_is_port_reserved(void);
void test_get_network_limits(void);

// Test setup and teardown
void setUp(void) {
    // Reset any global state if needed
}

void tearDown(void) {
    // Clean up after each test
}

// ===== PARAMETER VALIDATION TESTS =====

void test_load_network_config_null_root(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    // Test with NULL root - should handle gracefully and use defaults
    bool result = load_network_config(NULL, &config);

    // Function should initialize defaults and return success even with NULL root
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(16, config.network.max_interfaces);  // Default value
    TEST_ASSERT_EQUAL(1024, config.network.start_port);  // Default value

    cleanup_network_config(&config.network);
}

void test_load_network_config_empty_json(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();

    bool result = load_network_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(16, config.network.max_interfaces);  // Default value
    TEST_ASSERT_EQUAL(1024, config.network.start_port);  // Default value
    TEST_ASSERT_EQUAL(65535, config.network.end_port);  // Default value

    json_decref(root);
    cleanup_network_config(&config.network);
}

// ===== BASIC FIELD TESTS =====

void test_load_network_config_basic_fields(void) {
    AppConfig config = {0};
    initialize_config_defaults(&config);

    json_t* root = json_object();
    json_t* network_section = json_object();
    json_t* interfaces_section = json_object();
    json_t* port_section = json_object();

    // Set up basic network configuration
    json_object_set(interfaces_section, "MaxInterfaces", json_integer(8));
    json_object_set(interfaces_section, "MaxIPsPerInterface", json_integer(4));
    json_object_set(port_section, "StartPort", json_integer(2000));
    json_object_set(port_section, "EndPort", json_integer(3000));

    json_object_set(network_section, "Interfaces", interfaces_section);
    json_object_set(network_section, "PortAllocation", port_section);
    json_object_set(root, "Network", network_section);

    bool result = load_network_config(root, &config);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(8, config.network.max_interfaces);
    TEST_ASSERT_EQUAL(4, config.network.max_ips_per_interface);
    TEST_ASSERT_EQUAL(2000, config.network.start_port);
    TEST_ASSERT_EQUAL(3000, config.network.end_port);

    json_decref(root);
    cleanup_network_config(&config.network);
}

// ===== INIT FUNCTION TESTS =====

void test_config_network_init_null_pointer(void) {
    // Test with NULL pointer - should return -1
    int result = config_network_init(NULL);
    TEST_ASSERT_EQUAL(-1, result);
}

void test_config_network_init_basic(void) {
    NetworkConfig config = {0};

    // Test basic initialization
    int result = config_network_init(&config);

    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL(16, config.max_interfaces);
    TEST_ASSERT_EQUAL(1024, config.start_port);
    TEST_ASSERT_EQUAL(65535, config.end_port);
    TEST_ASSERT_NOT_NULL(config.reserved_ports);
    TEST_ASSERT_EQUAL(0, config.reserved_ports_count);

    cleanup_network_config(&config);
}

// ===== CLEANUP FUNCTION TESTS =====

void test_cleanup_network_config_null_pointer(void) {
    // Test cleanup with NULL pointer - should handle gracefully
    cleanup_network_config(NULL);
    // No assertions needed - function should not crash
}

void test_cleanup_network_config_empty_config(void) {
    NetworkConfig config = {0};

    // Test cleanup on empty/uninitialized config
    cleanup_network_config(&config);

    // Config should be zeroed out
    TEST_ASSERT_EQUAL(0, config.max_interfaces);
    TEST_ASSERT_NULL(config.reserved_ports);
    TEST_ASSERT_NULL(config.available_interfaces);
}

void test_cleanup_network_config_with_data(void) {
    NetworkConfig config = {0};

    // Initialize with some test data
    config_network_init(&config);
    config.max_interfaces = 8;
    config.start_port = 2000;

    // Add a reserved port
    config_network_add_reserved_port(&config, 8080);

    // Cleanup should free all allocated memory
    cleanup_network_config(&config);

    // Config should be zeroed out
    TEST_ASSERT_EQUAL(0, config.max_interfaces);
    TEST_ASSERT_NULL(config.reserved_ports);
    TEST_ASSERT_NULL(config.available_interfaces);
}

// ===== DUMP FUNCTION TESTS =====

void test_dump_network_config_null_pointer(void) {
    // Test dump with NULL pointer - should handle gracefully
    dump_network_config(NULL);
    // No assertions needed - function should not crash
}

void test_dump_network_config_basic(void) {
    NetworkConfig config = {0};

    // Initialize with test data
    config_network_init(&config);
    config.max_interfaces = 8;
    config.start_port = 2000;

    // Dump should not crash and handle the data properly
    dump_network_config(&config);

    // Clean up
    cleanup_network_config(&config);
}

// ===== RESERVED PORT TESTS =====

void test_config_network_add_reserved_port(void) {
    NetworkConfig config = {0};

    // Initialize config
    config_network_init(&config);

    // Test adding a valid reserved port
    int result = config_network_add_reserved_port(&config, 8080);
    TEST_ASSERT_EQUAL(0, result);
    TEST_ASSERT_EQUAL(1, config.reserved_ports_count);
    TEST_ASSERT_EQUAL(8080, config.reserved_ports[0]);

    // Test adding duplicate port (should fail)
    result = config_network_add_reserved_port(&config, 8080);
    TEST_ASSERT_EQUAL(-1, result);
    TEST_ASSERT_EQUAL(1, config.reserved_ports_count);  // Should not increase

    // Test adding port outside range (should fail)
    result = config_network_add_reserved_port(&config, 100);  // Below start_port
    TEST_ASSERT_EQUAL(-1, result);

    // Clean up
    cleanup_network_config(&config);
}

void test_config_network_is_port_reserved(void) {
    NetworkConfig config = {0};

    // Initialize config
    config_network_init(&config);

    // Add a reserved port
    config_network_add_reserved_port(&config, 8080);

    // Test checking reserved port
    int result = config_network_is_port_reserved(&config, 8080);
    TEST_ASSERT_EQUAL(1, result);

    // Test checking non-reserved port
    result = config_network_is_port_reserved(&config, 9090);
    TEST_ASSERT_EQUAL(0, result);

    // Test with NULL config
    result = config_network_is_port_reserved(NULL, 8080);
    TEST_ASSERT_EQUAL(-1, result);

    // Test with port outside range
    result = config_network_is_port_reserved(&config, 100);
    TEST_ASSERT_EQUAL(-1, result);

    // Clean up
    cleanup_network_config(&config);
}

// ===== LIMITS FUNCTION TESTS =====

void test_get_network_limits(void) {
    // Test getting network limits
    const NetworkLimits* limits = get_network_limits();

    TEST_ASSERT_NOT_NULL(limits);
    TEST_ASSERT_EQUAL(1, limits->min_interfaces);
    TEST_ASSERT_EQUAL(16, limits->max_interfaces);
    TEST_ASSERT_EQUAL(1024, limits->min_port);
    TEST_ASSERT_EQUAL(65535, limits->max_port);
}

// ===== MAIN TEST RUNNER =====

int main(void) {
    UNITY_BEGIN();

    // Parameter validation tests
    RUN_TEST(test_load_network_config_null_root);
    RUN_TEST(test_load_network_config_empty_json);

    // Basic field tests
    RUN_TEST(test_load_network_config_basic_fields);

    // Init function tests
    RUN_TEST(test_config_network_init_null_pointer);
    RUN_TEST(test_config_network_init_basic);

    // Cleanup function tests
    RUN_TEST(test_cleanup_network_config_null_pointer);
    RUN_TEST(test_cleanup_network_config_empty_config);
    RUN_TEST(test_cleanup_network_config_with_data);

    // Dump function tests
    RUN_TEST(test_dump_network_config_null_pointer);
    RUN_TEST(test_dump_network_config_basic);

    // Reserved port tests
    RUN_TEST(test_config_network_add_reserved_port);
    RUN_TEST(test_config_network_is_port_reserved);

    // Limits function tests
    RUN_TEST(test_get_network_limits);

    return UNITY_END();
}