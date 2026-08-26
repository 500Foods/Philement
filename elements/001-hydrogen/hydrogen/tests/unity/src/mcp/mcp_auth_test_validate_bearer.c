#include <src/hydrogen.h>
#include <unity.h>

#define USE_MOCK_LIBMICROHTTPD
#define USE_MOCK_AUTH_SERVICE_JWT

#include <unity/mocks/mock_libmicrohttpd.h>
#include <unity/mocks/mock_auth_service_jwt.h>
#include <src/mcp/mcp_auth.h>
#include <src/mcp/mcp_prm.h>
#include <src/mcp/mcp_stats.h>
#include <src/config/config_mcp.h>

static AppConfig test_app;
static struct { int dummy; } mock_conn;

void test_validate_bearer_missing(void);
void test_validate_bearer_malformed(void);
void test_validate_bearer_reserved_hydrogen(void);
void test_validate_bearer_bad_hydrogen_jwt(void);
void test_validate_bearer_expired_hydrogen_jwt(void);
void test_validate_bearer_happy_hydrogen_jwt(void);
void test_validate_bearer_require_jwt_false(void);
void test_validate_bearer_idp_reject(void);
void test_validate_bearer_idp_accept(void);
void test_validate_bearer_rp_reject(void);
void test_validate_bearer_rp_accept(void);
void test_validate_bearer_aud_mismatch(void);
void test_validate_bearer_scope_mismatch(void);
void test_www_authenticate_and_prm_shape(void);
void test_send_unauthorized_header(void);

static void apply_defaults(void) {
    memset(&test_app, 0, sizeof(test_app));
    mcp_config_apply_defaults(&test_app.mcp);
    test_app.mcp.Enabled = true;
    test_app.mcp.Resource = strdup("http://127.0.0.1:3100/mcp");
}

static void set_valid_jwt(void) {
    jwt_validation_result_t programmed;
    jwt_claims_t claims;

    memset(&programmed, 0, sizeof(programmed));
    memset(&claims, 0, sizeof(claims));
    claims.sub = (char *)"user-1";
    claims.database = (char *)"acuranzo";
    claims.roles = (char *)"admin";
    programmed.valid = true;
    programmed.claims = &claims;
    programmed.error = JWT_ERROR_NONE;
    mock_auth_service_jwt_set_validation_result(programmed);
}

static bool hook_idp_ok(const char *token, const AppConfig *app, OIDCTokenClaims **out) {
    OIDCTokenClaims *claims;

    (void)token;
    (void)app;
    claims = calloc(1, sizeof(*claims));
    TEST_ASSERT_NOT_NULL(claims);
    claims->sub = strdup("idp-sub");
    claims->iss = strdup("https://idp.example");
    claims->aud = calloc(1, sizeof(char *));
    TEST_ASSERT_NOT_NULL(claims->aud);
    claims->aud[0] = strdup("http://127.0.0.1:3100/mcp");
    claims->aud_count = 1;
    claims->scope = strdup("mcp.read");
    *out = claims;
    return true;
}

static bool hook_idp_aud_bad(const char *token, const AppConfig *app, OIDCTokenClaims **out) {
    OIDCTokenClaims *claims;

    (void)token;
    (void)app;
    claims = calloc(1, sizeof(*claims));
    TEST_ASSERT_NOT_NULL(claims);
    claims->sub = strdup("idp-sub");
    claims->iss = strdup("https://idp.example");
    claims->aud = calloc(1, sizeof(char *));
    TEST_ASSERT_NOT_NULL(claims->aud);
    claims->aud[0] = strdup("https://other");
    claims->aud_count = 1;
    *out = claims;
    return true;
}

static bool hook_idp_scope_bad(const char *token, const AppConfig *app, OIDCTokenClaims **out) {
    OIDCTokenClaims *claims;

    (void)token;
    (void)app;
    claims = calloc(1, sizeof(*claims));
    TEST_ASSERT_NOT_NULL(claims);
    claims->sub = strdup("idp-sub");
    claims->iss = strdup("https://idp.example");
    claims->scope = strdup("other");
    *out = claims;
    return true;
}

