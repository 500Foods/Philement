/*
 * Unity Test File: handle_response_building
 * This file contains unit tests for handle_response_building function
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

// Enable mock for build_response_json and api_send_json_response
#define USE_MOCK_BUILD_RESPONSE_JSON
#define USE_MOCK_API_SEND_JSON_RESPONSE

// Mock for build_response_json
json_t* mock_build_response_json(int query_ref, const char* database, const QueryCacheEntry* cache_entry,
                                const DatabaseQueue* selected_queue, PendingQueryResult* pending, const char* message);

// Mock for api_send_json_response
enum MHD_Result mock_api_send_json_response(struct MHD_Connection *connection, json_t* response, unsigned int status);

// Mock for json_decref
void mock_json_decref(json_t* json);

// Mock for free_parameter_list
void mock_free_parameter_list(ParameterList* param_list);

// Mock MHD_Connection struct (minimal)
typedef struct MockMHDConnection {
    int dummy;
} MockMHDConnection;

// Mock state
static json_t* mock_response_result = NULL;

// Function prototypes
void test_handle_response_building_success(void);

// Forward declaration for the function being tested
enum MHD_Result handle_response_building(struct MHD_Connection *connection, int query_ref,
                                        const char* database, const QueryCacheEntry* cache_entry,
                                        const DatabaseQueue* selected_queue, PendingQueryResult* pending,
                                        char* query_id, char* converted_sql, ParameterList* param_list,
                                        TypedParameter** ordered_params, const char* message);

void setUp(void) {
    mock_system_reset_all();
    mock_response_result = NULL;
}

void tearDown(void) {
    mock_system_reset_all();
    if (mock_response_result) {
        json_decref(mock_response_result);
        mock_response_result = NULL;
    }
}

// Mock implementations
json_t* mock_build_response_json(int query_ref, const char* database, const QueryCacheEntry* cache_entry,
                                const DatabaseQueue* selected_queue, PendingQueryResult* pending, const char* message) {
    (void)query_ref;
    (void)database;
    (void)cache_entry;
    (void)selected_queue;
    (void)pending;
    (void)message;

    if (mock_response_result) {
        return json_copy(mock_response_result);
    }

    // Return a default success response
    json_t* response = json_object();
    json_object_set_new(response, "success", json_true());
    return response;
}

enum MHD_Result mock_api_send_json_response(struct MHD_Connection *connection, json_t* response, unsigned int status) {
    (void)connection;
    (void)response;
    (void)status;
    return MHD_YES;
}

void mock_json_decref(json_t* json) {
    (void)json;
}

void mock_free_parameter_list(ParameterList* param_list) {
    (void)param_list;
}

// Test successful response building
void test_handle_response_building_success(void) {
    // Mock connection
    MockMHDConnection mock_connection = {0};

    // Set up mock response
    mock_response_result = json_object();
    json_object_set_new(mock_response_result, "success", json_true());

    DatabaseQueue selected_queue = {.queue_type = (char*)"test"};
    QueryCacheEntry cache_entry = {.queue_type = (char*)"read"};
    PendingQueryResult pending = {0};
    ParameterList* param_list = calloc(1, sizeof(ParameterList));
    char* converted_sql = strdup("SELECT 1");
    TypedParameter** ordered_params = NULL;
    char* query_id = strdup("test_id");

    enum MHD_Result result = handle_response_building((struct MHD_Connection*)&mock_connection, 123, "test_db",
                                                     &cache_entry, &selected_queue, &pending, query_id,
                                                     converted_sql, param_list, ordered_params, NULL);

    TEST_ASSERT_EQUAL(MHD_YES, result);

    // Cleanup - query_id, converted_sql, and param_list are freed by handle_response_building
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_handle_response_building_success);

    return UNITY_END();
}