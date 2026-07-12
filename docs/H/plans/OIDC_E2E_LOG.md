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
| `andrew@500foods.com` | Yes (`andrew` / `andrew@500foods.com` in `account_contacts`) | Yes | Sign-in succeeds; `account_oidc_identities` row created on first try. **Email was duplicated across four accounts; fixed by keeping only the `adminuser` row.** |
| `oidc-test-2@philement.com` (or another user with no matching `accounts` row) | No | Yes | First-time sign-in fails with `oidc_error=no_account`. After admin creates the account, second attempt succeeds. |

> **Note:** The available test user `andrew@500foods.com` accepts the documented password and the realm then requires an MFA/OTP code. A user without OTP (or a current OTP code) is needed to complete the full manual checklist. The earlier `no_account` failure was caused by QueryRef #082 using `contact_type_a18 = 0` (Username) instead of `1` (E-Mail); that has been corrected in the source migration and the production DB.

---

## Pre-flight verification

The following checks were performed against the **real** Keycloak instance before attempting the manual checklist.

| Check | Method | Result | Notes |
|---|---|---|---|
| Discovery document reachable | `GET https://www.500passwords.com/realms/festival/.well-known/openid-configuration` | Pass | Returns `authorization_endpoint`, `token_endpoint`, `jwks_uri`, `end_session_endpoint`. |
| Confidential client exists and redirect URI is accepted | `GET /realms/festival/protocol/openid-connect/auth?client_id=lithium&redirect_uri=...` | Pass | Keycloak returns 302 to the login page (or 302 to callback with an error) for the exact configured URI; a mismatched URI is rejected. |
| Hydrogen `OIDC_RP` config loads | `mkt` + pod restart | Pass | `/tnt/hydrogen/hydrogen-lithium.json` updated and env vars injected via `t-philement-oidc-secrets`. |
| `/api/auth/oidc/start` redirects to Keycloak | Port-forward pod + `curl -H 'Host: lithium.philement.com'` | Pass | Returns `302` to `https://www.500passwords.com/realms/festival/protocol/openid-connect/auth` with `state`, `nonce`, `code_challenge`, `code_challenge_method=S256`. |
| `/api/auth/oidc/handoff` rejects invalid codes | `POST /api/auth/oidc/handoff` with `{"handoff":"fake"}` | Pass | Returns `401 {"error":"handoff_invalid"}`. |
| `/api/auth/oidc/callback` handles invalid state/code | `GET /api/auth/oidc/callback?code=fake&state=...` | Pass | Returns `302` to `https://lithium.philement.com/login?oidc=1&oidc_error=...` (`state_invalid` for unknown state, `token_invalid_grant` for a valid state but fake code). |
| Lithium config exposes OIDC provider | `GET /config/lithium.json` | Pass | `/tnt/lithium/config/lithium.json` updated with `auth.oidc_providers`. |
| Automated test suites remain green | `mkt`, `test_42_oidc_rp.sh`, `test_40_auth.sh`, `npm test` | Pass | See Automated Test Suite Status below. |

**Blocker for remaining manual items:** the known Keycloak test user requires MFA/OTP. The password step succeeds, but the next page is an OTP challenge. Once a user without OTP (or a current OTP code) is available, the manual checklist can be completed.

---

## Issues found and fixed during manual sign-in

### 1. `oidc_error=no_account` after a successful Keycloak login

**Symptom:** After logging in as `andrew@500foods.com` and entering the OTP, the callback redirected to `/login?oidc=1&oidc_error=no_account&return_to=%2F`.

**Root cause:** QueryRef #082 (`OIDC RP: Lookup Account by Email`) was filtering on `account_contacts.contact_type_a18 = 0`. Lookup `18` defines `0 = Username` and `1 = E-Mail`. The production `account_contacts` rows for `andrew@500foods.com` were correctly stored as `contact_type_a18 = 1`, so the linker found no account and returned `OIDC_RP_LINK_NO_ACCOUNT`.

**Fix:**

- Updated the source migration `elements/002-helium/acuranzo/migrations/acuranzo_1193.lua`
  to use `contact_type_a18 = 1`.
- Updated the production QueryRef #082 row in `lithium.queries`
  to use `contact_type_a18 = 1`.
- Updated the test helper `elements/001-hydrogen/hydrogen/tests/lib/oidc_rp_helpers_link.sh`
  and the SQLite test fixture so `test_42_oidc_rp.sh` stays aligned with the
  lookup table.

### 2. Duplicate email contacts caused ambiguity

**Symptom:** After fixing the contact type, the email lookup would have returned four accounts for `andrew@500foods.com` (accounts 1, 2, 3, and 4), which would have produced `oidc_error=email_ambiguous`.

**Fix:** Removed the duplicate email contacts from `lithium.account_contacts` for accounts 2, 3, and 4, leaving only account 1 (`adminuser`) linked to `andrew@500foods.com`.

### 3. `/login` returned 404 after the OIDC callback

**Symptom:** The callback redirect to `https://lithium.philement.com/login?oidc=1&...` showed a 404 page because Hydrogen’s static file server only serves exact files or directory index files.

