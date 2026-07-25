/*
 * Unity Test File: handle_oidc_request routing
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/oidc/oidc_service.h>
#include <src/oidc/oidc_service.h>
#include <src/config/config_oidc.h>
#include <unity/mocks/mock_libmicrohttpd.h>

#include <unistd.h>

void test_handle_no_context(void);
void test_handle_routes_discovery_and_jwks(void);
void test_handle_unknown(void);
void test_handle_routes_introspection(void);
void test_handle_routes_revocation(void);
void test_handle_routes_registration(void);
void test_handle_routes_end_session(void);

static struct MHD_Connection *const FAKE = (struct MHD_Connection *)0x0D1C;

static char *g_test_dir = NULL;

static void setup_oidc_service(void) {
    char tmpl[] = "/tmp/oidc_route_XXXXXX";
    g_test_dir = mkdtemp(tmpl);
    TEST_ASSERT_NOT_NULL(g_test_dir);

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
    TEST_ASSERT_TRUE(init_oidc_endpoints(get_oidc_context()));
}

static void teardown_oidc_service(void) {
    cleanup_oidc_endpoints();
    shutdown_oidc_service();

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

void setUp(void) {
    mock_mhd_reset_all();
    mock_mhd_set_queue_response_result(MHD_YES);
    shutdown_oidc_service();
}

void tearDown(void) {
    cleanup_oidc_endpoints();
    shutdown_oidc_service();
    mock_mhd_reset_all();
}

void test_handle_no_context(void) {
    size_t usz = 0;
    void *cls = NULL;
    TEST_ASSERT_EQUAL_INT(MHD_NO,
                          handle_oidc_request(FAKE, "/oauth/jwks", "GET", "HTTP/1.1",
                                              NULL, &usz, &cls));
}

void test_handle_routes_discovery_and_jwks(void) {
    setup_oidc_service();

    size_t usz = 0;
    void *cls = NULL;
    TEST_ASSERT_EQUAL_INT(MHD_YES,
                          handle_oidc_request(FAKE, "/.well-known/openid-configuration",
                                              "GET", "HTTP/1.1", NULL, &usz, &cls));
    TEST_ASSERT_EQUAL_INT(MHD_YES,
                          handle_oidc_request(FAKE, "/oauth/jwks", "GET", "HTTP/1.1",
                                              NULL, &usz, &cls));

    teardown_oidc_service();
}

void test_handle_unknown(void) {
    setup_oidc_service();

    size_t usz = 0;
    void *cls = NULL;
    TEST_ASSERT_EQUAL_INT(MHD_NO,
                          handle_oidc_request(FAKE, "/oauth/not-a-thing", "GET",
                                              "HTTP/1.1", NULL, &usz, &cls));

    teardown_oidc_service();
}

void test_handle_routes_introspection(void) {
    setup_oidc_service();

    size_t usz = 0;
    void *cls = NULL;
    TEST_ASSERT_EQUAL_INT(MHD_YES,
                          handle_oidc_request(FAKE, "/oauth/introspect", "GET",
                                              "HTTP/1.1", NULL, &usz, &cls));

    teardown_oidc_service();
}

void test_handle_routes_revocation(void) {
    setup_oidc_service();

    size_t usz = 0;
    void *cls = NULL;
    TEST_ASSERT_EQUAL_INT(MHD_YES,
                          handle_oidc_request(FAKE, "/oauth/revoke", "GET",
                                              "HTTP/1.1", NULL, &usz, &cls));

    teardown_oidc_service();
}

void test_handle_routes_registration(void) {
    setup_oidc_service();

    size_t usz = 0;
    void *cls = NULL;
    TEST_ASSERT_EQUAL_INT(MHD_YES,
                          handle_oidc_request(FAKE, "/oauth/register", "GET",
                                              "HTTP/1.1", NULL, &usz, &cls));

    teardown_oidc_service();
}

void test_handle_routes_end_session(void) {
    setup_oidc_service();

    size_t usz = 0;
    void *cls = NULL;
    TEST_ASSERT_EQUAL_INT(MHD_YES,
                          handle_oidc_request(FAKE, "/oauth/end-session", "GET",
                                              "HTTP/1.1", NULL, &usz, &cls));

    teardown_oidc_service();
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_handle_no_context);
    RUN_TEST(test_handle_routes_discovery_and_jwks);
    RUN_TEST(test_handle_unknown);
    RUN_TEST(test_handle_routes_introspection);
    RUN_TEST(test_handle_routes_revocation);
    RUN_TEST(test_handle_routes_registration);
    RUN_TEST(test_handle_routes_end_session);
    return UNITY_END();
}
