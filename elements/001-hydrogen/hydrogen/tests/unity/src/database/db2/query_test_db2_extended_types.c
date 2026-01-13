/*
 * Unity Test File: DB2 Extended Parameter Types
 * This file contains unit tests for DB2 extended parameter type binding
 * Tests: TEXT, DATE, TIME, DATETIME, TIMESTAMP parameter types
 */

// Standard project header plus Unity Framework header
#include <src/hydrogen.h>
#include <unity.h>

// Include database parameter headers
#include <src/database/database_params.h>
#include <src/database/db2/types.h>

// Forward declarations for functions being tested
ParameterList* parse_typed_parameters(const char* json_params, const char* dqm_label);
char* convert_named_to_positional(const char* sql_template, ParameterList* params,
                                   DatabaseEngineType engine_type, TypedParameter*** ordered_params,
                                   size_t* param_count, const char* dqm_label);
void free_parameter_list(ParameterList* params);

// Forward declarations for test functions
void test_parse_text_parameter(void);
void test_parse_date_parameter(void);
void test_parse_time_parameter(void);
void test_parse_datetime_parameter(void);
void test_parse_timestamp_parameter(void);
void test_parse_mixed_parameters_with_extended_types(void);
void test_parse_invalid_date_format(void);
void test_parse_invalid_time_format(void);
void test_parse_empty_text_parameter(void);
void test_parse_null_date_parameter(void);
void test_convert_text_parameter_to_positional(void);
void test_convert_date_parameter_to_positional(void);
void test_convert_time_parameter_to_positional(void);
void test_convert_datetime_parameter_to_positional(void);
void test_convert_timestamp_parameter_to_positional(void);

void setUp(void) {
    // Set up test fixtures, if any
}

void tearDown(void) {
    // Clean up test fixtures, if any
}

// Test TEXT parameter parsing
void test_parse_text_parameter(void) {
    const char* json_params = "{"
        "\"TEXT\": {"
        "\"description\": \"This is a large text field for testing\""
        "}"
    "}";
    
    ParameterList* result = parse_typed_parameters(json_params, "TEST");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(1, result->count);
    free_parameter_list(result);
}

// Test DATE parameter parsing
void test_parse_date_parameter(void) {
    const char* json_params = "{"
        "\"DATE\": {"
        "\"birthDate\": \"1990-05-15\""
        "}"
    "}";
    
    ParameterList* result = parse_typed_parameters(json_params, "TEST");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(1, result->count);
    free_parameter_list(result);
}

// Test TIME parameter parsing
void test_parse_time_parameter(void) {
    const char* json_params = "{"
        "\"TIME\": {"
        "\"startTime\": \"14:30:00\""
        "}"
    "}";
    
    ParameterList* result = parse_typed_parameters(json_params, "TEST");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(1, result->count);
    free_parameter_list(result);
}

// Test DATETIME parameter parsing
void test_parse_datetime_parameter(void) {
    const char* json_params = "{"
        "\"DATETIME\": {"
        "\"createdAt\": \"2025-12-25 10:30:45\""
        "}"
    "}";
    
    ParameterList* result = parse_typed_parameters(json_params, "TEST");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(1, result->count);
    free_parameter_list(result);
}

// Test TIMESTAMP parameter parsing
void test_parse_timestamp_parameter(void) {
    const char* json_params = "{"
        "\"TIMESTAMP\": {"
        "\"modifiedAt\": \"2025-12-25 10:30:45.123\""
        "}"
    "}";
    
    ParameterList* result = parse_typed_parameters(json_params, "TEST");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(1, result->count);
    free_parameter_list(result);
}

// Test mixed parameter types including new types
void test_parse_mixed_parameters_with_extended_types(void) {
    const char* json_params = "{"
        "\"INTEGER\": {"
        "\"userId\": 12345"
        "},"
        "\"STRING\": {"
        "\"username\": \"testuser\""
        "},"
        "\"TEXT\": {"
        "\"biography\": \"Long biography text goes here...\""
        "},"
        "\"DATE\": {"
        "\"birthDate\": \"1985-03-20\""
        "},"
        "\"TIME\": {"
        "\"loginTime\": \"09:15:30\""
        "},"
        "\"DATETIME\": {"
        "\"lastLogin\": \"2025-01-13 09:15:30\""
        "},"
        "\"TIMESTAMP\": {"
        "\"updatedAt\": \"2025-01-13 09:15:30.456\""
        "}"
    "}";
    
    ParameterList* result = parse_typed_parameters(json_params, "TEST");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(7, result->count);
    free_parameter_list(result);
}

