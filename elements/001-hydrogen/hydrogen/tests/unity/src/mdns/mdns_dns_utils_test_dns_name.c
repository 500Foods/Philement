/*
 * Unity Test: mdns_linux_test_dns_name.c
 * Tests DNS name writing functions from mdns_linux.c
 *
 * This file follows the naming convention from tests/UNITY.md:
 * <source>_test_<function>.c where source is mdns_linux and function is dns_name
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/mdns/mdns_keys.h>

// Test function prototypes
void test_write_dns_name_simple(void);
void test_write_dns_name_multiple_labels(void);
void test_write_dns_name_empty(void);
void test_write_dns_name_max_length(void);
void test_write_dns_name_root_domain(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test writing a simple DNS name
void test_write_dns_name_simple(void) {
    uint8_t buffer[256];
    uint8_t *result;

    // Test simple name
    result = write_dns_name(buffer, "test.local");

    // Check that function returns pointer past the written data
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result > buffer);

    // Check the written data
    size_t expected_len = 4 + 1 + 5 + 1 + 1; // "test" + "." + "local" + "." + null terminator
    size_t actual_len = (size_t)(result - buffer);
    TEST_ASSERT_EQUAL_UINT(expected_len, actual_len);

    // Check content: [4]test[5]local[0]
    TEST_ASSERT_EQUAL_UINT8(4, buffer[0]);      // length of "test"
    TEST_ASSERT_EQUAL_STRING_LEN("test", (char*)&buffer[1], 4);
    TEST_ASSERT_EQUAL_UINT8(5, buffer[5]);      // length of "local"
    TEST_ASSERT_EQUAL_STRING_LEN("local", (char*)&buffer[6], 5);
    TEST_ASSERT_EQUAL_UINT8(0, buffer[11]);     // null terminator
}

// Test writing DNS name with multiple labels
void test_write_dns_name_multiple_labels(void) {
    uint8_t buffer[256];
    uint8_t *result;

    // Test name with multiple labels
    result = write_dns_name(buffer, "sub.domain.example.com");

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result > buffer);

    // Check content: [3]sub[6]domain[7]example[3]com[0]
    size_t offset = 0;
    TEST_ASSERT_EQUAL_UINT8(3, buffer[offset++]);      // "sub"
    TEST_ASSERT_EQUAL_STRING_LEN("sub", (char*)&buffer[offset], 3);
    offset += 3;
    TEST_ASSERT_EQUAL_UINT8(6, buffer[offset++]);      // "domain"
    TEST_ASSERT_EQUAL_STRING_LEN("domain", (char*)&buffer[offset], 6);
    offset += 6;
    TEST_ASSERT_EQUAL_UINT8(7, buffer[offset++]);      // "example"
    TEST_ASSERT_EQUAL_STRING_LEN("example", (char*)&buffer[offset], 7);
    offset += 7;
    TEST_ASSERT_EQUAL_UINT8(3, buffer[offset++]);      // "com"
    TEST_ASSERT_EQUAL_STRING_LEN("com", (char*)&buffer[offset], 3);
    offset += 3;
    TEST_ASSERT_EQUAL_UINT8(0, buffer[offset]);        // null terminator
}

// Test writing empty DNS name
void test_write_dns_name_empty(void) {
    uint8_t buffer[256];
    uint8_t *result;

    // Test empty name
    result = write_dns_name(buffer, "");

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result > buffer);

    // Should just write null terminator
    size_t actual_len = (size_t)(result - buffer);
    TEST_ASSERT_EQUAL_UINT(1, actual_len);
    TEST_ASSERT_EQUAL_UINT8(0, buffer[0]);
}

// Test writing DNS name at maximum reasonable length
void test_write_dns_name_max_length(void) {
    uint8_t buffer[256];
    uint8_t *result;

    // Create a long but valid DNS name (253 chars max)
    char long_name[254];
    memset(long_name, 'a', 250);
    long_name[250] = '.';
    long_name[251] = 'b';
    long_name[252] = '\0';

    result = write_dns_name(buffer, long_name);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result > buffer);

    // Check that it starts correctly
    TEST_ASSERT_EQUAL_UINT8(250, buffer[0]);     // 250 'a's
    TEST_ASSERT_EQUAL_UINT8('a', buffer[1]);
    TEST_ASSERT_EQUAL_UINT8('a', buffer[250]);
    TEST_ASSERT_EQUAL_UINT8(1, buffer[251]);     // "b"
    TEST_ASSERT_EQUAL_UINT8('b', buffer[252]);
    TEST_ASSERT_EQUAL_UINT8(0, buffer[253]);     // null terminator
}

// Test writing root domain
void test_write_dns_name_root_domain(void) {
    uint8_t buffer[256];
    uint8_t *result;

    // Test root domain (just a dot)
    result = write_dns_name(buffer, ".");

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(result > buffer);

    // Should just write null terminator for root domain
    size_t actual_len = (size_t)(result - buffer);
    TEST_ASSERT_EQUAL_UINT(2, actual_len);  // Root domain is [0] followed by null terminator
    TEST_ASSERT_EQUAL_UINT8(0, buffer[0]);  // First byte is length 0
    TEST_ASSERT_EQUAL_UINT8(0, buffer[1]);  // Second byte is null terminator
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_write_dns_name_simple);
    RUN_TEST(test_write_dns_name_multiple_labels);
    RUN_TEST(test_write_dns_name_empty);
    RUN_TEST(test_write_dns_name_max_length);
    RUN_TEST(test_write_dns_name_root_domain);

    return UNITY_END();
}
