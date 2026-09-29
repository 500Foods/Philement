/*
 * Unity Test: mdns_server_init_test_error_paths.c
 * Tests error paths for mdns_server_init and its helper functions
 * that require mocks to trigger failure conditions.
 *
 * USE_MOCK_NETWORK, USE_MOCK_SYSTEM, USE_MOCK_THREADS defined by CMake
 * (IS_MDNS_ERROR_PATHS_TEST match)
 */

#include <unity/mocks/mock_network.h>
#include <unity/mocks/mock_system.h>

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mdns/mdns_keys.h>
#include <src/mdns/mdns_server.h>
#include <src/network/network.h>

// Forward declarations for helper functions - made non-static for testing
mdns_server_t *mdns_server_allocate(void);
network_info_t *mdns_server_get_network_info(mdns_server_t *server);
int mdns_server_allocate_interfaces(mdns_server_t *server, const network_info_t *net_info_instance);
int mdns_server_validate_services(const mdns_server_service_t *services, size_t num_services);
int mdns_server_allocate_services(mdns_server_t *server, mdns_server_service_t *services, size_t num_services);
int mdns_server_init_services(mdns_server_t *server, const mdns_server_service_t *services, size_t num_services);
int mdns_server_setup_hostname(mdns_server_t *server);
int mdns_server_init_service_info(mdns_server_t *server, const char *app_name, const char *id,
                                  const char *friendly_name, const char *model, const char *manufacturer,
                                  const char *sw_version, const char *hw_version, const char *config_url);
void mdns_server_cleanup(mdns_server_t *server, network_info_t *net_info_instance);

// Test function prototypes
void test_mdns_server_init_malloc_failure(void);
void test_mdns_server_init_no_network_info(void);
void test_mdns_server_init_no_enabled_interfaces(void);
void test_mdns_server_get_network_info_zero_interfaces(void);
void test_mdns_server_get_network_info_null_filter_result(void);
void test_mdns_server_validate_services_null_services(void);
void test_mdns_server_allocate_interfaces_malloc_failure(void);
void test_mdns_server_init_services_null_array(void);
void test_mdns_server_init_setup_hostname_gethostname_failure(void);
void test_mdns_server_init_service_info_malloc_failure(void);

void setUp(void) {
    mock_network_reset_all();
    mock_system_reset_all();
}

void tearDown(void) {
    mock_network_reset_all();
    mock_system_reset_all();
}

// Test: malloc failure during server allocation (mdns_server_allocate returns NULL)
void test_mdns_server_init_malloc_failure(void) {
    mock_system_set_malloc_failure(1);

    mdns_server_t *server = mdns_server_init(
        "TestApp", "test123", "TestPrinter", "TestModel", "TestManufacturer",
        "1.0.0", "1.0.0", "http://config.test", NULL, 0, 0
    );

    TEST_ASSERT_NULL(server);
    mock_system_set_malloc_failure(0);
}

// Test: get_network_info returns NULL → mdns_server_get_network_info returns NULL
void test_mdns_server_init_no_network_info(void) {
    // mock_reset_all sets get_network_info result to NULL
    mdns_server_t *server = mdns_server_init(
        "TestApp", "test123", "TestPrinter", "TestModel", "TestManufacturer",
        "1.0.0", "1.0.0", "http://config.test", NULL, 0, 0
    );

    TEST_ASSERT_NULL(server);
}

// Test: filter_enabled_interfaces returns NULL → no enabled interfaces
void test_mdns_server_init_no_enabled_interfaces(void) {
    network_info_t mock_raw_info;
    memset(&mock_raw_info, 0, sizeof(mock_raw_info));
    mock_raw_info.count = 1;
    strcpy(mock_raw_info.interfaces[0].name, "eth0");
    mock_raw_info.interfaces[0].ip_count = 1;
    strcpy(mock_raw_info.interfaces[0].ips[0], "192.168.1.100");

    mock_network_set_get_network_info_result(&mock_raw_info);
    // filter_enabled_interfaces returns NULL by default (mock_reset_all)

    mdns_server_t *server = mdns_server_init(
        "TestApp", "test123", "TestPrinter", "TestModel", "TestManufacturer",
        "1.0.0", "1.0.0", "http://config.test", NULL, 0, 0
    );

    TEST_ASSERT_NULL(server);
}

// Test: filter returns result with count 0 → no usable interfaces
void test_mdns_server_get_network_info_zero_interfaces(void) {
    network_info_t mock_raw_info;
    memset(&mock_raw_info, 0, sizeof(mock_raw_info));
    mock_raw_info.count = 1;
    strcpy(mock_raw_info.interfaces[0].name, "eth0");
    mock_raw_info.interfaces[0].ip_count = 1;
    strcpy(mock_raw_info.interfaces[0].ips[0], "192.168.1.100");

    network_info_t mock_filtered_info;
    memset(&mock_filtered_info, 0, sizeof(mock_filtered_info));
    // count stays 0

    mock_network_set_get_network_info_result(&mock_raw_info);
    mock_network_set_filter_enabled_interfaces_result(&mock_filtered_info);

    mdns_server_t *server = mdns_server_init(
        "TestApp", "test123", "TestPrinter", "TestModel", "TestManufacturer",
        "1.0.0", "1.0.0", "http://config.test", NULL, 0, 0
    );

    TEST_ASSERT_NULL(server);
}

// Test: mdns_server_get_network_info directly with NULL filter result
void test_mdns_server_get_network_info_null_filter_result(void) {
    network_info_t *result = mdns_server_get_network_info(NULL);
    TEST_ASSERT_NULL(result);
}

