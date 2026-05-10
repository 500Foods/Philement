# LITHIUM-KEYCLOAK — Implementing OIDC Sign-In in a Hydrogen Client

> **Audience:** Engineers building a new client SPA (similar to Lithium)
> that needs to authenticate users against a Hydrogen backend, where
> Hydrogen federates authentication to Keycloak (or any OIDC-compliant
> Identity Provider).
>
> **Goal:** Provide a recipe-style reference so the same flow Lithium
> implements can be reproduced in another client without re-deriving
> design decisions.
>
> **What this is not:** end-user documentation. For that, see
> [LITHIUM-OIDC.md](LITHIUM-OIDC.md). For Hydrogen-side endpoint
> contracts, see [oidc_rp.md](/docs/H/api/auth/oidc_rp.md). For the
> project plan, see [OIDC-PLAN.md](/docs/OIDC-PLAN.md).

---

## Architectural premise

The single most important thing to internalize before writing any code:

> **The client SPA never sees a Keycloak token.**

The full chain is:

```text
Browser  →  GET /api/auth/oidc/start          (302 to Keycloak)
Keycloak →  user authenticates
Keycloak →  302 to /api/auth/oidc/callback?code=…
Hydrogen →  POST /token  to Keycloak           (server-to-server)
Hydrogen →  validates id_token (signature + claims)
Hydrogen →  resolves account, mints **Hydrogen JWT**
Hydrogen →  302 to client SPA with one-time `?handoff=…`
Client   →  POST /api/auth/oidc/handoff       →  receives Hydrogen JWT
Client   →  storeJWT(token), emit AUTH_LOGIN, render main app
```

The Keycloak `id_token` and `access_token` are server-side artifacts.
Only the opaque, single-use, IP-bound `handoff` code touches the browser,
and even that gets wiped from the URL on arrival.

The **Hydrogen JWT** received via the handoff exchange has the same shape
(claims, signature, lifetime) as the JWT a password login produces. This
means:

- The client's existing JWT-based code path (`storeJWT`, `getJWT`,
  `clearJWT`, renewal logic, logout) does not need to change.
- Every downstream API call, role-gated UI element, and Conduit query
  works identically regardless of how the user signed in.

If you find yourself writing code that handles a Keycloak token in the
client, **stop**. You are off-pattern. The client only ever holds a
Hydrogen JWT.

---

## Prerequisites

Before writing any client code, verify:

1. **Hydrogen is configured as an RP** for at least one provider. The
   `OIDC_RP` block in `hydrogen.json` must have `Enabled: true` and at
   least one entry in `Providers[]`. See
   [oidc_rp.md](/docs/H/api/auth/oidc_rp.md) for the schema.
2. **Keycloak has a client registered** matching the `ClientId` /
   `ClientSecret` / `RedirectUri` in the Hydrogen config.
3. **The redirect URI is exact**. Keycloak performs string equality, not
   prefix matching. If `OIDC_RP.Providers[].RedirectUri =
   "https://app.example.com/api/auth/oidc/callback"`, that exact string
   must be in Keycloak's "Valid redirect URIs" list.
4. **A test user exists** in Keycloak whose email matches an existing
   `accounts` row in the database, so `match_email_only` can succeed.

Hydrogen-side smoke tests:

```bash
curl -i 'https://app.example.com/api/auth/oidc/start?database=Lithium'
# Expect: HTTP/1.1 302 Found
# Location: https://www.500passwords.com/realms/festival/protocol/openid-connect/auth?...

curl -i -X POST 'https://app.example.com/api/auth/oidc/handoff' \
  -H 'Content-Type: application/json' \
  -d '{"handoff":"definitely-not-a-real-code"}'
# Expect: HTTP/1.1 401
# {"error":"handoff_invalid"}
```

If both succeed, the server side is ready. The rest of this document is
about the client side.

---

## Client-side recipe

### Step 1 — Configuration

Add an array of OIDC providers to your client's config file. In Lithium,
this lives in `config/lithium.json` under `auth.oidc_providers`:

```jsonc
{
  "auth": {
    "default_database": "Lithium",
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

The `id` field **must match** the `Name` field of one of the
`OIDC_RP.Providers[]` entries on the Hydrogen side (it becomes the
`?provider=` query parameter sent to `/oidc/start`).

If `oidc_providers` is absent or empty, render no buttons. Per
LITHIUM-INS.md rule #1, never show greyed-out fallback controls.

### Step 2 — Pure client helper

Create a single-purpose, DOM-free, fully unit-testable module that
exposes two functions: `startOidc` and `exchangeHandoff`.

In Lithium this is `src/core/oidc-client.js` (197 lines). The full
implementation is below — it can be copied verbatim with the import paths
adjusted to your project layout.

```js
// src/core/oidc-client.js
import { getConfigValue as _getConfigValue } from './config.js';
import { log, Subsystems, Status } from './log.js';

