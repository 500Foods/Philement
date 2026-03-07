/**
 * Login Manager
 * 
 * Handles user authentication against the Hydrogen server.
 * Loads CCC-style login page, validates credentials, stores JWT.
 * Features: Theme selector, Log viewer with fade transitions
 */

import { eventBus, Events } from '../../core/event-bus.js';
import { storeJWT } from '../../core/jwt.js';
import { getConfig, getConfigValue } from '../../core/config.js';
import { getTransitionDuration, waitForTransition } from '../../core/transitions.js';
import { hasLookup } from '../../shared/lookups.js';

/**
 * Login Manager Class
 */
export default class LoginManager {
  constructor(app, container) {
    this.app = app;
    this.container = container;
    this.elements = {};
    this.isSubmitting = false;
    this.isPasswordVisible = false;
    this.isCapsLockOn = false;
    this.currentPanel = 'login'; // 'login', 'theme', 'logs', 'language', 'help'
    this.panels = {};
    this.lookupListeners = [];
  }

  /**
   * Initialize the login manager
   */
  async init() {
    await this.render();
    this.setupEventListeners();
    this.setupLookupListeners();
    this.initializeButtonStates();

    // Load remembered username before showing (sets initial values)
    const hadRememberedUsername = this.loadRememberedUsername();

    // Show the panel with fade-in (pass focus info to avoid double focus)
    await this.show(hadRememberedUsername);

    // Check for URL query parameters for auto-login
    this.checkForAutoLogin();
  }

  /**
   * Load remembered username from localStorage
   * If username exists, pre-fill it and focus password field
   * @returns {boolean} True if a username was loaded
   */
  loadRememberedUsername() {
    try {
      const rememberedUsername = localStorage.getItem('lithium_last_username');
      if (rememberedUsername && this.elements.username) {
        this.elements.username.value = rememberedUsername;
        return true;
      }
      return false;
    } catch (error) {
      console.warn('[LoginManager] Could not load remembered username:', error);
      return false;
    }
  }

  /**
   * Save username to localStorage for next visit
   * @param {string} username - Username to remember
   */
  saveRememberedUsername(username) {
    try {
      if (username) {
        localStorage.setItem('lithium_last_username', username);
      }
    } catch (error) {
      console.warn('[LoginManager] Could not save username:', error);
    }
  }

  /**
   * Initialize button states based on current lookups availability
   * Buttons start disabled and are enabled when data arrives
   */
  initializeButtonStates() {
    // Start with buttons disabled
    this.setLanguageButtonEnabled(false);
    this.setThemeButtonEnabled(false);
    this.setLogsButtonEnabled(false);

    // If lookups are already loaded, enable buttons immediately
    if (hasLookup('lookup_names')) {
      this.setLanguageButtonEnabled(true);
    }
    if (hasLookup('themes')) {
      this.setThemeButtonEnabled(true);
    }
    if (hasLookup('system_info')) {
      this.setLogsButtonEnabled(true);
    }
  }

  /**
   * Set up event listeners for lookups loaded events
   */
  setupLookupListeners() {
    // Listen for lookup_names lookup - enable language button when translations are available
    const handleLookupNamesLoaded = () => {
      this.setLanguageButtonEnabled(true);
    };

    // Listen for themes lookup - enable theme button when themes are loaded
    const handleThemesLoaded = () => {
      this.setThemeButtonEnabled(true);
    };

    // Listen for system_info lookup - enable logs button when system info is loaded
    const handleSystemInfoLoaded = () => {
      this.setLogsButtonEnabled(true);
    };

    // Subscribe to specific lookup events
    eventBus.on(Events.LOOKUPS_LOOKUP_NAMES_LOADED, handleLookupNamesLoaded);
    eventBus.on(Events.LOOKUPS_THEMES_LOADED, handleThemesLoaded);
    eventBus.on(Events.LOOKUPS_SYSTEM_INFO_LOADED, handleSystemInfoLoaded);

    // Store unsubscribe functions for cleanup
    this.lookupListeners.push(() => eventBus.off(Events.LOOKUPS_LOOKUP_NAMES_LOADED, handleLookupNamesLoaded));
    this.lookupListeners.push(() => eventBus.off(Events.LOOKUPS_THEMES_LOADED, handleThemesLoaded));
    this.lookupListeners.push(() => eventBus.off(Events.LOOKUPS_SYSTEM_INFO_LOADED, handleSystemInfoLoaded));
  }

