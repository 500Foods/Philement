/*
 * Unity Test File: handle_query_submission
 * This file contains unit tests for handle_query_submission function
 * in src/api/conduit/query/query.c
 */

// Project includes
#include <src/hydrogen.h>
#include "unity.h"

// Include source header
#include <src/api/conduit/query/query.h>

// Enable mocks for testing
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

// Enable mock for prepare_and_submit_query
#define USE_MOCK_PREPARE_AND_SUBMIT_QUERY
#include <unity/mocks/mock_prepare_and_submit_query.h>



// Mock MHD_Connection struct (minimal)
typedef struct MockMHDConnection {
    int dummy;
} MockMHDConnection;

// Function prototypes
void test_handle_query_submission_success(void);
void test_handle_query_submission_failure(void);

// Forward declaration for the function being tested
enum MHD_Result handle_query_submission(struct MHD_Connection *connection, const char* database,
                                        int query_ref, DatabaseQueue* selected_queue, char* query_id,
                                        char* converted_sql, ParameterList* param_list,
                                        TypedParameter** ordered_params, size_t param_count,
                                        const QueryCacheEntry* cache_entry);

void setUp(void) {
    mock_system_reset_all();
    mock_prepare_and_submit_query_reset();
}

void tearDown(void) {
    mock_system_reset_all();
    mock_prepare_and_submit_query_reset();
}

// Test successful query submission
void test_handle_query_submission_success(void) {
    // Mock connection
    MockMHDConnection mock_connection = {0};

    // Set up mock to succeed
    mock_prepare_and_submit_query_set_result(true);

    DatabaseQueue selected_queue = {.queue_type = (char*)"test"};
    QueryCacheEntry cache_entry = {.queue_type = (char*)"read", .sql_template = (char*)"SELECT 1"};
    ParameterList* param_list = calloc(1, sizeof(ParameterList));
    char* converted_sql = strdup("SELECT 1");
    TypedParameter** ordered_params = NULL;
    char* query_id = strdup("test_id");

    enum MHD_Result result = handle_query_submission((struct MHD_Connection*)&mock_connection, "test_db", 123,
                                                     &selected_queue, query_id, converted_sql, param_list,
                                                     ordered_params, 0, &cache_entry);

    TEST_ASSERT_EQUAL(MHD_YES, result);

    // Cleanup - query_id, converted_sql, and param_list are freed by handle_query_submission on success
}

// Test query submission failure
void test_handle_query_submission_failure(void) {
    // Mock connection
    MockMHDConnection mock_connection = {0};

    // Set up mock to fail
    mock_prepare_and_submit_query_set_result(false);

    DatabaseQueue selected_queue = {.queue_type = (char*)"test"};
    QueryCacheEntry cache_entry = {.queue_type = (char*)"read", .sql_template = (char*)"SELECT 1"};
    ParameterList* param_list = calloc(1, sizeof(ParameterList));
    char* converted_sql = strdup("SELECT 1");
    TypedParameter** ordered_params = NULL;
    char* query_id = strdup("test_id");

    enum MHD_Result result = handle_query_submission((struct MHD_Connection*)&mock_connection, "test_db", 123,
                                                     &selected_queue, query_id, converted_sql, param_list,
                                                     ordered_params, 0, &cache_entry);

    TEST_ASSERT_EQUAL(MHD_NO, result);

    // Cleanup - query_id, converted_sql, and param_list are freed by handle_query_submission on failure
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_handle_query_submission_success);
    RUN_TEST(test_handle_query_submission_failure);

    return UNITY_END();
}