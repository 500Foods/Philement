/**
 * LoginManager — Phase 25 unit tests
 *
 * Tests for the OIDC provider button rendering surface introduced in Phase 25:
 *   - `renderOidcProviders(deps)` — config-driven button generation
 *   - `showError(message)` — proxy to LoginForm
 *
 * LoginManager is not instantiated directly here because it relies on
 * DOM fetch + a full HTML template. Instead we test `renderOidcProviders`
 * in isolation by calling it on a thin stub that mimics the relevant
 * `this.elements` shape and the methods under test.
 *
 * All config access, startOidc, and window are injected via the `deps`
 * parameter, matching the same dependency-injection pattern used across
 * Phases 23 and 24.
 */

import { describe, it, expect, vi, beforeEach } from 'vitest';

// ---------------------------------------------------------------------------
// Module mocks — required before the module under test is imported
// ---------------------------------------------------------------------------

vi.mock('../../../../src/core/event-bus.js', () => ({
  eventBus: { on: vi.fn(), once: vi.fn(), off: vi.fn() },
  Events: {
    LOOKUPS_LOOKUP_NAMES_LOADED: 'lookups:lookup_names:loaded',
    LOOKUPS_THEMES_LOADED: 'lookups:themes:loaded',
    STARTUP_COMPLETE: 'startup:complete',
    AUTH_LOGIN: 'auth:login',
  },
}));

vi.mock('../../../../src/core/transitions.js', () => ({
  getTransitionDuration: vi.fn().mockReturnValue(300),
}));

vi.mock('../../../../src/shared/lookups.js', () => ({
  hasLookup: vi.fn().mockReturnValue(false),
}));

vi.mock('../../../../src/core/log.js', () => ({
  log: vi.fn(),
  Subsystems: { LOGIN: 'Login', AUTH: 'Auth' },
  Status: { INFO: 'INFO', WARN: 'WARN', ERROR: 'ERROR', SUCCESS: 'SUCCESS' },
}));

vi.mock('../../../../src/managers/login/login-panels.js', () => ({
  LoginPanels: vi.fn().mockImplementation(() => ({
    switchPanel: vi.fn().mockResolvedValue(undefined),
    cancelPendingFocus: vi.fn(),
    teardown: vi.fn(),
    currentPanel: 'login',
    _panels: {},
  })),
}));

vi.mock('../../../../src/managers/login/login-password-manager.js', () => ({
  PasswordManager: vi.fn().mockImplementation(() => ({
    show: vi.fn(), hide: vi.fn(), removeAll: vi.fn(), destroy: vi.fn(),
  })),
}));

vi.mock('../../../../src/managers/login/login-language-panel.js', () => ({
  LanguagePanel: vi.fn().mockImplementation(() => ({
    show: vi.fn(), hide: vi.fn(), filter: vi.fn(), destroy: vi.fn(),
  })),
}));

vi.mock('../../../../src/managers/login/login-logs-panel.js', () => ({
  LogsPanel: vi.fn().mockImplementation(() => ({
    show: vi.fn(), destroy: vi.fn(),
  })),
}));

vi.mock('../../../../src/managers/login/login-help-panel.js', () => ({
  HelpPanel: vi.fn().mockImplementation(() => ({
    init: vi.fn(), destroy: vi.fn(),
  })),
}));

vi.mock('../../../../src/managers/login/login-form.js', () => ({
  LoginForm: vi.fn().mockImplementation(() => ({
    init: vi.fn(),
    destroy: vi.fn(),
    loadRememberedUsername: vi.fn().mockReturnValue(false),
    showError: vi.fn(),
    hideError: vi.fn(),
    prefillAndSubmit: vi.fn(),
    handleClearUsername: vi.fn(),
  })),
}));

vi.mock('../../../../src/managers/login/login-shortcuts.js', () => ({
  LoginShortcuts: vi.fn().mockImplementation(() => ({
    attach: vi.fn(), detach: vi.fn(),
  })),
}));

