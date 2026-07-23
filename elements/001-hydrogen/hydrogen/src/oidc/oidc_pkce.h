/*
 * OIDC IdP PKCE helpers (S256). Independent of OIDC RP helpers.
 */

#ifndef OIDC_PKCE_H
#define OIDC_PKCE_H

#include <stdbool.h>

/* RFC 7636 S256: code_challenge = BASE64URL(SHA256(verifier)) */
char* oidc_pkce_make_challenge_s256(const char *verifier);

/* true if verifier produces the expected challenge (S256 only). */
bool oidc_pkce_verify_s256(const char *verifier, const char *expected_challenge);

/* true if method is S256 (case-sensitive per common practice). */
bool oidc_pkce_method_is_s256(const char *method);

#endif /* OIDC_PKCE_H */
