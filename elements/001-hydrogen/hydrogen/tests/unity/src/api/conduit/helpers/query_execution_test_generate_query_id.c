/*
 * Unity Test File: generate_query_id()
 * This file contains unit tests for the query ID generation function
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/api/conduit/conduit_helpers.h>
#include <unity/mocks/mock_generate_query_id.h>
#include <string.h>
#include <time.h>

// Forward declaration for function being tested
char* generate_query_id(void);

// Function prototypes for test functions
void test_generate_query_id_basic(void);
void test_generate_query_id_unique(void);
void test_generate_query_id_format(void);
void test_generate_query_id_timestamp(void);
void test_generate_query_id_length(void);

void setUp(void) {
    // Set mock to return a valid query ID
    mock_generate_query_id_set_result("conduit_123_1640995200");
}

void tearDown(void) {
    // No teardown needed
}

// Test basic functionality
void test_generate_query_id_basic(void) {
    char* query_id = generate_query_id();
    
    TEST_ASSERT_NOT_NULL(query_id);
    TEST_ASSERT_GREATER_THAN(0, strlen(query_id));
    
    free(query_id);
}

// Test that each call returns a unique ID
void test_generate_query_id_unique(void) {
    char* query_id1 = generate_query_id();
    char* query_id2 = generate_query_id();
    
    TEST_ASSERT_NOT_NULL(query_id1);
    TEST_ASSERT_NOT_NULL(query_id2);
    TEST_ASSERT_TRUE(strcmp(query_id1, query_id2) != 0);
    
    free(query_id1);
    free(query_id2);
}

// Test that the format matches "conduit_{counter}_{timestamp}"
void test_generate_query_id_format(void) {
    char* query_id = generate_query_id();
    
    TEST_ASSERT_NOT_NULL(query_id);
    
    // Check prefix
    TEST_ASSERT_EQUAL_STRING_LEN("conduit_", query_id, (int)strlen("conduit_"));
    
    // Find the positions of the underscores
    char* first_underscore = strchr(query_id, '_');
    TEST_ASSERT_NOT_NULL(first_underscore);
    
    char* second_underscore = strchr(first_underscore + 1, '_');
    TEST_ASSERT_NOT_NULL(second_underscore);
    
    // Extract counter (should be numeric)
    const char* counter_str = first_underscore + 1;
    int counter_len = (int)(second_underscore - counter_str);
    TEST_ASSERT_GREATER_THAN(0, counter_len);
    
    for (int i = 0; i < counter_len; i++) {
        TEST_ASSERT_TRUE(isdigit((unsigned char)counter_str[i]));
    }
    
    // Extract timestamp (should be numeric)
    const char* timestamp_str = second_underscore + 1;
    TEST_ASSERT_GREATER_THAN(0, (int)strlen(timestamp_str));
    
    for (int i = 0; i < (int)strlen(timestamp_str); i++) {
        TEST_ASSERT_TRUE(isdigit((unsigned char)timestamp_str[i]));
    }
    
    free(query_id);
}

// Test that the timestamp is valid (close to current time)
void test_generate_query_id_timestamp(void) {
    time_t current_time = time(NULL);
    char* query_id = generate_query_id();
    
    TEST_ASSERT_NOT_NULL(query_id);
    
    char* second_underscore = strchr(strchr(query_id, '_') + 1, '_');
    TEST_ASSERT_NOT_NULL(second_underscore);
    
    time_t query_timestamp = atol(second_underscore + 1);
    
    // Check that timestamp is within a reasonable range (±10 seconds)
    TEST_ASSERT_INT_WITHIN(10, current_time, query_timestamp);
    
    free(query_id);
}

// Test that the generated ID fits within the allocated buffer (32 bytes)
void test_generate_query_id_length(void) {
    char* query_id = generate_query_id();
    
    TEST_ASSERT_NOT_NULL(query_id);
    TEST_ASSERT_LESS_THAN(32, strlen(query_id));
    
    free(query_id);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_generate_query_id_basic);
    RUN_TEST(test_generate_query_id_unique);
    RUN_TEST(test_generate_query_id_format);
    RUN_TEST(test_generate_query_id_timestamp);
    RUN_TEST(test_generate_query_id_length);
    
    return UNITY_END();
}
