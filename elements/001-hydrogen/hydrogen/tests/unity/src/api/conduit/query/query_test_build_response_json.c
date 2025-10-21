/*
 * Unity Test: build_response_json function
 * Tests JSON response building for query results
 */

#include "../../../../../src/hydrogen.h"
#include "unity.h"

// Include the source file to test static functions
#include "../../../../../src/api/conduit/query/query.c"

// Test function prototypes
void test_build_response_json_success(void);
void test_build_response_json_timeout(void);
void test_build_response_json_database_error(void);
void test_build_response_json_generic_failure(void);
void test_build_response_json_null_description(void);

// Use real struct definitions from included headers

// Forward declarations for functions used in build_response_json
// These are declared in the source file we're testing

// Mock implementations for testing - simplified to avoid conflicts
// These functions are declared in the source file, so we don't redefine them

void setUp(void) {
    // No setup needed
}

void tearDown(void) {
    // No cleanup needed
}

// Test successful query response
void test_build_response_json_success(void) {
    int query_ref = 123;
    const char* database = "test_db";
    QueryCacheEntry cache_entry = {
        .query_ref = 123,
        .sql_template = (char*)"SELECT * FROM test",
        .description = (char*)"Test query",
        .queue_type = (char*)"read_queue",
        .timeout_seconds = 30,
        .last_used = 0,
        .usage_count = 0
    };
    DatabaseQueue selected_queue = {
        .database_name = (char*)"test_db",
        .connection_string = (char*)"test_conn",
        .engine_type = DB_ENGINE_SQLITE,
        .queue_type = (char*)"read_queue",
        .bootstrap_query = NULL,
        .queue = NULL,
        .worker_thread = 0,
        .worker_thread_started = false,
        .is_lead_queue = false,
        .can_spawn_queues = false,
        .tags = NULL,
        .queue_number = 0,
        .active_connections = 0,
        .total_queries_processed = 0,
        .current_queue_depth = 0,
        .last_heartbeat = 0,
        .last_connection_attempt = 0,
        .last_request_time = 0,
        .heartbeat_interval_seconds = 30,
        .persistent_connection = NULL,
        .latest_available_migration = 0,
        .latest_installed_migration = 0,
        .empty_database = false,
        .query_cache = NULL,
        .shutdown_requested = false,
        .is_connected = false,
        .bootstrap_completed = false,
        .initial_connection_attempted = false,
        .conductor_sequence_completed = false
    };

    // Create mock pending result with success
    QueryResult result = {
        .success = true,
        .data_json = (char*)"[{\"id\": 1, \"name\": \"test\"}]",
        .row_count = 1,
        .column_count = 2,
        .column_names = NULL,
        .error_message = (char*)NULL,
        .execution_time_ms = 150,
        .affected_rows = 0
    };

    PendingQueryResult pending = {
        .query_id = (char*)"test_query_123",
        .result = &result,
        .completed = true,
        .timed_out = false,
        .submitted_at = time(NULL),
        .timeout_seconds = 30
    };

    // Initialize mutex and condition variable
    pthread_mutex_init(&pending.result_lock, NULL);
    pthread_cond_init(&pending.result_ready, NULL);

    json_t* response = build_response_json(query_ref, database, &cache_entry, &selected_queue, &pending);

    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_TRUE(json_is_object(response));

    // Check success field
    json_t* success = json_object_get(response, "success");
    TEST_ASSERT_TRUE(json_is_true(success));

    // Check query_ref
    json_t* resp_query_ref = json_object_get(response, "query_ref");
    TEST_ASSERT_TRUE(json_is_integer(resp_query_ref));
    TEST_ASSERT_EQUAL(123, json_integer_value(resp_query_ref));

    // Check description
    json_t* description = json_object_get(response, "description");
    TEST_ASSERT_TRUE(json_is_string(description));
    TEST_ASSERT_EQUAL_STRING("Test query", json_string_value(description));

    // Check rows (should be parsed from data_json)
    json_t* rows = json_object_get(response, "rows");
    TEST_ASSERT_NOT_NULL(rows);
    TEST_ASSERT_TRUE(json_is_array(rows));
    TEST_ASSERT_EQUAL(1, json_array_size(rows));

    // Check metadata
    json_t* row_count = json_object_get(response, "row_count");
    json_t* column_count = json_object_get(response, "column_count");
    json_t* execution_time = json_object_get(response, "execution_time_ms");
    json_t* queue_used = json_object_get(response, "queue_used");

    TEST_ASSERT_TRUE(json_is_integer(row_count));
    TEST_ASSERT_TRUE(json_is_integer(column_count));
    TEST_ASSERT_TRUE(json_is_integer(execution_time));
    TEST_ASSERT_TRUE(json_is_string(queue_used));

    TEST_ASSERT_EQUAL(1, json_integer_value(row_count));
    TEST_ASSERT_EQUAL(2, json_integer_value(column_count));
    TEST_ASSERT_EQUAL(150, json_integer_value(execution_time));
    TEST_ASSERT_EQUAL_STRING("read_queue", json_string_value(queue_used));

    json_decref(response);

    // Cleanup
    pthread_mutex_destroy(&pending.result_lock);
    pthread_cond_destroy(&pending.result_ready);
}

