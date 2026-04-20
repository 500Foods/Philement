/**
 * Profile Manager
 *
 * User preferences and profile settings management.
 * Uses index-based navigation where internal sections have negative indices
 * and manager sections use actual manager IDs.
 */

import { eventBus, Events } from '../../core/event-bus.js';
import { getClaims, storeJWT } from '../../core/jwt.js';
import { getConfigValue } from '../../core/config.js';
import { createRequest } from '../../core/json-request.js';
import { LithiumTable } from '../../tables/lithium-table-main.js';
import { LithiumSplitter } from '../../core/lithium-splitter.js';
import { PanelStateManager } from '../../core/panel-state-manager.js';
import {
  togglePanelCollapse,
  restorePanelState as restoreCollapsedPanelState,
} from '../../core/panel-collapse.js';
import { authQuery } from '../../shared/conduit.js';
import { log, Subsystems, Status } from '../../core/log.js';
import { processIcons } from '../../core/icons.js';
import { initToolbars, setupManagerFooterIcons } from '../../core/manager-ui.js';
import { ManagerEditHelper } from '../../core/manager-edit-helper.js';
import { getMenu, buildManagerIconsRegistry } from '../../shared/menu.js';
import { CollectionTabHandler } from './profile-manager-collection.js';
import { SettingsTabHandler } from './profile-manager-settings.js';
import { toast } from '../../shared/toast.js';
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

    this.managerId = 3;  // Matches lithium.json "003.Profile"
    this.optionsTable = null;
    this.userOptions = [];

    this.leftPanelState = new PanelStateManager('lithium_profile_left');
    this.splitter = null;
    this.leftPanelWidth = this.leftPanelState.loadWidth(314);
    this.isLeftPanelCollapsed = this.leftPanelState.loadCollapsed(false);

    this.editHelper = new ManagerEditHelper({ name: 'Profile' });

    this.defaultPreferences = {
      language: 'en-US',
      dateFormat: 'MM/DD/YYYY',
      timeFormat: '12h',
      numberFormat: 'en-US',
      defaultDatabase: '',
      activeTheme: '',
    };

    this.currentTab = 'settings';
    this.collectionHandler = null;
    this.settingsHandler = null;

    this._menuData = null;
    this._managerIcons = {};
    this._managerGroups = new Map(); // managerId -> groupName
  }

  /**
   * Initialize the profile manager
   */
  async init() {
    this.api = createRequest();

    await this.render();
    this.loadUserInfo();

    // Load menu data first (needed for merging with Lookup 60)
    await this.loadMenuData();

    await this.initOptionsTable();
    this.setupSplitter();

    await Promise.all([this.loadThemes(), this.loadDatabases()]);
    this.loadPreferences();
    this.setupEventListeners();
    this.restorePanelState();
    this.initTabHandlers();
    this.setupFooter();
    this.show();
  }

  /**
   * Load menu data and build manager icons registry and groups
   */
  async loadMenuData() {
    try {
      this._menuData = await getMenu(this.app.api);
      this._managerIcons = buildManagerIconsRegistry(this._menuData);
      this._buildManagerGroups();
      log(Subsystems.MANAGER, Status.INFO,
        `[ProfileManager] Loaded ${Object.keys(this._managerIcons).length} manager icons ` +
        `across ${new Set(this._managerGroups.values()).size} groups`);
    } catch (error) {
      log(Subsystems.MANAGER, Status.WARN, '[ProfileManager] Failed to load menu data:', error);
      this._managerIcons = {};
      this._managerGroups.clear();
    }
  }

  /**
   * Build manager groups mapping from menu data
   */
  _buildManagerGroups() {
    this._managerGroups.clear();

    if (!this._menuData || !Array.isArray(this._menuData)) return;

    this._menuData.forEach((group) => {
      const groupName = group.name || 'Modules';
      group.items?.forEach((item) => {
        if (item.managerId) {
          this._managerGroups.set(item.managerId, groupName);
        }
      });
    });
  }

  /**
   * Get the group name for a manager ID
   * @param {number} managerId - Manager ID
   * @returns {string}
   */
  _getManagerGroup(managerId) {
    return this._managerGroups.get(managerId) || 'Modules';
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

    this.cacheElements();
    processIcons(this.container);
    initToolbars();
  }

  /**
   * Cache DOM element references
   */
  cacheElements() {
    this.elements = {
      page: this.container.querySelector('#profile-manager-page'),
      leftPanel: this.container.querySelector('#profile-left-panel'),
      rightPanel: this.container.querySelector('#profile-right-panel'),
      tableContainer: this.container.querySelector('#profile-table-container'),
      navigatorContainer: this.container.querySelector('#profile-navigator'),
      splitter: this.container.querySelector('#profile-splitter'),
      collapseBtn: this.container.querySelector('#profile-collapse-btn'),
      tabToolbar: this.container.querySelector('#profile-tab-toolbar'),
      tabCollection: this.container.querySelector('#tab-collection'),
      panelSettings: this.container.querySelector('#panel-settings'),
      panelCollection: this.container.querySelector('#panel-collection'),
      collectionEditorContainer: this.container.querySelector('#collection-editor-container'),
      sectionLabelBtn: this.container.querySelector('#btn-section-label'),
      btnUndo: this.container.querySelector('#btn-undo'),
      btnRedo: this.container.querySelector('#btn-redo'),
      btnFold: this.container.querySelector('#btn-fold'),
      btnUnfold: this.container.querySelector('#btn-unfold'),
      btnFont: this.container.querySelector('#btn-font'),
      btnPrettify: this.container.querySelector('#btn-prettify'),
    };
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
   * Initialize the User Options table
   */
  async initOptionsTable() {
    if (!this.elements.tableContainer || !this.elements.navigatorContainer) return;

    this.optionsTable = new LithiumTable({
      container: this.elements.tableContainer,
      navigatorContainer: this.elements.navigatorContainer,
      tablePath: 'profile-manager/user-options',
      lookupKeyIdx: 4,
      cssPrefix: 'profile-options',
      storageKey: 'profile_options_table',
      app: this.app,
      readonly: true,
      panel: this.elements.leftPanel,
      panelStateManager: this.leftPanelState,
      onRowSelected: (rowData) => this.handleOptionSelected(rowData),
      onRowDeselected: () => this.handleOptionDeselected(),
      onRefresh: () => this.loadUserOptions(),
    });

    this.editHelper.registerTable(this.optionsTable);
    await this.optionsTable.init();
    await this.loadUserOptions();
  }

  /**
   * Load user options from Lookup 60 and merge with Main Menu data
   */
  async loadUserOptions() {
    if (!this.app?.api || !this.optionsTable?.table) return;

    try {
      this.optionsTable.showLoading();

      const rows = await authQuery(this.app.api, 26, { INTEGER: { LOOKUPID: 60 } });
      const profileSectionsRow = rows?.find(row => row.key_idx === 0);

      if (profileSectionsRow && profileSectionsRow.collection) {
        let collection = profileSectionsRow.collection;
        if (typeof collection === 'string') {
          try {
            collection = JSON.parse(collection);
          } catch (_e) {
            collection = {};
          }
        }

        const locale = navigator.language || 'en-US';
        const sections = collection[locale] || collection.default || [];

        // Transform sections, merging with menu data for manager entries
        this.userOptions = this.transformSections(sections);
      } else {
        this.userOptions = this.getDefaultUserOptions();
      }

      this.optionsTable.loadStaticData(this.userOptions);
      log(Subsystems.TABLE, Status.INFO,
        `[ProfileManager] Loaded ${this.userOptions.length} user options`);
    } catch (error) {
      log(Subsystems.TABLE, Status.ERROR,
        `[ProfileManager] Failed to load user options: ${error.message}`);
      this.userOptions = this.getDefaultUserOptions();
      this.optionsTable.loadStaticData(this.userOptions);
    } finally {
      this.optionsTable.hideLoading();
    }
  }

  /**
   * Transform sections from Lookup 60, merging with menu data
   * Internal sections have negative indices, managers use actual IDs
   */
  transformSections(sections) {
    return sections.map((section, position) => {
      const index = section[3] ?? (position + 1);
      const isInternal = index < 0;

      // For manager entries (index >= 0), merge with menu data
      if (!isInternal && this._managerIcons[index]) {
        const menuInfo = this._managerIcons[index];
        const groupName = this._getManagerGroup(index);
        return {
          key_idx: position + 1,
          section: groupName,
          icon: menuInfo.iconHtml || section[1] || '<fa fa-cube></fa>',
          label: menuInfo.name || section[2] || 'Unknown',
          index: index,
          status_a1: 1,
        };
      }

      // Internal sections or managers not in menu
      return {
        key_idx: position + 1,
        section: section[0] || '',
        icon: section[1] || '',
        label: section[2] || '',
        index: index,
        status_a1: 1,
      };
    });
  }

  /**
   * Get default user options when lookup data is not available
   */
  getDefaultUserOptions() {
    const baseOptions = [
      { key_idx: 1, section: 'General', icon: '<fa fa-id-card></fa>', label: 'Account', index: -1, status_a1: 1 },
      { key_idx: 2, section: 'General', icon: '<fa fa-id-badge></fa>', label: 'Names', index: -2, status_a1: 1 },
      { key_idx: 3, section: 'General', icon: '<fa fa-address-book></fa>', label: 'Addresses', index: -3, status_a1: 1 },
      { key_idx: 4, section: 'General', icon: '<fa fa-at></fa>', label: 'E-Mail', index: -4, status_a1: 1 },
      { key_idx: 5, section: 'General', icon: '<fa fa-phone></fa>', label: 'Phone', index: -5, status_a1: 1 },
      { key_idx: 6, section: 'Security', icon: '<fa fa-key></fa>', label: 'Authentication', index: -6, status_a1: 1 },
      { key_idx: 7, section: 'Security', icon: '<fa fa-user-key></fa>', label: 'Tokens', index: -7, status_a1: 1 },
      { key_idx: 8, section: 'Formatting', icon: '<fa fa-globe></fa>', label: 'Language', index: -8, status_a1: 1 },
      { key_idx: 9, section: 'Formatting', icon: '<fa fa-calendar></fa>', label: 'Date Formats', index: -9, status_a1: 1 },
      { key_idx: 10, section: 'Formatting', icon: '<fa fa-00></fa>', label: 'Number Formats', index: -10, status_a1: 1 },
      { key_idx: 11, section: 'Application', icon: '<fa fa-atom-simple></fa>', label: 'Startup', index: -11, status_a1: 1 },
      { key_idx: 12, section: 'Application', icon: '<fa fa-bell></fa>', label: 'Notifications', index: -12, status_a1: 1 },
      { key_idx: 13, section: 'Application', icon: '<fa fa-bell-concierge></fa>', label: 'Concierge', index: -13, status_a1: 1 },
      { key_idx: 14, section: 'Application', icon: '<fa fa-note-sticky></fa>', label: 'Annotations', index: -14, status_a1: 1 },
      { key_idx: 15, section: 'Application', icon: '<fa fa-signs-post></fa>', label: 'Tours', index: -15, status_a1: 1 },
      { key_idx: 16, section: 'Application', icon: '<fa fa-graduation-cap></fa>', label: 'Training', index: -16, status_a1: 1 },
      { key_idx: 17, section: 'Security', icon: '<fa fa-receipt></fa>', label: 'Login History', index: -17, status_a1: 1 },
    ];

    // Add manager entries from menu data, using group names from Main Menu
    const managerOptions = Object.entries(this._managerIcons).map(([id, info], position) => {
      const managerId = parseInt(id, 10);
      const groupName = this._getManagerGroup(managerId);
      return {
        key_idx: 100 + position,
        section: groupName,
        icon: info.iconHtml || '<fa fa-cube></fa>',
        label: info.name || `Manager ${id}`,
        index: managerId,
        status_a1: 1,
      };
    });

    return [...baseOptions, ...managerOptions];
  }

  /**
   * Handle when an option is selected from the table
   */
  async handleOptionSelected(rowData) {
    if (!rowData) return;

    log(Subsystems.MANAGER, Status.INFO,
      `[ProfileManager] Option selected: ${rowData.label}, index: ${rowData.index}`);

    // Update section label button - find fa element and span inside the button
    if (this.elements.sectionLabelBtn && rowData.label) {
      this.elements.sectionLabelBtn.classList.add('active');
      const iconEl = this.elements.sectionLabelBtn.querySelector('fa');
      const labelEl = this.elements.sectionLabelBtn.querySelector('span');
      if (iconEl && rowData.icon) {
        iconEl.outerHTML = rowData.icon;
        processIcons(this.elements.sectionLabelBtn);
      }
      if (labelEl) {
        labelEl.textContent = rowData.label;
      }
    }

    // Switch to settings tab if needed
    if (this.currentTab !== 'settings') {
      this.switchTab('settings');
    }

    // Show the settings page
    await this.settingsHandler?.showPage(rowData.index, rowData);
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
   * Initialize tab handlers
   */
  initTabHandlers() {
    // Settings tab handler
    this.settingsHandler = new SettingsTabHandler({
      container: this.elements.panelSettings,
      app: this.app,
      onPageChange: (index, data) => {
        log(Subsystems.MANAGER, Status.DEBUG, `[ProfileManager] Page changed to ${index}`);
      },
      onDirtyChange: (dirty) => {
        this.editHelper.setDirty(dirty);
      },
    });
    this.settingsHandler.init();

    // Collection tab handler
    this.collectionHandler = new CollectionTabHandler({
      container: this.elements.collectionEditorContainer,
      onDirtyChange: (dirty) => {
        this.editHelper.setDirty(dirty);
        this.updateFooterButtons(dirty);
      },
    });
  }

  /**
   * Switch between tabs
   */
  switchTab(tabId) {
    if (!['settings', 'collection'].includes(tabId)) return;

    this.currentTab = tabId;

    this.elements.tabCollection?.classList.toggle('active', tabId === 'collection');
    this.elements.sectionLabelBtn?.classList.toggle('active', tabId === 'settings');
    this.elements.panelSettings?.classList.toggle('active', tabId === 'settings');
    this.elements.panelCollection?.classList.toggle('active', tabId === 'collection');

    // Enable/disable editor controls
    const isCollection = tabId === 'collection';
    this.setEditorButtonsDisabled(!isCollection);

    // Update footer buttons
    const canSave = isCollection && this.collectionHandler?.isDirty();
    this.updateFooterButtons(canSave);

    // Initialize Collection editor when switching to Collection tab
    if (tabId === 'collection') {
      this.collectionHandler?.init(this.getCollectionData());
    }

    log(Subsystems.MANAGER, Status.DEBUG, `[ProfileManager] Switched to tab: ${tabId}`);
  }

  /**
   * Set disabled state for editor toolbar buttons
   */
  setEditorButtonsDisabled(disabled) {
    const buttons = this.container.querySelectorAll('[data-editor-btn]');
    buttons.forEach(btn => {
      btn.disabled = disabled;
      btn.classList.toggle('disabled', disabled);
    });
  }

  /**
   * Load user info from JWT claims
   */
  loadUserInfo() {
    const claims = getClaims();
    if (!claims) return;

    this.user = {
      id: claims.user_id,
      username: claims.username || 'User',
      email: claims.email || '-',
      roles: claims.roles || [],
      database: claims.database || '-',
    };

    // Update UI elements if they exist
    const usernameEl = this.container.querySelector('#profile-username');
    const emailEl = this.container.querySelector('#profile-email');
    const rolesEl = this.container.querySelector('#profile-roles');
    const dbEl = this.container.querySelector('#profile-database');

    if (usernameEl) usernameEl.textContent = this.user.username;
    if (emailEl) emailEl.textContent = this.user.email;
    if (rolesEl) rolesEl.textContent = Array.isArray(this.user.roles)
      ? this.user.roles.join(', ') : this.user.roles;
    if (dbEl) dbEl.textContent = this.user.database;
  }

  /**
   * Load themes from localStorage or use defaults
   */
  async loadThemes() {
    const storedThemes = localStorage.getItem('lithium_themes');
    if (storedThemes) {
      try {
        this.themes = JSON.parse(storedThemes);
      } catch (error) {
        log(Subsystems.MANAGER, Status.WARN, '[ProfileManager] Failed to parse themes:', error);
      }
    }

    if (!this.themes?.length) {
      this.themes = [
        { id: 'default', name: 'Default Dark' },
        { id: 1, name: 'Midnight Indigo' },
        { id: 2, name: 'Ocean Breeze' },
        { id: 3, name: 'Sunset Glow' },
      ];
    }

    this.populateThemeSelect();
  }

  /**
   * Populate theme select dropdown
   */
  populateThemeSelect() {
    const select = this.container.querySelector('#pref-theme');
    if (!select) return;

    select.innerHTML = '<option value="">Select a theme...</option>';
    this.themes.forEach((theme) => {
      const option = document.createElement('option');
      option.value = theme.id;
      option.textContent = theme.name;
      select.appendChild(option);
    });
  }

  /**
   * Load databases (mock for now)
   */
  async loadDatabases() {
    this.databases = [
      { id: 'Demo_PG', name: 'Demo PostgreSQL' },
      { id: 'Demo_MySQL', name: 'Demo MySQL' },
      { id: 'Demo_SQLite', name: 'Demo SQLite' },
    ];

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
    const select = this.container.querySelector('#pref-database');
    if (!select) return;

    select.innerHTML = '<option value="">Select a database...</option>';
    this.databases.forEach((db) => {
      const option = document.createElement('option');
      option.value = db.id;
      option.textContent = db.name;
      select.appendChild(option);
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

    this.applyPreferencesToForm();
  }

  /**
   * Apply preferences to form fields
   */
  applyPreferencesToForm() {
    const fields = {
      '#pref-language': this.preferences.language,
      '#pref-date-format': this.preferences.dateFormat,
      '#pref-time-format': this.preferences.timeFormat,
      '#pref-number-format': this.preferences.numberFormat,
      '#pref-database': this.preferences.defaultDatabase,
      '#pref-theme': this.preferences.activeTheme,
    };

    Object.entries(fields).forEach(([selector, value]) => {
      const el = this.container.querySelector(selector);
      if (el) el.value = value;
    });
  }

  /**
   * Setup event listeners
   */
  setupEventListeners() {
    this.elements.collapseBtn?.addEventListener('click', () => this.toggleLeftPanel());
    this.elements.sectionLabelBtn?.addEventListener('click', () => this.switchTab('settings'));
    this.elements.tabCollection?.addEventListener('click', () => this.switchTab('collection'));

    // Collection editor toolbar buttons
    this.elements.btnUndo?.addEventListener('click', () => this.collectionHandler?.undo());
    this.elements.btnRedo?.addEventListener('click', () => this.collectionHandler?.redo());
    this.elements.btnFold?.addEventListener('click', () => this.collectionHandler?.foldAll());
    this.elements.btnUnfold?.addEventListener('click', () => this.collectionHandler?.unfoldAll());
    this.elements.btnPrettify?.addEventListener('click', () => this.collectionHandler?.prettify());
  }

  /**
   * Setup footer with Save/Cancel buttons
   */
  setupFooter() {
    const slot = this.container.closest('.manager-slot');
    if (!slot) return;

    const footer = slot.querySelector('.manager-slot-footer');
    if (!footer) return;

    const group = footer.querySelector('.subpanel-header-group');
    if (!group) return;

    const placeholder = group.querySelector('.slot-footer-placeholder');

    const footerElements = setupManagerFooterIcons(group, {
      anchor: placeholder,
      showSaveCancel: true,
      onSave: () => this.handleSaveCollection(),
      onCancel: () => this.handleCancelCollection(),
    });

    this._footerSaveBtn = footerElements.saveBtn;
    this._footerCancelBtn = footerElements.cancelBtn;

    this.editHelper.wireFooterButtons(
      footerElements.saveBtn,
      footerElements.cancelBtn,
      footerElements.dummyBtn,
    );

    this.editHelper.activeEditingTable = {
      handleSave: () => this.handleSaveCollection(),
      handleCancel: () => this.handleCancelCollection(),
    };
  }

  /**
   * Enable/disable footer Save/Cancel buttons
   */
  updateFooterButtons(enabled) {
    if (this._footerSaveBtn) this._footerSaveBtn.disabled = !enabled;
    if (this._footerCancelBtn) this._footerCancelBtn.disabled = !enabled;
  }

  /**
   * Get preferences as collection data
   */
  getCollectionData() {
    return {
      language: this.preferences.language || 'en-US',
      dateFormat: this.preferences.dateFormat || 'MM/DD/YYYY',
      timeFormat: this.preferences.timeFormat || '12h',
      numberFormat: this.preferences.numberFormat || 'en-US',
      defaultDatabase: this.preferences.defaultDatabase || '',
      activeTheme: this.preferences.activeTheme || '',
    };
  }

  /**
   * Handle save from Collection footer button
   */
  async handleSaveCollection() {
    if (!this.collectionHandler) return;

    try {
      const data = this.collectionHandler.getData();
      if (!data) throw new Error('Invalid JSON data');

      this.preferences = { ...this.preferences, ...data };
      localStorage.setItem('lithium_preferences', JSON.stringify(this.preferences));

      await this.savePreferencesToApi(this.preferences);
      this.collectionHandler._initialData = JSON.parse(JSON.stringify(data));

      this.showToast('Preferences saved successfully');
      this.updateFooterButtons(false);
      log(Subsystems.MANAGER, Status.INFO, '[ProfileManager] Collection saved');
    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, '[ProfileManager] Save failed:', error);
      this.showToast('Failed to save: ' + error.message, 'error');
    }
  }

  /**
   * Handle cancel from Collection footer button
   */
  async handleCancelCollection() {
    await this.collectionHandler?.setData(this.getCollectionData());
    this.showToast('Changes cancelled');
    log(Subsystems.MANAGER, Status.INFO, '[ProfileManager] Collection cancelled');
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

    const data = await response.json();
    if (data.token) {
      storeJWT(data.token);
      eventBus.emit(Events.AUTH_RENEWED, { expiresAt: data.expires_at });
    }

    return data;
  }

  /**
   * Show toast notification using global toast system
   */
  showToast(message, type = 'success') {
    const toastType = type === 'error' ? 'error' : 'success';
    toast[toastType](message, { duration: 3000 });
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
    this.collectionHandler?.destroy();
    this.collectionHandler = null;

    this.settingsHandler?.destroy();
    this.settingsHandler = null;

    this.editHelper?.destroy();
    this.splitter?.destroy();
    this.optionsTable?.destroy();

    this.elements = {};
  }
}
