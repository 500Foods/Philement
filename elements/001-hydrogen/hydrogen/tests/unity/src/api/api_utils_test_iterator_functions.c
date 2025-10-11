/*
 * Unity Test File: API Utils Iterator Functions Tests
 * This file contains unit tests for the iterator functions in api_utils.c
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/api/api_utils.h>

void setUp(void) {
    // No setup needed for these pure functions
}

void tearDown(void) {
    // No teardown needed for these pure functions
}

// Function declarations
void test_query_param_iterator_valid_key_value(void);
void test_query_param_iterator_null_key(void);
void test_query_param_iterator_null_value(void);
void test_query_param_iterator_null_key_and_value(void);
void test_query_param_iterator_invalid_url_encoding(void);
void test_post_data_iterator_valid_key_value(void);
void test_post_data_iterator_null_key(void);
void test_post_data_iterator_null_value(void);
void test_post_data_iterator_null_key_and_value(void);
void test_post_data_iterator_invalid_url_encoding(void);
void test_post_data_iterator_plus_encoding(void);

// Test query_param_iterator with valid key and value
void test_query_param_iterator_valid_key_value(void) {
    json_t *params = json_object();
    TEST_ASSERT_NOT_NULL(params);

    const char *key = "test_key";
    const char *value = "test%20value";

    enum MHD_Result result = query_param_iterator((void *)params, MHD_GET_ARGUMENT_KIND, key, value);

    TEST_ASSERT_EQUAL(MHD_YES, result);

    // Check that the parameter was added and decoded
    json_t *stored_value = json_object_get(params, key);
    TEST_ASSERT_NOT_NULL(stored_value);
    TEST_ASSERT_TRUE(json_is_string(stored_value));
    TEST_ASSERT_EQUAL_STRING("test value", json_string_value(stored_value));

    json_decref(params);
}

// Test query_param_iterator with NULL key
void test_query_param_iterator_null_key(void) {
    json_t *params = json_object();
    TEST_ASSERT_NOT_NULL(params);

    const char *value = "test_value";

    enum MHD_Result result = query_param_iterator((void *)params, MHD_GET_ARGUMENT_KIND, NULL, value);

    TEST_ASSERT_EQUAL(MHD_YES, result);

    // Should not add anything to the object
    TEST_ASSERT_EQUAL(0, json_object_size(params));

    json_decref(params);
}

// Test query_param_iterator with NULL value
void test_query_param_iterator_null_value(void) {
    json_t *params = json_object();
    TEST_ASSERT_NOT_NULL(params);

    const char *key = "test_key";

    enum MHD_Result result = query_param_iterator((void *)params, MHD_GET_ARGUMENT_KIND, key, NULL);

    TEST_ASSERT_EQUAL(MHD_YES, result);

    // Should not add anything to the object
    TEST_ASSERT_EQUAL(0, json_object_size(params));

    json_decref(params);
}

// Test query_param_iterator with both NULL key and value
void test_query_param_iterator_null_key_and_value(void) {
    json_t *params = json_object();
    TEST_ASSERT_NOT_NULL(params);

    enum MHD_Result result = query_param_iterator((void *)params, MHD_GET_ARGUMENT_KIND, NULL, NULL);

    TEST_ASSERT_EQUAL(MHD_YES, result);

    // Should not add anything to the object
    TEST_ASSERT_EQUAL(0, json_object_size(params));

    json_decref(params);
}

// Test query_param_iterator with URL-encoded value that fails to decode
void test_query_param_iterator_invalid_url_encoding(void) {
    json_t *params = json_object();
    TEST_ASSERT_NOT_NULL(params);

    const char *key = "test_key";
    const char *value = "invalid%ZZencoding";

    enum MHD_Result result = query_param_iterator((void *)params, MHD_GET_ARGUMENT_KIND, key, value);

    TEST_ASSERT_EQUAL(MHD_YES, result);

    // Should still add the value as-is if decoding fails
    json_t *stored_value = json_object_get(params, key);
    TEST_ASSERT_NOT_NULL(stored_value);
    TEST_ASSERT_TRUE(json_is_string(stored_value));
    TEST_ASSERT_EQUAL_STRING(value, json_string_value(stored_value));

    json_decref(params);
}

// Test post_data_iterator with valid key and value
void test_post_data_iterator_valid_key_value(void) {
    json_t *post_data = json_object();
    TEST_ASSERT_NOT_NULL(post_data);

    const char *key = "username";
    const char *value = "user%2Bname%40example.com";

    enum MHD_Result result = post_data_iterator((void *)post_data, MHD_POSTDATA_KIND, key, value);

    TEST_ASSERT_EQUAL(MHD_YES, result);

    // Check that the parameter was added and decoded
    json_t *stored_value = json_object_get(post_data, key);
    TEST_ASSERT_NOT_NULL(stored_value);
    TEST_ASSERT_TRUE(json_is_string(stored_value));
    TEST_ASSERT_EQUAL_STRING("user+name@example.com", json_string_value(stored_value));

    json_decref(post_data);
}

// Test post_data_iterator with NULL key
void test_post_data_iterator_null_key(void) {
    json_t *post_data = json_object();
    TEST_ASSERT_NOT_NULL(post_data);

    const char *value = "test_value";

    enum MHD_Result result = post_data_iterator((void *)post_data, MHD_POSTDATA_KIND, NULL, value);

    TEST_ASSERT_EQUAL(MHD_YES, result);

    // Should not add anything to the object
    TEST_ASSERT_EQUAL(0, json_object_size(post_data));

    json_decref(post_data);
}

// Test post_data_iterator with NULL value
void test_post_data_iterator_null_value(void) {
    json_t *post_data = json_object();
    TEST_ASSERT_NOT_NULL(post_data);

    const char *key = "test_key";

    enum MHD_Result result = post_data_iterator((void *)post_data, MHD_POSTDATA_KIND, key, NULL);

    TEST_ASSERT_EQUAL(MHD_YES, result);

    // Should not add anything to the object
    TEST_ASSERT_EQUAL(0, json_object_size(post_data));

    json_decref(post_data);
}

// Test post_data_iterator with both NULL key and value
void test_post_data_iterator_null_key_and_value(void) {
    json_t *post_data = json_object();
    TEST_ASSERT_NOT_NULL(post_data);

    enum MHD_Result result = post_data_iterator((void *)post_data, MHD_POSTDATA_KIND, NULL, NULL);

    TEST_ASSERT_EQUAL(MHD_YES, result);

    // Should not add anything to the object
    TEST_ASSERT_EQUAL(0, json_object_size(post_data));

    json_decref(post_data);
}

// Test post_data_iterator with URL-encoded value that fails to decode
void test_post_data_iterator_invalid_url_encoding(void) {
    json_t *post_data = json_object();
    TEST_ASSERT_NOT_NULL(post_data);

    const char *key = "test_key";
    const char *value = "invalid%GGencoding";

    enum MHD_Result result = post_data_iterator((void *)post_data, MHD_POSTDATA_KIND, key, value);

    TEST_ASSERT_EQUAL(MHD_YES, result);

    // Should still add the value as-is if decoding fails
    json_t *stored_value = json_object_get(post_data, key);
    TEST_ASSERT_NOT_NULL(stored_value);
    TEST_ASSERT_TRUE(json_is_string(stored_value));
    TEST_ASSERT_EQUAL_STRING(value, json_string_value(stored_value));

    json_decref(post_data);
}

// Test post_data_iterator with plus signs (form encoding)
void test_post_data_iterator_plus_encoding(void) {
    json_t *post_data = json_object();
    TEST_ASSERT_NOT_NULL(post_data);

    const char *key = "message";
    const char *value = "hello+world";

    enum MHD_Result result = post_data_iterator((void *)post_data, MHD_POSTDATA_KIND, key, value);

    TEST_ASSERT_EQUAL(MHD_YES, result);

    // Check that plus was converted to space
    json_t *stored_value = json_object_get(post_data, key);
    TEST_ASSERT_NOT_NULL(stored_value);
    TEST_ASSERT_TRUE(json_is_string(stored_value));
    TEST_ASSERT_EQUAL_STRING("hello world", json_string_value(stored_value));

    json_decref(post_data);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_query_param_iterator_valid_key_value);
    RUN_TEST(test_query_param_iterator_null_key);
    RUN_TEST(test_query_param_iterator_null_value);
    RUN_TEST(test_query_param_iterator_null_key_and_value);
    RUN_TEST(test_query_param_iterator_invalid_url_encoding);

    RUN_TEST(test_post_data_iterator_valid_key_value);
    RUN_TEST(test_post_data_iterator_null_key);
    RUN_TEST(test_post_data_iterator_null_value);
    RUN_TEST(test_post_data_iterator_null_key_and_value);
    RUN_TEST(test_post_data_iterator_invalid_url_encoding);
    RUN_TEST(test_post_data_iterator_plus_encoding);

    return UNITY_END();
}