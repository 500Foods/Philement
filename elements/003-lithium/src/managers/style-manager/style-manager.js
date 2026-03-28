/**
 * Style Manager — Manager ID 1
 *
 * A three-panel interface for managing themes and their CSS sections:
 * - Left panel: List of all themes (parent table)
 * - Middle panel: CSS sections for the selected theme (child table)
 * - Right panel: Tabbed view with CSS Editor, Variables, and Preview
 *
 * Uses the reusable LithiumTable component for both tables.
 *
 * @module managers/style-manager
 */

import { LithiumTable } from '../../core/lithium-table-main.js';
import '../../styles/vendor-tabulator.css';
import { authQuery } from '../../shared/conduit.js';
import { toast } from '../../shared/toast.js';
import { log, Subsystems, Status } from '../../core/log.js';
import { processIcons } from '../../core/icons.js';
import { setupManagerFooterIcons } from '../../core/manager-ui.js';
import { getClaims } from '../../core/jwt.js';
import { hasFeature, getFeaturesForManager } from '../../core/permissions.js';
import '../../core/manager-panels.css';
import './style-manager.css';

// Dynamic imports for third-party libraries
let marked;

// Constants
const SELECTED_THEME_KEY = 'lithium_style_selected_theme_id';

// ── StyleManager Class ────────────────────────────────────────────────────

export default class StyleManager {
  constructor(app, container) {
    this.app = app;
    this.container = container;
    this.elements = {};

    // Style Manager ID = 1
    this.managerId = 1;

    // Parent table (themes list)
    this.themesTable = null;

    // Child table (sections list)
    this.sectionsTable = null;

    // Currently selected theme
    this.selectedThemeId = null;
    this.selectedThemeName = null;

    // Editor instances
    this.cssEditor = null;
    this.variablesEditor = null;

    // Splitter state
    this.leftPanelWidth = 280;
    this.middlePanelWidth = 350;
    this.isLeftPanelCollapsed = false;
    this.isMiddlePanelCollapsed = false;
    this.isResizingLeft = false;
    this.isResizingRight = false;

    // Active tab
    this.activeTab = 'css';

    // Font popup state
    this.fontPopup = null;
    this.editorFontSize = 14;
    this.editorFontFamily = 'var(--font-mono)';

    // Current theme data for preview
    this.currentThemeData = null;
  }

  async init() {
    await this.loadDependencies();
    await this.render();
    this.setupEventListeners();
    this.setupSplitters();
    this.applyPermissions();
    await this.initThemesTable();
    await this.initSectionsTable();
    this.setupFooter();
    this.setupTabs();
    this.show();
  }

  /**
   * Load third-party dependencies
   */
  async loadDependencies() {
    try {
      const markedModule = await import('marked');
      marked = markedModule.marked;
    } catch (error) {
      console.error('[StyleManager] Failed to load dependencies:', error);
    }
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
      themesTableContainer: this.container.querySelector('#style-themes-table-container'),
      themesNavigator: this.container.querySelector('#style-themes-navigator'),
      sectionsTableContainer: this.container.querySelector('#style-sections-table-container'),
      sectionsNavigator: this.container.querySelector('#style-sections-navigator'),
      childTitle: this.container.querySelector('#style-child-title'),
      splitterLeft: this.container.querySelector('#style-splitter-left'),
      splitterRight: this.container.querySelector('#style-splitter-right'),
      collapseLeftBtn: this.container.querySelector('#style-collapse-left-btn'),
      collapseMiddleBtn: this.container.querySelector('#style-collapse-middle-btn'),
      collapseLeftIcon: this.container.querySelector('#style-collapse-left-icon'),
      collapseMiddleIcon: this.container.querySelector('#style-collapse-middle-icon'),
      fontBtn: this.container.querySelector('#style-font-btn'),
      tabBtns: this.container.querySelectorAll('.tab-btn'),
      tabPanes: this.container.querySelectorAll('.tab-pane'),
      cssEditor: this.container.querySelector('#style-css-editor'),
      variablesEditor: this.container.querySelector('#style-variables-editor'),
      previewContent: this.container.querySelector('#style-preview-content'),
    };