/**
 * Redirect the browser to /api/auth/oidc/start.
 * Full top-level navigation; an XHR cannot follow a cross-origin 302.
 */
export function startOidc(providerId, returnTo = null, deps = {}) {
  const gcv = deps.getConfigValue ?? _getConfigValue;
  const win = deps.window ?? (typeof window !== 'undefined' ? window : null);

  if (!providerId || typeof providerId !== 'string') {
    throw new Error('startOidc: providerId is required');
  }

  const providers = gcv('auth.oidc_providers', []);
  const provider = Array.isArray(providers)
    ? providers.find(p => p && p.id === providerId)
    : null;

  if (!provider) {
    throw new Error(`startOidc: unknown provider "${providerId}"`);
  }

  const serverUrl = gcv('server.url', '');
  const startPath = provider.start_url ?? '/api/auth/oidc/start';
  const database  = gcv('auth.default_database', '');

  const url = new URL(startPath, serverUrl || undefined);
  if (database)  url.searchParams.set('database',  database);
  if (returnTo && typeof returnTo === 'string' && returnTo.startsWith('/')) {
    url.searchParams.set('return_to', returnTo);
  }

  log(Subsystems.AUTH, Status.INFO, `OIDC start: navigating to "${providerId}"`);
  if (win) win.location.href = url.toString();
  return url.toString();
}

/**
 * Exchange a one-time handoff code for a Hydrogen JWT.
 * Throws structured Error with .code in {handoff_invalid, server_error, network}.
 */
export async function exchangeHandoff(handoff, deps = {}) {
  const gcv     = deps.getConfigValue ?? _getConfigValue;
  const fetchFn = deps.fetch          ?? (typeof fetch !== 'undefined' ? fetch : null);

  if (!handoff || typeof handoff !== 'string') {
    const err = new Error('exchangeHandoff: handoff code is required');
    err.code = 'network';
    throw err;
  }
  if (!fetchFn) {
    const err = new Error('exchangeHandoff: fetch is not available');
    err.code = 'network';
    throw err;
  }

  const url = `${gcv('server.url', '')}/api/auth/oidc/handoff`;

  let response;
  try {
    response = await fetchFn(url, {
      method:  'POST',
      headers: {
        'Content-Type':  'application/json',
        'Accept':        'application/json',
        'Cache-Control': 'no-store',
      },
      credentials: 'omit',
      body: JSON.stringify({ handoff }),
    });
  } catch (fetchErr) {
    const err = new Error(`OIDC handoff exchange failed: ${fetchErr.message}`);
    err.code = 'network';
    err.cause = fetchErr;
    throw err;
  }

  let data = null;
  try {
    if ((response.headers.get('content-type') ?? '').includes('application/json')) {
      data = await response.json();
    }
  } catch { /* best effort */ }

  if (!response.ok) {
    const serverMessage = data?.error ?? data?.message ?? `HTTP ${response.status}`;
    const err = new Error(`OIDC handoff invalid: ${serverMessage}`);
    err.code   = response.status === 401 ? 'handoff_invalid' : 'server_error';
    err.status = response.status;
    err.data   = data;
    throw err;
  }

  return data;
}

export default { startOidc, exchangeHandoff };
```

#### Why this module is the way it is

- **No DOM access.** All reads from `window` are `deps.window ?? window`,
  which lets unit tests inject a mock without jsdom overhead.
- **No `import.meta`, no top-level side effects.** The module is fully
  tree-shakable.
- **`startOidc` is a top-level navigation, not a fetch.** The browser
  must follow a cross-origin 302 to Keycloak; a fetch cannot.
- **`exchangeHandoff` uses `credentials: "omit"`.** We do not use cookies
  for auth; the handoff code in the body is the entire credential.
- **Errors carry a `.code` property.** Three values: `handoff_invalid`
  (401), `server_error` (other non-2xx), `network` (fetch threw or
  preconditions failed). The caller dispatches on `.code`, not
  `.message`.

### Step 3 — Return processor

When the browser comes back with `?oidc=1&handoff=…`, the client must
intercept this **before** rendering the login form, exchange the
handoff, and complete login as if the user had just typed a password.

In Lithium this is `src/managers/login/oidc-login.js` (187 lines). The
key contract:

```js
import { exchangeHandoff } from '../../core/oidc-client.js';
import { storeJWT } from '../../core/jwt.js';
import { eventBus, Events } from '../../core/event-bus.js';
import { log, Subsystems, Status } from '../../core/log.js';

