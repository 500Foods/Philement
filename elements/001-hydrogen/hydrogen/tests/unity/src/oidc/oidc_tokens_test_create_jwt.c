/*
 * Unity Test File: oidc_create_jwt / id_token / access_token RS256 round-trip
 */

#include <src/hydrogen.h>
#include <unity.h>

#include <src/oidc/oidc_tokens.h>
#include <src/oidc/oidc_keys.h>
#include <src/utils/utils_crypto.h>

#include <jansson.h>
#include <string.h>
#include <unistd.h>

void test_create_claims_null(void);
void test_create_jwt_round_trip(void);
void test_id_and_access_token_distinct(void);

void setUp(void) {
}

void tearDown(void) {
}

void test_create_claims_null(void) {
    TEST_ASSERT_NULL(oidc_create_token_claims(NULL, "sub", "aud"));
    TEST_ASSERT_NULL(oidc_create_token_claims("iss", NULL, "aud"));
}

void test_create_jwt_round_trip(void) {
    char tmpl[] = "/tmp/oidc_jwt_rt_XXXXXX";
    char *dir = mkdtemp(tmpl);
    TEST_ASSERT_NOT_NULL(dir);

    OIDCKeyContext *keys = init_oidc_key_management(dir, false, 90);
    TEST_ASSERT_NOT_NULL(keys);

    OIDCTokenContext *tok = init_oidc_token_service(keys, 3600, 86400, 3600);
    TEST_ASSERT_NOT_NULL(tok);

    OIDCTokenClaims *claims = oidc_create_token_claims("http://issuer.example",
                                                       "42", "test-client");
    TEST_ASSERT_NOT_NULL(claims);
    claims->nonce = strdup("nonce-abc");
    claims->scope = strdup("openid profile");
    claims->auth_time = strdup("1700000000");
    claims->user_data = strdup("{\"email\":\"a@b.c\",\"email_verified\":true}");

    OIDCKey *key = oidc_get_active_signing_key(keys);
    TEST_ASSERT_NOT_NULL(key);

    char *jwt = oidc_create_jwt(tok, claims, key);
    TEST_ASSERT_NOT_NULL(jwt);

    /* Split compact JWT */
    char *dot1 = strchr(jwt, '.');
    TEST_ASSERT_NOT_NULL(dot1);
    char *dot2 = strchr(dot1 + 1, '.');
    TEST_ASSERT_NOT_NULL(dot2);

    size_t hlen = (size_t)(dot1 - jwt);
    size_t plen = (size_t)(dot2 - dot1 - 1);
    char *header_b64 = (char*)malloc(hlen + 1U);
    char *payload_b64 = (char*)malloc(plen + 1U);
    TEST_ASSERT_NOT_NULL(header_b64);
    TEST_ASSERT_NOT_NULL(payload_b64);
    memcpy(header_b64, jwt, hlen);
    header_b64[hlen] = '\0';
    memcpy(payload_b64, dot1 + 1, plen);
    payload_b64[plen] = '\0';
    const char *sig_b64 = dot2 + 1;

    size_t hraw_len = 0;
    unsigned char *hraw = utils_base64url_decode(header_b64, &hraw_len);
    TEST_ASSERT_NOT_NULL(hraw);
    json_error_t jerr;
    json_t *hdr = json_loadb((const char*)hraw, hraw_len, 0, &jerr);
    free(hraw);
    TEST_ASSERT_NOT_NULL(hdr);
    TEST_ASSERT_EQUAL_STRING("RS256", json_string_value(json_object_get(hdr, "alg")));
    TEST_ASSERT_NOT_NULL(json_string_value(json_object_get(hdr, "kid")));
    json_decref(hdr);

    size_t praw_len = 0;
    unsigned char *praw = utils_base64url_decode(payload_b64, &praw_len);
    TEST_ASSERT_NOT_NULL(praw);
    json_t *pl = json_loadb((const char*)praw, praw_len, 0, &jerr);
    free(praw);
    TEST_ASSERT_NOT_NULL(pl);
    TEST_ASSERT_EQUAL_STRING("http://issuer.example", json_string_value(json_object_get(pl, "iss")));
    TEST_ASSERT_EQUAL_STRING("42", json_string_value(json_object_get(pl, "sub")));
    TEST_ASSERT_EQUAL_STRING("test-client", json_string_value(json_object_get(pl, "aud")));
    TEST_ASSERT_EQUAL_STRING("a@b.c", json_string_value(json_object_get(pl, "email")));
    json_decref(pl);

    char *signing_input = NULL;
    TEST_ASSERT_TRUE(asprintf(&signing_input, "%s.%s", header_b64, payload_b64) >= 0);

    size_t sig_len = 0;
    unsigned char *sig = utils_base64url_decode(sig_b64, &sig_len);
    TEST_ASSERT_NOT_NULL(sig);

    TEST_ASSERT_TRUE(utils_rs256_verify((EVP_PKEY*)key->key_data,
                                        (const unsigned char*)signing_input,
                                        strlen(signing_input),
                                        sig, sig_len));

    free(sig);
    free(signing_input);
    free(header_b64);
    free(payload_b64);
    free(jwt);
    oidc_free_token_claims(claims);
    cleanup_oidc_token_service(tok);
    cleanup_oidc_key_management(keys);

    char path[512];
    snprintf(path, sizeof(path), "%s/signing-active.pem", dir);
    unlink(path);
    snprintf(path, sizeof(path), "%s/signing-active.kid", dir);
    unlink(path);
    rmdir(dir);
}

