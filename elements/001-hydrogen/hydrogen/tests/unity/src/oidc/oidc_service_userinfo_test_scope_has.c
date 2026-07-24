/*
 * Unity Test File: oidc_scope_has
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/oidc/oidc_service.h>

void test_scope_has_nulls(void);
void test_scope_has_match(void);
void test_scope_has_no_match(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_scope_has_nulls(void) {
    TEST_ASSERT_FALSE(oidc_scope_has(NULL, "openid"));
    TEST_ASSERT_FALSE(oidc_scope_has("openid", NULL));
    TEST_ASSERT_FALSE(oidc_scope_has("openid", ""));
}

void test_scope_has_match(void) {
    TEST_ASSERT_TRUE(oidc_scope_has("openid", "openid"));
    TEST_ASSERT_TRUE(oidc_scope_has("openid profile email", "profile"));
    TEST_ASSERT_TRUE(oidc_scope_has("openid profile email", "email"));
    TEST_ASSERT_TRUE(oidc_scope_has("  openid  profile  ", "openid"));
}

void test_scope_has_no_match(void) {
    TEST_ASSERT_FALSE(oidc_scope_has("openid profile", "email"));
    TEST_ASSERT_FALSE(oidc_scope_has("openid", "open"));
    TEST_ASSERT_FALSE(oidc_scope_has("openid", "openidx"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_scope_has_nulls);
    RUN_TEST(test_scope_has_match);
    RUN_TEST(test_scope_has_no_match);
    return UNITY_END();
}