vi.mock('../../../../src/managers/login/oidc-login.js', () => ({
  processOidcReturn: vi.fn().mockResolvedValue(false),
}));

vi.mock('../../../../src/core/oidc-client.js', () => ({
  startOidc: vi.fn(),
  exchangeHandoff: vi.fn(),
  default: { startOidc: vi.fn(), exchangeHandoff: vi.fn() },
}));

vi.mock('../../../../src/core/config.js', () => ({
  getConfigValue: vi.fn((key, defaultVal) => defaultVal),
}));

// CSS import — no-op in tests
vi.mock('../../../../src/managers/login/login.css', () => ({}));

// ---------------------------------------------------------------------------
// Import after mocks are in place
// ---------------------------------------------------------------------------

import LoginManager from '../../../../src/managers/login/login.js';

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/**
 * Build a minimal DOM environment for renderOidcProviders.
 * Returns an `elements` object matching what LoginManager.render() caches,
 * plus the two new Phase 25 elements.
 */
function makeDomElements() {
  // Minimal elements shape — only the ones renderOidcProviders reads.
  const divider = document.createElement('div');
  divider.id = 'login-oidc-divider';
  divider.style.display = 'none';

  const providersContainer = document.createElement('div');
  providersContainer.id = 'login-providers';
  providersContainer.style.display = 'none';

  // Append to body so querySelector works in tests (not strictly required
  // since we pass them by reference, but good practice).
  document.body.appendChild(divider);
  document.body.appendChild(providersContainer);

  return {
    oidcDivider: divider,
    oidcProviders: providersContainer,
  };
}

/**
 * Build a minimal LoginManager-like object that exposes only the surface
 * under test.  This avoids the async render/fetch flow entirely.
 */
function makeSubject(elements = makeDomElements()) {
  const lm = new LoginManager({ managers: {} }, document.createElement('div'));
  // Override elements with our controlled DOM nodes.
  lm.elements = elements;
  // Wire _loginForm stub for showError delegation.
  lm._loginForm = { showError: vi.fn(), hideError: vi.fn() };
  return lm;
}

/** One-provider config response. */
function oneProviderConfig(overrides = {}) {
  return [
    {
      id:        '500passwords',
      label:     'Sign in with 500 Passwords',
      icon:      'fa-key',
      start_url: '/api/auth/oidc/start',
      ...overrides,
    },
  ];
}

/** Deps that inject a single-provider config. */
function makeProviderDeps(providers = oneProviderConfig(), extraDeps = {}) {
  return {
    getConfigValue: (key, def) => (key === 'auth.oidc_providers' ? providers : def),
    startOidc: vi.fn(),
    window: { location: { pathname: '/login', href: '' } },
    ...extraDeps,
  };
}

// ---------------------------------------------------------------------------
// renderOidcProviders — empty / absent config
// ---------------------------------------------------------------------------

describe('renderOidcProviders — empty config', () => {
  beforeEach(() => {
    document.body.innerHTML = '';
  });

  it('does not show divider when oidc_providers is absent', () => {
    const lm   = makeSubject();
    const deps = { getConfigValue: () => undefined, startOidc: vi.fn(), window: null };
    lm.renderOidcProviders(deps);
    expect(lm.elements.oidcDivider.style.display).toBe('none');
  });

  it('does not show divider when oidc_providers is an empty array', () => {
    const lm   = makeSubject();
    const deps = makeProviderDeps([]);
    lm.renderOidcProviders(deps);
    expect(lm.elements.oidcDivider.style.display).toBe('none');
  });

  it('does not render any buttons when oidc_providers is empty', () => {
    const lm   = makeSubject();
    const deps = makeProviderDeps([]);
    lm.renderOidcProviders(deps);
    expect(lm.elements.oidcProviders.querySelectorAll('.login-btn-oidc').length).toBe(0);
  });

  it('does not show providers container when array is empty', () => {
    const lm   = makeSubject();
    const deps = makeProviderDeps([]);
    lm.renderOidcProviders(deps);
    expect(lm.elements.oidcProviders.style.display).toBe('none');
  });

  it('skips entries that are missing id or label', () => {
    const lm   = makeSubject();
    const deps = makeProviderDeps([
      { id: '', label: 'No ID' },
      { id: 'ok', label: '' },
    ]);
    lm.renderOidcProviders(deps);
    expect(lm.elements.oidcProviders.querySelectorAll('.login-btn-oidc').length).toBe(0);
    expect(lm.elements.oidcDivider.style.display).toBe('none');
  });
});

