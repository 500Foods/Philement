/*
 * Unity tests for firebase_select_value_to_json().
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/firebase/sql_expr.h>
#include <src/database/firebase/sql_select.h>
#include <jansson.h>

void test_firebase_select_value_to_json(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_firebase_select_value_to_json(void) {
    FirebaseValue val;
    firebase_value_init(&val);
    firebase_value_set_null(&val);
    json_t* j = firebase_select_value_to_json(&val);
    TEST_ASSERT_TRUE(json_is_null(j));
    json_decref(j);
    firebase_value_free(&val);

    firebase_value_set_double(&val, 1.5);
    j = firebase_select_value_to_json(&val);
    TEST_ASSERT_TRUE(json_is_real(j));
    json_decref(j);
    firebase_value_free(&val);

    firebase_value_copy_text(&val, "hi");
    j = firebase_select_value_to_json(&val);
    TEST_ASSERT_TRUE(json_is_string(j));
    TEST_ASSERT_EQUAL_STRING("hi", json_string_value(j));
    json_decref(j);
    firebase_value_free(&val);

    json_t* field = json_loads("{\"stringValue\":\"z\"}", 0, NULL);
    j = firebase_select_field_to_json(field);
    TEST_ASSERT_EQUAL_STRING("z", json_string_value(j));
    json_decref(j);
    json_decref(field);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_firebase_select_value_to_json);
    return UNITY_END();
}
