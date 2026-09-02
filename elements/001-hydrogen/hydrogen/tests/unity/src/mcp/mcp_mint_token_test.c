/*
 * Unity Test File: mcp_mint_token
 * Phase 8a — proves the MCP resource token mint primitive:
 *   1) claims shape (sub, aud=mcp_auth_resource(cfg), database, roles, jti, exp, iat, nbf)
 *   2) TTL within range
 *   3) accepted by mcp_try_hydrogen (round-trip through validate_jwt mock)
 *   4) rejected by chat helpers (validate_chat_jwt_claims / check_chat_jwt_claims)
 *   5) fail-closed on missing sub/database/null cfg
 */
#include <src/hydrogen.h>
#include <unity.h>

#define USE_MOCK_AUTH_SERVICE_JWT

#include <unity/mocks/mock_auth_service_jwt.h>
#include <unity/mocks/mock_libmicrohttpd.h>
#include <src/mcp/mcp_mint_token.h>
#include <src/mcp/mcp_auth.h>
#include <src/config/config_mcp.h>
#include <src/api/conduit/helpers/auth_jwt_helper.h>

static AppConfig test_app;

void test_mint_claims_shape(void);
void test_mint_ttl_in_range(void);
void test_mint_token_accepted_by_mcp_auth(void);
void test_mint_token_rejected_by_chat_helpers(void);
void test_mint_rejects_missing_sub(void);
void test_mint_rejects_missing_database(void);
void test_mint_rejects_null_cfg(void);
void test_mint_default_ttl_when_zero(void);

static int base64url_decode(const char *in, unsigned char *out, size_t out_cap, size_t *out_len) {
    size_t in_len = strlen(in);
    char *padded = NULL;
    size_t pad = (4 - (in_len % 4)) % 4;
    BIO *b64 = NULL;
    BIO *bmem = NULL;
    int n;

    if (in_len + pad + 1 > 4096) {
        return -1;
    }
    padded = calloc(1, in_len + pad + 1);
    if (!padded) return -1;
    memcpy(padded, in, in_len);
    for (size_t i = 0; i < pad; i++) padded[in_len + i] = '=';
    padded[in_len + pad] = '\0';

    bmem = BIO_new_mem_buf(padded, -1);
    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO *chain = BIO_push(b64, bmem);
    n = BIO_read(chain, out, (int)out_cap);
    BIO_free_all(chain);
    free(padded);
    if (n <= 0) {
        return -1;
    }
    *out_len = (size_t)n;
    return 0;
}

static int parse_jwt_payload(const char *token, json_t **out_payload) {
    const char *p1 = strchr(token, '.');
    if (!p1) return -1;
    const char *p2 = strchr(p1 + 1, '.');
    if (!p2) return -1;
    size_t b64_len = (size_t)(p2 - (p1 + 1));
    char *b64 = calloc(1, b64_len + 1);
    if (!b64) return -1;
    memcpy(b64, p1 + 1, b64_len);
    b64[b64_len] = '\0';

    unsigned char raw[4096];
    size_t raw_len = 0;
    if (base64url_decode(b64, raw, sizeof(raw), &raw_len) != 0) {
        free(b64);
        return -1;
    }
    free(b64);
    raw[raw_len] = '\0';

    json_error_t err;
    *out_payload = json_loadb((const char *)raw, raw_len, 0, &err);
    return *out_payload ? 0 : -1;
}

void setUp(void) {
    mock_mhd_reset_all();
    mock_auth_service_jwt_reset_all();
    memset(&test_app, 0, sizeof(test_app));
    mcp_config_apply_defaults(&test_app.mcp);
    test_app.mcp.Enabled = true;
    test_app.mcp.AcceptHydrogenJWT = true;
    test_app.mcp.Resource = strdup("http://127.0.0.1:3100/mcp");
    app_config = &test_app;
}

void tearDown(void) {
    mock_auth_service_jwt_reset_all();
    free(test_app.mcp.Resource);
    test_app.mcp.Resource = NULL;
}

void test_mint_claims_shape(void) {
    char *tok = mcp_mint_resource_token(&test_app.mcp, "user-42", "acuranzo", "chat", 900, "cid-shape");
    TEST_ASSERT_NOT_NULL(tok);
    json_t *payload = NULL;
    TEST_ASSERT_EQUAL_INT(0, parse_jwt_payload(tok, &payload));
    TEST_ASSERT_NOT_NULL(payload);

    TEST_ASSERT_EQUAL_STRING("hydrogen-auth", json_string_value(json_object_get(payload, "iss")));
    TEST_ASSERT_EQUAL_STRING("user-42", json_string_value(json_object_get(payload, "sub")));
    TEST_ASSERT_EQUAL_STRING("http://127.0.0.1:3100/mcp", json_string_value(json_object_get(payload, "aud")));
    TEST_ASSERT_EQUAL_STRING("chat", json_string_value(json_object_get(payload, "roles")));
    TEST_ASSERT_EQUAL_STRING("acuranzo", json_string_value(json_object_get(payload, "database")));
    TEST_ASSERT_NOT_NULL(json_string_value(json_object_get(payload, "jti")));
    json_decref(payload);
    free(tok);
}

void test_mint_ttl_in_range(void) {
    time_t before = time(NULL);
    char *tok = mcp_mint_resource_token(&test_app.mcp, "u", "db", "", 600, "cid-ttl");
    time_t after = time(NULL);

    TEST_ASSERT_NOT_NULL(tok);
    json_t *payload = NULL;
    TEST_ASSERT_EQUAL_INT(0, parse_jwt_payload(tok, &payload));
    long iat = (long)json_integer_value(json_object_get(payload, "iat"));
    long exp = (long)json_integer_value(json_object_get(payload, "exp"));
    json_decref(payload);
    free(tok);

    TEST_ASSERT_TRUE(iat >= before);
    TEST_ASSERT_TRUE(iat <= after);
    TEST_ASSERT_EQUAL_INT(600, (int)(exp - iat));
}

