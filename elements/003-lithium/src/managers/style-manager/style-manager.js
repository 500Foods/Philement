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

import { LithiumTable } from '../../core/lithium-table-main.js';
import { LithiumSplitter } from '../../core/lithium-splitter.js';
import { PanelStateManager } from '../../core/panel-state-manager.js';
import { togglePanelCollapse, restorePanelState } from '../../core/panel-collapse.js';
import '../../styles/vendor-tabulator.css';
import '../../core/manager-panels.css';
import './style-manager.css';
import { processIcons } from '../../core/icons.js';
import { setupManagerFooterIcons, createFontPopup } from '../../core/manager-ui.js';
import { authQuery } from '../../shared/conduit.js';
import { toast } from '../../shared/toast.js';
import { log, Subsystems, Status } from '../../core/log.js';
import { getClaims } from '../../core/jwt.js';
import { hasFeature } from '../../core/permissions.js';

// ── Hardcoded Sections Data ───────────────────────────────────────────────

const SECTIONS_DATA = [
  { sectionId: 1, sectionName: 'Base Variables', delta: 0, sortoder: 0 },
  { sectionId: 2, sectionName: 'Semantic Variables', delta: 0, sortoder: 0 },
  { sectionId: 3, sectionName: 'Custom Variables', delta: 0, sortoder: 0 },
  { sectionId: 4, sectionName: 'Login', delta: 0, sortoder: 0 },
  { sectionId: 5, sectionName: 'Progress', delta: 0, sortoder: 0 },           
  { sectionId: 6, sectionName: 'Menu Title', delta: 0, sortoder: 0 },
  { sectionId: 7, sectionName: 'Menu', delta: 0, sortoder: 0 },
  { sectionId: 8, sectionName: 'Menu Footer', delta: 0, sortoder: 0 },
  { sectionId: 9, sectionName: 'Manager Header', delta: 0, sortoder: 0 },
  { sectionId: 10, sectionName: 'Manager Toolbar', delta: 0, sortoder: 0 },
  { sectionId: 11, sectionName: 'Manager Footer', delta: 0, sortoder: 0 },
  { sectionId: 12, sectionName: 'Table', delta: 0, sortoder: 0 },
  { sectionId: 13, sectionName: 'Table Columns 1', delta: 0, sortoder: 0 },
  { sectionId: 14, sectionName: 'Table Columns 2', delta: 0, sortoder: 0 },
  { sectionId: 15, sectionName: 'SQL Editor', delta: 0, sortoder: 0 },
  { sectionId: 16, sectionName: 'CSS Editor', delta: 0, sortoder: 0 },
  { sectionId: 17, sectionName: 'Lua Editor', delta: 0, sortoder: 0 },
  { sectionId: 18, sectionName: 'HTML Editor', delta: 0, sortoder: 0 },
  { sectionId: 19, sectionName: 'Markdown Editor', delta: 0, sortoder: 0 },
  { sectionId: 20, sectionName: 'JSON Editor', delta: 0, sortoder: 0 },
  { sectionId: 21, sectionName: 'Crimson', delta: 0, sortoder: 0 },
  { sectionId: 22, sectionName: 'Logout', delta: 0, sortoder: 0 },
  { sectionId: 23, sectionName: 'Index - Variables', delta: 0 , sortoder: 0},
  { sectionId: 24, sectionName: 'Index - Classes', delta: 0, sortoder: 0 },
  { sectionId: 25, sectionName: 'Fonts', delta: 0, sortoder: 0 },
  { sectionId: 26, sectionName: 'Icons', delta: 0, sortoder: 0 },
  { sectionId: 27, sectionName: 'Cursors', delta: 0, sortoder: 0 },
  { sectionId: 28, sectionName: 'Tours', delta: 0, sortoder: 0 },
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

    // Panel state persistence
    this.leftPanelState = new PanelStateManager('lithium_style_left');
    this.middlePanelState = new PanelStateManager('lithium_style_middle');

    // Splitter state (loaded from persistence)
    this.leftPanelWidth = this.leftPanelState.loadWidth(280);
    this.middlePanelWidth = this.middlePanelState.loadWidth(350);
    this.isLeftPanelCollapsed = this.leftPanelState.loadCollapsed(false);
    this.isMiddlePanelCollapsed = this.middlePanelState.loadCollapsed(false);
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
      onRowSelected: (rowData) => this.handleLookupRowSelected(rowData),
      onRowDeselected: () => this.handleLookupRowDeselected(),
      onDataLoaded: (rows) => {
        log(Subsystems.TABLE, Status.INFO, `[Style] Loaded ${rows.length} lookup elements`);
      },
      onSetTableWidth: (mode) => this.setTableWidth(mode, 'left'),
      onEditModeChange: (isEditing, rowData) => this.handleTableEditModeChange(this.lookupTable, isEditing, rowData),
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

  async initSectionsTable() {
    if (!this.elements.sectionsTableContainer || !this.elements.sectionsNavigator) return;

    this.sectionsTable = new LithiumTable({
      container: this.elements.sectionsTableContainer,
      navigatorContainer: this.elements.sectionsNavigator,
      tablePath: 'style-manager/sections',
      cssPrefix: 'style-sections',
      storageKey: 'style_sections_table',
      readonly: true,
      app: this.app,
      onRowSelected: (rowData) => this.handleSectionRowSelected(rowData),
      onRowDeselected: () => this.handleSectionRowDeselected(),
      onSetTableWidth: (mode) => this.setTableWidth(mode, 'middle'),
    });

    await this.sectionsTable.init();

    // Populate with hardcoded sections data (no API call needed)
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

    // Use loadStaticData for the hardcoded sections data
    this.sectionsTable.loadStaticData(SECTIONS_DATA);

    log(Subsystems.TABLE, Status.INFO, `[Style] Loaded ${SECTIONS_DATA.length} hardcoded sections`);
  }

  handleSectionRowSelected(rowData) {
    if (!rowData) return;

    this.selectedSectionId = rowData.sectionId;
    this.selectedSectionName = rowData.sectionName;

    log(Subsystems.MANAGER, Status.INFO, `[Style] Section selected: ${this.selectedSectionName}`);

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
      circle.title = target.label;
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

    if (isCssState && this.cssEditor) {
      this.cssEditor.dispatch({
        changes: {
          from: 0,
          to: this.cssEditor.state.doc.length,
          insert: this.generateCss()
        }
      });
    }

    log(Subsystems.MANAGER, Status.INFO, `[Style] State changed to: ${state}`);
  }

  async initCssEditor() {
    if (this.cssEditor) return;

    try {
      // Dynamic imports for CodeMirror
      const [
        { EditorView, basicSetup },
        { EditorState },
        { css },
        { oneDark },
      ] = await Promise.all([
        import('@codemirror/view'),
        import('@codemirror/state'),
        import('@codemirror/lang-css'),
        import('@codemirror/theme-one-dark'),
      ]);

      this.EditorView = EditorView;

      const state = EditorState.create({
        doc: '',
        extensions: [
          basicSetup,
          css(),
          oneDark,
          EditorView.updateListener.of((update) => {
            if (update.docChanged) {
              // Handle CSS changes if needed
            }
          }),
          EditorView.theme({
            '&': { height: '100%' },
            '.cm-scroller': { overflow: 'auto' }
          })
        ]
      });

      this.cssEditor = new EditorView({
        state,
        parent: this.elements.cssEditor,
      });

    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, `[Style] Failed to load CodeMirror: ${error.message}`);
    }
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

  setTableWidth(mode, panel) {
    const panelElement = panel === 'left' ? this.elements.leftPanel : this.elements.middlePanel;
    if (!panelElement) return;

    const leftDefault = 280;
    const middleDefault = 350;

    // mode === null means LithiumTable has no saved width mode.
    // Apply the PanelStateManager pixel width as fallback.
    if (mode === null) {
      const width = panel === 'left' ? this.leftPanelWidth : this.middlePanelWidth;
      panelElement.style.width = `${width}px`;
      return;
    }

    const widthVar = `--table-width-${mode}`;
    const computedStyle = getComputedStyle(document.documentElement);
    const width = computedStyle.getPropertyValue(widthVar).trim();

    if (mode === 'auto' || !width) {
      panelElement.style.width = '';
      // Reset to CSS default width
      const defaultWidth = panel === 'left' ? leftDefault : middleDefault;
      if (panel === 'left') {
        this.leftPanelWidth = defaultWidth;
        this.leftPanelState.saveWidth(defaultWidth);
      } else {
        this.middlePanelWidth = defaultWidth;
        this.middlePanelState.saveWidth(defaultWidth);
      }
      return;
    }

    // Named mode: set the panel width but do NOT overwrite this.leftPanelWidth /
    // this.middlePanelWidth (which hold the pixel width for collapse/expand).
    // Only save to PanelStateManager when the user explicitly picks a mode via
    // the Width popup (handled in the LithiumTable UI layer).
    panelElement.style.width = width;

    log(Subsystems.MANAGER, Status.INFO, `[Style] ${panel} panel width set to: ${mode} (${width})`);
  }

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
    this.elements.prettifyBtn?.addEventListener('click', () => this.prettify());
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

    // Restore collapsed state using shared utility
    restorePanelState({
      panel: this.elements.leftPanel,
      splitter: this.leftSplitter,
      collapseBtn: this.elements.collapseLeftBtn,
      isCollapsed: this.isLeftPanelCollapsed,
    });

    restorePanelState({
      panel: this.elements.middlePanel,
      splitter: this.rightSplitter,
      collapseBtn: this.elements.collapseMiddleBtn,
      isCollapsed: this.isMiddlePanelCollapsed,
    });

    // Panel widths are handled by LithiumTable's setupPersistence():
    // - If a width mode was saved, it calls onSetTableWidth(mode)
    // - If no mode was saved, it calls onSetTableWidth(null), and setTableWidth
    //   applies the PanelStateManager pixel width as fallback
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
      onExport: (e) => this.toggleFooterExportPopup(e),
      reportOptions: [
        { value: 'style-lookup-view', label: 'Style Elements View' },
        { value: 'style-lookup-data', label: 'Style Elements Data' },
        { value: 'style-sections-view', label: 'Sections View' },
        { value: 'style-sections-data', label: 'Sections Data' },
      ],
      fillerTitle: 'Styles',
      anchor: placeholder,
      showSaveCancel: true,
      onSave: () => this.handleFooterSave(),
      onCancel: () => this.handleFooterCancel(),
    });

    // Store footer save/cancel element references for state management
    this.footerSaveBtn = footerElements.saveBtn;
    this.footerCancelBtn = footerElements.cancelBtn;
    this.footerDummyBtn = footerElements.dummyBtn;

    // The LithiumTable instance currently in edit mode (if any)
    this.activeEditingTable = null;

    // Wire footer Save/Cancel to the active editing table
    if (this.footerSaveBtn) {
      this.footerSaveBtn.addEventListener('click', () => {
        if (this.activeEditingTable?.handleSave) {
          this.activeEditingTable.handleSave();
        }
      });
    }
    if (this.footerCancelBtn) {
      this.footerCancelBtn.addEventListener('click', () => {
        if (this.activeEditingTable?.handleCancel) {
          this.activeEditingTable.handleCancel();
        }
      });
    }

    // Show the Save/Cancel buttons (disabled) initially
    // They will be enabled when a LithiumTable enters edit mode
    this.updateFooterSaveCancelState(true, false);

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
      default:
        log(Subsystems.MANAGER, Status.WARN, `Unknown export format: ${format}`);
    }
  }

  /**
   * Called when any LithiumTable in this manager changes edit mode.
   * Enables/disables the footer Save/Cancel buttons and binds them to
   * the table that is currently in edit mode.
   *
   * @param {LithiumTable} lithiumTable - The table instance
   * @param {boolean} isEditing - Whether the table is now in edit mode
   * @param {Object|null} rowData - The row data being edited (or null)
   */
  handleTableEditModeChange(lithiumTable, isEditing, rowData) {
    if (isEditing) {
      // If another table was already editing, exit its edit mode first
      if (this.activeEditingTable && this.activeEditingTable !== lithiumTable) {
        this.activeEditingTable.exitEditMode('cancel');
      }
      this.activeEditingTable = lithiumTable;
      this.updateFooterSaveCancelState(true, true);
      log(Subsystems.MANAGER, Status.INFO, `[Style] Footer Save/Cancel enabled for table`);
    } else {
      if (this.activeEditingTable === lithiumTable) {
        this.activeEditingTable = null;
      }
      this.updateFooterSaveCancelState(true, false);
      log(Subsystems.MANAGER, Status.INFO, `[Style] Footer Save/Cancel disabled`);
    }
  }

  /**
   * Show/hide and enable/disable the footer Save/Cancel buttons.
   * @param {boolean} visible - Whether the buttons should be visible
   * @param {boolean} enabled - Whether the buttons should be enabled (requires visible=true)
   */
  updateFooterSaveCancelState(visible, enabled) {
    if (this.footerSaveBtn) {
      this.footerSaveBtn.style.display = visible ? '' : 'none';
      this.footerSaveBtn.disabled = !visible || !enabled;
    }
    if (this.footerCancelBtn) {
      this.footerCancelBtn.style.display = visible ? '' : 'none';
      this.footerCancelBtn.disabled = !visible || !enabled;
    }
    if (this.footerDummyBtn) {
      this.footerDummyBtn.style.display = visible ? '' : 'none';
    }
  }

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
