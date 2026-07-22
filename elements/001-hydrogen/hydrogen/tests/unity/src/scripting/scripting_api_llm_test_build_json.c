/*
 * Unity Test File: scripting_api_llm_test_build_json.c
 *
 * Tests build_llm_request_json() helper function:
 *   - NULL prompt returns NULL
 *   - Valid prompt returns JSON with prompt field
 *   - max_tokens > 0 includes max_tokens in JSON
 *   - max_tokens <= 0 does not include max_tokens in JSON
 *   - temperature in valid range (0.0, 2.0) includes temperature
 *   - temperature at boundary values (0.0, 2.0) does not include temperature
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <string.h>
#include <stdlib.h>
#include <jansson.h>

#include <src/scripting/scripting_api_llm.h>

/* Forward declarations */
void test_build_llm_request_json_null_prompt(void);
void test_build_llm_request_json_basic_prompt(void);
void test_build_llm_request_json_with_max_tokens(void);
void test_build_llm_request_json_without_max_tokens(void);
void test_build_llm_request_json_with_temperature(void);
void test_build_llm_request_json_temperature_at_zero(void);
void test_build_llm_request_json_temperature_at_upper_bound(void);
void test_build_llm_request_json_full_parameters(void);

void setUp(void) {
}

void tearDown(void) {
}

/* NULL prompt returns NULL */
void test_build_llm_request_json_null_prompt(void) {
    char* json_str = build_llm_request_json(NULL, 100, 0.7);
    TEST_ASSERT_NULL(json_str);
}

/* Valid prompt returns JSON with prompt field */
void test_build_llm_request_json_basic_prompt(void) {
    char* json_str = build_llm_request_json("Hello, world!", 0, 0.0);
    TEST_ASSERT_NOT_NULL(json_str);
    TEST_ASSERT_TRUE(strstr(json_str, "\"prompt\"") != NULL);
    TEST_ASSERT_TRUE(strstr(json_str, "Hello, world!") != NULL);
    json_decref(json_loads(json_str, JSON_COMPACT, NULL));
    free(json_str);
}

/* max_tokens > 0 includes max_tokens in JSON */
void test_build_llm_request_json_with_max_tokens(void) {
    char* json_str = build_llm_request_json("test prompt", 500, 0.0);
    TEST_ASSERT_NOT_NULL(json_str);
    TEST_ASSERT_TRUE(strstr(json_str, "\"max_tokens\"") != NULL);
    TEST_ASSERT_TRUE(strstr(json_str, "500") != NULL);
    free(json_str);
}

/* max_tokens <= 0 does not include max_tokens in JSON */
void test_build_llm_request_json_without_max_tokens(void) {
    char* json_str = build_llm_request_json("test prompt", 0, 0.0);
    TEST_ASSERT_NOT_NULL(json_str);
    TEST_ASSERT_TRUE(strstr(json_str, "max_tokens") == NULL);

    json_str = build_llm_request_json("test prompt", -100, 0.0);
    TEST_ASSERT_NOT_NULL(json_str);
    TEST_ASSERT_TRUE(strstr(json_str, "max_tokens") == NULL);
    free(json_str);
}

/* temperature in valid range (0.0, 2.0) includes temperature */
void test_build_llm_request_json_with_temperature(void) {
    char* json_str = build_llm_request_json("test prompt", 0, 0.5);
    TEST_ASSERT_NOT_NULL(json_str);
    TEST_ASSERT_TRUE(strstr(json_str, "\"temperature\"") != NULL);
    free(json_str);
}

/* temperature at 0.0 does not include temperature */
void test_build_llm_request_json_temperature_at_zero(void) {
    char* json_str = build_llm_request_json("test prompt", 0, 0.0);
    TEST_ASSERT_NOT_NULL(json_str);
    TEST_ASSERT_TRUE(strstr(json_str, "temperature") == NULL);
    free(json_str);
}

/* temperature at upper bound 2.0 does not include temperature (exclusive range) */
void test_build_llm_request_json_temperature_at_upper_bound(void) {
    char* json_str = build_llm_request_json("test prompt", 0, 1.99);
    TEST_ASSERT_NOT_NULL(json_str);
    TEST_ASSERT_TRUE(strstr(json_str, "\"temperature\"") != NULL);
    free(json_str);

    json_str = build_llm_request_json("test prompt", 0, 2.0);
    TEST_ASSERT_NOT_NULL(json_str);
    TEST_ASSERT_TRUE(strstr(json_str, "temperature") == NULL);
    free(json_str);
}

/* Full parameters test */
void test_build_llm_request_json_full_parameters(void) {
    char* json_str = build_llm_request_json("Hello AI", 1024, 0.8);
    TEST_ASSERT_NOT_NULL(json_str);
    TEST_ASSERT_TRUE(strstr(json_str, "\"prompt\"") != NULL);
    TEST_ASSERT_TRUE(strstr(json_str, "\"max_tokens\"") != NULL);
    TEST_ASSERT_TRUE(strstr(json_str, "\"temperature\"") != NULL);
    TEST_ASSERT_TRUE(strstr(json_str, "1024") != NULL);
    free(json_str);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_build_llm_request_json_null_prompt);
    RUN_TEST(test_build_llm_request_json_basic_prompt);
    RUN_TEST(test_build_llm_request_json_with_max_tokens);
    RUN_TEST(test_build_llm_request_json_without_max_tokens);
    RUN_TEST(test_build_llm_request_json_with_temperature);
    RUN_TEST(test_build_llm_request_json_temperature_at_zero);
    RUN_TEST(test_build_llm_request_json_temperature_at_upper_bound);
    RUN_TEST(test_build_llm_request_json_full_parameters);

    return UNITY_END();
}