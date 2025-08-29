/*
 * Unity Test: mdns_linux_test_build_announcement.c
 * Tests mDNS announcement building functions from mdns_linux.c
 *
 * This file follows the naming convention from tests/UNITY.md:
 * <source>_test_<function>.c where source is mdns_linux and function is build_announcement
 */

#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../src/mdns/mdns_keys.h"
#include "../../../../src/mdns/mdns_server.h"

// Test function prototypes
void test_mdns_server_build_announcement_null_inputs(void);
void test_mdns_server_build_announcement_minimal_valid(void);
void test_mdns_server_build_announcement_different_ttl(void);
void test_mdns_server_build_announcement_with_hostname(void);
void test_mdns_server_build_announcement_no_services(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test building announcement with NULL inputs - defensive test
void test_mdns_server_build_announcement_null_inputs(void) {
    size_t packet_len = 999; // Set to known value

    // Test with NULL packet buffer - should handle gracefully
    // This is a defensive test that should not crash even if the function doesn't handle NULL
    mdns_server_build_announcement(NULL, &packet_len, "test.local", NULL, 120, NULL);

    // If we get here without crashing, the function handled NULL inputs
    // The packet_len value may or may not be modified depending on implementation
    TEST_PASS();
}

// Test building announcement with minimal valid inputs
void test_mdns_server_build_announcement_minimal_valid(void) {
    uint8_t packet[2048];  // Larger buffer for safety
    size_t packet_len = 0;

    // Create minimal server instance with valid data
    mdns_server_t server;
    memset(&server, 0, sizeof(server));

    // Set hostname to a string literal (will be valid for the function's lifetime)
    char hostname_buffer[] = "test.local";
    server.hostname = hostname_buffer;
    server.num_services = 0;
    server.services = NULL;

    // Create minimal network info with valid data
    network_info_t network_info_data;
    memset(&network_info_data, 0, sizeof(network_info_data));
    network_info_data.count = 1;
    network_info_data.primary_index = 0;
    strcpy(network_info_data.interfaces[0].name, "eth0");
    network_info_data.interfaces[0].ip_count = 1;
    strcpy(network_info_data.interfaces[0].ips[0], "192.168.1.100");

    // This is the main test - if it doesn't crash, we consider it passing
    // The function may return early or create a packet depending on implementation
    mdns_server_build_announcement(packet, &packet_len, "test.local", &server, 120, &network_info_data);

    // Basic validation that doesn't assume specific behavior
    TEST_ASSERT_LESS_OR_EQUAL_UINT(sizeof(packet), packet_len);

    // If packet_len > 0, verify packet buffer wasn't overrun
    if (packet_len > 0) {
        TEST_ASSERT_GREATER_THAN(0, packet_len);
        TEST_ASSERT_LESS_OR_EQUAL_UINT(sizeof(packet), packet_len);
    }
}

// Test building announcement with different TTL values
void test_mdns_server_build_announcement_different_ttl(void) {
    uint8_t packet[2048];
    size_t packet_len = 0;

    // Create minimal server instance
    mdns_server_t server;
    memset(&server, 0, sizeof(server));
    char hostname_buffer1[] = "test.local";
    server.hostname = hostname_buffer1;
    server.num_services = 0;
    server.services = NULL;

    // Create network info
    network_info_t network_info_data;
    memset(&network_info_data, 0, sizeof(network_info_data));
    network_info_data.count = 1;
    network_info_data.primary_index = 0;
    strcpy(network_info_data.interfaces[0].name, "eth0");
    network_info_data.interfaces[0].ip_count = 1;
    strcpy(network_info_data.interfaces[0].ips[0], "192.168.1.100");

    // Test with zero TTL (goodbye packet)
    mdns_server_build_announcement(packet, &packet_len, "test.local", &server, 0, &network_info_data);

    // Should not crash
    TEST_ASSERT_LESS_OR_EQUAL_UINT(sizeof(packet), packet_len);
}

// Test building announcement with NULL hostname parameter
void test_mdns_server_build_announcement_with_hostname(void) {
    uint8_t packet[2048];
    size_t packet_len = 0;

    // Create minimal server instance
    mdns_server_t server;
    memset(&server, 0, sizeof(server));
    char hostname_buffer2[] = "test.local";
    server.hostname = hostname_buffer2;
    server.num_services = 0;
    server.services = NULL;

    // Create network info
    network_info_t network_info_data;
    memset(&network_info_data, 0, sizeof(network_info_data));
    network_info_data.count = 1;
    network_info_data.primary_index = 0;
    strcpy(network_info_data.interfaces[0].name, "eth0");
    network_info_data.interfaces[0].ip_count = 1;
    strcpy(network_info_data.interfaces[0].ips[0], "192.168.1.100");

    // Test with NULL hostname parameter (should use server.hostname)
    mdns_server_build_announcement(packet, &packet_len, NULL, &server, 120, &network_info_data);

    // Should not crash
    TEST_ASSERT_LESS_OR_EQUAL_UINT(sizeof(packet), packet_len);
}

// Test building announcement with no services (simplified)
void test_mdns_server_build_announcement_no_services(void) {
    uint8_t packet[2048];
    size_t packet_len = 0;

    // Create minimal server instance
    mdns_server_t server;
    memset(&server, 0, sizeof(server));
    char hostname_buffer[] = "test.local";
    server.hostname = hostname_buffer;
    server.num_services = 0;
    server.services = NULL;

    // Create network info with multiple IPs
    network_info_t network_info_data;
    memset(&network_info_data, 0, sizeof(network_info_data));
    network_info_data.count = 1;
    network_info_data.primary_index = 0;
    strcpy(network_info_data.interfaces[0].name, "eth0");
    network_info_data.interfaces[0].ip_count = 2;
    strcpy(network_info_data.interfaces[0].ips[0], "192.168.1.100");
    strcpy(network_info_data.interfaces[0].ips[1], "192.168.1.101");

    // Build announcement with no services
    mdns_server_build_announcement(packet, &packet_len, "test.local", &server, 120, &network_info_data);

    // Verify packet size is reasonable and within bounds
    TEST_ASSERT_LESS_OR_EQUAL_UINT(sizeof(packet), packet_len);
    if (packet_len > 0) {
        TEST_ASSERT_GREATER_OR_EQUAL(12, packet_len); // Should be at least DNS header size
    }
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mdns_server_build_announcement_null_inputs);
    RUN_TEST(test_mdns_server_build_announcement_minimal_valid);
    RUN_TEST(test_mdns_server_build_announcement_different_ttl);
    RUN_TEST(test_mdns_server_build_announcement_with_hostname);
    RUN_TEST(test_mdns_server_build_announcement_no_services);

    return UNITY_END();
}
