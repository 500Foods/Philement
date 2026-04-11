/**
 * Profile Manager
 *
 * User preferences and profile settings management.
 * Handles language, date/time format, number format, default database, and theme preferences.
 * Includes a LithiumTable showing user options from Lookup 60 (App UI Lists).
 */

import { eventBus, Events } from '../../core/event-bus.js';
import { getClaims, storeJWT } from '../../core/jwt.js';
import { getConfigValue } from '../../core/config.js';
import { createRequest } from '../../core/json-request.js';
import { setIcon } from '../../core/icons.js';
import { LithiumTable } from '../../tables/lithium-table-main.js';
import { LithiumSplitter } from '../../core/lithium-splitter.js';
import { PanelStateManager } from '../../core/panel-state-manager.js';
import { togglePanelCollapse, restorePanelState as restoreCollapsedPanelState } from '../../core/panel-collapse.js';
import { authQuery } from '../../shared/conduit.js';
import { log, Subsystems, Status } from '../../core/log.js';
import { processIcons } from '../../core/icons.js';
import { initToolbars } from '../../core/manager-ui.js';
import { ManagerEditHelper } from '../../core/manager-edit-helper.js';
import '../../styles/vendor-tabulator.css';
import '../../core/manager-panels.css';
import './profile-manager.css';

/**
 * Profile Manager Class
 */
export default class ProfileManager {
  constructor(app, container) {
    this.app = app;
    this.container = container;
    this.elements = {};
    this.user = null;
    this.preferences = {};
    this.themes = [];
    this.databases = [];
    this.api = null;

    // Profile Manager ID = 2 (Utility)
    this.managerId = 2;

    // User options table
    this.optionsTable = null;
    this.userOptions = [];

    // Panel state persistence
    this.leftPanelState = new PanelStateManager('lithium_profile_left');

    // Splitter
    this.splitter = null;
    this.leftPanelWidth = this.leftPanelState.loadWidth(314);
    this.isLeftPanelCollapsed = this.leftPanelState.loadCollapsed(false);

    // Edit helper
    this.editHelper = new ManagerEditHelper({ name: 'Profile' });

    // Default preferences
    this.defaultPreferences = {
      language: 'en-US',
      dateFormat: 'MM/DD/YYYY',
      timeFormat: '12h',
      numberFormat: 'en-US',
      defaultDatabase: '',
      activeTheme: '',
    };

    // Preview update timer
    this.previewTimer = null;

    // Current tab
    this.currentTab = 'settings';
  }

  /**
   * Initialize the profile manager
   */
  async init() {
    // Initialize API client
    this.api = createRequest();

    // Render template
    await this.render();

    // Load user info from JWT
    this.loadUserInfo();

    // Initialize the options table (Lookup 60)
    await this.initOptionsTable();

    // Setup splitter
    this.setupSplitter();

    // Load themes and databases
    await Promise.all([this.loadThemes(), this.loadDatabases()]);

    // Load saved preferences
    this.loadPreferences();

    // Setup event listeners
    this.setupEventListeners();

    // Setup footer
    this.setupFooter();

    // Restore panel state
    this.restorePanelState();

    // Initialize preview
    this.updatePreview();

    // Show the page
    this.show();
  }

  /**
   * Render the profile manager template
   */
  async render() {
    try {
      const response = await fetch('/src/managers/profile-manager/profile-manager.html');
      const html = await response.text();
      this.container.innerHTML = html;
    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, '[ProfileManager] Failed to load template:', error);
      this.renderFallback();
    }

