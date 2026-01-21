/*
 * Unity Test File: handle_parameter_processing
 * This file contains unit tests for handle_parameter_processing function
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

// Enable mock for process_parameters
#define USE_MOCK_PROCESS_PARAMETERS

// Mock for api_send_json_response and json_decref
enum MHD_Result mock_api_send_json_response(struct MHD_Connection *connection, json_t* response, unsigned int status);
void mock_json_decref(json_t* json);

// Mock for process_parameters
bool mock_process_parameters(json_t* params_json, ParameterList** param_list,
                           const char* sql_template, DatabaseEngineType engine_type,
                           char** converted_sql, TypedParameter*** ordered_params, size_t* param_count);

// Mock MHD_Connection struct (minimal)
typedef struct MockMHDConnection {
    int dummy;
} MockMHDConnection;

// Mock state
static bool mock_process_result = true;

// Function prototypes
void test_handle_parameter_processing_success(void);
void test_handle_parameter_processing_failure(void);

// Forward declaration for the function being tested
enum MHD_Result handle_parameter_processing(struct MHD_Connection *connection, json_t* params_json,
                                           const DatabaseQueue* db_queue, const QueryCacheEntry* cache_entry,
                                           const char* database, int query_ref,
                                           ParameterList** param_list, char** converted_sql,
                                           TypedParameter*** ordered_params, size_t* param_count,
                                           char** message);

void setUp(void) {
    mock_system_reset_all();
    mock_process_result = true;
}

void tearDown(void) {
    mock_system_reset_all();
    mock_process_result = true;
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

bool mock_process_parameters(json_t* params_json, ParameterList** param_list,
                           const char* sql_template, DatabaseEngineType engine_type,
                           char** converted_sql, TypedParameter*** ordered_params, size_t* param_count) {
    (void)params_json;
    (void)sql_template;
    (void)engine_type;
    // Simulate setting some values
    *param_list = calloc(1, sizeof(ParameterList));
    *converted_sql = strdup("SELECT 1");
    *ordered_params = NULL;
    *param_count = 0;
    return mock_process_result;
}

// Test successful parameter processing
void test_handle_parameter_processing_success(void) {
    // Mock connection
    MockMHDConnection mock_connection = {0};

    // Mock data
    DatabaseQueue db_queue = {.engine_type = DB_ENGINE_SQLITE};
    QueryCacheEntry cache_entry = {.sql_template = (char*)"SELECT 1"};

    json_t* params_json = json_object();

    ParameterList* param_list = NULL;
    char* converted_sql = NULL;
    TypedParameter** ordered_params = NULL;
    size_t param_count = 0;
    char* message = NULL;

    enum MHD_Result result = handle_parameter_processing((struct MHD_Connection*)&mock_connection, params_json,
                                                        &db_queue, &cache_entry, "test_db", 123,
                                                        &param_list, &converted_sql, &ordered_params, &param_count, &message);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_NOT_NULL(param_list);
    TEST_ASSERT_NOT_NULL(converted_sql);

    // Cleanup
    json_decref(params_json);
    free(converted_sql);
    free_parameter_list(param_list);
    if (ordered_params) free(ordered_params);
}

// Test parameter processing failure (NULL db_queue)
void test_handle_parameter_processing_failure(void) {
    // Mock connection
    MockMHDConnection mock_connection = {0};

    // Mock data
    QueryCacheEntry cache_entry = {.sql_template = (char*)"SELECT 1"};

    json_t* params_json = json_object();

    ParameterList* param_list = NULL;
    char* converted_sql = NULL;
    TypedParameter** ordered_params = NULL;
    size_t param_count = 0;
    char* message = NULL;

    enum MHD_Result result = handle_parameter_processing((struct MHD_Connection*)&mock_connection, params_json,
                                                        NULL, &cache_entry, "test_db", 123,
                                                        &param_list, &converted_sql, &ordered_params, &param_count, &message);

    TEST_ASSERT_EQUAL(MHD_NO, result);

    // Cleanup
    json_decref(params_json);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_handle_parameter_processing_success);
    RUN_TEST(test_handle_parameter_processing_failure);

    return UNITY_END();
}