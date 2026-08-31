/*
 * Unity Test File: mdns_server_defend_test_mdns_server_addr_is_ours.c
 * Tests local address detection for A/AAAA conflict filtering.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mdns/mdns_server.h>
#include <arpa/inet.h>

void test_addr_is_ours_v4(void);
void test_addr_is_ours_v6(void);
void test_addr_is_not_ours(void);
void test_addr_is_ours_null_args(void);

void setUp(void)
{
}

void tearDown(void)
{
}

void test_addr_is_ours_v4(void)
{
    mdns_server_t server;
    mdns_server_interface_t iface;
    char *ips[] = { (char *)"192.168.1.100" };
    struct in_addr addr;

    memset(&server, 0, sizeof server);
    memset(&iface, 0, sizeof iface);
    iface.ip_addresses = ips;
    iface.num_addresses = 1;
    server.interfaces = &iface;
    server.num_interfaces = 1;
    inet_pton(AF_INET, "192.168.1.100", &addr);
    TEST_ASSERT_EQUAL_INT(1, mdns_server_addr_is_ours(&server, AF_INET,
                                                      (const uint8_t *)&addr, 4));
}

void test_addr_is_ours_v6(void)
{
    mdns_server_t server;
    mdns_server_interface_t iface;
    char *ips[] = { (char *)"::1" };
    struct in6_addr addr;

    memset(&server, 0, sizeof server);
    memset(&iface, 0, sizeof iface);
    iface.ip_addresses = ips;
    iface.num_addresses = 1;
    server.interfaces = &iface;
    server.num_interfaces = 1;
    inet_pton(AF_INET6, "::1", &addr);
    TEST_ASSERT_EQUAL_INT(1, mdns_server_addr_is_ours(&server, AF_INET6,
                                                      (const uint8_t *)&addr, 16));
}

void test_addr_is_not_ours(void)
{
    mdns_server_t server;
    mdns_server_interface_t iface;
    char *ips[] = { (char *)"192.168.1.100" };
    struct in_addr addr;

    memset(&server, 0, sizeof server);
    memset(&iface, 0, sizeof iface);
    iface.ip_addresses = ips;
    iface.num_addresses = 1;
    server.interfaces = &iface;
    server.num_interfaces = 1;
    inet_pton(AF_INET, "10.0.0.1", &addr);
    TEST_ASSERT_EQUAL_INT(0, mdns_server_addr_is_ours(&server, AF_INET,
                                                      (const uint8_t *)&addr, 4));
}

void test_addr_is_ours_null_args(void)
{
    TEST_ASSERT_EQUAL_INT(0, mdns_server_addr_is_ours(NULL, AF_INET, NULL, 0));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_addr_is_ours_v4);
    RUN_TEST(test_addr_is_ours_v6);
    RUN_TEST(test_addr_is_not_ours);
    RUN_TEST(test_addr_is_ours_null_args);
    return UNITY_END();
}
