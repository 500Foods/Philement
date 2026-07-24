/*
 * Unity Test File: oidc_validate_access_token + userinfo path
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/oidc/oidc_service.h>
#include <src/oidc/oidc_tokens.h>
#include <src/oidc/oidc_keys.h>
#include <src/oidc/oidc_pkce.h>
#include <src/config/config_oidc.h>
#include <src/utils/utils_crypto.h>

#include <jansson.h>
#include <string.h>
#include <unistd.h>

void test_validate_null(void);
void test_validate_happy_and_userinfo(void);
void test_validate_rejects_id_token(void);
void test_validate_tampered(void);
void test_validate_malformed_and_alg(void);
void test_validate_expired_and_nbf_future(void);
void test_validate_without_claims_out(void);
void test_validate_unknown_kid_falls_back(void);
void test_validate_refresh_and_revoke(void);

void setUp(void) {
}

void tearDown(void) {
    shutdown_oidc_service();
}

static void unlink_key_dir(const char *dir) {
    char path[512];
    snprintf(path, sizeof(path), "%s/signing-active.pem", dir);
    unlink(path);
    snprintf(path, sizeof(path), "%s/signing-active.kid", dir);
    unlink(path);
    rmdir(dir);
}

static bool setup_cfg(OIDCConfig *cfg, char *dir) {
    memset(cfg, 0, sizeof(*cfg));
    cfg->enabled = true;
    cfg->issuer = (char*)"http://issuer.test";
    cfg->keys.storage_path = dir;
    cfg->keys.encryption_enabled = false;
    cfg->keys.rotation_interval_days = 90;
    cfg->tokens.access_token_lifetime = 3600;
    cfg->tokens.refresh_token_lifetime = 86400;
    cfg->tokens.id_token_lifetime = 3600;
    return init_oidc_service(cfg);
}

/* Rebuild compact JWT with alternate header/payload JSON, re-signed. */
static char *resign_jwt(OIDCKey *key, const char *header_json, const char *payload_json) {
    return oidc_token_sign_compact(key, header_json, payload_json);
}

void test_validate_null(void) {
    TEST_ASSERT_FALSE(oidc_validate_access_token(NULL, "x", NULL));
    OIDCTokenContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    TEST_ASSERT_FALSE(oidc_validate_access_token(&ctx, NULL, NULL));
    TEST_ASSERT_FALSE(oidc_validate_access_token(&ctx, "tok", NULL));
}

void test_validate_happy_and_userinfo(void) {
    char tmpl[] = "/tmp/oidc_val_ok_XXXXXX";
    char *dir = mkdtemp(tmpl);
    TEST_ASSERT_NOT_NULL(dir);

    OIDCConfig cfg;
    TEST_ASSERT_TRUE(setup_cfg(&cfg, dir));

    OIDCContext *ctx = get_oidc_context();
    OIDCClient *client = oidc_client_create(
        "ui-client", NULL, "T", false, true,
        "[\"https://app.example/cb\"]", "authorization_code", "code");
    TEST_ASSERT_TRUE(oidc_client_registry_add((OIDCClientContext*)ctx->client_context, client));

    const char *verifier = "userinfo-verifier-abcdefghijklmnopqrstuv";
    char *challenge = oidc_pkce_make_challenge_s256(verifier);
    char *code = oidc_issue_authorization_code("ui-client", "https://app.example/cb",
                                               "openid email", "n", challenge, "S256", 55, NULL);
    TEST_ASSERT_NOT_NULL(code);

    char *tok_resp = oidc_process_token_request("authorization_code", code,
                                                "https://app.example/cb", "ui-client",
                                                NULL, NULL, verifier);
    TEST_ASSERT_NOT_NULL(tok_resp);
    json_error_t err;
    json_t *root = json_loads(tok_resp, 0, &err);
    const char *access = json_string_value(json_object_get(root, "access_token"));
    TEST_ASSERT_NOT_NULL(access);

    OIDCTokenClaims *claims = NULL;
    TEST_ASSERT_TRUE(oidc_validate_access_token((OIDCTokenContext*)ctx->token_context,
                                                access, &claims));
    TEST_ASSERT_NOT_NULL(claims);
    TEST_ASSERT_EQUAL_STRING("55", claims->sub);
    TEST_ASSERT_EQUAL_STRING("http://issuer.test", claims->iss);
    oidc_free_token_claims(claims);

    char *ui = oidc_process_userinfo_request(access);
    TEST_ASSERT_NOT_NULL(ui);
    json_t *uj = json_loads(ui, 0, &err);
    TEST_ASSERT_EQUAL_STRING("55", json_string_value(json_object_get(uj, "sub")));
    json_decref(uj);
    free(ui);

    json_decref(root);
    free(tok_resp);
    free(code);
    free(challenge);
    shutdown_oidc_service();
    unlink_key_dir(dir);
}

