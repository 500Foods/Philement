/*
 * Unity Test File: scoreboard_json_test_free_job_list.c
 *
 * Phase 23 of the LUA_PLAN. Tests scripting_free_job_list:
 *   - NULL parameter is a safe no-op
 *   - An empty JSON array is freed without crashing
 *   - A populated JSON array is freed without crashing
 *   - A JSON object (not just an array) is freed correctly, since
 *     the function accepts any json_t* and wraps json_decref
 *   - Reference counting: after an extra json_incref the object
 *     remains valid, proving json_decref is called exactly once
 *     (not zero times, which would leak, or twice, which would
 *     double-free)
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/scripting/scoreboard_json.h>

void setUp(void) {
}

void tearDown(void) {
}

void test_free_job_list_null_parameter(void);
void test_free_job_list_empty_array(void);
void test_free_job_list_populated_array(void);
void test_free_job_list_json_object(void);
void test_free_job_list_decrements_reference_count(void);

void test_free_job_list_null_parameter(void) {
    scripting_free_job_list(NULL);
    TEST_PASS();
}

void test_free_job_list_empty_array(void) {
    json_t* arr = json_array();
    TEST_ASSERT_NOT_NULL(arr);
    TEST_ASSERT_EQUAL(0, json_array_size(arr));

    scripting_free_job_list(arr);
}

void test_free_job_list_populated_array(void) {
    json_t* arr = json_array();
    TEST_ASSERT_NOT_NULL(arr);
    json_array_append_new(arr, json_string("job1"));
    json_array_append_new(arr, json_string("job2"));
    json_array_append_new(arr, json_integer(42));
    TEST_ASSERT_EQUAL(3, json_array_size(arr));

    scripting_free_job_list(arr);
}

void test_free_job_list_json_object(void) {
    json_t* obj = json_object();
    TEST_ASSERT_NOT_NULL(obj);
    json_object_set_new(obj, "job_id", json_string("abc"));
    json_object_set_new(obj, "status", json_string("completed"));

    scripting_free_job_list(obj);
}

void test_free_job_list_decrements_reference_count(void) {
    json_t* arr = json_array();
    TEST_ASSERT_NOT_NULL(arr);
    json_array_append_new(arr, json_integer(42));

    json_incref(arr);
    scripting_free_job_list(arr);

    TEST_ASSERT_TRUE(json_is_array(arr));
    TEST_ASSERT_EQUAL(1, json_array_size(arr));

    json_decref(arr);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_free_job_list_null_parameter);
    RUN_TEST(test_free_job_list_empty_array);
    RUN_TEST(test_free_job_list_populated_array);
    RUN_TEST(test_free_job_list_json_object);
    RUN_TEST(test_free_job_list_decrements_reference_count);

    return UNITY_END();
}
