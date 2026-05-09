/*
 * OIDC RP account linker — Phase 14 throwaway stub.
 *
 * See oidc_rp_link_stub.h for the rationale. The single function in
 * this file delegates entirely to lookup_account() (the same helper
 * that password login uses) and ignores the validated OIDC claims.
 * Phase 21 deletes this file when the real linker
 * (`oidc_rp_link.c`) lands.
 *
 * The fixed test account name is the one already provisioned by the
 * existing test database fixtures — see test_40_auth.sh's password
 * coverage for context. If that account ever changes, this stub
 * needs an update; that's an acceptable coupling for a temporary
 * scaffold.
 */

#include <src/hydrogen.h>

#include "oidc_rp_link_stub.h"

#include <stdlib.h>

// Fixed login_id used by the stub. The demo migrations create
// `adminuser` as account_id = 1 in the Acuranzo schema; that account
// exists across every supported DB engine fixture, so the stub
// resolves to it deterministically. Phase 21 replaces this with the
// real linker, at which point the stub goes away.
#define OIDC_RP_LINK_STUB_FIXED_LOGIN_ID "adminuser"

account_info_t *oidc_rp_link_stub_resolve(const OidcRpIdTokenClaims *claims,
                                          const char *database) {
    (void)claims;  // Stub deliberately ignores the OIDC identity.

    if (!database || !*database) {
        log_this(SR_AUTH,
                 "OIDC RP linker stub: missing database name",
                 LOG_LEVEL_ERROR, 0);
        return NULL;
    }

    account_info_t *account =
        lookup_account(OIDC_RP_LINK_STUB_FIXED_LOGIN_ID, database);
    if (!account) {
        log_this(SR_AUTH,
                 "OIDC RP linker stub: fixed account %s not found in %s",
                 LOG_LEVEL_ALERT, 2,
                 OIDC_RP_LINK_STUB_FIXED_LOGIN_ID,
                 database);
        return NULL;
    }

    // lookup_account intentionally does not populate username/email/
    // roles (they come back during password verification in the
    // password-login flow). For OIDC there is no password step, so
    // the stub fills in display-side fields from the fixed login_id
    // and (when present) the validated OIDC claims. Phase 21's real
    // linker will use the dedicated QueryRefs (#080–#084) plus a
    // role-mapping query to populate these properly.
    if (!account->username) {
        account->username = strdup(OIDC_RP_LINK_STUB_FIXED_LOGIN_ID);
    }
    if (claims && claims->email && !account->email) {
        account->email = strdup(claims->email);
    }
    if (!account->roles) {
        // Phase 14 stub: empty roles string — Phase 22 will populate
        // this from the accounts→account_roles join. Empty is the
        // safe default; the password login path also tolerates empty
        // roles when the join returns nothing.
        account->roles = strdup("");
    }

    log_this(SR_AUTH,
             "OIDC RP linker stub: resolved to account_id=%d username=%s (Phase 14 fixed-login stub)",
             LOG_LEVEL_DEBUG, 2,
             account->id,
             account->username ? account->username : "(null)");
    return account;
}
