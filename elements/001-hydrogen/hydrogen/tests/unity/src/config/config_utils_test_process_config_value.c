/*
 * Unity Test File: Configuration Utilities Tests
 * This file contains unit tests for the config_utils functions
 * from src/config/config_utils.c
 */

// Standard project header plus Unity Framework header
#include "../../../../src/hydrogen.h"
#include "unity.h"

// Include necessary headers for the module being tested
#include "../../../../src/config/config_utils.h"
#include "../../../../src/config/config.h"

// Forward declarations for functions being tested
bool process_config_value(json_t* root, ConfigValue value, ConfigValueType type,
                         const char* path, const char* section);
bool process_level_config(json_t* root, int* level_ptr, const char* level_name,
                         const char* path, const char* section, int default_value);
json_t* process_env_variable(const char* value);
char* process_env_variable_string(const char* value);
bool is_sensitive_value(const char* name);
const char* format_int(int value);
const char* format_int_buffer(int value);
bool process_int_array_config(json_t* root, ConfigIntArray value, const char* path, const char* section);
bool process_array_element_config(json_t* root, ConfigArrayElement value, const char* path, const char* section);
bool process_string_array_config(json_t* root, ConfigStringArray value, const char* path, const char* section);
bool process_direct_bool_value(ConfigValue value, const char* path, const char* section, bool direct_value);
bool process_direct_value(ConfigValue value, ConfigValueType type,
                         const char* path, const char* section,
                         const char* direct_value);

// Forward declarations for test functions
void test_process_config_value_null_parameters(void);
void test_process_config_value_basic_types(void);
void test_process_level_config_basic(void);
void test_process_env_variable_basic(void);
void test_process_env_variable_string_basic(void);
void test_is_sensitive_value(void);
void test_format_int(void);
void test_format_int_buffer(void);
void test_process_int_array_config_basic(void);
void test_process_array_element_config_basic(void);
void test_process_string_array_config_basic(void);
void test_process_direct_bool_value(void);
void test_process_direct_value(void);

// Test setup and teardown
void setUp(void) {
    // Reset any global state if needed
}

void tearDown(void) {
    // Clean up after each test
}

// ===== PARAMETER VALIDATION TESTS =====

void test_process_config_value_null_parameters(void) {
    // Test with NULL path - should return false
    bool result = process_config_value(NULL, (ConfigValue){0}, CONFIG_TYPE_BOOL, NULL, "Test");
    TEST_ASSERT_FALSE(result);

    // Test with invalid type - should return false
    bool bool_val = false;
    result = process_config_value(NULL, (ConfigValue){.bool_val = &bool_val}, (ConfigValueType)-1, "test.path", "Test");
    TEST_ASSERT_FALSE(result);
}

// ===== BASIC CONFIG VALUE TESTS =====

void test_process_config_value_basic_types(void) {
    json_t* root = json_object();
    json_t* test_section = json_object();

    // Set up test values
    json_object_set(test_section, "bool_val", json_true());
    json_object_set(test_section, "int_val", json_integer(42));
    json_object_set(test_section, "float_val", json_real(3.14));
    json_object_set(test_section, "string_val", json_string("test string"));
    json_object_set(root, "Test", test_section);

    // Test boolean processing
    bool bool_val = false;
    bool result = process_config_value(root, (ConfigValue){.bool_val = &bool_val}, CONFIG_TYPE_BOOL, "Test.bool_val", "Test");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(bool_val);

    // Test integer processing
    int int_val = 0;
    result = process_config_value(root, (ConfigValue){.int_val = &int_val}, CONFIG_TYPE_INT, "Test.int_val", "Test");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(42, int_val);

    // Test float processing
    double float_val = 0.0;
    result = process_config_value(root, (ConfigValue){.float_val = &float_val}, CONFIG_TYPE_FLOAT, "Test.float_val", "Test");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_FLOAT(3.14, float_val);

    // Test string processing
    char* string_val = NULL;
    result = process_config_value(root, (ConfigValue){.string_val = &string_val}, CONFIG_TYPE_STRING, "Test.string_val", "Test");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("test string", string_val);
    free(string_val);

    json_decref(root);
}

// ===== LEVEL CONFIG TESTS =====

void test_process_level_config_basic(void) {
    json_t* root = json_object();
    json_t* test_section = json_object();

    // Set up test level value
    json_object_set(test_section, "level", json_integer(2));
    json_object_set(root, "Test", test_section);

    // Test level processing
    int level_val = 1;  // Default
    bool result = process_level_config(root, &level_val, "DEBUG", "Test.level", "Test", 1);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(2, level_val);

    json_decref(root);
}

// ===== ENVIRONMENT VARIABLE TESTS =====

void test_process_env_variable_basic(void) {
    // Test with NULL value
    json_t* result = process_env_variable(NULL);
    TEST_ASSERT_NULL(result);

    // Test with non-env-var string
    result = process_env_variable("not an env var");
    TEST_ASSERT_NULL(result);

    // Test with malformed env var
    result = process_env_variable("${env.MISSING_BRACE");
    TEST_ASSERT_NULL(result);

    // Test with empty env var (if set)
    result = process_env_variable("${env.PATH}");  // Should be set
    TEST_ASSERT_NOT_NULL(result);
    if (result) {
        TEST_ASSERT_TRUE(json_is_string(result));
        json_decref(result);
    }
}

void test_process_env_variable_string_basic(void) {
    // Test with NULL value
    char* result = process_env_variable_string(NULL);
    TEST_ASSERT_NULL(result);

    // Test with non-env-var string
    result = process_env_variable_string("not an env var");
    TEST_ASSERT_NULL(result);

    // Test with valid env var
    result = process_env_variable_string("${env.PATH}");
    TEST_ASSERT_NOT_NULL(result);
    if (result) {
        TEST_ASSERT_TRUE(strlen(result) > 0);  // PATH should be set
        free(result);
    }
}

