/*
 * Unity Tests for Conduit Query Endpoint
 *
 * Tests the REST API endpoint for executing database queries by reference.
 */

#include "../../../../../src/hydrogen.h"
#include "unity.h"
#include "../../../../../src/api/conduit/query/query.h"

// Test function prototypes
void test_conduit_query_parse_request_valid(void);
void test_conduit_query_parse_request_invalid_json(void);
void test_conduit_query_parse_request_missing_query_ref(void);
void test_conduit_query_parse_request_with_database(void);
void test_conduit_query_parse_request_with_params(void);
void test_conduit_query_execute_placeholder(void);
void test_conduit_query_free_request(void);
void test_conduit_query_free_response(void);

// Test fixtures
static ConduitQueryRequest* test_request = NULL;
static ConduitQueryResponse* test_response = NULL;

void setUp(void) {
    test_request = NULL;
    test_response = NULL;
}

void tearDown(void) {
    if (test_request) {
        conduit_query_free_request(test_request);
        test_request = NULL;
    }
    if (test_response) {
        conduit_query_free_response(test_response);
        test_response = NULL;
    }
}

// Test parsing valid request
void test_conduit_query_parse_request_valid(void) {
    const char* json_str = "{\"query_ref\": 123, \"database\": \"testdb\"}";

    test_request = conduit_query_parse_request(json_str);

    TEST_ASSERT_NOT_NULL(test_request);
    TEST_ASSERT_EQUAL(123, test_request->query_ref);
    TEST_ASSERT_EQUAL_STRING("testdb", test_request->database);
    TEST_ASSERT_NULL(test_request->params);
}

// Test parsing invalid JSON
void test_conduit_query_parse_request_invalid_json(void) {
    const char* json_str = "{invalid json}";

    test_request = conduit_query_parse_request(json_str);

    TEST_ASSERT_NULL(test_request);
}

// Test parsing request missing query_ref
void test_conduit_query_parse_request_missing_query_ref(void) {
    const char* json_str = "{\"database\": \"testdb\"}";

    test_request = conduit_query_parse_request(json_str);

    TEST_ASSERT_NULL(test_request);
}

// Test parsing request with database field
void test_conduit_query_parse_request_with_database(void) {
    const char* json_str = "{\"query_ref\": 456, \"database\": \"production\"}";

    test_request = conduit_query_parse_request(json_str);

    TEST_ASSERT_NOT_NULL(test_request);
    TEST_ASSERT_EQUAL(456, test_request->query_ref);
    TEST_ASSERT_EQUAL_STRING("production", test_request->database);
}

// Test parsing request with params
void test_conduit_query_parse_request_with_params(void) {
    const char* json_str = "{\"query_ref\": 789, \"params\": {\"INTEGER\": {\"userId\": 123}}}";

    test_request = conduit_query_parse_request(json_str);

    TEST_ASSERT_NOT_NULL(test_request);
    TEST_ASSERT_EQUAL(789, test_request->query_ref);
    TEST_ASSERT_NOT_NULL(test_request->params);
    TEST_ASSERT_TRUE(json_is_object(test_request->params));

    // Verify params structure
    json_t* integer_params = json_object_get(test_request->params, "INTEGER");
    TEST_ASSERT_NOT_NULL(integer_params);
    TEST_ASSERT_TRUE(json_is_object(integer_params));

    json_t* user_id = json_object_get(integer_params, "userId");
    TEST_ASSERT_NOT_NULL(user_id);
    TEST_ASSERT_TRUE(json_is_integer(user_id));
    TEST_ASSERT_EQUAL(123, json_integer_value(user_id));
}

// Test execute function (placeholder)
void test_conduit_query_execute_placeholder(void) {
    // Create a test request
    test_request = (ConduitQueryRequest*)calloc(1, sizeof(ConduitQueryRequest));
    TEST_ASSERT_NOT_NULL(test_request);
    test_request->query_ref = 999;

    test_response = conduit_query_execute(test_request);

    TEST_ASSERT_NOT_NULL(test_response);
    TEST_ASSERT_FALSE(test_response->success);
    TEST_ASSERT_EQUAL(999, test_response->query_ref);
    TEST_ASSERT_NOT_NULL(test_response->error_message);
    TEST_ASSERT_EQUAL_STRING("Conduit query execution not yet implemented", test_response->error_message);
}

// Test freeing request
void test_conduit_query_free_request(void) {
    test_request = (ConduitQueryRequest*)calloc(1, sizeof(ConduitQueryRequest));
    TEST_ASSERT_NOT_NULL(test_request);
    test_request->query_ref = 123;
    test_request->database = strdup("testdb");
    test_request->params = json_object();  // Empty object

    // Free should not crash
    conduit_query_free_request(test_request);
    test_request = NULL;  // Prevent double free in tearDown
}

// Test freeing response
void test_conduit_query_free_response(void) {
    test_response = (ConduitQueryResponse*)calloc(1, sizeof(ConduitQueryResponse));
    TEST_ASSERT_NOT_NULL(test_response);
    test_response->success = false;
    test_response->query_ref = 456;
    test_response->description = strdup("Test query");
    test_response->error_message = strdup("Test error");
    test_response->rows = json_array();  // Empty array
    test_response->queue_used = strdup("fast");

    // Free should not crash
    conduit_query_free_response(test_response);
    test_response = NULL;  // Prevent double free in tearDown
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_conduit_query_parse_request_valid);
    RUN_TEST(test_conduit_query_parse_request_invalid_json);
    RUN_TEST(test_conduit_query_parse_request_missing_query_ref);
    RUN_TEST(test_conduit_query_parse_request_with_database);
    RUN_TEST(test_conduit_query_parse_request_with_params);
    RUN_TEST(test_conduit_query_execute_placeholder);
    RUN_TEST(test_conduit_query_free_request);
    RUN_TEST(test_conduit_query_free_response);

    return UNITY_END();
}