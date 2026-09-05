#include <src/hydrogen.h>
#include <unity.h>

#define USE_MOCK_AUTH_SERVICE_JWT
#define USE_MOCK_LIBMICROHTTPD

#include <unity/mocks/mock_auth_service_jwt.h>
#include <src/mcp/mcp_mint_token.h>
#include <src/mcp/mcp_auth.h>
#include <src/config/config_mcp.h>
#include <string.h>

void test_mint_missing_resource(void);
void test_mint_default_ttl_negative(void);
void test_mint_roles_null(void);
void test_mint_roles_empty(void);
void test_mint_correlation_id_null(void);
void test_mint_token_three_part_jwt(void);
void test_mint_token_aud_is_resource(void);
void test_mint_token_nbf_present(void);
void test_mint_token_rs256_not_used_by_default(void);

void setUp(void) {
    mock_auth_service_jwt_reset_all();
}

void tearDown(void) {
    mock_auth_service_jwt_reset_all();
}

static int base64url_decode(const char *in, unsigned char *out, size_t out_cap, size_t *out_len) {
    size_t in_len = strlen(in);
    char *padded = NULL;
    size_t pad = (4 - (in_len % 4)) % 4;
    BIO *b64 = NULL;
    BIO *bmem = NULL;
    int n;

    if (in_len + pad + 1 > 4096) return -1;
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
    if (n <= 0) return -1;
    *out_len = (size_t)n;
    return 0;
}

static int parse_jwt_parts(const char *token, char **out_payload_b64) {
    const char *p1 = strchr(token, '.');
    if (!p1) return -1;
    const char *p2 = strchr(p1 + 1, '.');
    if (!p2) return -1;
    size_t b64_len = (size_t)(p2 - (p1 + 1));
    char *b64 = calloc(1, b64_len + 1);
    if (!b64) return -1;
    memcpy(b64, p1 + 1, b64_len);
    *out_payload_b64 = b64;
    return 0;
}

static json_t* decode_jwt_payload(const char *token) {
    char *payload_b64 = NULL;
    unsigned char raw[4096];
    size_t raw_len = 0;
    json_error_t err;

    if (parse_jwt_parts(token, &payload_b64) != 0) return NULL;
    if (base64url_decode(payload_b64, raw, sizeof(raw), &raw_len) != 0) {
        free(payload_b64);
        return NULL;
    }
    free(payload_b64);
    raw[raw_len] = '\0';
    return json_loads((const char*)raw, 0, &err);
}

void test_mint_missing_resource(void) {
    MCPConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    mcp_config_apply_defaults(&cfg);
    cfg.Resource = NULL;
    cfg.AcceptHydrogenJWT = true;
    /* Resource is NULL but Interface is "127.0.0.1" and Port is 3100 —
     * so mcp_auth_resource derives a URL. To get empty resource, we'd
     * need Interface to be NULL too. But apply_defaults sets it.
     * Instead, test with explicitly empty resource by using NULL cfg's
     * behavior: mcp_auth_resource(NULL) returns "". */
    char *tok = mcp_mint_resource_token(NULL, "sub", "db", "r", 900, "cid-no-resource");
    TEST_ASSERT_NULL(tok);
}

void test_mint_default_ttl_negative(void) {
    MCPConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    mcp_config_apply_defaults(&cfg);
    cfg.Resource = strdup("http://127.0.0.1:3100/mcp");

    char *tok = mcp_mint_resource_token(&cfg, "sub", "db", "r", -1, "cid-neg-ttl");
    TEST_ASSERT_NOT_NULL(tok);
    json_t *payload = decode_jwt_payload(tok);
    TEST_ASSERT_NOT_NULL(payload);
    long iat = (long)json_integer_value(json_object_get(payload, "iat"));
    long exp = (long)json_integer_value(json_object_get(payload, "exp"));
    long diff = exp - iat;
    TEST_ASSERT_EQUAL_INT(MCP_MINT_DEFAULT_TTL_SECONDS, (int)diff);
    json_decref(payload);
    free(tok);
    free(cfg.Resource);
}

void test_mint_roles_null(void) {
    MCPConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    mcp_config_apply_defaults(&cfg);
    cfg.Resource = strdup("http://127.0.0.1:3100/mcp");

    char *tok = mcp_mint_resource_token(&cfg, "sub", "db", NULL, 900, "cid-noroles");
    TEST_ASSERT_NOT_NULL(tok);
    json_t *payload = decode_jwt_payload(tok);
    TEST_ASSERT_NOT_NULL(payload);
    TEST_ASSERT_EQUAL_STRING("", json_string_value(json_object_get(payload, "roles")));
    json_decref(payload);
    free(tok);
    free(cfg.Resource);
}

