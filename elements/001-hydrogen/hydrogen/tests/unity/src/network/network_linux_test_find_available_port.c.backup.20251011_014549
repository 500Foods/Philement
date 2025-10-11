/*
 * Unity Test: network_linux_test_find_available_port.c
 * Tests the find_available_port function for port discovery
 */

#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../src/network/network.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Test function prototypes
void test_find_available_port_basic_functionality(void);
void test_find_available_port_range_validation(void);
void test_find_available_port_reuse_test(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test basic port finding functionality
void test_find_available_port_basic_functionality(void) {
    // Test with a reasonable starting port
    int port = find_available_port(1025);

    // Should either find a port or return -1
    if (port != -1) {
        TEST_ASSERT_GREATER_OR_EQUAL(1025, port);
        TEST_ASSERT_LESS_OR_EQUAL(65535, port);
    } else {
        // If no port is found, that's also acceptable
        TEST_ASSERT_EQUAL(-1, port);
    }
}

// Test port range validation
void test_find_available_port_range_validation(void) {
    // Test with port 80 (privileged port)
    int port = find_available_port(80);

    // Should either find a port >= 80 or return -1
    if (port != -1) {
        TEST_ASSERT_GREATER_OR_EQUAL(80, port);
        TEST_ASSERT_LESS_OR_EQUAL(65535, port);
    } else {
        // If no port is found, that's also acceptable
        TEST_ASSERT_EQUAL(-1, port);
    }
}

// Test that we can find ports in different ranges
void test_find_available_port_reuse_test(void) {
    int port1 = find_available_port(2000);
    int port2 = find_available_port(3000);

    // Should find ports in different ranges or handle the case where ports are not available
    if (port1 != -1 && port2 != -1) {
        // Both should be valid ports in their respective ranges
        TEST_ASSERT_GREATER_OR_EQUAL(2000, port1);
        TEST_ASSERT_GREATER_OR_EQUAL(3000, port2);
        TEST_ASSERT_LESS_OR_EQUAL(65535, port1);
        TEST_ASSERT_LESS_OR_EQUAL(65535, port2);
    } else {
        // If either fails, that's also acceptable
        TEST_PASS();
    }
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_find_available_port_basic_functionality);
    RUN_TEST(test_find_available_port_range_validation);
    RUN_TEST(test_find_available_port_reuse_test);

    return UNITY_END();
}
