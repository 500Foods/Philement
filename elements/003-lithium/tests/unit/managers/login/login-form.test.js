/**
 * Login Form Tests
 *
 * Unit tests for the LoginForm class extracted from login.js.
 */

import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest';
import { LoginForm } from '../../../../src/managers/login/login-form.js';

// Mock log module (singleton, hoisted by Vitest).
vi.mock('../../../../src/core/log.js', () => ({
  log: vi.fn(),
  Subsystems: { LOGIN: 'LOGIN' },
  Status: { INFO: 'INFO', SUCCESS: 'SUCCESS', ERROR: 'ERROR', WARN: 'WARN' },
}));

// Event bus mock with a manual registry so tests can assert emissions.
vi.mock('../../../../src/core/event-bus.js', () => ({
  eventBus: { emit: vi.fn() },
  Events: { AUTH_LOGIN: 'auth:login' },
}));

// JWT storage mock.
vi.mock('../../../../src/core/jwt.js', () => ({
  storeJWT: vi.fn(),
}));

// Config mock — return whatever default the caller passes.
vi.mock('../../../../src/core/config.js', () => ({
  getConfigValue: vi.fn((key, defaultValue) => {
    const overrides = {
      'server.url': 'http://test.example',
      'server.api_prefix': '/api',
      'auth.api_key': 'TEST_KEY',
      'auth.default_database': 'TestDB',
    };
    return key in overrides ? overrides[key] : defaultValue;
  }),
}));

vi.mock('../../../../src/shared/lookups.js', () => ({
  hasLookup: vi.fn(() => true),
}));

vi.mock('../../../../src/core/tooltip-api.js', () => ({
  getTip: vi.fn(() => null),
}));

function makeElements() {
  const form = document.createElement('form');
  const username = document.createElement('input');
  username.type = 'text';
  const password = document.createElement('input');
  password.type = 'password';
  const submit = document.createElement('button');
  const error = document.createElement('div');
  const errorText = document.createElement('span');
  const clearUsernameBtn = document.createElement('button');
  const togglePasswordBtn = document.createElement('button');
  const passwordIcon = document.createElement('i');
  passwordIcon.classList.add('fa-eye');
  const languageBtn = document.createElement('button');
  const themeBtn = document.createElement('button');
  const logsBtn = document.createElement('button');
  const helpBtn = document.createElement('button');

  form.appendChild(username);
  form.appendChild(password);
  form.appendChild(submit);
  document.body.appendChild(form);
  document.body.appendChild(error);

  return {
    form, username, password, submit, error, errorText,
    clearUsernameBtn, togglePasswordBtn, passwordIcon,
    languageBtn, themeBtn, logsBtn, helpBtn,
  };
}

function cleanupElements(elements) {
  if (elements.form && elements.form.parentNode) elements.form.parentNode.removeChild(elements.form);
  if (elements.error && elements.error.parentNode) elements.error.parentNode.removeChild(elements.error);
}

