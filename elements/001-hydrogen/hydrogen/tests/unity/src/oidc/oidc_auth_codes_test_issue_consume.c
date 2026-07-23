/*
 * Unity Test File: oidc_auth_code_issue / consume
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/oidc/oidc_auth_codes.h>
#include <src/oidc/oidc_pkce.h>
#include <string.h>

void test_issue_consume_happy_path(void);
void test_second_consume_fails(void);
void test_bad_pkce_fails(void);
void test_wrong_client_redirect_fails(void);
void test_issue_rejects_plain_pkce(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_issue_consume_happy_path(void) {
    OIDCAuthCodeStore *store = oidc_auth_code_store_create(300);
    TEST_ASSERT_NOT_NULL(store);

    const char *verifier = "test-verifier-abcdefghijklmnopqrstuvwxyz012345";
    char *challenge = oidc_pkce_make_challenge_s256(verifier);
    TEST_ASSERT_NOT_NULL(challenge);

    char *code = oidc_auth_code_issue(store, "client-1", 42,
                                      "https://app.example/cb", "openid profile",
                                      "nonce-xyz", challenge, "S256");
    TEST_ASSERT_NOT_NULL(code);

    OIDCAuthCodeRecord out;
    memset(&out, 0, sizeof(out));
    TEST_ASSERT_TRUE(oidc_auth_code_consume(store, code, "client-1",
                                            "https://app.example/cb", verifier, &out));
    TEST_ASSERT_EQUAL_INT(42, out.account_id);
    TEST_ASSERT_EQUAL_STRING("openid profile", out.scope);
    TEST_ASSERT_EQUAL_STRING("nonce-xyz", out.nonce);

    free(code);
    free(challenge);
    oidc_auth_code_store_destroy(store);
}

void test_second_consume_fails(void) {
    OIDCAuthCodeStore *store = oidc_auth_code_store_create(300);
    const char *verifier = "verifier-two-consume-test-value-abcdef";
    char *challenge = oidc_pkce_make_challenge_s256(verifier);
    char *code = oidc_auth_code_issue(store, "c1", 1, "https://x/cb", "openid",
                                      NULL, challenge, "S256");
    TEST_ASSERT_NOT_NULL(code);
    TEST_ASSERT_TRUE(oidc_auth_code_consume(store, code, "c1", "https://x/cb", verifier, NULL));
    TEST_ASSERT_FALSE(oidc_auth_code_consume(store, code, "c1", "https://x/cb", verifier, NULL));
    free(code);
    free(challenge);
    oidc_auth_code_store_destroy(store);
}

void test_bad_pkce_fails(void) {
    OIDCAuthCodeStore *store = oidc_auth_code_store_create(300);
    const char *verifier = "good-verifier-for-bad-pkce-test-xx";
    char *challenge = oidc_pkce_make_challenge_s256(verifier);
    char *code = oidc_auth_code_issue(store, "c1", 1, "https://x/cb", "openid",
                                      NULL, challenge, "S256");
    TEST_ASSERT_FALSE(oidc_auth_code_consume(store, code, "c1", "https://x/cb",
                                             "wrong-verifier", NULL));
    free(code);
    free(challenge);
    oidc_auth_code_store_destroy(store);
}

void test_wrong_client_redirect_fails(void) {
    OIDCAuthCodeStore *store = oidc_auth_code_store_create(300);
    const char *verifier = "verifier-client-redirect-mismatch-yy";
    char *challenge = oidc_pkce_make_challenge_s256(verifier);
    char *code = oidc_auth_code_issue(store, "c1", 1, "https://x/cb", "openid",
                                      NULL, challenge, "S256");
    TEST_ASSERT_FALSE(oidc_auth_code_consume(store, code, "other", "https://x/cb", verifier, NULL));
    TEST_ASSERT_FALSE(oidc_auth_code_consume(store, code, "c1", "https://evil/cb", verifier, NULL));
    free(code);
    free(challenge);
    oidc_auth_code_store_destroy(store);
}

void test_issue_rejects_plain_pkce(void) {
    OIDCAuthCodeStore *store = oidc_auth_code_store_create(300);
    char *code = oidc_auth_code_issue(store, "c1", 1, "https://x/cb", "openid",
                                      NULL, "challenge", "plain");
    TEST_ASSERT_NULL(code);
    oidc_auth_code_store_destroy(store);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_issue_consume_happy_path);
    RUN_TEST(test_second_consume_fails);
    RUN_TEST(test_bad_pkce_fails);
    RUN_TEST(test_wrong_client_redirect_fails);
    RUN_TEST(test_issue_rejects_plain_pkce);
    return UNITY_END();
}
