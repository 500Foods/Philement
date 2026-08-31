/*
 * Unity Test File: mdns_server_probe_test_mdns_server_build_probe.c
 * Tests probe packet: QR=0, qdcount >= 2, nscount > 0
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/mdns/mdns_server.h>
#include <src/mdns/mdns_wire.h>

void test_build_probe_shape(void);
void test_build_probe_nulls(void);
void test_build_probe_skips_claimed(void);

void setUp(void)
{
}

void tearDown(void)
{
}

void test_build_probe_shape(void)
{
    mdns_server_t server;
    mdns_server_service_t svc;
    mdns_server_interface_t iface;
    char ip[] = "192.168.1.50";
    char *ips[] = { ip };
    uint8_t packet[512];
    size_t packet_len = 0;
    mdns_msg msg;

    memset(&server, 0, sizeof server);
    memset(&svc, 0, sizeof svc);
    memset(&iface, 0, sizeof iface);
    svc.name = (char *)"Printer";
    svc.type = (char *)"_http._tcp.local";
    svc.port = 80;
    server.hostname = (char *)"host.local";
    server.services = &svc;
    server.num_services = 1;
    iface.ip_addresses = ips;
    iface.num_addresses = 1;

    mdns_server_build_probe(packet, &packet_len, &server, &iface);
    TEST_ASSERT_GREATER_THAN(12, packet_len);
    TEST_ASSERT_EQUAL_INT(0, mdns_parse(packet, packet_len, &msg));
    TEST_ASSERT_EQUAL_UINT16(DNS_FLAG_QUERY, msg.flags);
    TEST_ASSERT_TRUE(msg.qdcount >= 2);
    TEST_ASSERT_TRUE(msg.nscount > 0);
    TEST_ASSERT_EQUAL_UINT16(msg.qdcount, (uint16_t)msg.nquestions);
}

void test_build_probe_nulls(void)
{
    size_t packet_len = 99;

    mdns_server_build_probe(NULL, &packet_len, NULL, NULL);
    TEST_ASSERT_EQUAL_UINT(0, packet_len);
}

void test_build_probe_skips_claimed(void)
{
    mdns_server_t server;
    mdns_server_service_t svc;
    uint8_t packet[512];
    size_t packet_len = 0;

    memset(&server, 0, sizeof server);
    memset(&svc, 0, sizeof svc);
    svc.name = (char *)"Printer";
    svc.type = (char *)"_http._tcp.local";
    svc.claimed = 1;
    server.hostname = (char *)"host.local";
    server.hostname_claimed = 1;
    server.services = &svc;
    server.num_services = 1;

    mdns_server_build_probe(packet, &packet_len, &server, NULL);
    TEST_ASSERT_EQUAL_UINT(0, packet_len);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_build_probe_shape);
    RUN_TEST(test_build_probe_nulls);
    RUN_TEST(test_build_probe_skips_claimed);
    return UNITY_END();
}