describe('LoginForm', () => {
  let elements;
  let form;
  let originalFetch;

  beforeEach(() => {
    vi.clearAllMocks();
    localStorage.clear();
    sessionStorage.clear();
    elements = makeElements();
    originalFetch = global.fetch;
  });

  afterEach(() => {
    if (form) form.destroy();
    form = null;
    cleanupElements(elements);
    global.fetch = originalFetch;
  });

  describe('init() / destroy()', () => {
    it('binds submit handler that calls handleSubmit', async () => {
      form = new LoginForm({ elements });
      const spy = vi.spyOn(form, 'handleSubmit').mockImplementation(() => {});
      form.init();

      elements.form.dispatchEvent(new Event('submit', { cancelable: true }));
      expect(spy).toHaveBeenCalled();
    });

    it('hides the error on input in either field', () => {
      form = new LoginForm({ elements });
      form.init();
      elements.error.classList.add('visible');

      elements.username.dispatchEvent(new Event('input'));
      expect(elements.error.classList.contains('visible')).toBe(false);

      elements.error.classList.add('visible');
      elements.password.dispatchEvent(new Event('input'));
      expect(elements.error.classList.contains('visible')).toBe(false);
    });

    it('destroy() removes listeners', () => {
      form = new LoginForm({ elements });
      const spy = vi.spyOn(form, 'handleSubmit').mockImplementation(() => {});
      form.init();
      form.destroy();

      elements.form.dispatchEvent(new Event('submit', { cancelable: true }));
      expect(spy).not.toHaveBeenCalled();
    });

    it('init() is idempotent (binding twice does not double-fire)', () => {
      form = new LoginForm({ elements });
      const spy = vi.spyOn(form, 'handleSubmit').mockImplementation(() => {});
      form.init();
      form.init();
      elements.form.dispatchEvent(new Event('submit', { cancelable: true }));
      expect(spy).toHaveBeenCalledTimes(1);
    });
  });

  describe('handleSubmit()', () => {
    it('shows error and returns when fields are empty', async () => {
      form = new LoginForm({ elements });
      form.init();
      await form.handleSubmit();
      expect(elements.errorText.textContent).toBe('Please enter both username and password');
      expect(elements.error.classList.contains('visible')).toBe(true);
    });

    it('does nothing when already submitting', async () => {
      form = new LoginForm({ elements });
      form.init();
      form.isSubmitting = true;
      await form.handleSubmit();
      // No error shown, no fetch attempted.
      expect(elements.error.classList.contains('visible')).toBe(false);
    });
  });

  describe('attemptLogin() — happy path', () => {
    it('stores JWT, calls onLoginSuccess, emits AUTH_LOGIN with the right shape', async () => {
      const { storeJWT } = await import('../../../../src/core/jwt.js');
      const { eventBus } = await import('../../../../src/core/event-bus.js');

      const onLoginSuccess = vi.fn(async () => {});
      const responseBody = {
        token: 'jwt.value.here',
        user_id: 42,
        roles: ['admin', 'user'],
        expires_at: '2026-12-31T00:00:00Z',
      };
      global.fetch = vi.fn(() => Promise.resolve({
        ok: true,
        json: () => Promise.resolve(responseBody),
        headers: new Headers(),
      }));

      form = new LoginForm({ elements, onLoginSuccess });
      form.init();
      elements.username.value = 'alice';
      elements.password.value = 'hunter2';

      await form.handleSubmit();

      expect(global.fetch).toHaveBeenCalledWith(
        'http://test.example/api/auth/login',
        expect.objectContaining({
          method: 'POST',
          headers: expect.objectContaining({ 'Content-Type': 'application/json' }),
        }),
      );
      expect(storeJWT).toHaveBeenCalledWith('jwt.value.here');
      expect(onLoginSuccess).toHaveBeenCalledWith(responseBody);
      expect(eventBus.emit).toHaveBeenCalledWith('auth:login', expect.objectContaining({
        userId: 42,
        username: 'alice',
        roles: ['admin', 'user'],
        expiresAt: '2026-12-31T00:00:00Z',
      }));
    });

    it('persists the username to localStorage on success', async () => {
      global.fetch = vi.fn(() => Promise.resolve({
        ok: true,
        json: () => Promise.resolve({ token: 't', user_id: 1, roles: [] }),
        headers: new Headers(),
      }));

      form = new LoginForm({ elements });
      form.init();
      elements.username.value = 'bob';
      elements.password.value = 'pw';
      await form.handleSubmit();

      expect(localStorage.getItem('lithium_last_username')).toBe('bob');
    });

    it('throws when the server response has no token', async () => {
      global.fetch = vi.fn(() => Promise.resolve({
        ok: true,
        json: () => Promise.resolve({}),
        headers: new Headers(),
      }));

      form = new LoginForm({ elements });
      form.init();
      elements.username.value = 'alice';
      elements.password.value = 'pw';
      await form.handleSubmit();

      expect(elements.error.classList.contains('visible')).toBe(true);
    });
  });

  describe('handleLoginError() — error mapping', () => {
    function makeFetchError(status, body, headers = new Headers()) {
      return vi.fn(() => Promise.resolve({
        ok: false,
        status,
        json: () => Promise.resolve(body || {}),
        headers,
      }));
    }

    it('401 shows "Invalid username or password" and clears+focuses password', async () => {
      global.fetch = makeFetchError(401, { message: null });

      form = new LoginForm({ elements });
      form.init();
      elements.username.value = 'alice';
      elements.password.value = 'wrong';
      const focusSpy = vi.spyOn(elements.password, 'focus');

      await form.handleSubmit();

      expect(elements.errorText.textContent).toBe('Invalid username or password');
      expect(elements.password.value).toBe('');
      expect(focusSpy).toHaveBeenCalled();
    });

    it('401 with custom server message uses that message', async () => {
      global.fetch = makeFetchError(401, { message: 'Account locked' });
      form = new LoginForm({ elements });
      form.init();
      elements.username.value = 'a';
      elements.password.value = 'b';
      await form.handleSubmit();
      expect(elements.errorText.textContent).toBe('Account locked');
    });

    it('429 with retry-after header reports minutes', async () => {
      const headers = new Headers({ 'retry-after': '120' });
      global.fetch = makeFetchError(429, {}, headers);
      form = new LoginForm({ elements });
      form.init();
      elements.username.value = 'a';
      elements.password.value = 'b';
      await form.handleSubmit();
      expect(elements.errorText.textContent).toBe('Too many attempts. Please try again in 2 minutes.');
    });

    it('429 without retry-after uses generic message', async () => {
      global.fetch = makeFetchError(429, {});
      form = new LoginForm({ elements });
      form.init();
      elements.username.value = 'a';
      elements.password.value = 'b';
      await form.handleSubmit();
      expect(elements.errorText.textContent).toBe('Too many attempts. Please try again later.');
    });

    it.each([500, 502, 503, 504])('5xx (%i) shows server error message', async (status) => {
      global.fetch = makeFetchError(status, {});
      form = new LoginForm({ elements });
      form.init();
      elements.username.value = 'a';
      elements.password.value = 'b';
      await form.handleSubmit();
      expect(elements.errorText.textContent).toBe('Server error. Please try again later.');
    });

    it('reports offline message when navigator.onLine is false', async () => {
      Object.defineProperty(global.navigator, 'onLine', { configurable: true, get: () => false });
      global.fetch = vi.fn(() => Promise.reject(new Error('network')));

      form = new LoginForm({ elements });
      form.init();
      elements.username.value = 'a';
      elements.password.value = 'b';
      await form.handleSubmit();

      expect(elements.errorText.textContent).toBe('No internet connection. Please check your network.');

      Object.defineProperty(global.navigator, 'onLine', { configurable: true, get: () => true });
    });
  });

  describe('rememberedUsername round-trip', () => {
    it('save then load returns the same value', () => {
      form = new LoginForm({ elements });
      form.saveRememberedUsername('charlie');
      const loaded = form.loadRememberedUsername();
      expect(loaded).toBe(true);
      expect(elements.username.value).toBe('charlie');
    });

    it('returns false when no username is stored', () => {
      form = new LoginForm({ elements });
      expect(form.loadRememberedUsername()).toBe(false);
    });

    it('skips saving when the no-save flag is set in sessionStorage', () => {
      sessionStorage.setItem('lithium_no_save_username', 'true');
      form = new LoginForm({ elements });
      form.saveRememberedUsername('dave');
      expect(localStorage.getItem('lithium_last_username')).toBe(null);
      // Flag is consumed.
      expect(sessionStorage.getItem('lithium_no_save_username')).toBe(null);
    });
  });

  describe('handleClearUsername()', () => {
    it('clears both fields, focuses username, hides error', () => {
      form = new LoginForm({ elements });
      form.init();
      elements.username.value = 'a';
      elements.password.value = 'b';
      elements.error.classList.add('visible');
      const focusSpy = vi.spyOn(elements.username, 'focus');

      form.handleClearUsername();

      expect(elements.username.value).toBe('');
      expect(elements.password.value).toBe('');
      expect(elements.error.classList.contains('visible')).toBe(false);
      expect(focusSpy).toHaveBeenCalled();
    });
  });

  describe('handleTogglePassword()', () => {
    it('flips type, swaps icon, updates tooltip', () => {
      form = new LoginForm({ elements });
      form.init();

      form.handleTogglePassword();
      expect(elements.password.type).toBe('text');
      expect(elements.passwordIcon.classList.contains('fa-eye-slash')).toBe(true);
      expect(elements.passwordIcon.classList.contains('fa-eye')).toBe(false);
      expect(elements.togglePasswordBtn.getAttribute('data-tooltip')).toBe('Hide password');

      form.handleTogglePassword();
      expect(elements.password.type).toBe('password');
      expect(elements.passwordIcon.classList.contains('fa-eye')).toBe(true);
      expect(elements.passwordIcon.classList.contains('fa-eye-slash')).toBe(false);
      expect(elements.togglePasswordBtn.getAttribute('data-tooltip')).toBe('Show password');
    });

    it('is a no-op when password or icon is missing', () => {
      const partial = makeElements();
      partial.passwordIcon = null;
      const f = new LoginForm({ elements: partial });
      f.init();
      expect(() => f.handleTogglePassword()).not.toThrow();
      expect(f.isPasswordVisible).toBe(false);
      f.destroy();
      cleanupElements(partial);
    });
  });

  describe('setLoading()', () => {
    it('disables submit and shows the spinner during loading', async () => {
      form = new LoginForm({ elements });
      form.init();
      form.setLoading(true);
      expect(elements.submit.disabled).toBe(true);
      expect(elements.submit.innerHTML).toContain('Logging in');
    });

    it('respects isStartupComplete callback when re-enabling logsBtn', async () => {
      const { hasLookup } = await import('../../../../src/shared/lookups.js');
      hasLookup.mockReturnValue(true);
      const isStartupComplete = vi.fn(() => false);
      form = new LoginForm({ elements, isStartupComplete });
      form.init();

      form.setLoading(false);

      expect(isStartupComplete).toHaveBeenCalled();
      // logsBtn stays disabled because startup is incomplete.
      expect(elements.logsBtn.disabled).toBe(true);
      // Other lookup-gated buttons re-enable.
      expect(elements.languageBtn.disabled).toBe(false);
    });
  });

  describe('prefillAndSubmit()', () => {
    it('pre-fills both fields and triggers handleSubmit', async () => {
      vi.useFakeTimers();
      form = new LoginForm({ elements });
      form.init();
      const spy = vi.spyOn(form, 'handleSubmit').mockImplementation(() => {});

      form.prefillAndSubmit('eve', 'pw');
      vi.advanceTimersByTime(150);

      expect(elements.username.value).toBe('eve');
      expect(elements.password.value).toBe('pw');
      expect(spy).toHaveBeenCalled();
      vi.useRealTimers();
    });
  });
});
