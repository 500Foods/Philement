/*
 * Unity Test File: Alt Queries Memory Allocation Failures
 * This file contains unit tests for memory allocation failure paths
 * in src/api/conduit/alt_queries/alt_queries.c
 *
 * Tests error handling when memory allocation fails in the
 * alt_queries_deduplicate_and_validate function.
 *
 * CHANGELOG:
 * 2026-02-18: Initial creation of unit tests for memory allocation failures
 * 2026-02-19: Implemented real malloc failure tests using USE_MOCK_SYSTEM
 *             mock_system_set_malloc_failure(N): fails the Nth intercepted alloc
 *             alt_queries.c intercepted allocs in dedup function:
 *               #1 = calloc for is_duplicate
 *               #2 = calloc for query_refs
 *               #5 = calloc for mapping_array
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
#include <src/api/conduit/alt_queries/alt_queries.h>

// Mock app_config for testing
extern AppConfig *app_config;

// Forward declaration for the function we're testing
enum MHD_Result alt_queries_deduplicate_and_validate(
    struct MHD_Connection *connection,
    json_t *queries_array,
    const char *database,
    json_t **deduplicated_queries,
    size_t **mapping_array,
    bool **is_duplicate,
    DeduplicationResult *result_code);

// Function prototypes for test functions
void test_alt_queries_dedup_is_duplicate_alloc_failure(void);
void test_alt_queries_dedup_query_refs_alloc_failure(void);
void test_alt_queries_dedup_output_arrays_alloc_failure(void);
void test_alt_queries_dedup_validate_jwt_null_token(void);
void test_alt_queries_validate_jwt_for_auth_alt_null_token(void);
void test_alt_queries_debug_allocations(void);

// Test fixtures
void setUp(void) {
    mock_mhd_reset_all();
    mock_system_reset_all();

    // Set the malloc failure point BEFORE any allocations happen
    // This way, the mock_system_set_malloc_failure() call won't reset our counter
    // For our tests, we'll set this in each test function instead
    // to have precise control

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

// Test alt_queries_deduplicate_and_validate when is_duplicate allocation fails
// mock_system_set_malloc_failure(1): first intercepted calloc fails → is_duplicate alloc
// Covers lines 113-114
void test_alt_queries_dedup_is_duplicate_alloc_failure(void) {
    json_t *queries_array = create_test_queries_array();
    json_t *deduplicated_queries = NULL;
    size_t *mapping_array = NULL;
    bool *is_duplicate = NULL;
    DeduplicationResult dedup_code = DEDUP_OK;

    // Set the failure mode first
    // This will reset the allocation counter to 0
    mock_system_set_malloc_failure(1);

    // Now call our function
    enum MHD_Result result = alt_queries_deduplicate_and_validate(
        NULL, queries_array, "testdb",
        &deduplicated_queries, &mapping_array, &is_duplicate, &dedup_code
    );

    // is_duplicate calloc fails → returns MHD_NO
    TEST_ASSERT_EQUAL(MHD_NO, result);
    TEST_ASSERT_NULL(is_duplicate);

    json_decref(queries_array);
}

// Test alt_queries_deduplicate_and_validate when query_refs allocation fails
// mock_system_set_malloc_failure(2): second intercepted calloc fails → query_refs/params/first_occurrence
// Covers lines 124-128
void test_alt_queries_dedup_query_refs_alloc_failure(void) {
    json_t *queries_array = create_test_queries_array();
    json_t *deduplicated_queries = NULL;
    size_t *mapping_array = NULL;
    bool *is_duplicate = NULL;
    DeduplicationResult dedup_code = DEDUP_OK;

    // Set the failure mode first - this resets the counter
    mock_system_set_malloc_failure(2);

    enum MHD_Result result = alt_queries_deduplicate_and_validate(
        NULL, queries_array, "testdb",
        &deduplicated_queries, &mapping_array, &is_duplicate, &dedup_code
    );

    // query_refs calloc fails → returns MHD_NO, frees is_duplicate
    TEST_ASSERT_EQUAL(MHD_NO, result);

    json_decref(queries_array);
}

// Test alt_queries_deduplicate_and_validate when output mapping_array allocation fails
// mock_system_set_malloc_failure(5): fifth intercepted calloc fails → mapping_array
// Covers lines 197-204
void test_alt_queries_dedup_output_arrays_alloc_failure(void) {
    json_t *queries_array = create_test_queries_array();
    json_t *deduplicated_queries = NULL;
    size_t *mapping_array = NULL;
    bool *is_duplicate = NULL;
    DeduplicationResult dedup_code = DEDUP_OK;

    // Set the failure mode first - this resets the counter
    mock_system_set_malloc_failure(5);

    enum MHD_Result result = alt_queries_deduplicate_and_validate(
        NULL, queries_array, "testdb",
        &deduplicated_queries, &mapping_array, &is_duplicate, &dedup_code
    );

    // mapping_array calloc fails → !*mapping_array → returns MHD_NO
    TEST_ASSERT_EQUAL(MHD_NO, result);

    json_decref(queries_array);
}

void test_alt_queries_debug_allocations(void) {
    json_t *queries_array = create_test_queries_array();
    json_t *deduplicated_queries = NULL;
    size_t *mapping_array = NULL;
    bool *is_duplicate = NULL;
    DeduplicationResult dedup_code = DEDUP_OK;

    // Reset and track allocations
    mock_system_reset_all();
    mock_system_set_malloc_failure(0); // Don't fail

    printf("=== TESTING ALLOCATIONS IN alt_queries_deduplicate_and_validate ===\n");
    printf("Initial allocation count: %d\n", mock_malloc_call_count);

    enum MHD_Result result = alt_queries_deduplicate_and_validate(
        NULL, queries_array, "testdb",
        &deduplicated_queries, &mapping_array, &is_duplicate, &dedup_code
    );

    printf("Final allocation count: %d\n", mock_malloc_call_count);
    printf("Result: %d\n", result);
    printf("is_duplicate: %p\n", (void*)is_duplicate);
    if (deduplicated_queries) {
        printf("deduplicated_queries count: %zu\n", json_array_size(deduplicated_queries));
    }
    if (mapping_array) {
        printf("mapping_array[0]: %zu\n", mapping_array[0]);
    }

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_NOT_NULL(is_duplicate);
    TEST_ASSERT_NOT_NULL(deduplicated_queries);
    TEST_ASSERT_NOT_NULL(mapping_array);

    if (deduplicated_queries) json_decref(deduplicated_queries);
    if (mapping_array) free(mapping_array);
    if (is_duplicate) free(is_duplicate);
    json_decref(queries_array);
}

// Test validate_jwt_for_auth_alt with NULL token (covers validate_jwt_for_auth_alt NULL path)
// This function is defined in alt_queries.c and is called from handle_conduit_alt_queries_request
void test_alt_queries_validate_jwt_for_auth_alt_null_token(void) {
    struct MHD_Connection *mock_connection = (void*)0x123;

    mock_mhd_set_queue_response_result(MHD_YES);

    enum MHD_Result result = validate_jwt_for_auth_alt(mock_connection, NULL);

    // NULL token → sends 400 error with json_false() success → returns MHD_YES (from mock)
    TEST_ASSERT_EQUAL(MHD_YES, result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_alt_queries_debug_allocations);
    RUN_TEST(test_alt_queries_dedup_is_duplicate_alloc_failure);
    RUN_TEST(test_alt_queries_dedup_query_refs_alloc_failure);
    RUN_TEST(test_alt_queries_dedup_output_arrays_alloc_failure);
    RUN_TEST(test_alt_queries_validate_jwt_for_auth_alt_null_token);

    return UNITY_END();
}
