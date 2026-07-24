/*
 * Unity Test File: oidc_userinfo_apply_scoped_claims
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/oidc/oidc_service.h>

#include <jansson.h>
#include <string.h>

void test_apply_nulls(void);
void test_apply_email_and_profile(void);
void test_apply_scope_filters(void);
void test_apply_bad_json(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_apply_nulls(void) {
    oidc_userinfo_apply_scoped_claims(NULL, "openid", "{}");
    json_t *out = json_object();
    TEST_ASSERT_NOT_NULL(out);
    oidc_userinfo_apply_scoped_claims(out, "openid email", NULL);
    oidc_userinfo_apply_scoped_claims(out, "openid email", "");
    TEST_ASSERT_NULL(json_object_get(out, "email"));
    json_decref(out);
}

void test_apply_email_and_profile(void) {
    json_t *out = json_object();
    const char *ud =
        "{\"email\":\"a@b.c\",\"email_verified\":true,"
        "\"name\":\"N\",\"preferred_username\":\"u\"}";
    oidc_userinfo_apply_scoped_claims(out, "openid email profile", ud);
    TEST_ASSERT_NOT_NULL(json_object_get(out, "email"));
    TEST_ASSERT_EQUAL_STRING("a@b.c", json_string_value(json_object_get(out, "email")));
    TEST_ASSERT_TRUE(json_is_true(json_object_get(out, "email_verified")));
    TEST_ASSERT_EQUAL_STRING("N", json_string_value(json_object_get(out, "name")));
    TEST_ASSERT_EQUAL_STRING("u", json_string_value(json_object_get(out, "preferred_username")));
    json_decref(out);
}

void test_apply_scope_filters(void) {
    json_t *out = json_object();
    const char *ud =
        "{\"email\":\"a@b.c\",\"name\":\"N\"}";
    oidc_userinfo_apply_scoped_claims(out, "openid", ud);
    TEST_ASSERT_NULL(json_object_get(out, "email"));
    TEST_ASSERT_NULL(json_object_get(out, "name"));

    oidc_userinfo_apply_scoped_claims(out, "openid email", ud);
    TEST_ASSERT_NOT_NULL(json_object_get(out, "email"));
    TEST_ASSERT_NULL(json_object_get(out, "name"));
    json_decref(out);
}

void test_apply_bad_json(void) {
    json_t *out = json_object();
    oidc_userinfo_apply_scoped_claims(out, "openid email", "not-json");
    TEST_ASSERT_NULL(json_object_get(out, "email"));
    oidc_userinfo_apply_scoped_claims(out, "openid email", "[1,2]");
    TEST_ASSERT_NULL(json_object_get(out, "email"));
    json_decref(out);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_apply_nulls);
    RUN_TEST(test_apply_email_and_profile);
    RUN_TEST(test_apply_scope_filters);
    RUN_TEST(test_apply_bad_json);
    return UNITY_END();
}
