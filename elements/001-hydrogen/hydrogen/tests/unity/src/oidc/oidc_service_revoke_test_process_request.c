/*
 * Unity Test File: oidc_process_revocation_request (RFC 7009)
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/oidc/oidc_service.h>
#include <src/oidc/oidc_pkce.h>
#include <src/config/config_oidc.h>

#include <jansson.h>
#include <string.h>
#include <unistd.h>

void test_revoke_not_init(void);
void test_revoke_bad_client(void);
void test_revoke_refresh_then_refresh_fails(void);
void test_revoke_unknown_ok(void);

void setUp(void) {
}

void tearDown(void) {
    shutdown_oidc_service();
}

static void cleanup_keys(const char *dir) {
    char path[512];
    snprintf(path, sizeof(path), "%s/signing-active.pem", dir);
    unlink(path);
    snprintf(path, sizeof(path), "%s/signing-active.kid", dir);
    unlink(path);
    rmdir(dir);
}

static bool init_with_client(char *dir) {
    OIDCConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.enabled = true;
    cfg.issuer = (char*)"http://localhost:5450";
    cfg.keys.storage_path = dir;
    cfg.keys.encryption_enabled = false;
    cfg.keys.rotation_interval_days = 90;
    cfg.tokens.access_token_lifetime = 3600;
    cfg.tokens.refresh_token_lifetime = 7200;
    cfg.tokens.id_token_lifetime = 3600;
    if (!init_oidc_service(&cfg)) {
        return false;
    }
    OIDCContext *ctx = get_oidc_context();
    OIDCClient *client = oidc_client_create(
        "rev-cli", NULL, "T", false, true,
        "[\"https://app.example/cb\"]",
        "authorization_code refresh_token", "code");
    return oidc_client_registry_add((OIDCClientContext*)ctx->client_context, client);
}

void test_revoke_not_init(void) {
    TEST_ASSERT_FALSE(oidc_process_revocation_request("t", NULL, "c", NULL));
}

void test_revoke_bad_client(void) {
    char tmpl[] = "/tmp/oidc_rev_bc_XXXXXX";
    char *dir = mkdtemp(tmpl);
    TEST_ASSERT_NOT_NULL(dir);
    TEST_ASSERT_TRUE(init_with_client(dir));
    TEST_ASSERT_FALSE(oidc_process_revocation_request("t", NULL, "nope", NULL));
    shutdown_oidc_service();
    cleanup_keys(dir);
}

void test_revoke_refresh_then_refresh_fails(void) {
    char tmpl[] = "/tmp/oidc_rev_rt_XXXXXX";
    char *dir = mkdtemp(tmpl);
    TEST_ASSERT_NOT_NULL(dir);
    TEST_ASSERT_TRUE(init_with_client(dir));

    const char *verifier = "revoke-refresh-verifier-abcdefghijkl";
    char *challenge = oidc_pkce_make_challenge_s256(verifier);
    char *code = oidc_issue_authorization_code("rev-cli", "https://app.example/cb",
                                               "openid offline_access", NULL,
                                               challenge, "S256", 11, NULL);
    TEST_ASSERT_NOT_NULL(code);
    char *tok = oidc_process_token_request("authorization_code", code,
                                           "https://app.example/cb", "rev-cli",
                                           NULL, NULL, verifier);
    TEST_ASSERT_NOT_NULL(tok);
    json_error_t err;
    json_t *root = json_loads(tok, 0, &err);
    const char *refresh = json_string_value(json_object_get(root, "refresh_token"));
    TEST_ASSERT_NOT_NULL(refresh);
    char *refresh_copy = strdup(refresh);
    TEST_ASSERT_NOT_NULL(refresh_copy);

    TEST_ASSERT_TRUE(oidc_process_revocation_request(refresh_copy, "refresh_token",
                                                     "rev-cli", NULL));

    char *tok2 = oidc_process_token_request("refresh_token", NULL, NULL, "rev-cli",
                                            NULL, refresh_copy, NULL);
    TEST_ASSERT_NOT_NULL(tok2);
    TEST_ASSERT_NOT_NULL(strstr(tok2, "invalid_grant"));

    json_decref(root);
    free(tok2);
    free(refresh_copy);
    free(tok);
    free(code);
    free(challenge);
    shutdown_oidc_service();
    cleanup_keys(dir);
}

void test_revoke_unknown_ok(void) {
    char tmpl[] = "/tmp/oidc_rev_unk_XXXXXX";
    char *dir = mkdtemp(tmpl);
    TEST_ASSERT_NOT_NULL(dir);
    TEST_ASSERT_TRUE(init_with_client(dir));
    TEST_ASSERT_TRUE(oidc_process_revocation_request("unknown-token", NULL,
                                                     "rev-cli", NULL));
    shutdown_oidc_service();
    cleanup_keys(dir);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_revoke_not_init);
    RUN_TEST(test_revoke_bad_client);
    RUN_TEST(test_revoke_refresh_then_refresh_fails);
    RUN_TEST(test_revoke_unknown_ok);
    return UNITY_END();
}