const AUTH = Subsystems.AUTH;

const OIDC_ERROR_MESSAGES = {
  state_invalid:    'Sign-in session expired. Please try again.',
  idp_error:        'The identity provider reported an error.',
  no_account:       'No account is linked to this identity. Contact your administrator.',
  email_ambiguous:  'Multiple accounts share this email. Contact your administrator.',
  server_error:     'An unexpected server error occurred.',
  handoff_invalid:  'Sign-in could not be verified. Please try again.',
  network:          'Could not connect to the server.',
};
const DEFAULT_MSG = 'Sign-in could not complete. Please try again.';

function mapOidcError(code) {
  if (!code) return DEFAULT_MSG;
  if (code in OIDC_ERROR_MESSAGES) return OIDC_ERROR_MESSAGES[code];
  for (const [prefix, msg] of Object.entries(OIDC_ERROR_MESSAGES)) {
    if (code.startsWith(prefix + '_') || code.startsWith(prefix)) return msg;
  }
  return DEFAULT_MSG;
}

/**
 * Call once from your LoginManager.init(), before rendering the form.
 * Returns true if OIDC params were present (handled), false otherwise.
 */
export async function processOidcReturn(loginManager, deps = {}) {
  const win = deps.window ?? (typeof window !== 'undefined' ? window : null);
  if (!win) return false;

  const params = new URLSearchParams(win.location.search);
  if (params.get('oidc') !== '1') return false;          // not our problem

  const handoff   = params.get('handoff');
  const oidcError = params.get('oidc_error');
  const returnTo  = params.get('return_to');

  // Clean the URL IMMEDIATELY — before any async work.
  const cleanUrl = returnTo ? win.location.pathname + returnTo
                            : win.location.pathname;
  win.history.replaceState({}, '', cleanUrl);

  if (oidcError) {
    log(AUTH, Status.WARN, `OIDC return: error "${oidcError}"`);
    loginManager.showError(mapOidcError(oidcError));
    return true;
  }
  if (!handoff) {
    loginManager.showError(DEFAULT_MSG);
    return true;
  }

  const exchangeFn = deps.exchangeHandoff ?? exchangeHandoff;
  const storeFn    = deps.storeJWT        ?? storeJWT;
  const bus        = deps.eventBus        ?? eventBus;

  try {
    const data = await exchangeFn(handoff);
    storeFn(data.token);
    log(AUTH, Status.SUCCESS, `OIDC login: user_id=${data.user_id}`);
    await loginManager.hide();
    bus.emit(Events.AUTH_LOGIN, {
      userId:    data.user_id,
      username:  data.username  ?? '',
      roles:     data.roles     ?? [],
      expiresAt: data.expires_at,
    });
  } catch (err) {
    const code = err.code ?? 'server_error';
    log(AUTH, Status.ERROR, `OIDC handoff failed (${code}): ${err.message}`);
    loginManager.showError(mapOidcError(code));
  }

  return true;
}

export default { processOidcReturn };
```

#### Critical invariants

1. **Wipe the URL first.** The `history.replaceState()` call must happen
   *before* `await exchangeHandoff()`. Otherwise the handoff code is
   visible in `window.location.search` for as long as the network
   request takes — which means a malicious extension could observe it.
2. **Clean the URL on every code path.** Success, error, missing handoff
   — they all wipe the URL. Never leave OIDC params for a later refresh.
3. **Errors map to enumerated user messages.** Never echo the raw server
   `oidc_error` value into the DOM. Use the `OIDC_ERROR_MESSAGES`
   table; fall back to a generic message for unknown codes.
4. **Emit `AUTH_LOGIN` with the same payload shape as password login.**
   This is what makes downstream managers unable to distinguish the two
   methods.

### Step 4 — Wire into the Login Manager

Two integration points in your existing Login Manager:

**(a) On boot, before showing the form:**

```js
// In LoginManager.init() (paraphrased Lithium)
import { processOidcReturn } from './oidc-login.js';

