/*
 * Unity Test: mdns_server_threads_test_process_query_packet.c
 * Tests mdns_server_process_query_packet function for DNS query processing
 * This function was extracted from responder_loop for better testability
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/mdns/mdns_keys.h>
#include <src/mdns/mdns_server.h>
#include <src/mdns/mdns_dns_utils.h>

// Forward declarations for functions being tested
bool mdns_server_process_query_packet(mdns_server_t *mdns_server_instance,
                                       const network_info_t *net_info_instance,
                                       const uint8_t *buffer,
                                       ssize_t len);

// Test function prototypes
void test_process_query_packet_null_server(void);
void test_process_query_packet_null_buffer(void);
void test_process_query_packet_invalid_length(void);
void test_process_query_packet_zero_questions(void);
void test_process_query_packet_ptr_match(void);
void test_process_query_packet_ptr_no_match(void);
void test_process_query_packet_srv_match(void);
void test_process_query_packet_txt_match(void);
void test_process_query_packet_a_match(void);
void test_process_query_packet_aaaa_match(void);
void test_process_query_packet_any_match(void);
void test_process_query_packet_a_no_match(void);
void test_process_query_packet_non_in_class(void);
void test_process_query_packet_multiple_questions(void);
void test_process_query_packet_no_services(void);

void setUp(void) {
    // Setup test fixtures
}

void tearDown(void) {
    // Clean up test fixtures
}

// Test with NULL mdns_server
void test_process_query_packet_null_server(void) {
    uint8_t buffer[512];
    memset(buffer, 0, sizeof(buffer));
    
    bool result = mdns_server_process_query_packet(NULL, NULL, buffer, 100);
    TEST_ASSERT_FALSE(result);
}

// Test with NULL buffer
void test_process_query_packet_null_buffer(void) {
    mdns_server_t server;
    memset(&server, 0, sizeof(server));
    
    bool result = mdns_server_process_query_packet(&server, NULL, NULL, 100);
    TEST_ASSERT_FALSE(result);
}

// Test with invalid length (too small for DNS header)
void test_process_query_packet_invalid_length(void) {
    mdns_server_t server;
    memset(&server, 0, sizeof(server));
    uint8_t buffer[512];
    memset(buffer, 0, sizeof(buffer));
    
    // Length smaller than DNS header
    bool result = mdns_server_process_query_packet(&server, NULL, buffer, 5);
    TEST_ASSERT_FALSE(result);
}

// Test with zero questions in DNS packet
void test_process_query_packet_zero_questions(void) {
    mdns_server_t server;
    memset(&server, 0, sizeof(server));
    server.hostname = (char *)"testhost.local";
    server.num_services = 0;
    
    // Create minimal DNS packet with 0 questions
    uint8_t buffer[512];
    memset(buffer, 0, sizeof(buffer));
    dns_header_t *header = (dns_header_t *)buffer;
    header->qdcount = htons(0);  // No questions
    
    bool result = mdns_server_process_query_packet(&server, NULL, buffer, sizeof(dns_header_t));
    TEST_ASSERT_TRUE(result);  // Should succeed but do nothing
}

// Test PTR query matching service type
void test_process_query_packet_ptr_match(void) {
    mdns_server_t server;
    memset(&server, 0, sizeof(server));
    server.hostname = (char *)"testhost.local";
    
    // Setup service
    mdns_server_service_t service;
    memset(&service, 0, sizeof(service));
    service.name = (char *)"Test Service";
    service.type = (char *)"_http._tcp.local";
    service.port = 80;
    
    server.services = &service;
    server.num_services = 1;
    
    // Create DNS PTR query packet for _http._tcp.local
    uint8_t buffer[512];
    memset(buffer, 0, sizeof(buffer));
    dns_header_t *header = (dns_header_t *)buffer;
    header->qdcount = htons(1);
    
    uint8_t *ptr = buffer + sizeof(dns_header_t);
    ptr = write_dns_name(ptr, "_http._tcp.local");
    *((uint16_t*)ptr) = htons(MDNS_TYPE_PTR); ptr += 2;
    *((uint16_t*)ptr) = htons(MDNS_CLASS_IN); ptr += 2;
    
    ssize_t len = ptr - buffer;
    
    bool result = mdns_server_process_query_packet(&server, NULL, buffer, len);
    TEST_ASSERT_TRUE(result);
}

// Test PTR query not matching any service
void test_process_query_packet_ptr_no_match(void) {
    mdns_server_t server;
    memset(&server, 0, sizeof(server));
    server.hostname = (char *)"testhost.local";
    
    // Setup service
    mdns_server_service_t service;
    memset(&service, 0, sizeof(service));
    service.name = (char *)"Test Service";
    service.type = (char *)"_http._tcp.local";
    service.port = 80;
    
    server.services = &service;
    server.num_services = 1;
    
    // Create DNS PTR query packet for different service type
    uint8_t buffer[512];
    memset(buffer, 0, sizeof(buffer));
    dns_header_t *header = (dns_header_t *)buffer;
    header->qdcount = htons(1);
    
    uint8_t *ptr = buffer + sizeof(dns_header_t);
    ptr = write_dns_name(ptr, "_printer._tcp.local");  // Different service
    *((uint16_t*)ptr) = htons(MDNS_TYPE_PTR); ptr += 2;
    *((uint16_t*)ptr) = htons(MDNS_CLASS_IN); ptr += 2;
    
    ssize_t len = ptr - buffer;
    
    bool result = mdns_server_process_query_packet(&server, NULL, buffer, len);
    TEST_ASSERT_TRUE(result);  // Processed successfully, just no match
}

// Test SRV query matching service
void test_process_query_packet_srv_match(void) {
    mdns_server_t server;
    memset(&server, 0, sizeof(server));
    server.hostname = (char *)"testhost.local";
    
    // Setup service
    mdns_server_service_t service;
    memset(&service, 0, sizeof(service));
    service.name = (char *)"Test Service";
    service.type = (char *)"_http._tcp.local";
    service.port = 80;
    
    server.services = &service;
    server.num_services = 1;
    
    // Create DNS SRV query packet for full service name
    uint8_t buffer[512];
    memset(buffer, 0, sizeof(buffer));
    dns_header_t *header = (dns_header_t *)buffer;
    header->qdcount = htons(1);
    
    uint8_t *ptr = buffer + sizeof(dns_header_t);
    ptr = write_dns_name(ptr, "Test Service._http._tcp.local");
    *((uint16_t*)ptr) = htons(MDNS_TYPE_SRV); ptr += 2;
    *((uint16_t*)ptr) = htons(MDNS_CLASS_IN); ptr += 2;
    
    ssize_t len = ptr - buffer;
    
    bool result = mdns_server_process_query_packet(&server, NULL, buffer, len);
    TEST_ASSERT_TRUE(result);
}

// Test TXT query matching service
void test_process_query_packet_txt_match(void) {
    mdns_server_t server;
    memset(&server, 0, sizeof(server));
    server.hostname = (char *)"testhost.local";
    
    // Setup service
    mdns_server_service_t service;
    memset(&service, 0, sizeof(service));
    service.name = (char *)"Test Service";
    service.type = (char *)"_http._tcp.local";
    service.port = 80;
    
    server.services = &service;
    server.num_services = 1;
    
    // Create DNS TXT query packet for full service name
    uint8_t buffer[512];
    memset(buffer, 0, sizeof(buffer));
    dns_header_t *header = (dns_header_t *)buffer;
    header->qdcount = htons(1);
    
    uint8_t *ptr = buffer + sizeof(dns_header_t);
    ptr = write_dns_name(ptr, "Test Service._http._tcp.local");
    *((uint16_t*)ptr) = htons(MDNS_TYPE_TXT); ptr += 2;
    *((uint16_t*)ptr) = htons(MDNS_CLASS_IN); ptr += 2;
    
    ssize_t len = ptr - buffer;
    
    bool result = mdns_server_process_query_packet(&server, NULL, buffer, len);
    TEST_ASSERT_TRUE(result);
}

// Test A query matching hostname
void test_process_query_packet_a_match(void) {
    mdns_server_t server;
    memset(&server, 0, sizeof(server));
    server.hostname = (char *)"testhost.local";
    server.num_services = 0;
    
    // Create DNS A query packet for hostname
    uint8_t buffer[512];
    memset(buffer, 0, sizeof(buffer));
    dns_header_t *header = (dns_header_t *)buffer;
    header->qdcount = htons(1);
    
    uint8_t *ptr = buffer + sizeof(dns_header_t);
    ptr = write_dns_name(ptr, "testhost.local");
    *((uint16_t*)ptr) = htons(MDNS_TYPE_A); ptr += 2;
    *((uint16_t*)ptr) = htons(MDNS_CLASS_IN); ptr += 2;
    
    ssize_t len = ptr - buffer;
    
    bool result = mdns_server_process_query_packet(&server, NULL, buffer, len);
    TEST_ASSERT_TRUE(result);
}

// Test AAAA query matching hostname
void test_process_query_packet_aaaa_match(void) {
    mdns_server_t server;
    memset(&server, 0, sizeof(server));
    server.hostname = (char *)"testhost.local";
    server.num_services = 0;
    
    // Create DNS AAAA query packet for hostname
    uint8_t buffer[512];
    memset(buffer, 0, sizeof(buffer));
    dns_header_t *header = (dns_header_t *)buffer;
    header->qdcount = htons(1);
    
    uint8_t *ptr = buffer + sizeof(dns_header_t);
    ptr = write_dns_name(ptr, "testhost.local");
    *((uint16_t*)ptr) = htons(MDNS_TYPE_AAAA); ptr += 2;
    *((uint16_t*)ptr) = htons(MDNS_CLASS_IN); ptr += 2;
    
    ssize_t len = ptr - buffer;
    
    bool result = mdns_server_process_query_packet(&server, NULL, buffer, len);
    TEST_ASSERT_TRUE(result);
}

// Test ANY query matching hostname
void test_process_query_packet_any_match(void) {
    mdns_server_t server;
    memset(&server, 0, sizeof(server));
    server.hostname = (char *)"testhost.local";
    server.num_services = 0;
    
    // Create DNS ANY query packet for hostname
    uint8_t buffer[512];
    memset(buffer, 0, sizeof(buffer));
    dns_header_t *header = (dns_header_t *)buffer;
    header->qdcount = htons(1);
    
    uint8_t *ptr = buffer + sizeof(dns_header_t);
    ptr = write_dns_name(ptr, "testhost.local");
    *((uint16_t*)ptr) = htons(MDNS_TYPE_ANY); ptr += 2;
    *((uint16_t*)ptr) = htons(MDNS_CLASS_IN); ptr += 2;
    
    ssize_t len = ptr - buffer;
    
    bool result = mdns_server_process_query_packet(&server, NULL, buffer, len);
    TEST_ASSERT_TRUE(result);
}

// Test A query not matching hostname
void test_process_query_packet_a_no_match(void) {
    mdns_server_t server;
    memset(&server, 0, sizeof(server));
    server.hostname = (char *)"testhost.local";
    server.num_services = 0;
    
    // Create DNS A query packet for different hostname
    uint8_t buffer[512];
    memset(buffer, 0, sizeof(buffer));
    dns_header_t *header = (dns_header_t *)buffer;
    header->qdcount = htons(1);
    
    uint8_t *ptr = buffer + sizeof(dns_header_t);
    ptr = write_dns_name(ptr, "otherhost.local");  // Different hostname
    *((uint16_t*)ptr) = htons(MDNS_TYPE_A); ptr += 2;
    *((uint16_t*)ptr) = htons(MDNS_CLASS_IN); ptr += 2;
    
    ssize_t len = ptr - buffer;
    
    bool result = mdns_server_process_query_packet(&server, NULL, buffer, len);
    TEST_ASSERT_TRUE(result);  // Processed successfully, just no match
}

// Test with non-IN class query
void test_process_query_packet_non_in_class(void) {
    mdns_server_t server;
    memset(&server, 0, sizeof(server));
    server.hostname = (char *)"testhost.local";
    server.num_services = 0;
    
    // Create DNS query with non-IN class
    uint8_t buffer[512];
    memset(buffer, 0, sizeof(buffer));
    dns_header_t *header = (dns_header_t *)buffer;
    header->qdcount = htons(1);
    
    uint8_t *ptr = buffer + sizeof(dns_header_t);
    ptr = write_dns_name(ptr, "testhost.local");
    *((uint16_t*)ptr) = htons(MDNS_TYPE_A); ptr += 2;
    *((uint16_t*)ptr) = htons(3);  // CLASS_CH (Chaos), not IN
    ptr += 2;
    
    ssize_t len = ptr - buffer;
    
    bool result = mdns_server_process_query_packet(&server, NULL, buffer, len);
    TEST_ASSERT_TRUE(result);  // Should process but ignore non-IN queries
}

// Test with multiple questions
void test_process_query_packet_multiple_questions(void) {
    mdns_server_t server;
    memset(&server, 0, sizeof(server));
    server.hostname = (char *)"testhost.local";
    
    // Setup service
    mdns_server_service_t service;
    memset(&service, 0, sizeof(service));
    service.name = (char *)"Test Service";
    service.type = (char *)"_http._tcp.local";
    service.port = 80;
    
    server.services = &service;
    server.num_services = 1;
    
    // Create DNS packet with 2 questions
    uint8_t buffer[512];
    memset(buffer, 0, sizeof(buffer));
    dns_header_t *header = (dns_header_t *)buffer;
    header->qdcount = htons(2);
    
    uint8_t *ptr = buffer + sizeof(dns_header_t);
    
    // First question: PTR for service
    ptr = write_dns_name(ptr, "_http._tcp.local");
    *((uint16_t*)ptr) = htons(MDNS_TYPE_PTR); ptr += 2;
    *((uint16_t*)ptr) = htons(MDNS_CLASS_IN); ptr += 2;
    
    // Second question: A for hostname
    ptr = write_dns_name(ptr, "testhost.local");
    *((uint16_t*)ptr) = htons(MDNS_TYPE_A); ptr += 2;
    *((uint16_t*)ptr) = htons(MDNS_CLASS_IN); ptr += 2;
    
    ssize_t len = ptr - buffer;
    
    bool result = mdns_server_process_query_packet(&server, NULL, buffer, len);
    TEST_ASSERT_TRUE(result);  // Should match first question and return
}

// Test with empty services array
void test_process_query_packet_no_services(void) {
    mdns_server_t server;
    memset(&server, 0, sizeof(server));
    server.hostname = (char *)"testhost.local";
    server.services = NULL;
    server.num_services = 0;
    
    // Create DNS PTR query
    uint8_t buffer[512];
    memset(buffer, 0, sizeof(buffer));
    dns_header_t *header = (dns_header_t *)buffer;
    header->qdcount = htons(1);
    
    uint8_t *ptr = buffer + sizeof(dns_header_t);
    ptr = write_dns_name(ptr, "_http._tcp.local");
    *((uint16_t*)ptr) = htons(MDNS_TYPE_PTR); ptr += 2;
    *((uint16_t*)ptr) = htons(MDNS_CLASS_IN); ptr += 2;
    
    ssize_t len = ptr - buffer;
    
    bool result = mdns_server_process_query_packet(&server, NULL, buffer, len);
    TEST_ASSERT_TRUE(result);  // Should process but not match anything
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_process_query_packet_null_server);
    RUN_TEST(test_process_query_packet_null_buffer);
    RUN_TEST(test_process_query_packet_invalid_length);
    RUN_TEST(test_process_query_packet_zero_questions);
    RUN_TEST(test_process_query_packet_ptr_match);
    RUN_TEST(test_process_query_packet_ptr_no_match);
    RUN_TEST(test_process_query_packet_srv_match);
    RUN_TEST(test_process_query_packet_txt_match);
    RUN_TEST(test_process_query_packet_a_match);
    RUN_TEST(test_process_query_packet_aaaa_match);
    RUN_TEST(test_process_query_packet_any_match);
    RUN_TEST(test_process_query_packet_a_no_match);
    RUN_TEST(test_process_query_packet_non_in_class);
    RUN_TEST(test_process_query_packet_multiple_questions);
    RUN_TEST(test_process_query_packet_no_services);
    
    return UNITY_END();
}