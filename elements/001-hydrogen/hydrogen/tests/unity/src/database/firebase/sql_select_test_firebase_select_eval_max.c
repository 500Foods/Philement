/*
 * Unity tests for firebase_select_eval_max().
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/firebase/sql_expr.h>
#include <src/database/firebase/sql_select.h>
#include <jansson.h>

void test_firebase_select_eval_max(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_firebase_select_eval_max(void) {
    FirebaseValue val;
    TEST_ASSERT_TRUE(firebase_select_eval_max(NULL, "query_id", &val));
    TEST_ASSERT_TRUE(firebase_value_is_null(&val));
    firebase_value_free(&val);

    json_t* empty = json_array();
    TEST_ASSERT_TRUE(firebase_select_eval_max(empty, "query_id", &val));
    TEST_ASSERT_TRUE(firebase_value_is_null(&val));
    firebase_value_free(&val);
    json_decref(empty);

    json_t* docs = json_loads(
        "[{\"fields\":{\"query_id\":{\"integerValue\":\"1\"}}},"
        "{\"fields\":{\"query_id\":{\"integerValue\":\"4\"}}},"
        "{\"fields\":{\"query_id\":{\"integerValue\":\"2\"}}}]", 0, NULL);
    TEST_ASSERT_TRUE(firebase_select_eval_max(docs, "query_id", &val));
    TEST_ASSERT_EQUAL(4, val.i);
    firebase_value_free(&val);
    TEST_ASSERT_TRUE(firebase_select_eval_max(docs, "missing", &val));
    TEST_ASSERT_TRUE(firebase_value_is_null(&val));
    firebase_value_free(&val);
    json_decref(docs);

    TEST_ASSERT_FALSE(firebase_select_eval_max(NULL, "query_id", NULL));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_firebase_select_eval_max);
    return UNITY_END();
}
