/*
 * OpenID Connect (OIDC) Token Service Implementation
 *
 * Manages the generation, validation, and handling of OIDC tokens:
 * - Access tokens (RS256 JWT)
 * - ID tokens (RS256 JWT)
 * - Refresh tokens (opaque stub until Phase 11)
 */

#include <src/hydrogen.h>

#include "oidc_tokens.h"
#include "oidc_keys.h"
#include <src/utils/utils_crypto.h>

#include <jansson.h>
#include <string.h>

/* Internal helpers (non-static for Unity / static-function gate) */
char* oidc_token_build_header_json(const char *kid);
char* oidc_token_build_payload_json(const OIDCTokenClaims *claims, bool is_id_token);
char* oidc_token_sign_compact(OIDCKey *key, const char *header_json, const char *payload_json);
void oidc_token_apply_lifetime(OIDCTokenClaims *claims, int lifetime_seconds);

char* oidc_token_build_header_json(const char *kid) {
    json_t *hdr = json_object();
    if (!hdr) {
        return NULL;
    }
    json_object_set_new(hdr, "alg", json_string("RS256"));
    json_object_set_new(hdr, "typ", json_string("JWT"));
    if (kid && kid[0] != '\0') {
        json_object_set_new(hdr, "kid", json_string(kid));
    }
    char *out = json_dumps(hdr, JSON_COMPACT);
    json_decref(hdr);
    return out;
}

char* oidc_token_build_payload_json(const OIDCTokenClaims *claims, bool is_id_token) {
    if (!claims || !claims->iss || !claims->sub) {
        return NULL;
    }

    json_t *payload = json_object();
    if (!payload) {
        return NULL;
    }

    json_object_set_new(payload, "iss", json_string(claims->iss));
    json_object_set_new(payload, "sub", json_string(claims->sub));

    if (claims->aud_count > 0U && claims->aud) {
        if (claims->aud_count == 1U && claims->aud[0]) {
            json_object_set_new(payload, "aud", json_string(claims->aud[0]));
        } else {
            json_t *arr = json_array();
            if (arr) {
                size_t i;
                for (i = 0; i < claims->aud_count; i++) {
                    if (claims->aud[i]) {
                        json_array_append_new(arr, json_string(claims->aud[i]));
                    }
                }
                json_object_set_new(payload, "aud", arr);
            }
        }
    } else if (claims->client_id && claims->client_id[0] != '\0') {
        json_object_set_new(payload, "aud", json_string(claims->client_id));
    }

    if (claims->exp > 0) {
        json_object_set_new(payload, "exp", json_integer((json_int_t)claims->exp));
    }
    if (claims->iat > 0) {
        json_object_set_new(payload, "iat", json_integer((json_int_t)claims->iat));
    }
    if (claims->nbf > 0) {
        json_object_set_new(payload, "nbf", json_integer((json_int_t)claims->nbf));
    }
    if (claims->jti && claims->jti[0] != '\0') {
        json_object_set_new(payload, "jti", json_string(claims->jti));
    }
    if (claims->scope && claims->scope[0] != '\0') {
        json_object_set_new(payload, "scope", json_string(claims->scope));
    }
    if (claims->client_id && claims->client_id[0] != '\0') {
        json_object_set_new(payload, "client_id", json_string(claims->client_id));
    }
    if (claims->azp && claims->azp[0] != '\0') {
        json_object_set_new(payload, "azp", json_string(claims->azp));
    }

    if (is_id_token) {
        if (claims->nonce && claims->nonce[0] != '\0') {
            json_object_set_new(payload, "nonce", json_string(claims->nonce));
        }
        if (claims->auth_time && claims->auth_time[0] != '\0') {
            /* Prefer numeric auth_time when the string is a decimal timestamp. */
            char *end = NULL;
            long auth_ts = strtol(claims->auth_time, &end, 10);
            if (end && *end == '\0' && auth_ts > 0) {
                json_object_set_new(payload, "auth_time", json_integer((json_int_t)auth_ts));
            } else {
                json_object_set_new(payload, "auth_time", json_string(claims->auth_time));
            }
        }
    }

    /* Optional extra user claims as a JSON object merge (profile/email). */
    if (claims->user_data && claims->user_data[0] != '\0') {
        json_error_t err;
        json_t *extra = json_loads(claims->user_data, 0, &err);
        if (extra && json_is_object(extra)) {
            const char *key;
            json_t *val;
            json_object_foreach(extra, key, val) {
                if (key && strcmp(key, "iss") != 0 && strcmp(key, "sub") != 0 &&
                    strcmp(key, "aud") != 0 && strcmp(key, "exp") != 0 &&
                    strcmp(key, "iat") != 0) {
                    json_object_set(payload, key, val);
                }
            }
        }
        if (extra) {
            json_decref(extra);
        }
    }

    if (!is_id_token) {
        json_object_set_new(payload, "token_use", json_string("access"));
    }

    char *out = json_dumps(payload, JSON_COMPACT);
    json_decref(payload);
    return out;
}

