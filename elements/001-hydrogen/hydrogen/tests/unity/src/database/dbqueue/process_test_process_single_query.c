/*
 * Unity Test File: process_test_process_single_query
 * This file contains comprehensive unit tests for database_queue_process_single_query
 */

#include <src/hydrogen.h>
#include <unity.h>

// Local includes
#include <src/database/dbqueue/dbqueue.h>
#include <src/database/database.h>

// Forward declaration of the function being tested
void database_queue_process_single_query(DatabaseQueue* db_queue);

// Test function prototypes
void test_process_single_query_null_queue(void);
void test_process_single_query_empty_queue(void);
void test_process_single_query_with_all_fields(void);
void test_process_single_query_with_null_fields(void);
void test_process_single_query_slow_queue_type(void);
void test_process_single_query_medium_queue_type(void);
void test_process_single_query_fast_queue_type(void);
void test_process_single_query_cache_queue_type(void);
void test_process_single_query_lead_queue_type(void);

void setUp(void) {
    // Initialize subsystems for testing
    if (!queue_system_initialized) {
        queue_system_init();
    }
    database_subsystem_init();
}

void tearDown(void) {
    // Clean up test fixtures
    database_subsystem_shutdown();
}

// Test NULL queue parameter
void test_process_single_query_null_queue(void) {
    // Should handle NULL gracefully
    database_queue_process_single_query(NULL);
    // No crash = success
}

// Test with empty queue (no queries to process)
void test_process_single_query_empty_queue(void) {
    DatabaseQueue* queue = database_queue_create_worker("testdb_empty", "sqlite:///tmp/empty.db", QUEUE_TYPE_FAST, NULL);
    TEST_ASSERT_NOT_NULL(queue);
    
    // Call with empty queue
    database_queue_process_single_query(queue);
    
    // Clean up
    database_queue_destroy(queue);
}

// Test query processing with all fields populated
void test_process_single_query_with_all_fields(void) {
    DatabaseQueue* queue = database_queue_create_worker("testdb_full", "sqlite:///tmp/full.db", QUEUE_TYPE_FAST, NULL);
    TEST_ASSERT_NOT_NULL(queue);
    
    // Create and submit a query with all fields
    DatabaseQuery* query = calloc(1, sizeof(DatabaseQuery));
    TEST_ASSERT_NOT_NULL(query);
    query->query_id = strdup("full_query");
    query->query_template = strdup("SELECT 1");
    query->parameter_json = strdup("{\"param\": \"value\"}");
    query->error_message = strdup("no error");
    query->queue_type_hint = DB_QUEUE_FAST;
    
    bool submit = database_queue_submit_query(queue, query);
    TEST_ASSERT_TRUE(submit);
    
    // Process the query
    database_queue_process_single_query(queue);
    
    // Verify queue is empty after processing
    size_t depth = database_queue_get_depth(queue);
    TEST_ASSERT_EQUAL(0, depth);
    
    // Clean up
    database_queue_destroy(queue);
}

// Test query processing with NULL fields in cleanup
void test_process_single_query_with_null_fields(void) {
    DatabaseQueue* queue = database_queue_create_worker("testdb_null", "sqlite:///tmp/null.db", QUEUE_TYPE_MEDIUM, NULL);
    TEST_ASSERT_NOT_NULL(queue);
    
    // Submit a valid query first
    DatabaseQuery* query = calloc(1, sizeof(DatabaseQuery));
    TEST_ASSERT_NOT_NULL(query);
    query->query_id = NULL;  // NULL ID to test cleanup path
    query->query_template = strdup("SELECT 1");
    query->parameter_json = NULL;  // NULL params to test cleanup path
    query->error_message = NULL;  // NULL error to test cleanup path
    query->queue_type_hint = DB_QUEUE_MEDIUM;
    
    bool submit = database_queue_submit_query(queue, query);
    TEST_ASSERT_TRUE(submit);
    
    // Process the query - should handle NULL fields in cleanup (lines 181-184)
    database_queue_process_single_query(queue);
    
    // Clean up
    database_queue_destroy(queue);
}

