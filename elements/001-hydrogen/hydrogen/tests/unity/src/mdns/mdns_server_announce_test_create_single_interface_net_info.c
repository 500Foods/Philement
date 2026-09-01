/*
 * Unity Test: mdns_server_announce_test_create_single_interface_net_info.c
 * Tests create_single_interface_net_info and free_single_interface_net_info
 * from mdns_server_announce.c
 *
 * CHANGELOG
 * 1.0.0 - 2026-09-01 - Initial creation
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mdns/mdns_keys.h>
#include <src/mdns/mdns_server.h>

void test_create_single_iface_normal_v4(void);
void test_create_single_iface_multiple_ips(void);
void test_create_single_iface_max_ips(void);
void test_free_single_iface_null(void);

void setUp(void) {}
void tearDown(void) {}

void test_create_single_iface_normal_v4(void) {
    mdns_server_interface_t iface;
    char *ips[1];
    network_info_t *info;

    memset(&iface, 0, sizeof(iface));
    iface.if_name = (char *)"eth0";
    iface.num_addresses = 1;
    ips[0] = (char *)"192.168.1.100";
    iface.ip_addresses = ips;

    info = create_single_interface_net_info(&iface);
    TEST_ASSERT_NOT_NULL(info);
    TEST_ASSERT_EQUAL_INT(1, info->count);
    TEST_ASSERT_EQUAL_INT(0, info->primary_index);
    TEST_ASSERT_EQUAL_STRING("eth0", info->interfaces[0].name);
    TEST_ASSERT_EQUAL_INT(1, info->interfaces[0].ip_count);
    TEST_ASSERT_EQUAL_STRING("192.168.1.100", info->interfaces[0].ips[0]);
    free_single_interface_net_info(info);
}

void test_create_single_iface_multiple_ips(void) {
    mdns_server_interface_t iface;
    char *ips[] = {(char *)"192.168.1.100", (char *)"10.0.0.1", (char *)"2001:db8::1"};
    network_info_t *info;

    memset(&iface, 0, sizeof(iface));
    iface.if_name = (char *)"wlan0";
    iface.ip_addresses = ips;
    iface.num_addresses = 3;

    info = create_single_interface_net_info(&iface);
    TEST_ASSERT_NOT_NULL(info);
    TEST_ASSERT_EQUAL_INT(1, info->count);
    TEST_ASSERT_EQUAL_STRING("wlan0", info->interfaces[0].name);
    TEST_ASSERT_EQUAL_INT(3, info->interfaces[0].ip_count);
    TEST_ASSERT_EQUAL_STRING("192.168.1.100", info->interfaces[0].ips[0]);
    TEST_ASSERT_EQUAL_STRING("10.0.0.1", info->interfaces[0].ips[1]);
    free_single_interface_net_info(info);
}

void test_create_single_iface_max_ips(void) {
    mdns_server_interface_t iface;
    char *ips[MAX_IPS + 5];
    int i;
    network_info_t *info;

    memset(&iface, 0, sizeof(iface));
    iface.if_name = (char *)"eth0";

    for (i = 0; i < MAX_IPS + 5; i++) {
        ips[i] = (char *)"192.168.1.100";
    }
    iface.ip_addresses = ips;
    iface.num_addresses = MAX_IPS + 5;

    info = create_single_interface_net_info(&iface);
    TEST_ASSERT_NOT_NULL(info);
    TEST_ASSERT_EQUAL_INT(MAX_IPS, info->interfaces[0].ip_count);
    free_single_interface_net_info(info);
}

void test_free_single_iface_null(void) {
    free_single_interface_net_info(NULL);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_create_single_iface_normal_v4);
    RUN_TEST(test_create_single_iface_multiple_ips);
    RUN_TEST(test_create_single_iface_max_ips);
    RUN_TEST(test_free_single_iface_null);

    return UNITY_END();
}
