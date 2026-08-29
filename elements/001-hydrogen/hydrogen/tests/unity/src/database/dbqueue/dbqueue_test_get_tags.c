/*
 * Unity Test File: database_queue_get_tags
 * This file contains unit tests for database_queue_get_tags functionality
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/dbqueue/dbqueue.h>

// Forward declarations for functions being tested
char* database_queue_get_tags(const DatabaseQueue* db_queue);

// Test function prototypes
void test_database_queue_get_tags_valid(void);
void test_database_queue_get_tags_null_queue(void);
void test_database_queue_get_tags_null_tags(void);
void test_database_queue_get_tags_returns_copy(void);
void test_database_queue_get_tags_after_set_tags(void);
void test_database_queue_get_tags_empty_string(void);

void setUp(void) {
    if (!queue_system_initialized) {
        queue_system_init();
    }
}

void tearDown(void) {
    // Clean up test fixtures
}

// Test: Get tags from a queue with existing tags
void test_database_queue_get_tags_valid(void) {
    DatabaseQueue* queue = database_queue_create_lead("testdb", "sqlite:///tmp/test.db", NULL);
    TEST_ASSERT_NOT_NULL(queue);
    TEST_ASSERT_EQUAL_STRING("LSMFC", queue->tags);

    char* tags = database_queue_get_tags(queue);
    TEST_ASSERT_NOT_NULL(tags);
    TEST_ASSERT_EQUAL_STRING("LSMFC", tags);

    free(tags);
    database_queue_destroy(queue);
}

// Test: NULL queue should return NULL
void test_database_queue_get_tags_null_queue(void) {
    char* tags = database_queue_get_tags(NULL);
    TEST_ASSERT_NULL(tags);
}

// Test: Queue with NULL tags should return NULL
void test_database_queue_get_tags_null_tags(void) {
    DatabaseQueue* queue = database_queue_create_lead("testdb", "sqlite:///tmp/test.db", NULL);
    TEST_ASSERT_NOT_NULL(queue);

    free(queue->tags);
    queue->tags = NULL;

    char* tags = database_queue_get_tags(queue);
    TEST_ASSERT_NULL(tags);

    database_queue_destroy(queue);
}

// Test: Returned string is a copy, not a pointer to internal state
void test_database_queue_get_tags_returns_copy(void) {
    DatabaseQueue* queue = database_queue_create_lead("testdb", "sqlite:///tmp/test.db", NULL);
    TEST_ASSERT_NOT_NULL(queue);
    TEST_ASSERT_EQUAL_STRING("LSMFC", queue->tags);

    char* tags = database_queue_get_tags(queue);
    TEST_ASSERT_NOT_NULL(tags);
    TEST_ASSERT_NOT_EQUAL(queue->tags, tags);

    tags[0] = 'X';
    TEST_ASSERT_EQUAL_STRING("LSMFC", queue->tags);

    free(tags);
    database_queue_destroy(queue);
}

// Test: Get tags after using set_tags
void test_database_queue_get_tags_after_set_tags(void) {
    DatabaseQueue* queue = database_queue_create_lead("testdb", "sqlite:///tmp/test.db", NULL);
    TEST_ASSERT_NOT_NULL(queue);

    database_queue_set_tags(queue, "ABCD");
    TEST_ASSERT_EQUAL_STRING("ABCD", queue->tags);

    char* tags = database_queue_get_tags(queue);
    TEST_ASSERT_NOT_NULL(tags);
    TEST_ASSERT_EQUAL_STRING("ABCD", tags);

    free(tags);
    database_queue_destroy(queue);
}

// Test: Get tags when tags is an empty string
void test_database_queue_get_tags_empty_string(void) {
    DatabaseQueue* queue = database_queue_create_lead("testdb", "sqlite:///tmp/test.db", NULL);
    TEST_ASSERT_NOT_NULL(queue);

    database_queue_set_tags(queue, "");

    char* tags = database_queue_get_tags(queue);
    TEST_ASSERT_NOT_NULL(tags);
    TEST_ASSERT_EQUAL_STRING("", tags);

    free(tags);
    database_queue_destroy(queue);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_get_tags_valid);
    RUN_TEST(test_database_queue_get_tags_null_queue);
    RUN_TEST(test_database_queue_get_tags_null_tags);
    RUN_TEST(test_database_queue_get_tags_returns_copy);
    RUN_TEST(test_database_queue_get_tags_after_set_tags);
    RUN_TEST(test_database_queue_get_tags_empty_string);

    return UNITY_END();
}
