/*
 * Unity Test: network_linux_test_get_network_info.c
 * Tests the core network discovery function get_network_info
 * This function is large and covers significant portions of network functionality
 */

#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../src/network/network.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>

// Test function prototypes
void test_get_network_info_basic_discovery(void);
void test_get_network_info_null_safety(void);
void test_get_network_info_interface_counting(void);
void test_get_network_info_ipv4_ipv6_detection(void);
void test_free_network_info_null_safety(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test basic network discovery functionality
void test_get_network_info_basic_discovery(void) {
    network_info_t *info = get_network_info();

    // Should return a valid structure
    TEST_ASSERT_NOT_NULL(info);

    if (info) {
        // Should have at least loopback interface
        TEST_ASSERT_GREATER_THAN(0, info->count);

        // Should have a primary interface (could be -1 if only loopback)
        TEST_ASSERT_GREATER_OR_EQUAL(-1, info->primary_index);

        // Validate interface names are not empty
        for (int i = 0; i < info->count; i++) {
            TEST_ASSERT_GREATER_THAN(0, strlen(info->interfaces[i].name));
            TEST_ASSERT_LESS_THAN(IF_NAMESIZE, strlen(info->interfaces[i].name));
        }

        // Clean up
        free_network_info(info);
    }
}

// Test NULL safety of get_network_info
void test_get_network_info_null_safety(void) {
    // This function should always return a valid structure or NULL
    // We can't really test the NULL case without mocking getifaddrs
    network_info_t *info = get_network_info();

    // Should either succeed or fail gracefully
    if (info) {
        TEST_ASSERT_GREATER_OR_EQUAL(0, info->count);
        free_network_info(info);
    } else {
        // If it returns NULL, that's also acceptable
        TEST_PASS();
    }
}

// Test interface counting and validation
void test_get_network_info_interface_counting(void) {
    network_info_t *info = get_network_info();

    TEST_ASSERT_NOT_NULL(info);

    if (info) {
        // Should not exceed maximum interface count
        TEST_ASSERT_LESS_OR_EQUAL(MAX_INTERFACES, info->count);

        // Each interface should have valid data
        for (int i = 0; i < info->count; i++) {
            // Name should not be empty
            TEST_ASSERT_GREATER_THAN(0, strlen(info->interfaces[i].name));

            // IP count should be valid
            TEST_ASSERT_GREATER_OR_EQUAL(0, info->interfaces[i].ip_count);
            TEST_ASSERT_LESS_OR_EQUAL(MAX_IPS, info->interfaces[i].ip_count);

            // Each IP should be valid
            for (int j = 0; j < info->interfaces[i].ip_count; j++) {
                TEST_ASSERT_GREATER_THAN(0, strlen(info->interfaces[i].ips[j]));
                TEST_ASSERT_LESS_THAN(INET6_ADDRSTRLEN, strlen(info->interfaces[i].ips[j]));
            }
        }

        free_network_info(info);
    }
}

// Test IPv4/IPv6 detection
void test_get_network_info_ipv4_ipv6_detection(void) {
    network_info_t *info = get_network_info();

    TEST_ASSERT_NOT_NULL(info);

    if (info) {
        // Just verify that the IPv6 detection doesn't crash and produces reasonable results
        // The detection might not be perfect, but it should be consistent
        for (int i = 0; i < info->count; i++) {
            for (int j = 0; j < info->interfaces[i].ip_count; j++) {
                // Check that IP address is valid (no crash)
                TEST_ASSERT_NOT_NULL(info->interfaces[i].ips[j]);

                // Check that IP string is valid
                TEST_ASSERT_GREATER_THAN(0, strlen(info->interfaces[i].ips[j]));
            }
        }

        free_network_info(info);
    }
}

// Test free_network_info NULL safety
void test_free_network_info_null_safety(void) {
    // Should not crash when passed NULL
    free_network_info(NULL);
    TEST_PASS();
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_get_network_info_basic_discovery);
    RUN_TEST(test_get_network_info_null_safety);
    RUN_TEST(test_get_network_info_interface_counting);
    RUN_TEST(test_get_network_info_ipv4_ipv6_detection);
    RUN_TEST(test_free_network_info_null_safety);

    return UNITY_END();
}
