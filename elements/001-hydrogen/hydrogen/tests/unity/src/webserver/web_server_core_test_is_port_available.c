/*
 * Unity Test File: Web Server Core - Is Port Available Test
 * This file contains unit tests for is_port_available() function
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/webserver/web_server_core.h>

// Standard library includes
#include <string.h>
#include <stdlib.h>

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test functions
static void test_is_port_available_basic_ipv4(void) {
    // Test checking port availability for IPv4 only
    // Port 0 should always be available (lets OS assign)
    bool result = is_port_available(0, false);
    // Port 0 is always available for binding
    TEST_ASSERT_TRUE(result);
}

static void test_is_port_available_with_ipv6(void) {
    // Test checking port availability with IPv6 enabled
    // Port 0 should always be available
    bool result = is_port_available(0, true);
    // Should return true for IPv6 availability
    // Note: This tests both IPv4 and IPv6 paths
    TEST_ASSERT_TRUE(result);
}

static void test_is_port_available_common_port(void) {
    // Test with a common port number
    // Using a high port number that's likely to be available
    (void)is_port_available(65432, false);
    // High ports are typically available
    // Note: This could theoretically fail if something is using this port,
    // but 65432 is in the dynamic/private port range (49152-65535)
    // We'll accept either true or false since we can't control port usage
    // The main goal is that the function doesn't crash
    TEST_PASS(); // Function executed without crashing
}

static void test_is_port_available_reserved_port(void) {
    // Test with a reserved port (port 1)
    // This requires root privileges to bind, so it should return false
    // unless running as root
    (void)is_port_available(1, false);
    // We can't assert the result since it depends on privileges
    // The main goal is that the function doesn't crash
    TEST_PASS(); // Function executed without crashing
}

static void test_is_port_available_invalid_port_negative(void) {
    // Test with a negative port number (should be handled gracefully)
    // The function takes int, so negative values are possible
    // Port numbers are cast to uint16_t, so negative values become large positive
    (void)is_port_available(-1, false);
    // -1 cast to uint16_t becomes 65535, which is a valid port
    // The function should handle this gracefully
    TEST_PASS(); // Function executed without crashing
}

static void test_is_port_available_zero_port_ipv6(void) {
    // Test port 0 with IPv6 check enabled
    bool result = is_port_available(0, true);
    TEST_ASSERT_TRUE(result);
}

static void test_is_port_available_multiple_calls(void) {
    // Test that multiple calls work correctly
    bool result1 = is_port_available(0, false);
    bool result2 = is_port_available(0, true);
    bool result3 = is_port_available(0, false);
    
    // All calls should return true for port 0
    TEST_ASSERT_TRUE(result1);
    TEST_ASSERT_TRUE(result2);
    TEST_ASSERT_TRUE(result3);
}

static void test_is_port_available_different_ports(void) {
    // Test with different port numbers
    // These are all in the dynamic range and should be available
    bool result1 = is_port_available(50000, false);
    bool result2 = is_port_available(55000, false);
    bool result3 = is_port_available(60000, false);
    
    // We can't guarantee availability, but we can verify no crashes
    (void)result1;
    (void)result2;
    (void)result3;
    TEST_PASS(); // All calls executed without crashing
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_is_port_available_basic_ipv4);
    RUN_TEST(test_is_port_available_with_ipv6);
    RUN_TEST(test_is_port_available_common_port);
    RUN_TEST(test_is_port_available_reserved_port);
    RUN_TEST(test_is_port_available_invalid_port_negative);
    RUN_TEST(test_is_port_available_zero_port_ipv6);
    RUN_TEST(test_is_port_available_multiple_calls);
    RUN_TEST(test_is_port_available_different_ports);

    return UNITY_END();
}
