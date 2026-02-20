/*
 * Unity Test File: Alt Queries Database Lookup Errors
 * This file contains unit tests for database lookup error handling in alt_queries.c
 *
 * Tests error paths for database lookup by connection name.
 *
 * CHANGELOG:
 * 2026-02-18: Initial creation of unit tests for database lookup errors
 *
 * TEST_VERSION: 1.0.0
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Enable mocks for external dependencies
#define USE_MOCK_LIBMICROHTTPD
#include <unity/mocks/mock_libmicrohttpd.h>

// Include source headers
#include <src/api/conduit/queries/queries.h>
#include <src/api/conduit/alt_queries/alt_queries.h>

// Mock app_config for testing
extern AppConfig *app_config;

// Function prototypes for test functions
void test_alt_queries_database_not_found(void);
void test_alt_queries_deduplicate_empty_array(void);
void test_alt_queries_deduplicate_null_parameters(void);
void test_alt_queries_rate_limit_exceeded(void);
void test_alt_queries_invalid_query_objects(void);

// Test fixtures
void setUp(void) {
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

// Test database not found error
void test_alt_queries_database_not_found(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;

    // Create a queries array with one query
    json_t *queries_array = json_array();
    json_t *query_obj = json_object();
    json_object_set_new(query_obj, "query_ref", json_integer(1));
    json_array_append_new(queries_array, query_obj);

    json_t *deduplicated_queries = NULL;
    size_t *mapping_array = NULL;
    bool *is_duplicate = NULL;
    DeduplicationResult result_code = DEDUP_OK;

    // Test with a database that doesn't exist
    enum MHD_Result result = alt_queries_deduplicate_and_validate(
        mock_connection, queries_array, "completely_nonexistent_database",
        &deduplicated_queries, &mapping_array, &is_duplicate, &result_code
    );

    json_decref(queries_array);

    // Should fail with database not found
    TEST_ASSERT_EQUAL(MHD_NO, result);
    TEST_ASSERT_EQUAL(DEDUP_DATABASE_NOT_FOUND, result_code);
}

// Test deduplication with empty array
void test_alt_queries_deduplicate_empty_array(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;

    // Create an empty queries array
    json_t *queries_array = json_array();

    json_t *deduplicated_queries = NULL;
    size_t *mapping_array = NULL;
    bool *is_duplicate = NULL;
    DeduplicationResult result_code = DEDUP_ERROR;

    // Test deduplication with empty array
    enum MHD_Result result = alt_queries_deduplicate_and_validate(
        mock_connection, queries_array, "testdb",
        &deduplicated_queries, &mapping_array, &is_duplicate, &result_code
    );

    json_decref(queries_array);

    // Should succeed with empty array
    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(DEDUP_OK, result_code);
    TEST_ASSERT_NOT_NULL(deduplicated_queries);
    TEST_ASSERT_EQUAL(0, json_array_size(deduplicated_queries));

    json_decref(deduplicated_queries);
    free(mapping_array);
    free(is_duplicate);
}

// Test deduplication with NULL parameters
void test_alt_queries_deduplicate_null_parameters(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;
    json_t *queries_array = json_array();

    // Test with NULL output parameters
    enum MHD_Result result = alt_queries_deduplicate_and_validate(
        mock_connection, queries_array, "testdb",
        NULL, NULL, NULL, NULL
    );

    json_decref(queries_array);

    // Should fail with NULL parameters
    TEST_ASSERT_EQUAL(MHD_NO, result);
}

// Test rate limit exceeded
void test_alt_queries_rate_limit_exceeded(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;

    // Create a queries array with many unique queries (over limit of 5)
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
    DeduplicationResult result_code = DEDUP_OK;

    // Test with valid database but too many queries
    enum MHD_Result result = alt_queries_deduplicate_and_validate(
        mock_connection, queries_array, "testdb",
        &deduplicated_queries, &mapping_array, &is_duplicate, &result_code
    );

    json_decref(queries_array);

    // Should fail due to rate limit
    TEST_ASSERT_EQUAL(MHD_NO, result);
    TEST_ASSERT_EQUAL(DEDUP_RATE_LIMIT, result_code);
    TEST_ASSERT_NULL(deduplicated_queries);
    TEST_ASSERT_NULL(mapping_array);
    TEST_ASSERT_NULL(is_duplicate);
}

// Test deduplication with invalid query objects
void test_alt_queries_invalid_query_objects(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;

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

    DeduplicationResult result_code = DEDUP_OK;
    enum MHD_Result result = alt_queries_deduplicate_and_validate(
        mock_connection, queries_array, "testdb",
        &deduplicated_queries, &mapping_array, &is_duplicate, &result_code
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_NOT_NULL(deduplicated_queries);
    TEST_ASSERT_EQUAL(1, json_array_size(deduplicated_queries));  // Only the valid query
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

    RUN_TEST(test_alt_queries_database_not_found);
    RUN_TEST(test_alt_queries_deduplicate_empty_array);
    RUN_TEST(test_alt_queries_deduplicate_null_parameters);
    RUN_TEST(test_alt_queries_rate_limit_exceeded);
    RUN_TEST(test_alt_queries_invalid_query_objects);

    return UNITY_END();
}
