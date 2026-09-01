/*
 * Unity Test: mdns_server_announce_test_send_announcement.c
 * Tests mdns_server_send_announcement from mdns_server_announce.c
 *
 * CHANGELOG
 * 1.0.0 - 2026-09-01 - Initial creation
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mdns/mdns_keys.h>
#include <src/mdns/mdns_server.h>

void test_mdns_server_send_announcement_null_server(void);
void test_mdns_server_send_announcement_probe_failed(void);
void test_mdns_server_send_announcement_not_claimed(void);
void test_mdns_server_send_announcement_no_interfaces(void);
void test_mdns_server_send_announcement_disabled_iface(void);
void test_mdns_server_send_announcement_no_sockets(void);

void setUp(void) {
    app_config = NULL;
}

void tearDown(void) {
}

void test_mdns_server_send_announcement_null_server(void) {
    mdns_server_send_announcement(NULL, NULL);
}

void test_mdns_server_send_announcement_probe_failed(void) {
    mdns_server_t server;
    memset(&server, 0, sizeof(server));
    server.probe_failed = 1;
    mdns_server_send_announcement(&server, NULL);
}

void test_mdns_server_send_announcement_not_claimed(void) {
    mdns_server_t server;
    memset(&server, 0, sizeof(server));
    server.hostname_claimed = 0;
    server.num_services = 0;
    server.services = NULL;
    mdns_server_send_announcement(&server, NULL);
}

void test_mdns_server_send_announcement_no_interfaces(void) {
    mdns_server_t server;
    memset(&server, 0, sizeof(server));
    server.hostname_claimed = 1;
    server.probe_failed = 0;
    server.interfaces = NULL;
    server.num_interfaces = 0;
    mdns_server_send_announcement(&server, NULL);
}

void test_mdns_server_send_announcement_disabled_iface(void) {
    mdns_server_t server;
    mdns_server_interface_t iface;

    memset(&server, 0, sizeof(server));
    server.hostname_claimed = 1;
    server.probe_failed = 0;

    memset(&iface, 0, sizeof(iface));
    iface.if_name = (char *)"eth0";
    iface.sockfd_v4 = -1;
    iface.sockfd_v6 = -1;
    iface.num_addresses = 0;
    iface.disabled = 1;

    server.interfaces = &iface;
    server.num_interfaces = 1;

    mdns_server_send_announcement(&server, NULL);
}

void test_mdns_server_send_announcement_no_sockets(void) {
    mdns_server_t server;
    mdns_server_interface_t iface;

    memset(&server, 0, sizeof(server));
    server.hostname_claimed = 1;
    server.probe_failed = 0;

    memset(&iface, 0, sizeof(iface));
    iface.if_name = (char *)"eth0";
    iface.sockfd_v4 = -1;
    iface.sockfd_v6 = -1;
    iface.num_addresses = 1;
    char *ips[1];
    ips[0] = (char *)"192.168.1.100";
    iface.ip_addresses = ips;

    server.interfaces = &iface;
    server.num_interfaces = 1;

    mdns_server_send_announcement(&server, NULL);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mdns_server_send_announcement_null_server);
    RUN_TEST(test_mdns_server_send_announcement_probe_failed);
    RUN_TEST(test_mdns_server_send_announcement_not_claimed);
    RUN_TEST(test_mdns_server_send_announcement_no_interfaces);
    RUN_TEST(test_mdns_server_send_announcement_disabled_iface);
    RUN_TEST(test_mdns_server_send_announcement_no_sockets);

    return UNITY_END();
}