    processIcons(this.container);
  }

  /**
   * Apply feature-gated UI based on permissions
   */
  applyPermissions() {
    const claims = getClaims();
    const features = getFeaturesForManager(this.managerId, claims?.punchcard);

    // Feature 1: View all styles (read-only)
    // Feature 2: Edit all styles
    const canEdit = hasFeature(this.managerId, 2, claims?.punchcard);
    // Feature 5: Delete themes
    const canDelete = hasFeature(this.managerId, 5, claims?.punchcard);

    this.permissions = { canEdit, canDelete };
  }

  setupEventListeners() {
    // Collapse/expand buttons
    this.elements.collapseLeftBtn?.addEventListener('click', () => {
      this.toggleLeftPanel();
    });

    this.elements.collapseMiddleBtn?.addEventListener('click', () => {
      this.toggleMiddlePanel();
    });

    // Font button
    this.elements.fontBtn?.addEventListener('click', (e) => {
      this.toggleFontPopup(e);
    });

    this.initFontPopup();
  }

  // ── Themes Table (Parent) ──────────────────────────────────────────────────

  async initThemesTable() {
    if (!this.elements.themesTableContainer || !this.elements.themesNavigator) return;

    this.themesTable = new LithiumTable({
      container: this.elements.themesTableContainer,
      navigatorContainer: this.elements.themesNavigator,
      tablePath: 'style-manager/themes-list',
      queryRef: 50, // Theme List
      searchQueryRef: 51, // Theme Search
      cssPrefix: 'style-parent',
      storageKey: 'style_themes_table',
      app: this.app,
      readonly: false,
      onRowSelected: (rowData) => this.handleThemeSelected(rowData),
      onRowDeselected: () => this.handleThemeDeselected(),
      onDataLoaded: (rows) => {
        log(Subsystems.TABLE, Status.INFO, `[Style] Loaded ${rows.length} themes`);
      },
    });

    await this.themesTable.init();
    await this.themesTable.loadData();
  }

  // ── Sections Table (Child) ─────────────────────────────────────────────────

  async initSectionsTable() {
    if (!this.elements.sectionsTableContainer || !this.elements.sectionsNavigator) return;

    this.sectionsTable = new LithiumTable({
      container: this.elements.sectionsTableContainer,
      navigatorContainer: this.elements.sectionsNavigator,
      tablePath: 'style-manager/sections-list',
      queryRef: 52, // Sections List (requires THEMEID param)
      cssPrefix: 'style-child',
      storageKey: 'style_sections_table',
      app: this.app,
      readonly: false,
      onRowSelected: (rowData) => this.handleSectionSelected(rowData),
      onRowDeselected: () => this.handleSectionDeselected(),
      onDataLoaded: (rows) => {
        log(Subsystems.TABLE, Status.INFO, `[Style] Loaded ${rows.length} sections`);
      },
    });

    await this.sectionsTable.init();
  }

  // ── Parent/Child Relationship ──────────────────────────────────────────────

  async handleThemeSelected(rowData) {
    if (!rowData) return;

    const themeId = rowData.theme_id;
    const themeName = rowData.name;

    if (!themeId) return;

    this.selectedThemeId = themeId;
    this.selectedThemeName = themeName;

    // Update child table title
    if (this.elements.childTitle) {
      this.elements.childTitle.textContent = themeName
        ? `${themeName} (${themeId})`
        : `Theme ${themeId}`;
    }

    // Persist selection
    try { localStorage.setItem(SELECTED_THEME_KEY, String(themeId)); } catch (_e) { /* ignore */ }

    // Load child data for this theme
    await this.loadSectionsData(themeId);

    // Load theme details for preview
    await this.loadThemeDetails(themeId);
  }

  handleThemeDeselected() {
    this.selectedThemeId = null;
    this.selectedThemeName = null;

    if (this.elements.childTitle) {
      this.elements.childTitle.textContent = 'Select a Theme';
    }

    // Clear child table data
    if (this.sectionsTable?.table) {
      this.sectionsTable.table.setData([]);
    }

    // Clear detail view
    this.clearDetailView();
  }

  async loadSectionsData(themeId) {
    if (!this.sectionsTable || !this.app?.api) return;

    try {
      this.sectionsTable.showLoading();

      const rows = await authQuery(this.app.api, 52, {
        INTEGER: { THEMEID: themeId },
      });

      if (!this.sectionsTable.table) return;

      this.sectionsTable.table.blockRedraw?.();
      try {
        this.sectionsTable.table.setData(rows);
        this.sectionsTable.discoverColumns(rows);
      } finally {
        this.sectionsTable.table.restoreRedraw?.();
      }

      // Auto-select first row if any
      requestAnimationFrame(() => {
        const activeRows = this.sectionsTable.table.getRows('active');
        if (activeRows.length > 0) {
          activeRows[0].select();
        }
        this.sectionsTable.updateMoveButtonState();
      });

      log(Subsystems.TABLE, Status.INFO, `[Style] Loaded ${rows.length} sections for theme ${themeId}`);
    } catch (error) {
      toast.error('Failed to load theme sections', {
        serverError: error.serverError,
        subsystem: 'Conduit',
        duration: 6000,
      });
    } finally {
      this.sectionsTable.hideLoading();
    }
  }

  async loadThemeDetails(themeId) {
    if (!this.app?.api) return;

    try {
      // QueryRef 53: Get single theme details
      const detail = await authQuery(this.app.api, 53, {
        INTEGER: { THEMEID: themeId },
      });

      if (detail && detail.length > 0) {
        this.currentThemeData = detail[0];
        this.updatePreview();
      }
    } catch (error) {
      log(Subsystems.TABLE, Status.ERROR, `[Style] Failed to load theme details: ${error.message}`);
    }
  }

  // ── Child Table Handlers ───────────────────────────────────────────────────

  handleSectionSelected(_rowData) {
    // Future: Load section CSS into editor when row is selected
    // This would populate the CSS editor with the section's CSS rules
  }

  handleSectionDeselected() {
    // Clear section editor
  }

  // ── Tab System ─────────────────────────────────────────────────────────────

  setupTabs() {
    this.elements.tabBtns?.forEach(btn => {
      btn.addEventListener('click', () => {
        const tabId = btn.dataset.tab;
        this.switchTab(tabId);
      });
    });
  }

  switchTab(tabId) {
    this.activeTab = tabId;

    // Update buttons
    this.elements.tabBtns.forEach(btn => {
      btn.classList.toggle('active', btn.dataset.tab === tabId);
    });

    // Update panes
    this.elements.tabPanes.forEach(pane => {
      pane.classList.toggle('active', pane.id === `style-tab-${tabId}`);
    });

    // Initialize CodeMirror when switching to CSS view
    if (tabId === 'css' && !this.cssEditor) {
      this.initCssEditor();
    }

    // Refresh preview if switching to preview tab
    if (tabId === 'preview') {
      this.updatePreview();
    }
  }

  // ── Editor Initialization ──────────────────────────────────────────────────

  async initCssEditor() {
    if (!this.elements.cssEditor) return;

    try {
      const { EditorState } = await import('@codemirror/state');
      const { EditorView, keymap, lineNumbers, highlightActiveLineGutter, highlightSpecialChars, drawSelection, highlightActiveLine } = await import('@codemirror/view');
      const { defaultKeymap, history, historyKeymap } = await import('@codemirror/commands');
      const { css } = await import('@codemirror/lang-css');
      const { oneDark } = await import('@codemirror/theme-one-dark');

      const startState = EditorState.create({
        doc: '/* Select a theme to edit its CSS */\n',
        extensions: [
          lineNumbers(),
          highlightActiveLineGutter(),
          highlightSpecialChars(),
          history(),
          drawSelection(),
          highlightActiveLine(),
          keymap.of([...defaultKeymap, ...historyKeymap]),
          css(),
          oneDark,
          EditorView.theme({
            '&': { height: '100%' },
            '.cm-scroller': { overflow: 'auto' },
          }),
        ],
      });

      this.cssEditor = new EditorView({
        state: startState,
        parent: this.elements.cssEditor,
      });
    } catch (error) {
      console.error('[StyleManager] Failed to initialize CSS editor:', error);
      this.elements.cssEditor.innerHTML = '<textarea id="style-css-editor-fallback" style="width:100%;height:100%;background:var(--bg-secondary);color:var(--text-primary);border:none;padding:16px;font-family:var(--font-mono);"></textarea>';
    }
  }

  async initVariablesEditor() {
    if (!this.elements.variablesEditor) return;

    try {
      const { EditorState } = await import('@codemirror/state');
      const { EditorView, keymap, lineNumbers, highlightActiveLineGutter, highlightSpecialChars, drawSelection, highlightActiveLine } = await import('@codemirror/view');
      const { defaultKeymap, history, historyKeymap } = await import('@codemirror/commands');
      const { json } = await import('@codemirror/lang-json');
      const { oneDark } = await import('@codemirror/theme-one-dark');

      const startState = EditorState.create({
        doc: '{\n  \n}',
        extensions: [
          lineNumbers(),
          highlightActiveLineGutter(),
          highlightSpecialChars(),
          history(),
          drawSelection(),
          highlightActiveLine(),
          keymap.of([...defaultKeymap, ...historyKeymap]),
          json(),
          oneDark,
          EditorView.theme({
            '&': { height: '100%' },
            '.cm-scroller': { overflow: 'auto' },
          }),
        ],
      });

      this.variablesEditor = new EditorView({
        state: startState,
        parent: this.elements.variablesEditor,
      });
    } catch (error) {
      console.error('[StyleManager] Failed to initialize variables editor:', error);
    }
  }

  // ── Preview ────────────────────────────────────────────────────────────────

  updatePreview() {
    if (!this.elements.previewContent) return;

    if (!this.currentThemeData) {
      this.elements.previewContent.innerHTML = '<p class="style-preview-placeholder">Select a theme to preview</p>';
      return;
    }

    try {
      let htmlContent = '';

      if (this.currentThemeData.summary && marked) {
        htmlContent = marked.parse(this.currentThemeData.summary);
      } else {
        htmlContent = '<p class="style-preview-placeholder">No summary content available</p>';
      }

      this.elements.previewContent.innerHTML = `<div class="style-preview-rendered">${htmlContent}</div>`;
    } catch (error) {
      console.error('[StyleManager] Failed to update preview:', error);
      this.elements.previewContent.innerHTML = '<p class="style-preview-placeholder">Error rendering preview</p>';
    }
  }

  clearDetailView() {
    this.currentThemeData = null;
    if (this.elements.previewContent) {
      this.elements.previewContent.innerHTML = '<p class="style-preview-placeholder">Select a theme to preview</p>';
    }
  }

  // ── Font Popup ─────────────────────────────────────────────────────────────

  initFontPopup() {
    this.fontPopup = document.createElement('div');
    this.fontPopup.className = 'style-font-popup';
    const fontSizeValue = parseInt(this.editorFontSize, 10) || 14;
    this.fontPopup.innerHTML = `
      <div class="style-font-popup-row">
        <label class="style-font-popup-label">Size</label>
        <input type="number" class="style-font-popup-input" id="style-font-size" value="${fontSizeValue}" min="8" max="32">
      </div>
      <div class="style-font-popup-row">
        <label class="style-font-popup-label">Family</label>
        <select class="style-font-popup-select" id="style-font-family">
          <option value="var(--font-sans)" ${this.editorFontFamily === 'var(--font-sans)' ? 'selected' : ''}>Sans Serif</option>
          <option value="var(--font-mono)" ${this.editorFontFamily === 'var(--font-mono)' ? 'selected' : ''}>Monospace</option>
          <option value="serif" ${this.editorFontFamily === 'serif' ? 'selected' : ''}>Serif</option>
        </select>
      </div>
      <div class="style-font-popup-row">
        <label class="style-font-popup-label">Style</label>
        <div class="style-font-popup-styles">
          <button type="button" class="style-font-popup-style-btn" data-style="normal">Normal</button>
          <button type="button" class="style-font-popup-style-btn" data-style="bold">Bold</button>
        </div>
      </div>
    `;

    if (this.elements.fontBtn) {
      this.elements.fontBtn.parentElement.appendChild(this.fontPopup);
    }

    const sizeInput = this.fontPopup.querySelector('#style-font-size');
    const familySelect = this.fontPopup.querySelector('#style-font-family');
    const styleBtns = this.fontPopup.querySelectorAll('.style-font-popup-style-btn');

    sizeInput?.addEventListener('change', (e) => {
      this.editorFontSize = `${e.target.value}px`;
      this.applyFontSettings();
    });

    familySelect?.addEventListener('change', (e) => {
      this.editorFontFamily = e.target.value;
      this.applyFontSettings();
    });

    styleBtns.forEach(btn => {
      btn.addEventListener('click', () => {
        styleBtns.forEach(b => b.classList.remove('active'));
        btn.classList.add('active');
        this.applyFontSettings();
      });
    });

    document.addEventListener('click', (e) => {
      if (this.fontPopup?.classList.contains('visible') &&
          !this.fontPopup.contains(e.target) &&
          !this.elements.fontBtn?.contains(e.target)) {
        this.fontPopup.classList.remove('visible');
      }
    });
  }

  toggleFontPopup(e) {
    e.stopPropagation();
    if (this.fontPopup) {
      this.fontPopup.classList.toggle('visible');
    }
  }

  applyFontSettings() {
    // Apply to editors when implemented
  }

  // ── Splitter Logic ─────────────────────────────────────────────────────────

  setupSplitters() {
    this.setupLeftSplitter();
    this.setupRightSplitter();
  }

  setupLeftSplitter() {
    const splitter = this.elements.splitterLeft;
    const leftPanel = this.elements.leftPanel;

    if (!splitter || !leftPanel) return;

    splitter.addEventListener('mousedown', (e) => {
      if (this.isLeftPanelCollapsed) return;

      this.isResizingLeft = true;
      splitter.classList.add('resizing');
      document.body.style.cursor = 'col-resize';
      document.body.style.userSelect = 'none';

      leftPanel.style.transition = 'none';

      const startX = e.clientX;
      const startWidth = leftPanel.offsetWidth;

      const onMouseMove = (e) => {
        if (!this.isResizingLeft) return;
        const delta = e.clientX - startX;
        const newWidth = Math.max(150, Math.min(450, startWidth + delta));
        leftPanel.style.width = `${newWidth}px`;
        this.leftPanelWidth = newWidth;
      };

      const onMouseUp = () => {
        this.isResizingLeft = false;
        splitter.classList.remove('resizing');
        document.body.style.cursor = '';
        document.body.style.userSelect = '';
        leftPanel.style.transition = '';

        document.removeEventListener('mousemove', onMouseMove);
        document.removeEventListener('mouseup', onMouseUp);

        this.themesTable?.table?.redraw?.();
        this.sectionsTable?.table?.redraw?.();
      };

      document.addEventListener('mousemove', onMouseMove);
      document.addEventListener('mouseup', onMouseUp);
    });

    // Touch support
    splitter.addEventListener('touchstart', (e) => {
      if (this.isLeftPanelCollapsed) return;

      this.isResizingLeft = true;
      splitter.classList.add('resizing');
      leftPanel.style.transition = 'none';

      const touch = e.touches[0];
      const startX = touch.clientX;
      const startWidth = leftPanel.offsetWidth;

      const onTouchMove = (e) => {
        if (!this.isResizingLeft) return;
        const touch = e.touches[0];
        const delta = touch.clientX - startX;
        const newWidth = Math.max(150, Math.min(450, startWidth + delta));
        leftPanel.style.width = `${newWidth}px`;
        this.leftPanelWidth = newWidth;
      };

      const onTouchEnd = () => {
        this.isResizingLeft = false;
        splitter.classList.remove('resizing');
        leftPanel.style.transition = '';

        document.removeEventListener('touchmove', onTouchMove);
        document.removeEventListener('touchend', onTouchEnd);

        this.themesTable?.table?.redraw?.();
        this.sectionsTable?.table?.redraw?.();
      };

      document.addEventListener('touchmove', onTouchMove);
      document.addEventListener('touchend', onTouchEnd);
    });
  }

  setupRightSplitter() {
    const splitter = this.elements.splitterRight;
    const middlePanel = this.elements.middlePanel;

    if (!splitter || !middlePanel) return;

    splitter.addEventListener('mousedown', (e) => {
      if (this.isMiddlePanelCollapsed) return;

      this.isResizingRight = true;
      splitter.classList.add('resizing');
      document.body.style.cursor = 'col-resize';
      document.body.style.userSelect = 'none';

      middlePanel.style.transition = 'none';

      const startX = e.clientX;
      const startWidth = middlePanel.offsetWidth;

      const onMouseMove = (e) => {
        if (!this.isResizingRight) return;
        const delta = e.clientX - startX;
        const newWidth = Math.max(200, Math.min(550, startWidth + delta));
        middlePanel.style.width = `${newWidth}px`;
        this.middlePanelWidth = newWidth;
      };

      const onMouseUp = () => {
        this.isResizingRight = false;
        splitter.classList.remove('resizing');
        document.body.style.cursor = '';
        document.body.style.userSelect = '';
        middlePanel.style.transition = '';

        document.removeEventListener('mousemove', onMouseMove);
        document.removeEventListener('mouseup', onMouseUp);

        this.themesTable?.table?.redraw?.();
        this.sectionsTable?.table?.redraw?.();
      };

      document.addEventListener('mousemove', onMouseMove);
      document.addEventListener('mouseup', onMouseUp);
    });

    // Touch support
    splitter.addEventListener('touchstart', (e) => {
      if (this.isMiddlePanelCollapsed) return;

      this.isResizingRight = true;
      splitter.classList.add('resizing');
      middlePanel.style.transition = 'none';

      const touch = e.touches[0];
      const startX = touch.clientX;
      const startWidth = middlePanel.offsetWidth;

      const onTouchMove = (e) => {
        if (!this.isResizingRight) return;
        const touch = e.touches[0];
        const delta = touch.clientX - startX;
        const newWidth = Math.max(200, Math.min(550, startWidth + delta));
        middlePanel.style.width = `${newWidth}px`;
        this.middlePanelWidth = newWidth;
      };

      const onTouchEnd = () => {
        this.isResizingRight = false;
        splitter.classList.remove('resizing');
        middlePanel.style.transition = '';

        document.removeEventListener('touchmove', onTouchMove);
        document.removeEventListener('touchend', onTouchEnd);

        this.themesTable?.table?.redraw?.();
        this.sectionsTable?.table?.redraw?.();
      };

      document.addEventListener('touchmove', onTouchMove);
      document.addEventListener('touchend', onTouchEnd);
    });
  }

  toggleLeftPanel() {
    const leftPanel = this.elements.leftPanel;
    const splitter = this.elements.splitterLeft;

    if (!leftPanel || !splitter) return;

    this.isLeftPanelCollapsed = !this.isLeftPanelCollapsed;

    if (this.isLeftPanelCollapsed) {
      leftPanel.classList.add('collapsed');
      splitter.classList.add('collapsed');
    } else {
      leftPanel.classList.remove('collapsed');
      leftPanel.style.width = `${this.leftPanelWidth}px`;
      splitter.classList.remove('collapsed');
    }

    setTimeout(() => {
      this.themesTable?.table?.redraw?.();
      this.sectionsTable?.table?.redraw?.();
    }, 350);
  }

  toggleMiddlePanel() {
    const middlePanel = this.elements.middlePanel;
    const splitter = this.elements.splitterRight;

    if (!middlePanel || !splitter) return;

    this.isMiddlePanelCollapsed = !this.isMiddlePanelCollapsed;

    if (this.isMiddlePanelCollapsed) {
      middlePanel.classList.add('collapsed');
      splitter.classList.add('collapsed');
    } else {
      middlePanel.classList.remove('collapsed');
      middlePanel.style.width = `${this.middlePanelWidth}px`;
      splitter.classList.remove('collapsed');
    }

    setTimeout(() => {
      this.themesTable?.table?.redraw?.();
      this.sectionsTable?.table?.redraw?.();
    }, 350);
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
        { value: 'themes-list-view', label: 'Themes List View' },
        { value: 'themes-list-data', label: 'Themes List Data' },
        { value: 'sections-list-view', label: 'Sections List View' },
        { value: 'sections-list-data', label: 'Sections List Data' },
      ],
      fillerTitle: 'Style',
      anchor: placeholder,
    });

    this._footerDatasource = footerElements.reportSelect;

    log(Subsystems.MANAGER, Status.INFO, '[StyleManager] Footer controls initialized');
  }

  _getFooterDatasource() {
    return this._footerDatasource?.value || 'themes-list-view';
  }

  handleFooterPrint() {
    const mode = this._getFooterDatasource();
    log(Subsystems.MANAGER, Status.INFO, `[Style] Footer: Print (${mode})`);

    const isParentMode = mode.startsWith('themes-list');
    const table = isParentMode ? this.themesTable?.table : this.sectionsTable?.table;

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
    log(Subsystems.MANAGER, Status.INFO, `[Style] Footer: Email (${mode})`);

    const isParentMode = mode.startsWith('themes-list');
    const table = isParentMode ? this.themesTable?.table : this.sectionsTable?.table;
    const tableName = isParentMode ? 'Themes List' : 'Sections List';

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
    log(Subsystems.MANAGER, Status.INFO, `[Style] Footer: Export ${format.toUpperCase()} (${mode})`);

    const isParentMode = mode.startsWith('themes-list');
    const table = isParentMode ? this.themesTable?.table : this.sectionsTable?.table;

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

  // ── Lifecycle ──────────────────────────────────────────────────────────────

  show() {
    requestAnimationFrame(() => {
      this.elements.container?.classList.add('visible');
    });
  }

  onActivate() {
    log(Subsystems.MANAGER, Status.INFO, '[Style] Activated');

    if (this.themesTable?.table) {
      this.themesTable.table.redraw?.();
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

    this._closeFooterExportPopup();

    if (this.fontPopup) {
      this.fontPopup.remove();
      this.fontPopup = null;
    }

    this.themesTable?.destroy();
    this.sectionsTable?.destroy();

    this.themesTable = null;
    this.sectionsTable = null;

    if (this.cssEditor) {
      this.cssEditor.destroy();
      this.cssEditor = null;
    }

    if (this.variablesEditor) {
      this.variablesEditor.destroy();
      this.variablesEditor = null;
    }
  }
}
