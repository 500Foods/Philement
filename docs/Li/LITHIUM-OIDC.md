# LITHIUM-OIDC — OpenID Connect Sign-In

> **For implementers building a new client SPA** that needs to integrate
> with Hydrogen + Keycloak, see [LITHIUM-KEYCLOAK.md](LITHIUM-KEYCLOAK.md)
> for a recipe-style guide.
>
> **For Hydrogen-side endpoint contracts**, see
> [oidc_rp.md](/docs/H/api/auth/oidc_rp.md).
>
> **For project architecture and rationale**, see
> [OIDC-PLAN.md](/docs/OIDC-PLAN.md).

---

## What this is

Lithium supports two ways to sign in:

1. **Username / password** — the original flow, against the Hydrogen-managed
   `accounts` table.
2. **OpenID Connect via Keycloak** — added in Phases 5–27 of the OIDC plan.
   Click "Sign in with 500 Passwords", authenticate at Keycloak, return to
   Lithium signed in.

Both paths produce **the same Hydrogen JWT**. Once signed in, every other
manager (Queries, Lookups, Style, Crimson, Tour, etc.) cannot tell the
difference between the two methods. Renew, logout, role-gated UI — all
work identically.

---

## End-user flow

```text
1. User opens Lithium.
2. Login Manager renders both the password form AND a "Sign in with 500 Passwords" button.
3. User clicks the button.
4. Browser redirects: Lithium → Hydrogen → Keycloak (full top-level navigation).
5. User authenticates at Keycloak.
6. Browser redirects: Keycloak → Hydrogen → Lithium (with a one-time `?handoff=…`).
7. Lithium's oidc-login.js intercepts on boot, exchanges the handoff for a JWT,
   stores it, and emits AUTH_LOGIN. The URL is cleaned via history.replaceState.
8. The main app renders as if the user had just logged in with password.
```

The Keycloak `id_token` and `access_token` never reach the browser. Only
the opaque, single-use `handoff` code does, and it is wiped from the URL
on arrival.

---

## Configuration

### `config/lithium.json`

```jsonc
{
  "auth": {
    "api_key": "…",
    "default_database": "Lithium",
    "session_timeout_minutes": 30,

    "oidc_providers": [
      {
        "id": "500passwords",
        "label": "Sign in with 500 Passwords",
        "icon": "fa-key",
        "start_url": "/api/auth/oidc/start"
      }
    ]
  }
}
```

| Field | Required | Description |
|---|---|---|
| `id` | yes | Stable identifier. Must match `OIDC_RP.Providers[].Name` on the Hydrogen side (the `?provider=` query param sent to `/oidc/start`). |
| `label` | yes | Button text. |
| `icon` | optional | Font Awesome icon name (e.g. `fa-key`, `fa-google`, `fa-microsoft`). |
| `start_url` | optional | Override the default `/api/auth/oidc/start` path. Useful in tests. |

If `auth.oidc_providers` is absent or empty, **no OIDC button is rendered**.
Per LITHIUM-INS.md rule #1, we never show a greyed-out fallback.

Multiple providers are supported. Each entry produces one button stacked
vertically in the login panel.

---

## File structure

| File | Purpose |
|---|---|
| `src/core/oidc-client.js` | Pure helpers: `startOidc(provider, returnTo)`, `exchangeHandoff(handoff)`. No DOM access. Fully unit-testable. |
| `src/managers/login/oidc-login.js` | `processOidcReturn(loginManager)` — runs during Login Manager `init()`, detects `?oidc=1&handoff=…`, exchanges, stores JWT, emits `AUTH_LOGIN`. |
| `src/managers/login/login.js` | Renders provider buttons via `renderOidcProviders()`. Wires button clicks to `startOidc()`. Calls `processOidcReturn()` during init. |
| `src/managers/login/login.html` | Contains the `<div id="login-providers">` mount point. |
| `src/managers/login/login.css` | Styles the `.login-divider` and `.login-btn-oidc` controls. |
| `tests/unit/core/oidc-client.test.js` | Unit tests for the pure helpers. |
| `tests/unit/managers/login/oidc-login.test.js` | Unit tests for the return processor (mocked fetch + storeJWT + eventBus). |

---

## Click flow (start)

```js
// On button click in login.js
import { startOidc } from '../../core/oidc-client.js';

button.addEventListener('click', () => {
  startOidc(providerId, currentReturnTo);  // full top-level navigation
});
```

`startOidc()` builds:

```text
${server.url}/api/auth/oidc/start?database=${default_database}&return_to=${path}
```

This is a `window.location.href` assignment, **not** a fetch. We need the
browser to follow Hydrogen's 302 to Keycloak naturally, which fetch cannot
do across origins.

---

## Return flow

When the browser comes back to Lithium with `?oidc=1&handoff=…` (or
`?oidc_error=…`), the Login Manager's `init()` calls `processOidcReturn()`:

