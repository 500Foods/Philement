/*
 * Unity Test File: chat_storage_compress
 * This file contains unit tests for compression functions
 * in src/api/conduit/helpers/chat_storage_compress.c
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include source header
#include <src/api/wschat/helpers/storage_compress.h>

// Function prototypes
void test_compress_message_valid_json(void);
void test_compress_message_empty_string(void);
void test_compress_message_null_message(void);
void test_compress_message_null_compressed_ptr(void);
void test_compress_message_null_size_ptr(void);
void test_decompress_message_valid_data(void);
void test_decompress_message_null_data(void);
void test_decompress_message_zero_size(void);
void test_decompress_message_null_decompressed_ptr(void);
void test_decompress_message_null_size_ptr(void);
void test_roundtrip_compress_decompress(void);
void test_compress_message_large_json(void);
void test_compress_decompress_preserves_data(void);

void setUp(void) {
    // No setup needed
}

void tearDown(void) {
    // No cleanup needed
}

/* Test compress_message with valid JSON */
void test_compress_message_valid_json(void) {
    const char* message = "{\"key\": \"value\", \"data\": \"test\"}";
    size_t message_len = strlen(message);
    uint8_t* compressed_data = NULL;
    size_t compressed_size = 0;

    bool result = chat_storage_compress_message(message, message_len, &compressed_data, &compressed_size);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(compressed_data);
    TEST_ASSERT_GREATER_THAN(0, compressed_size);
    /* Note: Small strings may not compress well due to Brotli overhead */

    free(compressed_data);
}

/* Test compress_message with empty string */
void test_compress_message_empty_string(void) {
    const char* message = "";
    size_t message_len = 0;
    uint8_t* compressed_data = NULL;
    size_t compressed_size = 0;

    bool result = chat_storage_compress_message(message, message_len, &compressed_data, &compressed_size);

    /* Empty string should fail */
    TEST_ASSERT_FALSE(result);
}

/* Test compress_message with NULL message */
void test_compress_message_null_message(void) {
    uint8_t* compressed_data = NULL;
    size_t compressed_size = 0;

    bool result = chat_storage_compress_message(NULL, 10, &compressed_data, &compressed_size);
    TEST_ASSERT_FALSE(result);
}

/* Test compress_message with NULL compressed_data pointer */
void test_compress_message_null_compressed_ptr(void) {
    const char* message = "test";
    size_t compressed_size = 0;

    bool result = chat_storage_compress_message(message, strlen(message), NULL, &compressed_size);
    TEST_ASSERT_FALSE(result);
}

/* Test compress_message with NULL compressed_size pointer */
void test_compress_message_null_size_ptr(void) {
    const char* message = "test";
    uint8_t* compressed_data = NULL;

    bool result = chat_storage_compress_message(message, strlen(message), &compressed_data, NULL);
    TEST_ASSERT_FALSE(result);
}

/* Test decompress_message with valid compressed data */
void test_decompress_message_valid_data(void) {
    /* First compress some data */
    const char* original = "{\"message\": \"This is a test message for compression\"}";
    size_t original_len = strlen(original);
    uint8_t* compressed_data = NULL;
    size_t compressed_size = 0;

    bool compress_result = chat_storage_compress_message(original, original_len, &compressed_data, &compressed_size);
    TEST_ASSERT_TRUE(compress_result);

    /* Now decompress */
    char* decompressed_data = NULL;
    size_t decompressed_size = 0;

    bool result = chat_storage_decompress_message(compressed_data, compressed_size, &decompressed_data, &decompressed_size);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(decompressed_data);
    TEST_ASSERT_EQUAL(original_len, decompressed_size);
    TEST_ASSERT_EQUAL_STRING(original, decompressed_data);

    free(compressed_data);
    free(decompressed_data);
}

/* Test decompress_message with NULL compressed data */
void test_decompress_message_null_data(void) {
    char* decompressed_data = NULL;
    size_t decompressed_size = 0;

    bool result = chat_storage_decompress_message(NULL, 10, &decompressed_data, &decompressed_size);
    TEST_ASSERT_FALSE(result);
}

/* Test decompress_message with zero size */
void test_decompress_message_zero_size(void) {
    const uint8_t data[] = {0x01, 0x02};
    char* decompressed_data = NULL;
    size_t decompressed_size = 0;

    bool result = chat_storage_decompress_message(data, 0, &decompressed_data, &decompressed_size);
    TEST_ASSERT_FALSE(result);
}

