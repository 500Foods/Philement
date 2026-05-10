/**
 * OIDC Login — return-from-IdP processor
 *
 * Detects and handles the OIDC return leg: when Lithium boots with
 * `?oidc=1&handoff=<code>` (or `?oidc_error=<code>`) in the URL, this
 * module exchanges the handoff code for a Hydrogen JWT and completes login
 * exactly as if a password login had succeeded.
 *
 * Entry point: `processOidcReturn(loginManager)` — call once from
 * `LoginManager.init()`, before the login form is shown.
 *
 * Return value:
 *   true   — the URL contained OIDC parameters; caller should skip showing
 *            the normal login form immediately (we handled it).
 *   false  — URL had no OIDC parameters; nothing was done; caller proceeds
 *            normally.
 *
 * On success the function:
 *   1. Wipes `?oidc=1&handoff=…` from the URL via `history.replaceState`
 *      so the code is never visible in browser history.
 *   2. Stores the Hydrogen JWT via `storeJWT`.
 *   3. Hides the login UI.
 *   4. Emits `AUTH_LOGIN` with the same payload shape as password login.
 *
 * On error (oidc_error param or failed exchange):
 *   1. Wipes OIDC params from the URL.
 *   2. Displays a user-friendly, non-leaky error message via
 *      `loginManager.showError()`.
 *   3. Returns true so the caller still shows the login form.
 *
 * Security:
 *   - The URL is cleaned *immediately*, before any async work, so the
 *     handoff code is not visible for more than one event-loop tick.
 *   - Error codes from the server are mapped to enumerated user-facing
 *     strings; the raw server value is never echoed into the DOM.
 */

import { exchangeHandoff } from '../../core/oidc-client.js';
import { storeJWT } from '../../core/jwt.js';
import { eventBus, Events } from '../../core/event-bus.js';
import { log, Subsystems, Status } from '../../core/log.js';

const AUTH = Subsystems.AUTH;

// ---------------------------------------------------------------------------
// User-facing error messages (never echo the raw server code to the DOM)
// ---------------------------------------------------------------------------

const OIDC_ERROR_MESSAGES = {
  // Errors forwarded from the IdP via ?oidc_error= on /callback
  state_invalid:              'Sign-in session expired or was tampered with. Please try again.',
  idp_error:                  'The identity provider reported an error. Please try again.',
  no_account:                 'No account is linked to this identity. Contact your administrator.',
  no_api_key:                 'Server configuration error. Contact your administrator.',
  email_ambiguous:            'Multiple accounts share this email address. Contact your administrator.',
  provision_disallowed_email: 'Your email domain is not allowed. Contact your administrator.',
  server_error:               'An unexpected server error occurred. Please try again later.',

  // Errors produced locally by exchangeHandoff
  handoff_invalid:            'Sign-in could not be verified. Please try again.',
  network:                    'Could not connect to the server. Check your connection and try again.',
};

const DEFAULT_OIDC_ERROR_MESSAGE =
  'Sign-in could not complete. Please try again.';

/**
 * Map a raw oidc_error code to a user-facing message.
 * Accepts both exact matches and prefix matches (e.g. "token_invalid_grant").
 * @param {string} code
 * @returns {string}
 */
function mapOidcError(code) {
  if (!code) return DEFAULT_OIDC_ERROR_MESSAGE;

  // Exact match first.
  if (code in OIDC_ERROR_MESSAGES) {
    return OIDC_ERROR_MESSAGES[code];
  }

  // Prefix match for compound codes like "token_invalid_grant",
  // "id_token_expired", etc.
  for (const [prefix, message] of Object.entries(OIDC_ERROR_MESSAGES)) {
    if (code.startsWith(prefix + '_') || code.startsWith(prefix)) {
      return message;
    }
  }

  return DEFAULT_OIDC_ERROR_MESSAGE;
}

// ---------------------------------------------------------------------------
// processOidcReturn
// ---------------------------------------------------------------------------

/**
 * Process an OIDC return-from-IdP URL.
 *
 * @param {Object} loginManager - The LoginManager instance. Must expose:
 *   - `showError(message)` — display an error in the login form.
 *   - `hide()` — hide the login UI (async, awaited on success).
 * @param {Object} [deps] - Dependency injection for tests:
 *   - `deps.exchangeHandoff(code)` — override handoff exchange
 *   - `deps.storeJWT(token)`       — override JWT storage
 *   - `deps.eventBus`              — override event bus
 *   - `deps.window`                — override window (for URL / history)
 * @returns {Promise<boolean>} true if OIDC params were present, false otherwise.
 */
export async function processOidcReturn(loginManager, deps = {}) {
  const win = deps.window ?? (typeof window !== 'undefined' ? window : null);

  if (!win) return false;

  const params = new URLSearchParams(win.location.search);

  // Fast exit — no OIDC involvement.
  if (params.get('oidc') !== '1') return false;

  const handoff   = params.get('handoff');
  const oidcError = params.get('oidc_error');
  const returnTo  = params.get('return_to');

  // Wipe OIDC params from the URL immediately — before any async work.
  // Keep return_to if it is present so navigation after login works.
  const cleanUrl = returnTo ? win.location.pathname + returnTo
                            : win.location.pathname;
  win.history.replaceState({}, '', cleanUrl);

  // --- Error path: IdP or callback reported an error ---
  if (oidcError) {
    log(AUTH, Status.WARN, `OIDC return: error code "${oidcError}"`);
    loginManager.showError(mapOidcError(oidcError));
    return true;
  }

  // --- Missing handoff code after no error flag ---
  if (!handoff) {
    log(AUTH, Status.WARN, 'OIDC return: missing handoff code');
    loginManager.showError(DEFAULT_OIDC_ERROR_MESSAGE);
    return true;
  }

  // --- Success path: exchange the handoff code ---
  const exchangeFn  = deps.exchangeHandoff ?? exchangeHandoff;
  const storeJWTFn  = deps.storeJWT       ?? storeJWT;
  const bus         = deps.eventBus       ?? eventBus;

  try {
    const data = await exchangeFn(handoff);

    storeJWTFn(data.token);

    log(AUTH, Status.SUCCESS,
      `OIDC login successful: user_id=${data.user_id}, username="${data.username ?? ''}"`);

    await loginManager.hide();

    bus.emit(Events.AUTH_LOGIN, {
      userId:    data.user_id,
      username:  data.username  ?? '',
      roles:     data.roles     ?? [],
      expiresAt: data.expires_at,
    });
  } catch (err) {
    const code = err.code ?? 'server_error';
    log(AUTH, Status.ERROR,
      `OIDC handoff exchange failed (${code}): ${err.message}`);
    loginManager.showError(mapOidcError(code));
  }

  return true;
}

// ---------------------------------------------------------------------------
// Default export (convenience)
// ---------------------------------------------------------------------------

export default { processOidcReturn };