// ---------------------------------------------------------------------------
// renderOidcProviders — one provider
// ---------------------------------------------------------------------------

describe('renderOidcProviders — one provider', () => {
  beforeEach(() => {
    document.body.innerHTML = '';
  });

  it('renders exactly one button for one provider', () => {
    const lm   = makeSubject();
    const deps = makeProviderDeps();
    lm.renderOidcProviders(deps);
    expect(lm.elements.oidcProviders.querySelectorAll('.login-btn-oidc').length).toBe(1);
  });

  it('shows the divider when a provider is present', () => {
    const lm   = makeSubject();
    const deps = makeProviderDeps();
    lm.renderOidcProviders(deps);
    expect(lm.elements.oidcDivider.style.display).not.toBe('none');
  });

  it('shows the providers container when a provider is present', () => {
    const lm   = makeSubject();
    const deps = makeProviderDeps();
    lm.renderOidcProviders(deps);
    expect(lm.elements.oidcProviders.style.display).not.toBe('none');
  });

  it('button has the correct label text', () => {
    const lm   = makeSubject();
    const deps = makeProviderDeps();
    lm.renderOidcProviders(deps);
    const btn = lm.elements.oidcProviders.querySelector('.login-btn-oidc');
    expect(btn.textContent).toContain('Sign in with 500 Passwords');
  });

  it('button has data-provider attribute matching provider id', () => {
    const lm   = makeSubject();
    const deps = makeProviderDeps();
    lm.renderOidcProviders(deps);
    const btn = lm.elements.oidcProviders.querySelector('.login-btn-oidc');
    expect(btn.getAttribute('data-provider')).toBe('500passwords');
  });

  it('button has aria-label matching provider label', () => {
    const lm   = makeSubject();
    const deps = makeProviderDeps();
    lm.renderOidcProviders(deps);
    const btn = lm.elements.oidcProviders.querySelector('.login-btn-oidc');
    expect(btn.getAttribute('aria-label')).toBe('Sign in with 500 Passwords');
  });

  it('button renders an icon element when icon is provided', () => {
    const lm   = makeSubject();
    const deps = makeProviderDeps();
    lm.renderOidcProviders(deps);
    const iconEl = lm.elements.oidcProviders.querySelector('.login-btn-oidc__icon');
    expect(iconEl).not.toBeNull();
    expect(iconEl.className).toContain('fa-key');
  });

  it('button does not render an icon span when icon is absent', () => {
    const lm   = makeSubject();
    const deps = makeProviderDeps(oneProviderConfig({ icon: undefined }));
    lm.renderOidcProviders(deps);
    const iconEl = lm.elements.oidcProviders.querySelector('.login-btn-oidc__icon');
    expect(iconEl).toBeNull();
  });

  it('button type is "button" (not submit)', () => {
    const lm   = makeSubject();
    const deps = makeProviderDeps();
    lm.renderOidcProviders(deps);
    const btn = lm.elements.oidcProviders.querySelector('.login-btn-oidc');
    expect(btn.type).toBe('button');
  });
});

// ---------------------------------------------------------------------------
// renderOidcProviders — click handler
// ---------------------------------------------------------------------------

