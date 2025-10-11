/*
 * Unity Test: mdns_linux_test_send_announcement.c
 * Tests mdns_server_send_announcement function for final coverage push
 * This function handles sending announcements on all interfaces
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/mdns/mdns_keys.h>
#include <src/mdns/mdns_server.h>
#include <src/network/network.h>

// Test function prototypes
void test_mdns_server_send_announcement_no_interfaces(void);
void test_mdns_server_send_announcement_with_interfaces(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test sending announcements with no interfaces
void test_mdns_server_send_announcement_no_interfaces(void) {
    mdns_server_t server = {
        .num_interfaces = 0,
        .interfaces = NULL
    };

    // This should not crash and should handle the empty case gracefully
    mdns_server_send_announcement(&server, NULL);

    // Test passes if no crash occurs
    TEST_ASSERT_TRUE(1);
}

// Test sending announcements with mock interfaces
void test_mdns_server_send_announcement_with_interfaces(void) {
    // Create a minimal server with one interface but invalid sockets
    char if_name[] = "lo";
    char hostname[] = "test.local";
    mdns_server_interface_t interface = {
        .if_name = if_name,
        .sockfd_v4 = -1,  // Invalid socket
        .sockfd_v6 = -1,  // Invalid socket
        .ip_addresses = NULL,
        .num_addresses = 0
    };

    mdns_server_t server = {
        .num_interfaces = 1,
        .interfaces = &interface,
        .hostname = hostname
    };

    // This should handle invalid sockets gracefully without crashing
    mdns_server_send_announcement(&server, NULL);

    // Test passes if no crash occurs
    TEST_ASSERT_TRUE(1);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mdns_server_send_announcement_no_interfaces);
    RUN_TEST(test_mdns_server_send_announcement_with_interfaces);

    return UNITY_END();
}
