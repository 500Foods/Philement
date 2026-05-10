/**
 * OIDC Login Unit Tests
 *
 * Tests for processOidcReturn() in src/managers/login/oidc-login.js.
 *
 * All side-effects (exchangeHandoff, storeJWT, eventBus, window/history)
 * are injected via the `deps` parameter so no real network or DOM is needed.
 */

import { describe, it, expect, vi, beforeEach } from 'vitest';
import { processOidcReturn } from '../../../../src/managers/login/oidc-login.js';

// ---------------------------------------------------------------------------
// Module-level mocks
// ---------------------------------------------------------------------------

vi.mock('../../../../src/core/oidc-client.js', () => ({
  exchangeHandoff: vi.fn(),
}));

vi.mock('../../../../src/core/jwt.js', () => ({
  storeJWT: vi.fn(),
}));

vi.mock('../../../../src/core/event-bus.js', () => ({
  eventBus: { emit: vi.fn() },
  Events: { AUTH_LOGIN: 'auth:login' },
}));

vi.mock('../../../../src/core/log.js', () => ({
  log: vi.fn(),
  Subsystems: { AUTH: 'Auth' },
  Status: { INFO: 'INFO', WARN: 'WARN', ERROR: 'ERROR', SUCCESS: 'SUCCESS' },
}));

// ---------------------------------------------------------------------------
// Test helpers
// ---------------------------------------------------------------------------

/** Build a minimal loginManager stub. */
function makeLoginManager() {
  return {
    showError: vi.fn(),
    hide:      vi.fn().mockResolvedValue(undefined),
  };
}

/** Build a window stub with a controllable search string and history spy. */
function makeWindow(search = '') {
  return {
    location: {
      search,
      pathname: '/login',
    },
    history: {
      replaceState: vi.fn(),
    },
  };
}

/** Build a successful exchangeHandoff stub. */
function makeExchangeSuccess(overrides = {}) {
  return vi.fn().mockResolvedValue({
    success:    true,
    token:      'header.payload.sig',
    expires_at: '2027-01-01T00:00:00Z',
    user_id:    42,
    username:   'testuser',
    roles:      ['user'],
    ...overrides,
  });
}

/** Build an eventBus stub with a spy. */
function makeEventBus() {
  return { emit: vi.fn() };
}

// ---------------------------------------------------------------------------
// No OIDC params
// ---------------------------------------------------------------------------

describe('processOidcReturn — no OIDC params', () => {
  it('returns false when oidc param is absent', async () => {
    const lm  = makeLoginManager();
    const win = makeWindow('?foo=bar');

    const result = await processOidcReturn(lm, { window: win });

    expect(result).toBe(false);
    expect(lm.showError).not.toHaveBeenCalled();
    expect(win.history.replaceState).not.toHaveBeenCalled();
  });

  it('returns false when query string is empty', async () => {
    const lm  = makeLoginManager();
    const win = makeWindow('');

    const result = await processOidcReturn(lm, { window: win });

    expect(result).toBe(false);
  });

  it('returns false when oidc param is not "1"', async () => {
    const lm  = makeLoginManager();
    const win = makeWindow('?oidc=0&handoff=abc');

    const result = await processOidcReturn(lm, { window: win });

    expect(result).toBe(false);
  });

  it('returns false when window is null', async () => {
    const lm = makeLoginManager();

    const result = await processOidcReturn(lm, { window: null });

    expect(result).toBe(false);
  });
});

// ---------------------------------------------------------------------------
// URL cleaning
// ---------------------------------------------------------------------------

