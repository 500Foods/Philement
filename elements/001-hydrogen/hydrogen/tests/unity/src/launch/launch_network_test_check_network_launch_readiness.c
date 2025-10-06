/*
 * Unity Test File: Network Launch Readiness Check Tests
 * This file contains unit tests for check_network_launch_readiness function
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../src/launch/launch.h"

// Forward declarations for functions being tested
LaunchReadiness check_network_launch_readiness(void);
int launch_network_subsystem(void);

// Forward declarations for test functions
void test_check_network_launch_readiness_server_stopping(void);
void test_check_network_launch_readiness_web_server_shutdown(void);
void test_check_network_launch_readiness_invalid_system_state(void);
void test_check_network_launch_readiness_no_config(void);
void test_check_network_launch_readiness_success(void);
void test_launch_network_subsystem_server_stopping(void);
void test_launch_network_subsystem_invalid_system_state(void);
void test_launch_network_subsystem_no_config(void);
void test_launch_network_subsystem_success(void);

// Test data structures
static AppConfig test_app_config;

void setUp(void) {
    // Reset global state
    server_stopping = 0;
    web_server_shutdown = 0;
    server_starting = 1;
    server_running = 1;
    app_config = &test_app_config;

    // Set up default valid config
    memset(&test_app_config, 0, sizeof(AppConfig));
    test_app_config.network.max_interfaces = 5;
    test_app_config.network.max_ips_per_interface = 3;
    test_app_config.network.max_interface_name_length = 10;
    test_app_config.network.max_ip_address_length = 20;
    test_app_config.network.start_port = 1000;
    test_app_config.network.end_port = 2000;
    test_app_config.network.reserved_ports_count = 0;
    test_app_config.network.reserved_ports = NULL;
}

void tearDown(void) {
    // Clean up
}

// Test early return conditions
void test_check_network_launch_readiness_server_stopping(void) {
    server_stopping = 1;

    LaunchReadiness result = check_network_launch_readiness();

    TEST_ASSERT_EQUAL_STRING("Network", result.subsystem);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

void test_check_network_launch_readiness_web_server_shutdown(void) {
    web_server_shutdown = 1;

    LaunchReadiness result = check_network_launch_readiness();

    TEST_ASSERT_EQUAL_STRING("Network", result.subsystem);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

void test_check_network_launch_readiness_invalid_system_state(void) {
    server_starting = 0;
    server_running = 0;

    LaunchReadiness result = check_network_launch_readiness();

    TEST_ASSERT_EQUAL_STRING("Network", result.subsystem);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

void test_check_network_launch_readiness_no_config(void) {
    app_config = NULL;

    LaunchReadiness result = check_network_launch_readiness();

    TEST_ASSERT_EQUAL_STRING("Network", result.subsystem);
    TEST_ASSERT_FALSE(result.ready);
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test successful case (may not actually be ready in test environment)
void test_check_network_launch_readiness_success(void) {
    LaunchReadiness result = check_network_launch_readiness();

    TEST_ASSERT_EQUAL_STRING("Network", result.subsystem);
    // In test environment, readiness depends on available network interfaces
    // Just check that the function completes without crashing
    TEST_ASSERT_NOT_NULL(result.messages);
}

// Test launch_network_subsystem function
void test_launch_network_subsystem_server_stopping(void) {
    server_stopping = 1;

    int result = launch_network_subsystem();

    TEST_ASSERT_EQUAL(0, result);
}

void test_launch_network_subsystem_invalid_system_state(void) {
    server_starting = 0;

    int result = launch_network_subsystem();

    TEST_ASSERT_EQUAL(0, result);
}

void test_launch_network_subsystem_no_config(void) {
    app_config = NULL;

    int result = launch_network_subsystem();

    TEST_ASSERT_EQUAL(0, result);
}

void test_launch_network_subsystem_success(void) {
    // Test that it doesn't crash and returns a value
    int result = launch_network_subsystem();

    // The result depends on system state, but it should not crash
    TEST_ASSERT(result == 0 || result == 1);
}

int main(void) {
    UNITY_BEGIN();

    // Early return tests
    RUN_TEST(test_check_network_launch_readiness_server_stopping);
    RUN_TEST(test_check_network_launch_readiness_web_server_shutdown);
    RUN_TEST(test_check_network_launch_readiness_invalid_system_state);
    RUN_TEST(test_check_network_launch_readiness_no_config);

    // Success case
    RUN_TEST(test_check_network_launch_readiness_success);

    // launch_network_subsystem tests
    RUN_TEST(test_launch_network_subsystem_server_stopping);
    RUN_TEST(test_launch_network_subsystem_invalid_system_state);
    RUN_TEST(test_launch_network_subsystem_no_config);
    RUN_TEST(test_launch_network_subsystem_success);

    return UNITY_END();
}