// Test invalid DATE format
void test_parse_invalid_date_format(void) {
    const char* json_params = "{"
        "\"DATE\": {"
        "\"invalidDate\": \"not-a-date\""
        "}"
    "}";
    
    // Parser should succeed (format validation happens at binding time)
    ParameterList* result = parse_typed_parameters(json_params, "TEST");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(1, result->count);
    free_parameter_list(result);
}

// Test invalid TIME format
void test_parse_invalid_time_format(void) {
    const char* json_params = "{"
        "\"TIME\": {"
        "\"invalidTime\": \"25:99:99\""
        "}"
    "}";
    
    // Parser should succeed (format validation happens at binding time)
    ParameterList* result = parse_typed_parameters(json_params, "TEST");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(1, result->count);
    free_parameter_list(result);
}

// Test empty TEXT parameter
void test_parse_empty_text_parameter(void) {
    const char* json_params = "{"
        "\"TEXT\": {"
        "\"emptyText\": \"\""
        "}"
    "}";
    
    ParameterList* result = parse_typed_parameters(json_params, "TEST");
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL(1, result->count);
    free_parameter_list(result);
}

// Test null DATE parameter value
void test_parse_null_date_parameter(void) {
    const char* json_params = "{"
        "\"DATE\": {"
        "\"nullDate\": null"
        "}"
    "}";
    
    // Parser should handle null values - expecting failure
    ParameterList* result = parse_typed_parameters(json_params, "TEST");
    TEST_ASSERT_NULL(result);
}

// Test parameter conversion with TEXT type
void test_convert_text_parameter_to_positional(void) {
    // First parse parameters
    const char* json_params = "{"
        "\"TEXT\": {"
        "\"description\": \"Sample description\""
        "}"
    "}";
    
    ParameterList* params = parse_typed_parameters(json_params, "TEST");
    TEST_ASSERT_NOT_NULL(params);
    
    // Test conversion to positional
    const char* sql_template = "INSERT INTO notes (content) VALUES (:description)";
    TypedParameter** ordered_params = NULL;
    size_t param_count = 0;
    
    char* positional_sql = convert_named_to_positional(sql_template, params,
                                                        DB_ENGINE_DB2, &ordered_params,
                                                        &param_count, "TEST");
    
    TEST_ASSERT_NOT_NULL(positional_sql);
    TEST_ASSERT_EQUAL(1, param_count);
    TEST_ASSERT_NOT_NULL(ordered_params);
    
    free(positional_sql);
    free(ordered_params);
    free_parameter_list(params);
}

// Test parameter conversion with DATE type
void test_convert_date_parameter_to_positional(void) {
    // First parse parameters
    const char* json_params = "{"
        "\"DATE\": {"
        "\"eventDate\": \"2025-06-15\""
        "}"
    "}";
    
    ParameterList* params = parse_typed_parameters(json_params, "TEST");
    TEST_ASSERT_NOT_NULL(params);
    
    // Test conversion to positional
    const char* sql_template = "SELECT * FROM events WHERE event_date = :eventDate";
    TypedParameter** ordered_params = NULL;
    size_t param_count = 0;
    
    char* positional_sql = convert_named_to_positional(sql_template, params,
                                                        DB_ENGINE_DB2, &ordered_params,
                                                        &param_count, "TEST");
    
    TEST_ASSERT_NOT_NULL(positional_sql);
    TEST_ASSERT_EQUAL(1, param_count);
    TEST_ASSERT_NOT_NULL(ordered_params);
    
    free(positional_sql);
    free(ordered_params);
    free_parameter_list(params);
}