describe('processOidcReturn — URL cleaning', () => {
  it('wipes OIDC params from the URL before any async work', async () => {
    const lm       = makeLoginManager();
    const win      = makeWindow('?oidc=1&handoff=abc123');
    const exchange = makeExchangeSuccess();
    const store    = vi.fn();
    const bus      = makeEventBus();

    await processOidcReturn(lm, {
      window: win, exchangeHandoff: exchange, storeJWT: store, eventBus: bus,
    });

    expect(win.history.replaceState).toHaveBeenCalledOnce();
    const [, , cleanUrl] = win.history.replaceState.mock.calls[0];
    expect(cleanUrl).not.toContain('oidc=1');
    expect(cleanUrl).not.toContain('handoff=');
  });

  it('cleans the URL even on oidc_error', async () => {
    const lm  = makeLoginManager();
    const win = makeWindow('?oidc=1&oidc_error=state_invalid');

    await processOidcReturn(lm, { window: win });

    expect(win.history.replaceState).toHaveBeenCalledOnce();
    const [, , cleanUrl] = win.history.replaceState.mock.calls[0];
    expect(cleanUrl).not.toContain('oidc_error=');
  });

  it('preserves return_to in the clean URL', async () => {
    const lm       = makeLoginManager();
    const win      = makeWindow('?oidc=1&handoff=abc&return_to=%2Fqueries');
    const exchange = makeExchangeSuccess();
    const store    = vi.fn();
    const bus      = makeEventBus();

    await processOidcReturn(lm, {
      window: win, exchangeHandoff: exchange, storeJWT: store, eventBus: bus,
    });

    const [, , cleanUrl] = win.history.replaceState.mock.calls[0];
    expect(cleanUrl).toContain('/queries');
    expect(cleanUrl).not.toContain('handoff=');
  });
});

// ---------------------------------------------------------------------------
// oidc_error param
// ---------------------------------------------------------------------------

describe('processOidcReturn — oidc_error param', () => {
  it('returns true when oidc_error is present', async () => {
    const lm  = makeLoginManager();
    const win = makeWindow('?oidc=1&oidc_error=state_invalid');

    const result = await processOidcReturn(lm, { window: win });

    expect(result).toBe(true);
  });

  it('calls showError with a user-friendly message for state_invalid', async () => {
    const lm  = makeLoginManager();
    const win = makeWindow('?oidc=1&oidc_error=state_invalid');

    await processOidcReturn(lm, { window: win });

    expect(lm.showError).toHaveBeenCalledOnce();
    expect(lm.showError.mock.calls[0][0]).toMatch(/session expired/i);
  });

  it('calls showError with a user-friendly message for no_account', async () => {
    const lm  = makeLoginManager();
    const win = makeWindow('?oidc=1&oidc_error=no_account');

    await processOidcReturn(lm, { window: win });

    expect(lm.showError.mock.calls[0][0]).toMatch(/no account/i);
  });

  it('calls showError with a user-friendly message for email_ambiguous', async () => {
    const lm  = makeLoginManager();
    const win = makeWindow('?oidc=1&oidc_error=email_ambiguous');

    await processOidcReturn(lm, { window: win });

    expect(lm.showError.mock.calls[0][0]).toMatch(/multiple accounts/i);
  });

  it('calls showError with a user-friendly message for provision_disallowed_email', async () => {
    const lm  = makeLoginManager();
    const win = makeWindow('?oidc=1&oidc_error=provision_disallowed_email');

    await processOidcReturn(lm, { window: win });

    expect(lm.showError.mock.calls[0][0]).toMatch(/email domain/i);
  });

  it('calls showError with a fallback message for unknown error codes', async () => {
    const lm  = makeLoginManager();
    const win = makeWindow('?oidc=1&oidc_error=xyzzy_unknown_code');

    await processOidcReturn(lm, { window: win });

    expect(lm.showError).toHaveBeenCalledOnce();
    // Should still be a friendly string, not the raw code.
    expect(lm.showError.mock.calls[0][0]).not.toContain('xyzzy_unknown_code');
  });

  it('calls showError with a compound code like token_invalid_grant', async () => {
    const lm  = makeLoginManager();
    const win = makeWindow('?oidc=1&oidc_error=token_invalid_grant');

    await processOidcReturn(lm, { window: win });

    // Should not echo the raw code.
    expect(lm.showError.mock.calls[0][0]).not.toContain('token_invalid_grant');
    expect(typeof lm.showError.mock.calls[0][0]).toBe('string');
  });

  it('does not call exchangeHandoff when oidc_error is present', async () => {
    const lm       = makeLoginManager();
    const win      = makeWindow('?oidc=1&oidc_error=state_invalid');
    const exchange = vi.fn();

    await processOidcReturn(lm, { window: win, exchangeHandoff: exchange });

    expect(exchange).not.toHaveBeenCalled();
  });
});

// ---------------------------------------------------------------------------
// Missing handoff code
// ---------------------------------------------------------------------------