    // Cache DOM elements
    this.elements = {
      page: this.container.querySelector('#profile-manager-page'),
      leftPanel: this.container.querySelector('#profile-left-panel'),
      rightPanel: this.container.querySelector('#profile-right-panel'),
      tableContainer: this.container.querySelector('#profile-table-container'),
      navigatorContainer: this.container.querySelector('#profile-navigator'),
      splitter: this.container.querySelector('#profile-splitter'),
      collapseBtn: this.container.querySelector('#profile-collapse-btn'),
      collapseIcon: this.container.querySelector('#profile-collapse-icon'),
      tabToolbar: this.container.querySelector('#profile-tab-toolbar'),
      tabSettings: this.container.querySelector('#tab-settings'),
      tabAccount: this.container.querySelector('#tab-account'),
      panelSettings: this.container.querySelector('#panel-settings'),
      panelAccount: this.container.querySelector('#panel-account'),
      username: this.container.querySelector('#profile-username'),
      email: this.container.querySelector('#profile-email'),
      roles: this.container.querySelector('#profile-roles'),
      database: this.container.querySelector('#profile-database'),
      preferencesForm: this.container.querySelector('#preferences-form'),
      language: this.container.querySelector('#pref-language'),
      dateFormat: this.container.querySelector('#pref-date-format'),
      timeFormat: this.container.querySelector('#pref-time-format'),
      numberFormat: this.container.querySelector('#pref-number-format'),
      defaultDatabase: this.container.querySelector('#pref-database'),
      activeTheme: this.container.querySelector('#pref-theme'),
      btnSave: this.container.querySelector('#btn-save-preferences'),
      btnReset: this.container.querySelector('#btn-reset-preferences'),
      previewDate: this.container.querySelector('#preview-date'),
      previewTime: this.container.querySelector('#preview-time'),
      previewNumber: this.container.querySelector('#preview-number'),
      previewCurrency: this.container.querySelector('#preview-currency'),
      toast: this.container.querySelector('#profile-toast'),
    };