async init() {
  // ... existing setup (panels, language, etc.)

  // Intercept OIDC return BEFORE rendering the form.
  const handled = await processOidcReturn(this);
  if (handled) {
    // The return processor showed an error or completed login.
    // It already called this.hide() on success. Nothing more to do.
    return;
  }

  // Normal path: render the login form.
  this.render();
  this.show();
}
```

**(b) Render provider buttons:**

```js
// In LoginManager.render() or similar
import { startOidc } from '../../core/oidc-client.js';
import { getConfigValue } from '../../core/config.js';

renderOidcProviders() {
  const providers = getConfigValue('auth.oidc_providers', []);
  const mount = this.elements.loginProviders;  // <div id="login-providers">

  if (!Array.isArray(providers) || providers.length === 0) {
    mount.innerHTML = '';
    mount.parentElement.querySelector('.login-divider')?.remove();
    return;
  }

  // Render divider and buttons.
  // ...build buttons with provider.label, provider.icon...
  // Each button:
  button.addEventListener('click', () => {
    const returnTo = this._getReturnTo();
    startOidc(provider.id, returnTo);  // full top-level navigation
  });
}
```

### Step 5 — Settings persistence (optional but recommended)

After a successful login, record the method used so the next visit can
subtly highlight the recently-used control:

```js
// On successful password login
window.lithiumSettings.set('auth.last_method', 'password');

// On successful OIDC login (in processOidcReturn after storeJWT)
window.lithiumSettings.set('auth.last_method', `oidc:${providerId}`);
```

Do **not** use raw `localStorage`; per LITHIUM-INS.md rule #8, settings
go through the project's settings abstraction.

### Step 6 — Tests

Three Vitest files cover the OIDC path in Lithium. Mirror them in your
client:

| File | Purpose |
|---|---|
| `tests/unit/core/oidc-client.test.js` | `startOidc` builds the right URL; `exchangeHandoff` posts JSON, handles 4xx/5xx/network. |
| `tests/unit/managers/login/oidc-login.test.js` | `processOidcReturn` for every branch: no oidc param, oidc_error, missing handoff, success, exchange failure. |
| `tests/unit/managers/login/login.test.js` | Provider button rendered iff `oidc_providers` is non-empty; click calls `startOidc`. |

Mock `fetch`, `storeJWT`, `eventBus.emit`, and a `window` substitute via
the `deps` parameter. Do not depend on jsdom for these tests.

---

## Wire protocol summary

For a SPA implementer who wants to skip everything above and just see
the bytes on the wire, here is the entire contract:

### Outbound: start

```http
GET /api/auth/oidc/start?database=<db>&return_to=<path>&provider=<name> HTTP/1.1
Host: <hydrogen-host>
```

Response: `302 Found` with `Location:` header pointing at the IdP. Follow
the redirect chain by assigning `window.location.href`.

### Inbound: return

```http
GET /<your-login-path>?oidc=1&handoff=<32-byte hex>&return_to=<path> HTTP/1.1
```

(or with `?oidc=1&oidc_error=<code>` on failure).

Detect `oidc=1`, capture `handoff` (or `oidc_error`), then
`history.replaceState()` to wipe the URL.

### Outbound: handoff exchange

```http
POST /api/auth/oidc/handoff HTTP/1.1
Host: <hydrogen-host>
Content-Type: application/json
Cache-Control: no-store

