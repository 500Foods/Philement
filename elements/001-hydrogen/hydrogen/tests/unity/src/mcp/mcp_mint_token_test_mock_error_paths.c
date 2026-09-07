/*
 * Unity Test File: mcp_mint_token_mock_error_paths
 *
 * Tests error paths in the mint-token module that require mock system and crypto
 * infrastructure. The mint-token source pulls in system and crypto mocks under
 * UNITY_TEST_MODE so utils_base64url_encode and asprintf are redirected.
 * get_jwt_config uses mock calloc/strdup via CMake.
 */
#include <src/hydrogen.h>
#include <unity.h>

#ifndef USE_MOCK_AUTH_SERVICE_JWT
#define USE_MOCK_AUTH_SERVICE_JWT
#endif
#ifndef USE_MOCK_CRYPTO
#define USE_MOCK_CRYPTO
#endif
#ifndef USE_MOCK_LIBMICROHTTPD
#define USE_MOCK_LIBMICROHTTPD
#endif
#ifndef USE_MOCK_SYSTEM
#define USE_MOCK_SYSTEM
#endif

#include <unity/mocks/mock_auth_service_jwt.h>
#include <unity/mocks/mock_crypto.h>
#include <unity/mocks/mock_system.h>
#include <src/mcp/mcp_mint_token.h>
#include <src/mcp/mcp_auth.h>
#include <src/config/config_mcp.h>
#include <string.h>

void test_mint_missing_resource(void);
void test_mint_config_failure(void);
void test_mint_config_no_secret(void);
void test_mint_resource_url_empty(void);
void test_mint_asprintf_header_failure(void);
void test_mint_asprintf_payload_failure(void);
void test_mint_asprintf_signing_input_failure(void);
void test_mint_asprintf_jwt_failure(void);
void test_mint_base64url_jti_failure(void);
void test_mint_base64url_header_failure(void);
void test_mint_base64url_payload_failure(void);
void test_mint_base64url_signature_failure(void);

void setUp(void) {
    mock_system_reset_all();
    mock_auth_service_jwt_reset_all();
    mock_crypto_reset_all();
}

void tearDown(void) {
    mock_system_reset_all();
    mock_auth_service_jwt_reset_all();
    mock_crypto_reset_all();
}

static MCPConfig *make_config(void) {
    MCPConfig *cfg = calloc(1, sizeof(MCPConfig));
    TEST_ASSERT_NOT_NULL(cfg);
    mcp_config_apply_defaults(cfg);
    cfg->Resource = strdup("http://127.0.0.1:3100/mcp");
    cfg->Enabled = true;
    cfg->AcceptHydrogenJWT = true;
    return cfg;
}

static void free_config(MCPConfig *cfg) {
    if (cfg) {
        free(cfg->Resource);
        free(cfg);
    }
}

/* Line 69-70: aud is empty (cfg is NULL, so mcp_auth_resource returns "") */
void test_mint_missing_resource(void) {
    /* Pass NULL cfg — mcp_auth_resource(NULL) returns "" */
    char *tok = mcp_mint_resource_token(NULL, "sub", "db", "r", 900, "cid-missing-res");
    TEST_ASSERT_NULL(tok);
}

/* Lines 78-79: get_jwt_config returns NULL (calloc fails inside get_jwt_config) */
void test_mint_config_failure(void) {
    MCPConfig *cfg = make_config();
    /* get_jwt_config calls calloc first — make it fail on call 1 */
    mock_system_set_calloc_failure(1);

    char *tok = mcp_mint_resource_token(cfg, "sub", "db", "r", 900, "cid-config-fail");
    TEST_ASSERT_NULL(tok);

    mock_system_set_calloc_failure(0);
    free_config(cfg);
}

/* Lines 82-84: config->hmac_secret is NULL (strdup fails inside get_jwt_config)
   get_jwt_config calls calloc (malloc call 1, succeeds) then strdup (malloc call 2, fails) */
void test_mint_config_no_secret(void) {
    MCPConfig *cfg = make_config();
    mock_system_set_malloc_failure(2);

    char *tok = mcp_mint_resource_token(cfg, "sub", "db", "r", 900, "cid-no-secret");
    TEST_ASSERT_NULL(tok);

    mock_system_set_malloc_failure(0);
    free_config(cfg);
}