char* oidc_token_sign_compact(OIDCKey *key, const char *header_json, const char *payload_json) {
    if (!key || !key->key_data || !header_json || !payload_json) {
        return NULL;
    }

    char *header_b64 = utils_base64url_encode((const unsigned char*)header_json, strlen(header_json));
    char *payload_b64 = utils_base64url_encode((const unsigned char*)payload_json, strlen(payload_json));
    if (!header_b64 || !payload_b64) {
        free(header_b64);
        free(payload_b64);
        return NULL;
    }

    char *signing_input = NULL;
    if (asprintf(&signing_input, "%s.%s", header_b64, payload_b64) < 0) {
        free(header_b64);
        free(payload_b64);
        return NULL;
    }

    unsigned char *sig = NULL;
    size_t sig_len = 0;
    if (!oidc_sign_data(key, (const unsigned char*)signing_input, strlen(signing_input),
                        &sig, &sig_len) || !sig) {
        free(header_b64);
        free(payload_b64);
        free(signing_input);
        return NULL;
    }

    char *sig_b64 = utils_base64url_encode(sig, sig_len);
    free(sig);
    if (!sig_b64) {
        free(header_b64);
        free(payload_b64);
        free(signing_input);
        return NULL;
    }

    char *jwt = NULL;
    if (asprintf(&jwt, "%s.%s.%s", header_b64, payload_b64, sig_b64) < 0) {
        jwt = NULL;
    }

    free(header_b64);
    free(payload_b64);
    free(sig_b64);
    free(signing_input);
    return jwt;
}

void oidc_token_apply_lifetime(OIDCTokenClaims *claims, int lifetime_seconds) {
    if (!claims) {
        return;
    }
    time_t now = time(NULL);
    if (claims->iat <= 0) {
        claims->iat = now;
    }
    if (claims->nbf <= 0) {
        claims->nbf = claims->iat;
    }
    if (claims->exp <= 0 && lifetime_seconds > 0) {
        claims->exp = claims->iat + (time_t)lifetime_seconds;
    }
}

