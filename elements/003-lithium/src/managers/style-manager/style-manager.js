/**
 * Style Manager — Manager ID TBD
 *
 * A dual-table interface for managing styles:
 * - Left panel: Lookup 41 (Style Elements via QueryRef 26)
 * - Middle panel: Sections (Hardcoded list)
 * - Right panel: Section visualization (with interactive targets) or JSON editor
 *
 * Uses the reusable LithiumTable component for both tables.
 *
 * @module managers/style-manager
 */

// Import Styles
import '../../styles/vendor-tabulator.css';
import '../../core/manager-panels.css';
import './style-manager.css';

import { LithiumTable } from '../../core/lithium-table-main.js';
import { LithiumSplitter } from '../../core/lithium-splitter.js';
import { PanelStateManager } from '../../core/panel-state-manager.js';
import { togglePanelCollapse, restorePanelState } from '../../core/panel-collapse.js';

import { processIcons } from '../../core/icons.js';
import { setupManagerFooterIcons, createFontPopup, closeExportPopup, initToolbars } from '../../core/manager-ui.js';
import { ManagerEditHelper } from '../../core/manager-edit-helper.js';
import { authQuery } from '../../shared/conduit.js';
import { toast } from '../../shared/toast.js';
import { log, Subsystems, Status } from '../../core/log.js';
import { getClaims } from '../../core/jwt.js';
import { hasFeature } from '../../core/permissions.js';

// CodeMirror — shared setup + direct imports for undo/redo
import {
  EditorView,
  EditorState,
  undo,
  redo,
} from '../../core/codemirror.js';
import {
  buildEditorExtensions,
  createReadOnlyCompartment,
  setEditorEditable,
  setEditorContentNoHistory,
  foldAllInEditor,
  unfoldAllInEditor,
} from '../../core/codemirror-setup.js';

// ── Hardcoded Sections Data ───────────────────────────────────────────────

const DEFAULT_SECTIONS_DATA = [
  { sectionId: 1, sectionName: 'Base Variables', delta: 0 },
  { sectionId: 2, sectionName: 'Semantic Variables', delta: 0 },
  { sectionId: 3, sectionName: 'Login', delta: 0 },
  { sectionId: 4, sectionName: 'Main Menu', delta: 0 },
  { sectionId: 5, sectionName: 'Sidebar', delta: 0 },
  { sectionId: 6, sectionName: 'Tables', delta: 0 },
  { sectionId: 7, sectionName: 'Forms', delta: 0 },
  { sectionId: 8, sectionName: 'Buttons', delta: 0 },
  { sectionId: 9, sectionName: 'Modals', delta: 0 },
  { sectionId: 10, sectionName: 'Tooltips', delta: 0 },
  { sectionId: 11, sectionName: 'Notifications', delta: 0 },
  { sectionId: 12, sectionName: 'Icons', delta: 0 },
  { sectionId: 13, sectionName: 'Animations', delta: 0 },
  { sectionId: 14, sectionName: 'Themes', delta: 0 },
  { sectionId: 15, sectionName: 'Responsive', delta: 0 },
  { sectionId: 16, sectionName: 'Accessibility', delta: 0 },
  { sectionId: 17, sectionName: 'Print Styles', delta: 0 },
  { sectionId: 18, sectionName: 'Error States', delta: 0 },
  { sectionId: 19, sectionName: 'Loading States', delta: 0 },
  { sectionId: 20, sectionName: 'Focus States', delta: 0 },
  { sectionId: 21, sectionName: 'Hover States', delta: 0 },
  { sectionId: 22, sectionName: 'Active States', delta: 0 },
];

// ── Section Mockups ───────────────────────────────────────────────────────

const SECTION_MOCKUPS = {
  4: { // Login
    name: 'Login',
    html: `
      <div class="login-mockup">
        <div class="login-card" data-target="login-panel" data-label="Login Card Background">
          <div class="login-header" data-target="login-header" data-label="Login Header">
            <div class="login-header-group">
              <div class="login-header-btn login-header-primary" data-target="login-logo-area" data-label="Logo Area">
                <img src="/assets/images/logo-li.svg" alt="" class="login-logo" width="32" height="32">
                <span>Lithium</span>
              </div>
              <div class="login-header-btn login-header-version" data-target="login-version" data-label="Version Box">
                <div class="version-row">1000</div>
                <div class="version-row">2026</div>
                <div class="version-row">0328</div>
                <div class="version-row">1620</div>
              </div>
            </div>
          </div>
          <form class="login-form" data-target="login-form" data-label="Login Form">
            <div class="form-group">
              <div class="input-with-icon" data-target="login-username-wrap" data-label="Username Input">
                <input type="text" class="login-username" placeholder="Username" data-target="login-username" data-label="Username Field" />
                <button type="button" class="input-icon-btn" data-target="login-clear-username" data-label="Clear Username"><fa fa-xmark></fa></button>
              </div>
            </div>
            <div class="form-group">
              <div class="input-with-icon" data-target="login-password-wrap" data-label="Password Input">
                <input type="password" class="login-password" placeholder="Password" data-target="login-password" data-label="Password Field" />
                <button type="button" class="input-icon-btn" data-target="login-toggle-password" data-label="Toggle Password"><fa fa-eye></fa></button>
              </div>
            </div>
            <div class="login-error" data-target="login-error" data-label="Error Message">
              <fa fa-exclamation-circle></fa>
              <span>Invalid username or password</span>
            </div>
            <div class="login-btn-group" data-target="login-btn-group" data-label="Button Group">
              <button type="button" class="login-btn-icon" data-target="login-language-btn" data-label="Language Button"><fa fa-earth-americas></fa></button>
              <button type="button" class="login-btn-icon" data-target="login-theme-btn" data-label="Theme Button"><fa fa-palette></fa></button>
              <button type="button" class="login-btn-primary" data-target="login-submit" data-label="Login Button"><fa fa-sign-in-alt></fa></button>
              <button type="button" class="login-btn-icon" data-target="login-logs-btn" data-label="Logs Button"><fa fa-receipt></fa></button>
              <button type="button" class="login-btn-icon" data-target="login-help-btn" data-label="Help Button"><fa fa-circle-question></fa></button>
            </div>
            <div class="login-alt-btn-group" data-target="login-alt-btn-group" data-label="Alternative Login Buttons">
              <button type="button" class="login-alt-btn" data-target="login-didit-btn" data-label="Didit Button">
                <img height=50 src="/assets/images/login_didit.png" alt="Digit" class="login-alt-icon">
              </button>
              <button type="button" class="login-alt-btn" data-target="login-apple-btn" data-label="Apple Button">
                <img src="/assets/images/login_apple.png" alt="Apple" class="login-alt-icon">
              </button>
              <button type="button" class="login-alt-btn" data-target="login-google-btn" data-label="Google Button">
                <img src="/assets/images/login_google.png" alt="Google" class="login-alt-icon">
              </button>
              <button type="button" class="login-alt-btn" data-target="login-microsoft-btn" data-label="Microsoft Button">
                <img src="/assets/images/login_microsoft.png" alt="Microsoft" class="login-alt-icon">
              </button>
            </div>
          </form>
        </div>
      </div>
    `,
    targets: [
      { id: 'login-panel', selector: '.login-card', label: 'Login Card Background' },
      { id: 'login-header', selector: '.login-header', label: 'Header' },
      { id: 'login-logo-area', selector: '.login-header-primary', label: 'Logo Area' },
      { id: 'login-version', selector: '.login-header-version', label: 'Version Box' },
      { id: 'login-username', selector: '.login-username', label: 'Username Input' },
      { id: 'login-password', selector: '.login-password', label: 'Password Input' },
      { id: 'login-submit', selector: '.login-btn-primary', label: 'Login Button' },
      { id: 'login-alt-btn-group', selector: '.login-alt-btn-group', label: 'Alternative Login Buttons' },
      { id: 'login-digit-btn', selector: '[data-target="login-digit-btn"]', label: 'Digit Button' },
      { id: 'login-apple-btn', selector: '[data-target="login-apple-btn"]', label: 'Apple Button' },
      { id: 'login-google-btn', selector: '[data-target="login-google-btn"]', label: 'Google Button' },
      { id: 'login-microsoft-btn', selector: '[data-target="login-microsoft-btn"]', label: 'Microsoft Button' },
    ]
  },
  7: { // Menu
    name: 'Menu',
    html: `
      <div class="menu-mockup" style="background: var(--bg-secondary); padding: 20px; border-radius: 8px;">
        <div style="padding: 12px 16px; font-weight: 600; color: var(--text-primary);" data-target="menu-title" data-label="Menu Title">Main Menu</div>
        <div style="padding: 8px 16px; color: var(--text-primary); cursor: pointer;" data-target="menu-item-1" data-label="Menu Item 1">Dashboard</div>
        <div style="padding: 8px 16px; color: var(--text-primary); cursor: pointer;" data-target="menu-item-2" data-label="Menu Item 2">Users</div>
        <div style="padding: 8px 16px; color: var(--text-primary); cursor: pointer;" data-target="menu-item-3" data-label="Menu Item 3">Settings</div>
      </div>
    `,
    targets: [
      { id: 'menu-title', selector: '[data-target="menu-title"]', label: 'Menu Title' },
      { id: 'menu-item-1', selector: '[data-target="menu-item-1"]', label: 'Menu Item 1' },
      { id: 'menu-item-2', selector: '[data-target="menu-item-2"]', label: 'Menu Item 2' },
      { id: 'menu-item-3', selector: '[data-target="menu-item-3"]', label: 'Menu Item 3' },
    ]
  },
};

