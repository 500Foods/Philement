/*
 * Unity Test File: handle_query_id_generation
 * This file contains unit tests for handle_query_id_generation function
 * in src/api/conduit/query/query.c
 */

// Include mock header
#include <unity/mocks/mock_generate_query_id.h>

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include source header
#include <src/api/conduit/query/query.h>

// Mock for api_send_json_response and json_decref
enum MHD_Result mock_api_send_json_response(struct MHD_Connection *connection, json_t* response, unsigned int status);
void mock_json_decref(json_t* json);

// Mock for generate_query_id (duplicate removed)

// Mock for free_parameter_list
void mock_free_parameter_list(ParameterList* param_list);

// Mock MHD_Connection struct (minimal)
typedef struct MockMHDConnection {
    int dummy;
} MockMHDConnection;

// Function prototypes
void test_handle_query_id_generation_success(void);
void test_handle_query_id_generation_failure(void);

// Forward declaration for the function being tested
enum MHD_Result handle_query_id_generation(struct MHD_Connection *connection, const char* database,
                                          int query_ref, ParameterList* param_list, char* converted_sql,
                                          TypedParameter** ordered_params, char** query_id);

void setUp(void) {
    mock_generate_query_id_reset();
}

void tearDown(void) {
    mock_generate_query_id_reset();
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

void mock_free_parameter_list(ParameterList* param_list) {
    (void)param_list;
}

// Test successful query ID generation
void test_handle_query_id_generation_success(void) {
    // Mock connection
    MockMHDConnection mock_connection = {0};

    // Set up mock to succeed
    mock_generate_query_id_set_result("test_query_id_123");

    ParameterList* param_list = calloc(1, sizeof(ParameterList));
    char* converted_sql = strdup("SELECT 1");
    TypedParameter** ordered_params = NULL;
    char* query_id = NULL;

    enum MHD_Result result = handle_query_id_generation((struct MHD_Connection*)&mock_connection, "test_db", 123,
                                                        param_list, converted_sql, ordered_params, &query_id);

    TEST_ASSERT_EQUAL(MHD_YES, result);
    TEST_ASSERT_NOT_NULL(query_id);
    TEST_ASSERT_EQUAL_STRING("test_query_id_123", query_id);

    // Cleanup
    free(query_id);
    free(converted_sql);
    free_parameter_list(param_list);
}

// Test query ID generation failure
void test_handle_query_id_generation_failure(void) {
    // Mock connection
    MockMHDConnection mock_connection = {0};

    // Set up mock to fail
    mock_generate_query_id_set_result(NULL);

    ParameterList* param_list = calloc(1, sizeof(ParameterList));
    char* converted_sql = strdup("SELECT 1");
    TypedParameter** ordered_params = NULL;
    char* query_id = NULL;

    enum MHD_Result result = handle_query_id_generation((struct MHD_Connection*)&mock_connection, "test_db", 123,
                                                        param_list, converted_sql, ordered_params, &query_id);

    TEST_ASSERT_EQUAL(MHD_NO, result);
    TEST_ASSERT_NULL(query_id);

    // Note: Resources are freed by the function on failure, so no cleanup needed
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_handle_query_id_generation_success);
    RUN_TEST(test_handle_query_id_generation_failure);

    return UNITY_END();
}