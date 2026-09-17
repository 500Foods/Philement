/*
 * Unity tests for firebase_json_value(). QueryRef 1114 shape: $.icon.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/firebase/fns_json.h>

void test_firebase_json_value_icon(void);
void test_firebase_json_value_missing_path(void);
void test_firebase_json_value_key_with_space(void);
void test_firebase_json_value_nested(void);
void test_firebase_json_value_null_and_number(void);
void test_firebase_json_value_invalid(void);
void test_firebase_json_value_array_index(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_firebase_json_value_icon(void) {
    const char* collection =
        "{\"icon\":\"<fa-user>\",\"name\":\"Active\",\"support prompts\":\"yes\"}";
    char* icon = firebase_json_value(collection, "$.icon");
    TEST_ASSERT_NOT_NULL(icon);
    TEST_ASSERT_EQUAL_STRING("<fa-user>", icon);
    free(icon);
}

void test_firebase_json_value_missing_path(void) {
    TEST_ASSERT_NULL(firebase_json_value("{\"icon\":\"x\"}", "$.nope"));
}

void test_firebase_json_value_key_with_space(void) {
    const char* collection =
        "{\"icon\":\"<fa-user>\",\"support prompts\":\"yes\"}";
    char* value = firebase_json_value(collection, "$.support prompts");
    TEST_ASSERT_NOT_NULL(value);
    TEST_ASSERT_EQUAL_STRING("yes", value);
    free(value);
}

void test_firebase_json_value_nested(void) {
    char* value = firebase_json_value("{\"a\":{\"b\":\"c\"}}", "$.a.b");
    TEST_ASSERT_NOT_NULL(value);
    TEST_ASSERT_EQUAL_STRING("c", value);
    free(value);
}

void test_firebase_json_value_null_and_number(void) {
    TEST_ASSERT_NULL(firebase_json_value("{\"icon\":null}", "$.icon"));
    char* num = firebase_json_value("{\"n\":42}", "$.n");
    TEST_ASSERT_NOT_NULL(num);
    TEST_ASSERT_EQUAL_STRING("42", num);
    free(num);
    char* flag = firebase_json_value("{\"ok\":true}", "$.ok");
    TEST_ASSERT_NOT_NULL(flag);
    TEST_ASSERT_EQUAL_STRING("true", flag);
    free(flag);
}

void test_firebase_json_value_invalid(void) {
    TEST_ASSERT_NULL(firebase_json_value(NULL, "$.icon"));
    TEST_ASSERT_NULL(firebase_json_value("{}", NULL));
    TEST_ASSERT_NULL(firebase_json_value("{bad", "$.icon"));
    TEST_ASSERT_NULL(firebase_json_value("{\"icon\":\"x\"}", "icon"));
}

void test_firebase_json_value_array_index(void) {
    char* value = firebase_json_value("{\"items\":[\"a\",\"b\"]}", "$.items[1]");
    TEST_ASSERT_NOT_NULL(value);
    TEST_ASSERT_EQUAL_STRING("b", value);
    free(value);
    TEST_ASSERT_NULL(firebase_json_value("{\"items\":[\"a\"]}", "$.items[9]"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_firebase_json_value_icon);
    RUN_TEST(test_firebase_json_value_missing_path);
    RUN_TEST(test_firebase_json_value_key_with_space);
    RUN_TEST(test_firebase_json_value_nested);
    RUN_TEST(test_firebase_json_value_null_and_number);
    RUN_TEST(test_firebase_json_value_invalid);
    RUN_TEST(test_firebase_json_value_array_index);
    return UNITY_END();
}
