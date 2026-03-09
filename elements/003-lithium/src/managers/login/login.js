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
import { log, logGroup, getRawLog, Subsystems, Status } from '../../core/log.js';
import './login.css';

// Convenience alias for this module's subsystem
const LOGIN = Subsystems.LOGIN;

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
    this._startupComplete = false; // Tracks whether background startup has finished
    this._loginFocusTimer = null; // Timer ID for the post-return-to-login focus delay
  }

  /**
   * Initialize the login manager
   */
  async init() {
    await this.render();
    this.setupEventListeners();
    this.setupLookupListeners();
    this.initializeButtonStates();
    this.loadVersionInfo();

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
    // Logs button starts disabled; enabled once background startup completes
    this.setLogsButtonEnabled(false);

    // If lookups are already loaded, enable buttons immediately
    if (hasLookup('lookup_names')) {
      this.setLanguageButtonEnabled(true);
    }
    if (hasLookup('themes')) {
      this.setThemeButtonEnabled(true);
    }
  }

  /**
   * Load version information from version.json and display it
   * Populates the login header version box and the help panel version/build fields
   * Uses cached data from early fetch in index.html if available
   */
  async loadVersionInfo() {
    try {
      let versionData;
      
      // Check if we have cached version data from the early fetch in index.html
      if (window.__lithiumVersionData) {
        versionData = window.__lithiumVersionData;
      } else if (window.__lithiumVersionPromise) {
        // Wait for the early fetch promise to resolve
        versionData = await window.__lithiumVersionPromise;
      } else {
        // Fallback: fetch version.json directly
        const response = await fetch('/version.json');
        if (!response.ok) return;
        versionData = await response.json();
      }
      
      if (!versionData) return;
      
      const { build, timestamp, version } = versionData;

      // Update version info box in login header (build, YYYY, MMDD, HHMM)
      if (this.elements.versionBuild && timestamp) {
        const date = new Date(timestamp);
        const year = date.getFullYear();
        const month = String(date.getMonth() + 1).padStart(2, '0');
        const day = String(date.getDate()).padStart(2, '0');
        const hours = String(date.getHours()).padStart(2, '0');
        const minutes = String(date.getMinutes()).padStart(2, '0');

        this.elements.versionBuild.textContent = build;
        this.elements.versionYear.textContent = year;
        this.elements.versionDate.textContent = month + day;
        this.elements.versionTime.textContent = hours + minutes;
      }

      // Update help panel version and build date
      if (this.elements.helpAppVersion) {
        this.elements.helpAppVersion.textContent = version || `0.1.${build}`;
      }
      if (this.elements.helpBuildDate) {
        // Format timestamp for help panel
        let buildDate = '';
        if (timestamp) {
          const date = new Date(timestamp);
          const months = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'];
          const hours = String(date.getUTCHours()).padStart(2, '0');
          const minutes = String(date.getUTCMinutes()).padStart(2, '0');
          buildDate = `${date.getFullYear()}-${months[date.getUTCMonth()]}-${String(date.getUTCDate()).padStart(2, '0')} ${hours}:${minutes} UTC`;
        }
        this.elements.helpBuildDate.textContent = buildDate || timestamp;
      }
    } catch (error) {
      console.warn('[LoginManager] Could not load version info:', error);
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

    // Listen for startup complete - enable logs button once background init is done
    const handleStartupComplete = () => {
      this._startupComplete = true;
      this.setLogsButtonEnabled(true);
    };

    // Subscribe to specific lookup events
    eventBus.on(Events.LOOKUPS_LOOKUP_NAMES_LOADED, handleLookupNamesLoaded);
    eventBus.on(Events.LOOKUPS_THEMES_LOADED, handleThemesLoaded);
    eventBus.once(Events.STARTUP_COMPLETE, handleStartupComplete);

    // Store unsubscribe functions for cleanup
    this.lookupListeners.push(() => eventBus.off(Events.LOOKUPS_LOOKUP_NAMES_LOADED, handleLookupNamesLoaded));
    this.lookupListeners.push(() => eventBus.off(Events.LOOKUPS_THEMES_LOADED, handleThemesLoaded));
    this.lookupListeners.push(() => eventBus.off(Events.STARTUP_COMPLETE, handleStartupComplete));
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
      log(LOGIN, Status.INFO, `Auto-login: credentials detected in URL for user "${username}"`);
      
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
      logViewer: this.container.querySelector('#log-viewer'),
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
      versionBox: this.container.querySelector('#login-version-box'),
      versionBuild: this.container.querySelector('#login-version-build'),
      versionYear: this.container.querySelector('#login-version-year'),
      versionDate: this.container.querySelector('#login-version-date'),
      versionTime: this.container.querySelector('#login-version-time'),
      helpAppVersion: this.container.querySelector('#help-app-version'),
      helpBuildDate: this.container.querySelector('#help-build-date'),
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
              <button type="button" class="login-btn-icon tooltip" id="login-logs-btn" data-tooltip="View Session Log">
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

    // Clear error on input; log field-level interactions on blur (not on every keystroke)
    this.elements.username?.addEventListener('input', () => this.hideError());
    this.elements.password?.addEventListener('input', () => this.hideError());

    this.elements.username?.addEventListener('blur', () => {
      const hasValue = !!(this.elements.username?.value?.trim());
      log(LOGIN, Status.INFO, `Username field: ${hasValue ? 'filled' : 'empty'}`);
    });

    this.elements.password?.addEventListener('blur', () => {
      const hasValue = !!(this.elements.password?.value);
      log(LOGIN, Status.INFO, `Password field: ${hasValue ? 'filled' : 'empty'}`);
    });

    // Language button click - show language panel
    this.elements.languageBtn?.addEventListener('click', () => {
      log(LOGIN, Status.INFO, 'Button clicked: Language');
      this.switchPanel('language');
    });

    // Theme button click - show theme panel
    this.elements.themeBtn?.addEventListener('click', () => {
      log(LOGIN, Status.INFO, 'Button clicked: Theme');
      this.switchPanel('theme');
    });

    // Logs button click - show logs panel
    this.elements.logsBtn?.addEventListener('click', () => {
      log(LOGIN, Status.INFO, 'Button clicked: Logs');
      this.switchPanel('logs');
    });

    // Help button click - show help panel
    this.elements.helpBtn?.addEventListener('click', () => {
      log(LOGIN, Status.INFO, 'Button clicked: Help');
      this.switchPanel('help');
    });

    // Close buttons
    this.elements.themeCloseBtn?.addEventListener('click', () => {
      log(LOGIN, Status.INFO, 'Button clicked: Theme Close');
      this.switchPanel('login');
    });

    this.elements.logsCloseBtn?.addEventListener('click', () => {
      log(LOGIN, Status.INFO, 'Button clicked: Logs Close');
      this.switchPanel('login');
    });

    this.elements.languageCloseBtn?.addEventListener('click', () => {
      log(LOGIN, Status.INFO, 'Button clicked: Language Close');
      this.switchPanel('login');
    });

    this.elements.helpCloseBtn?.addEventListener('click', () => {
      log(LOGIN, Status.INFO, 'Button clicked: Help Close');
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

    // Set up keyboard shortcuts (only active on login panel)
    this.setupKeyboardShortcuts();
  }

  /**
   * Handle all keyboard shortcuts
   * ESC - clear username and password and focus username (when on login panel)
   * Ctrl+Shift+U - focus username field and select its contents
   * Ctrl+Shift+P - focus password field and select its contents
   * F1 - click help button
   * Ctrl+Shift+I - click language button
   * Ctrl+Shift+T - click theme button
   * Ctrl+Shift+L - click log button
   * 
   * Note: Shortcuts (except ESC) only work when on the login panel
   */
  handleKeyboardShortcuts(event) {
    const key = event.key;
    const ctrl = event.ctrlKey;
    const shift = event.shiftKey;
    const alt = event.altKey;
    const isOnLoginPanel = this.currentPanel === 'login';
    
    // ESC - always works to return to login panel or clear fields
    if (key === 'Escape') {
      if (isOnLoginPanel) {
        // Clear username and password, focus username
        this.handleClearUsername();
        event.preventDefault();
      } else {
        // Switch to login panel from subpanels
        log(LOGIN, Status.INFO, `Keyboard: ESC closed ${this.currentPanel} panel`);
        this.switchPanel('login');
        event.preventDefault();
      }
      return;
    }
    
    // All other shortcuts only work when on login panel
    if (!isOnLoginPanel) {
      return;
    }
    
    // Ctrl+Shift+U - focus username and select contents
    if (ctrl && shift && key.toLowerCase() === 'u') {
      if (this.elements.username) {
        log(LOGIN, Status.INFO, 'Keyboard: Ctrl+Shift+U focused username field');
        this.elements.username.focus();
        this.elements.username.select();
      }
      event.preventDefault();
      return;
    }
    
    // Ctrl+Shift+P - focus password and select contents
    if (ctrl && shift && key.toLowerCase() === 'p') {
      if (this.elements.password) {
        log(LOGIN, Status.INFO, 'Keyboard: Ctrl+Shift+P focused password field');
        this.elements.password.focus();
        this.elements.password.select();
      }
      event.preventDefault();
      return;
    }
    
    // F1 - click help button
    if (key === 'F1') {
      if (this.elements.helpBtn && !this.elements.helpBtn.disabled) {
        this.elements.helpBtn.click();
      }
      event.preventDefault();
      return;
    }
    
    // Ctrl+Shift+I - click language (internationalization) button
    if (ctrl && shift && key.toLowerCase() === 'i') {
      if (this.elements.languageBtn && !this.elements.languageBtn.disabled) {
        this.elements.languageBtn.click();
      }
      event.preventDefault();
      return;
    }
    
    // Ctrl+Shift+T - click theme button
    if (ctrl && shift && key.toLowerCase() === 't') {
      if (this.elements.themeBtn && !this.elements.themeBtn.disabled) {
        this.elements.themeBtn.click();
      }
      event.preventDefault();
      return;
    }
    
    // Ctrl+Shift+L - click log button
    if (ctrl && shift && key.toLowerCase() === 'l') {
      if (this.elements.logsBtn && !this.elements.logsBtn.disabled) {
        this.elements.logsBtn.click();
      }
      event.preventDefault();
      return;
    }
  }

  /**
   * Handle ESC key to return to login panel from subpanels
   * @deprecated Use handleKeyboardShortcuts instead
   */
  handleEscapeKey(event) {
    if (event.key === 'Escape' && this.currentPanel !== 'login') {
      this.switchPanel('login');
    }
  }

  /**
   * Set up keyboard shortcut event listeners
   */
  setupKeyboardShortcuts() {
    // Only add if not already added
    if (this.handleKeyDown) return;
    
    this.handleKeyDown = (e) => {
      this.checkCapsLock(e);
      this.handleKeyboardShortcuts(e);
    };
    this.handleKeyUp = (e) => this.checkCapsLock(e);
    document.addEventListener('keydown', this.handleKeyDown);
    document.addEventListener('keyup', this.handleKeyUp);
  }

  /**
   * Remove keyboard shortcut event listeners
   */
  removeKeyboardShortcuts() {
    if (this.handleKeyDown) {
      document.removeEventListener('keydown', this.handleKeyDown);
      this.handleKeyDown = null;
    }
    if (this.handleKeyUp) {
      document.removeEventListener('keyup', this.handleKeyUp);
      this.handleKeyUp = null;
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
   * Password manager UI toggle state
   */
  _passwordManagerObserver = null;

  /**
   * Toggle password manager UI visibility
   * Uses body class approach with MutationObserver for robustness
   * @param {boolean} hide - True to hide, false to show
   */
  togglePasswordManagerUI(hide) {
    const body = document.body;
    
    if (hide) {
      body.classList.add('hide-password-manager-ui');
      
      // Selectors for password manager injected elements.
      const selectors = [
        'com-1password-button',
        '[id*="1p-"]',
        '[class*="lastpass"]',
        '[class*="bitwarden"]',
        '[data-bitwarden]',
        '[class*="dashlane"]',
        '[data-dashlane]',
      ];

      const suppressElements = () => {
        // Temporarily disconnect the observer while we mutate styles so that our
        // own setAttribute calls do not re-trigger this callback (infinite loop).
        // We immediately reconnect after the sweep so new 1Password injections
        // are still caught.
        this._passwordManagerObserver?.disconnect();
        try {
          for (const selector of selectors) {
            try {
              document.querySelectorAll(selector).forEach(el => {
                el.style.setProperty('display', 'none', 'important');
                el.style.setProperty('visibility', 'hidden', 'important');
                el.style.setProperty('opacity', '0', 'important');
                el.style.setProperty('pointer-events', 'none', 'important');
              });
            } catch (e) {
              // Invalid selector, skip
            }
          }
        } finally {
          // Reconnect so future 1Password re-injections/re-shows are caught.
          // Guard against calling observe() after teardown (observer may be null).
          if (this._passwordManagerObserver) {
            this._passwordManagerObserver.observe(document.body, {
              childList: true,
              subtree: true,
              attributes: true,
              attributeFilter: ['style', 'class'],
            });
          }
        }
      };

      // Create the observer (initially disconnected; suppressElements reconnects it).
      this._passwordManagerObserver = new MutationObserver(suppressElements);

      // Run the initial suppression sweep, which also starts the observer.
      suppressElements();

      // Schedule a follow-up sweep on the next animation frame so we get the
      // last word after any 1Password re-assertion in the same microtask flush.
      requestAnimationFrame(suppressElements);
    } else {
      body.classList.remove('hide-password-manager-ui');
      this._passwordManagerObserver?.disconnect();
      this._passwordManagerObserver = null;
    }
  }

  /**
   * Remove 1Password (and other password manager) injected elements from the DOM.
   * Called after a successful login — at that point we want the elements gone
   * entirely, not just hidden, so they do not linger in the post-login view.
   */
  removePasswordManagerElements() {
    const selectors = [
      'com-1password-button',
      '[id*="1p-"]',
      '[class*="lastpass"]',
      '[class*="bitwarden"]',
      '[data-bitwarden]',
      '[class*="dashlane"]',
      '[data-dashlane]',
    ];
    for (const selector of selectors) {
      try {
        document.querySelectorAll(selector).forEach(el => el.remove());
      } catch (e) {
        // Invalid selector, skip
      }
    }
  }

  /**
   * Hide password manager injected UI elements (legacy method, kept for compatibility)
   */
  hidePasswordManagerElements() {
    this.togglePasswordManagerUI(true);
  }

  /**
   * Switch between panels with fade transition
   * @param {string} targetPanel - 'login', 'theme', or 'logs'
   */
  async switchPanel(targetPanel) {
    if (targetPanel === this.currentPanel) return;
    const panelStart = performance.now();

    const fromPanel = this.panels[this.currentPanel];
    const toPanel = this.panels[targetPanel];

    if (!toPanel) return;

    // Cancel any pending focus-on-login timer from a previous switchPanel('login') call.
    // This prevents a stale focus from firing on the wrong panel if the user navigates
    // away before the delay expires (which would trigger 1Password on a subpanel).
    if (this._loginFocusTimer !== null) {
      clearTimeout(this._loginFocusTimer);
      this._loginFocusTimer = null;
    }

    // Hide password manager UI when leaving login panel, show when returning
    if (targetPanel === 'login') {
      // Returning to login - re-enable password manager UI
      this.togglePasswordManagerUI(false);
    } else {
      // Going to subpanel - hide password manager UI
      this.togglePasswordManagerUI(true);
    }

    // Block interactions during transition
    this.enableTransitionOverlay();

    // If switching to logs panel, populate it with action logs
    if (targetPanel === 'logs') {
      this.populateLogsPanel();
    }

    // Perform crossfade transition
    await this.performPanelTransition(fromPanel, toPanel);

    // Update current panel state
    const fromPanelName = this.currentPanel;
    this.currentPanel = targetPanel;
    const panelDuration = Math.round(performance.now() - panelStart);
    log(LOGIN, Status.INFO, `Panel: ${fromPanelName} → ${targetPanel}`, panelDuration);

    // Re-enable interactions
    this.disableTransitionOverlay();

    // Focus management
    if (targetPanel === 'login') {
      // Delay the focus slightly to let the transition fully settle, then check
      // we are still on the login panel before focusing.  Storing the timer ID
      // lets any subsequent switchPanel call cancel it immediately, so 1Password
      // is never triggered by a focus event that fires on the wrong panel.
      this._loginFocusTimer = setTimeout(() => {
        this._loginFocusTimer = null;
        if (this.currentPanel === 'login') {
          this.elements.username?.focus();
        }
      }, getTransitionDuration());
    }
  }

  /**
   * Populate the logs panel with action logs using CodeMirror
   */
  async populateLogsPanel() {
    const logViewer = this.elements.logViewer;
    if (!logViewer) {
      console.warn('[LoginManager] logViewer element not found');
      return;
    }

    const entries = getRawLog();

    let logText;
    if (entries.length === 0) {
      logText = '(No log entries yet)';
    } else {
      // Determine max subsystem width for fixed-width column alignment.
      // Bracketed entries (EventBus) are excluded from this calculation because their
      // brackets already account for the two-space column separator.
      const maxSubsystemLen = entries.reduce((max, e) => {
        const s = e.subsystem || '';
        return s.startsWith('[') ? max : Math.max(max, s.length);
      }, 0);

      // Build display lines, expanding grouped entries ({ title, items }) to multiple lines.
      // Subsystems stored with bracket notation (e.g. "[Lookups]") are EventBus entries:
      // they are rendered without extra padding because the brackets consume the two
      // separator spaces, keeping columns visually aligned with plain subsystem names.
      const lines = [];
      for (const entry of entries) {
        const date = new Date(entry.timestamp);
        const time = String(date.getHours()).padStart(2, '0') + ':' +
          String(date.getMinutes()).padStart(2, '0') + ':' +
          String(date.getSeconds()).padStart(2, '0') + '.' +
          String(date.getMilliseconds()).padStart(3, '0');
        const raw = entry.subsystem || '';
        const isBracketed = raw.startsWith('[');
        // Bracketed entries (EventBus) use a single space before and after the label.
        // The two bracket characters replace the two padding spaces from plain entries,
        // keeping columns visually aligned:
        //   time  Lookups  desc  →  pre(2) + name(7) + sep(2) = 11 chars before desc
        //   time [Lookups] desc  →  pre(1) + name(9) + sep(1) = 11 chars before desc ✓
        const subsystem = isBracketed ? raw : raw.padEnd(maxSubsystemLen);
        const pre = isBracketed ? ' ' : '  ';
        const sep = isBracketed ? ' ' : '  ';
        const desc = entry.description;

        if (desc && typeof desc === 'object' && desc.title !== undefined && Array.isArray(desc.items)) {
          // Grouped entry: render title + ― continuation lines
          lines.push(`${time}${pre}${subsystem}${sep}${desc.title}`);
          for (const item of desc.items) {
            lines.push(`${time}${pre}${subsystem}${sep}― ${item}`);
          }
        } else {
          lines.push(`${time}${pre}${subsystem}${sep}${desc}`);
        }
      }
      logText = lines.join('\n');
    }

    // If CodeMirror is already initialized, just update the content
    if (this._logEditor) {
      this._logEditor.dispatch({
        changes: { from: 0, to: this._logEditor.state.doc.length, insert: logText }
      });
      return;
    }

    // Build a proper CodeMirror 6 editor from the individual @codemirror packages
    // (there is no top-level 'codemirror' meta-package in this project)
    // SQL language is intentionally loaded: logs may contain SQL content in future,
    // and this also pre-warms the module for the SQL editor elsewhere in the app.
    try {
      const [
        { EditorView, lineNumbers, highlightActiveLine, highlightActiveLineGutter, drawSelection, keymap },
        { EditorState },
        { defaultKeymap, historyKeymap, history },
        { oneDark },
        { sql },
      ] = await Promise.all([
        import('@codemirror/view'),
        import('@codemirror/state'),
        import('@codemirror/commands'),
        import('@codemirror/theme-one-dark'),
        import('@codemirror/lang-sql'),
      ]);

      // Custom 4-digit zero-padded line number formatter
      const zeroPaddedLineNumbers = lineNumbers({
        formatNumber: (n) => String(n).padStart(4, '0'),
      });

      const extensions = [
        // Visual chrome
        zeroPaddedLineNumbers,
        highlightActiveLine(),
        highlightActiveLineGutter(),
        drawSelection(),
        history(),
        keymap.of([...defaultKeymap, ...historyKeymap]),

        // SQL language (pre-warms module; also highlights any SQL snippets in logs)
        sql(),

        // Dark theme
        oneDark,

        // Read-only
        EditorState.readOnly.of(true),

        // Sizing and font
        EditorView.theme({
          '&': {
            height: '100%',
            fontSize: '10px',
            fontFamily: "'Vanadium Mono', 'Courier New', Courier, monospace",
          },
          '.cm-scroller': {
            overflow: 'auto',
            scrollbarWidth: 'thin',       // Firefox: always show thin scrollbar
            scrollbarColor: '#555 #222',  // Firefox: thumb / track
          },
          // WebKit: always-visible scrollbars
          '.cm-scroller::-webkit-scrollbar': { width: '8px', height: '8px' },
          '.cm-scroller::-webkit-scrollbar-track': { background: '#1e1e2e' },
          '.cm-scroller::-webkit-scrollbar-thumb': { background: '#555', borderRadius: '4px' },
          '.cm-scroller::-webkit-scrollbar-corner': { background: '#1e1e2e' },
          '.cm-content': { whiteSpace: 'pre' },
        }),
      ];

      const state = EditorState.create({ doc: logText, extensions });

      logViewer.innerHTML = '';
      this._logEditor = new EditorView({ state, parent: logViewer });
    } catch (error) {
      console.warn('[LoginManager] CodeMirror failed to load, using plain text:', error);
      logViewer.innerHTML = `<pre class="log-content">${logText}</pre>`;
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
   * Handle toggle password visibility button click
   */
  handleTogglePassword() {
    if (!this.elements.password || !this.elements.passwordIcon) return;

    this.isPasswordVisible = !this.isPasswordVisible;
    log(LOGIN, Status.INFO, `Password visibility: ${this.isPasswordVisible ? 'shown' : 'hidden'}`);

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
    // Enable password manager UI when showing login
    this.togglePasswordManagerUI(false);
    
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
      
      // Hide password manager UI elements before fading out to main.
      // This adds the body class, starts the MutationObserver, and sweeps
      // any existing elements so they disappear immediately.
      this.togglePasswordManagerUI(true);

      // Physically remove all injected password manager elements from the DOM.
      // This ensures they do not linger in the post-login view as invisible orphan
      // nodes (e.g. 1Password's live-region divs) and that 1Password cannot simply
      // re-show them by attribute mutation without re-inserting them first.
      this.removePasswordManagerElements();
      
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

    // Success — log and emit login event
    const loginDuration = this._loginStart ? Math.round(performance.now() - this._loginStart) : 0;
    this._loginStart = null;
    log(LOGIN, Status.SUCCESS, `Login successful: user "${username}"`, loginDuration);
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
    const loginDuration = this._loginStart ? Math.round(performance.now() - this._loginStart) : 0;
    this._loginStart = null;
    log(LOGIN, Status.ERROR, `Login failed: HTTP ${error.status || error.message}`, loginDuration);
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
      // Logs button is gated on both loading state and startup completion
      this.elements.logsBtn.disabled = loading || !this._startupComplete;
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
    // Clean up CodeMirror editor if present
    if (this._logEditor) {
      this._logEditor.destroy();
      this._logEditor = null;
    }

    // Cancel any pending post-login-return focus timer
    if (this._loginFocusTimer !== null) {
      clearTimeout(this._loginFocusTimer);
      this._loginFocusTimer = null;
    }

    // Remove keyboard shortcut event listeners
    this.removeKeyboardShortcuts();

    // Clean up password manager UI observer and remove any lingering injected elements
    this._passwordManagerObserver?.disconnect();
    this._passwordManagerObserver = null;
    document.body.classList.remove('hide-password-manager-ui');
    this.removePasswordManagerElements();

    // Remove lookup event listeners
    this.lookupListeners.forEach(unsubscribe => unsubscribe());
    this.lookupListeners = [];

    // Clear references
    this.elements = {};
    this.panels = {};
    this.isPasswordVisible = false;
    this.isCapsLockOn = false;
    this.currentPanel = 'login';
  }
}
