/*
 * Unity Test File: oidc_validate_access_token + userinfo path
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/oidc/oidc_service.h>
#include <src/oidc/oidc_tokens.h>
#include <src/oidc/oidc_pkce.h>
#include <src/config/config_oidc.h>

#include <jansson.h>
#include <string.h>
#include <unistd.h>

void test_validate_null(void);
void test_validate_happy_and_userinfo(void);
void test_validate_rejects_id_token(void);
void test_validate_tampered(void);

void setUp(void) {
}

void tearDown(void) {
    shutdown_oidc_service();
}

void test_validate_null(void) {
    TEST_ASSERT_FALSE(oidc_validate_access_token(NULL, "x", NULL));
}

void test_validate_happy_and_userinfo(void) {
    char tmpl[] = "/tmp/oidc_val_ok_XXXXXX";
    char *dir = mkdtemp(tmpl);
    TEST_ASSERT_NOT_NULL(dir);

    OIDCConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.enabled = true;
    cfg.issuer = (char*)"http://issuer.test";
    cfg.keys.storage_path = dir;
    cfg.keys.encryption_enabled = false;
    cfg.keys.rotation_interval_days = 90;
    cfg.tokens.access_token_lifetime = 3600;
    cfg.tokens.refresh_token_lifetime = 86400;
    cfg.tokens.id_token_lifetime = 3600;
    TEST_ASSERT_TRUE(init_oidc_service(&cfg));

    OIDCContext *ctx = get_oidc_context();
    OIDCClient *client = oidc_client_create(
        "ui-client", NULL, "T", false, true,
        "[\"https://app.example/cb\"]", "authorization_code", "code");
    TEST_ASSERT_TRUE(oidc_client_registry_add((OIDCClientContext*)ctx->client_context, client));

    const char *verifier = "userinfo-verifier-abcdefghijklmnopqrstuv";
    char *challenge = oidc_pkce_make_challenge_s256(verifier);
    char *code = oidc_issue_authorization_code("ui-client", "https://app.example/cb",
                                               "openid email", "n", challenge, "S256", 55, NULL);
    TEST_ASSERT_NOT_NULL(code);

    char *tok_resp = oidc_process_token_request("authorization_code", code,
                                                "https://app.example/cb", "ui-client",
                                                NULL, NULL, verifier);
    TEST_ASSERT_NOT_NULL(tok_resp);
    json_error_t err;
    json_t *root = json_loads(tok_resp, 0, &err);
    const char *access = json_string_value(json_object_get(root, "access_token"));
    TEST_ASSERT_NOT_NULL(access);

    OIDCTokenClaims *claims = NULL;
    TEST_ASSERT_TRUE(oidc_validate_access_token((OIDCTokenContext*)ctx->token_context,
                                                access, &claims));
    TEST_ASSERT_NOT_NULL(claims);
    TEST_ASSERT_EQUAL_STRING("55", claims->sub);
    TEST_ASSERT_EQUAL_STRING("http://issuer.test", claims->iss);
    oidc_free_token_claims(claims);

    char *ui = oidc_process_userinfo_request(access);
    TEST_ASSERT_NOT_NULL(ui);
    json_t *uj = json_loads(ui, 0, &err);
    TEST_ASSERT_EQUAL_STRING("55", json_string_value(json_object_get(uj, "sub")));
    json_decref(uj);
    free(ui);

    json_decref(root);
    free(tok_resp);
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

void test_validate_rejects_id_token(void) {
    char tmpl[] = "/tmp/oidc_val_id_XXXXXX";
    char *dir = mkdtemp(tmpl);
    TEST_ASSERT_NOT_NULL(dir);

    OIDCConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.enabled = true;
    cfg.issuer = (char*)"http://iss";
    cfg.keys.storage_path = dir;
    cfg.keys.rotation_interval_days = 90;
    cfg.tokens.access_token_lifetime = 3600;
    cfg.tokens.refresh_token_lifetime = 86400;
    cfg.tokens.id_token_lifetime = 3600;
    TEST_ASSERT_TRUE(init_oidc_service(&cfg));

    OIDCContext *ctx = get_oidc_context();
    OIDCTokenClaims *c = oidc_create_token_claims("http://iss", "1", "cli");
    char *id_tok = oidc_generate_id_token((OIDCTokenContext*)ctx->token_context, c);
    TEST_ASSERT_NOT_NULL(id_tok);
    TEST_ASSERT_FALSE(oidc_validate_access_token((OIDCTokenContext*)ctx->token_context,
                                                 id_tok, NULL));
    free(id_tok);
    oidc_free_token_claims(c);
    shutdown_oidc_service();

    char path[512];
    snprintf(path, sizeof(path), "%s/signing-active.pem", dir);
    unlink(path);
    snprintf(path, sizeof(path), "%s/signing-active.kid", dir);
    unlink(path);
    rmdir(dir);
}

void test_validate_tampered(void) {
    char tmpl[] = "/tmp/oidc_val_tam_XXXXXX";
    char *dir = mkdtemp(tmpl);
    TEST_ASSERT_NOT_NULL(dir);

    OIDCConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.enabled = true;
    cfg.issuer = (char*)"http://iss";
    cfg.keys.storage_path = dir;
    cfg.keys.rotation_interval_days = 90;
    cfg.tokens.access_token_lifetime = 3600;
    cfg.tokens.refresh_token_lifetime = 86400;
    cfg.tokens.id_token_lifetime = 3600;
    TEST_ASSERT_TRUE(init_oidc_service(&cfg));

    OIDCContext *ctx = get_oidc_context();
    OIDCTokenClaims *c = oidc_create_token_claims("http://iss", "9", "cli");
    char *access = oidc_generate_access_token((OIDCTokenContext*)ctx->token_context, c, NULL);
    TEST_ASSERT_NOT_NULL(access);

    /* Corrupt signature mid-segment (last char can be base64-equivalent under padding) */
    size_t len = strlen(access);
    char *bad = strdup(access);
    TEST_ASSERT_NOT_NULL(bad);
    TEST_ASSERT_TRUE(len > 10U);
    size_t flip = len - 5U;
    if (bad[flip] == 'A') {
        bad[flip] = 'B';
    } else {
        bad[flip] = 'A';
    }
    TEST_ASSERT_FALSE(oidc_validate_access_token((OIDCTokenContext*)ctx->token_context,
                                                 bad, NULL));
    free(bad);
    free(access);
    oidc_free_token_claims(c);
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
    RUN_TEST(test_validate_null);
    RUN_TEST(test_validate_happy_and_userinfo);
    RUN_TEST(test_validate_rejects_id_token);
    RUN_TEST(test_validate_tampered);
    return UNITY_END();
}
