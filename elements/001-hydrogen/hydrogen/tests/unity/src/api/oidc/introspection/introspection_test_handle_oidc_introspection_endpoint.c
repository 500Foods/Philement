/*
 * Unity Test File: handle_oidc_introspection_endpoint
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/oidc/oidc_service.h>
#include <src/oidc/oidc_service.h>
#include <src/oidc/oidc_clients.h>
#include <src/config/config_oidc.h>
#include <unity/mocks/mock_libmicrohttpd.h>

#include <string.h>
#include <unistd.h>

void test_introspect_method_not_allowed(void);
void test_introspect_missing_token(void);
void test_introspect_no_client_credentials(void);
void test_introspect_failure_no_oidc(void);
void test_introspect_success(void);

static struct MHD_Connection *const FAKE = (struct MHD_Connection *)0xBEEF;

static char *g_tmpdir = NULL;

static enum MHD_Result drive_post(const char *body) {
    void *con_cls = NULL;
    size_t zero = 0;
    size_t len = body ? strlen(body) : 0;
    enum MHD_Result ret;

    ret = handle_oidc_introspection_endpoint(FAKE, "POST", NULL, &zero, &con_cls);
    if (ret != MHD_YES || !con_cls) {
        return ret;
    }
    if (len > 0) {
        ret = handle_oidc_introspection_endpoint(FAKE, "POST", body, &len, &con_cls);
        if (ret != MHD_YES) {
            return ret;
        }
    }
    zero = 0;
    return handle_oidc_introspection_endpoint(FAKE, "POST", NULL, &zero, &con_cls);
}

static bool init_oidc_with_client(void) {
    char tmpl[] = "/tmp/oidc_intro_ep_XXXXXX";
    g_tmpdir = mkdtemp(tmpl);
    if (!g_tmpdir) return false;

    OIDCConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.enabled = true;
    cfg.issuer = (char*)"http://localhost:5450";
    cfg.keys.storage_path = g_tmpdir;
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
        "cli", NULL, "T", false, true,
        "[\"https://app.example/cb\"]",
        "authorization_code refresh_token", "code");
    return oidc_client_registry_add((OIDCClientContext*)ctx->client_context, client);
}

static void cleanup_oidc_with_client(void) {
    shutdown_oidc_service();
    if (g_tmpdir) {
        char path[512];
        snprintf(path, sizeof(path), "%s/signing-active.pem", g_tmpdir);
        unlink(path);
        snprintf(path, sizeof(path), "%s/signing-active.kid", g_tmpdir);
        unlink(path);
        rmdir(g_tmpdir);
        g_tmpdir = NULL;
    }
}

void setUp(void) {
    mock_mhd_reset_all();
    mock_mhd_set_queue_response_result(MHD_YES);
}

void tearDown(void) {
    mock_mhd_reset_all();
    cleanup_oidc_with_client();
}

void test_introspect_method_not_allowed(void) {
    enum MHD_Result ret = handle_oidc_introspection_endpoint(FAKE, "GET", NULL, NULL, NULL);
    TEST_ASSERT_EQUAL_INT(MHD_YES, ret);
}

void test_introspect_missing_token(void) {
    enum MHD_Result ret = drive_post("client_id=cli");
    TEST_ASSERT_EQUAL_INT(MHD_YES, ret);
}

void test_introspect_no_client_credentials(void) {
    enum MHD_Result ret = drive_post("token=some-token");
    TEST_ASSERT_EQUAL_INT(MHD_YES, ret);
}

void test_introspect_failure_no_oidc(void) {
    mock_mhd_add_lookup("Authorization", "Basic Y2xpOnNlYw==");
    enum MHD_Result ret = drive_post("token=some-token");
    TEST_ASSERT_EQUAL_INT(MHD_YES, ret);
}

void test_introspect_success(void) {
    TEST_ASSERT_TRUE(init_oidc_with_client());
    enum MHD_Result ret = drive_post("token=some-token&client_id=cli");
    TEST_ASSERT_EQUAL_INT(MHD_YES, ret);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_introspect_method_not_allowed);
    RUN_TEST(test_introspect_missing_token);
    RUN_TEST(test_introspect_no_client_credentials);
    RUN_TEST(test_introspect_failure_no_oidc);
    RUN_TEST(test_introspect_success);
    return UNITY_END();
}
