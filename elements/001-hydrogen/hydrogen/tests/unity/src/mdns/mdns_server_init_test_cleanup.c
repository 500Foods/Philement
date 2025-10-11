/*
 * Unity Test: mdns_server_init_test_cleanup.c
 * Tests mdns_server_cleanup function
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/mdns/mdns_server.h>

// Forward declarations for helper functions being tested
void mdns_server_cleanup(mdns_server_t *server, network_info_t *net_info_instance);

// Test function prototypes
void test_mdns_server_cleanup_null_server(void);
void test_mdns_server_cleanup_empty_server(void);
void test_mdns_server_cleanup_partial_server(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test mdns_server_cleanup function with NULL server (should not crash)
void test_mdns_server_cleanup_null_server(void) {
    mdns_server_cleanup(NULL, NULL);
    // Should not crash
}

// Test mdns_server_cleanup function with empty server
void test_mdns_server_cleanup_empty_server(void) {
    mdns_server_t *server = malloc(sizeof(mdns_server_t));
    TEST_ASSERT_NOT_NULL(server);

    if (server) {
        // Initialize some fields to test cleanup
        server->num_interfaces = 0;
        server->interfaces = NULL;
        server->num_services = 0;
        server->services = NULL;
        server->hostname = NULL;
        server->service_name = NULL;
        server->device_id = NULL;
        server->friendly_name = NULL;
        server->model = NULL;
        server->manufacturer = NULL;
        server->sw_version = NULL;
        server->hw_version = NULL;
        server->config_url = NULL;
        server->secret_key = NULL;

        // Should not crash with empty server
        mdns_server_cleanup(server, NULL);
    }
}

// Test mdns_server_cleanup function with partially initialized server
void test_mdns_server_cleanup_partial_server(void) {
    mdns_server_t *server = malloc(sizeof(mdns_server_t));
    TEST_ASSERT_NOT_NULL(server);

    if (server) {
        // Set up some basic fields
        server->hostname = strdup("test.local");
        server->service_name = strdup("TestApp");
        server->num_interfaces = 0;
        server->interfaces = NULL;
        server->num_services = 0;
        server->services = NULL;

        // Should not crash with partially initialized server
        mdns_server_cleanup(server, NULL);
    }
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mdns_server_cleanup_null_server);
    RUN_TEST(test_mdns_server_cleanup_empty_server);
    RUN_TEST(test_mdns_server_cleanup_partial_server);

    return UNITY_END();
}