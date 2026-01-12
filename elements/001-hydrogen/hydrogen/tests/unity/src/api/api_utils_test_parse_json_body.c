/*
 * Unity Test File: API Utils api_parse_json_body Function Tests
 * This file contains unit tests for the api_parse_json_body function in api_utils.c
 * 
 * Target coverage: Lines 515-527 in api_utils.c (JSON parsing with error handling)
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include necessary headers for the module being tested
#include <src/api/api_utils.h>

void setUp(void) {
    // No setup needed
}

void tearDown(void) {
    // No teardown needed
}

// Test functions
void test_api_parse_json_body_null_buffer(void);
void test_api_parse_json_body_null_data(void);
void test_api_parse_json_body_zero_size(void);
void test_api_parse_json_body_valid_json(void);
void test_api_parse_json_body_invalid_json(void);
void test_api_parse_json_body_malformed_json(void);
void test_api_parse_json_body_empty_object(void);
void test_api_parse_json_body_empty_array(void);
void test_api_parse_json_body_special_chars(void);
void test_api_parse_json_body_trailing_garbage(void);

// Test with NULL buffer
void test_api_parse_json_body_null_buffer(void) {
    // Line 516: NULL buffer check
    json_t *result = api_parse_json_body(NULL);
    
    TEST_ASSERT_NULL(result);  // Line 517
}

// Test with NULL data in buffer
void test_api_parse_json_body_null_data(void) {
    ApiPostBuffer buffer = {
        .magic = API_POST_BUFFER_MAGIC,
        .data = NULL,
        .size = 0,
        .capacity = 0,
        .http_method = 'P'
    };
    
    // Line 516: NULL data check
    json_t *result = api_parse_json_body(&buffer);
    
    TEST_ASSERT_NULL(result);  // Line 517
}

// Test with zero size
void test_api_parse_json_body_zero_size(void) {
    char data[] = "{\"test\": \"value\"}";
    ApiPostBuffer buffer = {
        .magic = API_POST_BUFFER_MAGIC,
        .data = data,
        .size = 0,  // Zero size
        .capacity = sizeof(data),
        .http_method = 'P'
    };
    
    // Line 516: Zero size check
    json_t *result = api_parse_json_body(&buffer);
    
    TEST_ASSERT_NULL(result);  // Line 517
}

// Test with valid JSON
void test_api_parse_json_body_valid_json(void) {
    char data[] = "{\"name\": \"test\", \"value\": 123}";
    ApiPostBuffer buffer = {
        .magic = API_POST_BUFFER_MAGIC,
        .data = data,
        .size = strlen(data),
        .capacity = sizeof(data),
        .http_method = 'P'
    };
    
    json_t *result = api_parse_json_body(&buffer);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(json_is_object(result));
    
    json_t *name = json_object_get(result, "name");
    TEST_ASSERT_EQUAL_STRING("test", json_string_value(name));
    
    json_t *value = json_object_get(result, "value");
    TEST_ASSERT_EQUAL(123, json_integer_value(value));
    
    json_decref(result);
}

// Test with invalid JSON (triggers error path)
void test_api_parse_json_body_invalid_json(void) {
    char data[] = "{\"name\": \"test\", invalid}";  // Invalid JSON
    ApiPostBuffer buffer = {
        .magic = API_POST_BUFFER_MAGIC,
        .data = data,
        .size = strlen(data),
        .capacity = sizeof(data),
        .http_method = 'P'
    };
    
    // Lines 522-524: JSON parsing error
    json_t *result = api_parse_json_body(&buffer);
    
    TEST_ASSERT_NULL(result);  // Line 523 logs error, line 526 returns NULL
}

// Test with malformed JSON (missing closing brace)
void test_api_parse_json_body_malformed_json(void) {
    char data[] = "{\"name\": \"test\"";  // Missing closing brace
    ApiPostBuffer buffer = {
        .magic = API_POST_BUFFER_MAGIC,
        .data = data,
        .size = strlen(data),
        .capacity = sizeof(data),
        .http_method = 'P'
    };
    
    // Lines 522-524: JSON parsing error
    json_t *result = api_parse_json_body(&buffer);
    
    TEST_ASSERT_NULL(result);
}

// Test with empty JSON object
void test_api_parse_json_body_empty_object(void) {
    char data[] = "{}";
    ApiPostBuffer buffer = {
        .magic = API_POST_BUFFER_MAGIC,
        .data = data,
        .size = strlen(data),
        .capacity = sizeof(data),
        .http_method = 'P'
    };
    
    json_t *result = api_parse_json_body(&buffer);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(json_is_object(result));
    TEST_ASSERT_EQUAL(0, json_object_size(result));
    
    json_decref(result);
}

// Test with empty JSON array
void test_api_parse_json_body_empty_array(void) {
    char data[] = "[]";
    ApiPostBuffer buffer = {
        .magic = API_POST_BUFFER_MAGIC,
        .data = data,
        .size = strlen(data),
        .capacity = sizeof(data),
        .http_method = 'P'
    };
    
    json_t *result = api_parse_json_body(&buffer);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(json_is_array(result));
    TEST_ASSERT_EQUAL(0, json_array_size(result));
    
    json_decref(result);
}

// Test with JSON containing special characters
void test_api_parse_json_body_special_chars(void) {
    char data[] = "{\"message\": \"Hello\\nWorld\\t!\"}";
    ApiPostBuffer buffer = {
        .magic = API_POST_BUFFER_MAGIC,
        .data = data,
        .size = strlen(data),
        .capacity = sizeof(data),
        .http_method = 'P'
    };
    
    json_t *result = api_parse_json_body(&buffer);
    
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_TRUE(json_is_object(result));
    
    json_t *message = json_object_get(result, "message");
    TEST_ASSERT_EQUAL_STRING("Hello\nWorld\t!", json_string_value(message));
    
    json_decref(result);
}

// Test with trailing garbage after valid JSON
void test_api_parse_json_body_trailing_garbage(void) {
    char data[] = "{\"name\": \"test\"} garbage";
    ApiPostBuffer buffer = {
        .magic = API_POST_BUFFER_MAGIC,
        .data = data,
        .size = strlen(data),
        .capacity = sizeof(data),
        .http_method = 'P'
    };
    
    // json_loads fails with trailing garbage by default
    json_t *result = api_parse_json_body(&buffer);
    
    // Should return NULL because of the trailing garbage
    TEST_ASSERT_NULL(result);
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_api_parse_json_body_null_buffer);
    RUN_TEST(test_api_parse_json_body_null_data);
    RUN_TEST(test_api_parse_json_body_zero_size);
    RUN_TEST(test_api_parse_json_body_valid_json);
    RUN_TEST(test_api_parse_json_body_invalid_json);
    RUN_TEST(test_api_parse_json_body_malformed_json);
    RUN_TEST(test_api_parse_json_body_empty_object);
    RUN_TEST(test_api_parse_json_body_empty_array);
    RUN_TEST(test_api_parse_json_body_special_chars);
    RUN_TEST(test_api_parse_json_body_trailing_garbage);
    
    return UNITY_END();
}