/* Lines 90-93: asprintf header fails (asprintf call #1) */
void test_mint_asprintf_header_failure(void) {
    MCPConfig *cfg = make_config();
    mock_system_set_asprintf_failure(1);

    char *tok = mcp_mint_resource_token(cfg, "sub", "db", "r", 900, "cid-asp-fail");
    TEST_ASSERT_NULL(tok);

    free_config(cfg);
}

/* Lines 122-126: asprintf payload fails (asprintf call #2) */
void test_mint_asprintf_payload_failure(void) {
    MCPConfig *cfg = make_config();
    mock_system_set_asprintf_failure(2);

    char *tok = mcp_mint_resource_token(cfg, "sub", "db", "r", 900, "cid-asp-payload");
    TEST_ASSERT_NULL(tok);

    free_config(cfg);
}

/* Lines 143-150: asprintf signing_input fails (asprintf call #3) */
void test_mint_asprintf_signing_input_failure(void) {
    MCPConfig *cfg = make_config();
    mock_system_set_asprintf_failure(3);

    char *tok = mcp_mint_resource_token(cfg, "sub", "db", "r", 900, "cid-asp-signing");
    TEST_ASSERT_NULL(tok);

    free_config(cfg);
}

/* Lines 176-186: asprintf jwt fails (asprintf call #4) */
void test_mint_asprintf_jwt_failure(void) {
    MCPConfig *cfg = make_config();
    mock_system_set_asprintf_failure(4);

    char *tok = mcp_mint_resource_token(cfg, "sub", "db", "r", 900, "cid-asp-jwt");
    TEST_ASSERT_NULL(tok);

    free_config(cfg);
}

/* Lines 100-104: RAND_bytes... can't be tested. But jti base64url encode (105)
   can be tested: utils_base64url_encode call #1 */
void test_mint_base64url_jti_failure(void) {
    MCPConfig *cfg = make_config();
    mock_crypto_set_base64url_encode_failure(1);

    char *tok = mcp_mint_resource_token(cfg, "sub", "db", "r", 900, "cid-b64-jti");
    TEST_ASSERT_NULL(tok);

    free_config(cfg);
}

/* Lines 129-133 + 135-140: utils_base64url_encode for header (call #2) */
void test_mint_base64url_header_failure(void) {
    MCPConfig *cfg = make_config();
    mock_crypto_set_base64url_encode_failure(2);

    char *tok = mcp_mint_resource_token(cfg, "sub", "db", "r", 900, "cid-b64-header");
    TEST_ASSERT_NULL(tok);

    free_config(cfg);
}

/* utils_base64url_encode for payload (call #3) */
void test_mint_base64url_payload_failure(void) {
    MCPConfig *cfg = make_config();
    mock_crypto_set_base64url_encode_failure(3);

    char *tok = mcp_mint_resource_token(cfg, "sub", "db", "r", 900, "cid-b64-payload");
    TEST_ASSERT_NULL(tok);

    free_config(cfg);
}

/* utils_base64url_encode for signature (call #4) */
void test_mint_base64url_signature_failure(void) {
    MCPConfig *cfg = make_config();
    mock_crypto_set_base64url_encode_failure(4);

    char *tok = mcp_mint_resource_token(cfg, "sub", "db", "r", 900, "cid-b64-sig");
    TEST_ASSERT_NULL(tok);

    free_config(cfg);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_mint_missing_resource);
    RUN_TEST(test_mint_config_failure);
    RUN_TEST(test_mint_config_no_secret);
    RUN_TEST(test_mint_asprintf_header_failure);
    RUN_TEST(test_mint_asprintf_payload_failure);
    RUN_TEST(test_mint_asprintf_signing_input_failure);
    RUN_TEST(test_mint_asprintf_jwt_failure);
    RUN_TEST(test_mint_base64url_jti_failure);
    RUN_TEST(test_mint_base64url_header_failure);
    RUN_TEST(test_mint_base64url_payload_failure);
    RUN_TEST(test_mint_base64url_signature_failure);
    return UNITY_END();
}
