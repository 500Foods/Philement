/*
 * Unity Test File: oidc_process_introspection_request (RFC 7662)
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/oidc/oidc_service.h>
#include <src/oidc/oidc_pkce.h>
#include <src/config/config_oidc.h>

#include <jansson.h>
#include <string.h>
#include <unistd.h>

void test_introspect_not_init(void);
void test_introspect_bad_client(void);
void test_introspect_access_active(void);
void test_introspect_refresh_active_and_revoked(void);
void test_introspect_unknown(void);

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

static bool init_with_client(char *dir, const char *client_id) {
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
        client_id, NULL, "T", false, true,
        "[\"https://app.example/cb\"]",
        "authorization_code refresh_token", "code");
    return oidc_client_registry_add((OIDCClientContext*)ctx->client_context, client);
}

void test_introspect_not_init(void) {
    char *r = oidc_process_introspection_request("tok", NULL, "cli", NULL);
    TEST_ASSERT_NULL(r);
}

void test_introspect_bad_client(void) {
    char tmpl[] = "/tmp/oidc_intro_bc_XXXXXX";
    char *dir = mkdtemp(tmpl);
    TEST_ASSERT_NOT_NULL(dir);
    TEST_ASSERT_TRUE(init_with_client(dir, "intro-cli"));

    char *r = oidc_process_introspection_request("anything", NULL, "no-such", NULL);
    TEST_ASSERT_NULL(r);

    shutdown_oidc_service();
    cleanup_keys(dir);
}

void test_introspect_access_active(void) {
    char tmpl[] = "/tmp/oidc_intro_at_XXXXXX";
    char *dir = mkdtemp(tmpl);
    TEST_ASSERT_NOT_NULL(dir);
    TEST_ASSERT_TRUE(init_with_client(dir, "intro-cli"));

    const char *verifier = "introspect-access-verifier-abcdefghij";
    char *challenge = oidc_pkce_make_challenge_s256(verifier);
    char *code = oidc_issue_authorization_code("intro-cli", "https://app.example/cb",
                                               "openid", NULL, challenge, "S256", 5, NULL);
    TEST_ASSERT_NOT_NULL(code);
    char *tok = oidc_process_token_request("authorization_code", code,
                                           "https://app.example/cb", "intro-cli",
                                           NULL, NULL, verifier);
    TEST_ASSERT_NOT_NULL(tok);
    json_error_t err;
    json_t *root = json_loads(tok, 0, &err);
    const char *access = json_string_value(json_object_get(root, "access_token"));
    TEST_ASSERT_NOT_NULL(access);

    char *resp = oidc_process_introspection_request(access, "access_token", "intro-cli", NULL);
    TEST_ASSERT_NOT_NULL(resp);
    json_t *ir = json_loads(resp, 0, &err);
    TEST_ASSERT_TRUE(json_is_true(json_object_get(ir, "active")));
    TEST_ASSERT_EQUAL_STRING("access_token",
                             json_string_value(json_object_get(ir, "token_type")));
    TEST_ASSERT_EQUAL_STRING("5", json_string_value(json_object_get(ir, "sub")));

    json_decref(ir);
    json_decref(root);
    free(resp);
    free(tok);
    free(code);
    free(challenge);
    shutdown_oidc_service();
    cleanup_keys(dir);
}

void test_introspect_refresh_active_and_revoked(void) {
    char tmpl[] = "/tmp/oidc_intro_rt_XXXXXX";
    char *dir = mkdtemp(tmpl);
    TEST_ASSERT_NOT_NULL(dir);
    TEST_ASSERT_TRUE(init_with_client(dir, "intro-cli"));

    const char *verifier = "introspect-refresh-verifier-abcdefgh";
    char *challenge = oidc_pkce_make_challenge_s256(verifier);
    char *code = oidc_issue_authorization_code("intro-cli", "https://app.example/cb",
                                               "openid offline_access", NULL,
                                               challenge, "S256", 9, NULL);
    TEST_ASSERT_NOT_NULL(code);
    char *tok = oidc_process_token_request("authorization_code", code,
                                           "https://app.example/cb", "intro-cli",
                                           NULL, NULL, verifier);
    TEST_ASSERT_NOT_NULL(tok);
    json_error_t err;
    json_t *root = json_loads(tok, 0, &err);
    const char *refresh = json_string_value(json_object_get(root, "refresh_token"));
    TEST_ASSERT_NOT_NULL(refresh);
    char *refresh_copy = strdup(refresh);
    TEST_ASSERT_NOT_NULL(refresh_copy);

    char *resp = oidc_process_introspection_request(refresh_copy, "refresh_token",
                                                    "intro-cli", NULL);
    TEST_ASSERT_NOT_NULL(resp);
    json_t *ir = json_loads(resp, 0, &err);
    TEST_ASSERT_TRUE(json_is_true(json_object_get(ir, "active")));
    TEST_ASSERT_EQUAL_STRING("refresh_token",
                             json_string_value(json_object_get(ir, "token_type")));
    TEST_ASSERT_EQUAL_STRING("9", json_string_value(json_object_get(ir, "sub")));
    json_decref(ir);
    free(resp);

    TEST_ASSERT_TRUE(oidc_process_revocation_request(refresh_copy, "refresh_token",
                                                     "intro-cli", NULL));

    resp = oidc_process_introspection_request(refresh_copy, "refresh_token",
                                              "intro-cli", NULL);
    TEST_ASSERT_NOT_NULL(resp);
    ir = json_loads(resp, 0, &err);
    TEST_ASSERT_TRUE(json_is_false(json_object_get(ir, "active")));

    json_decref(ir);
    json_decref(root);
    free(resp);
    free(refresh_copy);
    free(tok);
    free(code);
    free(challenge);
    shutdown_oidc_service();
    cleanup_keys(dir);
}

void test_introspect_unknown(void) {
    char tmpl[] = "/tmp/oidc_intro_unk_XXXXXX";
    char *dir = mkdtemp(tmpl);
    TEST_ASSERT_NOT_NULL(dir);
    TEST_ASSERT_TRUE(init_with_client(dir, "intro-cli"));

    char *resp = oidc_process_introspection_request("not-a-real-token", NULL,
                                                    "intro-cli", NULL);
    TEST_ASSERT_NOT_NULL(resp);
    TEST_ASSERT_NOT_NULL(strstr(resp, "\"active\":false"));
    free(resp);

    shutdown_oidc_service();
    cleanup_keys(dir);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_introspect_not_init);
    RUN_TEST(test_introspect_bad_client);
    RUN_TEST(test_introspect_access_active);
    RUN_TEST(test_introspect_refresh_active_and_revoked);
    RUN_TEST(test_introspect_unknown);
    return UNITY_END();
}
