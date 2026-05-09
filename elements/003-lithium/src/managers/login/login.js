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
import { getTransitionDuration } from '../../core/transitions.js';
import { LoginPanels } from './login-panels.js';
import { hasLookup } from '../../shared/lookups.js';
import { log, logGroup, getRawLog, Subsystems, Status } from '../../core/log.js';
import { formatLogText } from '../../shared/log-formatter.js';
import { PasswordManager } from './login-password-manager.js';
import { LanguagePanel } from './login-language-panel.js';
import { getTip } from '../../core/tooltip-api.js';
import { scrollbarManager } from '../../core/scrollbar-manager.js';
import '../../styles/vendor-tabulator.css';
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
    this.lookupListeners = [];
    this._startupComplete = false; // Tracks whether background startup has finished
    this._loginPanels = null; // LoginPanels instance (initialized in render)
    this._passwordManager = null; // PasswordManager instance (initialized in render)
    this._languagePanel = null; // LanguagePanel instance (initialized in render)
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
      // Check if we should not save username (e.g., after public/global logout)
      if (sessionStorage.getItem('lithium_no_save_username') === 'true') {
        sessionStorage.removeItem('lithium_no_save_username');
        return; // Don't save
      }
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

    // Language filter input (delegated to LanguagePanel)
    this.elements.languageFilter?.addEventListener('input', (e) => {
      this._languagePanel?.filter(e.target.value);
    });

    // Language clear filter button
    this.elements.languageClearFilter?.addEventListener('click', () => {
      if (this.elements.languageFilter) {
        this.elements.languageFilter.value = '';
        this._languagePanel?.filter('');
        this.elements.languageFilter.focus();
      }
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
    const isOnLoginPanel = this._loginPanels?.currentPanel === 'login';
    
    // ESC - always works to return to login panel or clear fields
    if (event.key === 'Escape') {
      this._handleEscapeKey(event, isOnLoginPanel);
      return;
    }
    
    // All other shortcuts only work when on login panel
    if (!isOnLoginPanel) return;
    
    // Map of shortcut handlers
    const shortcuts = [
      { check: (e) => e.ctrlKey && e.shiftKey && e.key.toLowerCase() === 'u', handler: () => this._focusUsername() },
      { check: (e) => e.ctrlKey && e.shiftKey && e.key.toLowerCase() === 'p', handler: () => this._focusPassword() },
      { check: (e) => e.key === 'F1', handler: () => this._clickButton('helpBtn') },
      { check: (e) => e.ctrlKey && e.shiftKey && e.key.toLowerCase() === 'i', handler: () => this._clickButton('languageBtn') },
      { check: (e) => e.ctrlKey && e.shiftKey && e.key.toLowerCase() === 't', handler: () => this._clickButton('themeBtn') },
      { check: (e) => e.ctrlKey && e.shiftKey && e.key.toLowerCase() === 'l', handler: () => this._clickButton('logsBtn') },
    ];
    
    for (const shortcut of shortcuts) {
      if (shortcut.check(event)) {
        shortcut.handler();
        event.preventDefault();
        return;
      }
    }
  }

  /**
   * Handle ESC key press
   * @param {KeyboardEvent} event
   * @param {boolean} isOnLoginPanel
   * @private
   */
  _handleEscapeKey(event, isOnLoginPanel) {
    if (isOnLoginPanel) {
      this.handleClearUsername();
    } else {
      log(LOGIN, Status.INFO, `Keyboard: ESC closed ${this._loginPanels?.currentPanel} panel`);
      this.switchPanel('login');
    }
    event.preventDefault();
  }

  /**
   * Focus username field and select contents
   * @private
   */
  _focusUsername() {
    if (this.elements.username) {
      log(LOGIN, Status.INFO, 'Keyboard: Ctrl+Shift+U focused username field');
      this.elements.username.focus();
      this.elements.username.select();
    }
  }

  /**
   * Focus password field and select contents
   * @private
   */
  _focusPassword() {
    if (this.elements.password) {
      log(LOGIN, Status.INFO, 'Keyboard: Ctrl+Shift+P focused password field');
      this.elements.password.focus();
      this.elements.password.select();
    }
  }

  /**
   * Click a button by element name
   * @param {string} btnName - Button element name
   * @private
   */
  _clickButton(btnName) {
    const btn = this.elements[btnName];
    if (btn && !btn.disabled) {
      btn.click();
    }
  }

  /**
   * Handle ESC key to return to login panel from subpanels
   * @deprecated Use handleKeyboardShortcuts instead
   */
  handleEscapeKey(event) {
    if (event.key === 'Escape' && this._loginPanels?.currentPanel !== 'login') {
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
      this.populateLogsPanel();
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
   * Populate the logs panel with action logs using CodeMirror
   */
  async populateLogsPanel() {
    const logViewer = this.elements.logViewer;
    if (!logViewer) {
      console.warn('[LoginManager] logViewer element not found');
      return;
    }

    const entries = getRawLog();
    const logText = formatLogText(entries);

    // If CodeMirror is already initialized, just update the content
    if (this._logEditor) {
      this._logEditor.dispatch({
        changes: { from: 0, to: this._logEditor.state.doc.length, insert: logText }
      });
      return;
    }

    // Initialize CodeMirror — uses shared setup for consistent formatting
    try {
      const { EditorState, EditorView } = await import('../../core/codemirror.js');
      const { buildEditorExtensions, createReadOnlyCompartment } = await import('../../core/codemirror-setup.js');

      const roCompartment = createReadOnlyCompartment();
      const extensions = buildEditorExtensions({
        language: 'log',
        readOnlyCompartment: roCompartment,
        readOnly: true,
        fontSize: 10,
        fontFamily: "'Vanadium Mono', 'Courier New', Courier, monospace",
      });

      const state = EditorState.create({ doc: logText, extensions });

      logViewer.innerHTML = '';
      this._logEditor = new EditorView({ state, parent: logViewer });


    } catch (error) {
      console.warn('[LoginManager] CodeMirror failed to load, using plain text:', error);
      logViewer.innerHTML = `<pre class="log-content">${logText}</pre>`;
    }
  }

  /**
   * Set up language table event handlers
   * @private
   */
  _setupLanguageTableEvents() {
    this._languageTable.on('rowClick', (e, row) => {
      this.selectLanguage(row.getData().locale);
    });

    this._languageTable.on('rowSelected', (row) => {
      this._currentLocale = row.getData().locale;
    });
  }

  /**
   * Finalize table setup with selection and focus
   * @private
   */
  _finalizeLanguageTableSetup() {
    setTimeout(() => {
      this._refreshLanguageTableSelection();
      this._focusLanguageTable();
    }, 100);
  }

  /**
   * Handle language table initialization error
   * @param {Error} error - Error object
   * @param {HTMLElement} tableContainer - Container element
   * @private
   */
  _handleLanguageTableError(error, tableContainer) {
    console.error('[LoginManager] Failed to initialize language table:', error);
    log(LOGIN, Status.ERROR, `Failed to initialize language table: ${error.message}`);
    this._renderLanguageFallback(tableContainer);
  }

  /**
   * Render fallback language list
   * @param {HTMLElement} tableContainer - Container element
   * @private
   */
  _renderLanguageFallback(tableContainer) {
    tableContainer.innerHTML = this._languageData.map(lang => `
      <div class="language-item-fallback" data-locale="${lang.locale}" style="
        padding: var(--space-3);
        border-bottom: var(--border-standard);
        cursor: pointer;
        display: flex;
        align-items: center;
        gap: var(--space-3);
        ${lang.locale === this._currentLocale ? 'background: var(--accent-primary); color: white;' : ''}
      ">
        ${this._getFlagSvg(lang.countryCode)}
        <span style="flex: 1;">${lang.language}</span>
        <span style="color: ${lang.locale === this._currentLocale ? 'rgba(255,255,255,0.8)' : 'var(--text-muted)'};">${lang.country}</span>
        <span style="color: ${lang.locale === this._currentLocale ? 'rgba(255,255,255,0.8)' : 'var(--text-muted)'};">${lang.locale}</span>
      </div>
    `).join('');

    tableContainer.querySelectorAll('.language-item-fallback').forEach(item => {
      item.addEventListener('click', () => this.selectLanguage(item.dataset.locale));
    });
  }

  /**
   * Update the current-locale indicator button in the language panel header
   * Shows the flag SVG and locale code for the currently selected locale
   */
  _updateCurrentLocaleButton() {
    if (!this._currentLocale) return;

    const countryCode = getCountryCode(this._currentLocale);

    if (this.elements.languageCurrentFlag) {
      try {
        const flagSvg = getFlagSvg(countryCode, { width: 22, height: 16 });
        this.elements.languageCurrentFlag.innerHTML = flagSvg;
      } catch (e) {
        this.elements.languageCurrentFlag.textContent = countryCode;
      }
    }

    if (this.elements.languageCurrentCode) {
      this.elements.languageCurrentCode.textContent = this._currentLocale;
    }
  }

  /**
   * Refresh the language table selection and scroll to selected row
   * @param {number} retryCount - Number of retries attempted (internal use)
   */
  _refreshLanguageTableSelection(retryCount = 0) {
    if (!this._languageTable || !this._currentLocale) return;

    // Find and select the row for current locale
    // Use getRows() to get all rows
    const rows = this._languageTable.getRows();
    const targetRow = rows.find(r => r.getData().locale === this._currentLocale);

    if (targetRow) {
      // Deselect all other rows first
      this._languageTable.deselectRow();
      // Select the target row
      targetRow.select();
      // Scroll to row in the middle of the viewport
      this._languageTable.scrollToRow(targetRow, 'middle', false);
    } else if (retryCount < 3) {
      // Row not found yet, retry after a short delay
      setTimeout(() => {
        this._refreshLanguageTableSelection(retryCount + 1);
      }, 100);
    }
  }

  /**
   * Focus the language table for keyboard navigation
   */
  _focusLanguageTable() {
    // Use setTimeout to ensure the table is fully rendered
    setTimeout(() => {
      const tableElement = this.elements.languageTable;
      if (tableElement) {
        tableElement.setAttribute('tabindex', '-1');
        tableElement.focus();
      }
    }, 100);
  }

  /**
   * Handle keyboard navigation in the language table
   * @param {KeyboardEvent} event - The keyboard event
   */
  _handleLanguageTableKeydown(event) {
    if (!this._languageTable || this.currentPanel !== 'language') return;

    const key = event.key;
    let handled = false;

    switch (key) {
      case 'ArrowDown':
        this._moveLanguageSelection(1);
        handled = true;
        break;
      case 'ArrowUp':
        this._moveLanguageSelection(-1);
        handled = true;
        break;
      case 'PageDown':
        this._moveLanguageSelection(10);
        handled = true;
        break;
      case 'PageUp':
        this._moveLanguageSelection(-10);
        handled = true;
        break;
      case 'Enter':
        this._selectCurrentLanguageRow();
        handled = true;
        break;
      case 'Home':
        this._moveLanguageSelectionTo(0);
        handled = true;
        break;
      case 'End':
        this._moveLanguageSelectionTo(-1);
        handled = true;
        break;
    }

    if (handled) {
      event.preventDefault();
      event.stopPropagation();
    }
  }

  /**
   * Move the language selection by a number of rows
   * @param {number} delta - Number of rows to move (positive = down, negative = up)
   */
  _moveLanguageSelection(delta) {
    if (!this._languageTable) return;

    // Use getRows("active") to get rows in their displayed (sorted/filtered) order
    const rows = this._languageTable.getRows('active');
    if (rows.length === 0) return;

    // Find currently selected row index in the displayed order
    let currentIndex = -1;
    const selectedRows = this._languageTable.getSelectedRows();
    if (selectedRows.length > 0) {
      currentIndex = rows.findIndex(r => r.getData().locale === selectedRows[0].getData().locale);
    }

    // Calculate new index
    let newIndex = currentIndex + delta;
    if (newIndex < 0) newIndex = 0;
    if (newIndex >= rows.length) newIndex = rows.length - 1;

    this._selectRowAtIndex(newIndex);
  }

  /**
   * Move selection to a specific row index
   * @param {number} index - Row index (0 = first, -1 = last)
   */
  _moveLanguageSelectionTo(index) {
    if (!this._languageTable) return;

    // Use getRows("active") to get rows in their displayed (sorted/filtered) order
    const rows = this._languageTable.getRows('active');
    if (rows.length === 0) return;

    let newIndex = index;
    if (newIndex < 0) newIndex = rows.length + newIndex;
    if (newIndex < 0) newIndex = 0;
    if (newIndex >= rows.length) newIndex = rows.length - 1;

    this._selectRowAtIndex(newIndex);
  }

  /**
   * Select a row at a specific index and scroll it into view
   * @param {number} index - The row index to select
   */
  _selectRowAtIndex(index) {
    if (!this._languageTable) return;

    // Use getRows("active") to get rows in their displayed (sorted/filtered) order
    const rows = this._languageTable.getRows('active');
    if (index < 0 || index >= rows.length) return;

    const targetRow = rows[index];

    // Deselect all and select target
    this._languageTable.deselectRow();
    targetRow.select();

    // Scroll to row in the middle of the viewport
    this._languageTable.scrollToRow(targetRow, 'middle', false);
  }

  /**
   * Select the currently selected language row (Enter key action)
   */
  _selectCurrentLanguageRow() {
    if (!this._languageTable) return;

    const selectedRows = this._languageTable.getSelectedRows();
    if (selectedRows.length > 0) {
      const data = selectedRows[0].getData();
      this.selectLanguage(data.locale);
    }
  }

  /**
   * Filter the language table based on search input
   * Case-insensitive search on language, country, or locale
   * @param {string} filterText - Text to filter by
   */
  filterLanguageTable(filterText) {
    if (!this._languageTable) return;

    const searchLower = filterText.toLowerCase().trim();

    if (!searchLower) {
      this._languageTable.clearFilter();
      return;
    }

    // Custom filter function - case insensitive search on multiple fields
    this._languageTable.setFilter((data) => {
      const localeMatch = data.locale.toLowerCase().includes(searchLower);
      const languageMatch = data.language && data.language.toLowerCase().includes(searchLower);
      const countryMatch = data.country && data.country.toLowerCase().includes(searchLower);
      const nativeMatch = data.nativeName && data.nativeName.toLowerCase().includes(searchLower);

      return localeMatch || languageMatch || countryMatch || nativeMatch;
    });
  }

  /**
   * Select a language and save preference
   * @param {string} locale - The selected locale
   */
  selectLanguage(locale) {
    if (!supportedLocales.includes(locale)) {
      console.warn(`[LoginManager] Unsupported locale: ${locale}`);
      return;
    }

    const previousLocale = this._currentLocale;
    this._currentLocale = locale;
    saveLocalePreference(locale);

    // Update the current-locale indicator in the header
    this._updateCurrentLocaleButton();

    // Update row selection in table and scroll to it
    if (this._languageTable) {
      this._languageTable.deselectRow();
      const row = this._languageTable.getRows().find(r => r.getData().locale === locale);
      if (row) {
        row.select();
        this._languageTable.scrollToRow(row, 'middle', false);
      }
    }

    // Log the selection
    const langData = this._languageData.find(l => l.locale === locale);
    const langName = langData ? `${langData.language} (${langData.country})` : locale;
    log(LOGIN, Status.SUCCESS, `Language selected: ${langName} (${locale})`);

    // Emit locale changed event
    eventBus.emit(Events.LOCALE_CHANGED, {
      lang: locale,
      previousLang: previousLocale,
    });
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
      if (this.elements.togglePasswordBtn) {
        const t = getTip(this.elements.togglePasswordBtn);
        if (t) t.updateContent('Hide password');
      }
    } else {
      // Hide password
      this.elements.password.type = 'password';
      this.elements.passwordIcon.classList.remove('fa-eye-slash');
      this.elements.passwordIcon.classList.add('fa-eye');
      this.elements.togglePasswordBtn?.setAttribute('data-tooltip', 'Show password');
      if (this.elements.togglePasswordBtn) {
        const t = getTip(this.elements.togglePasswordBtn);
        if (t) t.updateContent('Show password');
      }
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

    // Fade out login page
    await this.hide();

    // Success — log and emit login event (after fade-out completes)
    const loginDuration = this._loginStart ? Math.round(performance.now() - this._loginStart) : 0;
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
        ? '<span class="spinner-fancy spinner-fancy-sm"></span> &nbsp;&nbsp; <span>Logging in...</span>'
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
    // Clean up CodeMirror OverlayScrollbars first
    if (this._logEditor?._osbInstance) {
      scrollbarManager.destroy(this._logEditor._osbInstance);
      this._logEditor._osbInstance = null;
    }

    // Clean up CodeMirror editor if present
    if (this._logEditor) {
      this._logEditor.destroy();
      this._logEditor = null;
    }

    // Teardown LanguagePanel
    this._languagePanel?.destroy();
    this._languagePanel = null;

    // Cancel any pending post-login-return focus timer
    this._loginPanels?.cancelPendingFocus();

    // Remove keyboard shortcut event listeners
    this.removeKeyboardShortcuts();

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
    this.isPasswordVisible = false;
    this.isCapsLockOn = false;
  }
}