  /**
   * Enable or disable the theme button
   * @param {boolean} enabled - Whether to enable the button
   */
  setThemeButtonEnabled(enabled) {
    if (this.elements.themeBtn) {
      this.elements.themeBtn.disabled = !enabled;
    }
  }

  /**
   * Enable or disable the logs button
   * @param {boolean} enabled - Whether to enable the button
   */
  setLogsButtonEnabled(enabled) {
    if (this.elements.logsBtn) {
      this.elements.logsBtn.disabled = !enabled;
    }
  }

  /**
   * Enable or disable the language button
   * @param {boolean} enabled - Whether to enable the button
   */
  setLanguageButtonEnabled(enabled) {
    if (this.elements.languageBtn) {
      this.elements.languageBtn.disabled = !enabled;
    }
  }

  /**
   * Check for USER and PASS URL query parameters and auto-login if present
   */
  checkForAutoLogin() {
    const urlParams = new URLSearchParams(window.location.search);
    const username = urlParams.get('USER');
    const password = urlParams.get('PASS');

    if (username && password) {
      console.log('[LoginManager] Auto-login credentials detected in URL');
      
      // Pre-fill the form fields
      if (this.elements.username) {
        this.elements.username.value = username;
      }
      if (this.elements.password) {
        this.elements.password.value = password;
      }

      // Trigger login after a short delay to ensure UI is ready
      setTimeout(() => {
        this.handleSubmit();
      }, 100);
    }
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
      overlay: this.container.querySelector('#transition-overlay'),
      loginPanel: this.container.querySelector('#login-panel'),
      themePanel: this.container.querySelector('#theme-panel'),
      logsPanel: this.container.querySelector('#logs-panel'),
      languagePanel: this.container.querySelector('#language-panel'),
      helpPanel: this.container.querySelector('#help-panel'),
      form: this.container.querySelector('#login-form'),
      username: this.container.querySelector('#login-username'),
      password: this.container.querySelector('#login-password'),
      submit: this.container.querySelector('#login-submit'),
      languageBtn: this.container.querySelector('#login-language-btn'),
      themeBtn: this.container.querySelector('#login-theme-btn'),
      logsBtn: this.container.querySelector('#login-logs-btn'),
      helpBtn: this.container.querySelector('#login-help-btn'),
      clearUsernameBtn: this.container.querySelector('#login-clear-username'),
      togglePasswordBtn: this.container.querySelector('#login-toggle-password'),
      passwordIcon: this.container.querySelector('#login-toggle-password i'),
      themeCloseBtn: this.container.querySelector('#theme-close-btn'),
      logsCloseBtn: this.container.querySelector('#logs-close-btn'),
      languageCloseBtn: this.container.querySelector('#language-close-btn'),
      helpCloseBtn: this.container.querySelector('#help-close-btn'),
      error: this.container.querySelector('#login-error'),
      errorText: this.container.querySelector('#login-error-text'),
    };

    // Cache panels for transition management
    this.panels = {
      login: this.elements.loginPanel,
      theme: this.elements.themePanel,
      logs: this.elements.logsPanel,
      language: this.elements.languagePanel,
      help: this.elements.helpPanel,
    };

