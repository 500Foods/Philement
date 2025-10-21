/*
 * Unity Test: generate_query_id function
 * Tests unique query ID generation
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include the source header to access the function
#include <src/api/conduit/query/query.h>

// Forward declaration for the static function we want to test
char* generate_query_id(void);

// Test function prototypes
void test_generate_query_id_not_null(void);
void test_generate_query_id_unique(void);
void test_generate_query_id_format(void);
void test_generate_query_id_memory_allocation(void);
void test_generate_query_id_rapid_calls(void);

void setUp(void) {
    // No setup needed
}

void tearDown(void) {
    // No cleanup needed
}

// Test that generate_query_id returns a non-NULL string
void test_generate_query_id_not_null(void) {
    char* query_id = generate_query_id();

    TEST_ASSERT_NOT_NULL(query_id);
    TEST_ASSERT_TRUE(strlen(query_id) > 0);

    free(query_id);
}

// Test that generate_query_id returns unique IDs on consecutive calls
void test_generate_query_id_unique(void) {
    char* id1 = generate_query_id();
    char* id2 = generate_query_id();

    TEST_ASSERT_NOT_NULL(id1);
    TEST_ASSERT_NOT_NULL(id2);
    TEST_ASSERT_TRUE(strcmp(id1, id2) != 0); // Should be different

    free(id1);
    free(id2);
}

// Test that generate_query_id follows expected format
void test_generate_query_id_format(void) {
    char* query_id = generate_query_id();

    TEST_ASSERT_NOT_NULL(query_id);

    // Should start with "conduit_"
    TEST_ASSERT_TRUE(strncmp(query_id, "conduit_", 8) == 0);

    // Should contain an underscore after the prefix
    char* first_underscore = strchr(query_id, '_');
    char* second_underscore = strchr(first_underscore + 1, '_');

    TEST_ASSERT_NOT_NULL(first_underscore);
    TEST_ASSERT_NOT_NULL(second_underscore);

    // Should be able to parse the timestamp part
    char* timestamp_str = second_underscore + 1;
    TEST_ASSERT_NOT_NULL(timestamp_str);

    // Timestamp should be numeric
    char* endptr;
    long timestamp = strtol(timestamp_str, &endptr, 10);
    TEST_ASSERT_TRUE(timestamp > 0);
    TEST_ASSERT_TRUE(*endptr == '\0' || *endptr == '\n'); // Should end with null or newline

    free(query_id);
}

// Test that generate_query_id handles memory allocation properly
void test_generate_query_id_memory_allocation(void) {
    // This test verifies that the function doesn't crash and handles memory properly
    // In a real scenario with mocked malloc failure, we would test failure cases
    // For now, we just ensure it works in normal conditions

    char* query_id = generate_query_id();
    TEST_ASSERT_NOT_NULL(query_id);

    // Verify the string is null-terminated
    TEST_ASSERT_EQUAL('\0', query_id[31]); // Should be null-terminated at position 31

    free(query_id);
}

// Test multiple rapid calls to ensure thread safety and uniqueness
void test_generate_query_id_rapid_calls(void) {
    const int NUM_IDS = 10;
    char* ids[NUM_IDS];

    // Generate multiple IDs rapidly
    for (int i = 0; i < NUM_IDS; i++) {
        ids[i] = generate_query_id();
        TEST_ASSERT_NOT_NULL(ids[i]);
    }

    // Verify all are unique
    for (int i = 0; i < NUM_IDS; i++) {
        for (int j = i + 1; j < NUM_IDS; j++) {
            TEST_ASSERT_TRUE(strcmp(ids[i], ids[j]) != 0);
        }
    }

    // Clean up
    for (int i = 0; i < NUM_IDS; i++) {
        free(ids[i]);
    }
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_generate_query_id_not_null);
    RUN_TEST(test_generate_query_id_unique);
    RUN_TEST(test_generate_query_id_format);
    RUN_TEST(test_generate_query_id_memory_allocation);
    RUN_TEST(test_generate_query_id_rapid_calls);

    return UNITY_END();
}