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

// Mock for api_send_json_response and json_decref
enum MHD_Result mock_api_send_json_response(struct MHD_Connection *connection, json_t* response, unsigned int status);
void mock_json_decref(json_t* json);

// Mock for prepare_and_submit_query
bool mock_prepare_and_submit_query(DatabaseQueue* selected_queue, const char* query_id,
                                  const char* sql_template, TypedParameter** ordered_params,
                                  size_t param_count, const QueryCacheEntry* cache_entry);

// Mock for free_parameter_list
void mock_free_parameter_list(ParameterList* param_list);

// Mock MHD_Connection struct (minimal)
typedef struct MockMHDConnection {
    int dummy;
} MockMHDConnection;

// Mock state
static bool mock_submit_result = true;

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
    mock_submit_result = true;
}

void tearDown(void) {
    mock_system_reset_all();
    mock_submit_result = true;
}

// Mock implementations
enum MHD_Result mock_api_send_json_response(struct MHD_Connection *connection, json_t* response, unsigned int status) {
    (void)connection;
    (void)response;
    (void)status;
    return MHD_YES;
}

void mock_json_decref(json_t* json) {
    (void)json;
}

bool mock_prepare_and_submit_query(DatabaseQueue* selected_queue, const char* query_id,
                                  const char* sql_template, TypedParameter** ordered_params,
                                  size_t param_count, const QueryCacheEntry* cache_entry) {
    (void)selected_queue;
    (void)query_id;
    (void)sql_template;
    (void)ordered_params;
    (void)param_count;
    (void)cache_entry;
    return mock_submit_result;
}

void mock_free_parameter_list(ParameterList* param_list) {
    (void)param_list;
}

// Test successful query submission
void test_handle_query_submission_success(void) {
    // Mock connection
    MockMHDConnection mock_connection = {0};

    // Set up mock to succeed
    mock_submit_result = true;

    DatabaseQueue selected_queue = {.queue_type = (char*)"test"};
    QueryCacheEntry cache_entry = {.queue_type = (char*)"read"};
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
    mock_submit_result = false;

    DatabaseQueue selected_queue = {.queue_type = (char*)"test"};
    QueryCacheEntry cache_entry = {.queue_type = (char*)"read"};
    ParameterList* param_list = calloc(1, sizeof(ParameterList));
    char* converted_sql = strdup("SELECT 1");
    TypedParameter** ordered_params = NULL;
    char* query_id = strdup("test_id");

    enum MHD_Result result = handle_query_submission((struct MHD_Connection*)&mock_connection, "test_db", 123,
                                                    &selected_queue, query_id, converted_sql, param_list,
                                                    ordered_params, 0, &cache_entry);

    TEST_ASSERT_EQUAL(MHD_YES, result);

    // Cleanup - query_id, converted_sql, and param_list are freed by handle_query_submission on failure
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_handle_query_submission_success);
    RUN_TEST(test_handle_query_submission_failure);

    return UNITY_END();
}