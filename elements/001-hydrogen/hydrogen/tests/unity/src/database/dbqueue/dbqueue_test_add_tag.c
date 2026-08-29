/*
 * Unity Test File: database_queue_add_tag
 * This file contains unit tests for database_queue_add_tag functionality
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/dbqueue/dbqueue.h>

// Forward declarations for functions being tested
bool database_queue_add_tag(DatabaseQueue* db_queue, char tag);

// Test function prototypes
void test_database_queue_add_tag_new_tag(void);
void test_database_queue_add_tag_duplicate(void);
void test_database_queue_add_tag_null_queue(void);
void test_database_queue_add_tag_null_tags(void);
void test_database_queue_add_tag_to_empty_string(void);
void test_database_queue_add_tag_special_character(void);
void test_database_queue_add_tag_multiple(void);

void setUp(void) {
    if (!queue_system_initialized) {
        queue_system_init();
    }
}

void tearDown(void) {
    // Clean up test fixtures
}

// Test: Add a new tag to existing tags
void test_database_queue_add_tag_new_tag(void) {
    DatabaseQueue* queue = database_queue_create_lead("testdb", "sqlite:///tmp/test.db", NULL);
    TEST_ASSERT_NOT_NULL(queue);
    TEST_ASSERT_EQUAL_STRING("LSMFC", queue->tags);

    bool result = database_queue_add_tag(queue, 'X');
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("LSMFCX", queue->tags);

    database_queue_destroy(queue);
}

// Test: Add a duplicate tag (already exists) should return true, no change
void test_database_queue_add_tag_duplicate(void) {
    DatabaseQueue* queue = database_queue_create_lead("testdb", "sqlite:///tmp/test.db", NULL);
    TEST_ASSERT_NOT_NULL(queue);
    TEST_ASSERT_EQUAL_STRING("LSMFC", queue->tags);

    bool result = database_queue_add_tag(queue, 'L');
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("LSMFC", queue->tags);

    result = database_queue_add_tag(queue, 'M');
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("LSMFC", queue->tags);

    database_queue_destroy(queue);
}

// Test: NULL queue should return false
void test_database_queue_add_tag_null_queue(void) {
    bool result = database_queue_add_tag(NULL, 'A');
    TEST_ASSERT_FALSE(result);
}

// Test: Queue with NULL tags should return false
void test_database_queue_add_tag_null_tags(void) {
    DatabaseQueue* queue = database_queue_create_lead("testdb", "sqlite:///tmp/test.db", NULL);
    TEST_ASSERT_NOT_NULL(queue);

    free(queue->tags);
    queue->tags = NULL;

    bool result = database_queue_add_tag(queue, 'A');
    TEST_ASSERT_FALSE(result);

    database_queue_destroy(queue);
}

// Test: Add tag to empty string tags
void test_database_queue_add_tag_to_empty_string(void) {
    DatabaseQueue* queue = database_queue_create_lead("testdb", "sqlite:///tmp/test.db", NULL);
    TEST_ASSERT_NOT_NULL(queue);

    database_queue_set_tags(queue, "");

    bool result = database_queue_add_tag(queue, 'A');
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("A", queue->tags);

    database_queue_destroy(queue);
}

// Test: Add a special character tag
void test_database_queue_add_tag_special_character(void) {
    DatabaseQueue* queue = database_queue_create_lead("testdb", "sqlite:///tmp/test.db", NULL);
    TEST_ASSERT_NOT_NULL(queue);
    TEST_ASSERT_EQUAL_STRING("LSMFC", queue->tags);

    bool result = database_queue_add_tag(queue, '!');
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("LSMFC!", queue->tags);

    database_queue_destroy(queue);
}

// Test: Add multiple tags sequentially
void test_database_queue_add_tag_multiple(void) {
    DatabaseQueue* queue = database_queue_create_lead("testdb", "sqlite:///tmp/test.db", NULL);
    TEST_ASSERT_NOT_NULL(queue);
    TEST_ASSERT_EQUAL_STRING("LSMFC", queue->tags);

    bool result = database_queue_add_tag(queue, '1');
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("LSMFC1", queue->tags);

    result = database_queue_add_tag(queue, '2');
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("LSMFC12", queue->tags);

    result = database_queue_add_tag(queue, '3');
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("LSMFC123", queue->tags);

    database_queue_destroy(queue);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_add_tag_new_tag);
    RUN_TEST(test_database_queue_add_tag_duplicate);
    RUN_TEST(test_database_queue_add_tag_null_queue);
    RUN_TEST(test_database_queue_add_tag_null_tags);
    RUN_TEST(test_database_queue_add_tag_to_empty_string);
    RUN_TEST(test_database_queue_add_tag_special_character);
    RUN_TEST(test_database_queue_add_tag_multiple);

    return UNITY_END();
}