```js
// In oidc-login.js (paraphrased)
export async function processOidcReturn(loginManager) {
  const params = new URLSearchParams(window.location.search);
  if (params.get('oidc') !== '1') return false;

  const handoff = params.get('handoff');
  const oidcError = params.get('oidc_error');

  // Always clean the URL immediately.
  const cleanUrl = window.location.pathname + (params.get('return_to') || '');
  window.history.replaceState({}, '', cleanUrl);

  if (oidcError) {
    loginManager.showError(mapOidcError(oidcError));
    return true;
  }

  if (!handoff) {
    loginManager.showError('Sign-in did not complete.');
    return true;
  }

  try {
    const data = await exchangeHandoff(handoff);
    storeJWT(data.token);
    await loginManager.hide();
    eventBus.emit(Events.AUTH_LOGIN, {
      userId: data.user_id,
      username: data.username,
      roles: data.roles || [],
      expiresAt: data.expires_at,
    });
    window.lithiumSettings.set('auth.last_method', `oidc:${providerId}`);
    return true;
  } catch (err) {
    loginManager.showError('Sign-in could not complete.');
    return true;
  }
}
```

The path mirrors the password `attemptLogin` success path **exactly**, so
every other manager downstream cannot tell the difference between the two
login methods.

---

## URL hygiene

- The handoff code is wiped from the URL via `history.replaceState()`
  immediately after being read. **It is not put into `localStorage`.**
- The handoff exchange uses `fetch` with `credentials: "omit"` (we do not
  use cookies for auth) and `Cache-Control: no-store`.
- `oidc_error` codes are an enumerated set, mapped client-side to
  user-friendly messages. We do not echo IdP error strings into the DOM.

### Error codes from Hydrogen

| `oidc_error` | Lithium message |
|---|---|
| `state_invalid` | "Sign-in session expired or was tampered with. Please try again." |
| `idp_error` | "The identity provider reported an error. Please try again." |
| `token_invalid_grant` | "Sign-in failed. Please try again." |
| `token_invalid_client` | "Sign-in failed. Please try again." |
| `token_server_error` | "Sign-in failed. Please try again." |
| `id_token_invalid` | "Sign-in failed. Please try again." |
| `account_not_found` | "No account is linked to this identity." |
| `email_ambiguous` | "Multiple accounts share this email. Contact your administrator." |
| `account_disabled` | "Your account has been disabled. Contact an administrator." |
| `provisioning_blocked` | "Account creation is not enabled. Contact an administrator." |
| `provision_disallowed_email` | "Your email domain is not allowed. Contact your administrator." |
| `email_not_verified` | "Please verify your email at the identity provider first." |
| `no_api_key` | "Server configuration error. Contact your administrator." |
| `server_error` / `internal_error` | "Sign-in failed. Please try again." |

---

## Settings persistence

After a successful login of either type, Lithium records:

```js
window.lithiumSettings.set('auth.last_method', 'password');           // or
window.lithiumSettings.set('auth.last_method', 'oidc:500passwords');
```

On the next visit, the matching control gets a `.is-recent` CSS class for
subtle highlighting (see Phase 26). Per LITHIUM-INS.md rule #8, we use
`lithiumSettings`, not raw `localStorage`.

---

## Operational scenarios

### A user who has both a password account and a 500passwords account

With `Strategy: "match_email_only"` (the production default), Hydrogen
looks up `(iss, sub)` first. If unfound, it falls back to email lookup.
The first time the user signs in via OIDC, an `account_oidc_identities`
row is inserted linking their existing account to their Keycloak
`(iss, sub)`. From then on, they can use either method interchangeably.

### A new 500passwords user with no Lithium account

With `ProvisionDefaults.Enabled = false` (the production default), the
sign-in fails with `oidc_error=account_not_found`. The user sees:

> "No account is linked to this identity."

An administrator must create the Lithium account first. To enable
auto-provisioning, set `ProvisionDefaults.Enabled = true` per environment.

### IdP outage

Password login is unaffected. The OIDC button still renders, but clicking
it will result in `oidc_error=token_exchange_failed` after the IdP times
out. The user is told to try again later.

### Disabling OIDC

Set `Enabled = false` in `OIDC_RP` (Hydrogen-side) **or** empty
`auth.oidc_providers` (Lithium-side). Either path removes OIDC from the
running system without affecting password login.

---

## Tests

Vitest tests covering the OIDC path (per LITHIUM-TST.md patterns):

- `tests/unit/core/oidc-client.test.js`
  - `startOidc()` builds the right URL with all params, looks up the
    provider, throws on unknown id.
  - `exchangeHandoff()` posts JSON, handles 4xx (401 / 5xx / network),
    parses success response.
- `tests/unit/managers/login/oidc-login.test.js`
  - `processOidcReturn()` for every branch: no oidc param, `oidc_error`,
    missing handoff, success, exchange failure.
  - Mocks `fetch`, `storeJWT`, `eventBus.emit`.
- `tests/unit/managers/login/login.test.js`
  - Provider button rendered iff `auth.oidc_providers` is non-empty.
  - Button click calls `startOidc(providerId, returnTo)`.

---

## Related documents

- [LITHIUM-KEYCLOAK.md](LITHIUM-KEYCLOAK.md) — implementer reference for new client SPAs.
- [LITHIUM-MGR-LOGIN.md](LITHIUM-MGR-LOGIN.md) — Login Manager overview.
- [LITHIUM-JWT.md](LITHIUM-JWT.md) — JWT lifecycle (same JWT for both methods).
- [LITHIUM-CFG.md](LITHIUM-CFG.md) — `lithium.json` reference.
- [oidc_rp.md](/docs/H/api/auth/oidc_rp.md) — Hydrogen RP endpoint contracts.
- [OIDC-PLAN.md](/docs/OIDC-PLAN.md) — Project plan and phase log.
