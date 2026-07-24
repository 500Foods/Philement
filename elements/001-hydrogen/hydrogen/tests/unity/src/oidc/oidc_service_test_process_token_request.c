/*
 * Unity Test File: oidc_process_token_request (authorization_code grant)
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/oidc/oidc_service.h>
#include <src/oidc/oidc_pkce.h>
#include <src/config/config_oidc.h>
#include <src/utils/utils_crypto.h>

#include <jansson.h>
#include <string.h>
#include <unistd.h>

void test_token_not_initialized(void);
void test_token_unsupported_grant(void);
void test_token_code_exchange_happy(void);
void test_token_code_reuse_fails(void);

void setUp(void) {
}

void tearDown(void) {
    shutdown_oidc_service();
}

void test_token_not_initialized(void) {
    char *r = oidc_process_token_request("authorization_code", "c", "https://x",
                                         "cli", NULL, NULL, "v");
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_NOT_NULL(strstr(r, "server_error"));
    free(r);
}

void test_token_unsupported_grant(void) {
    char tmpl[] = "/tmp/oidc_tok_ug_XXXXXX";
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

    char *r = oidc_process_token_request("password", NULL, NULL, "c", NULL, NULL, NULL);
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_NOT_NULL(strstr(r, "unsupported_grant_type"));
    free(r);

    shutdown_oidc_service();
    char path[512];
    snprintf(path, sizeof(path), "%s/signing-active.pem", dir);
    unlink(path);
    snprintf(path, sizeof(path), "%s/signing-active.kid", dir);
    unlink(path);
    rmdir(dir);
}

void test_token_code_exchange_happy(void) {
    char tmpl[] = "/tmp/oidc_tok_ok_XXXXXX";
    char *dir = mkdtemp(tmpl);
    TEST_ASSERT_NOT_NULL(dir);

    OIDCConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.enabled = true;
    cfg.issuer = (char*)"http://localhost:5450";
    cfg.keys.storage_path = dir;
    cfg.keys.encryption_enabled = false;
    cfg.keys.rotation_interval_days = 90;
    cfg.tokens.access_token_lifetime = 1800;
    cfg.tokens.refresh_token_lifetime = 86400;
    cfg.tokens.id_token_lifetime = 900;
    TEST_ASSERT_TRUE(init_oidc_service(&cfg));

    OIDCContext *ctx = get_oidc_context();
    OIDCClient *client = oidc_client_create(
        "tok-client", NULL, "Test", false, true,
        "[\"https://app.example/cb\"]",
        "authorization_code", "code");
    TEST_ASSERT_TRUE(oidc_client_registry_add((OIDCClientContext*)ctx->client_context, client));

    const char *verifier = "phase9-token-verifier-abcdefghijklmnopqrst";
    char *challenge = oidc_pkce_make_challenge_s256(verifier);
    TEST_ASSERT_NOT_NULL(challenge);

    const char *err = NULL;
    char *code = oidc_issue_authorization_code("tok-client", "https://app.example/cb",
                                               "openid profile", "nonce-xyz",
                                               challenge, "S256", 77, &err);
    TEST_ASSERT_NOT_NULL(code);
    TEST_ASSERT_NULL(err);

    char *resp = oidc_process_token_request("authorization_code", code,
                                            "https://app.example/cb", "tok-client",
                                            NULL, NULL, verifier);
    TEST_ASSERT_NOT_NULL(resp);
    TEST_ASSERT_NULL(strstr(resp, "\"error\""));

    json_error_t jerr;
    json_t *root = json_loads(resp, 0, &jerr);
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_NOT_NULL(json_string_value(json_object_get(root, "access_token")));
    TEST_ASSERT_NOT_NULL(json_string_value(json_object_get(root, "id_token")));
    TEST_ASSERT_EQUAL_STRING("Bearer", json_string_value(json_object_get(root, "token_type")));
    TEST_ASSERT_EQUAL_INT(1800, (int)json_integer_value(json_object_get(root, "expires_in")));

    const char *id_token = json_string_value(json_object_get(root, "id_token"));
    TEST_ASSERT_NOT_NULL(id_token);

    /* Verify RS256 signature on id_token */
    char *dot1 = strchr(id_token, '.');
    char *dot2 = strchr(dot1 + 1, '.');
    size_t hlen = (size_t)(dot1 - id_token);
    size_t plen = (size_t)(dot2 - dot1 - 1);
    char *hb64 = (char*)malloc(hlen + 1U);
    char *pb64 = (char*)malloc(plen + 1U);
    TEST_ASSERT_NOT_NULL(hb64);
    TEST_ASSERT_NOT_NULL(pb64);
    memcpy(hb64, id_token, hlen);
    hb64[hlen] = '\0';
    memcpy(pb64, dot1 + 1, plen);
    pb64[plen] = '\0';
    char *si = NULL;
    TEST_ASSERT_TRUE(asprintf(&si, "%s.%s", hb64, pb64) >= 0);
    size_t sig_len = 0;
    unsigned char *sig = utils_base64url_decode(dot2 + 1, &sig_len);
    OIDCKey *key = oidc_get_active_signing_key((OIDCKeyContext*)ctx->key_context);
    TEST_ASSERT_TRUE(utils_rs256_verify((EVP_PKEY*)key->key_data,
                                        (const unsigned char*)si, strlen(si),
                                        sig, sig_len));

    size_t praw_len = 0;
    unsigned char *praw = utils_base64url_decode(pb64, &praw_len);
    json_t *pl = json_loadb((const char*)praw, praw_len, 0, &jerr);
    TEST_ASSERT_EQUAL_STRING("77", json_string_value(json_object_get(pl, "sub")));
    TEST_ASSERT_EQUAL_STRING("nonce-xyz", json_string_value(json_object_get(pl, "nonce")));
    TEST_ASSERT_EQUAL_STRING("http://localhost:5450", json_string_value(json_object_get(pl, "iss")));

    json_decref(pl);
    free(praw);
    free(sig);
    free(si);
    free(hb64);
    free(pb64);
    json_decref(root);
    free(resp);
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

void test_token_code_reuse_fails(void) {
    char tmpl[] = "/tmp/oidc_tok_reuse_XXXXXX";
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
        "c-reuse", NULL, "T", false, true,
        "[\"https://app.example/cb\"]", "authorization_code", "code");
    TEST_ASSERT_TRUE(oidc_client_registry_add((OIDCClientContext*)ctx->client_context, client));

    const char *verifier = "reuse-verifier-abcdefghijklmnopqrstuvw";
    char *challenge = oidc_pkce_make_challenge_s256(verifier);
    char *code = oidc_issue_authorization_code("c-reuse", "https://app.example/cb",
                                               "openid", NULL, challenge, "S256", 1, NULL);
    TEST_ASSERT_NOT_NULL(code);

    char *r1 = oidc_process_token_request("authorization_code", code,
                                          "https://app.example/cb", "c-reuse",
                                          NULL, NULL, verifier);
    TEST_ASSERT_NOT_NULL(r1);
    TEST_ASSERT_NULL(strstr(r1, "\"error\""));
    free(r1);

    char *r2 = oidc_process_token_request("authorization_code", code,
                                          "https://app.example/cb", "c-reuse",
                                          NULL, NULL, verifier);
    TEST_ASSERT_NOT_NULL(r2);
    TEST_ASSERT_NOT_NULL(strstr(r2, "invalid_grant"));
    free(r2);

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

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_token_not_initialized);
    RUN_TEST(test_token_unsupported_grant);
    RUN_TEST(test_token_code_exchange_happy);
    RUN_TEST(test_token_code_reuse_fails);
    return UNITY_END();
}
