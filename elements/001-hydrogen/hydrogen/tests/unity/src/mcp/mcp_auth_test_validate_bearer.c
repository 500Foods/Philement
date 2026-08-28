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
#include <src/api/auth/oidc_rp/oidc_rp_discovery.h>
#include <src/api/auth/oidc_rp/oidc_rp_http.h>

static AppConfig test_app;
static struct { int dummy; } mock_conn;

void test_validate_bearer_missing(void);
void test_validate_bearer_malformed(void);
void test_validate_bearer_reserved_hydrogen(void);
void test_validate_bearer_bad_hydrogen_jwt(void);
void test_validate_bearer_expired_hydrogen_jwt(void);
void test_validate_bearer_unavailable_hydrogen_jwt(void);
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
void test_prm_rp_issuers(void);
void test_auth_resource_explicit(void);
void test_prm_metadata_empty_resource(void);
void test_body_params_hydrogen(void);
void test_validate_null_cfg(void);
void test_scopes_and_aud_helpers(void);
void test_copy_oidc_and_cleanup_null(void);
void test_try_hydrogen_no_bearer(void);
void test_rp_hook_aud_and_scope(void);
void test_rp_no_app(void);
void test_derived_resource(void);
void test_send_unauthorized_create_fail(void);
void test_hydrogen_jwt_empty_database(void);
void test_idp_no_hook_no_context(void);
void test_idp_issuer_mismatch(void);
void test_idp_hook_fail_with_claims(void);
void test_idp_copy_fails_no_sub(void);
void test_try_one_rp_discovery_fail(void);
void test_try_one_rp_split_and_header_and_sig(void);
void test_try_oidc_rp_provider_loop(void);

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

static bool hook_fail_with_claims(const char *token, const AppConfig *app, OIDCTokenClaims **out) {
    OIDCTokenClaims *claims;

    (void)token;
    (void)app;
    claims = calloc(1, sizeof(*claims));
    TEST_ASSERT_NOT_NULL(claims);
    claims->sub = strdup("x");
    *out = claims;
    return false;
}

static bool hook_idp_no_sub(const char *token, const AppConfig *app, OIDCTokenClaims **out) {
    OIDCTokenClaims *claims;

    (void)token;
    (void)app;
    claims = calloc(1, sizeof(*claims));
    TEST_ASSERT_NOT_NULL(claims);
    claims->iss = strdup("https://idp.example");
    *out = claims;
    return true;
}

static const char *RP_DISC =
    "{"
    "\"issuer\":\"https://idp.example.com/realms/foo\","
    "\"authorization_endpoint\":\"https://idp.example.com/realms/foo/auth\","
    "\"token_endpoint\":\"https://idp.example.com/realms/foo/token\","
    "\"jwks_uri\":\"https://idp.example.com/realms/foo/jwks\""
    "}";

static void fill_rp_provider(OIDCRPProviderConfig *p) {
    memset(p, 0, sizeof(*p));
    p->name = (char *)"test-prov";
    p->issuer = (char *)"https://idp.example.com/realms/foo";
    p->verify_ssl = false;
    p->discovery_cache_seconds = 60;
}

void setUp(void) {
    mock_mhd_reset_all();
    mock_auth_service_jwt_reset_all();
    mcp_stats_reset();
    mcp_auth_set_oidc_idp_hook(NULL);
    mcp_auth_set_oidc_rp_hook(NULL);
    oidc_rp_http_test_clear_responses();
    apply_defaults();
    app_config = &test_app;
}

