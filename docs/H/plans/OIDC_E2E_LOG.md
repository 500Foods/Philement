# OIDC End-to-End Test Log (Phase 27)

> Manual test checklist for Phase 27 of [OIDC-PLAN.md](/docs/OIDC-PLAN.md).
> This document records the result of running the full sign-in flow
> against the real Keycloak instance at
> `https://www.500passwords.com/realms/festival`, replacing the mock
> Keycloak used in Phases 1–22.

---

## Environment

| Item | Value |
|---|---|
| Keycloak issuer | `https://www.500passwords.com/realms/festival` |
| Keycloak client id | `lithium` |
| Hydrogen origin | `https://lithium.philement.com` |
| Lithium origin | `https://lithium.philement.com` (same host) |
| Redirect URI | `https://lithium.philement.com/api/auth/oidc/callback` |
| `OIDC_RP.Database` | `Lithium` |
| `OIDC_RP.Enabled` | `true` |
| `AccountLinking.Strategy` | `match_email_only` |
| `ProvisionDefaults.Enabled` | `false` |
| `RequireEmailVerified` | `true` |
| `BindHandoffToIp` | `true` |

### Environment variables

- `HYDROGEN_OIDC_CLIENT_ID` set (deploy environment only)
- `HYDROGEN_OIDC_CLIENT_SECRET` set (deploy environment only, never on disk)

### Keycloak users used for testing

| User | Pre-existing Lithium account? | Verified email? | Expected outcome |
|---|---|---|---|
| `oidc-test-1@philement.com` | Yes (linked by email) | Yes | Sign-in succeeds; `account_oidc_identities` row created on first try. |
| `oidc-test-2@philement.com` | No | Yes | First-time sign-in fails with `oidc_error=no_account`. After admin creates the account, second attempt succeeds. |

---

## Test Plan

The seven items below come directly from Phase 27 of the OIDC plan
(line 4972). Tick each one as the manual test passes. Note the date,
operator, and any observations.

### 1. Existing password user logs in with password

> **Precondition:** Test user has a `password_hash` in `accounts`.

- **Steps:** Open Lithium login page, type username + password, submit.
- **Expected:** Lands in main app; JWT stored; `AUTH_LOGIN` emitted.
- **Result:** [ ] Pass [ ] Fail
- **Date:**
- **Operator:**
- **Notes:**

### 2. Existing password user clicks "Sign in with 500 Passwords"

> **Precondition:** Same user as above; their email is verified in Keycloak.

- **Steps:** Open Lithium, click the OIDC button, authenticate at
  Keycloak with the user's IdP credentials.
- **Expected:** Returns to Lithium signed in with the **same**
  `account_id` as the password flow. A new `account_oidc_identities`
  row exists linking `(iss, sub)` to the account.
- **Result:** [ ] Pass [ ] Fail
- **Date:**
- **Operator:**
- **Notes:**

### 3. New 500passwords user signs in (no Lithium account)

> **Precondition:** Keycloak user exists; no `accounts` row matches their email.

- **Steps:** Click OIDC button, authenticate at Keycloak.
- **Expected:** Returns to Lithium login page with
  `oidc_error=no_account` mapped to "No account is linked to this
  identity." No `accounts` row is created (provisioning is disabled).
- **Result:** [ ] Pass [ ] Fail
- **Date:**
- **Operator:**
- **Notes:**

### 4. OIDC-logged-in user reloads page mid-session

> **Precondition:** User from test 2 is signed in.

- **Steps:** Hard reload the browser tab.
- **Expected:** JWT is still valid; user is not re-prompted; no
  duplicate handoff exchange (the URL was cleaned on first arrival).
- **Result:** [ ] Pass [ ] Fail
- **Date:**
- **Operator:**
- **Notes:**

### 5. OIDC-logged-in user clicks Logout

- **Steps:** Click the Logout control in Lithium.
- **Expected:** Hydrogen JWT is revoked; browser ends up on the login
  page; subsequent API calls would fail authentication.
- **Result:** [ ] Pass [ ] Fail
- **Date:**
- **Operator:**
- **Notes:**

### 6. JWT is renewed automatically while idle

- **Steps:** Sign in via OIDC, leave the tab idle past the session
  half-life, watch the network tab for a `/api/auth/renew` call.
- **Expected:** Renew succeeds; new `exp` is further in the future;
  no user-visible interruption.
- **Result:** [ ] Pass [ ] Fail
- **Date:**
- **Operator:**
- **Notes:**

### 7. All Lithium managers work identically for an OIDC user

- **Steps:** With an OIDC-signed-in user, exercise each manager:
  Queries, Lookups, Style, Crimson, Tour, Version, Session Log,
  User Profile.
- **Expected:** Identical behaviour to a password-logged-in user;
  role-gated UI shows the right items; Conduit calls succeed.
- **Result:** [ ] Pass [ ] Fail
- **Date:**
- **Operator:**
- **Notes:**

---

## Independent Reviewer Pass

Per Phase 27 Definition of Done, a second reviewer must run through the
checklist independently and confirm.

- **Reviewer:**
- **Date:**
- **Result:** [ ] Pass [ ] Fail
- **Notes:**

---

## Automated Test Suite Status

After flipping over to real Keycloak, the full automated suite must
remain green:

- [ ] `test_42_oidc_rp.sh` — passes against mock Keycloak (this is the
      pre-existing baseline; it does **not** target the real IdP).
- [ ] `test_40_auth.sh` — passes; OIDC-issued JWT is shape-equivalent
      to password-issued JWT.
- [ ] All Vitest suites green (`npm test`).
- [ ] `test_04_check_links.sh` green after new docs (`oidc_rp.md`,
      `LITHIUM-OIDC.md`, `LITHIUM-KEYCLOAK.md`) are linked from
      `SITEMAP.md` and `LITHIUM-TOC.md`.
- [ ] `test_90_markdownlint.sh` green on the new docs.

---

## Defects Found

> Record any issues discovered during the manual test pass. Each
> defect blocks Phase 27 sign-off until fixed.

(none yet)

---

## Sign-off

- [ ] All seven manual checks passed.
- [ ] Independent reviewer passed.
- [ ] Automated suite passed.
- [ ] No outstanding defects.

**Phase 27 complete on:** ____________
**Signed off by:** ____________
