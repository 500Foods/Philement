/*
 * Unity Test File: chat_storage_hex
 * This file contains unit tests for hex conversion functions
 * in src/api/conduit/helpers/chat_storage_hex.c
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include source header
#include <src/api/wschat/helpers/storage_hex.h>

// Function prototypes
void test_binary_to_hex_valid_data(void);
void test_binary_to_hex_single_byte(void);
void test_binary_to_hex_all_zeros(void);
void test_binary_to_hex_all_0xff(void);
void test_binary_to_hex_null_data(void);
void test_binary_to_hex_zero_length(void);
void test_hex_to_binary_valid_hex(void);
void test_hex_to_binary_single_byte(void);
void test_hex_to_binary_uppercase_hex(void);
void test_hex_to_binary_null_hex(void);
void test_hex_to_binary_null_output(void);
void test_hex_to_binary_null_length(void);
void test_hex_to_binary_odd_length(void);
void test_hex_to_binary_invalid_char(void);
void test_roundtrip_binary_hex_binary(void);
void test_roundtrip_hex_binary_hex(void);

void setUp(void) {
    // No setup needed
}

void tearDown(void) {
    // No cleanup needed
}

/* Test binary_to_hex with valid data */
void test_binary_to_hex_valid_data(void) {
    uint8_t data[] = {0x00, 0x01, 0x02, 0x0A, 0xFF};
    size_t len = sizeof(data);

    char* hex = chat_storage_binary_to_hex(data, len);

    TEST_ASSERT_NOT_NULL(hex);
    TEST_ASSERT_EQUAL_STRING("0001020aff", hex);

    free(hex);
}

/* Test binary_to_hex with single byte */
void test_binary_to_hex_single_byte(void) {
    const uint8_t data[] = {0x42};
    char* hex = chat_storage_binary_to_hex(data, 1);

    TEST_ASSERT_NOT_NULL(hex);
    TEST_ASSERT_EQUAL_STRING("42", hex);

    free(hex);
}

/* Test binary_to_hex with all zeros */
void test_binary_to_hex_all_zeros(void) {
    const uint8_t data[] = {0x00, 0x00, 0x00};
    char* hex = chat_storage_binary_to_hex(data, 3);

    TEST_ASSERT_NOT_NULL(hex);
    TEST_ASSERT_EQUAL_STRING("000000", hex);

    free(hex);
}

/* Test binary_to_hex with all 0xFF */
void test_binary_to_hex_all_0xff(void) {
    const uint8_t data[] = {0xFF, 0xFF, 0xFF};
    char* hex = chat_storage_binary_to_hex(data, 3);

    TEST_ASSERT_NOT_NULL(hex);
    TEST_ASSERT_EQUAL_STRING("ffffff", hex);

    free(hex);
}

/* Test binary_to_hex with NULL data */
void test_binary_to_hex_null_data(void) {
    char* hex = chat_storage_binary_to_hex(NULL, 5);
    TEST_ASSERT_NULL(hex);
}

/* Test binary_to_hex with zero length */
void test_binary_to_hex_zero_length(void) {
    const uint8_t data[] = {0x01, 0x02};
    char* hex = chat_storage_binary_to_hex(data, 0);
    TEST_ASSERT_NULL(hex);
}

/* Test hex_to_binary with valid hex string */
void test_hex_to_binary_valid_hex(void) {
    const char* hex = "0001020aff";
    uint8_t* data = NULL;
    size_t len = 0;

    bool result = chat_storage_hex_to_binary(hex, &data, &len);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_EQUAL(5, len);
    TEST_ASSERT_EQUAL(0x00, data[0]);
    TEST_ASSERT_EQUAL(0x01, data[1]);
    TEST_ASSERT_EQUAL(0x02, data[2]);
    TEST_ASSERT_EQUAL(0x0A, data[3]);
    TEST_ASSERT_EQUAL(0xFF, data[4]);

    free(data);
}

/* Test hex_to_binary with single byte */
void test_hex_to_binary_single_byte(void) {
    const char* hex = "42";
    uint8_t* data = NULL;
    size_t len = 0;

    bool result = chat_storage_hex_to_binary(hex, &data, &len);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_EQUAL(1, len);
    TEST_ASSERT_EQUAL(0x42, data[0]);

    free(data);
}

