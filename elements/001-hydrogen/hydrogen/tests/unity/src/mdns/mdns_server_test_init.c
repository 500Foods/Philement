/*
 * Unity Test: mdns_linux_test_init.c
 * Tests mdns_server_init function for maximum coverage impact
 * This function is large and covers significant portions of the codebase
 */

#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../src/mdns/mdns_keys.h"
#include "../../../../src/mdns/mdns_server.h"
#include "../../../../src/network/network.h"

// Test function prototypes
void test_mdns_server_init_basic_success(void);
void test_mdns_server_init_with_ipv6(void);
void test_mdns_server_init_multiple_services(void);
void test_mdns_server_init_empty_services(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
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
    TEST_ASSERT_EQUAL_STRING("TestApp", server->service_name);
    TEST_ASSERT_EQUAL_STRING("test123", server->device_id);
    TEST_ASSERT_EQUAL_STRING("TestPrinter", server->friendly_name);

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
    TEST_ASSERT_EQUAL(2, server->num_services);

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
    TEST_ASSERT_EQUAL(0, server->num_services);

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

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mdns_server_init_basic_success);
    RUN_TEST(test_mdns_server_init_with_ipv6);
    RUN_TEST(test_mdns_server_init_multiple_services);
    RUN_TEST(test_mdns_server_init_empty_services);

    return UNITY_END();
}
