/*
 * OIDC Relying Party — token-endpoint client (Phase 11).
 *
 * Exchanges an authorization code for OIDC tokens at the IdP's token
 * endpoint via POST. Pure server-to-server: the browser never sees
 * any of the values exchanged here.
 *
 * Implementation notes:
 *
 *   - All allocation is heap; failure on alloc returns BAD_INPUT
 *     (the caller cannot meaningfully retry an OOM mid-exchange).
 *   - The Authorization header for `client_secret_basic` is built
 *     from `client_id:client_secret` after URL-encoding both halves
 *     per RFC 6749 §2.3.1. This is critical: characters like ':' or
 *     '+' in a real Keycloak secret would otherwise corrupt the
 *     header.
 *   - On success, the response struct's strings are heap-allocated
 *     duplicates of the parsed JSON values. Callers must
 *     `oidc_rp_token_response_free` to release them, which scrubs
 *     the bytes before free per the OIDC RP module's
 *     sensitive-buffer hardening discipline.
 *   - 4xx response bodies are parsed best-effort to extract the
 *     `error` field per RFC 6749 §5.2 and translated to typed
 *     errors. Unknown error codes map to OTHER.
 */

#include <src/hydrogen.h>
#include <src/logging/logging.h>

#include "oidc_rp_token.h"

#include <src/api/api_utils.h>
#include <src/api/auth/oidc_rp/oidc_rp_http.h>
#include <src/utils/utils_crypto.h>

#include <jansson.h>
#include <stdlib.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

// Scrub-and-free for a single string field on the token response.
// `s` may be NULL.
void token_scrub_free(char **s) {
    if (!s || !*s) return;
    volatile char *p = (volatile char *)*s;
    size_t n = strlen(*s);
    while (n--) p[n] = 0;
    free(*s);
    *s = NULL;
}

// Append `key=urlencode(value)` (with optional leading '&') to a
// heap-grown buffer. Returns the new buffer pointer or NULL on
// failure (in which case `*buffer` is freed).
//
// The buffer pointer is updated in place; capacity is doubled as
// needed. Designed for building the small (< 1 KiB) form bodies
// the OAuth2 token endpoint expects.
char *append_form_param(char *buffer, size_t *out_len, size_t *out_cap,
                               const char *key, const char *value) {
    if (!key || !value) {
        free(buffer);
        return NULL;
    }
    char *enc = api_url_encode(value);
    if (!enc) {
        free(buffer);
        return NULL;
    }

    size_t key_len = strlen(key);
    size_t enc_len = strlen(enc);
    size_t needed = *out_len + key_len + 1 /*=*/ + enc_len + 1 /*&*/ + 1 /*NUL*/;

    if (needed > *out_cap) {
        size_t new_cap = *out_cap ? *out_cap * 2 : 256;
        while (new_cap < needed) new_cap *= 2;
        char *new_buf = realloc(buffer, new_cap);
        if (!new_buf) {
            free(buffer);
            free(enc);
            return NULL;
        }
        buffer = new_buf;
        *out_cap = new_cap;
    }

    if (*out_len > 0) {
        buffer[(*out_len)++] = '&';
    }
    memcpy(buffer + *out_len, key, key_len); *out_len += key_len;
    buffer[(*out_len)++] = '=';
    memcpy(buffer + *out_len, enc, enc_len); *out_len += enc_len;
    buffer[*out_len] = '\0';

    // Scrub the encoded value before freeing — for `client_secret`
    // and `code_verifier`, the buffer briefly holds them in the clear.
    volatile char *q = (volatile char *)enc;
    size_t k = enc_len;
    while (k--) q[k] = 0;
    free(enc);

    return buffer;
}

