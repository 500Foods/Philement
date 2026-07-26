/*
 * Unity Test File: handle_oidc_userinfo_endpoint / oidc_userinfo_send_unauthorized
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/oidc/oidc_service.h>
#include <src/api/oidc/userinfo/userinfo.h>
#include <src/oidc/oidc_service.h>
#include <src/oidc/oidc_tokens.h>
#include <src/oidc/oidc_clients.h>
#include <src/config/config_oidc.h>
#include <unity/mocks/mock_libmicrohttpd.h>

/* USE_MOCK_SYSTEM is defined globally by CMake for this test (else() branch),
 * but mock_system.h is not -include'd, so declare the control functions we need. */
void mock_system_reset_all(void);
void mock_system_set_asprintf_failure(int should_fail);

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

void test_send_unauthorized_null_description(void);
void test_send_unauthorized_with_description(void);
void test_send_unauthorized_asprintf_failure(void);
void test_send_unauthorized_response_creation_failure(void);
void test_send_unauthorized_success(void);
void test_send_unauthorized_queue_fails(void);

void test_handle_userinfo_method_not_allowed(void);
void test_handle_userinfo_no_auth_header(void);
void test_handle_userinfo_invalid_auth_scheme(void);
void test_handle_userinfo_empty_token(void);
void test_handle_userinfo_invalid_token(void);
void test_handle_userinfo_success(void);

static struct MHD_Connection *const FAKE_CONN = (struct MHD_Connection *)0x0D1C;

static char *g_tmpdir = NULL;

static bool init_oidc_with_keys(void) {
    char tmpl[] = "/tmp/oidc_ui_XXXXXX";
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
    return init_oidc_service(&cfg);
}

static void cleanup_oidc(void) {
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
    mock_system_reset_all();
    mock_mhd_set_queue_response_result(MHD_YES);
}

void tearDown(void) {
    mock_mhd_reset_all();
    mock_system_reset_all();
    cleanup_oidc();
}

/* ==================== oidc_userinfo_send_unauthorized ==================== */

void test_send_unauthorized_null_description(void) {
    TEST_ASSERT_EQUAL_INT(MHD_YES,
                          oidc_userinfo_send_unauthorized(FAKE_CONN, NULL));
}

void test_send_unauthorized_with_description(void) {
    TEST_ASSERT_EQUAL_INT(MHD_YES,
                          oidc_userinfo_send_unauthorized(FAKE_CONN, "Missing or invalid Authorization header"));
}

void test_send_unauthorized_asprintf_failure(void) {
    mock_system_set_asprintf_failure(1);
    TEST_ASSERT_EQUAL_INT(MHD_YES,
                          oidc_userinfo_send_unauthorized(FAKE_CONN, "test failure"));
    mock_system_reset_all();
}

void test_send_unauthorized_response_creation_failure(void) {
    mock_mhd_set_create_response_should_fail(true);
    TEST_ASSERT_EQUAL_INT(MHD_NO,
                          oidc_userinfo_send_unauthorized(FAKE_CONN, "test failure"));
}

void test_send_unauthorized_success(void) {
    TEST_ASSERT_EQUAL_INT(MHD_YES,
                          oidc_userinfo_send_unauthorized(FAKE_CONN, "Invalid access token"));
}

void test_send_unauthorized_queue_fails(void) {
    mock_mhd_set_queue_response_result(MHD_NO);
    TEST_ASSERT_EQUAL_INT(MHD_NO,
                          oidc_userinfo_send_unauthorized(FAKE_CONN, "test failure"));
}

/* ==================== handle_oidc_userinfo_endpoint ==================== */

void test_handle_userinfo_method_not_allowed(void) {
    enum MHD_Result ret = handle_oidc_userinfo_endpoint(FAKE_CONN, "PUT");
    TEST_ASSERT_EQUAL_INT(MHD_YES, ret);
}

void test_handle_userinfo_no_auth_header(void) {
    mock_mhd_set_lookup_result(NULL);
    enum MHD_Result ret = handle_oidc_userinfo_endpoint(FAKE_CONN, "GET");
    TEST_ASSERT_EQUAL_INT(MHD_YES, ret);
}

void test_handle_userinfo_invalid_auth_scheme(void) {
    mock_mhd_set_lookup_result("Basic dXNlcjpwYXNz");
    enum MHD_Result ret = handle_oidc_userinfo_endpoint(FAKE_CONN, "GET");
    TEST_ASSERT_EQUAL_INT(MHD_YES, ret);
}

void test_handle_userinfo_empty_token(void) {
    mock_mhd_set_lookup_result("Bearer     ");
    enum MHD_Result ret = handle_oidc_userinfo_endpoint(FAKE_CONN, "GET");
    TEST_ASSERT_EQUAL_INT(MHD_YES, ret);
}

void test_handle_userinfo_invalid_token(void) {
    TEST_ASSERT_TRUE(init_oidc_with_keys());
    mock_mhd_set_lookup_result("Bearer invalid_token_value");
    enum MHD_Result ret = handle_oidc_userinfo_endpoint(FAKE_CONN, "GET");
    TEST_ASSERT_EQUAL_INT(MHD_YES, ret);
}

void test_handle_userinfo_success(void) {
    TEST_ASSERT_TRUE(init_oidc_with_keys());

    OIDCContext *ctx = get_oidc_context();
    TEST_ASSERT_NOT_NULL(ctx);
    OIDCTokenContext *tok_ctx = (OIDCTokenContext*)ctx->token_context;
    TEST_ASSERT_NOT_NULL(tok_ctx);

    OIDCTokenClaims claims;
    memset(&claims, 0, sizeof(claims));
    claims.iss = (char*)ctx->config.issuer;
    claims.sub = (char*)"user123";
    claims.scope = (char*)"openid";

    char *access_token = oidc_generate_access_token(tok_ctx, &claims, NULL);
    TEST_ASSERT_NOT_NULL(access_token);

    char auth_header[512];
    snprintf(auth_header, sizeof(auth_header), "Bearer %s", access_token);
    mock_mhd_set_lookup_result(auth_header);

    enum MHD_Result ret = handle_oidc_userinfo_endpoint(FAKE_CONN, "GET");
    TEST_ASSERT_EQUAL_INT(MHD_YES, ret);

    free(access_token);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_send_unauthorized_null_description);
    RUN_TEST(test_send_unauthorized_with_description);
    RUN_TEST(test_send_unauthorized_asprintf_failure);
    RUN_TEST(test_send_unauthorized_response_creation_failure);
    RUN_TEST(test_send_unauthorized_success);
    RUN_TEST(test_send_unauthorized_queue_fails);

    RUN_TEST(test_handle_userinfo_method_not_allowed);
    RUN_TEST(test_handle_userinfo_no_auth_header);
    RUN_TEST(test_handle_userinfo_invalid_auth_scheme);
    RUN_TEST(test_handle_userinfo_empty_token);
    RUN_TEST(test_handle_userinfo_invalid_token);
    RUN_TEST(test_handle_userinfo_success);

    return UNITY_END();
}
