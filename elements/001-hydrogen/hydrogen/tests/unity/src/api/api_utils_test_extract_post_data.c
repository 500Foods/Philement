/*
 * Unity Test File: API Utils api_extract_post_data Function Tests
 * This file contains unit tests for the api_extract_post_data function in api_utils.c
 *
 * api_extract_post_data creates a JSON object and populates it by iterating
 * over POST form data (MHD_POSTDATA_KIND) from the MHD connection via
 * MHD_get_connection_values. The post_data_iterator callback URL-decodes
 * each value before storing it.
 *
 * Target coverage: Lines 242-249 in api_utils.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Enable mock for MHD functions
#define USE_MOCK_LIBMICROHTTPD
#include <unity/mocks/mock_libmicrohttpd.h>

// Include necessary headers for the module being tested
#include <src/api/api_utils.h>

void setUp(void) {
    // Reset mocks before each test
    mock_mhd_reset_all();
}

void tearDown(void) {
    // Clean up after each test
    mock_mhd_reset_all();
}

// Test functions
void test_api_extract_post_data_returns_empty_object_when_no_data(void);
void test_api_extract_post_data_single_value(void);
void test_api_extract_post_data_multiple_values(void);
void test_api_extract_post_data_url_decoded(void);
void test_api_extract_post_data_plus_decoded(void);
void test_api_extract_post_data_ignores_query_values(void);

// Test that api_extract_post_data returns an empty JSON object when no
// POST data is present (mock has no POSTDATA_KIND entries)
void test_api_extract_post_data_returns_empty_object_when_no_data(void) {
    struct MHD_Connection *connection = (struct MHD_Connection *)0x1234;

    json_t *result = api_extract_post_data(connection);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(json_is_object(result));
    TEST_ASSERT_EQUAL(0, json_object_size(result));

    json_decref(result);
}

// Test that api_extract_post_data extracts a single key-value pair from
// POST form data
void test_api_extract_post_data_single_value(void) {
    struct MHD_Connection *connection = (struct MHD_Connection *)0x1234;

    mock_mhd_add_value(MHD_POSTDATA_KIND, "username", "john");

    json_t *result = api_extract_post_data(connection);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(1, json_object_size(result));

    json_t *value = json_object_get(result, "username");
    TEST_ASSERT_NOT_NULL(value);
    TEST_ASSERT_TRUE(json_is_string(value));
    TEST_ASSERT_EQUAL_STRING("john", json_string_value(value));

    json_decref(result);
}

// Test that api_extract_post_data extracts multiple key-value pairs
void test_api_extract_post_data_multiple_values(void) {
    struct MHD_Connection *connection = (struct MHD_Connection *)0x1234;

    mock_mhd_add_value(MHD_POSTDATA_KIND, "username", "john");
    mock_mhd_add_value(MHD_POSTDATA_KIND, "password", "secret");
    mock_mhd_add_value(MHD_POSTDATA_KIND, "action", "login");

    json_t *result = api_extract_post_data(connection);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(3, json_object_size(result));
    TEST_ASSERT_EQUAL_STRING("john",
        json_string_value(json_object_get(result, "username")));
    TEST_ASSERT_EQUAL_STRING("secret",
        json_string_value(json_object_get(result, "password")));
    TEST_ASSERT_EQUAL_STRING("login",
        json_string_value(json_object_get(result, "action")));

    json_decref(result);
}

// Test that URL-encoded values in POST data are decoded by the
// post_data_iterator callback
void test_api_extract_post_data_url_decoded(void) {
    struct MHD_Connection *connection = (struct MHD_Connection *)0x1234;

    mock_mhd_add_value(MHD_POSTDATA_KIND, "email", "user%40example.com");

    json_t *result = api_extract_post_data(connection);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("user@example.com",
        json_string_value(json_object_get(result, "email")));

    json_decref(result);
}

// Test that plus signs in POST data values are decoded to spaces
void test_api_extract_post_data_plus_decoded(void) {
    struct MHD_Connection *connection = (struct MHD_Connection *)0x1234;

    mock_mhd_add_value(MHD_POSTDATA_KIND, "message", "hello+world");

    json_t *result = api_extract_post_data(connection);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_STRING("hello world",
        json_string_value(json_object_get(result, "message")));

    json_decref(result);
}

// Test that api_extract_post_data only extracts POSTDATA_KIND values and
// ignores GET_ARGUMENT_KIND (query parameter) values
void test_api_extract_post_data_ignores_query_values(void) {
    struct MHD_Connection *connection = (struct MHD_Connection *)0x1234;

    mock_mhd_add_value(MHD_GET_ARGUMENT_KIND, "query_param", "should_not_appear");
    mock_mhd_add_value(MHD_POSTDATA_KIND, "post_param", "should_appear");

    json_t *result = api_extract_post_data(connection);

    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(1, json_object_size(result));
    TEST_ASSERT_NULL(json_object_get(result, "query_param"));

    json_t *post_val = json_object_get(result, "post_param");
    TEST_ASSERT_NOT_NULL(post_val);
    TEST_ASSERT_EQUAL_STRING("should_appear", json_string_value(post_val));

    json_decref(result);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_api_extract_post_data_returns_empty_object_when_no_data);
    RUN_TEST(test_api_extract_post_data_single_value);
    RUN_TEST(test_api_extract_post_data_multiple_values);
    RUN_TEST(test_api_extract_post_data_url_decoded);
    RUN_TEST(test_api_extract_post_data_plus_decoded);
    RUN_TEST(test_api_extract_post_data_ignores_query_values);

    return UNITY_END();
}