// ── StyleManager Class ────────────────────────────────────────────────────

export default class StyleManager {
  constructor(app, container) {
    this.app = app;
    this.container = container;
    this.elements = {};

    // Manager ID
    this.managerId = 1; // TBD

    // Lookup table (Lookup 41)
    this.lookupTable = null;

    // Sections table (hardcoded)
    this.sectionsTable = null;

    // Currently selected items
    this.selectedLookupId = null;
    this.selectedSectionId = null;
    this.selectedSectionName = null;
    this.currentTarget = null;

    // Current mode: 'section' or 'json'
    this.currentMode = 'section';

    // Undo/Redo stacks
    this.undoStack = [];
    this.redoStack = [];
    this.maxUndoLevels = 50;

    // Font popup state
    this.fontPopup = null;
    this.editorFontSize = 14;
    this.editorFontFamily = 'var(--font-mono)';
    this.editorFontWeight = 'normal';

    // JSON editor instance
    this.jsonEditor = null;

    // Live preview style element
    this.previewStyleEl = null;

    // CSS editor state
    this.cssEditor = null;
    this.isCssEditorInEditMode = false;
    this._originalCssContent = '';
    
    // Edit helper — consolidates edit mode, dirty tracking, and save/cancel buttons
    this.editHelper = new ManagerEditHelper({ name: 'Style' });

    // Panel state persistence
    this.leftPanelState = new PanelStateManager('lithium_style_left');
    this.middlePanelState = new PanelStateManager('lithium_style_middle');

    // Splitter state (loaded from persistence)
    this.leftPanelWidth = this.leftPanelState.loadWidth(280);
    this.middlePanelWidth = this.middlePanelState.loadWidth(350);
    this.isLeftPanelCollapsed = this.leftPanelState.loadCollapsed(false);
    this.isMiddlePanelCollapsed = this.middlePanelState.loadCollapsed(false);

    // Sections data storage key
    this.sectionsStorageKey = 'lithium_style_sections_data';

    // Load sections data
    this.sectionsData = this._loadSectionsData();
  }

  async init() {
    await this.render();
    this.setupEventListeners();
    this.applyPermissions();
    await this.initLookupTable();
    await this.initSectionsTable();
    this.setupSplitters();
    this.setupModeToggle();
    this.setupToolbar();
    initToolbars();
    this.initFontPopup();
    this.initPreviewStyle();
    this.setupFooter();
    this.restorePanelState();
    this.show();
  }

  async render() {
    try {
      const response = await fetch('/src/managers/style-manager/style-manager.html');
      const html = await response.text();
      this.container.innerHTML = html;
    } catch (error) {
      console.error('[StyleManager] Failed to load template:', error);
      this.container.innerHTML = '<div class="error">Failed to load Style Manager</div>';
      return;
    }

    this.elements = {
      container: this.container.querySelector('.style-manager-container'),
      leftPanel: this.container.querySelector('#style-left-panel'),
      middlePanel: this.container.querySelector('#style-middle-panel'),
      rightPanel: this.container.querySelector('#style-right-panel'),
      lookupTableContainer: this.container.querySelector('#style-lookup-table-container'),
      lookupNavigator: this.container.querySelector('#style-lookup-navigator'),
      sectionsTableContainer: this.container.querySelector('#style-sections-table-container'),
      sectionsNavigator: this.container.querySelector('#style-sections-navigator'),
      splitterLeft: this.container.querySelector('#style-splitter-left'),
      splitterRight: this.container.querySelector('#style-splitter-right'),
      collapseLeftBtn: this.container.querySelector('#style-collapse-left-btn'),
      collapseMiddleBtn: this.container.querySelector('#style-collapse-middle-btn'),
      collapseLeftIcon: this.container.querySelector('#style-collapse-left-icon'),
      collapseMiddleIcon: this.container.querySelector('#style-collapse-middle-icon'),
      sectionBtn: this.container.querySelector('#style-section-btn'),
      jsonBtn: this.container.querySelector('#style-json-btn'),
      undoBtn: this.container.querySelector('#style-undo-btn'),
      redoBtn: this.container.querySelector('#style-redo-btn'),
      foldAllBtn: this.container.querySelector('#style-fold-all-btn'),
      unfoldAllBtn: this.container.querySelector('#style-unfold-all-btn'),
      fontBtn: this.container.querySelector('#style-font-btn'),
      prettifyBtn: this.container.querySelector('#style-prettify-btn'),
      sectionView: this.container.querySelector('#style-section-view'),
      jsonView: this.container.querySelector('#style-json-view'),
      mockupContainer: this.container.querySelector('#style-mockup-container'),
      editorPanel: this.container.querySelector('#style-editor-panel'),
      targetName: this.container.querySelector('#style-target-name'),
      targetNameBtn: this.container.querySelector('#style-target-name-btn'),
      propertyEditor: this.container.querySelector('#style-property-editor'),
      cssEditor: this.container.querySelector('#style-css-editor'),
      stateButtons: this.container.querySelectorAll('.style-state-btn'),
      applyBtn: this.container.querySelector('#style-apply-btn'),
      resetBtn: this.container.querySelector('#style-reset-btn'),
      copyCssBtn: this.container.querySelector('#style-copy-css-btn'),
      jsonEditor: this.container.querySelector('#style-json-editor'),
    };

    processIcons(this.container);
  }