void test_mint_default_ttl_when_zero(void) {
    char *tok = mcp_mint_resource_token(&test_app.mcp, "u", "db", NULL, 0, "cid-def");
    TEST_ASSERT_NOT_NULL(tok);
    json_t *payload = NULL;
    TEST_ASSERT_EQUAL_INT(0, parse_jwt_payload(tok, &payload));
    long iat = (long)json_integer_value(json_object_get(payload, "iat"));
    long exp = (long)json_integer_value(json_object_get(payload, "exp"));
    json_decref(payload);
    free(tok);

    TEST_ASSERT_EQUAL_INT(MCP_MINT_DEFAULT_TTL_SECONDS, (int)(exp - iat));
}

void test_mint_token_accepted_by_mcp_auth(void) {
    /* End-to-end: mint a token, then drive mcp_try_hydrogen via the
     * mocked validate_jwt. The mock returns claims matching what the
     * mint would have written; since aud == mcp_auth_resource(cfg),
     * the new aud-gate accepts. */
    char *tok = mcp_mint_resource_token(&test_app.mcp, "user-1", "acuranzo", "chat", 900, "cid-acc");
    TEST_ASSERT_NOT_NULL(tok);

    jwt_validation_result_t programmed;
    jwt_claims_t claims;
    memset(&programmed, 0, sizeof(programmed));
    memset(&claims, 0, sizeof(claims));
    claims.sub = (char *)"user-1";
    claims.database = (char *)"acuranzo";
    claims.aud = (char *)"http://127.0.0.1:3100/mcp";
    claims.roles = (char *)"chat";
    programmed.valid = true;
    programmed.claims = &claims;
    programmed.error = JWT_ERROR_NONE;
    mock_auth_service_jwt_set_validation_result(programmed);

    char header[1024];
    snprintf(header, sizeof(header), "Bearer %s", tok);
    McpAuthResult out;
    memset(&out, 0, sizeof(out));
    TEST_ASSERT_TRUE(mcp_try_hydrogen(header, &test_app.mcp, &out));
    TEST_ASSERT_EQUAL(MCP_AUTH_KIND_HYDROGEN_JWT, out.kind);
    TEST_ASSERT_EQUAL_STRING("user-1", out.sub);
    TEST_ASSERT_EQUAL_STRING("acuranzo", out.database);
    mcp_auth_result_cleanup(&out);
    free(tok);
}

void test_mint_token_rejected_by_chat_helpers(void) {
    /* The minted token has aud = "http://127.0.0.1:3100/mcp" (the
     * MCP Resource URL), but the chat helpers require aud =
     * "hydrogen-chat". This proves the two policies are mechanically
     * distinct. */
    char *tok = mcp_mint_resource_token(&test_app.mcp, "user-1", "acuranzo", "chat", 900, "cid-rej");
    TEST_ASSERT_NOT_NULL(tok);

    json_t *payload = NULL;
    TEST_ASSERT_EQUAL_INT(0, parse_jwt_payload(tok, &payload));
    char *sub_dup = strdup(json_string_value(json_object_get(payload, "sub")));
    char *aud_dup = strdup(json_string_value(json_object_get(payload, "aud")));
    char *db_dup = strdup(json_string_value(json_object_get(payload, "database")));
    char *roles_dup = strdup(json_string_value(json_object_get(payload, "roles")));
    json_decref(payload);

    jwt_validation_result_t programmed;
    jwt_claims_t claims;
    memset(&programmed, 0, sizeof(programmed));
    memset(&claims, 0, sizeof(claims));
    claims.sub = sub_dup;
    claims.database = db_dup;
    claims.aud = aud_dup;
    claims.roles = roles_dup;
    programmed.valid = true;
    programmed.claims = &claims;
    programmed.error = JWT_ERROR_NONE;
    mock_auth_service_jwt_set_validation_result(programmed);

    jwt_validation_result_t validated;
    memset(&validated, 0, sizeof(validated));
    TEST_ASSERT_TRUE(mock_extract_and_validate_jwt("Bearer dummy", &validated));
    TEST_ASSERT_TRUE(validated.valid);
    TEST_ASSERT_FALSE(check_chat_jwt_claims(&validated));
    mock_free_jwt_validation_result(&validated);
    free(tok);
}

void test_mint_rejects_missing_sub(void) {
    char *tok = mcp_mint_resource_token(&test_app.mcp, NULL, "acuranzo", "chat", 900, "cid-nosub");
    TEST_ASSERT_NULL(tok);
}

void test_mint_rejects_missing_database(void) {
    char *tok = mcp_mint_resource_token(&test_app.mcp, "u", "", "chat", 900, "cid-nodb");
    TEST_ASSERT_NULL(tok);
}

void test_mint_rejects_null_cfg(void) {
    char *tok = mcp_mint_resource_token(NULL, "u", "db", "r", 900, "cid-nocfg");
    TEST_ASSERT_NULL(tok);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_mint_claims_shape);
    RUN_TEST(test_mint_ttl_in_range);
    RUN_TEST(test_mint_default_ttl_when_zero);
    RUN_TEST(test_mint_token_accepted_by_mcp_auth);
    RUN_TEST(test_mint_token_rejected_by_chat_helpers);
    RUN_TEST(test_mint_rejects_missing_sub);
    RUN_TEST(test_mint_rejects_missing_database);
    RUN_TEST(test_mint_rejects_null_cfg);
    return UNITY_END();
}
