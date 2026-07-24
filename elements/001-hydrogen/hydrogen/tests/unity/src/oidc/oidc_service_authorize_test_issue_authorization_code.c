/*
 * Unity Test File: oidc_issue_authorization_code (with live init)
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/oidc/oidc_service.h>
#include <src/oidc/oidc_pkce.h>
#include <src/config/config_oidc.h>
#include <unity/mocks/mock_system.h>

#include <string.h>
#include <unistd.h>

void test_issue_code_requires_init(void);
void test_issue_code_happy_path(void);
void test_issue_code_bad_redirect(void);
void test_issue_code_invalid_request_params(void);
void test_issue_code_auth_code_issue_failure(void);

static void cleanup_key_dir(const char *dir) {
    char path[512];
    snprintf(path, sizeof(path), "%s/signing-active.pem", dir);
    unlink(path);
    snprintf(path, sizeof(path), "%s/signing-active.kid", dir);
    unlink(path);
    rmdir(dir);
}

static bool init_with_tmp(char *tmpl, char **dir_out) {
    char *dir = mkdtemp(tmpl);
    if (!dir) {
        return false;
    }
    *dir_out = dir;

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
    return init_oidc_service(&cfg);
}

void setUp(void) {
    mock_system_reset_all();
}

void tearDown(void) {
    shutdown_oidc_service();
    mock_system_reset_all();
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
    char *dir = NULL;
    TEST_ASSERT_TRUE(init_with_tmp(tmpl, &dir));

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
    cleanup_key_dir(dir);
}

void test_issue_code_bad_redirect(void) {
    char tmpl[] = "/tmp/oidc_auth_bad_XXXXXX";
    char *dir = NULL;
    TEST_ASSERT_TRUE(init_with_tmp(tmpl, &dir));
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
    cleanup_key_dir(dir);
}

void test_issue_code_invalid_request_params(void) {
    char tmpl[] = "/tmp/oidc_auth_inv_XXXXXX";
    char *dir = NULL;
    TEST_ASSERT_TRUE(init_with_tmp(tmpl, &dir));

    const char *err = NULL;
    TEST_ASSERT_NULL(oidc_issue_authorization_code(NULL, "https://x/cb", "openid", NULL,
                                                   "ch", "S256", 1, &err));
    TEST_ASSERT_EQUAL_STRING("invalid_request", err);

    err = NULL;
    TEST_ASSERT_NULL(oidc_issue_authorization_code("c", NULL, "openid", NULL,
                                                   "ch", "S256", 1, &err));
    TEST_ASSERT_EQUAL_STRING("invalid_request", err);

    err = NULL;
    TEST_ASSERT_NULL(oidc_issue_authorization_code("c", "https://x/cb", "openid", NULL,
                                                   "ch", "S256", 0, &err));
    TEST_ASSERT_EQUAL_STRING("invalid_request", err);

    err = NULL;
    TEST_ASSERT_NULL(oidc_issue_authorization_code("c", "https://x/cb", "openid", NULL,
                                                   NULL, "S256", 1, &err));
    TEST_ASSERT_EQUAL_STRING("invalid_request", err);

    err = NULL;
    TEST_ASSERT_NULL(oidc_issue_authorization_code("c", "https://x/cb", "openid", NULL,
                                                   "ch", "plain", 1, &err));
    TEST_ASSERT_EQUAL_STRING("invalid_request", err);

    /* NULL error_code pointer still returns NULL */
    TEST_ASSERT_NULL(oidc_issue_authorization_code(NULL, "https://x/cb", "openid", NULL,
                                                   "ch", "S256", 1, NULL));

    shutdown_oidc_service();
    cleanup_key_dir(dir);
}

void test_issue_code_auth_code_issue_failure(void) {
    char tmpl[] = "/tmp/oidc_auth_oom_XXXXXX";
    char *dir = NULL;
    TEST_ASSERT_TRUE(init_with_tmp(tmpl, &dir));

    OIDCContext *ctx = get_oidc_context();
    OIDCClient *client = oidc_client_create(
        "oom-client", NULL, "T", false, true,
        "[\"https://app.example/cb\"]", "authorization_code", "code");
    TEST_ASSERT_TRUE(oidc_client_registry_add((OIDCClientContext*)ctx->client_context, client));

    char *challenge = oidc_pkce_make_challenge_s256("verifier-for-oom-path");
    TEST_ASSERT_NOT_NULL(challenge);

    mock_system_set_malloc_failure(1);
    const char *err = NULL;
    char *code = oidc_issue_authorization_code("oom-client", "https://app.example/cb",
                                               "openid", NULL, challenge, "S256", 1, &err);
    TEST_ASSERT_NULL(code);
    TEST_ASSERT_EQUAL_STRING("server_error", err);

    free(challenge);
    shutdown_oidc_service();
    cleanup_key_dir(dir);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_issue_code_requires_init);
    RUN_TEST(test_issue_code_happy_path);
    RUN_TEST(test_issue_code_bad_redirect);
    RUN_TEST(test_issue_code_invalid_request_params);
    RUN_TEST(test_issue_code_auth_code_issue_failure);
    return UNITY_END();
}