  applyPermissions() {
    const claims = getClaims();
    const canEdit = hasFeature(this.managerId, 2, claims?.punchcard);
    const canDelete = hasFeature(this.managerId, 5, claims?.punchcard);
    this.permissions = { canEdit, canDelete };
  }

  // ── Event Listeners ─────────────────────────────────────────────────────

  setupEventListeners() {
    // Collapse/expand left panel button
    this.elements.collapseLeftBtn?.addEventListener('click', () => {
      this.toggleLeftPanel();
    });

    // Collapse/expand middle panel button
    this.elements.collapseMiddleBtn?.addEventListener('click', () => {
      this.toggleMiddlePanel();
    });

    // Editor action buttons
    this.elements.applyBtn?.addEventListener('click', () => this.applyStyleChanges());
    this.elements.resetBtn?.addEventListener('click', () => this.resetStyleChanges());
    this.elements.copyCssBtn?.addEventListener('click', () => this.copyCssToClipboard());

    // State toggle buttons
    this.elements.stateButtons?.forEach(btn => {
      btn.addEventListener('click', () => {
        const state = btn.dataset.state;
        this.handleStateToggle(state);
      });
    });

    // Keyboard shortcuts
    document.addEventListener('keydown', (e) => this.handleKeyboardShortcuts(e));
  }

  // ── Lookup 41 Table ──────────────────────────────────────────────────────

  async initLookupTable() {
    if (!this.elements.lookupTableContainer || !this.elements.lookupNavigator) return;

    this.lookupTable = new LithiumTable({
      container: this.elements.lookupTableContainer,
      navigatorContainer: this.elements.lookupNavigator,
      tablePath: 'style-manager/lookup-41',
      queryRef: 26, // QueryRef 26 - Get Lookup with LOOKUPID=41
      cssPrefix: 'style-lookup',
      storageKey: 'style_lookup_table',
      app: this.app,
      readonly: false,
      panel: this.elements.leftPanel,
      panelStateManager: this.leftPanelState,
      onRowSelected: (rowData) => this.handleLookupRowSelected(rowData),
      onRowDeselected: () => this.handleLookupRowDeselected(),
      onDataLoaded: (rows) => {
        log(Subsystems.TABLE, Status.INFO, `[Style] Loaded ${rows.length} lookup elements`);
      },
      onRefresh: () => this.loadLookupData(),
    });

    // Register with editHelper — auto-wires onEditModeChange + onDirtyChange
    this.editHelper.registerTable(this.lookupTable);

    // Register CSS editor with editHelper (bound to lookup table)
    this.editHelper.registerEditor('css', {
      getContent: () => this.cssEditor?.state?.doc?.toString() || '',
      setContent: (content) => setEditorContentNoHistory(this.cssEditor, content),
      setEditable: (editable) => this.setCssEditorEditable(editable),
      boundTable: this.lookupTable,
    });

    await this.lookupTable.init();

    // Load data with LOOKUPID=41 parameter
    await this.loadLookupData();
  }

  async loadLookupData() {
    if (!this.lookupTable || !this.app?.api) return;

    try {
      this.lookupTable.showLoading();

      const rows = await authQuery(this.app.api, 26, {
        INTEGER: { LOOKUPID: 41 },
      });

      if (!this.lookupTable.table) return;

      // Use loadStaticData to handle data loading consistently
      this.lookupTable.loadStaticData(rows);

      log(Subsystems.TABLE, Status.INFO, `[Style] Loaded ${rows.length} style elements from Lookup 41`);
    } catch (error) {
      log(Subsystems.TABLE, Status.ERROR, `[Style] Failed to load Lookup 41: ${error.message}`);
    } finally {
      this.lookupTable.hideLoading();
    }
  }

  handleLookupRowSelected(rowData) {
    if (!rowData) return;
    this.selectedLookupId = rowData.key_idx || rowData.lookupvalueid || rowData.id;
    log(Subsystems.MANAGER, Status.INFO, `[Style] Lookup element selected: ${this.selectedLookupId}`);
  }

  handleLookupRowDeselected() {
    this.selectedLookupId = null;
  }

  // ── Sections Table (Hardcoded) ───────────────────────────────────────────

  _loadSectionsData() {
    try {
      const stored = localStorage.getItem(this.sectionsStorageKey);
      if (stored) {
        const parsed = JSON.parse(stored);
        if (Array.isArray(parsed) && parsed.length > 0) {
          return parsed;
        }
      }
    } catch (e) {
      // Ignore storage errors
    }
    return [...DEFAULT_SECTIONS_DATA];
  }

  _saveSectionsData() {
    try {
      localStorage.setItem(this.sectionsStorageKey, JSON.stringify(this.sectionsData));
    } catch (e) {
      // Ignore storage errors
    }
  }

  async _executeSectionsSave(row, editHelper) {
    const rowData = row.getData();
    const pkField = this.sectionsTable.primaryKeyField || 'sectionId';
    const pkValue = rowData[pkField];
    const isInsert = pkValue == null || pkValue === '' || pkValue === 0;

    if (isInsert) {
      // Add new section
      const newId = Math.max(...this.sectionsData.map(s => s.sectionId), 0) + 1;
      const newSection = { ...rowData, sectionId: newId };
      this.sectionsData.push(newSection);
      // Update the row with the new ID
      row.update({ [pkField]: newId });
    } else {
      // Update existing section
      const index = this.sectionsData.findIndex(s => s[pkField] == pkValue);
      if (index >= 0) {
        this.sectionsData[index] = { ...this.sectionsData[index], ...rowData };
      }
    }

    this._saveSectionsData();
  }

  async initSectionsTable() {
    if (!this.elements.sectionsTableContainer || !this.elements.sectionsNavigator) return;

    this.sectionsTable = new LithiumTable({
      container: this.elements.sectionsTableContainer,
      navigatorContainer: this.elements.sectionsNavigator,
      tablePath: 'style-manager/sections',
      cssPrefix: 'style-sections',
      storageKey: 'style_sections_table',
      readonly: false,
      app: this.app,
      panel: this.elements.middlePanel,
      panelStateManager: this.middlePanelState,
      onRowSelected: (rowData) => this.handleSectionRowSelected(rowData),
      onRowDeselected: () => this.handleSectionRowDeselected(),
      onExecuteSave: (row, editHelper) => this._executeSectionsSave(row, editHelper),
    });

    // Register with editHelper — auto-wires onEditModeChange + onDirtyChange
    this.editHelper.registerTable(this.sectionsTable);

    await this.sectionsTable.init();

    // Populate with sections data
    this.loadSectionsData();
  }

