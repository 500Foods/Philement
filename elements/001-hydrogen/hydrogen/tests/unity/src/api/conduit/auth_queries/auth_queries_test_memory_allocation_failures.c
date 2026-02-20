/*
 * Unity Test File: Auth Queries Memory Allocation Failures
 * This file contains unit tests for memory allocation failure paths
 * in src/api/conduit/auth_queries/auth_queries.c
 *
 * Tests error handling when memory allocation fails in the
 * auth_queries_deduplicate_and_validate function.
 *
 * CHANGELOG:
 * 2026-02-19: Fixed function name from deduplicate_and_validate_queries to
 *             auth_queries_deduplicate_and_validate (was testing wrong module)
 *
 * TEST_VERSION: 1.1.0
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Enable mocks for external dependencies
#define USE_MOCK_LIBMICROHTTPD
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_libmicrohttpd.h>
#include <unity/mocks/mock_system.h>

// Include source headers - queries.h first for DeduplicationResult enum
#include <src/api/conduit/queries/queries.h>
#include <src/api/conduit/auth_queries/auth_queries.h>

// Mock app_config for testing
extern AppConfig *app_config;

// No forward declaration needed - auth_queries_deduplicate_and_validate is
// declared in auth_queries.h which is included above

// Function prototypes for test functions
void test_auth_queries_dedup_is_duplicate_alloc_failure(void);
void test_auth_queries_dedup_query_refs_alloc_failure(void);
void test_auth_queries_dedup_output_arrays_alloc_failure(void);

// Test fixtures
void setUp(void) {
    mock_mhd_reset_all();
    mock_system_reset_all();

    // Initialize mock app config
    app_config = calloc(1, sizeof(AppConfig));
    TEST_ASSERT_NOT_NULL(app_config);

    // Initialize databases config
    app_config->databases.connection_count = 1;

    // Set up test database connection
    DatabaseConnection *conn = &app_config->databases.connections[0];
    conn->enabled = true;
    conn->connection_name = strdup("testdb");
    conn->max_queries_per_request = 5;

    // Reset the mock allocation counter so tests start from 0 allocations
    // after setUp()
    mock_malloc_call_count = 0;
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
    mock_system_reset_all();
}

// Helper to create a queries array with one valid query
static json_t *create_test_queries_array(void) {
    json_t *queries = json_array();
    json_t *query = json_object();
    json_object_set_new(query, "query_ref", json_integer(1));
    json_array_append_new(queries, query);
    return queries;
}

// Test auth_queries_deduplicate_and_validate when is_duplicate allocation fails
// Covers lines 112-113 of auth_queries.c
void test_auth_queries_dedup_is_duplicate_alloc_failure(void) {
    json_t *queries_array = create_test_queries_array();
    json_t *deduplicated_queries = NULL;
    size_t *mapping_array = NULL;
    bool *is_duplicate = NULL;
    DeduplicationResult dedup_code = DEDUP_OK;

    // Set the failure mode first - this resets the counter
    mock_system_set_malloc_failure(1);

    enum MHD_Result result = auth_queries_deduplicate_and_validate(
        NULL, queries_array, "testdb",
        &deduplicated_queries, &mapping_array, &is_duplicate, &dedup_code
    );

    // is_duplicate calloc fails → returns MHD_NO
    TEST_ASSERT_EQUAL(MHD_NO, result);
    TEST_ASSERT_NULL(is_duplicate);

    json_decref(queries_array);
}

// Test auth_queries_deduplicate_and_validate when query_refs allocation fails
// Covers lines 123-128 of auth_queries.c
void test_auth_queries_dedup_query_refs_alloc_failure(void) {
    json_t *queries_array = create_test_queries_array();
    json_t *deduplicated_queries = NULL;
    size_t *mapping_array = NULL;
    bool *is_duplicate = NULL;
    DeduplicationResult dedup_code = DEDUP_OK;

    // Set the failure mode first - this resets the counter
    mock_system_set_malloc_failure(2);

    enum MHD_Result result = auth_queries_deduplicate_and_validate(
        NULL, queries_array, "testdb",
        &deduplicated_queries, &mapping_array, &is_duplicate, &dedup_code
    );

    // query_refs calloc fails → returns MHD_NO, frees is_duplicate
    TEST_ASSERT_EQUAL(MHD_NO, result);

    json_decref(queries_array);
}

// Test auth_queries_deduplicate_and_validate when output mapping_array allocation fails
// Covers lines 196-203 of auth_queries.c
void test_auth_queries_dedup_output_arrays_alloc_failure(void) {
    json_t *queries_array = create_test_queries_array();
    json_t *deduplicated_queries = NULL;
    size_t *mapping_array = NULL;
    bool *is_duplicate = NULL;
    DeduplicationResult dedup_code = DEDUP_OK;

    // Set the failure mode first - this resets the counter
    // (1=is_duplicate, 2=query_refs, 3=query_params, 4=first_occurrence, 5=mapping_array)
    mock_system_set_malloc_failure(5);

    enum MHD_Result result = auth_queries_deduplicate_and_validate(
        NULL, queries_array, "testdb",
        &deduplicated_queries, &mapping_array, &is_duplicate, &dedup_code
    );

    // mapping_array calloc fails → !*mapping_array → returns MHD_NO
    TEST_ASSERT_EQUAL(MHD_NO, result);

    json_decref(queries_array);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_auth_queries_dedup_is_duplicate_alloc_failure);
    RUN_TEST(test_auth_queries_dedup_query_refs_alloc_failure);
    RUN_TEST(test_auth_queries_dedup_output_arrays_alloc_failure);

    return UNITY_END();
}
