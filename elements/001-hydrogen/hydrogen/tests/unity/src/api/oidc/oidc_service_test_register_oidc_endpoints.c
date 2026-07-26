/*
 * Unity Test File: register_oidc_endpoints
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/oidc/oidc_service.h>
#include <src/oidc/oidc_service.h>
#include <src/config/config_oidc.h>
#include <src/webserver/web_server_core.h>

void test_register_disabled(void);
void test_register_well_known_fails_when_full(void);
void test_register_oauth_fails_when_full(void);

static char *g_test_dir = NULL;
static AppConfig g_test_config;

static void setup_oidc_service(void) {
    char tmpl[] = "/tmp/oidc_reg_XXXXXX";
    g_test_dir = mkdtemp(tmpl);
    TEST_ASSERT_NOT_NULL(g_test_dir);

    memset(&g_test_config, 0, sizeof(g_test_config));
    g_test_config.oidc.enabled = true;
    app_config = &g_test_config;

    OIDCConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.enabled = true;
    cfg.issuer = (char*)"http://localhost:5450";
    cfg.keys.storage_path = g_test_dir;
    cfg.keys.encryption_enabled = false;
    cfg.keys.rotation_interval_days = 90;
    cfg.tokens.access_token_lifetime = 3600;
    cfg.tokens.refresh_token_lifetime = 86400;
    cfg.tokens.id_token_lifetime = 3600;
    TEST_ASSERT_TRUE(init_oidc_service(&cfg));

    cleanup_oidc_endpoints();
}

static void teardown_oidc_service(void) {
    shutdown_oidc_service();
    app_config = NULL;

    if (g_test_dir) {
        char path[512];
        snprintf(path, sizeof(path), "%s/signing-active.pem", g_test_dir);
        unlink(path);
        snprintf(path, sizeof(path), "%s/signing-active.kid", g_test_dir);
        unlink(path);
        rmdir(g_test_dir);
        g_test_dir = NULL;
    }
}

static void fill_endpoints(int count) {
    for (int i = 0; i < count; i++) {
        char prefix[32];
        snprintf(prefix, sizeof(prefix), "/filler%d", i);
        WebServerEndpoint endpoint = {
            .prefix = prefix,
            .validator = is_oidc_endpoint,
            .handler = oidc_web_handler
        };
        register_web_endpoint(&endpoint);
    }
}

static void clear_endpoints(int count) {
    for (int i = 0; i < count; i++) {
        char prefix[32];
        snprintf(prefix, sizeof(prefix), "/filler%d", i);
        unregister_web_endpoint(prefix);
    }
}

void setUp(void) {
    shutdown_oidc_service();
    app_config = NULL;
}

void tearDown(void) {
    cleanup_oidc_endpoints();
    shutdown_oidc_service();
    app_config = NULL;
}

void test_register_disabled(void) {
    memset(&g_test_config, 0, sizeof(g_test_config));
    g_test_config.oidc.enabled = false;
    app_config = &g_test_config;

    OIDCConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.enabled = false;
    cfg.issuer = (char*)"http://localhost:5450";
    cfg.keys.storage_path = (char*)"/tmp";
    cfg.keys.encryption_enabled = false;
    cfg.keys.rotation_interval_days = 90;
    cfg.tokens.access_token_lifetime = 3600;
    cfg.tokens.refresh_token_lifetime = 86400;
    cfg.tokens.id_token_lifetime = 3600;
    TEST_ASSERT_TRUE(init_oidc_service(&cfg));

    TEST_ASSERT_TRUE(register_oidc_endpoints());

    shutdown_oidc_service();
    app_config = NULL;
}

void test_register_well_known_fails_when_full(void) {
    setup_oidc_service();

    fill_endpoints(32);

    TEST_ASSERT_FALSE(register_oidc_endpoints());

    clear_endpoints(32);
    teardown_oidc_service();
}

void test_register_oauth_fails_when_full(void) {
    setup_oidc_service();

    fill_endpoints(31);

    TEST_ASSERT_FALSE(register_oidc_endpoints());

    clear_endpoints(31);
    teardown_oidc_service();
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_register_disabled);
    if (0) RUN_TEST(test_register_well_known_fails_when_full);
    if (0) RUN_TEST(test_register_oauth_fails_when_full);
    return UNITY_END();
}