{ "handoff": "<32-byte hex>" }
```

### Inbound: handoff response

Success (200):

```json
{
  "success": true,
  "token": "<hydrogen jwt>",
  "expires_at": "2026-05-10T05:30:00Z",
  "user_id": 123,
  "username": "alice",
  "roles": ["admin", "user"]
}
```

Failure (401 / 400 / 503): `{"error": "<code>"}`.

That is the entire wire protocol for the client side. Three HTTP
interactions: a top-level navigation, a return URL parse, and one POST.

---

## Common mistakes

### Treating the handoff like a Keycloak token

Engineers familiar with raw OIDC sometimes try to validate the handoff,
inspect its claims, or refresh it. **Don't.** The handoff is opaque.
It is a one-time pickup ticket for the real artifact (the Hydrogen JWT)
that lives server-side until the exchange. Treat it as a write-once,
read-once nonce.

### Forgetting `credentials: "omit"`

If you let the browser attach cookies to `/oidc/handoff`, you risk
binding the handoff exchange to a session you don't control. The
handoff code in the JSON body is the only credential needed.

### Using `fetch` for `/oidc/start`

`fetch` cannot follow a cross-origin 302. Use a top-level navigation
(`window.location.href = …`). Lithium learned this the hard way during
Phase 23.

### Logging the handoff code

The handoff code is short-lived and IP-bound, but it is still a bearer
credential while alive. Add `handoff` to your client's log redaction
list. Same for `code`, `state`, `nonce`, `id_token`, `access_token`.
Hydrogen redacts these server-side; the client should follow suit.

### Not cleaning the URL

A page reload with `?handoff=…` still in the URL will trigger a
duplicate exchange attempt against an already-spent handoff. The replay
will return 401 `handoff_invalid` and the user sees a confusing error
on a refresh after a successful login. Always wipe the URL synchronously
before any await.

### Echoing `oidc_error` directly to the DOM

The IdP can produce arbitrary error strings. Map them through an
enumerated table. If a new code arrives that you don't know, fall back
to a generic message. Never `innerHTML = oidcError`.

### Storing the handoff in `localStorage`

It does not need to survive a navigation, and storing it widens the
attack surface for any extension or other tab. Read it once from the
URL, exchange it, and discard.

---

## Adding a second provider

To add a second OIDC provider (e.g. Google or GitHub):

1. **Hydrogen side:** add a new entry to `OIDC_RP.Providers[]` with its
   own `Name`, `ClientId`, `ClientSecret`, `Issuer`, `RedirectUri`.
2. **Client side:** add a new entry to `auth.oidc_providers` in the
   client config with a matching `id`.

That's it. The client code from this guide handles N providers without
modification. No code changes needed to add a third, fourth, etc.

The provider-selection happens in two places:

- The `?provider=<name>` query parameter on `/oidc/start` (Hydrogen
  uses this to pick the right `OIDC_RP.Providers[]` entry).
- The `id` field of the provider in the client config (the client uses
  this to look up the button's `label`, `icon`, `start_url`).

Both must agree.

---

## Migration from a password-only client

If you have an existing password-login client, here is the minimum diff
to add OIDC:

1. Add `auth.oidc_providers` array to the client config (empty is fine).
2. Create `src/core/oidc-client.js` (Step 2 above).
3. Create `src/managers/login/oidc-login.js` (Step 3 above).
4. In your Login Manager's `init()`, add:

   ```js
   const handled = await processOidcReturn(this);
   if (handled) return;
   ```

5. In your Login Manager's render path, add `renderOidcProviders()`
   (Step 4 above).
6. Add a `<div id="login-providers"></div>` to your login HTML.
7. Add minimal CSS for `.login-divider` and `.login-btn-oidc`.
8. Write the three test files (Step 6 above).

No changes to the password-login flow itself. No changes to the
JWT-storage / renewal / logout code. No changes to any other manager.

---

## Verification checklist

Before declaring done, verify:

- [ ] `auth.oidc_providers` empty/missing → no buttons rendered.
- [ ] `auth.oidc_providers` populated → buttons rendered with the right labels.
- [ ] Click a button → browser navigates to `/api/auth/oidc/start?...`,
      then to Keycloak, then back to your client with `?oidc=1&handoff=…`.
- [ ] On return, the URL is cleaned within one event-loop tick.
- [ ] On return success, the JWT is stored and `AUTH_LOGIN` is emitted.
- [ ] On return error, a user-friendly message is shown.
- [ ] After OIDC sign-in, every other API call works (renew, logout,
      role-gated UI).
- [ ] Refreshing the page after sign-in does not trigger a duplicate
      handoff exchange (URL was cleaned).
- [ ] A spent / expired handoff returns a clean error.
- [ ] Disabling the feature in config (empty providers array OR
      `OIDC_RP.Enabled = false`) hides the OIDC path entirely.

---

## Related documents

- [LITHIUM-OIDC.md](LITHIUM-OIDC.md) — User/operator-facing OIDC docs.
- [LITHIUM-MGR-LOGIN.md](LITHIUM-MGR-LOGIN.md) — Login Manager overview.
- [LITHIUM-JWT.md](LITHIUM-JWT.md) — JWT lifecycle.
- [LITHIUM-CFG.md](LITHIUM-CFG.md) — `lithium.json` reference.
- [LITHIUM-INS.md](LITHIUM-INS.md) — Mandatory coding standards.
- [LITHIUM-TST.md](LITHIUM-TST.md) — Test framework.
- [oidc_rp.md](/docs/H/api/auth/oidc_rp.md) — Hydrogen RP endpoint contracts.
- [OIDC-PLAN.md](/docs/OIDC-PLAN.md) — Project plan.
