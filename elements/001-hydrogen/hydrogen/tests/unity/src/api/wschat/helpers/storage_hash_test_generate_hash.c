/*
 * Unity Test File: chat_storage_hash
 * This file contains unit tests for hash generation functions
 * in src/api/conduit/helpers/chat_storage_hash.c
 */

#include <src/hydrogen.h>
#include <unity.h>

// Include source header
#include <src/api/wschat/helpers/storage_hash.h>

// Function prototypes
void test_generate_hash_valid_content(void);
void test_generate_hash_empty_content(void);
void test_generate_hash_null_content(void);
void test_generate_hash_zero_length(void);
void test_generate_hash_deterministic(void);
void test_generate_hash_different_inputs(void);
void test_generate_hash_from_json_valid(void);
void test_generate_hash_from_json_null(void);
void test_generate_hash_from_json_empty(void);
void test_hash_functions_consistency(void);
void test_generate_hash_binary_data(void);
void test_generate_hash_produces_base64url(void);

void setUp(void) {
    // No setup needed
}

void tearDown(void) {
    // No cleanup needed
}

/* Test generate_hash with valid content */
void test_generate_hash_valid_content(void) {
    const char* content = "Hello, World!";
    size_t length = strlen(content);

    char* hash = chat_storage_generate_hash(content, length);

    TEST_ASSERT_NOT_NULL(hash);
    /* SHA-256 hash is base64url encoded, typically 43-44 characters */
    TEST_ASSERT_GREATER_THAN(40, strlen(hash));

    free(hash);
}

/* Test generate_hash with empty string */
void test_generate_hash_empty_content(void) {
    char* hash = chat_storage_generate_hash("", 0);
    TEST_ASSERT_NULL(hash);
}

/* Test generate_hash with NULL content */
void test_generate_hash_null_content(void) {
    char* hash = chat_storage_generate_hash(NULL, 5);
    TEST_ASSERT_NULL(hash);
}

/* Test generate_hash with zero length */
void test_generate_hash_zero_length(void) {
    const char* content = "test";
    char* hash = chat_storage_generate_hash(content, 0);
    TEST_ASSERT_NULL(hash);
}

/* Test generate_hash determinism - same input produces same hash */
void test_generate_hash_deterministic(void) {
    const char* content = "deterministic test";
    size_t length = strlen(content);

    char* hash1 = chat_storage_generate_hash(content, length);
    char* hash2 = chat_storage_generate_hash(content, length);

    TEST_ASSERT_NOT_NULL(hash1);
    TEST_ASSERT_NOT_NULL(hash2);
    TEST_ASSERT_EQUAL_STRING(hash1, hash2);

    free(hash1);
    free(hash2);
}

/* Test generate_hash different inputs produce different hashes */
void test_generate_hash_different_inputs(void) {
    const char* content1 = "test1";
    const char* content2 = "test2";

    char* hash1 = chat_storage_generate_hash(content1, strlen(content1));
    char* hash2 = chat_storage_generate_hash(content2, strlen(content2));

    TEST_ASSERT_NOT_NULL(hash1);
    TEST_ASSERT_NOT_NULL(hash2);
    TEST_ASSERT_FALSE(strcmp(hash1, hash2) == 0);

    free(hash1);
    free(hash2);
}

/* Test generate_hash_from_json with valid JSON */
void test_generate_hash_from_json_valid(void) {
    const char* json = "{\"key\": \"value\"}";

    char* hash = chat_storage_generate_hash_from_json(json);

    TEST_ASSERT_NOT_NULL(hash);
    /* SHA-256 hash is base64url encoded, typically 43-44 characters */
    TEST_ASSERT_GREATER_THAN(40, strlen(hash));

    free(hash);
}

/* Test generate_hash_from_json with NULL */
void test_generate_hash_from_json_null(void) {
    char* hash = chat_storage_generate_hash_from_json(NULL);
    TEST_ASSERT_NULL(hash);
}

/* Test generate_hash_from_json with empty string */
void test_generate_hash_from_json_empty(void) {
    char* hash = chat_storage_generate_hash_from_json("");
    TEST_ASSERT_NOT_NULL(hash);
    /* Empty string still produces a valid hash */
    TEST_ASSERT_GREATER_THAN(40, strlen(hash));
    free(hash);
}

/* Test generate_hash and generate_hash_from_json produce same hash for same content */
void test_hash_functions_consistency(void) {
    const char* content = "test content";
    size_t length = strlen(content);

    char* hash1 = chat_storage_generate_hash(content, length);
    char* hash2 = chat_storage_generate_hash_from_json(content);

    TEST_ASSERT_NOT_NULL(hash1);
    TEST_ASSERT_NOT_NULL(hash2);
    TEST_ASSERT_EQUAL_STRING(hash1, hash2);

    free(hash1);
    free(hash2);
}

/* Test generate_hash with binary data containing null bytes */
void test_generate_hash_binary_data(void) {
    uint8_t data[] = {0x00, 0x01, 0x02, 0x00, 0xFF};
    size_t length = sizeof(data);

    char* hash = chat_storage_generate_hash((const char*)data, length);

    TEST_ASSERT_NOT_NULL(hash);
    TEST_ASSERT_GREATER_THAN(40, strlen(hash));

    free(hash);
}

/* Test generate_hash produces valid base64url string */
void test_generate_hash_produces_base64url(void) {
    const char* content = "test";
    char* hash = chat_storage_generate_hash(content, strlen(content));

    TEST_ASSERT_NOT_NULL(hash);

    /* Verify all characters are valid base64url characters */
    for (size_t i = 0; i < strlen(hash); i++) {
        char c = hash[i];
        bool is_base64url = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                            (c >= '0' && c <= '9') || c == '-' || c == '_';
        TEST_ASSERT_TRUE(is_base64url);
    }

    free(hash);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_generate_hash_valid_content);
    RUN_TEST(test_generate_hash_empty_content);
    RUN_TEST(test_generate_hash_null_content);
    RUN_TEST(test_generate_hash_zero_length);
    RUN_TEST(test_generate_hash_deterministic);
    RUN_TEST(test_generate_hash_different_inputs);

    RUN_TEST(test_generate_hash_from_json_valid);
    RUN_TEST(test_generate_hash_from_json_null);
    RUN_TEST(test_generate_hash_from_json_empty);

    RUN_TEST(test_hash_functions_consistency);
    RUN_TEST(test_generate_hash_binary_data);
    RUN_TEST(test_generate_hash_produces_base64url);

    return UNITY_END();
}
