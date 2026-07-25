/*
 * Unity Test File: authorization auth helpers
 * Tests for oidc_auth_send_html, oidc_auth_send_redirect,
 * oidc_auth_free_params, oidc_auth_safe_error_redirect,
 * oidc_auth_params_valid_for_code, oidc_auth_build_login_html,
 * oidc_auth_form_get, oidc_auth_merge_strdup
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/oidc/authorization/authorization.h>
#include <src/oidc/oidc_service.h>
#include <src/oidc/oidc_clients.h>
#include <src/oidc/oidc_pkce.h>
#include <src/api/oidc/oidc_service.h>
#include <src/api/api_utils.h>
#include <src/api/auth/auth_service.h>
#include <src/config/config_oidc.h>

#include <unity/mocks/mock_libmicrohttpd.h>
#include <unity/mocks/mock_logging.h>

/* USE_MOCK_SYSTEM is not actually defined by CMake for IS_AUTH_TEST (the
 * defines are passed as a single string), so we declare the mock control
 * functions we need directly instead of including mock_system.h. */
void mock_system_reset_all(void);
void mock_system_set_asprintf_failure(int should_fail);
void mock_system_set_malloc_failure(int should_fail);

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* Forward declarations for functions being tested (not in header) */
enum MHD_Result oidc_auth_send_html(struct MHD_Connection *connection,
                                    const char *html, unsigned int status);
enum MHD_Result oidc_auth_send_redirect(struct MHD_Connection *connection,
                                        const char *location);
void oidc_auth_free_params(char *client_id, char *redirect_uri, char *response_type,
                           char *scope, char *state, char *nonce,
                           char *code_challenge, char *code_challenge_method);
const char *oidc_auth_safe_error_redirect(const char *client_id, const char *redirect_uri);
bool oidc_auth_params_valid_for_code(const char *client_id, const char *redirect_uri,
                                     const char *response_type,
                                     const char *scope,
                                     const char *state,
                                     const char *nonce,
                                     const char *code_challenge,
                                     const char *code_challenge_method,
                                     const char **error_out);
char* oidc_auth_build_login_html(const char *client_id, const char *redirect_uri,
                                 const char *response_type, const char *scope,
                                 const char *state, const char *nonce,
                                 const char *code_challenge,
                                 const char *code_challenge_method,
                                 const char *error_msg);
char* oidc_auth_form_get(const char *body, const char *key);
void oidc_auth_merge_strdup(char **dest, const char *src);

static struct MHD_Connection *const FAKE_CONN = (struct MHD_Connection *)0x0D1C;

void setUp(void) {
    mock_mhd_reset_all();
    mock_system_reset_all();
    mock_logging_reset_all();
    mock_mhd_set_queue_response_result(MHD_YES);
    shutdown_oidc_service();
}

void tearDown(void) {
    shutdown_oidc_service();
    mock_mhd_reset_all();
    mock_system_reset_all();
    mock_logging_reset_all();
}

/* ==================== oidc_auth_send_html ==================== */

void test_send_html_null_html(void);
void test_send_html_valid(void);

void test_send_html_null_html(void) {
    TEST_ASSERT_EQUAL_INT(MHD_NO, oidc_auth_send_html(FAKE_CONN, NULL, MHD_HTTP_OK));
}

void test_send_html_valid(void) {
    const char *html = "<html>test</html>";
    TEST_ASSERT_EQUAL_INT(MHD_YES, oidc_auth_send_html(FAKE_CONN, html, MHD_HTTP_OK));
}

/* ==================== oidc_auth_send_redirect ==================== */

void test_send_redirect_null_location(void);
void test_send_redirect_valid(void);

void test_send_redirect_null_location(void) {
    TEST_ASSERT_EQUAL_INT(MHD_NO, oidc_auth_send_redirect(FAKE_CONN, NULL));
}

void test_send_redirect_valid(void) {
    TEST_ASSERT_EQUAL_INT(MHD_YES, oidc_auth_send_redirect(FAKE_CONN, "https://example.com/cb"));
}