void test_id_and_access_token_distinct(void) {
    char tmpl[] = "/tmp/oidc_jwt_id_XXXXXX";
    char *dir = mkdtemp(tmpl);
    TEST_ASSERT_NOT_NULL(dir);

    OIDCKeyContext *keys = init_oidc_key_management(dir, false, 90);
    OIDCTokenContext *tok = init_oidc_token_service(keys, 1800, 86400, 900);
    OIDCTokenClaims *claims = oidc_create_token_claims("http://iss", "7", "cli");
    claims->nonce = strdup("n1");

    char *id_tok = oidc_generate_id_token(tok, claims);
    char *access = oidc_generate_access_token(tok, claims, NULL);
    TEST_ASSERT_NOT_NULL(id_tok);
    TEST_ASSERT_NOT_NULL(access);
    TEST_ASSERT_TRUE(strcmp(id_tok, access) != 0);

    /* id_token payload should include nonce */
    char *d1 = strchr(id_tok, '.');
    char *d2 = strchr(d1 + 1, '.');
    size_t plen = (size_t)(d2 - d1 - 1);
    char *pb64 = (char*)malloc(plen + 1U);
    TEST_ASSERT_NOT_NULL(pb64);
    memcpy(pb64, d1 + 1, plen);
    pb64[plen] = '\0';
    size_t raw_len = 0;
    unsigned char *raw = utils_base64url_decode(pb64, &raw_len);
    json_error_t err;
    json_t *pl = json_loadb((const char*)raw, raw_len, 0, &err);
    TEST_ASSERT_EQUAL_STRING("n1", json_string_value(json_object_get(pl, "nonce")));
    TEST_ASSERT_NULL(json_object_get(pl, "token_use"));
    json_decref(pl);
    free(raw);
    free(pb64);

    /* access token has token_use=access, no nonce required */
    d1 = strchr(access, '.');
    d2 = strchr(d1 + 1, '.');
    plen = (size_t)(d2 - d1 - 1);
    pb64 = (char*)malloc(plen + 1U);
    TEST_ASSERT_NOT_NULL(pb64);
    memcpy(pb64, d1 + 1, plen);
    pb64[plen] = '\0';
    raw = utils_base64url_decode(pb64, &raw_len);
    pl = json_loadb((const char*)raw, raw_len, 0, &err);
    TEST_ASSERT_EQUAL_STRING("access", json_string_value(json_object_get(pl, "token_use")));
    json_decref(pl);
    free(raw);
    free(pb64);

    free(id_tok);
    free(access);
    oidc_free_token_claims(claims);
    cleanup_oidc_token_service(tok);
    cleanup_oidc_key_management(keys);

    char path[512];
    snprintf(path, sizeof(path), "%s/signing-active.pem", dir);
    unlink(path);
    snprintf(path, sizeof(path), "%s/signing-active.kid", dir);
    unlink(path);
    rmdir(dir);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_create_claims_null);
    RUN_TEST(test_create_jwt_round_trip);
    RUN_TEST(test_id_and_access_token_distinct);
    return UNITY_END();
}