// Test timeout error response
void test_build_response_json_timeout(void) {
    int query_ref = 456;
    const char* database = "test_db";
    QueryCacheEntry cache_entry = {
        .query_ref = 456,
        .sql_template = (char*)"SELECT * FROM timeout_test",
        .description = (char*)"Timeout query",
        .queue_type = (char*)"read_queue",
        .timeout_seconds = 30,
        .last_used = 0,
        .usage_count = 0
    };
    DatabaseQueue selected_queue = {
        .database_name = (char*)"test_db",
        .connection_string = (char*)"test_conn",
        .engine_type = DB_ENGINE_SQLITE,
        .queue_type = (char*)"read_queue",
        .bootstrap_query = NULL,
        .queue = NULL,
        .worker_thread = 0,
        .worker_thread_started = false,
        .is_lead_queue = false,
        .can_spawn_queues = false,
        .tags = NULL,
        .queue_number = 0,
        .active_connections = 0,
        .total_queries_processed = 0,
        .current_queue_depth = 0,
        .last_heartbeat = 0,
        .last_connection_attempt = 0,
        .last_request_time = 0,
        .heartbeat_interval_seconds = 30,
        .persistent_connection = NULL,
        .latest_available_migration = 0,
        .latest_installed_migration = 0,
        .empty_database = false,
        .query_cache = NULL,
        .shutdown_requested = false,
        .is_connected = false,
        .bootstrap_completed = false,
        .initial_connection_attempted = false,
        .conductor_sequence_completed = false
    };

    PendingQueryResult pending = {
        .query_id = (char*)"test_query_456",
        .result = NULL,
        .completed = true,
        .timed_out = true,
        .submitted_at = time(NULL),
        .timeout_seconds = 30
    };

    // Initialize mutex and condition variable
    pthread_mutex_init(&pending.result_lock, NULL);
    pthread_cond_init(&pending.result_ready, NULL);

    json_t* response = build_response_json(query_ref, database, &cache_entry, &selected_queue, &pending);

    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_TRUE(json_is_object(response));

    // Check failure
    json_t* success = json_object_get(response, "success");
    TEST_ASSERT_TRUE(json_is_false(success));

    // Check error message
    json_t* error = json_object_get(response, "error");
    TEST_ASSERT_TRUE(json_is_string(error));
    TEST_ASSERT_EQUAL_STRING("Query execution timeout", json_string_value(error));

    // Check timeout info
    json_t* timeout_seconds = json_object_get(response, "timeout_seconds");
    TEST_ASSERT_TRUE(json_is_integer(timeout_seconds));
    TEST_ASSERT_EQUAL(30, json_integer_value(timeout_seconds));

    json_decref(response);

    // Cleanup
    pthread_mutex_destroy(&pending.result_lock);
    pthread_cond_destroy(&pending.result_ready);
}

