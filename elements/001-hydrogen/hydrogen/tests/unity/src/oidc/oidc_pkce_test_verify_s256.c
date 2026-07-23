/*
 * Unity Test File: oidc_pkce S256
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/oidc/oidc_pkce.h>
#include <string.h>

void test_pkce_nulls(void);
void test_pkce_round_trip(void);
void test_pkce_wrong_verifier(void);
void test_pkce_method(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_pkce_nulls(void) {
    TEST_ASSERT_NULL(oidc_pkce_make_challenge_s256(NULL));
    TEST_ASSERT_NULL(oidc_pkce_make_challenge_s256(""));
    TEST_ASSERT_FALSE(oidc_pkce_verify_s256(NULL, "x"));
    TEST_ASSERT_FALSE(oidc_pkce_verify_s256("v", NULL));
}

void test_pkce_method(void) {
    TEST_ASSERT_TRUE(oidc_pkce_method_is_s256("S256"));
    TEST_ASSERT_FALSE(oidc_pkce_method_is_s256("plain"));
    TEST_ASSERT_FALSE(oidc_pkce_method_is_s256("s256"));
    TEST_ASSERT_FALSE(oidc_pkce_method_is_s256(NULL));
}

void test_pkce_round_trip(void) {
    const char *verifier = "dBjftJeZ4CVP-mB92K27uhbUJU1p1r_wW1gFWFOEjXk";
    char *challenge = oidc_pkce_make_challenge_s256(verifier);
    TEST_ASSERT_NOT_NULL(challenge);
    TEST_ASSERT_TRUE(strlen(challenge) > 20U);
    TEST_ASSERT_TRUE(oidc_pkce_verify_s256(verifier, challenge));
    free(challenge);
}

void test_pkce_wrong_verifier(void) {
    char *challenge = oidc_pkce_make_challenge_s256("correct-verifier-value-here");
    TEST_ASSERT_NOT_NULL(challenge);
    TEST_ASSERT_FALSE(oidc_pkce_verify_s256("wrong-verifier", challenge));
    free(challenge);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_pkce_nulls);
    RUN_TEST(test_pkce_method);
    RUN_TEST(test_pkce_round_trip);
    RUN_TEST(test_pkce_wrong_verifier);
    return UNITY_END();
}
