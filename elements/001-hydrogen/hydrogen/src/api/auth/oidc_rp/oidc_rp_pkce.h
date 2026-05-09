/**
 * @file oidc_rp_pkce.h
 * @brief OIDC Relying Party PKCE / nonce / state generators.
 *
 * Phase 8 of the OIDC plan (`docs/OIDC-PLAN.md`). Pure crypto helpers:
 * no networking, no HTTP, no state-store interaction. These functions
 * generate the secret values that bind a single OIDC authorization
 * round-trip:
 *
 *   - **PKCE code_verifier** (RFC 7636 §4.1): 43–128 character
 *     URL-safe random string. We standardise on 43 characters
 *     (32 random bytes encoded with base64url, no padding).
 *   - **PKCE code_challenge** (RFC 7636 §4.2): `BASE64URL(SHA256(verifier))`.
 *     Sent in the `/authorize` redirect under
 *     `code_challenge=…&code_challenge_method=S256`. The verifier is
 *     kept server-side and presented back at `/token` exchange.
 *   - **state** and **nonce**: opaque random hex strings, used as a
 *     CSRF guard and as the `nonce` claim asserted in the ID token.
 *     We use lower-case hex purely so the values are URL-safe across
 *     every framework without further encoding.
 *
 * Randomness is sourced from `utils_random_bytes()`, which wraps
 * OpenSSL's `RAND_bytes` — the same entropy source the rest of
 * Hydrogen (notably `generate_jti`) already trusts.
 *
 * All returned strings are heap-allocated and the caller owns them.
 * On any failure the function returns NULL and never writes to
 * partially allocated buffers.
 *
 * Thread safety: every function is reentrant. The underlying
 * `RAND_bytes` is thread-safe per OpenSSL.
 *
 * @author Hydrogen Framework
 * @date 2026-05-09
 */

#ifndef OIDC_RP_PKCE_H
#define OIDC_RP_PKCE_H

#include <stddef.h>

/**
 * @brief Number of random bytes used to derive a PKCE verifier.
 *
 * 32 bytes of entropy is the recommended minimum (RFC 7636 §7.1
 * "Entropy of the code_verifier"). Encoded as base64url without
 * padding this becomes a 43-character verifier — well inside the
 * 43..128 range mandated by §4.1.
 */
#define OIDC_RP_PKCE_VERIFIER_RAW_BYTES 32

/**
 * @brief Length of the verifier string returned by
 *        `oidc_rp_make_code_verifier()`, excluding the NUL terminator.
 *
 * 32 bytes → ⌈32 * 4 / 3⌉ = 43 base64url characters.
 */
#define OIDC_RP_PKCE_VERIFIER_LENGTH 43

/**
 * @brief Length of the challenge string returned by
 *        `oidc_rp_make_code_challenge()`, excluding NUL.
 *
 * SHA-256 produces 32 bytes; encoded as base64url without padding
 * that is 43 characters — same width as the verifier we ship.
 */
#define OIDC_RP_PKCE_CHALLENGE_LENGTH 43

/**
 * @brief Generate a PKCE `code_verifier`.
 *
 * Produces 32 bytes of cryptographically random data and encodes
 * them using URL-safe base64 without padding. The result satisfies
 * the RFC 7636 §4.1 grammar:
 *
 *     code-verifier = 43*128unreserved
 *     unreserved    = ALPHA / DIGIT / "-" / "." / "_" / "~"
 *
 * (Our output uses `A-Z a-z 0-9 - _` only, which is a strict subset
 * of the unreserved set.)
 *
 * @return Heap-allocated NUL-terminated string of exactly
 *         `OIDC_RP_PKCE_VERIFIER_LENGTH` characters, or NULL on
 *         allocation or RNG failure. Caller owns the buffer and
 *         must `free()` it. Treat the buffer as sensitive — scrub
 *         before free if it survives past the `/token` exchange.
 */
char* oidc_rp_make_code_verifier(void);

/**
 * @brief Derive a PKCE `code_challenge` from a verifier.
 *
 * Computes `BASE64URL(SHA256(verifier))` per RFC 7636 §4.2. This is
 * the only challenge method we advertise (`code_challenge_method=S256`),
 * matching the Keycloak client requirement set out in
 * `docs/OIDC-PLAN.md` line 157.
 *
 * The verifier is read but not retained; the caller must not assume
 * any side-effects on the input buffer.
 *
 * @param verifier NUL-terminated verifier string. Must be non-NULL
 *                 and non-empty. The contents are not validated
 *                 against RFC 7636 grammar — that is the caller's
 *                 responsibility (in our flow the verifier always
 *                 originates from `oidc_rp_make_code_verifier()`).
 * @return Heap-allocated NUL-terminated challenge of
 *         `OIDC_RP_PKCE_CHALLENGE_LENGTH` characters, or NULL on
 *         invalid input or allocation failure. Caller must `free()`.
 */
char* oidc_rp_make_code_challenge(const char* verifier);

/**
 * @brief Generate a random hex string of the given byte length.
 *
 * Used for OAuth `state` and OIDC `nonce`. The output is lower-case
 * hexadecimal — a URL-safe character set that needs no further
 * percent-encoding. Internally this draws `byte_count` bytes from
 * `utils_random_bytes()` and renders them as `2 * byte_count` hex
 * digits.
 *
 * The RFC does not mandate a specific length for `state` or `nonce`,
 * but `docs/OIDC-PLAN.md` line 224 fixes 32 bytes (= 64 hex chars)
 * for both.
 *
 * @param byte_count Number of random bytes to draw. Must be > 0
 *                   and ≤ 256 (a sanity cap; everything we generate
 *                   today is ≤ 32).
 * @return Heap-allocated NUL-terminated hex string of length
 *         `2 * byte_count`, or NULL on invalid input, RNG failure,
 *         or allocation failure. Caller must `free()`.
 */
char* oidc_rp_make_random_hex(size_t byte_count);

#endif // OIDC_RP_PKCE_H
