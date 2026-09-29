/*
 * Unity Test: mdns_server_test_coverage_helpers.c
 * Tests mDNS server helper functions to improve coverage:
 * - close_mdns_server_interfaces
 * - get_mdns_server_retry_count
 *
 * Tests for create_multicast_socket are in mdns_server_socket_test_create_multicast_socket.c
 * Tests for mdns_server_init are in mdns_server_init_test_mdns_server_init.c and
 *   mdns_server_init_test_error_paths.c
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mdns/mdns_keys.h>
#include <src/mdns/mdns_server.h>

// Test function prototypes
void test_close_mdns_server_interfaces_null_server(void);
void test_close_mdns_server_interfaces_null_interfaces(void);
void test_get_mdns_server_retry_count_null_config(void);
void test_get_mdns_server_retry_count_zero_retry(void);
void test_get_mdns_server_retry_count_valid(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_close_mdns_server_interfaces_null_server(void) {
    close_mdns_server_interfaces(NULL);
    TEST_PASS();
}

void test_close_mdns_server_interfaces_null_interfaces(void) {
    mdns_server_t server;
    memset(&server, 0, sizeof(server));
    server.interfaces = NULL;
    server.num_interfaces = 0;

    close_mdns_server_interfaces(&server);
    TEST_PASS();
}

void test_get_mdns_server_retry_count_null_config(void) {
    int result = get_mdns_server_retry_count(NULL);
    TEST_ASSERT_EQUAL_INT(1, result);
}

void test_get_mdns_server_retry_count_zero_retry(void) {
    AppConfig config;
    memset(&config, 0, sizeof(config));
    config.mdns_server.retry_count = 0;

    int result = get_mdns_server_retry_count(&config);
    TEST_ASSERT_EQUAL_INT(1, result);
}

void test_get_mdns_server_retry_count_valid(void) {
    AppConfig config;
    memset(&config, 0, sizeof(config));
    config.mdns_server.retry_count = 5;

    int result = get_mdns_server_retry_count(&config);
    TEST_ASSERT_EQUAL_INT(5, result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_close_mdns_server_interfaces_null_server);
    RUN_TEST(test_close_mdns_server_interfaces_null_interfaces);
    RUN_TEST(test_get_mdns_server_retry_count_null_config);
    RUN_TEST(test_get_mdns_server_retry_count_zero_retry);
    RUN_TEST(test_get_mdns_server_retry_count_valid);

    return UNITY_END();
}
