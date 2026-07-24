/*
 * Unity Test File: oidc_token_form_get
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/oidc/token/token.h>

#include <string.h>

void test_form_get_nulls(void);
void test_form_get_values(void);
void test_form_get_missing(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_form_get_nulls(void) {
    TEST_ASSERT_NULL(oidc_token_form_get(NULL, "a"));
    TEST_ASSERT_NULL(oidc_token_form_get("a=1", NULL));
}

void test_form_get_values(void) {
    const char *body = "grant_type=authorization_code&code=abc%2B1&client_id=cli";
    char *gt = oidc_token_form_get(body, "grant_type");
    char *code = oidc_token_form_get(body, "code");
    char *cid = oidc_token_form_get(body, "client_id");
    TEST_ASSERT_EQUAL_STRING("authorization_code", gt);
    TEST_ASSERT_EQUAL_STRING("abc+1", code);
    TEST_ASSERT_EQUAL_STRING("cli", cid);
    free(gt);
    free(code);
    free(cid);
}

void test_form_get_missing(void) {
    TEST_ASSERT_NULL(oidc_token_form_get("a=1&b=2", "c"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_form_get_nulls);
    RUN_TEST(test_form_get_values);
    RUN_TEST(test_form_get_missing);
    return UNITY_END();
}
