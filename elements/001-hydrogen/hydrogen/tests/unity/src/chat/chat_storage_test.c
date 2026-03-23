/*
 * Unity Tests for Chat Storage Module
 *
 * Phase 6: Tests the chat storage functionality for content-addressable storage
 * with SHA-256 hashing and Brotli compression.
 * Phase 7: Updated to match new function signatures with database parameter
 */

#include <src/hydrogen.h>
#include <unity.h>
#include <string.h>

#include <src/api/conduit/chat_common/chat_storage.h>
#include <src/utils/utils_compression.h>
#include <src/utils/utils_crypto.h>

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

static const char* test_message = "Hello, this is a test message for the chat storage module.";
static const char* test_message_2 = "This is a different message with different content.";
static const char* test_database = "testdb";

void setUp(void) {
}

void tearDown(void) {
}

void test_chat_storage_generate_hash(void) {
    char* hash = chat_storage_generate_hash(test_message, strlen(test_message));
    TEST_ASSERT_NOT_NULL(hash);
    TEST_ASSERT_TRUE(strlen(hash) > 0);
    chat_storage_free_hash(hash);
}

void test_chat_storage_generate_hash_from_json(void) {
    const char* json = "{\"role\": \"user\", \"content\": \"Hello, World!\"}";
    char* hash = chat_storage_generate_hash_from_json(json);
    TEST_ASSERT_NOT_NULL(hash);
    TEST_ASSERT_TRUE(strlen(hash) > 0);
    chat_storage_free_hash(hash);
}

void test_chat_storage_generate_hash_null_content(void) {
    char* hash = chat_storage_generate_hash(NULL, 100);
    TEST_ASSERT_NULL(hash);
}

void test_chat_storage_generate_hash_from_json_null(void) {
    char* hash = chat_storage_generate_hash_from_json(NULL);
    TEST_ASSERT_NULL(hash);
}

void test_chat_storage_compress_message(void) {
    uint8_t* compressed = NULL;
    size_t compressed_size = 0;
    bool result = chat_storage_compress_message(test_message, strlen(test_message), &compressed, &compressed_size);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_NOT_NULL(compressed);
    TEST_ASSERT_TRUE(compressed_size > 0);
    TEST_ASSERT_TRUE(compressed_size < strlen(test_message));
    chat_storage_free_compressed(compressed);
}

void test_chat_storage_decompress_message(void) {
    uint8_t* compressed = NULL;
    size_t compressed_size = 0;
    bool compress_result = chat_storage_compress_message(test_message, strlen(test_message), &compressed, &compressed_size);
    TEST_ASSERT_TRUE(compress_result);
    TEST_ASSERT_NOT_NULL(compressed);
    char* decompressed = NULL;
    size_t decompressed_size = 0;
    bool decompress_result = chat_storage_decompress_message(compressed, compressed_size, &decompressed, &decompressed_size);
    TEST_ASSERT_TRUE(decompress_result);
    TEST_ASSERT_NOT_NULL(decompressed);
    TEST_ASSERT_EQUAL_INT((int)strlen(test_message), (int)decompressed_size);
    TEST_ASSERT_EQUAL_STRING(test_message, decompressed);
    chat_storage_free_compressed(compressed);
    chat_storage_free_decompressed(decompressed);
}

void test_chat_storage_compress_decompress_roundtrip(void) {
    const char* messages[] = {
        "Short",
        "This is a medium length message for testing.",
        "A longer message that contains more text content and should achieve better compression ratios."
    };
    for (int i = 0; i < 3; i++) {
        uint8_t* compressed = NULL;
        size_t compressed_size = 0;
        bool compress_ok = chat_storage_compress_message(messages[i], strlen(messages[i]), &compressed, &compressed_size);
        TEST_ASSERT_TRUE(compress_ok);
        char* decompressed = NULL;
        size_t decompressed_size = 0;
        bool decompress_ok = chat_storage_decompress_message((const uint8_t*)compressed, compressed_size, &decompressed, &decompressed_size);
        TEST_ASSERT_TRUE(decompress_ok);
        TEST_ASSERT_EQUAL_STRING(messages[i], decompressed);
        chat_storage_free_compressed(compressed);
        chat_storage_free_decompressed(decompressed);
    }
}

void test_chat_storage_compress_null_params(void) {
    uint8_t* compressed = NULL;
    size_t compressed_size = 0;
    TEST_ASSERT_FALSE(chat_storage_compress_message(NULL, 100, &compressed, &compressed_size));
    TEST_ASSERT_FALSE(chat_storage_compress_message(test_message, strlen(test_message), NULL, &compressed_size));
    TEST_ASSERT_FALSE(chat_storage_compress_message(test_message, strlen(test_message), &compressed, NULL));
    TEST_ASSERT_FALSE(chat_storage_compress_message(test_message, 0, &compressed, &compressed_size));
}

