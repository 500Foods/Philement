/**
 * Login Form
 *
 * Owns the login form lifecycle: input handlers, credential submission to the
 * Hydrogen `/api/auth/login` endpoint, JWT storage, error mapping, loading
 * state, password visibility toggle, clear-username helper, and the
 * "remember last username" persistence in `localStorage`.
 *
 * Extracted from login.js to keep files under 1,000 lines per LITHIUM-INS.md
 * rule #2. Designed as a self-contained module: it receives DOM element
 * references and a small set of callbacks, never reaches up into LoginManager.
 *
 * Lifecycle: `init()` (binds listeners) → `destroy()` (removes listeners and
 * clears state). `LoginManager` calls `init()` from `render()` and
 * `destroy()` from `teardown()`.
 */

import { eventBus, Events } from '../../core/event-bus.js';
import { storeJWT } from '../../core/jwt.js';
import { getConfigValue } from '../../core/config.js';
import { log, Subsystems, Status } from '../../core/log.js';
import { hasLookup } from '../../shared/lookups.js';
import { getTip } from '../../core/tooltip-api.js';

const LOGIN = Subsystems.LOGIN;
const REMEMBERED_USERNAME_KEY = 'lithium_last_username';
const NO_SAVE_USERNAME_KEY = 'lithium_no_save_username';

export class LoginForm {
  /**
   * @param {Object} options
   * @param {Object} options.elements - DOM element references. Required:
   *   `form`, `username`, `password`, `submit`, `error`, `errorText`,
   *   `clearUsernameBtn`, `togglePasswordBtn`, `passwordIcon`. Optional
   *   buttons used during loading-state toggling: `languageBtn`,
   *   `themeBtn`, `logsBtn`, `helpBtn`.
   * @param {Function} [options.onLoginSuccess] - Called with the parsed
   *   server response after JWT is stored, before the success log/event.
   *   Typical use: trigger `LoginManager.hide()`. Async; awaited.
   * @param {Function} [options.isStartupComplete] - Returns boolean.
   *   Used by `setLoading()` to decide if the logs button should be
   *   re-enabled when loading finishes. Defaults to `() => true`.
   * @param {Object} [options.deps] - Dependency overrides (for testing).
   *   - `deps.lithiumSettings` — override for `window.lithiumSettings`.
   */
  constructor({ elements, onLoginSuccess, isStartupComplete, deps } = {}) {
    this.elements = elements || {};
    this._onLoginSuccess = onLoginSuccess || (async () => {});
    this._isStartupComplete = isStartupComplete || (() => true);
    this._deps = deps || {};

    this.isSubmitting = false;
    this.isPasswordVisible = false;
    this._loginStart = null;

    // Bound handler refs so we can remove them in destroy().
    this._handlers = {};
  }

  /**
   * Bind all form-related event listeners. Idempotent: a second call
   * removes the previous listeners before re-binding.
   */
  init() {
    this.destroy();

    const onSubmit = (e) => {
      e.preventDefault();
      this.handleSubmit();
    };
    const onUsernameInput = () => this.hideError();
    const onPasswordInput = () => this.hideError();
    const onUsernameBlur = () => {
      const hasValue = !!(this.elements.username?.value?.trim());
      log(LOGIN, Status.INFO, `Username field: ${hasValue ? 'filled' : 'empty'}`);
    };
    const onPasswordBlur = () => {
      const hasValue = !!(this.elements.password?.value);
      log(LOGIN, Status.INFO, `Password field: ${hasValue ? 'filled' : 'empty'}`);
    };
    const onClearUsernameClick = () => this.handleClearUsername();
    const onToggleVisibilityClick = () => this.handleTogglePassword();

    this.elements.form?.addEventListener('submit', onSubmit);
    this.elements.username?.addEventListener('input', onUsernameInput);
    this.elements.password?.addEventListener('input', onPasswordInput);
    this.elements.username?.addEventListener('blur', onUsernameBlur);
    this.elements.password?.addEventListener('blur', onPasswordBlur);
    this.elements.clearUsernameBtn?.addEventListener('click', onClearUsernameClick);
    this.elements.togglePasswordBtn?.addEventListener('click', onToggleVisibilityClick);

    this._handlers = {
      onSubmit,
      onUsernameInput,
      onPasswordInput,
      onUsernameBlur,
      onPasswordBlur,
      onClearUsernameClick,
      onToggleVisibilityClick,
    };
  }

  /**
   * Remove all event listeners and clear handler refs. Safe to call before
   * `init()` has run.
   */
  destroy() {
    const h = this._handlers;
    if (!h || Object.keys(h).length === 0) return;

    this.elements.form?.removeEventListener('submit', h.onSubmit);
    this.elements.username?.removeEventListener('input', h.onUsernameInput);
    this.elements.password?.removeEventListener('input', h.onPasswordInput);
    this.elements.username?.removeEventListener('blur', h.onUsernameBlur);
    this.elements.password?.removeEventListener('blur', h.onPasswordBlur);
    this.elements.clearUsernameBtn?.removeEventListener('click', h.onClearUsernameClick);
    this.elements.togglePasswordBtn?.removeEventListener('click', h.onToggleVisibilityClick);

    this._handlers = {};
    this.isPasswordVisible = false;
  }