  loadSectionsData() {
    if (!this.sectionsTable?.table) return;

    // Set only the two required columns
    this.sectionsTable.table.setColumns([
      {
        title: 'Section',
        field: 'sectionName',
        resizable: true,
        headerSort: true,
        headerSortTristate: true,
        widthGrow: 1,
      },
      {
        title: 'Delta',
        field: 'delta',
        width: 60,
        hozAlign: 'right',
        resizable: false,
        headerSort: true,
        headerSortTristate: true,
      },
    ]);

    // Use loadStaticData for the sections data
    this.sectionsTable.loadStaticData(this.sectionsData);

    log(Subsystems.TABLE, Status.INFO, `[Style] Loaded ${this.sectionsData.length} sections`);
  }

  handleSectionRowSelected(rowData) {
    if (!rowData) return;

    this.selectedSectionId = rowData.sectionId;
    this.selectedSectionName = rowData.sectionName;

    log(Subsystems.MANAGER, Status.INFO, `[Style] Section selected: ${this.selectedSectionName} (ID: ${this.selectedSectionId})`);

    // Switch to section mode if not already
    if (this.currentMode !== 'section') {
      this.setMode('section');
    }

    // Load section mockup
    this.loadSectionMockup(this.selectedSectionId);
  }

  handleSectionRowDeselected() {
    this.selectedSectionId = null;
    this.selectedSectionName = null;
    this.currentTarget = null;

    // Clear preview styles
    this.clearPreviewStyles();

    if (this.elements.mockupContainer) {
      this.elements.mockupContainer.innerHTML = '<p class="style-placeholder">Select a section to view its mockup</p>';
    }

    if (this.elements.editorPanel) {
      this.elements.editorPanel.style.display = 'none';
    }
  }

  loadSectionMockup(sectionId) {
    if (!this.elements.mockupContainer) return;

    const mockup = SECTION_MOCKUPS[sectionId];

    if (!mockup) {
      this.elements.mockupContainer.innerHTML = `
        <div style="padding: 20px; text-align: center; color: var(--text-muted);">
          <p>Mockup for "${this.selectedSectionName}" not yet implemented.</p>
          <p style="font-size: 12px; margin-top: 8px;">Select a section with a mockup (Login, Menu) to see the interactive editor.</p>
        </div>
      `;
      return;
    }

    this.elements.mockupContainer.innerHTML = `
      <div class="style-mockup-wrapper">
        ${mockup.html}
      </div>
    `;

    this.setupInteractiveTargets(mockup.targets);
  }

  setupInteractiveTargets(targets) {
    if (!targets || targets.length === 0) return;

    const wrapper = this.elements.mockupContainer.querySelector('.style-mockup-wrapper');
    if (!wrapper) return;

    targets.forEach((target, index) => {
      const element = wrapper.querySelector(target.selector);
      if (!element) return;

      const circle = document.createElement('div');
      circle.className = 'style-target-circle';
      circle.dataset.targetId = target.id;
      circle.dataset.tooltip = target.label;
      circle.textContent = index + 1;

      const rect = element.getBoundingClientRect();
      const wrapperRect = wrapper.getBoundingClientRect();

      circle.style.left = `${rect.left - wrapperRect.left - 30}px`;
      circle.style.top = `${rect.top - wrapperRect.top + rect.height / 2 - 12}px`;

      circle.addEventListener('click', (e) => {
        e.stopPropagation();
        this.handleTargetClick(target, element, circle);
      });

      wrapper.appendChild(circle);
    });
  }

  handleTargetClick(target, element, circle) {
    const allCircles = this.elements.mockupContainer.querySelectorAll('.style-target-circle');
    allCircles.forEach(c => c.classList.remove('active'));

    circle.classList.add('active');

    this.currentTarget = { target, element, circle };

    // Display the element class name in the toolbar button
    if (this.elements.targetName) {
      this.elements.targetName.textContent = `.${target.id}`;
    }

    if (this.elements.editorPanel) {
      this.elements.editorPanel.style.display = 'block';
    }

    // Reset state to default when selecting a new target
    this.handleStateToggle('default');

    this.loadStyleProperties(target.id);
  }

  loadStyleProperties(targetId) {
    if (!this.elements.propertyEditor) return;

    // Clear any existing preview styles
    this.clearPreviewStyles();

    // Placeholder property editor - future implementation will load actual CSS properties
    this.elements.propertyEditor.innerHTML = `
      <div class="style-property-group">
        <div class="style-property-group-title">Background</div>
        <div class="style-property-row">
          <span class="style-property-name">background-color</span>
          <div class="style-property-color">
            <input type="color" class="style-preview-input" data-property="background-color" value="#3b82f6" />
            <input type="text" class="style-property-input style-preview-input" data-property="background-color" value="#3b82f6" />
          </div>
        </div>
      </div>
      <div class="style-property-group">
        <div class="style-property-group-title">Border</div>
        <div class="style-property-row">
          <span class="style-property-name">border-color</span>
          <div class="style-property-color">
            <input type="color" class="style-preview-input" data-property="border-color" value="#2563eb" />
            <input type="text" class="style-property-input style-preview-input" data-property="border-color" value="#2563eb" />
          </div>
        </div>
        <div class="style-property-row">
          <span class="style-property-name">border-radius</span>
          <input type="text" class="style-property-input style-preview-input" data-property="border-radius" value="4px" style="flex:1;" />
        </div>
      </div>
      <div class="style-property-group">
        <div class="style-property-group-title">Typography</div>
        <div class="style-property-row">
          <span class="style-property-name">font-size</span>
          <input type="text" class="style-property-input style-preview-input" data-property="font-size" value="14px" style="flex:1;" />
        </div>
        <div class="style-property-row">
          <span class="style-property-name">font-weight</span>
          <input type="text" class="style-property-input style-preview-input" data-property="font-weight" value="500" style="flex:1;" />
        </div>
        <div class="style-property-row">
          <span class="style-property-name">color</span>
          <div class="style-property-color">
            <input type="color" class="style-preview-input" data-property="color" value="#ffffff" />
            <input type="text" class="style-property-input style-preview-input" data-property="color" value="#ffffff" />
          </div>
        </div>
      </div>
      <div class="style-property-group">
        <div class="style-property-group-title">Spacing</div>
        <div class="style-property-row">
          <span class="style-property-name">padding</span>
          <input type="text" class="style-property-input style-preview-input" data-property="padding" value="8px 16px" style="flex:1;" />
        </div>
        <div class="style-property-row">
          <span class="style-property-name">margin</span>
          <input type="text" class="style-property-input style-preview-input" data-property="margin" value="0" style="flex:1;" />
        </div>
      </div>
    `;

    // Add event listeners for live preview
    this.elements.propertyEditor.querySelectorAll('.style-preview-input').forEach(input => {
      input.addEventListener('input', (e) => {
        this.applyPreviewStyle(e.target.dataset.property, e.target.value);
      });
    });

    log(Subsystems.MANAGER, Status.INFO, `[Style] Loaded properties for target: ${targetId}`);
  }

  initPreviewStyle() {
    // Create a style element for live preview
    this.previewStyleEl = document.createElement('style');
    this.previewStyleEl.id = 'style-manager-preview';
    document.head.appendChild(this.previewStyleEl);
  }

