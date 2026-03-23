/*
 * Unity Tests for Chat Storage Module
 *
 * Phase 6: Tests the chat storage functionality for content-addressable storage
 * with SHA-256 hashing and Brotli compression.
 */

#include <src/hydrogen.h>
#include <unity.h>
#include <string.h>

#include <src/api/conduit/chat_common/chat_storage.h>
#include <src/utils/utils_compression.h>
#include <src/utils/utils_crypto.h>

// Test function prototypes
void test_chat_storage_generate_hash(void);
void test_chat_storage_generate_hash_from_json(void);
void test_chat_storage_compress_message(void);
void test_chat_storage_decompress_message(void);
void test_chat_storage_compress_decompress_roundtrip(void);
void test_chat_storage_compress_null_params(void);
void test_chat_storage_decompress_null_params(void);
void test_chat_storage_hash_uniqueness(void);
void test_chat_storage_hash_consistency(void);
void test_chat_storage_store_segment_null_params(void);
void test_chat_storage_free_functions(void);
void test_chat_storage_segment_exists_null(void);
void test_chat_storage_retrieve_segment_null(void);
void test_chat_storage_update_access_null(void);
void test_chat_storage_get_stats_null_params(void);
void test_chat_storage_generate_hash_null_content(void);
void test_chat_storage_generate_hash_from_json_null(void);

// Test data
static const char* test_message = "Hello, this is a test message for the chat storage module.";
static const char* test_message_2 = "This is a different message with different content.";

void setUp(void) {
}

void tearDown(void) {
}

// Test SHA-256 hash generation from content
void test_chat_storage_generate_hash(void) {
    char* hash = chat_storage_generate_hash(test_message, strlen(test_message));
    TEST_ASSERT_NOT_NULL(hash);
    TEST_ASSERT_TRUE(strlen(hash) > 0);

    chat_storage_free_hash(hash);
}

// Test SHA-256 hash generation from JSON string
void test_chat_storage_generate_hash_from_json(void) {
    const char* json = "{\"role\": \"user\", \"content\": \"Hello, World!\"}";
    char* hash = chat_storage_generate_hash_from_json(json);
    TEST_ASSERT_NOT_NULL(hash);
    TEST_ASSERT_TRUE(strlen(hash) > 0);

    chat_storage_free_hash(hash);
}

// Test hash from NULL content
void test_chat_storage_generate_hash_null_content(void) {
    char* hash = chat_storage_generate_hash(NULL, 100);
    TEST_ASSERT_NULL(hash);
}

// Test hash from NULL JSON
void test_chat_storage_generate_hash_from_json_null(void) {
    char* hash = chat_storage_generate_hash_from_json(NULL);
    TEST_ASSERT_NULL(hash);
}

// Test message compression
void test_chat_storage_compress_message(void) {
    uint8_t* compressed = NULL;
    size_t compressed_size = 0;

    bool result = chat_storage_compress_message(
        test_message, strlen(test_message),
        &compressed, &compressed_size);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(compressed);
    TEST_ASSERT_TRUE(compressed_size > 0);
    TEST_ASSERT_TRUE(compressed_size < strlen(test_message));

    chat_storage_free_compressed(compressed);
}

// Test message decompression
void test_chat_storage_decompress_message(void) {
    uint8_t* compressed = NULL;
    size_t compressed_size = 0;

    bool compress_result = chat_storage_compress_message(
        test_message, strlen(test_message),
        &compressed, &compressed_size);
    TEST_ASSERT_TRUE(compress_result);

    char* decompressed = NULL;
    size_t decompressed_size = 0;

    bool decompress_result = chat_storage_decompress_message(
        compressed, compressed_size,
        &decompressed, &decompressed_size);

    TEST_ASSERT_TRUE(decompress_result);
    TEST_ASSERT_NOT_NULL(decompressed);
    TEST_ASSERT_EQUAL(strlen(test_message), decompressed_size);
    TEST_ASSERT_EQUAL_STRING(test_message, decompressed);

    chat_storage_free_compressed(compressed);
    chat_storage_free_decompressed(decompressed);
}

