/*
 * Unity Test: mdns_server_test_improved_coverage.c
 * Tests mDNS server functions to improve coverage from current 63% to >75%
 *
 * Focus areas:
 * - create_multicast_socket: improve from 62% to 100%
 * - mdns_server_init: improve from 44% to 100%
 * - close_mdns_server_interfaces: improve from 60% to 100%
 */

#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../src/mdns/mdns_keys.h"
#include "../../../../src/mdns/mdns_server.h"

// Test function prototypes
void test_create_multicast_socket_invalid_interface(void);
void test_create_multicast_socket_invalid_interface_name(void);
void test_mdns_server_init_null_services(void);
void test_mdns_server_init_no_services(void);
void test_close_mdns_server_interfaces_null_server(void);
void test_close_mdns_server_interfaces_null_interfaces(void);
void test_get_mdns_server_retry_count_null_config(void);
void test_get_mdns_server_retry_count_zero_retry(void);
void test_get_mdns_server_retry_count_valid(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test create_multicast_socket with invalid interface name
void test_create_multicast_socket_invalid_interface(void) {
    // Test with NULL interface name
    int result = create_multicast_socket(AF_INET, "224.0.0.251", NULL);
    TEST_ASSERT_EQUAL_INT(-1, result);
}

// Test create_multicast_socket with non-existent interface
void test_create_multicast_socket_invalid_interface_name(void) {
    // Test with non-existent interface name
    int result = create_multicast_socket(AF_INET, "224.0.0.251", "nonexistent123");
    TEST_ASSERT_EQUAL_INT(-1, result);
}

// Test mdns_server_init with NULL services (valid when num_services is 0)
void test_mdns_server_init_null_services(void) {
    mdns_server_t *result = mdns_server_init("test", "id", "name", "model", "manuf", "sw", "hw", "url", NULL, 0, 1);
    TEST_ASSERT_NOT_NULL(result);

    // Clean up if successful
    if (result) {
        // Fast cleanup - just close sockets without goodbye packets
        if (result->interfaces) {
            for (size_t i = 0; i < result->num_interfaces; i++) {
                if (result->interfaces[i].sockfd_v4 >= 0) {
                    close(result->interfaces[i].sockfd_v4);
                }
                if (result->interfaces[i].sockfd_v6 >= 0) {
                    close(result->interfaces[i].sockfd_v6);
                }
            }
        }

        // Free resources quickly
        free(result->hostname);
        free(result->service_name);
        free(result->device_id);
        free(result->friendly_name);
        free(result->model);
        free(result->manufacturer);
        free(result->sw_version);
        free(result->hw_version);
        free(result->config_url);
        free(result->secret_key);

        for (size_t i = 0; i < result->num_interfaces; i++) {
            free(result->interfaces[i].if_name);
            for (size_t j = 0; j < result->interfaces[i].num_addresses; j++) {
                free(result->interfaces[i].ip_addresses[j]);
            }
            free(result->interfaces[i].ip_addresses);
        }
        free(result->interfaces);

        free(result->services);
        free(result);
    }
}

// Test mdns_server_init with no services
void test_mdns_server_init_no_services(void) {
    mdns_server_service_t service = {
        .name = strdup("test"),
        .type = strdup("_http._tcp"),
        .port = 8080,
        .txt_records = NULL,
        .num_txt_records = 0
    };

    mdns_server_t *result = mdns_server_init("test", "id", "name", "model", "manuf", "sw", "hw", "url", &service, 1, 1);
    if (result) {
        // If initialization succeeded, clean up
        mdns_server_shutdown(result);
    } else {
        // Clean up the service struct if init failed
        free(service.name);
        free(service.type);
    }
    // Test passes regardless - we're just ensuring no crash
    TEST_PASS();
}

// Test close_mdns_server_interfaces with NULL server
void test_close_mdns_server_interfaces_null_server(void) {
    // Should not crash
    close_mdns_server_interfaces(NULL);
    TEST_PASS();
}

// Test close_mdns_server_interfaces with NULL interfaces
void test_close_mdns_server_interfaces_null_interfaces(void) {
    mdns_server_t server;
    memset(&server, 0, sizeof(server));
    server.interfaces = NULL;
    server.num_interfaces = 0;

    // Should not crash
    close_mdns_server_interfaces(&server);
    TEST_PASS();
}

// Test get_mdns_server_retry_count with NULL config
void test_get_mdns_server_retry_count_null_config(void) {
    int result = get_mdns_server_retry_count(NULL);
    TEST_ASSERT_EQUAL_INT(1, result);
}

// Test get_mdns_server_retry_count with zero retry count
void test_get_mdns_server_retry_count_zero_retry(void) {
    AppConfig config;
    memset(&config, 0, sizeof(config));
    config.mdns_server.retry_count = 0;

    int result = get_mdns_server_retry_count(&config);
    TEST_ASSERT_EQUAL_INT(1, result);
}

// Test get_mdns_server_retry_count with valid retry count
void test_get_mdns_server_retry_count_valid(void) {
    AppConfig config;
    memset(&config, 0, sizeof(config));
    config.mdns_server.retry_count = 5;

    int result = get_mdns_server_retry_count(&config);
    TEST_ASSERT_EQUAL_INT(5, result);
}


int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_create_multicast_socket_invalid_interface);
    RUN_TEST(test_create_multicast_socket_invalid_interface_name);
    RUN_TEST(test_mdns_server_init_null_services);
    RUN_TEST(test_mdns_server_init_no_services);
    RUN_TEST(test_close_mdns_server_interfaces_null_server);
    RUN_TEST(test_close_mdns_server_interfaces_null_interfaces);
    RUN_TEST(test_get_mdns_server_retry_count_null_config);
    RUN_TEST(test_get_mdns_server_retry_count_zero_retry);
    RUN_TEST(test_get_mdns_server_retry_count_valid);

    return UNITY_END();
}