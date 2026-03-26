/*
 * Unity Test File: Chat Phase 10 - Cross-Server Segment Recovery Tests
 * This file contains unit tests for Phase 10 functionality
 *
 * Tests cover:
 * - Cache directory configuration changes
 * - Batch retrieval API (without database - invalid params)
 * - Pre-fetch function (without database - invalid params)
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers
#include <src/api/wschat/helpers/lru_cache.h>
#include <src/api/wschat/helpers/storage.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

// Forward declarations
void test_cache_dir_default(void);
void test_cache_dir_env_override(void);
void test_batch_retrieve_null_database(void);
void test_batch_retrieve_null_hashes(void);
void test_batch_retrieve_zero_count(void);
void test_prefetch_null_database(void);
void test_prefetch_null_hash(void);
void test_cache_two_tier_structure(void);
void test_cache_dir_no_database_in_path(void);

// Test database name (unused in Phase 10 but required by API)
static const char* TEST_DATABASE = "test_phase10_db";

void setUp(void) {
    // Clean up any test cache directory
    system("rm -rf ./cache 2>/dev/null");
    unsetenv(CHAT_CACHE_DIR_ENV_VAR);
}

void tearDown(void) {
    // Clean up after each test
    system("rm -rf ./cache 2>/dev/null");
}

// Test 1: Default cache directory is ./cache
void test_cache_dir_default(void) {
    char* dir = chat_lru_cache_get_dir(TEST_DATABASE);
    TEST_ASSERT_NOT_NULL(dir);

    // Should be "./cache" (relative to cwd)
    TEST_ASSERT_EQUAL_STRING("cache", dir);

    free(dir);
}

// Test 2: Cache directory can be overridden via environment variable
void test_cache_dir_env_override(void) {
    setenv(CHAT_CACHE_DIR_ENV_VAR, "/tmp/custom_cache", 1);

    char* dir = chat_lru_cache_get_dir(TEST_DATABASE);
    TEST_ASSERT_NOT_NULL(dir);

    // Should use the environment variable value
    TEST_ASSERT_EQUAL_STRING("/tmp/custom_cache", dir);

    free(dir);
    unsetenv(CHAT_CACHE_DIR_ENV_VAR);
}

// Test 3: Cache path does not include database name (no topology awareness)
void test_cache_dir_no_database_in_path(void) {
    char* dir1 = chat_lru_cache_get_dir("database_a");
    char* dir2 = chat_lru_cache_get_dir("database_b");
    char* dir3 = chat_lru_cache_get_dir("database_c");

    TEST_ASSERT_NOT_NULL(dir1);
    TEST_ASSERT_NOT_NULL(dir2);
    TEST_ASSERT_NOT_NULL(dir3);

    // All should return the same cache directory
    TEST_ASSERT_EQUAL_STRING(dir1, dir2);
    TEST_ASSERT_EQUAL_STRING(dir1, dir3);

    free(dir1);
    free(dir2);
    free(dir3);
}

// Test 4: Two-tier directory structure (first 2 chars as subfolder)
void test_cache_two_tier_structure(void) {
    // Create a mock cache to test the directory structure
    ChatLRUCache* cache = chat_lru_cache_init(TEST_DATABASE, 1024 * 1024);
    TEST_ASSERT_NOT_NULL(cache);

    // Get cache directory
    char* cache_dir = chat_lru_cache_get_dir(TEST_DATABASE);
    TEST_ASSERT_NOT_NULL(cache_dir);

    // Check that the cache directory exists
    struct stat st;
    int stat_result = stat(cache_dir, &st);
    TEST_ASSERT_EQUAL(0, stat_result);
    TEST_ASSERT_TRUE(S_ISDIR(st.st_mode));

    // Check that subdirectories 00-ff exist (two-tier structure)
    char prefix_dir[256];
    snprintf(prefix_dir, sizeof(prefix_dir), "%s/00", cache_dir);
    stat_result = stat(prefix_dir, &st);
    TEST_ASSERT_EQUAL(0, stat_result);
    TEST_ASSERT_TRUE(S_ISDIR(st.st_mode));

    snprintf(prefix_dir, sizeof(prefix_dir), "%s/ff", cache_dir);
    stat_result = stat(prefix_dir, &st);
    TEST_ASSERT_EQUAL(0, stat_result);
    TEST_ASSERT_TRUE(S_ISDIR(st.st_mode));

    free(cache_dir);
    chat_lru_cache_shutdown(cache);
}

// Test 5: Batch retrieve handles NULL database gracefully
void test_batch_retrieve_null_database(void) {
    const char* hashes[] = {"hash1", "hash2"};
    json_t* result = chat_storage_retrieve_segments_batch(NULL, hashes, 2);
    TEST_ASSERT_NULL(result);
}

// Test 6: Batch retrieve handles NULL hashes array gracefully
void test_batch_retrieve_null_hashes(void) {
    json_t* result = chat_storage_retrieve_segments_batch(TEST_DATABASE, NULL, 2);
    TEST_ASSERT_NULL(result);
}

// Test 7: Batch retrieve handles zero count gracefully
void test_batch_retrieve_zero_count(void) {
    const char* hashes[] = {"hash1"};
    json_t* result = chat_storage_retrieve_segments_batch(TEST_DATABASE, hashes, 0);
    TEST_ASSERT_NULL(result);
}

// Test 8: Pre-fetch handles NULL database gracefully
void test_prefetch_null_database(void) {
    bool result = chat_storage_prefetch_segment(NULL, "somehash");
    TEST_ASSERT_FALSE(result);
}

// Test 9: Pre-fetch handles NULL hash gracefully
void test_prefetch_null_hash(void) {
    bool result = chat_storage_prefetch_segment(TEST_DATABASE, NULL);
    TEST_ASSERT_FALSE(result);
}

int main(void) {
    UNITY_BEGIN();

    // Cache directory configuration tests
    RUN_TEST(test_cache_dir_default);
    RUN_TEST(test_cache_dir_env_override);
    RUN_TEST(test_cache_dir_no_database_in_path);
    RUN_TEST(test_cache_two_tier_structure);

    // Batch retrieval API tests (without database)
    RUN_TEST(test_batch_retrieve_null_database);
    RUN_TEST(test_batch_retrieve_null_hashes);
    RUN_TEST(test_batch_retrieve_zero_count);

    // Pre-fetch API tests (without database)
    RUN_TEST(test_prefetch_null_database);
    RUN_TEST(test_prefetch_null_hash);

    return UNITY_END();
}