// Test full compression/decompression roundtrip
void test_chat_storage_compress_decompress_roundtrip(void) {
    const char* messages[] = {
        "Short",
        "This is a medium length message for testing the compression and decompression pipeline.",
        "A longer message that contains more text content and should achieve better compression ratios when processed through the Brotli algorithm designed for content-addressable storage systems.",
        "{\"role\": \"user\", \"content\": \"Hello, how are you?\"}",
        ""
    };

    for (size_t i = 0; i < sizeof(messages) / sizeof(messages[0]); i++) {
        if (strlen(messages[i]) == 0) continue;

        uint8_t* compressed = NULL;
        size_t compressed_size = 0;

        bool compress_result = chat_storage_compress_message(
            messages[i], strlen(messages[i]),
            &compressed, &compressed_size);
        TEST_ASSERT_TRUE_MESSAGE(compress_result, "Failed to compress message");

        char* decompressed = NULL;
        size_t decompressed_size = 0;

        bool decompress_result = chat_storage_decompress_message(
            compressed, compressed_size,
            &decompressed, &decompressed_size);
        TEST_ASSERT_TRUE_MESSAGE(decompress_result, "Failed to decompress message");

        TEST_ASSERT_EQUAL_STRING_MESSAGE(messages[i], decompressed, "Roundtrip failed");

        chat_storage_free_compressed(compressed);
        chat_storage_free_decompressed(decompressed);
    }
}

// Test compression with null parameters
void test_chat_storage_compress_null_params(void) {
    uint8_t* compressed = NULL;
    size_t compressed_size = 0;

    // Test NULL message
    TEST_ASSERT_FALSE(chat_storage_compress_message(NULL, 100, &compressed, &compressed_size));

    // Test NULL output data pointer
    TEST_ASSERT_FALSE(chat_storage_compress_message(test_message, strlen(test_message), NULL, &compressed_size));

    // Test NULL output size pointer
    TEST_ASSERT_FALSE(chat_storage_compress_message(test_message, strlen(test_message), &compressed, NULL));

    // Test zero length
    TEST_ASSERT_FALSE(chat_storage_compress_message(test_message, 0, &compressed, &compressed_size));
}

// Test decompression with null parameters
void test_chat_storage_decompress_null_params(void) {
    uint8_t test_data[] = {0x00, 0x01, 0x02};
    char* decompressed = NULL;
    size_t decompressed_size = 0;

    // Test NULL input data
    TEST_ASSERT_FALSE(chat_storage_decompress_message(NULL, 10, &decompressed, &decompressed_size));

    // Test NULL output data pointer
    TEST_ASSERT_FALSE(chat_storage_decompress_message(test_data, sizeof(test_data), NULL, &decompressed_size));

    // Test NULL output size pointer
    TEST_ASSERT_FALSE(chat_storage_decompress_message(test_data, sizeof(test_data), &decompressed, NULL));

    // Test zero size
    TEST_ASSERT_FALSE(chat_storage_decompress_message(test_data, 0, &decompressed, &decompressed_size));
}

// Test hash uniqueness (different content produces different hashes)
void test_chat_storage_hash_uniqueness(void) {
    char* hash1 = chat_storage_generate_hash(test_message, strlen(test_message));
    char* hash2 = chat_storage_generate_hash(test_message_2, strlen(test_message_2));

    TEST_ASSERT_NOT_NULL(hash1);
    TEST_ASSERT_NOT_NULL(hash2);
    TEST_ASSERT_FALSE(strcmp(hash1, hash2) == 0);

    chat_storage_free_hash(hash1);
    chat_storage_free_hash(hash2);
}

