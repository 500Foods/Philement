/*
 * Unity Test File: handle_queue_selection
 * This file contains unit tests for handle_queue_selection function
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

// Enable mock for select_query_queue
#define USE_MOCK_SELECT_QUERY_QUEUE

// Mock for api_send_json_response and json_decref
enum MHD_Result mock_api_send_json_response(struct MHD_Connection *connection, json_t* response, unsigned int status);
void mock_json_decref(json_t* json);

// Mock for select_query_queue
DatabaseQueue* mock_select_query_queue(const char* database, const char* queue_type);

// Mock for free_parameter_list
void mock_free_parameter_list(ParameterList* param_list);

// Mock MHD_Connection struct (minimal)
typedef struct MockMHDConnection {
    int dummy;
} MockMHDConnection;

// Mock state
static DatabaseQueue* mock_queue_result = NULL;

// Function prototypes
void test_handle_queue_selection_success(void);
void test_handle_queue_selection_failure(void);

// Forward declaration for the function being tested
enum MHD_Result handle_queue_selection(struct MHD_Connection *connection, const char* database,
                                      int query_ref, const QueryCacheEntry* cache_entry,
                                      ParameterList* param_list, char* converted_sql,
                                      TypedParameter** ordered_params,
                                      DatabaseQueue** selected_queue);

void setUp(void) {
    mock_system_reset_all();
    mock_queue_result = NULL;
}

void tearDown(void) {
    mock_system_reset_all();
    mock_queue_result = NULL;
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

DatabaseQueue* mock_select_query_queue(const char* database, const char* queue_type) {
    (void)database;
    (void)queue_type;
    return mock_queue_result;
}

void mock_free_parameter_list(ParameterList* param_list) {
    (void)param_list;
}

// Test queue selection failure (no queue available)
void test_handle_queue_selection_failure(void) {
    // Mock connection
    MockMHDConnection mock_connection = {0};

    // Set up mock to fail (no queue available)
    mock_queue_result = NULL;

    QueryCacheEntry cache_entry = {.queue_type = (char*)"read"};
    ParameterList* param_list = calloc(1, sizeof(ParameterList));
    char* converted_sql = strdup("SELECT 1");
    TypedParameter** ordered_params = NULL;

    DatabaseQueue* selected_queue = NULL;

    enum MHD_Result result = handle_queue_selection((struct MHD_Connection*)&mock_connection, "test_db", 123,
                                                   &cache_entry, param_list, converted_sql, ordered_params,
                                                   &selected_queue);

    TEST_ASSERT_EQUAL(MHD_NO, result);
    TEST_ASSERT_NULL(selected_queue);

    // Note: handle_queue_selection frees param_list and converted_sql on failure, so no cleanup needed
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_handle_queue_selection_failure);

    return UNITY_END();
}