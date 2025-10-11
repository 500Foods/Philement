/*
 * Unity Test: mdns_server_init_test_comprehensive_cleanup.c
 * Tests mdns_server_cleanup function with fully initialized server structures
 * This test specifically targets the massive coverage gap in the cleanup function
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/mdns/mdns_server.h>
#include <src/network/network.h>

// Forward declarations for helper functions being tested
void mdns_server_cleanup(mdns_server_t *server, network_info_t *net_info_instance);

// Helper function prototype
mdns_server_t *create_test_server_with_interfaces_and_services(void);

// Test function prototypes
void test_mdns_server_cleanup_fully_initialized_server(void);
void test_mdns_server_cleanup_server_with_interfaces(void);
void test_mdns_server_cleanup_server_with_services(void);
void test_mdns_server_cleanup_server_with_all_fields(void);
void test_mdns_server_cleanup_with_network_info(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Helper function to create a fully initialized server for cleanup testing
mdns_server_t *create_test_server_with_interfaces_and_services(void) {
    mdns_server_t *server = malloc(sizeof(mdns_server_t));
    if (!server) return NULL;
    
    // Initialize all basic fields
    server->hostname = strdup("test.local");
    server->service_name = strdup("TestApp");
    server->device_id = strdup("test123");
    server->friendly_name = strdup("Test Printer");
    server->model = strdup("TestModel");
    server->manufacturer = strdup("TestCorp");
    server->sw_version = strdup("1.0.0");
    server->hw_version = strdup("HW-1.0");
    server->config_url = strdup("http://config.test");
    server->secret_key = strdup("secret123");
    server->enable_ipv6 = 0;
    
    // Initialize interfaces (2 interfaces)
    server->num_interfaces = 2;
    server->interfaces = malloc(sizeof(mdns_server_interface_t) * 2);
    
    // Interface 1
    server->interfaces[0].if_name = strdup("eth0");
    server->interfaces[0].num_addresses = 2;
    server->interfaces[0].ip_addresses = malloc(sizeof(char*) * 2);
    server->interfaces[0].ip_addresses[0] = strdup("192.168.1.100");
    server->interfaces[0].ip_addresses[1] = strdup("10.0.0.50");
    server->interfaces[0].sockfd_v4 = -1;  // Use -1 to avoid actual socket
    server->interfaces[0].sockfd_v6 = -1;
    server->interfaces[0].consecutive_failures = 0;
    server->interfaces[0].disabled = 0;
    
    // Interface 2
    server->interfaces[1].if_name = strdup("wlan0");
    server->interfaces[1].num_addresses = 1;
    server->interfaces[1].ip_addresses = malloc(sizeof(char*) * 1);
    server->interfaces[1].ip_addresses[0] = strdup("192.168.0.200");
    server->interfaces[1].sockfd_v4 = -1;  // Use -1 to avoid actual socket
    server->interfaces[1].sockfd_v6 = -1;
    server->interfaces[1].consecutive_failures = 0;
    server->interfaces[1].disabled = 0;
    
    // Initialize services (2 services)
    server->num_services = 2;
    server->services = malloc(sizeof(mdns_server_service_t) * 2);
    
    // Service 1 with TXT records
    server->services[0].name = strdup("printer_http");
    server->services[0].type = strdup("_http._tcp.local");
    server->services[0].port = 8080;
    server->services[0].num_txt_records = 2;
    server->services[0].txt_records = malloc(sizeof(char*) * 2);
    server->services[0].txt_records[0] = strdup("path=/api");
    server->services[0].txt_records[1] = strdup("version=1.0");
    
    // Service 2 with TXT records
    server->services[1].name = strdup("printer_ws");
    server->services[1].type = strdup("_websocket._tcp.local");
    server->services[1].port = 8081;
    server->services[1].num_txt_records = 1;
    server->services[1].txt_records = malloc(sizeof(char*) * 1);
    server->services[1].txt_records[0] = strdup("protocol=websocket");
    
    return server;
}

// Test mdns_server_cleanup with fully initialized server structure
void test_mdns_server_cleanup_fully_initialized_server(void) {
    mdns_server_t *server = create_test_server_with_interfaces_and_services();
    TEST_ASSERT_NOT_NULL(server);
    
    // This call should exercise all the cleanup paths
    mdns_server_cleanup(server, NULL);
    
    // If we reach here without crashing, the cleanup worked
    TEST_PASS();
}

// Test mdns_server_cleanup with server that has interfaces but no services
void test_mdns_server_cleanup_server_with_interfaces(void) {
    mdns_server_t *server = malloc(sizeof(mdns_server_t));
    TEST_ASSERT_NOT_NULL(server);
    
    // Initialize only interface-related fields
    server->hostname = strdup("test.local");
    server->service_name = strdup("TestApp");
    server->device_id = strdup("test123");
    server->friendly_name = strdup("Test Printer");
    server->model = strdup("TestModel");
    server->manufacturer = strdup("TestCorp");
    server->sw_version = strdup("1.0.0");
    server->hw_version = strdup("HW-1.0");
    server->config_url = strdup("http://config.test");
    server->secret_key = strdup("secret123");
    
    // Initialize interfaces
    server->num_interfaces = 1;
    server->interfaces = malloc(sizeof(mdns_server_interface_t));
    server->interfaces[0].if_name = strdup("eth0");
    server->interfaces[0].num_addresses = 1;
    server->interfaces[0].ip_addresses = malloc(sizeof(char*) * 1);
    server->interfaces[0].ip_addresses[0] = strdup("192.168.1.100");
    server->interfaces[0].sockfd_v4 = -1;  // Use -1 to avoid actual socket
    server->interfaces[0].sockfd_v6 = -1;
    
    // No services
    server->num_services = 0;
    server->services = NULL;
    
    // This should cleanup interfaces but skip services
    mdns_server_cleanup(server, NULL);
    
    TEST_PASS();
}

// Test mdns_server_cleanup with server that has services but no interfaces
void test_mdns_server_cleanup_server_with_services(void) {
    mdns_server_t *server = malloc(sizeof(mdns_server_t));
    TEST_ASSERT_NOT_NULL(server);
    
    // Initialize only service-related fields
    server->hostname = strdup("test.local");
    server->service_name = strdup("TestApp");
    server->device_id = strdup("test123");
    server->friendly_name = strdup("Test Printer");
    server->model = strdup("TestModel");
    server->manufacturer = strdup("TestCorp");
    server->sw_version = strdup("1.0.0");
    server->hw_version = strdup("HW-1.0");
    server->config_url = strdup("http://config.test");
    server->secret_key = strdup("secret123");
    
    // No interfaces
    server->num_interfaces = 0;
    server->interfaces = NULL;
    
    // Initialize services
    server->num_services = 1;
    server->services = malloc(sizeof(mdns_server_service_t));
    server->services[0].name = strdup("test_service");
    server->services[0].type = strdup("_http._tcp.local");
    server->services[0].port = 8080;
    server->services[0].num_txt_records = 1;
    server->services[0].txt_records = malloc(sizeof(char*) * 1);
    server->services[0].txt_records[0] = strdup("test=value");
    
    // This should cleanup services but skip interfaces
    mdns_server_cleanup(server, NULL);
    
    TEST_PASS();
}

// Test mdns_server_cleanup with all fields populated
void test_mdns_server_cleanup_server_with_all_fields(void) {
    mdns_server_t *server = create_test_server_with_interfaces_and_services();
    TEST_ASSERT_NOT_NULL(server);
    
    // Verify all fields are populated before cleanup
    TEST_ASSERT_NOT_NULL(server->hostname);
    TEST_ASSERT_NOT_NULL(server->service_name);
    TEST_ASSERT_NOT_NULL(server->interfaces);
    TEST_ASSERT_NOT_NULL(server->services);
    TEST_ASSERT_EQUAL(2, server->num_interfaces);
    TEST_ASSERT_EQUAL(2, server->num_services);
    
    // This should exercise all cleanup branches
    mdns_server_cleanup(server, NULL);
    
    TEST_PASS();
}

// Test mdns_server_cleanup with network_info_instance
void test_mdns_server_cleanup_with_network_info(void) {
    mdns_server_t *server = malloc(sizeof(mdns_server_t));
    TEST_ASSERT_NOT_NULL(server);
    
    // Initialize minimal server
    server->hostname = strdup("test.local");
    server->service_name = NULL;
    server->device_id = NULL;
    server->friendly_name = NULL;
    server->model = NULL;
    server->manufacturer = NULL;
    server->sw_version = NULL;
    server->hw_version = NULL;
    server->config_url = NULL;
    server->secret_key = NULL;
    server->num_interfaces = 0;
    server->interfaces = NULL;
    server->num_services = 0;
    server->services = NULL;
    
    // Create a dummy network_info_t to test that branch
    network_info_t *test_net_info = malloc(sizeof(network_info_t));
    TEST_ASSERT_NOT_NULL(test_net_info);
    test_net_info->count = 0;
    test_net_info->primary_index = -1;
    
    // This should cleanup both server and network_info
    mdns_server_cleanup(server, test_net_info);
    
    TEST_PASS();
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mdns_server_cleanup_fully_initialized_server);
    RUN_TEST(test_mdns_server_cleanup_server_with_interfaces);
    RUN_TEST(test_mdns_server_cleanup_server_with_services);
    RUN_TEST(test_mdns_server_cleanup_server_with_all_fields);
    RUN_TEST(test_mdns_server_cleanup_with_network_info);

    return UNITY_END();
}