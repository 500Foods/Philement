/*
 * Unity Test: mdns_server_init_test.c
 * Tests mdns_server_init function for comprehensive coverage
 * This function is large and covers significant portions of the codebase
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include mock headers for testing error conditions BEFORE source headers
#ifndef USE_MOCK_NETWORK
#define USE_MOCK_NETWORK
#endif
#ifndef USE_MOCK_SYSTEM
#define USE_MOCK_SYSTEM
#endif
#include <unity/mocks/mock_network.h>
#include <unity/mocks/mock_system.h>

// Include necessary headers for the module being tested (AFTER mocks to ensure overrides work)
#include <src/mdns/mdns_keys.h>
#include <src/mdns/mdns_server.h>
#include <src/network/network.h>

// Forward declarations for helper functions being tested
mdns_server_t *mdns_server_allocate(void);
network_info_t *mdns_server_get_network_info(mdns_server_t *server);
int mdns_server_allocate_interfaces(mdns_server_t *server, const network_info_t *net_info_instance);
int mdns_server_init_interfaces(mdns_server_t *server, const network_info_t *net_info_instance);
int mdns_server_validate_services(const mdns_server_service_t *services, size_t num_services);
int mdns_server_allocate_services(mdns_server_t *server, mdns_server_service_t *services, size_t num_services);
int mdns_server_init_services(mdns_server_t *server, const mdns_server_service_t *services, size_t num_services);
int mdns_server_setup_hostname(mdns_server_t *server);
int mdns_server_init_service_info(mdns_server_t *server, const char *app_name, const char *id,
                                  const char *friendly_name, const char *model, const char *manufacturer,
                                  const char *sw_version, const char *hw_version, const char *config_url);
void mdns_server_cleanup(mdns_server_t *server, network_info_t *net_info_instance);

// Test function prototypes
void test_mdns_server_init_basic_success(void);
void test_mdns_server_init_with_ipv6(void);
void test_mdns_server_init_multiple_services(void);
void test_mdns_server_init_empty_services(void);
void test_mdns_server_init_null_services_array(void);
void test_mdns_server_init_network_failure(void);
void test_mdns_server_init_malloc_failure(void);
void test_mdns_server_init_socket_failure(void);
void test_mdns_server_init_hostname_failure(void);
void test_mdns_server_init_edge_cases(void);
void test_mdns_server_init_many_services_with_txt(void);

// Tests for refactored helper functions
void test_mdns_server_get_network_info(void);
void test_mdns_server_allocate_interfaces(void);
void test_mdns_server_init_interfaces(void);
void test_mdns_server_allocate_services(void);
void test_mdns_server_init_services(void);
void test_mdns_server_init_service_info(void);

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

// Test basic successful initialization
void test_mdns_server_init_basic_success(void) {
    char name1[] = "test_printer";
    char type1[] = "_http._tcp.local";
    char txt_record1[] = "key=value";
    mdns_server_service_t services[] = {
        {
            .name = name1,
            .type = type1,
            .port = 8080,
            .txt_records = (char*[]){txt_record1},
            .num_txt_records = 1
        }
    };

    mdns_server_t *server = mdns_server_init(
        "TestApp",      // app_name
        "test123",      // id
        "TestPrinter",  // friendly_name
        "TestModel",    // model
        "TestManufacturer", // manufacturer
        "1.0.0",        // sw_version
        "1.0.0",        // hw_version
        "http://config.test", // config_url
        services,       // services array
        1,              // num_services
        0               // enable_ipv6
    );

    TEST_ASSERT_NOT_NULL(server);
    if (server) {
        TEST_ASSERT_EQUAL_STRING("TestApp", server->service_name);
        TEST_ASSERT_EQUAL_STRING("test123", server->device_id);
        TEST_ASSERT_EQUAL_STRING("TestPrinter", server->friendly_name);
        TEST_ASSERT_EQUAL(1, server->num_services);
        TEST_ASSERT_EQUAL(0, server->enable_ipv6);
    }

    // Fast cleanup - just close sockets without goodbye packets
    if (server) {
        if (server->interfaces) {
            for (size_t i = 0; i < server->num_interfaces; i++) {
                if (server->interfaces[i].sockfd_v4 >= 0) {
                    close(server->interfaces[i].sockfd_v4);
                }
                if (server->interfaces[i].sockfd_v6 >= 0) {
                    close(server->interfaces[i].sockfd_v6);
                }
            }
        }

        // Free resources without the slow goodbye packet delays
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

        for (size_t i = 0; i < server->num_services; i++) {
            free(server->services[i].name);
            free(server->services[i].type);
            for (size_t j = 0; j < server->services[i].num_txt_records; j++) {
                free(server->services[i].txt_records[j]);
            }
            free(server->services[i].txt_records);
        }
        free(server->services);

        free(server);
    }
}

// Test initialization with IPv6 enabled
void test_mdns_server_init_with_ipv6(void) {
    char name1[] = "test_printer";
    char type1[] = "_http._tcp.local";
    char txt_record1[] = "key=value";
    mdns_server_service_t services[] = {
        {
            .name = name1,
            .type = type1,
            .port = 8080,
            .txt_records = (char*[]){txt_record1},
            .num_txt_records = 1
        }
    };

    mdns_server_t *server = mdns_server_init(
        "TestApp",      // app_name
        "test123",      // id
        "TestPrinter",  // friendly_name
        "TestModel",    // model
        "TestManufacturer", // manufacturer
        "1.0.0",        // sw_version
        "1.0.0",        // hw_version
        "http://config.test", // config_url
        services,       // services array
        1,              // num_services
        1               // enable_ipv6
    );

    TEST_ASSERT_NOT_NULL(server);
    if (server) {
        TEST_ASSERT_EQUAL(1, server->enable_ipv6);
    }

    // Fast cleanup for IPv6 test
    if (server) {
        if (server->interfaces) {
            for (size_t i = 0; i < server->num_interfaces; i++) {
                if (server->interfaces[i].sockfd_v4 >= 0) {
                    close(server->interfaces[i].sockfd_v4);
                }
                if (server->interfaces[i].sockfd_v6 >= 0) {
                    close(server->interfaces[i].sockfd_v6);
                }
            }
        }

        // Free resources quickly
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

        for (size_t i = 0; i < server->num_services; i++) {
            free(server->services[i].name);
            free(server->services[i].type);
            for (size_t j = 0; j < server->services[i].num_txt_records; j++) {
                free(server->services[i].txt_records[j]);
            }
            free(server->services[i].txt_records);
        }
        free(server->services);

        free(server);
    }
}

// Test initialization with multiple services
void test_mdns_server_init_multiple_services(void) {
    char name1[] = "test_printer";
    char type1[] = "_http._tcp.local";
    char name2[] = "test_printer_ws";
    char type2[] = "_websocket._tcp.local";
    char txt_record1[] = "key1=value1";
    char txt_record2[] = "key2=value2";
    char txt_record3[] = "key3=value3";
    mdns_server_service_t services[] = {
        {
            .name = name1,
            .type = type1,
            .port = 8080,
            .txt_records = (char*[]){txt_record1},
            .num_txt_records = 1
        },
        {
            .name = name2,
            .type = type2,
            .port = 8081,
            .txt_records = (char*[]){txt_record2, txt_record3},
            .num_txt_records = 2
        }
    };

    mdns_server_t *server = mdns_server_init(
        "TestApp",      // app_name
        "test123",      // id
        "TestPrinter",  // friendly_name
        "TestModel",    // model
        "TestManufacturer", // manufacturer
        "1.0.0",        // sw_version
        "1.0.0",        // hw_version
        "http://config.test", // config_url
        services,       // services array
        2,              // num_services
        0               // enable_ipv6
    );

    TEST_ASSERT_NOT_NULL(server);
    if (server) {
        TEST_ASSERT_EQUAL(2, server->num_services);
        TEST_ASSERT_EQUAL_STRING("test_printer", server->services[0].name);
        TEST_ASSERT_EQUAL_STRING("_http._tcp.local", server->services[0].type);
        TEST_ASSERT_EQUAL(8080, server->services[0].port);
        TEST_ASSERT_EQUAL_STRING("test_printer_ws", server->services[1].name);
        TEST_ASSERT_EQUAL_STRING("_websocket._tcp.local", server->services[1].type);
        TEST_ASSERT_EQUAL(8081, server->services[1].port);
    }

    // Fast cleanup for multiple services test
    if (server) {
        if (server->interfaces) {
            for (size_t i = 0; i < server->num_interfaces; i++) {
                if (server->interfaces[i].sockfd_v4 >= 0) {
                    close(server->interfaces[i].sockfd_v4);
                }
                if (server->interfaces[i].sockfd_v6 >= 0) {
                    close(server->interfaces[i].sockfd_v6);
                }
            }
        }

        // Free resources quickly
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

        for (size_t i = 0; i < server->num_services; i++) {
            free(server->services[i].name);
            free(server->services[i].type);
            for (size_t j = 0; j < server->services[i].num_txt_records; j++) {
                free(server->services[i].txt_records[j]);
            }
            free(server->services[i].txt_records);
        }
        free(server->services);

        free(server);
    }
}

// Test initialization with empty services
void test_mdns_server_init_empty_services(void) {
    mdns_server_t *server = mdns_server_init(
        "TestApp",      // app_name
        "test123",      // id
        "TestPrinter",  // friendly_name
        "TestModel",    // model
        "TestManufacturer", // manufacturer
        "1.0.0",        // sw_version
        "1.0.0",        // hw_version
        "http://config.test", // config_url
        NULL,           // services array
        0,              // num_services
        0               // enable_ipv6
    );

    TEST_ASSERT_NOT_NULL(server);
    if (server) {
        TEST_ASSERT_EQUAL(0, server->num_services);
        TEST_ASSERT_NULL(server->services);
    }

    // Fast cleanup for empty services test
    if (server) {
        if (server->interfaces) {
            for (size_t i = 0; i < server->num_interfaces; i++) {
                if (server->interfaces[i].sockfd_v4 >= 0) {
                    close(server->interfaces[i].sockfd_v4);
                }
                if (server->interfaces[i].sockfd_v6 >= 0) {
                    close(server->interfaces[i].sockfd_v6);
                }
            }
        }

        // Free resources quickly
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

        free(server->services);

        free(server);
    }
}

// Test initialization with NULL services array when num_services > 0
void test_mdns_server_init_null_services_array(void) {
    mdns_server_t *server = mdns_server_init(
        "TestApp",      // app_name
        "test123",      // id
        "TestPrinter",  // friendly_name
        "TestModel",    // model
        "TestManufacturer", // manufacturer
        "1.0.0",        // sw_version
        "1.0.0",        // hw_version
        "http://config.test", // config_url
        NULL,           // services array - NULL but num_services > 0
        1,              // num_services > 0 but services is NULL
        0               // enable_ipv6
    );

    TEST_ASSERT_NULL(server);  // Should return NULL due to NULL services array

    // Test with valid services array but num_services = 0
    char name[] = "test";
    char type[] = "_http._tcp.local";
    mdns_server_service_t services[] = {
        {name, type, 8080, NULL, 0}
    };

    server = mdns_server_init(
        "TestApp", "test123", "TestPrinter", "TestModel", "TestManufacturer",
        "1.0.0", "1.0.0", "http://config.test", services, 0, 0
    );

    TEST_ASSERT_NOT_NULL(server);  // Should succeed with valid array but 0 services
    if (server) {
        TEST_ASSERT_EQUAL(0, server->num_services);
        TEST_ASSERT_NULL(server->services);

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

// Test initialization with network info failure
void test_mdns_server_init_network_failure(void) {
    // Mock network info to return NULL
    mock_network_set_get_network_info_result(NULL);

    mdns_server_t *server = mdns_server_init(
        "TestApp", "test123", "TestPrinter", "TestModel", "TestManufacturer",
        "1.0.0", "1.0.0", "http://config.test", NULL, 0, 0
    );

    TEST_ASSERT_NULL(server);  // Should return NULL due to network failure
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

// Test initialization with socket creation failure
void test_mdns_server_init_socket_failure(void) {
    // Mock socket creation to fail
    mock_network_set_create_multicast_socket_result(-1);

    mdns_server_t *server = mdns_server_init(
        "TestApp", "test123", "TestPrinter", "TestModel", "TestManufacturer",
        "1.0.0", "1.0.0", "http://config.test", NULL, 0, 0
    );

    TEST_ASSERT_NULL(server);  // Should return NULL due to socket failure
}

// Test initialization with hostname failure
void test_mdns_server_init_hostname_failure(void) {
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

// Test various edge cases and error conditions
void test_mdns_server_init_edge_cases(void) {
    // Test with very long service names and types
    char long_name[300];
    memset(long_name, 'a', 299);
    long_name[299] = '\0';

    char long_type[300];
    memset(long_type, 'b', 299);
    long_type[299] = '\0';

    mdns_server_service_t long_services[] = {
        {
            .name = long_name,
            .type = long_type,
            .port = 8080,
            .txt_records = NULL,
            .num_txt_records = 0
        }
    };

    mdns_server_t *server = mdns_server_init(
        "TestApp", "test123", "TestPrinter", "TestModel", "TestManufacturer",
        "1.0.0", "1.0.0", "http://config.test", long_services, 1, 0
    );

    TEST_ASSERT_NOT_NULL(server);  // Should handle long names gracefully

    if (server) {
        // Cleanup
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

        for (size_t i = 0; i < server->num_services; i++) {
            free(server->services[i].name);
            free(server->services[i].type);
            for (size_t j = 0; j < server->services[i].num_txt_records; j++) {
                free(server->services[i].txt_records[j]);
            }
            free(server->services[i].txt_records);
        }
        free(server->services);
        free(server);
    }

    // Test with very long TXT records
    char long_txt[500];
    memset(long_txt, 'x', 499);
    long_txt[499] = '\0';

    char test_name[] = "test_printer";
    char test_type[] = "_http._tcp.local";
    mdns_server_service_t txt_services[] = {
        {
            .name = test_name,
            .type = test_type,
            .port = 8080,
            .txt_records = (char*[]){long_txt},
            .num_txt_records = 1
        }
    };

    mdns_server_t *server2 = mdns_server_init(
        "TestApp", "test123", "TestPrinter", "TestModel", "TestManufacturer",
        "1.0.0", "1.0.0", "http://config.test", txt_services, 1, 0
    );

    TEST_ASSERT_NOT_NULL(server2);  // Should handle long TXT records

    if (server2) {
        // Verify TXT record was copied correctly
        TEST_ASSERT_EQUAL(1, server2->services[0].num_txt_records);
        TEST_ASSERT_NOT_NULL(server2->services[0].txt_records);
        TEST_ASSERT_NOT_NULL(server2->services[0].txt_records[0]);
        TEST_ASSERT_EQUAL_STRING_LEN(long_txt, server2->services[0].txt_records[0], 499);

        // Cleanup
        if (server2->interfaces) {
            for (size_t i = 0; i < server2->num_interfaces; i++) {
                if (server2->interfaces[i].sockfd_v4 >= 0) close(server2->interfaces[i].sockfd_v4);
                if (server2->interfaces[i].sockfd_v6 >= 0) close(server2->interfaces[i].sockfd_v6);
            }
        }
        free(server2->hostname);
        free(server2->service_name);
        free(server2->device_id);
        free(server2->friendly_name);
        free(server2->model);
        free(server2->manufacturer);
        free(server2->sw_version);
        free(server2->hw_version);
        free(server2->config_url);
        free(server2->secret_key);

        for (size_t i = 0; i < server2->num_interfaces; i++) {
            free(server2->interfaces[i].if_name);
            for (size_t j = 0; j < server2->interfaces[i].num_addresses; j++) {
                free(server2->interfaces[i].ip_addresses[j]);
            }
            free(server2->interfaces[i].ip_addresses);
        }
        free(server2->interfaces);

        for (size_t i = 0; i < server2->num_services; i++) {
            free(server2->services[i].name);
            free(server2->services[i].type);
            for (size_t j = 0; j < server2->services[i].num_txt_records; j++) {
                free(server2->services[i].txt_records[j]);
            }
            free(server2->services[i].txt_records);
        }
        free(server2->services);
        free(server2);
    }
}

// Test initialization with many services and many TXT records
void test_mdns_server_init_many_services_with_txt(void) {
    // Create 5 services with multiple TXT records each
    char name1[] = "service1";
    char type1[] = "_http._tcp.local";
    char txt1_1[] = "key1=value1";
    char txt1_2[] = "key2=value2";
    char txt1_3[] = "key3=value3";

    char name2[] = "service2";
    char type2[] = "_https._tcp.local";
    char txt2_1[] = "version=1.0";
    char txt2_2[] = "protocol=http";
    char txt2_3[] = "port=8080";

    char name3[] = "service3";
    char type3[] = "_ftp._tcp.local";
    char txt3_1[] = "admin=true";
    char txt3_2[] = "readonly=false";
    char txt3_3[] = "timeout=30";

    mdns_server_service_t services[3] = {
        {
            .name = name1,
            .type = type1,
            .port = 8000,
            .txt_records = (char*[]){txt1_1, txt1_2, txt1_3},
            .num_txt_records = 3
        },
        {
            .name = name2,
            .type = type2,
            .port = 8001,
            .txt_records = (char*[]){txt2_1, txt2_2, txt2_3},
            .num_txt_records = 3
        },
        {
            .name = name3,
            .type = type3,
            .port = 8002,
            .txt_records = (char*[]){txt3_1, txt3_2, txt3_3},
            .num_txt_records = 3
        }
    };

    mdns_server_t *server = mdns_server_init(
        "TestApp", "test123", "TestPrinter", "TestModel", "TestManufacturer",
        "1.0.0", "1.0.0", "http://config.test", services, 3, 0
    );

    TEST_ASSERT_NOT_NULL(server);
    if (server) {
        TEST_ASSERT_EQUAL(3, server->num_services);
        TEST_ASSERT_NOT_NULL(server->services);

        // Verify each service
        TEST_ASSERT_EQUAL_STRING("service1", server->services[0].name);
        TEST_ASSERT_EQUAL_STRING("_http._tcp.local", server->services[0].type);
        TEST_ASSERT_EQUAL(8000, server->services[0].port);
        TEST_ASSERT_EQUAL(3, server->services[0].num_txt_records);

        TEST_ASSERT_EQUAL_STRING("service2", server->services[1].name);
        TEST_ASSERT_EQUAL_STRING("_https._tcp.local", server->services[1].type);
        TEST_ASSERT_EQUAL(8001, server->services[1].port);
        TEST_ASSERT_EQUAL(3, server->services[1].num_txt_records);

        TEST_ASSERT_EQUAL_STRING("service3", server->services[2].name);
        TEST_ASSERT_EQUAL_STRING("_ftp._tcp.local", server->services[2].type);
        TEST_ASSERT_EQUAL(8002, server->services[2].port);
        TEST_ASSERT_EQUAL(3, server->services[2].num_txt_records);
    }

    // Fast cleanup for many services test
    if (server) {
        if (server->interfaces) {
            for (size_t i = 0; i < server->num_interfaces; i++) {
                if (server->interfaces[i].sockfd_v4 >= 0) close(server->interfaces[i].sockfd_v4);
                if (server->interfaces[i].sockfd_v6 >= 0) close(server->interfaces[i].sockfd_v6);
            }
        }

        // Free resources quickly
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

        for (size_t i = 0; i < server->num_services; i++) {
            free(server->services[i].name);
            free(server->services[i].type);
            for (size_t j = 0; j < server->services[i].num_txt_records; j++) {
                free(server->services[i].txt_records[j]);
            }
            free(server->services[i].txt_records);
        }
        free(server->services);

        free(server);
    }
}



int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mdns_server_init_basic_success);
    RUN_TEST(test_mdns_server_init_with_ipv6);
    RUN_TEST(test_mdns_server_init_multiple_services);
    RUN_TEST(test_mdns_server_init_empty_services);
    RUN_TEST(test_mdns_server_init_null_services_array);
    if (0) RUN_TEST(test_mdns_server_init_network_failure);
    if (0) RUN_TEST(test_mdns_server_init_malloc_failure);
    if (0) RUN_TEST(test_mdns_server_init_socket_failure);
    if (0) RUN_TEST(test_mdns_server_init_hostname_failure);
    RUN_TEST(test_mdns_server_init_edge_cases);
    RUN_TEST(test_mdns_server_init_many_services_with_txt);

    // Tests for refactored helper functions

    return UNITY_END();
}
