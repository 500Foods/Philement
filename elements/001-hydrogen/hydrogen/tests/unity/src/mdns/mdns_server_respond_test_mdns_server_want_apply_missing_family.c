/*
 * Unity Test File: mdns_server_respond_test_mdns_server_want_apply_missing_family.c
 * Tests mdns_server_want_apply_missing_family
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mdns/mdns_server.h>

void test_apply_missing_ipv4_only_drops_aaaa(void);
void test_apply_missing_ipv6_only_drops_a(void);
void test_apply_missing_dual_keeps_both(void);
void test_apply_missing_nulls(void);

void setUp(void)
{
}

void tearDown(void)
{
}

static void fill_iface(mdns_server_interface_t *iface, char **ips, size_t n)
{
    memset(iface, 0, sizeof(*iface));
    iface->ip_addresses = ips;
    iface->num_addresses = n;
}

void test_apply_missing_ipv4_only_drops_aaaa(void)
{
    mdns_server_interface_t iface;
    mdns_server_want_t want;
    char *ips[1];

    ips[0] = (char *)"192.0.2.10";
    fill_iface(&iface, ips, 1);
    mdns_server_want_clear(&want, 0);
    want.host_answer = MDNS_W_A | MDNS_W_AAAA | MDNS_W_NSEC;
    mdns_server_want_apply_missing_family(&want, &iface);
    TEST_ASSERT_TRUE((want.host_answer & MDNS_W_A) != 0);
    TEST_ASSERT_TRUE((want.host_answer & MDNS_W_AAAA) == 0);
    TEST_ASSERT_TRUE((want.host_answer & MDNS_W_NSEC) != 0);
}

void test_apply_missing_ipv6_only_drops_a(void)
{
    mdns_server_interface_t iface;
    mdns_server_want_t want;
    char *ips[1];

    ips[0] = (char *)"2001:db8::1";
    fill_iface(&iface, ips, 1);
    mdns_server_want_clear(&want, 0);
    want.host_answer = MDNS_W_A;
    mdns_server_want_apply_missing_family(&want, &iface);
    TEST_ASSERT_TRUE((want.host_answer & MDNS_W_A) == 0);
    TEST_ASSERT_TRUE((want.host_answer & MDNS_W_NSEC) != 0);
}

void test_apply_missing_dual_keeps_both(void)
{
    mdns_server_interface_t iface;
    mdns_server_want_t want;
    char *ips[2];

    ips[0] = (char *)"192.0.2.10";
    ips[1] = (char *)"2001:db8::1";
    fill_iface(&iface, ips, 2);
    mdns_server_want_clear(&want, 0);
    want.host_answer = MDNS_W_A | MDNS_W_AAAA | MDNS_W_NSEC;
    mdns_server_want_apply_missing_family(&want, &iface);
    TEST_ASSERT_TRUE((want.host_answer & MDNS_W_A) != 0);
    TEST_ASSERT_TRUE((want.host_answer & MDNS_W_AAAA) != 0);
    TEST_ASSERT_TRUE((want.host_answer & MDNS_W_NSEC) != 0);
}

void test_apply_missing_nulls(void)
{
    mdns_server_want_t want;

    mdns_server_want_clear(&want, 0);
    want.host_answer = MDNS_W_A;
    mdns_server_want_apply_missing_family(NULL, NULL);
    mdns_server_want_apply_missing_family(&want, NULL);
    TEST_ASSERT_TRUE((want.host_answer & MDNS_W_A) != 0);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_apply_missing_ipv4_only_drops_aaaa);
    RUN_TEST(test_apply_missing_ipv6_only_drops_a);
    RUN_TEST(test_apply_missing_dual_keeps_both);
    RUN_TEST(test_apply_missing_nulls);
    return UNITY_END();
}
