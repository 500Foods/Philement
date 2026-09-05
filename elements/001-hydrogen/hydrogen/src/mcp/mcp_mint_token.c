/*
 * mcp_mint_token.c
 * Phase 8a: MCP resource token mint primitive.
 *
 * Mints a short-TTL Hydrogen JWT whose `aud` claim is set to
 * `mcp_auth_resource(cfg)` (the MCP Resource URL the provider must call).
 * This is the same value `mcp_try_hydrogen`'s aud-gate checks for, so
 * the minted token round-trips: mint -> present on hosted MCP -> aud
 * matches -> accepted. The chat path (REST/WS) requires `aud =
 * "hydrogen-chat"` and therefore rejects the minted token on purpose.
 *
 * The token is intended to be sent to a third-party model provider as
 * the `authorization: Bearer <jwt>` value on a hosted MCP connector.
 * Treat the token as already leaked to the provider on departure; the
 * short TTL and narrow claims are the only controls.
 */
#include <src/hydrogen.h>

#ifdef UNITY_TEST_MODE
#ifndef USE_MOCK_SYSTEM
#define USE_MOCK_SYSTEM
#endif
#ifndef USE_MOCK_CRYPTO
#define USE_MOCK_CRYPTO
#endif
#include <unity/mocks/mock_system.h>
#include <unity/mocks/mock_crypto.h>
#endif

#include <src/mcp/mcp_mint_token.h>
#include <src/mcp/mcp_auth.h>
#include <src/api/auth/auth_service.h>
#include <src/utils/utils_crypto.h>

#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MCP_MINT_ISS "hydrogen-auth"

