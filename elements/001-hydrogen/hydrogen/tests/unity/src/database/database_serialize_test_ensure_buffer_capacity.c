/*
 * Unity Test File: Database JSON Ensure Buffer Capacity Function Tests
 * Tests for database_json_ensure_buffer_capacity function in database_json.c
 */

/*
 * CHANGELOG
 * 2025-11-18: Created comprehensive unit tests for database_json_ensure_buffer_capacity
 *             Tests parameter validation, capacity expansion, edge cases,
 *             and memory allocation scenarios for 75%+ coverage
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/database/database_serialize.h>

// Forward declarations for functions being tested
bool database_json_ensure_buffer_capacity(char** buffer, size_t current_size,
                                         size_t* capacity, size_t needed_size);

// Test function declarations
void test_database_json_ensure_buffer_capacity_null_buffer(void);
void test_database_json_ensure_buffer_capacity_null_capacity(void);
void test_database_json_ensure_buffer_capacity_sufficient_capacity(void);
void test_database_json_ensure_buffer_capacity_exact_capacity(void);
void test_database_json_ensure_buffer_capacity_needs_expansion(void);
void test_database_json_ensure_buffer_capacity_double_expansion(void);
void test_database_json_ensure_buffer_capacity_large_needed_size(void);
void test_database_json_ensure_buffer_capacity_zero_current_size(void);
void test_database_json_ensure_buffer_capacity_zero_needed_size(void);
void test_database_json_ensure_buffer_capacity_null_buffer_pointer(void);
void test_database_json_ensure_buffer_capacity_realloc_failure_simulation(void);
void test_database_json_ensure_buffer_capacity_multiple_expansions(void);
void test_database_json_ensure_buffer_capacity_edge_case_needed_exactly_fits(void);
void test_database_json_ensure_buffer_capacity_small_initial_buffer(void);
void test_database_json_ensure_buffer_capacity_preserve_content(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// ============================================================================
// Tests for database_json_ensure_buffer_capacity
// ============================================================================

void test_database_json_ensure_buffer_capacity_null_buffer(void) {
    size_t capacity = 100;
    bool result = database_json_ensure_buffer_capacity(NULL, 0, &capacity, 50);
    TEST_ASSERT_FALSE(result);
}

void test_database_json_ensure_buffer_capacity_null_capacity(void) {
    char* buffer = malloc(100);
    bool result = database_json_ensure_buffer_capacity(&buffer, 0, NULL, 50);
    TEST_ASSERT_FALSE(result);
    free(buffer);
}

void test_database_json_ensure_buffer_capacity_sufficient_capacity(void) {
    char* buffer = malloc(100);
    size_t capacity = 100;
    bool result = database_json_ensure_buffer_capacity(&buffer, 50, &capacity, 40);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(100, capacity); // Should not change
    TEST_ASSERT_NOT_NULL(buffer);
    free(buffer);
}

void test_database_json_ensure_buffer_capacity_exact_capacity(void) {
    char* buffer = malloc(100);
    size_t capacity = 100;
    bool result = database_json_ensure_buffer_capacity(&buffer, 60, &capacity, 40);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(100, capacity); // Should not change (60 + 40 = 100, which is not < 100)
    TEST_ASSERT_NOT_NULL(buffer);
    free(buffer);
}

void test_database_json_ensure_buffer_capacity_needs_expansion(void) {
    char* buffer = malloc(100);
    size_t capacity = 100;
    bool result = database_json_ensure_buffer_capacity(&buffer, 70, &capacity, 40);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(capacity >= 110); // Should be at least 100*2 = 200, or 70+40+1024 = 1134
    TEST_ASSERT_NOT_NULL(buffer);
    free(buffer);
}

void test_database_json_ensure_buffer_capacity_double_expansion(void) {
    char* buffer = malloc(100);
    size_t capacity = 100;
    bool result = database_json_ensure_buffer_capacity(&buffer, 0, &capacity, 250);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(capacity >= 250); // Should be at least 100*2 = 200, but since 200 < 0+250+1024, should be 1274
    TEST_ASSERT_NOT_NULL(buffer);
    free(buffer);
}

void test_database_json_ensure_buffer_capacity_large_needed_size(void) {
    char* buffer = malloc(100);
    size_t capacity = 100;
    bool result = database_json_ensure_buffer_capacity(&buffer, 10, &capacity, 2000);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(capacity >= 2010); // Should be at least 10 + 2000 + 1024 = 3034
    TEST_ASSERT_NOT_NULL(buffer);
    free(buffer);
}

void test_database_json_ensure_buffer_capacity_zero_current_size(void) {
    char* buffer = malloc(100);
    size_t capacity = 100;
    bool result = database_json_ensure_buffer_capacity(&buffer, 0, &capacity, 50);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(100, capacity); // 0 + 50 = 50 < 100, so no expansion
    TEST_ASSERT_NOT_NULL(buffer);
    free(buffer);
}

void test_database_json_ensure_buffer_capacity_zero_needed_size(void) {
    char* buffer = malloc(100);
    size_t capacity = 100;
    bool result = database_json_ensure_buffer_capacity(&buffer, 50, &capacity, 0);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(100, capacity); // 50 + 0 = 50 < 100, so no expansion
    TEST_ASSERT_NOT_NULL(buffer);
    free(buffer);
}

void test_database_json_ensure_buffer_capacity_null_buffer_pointer(void) {
    char* buffer = NULL;
    size_t capacity = 0;
    bool result = database_json_ensure_buffer_capacity(&buffer, 0, &capacity, 100);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(capacity >= 100); // Should allocate new buffer
    TEST_ASSERT_NOT_NULL(buffer);
    free(buffer);
}

void test_database_json_ensure_buffer_capacity_realloc_failure_simulation(void) {
    // This is hard to test directly since we can't easily simulate realloc failure
    // But we can test with valid parameters to ensure success path works
    char* buffer = malloc(50);
    size_t capacity = 50;
    bool result = database_json_ensure_buffer_capacity(&buffer, 40, &capacity, 20);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1084, capacity); // Should expand: max(50*2, 40+20+1024) = max(100, 1084) = 1084
    free(buffer);
}

void test_database_json_ensure_buffer_capacity_multiple_expansions(void) {
    char* buffer = malloc(100);
    size_t capacity = 100;

    // First expansion
    bool result1 = database_json_ensure_buffer_capacity(&buffer, 80, &capacity, 30);
    TEST_ASSERT_TRUE(result1);
    size_t first_capacity = capacity;
    TEST_ASSERT_TRUE(first_capacity >= 110);

    // Second expansion (should use the new capacity)
    bool result2 = database_json_ensure_buffer_capacity(&buffer, first_capacity - 10, &capacity, 50);
    TEST_ASSERT_TRUE(result2);
    TEST_ASSERT_TRUE(capacity >= first_capacity + 40); // At least previous + needed

    free(buffer);
}

void test_database_json_ensure_buffer_capacity_edge_case_needed_exactly_fits(void) {
    char* buffer = malloc(100);
    size_t capacity = 100;
    // Test case where current_size + needed_size exactly equals capacity
    bool result = database_json_ensure_buffer_capacity(&buffer, 50, &capacity, 50);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(100, capacity); // Should not expand (50 + 50 = 100, not < 100)
    free(buffer);
}

void test_database_json_ensure_buffer_capacity_small_initial_buffer(void) {
    char* buffer = malloc(10);
    size_t capacity = 10;
    bool result = database_json_ensure_buffer_capacity(&buffer, 5, &capacity, 20);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(capacity >= 25); // Should expand significantly
    free(buffer);
}

void test_database_json_ensure_buffer_capacity_preserve_content(void) {
    char* buffer = malloc(100);
    TEST_ASSERT_NOT_NULL(buffer); // Ensure malloc succeeded
    strcpy(buffer, "test content");
    size_t capacity = 100;

    bool result = database_json_ensure_buffer_capacity(&buffer, strlen("test content"), &capacity, 200);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("test content", buffer); // Content should be preserved
    TEST_ASSERT_TRUE(capacity >= 212); // strlen + 200 + 1024 minimum

    free(buffer);
}

int main(void) {
    UNITY_BEGIN();

    // Parameter validation tests
    RUN_TEST(test_database_json_ensure_buffer_capacity_null_buffer);
    RUN_TEST(test_database_json_ensure_buffer_capacity_null_capacity);

    // Basic functionality tests
    RUN_TEST(test_database_json_ensure_buffer_capacity_sufficient_capacity);
    RUN_TEST(test_database_json_ensure_buffer_capacity_exact_capacity);
    RUN_TEST(test_database_json_ensure_buffer_capacity_needs_expansion);
    RUN_TEST(test_database_json_ensure_buffer_capacity_double_expansion);
    RUN_TEST(test_database_json_ensure_buffer_capacity_large_needed_size);

    // Edge case tests
    RUN_TEST(test_database_json_ensure_buffer_capacity_zero_current_size);
    RUN_TEST(test_database_json_ensure_buffer_capacity_zero_needed_size);
    RUN_TEST(test_database_json_ensure_buffer_capacity_null_buffer_pointer);
    RUN_TEST(test_database_json_ensure_buffer_capacity_multiple_expansions);
    RUN_TEST(test_database_json_ensure_buffer_capacity_edge_case_needed_exactly_fits);
    RUN_TEST(test_database_json_ensure_buffer_capacity_small_initial_buffer);
    RUN_TEST(test_database_json_ensure_buffer_capacity_preserve_content);

    // Memory allocation tests (limited since we can't easily simulate failures)
    RUN_TEST(test_database_json_ensure_buffer_capacity_realloc_failure_simulation);

    return UNITY_END();
}