void test_validate_rejects_id_token(void) {
    char tmpl[] = "/tmp/oidc_val_id_XXXXXX";
    char *dir = mkdtemp(tmpl);
    TEST_ASSERT_NOT_NULL(dir);

    OIDCConfig cfg;
    TEST_ASSERT_TRUE(setup_cfg(&cfg, dir));
    cfg.issuer = (char*)"http://iss";

    OIDCContext *ctx = get_oidc_context();
    OIDCTokenClaims *c = oidc_create_token_claims("http://iss", "1", "cli");
    char *id_tok = oidc_generate_id_token((OIDCTokenContext*)ctx->token_context, c);
    TEST_ASSERT_NOT_NULL(id_tok);
    TEST_ASSERT_FALSE(oidc_validate_access_token((OIDCTokenContext*)ctx->token_context,
                                                 id_tok, NULL));
    free(id_tok);
    oidc_free_token_claims(c);
    shutdown_oidc_service();
    unlink_key_dir(dir);
}

void test_validate_tampered(void) {
    char tmpl[] = "/tmp/oidc_val_tam_XXXXXX";
    char *dir = mkdtemp(tmpl);
    TEST_ASSERT_NOT_NULL(dir);

    OIDCConfig cfg;
    TEST_ASSERT_TRUE(setup_cfg(&cfg, dir));

    OIDCContext *ctx = get_oidc_context();
    OIDCTokenClaims *c = oidc_create_token_claims("http://iss", "9", "cli");
    char *access = oidc_generate_access_token((OIDCTokenContext*)ctx->token_context, c, NULL);
    TEST_ASSERT_NOT_NULL(access);

    size_t len = strlen(access);
    char *bad = strdup(access);
    TEST_ASSERT_NOT_NULL(bad);
    TEST_ASSERT_TRUE(len > 10U);
    size_t flip = len - 5U;
    if (bad[flip] == 'A') {
        bad[flip] = 'B';
    } else {
        bad[flip] = 'A';
    }
    TEST_ASSERT_FALSE(oidc_validate_access_token((OIDCTokenContext*)ctx->token_context,
                                                 bad, NULL));
    free(bad);
    free(access);
    oidc_free_token_claims(c);
    shutdown_oidc_service();
    unlink_key_dir(dir);
}

void test_validate_malformed_and_alg(void) {
    char tmpl[] = "/tmp/oidc_val_mal_XXXXXX";
    char *dir = mkdtemp(tmpl);
    TEST_ASSERT_NOT_NULL(dir);

    OIDCConfig cfg;
    TEST_ASSERT_TRUE(setup_cfg(&cfg, dir));
    OIDCContext *ctx = get_oidc_context();
    OIDCTokenContext *tc = (OIDCTokenContext*)ctx->token_context;
    OIDCKey *key = oidc_get_active_signing_key((OIDCKeyContext*)tc->key_context);
    TEST_ASSERT_NOT_NULL(key);

    TEST_ASSERT_FALSE(oidc_validate_access_token(tc, "not.a.jwt!!!", NULL));
    TEST_ASSERT_FALSE(oidc_validate_access_token(tc, "a.b.c", NULL));

    /* Bad alg in header */
    char *bad_alg = resign_jwt(key,
        "{\"alg\":\"none\",\"typ\":\"JWT\",\"kid\":\"x\"}",
        "{\"iss\":\"i\",\"sub\":\"s\",\"exp\":9999999999,\"token_use\":\"access\"}");
    TEST_ASSERT_NOT_NULL(bad_alg);
    TEST_ASSERT_FALSE(oidc_validate_access_token(tc, bad_alg, NULL));
    free(bad_alg);

    /* Missing exp */
    char *no_exp = resign_jwt(key,
        "{\"alg\":\"RS256\",\"typ\":\"JWT\"}",
        "{\"iss\":\"i\",\"sub\":\"s\",\"token_use\":\"access\"}");
    TEST_ASSERT_NOT_NULL(no_exp);
    TEST_ASSERT_FALSE(oidc_validate_access_token(tc, no_exp, NULL));
    free(no_exp);

    /* Wrong token_use */
    char *wrong_use = resign_jwt(key,
        "{\"alg\":\"RS256\",\"typ\":\"JWT\"}",
        "{\"iss\":\"i\",\"sub\":\"s\",\"exp\":9999999999,\"token_use\":\"id\"}");
    TEST_ASSERT_NOT_NULL(wrong_use);
    TEST_ASSERT_FALSE(oidc_validate_access_token(tc, wrong_use, NULL));
    free(wrong_use);

    /* Invalid base64 signature segment still three parts */
    char *hb = utils_base64url_encode((const unsigned char*)"{\"alg\":\"RS256\"}", 15);
    char *pb = utils_base64url_encode((const unsigned char*)"{\"token_use\":\"access\",\"exp\":9}", 30);
    char *bogus = NULL;
    TEST_ASSERT_TRUE(asprintf(&bogus, "%s.%s.!!!", hb, pb) >= 0);
    TEST_ASSERT_FALSE(oidc_validate_access_token(tc, bogus, NULL));
    free(bogus);
    free(hb);
    free(pb);

    shutdown_oidc_service();
    unlink_key_dir(dir);
}