char *mcp_mint_resource_token(const MCPConfig *cfg,
                              const char *sub,
                              const char *database,
                              const char *roles,
                              time_t ttl_seconds,
                              const char *correlation_id) {
    jwt_config_t *config;
    char *header_json = NULL;
    char *payload_json = NULL;
    char *header_b64 = NULL;
    char *payload_b64 = NULL;
    char *signing_input = NULL;
    char *signature_b64 = NULL;
    char *jwt = NULL;
    char *jti = NULL;
    time_t now;
    time_t exp;
    int ret;
    unsigned char signature[SHA256_DIGEST_LENGTH];
    unsigned int signature_len = SHA256_DIGEST_LENGTH;
    const char *use_roles = (roles && roles[0]) ? roles : "";
    const char *cid = correlation_id ? correlation_id : "-";
    const char *safe_sub = sub ? sub : "";
    const char *safe_db = database ? database : "";
    const char *aud;

    if (!cfg) {
        log_this("MCP", "mint: missing MCPConfig (cid=%s)", LOG_LEVEL_ERROR, 1, cid);
        return NULL;
    }
    if (!safe_sub[0] || !safe_db[0]) {
        log_this("MCP", "mint: missing sub or database (cid=%s)", LOG_LEVEL_ERROR, 1, cid);
        return NULL;
    }
    aud = mcp_auth_resource(cfg);
    if (!aud || aud[0] == '\0') {
        log_this("MCP", "mint: cfg has no Resource URL (cid=%s)", LOG_LEVEL_ERROR, 1, cid);
        return NULL;
    }
    if (ttl_seconds <= 0) {
        ttl_seconds = MCP_MINT_DEFAULT_TTL_SECONDS;
    }

    config = get_jwt_config();
    if (!config) {
        log_this("MCP", "mint: JWT config unavailable (cid=%s)", LOG_LEVEL_ERROR, 1, cid);
        return NULL;
    }
    if (!config->hmac_secret) {
        log_this("MCP", "mint: JWT config missing hmac_secret (cid=%s)", LOG_LEVEL_ERROR, 1, cid);
        free_jwt_config(config);
        return NULL;
    }

    ret = asprintf(&header_json, "{\"alg\":\"%s\",\"typ\":\"%s\"}",
                   config->use_rsa ? "RS256" : "HS256", "JWT");
    if (ret == -1) {
        log_this("MCP", "mint: header alloc failed (cid=%s)", LOG_LEVEL_ERROR, 1, cid);
        free_jwt_config(config);
        return NULL;
    }

    now = time(NULL);
    exp = now + ttl_seconds;
    {
        unsigned char random_bytes[16];
        if (RAND_bytes(random_bytes, sizeof(random_bytes)) != 1) {
            log_this("MCP", "mint: jti generation failed (cid=%s)", LOG_LEVEL_ERROR, 1, cid);
            free(header_json);
            free_jwt_config(config);
            return NULL;
        }
        jti = utils_base64url_encode(random_bytes, sizeof(random_bytes));
        if (!jti) {
            log_this("MCP", "mint: jti base64url failed (cid=%s)", LOG_LEVEL_ERROR, 1, cid);
            free(header_json);
            free_jwt_config(config);
            return NULL;
        }
    }

    ret = asprintf(&payload_json,
                   "{\"iss\":\"%s\",\"sub\":\"%s\",\"aud\":\"%s\","
                   "\"exp\":%ld,\"iat\":%ld,\"nbf\":%ld,\"jti\":\"%s\","
                   "\"roles\":\"%s\",\"database\":\"%s\"}",
                   MCP_MINT_ISS, safe_sub, aud,
                   (long)exp, (long)now, (long)now, jti,
                   use_roles, safe_db);
    if (ret == -1) {
        log_this("MCP", "mint: payload alloc failed (cid=%s)", LOG_LEVEL_ERROR, 1, cid);
        free(header_json);
        free(jti);
        free_jwt_config(config);
        return NULL;
    }

    header_b64 = utils_base64url_encode((const unsigned char *)header_json, strlen(header_json));
    payload_b64 = utils_base64url_encode((const unsigned char *)payload_json, strlen(payload_json));
    free(header_json);
    free(payload_json);

    if (!header_b64 || !payload_b64) {
        log_this("MCP", "mint: base64url encode failed (cid=%s)", LOG_LEVEL_ERROR, 1, cid);
        free(header_b64);
        free(payload_b64);
        free(jti);
        free_jwt_config(config);
        return NULL;
    }

    ret = asprintf(&signing_input, "%s.%s", header_b64, payload_b64);
    if (ret == -1) {
        log_this("MCP", "mint: signing input alloc failed (cid=%s)", LOG_LEVEL_ERROR, 1, cid);
        free(header_b64);
        free(payload_b64);
        free(jti);
        free_jwt_config(config);
        return NULL;
    }

    if (HMAC(EVP_sha256(), config->hmac_secret, (int)strlen(config->hmac_secret),
             (const unsigned char *)signing_input, strlen(signing_input),
             signature, &signature_len) == NULL) {
        log_this("MCP", "mint: HMAC sign failed (cid=%s)", LOG_LEVEL_ERROR, 1, cid);
        free(header_b64);
        free(payload_b64);
        free(signing_input);
        free(jti);
        free_jwt_config(config);
        return NULL;
    }

    signature_b64 = utils_base64url_encode(signature, signature_len);
    if (!signature_b64) {
        log_this("MCP", "mint: signature base64url failed (cid=%s)", LOG_LEVEL_ERROR, 1, cid);
        free(header_b64);
        free(payload_b64);
        free(signing_input);
        free(jti);
        free_jwt_config(config);
        return NULL;
    }

    ret = asprintf(&jwt, "%s.%s.%s", header_b64, payload_b64, signature_b64);
    free(header_b64);
    free(payload_b64);
    free(signing_input);
    free(signature_b64);
    free(jti);
    free_jwt_config(config);

    if (ret == -1) {
        log_this("MCP", "mint: final token alloc failed (cid=%s)", LOG_LEVEL_ERROR, 1, cid);
        return NULL;
    }

    log_this("MCP",
             "mint: issued resource token sub=%s db=%s aud=%s ttl=%lds (cid=%s)",
             LOG_LEVEL_STATE, 5, safe_sub, safe_db, aud, (long)ttl_seconds, cid);
    return jwt;
}