static bool hook_fail(const char *token, const AppConfig *app, OIDCTokenClaims **out) {
    (void)token;
    (void)app;
    *out = NULL;
    return false;
}

void setUp(void) {
    mock_mhd_reset_all();
    mock_auth_service_jwt_reset_all();
    mcp_stats_reset();
    mcp_auth_set_oidc_idp_hook(NULL);
    mcp_auth_set_oidc_rp_hook(NULL);
    apply_defaults();
    app_config = &test_app;
}

void tearDown(void) {
    mcp_auth_set_oidc_idp_hook(NULL);
    mcp_auth_set_oidc_rp_hook(NULL);
    cleanup_mcp_config(&test_app.mcp);
    app_config = NULL;
    mock_mhd_reset_all();
    mock_auth_service_jwt_reset_all();
    mcp_stats_reset();
}

void test_validate_bearer_missing(void) {
    McpAuthResult auth;
    McpMetrics snap;

    TEST_ASSERT_FALSE(mcp_validate_bearer(NULL, NULL, 0, &test_app.mcp, &test_app, &auth));
    TEST_ASSERT_EQUAL(MCP_AUTH_REJECT_MISSING, auth.reject_reason);
    mcp_collect_metrics(&snap);
    TEST_ASSERT_EQUAL_UINT64(1, snap.auth_rejected_missing);
    mcp_auth_result_cleanup(&auth);
}

void test_validate_bearer_malformed(void) {
    McpAuthResult auth;

    TEST_ASSERT_FALSE(mcp_validate_bearer("Basic abc", NULL, 0, &test_app.mcp, &test_app, &auth));
    TEST_ASSERT_EQUAL(MCP_AUTH_REJECT_MALFORMED, auth.reject_reason);
    mcp_auth_result_cleanup(&auth);
}

void test_validate_bearer_reserved_hydrogen(void) {
    McpAuthResult auth;
    const char *body = "{\"_hydrogen\":{}}";

    TEST_ASSERT_FALSE(mcp_validate_bearer("Bearer token", body, strlen(body),
                                          &test_app.mcp, &test_app, &auth));
    TEST_ASSERT_EQUAL(MCP_AUTH_REJECT_MALFORMED, auth.reject_reason);
    mcp_auth_result_cleanup(&auth);
}

void test_validate_bearer_bad_hydrogen_jwt(void) {
    McpAuthResult auth;
    jwt_validation_result_t programmed;

    memset(&programmed, 0, sizeof(programmed));
    programmed.valid = false;
    programmed.error = JWT_ERROR_INVALID_SIGNATURE;
    mock_auth_service_jwt_set_validation_result(programmed);

    TEST_ASSERT_FALSE(mcp_validate_bearer("Bearer bad", NULL, 0, &test_app.mcp, &test_app, &auth));
    TEST_ASSERT_EQUAL(MCP_AUTH_REJECT_HYDROGEN_JWT, auth.reject_reason);
    mcp_auth_result_cleanup(&auth);
}

void test_validate_bearer_expired_hydrogen_jwt(void) {
    McpAuthResult auth;
    jwt_validation_result_t programmed;

    memset(&programmed, 0, sizeof(programmed));
    programmed.valid = false;
    programmed.error = JWT_ERROR_EXPIRED;
    mock_auth_service_jwt_set_validation_result(programmed);

    TEST_ASSERT_FALSE(mcp_validate_bearer("Bearer expired", NULL, 0, &test_app.mcp, &test_app, &auth));
    TEST_ASSERT_EQUAL(MCP_AUTH_REJECT_HYDROGEN_JWT, auth.reject_reason);
    mcp_auth_result_cleanup(&auth);
}

