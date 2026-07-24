/*
 * Unity Test File: oidc_process_userinfo_request
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/oidc/oidc_service.h>
#include <src/config/config_oidc.h>

#include <jansson.h>
#include <string.h>
#include <unistd.h>

void test_userinfo_not_init(void);
void test_userinfo_null_token(void);
void test_userinfo_invalid_token(void);
void test_userinfo_happy_sub(void);

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

void test_userinfo_not_init(void) {
    TEST_ASSERT_NULL(oidc_process_userinfo_request("x.y.z"));
}

void test_userinfo_null_token(void) {
    char tmpl[] = "/tmp/oidc_ui_null_XXXXXX";
    char *dir = mkdtemp(tmpl);
    TEST_ASSERT_NOT_NULL(dir);

    OIDCConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.enabled = true;
    cfg.issuer = (char*)"http://iss";
    cfg.keys.storage_path = dir;
    cfg.keys.encryption_enabled = false;
    cfg.keys.rotation_interval_days = 90;
    cfg.tokens.access_token_lifetime = 3600;
    cfg.tokens.refresh_token_lifetime = 86400;
    cfg.tokens.id_token_lifetime = 3600;
    TEST_ASSERT_TRUE(init_oidc_service(&cfg));

    TEST_ASSERT_NULL(oidc_process_userinfo_request(NULL));
    TEST_ASSERT_NULL(oidc_process_userinfo_request(""));

    shutdown_oidc_service();
    cleanup_keys(dir);
}

void test_userinfo_invalid_token(void) {
    char tmpl[] = "/tmp/oidc_ui_bad_XXXXXX";
    char *dir = mkdtemp(tmpl);
    TEST_ASSERT_NOT_NULL(dir);

    OIDCConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.enabled = true;
    cfg.issuer = (char*)"http://iss";
    cfg.keys.storage_path = dir;
    cfg.keys.encryption_enabled = false;
    cfg.keys.rotation_interval_days = 90;
    cfg.tokens.access_token_lifetime = 3600;
    cfg.tokens.refresh_token_lifetime = 86400;
    cfg.tokens.id_token_lifetime = 3600;
    TEST_ASSERT_TRUE(init_oidc_service(&cfg));

    TEST_ASSERT_NULL(oidc_process_userinfo_request("not.a.jwt"));

    shutdown_oidc_service();
    cleanup_keys(dir);
}

void test_userinfo_happy_sub(void) {
    char tmpl[] = "/tmp/oidc_ui_ok_XXXXXX";
    char *dir = mkdtemp(tmpl);
    TEST_ASSERT_NOT_NULL(dir);

    OIDCConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.enabled = true;
    cfg.issuer = (char*)"http://iss";
    cfg.keys.storage_path = dir;
    cfg.keys.encryption_enabled = false;
    cfg.keys.rotation_interval_days = 90;
    cfg.tokens.access_token_lifetime = 3600;
    cfg.tokens.refresh_token_lifetime = 86400;
    cfg.tokens.id_token_lifetime = 3600;
    TEST_ASSERT_TRUE(init_oidc_service(&cfg));

    OIDCContext *ctx = get_oidc_context();
    OIDCTokenClaims *c = oidc_create_token_claims("http://iss", "42", "cli");
    char *access = oidc_generate_access_token((OIDCTokenContext*)ctx->token_context, c, NULL);
    TEST_ASSERT_NOT_NULL(access);

    char *ui = oidc_process_userinfo_request(access);
    TEST_ASSERT_NOT_NULL(ui);
    json_error_t err;
    json_t *root = json_loads(ui, 0, &err);
    TEST_ASSERT_EQUAL_STRING("42", json_string_value(json_object_get(root, "sub")));

    json_decref(root);
    free(ui);
    free(access);
    oidc_free_token_claims(c);
    shutdown_oidc_service();
    cleanup_keys(dir);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_userinfo_not_init);
    RUN_TEST(test_userinfo_null_token);
    RUN_TEST(test_userinfo_invalid_token);
    RUN_TEST(test_userinfo_happy_sub);
    return UNITY_END();
}
