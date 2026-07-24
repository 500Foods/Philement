/*
 * Unity Test File: oidc_generate_jwks_document
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/oidc/oidc_service.h>
#include <src/config/config_oidc.h>

#include <jansson.h>
#include <unistd.h>

void test_jwks_doc_not_init(void);
void test_jwks_doc_has_keys(void);

void setUp(void) {
}

void tearDown(void) {
    shutdown_oidc_service();
}

void test_jwks_doc_not_init(void) {
    TEST_ASSERT_NULL(oidc_generate_jwks_document());
}

void test_jwks_doc_has_keys(void) {
    char tmpl[] = "/tmp/oidc_jwks_doc_XXXXXX";
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

    char *jwks = oidc_generate_jwks_document();
    TEST_ASSERT_NOT_NULL(jwks);
    json_error_t err;
    json_t *root = json_loads(jwks, 0, &err);
    TEST_ASSERT_NOT_NULL(root);
    json_t *keys = json_object_get(root, "keys");
    TEST_ASSERT_TRUE(json_is_array(keys));
    TEST_ASSERT_TRUE(json_array_size(keys) >= 1);

    json_decref(root);
    free(jwks);
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
    RUN_TEST(test_jwks_doc_not_init);
    RUN_TEST(test_jwks_doc_has_keys);
    return UNITY_END();
}
