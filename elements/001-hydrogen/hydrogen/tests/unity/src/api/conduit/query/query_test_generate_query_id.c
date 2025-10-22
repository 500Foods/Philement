/*
 * Unity Test File: Test generate_query_id function
 * This file contains unit tests for src/api/conduit/query/query.c generate_query_id function
 */

// Standard includes
#include <string.h>

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Forward declaration for the function being tested
char* generate_query_id(void);

// Function prototypes
void setUp(void);
void tearDown(void);
void test_generate_query_id_basic(void);
void test_generate_query_id_uniqueness(void);

void setUp(void) {
    // No specific setup needed
}

void tearDown(void) {
    // No specific teardown needed
}

// Test basic generation - should return non-NULL string
void test_generate_query_id_basic(void) {
    char* query_id = generate_query_id();

    TEST_ASSERT_NOT_NULL(query_id);
    TEST_ASSERT_GREATER_THAN(0, strlen(query_id));
    TEST_ASSERT(strncmp(query_id, "conduit_", 8) == 0);

    free(query_id);
}

// Test multiple generations produce different IDs
void test_generate_query_id_uniqueness(void) {
    char* id1 = generate_query_id();
    char* id2 = generate_query_id();
    char* id3 = generate_query_id();

    TEST_ASSERT_NOT_NULL(id1);
    TEST_ASSERT_NOT_NULL(id2);
    TEST_ASSERT_NOT_NULL(id3);

    TEST_ASSERT(strcmp(id1, id2) != 0);
    TEST_ASSERT(strcmp(id2, id3) != 0);
    TEST_ASSERT(strcmp(id1, id3) != 0);

    free(id1);
    free(id2);
    free(id3);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_generate_query_id_basic);
    RUN_TEST(test_generate_query_id_uniqueness);

    return UNITY_END();
}