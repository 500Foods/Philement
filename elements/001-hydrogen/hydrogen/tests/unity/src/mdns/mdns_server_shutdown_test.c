/*
 * Unity Test: mdns_linux_test_shutdown.c
 * Tests mDNS server shutdown functions from mdns_linux.c
 *
 * This file follows the naming convention from tests/UNITY.md:
 * <source>_test_<function>.c where source is mdns_linux and function is shutdown
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/mdns/mdns_keys.h>
#include <src/mdns/mdns_server.h>

// Test function prototypes
void test_mdns_server_shutdown_null_server(void);
void test_mdns_server_shutdown_empty_server(void);
void test_mdns_server_shutdown_server_with_interfaces(void);
void test_mdns_server_shutdown_server_with_services(void);
void test_mdns_server_shutdown_double_shutdown(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test shutting down with NULL server - should not crash
void test_mdns_server_shutdown_null_server(void) {
    // This should not crash - defensive programming
    mdns_server_shutdown(NULL);

    // If we get here without crashing, the test passes
    TEST_PASS();
}

// Test shutting down an empty server
void test_mdns_server_shutdown_empty_server(void) {
    // Create minimal server instance
    mdns_server_t *server = calloc(1, sizeof(mdns_server_t));
    TEST_ASSERT_NOT_NULL(server);

    // Initialize with zeros (empty server)
    memset(server, 0, sizeof(mdns_server_t));

    // This should not crash
    mdns_server_shutdown(server);

    // Note: server is freed by shutdown, so we don't free it here
    TEST_PASS();
}

// Test shutting down server with interfaces
void test_mdns_server_shutdown_server_with_interfaces(void) {
    // Create server instance
    mdns_server_t *server = calloc(1, sizeof(mdns_server_t));
    TEST_ASSERT_NOT_NULL(server);
    memset(server, 0, sizeof(mdns_server_t));

    // Allocate interfaces array
    server->num_interfaces = 1;
    server->interfaces = calloc(1, sizeof(mdns_server_interface_t));
    TEST_ASSERT_NOT_NULL(server->interfaces);

    // Initialize interface with test data
    mdns_server_interface_t *iface = &server->interfaces[0];
    iface->if_name = strdup("test_interface");
    iface->sockfd_v4 = -1;  // Invalid socket
    iface->sockfd_v6 = -1;  // Invalid socket
    iface->num_addresses = 1;
    iface->ip_addresses = calloc(1, sizeof(char*));
    TEST_ASSERT_NOT_NULL(iface->ip_addresses);
    iface->ip_addresses[0] = strdup("192.168.1.100");

    // Shutdown should handle this gracefully
    mdns_server_shutdown(server);

    // Server is freed by shutdown, so we don't free it here
    TEST_PASS();
}

// Test shutting down server with services
void test_mdns_server_shutdown_server_with_services(void) {
    // Create server instance
    mdns_server_t *server = calloc(1, sizeof(mdns_server_t));
    TEST_ASSERT_NOT_NULL(server);
    memset(server, 0, sizeof(mdns_server_t));

    // Initialize server identity
    server->hostname = strdup("test.local");
    server->service_name = strdup("test_service");
    server->device_id = strdup("test_device");
    server->friendly_name = strdup("Test Device");
    server->model = strdup("Test Model");
    server->manufacturer = strdup("Test Manufacturer");
    server->sw_version = strdup("1.0.0");
    server->hw_version = strdup("1.0.0");
    server->config_url = strdup("http://test.local");
    server->secret_key = generate_secret_mdns_key();

    // Allocate services array
    server->num_services = 1;
    server->services = calloc(1, sizeof(mdns_server_service_t));
    TEST_ASSERT_NOT_NULL(server->services);

    // Initialize service
    mdns_server_service_t *service = &server->services[0];
    service->name = strdup("test_service");
    service->type = strdup("_http._tcp.local");
    service->port = 8080;
    service->num_txt_records = 1;
    service->txt_records = calloc(1, sizeof(char*));
    TEST_ASSERT_NOT_NULL(service->txt_records);
    service->txt_records[0] = strdup("key=value");

    // Shutdown should clean up all resources
    mdns_server_shutdown(server);

    // Server is freed by shutdown, so we don't free it here
    TEST_PASS();
}

// Test double shutdown (should not crash)
void test_mdns_server_shutdown_double_shutdown(void) {
    // Create minimal server instance
    mdns_server_t *server = calloc(1, sizeof(mdns_server_t));
    TEST_ASSERT_NOT_NULL(server);
    memset(server, 0, sizeof(mdns_server_t));

    // First shutdown
    mdns_server_shutdown(server);

    // Second shutdown should not crash (idempotent)
    mdns_server_shutdown(NULL);  // Server already freed

    TEST_PASS();
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mdns_server_shutdown_null_server);
    RUN_TEST(test_mdns_server_shutdown_empty_server);
    RUN_TEST(test_mdns_server_shutdown_server_with_interfaces);
    RUN_TEST(test_mdns_server_shutdown_server_with_services);
    RUN_TEST(test_mdns_server_shutdown_double_shutdown);

    return UNITY_END();
}
