#include <src/hydrogen.h>
#include <unity.h>

#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

#include <src/api/wschat/helpers/storage_hash.h>
#include <string.h>

void test_storage_hash_null_content(void);
void test_storage_hash_zero_length(void);
void test_storage_hash_valid_content(void);
void test_storage_hash_deterministic(void);

void setUp(void) {}
void tearDown(void) {}

void test_storage_hash_null_content(void) {
    TEST_ASSERT_NULL(chat_storage_generate_hash(NULL, 100));
}

void test_storage_hash_zero_length(void) {
    const char *content = "some content";
    TEST_ASSERT_NULL(chat_storage_generate_hash(content, 0));
}

void test_storage_hash_valid_content(void) {
    const char *content = "hello world";
    char *hash = chat_storage_generate_hash(content, strlen(content));
    TEST_ASSERT_NOT_NULL(hash);
    TEST_ASSERT_TRUE(strlen(hash) > 0);
    free(hash);
}

void test_storage_hash_deterministic(void) {
    const char *content = "deterministic test";
    char *hash1 = chat_storage_generate_hash(content, strlen(content));
    char *hash2 = chat_storage_generate_hash(content, strlen(content));
    TEST_ASSERT_NOT_NULL(hash1);
    TEST_ASSERT_NOT_NULL(hash2);
    TEST_ASSERT_EQUAL_STRING(hash1, hash2);
    free(hash1);
    free(hash2);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_storage_hash_null_content);
    RUN_TEST(test_storage_hash_zero_length);
    RUN_TEST(test_storage_hash_valid_content);
    RUN_TEST(test_storage_hash_deterministic);
    return UNITY_END();
}