  applyPreviewStyle(property, value) {
    if (!this.currentTarget || !this.previewStyleEl) return;

    const selector = `[data-target="${this.currentTarget.target.id}"]`;
    const state = this.currentState === 'default' ? '' : this.currentState;
    
    // Build the CSS rule
    const cssRule = `${selector}${state} { ${property}: ${value}; }`;
    
    // Add to the preview style element
    const existingRules = this.previewStyleEl.textContent;
    const rulePattern = new RegExp(`${selector.replace(/[.*+?^${}()|[\]\\]/g, '\\$&')}\\s*\\{[^}]*${property}[^}]*\\}`, 'g');
    
    // Remove existing rule for this property
    const updatedRules = existingRules.replace(rulePattern, '');
    
    // Add the new rule
    this.previewStyleEl.textContent = updatedRules + '\n' + cssRule;
    
    log(Subsystems.MANAGER, Status.DEBUG, `[Style] Preview: ${cssRule}`);
  }

  clearPreviewStyles() {
    if (this.previewStyleEl) {
      this.previewStyleEl.textContent = '';
    }
  }

  handleStateToggle(state) {
    this.currentState = state;

    this.elements.stateButtons.forEach(btn => {
      btn.classList.toggle('active', btn.dataset.state === state);
    });

    const isCssState = state === 'css';
    this.elements.propertyEditor.style.display = isCssState ? 'none' : 'block';
    this.elements.cssEditor.style.display = isCssState ? 'block' : 'none';

    // Enable/disable prettify button
    if (this.elements.prettifyBtn) {
      this.elements.prettifyBtn.disabled = !isCssState;
    }

    if (isCssState && !this.cssEditor) {
      this.initCssEditor();
    }

    // Only populate CSS editor if it's empty (first time showing CSS view)
    // Don't repopulate if user has already seen/edited the content
    if (isCssState && this.cssEditor && this.cssEditor.state.doc.length === 0) {
      // Programmatic content population — exclude from undo history
      setEditorContentNoHistory(this.cssEditor, this.generateCss());
    }

    log(Subsystems.MANAGER, Status.INFO, `[Style] State changed to: ${state}`);
  }