    processIcons(this.container);
    initToolbars();
  }

  /**
   * Render fallback if template fails
   */
  renderFallback() {
    this.container.innerHTML = `
      <div id="profile-manager-page">
        <p>Profile Manager loading...</p>
      </div>
    `;
  }

  /**
   * Initialize the User Options table (Lookup 60)
   */
  async initOptionsTable() {
    if (!this.elements.tableContainer || !this.elements.navigatorContainer) return;

    this.optionsTable = new LithiumTable({
      container: this.elements.tableContainer,
      navigatorContainer: this.elements.navigatorContainer,
      tablePath: 'profile-manager/user-options',
      lookupKeyIdx: 8,
      cssPrefix: 'profile-options',
      storageKey: 'profile_options_table',
      app: this.app,
      readonly: true,
      panel: this.elements.leftPanel,
      panelStateManager: this.leftPanelState,
      onRowSelected: (rowData) => this.handleOptionSelected(rowData),
      onRowDeselected: () => this.handleOptionDeselected(),
      onDataLoaded: (rows) => {
        log(Subsystems.TABLE, Status.INFO, `[ProfileManager] Loaded ${rows.length} user options`);
      },
      onRefresh: () => this.loadUserOptions(),
    });

    // Register with editHelper
    this.editHelper.registerTable(this.optionsTable);

    await this.optionsTable.init();

    // Load the user options data
    await this.loadUserOptions();
  }

  /**
   * Load user options from Lookup 60 (App UI Lists)
   * Uses QueryRef 26 with LOOKUPID=60 to get the Profile Sections (key_idx=0)
   */
  async loadUserOptions() {
    if (!this.app?.api || !this.optionsTable?.table) return;

    try {
      this.optionsTable.showLoading();

      // QueryRef 26 with LOOKUPID=60 to get Lookup 60 (App UI Lists)
      const rows = await authQuery(this.app.api, 26, { INTEGER: { LOOKUPID: 60 } });

      // Find the Profile Sections entry (key_idx=0)
      const profileSectionsRow = rows?.find(row => row.key_idx === 0);

      if (profileSectionsRow && profileSectionsRow.collection) {
        // Parse the collection JSON which contains the profile sections
        let collection = profileSectionsRow.collection;
        if (typeof collection === 'string') {
          try {
            collection = JSON.parse(collection);
          } catch (_e) {
            collection = {};
          }
        }

        // Get the sections array (use current locale or default)
        const locale = navigator.language || 'en-US';
        const sections = collection[locale] || collection.default || [];

        // Transform into table rows
        // Format: ["Section", "Icon", "Label"]
        this.userOptions = sections.map((section, index) => ({
          key_idx: index + 1,
          section: section[0] || '',
          icon: section[1] || '',
          label: section[2] || '',
          status_a1: 1,
        }));
      } else {
        // Fallback to default options if lookup data not available
        this.userOptions = this.getDefaultUserOptions();
      }

      // Load data into the table
      this.optionsTable.loadStaticData(this.userOptions);

      log(Subsystems.TABLE, Status.INFO, `[ProfileManager] Loaded ${this.userOptions.length} user options from Lookup 60`);
    } catch (error) {
      log(Subsystems.TABLE, Status.ERROR, `[ProfileManager] Failed to load user options: ${error.message}`);
      // Load default options on error
      this.userOptions = this.getDefaultUserOptions();
      this.optionsTable.loadStaticData(this.userOptions);
    } finally {
      this.optionsTable.hideLoading();
    }
  }

  /**
   * Get default user options when lookup data is not available
   */
  getDefaultUserOptions() {
    return [
      { key_idx: 1, section: 'General', icon: '<fa fa-user></fa>', label: 'Account and Names', status_a1: 1 },
      { key_idx: 2, section: 'Security', icon: '<fa fa-key></fa>', label: 'Authentication', status_a1: 1 },
      { key_idx: 3, section: 'Formatting', icon: '<fa fa-globe></fa>', label: 'Language', status_a1: 1 },
      { key_idx: 4, section: 'Formatting', icon: '<fa fa-calendar></fa>', label: 'Date/Time Formats', status_a1: 1 },
      { key_idx: 5, section: 'Formatting', icon: '<fa fa-00></fa>', label: 'Number Formats', status_a1: 1 },
      { key_idx: 6, section: 'Application', icon: '<fa fa-flag-pennant></fa>', label: 'Startup', status_a1: 1 },
      { key_idx: 7, section: 'Application', icon: '<fa fa-bell></fa>', label: 'Notifications', status_a1: 1 },
    ];
  }

  /**
   * Handle when an option is selected from the table
   */
  handleOptionSelected(rowData) {
    if (!rowData) return;

    log(Subsystems.MANAGER, Status.INFO, `[ProfileManager] Option selected: ${rowData.label}`);

    // Map option labels to tabs
    const labelToTab = {
      'Account and Names': 'account',
      'Authentication': 'account',
      'Language': 'settings',
      'Date/Time Formats': 'settings',
      'Number Formats': 'settings',
      'Startup': 'settings',
      'Notifications': 'settings',
    };

    const targetTab = labelToTab[rowData.label];
    if (targetTab && targetTab !== this.currentTab) {
      this.switchTab(targetTab);
    }
  }

  /**
   * Handle when an option is deselected
   */
  handleOptionDeselected() {
    log(Subsystems.MANAGER, Status.DEBUG, '[ProfileManager] Option deselected');
  }

  /**
   * Setup the splitter between panels
   */
  setupSplitter() {
    this.splitter = new LithiumSplitter({
      element: this.elements.splitter,
      leftPanel: this.elements.leftPanel,
      minWidth: 157,
      maxWidth: 600,
      tables: this.optionsTable,
      onResize: (width) => {
        this.leftPanelWidth = width;
      },
      onResizeEnd: (width) => {
        this.leftPanelState.saveWidth(width);
        this.optionsTable?.table?.redraw?.();
      },
    });

    // Bind splitter to table
    this.optionsTable?.setSplitter(this.splitter);
  }

  /**
   * Toggle left panel collapse
   */
  toggleLeftPanel() {
    this.isLeftPanelCollapsed = togglePanelCollapse({
      panel: this.elements.leftPanel,
      splitter: this.splitter,
      collapseBtn: this.elements.collapseBtn,
      panelWidth: this.leftPanelWidth,
      isCollapsed: this.isLeftPanelCollapsed,
      onAfterToggle: () => this.optionsTable?.table?.redraw?.(),
    });

    // Save collapsed state
    this.leftPanelState.saveCollapsed(this.isLeftPanelCollapsed);
  }

  /**
   * Restore panel state from persistence
   */
  restorePanelState() {
    this.isLeftPanelCollapsed = this.leftPanelState.loadCollapsed(this.isLeftPanelCollapsed);

    restoreCollapsedPanelState({
      panel: this.elements.leftPanel,
      splitter: this.splitter,
      collapseBtn: this.elements.collapseBtn,
      isCollapsed: this.isLeftPanelCollapsed,
    });
  }

  /**
   * Switch between tabs
   */
  switchTab(tabId) {
    if (!['settings', 'account'].includes(tabId)) return;

    this.currentTab = tabId;

    // Update tab buttons
    this.elements.tabSettings?.classList.toggle('active', tabId === 'settings');
    this.elements.tabAccount?.classList.toggle('active', tabId === 'account');

    // Update panels
    this.elements.panelSettings?.classList.toggle('active', tabId === 'settings');
    this.elements.panelAccount?.classList.toggle('active', tabId === 'account');

    log(Subsystems.MANAGER, Status.DEBUG, `[ProfileManager] Switched to tab: ${tabId}`);
  }

  /**
   * Load user info from JWT claims
   */
  loadUserInfo() {
    const claims = getClaims();
    if (claims) {
      this.user = {
        id: claims.user_id,
        username: claims.username || 'User',
        email: claims.email || '-',
        roles: claims.roles || [],
        database: claims.database || '-',
      };

      // Update UI
      if (this.elements.username) {
        this.elements.username.textContent = this.user.username;
      }
      if (this.elements.email) {
        this.elements.email.textContent = this.user.email;
      }
      if (this.elements.roles) {
        this.elements.roles.textContent = Array.isArray(this.user.roles)
          ? this.user.roles.join(', ')
          : this.user.roles;
      }
      if (this.elements.database) {
        this.elements.database.textContent = this.user.database;
      }
    }
  }

  /**
   * Load themes from localStorage or mock data
   */
  async loadThemes() {
    // First, try to get from localStorage (set by Style Manager)
    const storedThemes = localStorage.getItem('lithium_themes');
    if (storedThemes) {
      try {
        this.themes = JSON.parse(storedThemes);
      } catch (error) {
        log(Subsystems.MANAGER, Status.WARN, '[ProfileManager] Failed to parse stored themes:', error);
      }
    }

    // If no themes stored, use mock data
    if (!this.themes || this.themes.length === 0) {
      this.themes = this.getMockThemes();
    }

    // Populate theme select
    this.populateThemeSelect();
  }

  /**
   * Get mock themes
   */
  getMockThemes() {
    return [
      { id: 'default', name: 'Default Dark' },
      { id: 1, name: 'Midnight Indigo' },
      { id: 2, name: 'Ocean Breeze' },
      { id: 3, name: 'Sunset Glow' },
    ];
  }

  /**
   * Populate theme select dropdown
   */
  populateThemeSelect() {
    if (!this.elements.activeTheme) return;

    // Clear existing options except first placeholder
    this.elements.activeTheme.innerHTML = '<option value="">Select a theme...</option>';

    this.themes.forEach((theme) => {
      const option = document.createElement('option');
      option.value = theme.id;
      option.textContent = theme.name;
      this.elements.activeTheme.appendChild(option);
    });
  }

  /**
   * Load databases (mock for now, API integration later)
   */
  async loadDatabases() {
    // TODO: Load from Hydrogen API when available
    // For now, use mock data
    this.databases = [
      { id: 'Demo_PG', name: 'Demo PostgreSQL' },
      { id: 'Demo_MySQL', name: 'Demo MySQL' },
      { id: 'Demo_SQLite', name: 'Demo SQLite' },
    ];

    // Get default from config
    const configDefault = getConfigValue('auth.default_database', '');
    if (configDefault && !this.defaultPreferences.defaultDatabase) {
      this.defaultPreferences.defaultDatabase = configDefault;
    }

    this.populateDatabaseSelect();
  }

  /**
   * Populate database select dropdown
   */
  populateDatabaseSelect() {
    if (!this.elements.defaultDatabase) return;

    // Clear existing options
    this.elements.defaultDatabase.innerHTML = '<option value="">Select a database...</option>';

    this.databases.forEach((db) => {
      const option = document.createElement('option');
      option.value = db.id;
      option.textContent = db.name;
      this.elements.defaultDatabase.appendChild(option);
    });
  }

  /**
   * Load preferences from localStorage
   */
  loadPreferences() {
    const stored = localStorage.getItem('lithium_preferences');
    if (stored) {
      try {
        const parsed = JSON.parse(stored);
        this.preferences = { ...this.defaultPreferences, ...parsed };
      } catch (error) {
        log(Subsystems.MANAGER, Status.WARN, '[ProfileManager] Failed to parse preferences:', error);
        this.preferences = { ...this.defaultPreferences };
      }
    } else {
      this.preferences = { ...this.defaultPreferences };
    }

    // Apply to form
    this.applyPreferencesToForm();
  }

  /**
   * Apply preferences to form fields
   */
  applyPreferencesToForm() {
    if (this.elements.language) {
      this.elements.language.value = this.preferences.language;
    }
    if (this.elements.dateFormat) {
      this.elements.dateFormat.value = this.preferences.dateFormat;
    }
    if (this.elements.timeFormat) {
      this.elements.timeFormat.value = this.preferences.timeFormat;
    }
    if (this.elements.numberFormat) {
      this.elements.numberFormat.value = this.preferences.numberFormat;
    }
    if (this.elements.defaultDatabase) {
      this.elements.defaultDatabase.value = this.preferences.defaultDatabase;
    }
    if (this.elements.activeTheme) {
      this.elements.activeTheme.value = this.preferences.activeTheme;
    }
  }

  /**
   * Setup event listeners
   */
  setupEventListeners() {
    // Collapse button
    this.elements.collapseBtn?.addEventListener('click', () => {
      this.toggleLeftPanel();
    });

    // Tab buttons
    this.elements.tabSettings?.addEventListener('click', () => this.switchTab('settings'));
    this.elements.tabAccount?.addEventListener('click', () => this.switchTab('account'));

    // Form submission
    this.elements.preferencesForm?.addEventListener('submit', (e) => {
      e.preventDefault();
      this.savePreferences();
    });

    // Reset button
    this.elements.btnReset?.addEventListener('click', () => {
      this.resetPreferences();
    });

    // Real-time preview updates
    const previewFields = [
      this.elements.language,
      this.elements.dateFormat,
      this.elements.timeFormat,
      this.elements.numberFormat,
    ];

    previewFields.forEach((field) => {
      field?.addEventListener('change', () => {
        this.debouncePreviewUpdate();
      });
    });

    // Theme change applies immediately
    this.elements.activeTheme?.addEventListener('change', () => {
      this.applyTheme();
    });
  }

  /**
   * Setup footer controls
   */
  setupFooter() {
    // Footer controls can be added here if needed
    // For now, the profile manager doesn't need print/email/export
    log(Subsystems.MANAGER, Status.INFO, '[ProfileManager] Footer setup complete');
  }

  /**
   * Debounce preview updates
   */
  debouncePreviewUpdate() {
    if (this.previewTimer) {
      clearTimeout(this.previewTimer);
    }
    this.previewTimer = setTimeout(() => {
      this.updatePreview();
    }, 100);
  }

  /**
   * Update preview panel with current format settings
   */
  updatePreview() {
    const language = this.elements.language?.value || 'en-US';
    const dateFormat = this.elements.dateFormat?.value || 'MM/DD/YYYY';
    const timeFormat = this.elements.timeFormat?.value || '12h';
    const numberFormat = this.elements.numberFormat?.value || 'en-US';

    // Sample date/time
    const now = new Date();

    // Format date
    if (this.elements.previewDate) {
      this.elements.previewDate.textContent = this.formatDate(now, dateFormat, language);
    }

    // Format time
    if (this.elements.previewTime) {
      this.elements.previewTime.textContent = this.formatTime(now, timeFormat, language);
    }

    // Format number
    if (this.elements.previewNumber) {
      const sampleNumber = 1234567.89;
      this.elements.previewNumber.textContent = new Intl.NumberFormat(numberFormat).format(sampleNumber);
    }

    // Format currency
    if (this.elements.previewCurrency) {
      const sampleAmount = 1234567.89;
      this.elements.previewCurrency.textContent = new Intl.NumberFormat(numberFormat, {
        style: 'currency',
        currency: 'USD',
      }).format(sampleAmount);
    }
  }

  /**
   * Format date according to preference
   */
  formatDate(date, format, locale) {
    try {
      // Use Intl.DateTimeFormat with locale
      const options = { year: 'numeric', month: 'short', day: 'numeric' };
      return new Intl.DateTimeFormat(locale, options).format(date);
    } catch (error) {
      // Fallback to simple formatting
      return date.toLocaleDateString();
    }
  }

  /**
   * Format time according to preference
   */
  formatTime(date, format, locale) {
    try {
      const options = {
        hour: 'numeric',
        minute: '2-digit',
        hour12: format === '12h',
      };
      return new Intl.DateTimeFormat(locale, options).format(date);
    } catch (error) {
      return date.toLocaleTimeString();
    }
  }

  /**
   * Apply selected theme immediately
   */
  applyTheme() {
    const themeId = this.elements.activeTheme?.value;
    if (!themeId) return;

    // Store in localStorage
    localStorage.setItem('activeThemeId', themeId);

    // Fire theme changed event
    const theme = this.themes.find((t) => String(t.id) === String(themeId));
    eventBus.emit(Events.THEME_CHANGED, {
      themeId: themeId,
      themeName: theme?.name || 'Unknown',
    });

    // Note: Actual theme CSS application is handled by Style Manager or app-level listener
  }

  /**
   * Save preferences
   */
  async savePreferences() {
    const previousLanguage = this.preferences.language;

    // Gather form values
    const newPreferences = {
      language: this.elements.language?.value || 'en-US',
      dateFormat: this.elements.dateFormat?.value || 'MM/DD/YYYY',
      timeFormat: this.elements.timeFormat?.value || '12h',
      numberFormat: this.elements.numberFormat?.value || 'en-US',
      defaultDatabase: this.elements.defaultDatabase?.value || '',
      activeTheme: this.elements.activeTheme?.value || '',
    };

    // Update local state
    this.preferences = newPreferences;

    // Save to localStorage
    localStorage.setItem('lithium_preferences', JSON.stringify(this.preferences));

    // Save to API
    try {
      await this.savePreferencesToApi(newPreferences);
    } catch (error) {
      log(Subsystems.MANAGER, Status.WARN, '[ProfileManager] API save failed, preferences stored locally:', error);
    }

    // Check if language changed
    const languageChanged = previousLanguage !== newPreferences.language;

    // Fire appropriate events
    if (languageChanged) {
      eventBus.emit(Events.LOCALE_CHANGED, {
        lang: newPreferences.language,
        formats: {
          date: newPreferences.dateFormat,
          time: newPreferences.timeFormat,
          number: newPreferences.numberFormat,
        },
      });
    }

    // Show success notification
    this.showToast('Preferences saved successfully');

    // Update preview
    this.updatePreview();
  }

  /**
   * Save preferences to API
   */
  async savePreferencesToApi(preferences) {
    const serverUrl = getConfigValue('server.url', 'http://localhost:8080');
    const apiPrefix = getConfigValue('server.api_prefix', '/api');

    const response = await fetch(`${serverUrl}${apiPrefix}/user/preferences`, {
      method: 'PUT',
      headers: {
        'Content-Type': 'application/json',
        'Accept': 'application/json',
      },
      body: JSON.stringify(preferences),
    });

    if (!response.ok) {
      throw new Error(`API error: ${response.status}`);
    }

    // Check for new JWT in response (if language changed)
    const data = await response.json();
    if (data.token) {
      storeJWT(data.token);
      eventBus.emit(Events.AUTH_RENEWED, { expiresAt: data.expires_at });
    }

    return data;
  }

  /**
   * Reset preferences to defaults
   */
  resetPreferences() {
    if (confirm('Reset all preferences to default values?')) {
      this.preferences = { ...this.defaultPreferences };
      this.applyPreferencesToForm();
      this.updatePreview();
      this.showToast('Preferences reset to defaults');
    }
  }

  /**
   * Show toast notification
   */
  showToast(message, type = 'success') {
    if (!this.elements.toast) return;

    const toastMessage = this.elements.toast.querySelector('.toast-message');
    if (toastMessage) {
      toastMessage.textContent = message;
    }

    // Update icon based on type
    const icon = this.elements.toast.querySelector('i');
    if (icon) {
      setIcon(icon, type === 'error' ? 'exclamation-circle' : 'check-circle');
    }

    // Update border color
    this.elements.toast.classList.toggle('error', type === 'error');

    // Show
    this.elements.toast.classList.add('visible');

    // Hide after delay
    setTimeout(() => {
      this.elements.toast?.classList.remove('visible');
    }, 3000);
  }

  /**
   * Show the profile manager
   */
  show() {
    requestAnimationFrame(() => {
      this.elements.page?.classList.add('visible');
    });
  }

  /**
   * Teardown the profile manager
   */
  teardown() {
    // Clear preview timer
    if (this.previewTimer) {
      clearTimeout(this.previewTimer);
      this.previewTimer = null;
    }

    // Clean up edit helper
    this.editHelper?.destroy();

    // Clean up splitter
    this.splitter?.destroy();
    this.splitter = null;

    // Clean up table
    this.optionsTable?.destroy();
    this.optionsTable = null;

    // Clear elements
    this.elements = {};
  }
}
