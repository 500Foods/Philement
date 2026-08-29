/*
 * Unity Test File: database_queue_remove_tag
 * This file contains unit tests for database_queue_remove_tag functionality
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/dbqueue/dbqueue.h>

// Forward declarations for functions being tested
bool database_queue_remove_tag(DatabaseQueue* db_queue, char tag);

// Test function prototypes
void test_database_queue_remove_tag_existing_first(void);
void test_database_queue_remove_tag_existing_middle(void);
void test_database_queue_remove_tag_existing_last(void);
void test_database_queue_remove_tag_not_found(void);
void test_database_queue_remove_tag_null_queue(void);
void test_database_queue_remove_tag_null_tags(void);
void test_database_queue_remove_tag_remove_all(void);
void test_database_queue_remove_tag_empty_string(void);

void setUp(void) {
    if (!queue_system_initialized) {
        queue_system_init();
    }
}

void tearDown(void) {
    // Clean up test fixtures
}

// Test: Remove the first tag from "LSMFC" -> "SMFC"
void test_database_queue_remove_tag_existing_first(void) {
    DatabaseQueue* queue = database_queue_create_lead("testdb", "sqlite:///tmp/test.db", NULL);
    TEST_ASSERT_NOT_NULL(queue);
    TEST_ASSERT_EQUAL_STRING("LSMFC", queue->tags);

    bool result = database_queue_remove_tag(queue, 'L');
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("SMFC", queue->tags);

    database_queue_destroy(queue);
}

// Test: Remove a tag from the middle of "LSMFC" -> "LSFC"
void test_database_queue_remove_tag_existing_middle(void) {
    DatabaseQueue* queue = database_queue_create_lead("testdb", "sqlite:///tmp/test.db", NULL);
    TEST_ASSERT_NOT_NULL(queue);
    TEST_ASSERT_EQUAL_STRING("LSMFC", queue->tags);

    bool result = database_queue_remove_tag(queue, 'M');
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("LSFC", queue->tags);

    database_queue_destroy(queue);
}

// Test: Remove the last tag from "LSMFC" -> "LSMF"
void test_database_queue_remove_tag_existing_last(void) {
    DatabaseQueue* queue = database_queue_create_lead("testdb", "sqlite:///tmp/test.db", NULL);
    TEST_ASSERT_NOT_NULL(queue);
    TEST_ASSERT_EQUAL_STRING("LSMFC", queue->tags);

    bool result = database_queue_remove_tag(queue, 'C');
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("LSMF", queue->tags);

    database_queue_destroy(queue);
}

// Test: Remove a tag that doesn't exist
void test_database_queue_remove_tag_not_found(void) {
    DatabaseQueue* queue = database_queue_create_lead("testdb", "sqlite:///tmp/test.db", NULL);
    TEST_ASSERT_NOT_NULL(queue);
    TEST_ASSERT_EQUAL_STRING("LSMFC", queue->tags);

    bool result = database_queue_remove_tag(queue, 'X');
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_STRING("LSMFC", queue->tags);

    database_queue_destroy(queue);
}

// Test: NULL queue should return false
void test_database_queue_remove_tag_null_queue(void) {
    bool result = database_queue_remove_tag(NULL, 'L');
    TEST_ASSERT_FALSE(result);
}

// Test: Queue with NULL tags should return false
void test_database_queue_remove_tag_null_tags(void) {
    DatabaseQueue* queue = database_queue_create_lead("testdb", "sqlite:///tmp/test.db", NULL);
    TEST_ASSERT_NOT_NULL(queue);

    free(queue->tags);
    queue->tags = NULL;

    bool result = database_queue_remove_tag(queue, 'L');
    TEST_ASSERT_FALSE(result);

    database_queue_destroy(queue);
}

// Test: Remove all tags one by one
void test_database_queue_remove_tag_remove_all(void) {
    DatabaseQueue* queue = database_queue_create_lead("testdb", "sqlite:///tmp/test.db", NULL);
    TEST_ASSERT_NOT_NULL(queue);
    TEST_ASSERT_EQUAL_STRING("LSMFC", queue->tags);

    database_queue_remove_tag(queue, 'L');
    TEST_ASSERT_EQUAL_STRING("SMFC", queue->tags);

    database_queue_remove_tag(queue, 'S');
    TEST_ASSERT_EQUAL_STRING("MFC", queue->tags);

    database_queue_remove_tag(queue, 'M');
    TEST_ASSERT_EQUAL_STRING("FC", queue->tags);

    database_queue_remove_tag(queue, 'F');
    TEST_ASSERT_EQUAL_STRING("C", queue->tags);

    database_queue_remove_tag(queue, 'C');
    TEST_ASSERT_EQUAL_STRING("", queue->tags);

    database_queue_destroy(queue);
}

// Test: Remove tag from an empty string
void test_database_queue_remove_tag_empty_string(void) {
    DatabaseQueue* queue = database_queue_create_lead("testdb", "sqlite:///tmp/test.db", NULL);
    TEST_ASSERT_NOT_NULL(queue);

    database_queue_set_tags(queue, "");

    bool result = database_queue_remove_tag(queue, 'A');
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL_STRING("", queue->tags);

    database_queue_destroy(queue);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_remove_tag_existing_first);
    RUN_TEST(test_database_queue_remove_tag_existing_middle);
    RUN_TEST(test_database_queue_remove_tag_existing_last);
    RUN_TEST(test_database_queue_remove_tag_not_found);
    RUN_TEST(test_database_queue_remove_tag_null_queue);
    RUN_TEST(test_database_queue_remove_tag_null_tags);
    RUN_TEST(test_database_queue_remove_tag_remove_all);
    RUN_TEST(test_database_queue_remove_tag_empty_string);

    return UNITY_END();
}