void test_mint_roles_empty(void) {
    MCPConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    mcp_config_apply_defaults(&cfg);
    cfg.Resource = strdup("http://127.0.0.1:3100/mcp");

    char *tok = mcp_mint_resource_token(&cfg, "sub", "db", "", 900, "cid-emptyroles");
    TEST_ASSERT_NOT_NULL(tok);
    json_t *payload = decode_jwt_payload(tok);
    TEST_ASSERT_NOT_NULL(payload);
    TEST_ASSERT_EQUAL_STRING("", json_string_value(json_object_get(payload, "roles")));
    json_decref(payload);
    free(tok);
    free(cfg.Resource);
}

void test_mint_correlation_id_null(void) {
    MCPConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    mcp_config_apply_defaults(&cfg);
    cfg.Resource = strdup("http://127.0.0.1:3100/mcp");

    char *tok = mcp_mint_resource_token(&cfg, "sub", "db", "r", 900, NULL);
    TEST_ASSERT_NOT_NULL(tok);
    free(tok);
    free(cfg.Resource);
}

void test_mint_token_three_part_jwt(void) {
    MCPConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    mcp_config_apply_defaults(&cfg);
    cfg.Resource = strdup("http://127.0.0.1:3100/mcp");

    char *tok = mcp_mint_resource_token(&cfg, "user-99", "mydb", "admin", 900, "cid-format");
    TEST_ASSERT_NOT_NULL(tok);
    /* A JWT has exactly 2 dots separating 3 parts */
    int dot_count = 0;
    for (const char *p = tok; *p; p++) {
        if (*p == '.') dot_count++;
    }
    TEST_ASSERT_EQUAL_INT(2, dot_count);
    free(tok);
    free(cfg.Resource);
}

void test_mint_token_aud_is_resource(void) {
    MCPConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    mcp_config_apply_defaults(&cfg);
    cfg.Resource = strdup("https://custom.mcp.example/mcp");

    char *tok = mcp_mint_resource_token(&cfg, "user-1", "db1", "chat", 900, "cid-aud");
    TEST_ASSERT_NOT_NULL(tok);
    json_t *payload = decode_jwt_payload(tok);
    TEST_ASSERT_NOT_NULL(payload);
    TEST_ASSERT_EQUAL_STRING("https://custom.mcp.example/mcp",
                             json_string_value(json_object_get(payload, "aud")));
    json_decref(payload);
    free(tok);
    free(cfg.Resource);
}

void test_mint_token_nbf_present(void) {
    MCPConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    mcp_config_apply_defaults(&cfg);
    cfg.Resource = strdup("http://127.0.0.1:3100/mcp");

    char *tok = mcp_mint_resource_token(&cfg, "user-1", "db1", "chat", 900, "cid-nbf");
    TEST_ASSERT_NOT_NULL(tok);
    json_t *payload = decode_jwt_payload(tok);
    TEST_ASSERT_NOT_NULL(payload);
    /* nbf should be equal to iat */
    long iat = (long)json_integer_value(json_object_get(payload, "iat"));
    long nbf = (long)json_integer_value(json_object_get(payload, "nbf"));
    TEST_ASSERT_EQUAL_INT(iat, nbf);
    json_decref(payload);
    free(tok);
    free(cfg.Resource);
}

void test_mint_token_rs256_not_used_by_default(void) {
    MCPConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    mcp_config_apply_defaults(&cfg);
    cfg.Resource = strdup("http://127.0.0.1:3100/mcp");
    cfg.AcceptHydrogenJWT = true;

    char *tok = mcp_mint_resource_token(&cfg, "user-1", "db1", "chat", 900, "cid-alg");
    TEST_ASSERT_NOT_NULL(tok);
    /* The header should say HS256 since use_rsa defaults to false */
    const char *p1 = strchr(tok, '.');
    TEST_ASSERT_NOT_NULL(p1);
    size_t header_len = (size_t)(p1 - tok);
    char *header_b64 = calloc(1, header_len + 1);
    TEST_ASSERT_NOT_NULL(header_b64);
    memcpy(header_b64, tok, header_len);
    unsigned char raw[256];
    size_t raw_len = 0;
    TEST_ASSERT_EQUAL_INT(0, base64url_decode(header_b64, raw, sizeof(raw), &raw_len));
    raw[raw_len] = '\0';
    json_error_t err;
    json_t *header = json_loads((const char*)raw, 0, &err);
    TEST_ASSERT_NOT_NULL(header);
    TEST_ASSERT_EQUAL_STRING("HS256", json_string_value(json_object_get(header, "alg")));
    json_decref(header);
    free(header_b64);
    free(tok);
    free(cfg.Resource);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_mint_missing_resource);
    RUN_TEST(test_mint_default_ttl_negative);
    RUN_TEST(test_mint_roles_null);
    RUN_TEST(test_mint_roles_empty);
    RUN_TEST(test_mint_correlation_id_null);
    RUN_TEST(test_mint_token_three_part_jwt);
    RUN_TEST(test_mint_token_aud_is_resource);
    RUN_TEST(test_mint_token_nbf_present);
    RUN_TEST(test_mint_token_rs256_not_used_by_default);
    return UNITY_END();
}