OIDCTokenContext* init_oidc_token_service(OIDCKeyContext *key_context,
                                        int access_token_lifetime,
                                        int refresh_token_lifetime,
                                        int id_token_lifetime) {
    log_this(SR_OIDC, "Initializing token service", LOG_LEVEL_STATE, 0);

    if (!key_context) {
        log_this(SR_OIDC, "Cannot initialize token service: Invalid key context", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    OIDCTokenContext *context = (OIDCTokenContext*)calloc(1, sizeof(OIDCTokenContext));
    if (!context) {
        log_this(SR_OIDC, "Failed to allocate memory for token context", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    context->key_context = key_context;
    context->access_token_lifetime = access_token_lifetime;
    context->refresh_token_lifetime = refresh_token_lifetime;
    context->id_token_lifetime = id_token_lifetime;
    context->token_storage = NULL;

    log_this(SR_OIDC, "Token service initialized successfully", LOG_LEVEL_STATE, 0);
    return context;
}

void cleanup_oidc_token_service(OIDCTokenContext *context) {
    if (!context) {
        return;
    }

    log_this(SR_OIDC, "Cleaning up token service", LOG_LEVEL_STATE, 0);
    free(context);
    log_this(SR_OIDC, "Token service cleanup completed", LOG_LEVEL_STATE, 0);
}

OIDCTokenClaims* oidc_create_token_claims(const char *issuer, const char *subject, const char *audience) {
    if (!issuer || !subject) {
        return NULL;
    }

    OIDCTokenClaims *claims = (OIDCTokenClaims*)calloc(1, sizeof(OIDCTokenClaims));
    if (!claims) {
        return NULL;
    }

    claims->iss = strdup(issuer);
    claims->sub = strdup(subject);
    if (!claims->iss || !claims->sub) {
        oidc_free_token_claims(claims);
        return NULL;
    }

    if (audience && audience[0] != '\0') {
        claims->aud = (char**)calloc(1, sizeof(char*));
        if (!claims->aud) {
            oidc_free_token_claims(claims);
            return NULL;
        }
        claims->aud[0] = strdup(audience);
        if (!claims->aud[0]) {
            oidc_free_token_claims(claims);
            return NULL;
        }
        claims->aud_count = 1;
        claims->client_id = strdup(audience);
        claims->azp = strdup(audience);
        if (!claims->client_id || !claims->azp) {
            oidc_free_token_claims(claims);
            return NULL;
        }
    }

    claims->iat = time(NULL);
    claims->nbf = claims->iat;
    return claims;
}

void oidc_free_token_claims(OIDCTokenClaims *claims) {
    if (!claims) {
        return;
    }
    free(claims->iss);
    free(claims->sub);
    if (claims->aud) {
        size_t i;
        for (i = 0; i < claims->aud_count; i++) {
            free(claims->aud[i]);
        }
        free(claims->aud);
    }
    free(claims->jti);
    free(claims->nonce);
    free(claims->auth_time);
    free(claims->acr);
    free(claims->amr);
    free(claims->azp);
    free(claims->scope);
    free(claims->client_id);
    free(claims->user_data);
    free(claims);
}

char* oidc_create_jwt(OIDCTokenContext *context, const OIDCTokenClaims *claims, OIDCKey *key) {
    if (!claims || !key) {
        log_this(SR_OIDC, "Invalid parameters for JWT creation", LOG_LEVEL_ERROR, 0);
        return NULL;
    }
    (void)context;

    char *header_json = oidc_token_build_header_json(key->kid);
    /* Default to access-style payload; id_token path uses oidc_generate_id_token. */
    char *payload_json = oidc_token_build_payload_json(claims, false);
    if (!header_json || !payload_json) {
        free(header_json);
        free(payload_json);
        return NULL;
    }

    char *jwt = oidc_token_sign_compact(key, header_json, payload_json);
    free(header_json);
    free(payload_json);
    return jwt;
}

char* oidc_generate_id_token(const OIDCTokenContext *context,
                             const OIDCTokenClaims *claims) {
    if (!context || !claims || !context->key_context) {
        log_this(SR_OIDC, "Invalid parameters for ID token generation", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    OIDCKey *key = oidc_get_active_signing_key((OIDCKeyContext*)context->key_context);
    if (!key) {
        log_this(SR_OIDC, "No active signing key for ID token", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    OIDCTokenClaims mutable_copy = *claims;
    oidc_token_apply_lifetime(&mutable_copy, context->id_token_lifetime);

    char *header_json = oidc_token_build_header_json(key->kid);
    char *payload_json = oidc_token_build_payload_json(&mutable_copy, true);
    if (!header_json || !payload_json) {
        free(header_json);
        free(payload_json);
        return NULL;
    }

    char *jwt = oidc_token_sign_compact(key, header_json, payload_json);
    free(header_json);
    free(payload_json);
    if (jwt) {
        log_this(SR_OIDC, "Generated ID token", LOG_LEVEL_DEBUG, 0);
    }
    return jwt;
}

char* oidc_generate_access_token(const OIDCTokenContext *context,
                                 const OIDCTokenClaims *claims,
                                 char **reference) {
    if (reference) {
        *reference = NULL;
    }
    if (!context || !claims || !context->key_context) {
        log_this(SR_OIDC, "Invalid parameters for access token generation", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    OIDCKey *key = oidc_get_active_signing_key((OIDCKeyContext*)context->key_context);
    if (!key) {
        log_this(SR_OIDC, "No active signing key for access token", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    OIDCTokenClaims mutable_copy = *claims;
    oidc_token_apply_lifetime(&mutable_copy, context->access_token_lifetime);

    char *header_json = oidc_token_build_header_json(key->kid);
    char *payload_json = oidc_token_build_payload_json(&mutable_copy, false);
    if (!header_json || !payload_json) {
        free(header_json);
        free(payload_json);
        return NULL;
    }

    char *jwt = oidc_token_sign_compact(key, header_json, payload_json);
    free(header_json);
    free(payload_json);
    if (jwt) {
        log_this(SR_OIDC, "Generated access token", LOG_LEVEL_DEBUG, 0);
    }
    return jwt;
}

char* oidc_generate_refresh_token(const OIDCTokenContext *context,
                                  const OIDCTokenClaims *claims) {
    if (!context || !claims) {
        log_this(SR_OIDC, "Invalid parameters for refresh token generation", LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    /* Phase 11: durable opaque refresh. Stub remains for link compatibility. */
    log_this(SR_OIDC, "Refresh token generation deferred to Phase 11", LOG_LEVEL_DEBUG, 0);
    return NULL;
}

bool oidc_token_split_compact(const char *jwt,
                              char **header_b64_out,
                              char **payload_b64_out,
                              char **sig_b64_out) {
    if (!jwt || !header_b64_out || !payload_b64_out || !sig_b64_out) {
        return false;
    }
    *header_b64_out = NULL;
    *payload_b64_out = NULL;
    *sig_b64_out = NULL;

    const char *dot1 = strchr(jwt, '.');
    if (!dot1) {
        return false;
    }
    const char *dot2 = strchr(dot1 + 1, '.');
    if (!dot2 || strchr(dot2 + 1, '.') != NULL) {
        return false;
    }

    size_t hlen = (size_t)(dot1 - jwt);
    size_t plen = (size_t)(dot2 - dot1 - 1);
    size_t slen = strlen(dot2 + 1);
    if (hlen == 0U || plen == 0U || slen == 0U) {
        return false;
    }

    char *hb = (char*)malloc(hlen + 1U);
    char *pb = (char*)malloc(plen + 1U);
    char *sb = (char*)malloc(slen + 1U);
    if (!hb || !pb || !sb) {
        free(hb);
        free(pb);
        free(sb);
        return false;
    }
    memcpy(hb, jwt, hlen);
    hb[hlen] = '\0';
    memcpy(pb, dot1 + 1, plen);
    pb[plen] = '\0';
    memcpy(sb, dot2 + 1, slen);
    sb[slen] = '\0';

    *header_b64_out = hb;
    *payload_b64_out = pb;
    *sig_b64_out = sb;
    return true;
}

OIDCTokenClaims* oidc_claims_from_payload_json(const char *payload_json) {
    if (!payload_json) {
        return NULL;
    }
    json_error_t err;
    json_t *root = json_loads(payload_json, 0, &err);
    if (!root || !json_is_object(root)) {
        if (root) {
            json_decref(root);
        }
        return NULL;
    }

    const char *iss = json_string_value(json_object_get(root, "iss"));
    const char *sub = json_string_value(json_object_get(root, "sub"));
    if (!iss || !sub) {
        json_decref(root);
        return NULL;
    }

    OIDCTokenClaims *claims = (OIDCTokenClaims*)calloc(1, sizeof(OIDCTokenClaims));
    if (!claims) {
        json_decref(root);
        return NULL;
    }
    claims->iss = strdup(iss);
    claims->sub = strdup(sub);
    if (!claims->iss || !claims->sub) {
        oidc_free_token_claims(claims);
        json_decref(root);
        return NULL;
    }

    json_t *aud = json_object_get(root, "aud");
    if (json_is_string(aud)) {
        const char *a = json_string_value(aud);
        claims->aud = (char**)calloc(1, sizeof(char*));
        if (claims->aud) {
            claims->aud[0] = strdup(a ? a : "");
            claims->aud_count = claims->aud[0] ? 1U : 0U;
        }
        claims->client_id = a ? strdup(a) : NULL;
    } else if (json_is_array(aud) && json_array_size(aud) > 0) {
        const char *a = json_string_value(json_array_get(aud, 0));
        claims->aud = (char**)calloc(1, sizeof(char*));
        if (claims->aud && a) {
            claims->aud[0] = strdup(a);
            claims->aud_count = 1U;
        }
        claims->client_id = a ? strdup(a) : NULL;
    }

    json_t *exp = json_object_get(root, "exp");
    if (json_is_integer(exp)) {
        claims->exp = (time_t)json_integer_value(exp);
    }
    json_t *iat = json_object_get(root, "iat");
    if (json_is_integer(iat)) {
        claims->iat = (time_t)json_integer_value(iat);
    }
    json_t *nbf = json_object_get(root, "nbf");
    if (json_is_integer(nbf)) {
        claims->nbf = (time_t)json_integer_value(nbf);
    }

    const char *scope = json_string_value(json_object_get(root, "scope"));
    if (scope) {
        claims->scope = strdup(scope);
    }
    const char *nonce = json_string_value(json_object_get(root, "nonce"));
    if (nonce) {
        claims->nonce = strdup(nonce);
    }
    const char *jti = json_string_value(json_object_get(root, "jti"));
    if (jti) {
        claims->jti = strdup(jti);
    }
    const char *client_id = json_string_value(json_object_get(root, "client_id"));
    if (client_id && !claims->client_id) {
        claims->client_id = strdup(client_id);
    }

    /* Preserve non-core claims as user_data for userinfo filtering. */
    json_t *extra = json_object();
    if (extra) {
        const char *key;
        json_t *val;
        json_object_foreach(root, key, val) {
            if (!key) {
                continue;
            }
            if (strcmp(key, "iss") == 0 || strcmp(key, "sub") == 0 ||
                strcmp(key, "aud") == 0 || strcmp(key, "exp") == 0 ||
                strcmp(key, "iat") == 0 || strcmp(key, "nbf") == 0 ||
                strcmp(key, "jti") == 0 || strcmp(key, "scope") == 0 ||
                strcmp(key, "nonce") == 0 || strcmp(key, "client_id") == 0 ||
                strcmp(key, "token_use") == 0 || strcmp(key, "azp") == 0) {
                continue;
            }
            json_object_set(extra, key, val);
        }
        if (json_object_size(extra) > 0U) {
            claims->user_data = json_dumps(extra, JSON_COMPACT);
        }
        json_decref(extra);
    }

    json_decref(root);
    return claims;
}

bool oidc_validate_access_token(const OIDCTokenContext *context, const char *access_token,
                                OIDCTokenClaims **claims_out) {
    if (claims_out) {
        *claims_out = NULL;
    }
    if (!context || !access_token || !context->key_context) {
        log_this(SR_OIDC, "Invalid parameters for token validation", LOG_LEVEL_ERROR, 0);
        return false;
    }

    char *header_b64 = NULL;
    char *payload_b64 = NULL;
    char *sig_b64 = NULL;
    if (!oidc_token_split_compact(access_token, &header_b64, &payload_b64, &sig_b64)) {
        return false;
    }

    size_t hraw_len = 0;
    unsigned char *hraw = utils_base64url_decode(header_b64, &hraw_len);
    if (!hraw) {
        free(header_b64);
        free(payload_b64);
        free(sig_b64);
        return false;
    }
    json_error_t jerr;
    json_t *hdr = json_loadb((const char*)hraw, hraw_len, 0, &jerr);
    free(hraw);
    if (!hdr) {
        free(header_b64);
        free(payload_b64);
        free(sig_b64);
        return false;
    }

    const char *alg = json_string_value(json_object_get(hdr, "alg"));
    if (!alg || strcmp(alg, "RS256") != 0) {
        json_decref(hdr);
        free(header_b64);
        free(payload_b64);
        free(sig_b64);
        return false;
    }
    const char *kid = json_string_value(json_object_get(hdr, "kid"));
    OIDCKeyContext *keys = (OIDCKeyContext*)context->key_context;
    OIDCKey *key = NULL;
    if (kid && kid[0] != '\0') {
        key = oidc_find_key_by_id(keys, kid);
    }
    if (!key) {
        key = oidc_get_active_signing_key(keys);
    }
    json_decref(hdr);
    if (!key || !key->key_data) {
        free(header_b64);
        free(payload_b64);
        free(sig_b64);
        return false;
    }

    char *signing_input = NULL;
    if (asprintf(&signing_input, "%s.%s", header_b64, payload_b64) < 0) {
        free(header_b64);
        free(payload_b64);
        free(sig_b64);
        return false;
    }

    size_t sig_len = 0;
    unsigned char *sig = utils_base64url_decode(sig_b64, &sig_len);
    free(header_b64);
    free(sig_b64);
    if (!sig) {
        free(payload_b64);
        free(signing_input);
        return false;
    }

    bool ok = utils_rs256_verify((EVP_PKEY*)key->key_data,
                                 (const unsigned char*)signing_input,
                                 strlen(signing_input),
                                 sig, sig_len);
    free(sig);
    free(signing_input);
    if (!ok) {
        free(payload_b64);
        return false;
    }

    size_t praw_len = 0;
    unsigned char *praw = utils_base64url_decode(payload_b64, &praw_len);
    free(payload_b64);
    if (!praw) {
        return false;
    }
    char *payload_json = (char*)malloc(praw_len + 1U);
    if (!payload_json) {
        free(praw);
        return false;
    }
    memcpy(payload_json, praw, praw_len);
    payload_json[praw_len] = '\0';
    free(praw);

    json_t *pl = json_loads(payload_json, 0, &jerr);
    if (!pl) {
        free(payload_json);
        return false;
    }

    /* Access tokens must carry token_use=access (id_tokens rejected). */
    const char *token_use = json_string_value(json_object_get(pl, "token_use"));
    if (!token_use || strcmp(token_use, "access") != 0) {
        json_decref(pl);
        free(payload_json);
        return false;
    }

    json_t *exp_j = json_object_get(pl, "exp");
    if (!json_is_integer(exp_j)) {
        json_decref(pl);
        free(payload_json);
        return false;
    }
    time_t exp = (time_t)json_integer_value(exp_j);
    time_t now = time(NULL);
    /* Small clock skew allowance (60s). */
    if (exp + 60 < now) {
        json_decref(pl);
        free(payload_json);
        return false;
    }
    json_t *nbf_j = json_object_get(pl, "nbf");
    if (json_is_integer(nbf_j)) {
        time_t nbf = (time_t)json_integer_value(nbf_j);
        if (nbf > now + 60) {
            json_decref(pl);
            free(payload_json);
            return false;
        }
    }
    json_decref(pl);

    OIDCTokenClaims *claims = oidc_claims_from_payload_json(payload_json);
    free(payload_json);
    if (!claims) {
        return false;
    }

    if (claims_out) {
        *claims_out = claims;
    } else {
        oidc_free_token_claims(claims);
    }
    return true;
}

bool oidc_validate_refresh_token(const OIDCTokenContext *context, const char *refresh_token, const char *client_id) {
    if (!context || !refresh_token || !client_id) {
        log_this(SR_OIDC, "Invalid parameters for token validation", LOG_LEVEL_ERROR, 0);
        return false;
    }
    (void)refresh_token;
    (void)client_id;
    return false;
}

bool oidc_revoke_token(const OIDCTokenContext *context,
                       const char *token,
                       const char *token_type_hint,
                       const char *client_id) {
    if (!context || !token || !client_id) {
        log_this(SR_OIDC, "Invalid parameters for token revocation", LOG_LEVEL_ERROR, 0);
        return false;
    }
    (void)token_type_hint;
    (void)token;
    (void)client_id;
    return false;
}