// Test: validate_services returns -1 with NULL services and num_services > 0
void test_mdns_server_validate_services_null_services(void) {
    int result = mdns_server_validate_services(NULL, 1);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

// Test: mdns_server_allocate_interfaces with malloc failure
void test_mdns_server_allocate_interfaces_malloc_failure(void) {
    network_info_t mock_net_info;
    memset(&mock_net_info, 0, sizeof(mock_net_info));
    mock_net_info.count = 1;
    strcpy(mock_net_info.interfaces[0].name, "eth0");
    mock_net_info.interfaces[0].ip_count = 1;
    strcpy(mock_net_info.interfaces[0].ips[0], "192.168.1.100");

    // Test mdns_server_allocate_interfaces directly with a mock server
    mdns_server_t mock_server;
    memset(&mock_server, 0, sizeof(mock_server));
    mock_server.num_interfaces = 0;

    int result = mdns_server_allocate_interfaces(&mock_server, &mock_net_info);
    TEST_ASSERT_EQUAL_INT(0, result);
    free(mock_server.interfaces);

    // Now test with malloc failure
    mock_server.interfaces = NULL;
    mock_server.num_interfaces = 0;
    mock_system_set_malloc_failure(1);
    result = mdns_server_allocate_interfaces(&mock_server, &mock_net_info);
    TEST_ASSERT_EQUAL_INT(-1, result);
    mock_system_set_malloc_failure(0);
}

// Test: mdns_server_init with NULL services array when num_services > 0
void test_mdns_server_init_services_null_array(void) {
    network_info_t mock_net_info;
    memset(&mock_net_info, 0, sizeof(mock_net_info));
    mock_net_info.count = 1;
    strcpy(mock_net_info.interfaces[0].name, "eth0");
    mock_net_info.interfaces[0].ip_count = 1;
    strcpy(mock_net_info.interfaces[0].ips[0], "192.168.1.100");

    mock_network_set_get_network_info_result(&mock_net_info);
    mock_network_set_filter_enabled_interfaces_result(&mock_net_info);

    mdns_server_t *server = mdns_server_init(
        "TestApp", "test123", "TestPrinter", "TestModel", "TestManufacturer",
        "1.0.0", "1.0.0", "http://config.test", NULL, 1, 0
    );

    TEST_ASSERT_NULL(server);
}

// Test: mdns_server_setup_hostname with gethostname failure
void test_mdns_server_init_setup_hostname_gethostname_failure(void) {
    network_info_t mock_net_info;
    memset(&mock_net_info, 0, sizeof(mock_net_info));
    mock_net_info.count = 1;
    strcpy(mock_net_info.interfaces[0].name, "eth0");
    mock_net_info.interfaces[0].ip_count = 1;
    strcpy(mock_net_info.interfaces[0].ips[0], "192.168.1.100");

    mock_network_set_get_network_info_result(&mock_net_info);
    mock_network_set_filter_enabled_interfaces_result(&mock_net_info);
    mock_network_set_create_multicast_socket_result(0);
    mock_system_set_gethostname_failure(1);

    mdns_server_t *server = mdns_server_init(
        "TestApp", "test123", "TestPrinter", "TestModel", "TestManufacturer",
        "1.0.0", "1.0.0", "http://config.test", NULL, 0, 0
    );

    // Should succeed with fallback hostname "unknown"
    TEST_ASSERT_NOT_NULL(server);
    if (server) {
        TEST_ASSERT_EQUAL_STRING("unknown.local", server->hostname);
    }
}

// Test: mdns_server_init_service_info with malloc failure
void test_mdns_server_init_service_info_malloc_failure(void) {
    mdns_server_t *server = calloc(1, sizeof(mdns_server_t));
    TEST_ASSERT_NOT_NULL(server);

    int result = mdns_server_init_service_info(server, "TestApp", "test123", "TestPrinter",
                                                "TestModel", "TestManufacturer", "1.0.0", "1.0.0", "http://config.test");
    TEST_ASSERT_EQUAL_INT(0, result);

    // Clean up the first successful call's allocations
    free(server->service_name);
    free(server->device_id);
    free(server->friendly_name);
    free(server->model);
    free(server->manufacturer);
    free(server->sw_version);
    free(server->hw_version);
    free(server->config_url);
    free(server->secret_key);
    memset(server, 0, sizeof(mdns_server_t));

    // Force malloc to fail during strdup calls
    mock_system_set_malloc_failure(1);
    result = mdns_server_init_service_info(server, "TestApp", "test123", "TestPrinter",
                                           "TestModel", "TestManufacturer", "1.0.0", "1.0.0", "http://config.test");
    TEST_ASSERT_EQUAL_INT(-1, result);
    mock_system_set_malloc_failure(0);

    free(server);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mdns_server_init_malloc_failure);
    RUN_TEST(test_mdns_server_init_no_network_info);
    RUN_TEST(test_mdns_server_init_no_enabled_interfaces);
    RUN_TEST(test_mdns_server_get_network_info_zero_interfaces);
    RUN_TEST(test_mdns_server_get_network_info_null_filter_result);
    RUN_TEST(test_mdns_server_validate_services_null_services);
    RUN_TEST(test_mdns_server_allocate_interfaces_malloc_failure);
    RUN_TEST(test_mdns_server_init_services_null_array);
    RUN_TEST(test_mdns_server_init_setup_hostname_gethostname_failure);
    RUN_TEST(test_mdns_server_init_service_info_malloc_failure);

    return UNITY_END();
}