  /**
   * Read a previously remembered username from `localStorage` and pre-fill
   * the username field. Returns true if a value was loaded (so callers can
   * skip the username focus step in favour of focusing the password field).
   * @returns {boolean}
   */
  loadRememberedUsername() {
    try {
      const remembered = localStorage.getItem(REMEMBERED_USERNAME_KEY);
      if (remembered && this.elements.username) {
        this.elements.username.value = remembered;
        return true;
      }
      return false;
    } catch (error) {
      console.warn('[LoginForm] Could not load remembered username:', error);
      return false;
    }
  }

  /**
   * Persist the supplied username to `localStorage` for next visit, unless
   * the session-scoped suppression flag is set (e.g. after a public/global
   * logout).
   * @param {string} username
   */
  saveRememberedUsername(username) {
    try {
      if (sessionStorage.getItem(NO_SAVE_USERNAME_KEY) === 'true') {
        sessionStorage.removeItem(NO_SAVE_USERNAME_KEY);
        return;
      }
      if (username) {
        localStorage.setItem(REMEMBERED_USERNAME_KEY, username);
      }
    } catch (error) {
      console.warn('[LoginForm] Could not save username:', error);
    }
  }

  /**
   * Pre-fill credentials and immediately submit the form. Used by
   * `LoginManager.checkForAutoLogin()` for the URL-driven auto-login flow.
   * @param {string} username
   * @param {string} password
   */
  prefillAndSubmit(username, password) {
    if (this.elements.username) this.elements.username.value = username;
    if (this.elements.password) this.elements.password.value = password;
    setTimeout(() => this.handleSubmit(), 100);
  }

  /**
   * Clear both fields, focus username, and dismiss any visible error.
   */
  handleClearUsername() {
    log(LOGIN, Status.INFO, 'Credentials cleared');
    if (this.elements.username) {
      this.elements.username.value = '';
      this.elements.username.focus();
    }
    if (this.elements.password) {
      this.elements.password.value = '';
    }
    this.hideError();
  }

  /**
   * Toggle the password field between `password` and `text` types, swap
   * the eye/eye-slash icon, and refresh the toggle button's tooltip text.
   */
  handleTogglePassword() {
    if (!this.elements.password || !this.elements.passwordIcon) return;

    this.isPasswordVisible = !this.isPasswordVisible;
    log(LOGIN, Status.INFO, `Password visibility: ${this.isPasswordVisible ? 'shown' : 'hidden'}`);

    const tooltipText = this.isPasswordVisible ? 'Hide password' : 'Show password';
    const addIcon = this.isPasswordVisible ? 'fa-eye-slash' : 'fa-eye';
    const removeIcon = this.isPasswordVisible ? 'fa-eye' : 'fa-eye-slash';

    this.elements.password.type = this.isPasswordVisible ? 'text' : 'password';
    this.elements.passwordIcon.classList.remove(removeIcon);
    this.elements.passwordIcon.classList.add(addIcon);

    if (this.elements.togglePasswordBtn) {
      this.elements.togglePasswordBtn.setAttribute('data-tooltip', tooltipText);
      const tip = getTip(this.elements.togglePasswordBtn);
      if (tip) tip.updateContent(tooltipText);
    }
  }

  /**
   * Validate form values and dispatch a login attempt. Guards against
   * concurrent submissions.
   */
  async handleSubmit() {
    if (this.isSubmitting) return;

    const username = this.elements.username?.value?.trim();
    const password = this.elements.password?.value;

    if (!username || !password) {
      log(LOGIN, Status.WARN, 'Login attempted with empty credentials');
      this.showError('Please enter both username and password');
      return;
    }

    log(LOGIN, Status.INFO, `Login attempt: user "${username}"`);
    this._loginStart = performance.now();

    this.isSubmitting = true;
    this.setLoading(true);
    this.hideError();

    try {
      await this.attemptLogin(username, password);
    } catch (error) {
      this.handleLoginError(error);
    } finally {
      this.isSubmitting = false;
      this.setLoading(false);
    }
  }

