/*
 * Unity Test File: oidc_generate_discovery_document
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/oidc/oidc_service.h>
#include <src/config/config_oidc.h>

#include <jansson.h>
#include <string.h>
#include <unistd.h>

void test_discovery_not_init(void);
void test_discovery_shape(void);

void setUp(void) {
}

void tearDown(void) {
    shutdown_oidc_service();
}

void test_discovery_not_init(void) {
    TEST_ASSERT_NULL(oidc_generate_discovery_document());
}

void test_discovery_shape(void) {
    char tmpl[] = "/tmp/oidc_disc_XXXXXX";
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

    char *doc = oidc_generate_discovery_document();
    TEST_ASSERT_NOT_NULL(doc);

    json_error_t err;
    json_t *root = json_loads(doc, 0, &err);
    TEST_ASSERT_NOT_NULL(root);
    TEST_ASSERT_EQUAL_STRING("http://localhost:5450",
                             json_string_value(json_object_get(root, "issuer")));
    TEST_ASSERT_NOT_NULL(strstr(json_string_value(json_object_get(root, "authorization_endpoint")),
                                "/oauth/authorize"));
    TEST_ASSERT_NOT_NULL(strstr(json_string_value(json_object_get(root, "token_endpoint")),
                                "/oauth/token"));
    TEST_ASSERT_NOT_NULL(strstr(json_string_value(json_object_get(root, "jwks_uri")),
                                "/oauth/jwks"));
    TEST_ASSERT_NOT_NULL(strstr(json_string_value(json_object_get(root, "introspection_endpoint")),
                                "/oauth/introspect"));
    TEST_ASSERT_NOT_NULL(strstr(json_string_value(json_object_get(root, "revocation_endpoint")),
                                "/oauth/revoke"));
    TEST_ASSERT_NOT_NULL(json_object_get(root, "code_challenge_methods_supported"));

    json_decref(root);
    free(doc);
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
    RUN_TEST(test_discovery_not_init);
    RUN_TEST(test_discovery_shape);
    return UNITY_END();
}
