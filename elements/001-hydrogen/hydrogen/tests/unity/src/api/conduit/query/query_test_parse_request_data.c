/*
 * Unity Test: parse_request_data function
 * Tests request data parsing for both GET and POST methods
 */

// Project includes
#include <src/hydrogen.h>
#include <unity.h>

// Include the source header to access the function
#include <src/api/conduit/query/query.h>

// Forward declaration for the static function we want to test
json_t* parse_request_data(struct MHD_Connection* connection, const char* method,
                          const char* upload_data, const size_t* upload_data_size);

// Mock MHD connection for testing
struct MHD_Connection {
    void* dummy;
};

// Test function prototypes
void test_parse_request_data_post_valid_json(void);
void test_parse_request_data_post_invalid_json(void);
void test_parse_request_data_post_null_data(void);
void test_parse_request_data_post_empty_data(void);
void test_parse_request_data_get_valid_params(void);
void test_parse_request_data_get_with_params(void);

void setUp(void) {
    // No setup needed
}

void tearDown(void) {
    // No cleanup needed
}

// Test POST method with valid JSON
void test_parse_request_data_post_valid_json(void) {
    struct MHD_Connection connection;
    const char* upload_data = "{\"query_ref\": 123, \"database\": \"test_db\"}";
    size_t upload_data_size = strlen(upload_data);

    json_t* result = parse_request_data(&connection, "POST", upload_data, &upload_data_size);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(json_is_object(result));

    json_t* query_ref = json_object_get(result, "query_ref");
    json_t* database = json_object_get(result, "database");

    TEST_ASSERT_NOT_NULL(query_ref);
    TEST_ASSERT_NOT_NULL(database);
    TEST_ASSERT_TRUE(json_is_integer(query_ref));
    TEST_ASSERT_TRUE(json_is_string(database));
    TEST_ASSERT_EQUAL(123, json_integer_value(query_ref));
    TEST_ASSERT_EQUAL_STRING("test_db", json_string_value(database));

    json_decref(result);
}

// Test POST method with invalid JSON
void test_parse_request_data_post_invalid_json(void) {
    struct MHD_Connection connection;
    const char* upload_data = "{\"query_ref\": 123, \"database\": "; // Invalid JSON
    size_t upload_data_size = strlen(upload_data);

    json_t* result = parse_request_data(&connection, "POST", upload_data, &upload_data_size);

    TEST_ASSERT_NULL(result);
}

// Test POST method with NULL upload_data
void test_parse_request_data_post_null_data(void) {
    struct MHD_Connection connection;
    size_t upload_data_size = 0;

    json_t* result = parse_request_data(&connection, "POST", NULL, &upload_data_size);

    TEST_ASSERT_NULL(result);
}

// Test POST method with empty upload_data
void test_parse_request_data_post_empty_data(void) {
    struct MHD_Connection connection;
    const char* upload_data = "";
    size_t upload_data_size = 0;

    json_t* result = parse_request_data(&connection, "POST", upload_data, &upload_data_size);

    TEST_ASSERT_NULL(result);
}

// Test GET method with valid parameters
void test_parse_request_data_get_valid_params(void) {
    // This test would require mocking MHD_lookup_connection_value
    // For now, we'll test the basic structure
    struct MHD_Connection connection;

    // Since we can't easily mock MHD functions, we'll test with NULL data
    // which should still return a valid json_t object for GET
    json_t* result = parse_request_data(&connection, "GET", NULL, NULL);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(json_is_object(result));

    json_decref(result);
}

// Test GET method with parameters (would need MHD mocking)
void test_parse_request_data_get_with_params(void) {
    // Note: This test would require extensive MHD mocking
    // For now, we verify the function doesn't crash
    struct MHD_Connection connection;

    json_t* result = parse_request_data(&connection, "GET", NULL, NULL);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(json_is_object(result));

    json_decref(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_parse_request_data_post_valid_json);
    RUN_TEST(test_parse_request_data_post_invalid_json);
    RUN_TEST(test_parse_request_data_post_null_data);
    RUN_TEST(test_parse_request_data_post_empty_data);
    RUN_TEST(test_parse_request_data_get_valid_params);
    RUN_TEST(test_parse_request_data_get_with_params);

    return UNITY_END();
}