/*
 * Unity Test File: Alt Queries Database Lookup by Connection Name
 * This file contains unit tests for the database lookup by connection_name
 * in src/api/conduit/alt_queries/alt_queries.c
 *
 * Tests the path where database is found by connection_name instead of db name.
 *
 * CHANGELOG:
 * 2026-02-18: Initial creation
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
void test_alt_queries_dedup_database_lookup_by_connection_name(void);

// Test fixtures
void setUp(void) {
    mock_mhd_reset_all();

    // Initialize mock app config
    app_config = calloc(1, sizeof(AppConfig));
    TEST_ASSERT_NOT_NULL(app_config);

    // Initialize databases config with a connection that has a different connection_name
    app_config->databases.connection_count = 1;

    // Set up test database connection with connection_name
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

// Test database lookup by connection_name (lines 97-98)
void test_alt_queries_dedup_database_lookup_by_connection_name(void) {
    json_t *queries_array = json_array();

    // Add a query
    json_t *query = json_object();
    json_object_set_new(query, "query_ref", json_integer(1));
    json_array_append_new(queries_array, query);

    json_t *deduplicated_queries = NULL;
    size_t *mapping_array = NULL;
    bool *is_duplicate = NULL;

    // Use connection_name instead of db_name - this should still find the database
    DeduplicationResult dedup_code;
    enum MHD_Result result = alt_queries_deduplicate_and_validate(
        NULL, queries_array, "myconnection", &deduplicated_queries, 
        &mapping_array, &is_duplicate, &dedup_code
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

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_alt_queries_dedup_database_lookup_by_connection_name);

    return UNITY_END();
}