/* Test decompress_message with NULL decompressed_data pointer */
void test_decompress_message_null_decompressed_ptr(void) {
    const uint8_t data[] = {0x01, 0x02, 0x03};
    size_t decompressed_size = 0;

    bool result = chat_storage_decompress_message(data, 3, NULL, &decompressed_size);
    TEST_ASSERT_FALSE(result);
}

/* Test decompress_message with NULL decompressed_size pointer */
void test_decompress_message_null_size_ptr(void) {
    const uint8_t data[] = {0x01, 0x02, 0x03};
    char* decompressed_data = NULL;

    bool result = chat_storage_decompress_message(data, 3, &decompressed_data, NULL);
    TEST_ASSERT_FALSE(result);
}

/* Test roundtrip: compress then decompress */
void test_roundtrip_compress_decompress(void) {
    const char* original = "The quick brown fox jumps over the lazy dog. 1234567890!@#$%";
    size_t original_len = strlen(original);

    /* Compress */
    uint8_t* compressed = NULL;
    size_t compressed_size = 0;
    bool compress_ok = chat_storage_compress_message(original, original_len, &compressed, &compressed_size);
    TEST_ASSERT_TRUE(compress_ok);

    /* Decompress */
    char* decompressed = NULL;
    size_t decompressed_size = 0;
    bool decompress_ok = chat_storage_decompress_message(compressed, compressed_size, &decompressed, &decompressed_size);
    TEST_ASSERT_TRUE(decompress_ok);

    /* Verify */
    TEST_ASSERT_EQUAL(original_len, decompressed_size);
    TEST_ASSERT_EQUAL_STRING(original, decompressed);

    free(compressed);
    free(decompressed);
}

/* Test compress_message with large JSON */
void test_compress_message_large_json(void) {
    /* Create a reasonably large JSON string */
    char large_message[4096];
    strcpy(large_message, "{\"data\": [");
    for (int i = 0; i < 100; i++) {
        char item[64];
        snprintf(item, sizeof(item), "%s{\"id\": %d, \"value\": \"item_%d\"}",
                 i > 0 ? "," : "", i, i);
        strcat(large_message, item);
    }
    strcat(large_message, "]}");

    size_t message_len = strlen(large_message);
    uint8_t* compressed_data = NULL;
    size_t compressed_size = 0;

    bool result = chat_storage_compress_message(large_message, message_len, &compressed_data, &compressed_size);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(compressed_data);
    /* Large JSON with repetition should compress well */
    TEST_ASSERT_LESS_THAN(message_len / 2, compressed_size);

    free(compressed_data);
}

/* Test compress then decompress preserves binary data */
void test_compress_decompress_preserves_data(void) {
    const char* original = "{\"special\": \"chars: \\n\\t\\r\\\"\\\\\"}";
    size_t original_len = strlen(original);

    uint8_t* compressed = NULL;
    size_t compressed_size = 0;
    bool compress_ok = chat_storage_compress_message(original, original_len, &compressed, &compressed_size);
    TEST_ASSERT_TRUE(compress_ok);

    char* decompressed = NULL;
    size_t decompressed_size = 0;
    bool decompress_ok = chat_storage_decompress_message(compressed, compressed_size, &decompressed, &decompressed_size);
    TEST_ASSERT_TRUE(decompress_ok);

    TEST_ASSERT_EQUAL_STRING(original, decompressed);

    free(compressed);
    free(decompressed);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_compress_message_valid_json);
    RUN_TEST(test_compress_message_empty_string);
    RUN_TEST(test_compress_message_null_message);
    RUN_TEST(test_compress_message_null_compressed_ptr);
    RUN_TEST(test_compress_message_null_size_ptr);

    RUN_TEST(test_decompress_message_valid_data);
    RUN_TEST(test_decompress_message_null_data);
    RUN_TEST(test_decompress_message_zero_size);
    RUN_TEST(test_decompress_message_null_decompressed_ptr);
    RUN_TEST(test_decompress_message_null_size_ptr);

    RUN_TEST(test_roundtrip_compress_decompress);
    RUN_TEST(test_compress_message_large_json);
    RUN_TEST(test_compress_decompress_preserves_data);

    return UNITY_END();
}
