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
import { LithiumSplitter } from '../../core/lithium-splitter.js';
import { PanelStateManager } from '../../core/panel-state-manager.js';
import {
  togglePanelCollapse,
  restorePanelState as restoreCollapsedPanelState,
} from '../../core/panel-collapse.js';
import { log, Subsystems, Status } from '../../core/log.js';
import { processIcons } from '../../core/icons.js';
import { initToolbars, setupManagerFooterIcons, createFontPopup, closeAllPopups } from '../../core/manager-ui.js';
import { ManagerEditHelper } from '../../core/manager-edit-helper.js';
import { getMenu, buildManagerIconsRegistry } from '../../shared/menu.js';
import { CollectionTabHandler } from './profile-manager-collection.js';
import { SettingsTabHandler } from './profile-manager-settings.js';
import { ProfileManagerTable } from './profile-manager-table.js';

import { toast } from '../../shared/toast.js';
import '../../styles/vendor-tabulator.css';
import '../../core/manager-panels.css';
import '../../core/manager-ui.css';
import '../../core/popup.css';
import 'flatpickr/dist/flatpickr.css';
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
    this.pendingExternalSection = null;

    this.leftPanelState = new PanelStateManager('lithium_profile_left');
    this.splitter = null;
    this.leftPanelWidth = this.leftPanelState.loadWidth(314);
    this.isLeftPanelCollapsed = this.leftPanelState.loadCollapsed(false);

    this.editHelper = new ManagerEditHelper({ name: 'Profile' });

    // Initialize sub-modules
    this.tableManager = new ProfileManagerTable(this);

    // Use the global settings service with sectioned access wrapper
    const globalSettings = window.lithiumSettings;
    this.settingsService = {
      getSection: (sectionKey, path, defaultValue) => {
        const fullPath = path ? `${sectionKey}.${path}` : sectionKey;
        return globalSettings.get(fullPath, defaultValue);
      },
      setSection: (sectionKey, path, value) => {
        const fullPath = path ? `${sectionKey}.${path}` : sectionKey;
        globalSettings.set(fullPath, value);
        // Sync to collection tab
        this.collectionHandler?.setData(this.getCollectionData());
      },
      removeSection: (sectionKey) => {
        // Remove by setting to undefined which deletes the key
        globalSettings.removeSection(sectionKey);
        // Sync to collection tab
        this.collectionHandler?.setData(this.getCollectionData());
      },
      get: (path, defaultValue) => globalSettings.get(path, defaultValue),
      set: (path, value) => globalSettings.set(path, value),
      getSetting: (path, defaultValue) => globalSettings.get(path, defaultValue),
      getAll: () => globalSettings.getAll(),
      onChange: (callback) => globalSettings.onChange(callback),
    };

    // Initialize default formats in settings if not present
    this._initializeDefaultFormats();

    // Load saved font settings
    const savedFontSettings = this.settingsService.getSetting('collection.font', {});
    if (savedFontSettings.fontSize) this.editorFontSize = savedFontSettings.fontSize;
    if (savedFontSettings.fontFamily) this.editorFontFamily = savedFontSettings.fontFamily;

    // Storage key for selected section persistence
    this._selectedSectionStorageKey = 'lithium_profile_selected_section';

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

    // Tab transition state
    this._inTabTransition = false;

    // Font popup
    this.fontPopup = null;
    this.editorFontSize = 13;
    this.editorFontFamily = '"Vanadium Mono", var(--font-mono, monospace)';

    this._menuData = null;
    this._managerIcons = {};
    this._managerGroups = new Map(); // managerId -> groupName
  }

  /**
   * Initialize the profile manager
   */
  async init() {
    log(Subsystems.MANAGER, Status.INFO, '[ProfileManager] init() START');
    this.api = createRequest();

    await this.render();
    this.loadUserInfo();

    // Load menu data first (needed for merging with Lookup 60)
    await this.loadMenuData();

    // Initialize tab handlers BEFORE table so settingsHandler is ready for section restoration
    this.initTabHandlers();

    // Initialize to settings tab to set button states properly
    this.switchTab('settings');

    await this.tableManager.initOptionsTable();
    log(Subsystems.MANAGER, Status.INFO, `[ProfileManager] initOptionsTable completed, tableManager.optionsTable=${!!this.tableManager?.optionsTable}, table=${!!this.tableManager?.optionsTable?.table}`);

    this.setupSplitter();
    this.setupSplitter();

    await Promise.all([this.loadThemes(), this.loadDatabases()]);
    this.loadPreferences();
    this.setupEventListeners();
    this.restorePanelState();
    this.setupFooter();
    this.initFontPopup();
    this.overrideCloseButton();
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
    const response = await fetch('/src/managers/profile-manager/profile-manager.html');
    this.container.innerHTML = await response.text();

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
      settingsService: this.settingsService,
      onPageChange: (index, _data) => {
        log(Subsystems.MANAGER, Status.DEBUG, `[ProfileManager] Page changed to ${index}`);
      },
      onDirtyChange: (dirty) => {
        this.updateFooterButtons(dirty);
      },
    });
    this.settingsHandler.init();

    // Collection tab handler
    this.collectionHandler = new CollectionTabHandler({
      container: this.elements.collectionEditorContainer,
      parent: this.container, // Manager main container (sibling of toolbar)
      onDirtyChange: (dirty) => {
        this.updateFooterButtons(dirty);
      },
      onSave: () => this.handleSaveCollection(),
      onCancel: () => this.handleCancelCollection(),
    });

    // Register collection editor with editHelper for dirty tracking
    this.editHelper.registerEditor('collection', {
      getContent: () => this.collectionHandler?.getContent() || '{}',
      setContent: (content) => {
        try {
          const data = JSON.parse(content);
          this.collectionHandler?.setData(data);
        } catch (e) {
          log(Subsystems.MANAGER, Status.WARN, '[ProfileManager] Failed to set collection content:', e);
        }
      },
      setEditable: (editable) => this.collectionHandler?.setEditable(editable),
      boundTable: null,
    });
  }

  /**
   * Switch between tabs
   */
  switchTab(tabId) {
    if (!['settings', 'collection'].includes(tabId)) return;

    // Guard: same tab requested or already transitioning
    if (tabId === this.currentTab || this._inTabTransition) return;

    this._inTabTransition = true;

    closeAllPopups();

    // Update button states immediately
    this.elements.tabCollection?.classList.toggle('active', tabId === 'collection');
    const hasPageSelected = this.settingsHandler?.getCurrentPage() !== null;
    this.elements.sectionLabelBtn?.classList.toggle('active', tabId === 'settings' && hasPageSelected);

    // Update state
    this.currentTab = tabId;

    // Enable/disable editor controls
    const isCollection = tabId === 'collection';
    this.setEditorButtonsDisabled(!isCollection);

    // Show/hide panels immediately
    this.elements.panelSettings?.classList.toggle('active', tabId === 'settings');
    this.elements.panelCollection?.classList.toggle('active', tabId === 'collection');

    // Init collection editor after panel is active to ensure proper sizing
    if (tabId === 'collection') {
      this.collectionHandler?.init(this.getCollectionData());
      // Request measure after a frame to ensure layout is updated
      requestAnimationFrame(() => {
        this.collectionHandler?.editor?.requestMeasure();
      });
    }

    this._inTabTransition = false;

    // Update footer buttons — on collection tab, dirty state drives save/cancel
    if (isCollection) {
      const canSave = this.collectionHandler?.isDirty() || false;
      this.updateFooterButtons(canSave);
    } else {
      this.updateFooterButtons(false);
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
    // Listen for external section selection requests
    this._eventListenerBound = this.handleExternalSectionSelect.bind(this);
    eventBus.on('profile:select-section', this._eventListenerBound);

    this.elements.collapseBtn?.addEventListener('click', () => this.toggleLeftPanel());
    this.elements.sectionLabelBtn?.addEventListener('click', () => this.switchTab('settings'));
    this.elements.tabCollection?.addEventListener('click', () => this.switchTab('collection'));

    // Collection editor toolbar buttons
    this.elements.btnUndo?.addEventListener('click', () => this.collectionHandler?.undo());
    this.elements.btnRedo?.addEventListener('click', () => this.collectionHandler?.redo());
    this.elements.btnFold?.addEventListener('click', () => this.collectionHandler?.foldAll());
    this.elements.btnUnfold?.addEventListener('click', () => this.collectionHandler?.unfoldAll());
    this.elements.btnPrettify?.addEventListener('click', () => this.collectionHandler?.prettify());

    // Font button — toggle font popup
    this.elements.btnFont?.addEventListener('click', (e) => this.toggleFontPopup(e));


  }





  /**
   * Handle external request to select a section
   * @param {Object} data - Event data with index property
   */
  async handleExternalSectionSelect(data) {
    const { index } = data;
    if (index === undefined) return;

    log(Subsystems.MANAGER, Status.INFO, `[ProfileManager] External section select: ${index}`);

    // Store the pending selection
    this.pendingExternalSection = index;

    // If table is already loaded with data, apply immediately
    if (this.tableManager?.optionsTable?.table && this.userOptions?.length > 0) {
      await this._applyExternalSectionSelection();
    }
    // Otherwise, it will be applied after table loading in _restoreSelectedSection
  }

  /**
   * Select a section externally (called from main manager)
   * @param {number} index - The section index to select
   */
  async selectSection(index) {
    log(Subsystems.MANAGER, Status.INFO, `[ProfileManager] External selectSection: ${index}, this=${!!this}, ctor=${this?.constructor?.name}`);
    this.pendingExternalSection = index;

    log(Subsystems.MANAGER, Status.INFO,
      `[ProfileManager] selectSection: tableManager=${!!this.tableManager}, optionsTable=${!!this.tableManager?.optionsTable}, table=${!!this.tableManager?.optionsTable?.table}`);

    // Wait for table to be ready
    let attempts = 0;
    while (!this.tableManager?.optionsTable?.table && attempts < 100) {
      await new Promise(resolve => setTimeout(resolve, 50));
      attempts++;
    }

    log(Subsystems.MANAGER, Status.INFO,
      `[ProfileManager] selectSection: after while, attempts=${attempts}, table=${!!this.tableManager?.optionsTable?.table}`);

    if (!this.tableManager?.optionsTable?.table) {
      log(Subsystems.MANAGER, Status.ERROR, `[ProfileManager] Table still not ready after waiting (${attempts} attempts)`);
      this.pendingExternalSection = null;
      return;
    }

    log(Subsystems.MANAGER, Status.INFO, `[ProfileManager] Table is ready, calling _applyExternalSectionSelection, pending=${this.pendingExternalSection}`);

    // Clear any existing selection first to avoid conflicts
    this.tableManager.optionsTable.table.deselectRow();
    this.settingsHandler.currentPageIndex = null;

    try {
      await this._applyExternalSectionSelection();
      log(Subsystems.MANAGER, Status.INFO, `[ProfileManager] selectSection: _applyExternalSectionSelection completed`);
    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, `[ProfileManager] Error in _applyExternalSectionSelection: ${error.message}`);
    }
  }

  /**
   * Apply pending external section selection
   * @private
   */
  async _applyExternalSectionSelection() {
    log(Subsystems.MANAGER, Status.INFO, `[ProfileManager] _applyExternalSectionSelection called, pending: ${this.pendingExternalSection}`);
    if (this.pendingExternalSection === null) {
      log(Subsystems.MANAGER, Status.INFO, `[ProfileManager] No pending external section, returning`);
      return;
    }

    const index = this.pendingExternalSection;
    this.pendingExternalSection = null; // Clear pending

    log(Subsystems.MANAGER, Status.INFO, `[ProfileManager] Applying external section select: ${index}`);

    // Ensure we're on the settings tab
    if (this.currentTab !== 'settings') {
      this.switchTab('settings');
      log(Subsystems.MANAGER, Status.INFO, `[ProfileManager] Switched to settings tab`);
    }

    // Find and select the row in the table
    if (this.tableManager?.optionsTable?.table) {
      const table = this.tableManager.optionsTable.table;
      const rows = table.getRows();

      log(Subsystems.MANAGER, Status.INFO, `[ProfileManager] Table has ${rows.length} rows`);

      // Find the row with matching index
      const targetRow = rows.find(row => row.getData().index === index);

      log(Subsystems.MANAGER, Status.INFO, `[ProfileManager] Target row found: ${!!targetRow}`);

      if (targetRow) {
        const rowData = targetRow.getData();

        log(Subsystems.MANAGER, Status.INFO, `[ProfileManager] Selecting: ${rowData.label} (index ${rowData.index})`);

        // Expand the group if it's collapsed - get group directly from row
        const group = targetRow.getGroup?.();
        if (group) {
          group.show(); // Expand the group
        }
        log(Subsystems.MANAGER, Status.INFO, `[ProfileManager] Expanded group for: ${rowData.label}`);

        // Select the row
        table.deselectRow();
        targetRow.select();

        log(Subsystems.MANAGER, Status.INFO, `[ProfileManager] Selected row`);

        // Scroll into view
        targetRow.getElement().scrollIntoView({ behavior: 'smooth', block: 'nearest' });

        // Trigger the selection handler
        await this.tableManager.handleOptionSelected(rowData);

        log(Subsystems.MANAGER, Status.INFO, `[ProfileManager] Triggered selection handler`);
      } else {
        log(Subsystems.MANAGER, Status.WARN, `[ProfileManager] Section with index ${index} not found`);
        const indices = rows.map(r => r.getData().index);
        log(Subsystems.MANAGER, Status.DEBUG, `[ProfileManager] Available indices:`, indices);
      }
    } else {
      log(Subsystems.MANAGER, Status.WARN, `[ProfileManager] Table not available`);
    }
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
      onPrint: () => this._handleFooterPrint(),
      onEmail: () => this._handleFooterEmail(),
      onDownload: () => this._handleFooterDownload(),
      onExport: (value, label) => this._handleFooterExport(value, label),
      exportOptions: [
        { value: 'pdf', label: 'PDF', icon: 'fa-file-pdf', enabled: true },
        { value: 'csv', label: 'CSV', icon: 'fa-file-csv', enabled: true },
        { value: 'json', label: 'JSON', icon: 'fa-brackets-curly', enabled: true },
        { value: 'html', label: 'HTML', icon: 'fa-file-html', enabled: true },
      ],
    });

    this._footerSaveBtn = footerElements.saveBtn;
    this._footerCancelBtn = footerElements.cancelBtn;

    this.editHelper.wireFooterButtons(
      footerElements.saveBtn,
      footerElements.cancelBtn,
      footerElements.dummyBtn,
    );

    // Override editHelper's activeEditingTable to handle save/cancel for the collection tab.
    // The Profile Manager doesn't use LithiumTable edit mode — save/cancel drive the
    // collection JSON editor's dirty state directly.
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

  // ============ FOOTER HANDLERS ============

  /**
   * Handle Print button from footer
   */
  _handleFooterPrint() {
    // Print the current settings page or collection
    if (this.currentTab === 'collection') {
      // Print collection JSON
      const data = this.getCollectionData();
      const printWindow = window.open('', '_blank');
      printWindow.document.write('<pre>' + JSON.stringify(data, null, 2) + '</pre>');
      printWindow.document.close();
      printWindow.print();
    } else {
      // Print the settings page
      window.print();
    }
  }

  /**
   * Handle Email button from footer
   */
  _handleFooterEmail() {
    const data = this.getCollectionData();
    const subject = encodeURIComponent('User Profile Preferences');
    const body = encodeURIComponent(
      'User Profile Preferences:\n\n' +
      Object.entries(data).map(([key, value]) => `${key}: ${value}`).join('\n')
    );
    window.open(`mailto:?subject=${subject}&body=${body}`, '_self');
  }

  /**
   * Handle Download button from footer
   */
  _handleFooterDownload() {
    const data = this.getCollectionData();
    // eslint-disable-next-line no-undef
    const blob = new Blob([JSON.stringify(data, null, 2)], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = 'user-profile-preferences.json';
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
  }

  /**
   * Handle Export button from footer
   */
  _handleFooterExport(value, label) {
    log(Subsystems.MANAGER, Status.INFO, `[ProfileManager] Export to ${label} (${value})`);
    // The export format is already saved by the footer system
    // Actual export would be implemented based on format
    this.showToast(`Export format set to ${label}`);
  }

  /**
   * Get preferences as collection data.
   * Delegates to the Settings Service for the canonical JSON object.
   */
  getCollectionData() {
    return this.settingsService?.getAll() || { ...this.preferences };
  }

  /**
   * Update a specific preference value and sync to collection JSON.
   * Called by settings pages when a preference changes.
   * Delegates to the Settings Service for structured storage.
   * @param {string} key - Preference key
   * @param {*} value - New value
   */
  setPreference(key, value) {
    this.preferences[key] = value;

    // Also write to settings service under a legacy section for compatibility
    this.settingsService?.set('_legacy', key, value);

    // Sync to collection editor if initialized
    if (this.collectionHandler?.editor) {
      this.collectionHandler.setData(this.getCollectionData());
    }

    // Enable save/cancel buttons since preferences changed
    this.updateFooterButtons(true);
    log(Subsystems.MANAGER, Status.DEBUG, `[ProfileManager] Preference updated: ${key} = ${value}`);
  }

  /**
   * Handle save from Collection footer button
   */
  async handleSaveCollection() {
    if (!this.collectionHandler) return;

    try {
      const data = this.collectionHandler.getData();
      if (!data) throw new Error('Invalid JSON data');

      // Replace the entire profile JSON via the Settings Service
      this.settingsService.setAll(data);

      // Also keep legacy flat preferences in sync
      this.preferences = { ...this.preferences, ...data };

      // Apply updated preferences to form fields
      this.applyPreferencesToForm();

      // Attempt API save (may fail if endpoint not ready — data is still in localStorage)
      try {
        await this.savePreferencesToApi(this.settingsService.getAll());
      } catch (apiError) {
        log(Subsystems.MANAGER, Status.WARN,
          '[ProfileManager] API save failed (expected if endpoint not ready):', apiError.message);
      }

      // Reset dirty state — setInitialData updates the baseline without changing content
      this.collectionHandler.setInitialData(data);

      // Return editor to read-only after save
      this.collectionHandler.setEditable(false);

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
    // Flush any pending debounced writes before reverting
    this.settingsService?.flush();
    await this.collectionHandler?.setData(this.getCollectionData());
    this.updateFooterButtons(false);
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

  // ── Font Popup ───────────────────────────────────────────────────────────

  /**
   * Initialize and show the font popup for the collection editor
   */
  initFontPopup() {
    if (this.fontPopup?.popup) return;

    const { popup, toggle, getState, setState, destroy } = createFontPopup({
      anchor: this.elements.btnFont,
      fontSize: this.editorFontSize,
      fontFamily: this.editorFontFamily,
      fontWeight: 'normal',
      onPreview: (state) => this._updateFontPreview(state),
      onSave: (state) => this._saveFontSettings(state),
      onCancel: () => this._cancelFontChanges(),
      onReset: () => this._resetFontToDefault(),
    });

    this.fontPopup = { popup, destroy };
    this._fontPopupToggle = toggle;
    this._fontPopupGetState = getState;
    this._fontPopupSetState = setState;
  }

  toggleFontPopup(e) {
    e.stopPropagation();
    if (!this.fontPopup?.popup) {
      this.initFontPopup();
    }
    this._fontPopupToggle?.(e);
  }

  _updateFontPreview(state) {
    // Apply font settings to the collection editor for preview
    this.collectionHandler?.setFontSize(state.fontSize);
    this.collectionHandler?.setFontFamily(state.fontFamily);
  }

  _saveFontSettings(state) {
    // Save font settings to profile
    this.editorFontSize = state.fontSize;
    this.editorFontFamily = state.fontFamily;
    // Store in profile settings under "collection.font"
    this.settingsService?.setSetting('collection.font', {
      fontSize: state.fontSize,
      fontFamily: state.fontFamily,
      fontWeight: state.fontWeight,
    });
  }

  _cancelFontChanges() {
    // Revert to saved settings
    const saved = this.settingsService?.getSetting('collection.font', {
      fontSize: 13,
      fontFamily: '"Vanadium Mono", var(--font-mono, monospace)',
      fontWeight: 'normal',
    });
    this.editorFontSize = saved.fontSize;
    this.editorFontFamily = saved.fontFamily;
    this.collectionHandler?.setFontSize(saved.fontSize);
    this.collectionHandler?.setFontFamily(saved.fontFamily);
  }

  _resetFontToDefault() {
    const defaults = {
      fontSize: '13px',
      fontFamily: '"Vanadium Mono", var(--font-mono, monospace)',
      fontWeight: 'normal',
    };
    return defaults;
  }

  /**
   * Initialize default date/time formats in the settings if not already present
   */
  _initializeDefaultFormats() {
    const defaultFormats = {
      dates: {
        'Short Date': 'yyyy-MM-dd',
        'Medium Date': 'yyyy-MMM-dd',
        'Long Date': 'MMMM d, y',
        'Week Number': 'yyyy-\'W\'nn',
      },
      times: {
        'Short Time': 'HH:mm',
        'Medium Time': 'H:mm:ss',
        'Long Time': 'HH:mm:ss.SSS',
      },
      datetimes: {
        'Short DateTime': 'yyyy-MM-dd HH:mm:ss',
        'Medium DateTime': 'yyyy-MMM-dd (EEE) HH:mm:ss',
        'Long DateTime': 'MMMM d, y \'at\' HH:mm:ss',
      },
    };

    // Check if we already have formats initialized under section "-9"
    const currentDates = this.settingsService.getSection('-9', 'dates', {});
    const currentTimes = this.settingsService.getSection('-9', 'times', {});
    const currentDateTimes = this.settingsService.getSection('-9', 'datetimes', {});

    // Only initialize if we don't have any formats yet
    if (Object.keys(currentDates).length === 0) {
      this.settingsService.setSection('-9', 'dates', defaultFormats.dates);
    }
    if (Object.keys(currentTimes).length === 0) {
      this.settingsService.setSection('-9', 'times', defaultFormats.times);
    }
    if (Object.keys(currentDateTimes).length === 0) {
      this.settingsService.setSection('-9', 'datetimes', defaultFormats.datetimes);
    }
  }

  /**
   * Show toast notification using global toast system
   */
  showToast(message, type = 'success') {
    const toastType = type === 'error' ? 'error' : 'success';
    toast[toastType](message, { duration: 3000 });
  }

  /**
   * Override the slot close button to perform global logout instead.
   * Replaces the X icon with a reload icon and changes the click behavior.
   */
  overrideCloseButton() {
    const managerSlot = this.container.closest('.manager-slot');
    if (!managerSlot) return;

    const closeBtn = managerSlot.querySelector('.manager-ui-close-btn');
    if (!closeBtn) return;

    closeBtn.innerHTML = '<fa fa-radiation></fa>';
    closeBtn.dataset.tooltip = 'Global Logout';
    processIcons(closeBtn);

    closeBtn.onclick = async (e) => {
      e.preventDefault();
      e.stopPropagation();
      eventBus.emit('logout:initiate', { logoutType: 'global' });
    };
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

    this.settingsService?.destroy();
    this.settingsService = null;

    this.editHelper?.destroy();
    this.splitter?.destroy();
    this.optionsTable?.destroy();

    // Destroy font popup if it exists
    if (this.fontPopup?.destroy && typeof this.fontPopup.destroy === 'function') {
      this.fontPopup.destroy();
    }

    this.elements = {};
  }
}