void test_chat_storage_decompress_null_params(void) {
    char* decompressed = NULL;
    size_t decompressed_size = 0;
    const char* test_data = "test";
    TEST_ASSERT_FALSE(chat_storage_decompress_message(NULL, 10, &decompressed, &decompressed_size));
    TEST_ASSERT_FALSE(chat_storage_decompress_message((const uint8_t*)test_data, sizeof(test_data), NULL, &decompressed_size));
    TEST_ASSERT_FALSE(chat_storage_decompress_message((const uint8_t*)test_data, sizeof(test_data), &decompressed, NULL));
    TEST_ASSERT_FALSE(chat_storage_decompress_message((const uint8_t*)test_data, 0, &decompressed, &decompressed_size));
}

void test_chat_storage_hash_uniqueness(void) {
    char* hash1 = chat_storage_generate_hash(test_message, strlen(test_message));
    char* hash2 = chat_storage_generate_hash(test_message_2, strlen(test_message_2));
    TEST_ASSERT_NOT_NULL(hash1);
    TEST_ASSERT_NOT_NULL(hash2);
    TEST_ASSERT_FALSE(strcmp(hash1, hash2) == 0);
    chat_storage_free_hash(hash1);
    chat_storage_free_hash(hash2);
}

void test_chat_storage_hash_consistency(void) {
    char* hash1 = chat_storage_generate_hash(test_message, strlen(test_message));
    char* hash2 = chat_storage_generate_hash(test_message, strlen(test_message));
    TEST_ASSERT_NOT_NULL(hash1);
    TEST_ASSERT_NOT_NULL(hash2);
    TEST_ASSERT_TRUE(strcmp(hash1, hash2) == 0);
    chat_storage_free_hash(hash1);
    chat_storage_free_hash(hash2);
}

void test_chat_storage_store_segment_null_params(void) {
    char* result = chat_storage_store_segment(NULL, "test", 10);
    TEST_ASSERT_NULL(result);
    result = chat_storage_store_segment(test_database, NULL, 0);
    TEST_ASSERT_NULL(result);
}

void test_chat_storage_free_functions(void) {
    chat_storage_free_compressed(NULL);
    chat_storage_free_decompressed(NULL);
    chat_storage_free_hash(NULL);
    uint8_t* data = malloc(10);
    char* str = strdup("test");
    chat_storage_free_compressed(data);
    chat_storage_free_decompressed(str);
}

void test_chat_storage_segment_exists_null(void) {
    TEST_ASSERT_FALSE(chat_storage_segment_exists(NULL, "hash123"));
    TEST_ASSERT_FALSE(chat_storage_segment_exists(test_database, NULL));
}

void test_chat_storage_retrieve_segment_null(void) {
    char* result = chat_storage_retrieve_segment(NULL, "hash123");
    TEST_ASSERT_NULL(result);
    result = chat_storage_retrieve_segment(test_database, NULL);
    TEST_ASSERT_NULL(result);
}

void test_chat_storage_update_access_null(void) {
    TEST_ASSERT_FALSE(chat_storage_update_access(NULL, "hash123"));
    TEST_ASSERT_FALSE(chat_storage_update_access(test_database, NULL));
}

void test_chat_storage_get_stats_null_params(void) {
    size_t uncomp_size = 0;
    size_t comp_size = 0;
    double ratio = 0.0;
    int count = 0;
    TEST_ASSERT_FALSE(chat_storage_get_stats(NULL, "hash123", &uncomp_size, &comp_size, &ratio, &count));
    TEST_ASSERT_FALSE(chat_storage_get_stats(test_database, NULL, &uncomp_size, &comp_size, &ratio, &count));
    TEST_ASSERT_FALSE(chat_storage_get_stats(test_database, "hash123", NULL, &comp_size, &ratio, &count));
    TEST_ASSERT_FALSE(chat_storage_get_stats(test_database, "hash123", &uncomp_size, NULL, &ratio, &count));
    TEST_ASSERT_FALSE(chat_storage_get_stats(test_database, "hash123", &uncomp_size, &comp_size, NULL, &count));
    TEST_ASSERT_FALSE(chat_storage_get_stats(test_database, "hash123", &uncomp_size, &comp_size, &ratio, NULL));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_chat_storage_generate_hash);
    RUN_TEST(test_chat_storage_generate_hash_from_json);
    RUN_TEST(test_chat_storage_generate_hash_null_content);
    RUN_TEST(test_chat_storage_generate_hash_from_json_null);
    RUN_TEST(test_chat_storage_hash_uniqueness);
    RUN_TEST(test_chat_storage_hash_consistency);
    RUN_TEST(test_chat_storage_compress_message);
    RUN_TEST(test_chat_storage_compress_null_params);
    RUN_TEST(test_chat_storage_decompress_message);
    RUN_TEST(test_chat_storage_decompress_null_params);
    RUN_TEST(test_chat_storage_compress_decompress_roundtrip);
    RUN_TEST(test_chat_storage_store_segment_null_params);
    RUN_TEST(test_chat_storage_free_functions);
    RUN_TEST(test_chat_storage_segment_exists_null);
    RUN_TEST(test_chat_storage_retrieve_segment_null);
    RUN_TEST(test_chat_storage_update_access_null);
    RUN_TEST(test_chat_storage_get_stats_null_params);
    return UNITY_END();
}