/*
 * Unity Test: mdns_linux_test_dns_records.c
 * Tests DNS record writing functions from mdns_linux.c
 * This will cover multiple record types for maximum coverage impact
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/mdns/mdns_keys.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Test function prototypes
void test_write_dns_record_basic_a_record(void);
void test_write_dns_record_aaaa_record(void);
void test_write_dns_ptr_record(void);
void test_write_dns_srv_record(void);
void test_write_dns_txt_record_single(void);
void test_write_dns_txt_record_multiple(void);
void test_write_dns_name_null_safety(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test basic A record writing
void test_write_dns_record_basic_a_record(void) {
    uint8_t buffer[256];
    uint8_t *result;
    struct in_addr test_addr;

    // Create test IP address
    inet_pton(AF_INET, "192.168.1.100", &test_addr);

    // Write A record
    result = write_dns_name(buffer, "test.local");
    result = write_dns_record(result, "test.local", MDNS_TYPE_A, MDNS_CLASS_IN, 120, &test_addr.s_addr, 4);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result > buffer);

    // Verify packet structure
    size_t packet_len = (size_t)(result - buffer);
    TEST_ASSERT_GREATER_THAN(12, packet_len); // Should be more than DNS header size
}

// Test AAAA record writing
void test_write_dns_record_aaaa_record(void) {
    uint8_t buffer[256];
    uint8_t *result;
    struct in6_addr test_addr;

    // Create test IPv6 address
    inet_pton(AF_INET6, "2001:db8::1", &test_addr);

    // Write AAAA record
    result = write_dns_name(buffer, "test.local");
    result = write_dns_record(result, "test.local", MDNS_TYPE_AAAA, MDNS_CLASS_IN, 120, &test_addr.s6_addr, 16);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result > buffer);

    size_t packet_len = (size_t)(result - buffer);
    TEST_ASSERT_GREATER_THAN(12, packet_len);
}

// Test PTR record writing
void test_write_dns_ptr_record(void) {
    uint8_t buffer[256];
    uint8_t *result;

    // Write PTR record
    result = write_dns_name(buffer, "_http._tcp.local");
    result = write_dns_ptr_record(result, "_http._tcp.local", "printer._http._tcp.local", 120);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result > buffer);

    size_t packet_len = (size_t)(result - buffer);
    TEST_ASSERT_GREATER_THAN(12, packet_len);
}

// Test SRV record writing
void test_write_dns_srv_record(void) {
    uint8_t buffer[256];
    uint8_t *result;

    // Write SRV record
    result = write_dns_name(buffer, "printer._http._tcp.local");
    result = write_dns_srv_record(result, "printer._http._tcp.local", 0, 0, 8080, "test.local", 120);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result > buffer);

    size_t packet_len = (size_t)(result - buffer);
    TEST_ASSERT_GREATER_THAN(12, packet_len);
}

// Test TXT record writing with single entry
void test_write_dns_txt_record_single(void) {
    uint8_t buffer[256];
    uint8_t *result;
    char record1[] = "key=value";
    char *txt_records[] = {record1};

    // Write TXT record
    result = write_dns_name(buffer, "printer._http._tcp.local");
    result = write_dns_txt_record(result, "printer._http._tcp.local", txt_records, 1, 120);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result > buffer);

    size_t packet_len = (size_t)(result - buffer);
    TEST_ASSERT_GREATER_THAN(12, packet_len);
}

// Test TXT record writing with multiple entries
void test_write_dns_txt_record_multiple(void) {
    uint8_t buffer[256];
    uint8_t *result;
    char record1[] = "key1=value1";
    char record2[] = "key2=value2";
    char record3[] = "key3=value3";
    char *txt_records[] = {record1, record2, record3};

    // Write TXT record with multiple entries
    result = write_dns_name(buffer, "printer._http._tcp.local");
    result = write_dns_txt_record(result, "printer._http._tcp.local", txt_records, 3, 120);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result > buffer);

    size_t packet_len = (size_t)(result - buffer);
    TEST_ASSERT_GREATER_THAN(12, packet_len);
}

// Test NULL safety in write_dns_name
void test_write_dns_name_null_safety(void) {
    uint8_t buffer[256];
    uint8_t *result;

    // Test NULL name parameter
    result = write_dns_name(buffer, NULL);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result > buffer);

    // Should write just null terminator
    size_t packet_len = (size_t)(result - buffer);
    TEST_ASSERT_EQUAL_UINT(1, packet_len);
    TEST_ASSERT_EQUAL_UINT8(0, buffer[0]);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_write_dns_record_basic_a_record);
    RUN_TEST(test_write_dns_record_aaaa_record);
    RUN_TEST(test_write_dns_ptr_record);
    RUN_TEST(test_write_dns_srv_record);
    RUN_TEST(test_write_dns_txt_record_single);
    RUN_TEST(test_write_dns_txt_record_multiple);
    RUN_TEST(test_write_dns_name_null_safety);

    return UNITY_END();
}