void test_validate_expired_and_nbf_future(void) {
    char tmpl[] = "/tmp/oidc_val_exp_XXXXXX";
    char *dir = mkdtemp(tmpl);
    TEST_ASSERT_NOT_NULL(dir);

    OIDCConfig cfg;
    TEST_ASSERT_TRUE(setup_cfg(&cfg, dir));
    OIDCContext *ctx = get_oidc_context();
    OIDCTokenContext *tc = (OIDCTokenContext*)ctx->token_context;
    OIDCKey *key = oidc_get_active_signing_key((OIDCKeyContext*)tc->key_context);

    /* Expired far past clock skew */
    char *expired = resign_jwt(key,
        "{\"alg\":\"RS256\",\"typ\":\"JWT\"}",
        "{\"iss\":\"i\",\"sub\":\"s\",\"exp\":100,\"token_use\":\"access\"}");
    TEST_ASSERT_NOT_NULL(expired);
    TEST_ASSERT_FALSE(oidc_validate_access_token(tc, expired, NULL));
    free(expired);

    /* nbf far in the future */
    char *future_nbf = resign_jwt(key,
        "{\"alg\":\"RS256\",\"typ\":\"JWT\"}",
        "{\"iss\":\"i\",\"sub\":\"s\",\"exp\":9999999999,\"nbf\":9999999900,\"token_use\":\"access\"}");
    TEST_ASSERT_NOT_NULL(future_nbf);
    TEST_ASSERT_FALSE(oidc_validate_access_token(tc, future_nbf, NULL));
    free(future_nbf);

    shutdown_oidc_service();
    unlink_key_dir(dir);
}

void test_validate_without_claims_out(void) {
    char tmpl[] = "/tmp/oidc_val_nc_XXXXXX";
    char *dir = mkdtemp(tmpl);
    TEST_ASSERT_NOT_NULL(dir);

    OIDCConfig cfg;
    TEST_ASSERT_TRUE(setup_cfg(&cfg, dir));
    OIDCContext *ctx = get_oidc_context();
    OIDCTokenClaims *c = oidc_create_token_claims("http://issuer.test", "u", "cli");
    char *access = oidc_generate_access_token((OIDCTokenContext*)ctx->token_context, c, NULL);
    TEST_ASSERT_NOT_NULL(access);
    TEST_ASSERT_TRUE(oidc_validate_access_token((OIDCTokenContext*)ctx->token_context,
                                                access, NULL));
    free(access);
    oidc_free_token_claims(c);
    shutdown_oidc_service();
    unlink_key_dir(dir);
}

void test_validate_unknown_kid_falls_back(void) {
    char tmpl[] = "/tmp/oidc_val_kid_XXXXXX";
    char *dir = mkdtemp(tmpl);
    TEST_ASSERT_NOT_NULL(dir);

    OIDCConfig cfg;
    TEST_ASSERT_TRUE(setup_cfg(&cfg, dir));
    OIDCContext *ctx = get_oidc_context();
    OIDCTokenContext *tc = (OIDCTokenContext*)ctx->token_context;
    OIDCKey *key = oidc_get_active_signing_key((OIDCKeyContext*)tc->key_context);

    /* Unknown kid should fall back to active signing key */
    char *jwt = resign_jwt(key,
        "{\"alg\":\"RS256\",\"typ\":\"JWT\",\"kid\":\"unknown-kid-xyz\"}",
        "{\"iss\":\"i\",\"sub\":\"s\",\"exp\":9999999999,\"iat\":1,\"token_use\":\"access\"}");
    TEST_ASSERT_NOT_NULL(jwt);
    OIDCTokenClaims *out = NULL;
    TEST_ASSERT_TRUE(oidc_validate_access_token(tc, jwt, &out));
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_EQUAL_STRING("s", out->sub);
    oidc_free_token_claims(out);
    free(jwt);

    shutdown_oidc_service();
    unlink_key_dir(dir);
}

void test_validate_refresh_and_revoke(void) {
    TEST_ASSERT_FALSE(oidc_validate_refresh_token(NULL, NULL, NULL));
    TEST_ASSERT_FALSE(oidc_revoke_token(NULL, NULL, NULL, NULL));

    OIDCTokenContext ctx;
    memset(&ctx, 0, sizeof(ctx));
    TEST_ASSERT_FALSE(oidc_validate_refresh_token(&ctx, "rt", "cli"));
    TEST_ASSERT_FALSE(oidc_revoke_token(&ctx, "tok", "access_token", "cli"));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_validate_null);
    RUN_TEST(test_validate_happy_and_userinfo);
    RUN_TEST(test_validate_rejects_id_token);
    RUN_TEST(test_validate_tampered);
    RUN_TEST(test_validate_malformed_and_alg);
    RUN_TEST(test_validate_expired_and_nbf_future);
    RUN_TEST(test_validate_without_claims_out);
    RUN_TEST(test_validate_unknown_kid_falls_back);
    RUN_TEST(test_validate_refresh_and_revoke);
    return UNITY_END();
}