describe('processOidcReturn — missing handoff', () => {
  it('returns true and shows error when handoff is missing', async () => {
    const lm  = makeLoginManager();
    const win = makeWindow('?oidc=1');

    const result = await processOidcReturn(lm, { window: win });

    expect(result).toBe(true);
    expect(lm.showError).toHaveBeenCalledOnce();
  });

  it('does not call exchangeHandoff when handoff is absent', async () => {
    const lm       = makeLoginManager();
    const win      = makeWindow('?oidc=1');
    const exchange = vi.fn();

    await processOidcReturn(lm, { window: win, exchangeHandoff: exchange });

    expect(exchange).not.toHaveBeenCalled();
  });
});

// ---------------------------------------------------------------------------
// Successful handoff exchange
// ---------------------------------------------------------------------------

describe('processOidcReturn — successful exchange', () => {
  it('returns true on success', async () => {
    const lm       = makeLoginManager();
    const win      = makeWindow('?oidc=1&handoff=abc123');
    const exchange = makeExchangeSuccess();
    const store    = vi.fn();
    const bus      = makeEventBus();

    const result = await processOidcReturn(lm, {
      window: win, exchangeHandoff: exchange, storeJWT: store, eventBus: bus,
    });

    expect(result).toBe(true);
  });

  it('calls exchangeHandoff with the handoff code', async () => {
    const lm       = makeLoginManager();
    const win      = makeWindow('?oidc=1&handoff=abc123');
    const exchange = makeExchangeSuccess();
    const store    = vi.fn();
    const bus      = makeEventBus();

    await processOidcReturn(lm, {
      window: win, exchangeHandoff: exchange, storeJWT: store, eventBus: bus,
    });

    expect(exchange).toHaveBeenCalledOnce();
    expect(exchange).toHaveBeenCalledWith('abc123');
  });

  it('calls storeJWT with the token', async () => {
    const lm       = makeLoginManager();
    const win      = makeWindow('?oidc=1&handoff=abc123');
    const exchange = makeExchangeSuccess({ token: 'the.real.token' });
    const store    = vi.fn();
    const bus      = makeEventBus();

    await processOidcReturn(lm, {
      window: win, exchangeHandoff: exchange, storeJWT: store, eventBus: bus,
    });

    expect(store).toHaveBeenCalledOnce();
    expect(store).toHaveBeenCalledWith('the.real.token');
  });

  it('calls loginManager.hide() after storing the JWT', async () => {
    const lm       = makeLoginManager();
    const win      = makeWindow('?oidc=1&handoff=abc123');
    const exchange = makeExchangeSuccess();
    const store    = vi.fn();
    const bus      = makeEventBus();

    await processOidcReturn(lm, {
      window: win, exchangeHandoff: exchange, storeJWT: store, eventBus: bus,
    });

    expect(lm.hide).toHaveBeenCalledOnce();
  });

  it('emits AUTH_LOGIN with the correct payload', async () => {
    const lm       = makeLoginManager();
    const win      = makeWindow('?oidc=1&handoff=abc123');
    const exchange = makeExchangeSuccess({
      user_id:    99,
      username:   'oidcuser',
      roles:      ['admin', 'user'],
      expires_at: '2027-06-01T00:00:00Z',
    });
    const store = vi.fn();
    const bus   = makeEventBus();

    await processOidcReturn(lm, {
      window: win, exchangeHandoff: exchange, storeJWT: store, eventBus: bus,
    });

    expect(bus.emit).toHaveBeenCalledOnce();
    const [eventName, payload] = bus.emit.mock.calls[0];
    expect(eventName).toBe('auth:login');
    expect(payload.userId).toBe(99);
    expect(payload.username).toBe('oidcuser');
    expect(payload.roles).toEqual(['admin', 'user']);
    expect(payload.expiresAt).toBe('2027-06-01T00:00:00Z');
  });

  it('does not call showError on success', async () => {
    const lm       = makeLoginManager();
    const win      = makeWindow('?oidc=1&handoff=abc123');
    const exchange = makeExchangeSuccess();
    const store    = vi.fn();
    const bus      = makeEventBus();

    await processOidcReturn(lm, {
      window: win, exchangeHandoff: exchange, storeJWT: store, eventBus: bus,
    });

    expect(lm.showError).not.toHaveBeenCalled();
  });
});

// ---------------------------------------------------------------------------
// Failed handoff exchange
// ---------------------------------------------------------------------------