// Test that same content produces same hash (deduplication)
void test_chat_storage_hash_consistency(void) {
    char* hash1 = chat_storage_generate_hash(test_message, strlen(test_message));
    char* hash2 = chat_storage_generate_hash(test_message, strlen(test_message));

    TEST_ASSERT_NOT_NULL(hash1);
    TEST_ASSERT_NOT_NULL(hash2);
    TEST_ASSERT_TRUE(strcmp(hash1, hash2) == 0);

    chat_storage_free_hash(hash1);
    chat_storage_free_hash(hash2);
}

// Test store segment with NULL parameters
void test_chat_storage_store_segment_null_params(void) {
    // Test NULL message
    char* result = chat_storage_store_segment(NULL, 100);
    TEST_ASSERT_NULL(result);

    // Test zero length
    result = chat_storage_store_segment(test_message, 0);
    TEST_ASSERT_NULL(result);
}

// Test free functions
void test_chat_storage_free_functions(void) {
    // Should not crash on NULL
    chat_storage_free_compressed(NULL);
    chat_storage_free_decompressed(NULL);
    chat_storage_free_hash(NULL);

    // Test with valid data
    uint8_t* data = malloc(100);
    char* str = malloc(100);

    chat_storage_free_compressed(data);
    chat_storage_free_decompressed(str);
    // Cannot free hash as it uses special encoding
}

// Test segment_exists with NULL
void test_chat_storage_segment_exists_null(void) {
    TEST_ASSERT_FALSE(chat_storage_segment_exists(NULL));
}

// Test retrieve_segment with NULL
void test_chat_storage_retrieve_segment_null(void) {
    char* result = chat_storage_retrieve_segment(NULL);
    TEST_ASSERT_NULL(result);
}

// Test update_access with NULL
void test_chat_storage_update_access_null(void) {
    TEST_ASSERT_FALSE(chat_storage_update_access(NULL));
}

// Test get_stats with NULL parameters
void test_chat_storage_get_stats_null_params(void) {
    size_t uncomp_size = 0;
    size_t comp_size = 0;
    double ratio = 0.0;
    int count = 0;

    // Test NULL hash
    TEST_ASSERT_FALSE(chat_storage_get_stats(NULL, &uncomp_size, &comp_size, &ratio, &count));

    // Test NULL output pointers
    TEST_ASSERT_FALSE(chat_storage_get_stats("hash123", NULL, &comp_size, &ratio, &count));
    TEST_ASSERT_FALSE(chat_storage_get_stats("hash123", &uncomp_size, NULL, &ratio, &count));
    TEST_ASSERT_FALSE(chat_storage_get_stats("hash123", &uncomp_size, &comp_size, NULL, &count));
    TEST_ASSERT_FALSE(chat_storage_get_stats("hash123", &uncomp_size, &comp_size, &ratio, NULL));
}

// Main test runner
int main(void) {
    UNITY_BEGIN();

    // Hash generation tests
    RUN_TEST(test_chat_storage_generate_hash);
    RUN_TEST(test_chat_storage_generate_hash_from_json);
    RUN_TEST(test_chat_storage_generate_hash_null_content);
    RUN_TEST(test_chat_storage_generate_hash_from_json_null);
    RUN_TEST(test_chat_storage_hash_uniqueness);
    RUN_TEST(test_chat_storage_hash_consistency);

    // Compression tests
    RUN_TEST(test_chat_storage_compress_message);
    RUN_TEST(test_chat_storage_compress_null_params);

    // Decompression tests
    RUN_TEST(test_chat_storage_decompress_message);
    RUN_TEST(test_chat_storage_decompress_null_params);

    // Roundtrip tests
    RUN_TEST(test_chat_storage_compress_decompress_roundtrip);

    // Storage pipeline tests
    RUN_TEST(test_chat_storage_store_segment_null_params);

    // Utility function tests
    RUN_TEST(test_chat_storage_free_functions);
    RUN_TEST(test_chat_storage_segment_exists_null);
    RUN_TEST(test_chat_storage_retrieve_segment_null);
    RUN_TEST(test_chat_storage_update_access_null);
    RUN_TEST(test_chat_storage_get_stats_null_params);

    return UNITY_END();
}