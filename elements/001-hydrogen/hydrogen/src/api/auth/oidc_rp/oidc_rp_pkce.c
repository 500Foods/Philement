/*
 * OIDC Relying Party PKCE / nonce / state generators.
 *
 * Implements `docs/H/plans/OIDC-PLAN.md` Phase 8. See the header for the
 * contract and rationale; this file holds only the mechanics.
 *
 * Every function delegates entropy to `utils_random_bytes()` (OpenSSL
 * `RAND_bytes`) and base64url encoding to `utils_base64url_encode()`.
 * That keeps the OIDC RP code aligned with what `auth_service_jwt.c`
 * already uses for `jti` generation, so a future audit only has to
 * trust one entropy/encoding pair across the auth subsystem.
 */

// Global includes
#include <src/hydrogen.h>

// Standard library
#include <stdlib.h>
#include <string.h>

// Local
#include "oidc_rp_pkce.h"
#include <src/utils/utils_crypto.h>
#include <src/logging/logging.h>

// Cap for `oidc_rp_make_random_hex`: keeps the buffer math safe even
// if a future caller passes nonsense. Everything we generate in this
// codebase is ≤ 32 bytes (64 hex chars), so 256 is comfortable
// headroom without risking integer overflow on the output length.
#define OIDC_RP_RANDOM_HEX_MAX_BYTES 256

// Lower-case hex digit table. Used by `oidc_rp_make_random_hex` to
// produce URL-safe values without percent-encoding.
static const char hex_digits[] = "0123456789abcdef";

char* oidc_rp_make_code_verifier(void) {
    unsigned char raw[OIDC_RP_PKCE_VERIFIER_RAW_BYTES];

    if (!utils_random_bytes(raw, sizeof(raw))) {
        log_this(SR_AUTH,
                 "OIDC_RP: failed to draw entropy for PKCE verifier",
                 LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    // 32 bytes -> 43 base64url chars (no padding). The caller treats
    // this as a secret; the encoder is the same one used for JWT
    // segments, so no novel surface area is introduced.
    char* verifier = utils_base64url_encode(raw, sizeof(raw));

    // Scrub the raw entropy buffer regardless of encoder outcome.
    // `volatile` guards against dead-store elimination at -O2.
    {
        volatile unsigned char* p = raw;
        for (size_t i = 0; i < sizeof(raw); ++i) p[i] = 0;
    }

    if (!verifier) {
        log_this(SR_AUTH,
                 "OIDC_RP: failed to encode PKCE verifier",
                 LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    return verifier;
}

char* oidc_rp_make_code_challenge(const char* verifier) {
    if (!verifier || verifier[0] == '\0') {
        return NULL;
    }

    // RFC 7636 §4.2: code_challenge = BASE64URL(SHA256(verifier)).
    // `utils_sha256_hash` performs both steps, so no intermediate
    // 32-byte digest buffer needs to live in our scope.
    char* challenge = utils_sha256_hash((const unsigned char*)verifier,
                                        strlen(verifier));
    if (!challenge) {
        log_this(SR_AUTH,
                 "OIDC_RP: failed to compute PKCE challenge",
                 LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    return challenge;
}

char* oidc_rp_make_random_hex(size_t byte_count) {
    if (byte_count == 0 || byte_count > OIDC_RP_RANDOM_HEX_MAX_BYTES) {
        return NULL;
    }

    unsigned char* raw = malloc(byte_count);
    if (!raw) {
        return NULL;
    }

    if (!utils_random_bytes(raw, byte_count)) {
        log_this(SR_AUTH,
                 "OIDC_RP: failed to draw entropy for random hex",
                 LOG_LEVEL_ERROR, 0);
        free(raw);
        return NULL;
    }

    // Two hex chars per byte, plus NUL.
    size_t out_len = byte_count * 2;
    char* hex = malloc(out_len + 1);
    if (!hex) {
        // Scrub the raw bytes before releasing them — they may be
        // about to be reused for `nonce` or `state` and we shouldn't
        // leave them sitting in a freed allocation either way.
        volatile unsigned char* p = raw;
        for (size_t i = 0; i < byte_count; ++i) p[i] = 0;
        free(raw);
        return NULL;
    }

    for (size_t i = 0; i < byte_count; ++i) {
        unsigned char b = raw[i];
        hex[(i * 2) + 0] = hex_digits[(b >> 4) & 0x0F];
        hex[(i * 2) + 1] = hex_digits[b & 0x0F];
    }
    hex[out_len] = '\0';

    // Scrub raw entropy. The encoded form is the only copy that
    // should outlive this function.
    {
        volatile unsigned char* p = raw;
        for (size_t i = 0; i < byte_count; ++i) p[i] = 0;
    }
    free(raw);

    return hex;
}
