/**
 * @file cap_verify.h
 * @brief ChaCha (Cap) token verification helper.
 *
 * Provides a single call that verifies a Cap token with the
 * configured ChaCha siteverify endpoint. The helper is used by the
 * protected `cap_query` conduit endpoint before any database query
 * is executed.
 *
 * The helper reads `WebServer.ChaChaServer` and
 * `WebServer.ChaChaSecret` from the global `app_config`. The site
 * identifier is supplied by the caller (typically parsed from the
 * request body).
 *
 * A test seam lets Unity tests substitute the outbound POST call
 * without mocking libcurl symbols.
 */

#ifndef CAP_VERIFY_H
#define CAP_VERIFY_H

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Result of a Cap token verification attempt.
 */
typedef enum {
    CAP_VERIFY_OK = 0,        // Token verified successfully
    CAP_VERIFY_HARD_FAIL = 1, // Token missing, invalid, or explicitly rejected
    CAP_VERIFY_FALLBACK = 2   // Transient siteverify failure; accept but flag for review
} cap_verify_result_t;

/**
 * @brief Verify a Cap token with the siteverify endpoint.
 *
 * On success returns CAP_VERIFY_OK and leaves `error_out` empty. On a
 * hard failure (missing/empty token, misconfiguration, explicit rejection)
 * returns CAP_VERIFY_HARD_FAIL and writes a short, stable error token into
 * `error_out`. On a transient failure (network error, HTTP 5xx, timeout)
 * returns CAP_VERIFY_FALLBACK so the caller can accept the request but
 * flag it for later review.
 *
 * Secrets are never written to `error_out` or to non-DUMP logs.
 *
 * @param token     The Cap response token from the client.
 * @param site_id   The ChaCha site id to verify against.
 * @param error_out Buffer to receive an error token on hard failure.
 * @param error_sz  Size of `error_out`.
 * @return CAP_VERIFY_OK, CAP_VERIFY_HARD_FAIL, or CAP_VERIFY_FALLBACK.
 */
cap_verify_result_t cap_verify_token(const char* token,
                                     const char* site_id,
                                     char* error_out,
                                     size_t error_sz);

/**
 * @brief Test-only: replace the outbound POST implementation.
 *
 * The default implementation uses libcurl to POST JSON to the
 * siteverify endpoint. Tests can install a fake that returns a
 * heap-allocated response body (caller-owned) and an HTTP status.
 * Passing NULL restores the default implementation.
 *
 * The fake must return a malloc'd string on success (even empty) or
 * NULL on transport failure. When returning NULL it should populate
 * `error_out`.
 */
void cap_verify_test_set_post_fn(
    char* (*fn)(const char* url,
                const char* body,
                long* out_status,
                char* error_out,
                size_t error_sz));

/**
 * @brief Test-only: clear any installed POST fake.
 */
void cap_verify_test_clear_post_fn(void);

#endif // CAP_VERIFY_H