// ===== SENSITIVE VALUE TESTS =====

void test_is_sensitive_value(void) {
    // Test sensitive terms
    TEST_ASSERT_TRUE(is_sensitive_value("password"));
    TEST_ASSERT_TRUE(is_sensitive_value("secret_key"));
    TEST_ASSERT_TRUE(is_sensitive_value("jwt_token"));
    TEST_ASSERT_TRUE(is_sensitive_value("api_key"));

    // Test non-sensitive terms
    TEST_ASSERT_FALSE(is_sensitive_value("server_name"));
    TEST_ASSERT_FALSE(is_sensitive_value("port"));
    TEST_ASSERT_FALSE(is_sensitive_value("enabled"));

    // Test NULL
    TEST_ASSERT_FALSE(is_sensitive_value(NULL));
}

// ===== FORMAT TESTS =====

void test_format_int(void) {
    const char* result = format_int(42);
    TEST_ASSERT_EQUAL_STRING("42", result);

    result = format_int(-123);
    TEST_ASSERT_EQUAL_STRING("-123", result);

    result = format_int(0);
    TEST_ASSERT_EQUAL_STRING("0", result);
}

void test_format_int_buffer(void) {
    const char* result = format_int_buffer(99);
    TEST_ASSERT_EQUAL_STRING("99", result);

    result = format_int_buffer(-456);
    TEST_ASSERT_EQUAL_STRING("-456", result);
}

// ===== ARRAY PROCESSING TESTS =====

void test_process_int_array_config_basic(void) {
    json_t* root = json_object();
    json_t* test_section = json_object();
    json_t* int_array = json_array();

    // Create test array
    json_array_append(int_array, json_integer(10));
    json_array_append(int_array, json_integer(20));
    json_array_append(int_array, json_integer(30));
    json_object_set(test_section, "int_array", int_array);
    json_object_set(root, "Test", test_section);

    // Test array processing
    int array[5];
    size_t count = 0;
    ConfigIntArray config_array = {
        .array = array,
        .count = &count,
        .capacity = 5
    };

    bool result = process_int_array_config(root, config_array, "Test.int_array", "Test");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(3, count);
    TEST_ASSERT_EQUAL(10, array[0]);
    TEST_ASSERT_EQUAL(20, array[1]);
    TEST_ASSERT_EQUAL(30, array[2]);

    json_decref(root);
}

void test_process_array_element_config_basic(void) {
    json_t* root = json_object();
    json_t* test_section = json_object();
    json_t* string_array = json_array();

    // Create test array
    json_array_append(string_array, json_string("first"));
    json_array_append(string_array, json_string("second"));
    json_array_append(string_array, json_string("third"));
    json_object_set(test_section, "string_array", string_array);
    json_object_set(root, "Test", test_section);

    // Test array element processing
    char* element = NULL;
    ConfigArrayElement config_element = {
        .element = &element,
        .index = 1
    };

    bool result = process_array_element_config(root, config_element, "Test.string_array", "Test");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("second", element);
    free(element);

    json_decref(root);
}

void test_process_string_array_config_basic(void) {
    json_t* root = json_object();
    json_t* test_section = json_object();
    json_t* string_array = json_array();

    // Create test array
    json_array_append(string_array, json_string("apple"));
    json_array_append(string_array, json_string("banana"));
    json_object_set(test_section, "string_array", string_array);
    json_object_set(root, "Test", test_section);

    // Test string array processing
    char* array[5];
    size_t count = 0;
    ConfigStringArray config_array = {
        .array = array,
        .count = &count,
        .capacity = 5
    };

    bool result = process_string_array_config(root, config_array, "Test.string_array", "Test");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(2, count);
    TEST_ASSERT_EQUAL_STRING("apple", array[0]);
    TEST_ASSERT_EQUAL_STRING("banana", array[1]);

    // Clean up
    free(array[0]);
    free(array[1]);

    json_decref(root);
}

// ===== DIRECT VALUE TESTS =====

void test_process_direct_bool_value(void) {
    bool bool_val = false;

    // Test setting direct boolean value
    bool result = process_direct_bool_value((ConfigValue){.bool_val = &bool_val}, "test.bool", "Test", true);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(bool_val);

    result = process_direct_bool_value((ConfigValue){.bool_val = &bool_val}, "test.bool", "Test", false);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(bool_val);
}

void test_process_direct_value(void) {
    char* string_val = NULL;

    // Test setting direct string value
    bool result = process_direct_value((ConfigValue){.string_val = &string_val}, CONFIG_TYPE_STRING,
                                      "test.string", "Test", "direct value");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("direct value", string_val);

    free(string_val);
}

// ===== MAIN TEST RUNNER =====

int main(void) {
    UNITY_BEGIN();

    // Parameter validation tests
    RUN_TEST(test_process_config_value_null_parameters);

    // Basic config value tests
    RUN_TEST(test_process_config_value_basic_types);

    // Level config tests
    RUN_TEST(test_process_level_config_basic);

    // Environment variable tests
    RUN_TEST(test_process_env_variable_basic);
    RUN_TEST(test_process_env_variable_string_basic);

    // Sensitive value tests
    RUN_TEST(test_is_sensitive_value);

    // Format tests
    RUN_TEST(test_format_int);
    RUN_TEST(test_format_int_buffer);

    // Array processing tests
    RUN_TEST(test_process_int_array_config_basic);
    RUN_TEST(test_process_array_element_config_basic);
    RUN_TEST(test_process_string_array_config_basic);

    // Direct value tests
    RUN_TEST(test_process_direct_bool_value);
    RUN_TEST(test_process_direct_value);

    return UNITY_END();
}