void test_validate_bearer_happy_hydrogen_jwt(void) {
    McpAuthResult auth;

    set_valid_jwt();
    TEST_ASSERT_TRUE(mcp_validate_bearer("Bearer good", NULL, 0, &test_app.mcp, &test_app, &auth));
    TEST_ASSERT_EQUAL(MCP_AUTH_KIND_HYDROGEN_JWT, auth.kind);
    TEST_ASSERT_EQUAL_STRING("user-1", auth.sub);
    TEST_ASSERT_EQUAL_STRING("acuranzo", auth.database);
    mcp_auth_result_cleanup(&auth);
}

void test_validate_bearer_require_jwt_false(void) {
    McpAuthResult auth;

    test_app.mcp.RequireJWT = false;
    TEST_ASSERT_TRUE(mcp_validate_bearer(NULL, NULL, 0, &test_app.mcp, &test_app, &auth));
    TEST_ASSERT_EQUAL(MCP_AUTH_KIND_NONE, auth.kind);
    mcp_auth_result_cleanup(&auth);
}

void test_validate_bearer_idp_reject(void) {
    McpAuthResult auth;

    test_app.mcp.AcceptHydrogenJWT = false;
    test_app.mcp.AcceptOidcIdP = true;
    mcp_auth_set_oidc_idp_hook(hook_fail);
    TEST_ASSERT_FALSE(mcp_validate_bearer("Bearer x", NULL, 0, &test_app.mcp, &test_app, &auth));
    TEST_ASSERT_EQUAL(MCP_AUTH_REJECT_OIDC_IDP, auth.reject_reason);
    mcp_auth_result_cleanup(&auth);
}

void test_validate_bearer_idp_accept(void) {
    McpAuthResult auth;

    test_app.mcp.AcceptHydrogenJWT = false;
    test_app.mcp.AcceptOidcIdP = true;
    test_app.oidc.issuer = (char *)"https://idp.example";
    mcp_auth_set_oidc_idp_hook(hook_idp_ok);
    TEST_ASSERT_TRUE(mcp_validate_bearer("Bearer x", NULL, 0, &test_app.mcp, &test_app, &auth));
    TEST_ASSERT_EQUAL(MCP_AUTH_KIND_OIDC_IDP, auth.kind);
    TEST_ASSERT_EQUAL_STRING("idp-sub", auth.sub);
    mcp_auth_result_cleanup(&auth);
}

void test_validate_bearer_rp_reject(void) {
    McpAuthResult auth;

    test_app.mcp.AcceptHydrogenJWT = false;
    test_app.mcp.AcceptOidcRp = true;
    mcp_auth_set_oidc_rp_hook(hook_fail);
    TEST_ASSERT_FALSE(mcp_validate_bearer("Bearer x", NULL, 0, &test_app.mcp, &test_app, &auth));
    TEST_ASSERT_EQUAL(MCP_AUTH_REJECT_OIDC_RP, auth.reject_reason);
    mcp_auth_result_cleanup(&auth);
}

void test_validate_bearer_rp_accept(void) {
    McpAuthResult auth;

    test_app.mcp.AcceptHydrogenJWT = false;
    test_app.mcp.AcceptOidcRp = true;
    mcp_auth_set_oidc_rp_hook(hook_idp_ok);
    TEST_ASSERT_TRUE(mcp_validate_bearer("Bearer x", NULL, 0, &test_app.mcp, &test_app, &auth));
    TEST_ASSERT_EQUAL(MCP_AUTH_KIND_OIDC_RP, auth.kind);
    mcp_auth_result_cleanup(&auth);
}

void test_validate_bearer_aud_mismatch(void) {
    McpAuthResult auth;

    test_app.mcp.AcceptHydrogenJWT = false;
    test_app.mcp.AcceptOidcIdP = true;
    mcp_auth_set_oidc_idp_hook(hook_idp_aud_bad);
    TEST_ASSERT_FALSE(mcp_validate_bearer("Bearer x", NULL, 0, &test_app.mcp, &test_app, &auth));
    TEST_ASSERT_EQUAL(MCP_AUTH_REJECT_AUD, auth.reject_reason);
    mcp_auth_result_cleanup(&auth);
}

