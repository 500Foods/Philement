/**
 * Login Manager
 * 
 * Handles user authentication against the Hydrogen server.
 * Loads CCC-style login page, validates credentials, stores JWT.
 */

import { eventBus, Events } from '../../core/event-bus.js';
import { storeJWT } from '../../core/jwt.js';
import { getConfig, getConfigValue } from '../../core/config.js';

/**
 * Login Manager Class
 */
export default class LoginManager {
  constructor(app, container) {
    this.app = app;
    this.container = container;
    this.elements = {};
    this.isSubmitting = false;
  }

  /**
   * Initialize the login manager
   */
  async init() {
    await this.render();
    this.setupEventListeners();
    this.show();
  }

  /**
   * Render the login page
   */
  async render() {
    // Load HTML template
    try {
      const response = await fetch('/src/managers/login/login.html');
      const html = await response.text();
      this.container.innerHTML = html;
    } catch (error) {
      console.error('[LoginManager] Failed to load template:', error);
      this.renderFallback();
    }

    // Cache DOM elements
    this.elements = {
      page: this.container.querySelector('#login-page'),
      form: this.container.querySelector('#login-form'),
      username: this.container.querySelector('#login-username'),
      password: this.container.querySelector('#login-password'),
      submit: this.container.querySelector('#login-submit'),
      error: this.container.querySelector('#login-error'),
      errorText: this.container.querySelector('#login-error-text'),
    };

    // Focus username field after a short delay (allows transition)
    setTimeout(() => {
      this.elements.username?.focus();
    }, 500);
  }

  /**
   * Render a fallback login form if template fails to load
   */
  renderFallback() {
    this.container.innerHTML = `
      <div id="login-page">
        <div class="login-container">
          <div class="login-header">
            <h1>Lithium</h1>
            <p>A Philement Project</p>
          </div>
          <form class="login-form" id="login-form">
            <div class="form-group">
              <input type="text" id="login-username" placeholder="Username" required>
            </div>
            <div class="form-group">
              <input type="password" id="login-password" placeholder="Password" required>
            </div>
            <div class="login-error" id="login-error">
              <span id="login-error-text">Error</span>
            </div>
            <button type="submit" class="login-btn" id="login-submit">Login</button>
          </form>
        </div>
      </div>
    `;
  }

  /**
   * Set up event listeners
   */
  setupEventListeners() {
    // Form submission
    this.elements.form?.addEventListener('submit', (e) => {
      e.preventDefault();
      this.handleSubmit();
    });

    // Clear error on input
    this.elements.username?.addEventListener('input', () => this.hideError());
    this.elements.password?.addEventListener('input', () => this.hideError());
  }

  /**
   * Show the login page with fade-in
   */
  show() {
    // Trigger reflow for transition
    requestAnimationFrame(() => {
      this.elements.page?.classList.add('visible');
    });
  }

  /**
   * Hide the login page with fade-out
   */
  hide() {
    return new Promise((resolve) => {
      this.elements.page?.classList.add('fade-out');
      setTimeout(resolve, 800); // Match CSS transition duration
    });
  }

  /**
   * Handle form submission
   */
  async handleSubmit() {
    if (this.isSubmitting) return;

    const username = this.elements.username?.value?.trim();
    const password = this.elements.password?.value;

    if (!username || !password) {
      this.showError('Please enter both username and password');
      return;
    }

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
   * Attempt to log in with Hydrogen API
   */
  async attemptLogin(username, password) {
    const config = getConfig();
    const serverUrl = getConfigValue('server.url', 'http://localhost:8080');
    const apiPrefix = getConfigValue('server.api_prefix', '/api');

    // Build login request
    const loginData = {
      login_id: username,
      password: password,
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
      
      // Try to parse error details
      try {
        const data = await response.json();
        error.data = data;
      } catch (e) {
        // Ignore parse error
      }

      // Check for rate limit header
      if (response.status === 429) {
        const retryAfter = response.headers.get('retry-after');
        error.retryAfter = retryAfter ? parseInt(retryAfter, 10) : null;
      }

      throw error;
    }

    const data = await response.json();

    // Store JWT
    if (data.token) {
      storeJWT(data.token);
    } else {
      throw new Error('No token received from server');
    }

    // Success - emit login event
    eventBus.emit(Events.AUTH_LOGIN, {
      userId: data.user_id,
      username,
      roles: data.roles || [],
      expiresAt: data.expires_at,
    });

    // Fade out login page
    await this.hide();
  }

  /**
   * Handle login errors
   */
  handleLoginError(error) {
    console.error('[LoginManager] Login failed:', error);

    let message = 'Login failed. Please try again.';

    switch (error.status) {
      case 401:
        message = error.data?.message || 'Invalid username or password';
        // Clear password field and focus it
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
        if (!navigator.onLine) {
          message = 'No internet connection. Please check your network.';
        } else {
          message = error.data?.message || 'Login failed. Please try again.';
        }
    }

    this.showError(message);
  }

  /**
   * Show error message
   */
  showError(message) {
    if (this.elements.errorText) {
      this.elements.errorText.textContent = message;
    }
    this.elements.error?.classList.add('visible');
  }

  /**
   * Hide error message
   */
  hideError() {
    this.elements.error?.classList.remove('visible');
  }

  /**
   * Set loading state
   */
  setLoading(loading) {
    if (this.elements.submit) {
      this.elements.submit.disabled = loading;
      this.elements.submit.innerHTML = loading
        ? '<i class="fas fa-spinner fa-spin"></i> <span>Logging in...</span>'
        : '<i class="fas fa-sign-in-alt"></i> <span>Login</span>';
    }
  }

  /**
   * Teardown the login manager
   */
  teardown() {
    // Clear references
    this.elements = {};
  }
}