// Test database error response
void test_build_response_json_database_error(void) {
    int query_ref = 789;
    const char* database = "test_db";
    QueryCacheEntry cache_entry = {
        .query_ref = 789,
        .sql_template = (char*)"SELECT * FROM error_test",
        .description = (char*)"Error query",
        .queue_type = (char*)"read_queue",
        .timeout_seconds = 30,
        .last_used = 0,
        .usage_count = 0
    };
    DatabaseQueue selected_queue = {
        .database_name = (char*)"test_db",
        .connection_string = (char*)"test_conn",
        .engine_type = DB_ENGINE_SQLITE,
        .queue_type = (char*)"read_queue",
        .bootstrap_query = NULL,
        .queue = NULL,
        .worker_thread = 0,
        .worker_thread_started = false,
        .is_lead_queue = false,
        .can_spawn_queues = false,
        .tags = NULL,
        .queue_number = 0,
        .active_connections = 0,
        .total_queries_processed = 0,
        .current_queue_depth = 0,
        .last_heartbeat = 0,
        .last_connection_attempt = 0,
        .last_request_time = 0,
        .heartbeat_interval_seconds = 30,
        .persistent_connection = NULL,
        .latest_available_migration = 0,
        .latest_installed_migration = 0,
        .empty_database = false,
        .query_cache = NULL,
        .shutdown_requested = false,
        .is_connected = false,
        .bootstrap_completed = false,
        .initial_connection_attempted = false,
        .conductor_sequence_completed = false
    };

    QueryResult result = {
        .success = false,
        .data_json = NULL,
        .row_count = 0,
        .column_count = 0,
        .column_names = NULL,
        .error_message = (char*)"Connection failed",
        .execution_time_ms = 0,
        .affected_rows = 0
    };

    PendingQueryResult pending = {
        .query_id = (char*)"test_query_789",
        .result = &result,
        .completed = true,
        .timed_out = false,
        .submitted_at = time(NULL),
        .timeout_seconds = 30
    };

    // Initialize mutex and condition variable
    pthread_mutex_init(&pending.result_lock, NULL);
    pthread_cond_init(&pending.result_ready, NULL);

    json_t* response = build_response_json(query_ref, database, &cache_entry, &selected_queue, &pending);

    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_TRUE(json_is_object(response));

    // Check failure
    json_t* success = json_object_get(response, "success");
    TEST_ASSERT_TRUE(json_is_false(success));

    // Check error messages
    json_t* error = json_object_get(response, "error");
    json_t* db_error = json_object_get(response, "database_error");

    TEST_ASSERT_TRUE(json_is_string(error));
    TEST_ASSERT_TRUE(json_is_string(db_error));
    TEST_ASSERT_EQUAL_STRING("Database error", json_string_value(error));
    TEST_ASSERT_EQUAL_STRING("Connection failed", json_string_value(db_error));

    json_decref(response);

    // Cleanup
    pthread_mutex_destroy(&pending.result_lock);
    pthread_cond_destroy(&pending.result_ready);
}

// Test generic failure response
void test_build_response_json_generic_failure(void) {
    int query_ref = 999;
    const char* database = "test_db";
    QueryCacheEntry cache_entry = {
        .query_ref = 999,
        .sql_template = (char*)"SELECT * FROM failure_test",
        .description = (char*)"Failed query",
        .queue_type = (char*)"read_queue",
        .timeout_seconds = 30,
        .last_used = 0,
        .usage_count = 0
    };
    DatabaseQueue selected_queue = {
        .database_name = (char*)"test_db",
        .connection_string = (char*)"test_conn",
        .engine_type = DB_ENGINE_SQLITE,
        .queue_type = (char*)"read_queue",
        .bootstrap_query = NULL,
        .queue = NULL,
        .worker_thread = 0,
        .worker_thread_started = false,
        .is_lead_queue = false,
        .can_spawn_queues = false,
        .tags = NULL,
        .queue_number = 0,
        .active_connections = 0,
        .total_queries_processed = 0,
        .current_queue_depth = 0,
        .last_heartbeat = 0,
        .last_connection_attempt = 0,
        .last_request_time = 0,
        .heartbeat_interval_seconds = 30,
        .persistent_connection = NULL,
        .latest_available_migration = 0,
        .latest_installed_migration = 0,
        .empty_database = false,
        .query_cache = NULL,
        .shutdown_requested = false,
        .is_connected = false,
        .bootstrap_completed = false,
        .initial_connection_attempted = false,
        .conductor_sequence_completed = false
    };

    PendingQueryResult pending = {
        .query_id = (char*)"test_query_999",
        .result = NULL,  // No result means generic failure
        .completed = true,
        .timed_out = false,
        .submitted_at = time(NULL),
        .timeout_seconds = 30
    };

    // Initialize mutex and condition variable
    pthread_mutex_init(&pending.result_lock, NULL);
    pthread_cond_init(&pending.result_ready, NULL);

    json_t* response = build_response_json(query_ref, database, &cache_entry, &selected_queue, &pending);

    TEST_ASSERT_NOT_NULL(response);
    TEST_ASSERT_TRUE(json_is_object(response));

    // Check failure
    json_t* success = json_object_get(response, "success");
    TEST_ASSERT_TRUE(json_is_false(success));

    // Check error message
    json_t* error = json_object_get(response, "error");
    TEST_ASSERT_TRUE(json_is_string(error));
    TEST_ASSERT_EQUAL_STRING("Query execution failed", json_string_value(error));

    json_decref(response);

    // Cleanup
    pthread_mutex_destroy(&pending.result_lock);
    pthread_cond_destroy(&pending.result_ready);
}