void test_validate_bearer_scope_mismatch(void) {
    McpAuthResult auth;

    test_app.mcp.AcceptHydrogenJWT = false;
    test_app.mcp.AcceptOidcIdP = true;
    test_app.mcp.RequiredScopes[0] = strdup("mcp.read");
    test_app.mcp.RequiredScopeCount = 1;
    mcp_auth_set_oidc_idp_hook(hook_idp_scope_bad);
    TEST_ASSERT_FALSE(mcp_validate_bearer("Bearer x", NULL, 0, &test_app.mcp, &test_app, &auth));
    TEST_ASSERT_EQUAL(MCP_AUTH_REJECT_SCOPE, auth.reject_reason);
    mcp_auth_result_cleanup(&auth);
}

void test_www_authenticate_and_prm_shape(void) {
    char *www;
    char *prm;

    test_app.mcp.AcceptOidcIdP = true;
    test_app.oidc.enabled = true;
    test_app.oidc.issuer = (char *)"https://idp.example";

    www = mcp_www_authenticate_value(&test_app.mcp);
    TEST_ASSERT_NOT_NULL(www);
    TEST_ASSERT_NOT_NULL(strstr(www, "Bearer realm=\"hydrogen-mcp\""));
    TEST_ASSERT_NOT_NULL(strstr(www, "resource_metadata=\""));
    TEST_ASSERT_NOT_NULL(strstr(www, "/.well-known/oauth-protected-resource/mcp"));
    free(www);

    prm = mcp_prm_build(&test_app.mcp, &test_app);
    TEST_ASSERT_NOT_NULL(prm);
    TEST_ASSERT_NOT_NULL(strstr(prm, "\"resource\":\"http://127.0.0.1:3100/mcp\""));
    TEST_ASSERT_NOT_NULL(strstr(prm, "https://idp.example"));
    TEST_ASSERT_NOT_NULL(strstr(prm, "\"header\""));
    TEST_ASSERT_NOT_NULL(strstr(prm, "HS256"));
    TEST_ASSERT_NOT_NULL(strstr(prm, "RS256"));
    free(prm);
}

void test_send_unauthorized_header(void) {
    char *www;

    www = mcp_www_authenticate_value(&test_app.mcp);
    TEST_ASSERT_NOT_NULL(www);
    TEST_ASSERT_EQUAL(MHD_YES, mcp_send_unauthorized((struct MHD_Connection *)&mock_conn, &test_app.mcp));
    TEST_ASSERT_EQUAL(MHD_HTTP_UNAUTHORIZED, mock_mhd_get_last_status_code());
    TEST_ASSERT_TRUE(mock_mhd_header_was_added("WWW-Authenticate", www));
    free(www);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_validate_bearer_missing);
    RUN_TEST(test_validate_bearer_malformed);
    RUN_TEST(test_validate_bearer_reserved_hydrogen);
    RUN_TEST(test_validate_bearer_bad_hydrogen_jwt);
    RUN_TEST(test_validate_bearer_expired_hydrogen_jwt);
    RUN_TEST(test_validate_bearer_happy_hydrogen_jwt);
    RUN_TEST(test_validate_bearer_require_jwt_false);
    RUN_TEST(test_validate_bearer_idp_reject);
    RUN_TEST(test_validate_bearer_idp_accept);
    RUN_TEST(test_validate_bearer_rp_reject);
    RUN_TEST(test_validate_bearer_rp_accept);
    RUN_TEST(test_validate_bearer_aud_mismatch);
    RUN_TEST(test_validate_bearer_scope_mismatch);
    RUN_TEST(test_www_authenticate_and_prm_shape);
    RUN_TEST(test_send_unauthorized_header);
    return UNITY_END();
}
