/*
 * Unity Test File: Conduit Queries Deduplicate and Validate
 * This file contains unit tests for the deduplicate_and_validate_queries function
 * in src/api/conduit/queries/queries.c
 *
 * Tests rate limiting and query deduplication logic.
 *
 * CHANGELOG:
 * 2026-01-18: Initial creation of unit tests for deduplicate_and_validate_queries
 *
 * TEST_VERSION: 1.0.0
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Enable mocks for external dependencies
#define USE_MOCK_LIBMICROHTTPD
#include <unity/mocks/mock_libmicrohttpd.h>

// Include source header
#include <src/api/conduit/queries/queries.h>

// Mock app_config for testing
extern AppConfig *app_config;

// Function prototypes for test functions
void test_deduplicate_and_validate_queries_null_parameters(void);
void test_deduplicate_and_validate_queries_empty_array(void);
void test_deduplicate_and_validate_queries_unique_under_limit(void);
void test_deduplicate_and_validate_queries_with_duplicates(void);
void test_deduplicate_and_validate_queries_rate_limit_exceeded(void);
void test_deduplicate_and_validate_queries_duplicates_over_limit(void);
void test_deduplicate_and_validate_queries_unknown_database(void);
void test_deduplicate_and_validate_queries_memory_allocation_failure_duplicate_tracking(void);
void test_deduplicate_and_validate_queries_memory_allocation_failure_query_arrays(void);
void test_deduplicate_and_validate_queries_memory_allocation_failure_output_arrays(void);
void test_deduplicate_and_validate_queries_invalid_query_objects(void);

// Test fixtures
void setUp(void) {
    // Reset all mocks before each test
    mock_mhd_reset_all();

    // Initialize mock app config
    app_config = calloc(1, sizeof(AppConfig));
    TEST_ASSERT_NOT_NULL(app_config);

    // Initialize databases config
    app_config->databases.connection_count = 1;

    // Set up test database connection
    DatabaseConnection *conn = &app_config->databases.connections[0];
    conn->enabled = true;
    conn->connection_name = strdup("testdb");
    conn->max_queries_per_request = 5;  // Test limit
}

void tearDown(void) {
    // Clean up app config
    if (app_config) {
        for (int i = 0; i < app_config->databases.connection_count; i++) {
            free(app_config->databases.connections[i].connection_name);
        }
        free(app_config);
        app_config = NULL;
    }

    mock_mhd_reset_all();
}

// Test deduplicate_and_validate_queries with empty array
void test_deduplicate_and_validate_queries_empty_array(void) {
    json_t *queries_array = json_array();
    json_t *deduplicated_queries = NULL;
    size_t *mapping_array = NULL;
    bool *is_duplicate = NULL;

    DeduplicationResult dedup_code;
    enum MHD_Result result = deduplicate_and_validate_queries(NULL, queries_array, "testdb", &deduplicated_queries, &mapping_array, &is_duplicate, &dedup_code);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_NOT_NULL(deduplicated_queries);
    TEST_ASSERT_EQUAL(0, json_array_size(deduplicated_queries));
    TEST_ASSERT_NULL(mapping_array);
    TEST_ASSERT_NULL(is_duplicate);

    json_decref(deduplicated_queries);
    json_decref(queries_array);
}

// Test deduplicate_and_validate_queries with unique queries under limit
void test_deduplicate_and_validate_queries_unique_under_limit(void) {
    json_t *queries_array = json_array();

    // Add 3 unique queries
    json_t *query1 = json_object();
    json_object_set_new(query1, "query_ref", json_integer(1));
    json_array_append_new(queries_array, query1);

    json_t *query2 = json_object();
    json_object_set_new(query2, "query_ref", json_integer(2));
    json_array_append_new(queries_array, query2);

    json_t *query3 = json_object();
    json_object_set_new(query3, "query_ref", json_integer(3));
    json_array_append_new(queries_array, query3);

    json_t *deduplicated_queries = NULL;
    size_t *mapping_array = NULL;
    bool *is_duplicate = NULL;

    DeduplicationResult dedup_code;
    enum MHD_Result result = deduplicate_and_validate_queries(NULL, queries_array, "testdb", &deduplicated_queries, &mapping_array, &is_duplicate, &dedup_code);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_NOT_NULL(deduplicated_queries);
    TEST_ASSERT_EQUAL(3, json_array_size(deduplicated_queries));
    TEST_ASSERT_NOT_NULL(mapping_array);
    TEST_ASSERT_NOT_NULL(is_duplicate);

    // Check that none are duplicates
    for (size_t i = 0; i < 3; i++) {
        TEST_ASSERT_FALSE(is_duplicate[i]);
    }

    // Check mapping
    TEST_ASSERT_EQUAL(0, mapping_array[0]);
    TEST_ASSERT_EQUAL(1, mapping_array[1]);
    TEST_ASSERT_EQUAL(2, mapping_array[2]);

    free(mapping_array);
    free(is_duplicate);
    json_decref(deduplicated_queries);
    json_decref(queries_array);
}

// Test deduplicate_and_validate_queries with duplicates
void test_deduplicate_and_validate_queries_with_duplicates(void) {
    json_t *queries_array = json_array();

    // Add queries: 1, 2, 1, 3, 2 (duplicates)
    json_t *query1 = json_object();
    json_object_set_new(query1, "query_ref", json_integer(1));
    json_array_append_new(queries_array, query1);

    json_t *query2 = json_object();
    json_object_set_new(query2, "query_ref", json_integer(2));
    json_array_append_new(queries_array, query2);

    json_t *query3 = json_object();
    json_object_set_new(query3, "query_ref", json_integer(1));  // Duplicate
    json_array_append_new(queries_array, query3);

    json_t *query4 = json_object();
    json_object_set_new(query4, "query_ref", json_integer(3));
    json_array_append_new(queries_array, query4);

    json_t *query5 = json_object();
    json_object_set_new(query5, "query_ref", json_integer(2));  // Duplicate
    json_array_append_new(queries_array, query5);

    json_t *deduplicated_queries = NULL;
    size_t *mapping_array = NULL;
    bool *is_duplicate = NULL;

    DeduplicationResult dedup_code;
    enum MHD_Result result = deduplicate_and_validate_queries(NULL, queries_array, "testdb", &deduplicated_queries, &mapping_array, &is_duplicate, &dedup_code);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_NOT_NULL(deduplicated_queries);
    TEST_ASSERT_EQUAL(3, json_array_size(deduplicated_queries));  // Should be deduplicated to 3 unique
    TEST_ASSERT_NOT_NULL(mapping_array);
    TEST_ASSERT_NOT_NULL(is_duplicate);

    // Check duplicates
    TEST_ASSERT_FALSE(is_duplicate[0]);  // First occurrence
    TEST_ASSERT_FALSE(is_duplicate[1]);  // First occurrence
    TEST_ASSERT_TRUE(is_duplicate[2]);   // Duplicate
    TEST_ASSERT_FALSE(is_duplicate[3]);  // First occurrence
    TEST_ASSERT_TRUE(is_duplicate[4]);   // Duplicate

    // Check mapping - duplicates should map to first occurrence
    TEST_ASSERT_EQUAL(0, mapping_array[0]);  // query_ref 1 -> index 0
    TEST_ASSERT_EQUAL(1, mapping_array[1]);  // query_ref 2 -> index 1
    TEST_ASSERT_EQUAL(0, mapping_array[2]);  // duplicate query_ref 1 -> index 0
    TEST_ASSERT_EQUAL(2, mapping_array[3]);  // query_ref 3 -> index 2
    TEST_ASSERT_EQUAL(1, mapping_array[4]);  // duplicate query_ref 2 -> index 1

    free(mapping_array);
    free(is_duplicate);
    json_decref(deduplicated_queries);
    json_decref(queries_array);
}

// Test deduplicate_and_validate_queries exceeding rate limit
void test_deduplicate_and_validate_queries_rate_limit_exceeded(void) {
    json_t *queries_array = json_array();

    // Add 6 unique queries (limit is 5)
    for (int i = 1; i <= 6; i++) {
        json_t *query = json_object();
        json_object_set_new(query, "query_ref", json_integer(i));
        json_array_append_new(queries_array, query);
    }

    json_t *deduplicated_queries = NULL;
    size_t *mapping_array = NULL;
    bool *is_duplicate = NULL;

    DeduplicationResult dedup_code;
    enum MHD_Result result = deduplicate_and_validate_queries(NULL, queries_array, "testdb", &deduplicated_queries, &mapping_array, &is_duplicate, &dedup_code);

    TEST_ASSERT_EQUAL(MHD_NO, result);  // Should fail due to rate limit
    TEST_ASSERT_NULL(deduplicated_queries);
    TEST_ASSERT_NULL(mapping_array);
    TEST_ASSERT_NULL(is_duplicate);

    json_decref(queries_array);
}

// Test deduplicate_and_validate_queries with duplicates exceeding limit after deduplication
void test_deduplicate_and_validate_queries_duplicates_over_limit(void) {
    json_t *queries_array = json_array();

    // Add 8 queries with duplicates that result in 6 unique queries (over limit of 5)
    int query_refs[] = {1, 2, 3, 1, 4, 2, 5, 6};
    for (int i = 0; i < 8; i++) {
        json_t *query = json_object();
        json_object_set_new(query, "query_ref", json_integer(query_refs[i]));
        json_array_append_new(queries_array, query);
    }

    json_t *deduplicated_queries = NULL;
    size_t *mapping_array = NULL;
    bool *is_duplicate = NULL;

    DeduplicationResult dedup_code;
    enum MHD_Result result = deduplicate_and_validate_queries(NULL, queries_array, "testdb", &deduplicated_queries, &mapping_array, &is_duplicate, &dedup_code);

    TEST_ASSERT_EQUAL(MHD_NO, result);  // Should fail due to rate limit (6 unique > 5 limit)
    TEST_ASSERT_NULL(deduplicated_queries);
    TEST_ASSERT_NULL(mapping_array);
    TEST_ASSERT_NULL(is_duplicate);

    json_decref(queries_array);
}

// Test deduplicate_and_validate_queries with unknown database
void test_deduplicate_and_validate_queries_unknown_database(void) {
    json_t *queries_array = json_array();

    json_t *query = json_object();
    json_object_set_new(query, "query_ref", json_integer(1));
    json_array_append_new(queries_array, query);

    json_t *deduplicated_queries = NULL;
    size_t *mapping_array = NULL;
    bool *is_duplicate = NULL;

    DeduplicationResult dedup_code;
    enum MHD_Result result = deduplicate_and_validate_queries(NULL, queries_array, "nonexistent", &deduplicated_queries, &mapping_array, &is_duplicate, &dedup_code);

    TEST_ASSERT_EQUAL(MHD_NO, result);  // Should fail
    TEST_ASSERT_EQUAL(DEDUP_DATABASE_NOT_FOUND, dedup_code);  // Should have correct error code
    TEST_ASSERT_NULL(deduplicated_queries);
    TEST_ASSERT_NULL(mapping_array);
    TEST_ASSERT_NULL(is_duplicate);

    json_decref(queries_array);
}

// Test deduplicate_and_validate_queries with NULL parameters
void test_deduplicate_and_validate_queries_null_parameters(void) {
    json_t *queries_array = json_array();
    json_t *query = json_object();
    json_object_set_new(query, "query_ref", json_integer(1));
    json_array_append_new(queries_array, query);

    DeduplicationResult dedup_code;

    // Test NULL queries_array
    enum MHD_Result result = deduplicate_and_validate_queries(NULL, NULL, "testdb", NULL, NULL, NULL, &dedup_code);
    TEST_ASSERT_EQUAL(MHD_NO, result);
    TEST_ASSERT_EQUAL(DEDUP_ERROR, dedup_code);

    // Test NULL database
    result = deduplicate_and_validate_queries(NULL, queries_array, NULL, NULL, NULL, NULL, &dedup_code);
    TEST_ASSERT_EQUAL(MHD_NO, result);
    TEST_ASSERT_EQUAL(DEDUP_ERROR, dedup_code);

    // Test NULL deduplicated_queries
    result = deduplicate_and_validate_queries(NULL, queries_array, "testdb", NULL, NULL, NULL, &dedup_code);
    TEST_ASSERT_EQUAL(MHD_NO, result);
    TEST_ASSERT_EQUAL(DEDUP_ERROR, dedup_code);

    // Test NULL mapping_array
    size_t *dummy_mapping = NULL;
    result = deduplicate_and_validate_queries(NULL, queries_array, "testdb", &queries_array, NULL, NULL, &dedup_code);
    TEST_ASSERT_EQUAL(MHD_NO, result);
    TEST_ASSERT_EQUAL(DEDUP_ERROR, dedup_code);

    // Test NULL is_duplicate
    result = deduplicate_and_validate_queries(NULL, queries_array, "testdb", &queries_array, &dummy_mapping, NULL, &dedup_code);
    TEST_ASSERT_EQUAL(MHD_NO, result);
    TEST_ASSERT_EQUAL(DEDUP_ERROR, dedup_code);

    json_decref(queries_array);
}

// Test deduplicate_and_validate_queries with memory allocation failure for duplicate tracking
void test_deduplicate_and_validate_queries_memory_allocation_failure_duplicate_tracking(void) {
    // This test would require mocking malloc to fail, but since we can't easily do that
    // in this test framework, we'll skip it for now. In a real implementation with
    // dependency injection or malloc mocking, we'd test this path.
    TEST_IGNORE_MESSAGE("Memory allocation failure testing requires malloc mocking");
}

// Test deduplicate_and_validate_queries with memory allocation failure for query arrays
void test_deduplicate_and_validate_queries_memory_allocation_failure_query_arrays(void) {
    // Similar to above, requires malloc mocking
    TEST_IGNORE_MESSAGE("Memory allocation failure testing requires malloc mocking");
}

// Test deduplicate_and_validate_queries with memory allocation failure for output arrays
void test_deduplicate_and_validate_queries_memory_allocation_failure_output_arrays(void) {
    // Similar to above, requires malloc mocking
    TEST_IGNORE_MESSAGE("Memory allocation failure testing requires malloc mocking");
}

// Test deduplicate_and_validate_queries with invalid query objects
void test_deduplicate_and_validate_queries_invalid_query_objects(void) {
    json_t *queries_array = json_array();

    // Add a valid query
    json_t *valid_query = json_object();
    json_object_set_new(valid_query, "query_ref", json_integer(1));
    json_array_append_new(queries_array, valid_query);

    // Add an invalid query (not an object)
    json_array_append_new(queries_array, json_string("invalid"));

    // Add another invalid query (object but no query_ref)
    json_t *invalid_query = json_object();
    json_object_set_new(invalid_query, "some_field", json_integer(123));
    json_array_append_new(queries_array, invalid_query);

    // Add another invalid query (query_ref not integer)
    json_t *invalid_query2 = json_object();
    json_object_set_new(invalid_query2, "query_ref", json_string("not_a_number"));
    json_array_append_new(queries_array, invalid_query2);

    json_t *deduplicated_queries = NULL;
    size_t *mapping_array = NULL;
    bool *is_duplicate = NULL;

    DeduplicationResult dedup_code;
    enum MHD_Result result = deduplicate_and_validate_queries(NULL, queries_array, "testdb", &deduplicated_queries, &mapping_array, &is_duplicate, &dedup_code);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_NOT_NULL(deduplicated_queries);
    TEST_ASSERT_EQUAL(1, json_array_size(deduplicated_queries));  // Only the valid query should be deduplicated
    TEST_ASSERT_NOT_NULL(mapping_array);
    TEST_ASSERT_NOT_NULL(is_duplicate);

    // Check that invalid queries are marked as duplicates
    TEST_ASSERT_FALSE(is_duplicate[0]);  // First query is valid
    TEST_ASSERT_TRUE(is_duplicate[1]);   // Second query is invalid
    TEST_ASSERT_TRUE(is_duplicate[2]);   // Third query has no query_ref
    TEST_ASSERT_TRUE(is_duplicate[3]);   // Fourth query has invalid query_ref type

    free(mapping_array);
    free(is_duplicate);
    json_decref(deduplicated_queries);
    json_decref(queries_array);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_deduplicate_and_validate_queries_null_parameters);
    RUN_TEST(test_deduplicate_and_validate_queries_empty_array);
    RUN_TEST(test_deduplicate_and_validate_queries_unique_under_limit);
    RUN_TEST(test_deduplicate_and_validate_queries_with_duplicates);
    RUN_TEST(test_deduplicate_and_validate_queries_rate_limit_exceeded);
    RUN_TEST(test_deduplicate_and_validate_queries_duplicates_over_limit);
    RUN_TEST(test_deduplicate_and_validate_queries_unknown_database);
    RUN_TEST(test_deduplicate_and_validate_queries_memory_allocation_failure_duplicate_tracking);
    RUN_TEST(test_deduplicate_and_validate_queries_memory_allocation_failure_query_arrays);
    RUN_TEST(test_deduplicate_and_validate_queries_memory_allocation_failure_output_arrays);
    RUN_TEST(test_deduplicate_and_validate_queries_invalid_query_objects);

    return UNITY_END();
}