describe('processOidcReturn — exchange failure', () => {
  it('returns true even when exchange fails', async () => {
    const lm  = makeLoginManager();
    const win = makeWindow('?oidc=1&handoff=bad');
    const err = Object.assign(new Error('handoff expired'), { code: 'handoff_invalid' });
    const exchange = vi.fn().mockRejectedValue(err);

    const result = await processOidcReturn(lm, {
      window: win, exchangeHandoff: exchange,
    });

    expect(result).toBe(true);
  });

  it('shows a friendly error on handoff_invalid', async () => {
    const lm  = makeLoginManager();
    const win = makeWindow('?oidc=1&handoff=bad');
    const err = Object.assign(new Error('handoff expired'), { code: 'handoff_invalid' });
    const exchange = vi.fn().mockRejectedValue(err);

    await processOidcReturn(lm, { window: win, exchangeHandoff: exchange });

    expect(lm.showError).toHaveBeenCalledOnce();
    expect(lm.showError.mock.calls[0][0]).toMatch(/could not be verified/i);
  });

  it('shows a friendly error on network failure', async () => {
    const lm  = makeLoginManager();
    const win = makeWindow('?oidc=1&handoff=x');
    const err = Object.assign(new Error('Failed to fetch'), { code: 'network' });
    const exchange = vi.fn().mockRejectedValue(err);

    await processOidcReturn(lm, { window: win, exchangeHandoff: exchange });

    expect(lm.showError.mock.calls[0][0]).toMatch(/connect/i);
  });

  it('does not emit AUTH_LOGIN on exchange failure', async () => {
    const lm  = makeLoginManager();
    const win = makeWindow('?oidc=1&handoff=bad');
    const err = Object.assign(new Error('oops'), { code: 'server_error' });
    const exchange = vi.fn().mockRejectedValue(err);
    const bus = makeEventBus();

    await processOidcReturn(lm, {
      window: win, exchangeHandoff: exchange, eventBus: bus,
    });

    expect(bus.emit).not.toHaveBeenCalled();
  });

  it('does not call loginManager.hide() on exchange failure', async () => {
    const lm  = makeLoginManager();
    const win = makeWindow('?oidc=1&handoff=bad');
    const err = Object.assign(new Error('oops'), { code: 'server_error' });
    const exchange = vi.fn().mockRejectedValue(err);

    await processOidcReturn(lm, { window: win, exchangeHandoff: exchange });

    expect(lm.hide).not.toHaveBeenCalled();
  });
});

// ---------------------------------------------------------------------------
// auth.last_method — Phase 26
// ---------------------------------------------------------------------------

describe('processOidcReturn — auth.last_method (Phase 26)', () => {
  it('sets auth.last_method to "oidc" on successful exchange', async () => {
    const lm       = makeLoginManager();
    const win      = makeWindow('?oidc=1&handoff=abc123');
    const exchange = makeExchangeSuccess();
    const store    = vi.fn();
    const bus      = makeEventBus();
    const settings = { set: vi.fn() };

    await processOidcReturn(lm, {
      window: win, exchangeHandoff: exchange, storeJWT: store,
      eventBus: bus, lithiumSettings: settings,
    });

    expect(settings.set).toHaveBeenCalledWith('auth.last_method', 'oidc');
  });

  it('does not set auth.last_method when exchange fails', async () => {
    const lm       = makeLoginManager();
    const win      = makeWindow('?oidc=1&handoff=bad');
    const err      = Object.assign(new Error('handoff expired'), { code: 'handoff_invalid' });
    const exchange = vi.fn().mockRejectedValue(err);
    const settings = { set: vi.fn() };

    await processOidcReturn(lm, {
      window: win, exchangeHandoff: exchange, lithiumSettings: settings,
    });

    expect(settings.set).not.toHaveBeenCalledWith('auth.last_method', 'oidc');
  });

  it('does not set auth.last_method when oidc_error is present', async () => {
    const lm       = makeLoginManager();
    const win      = makeWindow('?oidc=1&oidc_error=state_invalid');
    const settings = { set: vi.fn() };

    await processOidcReturn(lm, { window: win, lithiumSettings: settings });

    expect(settings.set).not.toHaveBeenCalled();
  });

  it('does not throw when lithiumSettings is absent', async () => {
    const lm       = makeLoginManager();
    const win      = makeWindow('?oidc=1&handoff=abc123');
    const exchange = makeExchangeSuccess();
    const store    = vi.fn();
    const bus      = makeEventBus();

    await expect(
      processOidcReturn(lm, {
        window: win, exchangeHandoff: exchange, storeJWT: store, eventBus: bus,
        lithiumSettings: null,
      })
    ).resolves.toBe(true);
  });
});