// Build the form body for the POST. Returns NULL on allocation
// failure or invalid input.
//
// Body fields (always):
//   grant_type=authorization_code
//   code=...
//   redirect_uri=...
//   code_verifier=...
//
// When auth_method == client_secret_post, also appends:
//   client_id=...
//   client_secret=...
//
// All values are URL-encoded.
char *build_token_body(const OIDCRPProviderConfig *provider,
                              const char *code,
                              const char *redirect_uri,
                              const char *code_verifier) {
    char *buf = NULL;
    size_t len = 0, cap = 0;

    buf = append_form_param(buf, &len, &cap, "grant_type", "authorization_code");
    if (!buf) return NULL;
    buf = append_form_param(buf, &len, &cap, "code", code);
    if (!buf) return NULL;
    buf = append_form_param(buf, &len, &cap, "redirect_uri", redirect_uri);
    if (!buf) return NULL;
    buf = append_form_param(buf, &len, &cap, "code_verifier", code_verifier);
    if (!buf) return NULL;

    if (provider->auth_method == OIDC_RP_AUTH_METHOD_CLIENT_SECRET_POST) {
        buf = append_form_param(buf, &len, &cap, "client_id", provider->client_id);
        if (!buf) return NULL;
        buf = append_form_param(buf, &len, &cap, "client_secret",
                                provider->client_secret ? provider->client_secret : "");
        if (!buf) return NULL;
    }

    return buf;
}

// Build the `Basic xxx` Authorization header value for
// client_secret_basic auth. Per RFC 6749 §2.3.1, both halves are
// URL-encoded with form-encoding rules before being joined with `:`
// and base64-encoded.
//
// Returns a heap-allocated string ("Basic <base64>") or NULL on
// allocation failure / NULL input.
char *build_basic_auth_header(const char *client_id,
                                     const char *client_secret) {
    if (!client_id || !client_secret) return NULL;

    char *enc_id = api_url_encode(client_id);
    char *enc_secret = api_url_encode(client_secret);
    if (!enc_id || !enc_secret) {
        free(enc_id);
        free(enc_secret);
        return NULL;
    }

    size_t id_len = strlen(enc_id);
    size_t secret_len = strlen(enc_secret);
    size_t cred_len = id_len + 1 /*:*/ + secret_len;
    char *creds = malloc(cred_len + 1);
    if (!creds) {
        free(enc_id);
        free(enc_secret);
        return NULL;
    }
    memcpy(creds, enc_id, id_len);
    creds[id_len] = ':';
    memcpy(creds + id_len + 1, enc_secret, secret_len);
    creds[cred_len] = '\0';

    // Encoded URL-encoded values are no longer needed; scrub before free.
    volatile char *p = (volatile char *)enc_secret;
    size_t k = secret_len;
    while (k--) p[k] = 0;
    free(enc_id);
    free(enc_secret);

    char *b64 = utils_base64_encode((const unsigned char *)creds, cred_len);

    // Scrub the cleartext credentials buffer before free.
    volatile char *q = (volatile char *)creds;
    size_t n = cred_len;
    while (n--) q[n] = 0;
    free(creds);

    if (!b64) return NULL;

    size_t b64_len = strlen(b64);
    size_t hdr_len = strlen("Basic ") + b64_len + 1;
    char *header = malloc(hdr_len);
    if (!header) {
        free(b64);
        return NULL;
    }
    snprintf(header, hdr_len, "Basic %s", b64);

    // base64 of credentials still contains the secret material;
    // scrub before free.
    volatile char *r = (volatile char *)b64;
    size_t m = b64_len;
    while (m--) r[m] = 0;
    free(b64);

    return header;
}

// Map a 4xx response with an `error` JSON field (RFC 6749 §5.2) to
// the typed error code Hydrogen exposes upstream.
OidcRpTokenError map_oauth_error_code(const char *error) {
    if (!error) return OIDC_RP_TOKEN_ERR_OTHER;
    if (strcmp(error, "invalid_grant") == 0)        return OIDC_RP_TOKEN_ERR_INVALID_GRANT;
    if (strcmp(error, "invalid_request") == 0)      return OIDC_RP_TOKEN_ERR_INVALID_GRANT;
    if (strcmp(error, "unauthorized_client") == 0)  return OIDC_RP_TOKEN_ERR_INVALID_CLIENT;
    if (strcmp(error, "invalid_client") == 0)       return OIDC_RP_TOKEN_ERR_INVALID_CLIENT;
    if (strcmp(error, "unsupported_grant_type") == 0) return OIDC_RP_TOKEN_ERR_INVALID_GRANT;
    if (strcmp(error, "invalid_scope") == 0)        return OIDC_RP_TOKEN_ERR_INVALID_GRANT;
    return OIDC_RP_TOKEN_ERR_OTHER;
}

