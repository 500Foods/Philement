/*
 * Unity Test File: oidc_issue_authorization_code (with live init)
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/oidc/oidc_service.h>
#include <src/oidc/oidc_pkce.h>
#include <src/config/config_oidc.h>
#include <string.h>
#include <unistd.h>

void test_issue_code_requires_init(void);
void test_issue_code_happy_path(void);
void test_issue_code_bad_redirect(void);

void setUp(void) {
}

void tearDown(void) {
    shutdown_oidc_service();
}

void test_issue_code_requires_init(void) {
    const char *err = NULL;
    char *code = oidc_issue_authorization_code("c", "https://x/cb", "openid", NULL,
                                               "ch", "S256", 1, &err);
    TEST_ASSERT_NULL(code);
    TEST_ASSERT_NOT_NULL(err);
}

void test_issue_code_happy_path(void) {
    char tmpl[] = "/tmp/oidc_auth_issue_XXXXXX";
    char *dir = mkdtemp(tmpl);
    TEST_ASSERT_NOT_NULL(dir);

    OIDCConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.enabled = true;
    cfg.issuer = (char*)"http://localhost:5450";
    cfg.keys.storage_path = dir;
    cfg.keys.encryption_enabled = false;
    cfg.keys.rotation_interval_days = 90;
    cfg.tokens.access_token_lifetime = 3600;
    cfg.tokens.refresh_token_lifetime = 86400;
    cfg.tokens.id_token_lifetime = 3600;

    TEST_ASSERT_TRUE(init_oidc_service(&cfg));

    OIDCContext *ctx = get_oidc_context();
    TEST_ASSERT_NOT_NULL(ctx);
    OIDCClient *client = oidc_client_create(
        "test-client", NULL, "Test", false, true,
        "[\"https://app.example/cb\"]",
        "authorization_code", "code");
    TEST_ASSERT_NOT_NULL(client);
    TEST_ASSERT_TRUE(oidc_client_registry_add((OIDCClientContext*)ctx->client_context, client));

    const char *verifier = "phase8-verifier-abcdefghijklmnopqrstuv";
    char *challenge = oidc_pkce_make_challenge_s256(verifier);
    TEST_ASSERT_NOT_NULL(challenge);

    const char *err = NULL;
    char *code = oidc_issue_authorization_code("test-client", "https://app.example/cb",
                                               "openid profile", "n1", challenge, "S256",
                                               99, &err);
    TEST_ASSERT_NOT_NULL(code);
    TEST_ASSERT_NULL(err);

    OIDCAuthCodeRecord rec;
    memset(&rec, 0, sizeof(rec));
    TEST_ASSERT_TRUE(oidc_auth_code_consume(ctx->auth_code_store, code, "test-client",
                                            "https://app.example/cb", verifier, &rec));
    TEST_ASSERT_EQUAL_INT(99, rec.account_id);

    free(code);
    free(challenge);
    shutdown_oidc_service();

    char path[512];
    snprintf(path, sizeof(path), "%s/signing-active.pem", dir);
    unlink(path);
    snprintf(path, sizeof(path), "%s/signing-active.kid", dir);
    unlink(path);
    rmdir(dir);
}

void test_issue_code_bad_redirect(void) {
    char tmpl[] = "/tmp/oidc_auth_bad_XXXXXX";
    char *dir = mkdtemp(tmpl);
    TEST_ASSERT_NOT_NULL(dir);

    OIDCConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.enabled = true;
    cfg.issuer = (char*)"http://localhost";
    cfg.keys.storage_path = dir;
    cfg.keys.rotation_interval_days = 90;
    cfg.tokens.access_token_lifetime = 3600;
    cfg.tokens.refresh_token_lifetime = 86400;
    cfg.tokens.id_token_lifetime = 3600;

    TEST_ASSERT_TRUE(init_oidc_service(&cfg));
    OIDCContext *ctx = get_oidc_context();
    OIDCClient *client = oidc_client_create(
        "c2", NULL, "T", false, true,
        "[\"https://good.example/cb\"]", "authorization_code", "code");
    TEST_ASSERT_TRUE(oidc_client_registry_add((OIDCClientContext*)ctx->client_context, client));

    char *challenge = oidc_pkce_make_challenge_s256("v");
    const char *err = NULL;
    char *code = oidc_issue_authorization_code("c2", "https://evil.example/cb",
                                               "openid", NULL, challenge, "S256", 1, &err);
    TEST_ASSERT_NULL(code);
    TEST_ASSERT_EQUAL_STRING("unauthorized_client", err);

    free(challenge);
    shutdown_oidc_service();
    char path[512];
    snprintf(path, sizeof(path), "%s/signing-active.pem", dir);
    unlink(path);
    snprintf(path, sizeof(path), "%s/signing-active.kid", dir);
    unlink(path);
    rmdir(dir);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_issue_code_requires_init);
    RUN_TEST(test_issue_code_happy_path);
    RUN_TEST(test_issue_code_bad_redirect);
    return UNITY_END();
}