/* ==================== oidc_auth_free_params ==================== */

void test_free_params_nulls(void);
void test_free_params_allocated(void);

void test_free_params_nulls(void) {
    oidc_auth_free_params(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    TEST_ASSERT_TRUE(true);
}

void test_free_params_allocated(void) {
    char *a = strdup("a");
    char *b = strdup("b");
    char *c = strdup("c");
    char *d = strdup("d");
    char *e = strdup("e");
    char *f = strdup("f");
    char *g = strdup("g");
    char *h = strdup("h");
    oidc_auth_free_params(a, b, c, d, e, f, g, h);
    TEST_ASSERT_TRUE(true);
}

/* ==================== oidc_auth_safe_error_redirect ==================== */

void test_safe_error_redirect_null_client_id(void);
void test_safe_error_redirect_null_redirect_uri(void);
void test_safe_error_redirect_invalid_scheme(void);
void test_safe_error_redirect_no_context(void);
void test_safe_error_redirect_invalid_client(void);

void test_safe_error_redirect_null_client_id(void) {
    TEST_ASSERT_NULL(oidc_auth_safe_error_redirect(NULL, "https://example.com/cb"));
}

void test_safe_error_redirect_null_redirect_uri(void) {
    TEST_ASSERT_NULL(oidc_auth_safe_error_redirect("client1", NULL));
}

void test_safe_error_redirect_invalid_scheme(void) {
    TEST_ASSERT_NULL(oidc_auth_safe_error_redirect("client1", "javascript:alert(1)"));
}

void test_safe_error_redirect_no_context(void) {
    TEST_ASSERT_NULL(oidc_auth_safe_error_redirect("client1", "https://example.com/cb"));
}

void test_safe_error_redirect_invalid_client(void) {
    char tmpl[] = "/tmp/oidc_redir_XXXXXX";
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

    TEST_ASSERT_NULL(oidc_auth_safe_error_redirect("unregistered_client", "https://example.com/cb"));

    shutdown_oidc_service();

    char path[512];
    snprintf(path, sizeof(path), "%s/signing-active.pem", dir);
    unlink(path);
    snprintf(path, sizeof(path), "%s/signing-active.kid", dir);
    unlink(path);
    rmdir(dir);
}

/* ==================== oidc_auth_params_valid_for_code ==================== */

void test_params_valid_null_params(void);
void test_params_valid_invalid_scheme(void);
void test_params_valid_unsupported_response_type(void);
void test_params_valid_missing_state(void);
void test_params_valid_missing_nonce(void);
void test_params_valid_missing_code_challenge(void);
void test_params_valid_wrong_pkce_method(void);
void test_params_valid_no_context(void);
void test_params_valid_invalid_client(void);

void test_params_valid_null_params(void) {
    const char *err = NULL;
    TEST_ASSERT_FALSE(oidc_auth_params_valid_for_code(NULL, "https://example.com/cb",
                                                      "code", "openid", "state1", "nonce1",
                                                      "challenge", "S256", &err));
    TEST_ASSERT_EQUAL_STRING("invalid_request", err);
}

void test_params_valid_invalid_scheme(void) {
    const char *err = NULL;
    TEST_ASSERT_FALSE(oidc_auth_params_valid_for_code("client1", "javascript:alert(1)",
                                                      "code", "openid", "state1", "nonce1",
                                                      "challenge", "S256", &err));
    TEST_ASSERT_EQUAL_STRING("invalid_request", err);
}

void test_params_valid_unsupported_response_type(void) {
    const char *err = NULL;
    TEST_ASSERT_FALSE(oidc_auth_params_valid_for_code("client1", "https://example.com/cb",
                                                      "token", "openid", "state1", "nonce1",
                                                      "challenge", "S256", &err));
    TEST_ASSERT_EQUAL_STRING("unsupported_response_type", err);
}

void test_params_valid_missing_state(void) {
    const char *err = NULL;
    TEST_ASSERT_FALSE(oidc_auth_params_valid_for_code("client1", "https://example.com/cb",
                                                      "code", "openid", NULL, "nonce1",
                                                      "challenge", "S256", &err));
    TEST_ASSERT_EQUAL_STRING("invalid_request", err);
}

void test_params_valid_missing_nonce(void) {
    const char *err = NULL;
    TEST_ASSERT_FALSE(oidc_auth_params_valid_for_code("client1", "https://example.com/cb",
                                                      "code", "openid", "state1", NULL,
                                                      "challenge", "S256", &err));
    TEST_ASSERT_EQUAL_STRING("invalid_request", err);
}

void test_params_valid_missing_code_challenge(void) {
    const char *err = NULL;
    TEST_ASSERT_FALSE(oidc_auth_params_valid_for_code("client1", "https://example.com/cb",
                                                      "code", "openid", "state1", "nonce1",
                                                      NULL, "S256", &err));
    TEST_ASSERT_EQUAL_STRING("invalid_request", err);
}

void test_params_valid_wrong_pkce_method(void) {
    const char *err = NULL;
    TEST_ASSERT_FALSE(oidc_auth_params_valid_for_code("client1", "https://example.com/cb",
                                                      "code", "openid", "state1", "nonce1",
                                                      "challenge", "plain", &err));
    TEST_ASSERT_EQUAL_STRING("invalid_request", err);
}

void test_params_valid_no_context(void) {
    const char *err = NULL;
    TEST_ASSERT_FALSE(oidc_auth_params_valid_for_code("client1", "https://example.com/cb",
                                                      "code", "openid", "state1", "nonce1",
                                                      "challenge", "S256", &err));
    TEST_ASSERT_EQUAL_STRING("server_error", err);
}

void test_params_valid_invalid_client(void) {
    char tmpl[] = "/tmp/oidc_pvc_XXXXXX";
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

    const char *err = NULL;
    TEST_ASSERT_FALSE(oidc_auth_params_valid_for_code("unregistered_client", "https://example.com/cb",
                                                      "code", "openid", "state1", "nonce1",
                                                      "challenge", "S256", &err));
    TEST_ASSERT_EQUAL_STRING("unauthorized_client", err);

    shutdown_oidc_service();

    char path[512];
    snprintf(path, sizeof(path), "%s/signing-active.pem", dir);
    unlink(path);
    snprintf(path, sizeof(path), "%s/signing-active.kid", dir);
    unlink(path);
    rmdir(dir);
}

/* ==================== oidc_auth_build_login_html ==================== */

void test_build_login_html_null_params(void);
void test_build_login_html_with_error(void);
void test_build_login_html_asprintf_failure(void);

void test_build_login_html_null_params(void) {
    char *html = oidc_auth_build_login_html(NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    TEST_ASSERT_NOT_NULL(html);
    TEST_ASSERT_TRUE(strstr(html, "Sign in") != NULL);
    free(html);
}

void test_build_login_html_with_error(void) {
    char *html = oidc_auth_build_login_html("cid", "https://example.com/cb", "code",
                                            "openid", "st", "n1", "cc", "S256",
                                            "Something went wrong");
    TEST_ASSERT_NOT_NULL(html);
    TEST_ASSERT_TRUE(strstr(html, "Something went wrong") != NULL);
    free(html);
}

void test_build_login_html_asprintf_failure(void) {
    mock_system_set_asprintf_failure(1);
    char *html = oidc_auth_build_login_html("cid", "https://example.com/cb", "code",
                                            "openid", "st", "n1", "cc", "S256", NULL);
    TEST_ASSERT_NULL(html);
    mock_system_reset_all();
}

/* ==================== oidc_auth_form_get ==================== */

void test_form_get_nulls(void);
void test_form_get_missing(void);
void test_form_get_values(void);
void test_form_get_url_encoded(void);
void test_form_get_malloc_failure(void);

void test_form_get_nulls(void) {
    TEST_ASSERT_NULL(oidc_auth_form_get(NULL, "a"));
    TEST_ASSERT_NULL(oidc_auth_form_get("a=1", NULL));
}

void test_form_get_missing(void) {
    TEST_ASSERT_NULL(oidc_auth_form_get("a=1&b=2", "c"));
}

void test_form_get_values(void) {
    const char *body = "client_id=cli&redirect_uri=https%3A%2F%2Fcb&state=st";
    char *cid = oidc_auth_form_get(body, "client_id");
    char *ru = oidc_auth_form_get(body, "redirect_uri");
    char *st = oidc_auth_form_get(body, "state");
    TEST_ASSERT_EQUAL_STRING("cli", cid);
    TEST_ASSERT_EQUAL_STRING("https://cb", ru);
    TEST_ASSERT_EQUAL_STRING("st", st);
    free(cid);
    free(ru);
    free(st);
}

void test_form_get_url_encoded(void) {
    const char *body = "code=abc%2B1";
    char *code = oidc_auth_form_get(body, "code");
    TEST_ASSERT_EQUAL_STRING("abc+1", code);
    free(code);
}

void test_form_get_malloc_failure(void) {
    const char *body = "key=value";
    mock_system_set_malloc_failure(1);
    TEST_ASSERT_NULL(oidc_auth_form_get(body, "key"));
    mock_system_reset_all();
}

/* ==================== oidc_auth_merge_strdup ==================== */

void test_merge_strdup_null_dest(void);
void test_merge_strdup_null_src(void);
void test_merge_strdup_existing(void);
void test_merge_strdup_valid(void);

void test_merge_strdup_null_dest(void) {
    oidc_auth_merge_strdup(NULL, "value");
    TEST_ASSERT_TRUE(true);
}

void test_merge_strdup_null_src(void) {
    char *dest = NULL;
    oidc_auth_merge_strdup(&dest, NULL);
    TEST_ASSERT_NULL(dest);
}

void test_merge_strdup_existing(void) {
    char *dest = strdup("existing");
    oidc_auth_merge_strdup(&dest, "new");
    TEST_ASSERT_EQUAL_STRING("existing", dest);
    free(dest);
}

void test_merge_strdup_valid(void) {
    char *dest = NULL;
    oidc_auth_merge_strdup(&dest, "new");
    TEST_ASSERT_EQUAL_STRING("new", dest);
    free(dest);
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_send_html_null_html);
    RUN_TEST(test_send_html_valid);

    RUN_TEST(test_send_redirect_null_location);
    RUN_TEST(test_send_redirect_valid);

    RUN_TEST(test_free_params_nulls);
    RUN_TEST(test_free_params_allocated);

    RUN_TEST(test_safe_error_redirect_null_client_id);
    RUN_TEST(test_safe_error_redirect_null_redirect_uri);
    RUN_TEST(test_safe_error_redirect_invalid_scheme);
    RUN_TEST(test_safe_error_redirect_no_context);
    RUN_TEST(test_safe_error_redirect_invalid_client);

    RUN_TEST(test_params_valid_null_params);
    RUN_TEST(test_params_valid_invalid_scheme);
    RUN_TEST(test_params_valid_unsupported_response_type);
    RUN_TEST(test_params_valid_missing_state);
    RUN_TEST(test_params_valid_missing_nonce);
    RUN_TEST(test_params_valid_missing_code_challenge);
    RUN_TEST(test_params_valid_wrong_pkce_method);
    RUN_TEST(test_params_valid_no_context);
    RUN_TEST(test_params_valid_invalid_client);

    RUN_TEST(test_build_login_html_null_params);
    RUN_TEST(test_build_login_html_with_error);
    RUN_TEST(test_build_login_html_asprintf_failure);

    RUN_TEST(test_form_get_nulls);
    RUN_TEST(test_form_get_missing);
    RUN_TEST(test_form_get_values);
    RUN_TEST(test_form_get_url_encoded);
    RUN_TEST(test_form_get_malloc_failure);

    RUN_TEST(test_merge_strdup_null_dest);
    RUN_TEST(test_merge_strdup_null_src);
    RUN_TEST(test_merge_strdup_existing);
    RUN_TEST(test_merge_strdup_valid);

    return UNITY_END();
}