    // Add tooltip classes to buttons
    this.elements.submit?.classList.add('tooltip');
    this.elements.languageBtn?.classList.add('tooltip');
    this.elements.themeBtn?.classList.add('tooltip');
    this.elements.logsBtn?.classList.add('tooltip');
    this.elements.helpBtn?.classList.add('tooltip');
  }

  /**
   * Render a fallback login form if template fails to load
   */
  renderFallback() {
    this.container.innerHTML = `
      <div id="login-page">
        <div class="login-container" id="login-panel">
          <div class="login-header">
            <h1>Lithium</h1>
            <p>A Philement Project</p>
          </div>
          <form class="login-form" id="login-form">
            <div class="form-group">
              <div class="input-with-icon">
                <input type="text" id="login-username" placeholder="Username" required>
                <button type="button" class="input-icon-btn" id="login-clear-username" data-tooltip="Clear">
                  <fa fa-xmark></fa>
                </button>
              </div>
            </div>
            <div class="form-group">
              <div class="input-with-icon">
                <input type="password" id="login-password" placeholder="Password" required>
                <button type="button" class="input-icon-btn" id="login-toggle-password" data-tooltip="Show password">
                  <fa fa-eye></fa>
                </button>
              </div>
            </div>
            <div class="login-error" id="login-error">
              <span id="login-error-text">Error</span>
            </div>
            <div class="login-btn-group">
              <button type="button" class="login-btn-icon tooltip" id="login-language-btn" data-tooltip="Select language">
                <fa fa-globe></fa>
              </button>
              <button type="button" class="login-btn-icon tooltip" id="login-theme-btn" data-tooltip="Select theme">
                <fa fa-palette></fa>
              </button>
              <button type="submit" class="login-btn-primary tooltip" id="login-submit" data-tooltip="Login">
                <fa fa-sign-in-alt></fa>
                <span>Login</span>
              </button>
              <button type="button" class="login-btn-icon tooltip" id="login-logs-btn" data-tooltip="View Logs">
                <fa fa-scroll></fa>
              </button>
              <button type="button" class="login-btn-icon tooltip" id="login-help-btn" data-tooltip="Help">
                <fa fa-circle-question></fa>
              </button>
            </div>
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

    // Language button click - show language panel
    this.elements.languageBtn?.addEventListener('click', () => {
      this.switchPanel('language');
    });

    // Theme button click - show theme panel
    this.elements.themeBtn?.addEventListener('click', () => {
      this.switchPanel('theme');
    });

    // Logs button click - show logs panel
    this.elements.logsBtn?.addEventListener('click', () => {
      this.switchPanel('logs');
    });

    // Help button click - show help panel
    this.elements.helpBtn?.addEventListener('click', () => {
      this.switchPanel('help');
    });

    // Close buttons
    this.elements.themeCloseBtn?.addEventListener('click', () => {
      this.switchPanel('login');
    });

    this.elements.logsCloseBtn?.addEventListener('click', () => {
      this.switchPanel('login');
    });

    this.elements.languageCloseBtn?.addEventListener('click', () => {
      this.switchPanel('login');
    });

    this.elements.helpCloseBtn?.addEventListener('click', () => {
      this.switchPanel('login');
    });

    // Clear username button click
    this.elements.clearUsernameBtn?.addEventListener('click', () => {
      this.handleClearUsername();
    });

    // Toggle password visibility button click
    this.elements.togglePasswordBtn?.addEventListener('click', () => {
      this.handleTogglePassword();
    });

    // Global CAPS LOCK detection - monitor entire page
    this.handleKeyDown = (e) => {
      this.checkCapsLock(e);
      this.handleEscapeKey(e);
    };
    this.handleKeyUp = (e) => this.checkCapsLock(e);
    document.addEventListener('keydown', this.handleKeyDown);
    document.addEventListener('keyup', this.handleKeyUp);
  }

  /**
   * Handle ESC key to return to login panel from subpanels
   */
  handleEscapeKey(event) {
    if (event.key === 'Escape' && this.currentPanel !== 'login') {
      this.switchPanel('login');
    }
  }

  /**
   * Enable the transition overlay to block interactions
   */
  enableTransitionOverlay() {
    this.elements.overlay?.classList.add('active');
  }

  /**
   * Disable the transition overlay
   */
  disableTransitionOverlay() {
    this.elements.overlay?.classList.remove('active');
  }

  /**
   * Switch between panels with fade transition
   * @param {string} targetPanel - 'login', 'theme', or 'logs'
   */
  async switchPanel(targetPanel) {
    if (targetPanel === this.currentPanel) return;

    const fromPanel = this.panels[this.currentPanel];
    const toPanel = this.panels[targetPanel];

    if (!toPanel) return;

    // Block interactions during transition
    this.enableTransitionOverlay();

    // Perform crossfade transition
    await this.performPanelTransition(fromPanel, toPanel);

    // Update current panel state
    this.currentPanel = targetPanel;

    // Re-enable interactions
    this.disableTransitionOverlay();

    // Focus management
    if (targetPanel === 'login') {
      // Focus username when returning to login
      setTimeout(() => {
        this.elements.username?.focus();
      }, getTransitionDuration());
    }
  }

  /**
   * Perform the actual panel transition animation with crossfade
   */
  async performPanelTransition(fromElement, toElement) {
    const duration = getTransitionDuration();

    // Determine the correct display value for the target panel
    const isLoginPanel = toElement === this.elements.loginPanel;
    const targetDisplay = isLoginPanel ? 'block' : 'flex';

    // If we have both elements, do a true crossfade with absolute positioning
    if (fromElement && toElement) {
      // Get current dimensions before we modify anything
      const fromWidth = fromElement.offsetWidth;
      const fromHeight = fromElement.offsetHeight;
      const toWidth = toElement.offsetWidth || fromWidth; // fallback if not yet rendered
      const toHeight = toElement.offsetHeight || fromHeight;
      
      // Position both panels absolutely in the center of the container
      // This ensures they overlap perfectly during the crossfade
      const centerStyles = `
        position: absolute;
        top: 50%;
        left: 50%;
        transform: translate(-50%, -50%);
        margin: 0;
      `;
      
      // Apply absolute positioning to from element (keep visible during fade)
      fromElement.style.cssText = `
        ${centerStyles}
        width: ${fromWidth}px;
        height: ${fromHeight}px;
        display: ${fromElement.style.display || (fromElement === this.elements.loginPanel ? 'block' : 'flex')};
        opacity: 1;
        transition: opacity ${duration}ms ease-in-out;
        z-index: 1;
      `;
      
      // Prepare target element - also absolutely positioned in center
      toElement.style.cssText = `
        ${centerStyles}
        width: ${toElement === this.elements.loginPanel ? '360px' : '480px'};
        display: ${targetDisplay};
        opacity: 0;
        transition: opacity ${duration}ms ease-in-out;
        z-index: 2;
      `;
      
      // Force reflow
      void toElement.offsetHeight;

      // Start crossfade
      requestAnimationFrame(() => {
        // Fade out current panel
        fromElement.style.opacity = '0';
        
        // Fade in new panel (with slight delay for overlap effect)
        setTimeout(() => {
          toElement.style.opacity = '1';
        }, duration / 4);
      });

      // Wait for transition to complete
      await waitForTransition(duration + duration / 4 + 50);

      // Clean up from element
      fromElement.style.cssText = 'display: none;';
      
      // Restore target element to normal flow (remove absolute positioning)
      // Keep its natural display and size
      toElement.style.cssText = '';
      toElement.style.display = targetDisplay;
      toElement.style.opacity = '1';
      toElement.classList.add('visible');
    } else if (toElement) {
      // No from element, just fade in normally
      toElement.style.display = targetDisplay;
      void toElement.offsetHeight;
      requestAnimationFrame(() => {
        toElement.classList.add('visible');
      });
      await waitForTransition(duration);
    }
  }

  /**
   * Check if CAPS LOCK is on
   * Called on any key event anywhere on the page
   */
  checkCapsLock(event) {
    // getModifierState is the standard way to detect CAPS LOCK state
    // It returns true if CAPS LOCK is currently on, regardless of which key was pressed
    const capsLockOn = event.getModifierState && event.getModifierState('CapsLock');
    
    // Only update if the state has changed
    if (this.isCapsLockOn !== capsLockOn) {
      this.isCapsLockOn = capsLockOn;
      this.updateCapsLockIndicator(capsLockOn);
    }
  }

  /**
   * Update the password field background based on CAPS LOCK state
   */
  updateCapsLockIndicator(isCapsLockOn) {
    if (this.elements.password) {
      if (isCapsLockOn) {
        this.elements.password.classList.add('caps-lock-active');
      } else {
        this.elements.password.classList.remove('caps-lock-active');
      }
    }
  }

  /**
   * Handle clear username button click
   * Clears both username and password, then focuses username for immediate typing
   */
  handleClearUsername() {
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
   * Handle toggle password visibility button click
   */
  handleTogglePassword() {
    if (!this.elements.password || !this.elements.passwordIcon) return;

    this.isPasswordVisible = !this.isPasswordVisible;

    if (this.isPasswordVisible) {
      // Show password
      this.elements.password.type = 'text';
      this.elements.passwordIcon.classList.remove('fa-eye');
      this.elements.passwordIcon.classList.add('fa-eye-slash');
      this.elements.togglePasswordBtn?.setAttribute('data-tooltip', 'Hide password');
    } else {
      // Hide password
      this.elements.password.type = 'password';
      this.elements.passwordIcon.classList.remove('fa-eye-slash');
      this.elements.passwordIcon.classList.add('fa-eye');
      this.elements.togglePasswordBtn?.setAttribute('data-tooltip', 'Show password');
    }
  }

  /**
   * Show the login page with fade-in
   * @param {boolean} skipUsernameFocus - If true, focus password (username has value)
   */
  async show(skipUsernameFocus = false) {
    if (this.elements.loginPanel) {
      // Step 1: Set display to block (but opacity is still 0 from CSS)
      this.elements.loginPanel.style.display = 'block';

      // Step 2: Force reflow to ensure display is applied
      void this.elements.loginPanel.offsetHeight;

      // Step 3: Add visible class to trigger opacity transition from 0 to 1
      requestAnimationFrame(() => {
        this.elements.loginPanel.classList.add('visible');
      });
    }

    // Focus appropriate field after transition completes
    // Use a slightly longer delay to ensure transition is fully done and element is interactive
    setTimeout(() => {
      if (skipUsernameFocus && this.elements.password) {
        // Username was pre-filled, focus password
        this.elements.password.focus();
      } else if (this.elements.username) {
        // No username or empty, focus username
        this.elements.username.focus();
      }
    }, getTransitionDuration() + 50);
  }

  /**
   * Hide the login page with fade-out
   */
  async hide() {
    return new Promise((resolve) => {
      const duration = getTransitionDuration();
      
      // Fade out the current panel only
      // Let the app handle the transition to main manager
      const currentPanelEl = this.panels[this.currentPanel];
      if (currentPanelEl) {
        currentPanelEl.style.transition = `opacity ${duration}ms ease-in-out`;
        currentPanelEl.style.opacity = '0';
      }
      
      setTimeout(resolve, duration);
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

    // Remember username for next visit
    this.saveRememberedUsername(username);

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
        ? '<fa fa-spinner fa-spin></fa> <span>Logging in...</span>'
        : '<fa fa-sign-in-alt></fa> <span>Login</span>';
    }

    // Disable icon buttons during loading
    // Note: Language, Theme, and Logs buttons are also controlled by lookup availability
    // We'll respect the lookup-based state when not loading
    if (this.elements.languageBtn) {
      this.elements.languageBtn.disabled = loading || !hasLookup('lookup_names');
    }
    if (this.elements.themeBtn) {
      this.elements.themeBtn.disabled = loading || !hasLookup('themes');
    }
    if (this.elements.logsBtn) {
      this.elements.logsBtn.disabled = loading || !hasLookup('system_info');
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

  /**
   * Teardown the login manager
   */
  teardown() {
    // Remove global event listeners
    if (this.handleKeyDown) {
      document.removeEventListener('keydown', this.handleKeyDown);
    }
    if (this.handleKeyUp) {
      document.removeEventListener('keyup', this.handleKeyUp);
    }

    // Remove lookup event listeners
    this.lookupListeners.forEach(unsubscribe => unsubscribe());
    this.lookupListeners = [];

    // Clear references
    this.elements = {};
    this.panels = {};
    this.isPasswordVisible = false;
    this.isCapsLockOn = false;
    this.currentPanel = 'login';
    this.handleKeyDown = null;
    this.handleKeyUp = null;
  }
}