void tearDown(void) {
    mcp_auth_set_oidc_idp_hook(NULL);
    mcp_auth_set_oidc_rp_hook(NULL);
    oidc_rp_http_test_clear_responses();
    oidc_rp_discovery_shutdown();
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

void test_validate_bearer_unavailable_hydrogen_jwt(void) {
    McpAuthResult auth;
    jwt_validation_result_t programmed;

    memset(&programmed, 0, sizeof(programmed));
    programmed.valid = false;
    programmed.error = JWT_ERROR_UNAVAILABLE;
    mock_auth_service_jwt_set_validation_result(programmed);

    TEST_ASSERT_FALSE(mcp_validate_bearer("Bearer congested", NULL, 0, &test_app.mcp, &test_app, &auth));
    TEST_ASSERT_EQUAL(MCP_AUTH_REJECT_UNAVAILABLE, auth.reject_reason);
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

void test_prm_rp_issuers(void) {
    char *prm;

    test_app.mcp.AcceptHydrogenJWT = false;
    test_app.mcp.AcceptOidcRp = true;
    test_app.oidc_rp.provider_count = 1;
    test_app.oidc_rp.providers[0].issuer = (char *)"https://kc.example";

    prm = mcp_prm_build(&test_app.mcp, &test_app);
    TEST_ASSERT_NOT_NULL(prm);
    TEST_ASSERT_NOT_NULL(strstr(prm, "https://kc.example"));
    TEST_ASSERT_NOT_NULL(strstr(prm, "RS256"));
    TEST_ASSERT_NULL(strstr(prm, "HS256"));
    free(prm);
}

void test_auth_resource_explicit(void) {
    free(test_app.mcp.Resource);
    test_app.mcp.Resource = strdup("https://mcp.example/mcp");
    TEST_ASSERT_EQUAL_STRING("https://mcp.example/mcp", mcp_auth_resource(&test_app.mcp));
    TEST_ASSERT_EQUAL_STRING("", mcp_auth_resource(NULL));
}

void test_prm_metadata_empty_resource(void) {
    char *url = mcp_prm_metadata_url(NULL);
    TEST_ASSERT_NOT_NULL(url);
    TEST_ASSERT_EQUAL_STRING("/.well-known/oauth-protected-resource", url);
    free(url);
}

void test_body_params_hydrogen(void) {
    const char *body = "{\"params\":{\"_hydrogen\":{}}}";
    TEST_ASSERT_TRUE(mcp_auth_body_has_hydrogen(body, strlen(body)));
    TEST_ASSERT_FALSE(mcp_auth_body_has_hydrogen("{", 1));
}

void test_validate_null_cfg(void) {
    McpAuthResult auth;
    TEST_ASSERT_FALSE(mcp_validate_bearer("Bearer x", NULL, 0, NULL, &test_app, &auth));
    TEST_ASSERT_EQUAL(MCP_AUTH_REJECT_MALFORMED, auth.reject_reason);
    mcp_auth_result_cleanup(&auth);
}

void test_scopes_and_aud_helpers(void) {
    OIDCTokenClaims claims;
    MCPConfig cfg;

    memset(&claims, 0, sizeof(claims));
    TEST_ASSERT_TRUE(mcp_auth_aud_contains(NULL, "x"));
    TEST_ASSERT_TRUE(mcp_auth_aud_contains(&claims, ""));
    TEST_ASSERT_TRUE(mcp_auth_aud_contains(&claims, "http://r"));

    memset(&cfg, 0, sizeof(cfg));
    TEST_ASSERT_TRUE(mcp_auth_scopes_satisfied(NULL, NULL));
    cfg.RequiredScopeCount = 1;
    cfg.RequiredScopes[0] = (char *)"";
    TEST_ASSERT_TRUE(mcp_auth_scopes_satisfied("mcp.read", &cfg));
    cfg.RequiredScopes[0] = (char *)"mcp.read";
    TEST_ASSERT_FALSE(mcp_auth_scopes_satisfied(NULL, &cfg));
    TEST_ASSERT_TRUE(mcp_auth_scopes_satisfied("mcp.read mcp.write", &cfg));
}

void test_copy_oidc_and_cleanup_null(void) {
    McpAuthResult out;
    OIDCTokenClaims claims;

    mcp_auth_result_cleanup(NULL);
    TEST_ASSERT_FALSE(mcp_auth_copy_oidc(NULL, MCP_AUTH_KIND_OIDC_IDP, NULL));
    memset(&out, 0, sizeof(out));
    memset(&claims, 0, sizeof(claims));
    TEST_ASSERT_FALSE(mcp_auth_copy_oidc(&out, MCP_AUTH_KIND_OIDC_IDP, NULL));
    claims.sub = (char *)"s";
    claims.scope = (char *)"a b";
    TEST_ASSERT_TRUE(mcp_auth_copy_oidc(&out, MCP_AUTH_KIND_OIDC_RP, &claims));
    TEST_ASSERT_EQUAL_UINT(2, out.scope_count);
    mcp_auth_result_cleanup(&out);
}

void test_try_hydrogen_no_bearer(void) {
    McpAuthResult out;
    memset(&out, 0, sizeof(out));
    TEST_ASSERT_FALSE(mcp_try_hydrogen("Token x", &test_app.mcp, &out));
}

void test_rp_hook_aud_and_scope(void) {
    McpAuthResult auth;

    test_app.mcp.AcceptHydrogenJWT = false;
    test_app.mcp.AcceptOidcRp = true;
    mcp_auth_set_oidc_rp_hook(hook_idp_aud_bad);
    TEST_ASSERT_FALSE(mcp_validate_bearer("Bearer x", NULL, 0, &test_app.mcp, &test_app, &auth));
    TEST_ASSERT_EQUAL(MCP_AUTH_REJECT_AUD, auth.reject_reason);
    mcp_auth_result_cleanup(&auth);

    test_app.mcp.RequiredScopes[0] = strdup("mcp.read");
    test_app.mcp.RequiredScopeCount = 1;
    mcp_auth_set_oidc_rp_hook(hook_idp_scope_bad);
    TEST_ASSERT_FALSE(mcp_validate_bearer("Bearer x", NULL, 0, &test_app.mcp, &test_app, &auth));
    TEST_ASSERT_EQUAL(MCP_AUTH_REJECT_SCOPE, auth.reject_reason);
    mcp_auth_result_cleanup(&auth);
}

void test_rp_no_app(void) {
    McpAuthResult auth;
    McpAuthRejectReason detail = MCP_AUTH_REJECT_HYDROGEN_JWT;

    test_app.mcp.AcceptOidcRp = true;
    TEST_ASSERT_FALSE(mcp_try_oidc_rp("tok", &test_app.mcp, NULL, &auth, &detail));
    TEST_ASSERT_EQUAL(MCP_AUTH_REJECT_OIDC_RP, detail);
}

void test_derived_resource(void) {
    free(test_app.mcp.Resource);
    test_app.mcp.Resource = NULL;
    TEST_ASSERT_NOT_NULL(strstr(mcp_auth_resource(&test_app.mcp), "http://"));
}

void test_send_unauthorized_create_fail(void) {
    mock_mhd_set_create_response_should_fail(true);
    TEST_ASSERT_EQUAL(MHD_NO, mcp_send_unauthorized((struct MHD_Connection *)&mock_conn, &test_app.mcp));
}

void test_hydrogen_jwt_empty_database(void) {
    McpAuthResult out;
    jwt_validation_result_t programmed;
    jwt_claims_t claims;

    memset(&programmed, 0, sizeof(programmed));
    memset(&claims, 0, sizeof(claims));
    memset(&out, 0, sizeof(out));
    claims.sub = (char *)"u";
    claims.database = (char *)"";
    programmed.valid = true;
    programmed.claims = &claims;
    mock_auth_service_jwt_set_validation_result(programmed);
    TEST_ASSERT_FALSE(mcp_try_hydrogen("Bearer x", &test_app.mcp, &out));
}

void test_idp_no_hook_no_context(void) {
    McpAuthResult out;
    McpAuthRejectReason detail = MCP_AUTH_REJECT_HYDROGEN_JWT;

    memset(&out, 0, sizeof(out));
    test_app.mcp.AcceptOidcIdP = true;
    TEST_ASSERT_FALSE(mcp_try_oidc_idp("tok", &test_app.mcp, &test_app, &out, &detail));
    TEST_ASSERT_EQUAL(MCP_AUTH_REJECT_OIDC_IDP, detail);
}

void test_idp_issuer_mismatch(void) {
    McpAuthResult out;
    McpAuthRejectReason detail = MCP_AUTH_REJECT_HYDROGEN_JWT;

    memset(&out, 0, sizeof(out));
    test_app.mcp.AcceptOidcIdP = true;
    test_app.oidc.issuer = (char *)"https://other.example";
    mcp_auth_set_oidc_idp_hook(hook_idp_ok);
    TEST_ASSERT_FALSE(mcp_try_oidc_idp("tok", &test_app.mcp, &test_app, &out, &detail));
    TEST_ASSERT_EQUAL(MCP_AUTH_REJECT_OIDC_IDP, detail);
}

void test_idp_hook_fail_with_claims(void) {
    McpAuthResult out;
    McpAuthRejectReason detail = MCP_AUTH_REJECT_HYDROGEN_JWT;

    memset(&out, 0, sizeof(out));
    test_app.mcp.AcceptOidcIdP = true;
    mcp_auth_set_oidc_idp_hook(hook_fail_with_claims);
    TEST_ASSERT_FALSE(mcp_try_oidc_idp("tok", &test_app.mcp, &test_app, &out, &detail));
    TEST_ASSERT_EQUAL(MCP_AUTH_REJECT_OIDC_IDP, detail);
}

void test_idp_copy_fails_no_sub(void) {
    McpAuthResult out;
    McpAuthRejectReason detail = MCP_AUTH_REJECT_HYDROGEN_JWT;

    memset(&out, 0, sizeof(out));
    test_app.mcp.AcceptOidcIdP = true;
    mcp_auth_set_oidc_idp_hook(hook_idp_no_sub);
    TEST_ASSERT_FALSE(mcp_try_oidc_idp("tok", &test_app.mcp, &test_app, &out, &detail));
    TEST_ASSERT_EQUAL(MCP_AUTH_REJECT_OIDC_IDP, detail);
}

void test_try_one_rp_discovery_fail(void) {
    OIDCRPProviderConfig p;
    McpAuthRejectReason detail = MCP_AUTH_REJECT_HYDROGEN_JWT;
    OIDCTokenClaims *claims = NULL;

    fill_rp_provider(&p);
    TEST_ASSERT_FALSE(mcp_try_one_rp("a.b.c", &p, &test_app.mcp, &detail, &claims));
    TEST_ASSERT_EQUAL(MCP_AUTH_REJECT_OIDC_RP, detail);
}

void test_try_one_rp_split_and_header_and_sig(void) {
    OIDCRPProviderConfig p;
    McpAuthRejectReason detail = MCP_AUTH_REJECT_HYDROGEN_JWT;
    OIDCTokenClaims *claims = NULL;

    fill_rp_provider(&p);
    TEST_ASSERT_TRUE(oidc_rp_discovery_init());
    oidc_rp_http_test_set_response("well-known", 200, RP_DISC);
    TEST_ASSERT_FALSE(mcp_try_one_rp("not-a-jwt", &p, &test_app.mcp, &detail, &claims));
    TEST_ASSERT_EQUAL(MCP_AUTH_REJECT_OIDC_RP, detail);

    detail = MCP_AUTH_REJECT_HYDROGEN_JWT;
    oidc_rp_http_test_set_response("well-known", 200, RP_DISC);
    TEST_ASSERT_FALSE(mcp_try_one_rp("e30.e30.e30", &p, &test_app.mcp, &detail, &claims));
    TEST_ASSERT_EQUAL(MCP_AUTH_REJECT_OIDC_RP, detail);

    detail = MCP_AUTH_REJECT_HYDROGEN_JWT;
    TEST_ASSERT_FALSE(mcp_try_one_rp(
        "eyJhbGciOiJSUzI1NiIsImtpZCI6ImsifQ.eyJzdWIiOiJ4In0.YQ",
        &p, &test_app.mcp, &detail, &claims));
    TEST_ASSERT_EQUAL(MCP_AUTH_REJECT_OIDC_RP, detail);
}

void test_try_oidc_rp_provider_loop(void) {
    McpAuthResult out;
    McpAuthRejectReason detail = MCP_AUTH_REJECT_HYDROGEN_JWT;

    memset(&out, 0, sizeof(out));
    test_app.mcp.AcceptOidcRp = true;
    test_app.oidc_rp.provider_count = 1;
    fill_rp_provider(&test_app.oidc_rp.providers[0]);
    TEST_ASSERT_FALSE(mcp_try_oidc_rp("tok", &test_app.mcp, &test_app, &out, &detail));
    TEST_ASSERT_EQUAL(MCP_AUTH_REJECT_OIDC_RP, detail);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_validate_bearer_missing);
    RUN_TEST(test_validate_bearer_malformed);
    RUN_TEST(test_validate_bearer_reserved_hydrogen);
    RUN_TEST(test_validate_bearer_bad_hydrogen_jwt);
    RUN_TEST(test_validate_bearer_expired_hydrogen_jwt);
    RUN_TEST(test_validate_bearer_unavailable_hydrogen_jwt);
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
    RUN_TEST(test_prm_rp_issuers);
    RUN_TEST(test_auth_resource_explicit);
    RUN_TEST(test_prm_metadata_empty_resource);
    RUN_TEST(test_body_params_hydrogen);
    RUN_TEST(test_validate_null_cfg);
    RUN_TEST(test_scopes_and_aud_helpers);
    RUN_TEST(test_copy_oidc_and_cleanup_null);
    RUN_TEST(test_try_hydrogen_no_bearer);
    RUN_TEST(test_rp_hook_aud_and_scope);
    RUN_TEST(test_rp_no_app);
    RUN_TEST(test_derived_resource);
    RUN_TEST(test_send_unauthorized_create_fail);
    RUN_TEST(test_hydrogen_jwt_empty_database);
    RUN_TEST(test_idp_no_hook_no_context);
    RUN_TEST(test_idp_issuer_mismatch);
    RUN_TEST(test_idp_hook_fail_with_claims);
    RUN_TEST(test_idp_copy_fails_no_sub);
    RUN_TEST(test_try_one_rp_discovery_fail);
    RUN_TEST(test_try_one_rp_split_and_header_and_sig);
    RUN_TEST(test_try_oidc_rp_provider_loop);
    return UNITY_END();
}
