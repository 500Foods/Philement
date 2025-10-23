/*
 * Unity Test File: handle_pending_registration
 * This file contains unit tests for handle_pending_registration function
 * in src/api/conduit/query/query.c
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include source header
#include <src/api/conduit/query/query.h>

// Enable mocks for testing
#define USE_MOCK_SYSTEM
#include <unity/mocks/mock_system.h>

// Enable mock for pending_result_register
#define USE_MOCK_PENDING_RESULT_REGISTER

// Mock for api_send_json_response and json_decref
enum MHD_Result mock_api_send_json_response(struct MHD_Connection *connection, json_t* response, unsigned int status);
void mock_json_decref(json_t* json);

// Mock for pending_result_register
PendingQueryResult* mock_pending_result_register(PendingResultManager* manager, char* query_id, int timeout_seconds);

// Mock for free_parameter_list
void mock_free_parameter_list(ParameterList* param_list);

// Mock MHD_Connection struct (minimal)
typedef struct MockMHDConnection {
    int dummy;
} MockMHDConnection;

// Mock state
static PendingQueryResult* mock_pending_result = NULL;

// Function prototypes
void test_handle_pending_registration_success(void);
void test_handle_pending_registration_failure(void);

// Forward declaration for the function being tested
enum MHD_Result handle_pending_registration(struct MHD_Connection *connection, const char* database,
                                           int query_ref, char* query_id, ParameterList* param_list,
                                           char* converted_sql, TypedParameter** ordered_params,
                                           const QueryCacheEntry* cache_entry, PendingQueryResult** pending);

void setUp(void) {
    mock_system_reset_all();
    mock_pending_result = NULL;
}

void tearDown(void) {
    mock_system_reset_all();
    mock_pending_result = NULL;
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

PendingQueryResult* mock_pending_result_register(PendingResultManager* manager, char* query_id, int timeout_seconds) {
    (void)manager;
    (void)query_id;
    (void)timeout_seconds;
    return mock_pending_result;
}

void mock_free_parameter_list(ParameterList* param_list) {
    (void)param_list;
}

// Test successful pending registration
void test_handle_pending_registration_success(void) {
    // Mock connection
    MockMHDConnection mock_connection = {0};

    // Set up mock to succeed
    PendingQueryResult mock_pending = {0};
    mock_pending_result = &mock_pending;

    QueryCacheEntry cache_entry = {.timeout_seconds = 30};
    ParameterList* param_list = calloc(1, sizeof(ParameterList));
    char* converted_sql = strdup("SELECT 1");
    TypedParameter** ordered_params = NULL;
    char* query_id = strdup("test_id");
    PendingQueryResult* pending = NULL;

    enum MHD_Result result = handle_pending_registration((struct MHD_Connection*)&mock_connection, "test_db", 123,
                                                        query_id, param_list, converted_sql, ordered_params,
                                                        &cache_entry, &pending);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_NOT_NULL(pending);

    // Cleanup
    free(query_id);
    free(converted_sql);
    free_parameter_list(param_list);
}

// Test pending registration failure
void test_handle_pending_registration_failure(void) {
    // Mock connection
    MockMHDConnection mock_connection = {0};

    // Set up mock to fail
    mock_pending_result = NULL;

    QueryCacheEntry cache_entry = {.timeout_seconds = 30};
    ParameterList* param_list = calloc(1, sizeof(ParameterList));
    char* converted_sql = strdup("SELECT 1");
    TypedParameter** ordered_params = NULL;
    char* query_id = strdup("test_id");
    PendingQueryResult* pending = NULL;

    enum MHD_Result result = handle_pending_registration((struct MHD_Connection*)&mock_connection, "test_db", 123,
                                                        query_id, param_list, converted_sql, ordered_params,
                                                        &cache_entry, &pending);

    TEST_ASSERT_EQUAL(MHD_NO, result);
    TEST_ASSERT_NULL(pending);

    // Cleanup
    free(query_id);
    free(converted_sql);
    free_parameter_list(param_list);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_handle_pending_registration_success);
    if (0) RUN_TEST(test_handle_pending_registration_failure);

    return UNITY_END();
}