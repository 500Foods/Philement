/*
 * Unity Test File: get_payload_files_by_prefix Function Tests
 * This file contains unit tests for the get_payload_files_by_prefix() function
 * from src/payload/payload_cache.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Forward declaration for the function being tested
bool get_payload_files_by_prefix(const char *prefix, PayloadFile **files, size_t *num_files, size_t *capacity);
void cleanup_payload_cache(void);
bool initialize_payload_cache(void);
bool is_payload_cache_available(void);

// Extern declaration for global cache (for testing)
extern PayloadCache global_payload_cache;

// Test function prototypes
void setUp(void);
void tearDown(void);
void test_get_payload_files_by_prefix_null_parameters(void);
void test_get_payload_files_by_prefix_cache_not_available(void);
void test_get_payload_files_by_prefix_empty_prefix(void);
void test_get_payload_files_by_prefix_specific_prefix(void);
void test_get_payload_files_by_prefix_no_matches(void);

void setUp(void) {
    // Clean up any existing state and initialize
    cleanup_payload_cache();
    initialize_payload_cache();
}

void tearDown(void) {
    // Clean up after each test
    cleanup_payload_cache();
}

void test_get_payload_files_by_prefix_null_parameters(void) {
    PayloadFile *files = NULL;
    size_t num_files = 0;
    size_t capacity = 0;

    // Test with null files parameter
    TEST_ASSERT_FALSE(get_payload_files_by_prefix("test", NULL, &num_files, &capacity));

    // Test with null num_files parameter
    TEST_ASSERT_FALSE(get_payload_files_by_prefix("test", &files, NULL, &capacity));

    // Test with null capacity parameter
    TEST_ASSERT_FALSE(get_payload_files_by_prefix("test", &files, &num_files, NULL));
}

void test_get_payload_files_by_prefix_cache_not_available(void) {
    PayloadFile *files = NULL;
    size_t num_files = 0;
    size_t capacity = 0;

    // Cache is initialized but not available (no payload loaded)
    TEST_ASSERT_FALSE(get_payload_files_by_prefix("test", &files, &num_files, &capacity));
}

void test_get_payload_files_by_prefix_empty_prefix(void) {
    // Set up cache with some files
    global_payload_cache.is_available = true;
    global_payload_cache.num_files = 2;
    global_payload_cache.capacity = 16;
    global_payload_cache.files = calloc(16, sizeof(PayloadFile));

    global_payload_cache.files[0].name = strdup("swagger/index.html");
    global_payload_cache.files[0].data = malloc(100);
    global_payload_cache.files[0].size = 100;
    global_payload_cache.files[0].is_compressed = false;

    global_payload_cache.files[1].name = strdup("terminal/app.js");
    global_payload_cache.files[1].data = malloc(200);
    global_payload_cache.files[1].size = 200;
    global_payload_cache.files[1].is_compressed = true;

    PayloadFile *files = NULL;
    size_t num_files = 0;
    size_t capacity = 0;

    // Empty prefix should return all files
    TEST_ASSERT_TRUE(get_payload_files_by_prefix("", &files, &num_files, &capacity));
    TEST_ASSERT_NOT_NULL(files);
    TEST_ASSERT_EQUAL(2, num_files);
    TEST_ASSERT_EQUAL(16, capacity); // Initial capacity of the cache array

    // Clean up returned files
    for (size_t i = 0; i < num_files; i++) {
        free(files[i].name);
        free(files[i].data);
    }
    free(files);
}

void test_get_payload_files_by_prefix_specific_prefix(void) {
    // Set up cache with files
    global_payload_cache.is_available = true;
    global_payload_cache.num_files = 3;
    global_payload_cache.capacity = 16;
    global_payload_cache.files = calloc(16, sizeof(PayloadFile));

    global_payload_cache.files[0].name = strdup("swagger/index.html");
    global_payload_cache.files[0].data = malloc(100);
    global_payload_cache.files[0].size = 100;

    global_payload_cache.files[1].name = strdup("swagger/app.js");
    global_payload_cache.files[1].data = malloc(200);
    global_payload_cache.files[1].size = 200;

    global_payload_cache.files[2].name = strdup("terminal/main.js");
    global_payload_cache.files[2].data = malloc(150);
    global_payload_cache.files[2].size = 150;

    PayloadFile *files = NULL;
    size_t num_files = 0;
    size_t capacity = 0;

    // Filter by "swagger/" prefix
    TEST_ASSERT_TRUE(get_payload_files_by_prefix("swagger/", &files, &num_files, &capacity));
    TEST_ASSERT_NOT_NULL(files);
    TEST_ASSERT_EQUAL(2, num_files);
    TEST_ASSERT_EQUAL(2, capacity); // Capacity matches number of matching files

    // Verify the returned files
    TEST_ASSERT_EQUAL_STRING("swagger/index.html", files[0].name);
    TEST_ASSERT_EQUAL_STRING("swagger/app.js", files[1].name);

    // Clean up
    for (size_t i = 0; i < num_files; i++) {
        free(files[i].name);
        free(files[i].data);
    }
    free(files);
}

void test_get_payload_files_by_prefix_no_matches(void) {
    // Set up cache with files
    global_payload_cache.is_available = true;
    global_payload_cache.num_files = 1;
    global_payload_cache.capacity = 16;
    global_payload_cache.files = calloc(16, sizeof(PayloadFile));

    global_payload_cache.files[0].name = strdup("swagger/index.html");
    global_payload_cache.files[0].data = malloc(100);
    global_payload_cache.files[0].size = 100;

    PayloadFile *files = NULL;
    size_t num_files = 0;
    size_t capacity = 0;

    // Filter by non-matching prefix
    TEST_ASSERT_TRUE(get_payload_files_by_prefix("terminal/", &files, &num_files, &capacity));
    TEST_ASSERT_NULL(files);
    TEST_ASSERT_EQUAL(0, num_files);
    TEST_ASSERT_EQUAL(0, capacity);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_get_payload_files_by_prefix_null_parameters);
    RUN_TEST(test_get_payload_files_by_prefix_cache_not_available);
    RUN_TEST(test_get_payload_files_by_prefix_empty_prefix);
    RUN_TEST(test_get_payload_files_by_prefix_specific_prefix);
    RUN_TEST(test_get_payload_files_by_prefix_no_matches);

    return UNITY_END();
}