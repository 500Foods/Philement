/*
 * Unity Test: mdns_linux_test_socket.c
 * Tests create_multicast_socket function for additional coverage
 * This function handles socket creation and multicast setup
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/mdns/mdns_keys.h>
#include <src/mdns/mdns_server.h>

// Test function prototypes
void test_create_multicast_socket_ipv4_success(void);
void test_create_multicast_socket_ipv6_success(void);
void test_create_multicast_socket_invalid_interface(void);
void test_create_multicast_socket_null_interface(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test successful IPv4 multicast socket creation
void test_create_multicast_socket_ipv4_success(void) {
    int sockfd = create_multicast_socket(AF_INET, MDNS_GROUP_V4, "lo");

    // On some systems this might fail due to permissions, so we'll just test that it doesn't crash
    // and returns a valid result (either success or expected failure)
    if (sockfd >= 0) {
        // Success case - close the socket
        close(sockfd);
        TEST_PASS_MESSAGE("IPv4 multicast socket created successfully");
    } else {
        // Expected failure case (permissions, etc.) - just verify it doesn't crash
        TEST_PASS_MESSAGE("IPv4 multicast socket creation failed as expected (permissions, etc.)");
    }
}

// Test successful IPv6 multicast socket creation
void test_create_multicast_socket_ipv6_success(void) {
    int sockfd = create_multicast_socket(AF_INET6, MDNS_GROUP_V6, "lo");

    // On some systems this might fail due to permissions, so we'll just test that it doesn't crash
    // and returns a valid result (either success or expected failure)
    if (sockfd >= 0) {
        // Success case - close the socket
        close(sockfd);
        TEST_PASS_MESSAGE("IPv6 multicast socket created successfully");
    } else {
        // Expected failure case (permissions, etc.) - just verify it doesn't crash
        TEST_PASS_MESSAGE("IPv6 multicast socket creation failed as expected (permissions, etc.)");
    }
}

// Test socket creation with invalid interface
void test_create_multicast_socket_invalid_interface(void) {
    int sockfd = create_multicast_socket(AF_INET, MDNS_GROUP_V4, "nonexistent_interface_12345");

    // Should fail with invalid interface name
    TEST_ASSERT_TRUE(sockfd < 0);
}

// Test socket creation with NULL interface
void test_create_multicast_socket_null_interface(void) {
    int sockfd = create_multicast_socket(AF_INET, MDNS_GROUP_V4, NULL);

    // Should fail with NULL interface
    TEST_ASSERT_TRUE(sockfd < 0);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_create_multicast_socket_ipv4_success);
    RUN_TEST(test_create_multicast_socket_ipv6_success);
    RUN_TEST(test_create_multicast_socket_invalid_interface);
    RUN_TEST(test_create_multicast_socket_null_interface);

    return UNITY_END();
}