// Test response with NULL description
void test_build_response_json_null_description(void) {
    int query_ref = 111;
    const char* database = "test_db";
    QueryCacheEntry cache_entry = {
        .query_ref = 111,
        .sql_template = (char*)"SELECT * FROM null_desc_test",
        .description = NULL,
        .queue_type = (char*)"read_queue",
        .timeout_seconds = 30,
        .last_used = 0,
        .usage_count = 0
    };
    DatabaseQueue selected_queue = {
        .database_name = (char*)"test_db",
        .connection_string = (char*)"test_conn",
        .engine_type = DB_ENGINE_SQLITE,
        .queue_type = (char*)"read_queue",
        .bootstrap_query = NULL,
        .queue = NULL,
        .worker_thread = 0,
        .worker_thread_started = false,
        .is_lead_queue = false,
        .can_spawn_queues = false,
        .tags = NULL,
        .queue_number = 0,
        .active_connections = 0,
        .total_queries_processed = 0,
        .current_queue_depth = 0,
        .last_heartbeat = 0,
        .last_connection_attempt = 0,
        .last_request_time = 0,
        .heartbeat_interval_seconds = 30,
        .persistent_connection = NULL,
        .latest_available_migration = 0,
        .latest_installed_migration = 0,
        .empty_database = false,
        .query_cache = NULL,
        .shutdown_requested = false,
        .is_connected = false,
        .bootstrap_completed = false,
        .initial_connection_attempted = false,
        .conductor_sequence_completed = false
    };

    QueryResult result = {
        .success = true,
        .data_json = (char*)"[]",
        .row_count = 0,
        .column_count = 0,
        .column_names = NULL,
        .error_message = (char*)NULL,
        .execution_time_ms = 50,
        .affected_rows = 0
    };

    PendingQueryResult pending = {
        .query_id = (char*)"test_query_111",
        .result = &result,
        .completed = true,
        .timed_out = false,
        .submitted_at = time(NULL),
        .timeout_seconds = 30
    };

    // Initialize mutex and condition variable
    pthread_mutex_init(&pending.result_lock, NULL);
    pthread_cond_init(&pending.result_ready, NULL);

    json_t* response = build_response_json(query_ref, database, &cache_entry, &selected_queue, &pending);

    TEST_ASSERT_NOT_NULL(response);

    // Check description is empty string when NULL
    json_t* description = json_object_get(response, "description");
    TEST_ASSERT_TRUE(json_is_string(description));
    TEST_ASSERT_EQUAL_STRING("", json_string_value(description));

    json_decref(response);

    // Cleanup
    pthread_mutex_destroy(&pending.result_lock);
    pthread_cond_destroy(&pending.result_ready);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_build_response_json_success);
    RUN_TEST(test_build_response_json_timeout);
    RUN_TEST(test_build_response_json_database_error);
    RUN_TEST(test_build_response_json_generic_failure);
    RUN_TEST(test_build_response_json_null_description);

    return UNITY_END();
}