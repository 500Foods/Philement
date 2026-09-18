/*
 * Unity tests for firebase_expr_parse().
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/firebase/sql_expr.h>
#include <src/database/firebase/sql_parse.h>

void test_firebase_expr_parse_literals(void);
void test_firebase_expr_parse_call_nested(void);
void test_firebase_expr_parse_errors(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_firebase_expr_parse_literals(void) {
    const char* sql = "NULL";
    char* err = NULL;
    FirebaseExpr* expr = firebase_expr_parse(&sql, &err);
    TEST_ASSERT_NOT_NULL(expr);
    TEST_ASSERT_EQUAL(FIREBASE_EXPR_NULL, expr->kind);
    firebase_expr_free(expr);
    free(err);

    sql = "  030";
    expr = firebase_expr_parse(&sql, &err);
    TEST_ASSERT_EQUAL(FIREBASE_EXPR_INTEGER, expr->kind);
    TEST_ASSERT_EQUAL_STRING("030", expr->text);
    firebase_expr_free(expr);

    sql = "-12.5";
    expr = firebase_expr_parse(&sql, &err);
    TEST_ASSERT_EQUAL(FIREBASE_EXPR_NUMBER, expr->kind);
    firebase_expr_free(expr);

    sql = "'it''s'";
    expr = firebase_expr_parse(&sql, &err);
    TEST_ASSERT_EQUAL(FIREBASE_EXPR_STRING, expr->kind);
    TEST_ASSERT_EQUAL_STRING("it's", expr->text);
    firebase_expr_free(expr);

    sql = "lookup_id";
    expr = firebase_expr_parse(&sql, &err);
    TEST_ASSERT_EQUAL(FIREBASE_EXPR_IDENT, expr->kind);
    TEST_ASSERT_EQUAL_STRING("lookup_id", expr->text);
    firebase_expr_free(expr);
    firebase_expr_free(NULL);
}

void test_firebase_expr_parse_call_nested(void) {
    const char* sql = "FB_BROTLI_DECOMPRESS(FB_BASE64_DECODE('iwWA'))";
    char* err = NULL;
    FirebaseExpr* expr = firebase_expr_parse(&sql, &err);
    TEST_ASSERT_NOT_NULL(expr);
    TEST_ASSERT_EQUAL(FIREBASE_EXPR_CALL, expr->kind);
    TEST_ASSERT_EQUAL_STRING("FB_BROTLI_DECOMPRESS", expr->text);
    TEST_ASSERT_EQUAL(1, expr->arg_count);
    TEST_ASSERT_EQUAL(FIREBASE_EXPR_CALL, expr->args[0]->kind);
    TEST_ASSERT_EQUAL_STRING("FB_BASE64_DECODE", expr->args[0]->text);
    firebase_expr_free(expr);

    sql = "FB_NOW()";
    expr = firebase_expr_parse(&sql, &err);
    TEST_ASSERT_EQUAL(FIREBASE_EXPR_CALL, expr->kind);
    TEST_ASSERT_EQUAL(0, expr->arg_count);
    firebase_expr_free(expr);

    sql = "(42)";
    expr = firebase_expr_parse(&sql, &err);
    TEST_ASSERT_EQUAL(FIREBASE_EXPR_INTEGER, expr->kind);
    TEST_ASSERT_EQUAL_STRING("42", expr->text);
    firebase_expr_free(expr);

    sql = "COALESCE(MAX(query_id), 0) + 1";
    expr = firebase_expr_parse(&sql, &err);
    TEST_ASSERT_EQUAL(FIREBASE_EXPR_ADD, expr->kind);
    TEST_ASSERT_EQUAL(2, expr->arg_count);
    TEST_ASSERT_EQUAL(FIREBASE_EXPR_CALL, expr->args[0]->kind);
    TEST_ASSERT_EQUAL_STRING("COALESCE", expr->args[0]->text);
    firebase_expr_free(expr);
}

void test_firebase_expr_parse_errors(void) {
    const char* sql = "";
    char* err = NULL;
    FirebaseExpr* expr = firebase_expr_parse(&sql, &err);
    TEST_ASSERT_NULL(expr);
    TEST_ASSERT_NOT_NULL(err);
    free(err);

    sql = "'unterminated";
    err = NULL;
    expr = firebase_expr_parse(&sql, &err);
    TEST_ASSERT_NULL(expr);
    TEST_ASSERT_NOT_NULL(err);
    free(err);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_firebase_expr_parse_literals);
    RUN_TEST(test_firebase_expr_parse_call_nested);
    RUN_TEST(test_firebase_expr_parse_errors);
    return UNITY_END();
}
