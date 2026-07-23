/*
 * Unity Test File: oidc_validate_client / authenticate / register
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/oidc/oidc_clients.h>
#include <string.h>

void test_validate_null_params(void);
void test_validate_redirect_exact_match(void);
void test_authenticate_public_and_confidential(void);
void test_register_and_find(void);
void test_redirect_uri_allowed_helpers(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_validate_null_params(void) {
    OIDCClientContext *ctx = init_oidc_client_registry();
    TEST_ASSERT_NOT_NULL(ctx);
    TEST_ASSERT_FALSE(oidc_validate_client(NULL, "c", "http://x"));
    TEST_ASSERT_FALSE(oidc_validate_client(ctx, NULL, "http://x"));
    TEST_ASSERT_FALSE(oidc_validate_client(ctx, "c", NULL));
    TEST_ASSERT_FALSE(oidc_authenticate_client(NULL, "c", "s"));
    cleanup_oidc_client_registry(ctx);
}

void test_redirect_uri_allowed_helpers(void) {
    TEST_ASSERT_TRUE(oidc_redirect_uri_allowed(
        "[\"https://app.example/cb\",\"http://localhost:3000/cb\"]",
        "https://app.example/cb"));
    TEST_ASSERT_FALSE(oidc_redirect_uri_allowed(
        "[\"https://app.example/cb\"]",
        "https://evil.example/cb"));
    TEST_ASSERT_FALSE(oidc_redirect_uri_allowed("not-json", "https://x"));
    TEST_ASSERT_FALSE(oidc_redirect_uri_allowed(NULL, "https://x"));
}

void test_validate_redirect_exact_match(void) {
    OIDCClientContext *ctx = init_oidc_client_registry();
    TEST_ASSERT_NOT_NULL(ctx);

    OIDCClient *c = oidc_client_create(
        "spa-public",
        NULL,
        "SPA",
        false,
        true,
        "[\"https://app.example/cb\"]",
        "authorization_code",
        "code");
    TEST_ASSERT_NOT_NULL(c);
    TEST_ASSERT_TRUE(oidc_client_registry_add(ctx, c));

    TEST_ASSERT_TRUE(oidc_validate_client(ctx, "spa-public", "https://app.example/cb"));
    TEST_ASSERT_FALSE(oidc_validate_client(ctx, "spa-public", "https://app.example/cb/extra"));
    TEST_ASSERT_FALSE(oidc_validate_client(ctx, "missing", "https://app.example/cb"));

    cleanup_oidc_client_registry(ctx);
}

void test_authenticate_public_and_confidential(void) {
    OIDCClientContext *ctx = init_oidc_client_registry();
    TEST_ASSERT_NOT_NULL(ctx);

    char *hash = oidc_hash_client_secret("s3cret-value");
    TEST_ASSERT_NOT_NULL(hash);

    OIDCClient *conf = oidc_client_create(
        "conf-app", hash, "Confidential", true, false,
        "[\"https://app.example/cb\"]", "authorization_code", "code");
    free(hash);
    TEST_ASSERT_NOT_NULL(conf);
    TEST_ASSERT_TRUE(oidc_client_registry_add(ctx, conf));

    OIDCClient *pub = oidc_client_create(
        "pub-app", NULL, "Public", false, true,
        "[\"https://app.example/cb\"]", "authorization_code", "code");
    TEST_ASSERT_NOT_NULL(pub);
    TEST_ASSERT_TRUE(oidc_client_registry_add(ctx, pub));

    TEST_ASSERT_TRUE(oidc_authenticate_client(ctx, "conf-app", "s3cret-value"));
    TEST_ASSERT_FALSE(oidc_authenticate_client(ctx, "conf-app", "wrong"));
    TEST_ASSERT_FALSE(oidc_authenticate_client(ctx, "conf-app", NULL));
    TEST_ASSERT_TRUE(oidc_authenticate_client(ctx, "pub-app", NULL));
    TEST_ASSERT_TRUE(oidc_authenticate_client(ctx, "pub-app", ""));
    TEST_ASSERT_FALSE(oidc_authenticate_client(ctx, "pub-app", "should-not-send"));

    cleanup_oidc_client_registry(ctx);
}

void test_register_and_find(void) {
    OIDCClientContext *ctx = init_oidc_client_registry();
    TEST_ASSERT_NOT_NULL(ctx);

    char *cid = NULL;
    char *sec = NULL;
    TEST_ASSERT_TRUE(oidc_register_client(ctx, "New App", "https://new.example/cb",
                                          true, &cid, &sec));
    TEST_ASSERT_NOT_NULL(cid);
    TEST_ASSERT_NOT_NULL(sec);
    TEST_ASSERT_TRUE(oidc_validate_client(ctx, cid, "https://new.example/cb"));
    TEST_ASSERT_TRUE(oidc_authenticate_client(ctx, cid, sec));

    free(cid);
    free(sec);
    cleanup_oidc_client_registry(ctx);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_validate_null_params);
    RUN_TEST(test_redirect_uri_allowed_helpers);
    RUN_TEST(test_validate_redirect_exact_match);
    RUN_TEST(test_authenticate_public_and_confidential);
    RUN_TEST(test_register_and_find);
    return UNITY_END();
}