/* Test hex_to_binary with uppercase hex */
void test_hex_to_binary_uppercase_hex(void) {
    const char* hex = "DEADBEEF";
    uint8_t* data = NULL;
    size_t len = 0;

    bool result = chat_storage_hex_to_binary(hex, &data, &len);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_EQUAL(4, len);
    TEST_ASSERT_EQUAL(0xDE, data[0]);
    TEST_ASSERT_EQUAL(0xAD, data[1]);
    TEST_ASSERT_EQUAL(0xBE, data[2]);
    TEST_ASSERT_EQUAL(0xEF, data[3]);

    free(data);
}

/* Test hex_to_binary with NULL hex */
void test_hex_to_binary_null_hex(void) {
    uint8_t* data = NULL;
    size_t len = 0;

    bool result = chat_storage_hex_to_binary(NULL, &data, &len);
    TEST_ASSERT_FALSE(result);
}

/* Test hex_to_binary with NULL output pointer */
void test_hex_to_binary_null_output(void) {
    const char* hex = "0001";
    size_t len = 0;

    bool result = chat_storage_hex_to_binary(hex, NULL, &len);
    TEST_ASSERT_FALSE(result);
}

/* Test hex_to_binary with NULL length pointer */
void test_hex_to_binary_null_length(void) {
    const char* hex = "0001";
    uint8_t* data = NULL;

    bool result = chat_storage_hex_to_binary(hex, &data, NULL);
    TEST_ASSERT_FALSE(result);
}

/* Test hex_to_binary with odd length hex (invalid) */
void test_hex_to_binary_odd_length(void) {
    const char* hex = "001";
    uint8_t* data = NULL;
    size_t len = 0;

    bool result = chat_storage_hex_to_binary(hex, &data, &len);
    TEST_ASSERT_FALSE(result);
}

/* Test hex_to_binary with invalid hex character */
void test_hex_to_binary_invalid_char(void) {
    const char* hex = "0G";
    uint8_t* data = NULL;
    size_t len = 0;

    bool result = chat_storage_hex_to_binary(hex, &data, &len);
    TEST_ASSERT_FALSE(result);
}

/* Test roundtrip: binary -> hex -> binary */
void test_roundtrip_binary_hex_binary(void) {
    uint8_t original[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
    size_t original_len = sizeof(original);

    /* Convert to hex */
    char* hex = chat_storage_binary_to_hex(original, original_len);
    TEST_ASSERT_NOT_NULL(hex);

    /* Convert back to binary */
    uint8_t* restored = NULL;
    size_t restored_len = 0;
    bool result = chat_storage_hex_to_binary(hex, &restored, &restored_len);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(original_len, restored_len);
    TEST_ASSERT_EQUAL_MEMORY(original, restored, original_len);

    free(hex);
    free(restored);
}

/* Test roundtrip: hex -> binary -> hex */
void test_roundtrip_hex_binary_hex(void) {
    const char* original_hex = "deadbeefcafe";
    uint8_t* binary = NULL;
    size_t binary_len = 0;

    /* Convert to binary */
    bool result = chat_storage_hex_to_binary(original_hex, &binary, &binary_len);
    TEST_ASSERT_TRUE(result);

    /* Convert back to hex */
    char* restored_hex = chat_storage_binary_to_hex(binary, binary_len);
    TEST_ASSERT_NOT_NULL(restored_hex);
    TEST_ASSERT_EQUAL_STRING(original_hex, restored_hex);

    free(binary);
    free(restored_hex);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_binary_to_hex_valid_data);
    RUN_TEST(test_binary_to_hex_single_byte);
    RUN_TEST(test_binary_to_hex_all_zeros);
    RUN_TEST(test_binary_to_hex_all_0xff);
    RUN_TEST(test_binary_to_hex_null_data);
    RUN_TEST(test_binary_to_hex_zero_length);

    RUN_TEST(test_hex_to_binary_valid_hex);
    RUN_TEST(test_hex_to_binary_single_byte);
    RUN_TEST(test_hex_to_binary_uppercase_hex);
    RUN_TEST(test_hex_to_binary_null_hex);
    RUN_TEST(test_hex_to_binary_null_output);
    RUN_TEST(test_hex_to_binary_null_length);
    RUN_TEST(test_hex_to_binary_odd_length);
    RUN_TEST(test_hex_to_binary_invalid_char);

    RUN_TEST(test_roundtrip_binary_hex_binary);
    RUN_TEST(test_roundtrip_hex_binary_hex);

    return UNITY_END();
}