  /**
   * POST credentials to Hydrogen, store JWT on success, invoke the
   * `onLoginSuccess` callback, then emit `AUTH_LOGIN`.
   * @param {string} username
   * @param {string} password
   */
  async attemptLogin(username, password) {
    const serverUrl = getConfigValue('server.url', 'http://localhost:8080');
    const apiPrefix = getConfigValue('server.api_prefix', '/api');

    const loginData = {
      login_id: username,
      password,
      api_key: getConfigValue('auth.api_key', ''),
      tz: Intl.DateTimeFormat().resolvedOptions().timeZone,
      database: getConfigValue('auth.default_database', 'Demo_PG'),
    };

    const response = await fetch(`${serverUrl}${apiPrefix}/auth/login`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'Accept': 'application/json',
      },
      body: JSON.stringify(loginData),
    });

    if (!response.ok) {
      const error = new Error(`HTTP ${response.status}`);
      error.status = response.status;
      try {
        error.data = await response.json();
      } catch (e) {
        // Body is not JSON; ignore.
      }
      if (response.status === 429) {
        const retryAfter = response.headers.get('retry-after');
        error.retryAfter = retryAfter ? parseInt(retryAfter, 10) : null;
      }
      throw error;
    }

    const data = await response.json();

    if (!data.token) {
      throw new Error('No token received from server');
    }
    storeJWT(data.token);

    // Record the last-used login method so the UI can subtly highlight it
    // on the next visit (Phase 26).
    const settings = this._deps.lithiumSettings ??
      (typeof window !== 'undefined' ? window.lithiumSettings : null);
    settings?.set('auth.last_method', 'password');

    this.saveRememberedUsername(username);

    // Hand off to caller (e.g. LoginManager.hide()) before emitting the
    // login event, so any UI fade-out completes first.
    await this._onLoginSuccess(data);

    const loginDuration = this._loginStart
      ? Math.round(performance.now() - this._loginStart)
      : 0;
    this._loginStart = null;
    log(LOGIN, Status.SUCCESS, `Login successful: user "${username}"`, loginDuration);

    eventBus.emit(Events.AUTH_LOGIN, {
      userId: data.user_id,
      username,
      roles: data.roles || [],
      expiresAt: data.expires_at,
    });
  }

  /**
   * Map an error from `attemptLogin` to a user-facing message and surface it.
   * @param {Error & { status?: number, data?: any, retryAfter?: number|null }} error
   */
  handleLoginError(error) {
    const loginDuration = this._loginStart
      ? Math.round(performance.now() - this._loginStart)
      : 0;
    this._loginStart = null;
    log(LOGIN, Status.ERROR, `Login failed: HTTP ${error.status || error.message}`, loginDuration);
    console.error('[LoginForm] Login failed:', error);

    let message = 'Login failed. Please try again.';

    switch (error.status) {
      case 401:
        message = error.data?.message || 'Invalid username or password';
        if (this.elements.password) {
          this.elements.password.value = '';
          this.elements.password.focus();
        }
        break;

      case 429:
        if (error.retryAfter) {
          const minutes = Math.ceil(error.retryAfter / 60);
          message = `Too many attempts. Please try again in ${minutes} minute${minutes > 1 ? 's' : ''}.`;
        } else {
          message = 'Too many attempts. Please try again later.';
        }
        break;

      case 500:
      case 502:
      case 503:
      case 504:
        message = 'Server error. Please try again later.';
        break;

      default:
        if (typeof navigator !== 'undefined' && !navigator.onLine) {
          message = 'No internet connection. Please check your network.';
        } else {
          message = error.data?.message || 'Login failed. Please try again.';
        }
    }

    this.showError(message);
  }

  /**
   * Show an error banner below the form.
   * @param {string} message
   */
  showError(message) {
    if (this.elements.errorText) {
      this.elements.errorText.textContent = message;
    }
    this.elements.error?.classList.add('visible');
  }

  /**
   * Hide the error banner.
   */
  hideError() {
    this.elements.error?.classList.remove('visible');
  }

  /**
   * Toggle the loading state of the submit button and ancillary buttons.
   * Buttons whose enabled state depends on lookup availability or startup
   * completion are restored to their proper state when `loading` is false.
   * @param {boolean} loading
   */
  setLoading(loading) {
    if (this.elements.submit) {
      this.elements.submit.disabled = loading;
      this.elements.submit.innerHTML = loading
        ? '<span class="spinner-fancy spinner-fancy-sm"></span> &nbsp;&nbsp; <span>Logging in...</span>'
        : '<fa fa-sign-in-alt></fa> <span>Login</span>';
    }

    if (this.elements.languageBtn) {
      this.elements.languageBtn.disabled = loading || !hasLookup('lookup_names');
    }
    if (this.elements.themeBtn) {
      this.elements.themeBtn.disabled = loading || !hasLookup('themes');
    }
    if (this.elements.logsBtn) {
      this.elements.logsBtn.disabled = loading || !this._isStartupComplete();
    }
    if (this.elements.helpBtn) {
      this.elements.helpBtn.disabled = loading;
    }
    if (this.elements.clearUsernameBtn) {
      this.elements.clearUsernameBtn.disabled = loading;
    }
    if (this.elements.togglePasswordBtn) {
      this.elements.togglePasswordBtn.disabled = loading;
    }
  }
}
