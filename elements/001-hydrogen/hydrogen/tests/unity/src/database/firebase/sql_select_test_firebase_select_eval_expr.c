/*
 * Unity tests for firebase_select_eval_expr().
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/firebase/sql_parse.h>
#include <src/database/firebase/sql_expr.h>
#include <src/database/firebase/sql_select.h>
#include <jansson.h>

void test_firebase_select_eval_expr(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_firebase_select_eval_expr(void) {
    const char* sql = "COALESCE(MAX(query_id), 0) + 1";
    char* err = NULL;
    FirebaseExpr* expr = firebase_expr_parse(&sql, &err);
    TEST_ASSERT_NOT_NULL(expr);
    free(err);

    FirebaseValue val;
    TEST_ASSERT_TRUE(firebase_select_eval_expr(expr, NULL, NULL, &val, &err));
    TEST_ASSERT_EQUAL(1, val.i);
    firebase_value_free(&val);

    json_t* docs = json_loads(
        "[{\"fields\":{\"query_id\":{\"integerValue\":\"7\"}}}]", 0, NULL);
    TEST_ASSERT_TRUE(firebase_select_eval_expr(expr, NULL, docs, &val, &err));
    TEST_ASSERT_EQUAL(8, val.i);
    firebase_value_free(&val);
    json_decref(docs);
    firebase_expr_free(expr);

    sql = "new_query_id";
    expr = firebase_expr_parse(&sql, &err);
    json_t* row = json_loads("{\"new_query_id\":{\"integerValue\":\"2\"}}", 0, NULL);
    TEST_ASSERT_TRUE(firebase_select_eval_expr(expr, row, NULL, &val, &err));
    TEST_ASSERT_EQUAL(2, val.i);
    firebase_value_free(&val);
    json_decref(row);
    firebase_expr_free(expr);

    sql = "MAX(1, 2)";
    expr = firebase_expr_parse(&sql, &err);
    TEST_ASSERT_FALSE(firebase_select_eval_expr(expr, NULL, NULL, &val, &err));
    firebase_value_free(&val);
    firebase_expr_free(expr);
    free(err);
    err = NULL;

    TEST_ASSERT_FALSE(firebase_select_eval_expr(NULL, NULL, NULL, NULL, &err));
    TEST_ASSERT_FALSE(firebase_select_eval_expr(NULL, NULL, NULL, &val, &err));
    firebase_value_free(&val);
    free(err);
    err = NULL;

    FirebaseExpr* add = firebase_expr_alloc(FIREBASE_EXPR_ADD);
    TEST_ASSERT_FALSE(firebase_select_eval_expr(add, NULL, NULL, &val, &err));
    firebase_value_free(&val);
    firebase_expr_free(add);
    free(err);
    err = NULL;

    sql = "NULL + 1";
    expr = firebase_expr_parse(&sql, &err);
    TEST_ASSERT_TRUE(firebase_select_eval_expr(expr, NULL, NULL, &val, &err));
    TEST_ASSERT_TRUE(firebase_value_is_null(&val));
    firebase_value_free(&val);
    firebase_expr_free(expr);

    sql = "'a' + 1";
    expr = firebase_expr_parse(&sql, &err);
    TEST_ASSERT_FALSE(firebase_select_eval_expr(expr, NULL, NULL, &val, &err));
    firebase_value_free(&val);
    firebase_expr_free(expr);
    free(err);
    err = NULL;

    sql = "COALESCE()";
    expr = firebase_expr_parse(&sql, &err);
    TEST_ASSERT_FALSE(firebase_select_eval_expr(expr, NULL, NULL, &val, &err));
    firebase_value_free(&val);
    firebase_expr_free(expr);
    free(err);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_firebase_select_eval_expr);
    return UNITY_END();
}
