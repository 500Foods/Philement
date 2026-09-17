/*
 * Unity tests for firebase_expr_eval().
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/database/firebase/types.h>
#include <src/database/firebase/sql_expr.h>
#include <src/database/firebase/fns_tz.h>

void test_firebase_expr_eval_literals_and_now(void);
void test_firebase_expr_eval_brotli_sha256_json(void);
void test_firebase_expr_eval_errors(void);
void test_firebase_expr_eval_encode_tz_lookup(void);
FirebaseExpr* parse_expr(const char* sql);
bool test_lookup_id(const char* name, FirebaseValue* out, void* userdata);

void setUp(void) {
    firebase_now_test_clear();
}

void tearDown(void) {
    firebase_now_test_clear();
}

FirebaseExpr* parse_expr(const char* sql) {
    char* err = NULL;
    FirebaseExpr* expr = firebase_expr_parse(&sql, &err);
    free(err);
    return expr;
}

void test_firebase_expr_eval_literals_and_now(void) {
    FirebaseExpr* expr = parse_expr("NULL");
    FirebaseValue val;
    char* err = NULL;
    TEST_ASSERT_TRUE(firebase_expr_eval(expr, NULL, NULL, &val, &err));
    TEST_ASSERT_TRUE(firebase_value_is_null(&val));
    firebase_value_free(&val);
    firebase_expr_free(expr);

    expr = parse_expr("030");
    TEST_ASSERT_TRUE(firebase_expr_eval(expr, NULL, NULL, &val, &err));
    TEST_ASSERT_EQUAL(FIREBASE_VAL_INT, val.kind);
    TEST_ASSERT_EQUAL(30, val.i);
    char* id = firebase_value_as_text(&val);
    TEST_ASSERT_EQUAL_STRING("30", id);
    free(id);
    firebase_value_free(&val);
    firebase_expr_free(expr);

    firebase_now_test_set_unix(1700000000);
    expr = parse_expr("FB_NOW()");
    TEST_ASSERT_TRUE(firebase_expr_eval(expr, NULL, NULL, &val, &err));
    TEST_ASSERT_EQUAL_STRING("2023-11-14 22:13:20", val.data);
    firebase_value_free(&val);
    firebase_expr_free(expr);
}

void test_firebase_expr_eval_brotli_sha256_json(void) {
    FirebaseExpr* expr = parse_expr("FB_BROTLI_DECOMPRESS(FB_BASE64_DECODE('iwWASGVsbG8gV29ybGQhAw=='))");
    FirebaseValue val;
    char* err = NULL;
    TEST_ASSERT_TRUE(firebase_expr_eval(expr, NULL, NULL, &val, &err));
    TEST_ASSERT_EQUAL_STRING("Hello World!", val.data);
    firebase_value_free(&val);
    firebase_expr_free(expr);

    expr = parse_expr("FB_SHA256_B64('0', 'testpass')");
    TEST_ASSERT_TRUE(firebase_expr_eval(expr, NULL, NULL, &val, &err));
    TEST_ASSERT_EQUAL_STRING("CUQEdl7cgIo2iGBfQmsuosLbdT9uLVpbm/rRJGQlbw0=", val.data);
    firebase_value_free(&val);
    firebase_expr_free(expr);

    expr = parse_expr("FB_JSON_INGEST('{\"icon\":\"fb\"}')");
    TEST_ASSERT_TRUE(firebase_expr_eval(expr, NULL, NULL, &val, &err));
    TEST_ASSERT_EQUAL_STRING("{\"icon\":\"fb\"}", val.data);
    firebase_value_free(&val);
    firebase_expr_free(expr);

    expr = parse_expr("FB_JSON_VALUE('{\"icon\":\"fb\"}', '$.icon')");
    TEST_ASSERT_TRUE(firebase_expr_eval(expr, NULL, NULL, &val, &err));
    TEST_ASSERT_EQUAL_STRING("fb", val.data);
    firebase_value_free(&val);
    firebase_expr_free(expr);
}

void test_firebase_expr_eval_errors(void) {
    FirebaseExpr* expr = parse_expr("lookup_id");
    FirebaseValue val;
    char* err = NULL;
    TEST_ASSERT_FALSE(firebase_expr_eval(expr, NULL, NULL, &val, &err));
    TEST_ASSERT_NOT_NULL(err);
    firebase_value_free(&val);
    firebase_expr_free(expr);
    free(err);

    expr = parse_expr("FB_TIME_ADD(1, 2, 'minutes')");
    err = NULL;
    TEST_ASSERT_FALSE(firebase_expr_eval(expr, NULL, NULL, &val, &err));
    TEST_ASSERT_NOT_NULL(strstr(err, "not implemented"));
    firebase_value_free(&val);
    firebase_expr_free(expr);
    free(err);

    TEST_ASSERT_TRUE(firebase_value_exceeds_max(NULL) == false);
    FirebaseValue big;
    firebase_value_init(&big);
    char* data = malloc(FIREBASE_MAX_FIELD_BYTES + 2);
    TEST_ASSERT_NOT_NULL(data);
    memset(data, 'A', FIREBASE_MAX_FIELD_BYTES + 1);
    data[FIREBASE_MAX_FIELD_BYTES + 1] = '\0';
    TEST_ASSERT_TRUE(firebase_value_take_data(&big, FIREBASE_VAL_TEXT, data, FIREBASE_MAX_FIELD_BYTES + 1));
    TEST_ASSERT_TRUE(firebase_value_exceeds_max(&big));
    firebase_value_free(&big);
}

bool test_lookup_id(const char* name, FirebaseValue* out, void* userdata) {
    (void)userdata;
    if (name && strcmp(name, "lookup_id") == 0) {
        return firebase_value_set_int(out, 30);
    }
    return false;
}

void test_firebase_expr_eval_encode_tz_lookup(void) {
    FirebaseExpr* expr = parse_expr("12.5");
    FirebaseValue val;
    char* err = NULL;
    TEST_ASSERT_TRUE(firebase_expr_eval(expr, NULL, NULL, &val, &err));
    TEST_ASSERT_EQUAL(FIREBASE_VAL_DOUBLE, val.kind);
    firebase_value_free(&val);
    firebase_expr_free(expr);

    expr = parse_expr("FB_BASE64_ENCODE('Hi')");
    TEST_ASSERT_TRUE(firebase_expr_eval(expr, NULL, NULL, &val, &err));
    TEST_ASSERT_EQUAL_STRING("SGk=", val.data);
    firebase_value_free(&val);
    firebase_expr_free(expr);

    expr = parse_expr("FB_CONVERT_TZ('2023-11-14 22:13:20', 'UTC', 'UTC')");
    TEST_ASSERT_TRUE(firebase_expr_eval(expr, NULL, NULL, &val, &err));
    TEST_ASSERT_EQUAL_STRING("2023-11-14 22:13:20", val.data);
    firebase_value_free(&val);
    firebase_expr_free(expr);

    expr = parse_expr("FB_JSON_INGEST(NULL)");
    TEST_ASSERT_TRUE(firebase_expr_eval(expr, NULL, NULL, &val, &err));
    TEST_ASSERT_TRUE(firebase_value_is_null(&val));
    firebase_value_free(&val);
    firebase_expr_free(expr);

    expr = parse_expr("FB_JSON_VALUE('{\"a\":1}', '$.missing')");
    TEST_ASSERT_TRUE(firebase_expr_eval(expr, NULL, NULL, &val, &err));
    TEST_ASSERT_TRUE(firebase_value_is_null(&val));
    firebase_value_free(&val);
    firebase_expr_free(expr);

    expr = parse_expr("lookup_id");
    TEST_ASSERT_TRUE(firebase_expr_eval(expr, test_lookup_id, NULL, &val, &err));
    TEST_ASSERT_EQUAL(30, val.i);
    firebase_value_free(&val);
    firebase_expr_free(expr);

    expr = parse_expr("other_col");
    TEST_ASSERT_TRUE(firebase_expr_eval(expr, test_lookup_id, NULL, &val, &err));
    TEST_ASSERT_TRUE(firebase_value_is_null(&val));
    firebase_value_free(&val);
    firebase_expr_free(expr);

    expr = parse_expr("FB_NOW(1)");
    TEST_ASSERT_FALSE(firebase_expr_eval(expr, NULL, NULL, &val, &err));
    firebase_value_free(&val);
    firebase_expr_free(expr);
    free(err);

    expr = parse_expr("NOPE()");
    err = NULL;
    TEST_ASSERT_FALSE(firebase_expr_eval(expr, NULL, NULL, &val, &err));
    TEST_ASSERT_NOT_NULL(strstr(err, "unknown function"));
    firebase_value_free(&val);
    firebase_expr_free(expr);
    free(err);

    FirebaseValue a;
    FirebaseValue b;
    firebase_value_init(&a);
    firebase_value_init(&b);
    firebase_value_set_null(&a);
    firebase_value_set_null(&b);
    TEST_ASSERT_TRUE(firebase_value_equals(&a, &b));
    firebase_value_set_int(&a, 1);
    TEST_ASSERT_FALSE(firebase_value_equals(&a, &b));
    firebase_value_set_int(&b, 1);
    TEST_ASSERT_TRUE(firebase_value_equals(&a, &b));
    firebase_value_set_double(&a, 1.5);
    firebase_value_set_double(&b, 1.5);
    TEST_ASSERT_TRUE(firebase_value_equals(&a, &b));
    firebase_value_copy_text(&a, "x");
    firebase_value_copy_text(&b, "x");
    TEST_ASSERT_TRUE(firebase_value_equals(&a, &b));
    TEST_ASSERT_FALSE(firebase_value_set_double(NULL, 1.0));
    firebase_value_free(&a);
    firebase_value_free(&b);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_firebase_expr_eval_literals_and_now);
    RUN_TEST(test_firebase_expr_eval_brotli_sha256_json);
    RUN_TEST(test_firebase_expr_eval_errors);
    RUN_TEST(test_firebase_expr_eval_encode_tz_lookup);
    return UNITY_END();
}
