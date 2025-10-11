/*
 * Unity Test: mdns_dns_utils_test.c
 * Tests mDNS DNS utility functions from mdns_dns_utils.c
 *
 * This file follows the naming convention from tests/UNITY.md:
 * <source>_test.c where source is mdns_dns_utils
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/mdns/mdns_keys.h>
#include <src/mdns/mdns_dns_utils.h>

// Test function prototypes
void test_write_dns_name_basic(void);
void test_write_dns_name_null_name(void);
void test_write_dns_name_empty_name(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test basic DNS name writing
void test_write_dns_name_basic(void) {
    uint8_t buffer[256];

    write_dns_name(buffer, "test.local");

    // Verify the written data
    TEST_ASSERT_EQUAL_UINT8(4, buffer[0]); // "test" length
    TEST_ASSERT_EQUAL_MEMORY("test", buffer + 1, 4);
    TEST_ASSERT_EQUAL_UINT8(5, buffer[5]); // "local" length
    TEST_ASSERT_EQUAL_MEMORY("local", buffer + 6, 5);
    TEST_ASSERT_EQUAL_UINT8(0, buffer[11]); // Null terminator
}

// Test DNS name writing with NULL name
void test_write_dns_name_null_name(void) {
    uint8_t buffer[256];
    uint8_t *result;

    result = write_dns_name(buffer, NULL);

    // Should write just a null terminator
    TEST_ASSERT_EQUAL_UINT8(0, buffer[0]);
    TEST_ASSERT_EQUAL_PTR(buffer + 1, result);
}

// Test DNS name writing with empty name
void test_write_dns_name_empty_name(void) {
    uint8_t buffer[256];
    uint8_t *result;

    result = write_dns_name(buffer, "");

    // Should write just a null terminator
    TEST_ASSERT_EQUAL_UINT8(0, buffer[0]);
    TEST_ASSERT_EQUAL_PTR(buffer + 1, result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_write_dns_name_basic);
    RUN_TEST(test_write_dns_name_null_name);
    RUN_TEST(test_write_dns_name_empty_name);

    return UNITY_END();
}