// Test slow queue type simulation path
void test_process_single_query_slow_queue_type(void) {
    DatabaseQueue* queue = database_queue_create_worker("testdb_slow", "sqlite:///tmp/slow.db", QUEUE_TYPE_SLOW, NULL);
    TEST_ASSERT_NOT_NULL(queue);
    
    // Create query without persistent connection to trigger simulation
    DatabaseQuery* query = calloc(1, sizeof(DatabaseQuery));
    TEST_ASSERT_NOT_NULL(query);
    query->query_id = strdup("slow_query");
    query->query_template = strdup("SELECT 1");
    query->parameter_json = strdup("{}");
    
    bool submit = database_queue_submit_query(queue, query);
    TEST_ASSERT_TRUE(submit);
    
    // Process - should hit slow queue simulation (line 160-161)
    database_queue_process_single_query(queue);
    
    // Clean up
    database_queue_destroy(queue);
}

// Test medium queue type simulation path
void test_process_single_query_medium_queue_type(void) {
    DatabaseQueue* queue = database_queue_create_worker("testdb_med", "sqlite:///tmp/med.db", QUEUE_TYPE_MEDIUM, NULL);
    TEST_ASSERT_NOT_NULL(queue);
    
    DatabaseQuery* query = calloc(1, sizeof(DatabaseQuery));
    TEST_ASSERT_NOT_NULL(query);
    query->query_id = strdup("med_query");
    query->query_template = strdup("SELECT 1");
    query->parameter_json = strdup("{}");
    
    bool submit = database_queue_submit_query(queue, query);
    TEST_ASSERT_TRUE(submit);
    
    // Process - should hit medium queue simulation (line 162-163)
    database_queue_process_single_query(queue);
    
    database_queue_destroy(queue);
}

// Test fast queue type simulation path
void test_process_single_query_fast_queue_type(void) {
    DatabaseQueue* queue = database_queue_create_worker("testdb_fast", "sqlite:///tmp/fast.db", QUEUE_TYPE_FAST, NULL);
    TEST_ASSERT_NOT_NULL(queue);
    
    DatabaseQuery* query = calloc(1, sizeof(DatabaseQuery));
    TEST_ASSERT_NOT_NULL(query);
    query->query_id = strdup("fast_query");
    query->query_template = strdup("SELECT 1");
    query->parameter_json = strdup("{}");
    
    bool submit = database_queue_submit_query(queue, query);
    TEST_ASSERT_TRUE(submit);
    
    // Process - should hit fast queue simulation (line 164-165)
    database_queue_process_single_query(queue);
    
    database_queue_destroy(queue);
}

// Test cache queue type simulation path
void test_process_single_query_cache_queue_type(void) {
    DatabaseQueue* queue = database_queue_create_worker("testdb_cache", "sqlite:///tmp/cache.db", QUEUE_TYPE_CACHE, NULL);
    TEST_ASSERT_NOT_NULL(queue);
    
    DatabaseQuery* query = calloc(1, sizeof(DatabaseQuery));
    TEST_ASSERT_NOT_NULL(query);
    query->query_id = strdup("cache_query");
    query->query_template = strdup("SELECT 1");
    query->parameter_json = strdup("{}");
    
    bool submit = database_queue_submit_query(queue, query);
    TEST_ASSERT_TRUE(submit);
    
    // Process - should hit cache queue simulation (line 166-167)
    database_queue_process_single_query(queue);
    
    database_queue_destroy(queue);
}

// Test Lead queue type simulation path
void test_process_single_query_lead_queue_type(void) {
    DatabaseQueue* queue = database_queue_create_lead("testdb_lead", "sqlite:///tmp/lead.db", NULL);
    TEST_ASSERT_NOT_NULL(queue);
    
    DatabaseQuery* query = calloc(1, sizeof(DatabaseQuery));
    TEST_ASSERT_NOT_NULL(query);
    query->query_id = strdup("lead_query");
    query->query_template = strdup("SELECT 1");
    query->parameter_json = strdup("{}");
    
    bool submit = database_queue_submit_query(queue, query);
    TEST_ASSERT_TRUE(submit);
    
    // Process - should hit Lead queue simulation (line 168-169)
    // and also manage_child_queues (line 174-175)
    database_queue_process_single_query(queue);
    
    database_queue_destroy(queue);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_process_single_query_null_queue);
    RUN_TEST(test_process_single_query_empty_queue);
    RUN_TEST(test_process_single_query_with_all_fields);
    RUN_TEST(test_process_single_query_with_null_fields);
    RUN_TEST(test_process_single_query_slow_queue_type);
    RUN_TEST(test_process_single_query_medium_queue_type);
    RUN_TEST(test_process_single_query_fast_queue_type);
    RUN_TEST(test_process_single_query_cache_queue_type);
    RUN_TEST(test_process_single_query_lead_queue_type);

    return UNITY_END();
}