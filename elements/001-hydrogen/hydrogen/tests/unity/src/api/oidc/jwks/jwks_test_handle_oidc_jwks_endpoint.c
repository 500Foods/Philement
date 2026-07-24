/*
 * Unity Test File: handle_oidc_jwks_endpoint
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/oidc/jwks/jwks.h>
#include <src/api/oidc/oidc_service.h>
#include <src/oidc/oidc_service.h>
#include <src/config/config_oidc.h>
#include <unity/mocks/mock_libmicrohttpd.h>

#include <unistd.h>

void test_jwks_handler_without_service(void);
void test_jwks_handler_ok(void);

static struct MHD_Connection *const FAKE = (struct MHD_Connection *)0x1A1A;

void setUp(void) {
    mock_mhd_reset_all();
    mock_mhd_set_queue_response_result(MHD_YES);
    shutdown_oidc_service();
}

void tearDown(void) {
    shutdown_oidc_service();
    mock_mhd_reset_all();
}

void test_jwks_handler_without_service(void) {
    TEST_ASSERT_EQUAL_INT(MHD_YES, handle_oidc_jwks_endpoint(FAKE));
}

void test_jwks_handler_ok(void) {
    char tmpl[] = "/tmp/oidc_jwks_h_XXXXXX";
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
    TEST_ASSERT_TRUE(init_oidc_endpoints(get_oidc_context()));

    TEST_ASSERT_EQUAL_INT(MHD_YES, handle_oidc_jwks_endpoint(FAKE));

    cleanup_oidc_endpoints();
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
    RUN_TEST(test_jwks_handler_without_service);
    RUN_TEST(test_jwks_handler_ok);
    return UNITY_END();
}