describe('renderOidcProviders — click handler', () => {
  beforeEach(() => {
    document.body.innerHTML = '';
  });

  it('calls startOidc with the provider id when button is clicked', () => {
    const lm       = makeSubject();
    const startFn  = vi.fn();
    const deps     = makeProviderDeps(oneProviderConfig(), { startOidc: startFn });
    lm.renderOidcProviders(deps);
    const btn = lm.elements.oidcProviders.querySelector('.login-btn-oidc');
    btn.click();
    expect(startFn).toHaveBeenCalledOnce();
    expect(startFn.mock.calls[0][0]).toBe('500passwords');
  });

  it('passes the current window.location.pathname as returnTo', () => {
    const lm      = makeSubject();
    const startFn = vi.fn();
    const win     = { location: { pathname: '/queries', href: '' } };
    const deps    = makeProviderDeps(oneProviderConfig(), { startOidc: startFn, window: win });
    lm.renderOidcProviders(deps);
    const btn = lm.elements.oidcProviders.querySelector('.login-btn-oidc');
    btn.click();
    expect(startFn.mock.calls[0][1]).toBe('/queries');
  });

  it('calls startOidc for the correct provider when multiple are present', () => {
    const lm      = makeSubject();
    const startFn = vi.fn();
    const providers = [
      { id: 'alpha', label: 'Alpha', icon: 'fa-a' },
      { id: 'beta',  label: 'Beta',  icon: 'fa-b' },
    ];
    const deps = makeProviderDeps(providers, { startOidc: startFn });
    lm.renderOidcProviders(deps);
    const buttons = lm.elements.oidcProviders.querySelectorAll('.login-btn-oidc');
    // Click the second button.
    buttons[1].click();
    expect(startFn.mock.calls[0][0]).toBe('beta');
  });

  it('calls showError if startOidc throws', () => {
    const lm      = makeSubject();
    const startFn = vi.fn().mockImplementation(() => { throw new Error('config missing'); });
    const deps    = makeProviderDeps(oneProviderConfig(), { startOidc: startFn });
    lm.renderOidcProviders(deps);
    const btn = lm.elements.oidcProviders.querySelector('.login-btn-oidc');
    btn.click();
    expect(lm._loginForm.showError).toHaveBeenCalledOnce();
    expect(lm._loginForm.showError.mock.calls[0][0]).toMatch(/could not start/i);
  });
});

// ---------------------------------------------------------------------------
// renderOidcProviders — multiple providers
// ---------------------------------------------------------------------------

describe('renderOidcProviders — multiple providers', () => {
  beforeEach(() => {
    document.body.innerHTML = '';
  });

  it('renders one button per provider entry', () => {
    const lm   = makeSubject();
    const deps = makeProviderDeps([
      { id: 'alpha', label: 'Alpha', icon: 'fa-a' },
      { id: 'beta',  label: 'Beta',  icon: 'fa-b' },
      { id: 'gamma', label: 'Gamma', icon: 'fa-g' },
    ]);
    lm.renderOidcProviders(deps);
    expect(lm.elements.oidcProviders.querySelectorAll('.login-btn-oidc').length).toBe(3);
  });

  it('shows the divider with multiple providers', () => {
    const lm   = makeSubject();
    const deps = makeProviderDeps([
      { id: 'a', label: 'A', icon: 'fa-a' },
      { id: 'b', label: 'B', icon: 'fa-b' },
    ]);
    lm.renderOidcProviders(deps);
    expect(lm.elements.oidcDivider.style.display).not.toBe('none');
  });
});

// ---------------------------------------------------------------------------
// showError proxy
// ---------------------------------------------------------------------------

describe('showError — proxy to LoginForm', () => {
  it('delegates to _loginForm.showError', () => {
    const lm = makeSubject();
    lm.showError('Something went wrong');
    expect(lm._loginForm.showError).toHaveBeenCalledOnce();
    expect(lm._loginForm.showError).toHaveBeenCalledWith('Something went wrong');
  });

  it('does not throw when _loginForm is null', () => {
    const lm = makeSubject();
    lm._loginForm = null;
    expect(() => lm.showError('oops')).not.toThrow();
  });
});
