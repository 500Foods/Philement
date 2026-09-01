/*
 * Unity Test: mdns_server_respond_test_mdns_server_sockaddr_port.c
 * Tests mdns_server_sockaddr_port and mdns_server_iface_for_sock from mdns_server_respond.c
 *
 * CHANGELOG
 * 1.0.0 - 2026-09-01 - Initial creation
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include <src/mdns/mdns_keys.h>
#include <src/mdns/mdns_server.h>

void test_sockaddr_port_null_addr(void);
void test_sockaddr_port_short_len(void);
void test_sockaddr_port_ipv4(void);
void test_sockaddr_port_ipv6(void);
void test_sockaddr_port_unknown_family(void);
void test_iface_for_sock_null_server(void);
void test_iface_for_sock_negative_sockfd(void);
void test_iface_for_sock_no_match(void);
void test_iface_for_sock_match_v4(void);
void test_iface_for_sock_match_v6(void);

void setUp(void) {}
void tearDown(void) {}

void test_sockaddr_port_null_addr(void) {
    TEST_ASSERT_EQUAL_UINT16(MDNS_PORT, mdns_server_sockaddr_port(NULL, 100));
}

void test_sockaddr_port_short_len(void) {
    struct sockaddr sa;
    memset(&sa, 0, sizeof(sa));
    TEST_ASSERT_EQUAL_UINT16(MDNS_PORT, mdns_server_sockaddr_port(&sa, 0));
}

void test_sockaddr_port_ipv4(void) {
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(5353);
    TEST_ASSERT_EQUAL_UINT16(5353, mdns_server_sockaddr_port(&sa, sizeof(sa)));
}

void test_sockaddr_port_ipv6(void) {
    struct sockaddr_in6 sa6;
    memset(&sa6, 0, sizeof(sa6));
    sa6.sin6_family = AF_INET6;
    sa6.sin6_port = htons(5354);
    TEST_ASSERT_EQUAL_UINT16(5354, mdns_server_sockaddr_port(&sa6, sizeof(sa6)));
}

void test_sockaddr_port_unknown_family(void) {
    struct sockaddr sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_family = AF_BLUETOOTH;
    TEST_ASSERT_EQUAL_UINT16(MDNS_PORT, mdns_server_sockaddr_port(&sa, sizeof(sa)));
}

void test_iface_for_sock_null_server(void) {
    TEST_ASSERT_NULL(mdns_server_iface_for_sock(NULL, 3));
}

void test_iface_for_sock_negative_sockfd(void) {
    mdns_server_t server;
    memset(&server, 0, sizeof(server));
    TEST_ASSERT_NULL(mdns_server_iface_for_sock(&server, -1));
}

void test_iface_for_sock_no_match(void) {
    mdns_server_t server;
    memset(&server, 0, sizeof(server));
    server.interfaces = NULL;
    server.num_interfaces = 0;
    TEST_ASSERT_NULL(mdns_server_iface_for_sock(&server, 5));
}

void test_iface_for_sock_match_v4(void) {
    mdns_server_t server;
    mdns_server_interface_t iface;
    memset(&server, 0, sizeof(server));
    memset(&iface, 0, sizeof(iface));
    iface.sockfd_v4 = 7;
    iface.sockfd_v6 = -1;
    server.interfaces = &iface;
    server.num_interfaces = 1;
    TEST_ASSERT_EQUAL_PTR(&iface, mdns_server_iface_for_sock(&server, 7));
}

void test_iface_for_sock_match_v6(void) {
    mdns_server_t server;
    mdns_server_interface_t iface;
    memset(&server, 0, sizeof(server));
    memset(&iface, 0, sizeof(iface));
    iface.sockfd_v4 = -1;
    iface.sockfd_v6 = 9;
    server.interfaces = &iface;
    server.num_interfaces = 1;
    TEST_ASSERT_EQUAL_PTR(&iface, mdns_server_iface_for_sock(&server, 9));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_sockaddr_port_null_addr);
    RUN_TEST(test_sockaddr_port_short_len);
    RUN_TEST(test_sockaddr_port_ipv4);
    RUN_TEST(test_sockaddr_port_ipv6);
    RUN_TEST(test_sockaddr_port_unknown_family);
    RUN_TEST(test_iface_for_sock_null_server);
    RUN_TEST(test_iface_for_sock_negative_sockfd);
    RUN_TEST(test_iface_for_sock_no_match);
    RUN_TEST(test_iface_for_sock_match_v4);
    RUN_TEST(test_iface_for_sock_match_v6);

    return UNITY_END();
}
