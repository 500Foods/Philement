/*
 * Unity Test File: OIDC RP token-endpoint client (Phase 11).
 *
 * Covers `oidc_rp_exchange_code` and supporting helpers from
 * `src/api/auth/oidc_rp/oidc_rp_token.c`. The token-endpoint POST
 * is exercised through the http test seam
 * (`oidc_rp_http_test_set_response`) — we never actually open a
 * socket; the seam returns canned status + body for the next call to
 * `oidc_rp_http_post`.
 *
 * Coverage:
 *   - NULL/empty input → BAD_INPUT (every required argument).
 *   - Missing token endpoint → NO_TOKEN_ENDPOINT.
 *   - client_secret_basic without a configured secret → BAD_INPUT.
 *   - Transport failure (status==0) → TRANSPORT.
 *   - 200 with valid id_token → OK; response struct populated.
 *   - 200 missing id_token → BAD_RESPONSE.
 *   - 200 with malformed JSON → BAD_RESPONSE.
 *   - 400 with `error=invalid_grant` → INVALID_GRANT.
 *   - 400 with `error=invalid_request` → INVALID_GRANT (mapped).
 *   - 400 with `error=unauthorized_client` → INVALID_CLIENT.
 *   - 401 with no body → INVALID_CLIENT (default for 401).
 *   - 4xx with unknown error code → OTHER.
 *   - 4xx with no JSON body → OTHER.
 *   - 5xx → SERVER.
 *   - error-name table covers every enum value.
 *
 * The lifecycle test guarantees the test seam is drained between
 * tests so no fixture leaks into the next.
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/api/auth/oidc_rp/oidc_rp_token.h>
#include <src/api/auth/oidc_rp/oidc_rp_http.h>
#include <src/config/config_oidc_rp.h>

#include <stdlib.h>
#include <string.h>

// Forward declarations.
void test_response_free_handles_null(void);
void test_error_name_returns_stable_strings(void);
void test_exchange_rejects_null_provider(void);
void test_exchange_rejects_null_out(void);
void test_exchange_rejects_missing_client_id(void);
void test_exchange_rejects_missing_code(void);
void test_exchange_rejects_missing_redirect_uri(void);
void test_exchange_rejects_missing_code_verifier(void);
void test_exchange_rejects_missing_token_endpoint(void);
void test_exchange_basic_auth_requires_client_secret(void);
void test_exchange_post_auth_works_without_basic_secret(void);
void test_exchange_transport_failure_maps_to_transport(void);
void test_exchange_success_populates_response(void);
void test_exchange_success_without_optional_fields(void);
void test_exchange_success_with_malformed_json_returns_bad_response(void);
void test_exchange_success_without_id_token_returns_bad_response(void);
void test_exchange_400_invalid_grant_mapped(void);
void test_exchange_400_invalid_request_mapped_to_invalid_grant(void);
void test_exchange_400_unauthorized_client_mapped_to_invalid_client(void);
void test_exchange_400_unsupported_grant_type_mapped(void);
void test_exchange_401_with_no_body_defaults_to_invalid_client(void);
void test_exchange_4xx_with_unknown_error_returns_other(void);
void test_exchange_5xx_maps_to_server_error(void);

// ---------------------------------------------------------------------------
// Fixtures: a fully-populated provider config used by every success path.
// ---------------------------------------------------------------------------

static OIDCRPProviderConfig make_provider(OIDCRPAuthMethod method) {
    OIDCRPProviderConfig p;
    memset(&p, 0, sizeof(p));
    p.name = (char *)"test";
    p.client_id = (char *)"lithium-web";
    p.client_secret = (char *)"super-secret-value";
    p.redirect_uri = (char *)"http://localhost:5243/api/auth/oidc/callback";
    p.scopes = (char *)"openid profile email";
    p.verify_ssl = false;
    p.auth_method = method;
    p.allowed_algorithms[0] = (char *)"RS256";
    p.allowed_algorithm_count = 1;
    p.discovery_cache_seconds = 60;
    p.jwks_cache_seconds = 60;
    p.state_ttl_seconds = 600;
    p.handoff_ttl_seconds = 60;
    return p;
}

void setUp(void) {
    oidc_rp_http_test_clear_responses();
}

void tearDown(void) {
    oidc_rp_http_test_clear_responses();
}

// ---------------------------------------------------------------------------
// Lifecycle / argument-validation tests
// ---------------------------------------------------------------------------

void test_response_free_handles_null(void) {
    oidc_rp_token_response_free(NULL);  // must not crash
    TEST_PASS();
}

void test_error_name_returns_stable_strings(void) {
    TEST_ASSERT_EQUAL_STRING("ok",                oidc_rp_token_error_name(OIDC_RP_TOKEN_OK));
    TEST_ASSERT_EQUAL_STRING("bad_input",         oidc_rp_token_error_name(OIDC_RP_TOKEN_ERR_BAD_INPUT));
    TEST_ASSERT_EQUAL_STRING("no_token_endpoint", oidc_rp_token_error_name(OIDC_RP_TOKEN_ERR_NO_TOKEN_ENDPOINT));
    TEST_ASSERT_EQUAL_STRING("transport",         oidc_rp_token_error_name(OIDC_RP_TOKEN_ERR_TRANSPORT));
    TEST_ASSERT_EQUAL_STRING("invalid_grant",     oidc_rp_token_error_name(OIDC_RP_TOKEN_ERR_INVALID_GRANT));
    TEST_ASSERT_EQUAL_STRING("invalid_client",    oidc_rp_token_error_name(OIDC_RP_TOKEN_ERR_INVALID_CLIENT));
    TEST_ASSERT_EQUAL_STRING("server_error",      oidc_rp_token_error_name(OIDC_RP_TOKEN_ERR_SERVER));
    TEST_ASSERT_EQUAL_STRING("bad_response",      oidc_rp_token_error_name(OIDC_RP_TOKEN_ERR_BAD_RESPONSE));
    TEST_ASSERT_EQUAL_STRING("other",             oidc_rp_token_error_name(OIDC_RP_TOKEN_ERR_OTHER));
    TEST_ASSERT_EQUAL_STRING("unknown",           oidc_rp_token_error_name((OidcRpTokenError)999));
}

void test_exchange_rejects_null_provider(void) {
    OidcRpTokenResponse *r = NULL;
    TEST_ASSERT_EQUAL_INT(OIDC_RP_TOKEN_ERR_BAD_INPUT,
        oidc_rp_exchange_code(NULL, "https://idp/token", "c", "uri", "v", &r));
    TEST_ASSERT_NULL(r);
}

void test_exchange_rejects_null_out(void) {
    OIDCRPProviderConfig p = make_provider(OIDC_RP_AUTH_METHOD_CLIENT_SECRET_BASIC);
    TEST_ASSERT_EQUAL_INT(OIDC_RP_TOKEN_ERR_BAD_INPUT,
        oidc_rp_exchange_code(&p, "https://idp/token", "c", "uri", "v", NULL));
}

void test_exchange_rejects_missing_client_id(void) {
    OIDCRPProviderConfig p = make_provider(OIDC_RP_AUTH_METHOD_CLIENT_SECRET_BASIC);
    p.client_id = NULL;
    OidcRpTokenResponse *r = NULL;
    TEST_ASSERT_EQUAL_INT(OIDC_RP_TOKEN_ERR_BAD_INPUT,
        oidc_rp_exchange_code(&p, "https://idp/token", "c", "uri", "v", &r));
    TEST_ASSERT_NULL(r);
}

void test_exchange_rejects_missing_code(void) {
    OIDCRPProviderConfig p = make_provider(OIDC_RP_AUTH_METHOD_CLIENT_SECRET_BASIC);
    OidcRpTokenResponse *r = NULL;
    TEST_ASSERT_EQUAL_INT(OIDC_RP_TOKEN_ERR_BAD_INPUT,
        oidc_rp_exchange_code(&p, "https://idp/token", "", "uri", "v", &r));
    TEST_ASSERT_NULL(r);

    TEST_ASSERT_EQUAL_INT(OIDC_RP_TOKEN_ERR_BAD_INPUT,
        oidc_rp_exchange_code(&p, "https://idp/token", NULL, "uri", "v", &r));
    TEST_ASSERT_NULL(r);
}

void test_exchange_rejects_missing_redirect_uri(void) {
    OIDCRPProviderConfig p = make_provider(OIDC_RP_AUTH_METHOD_CLIENT_SECRET_BASIC);
    OidcRpTokenResponse *r = NULL;
    TEST_ASSERT_EQUAL_INT(OIDC_RP_TOKEN_ERR_BAD_INPUT,
        oidc_rp_exchange_code(&p, "https://idp/token", "c", NULL, "v", &r));
    TEST_ASSERT_NULL(r);
}

void test_exchange_rejects_missing_code_verifier(void) {
    OIDCRPProviderConfig p = make_provider(OIDC_RP_AUTH_METHOD_CLIENT_SECRET_BASIC);
    OidcRpTokenResponse *r = NULL;
    TEST_ASSERT_EQUAL_INT(OIDC_RP_TOKEN_ERR_BAD_INPUT,
        oidc_rp_exchange_code(&p, "https://idp/token", "c", "uri", NULL, &r));
    TEST_ASSERT_NULL(r);
}

void test_exchange_rejects_missing_token_endpoint(void) {
    OIDCRPProviderConfig p = make_provider(OIDC_RP_AUTH_METHOD_CLIENT_SECRET_BASIC);
    OidcRpTokenResponse *r = NULL;
    TEST_ASSERT_EQUAL_INT(OIDC_RP_TOKEN_ERR_NO_TOKEN_ENDPOINT,
        oidc_rp_exchange_code(&p, NULL, "c", "uri", "v", &r));
    TEST_ASSERT_NULL(r);

    TEST_ASSERT_EQUAL_INT(OIDC_RP_TOKEN_ERR_NO_TOKEN_ENDPOINT,
        oidc_rp_exchange_code(&p, "", "c", "uri", "v", &r));
    TEST_ASSERT_NULL(r);
}

void test_exchange_basic_auth_requires_client_secret(void) {
    OIDCRPProviderConfig p = make_provider(OIDC_RP_AUTH_METHOD_CLIENT_SECRET_BASIC);
    p.client_secret = NULL;
    OidcRpTokenResponse *r = NULL;
    TEST_ASSERT_EQUAL_INT(OIDC_RP_TOKEN_ERR_BAD_INPUT,
        oidc_rp_exchange_code(&p, "https://idp/token", "c", "uri", "v", &r));
    TEST_ASSERT_NULL(r);
}

void test_exchange_post_auth_works_without_basic_secret(void) {
    // client_secret_post is allowed to send an empty client_secret in
    // the body (e.g. for public clients). The exchange itself does
    // not gate on this; the IdP will reject it. We just verify our
    // pre-flight check is specific to BASIC.
    OIDCRPProviderConfig p = make_provider(OIDC_RP_AUTH_METHOD_CLIENT_SECRET_POST);
    p.client_secret = NULL;

    // Canned 200 response so the exchange does not actually try to
    // open a socket; we only care that our pre-flight check did NOT
    // trip.
    oidc_rp_http_test_set_response(NULL, 200,
        "{\"id_token\":\"itok\",\"token_type\":\"Bearer\"}");

    OidcRpTokenResponse *r = NULL;
    TEST_ASSERT_EQUAL_INT(OIDC_RP_TOKEN_OK,
        oidc_rp_exchange_code(&p, "https://idp/token", "c", "uri", "v", &r));
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_EQUAL_STRING("itok", r->id_token);
    oidc_rp_token_response_free(r);
}

// ---------------------------------------------------------------------------
// Network-layer translation
// ---------------------------------------------------------------------------

void test_exchange_transport_failure_maps_to_transport(void) {
    // Status==0, body==NULL ⇒ transport failure simulation.
    oidc_rp_http_test_set_response(NULL, 0, NULL);

    OIDCRPProviderConfig p = make_provider(OIDC_RP_AUTH_METHOD_CLIENT_SECRET_BASIC);
    OidcRpTokenResponse *r = NULL;
    TEST_ASSERT_EQUAL_INT(OIDC_RP_TOKEN_ERR_TRANSPORT,
        oidc_rp_exchange_code(&p, "https://idp/token", "c", "uri", "v", &r));
    TEST_ASSERT_NULL(r);
}

// ---------------------------------------------------------------------------
// Success paths
// ---------------------------------------------------------------------------

void test_exchange_success_populates_response(void) {
    oidc_rp_http_test_set_response(NULL, 200,
        "{"
        "\"id_token\":\"id-tok-value\","
        "\"access_token\":\"acc-tok-value\","
        "\"token_type\":\"Bearer\","
        "\"expires_in\":300,"
        "\"scope\":\"openid profile email\""
        "}");

    OIDCRPProviderConfig p = make_provider(OIDC_RP_AUTH_METHOD_CLIENT_SECRET_BASIC);
    OidcRpTokenResponse *r = NULL;
    TEST_ASSERT_EQUAL_INT(OIDC_RP_TOKEN_OK,
        oidc_rp_exchange_code(&p, "https://idp/token", "c", "uri", "v", &r));
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_EQUAL_STRING("id-tok-value",  r->id_token);
    TEST_ASSERT_EQUAL_STRING("acc-tok-value", r->access_token);
    TEST_ASSERT_EQUAL_STRING("Bearer",        r->token_type);
    TEST_ASSERT_EQUAL_INT(300,                (int)r->expires_in);
    oidc_rp_token_response_free(r);
}

void test_exchange_success_without_optional_fields(void) {
    // Spec requires id_token (Hydrogen always sends openid scope).
    // access_token, token_type, expires_in are nominally required by
    // RFC 6749 §5.1 but we tolerate IdPs that omit some of them.
    oidc_rp_http_test_set_response(NULL, 200,
        "{\"id_token\":\"only-id-token\"}");

    OIDCRPProviderConfig p = make_provider(OIDC_RP_AUTH_METHOD_CLIENT_SECRET_BASIC);
    OidcRpTokenResponse *r = NULL;
    TEST_ASSERT_EQUAL_INT(OIDC_RP_TOKEN_OK,
        oidc_rp_exchange_code(&p, "https://idp/token", "c", "uri", "v", &r));
    TEST_ASSERT_NOT_NULL(r);
    TEST_ASSERT_EQUAL_STRING("only-id-token", r->id_token);
    TEST_ASSERT_NULL(r->access_token);
    TEST_ASSERT_NULL(r->token_type);
    TEST_ASSERT_EQUAL_INT(0, (int)r->expires_in);
    oidc_rp_token_response_free(r);
}

void test_exchange_success_with_malformed_json_returns_bad_response(void) {
    oidc_rp_http_test_set_response(NULL, 200, "{not even close to JSON");

    OIDCRPProviderConfig p = make_provider(OIDC_RP_AUTH_METHOD_CLIENT_SECRET_BASIC);
    OidcRpTokenResponse *r = NULL;
    TEST_ASSERT_EQUAL_INT(OIDC_RP_TOKEN_ERR_BAD_RESPONSE,
        oidc_rp_exchange_code(&p, "https://idp/token", "c", "uri", "v", &r));
    TEST_ASSERT_NULL(r);
}

void test_exchange_success_without_id_token_returns_bad_response(void) {
    // An OAuth2-only flow (no id_token) is not enough — Hydrogen
    // requires the OIDC layer.
    oidc_rp_http_test_set_response(NULL, 200,
        "{\"access_token\":\"acc-only\",\"token_type\":\"Bearer\"}");

    OIDCRPProviderConfig p = make_provider(OIDC_RP_AUTH_METHOD_CLIENT_SECRET_BASIC);
    OidcRpTokenResponse *r = NULL;
    TEST_ASSERT_EQUAL_INT(OIDC_RP_TOKEN_ERR_BAD_RESPONSE,
        oidc_rp_exchange_code(&p, "https://idp/token", "c", "uri", "v", &r));
    TEST_ASSERT_NULL(r);
}

// ---------------------------------------------------------------------------
// 4xx / 5xx error mapping
// ---------------------------------------------------------------------------

void test_exchange_400_invalid_grant_mapped(void) {
    oidc_rp_http_test_set_response(NULL, 400,
        "{\"error\":\"invalid_grant\",\"error_description\":\"code expired\"}");

    OIDCRPProviderConfig p = make_provider(OIDC_RP_AUTH_METHOD_CLIENT_SECRET_BASIC);
    OidcRpTokenResponse *r = NULL;
    TEST_ASSERT_EQUAL_INT(OIDC_RP_TOKEN_ERR_INVALID_GRANT,
        oidc_rp_exchange_code(&p, "https://idp/token", "c", "uri", "v", &r));
    TEST_ASSERT_NULL(r);
}

void test_exchange_400_invalid_request_mapped_to_invalid_grant(void) {
    oidc_rp_http_test_set_response(NULL, 400,
        "{\"error\":\"invalid_request\"}");

    OIDCRPProviderConfig p = make_provider(OIDC_RP_AUTH_METHOD_CLIENT_SECRET_BASIC);
    OidcRpTokenResponse *r = NULL;
    TEST_ASSERT_EQUAL_INT(OIDC_RP_TOKEN_ERR_INVALID_GRANT,
        oidc_rp_exchange_code(&p, "https://idp/token", "c", "uri", "v", &r));
    TEST_ASSERT_NULL(r);
}

void test_exchange_400_unauthorized_client_mapped_to_invalid_client(void) {
    oidc_rp_http_test_set_response(NULL, 400,
        "{\"error\":\"unauthorized_client\"}");

    OIDCRPProviderConfig p = make_provider(OIDC_RP_AUTH_METHOD_CLIENT_SECRET_BASIC);
    OidcRpTokenResponse *r = NULL;
    TEST_ASSERT_EQUAL_INT(OIDC_RP_TOKEN_ERR_INVALID_CLIENT,
        oidc_rp_exchange_code(&p, "https://idp/token", "c", "uri", "v", &r));
    TEST_ASSERT_NULL(r);
}

void test_exchange_400_unsupported_grant_type_mapped(void) {
    oidc_rp_http_test_set_response(NULL, 400,
        "{\"error\":\"unsupported_grant_type\"}");

    OIDCRPProviderConfig p = make_provider(OIDC_RP_AUTH_METHOD_CLIENT_SECRET_BASIC);
    OidcRpTokenResponse *r = NULL;
    TEST_ASSERT_EQUAL_INT(OIDC_RP_TOKEN_ERR_INVALID_GRANT,
        oidc_rp_exchange_code(&p, "https://idp/token", "c", "uri", "v", &r));
    TEST_ASSERT_NULL(r);
}

void test_exchange_401_with_no_body_defaults_to_invalid_client(void) {
    // A 401 with no parsable JSON body almost always means the IdP
    // rejected the Authorization header. Map to invalid_client.
    oidc_rp_http_test_set_response(NULL, 401, "");

    OIDCRPProviderConfig p = make_provider(OIDC_RP_AUTH_METHOD_CLIENT_SECRET_BASIC);
    OidcRpTokenResponse *r = NULL;
    TEST_ASSERT_EQUAL_INT(OIDC_RP_TOKEN_ERR_INVALID_CLIENT,
        oidc_rp_exchange_code(&p, "https://idp/token", "c", "uri", "v", &r));
    TEST_ASSERT_NULL(r);
}

void test_exchange_4xx_with_unknown_error_returns_other(void) {
    oidc_rp_http_test_set_response(NULL, 403,
        "{\"error\":\"some_keycloak_specific_thing\"}");

    OIDCRPProviderConfig p = make_provider(OIDC_RP_AUTH_METHOD_CLIENT_SECRET_BASIC);
    OidcRpTokenResponse *r = NULL;
    TEST_ASSERT_EQUAL_INT(OIDC_RP_TOKEN_ERR_OTHER,
        oidc_rp_exchange_code(&p, "https://idp/token", "c", "uri", "v", &r));
    TEST_ASSERT_NULL(r);
}

void test_exchange_5xx_maps_to_server_error(void) {
    oidc_rp_http_test_set_response(NULL, 503,
        "<html>Service Unavailable</html>");

    OIDCRPProviderConfig p = make_provider(OIDC_RP_AUTH_METHOD_CLIENT_SECRET_BASIC);
    OidcRpTokenResponse *r = NULL;
    TEST_ASSERT_EQUAL_INT(OIDC_RP_TOKEN_ERR_SERVER,
        oidc_rp_exchange_code(&p, "https://idp/token", "c", "uri", "v", &r));
    TEST_ASSERT_NULL(r);
}

// ---------------------------------------------------------------------------
// Test runner
// ---------------------------------------------------------------------------

int main(void) {
    UNITY_BEGIN();

    // Lifecycle / argument validation
    RUN_TEST(test_response_free_handles_null);
    RUN_TEST(test_error_name_returns_stable_strings);
    RUN_TEST(test_exchange_rejects_null_provider);
    RUN_TEST(test_exchange_rejects_null_out);
    RUN_TEST(test_exchange_rejects_missing_client_id);
    RUN_TEST(test_exchange_rejects_missing_code);
    RUN_TEST(test_exchange_rejects_missing_redirect_uri);
    RUN_TEST(test_exchange_rejects_missing_code_verifier);
    RUN_TEST(test_exchange_rejects_missing_token_endpoint);
    RUN_TEST(test_exchange_basic_auth_requires_client_secret);
    RUN_TEST(test_exchange_post_auth_works_without_basic_secret);

    // Network layer
    RUN_TEST(test_exchange_transport_failure_maps_to_transport);

    // Success paths
    RUN_TEST(test_exchange_success_populates_response);
    RUN_TEST(test_exchange_success_without_optional_fields);
    RUN_TEST(test_exchange_success_with_malformed_json_returns_bad_response);
    RUN_TEST(test_exchange_success_without_id_token_returns_bad_response);

    // Error mapping
    RUN_TEST(test_exchange_400_invalid_grant_mapped);
    RUN_TEST(test_exchange_400_invalid_request_mapped_to_invalid_grant);
    RUN_TEST(test_exchange_400_unauthorized_client_mapped_to_invalid_client);
    RUN_TEST(test_exchange_400_unsupported_grant_type_mapped);
    RUN_TEST(test_exchange_401_with_no_body_defaults_to_invalid_client);
    RUN_TEST(test_exchange_4xx_with_unknown_error_returns_other);
    RUN_TEST(test_exchange_5xx_maps_to_server_error);

    return UNITY_END();
}