// Best-effort parse of a 4xx JSON body for the OAuth2 `error` field.
// Returns the mapped typed error, or OTHER on any parse failure.
OidcRpTokenError parse_error_body(const char *body) {
    if (!body || !*body) return OIDC_RP_TOKEN_ERR_OTHER;
    json_error_t jerr;
    json_t *root = json_loads(body, JSON_REJECT_DUPLICATES, &jerr);
    if (!root) return OIDC_RP_TOKEN_ERR_OTHER;
    OidcRpTokenError mapped = OIDC_RP_TOKEN_ERR_OTHER;
    if (json_is_object(root)) {
        json_t *err = json_object_get(root, "error");
        if (json_is_string(err)) {
            mapped = map_oauth_error_code(json_string_value(err));
        }
    }
    json_decref(root);
    return mapped;
}

// Parse a 2xx JSON body into a token response. Returns NULL on any
// failure (malformed JSON, missing id_token, allocation). The
// caller logs the typed error.
OidcRpTokenResponse *parse_success_body(const char *body) {
    if (!body || !*body) return NULL;

    json_error_t jerr;
    json_t *root = json_loads(body, JSON_REJECT_DUPLICATES, &jerr);
    if (!root) return NULL;

    OidcRpTokenResponse *resp = NULL;
    if (!json_is_object(root)) goto out;

    json_t *id_token = json_object_get(root, "id_token");
    if (!json_is_string(id_token)) goto out;

    resp = calloc(1, sizeof(*resp));
    if (!resp) goto out;

    resp->id_token = strdup(json_string_value(id_token));
    if (!resp->id_token) { oidc_rp_token_response_free(resp); resp = NULL; goto out; }

    json_t *access_token = json_object_get(root, "access_token");
    if (json_is_string(access_token)) {
        resp->access_token = strdup(json_string_value(access_token));
        if (!resp->access_token) {
            oidc_rp_token_response_free(resp); resp = NULL; goto out;
        }
    }

    json_t *token_type = json_object_get(root, "token_type");
    if (json_is_string(token_type)) {
        resp->token_type = strdup(json_string_value(token_type));
        if (!resp->token_type) {
            oidc_rp_token_response_free(resp); resp = NULL; goto out;
        }
    }

    json_t *expires_in = json_object_get(root, "expires_in");
    if (json_is_integer(expires_in)) {
        resp->expires_in = (long)json_integer_value(expires_in);
    }

out:
    json_decref(root);
    return resp;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void oidc_rp_token_response_free(OidcRpTokenResponse *response) {
    if (!response) return;
    token_scrub_free(&response->id_token);
    token_scrub_free(&response->access_token);
    token_scrub_free(&response->token_type);
    response->expires_in = 0;
    free(response);
}

const char *oidc_rp_token_error_name(OidcRpTokenError err) {
    switch (err) {
        case OIDC_RP_TOKEN_OK:                   return "ok";
        case OIDC_RP_TOKEN_ERR_BAD_INPUT:        return "bad_input";
        case OIDC_RP_TOKEN_ERR_NO_TOKEN_ENDPOINT:return "no_token_endpoint";
        case OIDC_RP_TOKEN_ERR_TRANSPORT:        return "transport";
        case OIDC_RP_TOKEN_ERR_INVALID_GRANT:    return "invalid_grant";
        case OIDC_RP_TOKEN_ERR_INVALID_CLIENT:   return "invalid_client";
        case OIDC_RP_TOKEN_ERR_SERVER:           return "server_error";
        case OIDC_RP_TOKEN_ERR_BAD_RESPONSE:     return "bad_response";
        case OIDC_RP_TOKEN_ERR_OTHER:            return "other";
        default:                                 return "unknown";
    }
}

OidcRpTokenError oidc_rp_exchange_code(const OIDCRPProviderConfig *provider,
                                       const char *token_endpoint,
                                       const char *code,
                                       const char *redirect_uri,
                                       const char *code_verifier,
                                       OidcRpTokenResponse **out_response) {
    if (out_response) *out_response = NULL;
    if (!provider || !out_response) return OIDC_RP_TOKEN_ERR_BAD_INPUT;
    if (!provider->client_id || !*provider->client_id) return OIDC_RP_TOKEN_ERR_BAD_INPUT;
    if (!code || !*code) return OIDC_RP_TOKEN_ERR_BAD_INPUT;
    if (!redirect_uri || !*redirect_uri) return OIDC_RP_TOKEN_ERR_BAD_INPUT;
    if (!code_verifier || !*code_verifier) return OIDC_RP_TOKEN_ERR_BAD_INPUT;
    if (!token_endpoint || !*token_endpoint) return OIDC_RP_TOKEN_ERR_NO_TOKEN_ENDPOINT;

    // Build the form body (always includes grant_type/code/redirect_uri/
    // code_verifier; conditionally includes client_id/client_secret for
    // client_secret_post).
    char *body = build_token_body(provider, code, redirect_uri, code_verifier);
    if (!body) return OIDC_RP_TOKEN_ERR_BAD_INPUT;

    // Build the Authorization header for client_secret_basic.
    char *auth_header = NULL;
    if (provider->auth_method == OIDC_RP_AUTH_METHOD_CLIENT_SECRET_BASIC) {
        // For confidential clients (the typical Hydrogen RP setup),
        // a missing client_secret with basic auth is a configuration
        // error. Fail fast rather than sending an empty secret.
        if (!provider->client_secret || !*provider->client_secret) {
            // Scrub body before free.
            volatile char *p = (volatile char *)body;
            size_t n = strlen(body);
            while (n--) p[n] = 0;
            free(body);
            return OIDC_RP_TOKEN_ERR_BAD_INPUT;
        }
        auth_header = build_basic_auth_header(provider->client_id,
                                              provider->client_secret);
        if (!auth_header) {
            volatile char *p = (volatile char *)body;
            size_t n = strlen(body);
            while (n--) p[n] = 0;
            free(body);
            return OIDC_RP_TOKEN_ERR_BAD_INPUT;
        }
    }

    OidcRpHttpResponse *http = oidc_rp_http_post(
        token_endpoint,
        provider->verify_ssl,
        body,
        "application/x-www-form-urlencoded",
        "application/json",
        auth_header);

    // Scrub the form body before free — it contained code_verifier
    // and (for client_secret_post) client_secret in the clear.
    {
        volatile char *p = (volatile char *)body;
        size_t n = strlen(body);
        while (n--) p[n] = 0;
        free(body);
    }
    if (auth_header) {
        volatile char *p = (volatile char *)auth_header;
        size_t n = strlen(auth_header);
        while (n--) p[n] = 0;
        free(auth_header);
    }

    if (!http) {
        // calloc failure inside the wrapper.
        return OIDC_RP_TOKEN_ERR_TRANSPORT;
    }

    OidcRpTokenError result;
    long status = http->http_status;

    if (status == 0) {
        // Transport-level failure already logged by the wrapper.
        result = OIDC_RP_TOKEN_ERR_TRANSPORT;
    } else if (status >= 200 && status < 300) {
        OidcRpTokenResponse *parsed = parse_success_body(http->body);
        if (parsed) {
            *out_response = parsed;
            result = OIDC_RP_TOKEN_OK;
        } else {
            log_this(SR_AUTH,
                "OIDC_RP: token exchange returned HTTP %ld but body could not be parsed",
                LOG_LEVEL_ALERT, 1, status);
            result = OIDC_RP_TOKEN_ERR_BAD_RESPONSE;
        }
    } else if (status >= 500) {
        log_this(SR_AUTH,
            "OIDC_RP: token endpoint returned HTTP %ld (server error)",
            LOG_LEVEL_ALERT, 1, status);
        result = OIDC_RP_TOKEN_ERR_SERVER;
    } else {
        // 4xx: try to extract the OAuth2 error code.
        result = parse_error_body(http->body);
        // 401 with an unparseable body is almost certainly an
        // invalid_client (RFC 6749 §5.2).
        if (result == OIDC_RP_TOKEN_ERR_OTHER && status == 401) {
            result = OIDC_RP_TOKEN_ERR_INVALID_CLIENT;
        }
        log_this(SR_AUTH,
            "OIDC_RP: token endpoint returned HTTP %ld (mapped error: %s)",
            LOG_LEVEL_ALERT, 2, status, oidc_rp_token_error_name(result));
    }

    oidc_rp_http_response_free(http);
    return result;
}
