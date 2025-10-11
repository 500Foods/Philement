/*
 * Unity Test: mdns_server_announce_test_build_interface_announcement.c
 * Tests mDNS interface announcement building functions from mdns_server_announce.c
 *
 * This file follows the naming convention from tests/UNITY.md:
 * <source>_test_<function>.c where source is mdns_server_announce and function is build_interface_announcement
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/mdns/mdns_keys.h>
#include <src/mdns/mdns_server.h>

// Test function prototypes
void test_mdns_server_build_interface_announcement_null_interface(void);
void test_mdns_server_build_interface_announcement_null_packet(void);
void test_mdns_server_build_interface_announcement_minimal_valid(void);
void test_mdns_server_build_interface_announcement_with_services(void);
void test_mdns_server_build_interface_announcement_ipv4_only(void);
void test_mdns_server_build_interface_announcement_ipv6_only(void);
void test_mdns_server_build_interface_announcement_multiple_ips(void);
void test_mdns_server_build_interface_announcement_long_service_name(void);
void test_mdns_server_build_interface_announcement_packet_size_limit(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test building interface announcement with NULL interface
void test_mdns_server_build_interface_announcement_null_interface(void) {
    uint8_t packet[2048];
    size_t packet_len = 0;

    // Create minimal server instance
    mdns_server_t server;
    memset(&server, 0, sizeof(server));
    char hostname_buffer[] = "test.local";
    server.hostname = hostname_buffer;
    server.num_services = 0;
    server.services = NULL;

    // Test with NULL interface - should handle gracefully
    _mdns_server_build_interface_announcement(packet, &packet_len, "test.local", &server, 120, NULL);

    // Should create minimal packet with just DNS header
    TEST_ASSERT_EQUAL_UINT(12, packet_len); // DNS header size
    TEST_ASSERT_EQUAL_UINT(0, packet[2]); // QDCOUNT should be 0
    TEST_ASSERT_EQUAL_UINT(0, packet[3]);
}

// Test building interface announcement with NULL packet buffer
void test_mdns_server_build_interface_announcement_null_packet(void) {
    size_t packet_len = 999;

    // Create minimal server instance
    mdns_server_t server;
    memset(&server, 0, sizeof(server));
    char hostname_buffer[] = "test.local";
    server.hostname = hostname_buffer;
    server.num_services = 0;
    server.services = NULL;

    // Create minimal interface
    mdns_server_interface_t iface;
    memset(&iface, 0, sizeof(iface));
    strcpy(iface.if_name, "eth0");
    iface.num_addresses = 1;
    strcpy(iface.ip_addresses[0], "192.168.1.100");

    // Test with NULL packet buffer - should handle gracefully
    _mdns_server_build_interface_announcement(NULL, &packet_len, "test.local", &server, 120, &iface);

    // Should not crash and packet_len should be set to 0
    TEST_ASSERT_EQUAL_UINT(0, packet_len);
}

// Test building interface announcement with minimal valid inputs
void test_mdns_server_build_interface_announcement_minimal_valid(void) {
    uint8_t packet[2048];
    size_t packet_len = 0;

    // Create minimal server instance
    mdns_server_t server;
    memset(&server, 0, sizeof(server));
    char hostname_buffer[] = "test.local";
    server.hostname = hostname_buffer;
    server.num_services = 0;
    server.services = NULL;

    // Create minimal interface
    mdns_server_interface_t iface;
    memset(&iface, 0, sizeof(iface));
    strcpy(iface.if_name, "eth0");
    iface.num_addresses = 1;
    strcpy(iface.ip_addresses[0], "192.168.1.100");

    // Build announcement
    _mdns_server_build_interface_announcement(packet, &packet_len, "test.local", &server, 120, &iface);

    // Verify basic properties
    TEST_ASSERT_GREATER_THAN(12, packet_len); // Should be more than just DNS header
    TEST_ASSERT_LESS_OR_EQUAL_UINT(sizeof(packet), packet_len);

    // Verify DNS header structure
    dns_header_t *header = (dns_header_t *)packet;
    TEST_ASSERT_EQUAL_UINT(0, ntohs(header->id));
    TEST_ASSERT_EQUAL_UINT(0, ntohs(header->qdcount)); // No questions
    TEST_ASSERT_GREATER_THAN(0, ntohs(header->ancount)); // Should have answers
}

// Test building interface announcement with services
void test_mdns_server_build_interface_announcement_with_services(void) {
    uint8_t packet[2048];
    size_t packet_len = 0;

    // Create server instance with services
    mdns_server_t server;
    memset(&server, 0, sizeof(server));
    char hostname_buffer[] = "test.local";
    server.hostname = hostname_buffer;

    // Create a service
    mdns_server_service_t service;
    memset(&service, 0, sizeof(service));
    char service_name[] = "testservice";
    char service_type[] = "_http._tcp.local";
    service.name = service_name;
    service.type = service_type;
    service.port = 8080;

    // Add TXT records
    service.txt_records = malloc(sizeof(char*) * 2);
    service.txt_records[0] = strdup("version=1.0");
    service.txt_records[1] = strdup("path=/api");
    service.num_txt_records = 2;

    server.services = &service;
    server.num_services = 1;

    // Create interface
    mdns_server_interface_t iface;
    memset(&iface, 0, sizeof(iface));
    strcpy(iface.if_name, "eth0");
    iface.num_addresses = 1;
    strcpy(iface.ip_addresses[0], "192.168.1.100");

    // Build announcement
    _mdns_server_build_interface_announcement(packet, &packet_len, "test.local", &server, 120, &iface);

    // Verify packet contains service records
    TEST_ASSERT_GREATER_THAN(100, packet_len); // Should be substantial with services

    // Verify DNS header has multiple answer records (A + PTR + SRV + TXT)
    dns_header_t *header = (dns_header_t *)packet;
    TEST_ASSERT_GREATER_OR_EQUAL(4, ntohs(header->ancount)); // At least A, PTR, SRV, TXT records

    // Cleanup
    free(service.txt_records[0]);
    free(service.txt_records[1]);
    free(service.txt_records);
}

// Test building interface announcement with IPv4 only
void test_mdns_server_build_interface_announcement_ipv4_only(void) {
    uint8_t packet[2048];
    size_t packet_len = 0;

    // Create minimal server instance
    mdns_server_t server;
    memset(&server, 0, sizeof(server));
    char hostname_buffer[] = "test.local";
    server.hostname = hostname_buffer;
    server.num_services = 0;
    server.services = NULL;

    // Create interface with IPv4 address
    mdns_server_interface_t iface;
    memset(&iface, 0, sizeof(iface));
    strcpy(iface.if_name, "eth0");
    iface.num_addresses = 1;
    strcpy(iface.ip_addresses[0], "192.168.1.100");

    // Build announcement
    _mdns_server_build_interface_announcement(packet, &packet_len, "test.local", &server, 120, &iface);

    // Verify packet was created successfully
    TEST_ASSERT_GREATER_THAN(12, packet_len);
    TEST_ASSERT_LESS_OR_EQUAL_UINT(sizeof(packet), packet_len);
}

// Test building interface announcement with IPv6 only
void test_mdns_server_build_interface_announcement_ipv6_only(void) {
    uint8_t packet[2048];
    size_t packet_len = 0;

    // Create minimal server instance
    mdns_server_t server;
    memset(&server, 0, sizeof(server));
    char hostname_buffer[] = "test.local";
    server.hostname = hostname_buffer;
    server.num_services = 0;
    server.services = NULL;

    // Create interface with IPv6 address
    mdns_server_interface_t iface;
    memset(&iface, 0, sizeof(iface));
    strcpy(iface.if_name, "eth0");
    iface.num_addresses = 1;
    strcpy(iface.ip_addresses[0], "2001:db8::1");

    // Build announcement
    _mdns_server_build_interface_announcement(packet, &packet_len, "test.local", &server, 120, &iface);

    // Verify packet was created successfully
    TEST_ASSERT_GREATER_THAN(12, packet_len);
    TEST_ASSERT_LESS_OR_EQUAL_UINT(sizeof(packet), packet_len);
}

// Test building interface announcement with multiple IP addresses
void test_mdns_server_build_interface_announcement_multiple_ips(void) {
    uint8_t packet[2048];
    size_t packet_len = 0;

    // Create minimal server instance
    mdns_server_t server;
    memset(&server, 0, sizeof(server));
    char hostname_buffer[] = "test.local";
    server.hostname = hostname_buffer;
    server.num_services = 0;
    server.services = NULL;

    // Create interface with multiple IP addresses
    mdns_server_interface_t iface;
    memset(&iface, 0, sizeof(iface));
    strcpy(iface.if_name, "eth0");
    iface.num_addresses = 3;
    strcpy(iface.ip_addresses[0], "192.168.1.100");
    strcpy(iface.ip_addresses[1], "192.168.1.101");
    strcpy(iface.ip_addresses[2], "2001:db8::1");

    // Build announcement
    _mdns_server_build_interface_announcement(packet, &packet_len, "test.local", &server, 120, &iface);

    // Verify packet was created successfully
    TEST_ASSERT_GREATER_THAN(12, packet_len);
    TEST_ASSERT_LESS_OR_EQUAL_UINT(sizeof(packet), packet_len);

    // Should have multiple A/AAAA records
    dns_header_t *header = (dns_header_t *)packet;
    TEST_ASSERT_GREATER_OR_EQUAL(3, ntohs(header->ancount)); // At least 3 address records
}

// Test building interface announcement with long service name (truncation test)
void test_mdns_server_build_interface_announcement_long_service_name(void) {
    uint8_t packet[2048];
    size_t packet_len = 0;

    // Create server instance with very long service name
    mdns_server_t server;
    memset(&server, 0, sizeof(server));
    char hostname_buffer[] = "test.local";
    server.hostname = hostname_buffer;

    // Create a service with very long name
    mdns_server_service_t service;
    memset(&service, 0, sizeof(service));
    char service_name[] = "verylongservicenamethatexceedsnormallimitsandshouldbetruncated";
    char service_type[] = "_http._tcp.local";
    service.name = service_name;
    service.type = service_type;
    service.port = 8080;
    service.txt_records = NULL;
    service.num_txt_records = 0;

    server.services = &service;
    server.num_services = 1;

    // Create interface
    mdns_server_interface_t iface;
    memset(&iface, 0, sizeof(iface));
    strcpy(iface.if_name, "eth0");
    iface.num_addresses = 1;
    strcpy(iface.ip_addresses[0], "192.168.1.100");

    // Build announcement - should handle long names gracefully
    _mdns_server_build_interface_announcement(packet, &packet_len, "test.local", &server, 120, &iface);

    // Verify packet was created successfully despite long name
    TEST_ASSERT_GREATER_THAN(12, packet_len);
    TEST_ASSERT_LESS_OR_EQUAL_UINT(sizeof(packet), packet_len);
}

// Test building interface announcement packet size limit
void test_mdns_server_build_interface_announcement_packet_size_limit(void) {
    uint8_t packet[2048];
    size_t packet_len = 0;

    // Create server instance with many services to test packet size limits
    mdns_server_t server;
    memset(&server, 0, sizeof(server));
    char hostname_buffer[] = "test.local";
    server.hostname = hostname_buffer;

    // Create multiple services
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

    // Create interface with multiple IPs
    mdns_server_interface_t iface;
    memset(&iface, 0, sizeof(iface));
    strcpy(iface.if_name, "eth0");
    iface.num_addresses = 4;
    strcpy(iface.ip_addresses[0], "192.168.1.100");
    strcpy(iface.ip_addresses[1], "192.168.1.101");
    strcpy(iface.ip_addresses[2], "10.0.0.1");
    strcpy(iface.ip_addresses[3], "2001:db8::1");

    // Build announcement
    _mdns_server_build_interface_announcement(packet, &packet_len, "test.local", &server, 120, &iface);

    // Verify packet size is reasonable
    TEST_ASSERT_GREATER_THAN(100, packet_len);
    TEST_ASSERT_LESS_OR_EQUAL_UINT(1500, packet_len); // Should be under typical MTU
    TEST_ASSERT_LESS_OR_EQUAL_UINT(sizeof(packet), packet_len);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_mdns_server_build_interface_announcement_null_interface);

    return UNITY_END();
}