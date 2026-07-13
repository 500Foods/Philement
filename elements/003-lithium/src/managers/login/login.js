/**
 * Login Manager
 * 
 * Handles user authentication against the Hydrogen server.
 * Loads CCC-style login page, validates credentials, stores JWT.
 * Features: Theme selector, Log viewer with fade transitions
 */

import { eventBus, Events } from '../../core/event-bus.js';
import { getTransitionDuration } from '../../core/transitions.js';
import { LoginPanels } from './login-panels.js';
import { hasLookup } from '../../shared/lookups.js';
import { log, Subsystems, Status } from '../../core/log.js';
import { PasswordManager } from './login-password-manager.js';
import { LanguagePanel } from './login-language-panel.js';
import { LogsPanel } from './login-logs-panel.js';
import { HelpPanel } from './login-help-panel.js';
import { LoginForm } from './login-form.js';
import { LoginShortcuts } from './login-shortcuts.js';
import { processOidcReturn } from './oidc-login.js';
import { startOidc } from '../../core/oidc-client.js';
import { getConfigValue } from '../../core/config.js';
import { retrieveJWT } from '../../core/jwt.js';
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
    this.lookupListeners = [];
    this._startupComplete = false; // Tracks whether background startup has finished
    this._loginPanels = null; // LoginPanels instance (initialized in render)
    this._passwordManager = null; // PasswordManager instance (initialized in render)
    this._languagePanel = null; // LanguagePanel instance (initialized in render)
    this._logsPanel = null; // LogsPanel instance (initialized in render)
    this._helpPanel = null; // HelpPanel instance (initialized in render)
    this._loginForm = null; // LoginForm instance (initialized in render)
    this._shortcuts = null; // LoginShortcuts instance (initialized in render)
  }

  /**
   * Initialize the login manager
   */
  async init() {
    await this.render();
    this.setupEventListeners();
    this.setupLookupListeners();
    this.initializeButtonStates();
    this._helpPanel?.init();

    // Handle OIDC return-from-IdP before showing the login form.
    // If the URL contains ?oidc=1, processOidcReturn handles the entire
    // login completion (exchange handoff → store JWT → hide → AUTH_LOGIN)
    // or shows an error message, and returns true to signal we're done.
    // In the success case the manager hides itself; in the error case
    // the caller still needs to show the login form for a retry.
    const wasOidcReturn = await processOidcReturn(this);
    if (wasOidcReturn) {
      // On a successful OIDC return, hide() was called inside
      // processOidcReturn and AUTH_LOGIN was emitted — nothing more to do.
      // On an error return, showError() was called and the form needs to be
      // shown so the user can retry; fall through to the show() call below.
      // We check whether the JWT was stored to decide which case we are in.
      if (retrieveJWT()) return;
    }

    // Load remembered username before showing (sets initial values)
    const hadRememberedUsername = this._loginForm?.loadRememberedUsername() || false;

    // Show the panel with fade-in (pass focus info to avoid double focus)
    await this.show(hadRememberedUsername);

    // Check for URL query parameters for auto-login
    this.checkForAutoLogin();
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

    if (username && password && this._loginForm) {
      log(LOGIN, Status.INFO, `Auto-login: credentials detected in URL for user "${username}"`);
      this._loginForm.prefillAndSubmit(username, password);
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
      versionBox: this.container.querySelector('#login-version-btn'),
      versionBuild: this.container.querySelector('#login-version-build'),
      versionYear: this.container.querySelector('#login-version-year'),
      versionDate: this.container.querySelector('#login-version-date'),
      versionTime: this.container.querySelector('#login-version-time'),
      helpAppVersion: this.container.querySelector('#help-app-version'),
      helpBuildDate: this.container.querySelector('#help-build-date'),
      languageFilter: this.container.querySelector('#language-filter'),
      languageClearFilter: this.container.querySelector('#language-clear-filter'),
      languageTable: this.container.querySelector('#language-table'),
      languageCurrentBtn: this.container.querySelector('#language-current-btn'),
      languageCurrentFlag: this.container.querySelector('#language-current-flag'),
      languageCurrentCode: this.container.querySelector('#language-current-code'),
      oidcDivider: this.container.querySelector('#login-oidc-divider'),
      oidcProviders: this.container.querySelector('#login-providers'),
      altButtonGroup: this.container.querySelector('#login-alt-btn-group'),
    };

    // Initialize PasswordManager
    this._passwordManager = new PasswordManager();

    // Initialize LanguagePanel
    this._languagePanel = new LanguagePanel({
      elements: {
        languageTable: this.elements.languageTable,
        languageFilter: this.elements.languageFilter,
        languageClearFilter: this.elements.languageClearFilter,
        languageCurrentBtn: this.elements.languageCurrentBtn,
        languageCurrentFlag: this.elements.languageCurrentFlag,
        languageCurrentCode: this.elements.languageCurrentCode,
      },
      onLocaleSelected: (locale, previousLocale) => {
        // Locale selection is handled entirely within LanguagePanel;
        // this callback exists for future extension if LoginManager
        // needs to react to locale changes.
      },
    });

    // Initialize LogsPanel
    this._logsPanel = new LogsPanel({
      elements: { logViewer: this.elements.logViewer },
    });

    // Initialize HelpPanel (loads and renders version info on init)
    this._helpPanel = new HelpPanel({
      elements: {
        versionBuild: this.elements.versionBuild,
        versionYear: this.elements.versionYear,
        versionDate: this.elements.versionDate,
        versionTime: this.elements.versionTime,
        helpAppVersion: this.elements.helpAppVersion,
        helpBuildDate: this.elements.helpBuildDate,
      },
    });

    // Initialize LoginForm (owns submit, attemptLogin, error mapping,
    // password visibility, remembered username, etc.)
    this._loginForm = new LoginForm({
      elements: {
        form: this.elements.form,
        username: this.elements.username,
        password: this.elements.password,
        submit: this.elements.submit,
        error: this.elements.error,
        errorText: this.elements.errorText,
        clearUsernameBtn: this.elements.clearUsernameBtn,
        togglePasswordBtn: this.elements.togglePasswordBtn,
        passwordIcon: this.elements.passwordIcon,
        languageBtn: this.elements.languageBtn,
        themeBtn: this.elements.themeBtn,
        logsBtn: this.elements.logsBtn,
        helpBtn: this.elements.helpBtn,
      },
      onLoginSuccess: async () => {
        await this.hide();
      },
      isStartupComplete: () => this._startupComplete,
    });

    // Initialize LoginPanels with callbacks for panel-specific logic
    this._loginPanels = new LoginPanels({
      panels: {
        login: this.elements.loginPanel,
        theme: this.elements.themePanel,
        logs: this.elements.logsPanel,
        language: this.elements.languagePanel,
        help: this.elements.helpPanel,
      },
      loginPanel: this.elements.loginPanel,
      overlay: this.elements.overlay,
      onBeforeSwitch: (targetPanel) => this._onBeforePanelSwitch(targetPanel),
      onAfterSwitch: (fromPanel, toPanel, duration) => this._onAfterPanelSwitch(fromPanel, toPanel, duration),
    });

    // Initialize LoginShortcuts (keyboard handlers + CAPS LOCK indicator).
    this._shortcuts = new LoginShortcuts({
      elements: {
        username: this.elements.username,
        password: this.elements.password,
        helpBtn: this.elements.helpBtn,
        languageBtn: this.elements.languageBtn,
        themeBtn: this.elements.themeBtn,
        logsBtn: this.elements.logsBtn,
      },
      getCurrentPanel: () => this._loginPanels?.currentPanel,
      onClearUsername: () => this._loginForm?.handleClearUsername(),
      onSwitchToLogin: () => this.switchPanel('login'),
    });

    // Render OIDC provider buttons from config (no-op if array is empty).
    this.renderOidcProviders();

    // Subtly highlight the last-used login method (Phase 26).
    this.applyPasswordRecentHighlight();
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
          <form class="login-form" id="login-form" novalidate>
            <div class="form-group">
              <div class="input-with-icon">
                <input type="text" id="login-username" placeholder="Username" required>
                <button type="button" class="input-icon-btn" id="login-clear-username" data-tooltip="Clear" data-tip-placement="right">
                  <fa fa-xmark></fa>
                </button>
              </div>
            </div>
            <div class="form-group">
              <div class="input-with-icon">
                <input type="password" id="login-password" placeholder="Password" required>
                <button type="button" class="input-icon-btn" id="login-toggle-password" data-tooltip="Show password" data-tip-placement="right">
                  <fa fa-eye></fa>
                </button>
              </div>
            </div>
            <div class="login-error" id="login-error">
              <span id="login-error-text">Error</span>
            </div>
            <div class="login-btn-group">
              <button type="button" class="login-btn-icon" id="login-language-btn" data-tooltip="Select language" data-tip-placement="left">
                <fa fa-earth-americas></fa>
              </button>
              <button type="button" class="login-btn-icon" id="login-theme-btn" data-tooltip="Select theme" data-tip-placement="left">
                <fa fa-palette></fa>
              </button>
              <button type="submit" class="login-btn-primary" id="login-submit" data-tooltip="Login" data-tip-placement="left">
                <fa fa-sign-in-alt></fa>
                <span>Login</span>
              </button>
              <button type="button" class="login-btn-icon" id="login-logs-btn" data-tooltip="View Session Log" data-tip-placement="left">
                <fa fa-receipt></fa>
              </button>
              <button type="button" class="login-btn-icon" id="login-help-btn" data-tooltip="Help" data-tip-placement="left">
                <fa circle-question></fa>
              </button>
            </div>
          </form>
        </div>
      </div>
    `;
  }

  /**
   * Render zero-or-more OIDC provider buttons in the alt login row.
   *
   * Reads `auth.oidc_providers` from the Lithium config. If the array is
   * absent or empty, nothing is rendered.
   * Per LITHIUM-INS.md rule #1 (no fallback path), there is no greyed-out
   * placeholder — the UI simply does not render when OIDC is disabled.
   *
   * Each button:
   *   - Has class `login-alt-btn login-btn-oidc` and `data-provider="<id>"`.
   *   - Displays an image icon from `provider.image` if available, otherwise
   *     falls back to the Font Awesome icon from `provider.icon`.
   *   - On click: records `auth.last_method` as `'oidc:<id>'` in
   *     `lithiumSettings` (before navigation away), then calls
   *     `startOidc(provider.id, window.location.pathname)`.
   *
   * On render, if `auth.last_method` in `lithiumSettings` starts with
   * `'oidc:'` and matches this provider's id, the `.is-recent` class is
   * added to the button as a subtle highlight (Phase 26).
   *
   * @param {Object} [deps] - Dependency injection for tests.
   * @param {Function} [deps.getConfigValue]   - Config accessor override.
   * @param {Function} [deps.startOidc]        - startOidc override.
   * @param {Object}   [deps.window]           - Window override.
   * @param {Object}   [deps.lithiumSettings]  - lithiumSettings override.
   */
  renderOidcProviders(deps = {}) {
    const gcv             = deps.getConfigValue ?? getConfigValue;
    const startOidcFn     = deps.startOidc      ?? startOidc;
    const win             = deps.window         ?? (typeof window !== 'undefined' ? window : null);
    const settings        = deps.lithiumSettings ??
      (typeof window !== 'undefined' ? window.lithiumSettings : null);

    const providers = gcv('auth.oidc_providers', []);

    // Nothing to render.
    if (!Array.isArray(providers) || providers.length === 0) return;

    const container = this.elements.altButtonGroup;

    if (!container) return;

    // Read the stored last-used login method for the `.is-recent` highlight.
    const lastMethod = settings?.get('auth.last_method') ?? null;

    // Build one button per valid provider.
    let rendered = 0;
    providers.forEach((provider) => {
      if (!provider || !provider.id || !provider.label) return;

      const btn = document.createElement('button');
      btn.type = 'button';
      btn.className = 'login-alt-btn login-btn-oidc';
      btn.setAttribute('data-provider', provider.id);
      btn.setAttribute('data-tooltip', provider.label);
      btn.setAttribute('data-tip-placement', 'top');
      btn.setAttribute('aria-label', provider.label);

      // Image icon (preferred) or Font Awesome fallback.
      if (provider.image) {
        const imgEl = document.createElement('img');
        imgEl.src = provider.image;
        imgEl.alt = provider.label;
        imgEl.className = 'login-alt-icon';
        btn.appendChild(imgEl);
      } else if (provider.icon) {
        const iconEl = document.createElement('span');
        iconEl.className = `login-btn-oidc__icon fa-duotone fa-thin ${provider.icon}`;
        iconEl.setAttribute('aria-hidden', 'true');
        btn.appendChild(iconEl);
      }

      // Subtle highlight if this provider was the last login method used.
      // Matches 'oidc:<id>' (written pre-navigation) or plain 'oidc'
      // (written on successful handoff exchange).
      if (lastMethod === `oidc:${provider.id}` || lastMethod === 'oidc') {
        btn.classList.add('is-recent');
      }

      // Click handler: full-page navigation to /oidc/start.
      btn.addEventListener('click', () => {
        log(LOGIN, Status.INFO, `OIDC provider button clicked: "${provider.id}"`);
        try {
          // Record the selected provider *before* navigating away so it
          // survives the page reload and can be used for the highlight on
          // the next render (Phase 26).
          settings?.set('auth.last_method', `oidc:${provider.id}`);

          const returnTo = win?.location?.pathname ?? null;
          startOidcFn(provider.id, returnTo, { getConfigValue: gcv, window: win });
        } catch (err) {
          log(LOGIN, Status.ERROR, `OIDC startOidc failed: ${err.message}`);
          this.showError('Sign-in could not start. Please try again.');
        }
      });

      container.appendChild(btn);
      rendered++;
    });

    log(LOGIN, Status.INFO, `OIDC: rendered ${rendered} provider button(s)`);
  }

  /**
   * Apply the `.is-recent` highlight to the password submit button when
   * `auth.last_method === 'password'` in `lithiumSettings` (Phase 26).
   *
   * Called from `render()` after all sub-modules are initialised.
   *
   * @param {Object} [deps] - Dependency injection for tests.
   * @param {Object} [deps.lithiumSettings] - lithiumSettings override.
   */
  applyPasswordRecentHighlight(deps = {}) {
    const settings = deps.lithiumSettings ??
      (typeof window !== 'undefined' ? window.lithiumSettings : null);
    const lastMethod = settings?.get('auth.last_method') ?? null;
    if (lastMethod === 'password' && this.elements.submit) {
      this.elements.submit.classList.add('is-recent');
    }
  }

  /**
   * Show an error banner in the login form.
   * Delegates to LoginForm; safe to call even before render() completes
   * (LoginForm may not yet be initialised).
   * @param {string} message
   */
  showError(message) {
    this._loginForm?.showError(message);
  }

  /**
   * Set up event listeners. Form-related listeners (submit, input, blur,
   * clear, password-toggle) are owned by LoginForm; this method wires the
   * panel-switching buttons and the language filter UI only.
   */
  setupEventListeners() {
    // Bind form-related listeners via LoginForm.
    this._loginForm?.init();

    // Panel-switch buttons.
    const panelButton = (btn, name) => {
      this.elements[btn]?.addEventListener('click', () => {
        log(LOGIN, Status.INFO, `Button clicked: ${name}`);
        this.switchPanel(name.toLowerCase());
      });
    };
    panelButton('languageBtn', 'Language');
    panelButton('themeBtn', 'Theme');
    panelButton('logsBtn', 'Logs');
    panelButton('helpBtn', 'Help');

    // Language filter input (delegated to LanguagePanel).
    this.elements.languageFilter?.addEventListener('input', (e) => {
      this._languagePanel?.filter(e.target.value);
    });
    this.elements.languageClearFilter?.addEventListener('click', () => {
      if (this.elements.languageFilter) {
        this.elements.languageFilter.value = '';
        this._languagePanel?.filter('');
        this.elements.languageFilter.focus();
      }
    });

    // Close buttons on each subpanel.
    const closeButton = (btn, name) => {
      this.elements[btn]?.addEventListener('click', () => {
        log(LOGIN, Status.INFO, `Button clicked: ${name} Close`);
        this.switchPanel('login');
      });
    };
    closeButton('themeCloseBtn', 'Theme');
    closeButton('logsCloseBtn', 'Logs');
    closeButton('languageCloseBtn', 'Language');
    closeButton('helpCloseBtn', 'Help');

    // Attach keyboard shortcuts (LoginShortcuts handles ESC, Ctrl+Shift+*,
    // F1, and CAPS-LOCK indicator).
    this._shortcuts?.attach();
  }

  /**
   * Switch between panels with fade transition
   * Delegates to LoginPanels; keeps panel-specific logic here.
   * @param {string} targetPanel - 'login', 'theme', 'logs', 'language', 'help'
   */
  async switchPanel(targetPanel) {
    if (!this._loginPanels) return;
    await this._loginPanels.switchPanel(targetPanel);
  }

  /**
   * Called by LoginPanels before performing a panel transition.
   * Handles panel-specific prep: password manager visibility, logs population,
   * language panel init/cleanup.
   * @param {string} targetPanel
   * @private
   */
  _onBeforePanelSwitch(targetPanel) {
    // Hide password manager UI when leaving login panel, show when returning
    if (targetPanel === 'login') {
      this._passwordManager?.show();
    } else {
      this._passwordManager?.hide();
    }

    // If switching to logs panel, populate it with action logs
    if (targetPanel === 'logs') {
      this._logsPanel?.show();
    }

    // If switching to language panel, show it; otherwise hide it
    if (targetPanel === 'language') {
      this._languagePanel?.show();
    } else {
      this._languagePanel?.hide();
    }
  }

  /**
   * Called by LoginPanels after a panel transition completes.
   * Handles logging and focus management.
   * @param {string} fromPanel
   * @param {string} toPanel
   * @private
   */
  _onAfterPanelSwitch(fromPanel, toPanel) {
    log(LOGIN, Status.INFO, `Panel: ${fromPanel} → ${toPanel}`);

    // Focus management when returning to login panel
    if (toPanel === 'login') {
      this._loginPanels.scheduleFocus(() => {
        if (this._loginPanels.currentPanel === 'login') {
          this.elements.username?.focus();
        }
      });
    }
  }

  /**
   * Show the login page with fade-in
   * @param {boolean} skipUsernameFocus - If true, focus password (username has value)
   */
  async show(skipUsernameFocus = false) {
    // Enable password manager UI when showing login
    this._passwordManager?.show();
    
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
      this._passwordManager?.hide();

      // Physically remove all injected password manager elements from the DOM.
      // This ensures they do not linger in the post-login view as invisible orphan
      // nodes (e.g. 1Password's live-region divs) and that 1Password cannot simply
      // re-show them by attribute mutation without re-inserting them first.
      this._passwordManager?.removeAll();
      
      // Fade out the current panel only
      // Let the app handle the transition to main manager
      const currentPanelEl = this._loginPanels?._panels[this._loginPanels?.currentPanel];
      if (currentPanelEl) {
        currentPanelEl.style.transition = `opacity ${duration}ms ease-in-out`;
        currentPanelEl.style.opacity = '0';
      }
      
      setTimeout(resolve, duration);
    });
  }

  /**
   * Teardown the login manager
   */
  teardown() {
    // Teardown LoginForm (removes form listeners, clears state)
    this._loginForm?.destroy();
    this._loginForm = null;

    // Teardown LogsPanel (cleans up CodeMirror editor + OverlayScrollbars)
    this._logsPanel?.destroy();
    this._logsPanel = null;

    // Teardown HelpPanel
    this._helpPanel?.destroy();
    this._helpPanel = null;

    // Teardown LanguagePanel
    this._languagePanel?.destroy();
    this._languagePanel = null;

    // Cancel any pending post-login-return focus timer
    this._loginPanels?.cancelPendingFocus();

    // Detach keyboard shortcut event listeners
    this._shortcuts?.detach();
    this._shortcuts = null;

    // Clean up password manager UI observer and remove any lingering injected elements
    this._passwordManager?.destroy();
    this._passwordManager = null;

    // Remove lookup event listeners
    this.lookupListeners.forEach(unsubscribe => unsubscribe());
    this.lookupListeners = [];

    // Teardown LoginPanels
    this._loginPanels?.teardown();
    this._loginPanels = null;

    // Clear references
    this.elements = {};
  }
}
