/*
 * Unity Test: mdns_server_announce_test_build_interface_announcement.c
 * Tests mDNS interface announcement building functions from mdns_server_announce.c
 *
 * This file follows the naming convention from tests/UNITY.md:
 * <source>_test_<function>.c where source is mdns_server_announce and function is build_interface_announcement
 *
 * CHANGELOG
 * 1.0.1 - 2026-09-01 - Wire up all 8 remaining test functions in main() (only null_interface was running)
 * 1.0.0 - 2026-08-31 - Initial creation
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mdns/mdns_keys.h>
#include <src/mdns/mdns_server.h>

void test_mdns_server_build_interface_announcement_null_interface(void);
void test_mdns_server_build_interface_announcement_null_packet(void);
void test_mdns_server_build_interface_announcement_minimal_valid(void);
void test_mdns_server_build_interface_announcement_with_services(void);
void test_mdns_server_build_interface_announcement_ipv4_only(void);
void test_mdns_server_build_interface_announcement_ipv6_only(void);
void test_mdns_server_build_interface_announcement_multiple_ips(void);
void test_mdns_server_build_interface_announcement_long_service_name(void);
void test_mdns_server_build_interface_announcement_packet_size_limit(void);
void test_mdns_server_build_interface_announcement_unclaimed_hostname(void);
void test_mdns_server_build_interface_announcement_null_ip_addresses(void);

static void fill_minimal_server(mdns_server_t *server)
{
    memset(server, 0, sizeof(*server));
    server->hostname = (char *)"test.local";
    server->hostname_claimed = 1;
    server->num_services = 0;
    server->services = NULL;
}

static void fill_iface_v4(mdns_server_interface_t *iface, char **ips)
{
    memset(iface, 0, sizeof(*iface));
    iface->if_name = (char *)"eth0";
    iface->sockfd_v4 = -1;
    iface->sockfd_v6 = -1;
    ips[0] = (char *)"192.168.1.100";
    iface->ip_addresses = ips;
    iface->num_addresses = 1;
}

static void fill_iface_v6(mdns_server_interface_t *iface, char **ips)
{
    memset(iface, 0, sizeof(*iface));
    iface->if_name = (char *)"eth0";
    iface->sockfd_v4 = -1;
    iface->sockfd_v6 = -1;
    ips[0] = (char *)"2001:db8::1";
    iface->ip_addresses = ips;
    iface->num_addresses = 1;
}

static void fill_iface_multi(mdns_server_interface_t *iface, char **ips)
{
    memset(iface, 0, sizeof(*iface));
    iface->if_name = (char *)"eth0";
    iface->sockfd_v4 = -1;
    iface->sockfd_v6 = -1;
    ips[0] = (char *)"192.168.1.100";
    ips[1] = (char *)"192.168.1.101";
    ips[2] = (char *)"2001:db8::1";
    iface->ip_addresses = ips;
    iface->num_addresses = 3;
}

void setUp(void) {
}

void tearDown(void) {
}

void test_mdns_server_build_interface_announcement_null_interface(void) {
    uint8_t packet[2048];
    size_t packet_len = 0;

    mdns_server_t server;
    fill_minimal_server(&server);

    _mdns_server_build_interface_announcement(packet, &packet_len, "test.local", &server, 120, NULL);

    TEST_ASSERT_EQUAL_UINT(12, packet_len);
    TEST_ASSERT_EQUAL_UINT(0, packet[2]);
    TEST_ASSERT_EQUAL_UINT(0, packet[3]);
}

void test_mdns_server_build_interface_announcement_null_packet(void) {
    size_t packet_len = 999;

    mdns_server_t server;
    fill_minimal_server(&server);

    mdns_server_interface_t iface;
    char *ips[1];
    fill_iface_v4(&iface, ips);

    _mdns_server_build_interface_announcement(NULL, &packet_len, "test.local", &server, 120, &iface);

    TEST_ASSERT_EQUAL_UINT(0, packet_len);
}

void test_mdns_server_build_interface_announcement_minimal_valid(void) {
    uint8_t packet[2048];
    size_t packet_len = 0;

    mdns_server_t server;
    fill_minimal_server(&server);

    mdns_server_interface_t iface;
    char *ips[1];
    fill_iface_v4(&iface, ips);

    _mdns_server_build_interface_announcement(packet, &packet_len, "test.local", &server, 120, &iface);

    TEST_ASSERT_GREATER_THAN(12, packet_len);

    dns_header_t *header = (dns_header_t *)packet;
    TEST_ASSERT_EQUAL_UINT(0, ntohs(header->id));
    TEST_ASSERT_EQUAL_UINT(0, ntohs(header->qdcount));
    TEST_ASSERT_GREATER_THAN(0, ntohs(header->ancount));
}

void test_mdns_server_build_interface_announcement_with_services(void) {
    uint8_t packet[2048];
    size_t packet_len = 0;

    mdns_server_t server;
    memset(&server, 0, sizeof(server));
    server.hostname = (char *)"test.local";
    server.hostname_claimed = 1;

    mdns_server_service_t service;
    memset(&service, 0, sizeof(service));
    char service_name[] = "testservice";
    char service_type[] = "_http._tcp.local";
    service.name = service_name;
    service.type = service_type;
    service.port = 8080;
    service.claimed = 1;

    service.txt_records = malloc(sizeof(char*) * 2);
    service.txt_records[0] = strdup("version=1.0");
    service.txt_records[1] = strdup("path=/api");
    service.num_txt_records = 2;

    server.services = &service;
    server.num_services = 1;

    mdns_server_interface_t iface;
    char *ips[1];
    fill_iface_v4(&iface, ips);

    _mdns_server_build_interface_announcement(packet, &packet_len, "test.local", &server, 120, &iface);

    TEST_ASSERT_GREATER_THAN(100, packet_len);

    dns_header_t *header = (dns_header_t *)packet;
    TEST_ASSERT_GREATER_OR_EQUAL(4, ntohs(header->ancount));

    free(service.txt_records[0]);
    free(service.txt_records[1]);
    free(service.txt_records);
}

void test_mdns_server_build_interface_announcement_ipv4_only(void) {
    uint8_t packet[2048];
    size_t packet_len = 0;

    mdns_server_t server;
    fill_minimal_server(&server);

    mdns_server_interface_t iface;
    char *ips[1];
    fill_iface_v4(&iface, ips);

    _mdns_server_build_interface_announcement(packet, &packet_len, "test.local", &server, 120, &iface);

    TEST_ASSERT_GREATER_THAN(12, packet_len);
}

void test_mdns_server_build_interface_announcement_ipv6_only(void) {
    uint8_t packet[2048];
    size_t packet_len = 0;

    mdns_server_t server;
    fill_minimal_server(&server);

    mdns_server_interface_t iface;
    char *ips[1];
    fill_iface_v6(&iface, ips);

    _mdns_server_build_interface_announcement(packet, &packet_len, "test.local", &server, 120, &iface);

    TEST_ASSERT_GREATER_THAN(12, packet_len);
}

void test_mdns_server_build_interface_announcement_multiple_ips(void) {
    uint8_t packet[2048];
    size_t packet_len = 0;

    mdns_server_t server;
    fill_minimal_server(&server);

    mdns_server_interface_t iface;
    char *ips[3];
    fill_iface_multi(&iface, ips);

    _mdns_server_build_interface_announcement(packet, &packet_len, "test.local", &server, 120, &iface);

    TEST_ASSERT_GREATER_THAN(12, packet_len);

    dns_header_t *header = (dns_header_t *)packet;
    TEST_ASSERT_GREATER_OR_EQUAL(3, ntohs(header->ancount));
}

void test_mdns_server_build_interface_announcement_long_service_name(void) {
    uint8_t packet[2048];
    size_t packet_len = 0;

    mdns_server_t server;
    memset(&server, 0, sizeof(server));
    server.hostname = (char *)"test.local";
    server.hostname_claimed = 1;

    mdns_server_service_t service;
    memset(&service, 0, sizeof(service));
    char service_name[] = "verylongservicenamethatexceedsnormallimitsandshouldbetruncated";
    char service_type[] = "_http._tcp.local";
    service.name = service_name;
    service.type = service_type;
    service.port = 8080;
    service.claimed = 1;
    service.txt_records = NULL;
    service.num_txt_records = 0;

    server.services = &service;
    server.num_services = 1;

    mdns_server_interface_t iface;
    char *ips[1];
    fill_iface_v4(&iface, ips);

    _mdns_server_build_interface_announcement(packet, &packet_len, "test.local", &server, 120, &iface);

    TEST_ASSERT_GREATER_THAN(12, packet_len);
}

void test_mdns_server_build_interface_announcement_packet_size_limit(void) {
    uint8_t packet[2048];
    size_t packet_len = 0;

    mdns_server_t server;
    memset(&server, 0, sizeof(server));
    server.hostname = (char *)"test.local";
    server.hostname_claimed = 1;

    mdns_server_service_t services[5];
    memset(services, 0, sizeof(services));

    char service_names[5][20] = {"svc1", "svc2", "svc3", "svc4", "svc5"};
    char service_types[5][20] = {"_http._tcp.local", "_https._tcp.local", "_ftp._tcp.local", "_ssh._tcp.local", "_telnet._tcp.local"};

    for (int i = 0; i < 5; i++) {
        services[i].name = service_names[i];
        services[i].type = service_types[i];
        services[i].port = 8000 + i;
        services[i].txt_records = NULL;
        services[i].num_txt_records = 0;
    }

    server.services = services;
    server.num_services = 5;

    mdns_server_interface_t iface;
    char *ips[4];
    memset(&iface, 0, sizeof(iface));
    iface.if_name = (char *)"eth0";
    iface.sockfd_v4 = -1;
    iface.sockfd_v6 = -1;
    ips[0] = (char *)"192.168.1.100";
    ips[1] = (char *)"192.168.1.101";
    ips[2] = (char *)"10.0.0.1";
    ips[3] = (char *)"2001:db8::1";
    iface.ip_addresses = ips;
    iface.num_addresses = 4;

    _mdns_server_build_interface_announcement(packet, &packet_len, "test.local", &server, 120, &iface);

    TEST_ASSERT_GREATER_THAN(100, packet_len);
    TEST_ASSERT_LESS_OR_EQUAL_UINT(sizeof(packet), packet_len);
}

void test_mdns_server_build_interface_announcement_unclaimed_hostname(void) {
    uint8_t packet[2048];
    size_t packet_len = 0;

    mdns_server_t server;
    memset(&server, 0, sizeof(server));
    server.hostname = (char *)"test.local";
    server.hostname_claimed = 0;
    server.num_services = 0;
    server.services = NULL;

    mdns_server_interface_t iface;
    char *ips[1];
    memset(&iface, 0, sizeof(iface));
    iface.if_name = (char *)"eth0";
    iface.sockfd_v4 = -1;
    iface.sockfd_v6 = -1;
    ips[0] = (char *)"192.168.1.100";
    iface.ip_addresses = ips;
    iface.num_addresses = 1;

    _mdns_server_build_interface_announcement(packet, &packet_len, "test.local", &server, 120, &iface);

    TEST_ASSERT_EQUAL_UINT(12, packet_len);

    dns_header_t *header = (dns_header_t *)packet;
    TEST_ASSERT_EQUAL_UINT(0, ntohs(header->ancount));
}

void test_mdns_server_build_interface_announcement_null_ip_addresses(void) {
    uint8_t packet[2048];
    size_t packet_len = 0;

    mdns_server_t server;
    fill_minimal_server(&server);

    mdns_server_interface_t iface;
    memset(&iface, 0, sizeof(iface));
    iface.if_name = (char *)"eth0";
    iface.sockfd_v4 = -1;
    iface.sockfd_v6 = -1;
    iface.ip_addresses = NULL;
    iface.num_addresses = 1;

    _mdns_server_build_interface_announcement(packet, &packet_len, "test.local", &server, 120, &iface);

    TEST_ASSERT_EQUAL_UINT(12, packet_len);

    dns_header_t *header = (dns_header_t *)packet;
    TEST_ASSERT_EQUAL_UINT(0, ntohs(header->ancount));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mdns_server_build_interface_announcement_null_interface);
    RUN_TEST(test_mdns_server_build_interface_announcement_null_packet);
    RUN_TEST(test_mdns_server_build_interface_announcement_minimal_valid);
    RUN_TEST(test_mdns_server_build_interface_announcement_with_services);
    RUN_TEST(test_mdns_server_build_interface_announcement_ipv4_only);
    RUN_TEST(test_mdns_server_build_interface_announcement_ipv6_only);
    RUN_TEST(test_mdns_server_build_interface_announcement_multiple_ips);
    RUN_TEST(test_mdns_server_build_interface_announcement_long_service_name);
    RUN_TEST(test_mdns_server_build_interface_announcement_packet_size_limit);
    RUN_TEST(test_mdns_server_build_interface_announcement_unclaimed_hostname);
    RUN_TEST(test_mdns_server_build_interface_announcement_null_ip_addresses);

    return UNITY_END();
}