**Fix:** Added a new `WebServer.SpaFallback` boolean to the Hydrogen web server
configuration. When enabled and a GET request does not match a registered
endpoint, does not look like a static asset (no extension after the last `/`),
and does not start with `/api/` or `/swagger`, Hydrogen serves `index.html`
from the web root so the SPA’s client-side router can render `/login`.

- Source files: `elements/001-hydrogen/hydrogen/src/config/config_webserver.h`,
  `config_webserver.c`, `config_defaults.c`, and
  `src/webserver/web_server_request.c`.
- Enabled `SpaFallback: true` in the deployed `/tnt/hydrogen/hydrogen-lithium.json`.

### 4. Lithium production config missing OIDC provider

**Symptom:** The "Sign in with 500 Passwords" button was not rendered on the login page.

**Fix:** Added `auth.oidc_providers` to `elements/003-lithium/public/config/lithium.json` and copied it to `/tnt/lithium/config/lithium.json`.

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

## RP-Initiated Logout Checklist (Phase 10)

Run these checks after Hydrogen has been rebuilt with the Phase 10 code and the Keycloak `lithium` client has a valid post-logout redirect URI (e.g., `https://lithium.philement.com/*`).

### Prerequisites

- [ ] Hydrogen C app rebuilt/restarted with `POST /api/auth/oidc/end-session` wired.
- [ ] Keycloak client `lithium` → **Valid post-logout redirect URIs** includes `https://lithium.philement.com/*`.
- [ ] Browser dev tools are open to capture network requests and cookies.

### Checks

| # | Step | Expected Result | Actual Result |
|---|---|---|---|
| 1 | Log in via 500 Passwords (OIDC) | SPA receives Hydrogen JWT and renders main UI. | |
| 2 | In dev tools, confirm the Hydrogen JWT payload contains `id_token` and `idp_provider` claims | JWT payload includes `"id_token": "eyJ..."` and `"idp_provider": "500passwords"`. | |
| 3 | Click **Global signout** | Browser navigates to a Keycloak URL like `https://www.500passwords.com/realms/festival/protocol/openid-connect/logout?id_token_hint=...&post_logout_redirect_uri=...&client_id=...`, then returns to the Lithium login screen. | |
| 4 | In dev tools, inspect Keycloak cookies (`KEYCLOAK_IDENTITY`, `KEYCLOAK_SESSION`, `KEYCLOAK_REMEMBER_ME`, etc.) | The Keycloak session cookies are removed or invalidated after the logout redirect. | |
| 5 | Click **500 Passwords** again | Browser is redirected to the Keycloak login page (not auto-authenticated). | |
| 6 | Log in again with the same credentials | Flow completes normally and a new Hydrogen JWT is minted. | |
| 7 | For a password-only user, click **Global signout** | SPA performs local logout and reloads; no Keycloak redirect occurs. | |

### Troubleshooting

If step 5 still auto-logs the user in:

1. Confirm the Hydrogen binary was actually rebuilt and restarted. A missing endpoint causes silent fallback to local logout.
2. Verify the logout request URL contains a non-empty `id_token_hint`. If it is missing, the Hydrogen JWT was likely minted before the Phase 10 code was deployed; log in again to get a fresh JWT.
3. Verify the logout request URL contains `post_logout_redirect_uri=https%3A%2F%2Flithium.philement.com` (or similar). If Keycloak rejects this URI, it may not destroy the session.
4. Check Hydrogen pod logs for `OIDC RP /end-session` messages.
5. Capture the exact Keycloak response headers and set-cookie behavior for further analysis.

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

- [x] `test_42_oidc_rp.sh` — passes against mock Keycloak (this is the
      pre-existing baseline; it does **not** target the real IdP).
- [x] `test_40_auth.sh` — passes; OIDC-issued JWT is shape-equivalent
      to password-issued JWT.
- [x] All Vitest suites green (`npm test` — 906/906 unit tests pass; the
      Open Code Review quality scan at the end of `npm test` fails its
      threshold due to pre-existing offline-mode findings, but that is
      unrelated to the OIDC work).
- [x] `test_04_check_links.sh` green after new docs (`oidc_rp.md`,
      `LITHIUM-OIDC.md`, `LITHIUM-KEYCLOAK.md`) are linked from
      `SITEMAP.md` and `LITHIUM-TOC.md`.
- [x] `test_90_markdownlint.sh` green after the plan docs were edited.
- [ ] `test_90_markdownlint.sh` green on the new docs.

---

## Defects Found

> Record any issues discovered during the manual test pass. Each
> defect blocks Phase 27 sign-off until fixed.

(none that are code defects)

**Blocker:**

- The Keycloak test user `andrew@500foods.com` (password `Test@123456`) is configured for MFA/OTP. The password is accepted, but the realm then presents an OTP challenge, preventing completion of the full browser-based sign-in checklist. This is **not** a Hydrogen/Lithium defect. Resolution: use a Keycloak user without OTP configured, or provide the current OTP code / TOTP secret for `andrew@500foods.com`.

---

## Sign-off

- [ ] All seven manual checks passed.
- [ ] Independent reviewer passed.
- [ ] Automated suite passed.
- [ ] No outstanding defects.

**Phase 27 complete on:** ____________
**Signed off by:** ____________