// Test parameter conversion with TIME type
void test_convert_time_parameter_to_positional(void) {
    // First parse parameters
    const char* json_params = "{"
        "\"TIME\": {"
        "\"meetingTime\": \"15:45:00\""
        "}"
    "}";
    
    ParameterList* params = parse_typed_parameters(json_params, "TEST");
    TEST_ASSERT_NOT_NULL(params);
    
    // Test conversion to positional
    const char* sql_template = "SELECT * FROM meetings WHERE start_time = :meetingTime";
    TypedParameter** ordered_params = NULL;
    size_t param_count = 0;
    
    char* positional_sql = convert_named_to_positional(sql_template, params,
                                                        DB_ENGINE_DB2, &ordered_params,
                                                        &param_count, "TEST");
    
    TEST_ASSERT_NOT_NULL(positional_sql);
    TEST_ASSERT_EQUAL(1, param_count);
    TEST_ASSERT_NOT_NULL(ordered_params);
    
    free(positional_sql);
    free(ordered_params);
    free_parameter_list(params);
}

// Test parameter conversion with DATETIME type
void test_convert_datetime_parameter_to_positional(void) {
    // First parse parameters
    const char* json_params = "{"
        "\"DATETIME\": {"
        "\"appointmentTime\": \"2025-08-20 14:30:00\""
        "}"
    "}";
    
    ParameterList* params = parse_typed_parameters(json_params, "TEST");
    TEST_ASSERT_NOT_NULL(params);
    
    // Test conversion to positional
    const char* sql_template = "SELECT * FROM appointments WHERE appt_time = :appointmentTime";
    TypedParameter** ordered_params = NULL;
    size_t param_count = 0;
    
    char* positional_sql = convert_named_to_positional(sql_template, params,
                                                        DB_ENGINE_DB2, &ordered_params,
                                                        &param_count, "TEST");
    
    TEST_ASSERT_NOT_NULL(positional_sql);
    TEST_ASSERT_EQUAL(1, param_count);
    TEST_ASSERT_NOT_NULL(ordered_params);
    
    free(positional_sql);
    free(ordered_params);
    free_parameter_list(params);
}

// Test parameter conversion with TIMESTAMP type
void test_convert_timestamp_parameter_to_positional(void) {
    // First parse parameters
    const char* json_params = "{"
        "\"TIMESTAMP\": {"
        "\"recordTimestamp\": \"2025-08-20 14:30:00.789\""
        "}"
    "}";
    
    ParameterList* params = parse_typed_parameters(json_params, "TEST");
    TEST_ASSERT_NOT_NULL(params);
    
    // Test conversion to positional
    const char* sql_template = "SELECT * FROM records WHERE created_at = :recordTimestamp";
    TypedParameter** ordered_params = NULL;
    size_t param_count = 0;
    
    char* positional_sql = convert_named_to_positional(sql_template, params,
                                                        DB_ENGINE_DB2, &ordered_params,
                                                        &param_count, "TEST");
    
    TEST_ASSERT_NOT_NULL(positional_sql);
    TEST_ASSERT_EQUAL(1, param_count);
    TEST_ASSERT_NOT_NULL(ordered_params);
    
    free(positional_sql);
    free(ordered_params);
    free_parameter_list(params);
}

int main(void) {
    UNITY_BEGIN();

    // Parameter parsing tests
    RUN_TEST(test_parse_text_parameter);
    RUN_TEST(test_parse_date_parameter);
    RUN_TEST(test_parse_time_parameter);
    RUN_TEST(test_parse_datetime_parameter);
    RUN_TEST(test_parse_timestamp_parameter);
    RUN_TEST(test_parse_mixed_parameters_with_extended_types);
    
    // Invalid format tests
    RUN_TEST(test_parse_invalid_date_format);
    RUN_TEST(test_parse_invalid_time_format);
    RUN_TEST(test_parse_empty_text_parameter);
    RUN_TEST(test_parse_null_date_parameter);
    
    // Parameter conversion tests (placeholders - may need API adjustments)
    RUN_TEST(test_convert_text_parameter_to_positional);
    RUN_TEST(test_convert_date_parameter_to_positional);
    RUN_TEST(test_convert_time_parameter_to_positional);
    RUN_TEST(test_convert_datetime_parameter_to_positional);
    RUN_TEST(test_convert_timestamp_parameter_to_positional);

    return UNITY_END();
}
