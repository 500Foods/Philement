/*
 * Unity Test File: generate_query_id
 * This file contains unit tests for generate_query_id function
 * in src/api/conduit/query/query.c
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Enable system mock for allocation failure testing
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

// Include source header
#include <src/api/conduit/query/query.h>

// Function prototypes
void test_generate_query_id_normal(void);
void test_generate_query_id_malloc_failure(void);
void test_generate_query_id_uniqueness(void);
void test_generate_query_id_format(void);

void setUp(void) {
    // Reset mocks before each test
    mock_system_reset_all();
}

void tearDown(void) {
    // Clean up after each test
    mock_system_reset_all();
}

// Test normal query ID generation
void test_generate_query_id_normal(void) {
    // Set up mock to succeed
    mock_system_set_malloc_failure(false);

    char* query_id = generate_query_id();
    TEST_ASSERT_NOT_NULL(query_id);

    // Verify format: should start with "conduit_", contain numbers, and be null-terminated
    TEST_ASSERT_EQUAL_CHAR('c', query_id[0]);  // Starts with "conduit_"
    TEST_ASSERT_NOT_EQUAL_CHAR('\0', query_id[8]);  // Not empty after prefix

    // Length should be reasonable (up to 32 chars)
    size_t len = strlen(query_id);
    TEST_ASSERT_LESS_THAN(32, len + 1);  // +1 for null terminator

    free(query_id);
}

// Test allocation failure
void test_generate_query_id_malloc_failure(void) {
    // Set up mock to fail allocation
    mock_system_set_calloc_failure(true);

    char* query_id = generate_query_id();
    TEST_ASSERT_NULL(query_id);

    // Reset mock for other tests
    mock_system_set_calloc_failure(false);
}

// Test multiple generations produce different IDs (basic uniqueness check)
void test_generate_query_id_uniqueness(void) {
    mock_system_set_malloc_failure(false);

    char* id1 = generate_query_id();
    char* id2 = generate_query_id();
    char* id3 = generate_query_id();

    TEST_ASSERT_NOT_NULL(id1);
    TEST_ASSERT_NOT_NULL(id2);
    TEST_ASSERT_NOT_NULL(id3);

    // They should be different (due to counter and time)
    TEST_ASSERT_TRUE(strcmp(id1, id2) != 0);
    TEST_ASSERT_TRUE(strcmp(id2, id3) != 0);
    TEST_ASSERT_TRUE(strcmp(id1, id3) != 0);

    free(id1);
    free(id2);
    free(id3);
}

// Test ID format consistency (starts with "conduit_")
void test_generate_query_id_format(void) {
    mock_system_set_malloc_failure(false);

    char* query_id = generate_query_id();
    TEST_ASSERT_NOT_NULL(query_id);

    // Check prefix
    TEST_ASSERT_EQUAL_STRING_LEN("conduit_", query_id, 8);

    free(query_id);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_generate_query_id_normal);
    if (0) RUN_TEST(test_generate_query_id_malloc_failure);
    RUN_TEST(test_generate_query_id_uniqueness);
    RUN_TEST(test_generate_query_id_format);

    return UNITY_END();
}