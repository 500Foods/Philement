/*
 * Unity Test: mdns_server_test_basic_coverage.c
 * Basic tests for mDNS server functions to ensure build works after landing fix
 */

#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../src/mdns/mdns_keys.h"
#include "../../../../src/mdns/mdns_server.h"
#include "../../../../src/network/network.h"

// Basic test functions that build properly
void test_read_dns_name_simple_case(void);
void test_read_dns_name_root_domain(void);
void test_read_dns_name_buffer_overflow(void);
void test_close_mdns_server_interfaces_null_server(void);
void test_close_mdns_server_interfaces_null_interfaces(void);
void test_create_multicast_socket_null_interface(void);
void test_create_multicast_socket_empty_interface(void);
void test_write_dns_name_null_input(void);
void test_write_dns_name_simple(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}



// Test read_dns_name with simple domain
void test_read_dns_name_simple_case(void) {
    uint8_t packet[256];
    uint8_t *ptr = packet;

    // Write "test.local" as DNS name format
    *ptr++ = 4; // "test"
    memcpy(ptr, "test", 4); ptr += 4;
    *ptr++ = 5; // "local"
    memcpy(ptr, "local", 5); ptr += 5;
    *ptr++ = 0; // null terminator

    char name[256];
    uint8_t *result = read_dns_name(packet, packet, name, sizeof(name));

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("test.local", name);
}

// Test read_dns_name with root domain (just null)
void test_read_dns_name_root_domain(void) {
    uint8_t packet[2] = {0}; // Just null terminator
    char name[256];
    uint8_t *result = read_dns_name(packet, packet, name, sizeof(name));

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("", name);
}

// Test read_dns_name with buffer overflow prevention
void test_read_dns_name_buffer_overflow(void) {
    uint8_t packet[256];
    uint8_t *ptr = packet;

    // Write very long domain name that exceeds name_len
    *ptr++ = 10;
    memcpy(ptr, "verylongdomain", 14); ptr += 14;
    *ptr++ = 0;

    char name[10]; // Very small buffer
    uint8_t *result = read_dns_name(packet, packet, name, sizeof(name));

    TEST_ASSERT_NULL(result); // Should return NULL due to buffer overflow
}

// Test close_mdns_server_interfaces with NULL server
void test_close_mdns_server_interfaces_null_server(void) {
    // This should not crash
    close_mdns_server_interfaces(NULL);
    TEST_PASS();
}

// Test close_mdns_server_interfaces with NULL interfaces array
void test_close_mdns_server_interfaces_null_interfaces(void) {
    mdns_server_t server;
    server.interfaces = NULL;
    server.num_interfaces = 0;

    // This should not crash
    close_mdns_server_interfaces(&server);
    TEST_PASS();
}

// Test create_multicast_socket with NULL interface name
void test_create_multicast_socket_null_interface(void) {
    int result = create_multicast_socket(AF_INET, "224.0.0.251", NULL);
    TEST_ASSERT_EQUAL(-1, result);
}

// Test create_multicast_socket with empty interface name
void test_create_multicast_socket_empty_interface(void) {
    int result = create_multicast_socket(AF_INET, "224.0.0.251", "");
    TEST_ASSERT_EQUAL(-1, result);
}

// Test write_dns_name with NULL name
void test_write_dns_name_null_input(void) {
    uint8_t buffer[256];
    uint8_t *ptr = buffer;
    uint8_t *result = write_dns_name(ptr, NULL);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(1, result - ptr); // Should just write null byte
    TEST_ASSERT_EQUAL(0, *ptr); // First byte should be null
}

// Test simple write_dns_name
void test_write_dns_name_simple(void) {
    uint8_t buffer[256];
    uint8_t *ptr = buffer;
    uint8_t *result = write_dns_name(ptr, "test.local");

    TEST_ASSERT_NOT_NULL(result);
    // Expected format: [4]test[5]local[0] = 4+1 + 5+1 + 1 = 12 bytes
    TEST_ASSERT_EQUAL(12, result - ptr);

    // Check first label: [4]test
    TEST_ASSERT_EQUAL(4, *ptr++);
    TEST_ASSERT_EQUAL('t', *ptr++);
    TEST_ASSERT_EQUAL('e', *ptr++);
    TEST_ASSERT_EQUAL('s', *ptr++);
    TEST_ASSERT_EQUAL('t', *ptr++);

    // Check second label: [5]local
    TEST_ASSERT_EQUAL(5, *ptr++);
    TEST_ASSERT_EQUAL('l', *ptr++);
    TEST_ASSERT_EQUAL('o', *ptr++);
    TEST_ASSERT_EQUAL('c', *ptr++);
    TEST_ASSERT_EQUAL('a', *ptr++);
    TEST_ASSERT_EQUAL('l', *ptr++);

    // Check null terminator
    TEST_ASSERT_EQUAL(0, *ptr++);
}

int main(void) {
    UNITY_BEGIN();

    // Run tests that exercise uncovered error paths
    RUN_TEST(test_read_dns_name_simple_case);
    RUN_TEST(test_read_dns_name_root_domain);
    RUN_TEST(test_read_dns_name_buffer_overflow);
    RUN_TEST(test_close_mdns_server_interfaces_null_server);
    RUN_TEST(test_close_mdns_server_interfaces_null_interfaces);
    RUN_TEST(test_create_multicast_socket_null_interface);
    RUN_TEST(test_create_multicast_socket_empty_interface);
    RUN_TEST(test_write_dns_name_null_input);
    RUN_TEST(test_write_dns_name_simple);

    return UNITY_END();
}
