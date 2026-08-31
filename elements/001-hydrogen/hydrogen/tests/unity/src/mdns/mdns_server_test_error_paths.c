/*
 * Unity Test: mdns_server_test_basic_coverage.c
 * Basic tests for mDNS server functions to ensure build works after landing fix
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/mdns/mdns_keys.h>
#include <src/mdns/mdns_server.h>
#include <src/network/network.h>

void test_close_mdns_server_interfaces_null_server(void);
void test_close_mdns_server_interfaces_null_interfaces(void);
void test_create_multicast_socket_null_interface(void);
void test_create_multicast_socket_empty_interface(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}



void test_close_mdns_server_interfaces_null_server(void) {
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

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_close_mdns_server_interfaces_null_server);
    RUN_TEST(test_close_mdns_server_interfaces_null_interfaces);
    RUN_TEST(test_create_multicast_socket_null_interface);
    RUN_TEST(test_create_multicast_socket_empty_interface);

    return UNITY_END();
}
