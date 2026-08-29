/*
 * Unity Test File: database_queue_set_tags
 * This file contains unit tests for database_queue_set_tags functionality
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/dbqueue/dbqueue.h>

// Forward declarations for functions being tested
bool database_queue_set_tags(DatabaseQueue* db_queue, const char* tags);

// Test function prototypes
void test_database_queue_set_tags_valid_replacement(void);
void test_database_queue_set_tags_null_queue(void);
void test_database_queue_set_tags_null_tags(void);
void test_database_queue_set_tags_empty_string(void);
void test_database_queue_set_tags_replaces_existing(void);
void test_database_queue_set_tags_multiple_times(void);
void test_database_queue_set_tags_long_string(void);

void setUp(void) {
    if (!queue_system_initialized) {
        queue_system_init();
    }
}

void tearDown(void) {
    // Clean up test fixtures
}

// Test: Set tags on a queue with existing tags (replaces "LSMFC")
void test_database_queue_set_tags_valid_replacement(void) {
    DatabaseQueue* queue = database_queue_create_lead("testdb", "sqlite:///tmp/test.db", NULL);
    TEST_ASSERT_NOT_NULL(queue);

    bool result = database_queue_set_tags(queue, "ABC");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("ABC", queue->tags);

    database_queue_destroy(queue);
}

// Test: NULL queue should return false
void test_database_queue_set_tags_null_queue(void) {
    bool result = database_queue_set_tags(NULL, "ABC");
    TEST_ASSERT_FALSE(result);
}

// Test: NULL tags string should return false
void test_database_queue_set_tags_null_tags(void) {
    DatabaseQueue* queue = database_queue_create_lead("testdb", "sqlite:///tmp/test.db", NULL);
    TEST_ASSERT_NOT_NULL(queue);

    bool result = database_queue_set_tags(queue, NULL);
    TEST_ASSERT_FALSE(result);

    database_queue_destroy(queue);
}

// Test: Empty string tags
void test_database_queue_set_tags_empty_string(void) {
    DatabaseQueue* queue = database_queue_create_lead("testdb", "sqlite:///tmp/test.db", NULL);
    TEST_ASSERT_NOT_NULL(queue);

    bool result = database_queue_set_tags(queue, "");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("", queue->tags);

    database_queue_destroy(queue);
}

// Test: Verify old tags are freed when setting new tags
void test_database_queue_set_tags_replaces_existing(void) {
    DatabaseQueue* queue = database_queue_create_lead("testdb", "sqlite:///tmp/test.db", NULL);
    TEST_ASSERT_NOT_NULL(queue);
    TEST_ASSERT_EQUAL_STRING("LSMFC", queue->tags);

    bool result = database_queue_set_tags(queue, "XYZ");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("XYZ", queue->tags);

    database_queue_destroy(queue);
}

// Test: Set tags multiple times, each should replace the previous
void test_database_queue_set_tags_multiple_times(void) {
    DatabaseQueue* queue = database_queue_create_lead("testdb", "sqlite:///tmp/test.db", NULL);
    TEST_ASSERT_NOT_NULL(queue);

    database_queue_set_tags(queue, "A");
    TEST_ASSERT_EQUAL_STRING("A", queue->tags);

    database_queue_set_tags(queue, "AB");
    TEST_ASSERT_EQUAL_STRING("AB", queue->tags);

    database_queue_set_tags(queue, "ABC");
    TEST_ASSERT_EQUAL_STRING("ABC", queue->tags);

    database_queue_destroy(queue);
}

// Test: Long tags string
void test_database_queue_set_tags_long_string(void) {
    DatabaseQueue* queue = database_queue_create_lead("testdb", "sqlite:///tmp/test.db", NULL);
    TEST_ASSERT_NOT_NULL(queue);

    const char* long_tags = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    bool result = database_queue_set_tags(queue, long_tags);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING(long_tags, queue->tags);

    database_queue_destroy(queue);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_set_tags_valid_replacement);
    RUN_TEST(test_database_queue_set_tags_null_queue);
    RUN_TEST(test_database_queue_set_tags_null_tags);
    RUN_TEST(test_database_queue_set_tags_empty_string);
    RUN_TEST(test_database_queue_set_tags_replaces_existing);
    RUN_TEST(test_database_queue_set_tags_multiple_times);
    RUN_TEST(test_database_queue_set_tags_long_string);

    return UNITY_END();
}
