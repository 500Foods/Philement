/*
 * Unity Test File: refresh_token grant via oidc_process_token_request
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/oidc/oidc_service.h>
#include <src/oidc/oidc_pkce.h>
#include <src/config/config_oidc.h>

#include <jansson.h>
#include <string.h>
#include <unistd.h>

void test_refresh_grant_happy(void);
void test_refresh_grant_reuse_fails(void);

void setUp(void) {
}

void tearDown(void) {
    shutdown_oidc_service();
}

void test_refresh_grant_happy(void) {
    char tmpl[] = "/tmp/oidc_ref_ok_XXXXXX";
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
    cfg.tokens.refresh_token_lifetime = 7200;
    cfg.tokens.id_token_lifetime = 3600;
    TEST_ASSERT_TRUE(init_oidc_service(&cfg));

    OIDCContext *ctx = get_oidc_context();
    OIDCClient *client = oidc_client_create(
        "ref-client", NULL, "T", false, true,
        "[\"https://app.example/cb\"]",
        "authorization_code refresh_token", "code");
    TEST_ASSERT_TRUE(oidc_client_registry_add((OIDCClientContext*)ctx->client_context, client));

    const char *verifier = "refresh-grant-verifier-abcdefghijklmnop";
    char *challenge = oidc_pkce_make_challenge_s256(verifier);
    char *code = oidc_issue_authorization_code("ref-client", "https://app.example/cb",
                                               "openid offline_access", NULL,
                                               challenge, "S256", 3, NULL);
    TEST_ASSERT_NOT_NULL(code);

    char *tok1 = oidc_process_token_request("authorization_code", code,
                                            "https://app.example/cb", "ref-client",
                                            NULL, NULL, verifier);
    TEST_ASSERT_NOT_NULL(tok1);
    json_error_t err;
    json_t *r1 = json_loads(tok1, 0, &err);
    const char *refresh = json_string_value(json_object_get(r1, "refresh_token"));
    TEST_ASSERT_NOT_NULL(refresh);
    char *refresh_copy = strdup(refresh);
    TEST_ASSERT_NOT_NULL(refresh_copy);

    char *tok2 = oidc_process_token_request("refresh_token", NULL, NULL, "ref-client",
                                            NULL, refresh_copy, NULL);
    TEST_ASSERT_NOT_NULL(tok2);
    TEST_ASSERT_NULL(strstr(tok2, "\"error\""));
    json_t *r2 = json_loads(tok2, 0, &err);
    TEST_ASSERT_NOT_NULL(json_string_value(json_object_get(r2, "access_token")));
    TEST_ASSERT_NOT_NULL(json_string_value(json_object_get(r2, "refresh_token")));
    TEST_ASSERT_TRUE(strcmp(refresh_copy,
                            json_string_value(json_object_get(r2, "refresh_token"))) != 0);

    json_decref(r1);
    json_decref(r2);
    free(refresh_copy);
    free(tok1);
    free(tok2);
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

void test_refresh_grant_reuse_fails(void) {
    char tmpl[] = "/tmp/oidc_ref_reuse_XXXXXX";
    char *dir = mkdtemp(tmpl);
    TEST_ASSERT_NOT_NULL(dir);

    OIDCConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.enabled = true;
    cfg.issuer = (char*)"http://iss";
    cfg.keys.storage_path = dir;
    cfg.keys.rotation_interval_days = 90;
    cfg.tokens.access_token_lifetime = 3600;
    cfg.tokens.refresh_token_lifetime = 7200;
    cfg.tokens.id_token_lifetime = 3600;
    TEST_ASSERT_TRUE(init_oidc_service(&cfg));

    OIDCContext *ctx = get_oidc_context();
    OIDCClient *client = oidc_client_create(
        "r2", NULL, "T", false, true,
        "[\"https://app.example/cb\"]",
        "authorization_code refresh_token", "code");
    TEST_ASSERT_TRUE(oidc_client_registry_add((OIDCClientContext*)ctx->client_context, client));

    const char *verifier = "reuse-ref-verifier-abcdefghijklmnopqrst";
    char *challenge = oidc_pkce_make_challenge_s256(verifier);
    char *code = oidc_issue_authorization_code("r2", "https://app.example/cb",
                                               "openid", NULL, challenge, "S256", 1, NULL);
    char *tok1 = oidc_process_token_request("authorization_code", code,
                                            "https://app.example/cb", "r2",
                                            NULL, NULL, verifier);
    json_error_t err;
    json_t *r1 = json_loads(tok1, 0, &err);
    char *refresh = strdup(json_string_value(json_object_get(r1, "refresh_token")));

    char *ok = oidc_process_token_request("refresh_token", NULL, NULL, "r2",
                                          NULL, refresh, NULL);
    TEST_ASSERT_NULL(strstr(ok, "\"error\""));

    char *bad = oidc_process_token_request("refresh_token", NULL, NULL, "r2",
                                           NULL, refresh, NULL);
    TEST_ASSERT_NOT_NULL(strstr(bad, "invalid_grant"));

    free(ok);
    free(bad);
    free(refresh);
    json_decref(r1);
    free(tok1);
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
    RUN_TEST(test_refresh_grant_happy);
    RUN_TEST(test_refresh_grant_reuse_fails);
    return UNITY_END();
}
