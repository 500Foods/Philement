/*
 * Unity Test File: database_queue_test_get_stats_json
 * This file contains unit tests for database_queue_get_stats_json function
 */

#include <src/hydrogen.h>
#include <unity.h>

// Local includes
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/database.h>

// Test function prototypes
void test_database_queue_get_stats_json_null_queue(void);
void test_database_queue_get_stats_json_empty_stats(void);
void test_database_queue_get_stats_json_with_stats(void);

void setUp(void) {
    // Initialize queue system for testing
    if (!queue_system_initialized) {
        queue_system_init();
    }
}

void tearDown(void) {
    // Clean up test fixtures
}

void test_database_queue_get_stats_json_null_queue(void) {
    json_t* result = database_queue_get_stats_json(NULL);
    TEST_ASSERT_NULL(result);
}

void test_database_queue_get_stats_json_empty_stats(void) {
    DatabaseQueue* queue = database_queue_create_lead("testdb", "sqlite:///tmp/test.db", NULL);
    TEST_ASSERT_NOT_NULL(queue);

    json_t* stats = database_queue_get_stats_json(queue);
    TEST_ASSERT_NOT_NULL(stats);
    TEST_ASSERT_TRUE(json_is_object(stats));

    // Check overall statistics
    json_t* total_submitted = json_object_get(stats, "total_queries_submitted");
    TEST_ASSERT_NOT_NULL(total_submitted);
    TEST_ASSERT_TRUE(json_is_integer(total_submitted));
    TEST_ASSERT_EQUAL_UINT64(0, json_integer_value(total_submitted));

    json_t* total_completed = json_object_get(stats, "total_queries_completed");
    TEST_ASSERT_NOT_NULL(total_completed);
    TEST_ASSERT_TRUE(json_is_integer(total_completed));
    TEST_ASSERT_EQUAL_UINT64(0, json_integer_value(total_completed));

    json_t* total_failed = json_object_get(stats, "total_queries_failed");
    TEST_ASSERT_NOT_NULL(total_failed);
    TEST_ASSERT_TRUE(json_is_integer(total_failed));
    TEST_ASSERT_EQUAL_UINT64(0, json_integer_value(total_failed));

    json_t* total_timeouts = json_object_get(stats, "total_timeouts");
    TEST_ASSERT_NOT_NULL(total_timeouts);
    TEST_ASSERT_TRUE(json_is_integer(total_timeouts));
    TEST_ASSERT_EQUAL_UINT64(0, json_integer_value(total_timeouts));

    // Check queue selection counters
    json_t* selection_counters = json_object_get(stats, "queue_selection_counters");
    TEST_ASSERT_NOT_NULL(selection_counters);
    TEST_ASSERT_TRUE(json_is_array(selection_counters));
    TEST_ASSERT_EQUAL(5, json_array_size(selection_counters));

    // Check per-queue statistics
    json_t* per_queue = json_object_get(stats, "per_queue_stats");
    TEST_ASSERT_NOT_NULL(per_queue);
    TEST_ASSERT_TRUE(json_is_array(per_queue));
    TEST_ASSERT_EQUAL(5, json_array_size(per_queue));

    // Check queue names
    const char* expected_names[] = {"slow", "medium", "fast", "cache", "lead"};
    for (size_t i = 0; i < 5; i++) {
        json_t* queue_stat = json_array_get(per_queue, i);
        TEST_ASSERT_NOT_NULL(queue_stat);
        TEST_ASSERT_TRUE(json_is_object(queue_stat));

        json_t* queue_type = json_object_get(queue_stat, "queue_type");
        TEST_ASSERT_NOT_NULL(queue_type);
        TEST_ASSERT_TRUE(json_is_string(queue_type));
        TEST_ASSERT_EQUAL_STRING(expected_names[i], json_string_value(queue_type));
    }

    json_decref(stats);
    database_queue_destroy(queue);
}

void test_database_queue_get_stats_json_with_stats(void) {
    DatabaseQueue* queue = database_queue_create_lead("testdb", "sqlite:///tmp/test.db", NULL);
    TEST_ASSERT_NOT_NULL(queue);

    // Add some statistics
    database_queue_record_query_submission(queue, 0);
    database_queue_record_query_completion(queue, 0, 1000);
    database_queue_record_query_failure(queue, 1);
    database_queue_record_timeout(queue);

    json_t* stats = database_queue_get_stats_json(queue);
    TEST_ASSERT_NOT_NULL(stats);

    // Check updated statistics
    json_t* total_submitted = json_object_get(stats, "total_queries_submitted");
    TEST_ASSERT_EQUAL_UINT64(1, json_integer_value(total_submitted));

    json_t* total_completed = json_object_get(stats, "total_queries_completed");
    TEST_ASSERT_EQUAL_UINT64(1, json_integer_value(total_completed));

    json_t* total_failed = json_object_get(stats, "total_queries_failed");
    TEST_ASSERT_EQUAL_UINT64(1, json_integer_value(total_failed));

    json_t* total_timeouts = json_object_get(stats, "total_timeouts");
    TEST_ASSERT_EQUAL_UINT64(1, json_integer_value(total_timeouts));

    // Check per-queue statistics
    json_t* per_queue = json_object_get(stats, "per_queue_stats");
    json_t* queue_0_stat = json_array_get(per_queue, 0);
    json_t* submitted_0 = json_object_get(queue_0_stat, "submitted");
    TEST_ASSERT_EQUAL_UINT64(1, json_integer_value(submitted_0));

    json_t* completed_0 = json_object_get(queue_0_stat, "completed");
    TEST_ASSERT_EQUAL_UINT64(1, json_integer_value(completed_0));

    json_t* avg_time_0 = json_object_get(queue_0_stat, "avg_execution_time_us");
    TEST_ASSERT_EQUAL_UINT64(1000, json_integer_value(avg_time_0));

    json_t* queue_1_stat = json_array_get(per_queue, 1);
    json_t* failed_1 = json_object_get(queue_1_stat, "failed");
    TEST_ASSERT_EQUAL_UINT64(1, json_integer_value(failed_1));

    json_decref(stats);
    database_queue_destroy(queue);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_database_queue_get_stats_json_null_queue);
    RUN_TEST(test_database_queue_get_stats_json_empty_stats);
    RUN_TEST(test_database_queue_get_stats_json_with_stats);

    return UNITY_END();
}