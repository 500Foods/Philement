/*
 * Unity Test File: mdns_server_respond_test_mdns_server_iface_has_af.c
 * Tests mdns_server_iface_has_af
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mdns/mdns_server.h>

void test_iface_has_af_v4(void);
void test_iface_has_af_v6(void);
void test_iface_has_af_null(void);

void setUp(void)
{
}

void tearDown(void)
{
}

void test_iface_has_af_v4(void)
{
    mdns_server_interface_t iface;
    char *ips[1];

    memset(&iface, 0, sizeof iface);
    ips[0] = (char *)"192.0.2.10";
    iface.ip_addresses = ips;
    iface.num_addresses = 1;
    TEST_ASSERT_EQUAL_INT(1, mdns_server_iface_has_af(&iface, AF_INET));
    TEST_ASSERT_EQUAL_INT(0, mdns_server_iface_has_af(&iface, AF_INET6));
}

void test_iface_has_af_v6(void)
{
    mdns_server_interface_t iface;
    char *ips[1];

    memset(&iface, 0, sizeof iface);
    ips[0] = (char *)"2001:db8::1";
    iface.ip_addresses = ips;
    iface.num_addresses = 1;
    TEST_ASSERT_EQUAL_INT(0, mdns_server_iface_has_af(&iface, AF_INET));
    TEST_ASSERT_EQUAL_INT(1, mdns_server_iface_has_af(&iface, AF_INET6));
}

void test_iface_has_af_null(void)
{
    TEST_ASSERT_EQUAL_INT(0, mdns_server_iface_has_af(NULL, AF_INET));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_iface_has_af_v4);
    RUN_TEST(test_iface_has_af_v6);
    RUN_TEST(test_iface_has_af_null);
    return UNITY_END();
}
