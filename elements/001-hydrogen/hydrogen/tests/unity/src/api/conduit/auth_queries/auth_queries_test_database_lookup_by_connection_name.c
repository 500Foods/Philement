/*
 * Unity Test File: Auth Queries Database Lookup by Connection Name
 * This file contains unit tests for the database lookup by connection_name
 * fallback path in src/api/conduit/auth_queries/auth_queries.c
 *
 * Tests the path where database is found by connection_name instead of
 * database name in auth_queries_deduplicate_and_validate.
 * Covers lines 96-97 of auth_queries.c.
 *
 * CHANGELOG:
 * 2026-02-19: Initial creation
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
#include <src/api/conduit/auth_queries/auth_queries.h>

// Mock app_config for testing
extern AppConfig *app_config;

// Function prototypes for test functions
void test_auth_queries_dedup_database_lookup_by_connection_name(void);
void test_auth_queries_dedup_database_not_found(void);
void test_auth_queries_dedup_empty_array(void);
void test_auth_queries_dedup_null_parameters(void);
void test_auth_queries_dedup_rate_limit_exceeded(void);
void test_auth_queries_dedup_invalid_query_objects(void);
void test_auth_queries_dedup_duplicate_queries(void);

// Test fixtures
void setUp(void) {
    mock_mhd_reset_all();

    // Initialize mock app config
    app_config = calloc(1, sizeof(AppConfig));
    TEST_ASSERT_NOT_NULL(app_config);

    // Set up with a connection that only has connection_name, no database name
    // This forces the fallback path through connection_name lookup
    app_config->databases.connection_count = 1;

    DatabaseConnection *conn = &app_config->databases.connections[0];
    conn->enabled = true;
    conn->connection_name = strdup("myconnection");
    conn->max_queries_per_request = 5;
}

void tearDown(void) {
    if (app_config) {
        for (int i = 0; i < app_config->databases.connection_count; i++) {
            free(app_config->databases.connections[i].connection_name);
        }
        free(app_config);
        app_config = NULL;
    }
    mock_mhd_reset_all();
}

// Test database lookup by connection_name fallback path
// Covers lines 96-97: db_conn = conn; break;
void test_auth_queries_dedup_database_lookup_by_connection_name(void) {
    json_t *queries_array = json_array();
    json_t *query = json_object();
    json_object_set_new(query, "query_ref", json_integer(1));
    json_array_append_new(queries_array, query);

    json_t *deduplicated_queries = NULL;
    size_t *mapping_array = NULL;
    bool *is_duplicate = NULL;
    DeduplicationResult dedup_code = DEDUP_OK;

    // Use "myconnection" - find_database_connection won't find it by db name,
    // so the fallback loop checks connection_name and finds it
    enum MHD_Result result = auth_queries_deduplicate_and_validate(
        NULL, queries_array, "myconnection",
        &deduplicated_queries, &mapping_array, &is_duplicate, &dedup_code
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_NOT_NULL(deduplicated_queries);
    TEST_ASSERT_EQUAL(1, json_array_size(deduplicated_queries));
    TEST_ASSERT_EQUAL(DEDUP_OK, dedup_code);

    free(mapping_array);
    free(is_duplicate);
    json_decref(deduplicated_queries);
    json_decref(queries_array);
}

// Test database not found error
void test_auth_queries_dedup_database_not_found(void) {
    json_t *queries_array = json_array();
    json_t *query = json_object();
    json_object_set_new(query, "query_ref", json_integer(1));
    json_array_append_new(queries_array, query);

    json_t *deduplicated_queries = NULL;
    size_t *mapping_array = NULL;
    bool *is_duplicate = NULL;
    DeduplicationResult result_code = DEDUP_OK;

    // Use a database name that doesn't exist
    enum MHD_Result result = auth_queries_deduplicate_and_validate(
        NULL, queries_array, "completely_nonexistent_database",
        &deduplicated_queries, &mapping_array, &is_duplicate, &result_code
    );

    json_decref(queries_array);

    TEST_ASSERT_EQUAL(MHD_NO, result);
    TEST_ASSERT_EQUAL(DEDUP_DATABASE_NOT_FOUND, result_code);
}

// Test deduplication with empty array
void test_auth_queries_dedup_empty_array(void) {
    json_t *queries_array = json_array();

    json_t *deduplicated_queries = NULL;
    size_t *mapping_array = NULL;
    bool *is_duplicate = NULL;
    DeduplicationResult result_code = DEDUP_ERROR;

    enum MHD_Result result = auth_queries_deduplicate_and_validate(
        NULL, queries_array, "myconnection",
        &deduplicated_queries, &mapping_array, &is_duplicate, &result_code
    );

    json_decref(queries_array);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_EQUAL(DEDUP_OK, result_code);
    TEST_ASSERT_NOT_NULL(deduplicated_queries);
    TEST_ASSERT_EQUAL(0, json_array_size(deduplicated_queries));

    json_decref(deduplicated_queries);
}

// Test deduplication with NULL parameters
void test_auth_queries_dedup_null_parameters(void) {
    json_t *queries_array = json_array();
    DeduplicationResult result_code = DEDUP_OK;

    enum MHD_Result result = auth_queries_deduplicate_and_validate(
        NULL, queries_array, "myconnection",
        NULL, NULL, NULL, &result_code
    );

    json_decref(queries_array);

    TEST_ASSERT_EQUAL(MHD_NO, result);
    TEST_ASSERT_EQUAL(DEDUP_ERROR, result_code);
}

// Test rate limit exceeded
void test_auth_queries_dedup_rate_limit_exceeded(void) {
    // Create more unique queries than max_queries_per_request (5)
    json_t *queries_array = json_array();
    for (int i = 1; i <= 6; i++) {
        json_t *query = json_object();
        json_object_set_new(query, "query_ref", json_integer(i));
        json_array_append_new(queries_array, query);
    }

    json_t *deduplicated_queries = NULL;
    size_t *mapping_array = NULL;
    bool *is_duplicate = NULL;
    DeduplicationResult result_code = DEDUP_OK;

    enum MHD_Result result = auth_queries_deduplicate_and_validate(
        NULL, queries_array, "myconnection",
        &deduplicated_queries, &mapping_array, &is_duplicate, &result_code
    );

    json_decref(queries_array);

    TEST_ASSERT_EQUAL(MHD_NO, result);
    TEST_ASSERT_EQUAL(DEDUP_RATE_LIMIT, result_code);
    TEST_ASSERT_NULL(deduplicated_queries);
    TEST_ASSERT_NULL(mapping_array);
    TEST_ASSERT_NULL(is_duplicate);
}

// Test deduplication with invalid query objects
void test_auth_queries_dedup_invalid_query_objects(void) {
    json_t *queries_array = json_array();

    // Add a valid query
    json_t *valid_query = json_object();
    json_object_set_new(valid_query, "query_ref", json_integer(1));
    json_array_append_new(queries_array, valid_query);

    // Add an invalid query (not an object)
    json_array_append_new(queries_array, json_string("invalid"));

    // Add a query without query_ref
    json_t *no_ref_query = json_object();
    json_object_set_new(no_ref_query, "some_field", json_integer(123));
    json_array_append_new(queries_array, no_ref_query);

    // Add a query with non-integer query_ref
    json_t *bad_ref_query = json_object();
    json_object_set_new(bad_ref_query, "query_ref", json_string("not_a_number"));
    json_array_append_new(queries_array, bad_ref_query);

    json_t *deduplicated_queries = NULL;
    size_t *mapping_array = NULL;
    bool *is_duplicate = NULL;
    DeduplicationResult result_code = DEDUP_OK;

    enum MHD_Result result = auth_queries_deduplicate_and_validate(
        NULL, queries_array, "myconnection",
        &deduplicated_queries, &mapping_array, &is_duplicate, &result_code
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_NOT_NULL(deduplicated_queries);
    TEST_ASSERT_EQUAL(1, json_array_size(deduplicated_queries));
    TEST_ASSERT_NOT_NULL(is_duplicate);

    // Invalid queries are marked as duplicates
    TEST_ASSERT_FALSE(is_duplicate[0]);  // Valid query
    TEST_ASSERT_TRUE(is_duplicate[1]);   // Not an object
    TEST_ASSERT_TRUE(is_duplicate[2]);   // No query_ref
    TEST_ASSERT_TRUE(is_duplicate[3]);   // Non-integer query_ref

    free(mapping_array);
    free(is_duplicate);
    json_decref(deduplicated_queries);
    json_decref(queries_array);
}

// Test deduplication with duplicate queries
void test_auth_queries_dedup_duplicate_queries(void) {
    json_t *queries_array = json_array();

    // Add same query twice
    json_t *query1 = json_object();
    json_object_set_new(query1, "query_ref", json_integer(1));
    json_array_append_new(queries_array, query1);

    json_t *query2 = json_object();
    json_object_set_new(query2, "query_ref", json_integer(1));
    json_array_append_new(queries_array, query2);

    // Add a different query
    json_t *query3 = json_object();
    json_object_set_new(query3, "query_ref", json_integer(2));
    json_array_append_new(queries_array, query3);

    json_t *deduplicated_queries = NULL;
    size_t *mapping_array = NULL;
    bool *is_duplicate = NULL;
    DeduplicationResult result_code = DEDUP_OK;

    enum MHD_Result result = auth_queries_deduplicate_and_validate(
        NULL, queries_array, "myconnection",
        &deduplicated_queries, &mapping_array, &is_duplicate, &result_code
    );

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_NOT_NULL(deduplicated_queries);
    TEST_ASSERT_EQUAL(2, json_array_size(deduplicated_queries));  // Two unique queries
    TEST_ASSERT_EQUAL(DEDUP_OK, result_code);

    // First query is original, second is duplicate, third is unique
    TEST_ASSERT_FALSE(is_duplicate[0]);
    TEST_ASSERT_TRUE(is_duplicate[1]);   // Duplicate of query 1
    TEST_ASSERT_FALSE(is_duplicate[2]);

    free(mapping_array);
    free(is_duplicate);
    json_decref(deduplicated_queries);
    json_decref(queries_array);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_auth_queries_dedup_database_lookup_by_connection_name);
    RUN_TEST(test_auth_queries_dedup_database_not_found);
    RUN_TEST(test_auth_queries_dedup_empty_array);
    RUN_TEST(test_auth_queries_dedup_null_parameters);
    RUN_TEST(test_auth_queries_dedup_rate_limit_exceeded);
    RUN_TEST(test_auth_queries_dedup_invalid_query_objects);
    RUN_TEST(test_auth_queries_dedup_duplicate_queries);

    return UNITY_END();
}
