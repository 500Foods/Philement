/*
 * Unity Test: mdns_server_init_test_error_paths.c
 * Tests error paths for mdns_server_init function that require mocks
 */

// USE_MOCK_NETWORK, USE_MOCK_SYSTEM, USE_MOCK_THREADS defined by CMake
#include <unity/mocks/mock_network.h>
#include <unity/mocks/mock_system.h>

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/mdns/mdns_keys.h>
#include <src/mdns/mdns_server.h>
#include <src/network/network.h>

// Forward declarations
mdns_server_t *mdns_server_init(const char *app_name, const char *id, const char *friendly_name,
                  const char *model, const char *manufacturer, const char *sw_version,
                  const char *hw_version, const char *config_url,
                  mdns_server_service_t *services, size_t num_services, int enable_ipv6);

// Test function prototypes
void test_mdns_server_init_malloc_failure(void);
void test_mdns_server_init_hostname_failure(void);

void setUp(void) {
    // Reset all mocks to default state
    mock_network_reset_all();
    mock_system_reset_all();
}

void tearDown(void) {
    // Clean up test fixtures, if any
    mock_network_reset_all();
    mock_system_reset_all();
}

// Test initialization with malloc failure
void test_mdns_server_init_malloc_failure(void) {
    // Mock malloc to fail
    mock_system_set_malloc_failure(1);

    mdns_server_t *server = mdns_server_init(
        "TestApp", "test123", "TestPrinter", "TestModel", "TestManufacturer",
        "1.0.0", "1.0.0", "http://config.test", NULL, 0, 0
    );

    TEST_ASSERT_NULL(server);  // Should return NULL due to malloc failure
}

// Test initialization with hostname failure
void test_mdns_server_init_hostname_failure(void) {
    // Set up mock network info
    network_info_t mock_net_info;
    memset(&mock_net_info, 0, sizeof(mock_net_info));
    mock_net_info.count = 1;
    strcpy(mock_net_info.interfaces[0].name, "eth0");
    mock_net_info.interfaces[0].ip_count = 1;
    strcpy(mock_net_info.interfaces[0].ips[0], "192.168.1.100");
    mock_network_set_get_network_info_result(&mock_net_info);
    mock_network_set_filter_enabled_interfaces_result(&mock_net_info);

    // Mock gethostname to fail
    mock_system_set_gethostname_failure(1);

    mdns_server_t *server = mdns_server_init(
        "TestApp", "test123", "TestPrinter", "TestModel", "TestManufacturer",
        "1.0.0", "1.0.0", "http://config.test", NULL, 0, 0
    );

    // Should still succeed with fallback hostname "unknown"
    TEST_ASSERT_NOT_NULL(server);
    if (server) {
        TEST_ASSERT_EQUAL_STRING("unknown.local", server->hostname);

        // Fast cleanup
        if (server->interfaces) {
            for (size_t i = 0; i < server->num_interfaces; i++) {
                if (server->interfaces[i].sockfd_v4 >= 0) close(server->interfaces[i].sockfd_v4);
                if (server->interfaces[i].sockfd_v6 >= 0) close(server->interfaces[i].sockfd_v6);
            }
        }
        free(server->hostname);
        free(server->service_name);
        free(server->device_id);
        free(server->friendly_name);
        free(server->model);
        free(server->manufacturer);
        free(server->sw_version);
        free(server->hw_version);
        free(server->config_url);
        free(server->secret_key);

        for (size_t i = 0; i < server->num_interfaces; i++) {
            free(server->interfaces[i].if_name);
            for (size_t j = 0; j < server->interfaces[i].num_addresses; j++) {
                free(server->interfaces[i].ip_addresses[j]);
            }
            free(server->interfaces[i].ip_addresses);
        }
        free(server->interfaces);
        free(server);
    }
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mdns_server_init_malloc_failure);
    RUN_TEST(test_mdns_server_init_hostname_failure);

    return UNITY_END();
}