  async initCssEditor() {
    if (this.cssEditor) return;

    try {
      this.cmReadOnlyCompartment = createReadOnlyCompartment();

      const extensions = buildEditorExtensions({
        language: 'css',
        readOnlyCompartment: this.cmReadOnlyCompartment,
        readOnly: true,
        fontSize: 14,
        onUpdate: (update) => {
          if (update.docChanged && this.isCssEditorInEditMode) {
            this.handleCssEditorDirty();
          }
        },
      });

      const state = EditorState.create({ doc: '', extensions });

      this.cssEditor = new EditorView({
        state,
        parent: this.elements.cssEditor,
      });

      // Set initial visual state (readonly)
      setEditorEditable(this.cssEditor, this.cmReadOnlyCompartment, false, this.elements.cssEditor);

      // Double-click to enter edit mode
      this.elements.cssEditor?.addEventListener('dblclick', () => {
        if (!this.lookupTable?.isEditing && this.lookupTable?.table) {
          const selected = this.lookupTable.table.getSelectedRows();
          if (selected.length > 0) {
            this.lookupTable.enterEditMode(selected[0]);
          }
        }
      });

    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, `[Style] Failed to load CodeMirror: ${error.message}`);
    }
  }

  /**
   * Track CSS editor dirty state
   */
  handleCssEditorDirty() {
    if (!this.cssEditor || !this.isCssEditorInEditMode) return;
    
    // Delegate to editHelper — compares against snapshot
    this.editHelper.checkDirtyState();
  }

  // Snapshot-based dirty tracking is now handled by editHelper.
  // See ManagerEditHelper._takeSnapshot(), isAnythingDirty(), checkDirtyState().

  /**
   * Set CSS editor readonly/editable state.
   * Uses the shared setEditorEditable utility for consistent behavior
   * (compartment reconfigure + contentEditable + gutter color change).
   * @param {boolean} editable - Whether the editor should be editable
   */
  setCssEditorEditable(editable) {
    if (!this.cssEditor || !this.cmReadOnlyCompartment) return;

    this.isCssEditorInEditMode = editable;

    setEditorEditable(this.cssEditor, this.cmReadOnlyCompartment, editable, this.elements.cssEditor);

    log(Subsystems.MANAGER, Status.INFO, `[Style] CSS editor set to ${editable ? 'editable' : 'readonly'}`);
  }

  generateCss() {
    const targetId = this.currentTarget?.target?.id || 'unknown';
    const selector = `[data-target="${targetId}"]`;

    return `/* Generated CSS for ${targetId} */
${selector} {
  /* Default styles here */
}

${selector}:hover {
  /* Hover styles here */
}

${selector}:focus {
  /* Focus styles here */
}

${selector}:disabled {
  /* Disabled styles here */
}
`;
  }

  // ── Table Width Control ────────────────────────────────────────────────────
  // Width persistence is now handled centrally by LithiumTable.
  // The Width popup in the Navigator calls LithiumTable.setTableWidth() directly,
  // which saves the mode to localStorage and applies the width to the panel.
  // Splitter drag clears the width mode via LithiumSplitter._clearWidthModes().
  // Panel pixel width is saved by the onResizeEnd callbacks in setupSplitters().

  // ── Mode Toggle ──────────────────────────────────────────────────────────

  setupModeToggle() {
    this.elements.sectionBtn?.addEventListener('click', () => this.setMode('section'));
    this.elements.jsonBtn?.addEventListener('click', () => this.setMode('json'));
  }

  setMode(mode) {
    this.currentMode = mode;

    this.elements.sectionBtn?.classList.toggle('active', mode === 'section');
    this.elements.jsonBtn?.classList.toggle('active', mode === 'json');

    this.elements.sectionView?.classList.toggle('active', mode === 'section');
    this.elements.jsonView?.classList.toggle('active', mode === 'json');

    log(Subsystems.MANAGER, Status.INFO, `[Style] Mode changed to: ${mode}`);
  }

  // ── Toolbar ──────────────────────────────────────────────────────────────

  setupToolbar() {
    this.elements.undoBtn?.addEventListener('click', () => this.undo());
    this.elements.redoBtn?.addEventListener('click', () => this.redo());
    this.elements.foldAllBtn?.addEventListener('click', () => {
      const editor = this._getActiveEditor();
      if (editor) foldAllInEditor(editor);
    });
    this.elements.unfoldAllBtn?.addEventListener('click', () => {
      const editor = this._getActiveEditor();
      if (editor) unfoldAllInEditor(editor);
    });
    this.elements.prettifyBtn?.addEventListener('click', () => this.prettify());
  }

  /**
   * Get the currently active CodeMirror editor (CSS or JSON).
   * @returns {EditorView|null}
   */
  _getActiveEditor() {
    // Check if JSON view is visible
    if (this.elements.jsonView?.classList.contains('active')) {
      return this.jsonEditor;
    }
    // Otherwise use CSS editor (when in CSS state)
    if (this.elements.cssEditor?.style.display !== 'none') {
      return this.cssEditor;
    }
    return null;
  }

  undo() {
    if (this.undoStack.length === 0) return;

    const state = this.undoStack.pop();
    this.redoStack.push(state);
    this.applyState(state);

    this.updateUndoRedoButtons();
    log(Subsystems.MANAGER, Status.INFO, '[Style] Undo');
  }

  redo() {
    if (this.redoStack.length === 0) return;

    const state = this.redoStack.pop();
    this.undoStack.push(state);
    this.applyState(state);

    this.updateUndoRedoButtons();
    log(Subsystems.MANAGER, Status.INFO, '[Style] Redo');
  }

  pushUndoState(state) {
    this.undoStack.push(state);
    if (this.undoStack.length > this.maxUndoLevels) {
      this.undoStack.shift();
    }
    this.redoStack = [];
    this.updateUndoRedoButtons();
  }

  applyState(_state) {
    // TODO: Apply state to current target
  }

  updateUndoRedoButtons() {
    if (this.elements.undoBtn) {
      this.elements.undoBtn.disabled = this.undoStack.length === 0;
    }
    if (this.elements.redoBtn) {
      this.elements.redoBtn.disabled = this.redoStack.length === 0;
    }
  }

  prettify() {
    if (this.currentState === 'css' && this.cssEditor) {
      // Use CodeMirror's format command
      import('@codemirror/commands').then(({ formatCode }) => {
        if (formatCode) {
          formatCode({ state: this.cssEditor.state, dispatch: this.cssEditor.dispatch });
        }
      });
      log(Subsystems.MANAGER, Status.INFO, '[Style] Prettify CSS');
    } else {
      // TODO: Implement prettify for JSON
      log(Subsystems.MANAGER, Status.INFO, '[Style] Prettify (not available in current mode)');
    }
  }

  // ── Editor Actions ───────────────────────────────────────────────────────

  applyStyleChanges() {
    // TODO: Apply style changes to configuration
    log(Subsystems.MANAGER, Status.INFO, '[Style] Apply style changes');
    toast.success('Style changes applied');
  }

  resetStyleChanges() {
    // TODO: Reset to original values
    log(Subsystems.MANAGER, Status.INFO, '[Style] Reset style changes');
    if (this.currentTarget) {
      this.loadStyleProperties(this.currentTarget.target.id);
    }
  }

  copyCssToClipboard() {
    // TODO: Generate CSS from current properties and copy to clipboard
    const css = `/* Generated CSS */\n/* Target: ${this.currentTarget?.target?.label || 'Unknown'} */\n`;
    navigator.clipboard.writeText(css).then(() => {
      toast.success('CSS copied to clipboard');
    }).catch(() => {
      toast.error('Failed to copy CSS');
    });
  }

  // ── Keyboard Shortcuts ───────────────────────────────────────────────────

  handleKeyboardShortcuts(e) {
    const isMac = navigator.platform.toUpperCase().indexOf('MAC') >= 0;
    const modifier = isMac ? e.metaKey : e.ctrlKey;

    if (!modifier) return;

    switch (e.key.toLowerCase()) {
      case 'z':
        if (e.shiftKey) {
          e.preventDefault();
          this.redo();
        } else {
          e.preventDefault();
          this.undo();
        }
        break;
      case 'p':
        e.preventDefault();
        this.prettify();
        break;
      case '1':
        e.preventDefault();
        this.setMode('section');
        break;
      case '2':
        e.preventDefault();
        this.setMode('json');
        break;
    }
  }

  // ── Font Popup ───────────────────────────────────────────────────────────

  initFontPopup() {
    const { popup, getState } = createFontPopup({
      anchor: this.elements.fontBtn,
      fontSize: parseInt(this.editorFontSize, 10) || 14,
      fontFamily: this.editorFontFamily,
      fontWeight: this.editorFontWeight,
      onChange: () => this.applyFontSettings(),
    });
    this.fontPopup = popup;
    this._fontPopupGetState = getState;
  }

  applyFontSettings() {
    // Apply font settings to JSON editor when implemented
    if (this.jsonEditor) {
      // CodeMirror theme customization would go here
    }
  }

  // ── Splitter Logic ───────────────────────────────────────────────────────

  setupSplitters() {
    // Left splitter (between left and middle panels)
    this.leftSplitter = new LithiumSplitter({
      element: this.elements.splitterLeft,
      leftPanel: this.elements.leftPanel,
      minWidth: 157,
      maxWidth: 1000,
      tables: this.lookupTable,
      onResize: (width) => {
        this.leftPanelWidth = width;
      },
      onResizeEnd: (width) => {
        this.leftPanelState.saveWidth(width);
        this.lookupTable?.table?.redraw?.();
        this.sectionsTable?.table?.redraw?.();
      },
    });

    // Right splitter (between middle and right panels)
    this.rightSplitter = new LithiumSplitter({
      element: this.elements.splitterRight,
      leftPanel: this.elements.middlePanel,
      minWidth: 157,
      maxWidth: 1000,
      tables: this.sectionsTable,
      onResize: (width) => {
        this.middlePanelWidth = width;
      },
      onResizeEnd: (width) => {
        this.middlePanelState.saveWidth(width);
        this.lookupTable?.table?.redraw?.();
        this.sectionsTable?.table?.redraw?.();
      },
    });

    // Bind splitters to tables for centralized width mode clearing
    this.lookupTable?.setSplitter(this.leftSplitter);
    this.sectionsTable?.setSplitter(this.rightSplitter);
  }

  toggleLeftPanel() {
    this.isLeftPanelCollapsed = togglePanelCollapse({
      panel: this.elements.leftPanel,
      splitter: this.leftSplitter,
      collapseBtn: this.elements.collapseLeftBtn,
      panelWidth: this.leftPanelWidth,
      isCollapsed: this.isLeftPanelCollapsed,
      onAfterToggle: () => {
        this.lookupTable?.table?.redraw?.();
        this.sectionsTable?.table?.redraw?.();
      },
    });

    // Save collapsed state
    this.leftPanelState.saveCollapsed(this.isLeftPanelCollapsed);
  }

  toggleMiddlePanel() {
    this.isMiddlePanelCollapsed = togglePanelCollapse({
      panel: this.elements.middlePanel,
      splitter: this.rightSplitter,
      collapseBtn: this.elements.collapseMiddleBtn,
      panelWidth: this.middlePanelWidth,
      isCollapsed: this.isMiddlePanelCollapsed,
      onAfterToggle: () => {
        this.lookupTable?.table?.redraw?.();
        this.sectionsTable?.table?.redraw?.();
      },
    });

    // Save collapsed state
    this.middlePanelState.saveCollapsed(this.isMiddlePanelCollapsed);
  }

  restorePanelState() {
    // Re-read collapsed state from localStorage (handles edge cases)
    this.isLeftPanelCollapsed = this.leftPanelState.loadCollapsed(this.isLeftPanelCollapsed);
    this.isMiddlePanelCollapsed = this.middlePanelState.loadCollapsed(this.isMiddlePanelCollapsed);

    // Restore collapsed state directly via inline styles
    const leftPanel = this.elements.leftPanel;
    if (leftPanel && this.elements.collapseLeftBtn && this.leftSplitter) {
      if (this.isLeftPanelCollapsed) {
        leftPanel.style.width = '0px';
        leftPanel.style.minWidth = '0px';
        leftPanel.style.maxWidth = '0px';
        leftPanel.style.overflow = 'hidden';
        this.elements.collapseLeftBtn.classList.add('collapsed');
        this.leftSplitter.setCollapsed(true);
      } else {
        this.elements.collapseLeftBtn.classList.remove('collapsed');
        this.leftSplitter.setCollapsed(false);
      }
    }

    const middlePanel = this.elements.middlePanel;
    if (middlePanel && this.elements.collapseMiddleBtn && this.rightSplitter) {
      if (this.isMiddlePanelCollapsed) {
        middlePanel.style.width = '0px';
        middlePanel.style.minWidth = '0px';
        middlePanel.style.maxWidth = '0px';
        middlePanel.style.overflow = 'hidden';
        this.elements.collapseMiddleBtn.classList.add('collapsed');
        this.rightSplitter.setCollapsed(true);
      } else {
        this.elements.collapseMiddleBtn.classList.remove('collapsed');
        this.rightSplitter.setCollapsed(false);
      }
    }
  }

  // ── Footer Setup ───────────────────────────────────────────────────────────

  setupFooter() {
    const slot = this.container.closest('.manager-slot');
    if (!slot) return;

    const footer = slot.querySelector('.manager-slot-footer');
    if (!footer) return;

    const group = footer.querySelector('.subpanel-header-group');
    if (!group) return;

    const placeholder = group.querySelector('.slot-footer-placeholder');

    const footerElements = setupManagerFooterIcons(group, {
      onPrint: () => this.handleFooterPrint(),
      onEmail: () => this.handleFooterEmail(),
      onDownload: () => this.handleFooterDownload(),
      onExport: (value, label) => this.handleFooterExport(value, label),
      onClipboard: (value, label) => this.handleFooterClipboard(value, label),
      reportOptions: [
        { value: 'style-lookup-view', label: 'Style Elements View' },
        { value: 'style-lookup-data', label: 'Style Elements Data' },
        { value: 'style-sections-view', label: 'Sections View' },
        { value: 'style-sections-data', label: 'Sections Data' },
      ],
      fillerTitle: 'Styles',
      anchor: placeholder,
      showSaveCancel: true,
      showClipboard: true,
      exportOptions: [
        { value: 'pdf', label: 'PDF', icon: 'fa-file-pdf', enabled: true },
        { value: 'csv', label: 'CSV', icon: 'fa-file-csv', enabled: true },
        { value: 'xls', label: 'XLS', icon: 'fa-file-excel', enabled: true },
        { value: 'json', label: 'JSON', icon: 'fa-brackets-curly', enabled: true },
        { value: 'html', label: 'HTML', icon: 'fa-file-html', enabled: true },
        { value: 'markdown', label: 'Markdown', icon: 'fa-file-code', enabled: true },
        { value: 'txt', label: 'Text', icon: 'fa-file-lines', enabled: true },
      ],
    });

    this._footerDatasource = footerElements.reportSelect;
    this._footerElements = footerElements;

    // Wire save/cancel buttons to the editHelper (handles all state management)
    this.editHelper.wireFooterButtons(
      footerElements.saveBtn,
      footerElements.cancelBtn,
      footerElements.dummyBtn,
    );

    log(Subsystems.MANAGER, Status.INFO, '[StyleManager] Footer controls initialized');
  }

  _getFooterDatasource() {
    return this._footerDatasource?.value || 'style-lookup-view';
  }

  handleFooterPrint() {
    const mode = this._getFooterDatasource();
    const isLookupMode = mode.startsWith('style-lookup');
    const table = isLookupMode ? this.lookupTable?.table : this.sectionsTable?.table;

    if (!table) {
      toast.info('No Data', { description: 'No table available to print', duration: 3000 });
      return;
    }

    if (mode.endsWith('-view')) {
      table.print();
    } else {
      table.print('all', true);
    }
  }

  handleFooterEmail() {
    const mode = this._getFooterDatasource();
    const isLookupMode = mode.startsWith('style-lookup');
    const table = isLookupMode ? this.lookupTable?.table : this.sectionsTable?.table;
    const tableName = isLookupMode ? 'Style Elements' : 'Sections';

    if (!table) {
      toast.info('No Data', { description: 'No table available to email', duration: 3000 });
      return;
    }

    const isViewMode = mode.endsWith('-view');
    const rows = isViewMode ? table.getRows('active') : table.getRows();

    if (rows.length === 0) {
      toast.info('No Data', { description: 'No rows to include in the email', duration: 3000 });
      return;
    }

    const visibleCols = isViewMode
      ? table.getColumns().filter(col => col.isVisible() && col.getField() !== '_selector')
      : table.getColumns().filter(col => col.getField() !== '_selector');

    const headers = visibleCols.map(col => col.getDefinition().title || col.getField());
    const separator = headers.map(h => '-'.repeat(h.length)).join('  ');

    const dataLines = rows.slice(0, 50).map(row => {
      const data = row.getData();
      return visibleCols.map(col => {
        const val = data[col.getField()];
        return val != null ? String(val) : '';
      }).join('\t');
    });

    const totalRows = rows.length;
    const truncated = totalRows > 50 ? `\n... (${totalRows - 50} more rows not shown)` : '';
    const modeLabel = isViewMode ? 'Filtered View' : 'Full Data';

    const subject = encodeURIComponent(`${tableName} — ${totalRows} rows (${modeLabel})`);
    const body = encodeURIComponent(
      `${tableName} Export (${modeLabel})\n` +
      `${totalRows} row(s)\n\n` +
      `${headers.join('\t')}\n${separator}\n` +
      `${dataLines.join('\n')}${truncated}\n`
    );

    window.open(`mailto:?subject=${subject}&body=${body}`, '_self');
  }

  toggleFooterExportPopup(e) {
    e.stopPropagation();

    if (this._footerExportPopup) {
      this._closeFooterExportPopup();
      return;
    }

    const btn = e.currentTarget;
    const mode = this._getFooterDatasource();
    const formats = [
      { label: 'PDF', action: () => this.handleFooterExport('pdf', mode) },
      { label: 'CSV', action: () => this.handleFooterExport('csv', mode) },
      { label: 'TXT', action: () => this.handleFooterExport('txt', mode) },
      { label: 'XLS', action: () => this.handleFooterExport('xls', mode) },
    ];

    const popup = document.createElement('div');
    popup.className = 'style-footer-export-popup';
    formats.forEach(item => {
      const row = document.createElement('button');
      row.type = 'button';
      row.className = 'style-footer-export-popup-item';
      row.textContent = item.label;
      row.addEventListener('click', () => {
        this._closeFooterExportPopup();
        item.action();
      });
      popup.appendChild(row);
    });

    const btnRect = btn.getBoundingClientRect();
    document.body.appendChild(popup);

    requestAnimationFrame(() => {
      const popupRect = popup.getBoundingClientRect();
      popup.style.position = 'fixed';
      popup.style.top = `${btnRect.top - popupRect.height - 8}px`;
      popup.style.left = `${btnRect.left}px`;
    });

    setTimeout(() => {
      popup.classList.add('visible');
    }, 10);

    this._footerExportPopup = popup;

    this._footerExportCloseHandler = (evt) => {
      if (!popup.contains(evt.target) && !btn.contains(evt.target)) {
        this._closeFooterExportPopup();
      }
    };
    document.addEventListener('click', this._footerExportCloseHandler);
  }

  _closeFooterExportPopup() {
    if (this._footerExportPopup) {
      this._footerExportPopup.remove();
      this._footerExportPopup = null;
    }
    if (this._footerExportCloseHandler) {
      document.removeEventListener('click', this._footerExportCloseHandler);
      this._footerExportCloseHandler = null;
    }
  }

  handleFooterExport(format, mode) {
    const isLookupMode = mode.startsWith('style-lookup');
    const table = isLookupMode ? this.lookupTable?.table : this.sectionsTable?.table;

    if (!table) {
      toast.info('No Data', { description: 'No table available to export', duration: 3000 });
      return;
    }

    const filename = `style-export-${new Date().toISOString().slice(0, 10)}`;
    const isViewMode = mode.endsWith('-view');
    const downloadOpts = isViewMode ? {} : { rowGroups: false };

    this._doDownload(table, format, filename, downloadOpts);
  }

  handleFooterDownload() {
    const mode = this._getFooterDatasource();
    const isLookupMode = mode.startsWith('style-lookup');
    const table = isLookupMode ? this.lookupTable?.table : this.sectionsTable?.table;

    if (!table) {
      toast.info('No Data', { description: 'No table available to download', duration: 3000 });
      return;
    }

    const filename = `style-export-${new Date().toISOString().slice(0, 10)}`;
    const isViewMode = mode.endsWith('-view');
    const downloadOpts = isViewMode ? {} : { rowGroups: false };

    const exportFormat = this._footerElements?.exportFormat || 'pdf';
    this._doDownload(table, exportFormat, filename, downloadOpts);
  }

  _doDownload(table, format, filename, downloadOpts) {
    switch (format) {
      case 'pdf':
        table.download('pdf', `${filename}.pdf`, { orientation: 'landscape', ...downloadOpts });
        break;
      case 'csv':
        table.download('csv', `${filename}.csv`, downloadOpts);
        break;
      case 'txt':
        table.download('csv', `${filename}.txt`, downloadOpts);
        break;
      case 'xls':
        table.download('xlsx', `${filename}.xlsx`, downloadOpts);
        break;
      case 'json':
        table.download('json', `${filename}.json`, downloadOpts);
        break;
      case 'html':
        table.download('html', `${filename}.html`, downloadOpts);
        break;
      case 'markdown':
        this.handleMarkdownExport(table, this._getFooterDatasource(), filename);
        break;
      default:
        log(Subsystems.MANAGER, Status.WARN, `Unknown export format: ${format}`);
    }
  }

  handleFooterClipboard(value, label) {
    const mode = this._getFooterDatasource();
    const isLookupMode = mode.startsWith('style-lookup');
    const table = isLookupMode ? this.lookupTable?.table : this.sectionsTable?.table;

    if (!table) {
      toast.info('No Data', { description: 'No table available to copy', duration: 3000 });
      return;
    }

    const isViewMode = mode.endsWith('-view');
    const rows = isViewMode ? table.getRows('active') : table.getRows();

    if (rows.length === 0) {
      toast.info('No Data', { description: 'No rows to copy', duration: 3000 });
      return;
    }

    const visibleCols = isViewMode
      ? table.getColumns().filter(col => col.isVisible() && col.getField() !== '_selector')
      : table.getColumns().filter(col => col.getField() !== '_selector');

    const headers = visibleCols.map(col => col.getDefinition().title || col.getField());
    const dataLines = rows.map(row => {
      const data = row.getData();
      return visibleCols.map(col => {
        const val = data[col.getField()];
        return val != null ? String(val) : '';
      }).join('\t');
    });

    const text = `${headers.join('\t')}\n${dataLines.join('\n')}`;

    navigator.clipboard.writeText(text).then(() => {
      toast.success('Copied', { description: 'Table data copied to clipboard', duration: 2000 });
    }).catch(err => {
      toast.error('Copy Failed', { description: 'Failed to copy to clipboard', duration: 3000 });
    });
  }

  handleMarkdownExport(table, mode, filename) {
    const isViewMode = mode.endsWith('-view');
    const rows = isViewMode ? table.getRows('active') : table.getRows();

    if (rows.length === 0) return;

    const visibleCols = isViewMode
      ? table.getColumns().filter(col => col.isVisible() && col.getField() !== '_selector')
      : table.getColumns().filter(col => col.getField() !== '_selector');

    const headers = visibleCols.map(col => col.getDefinition().title || col.getField());
    const headerRow = `| ${headers.join(' | ')} |`;
    const separatorRow = `| ${headers.map(() => '---').join(' | ')} |`;
    const dataRows = rows.map(row => {
      const data = row.getData();
      const rowData = visibleCols.map(col => {
        const val = data[col.getField()];
        return val != null ? String(val).replace(/\|/g, '\\|') : '';
      });
      return `| ${rowData.join(' | ')} |`;
    });

    const markdown = `${headerRow}\n${separatorRow}\n${dataRows.join('\n')}`;

    const blob = new Blob([markdown], { type: 'text/markdown' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `${filename}.md`;
    a.click();
    URL.revokeObjectURL(url);
  }

  // Edit mode, dirty tracking, and save/cancel button management are now
  // handled by this.editHelper (ManagerEditHelper).
  // See editHelper.registerTable(), editHelper.registerEditor(), and
  // editHelper.wireFooterButtons() in init/setup methods above.

  // ── Lifecycle ─────────────────────────────────────────────────────────────

  show() {
    requestAnimationFrame(() => {
      this.elements.container?.classList.add('visible');
    });
  }

  onActivate() {
    log(Subsystems.MANAGER, Status.INFO, '[Style] Activated');

    if (this.lookupTable?.table) {
      this.lookupTable.table.redraw?.();
    }
    if (this.sectionsTable?.table) {
      this.sectionsTable.table.redraw?.();
    }
  }

  onDeactivate() {
    log(Subsystems.MANAGER, Status.INFO, '[Style] Deactivated');
  }

  cleanup() {
    log(Subsystems.MANAGER, Status.INFO, '[Style] Cleaning up...');

    // Clean up edit helper
    this.editHelper?.destroy();

    // Remove preview style element
    if (this.previewStyleEl) {
      this.previewStyleEl.remove();
      this.previewStyleEl = null;
    }

    if (this.fontPopup) {
      this.fontPopup.remove();
      this.fontPopup = null;
    }

    this._closeFooterExportPopup();

    // Clean up splitters
    this.leftSplitter?.destroy();
    this.rightSplitter?.destroy();
    this.leftSplitter = null;
    this.rightSplitter = null;

    // Clean up tables
    this.lookupTable?.destroy();
    this.sectionsTable?.destroy();

    this.lookupTable = null;
    this.sectionsTable = null;

    if (this.jsonEditor) {
      this.jsonEditor.destroy();
      this.jsonEditor = null;
    }
  }
}
