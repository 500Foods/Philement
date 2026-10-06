/*
 * Unity Test File: nats_server_endpoint
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/config/config_nats.h>

void test_nats_server_endpoint_host_and_port(void);
void test_nats_server_endpoint_default_port(void);
void test_nats_server_endpoint_ipv6_brackets(void);
void test_nats_server_endpoint_rejects_route_port(void);
void test_nats_server_endpoint_rejects_null(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_nats_server_endpoint_host_and_port(void) {
    char host[256];
    int port = 0;

    TEST_ASSERT_TRUE(nats_server_endpoint("nats://127.0.0.1:4222", host, sizeof(host), &port));
    TEST_ASSERT_EQUAL_STRING("127.0.0.1", host);
    TEST_ASSERT_EQUAL(4222, port);
}

void test_nats_server_endpoint_default_port(void) {
    char host[256];
    int port = 0;

    TEST_ASSERT_TRUE(nats_server_endpoint("nats://nats.example", host, sizeof(host), &port));
    TEST_ASSERT_EQUAL_STRING("nats.example", host);
    TEST_ASSERT_EQUAL(4222, port);
}

void test_nats_server_endpoint_ipv6_brackets(void) {
    char host[256];
    int port = 0;

    TEST_ASSERT_TRUE(nats_server_endpoint("nats://[::1]:4222", host, sizeof(host), &port));
    TEST_ASSERT_EQUAL_STRING("::1", host);
    TEST_ASSERT_EQUAL(4222, port);
}

void test_nats_server_endpoint_rejects_route_port(void) {
    char host[256];
    int port = 0;

    TEST_ASSERT_FALSE(nats_server_endpoint("nats://127.0.0.1:6222", host, sizeof(host), &port));
}

void test_nats_server_endpoint_rejects_null(void) {
    char host[256];
    int port = 0;

    TEST_ASSERT_FALSE(nats_server_endpoint(NULL, host, sizeof(host), &port));
    TEST_ASSERT_FALSE(nats_server_endpoint("nats://127.0.0.1:4222", NULL, sizeof(host), &port));
    TEST_ASSERT_FALSE(nats_server_endpoint("nats://127.0.0.1:4222", host, sizeof(host), NULL));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_nats_server_endpoint_host_and_port);
    RUN_TEST(test_nats_server_endpoint_default_port);
    RUN_TEST(test_nats_server_endpoint_ipv6_brackets);
    RUN_TEST(test_nats_server_endpoint_rejects_route_port);
    RUN_TEST(test_nats_server_endpoint_rejects_null);
    return UNITY_END();
}
