/**
 * Lookups Manager — Manager ID 5
 *
 * A three-panel interface for managing lookup tables:
 * - Left panel: List of all lookups (parent table)
 * - Middle panel: Values for the selected lookup (child table)
 * - Right panel: Tabbed view with JSON, Summary, and Preview
 *
 * Uses the reusable LithiumTable component for both tables.
 *
 * @module managers/lookups
 */

import { LithiumTable } from '../../core/lithium-table-main.js';
import { LithiumSplitter } from '../../core/lithium-splitter.js';
import { PanelStateManager } from '../../core/panel-state-manager.js';
import '../../styles/vendor-tabulator.css';
import { authQuery } from '../../shared/conduit.js';
import { toast } from '../../shared/toast.js';
import { log, Subsystems, Status } from '../../core/log.js';
import { processIcons } from '../../core/icons.js';
import { setupManagerFooterIcons, createFontPopup } from '../../core/manager-ui.js';
import SunEditor from 'suneditor';
import 'suneditor/css/editor';
import { initJsonTree, getJsonTreeData, setJsonTreeData, destroyJsonTree } from '../../components/json-tree-component.js';
import './lookups.css';

// ── Footer Select Options ───────────────────────────────────────────────────

const FOOTER_SELECTS = [
  {
    id: 'lookups-parent-select',
    options: [
      { value: 'view', label: 'Lookup List View' },
      { value: 'data', label: 'Lookup List Data' },
    ],
    value: 'view',
  },
  {
    id: 'lookups-child-select',
    options: [
      { value: 'view', label: 'Lookup Values View' },
      { value: 'data', label: 'Lookup Values Data' },
    ],
    value: 'view',
  },
];

// ── LookupsManager Class ────────────────────────────────────────────────────

export default class LookupsManager {
  constructor(app, container) {
    this.app = app;
    this.container = container;
    this.elements = {};

    // Parent table (lookups list)
    this.parentTable = null;

    // Child table (lookup values)
    this.childTable = null;

    // Currently selected lookup
    this.selectedLookupId = null;
    this.selectedLookupName = null;

    // Currently selected lookup value
    this.selectedLookupValue = null;

    // Panel state persistence
    this.leftPanelState = new PanelStateManager('lithium_lookups_left');
    this.middlePanelState = new PanelStateManager('lithium_lookups_middle');

    // Splitter state (loaded from persistence)
    this.leftPanelWidth = this.leftPanelState.loadWidth(280);
    this.middlePanelWidth = this.middlePanelState.loadWidth(350);
    this.isLeftPanelCollapsed = this.leftPanelState.loadCollapsed(false);
    this.isMiddlePanelCollapsed = this.middlePanelState.loadCollapsed(false);
    this.isResizingLeft = false;
    this.isResizingRight = false;

    // Editor instances
    this.sunEditor = null;
    this.collectionEditor = null; // JSON editor (CodeMirror) instance
    this.currentDetailData = null;

    // Active tab
    this.activeTab = 'json';

    // Font popup state
    this.fontPopup = null;
    this.editorFontSize = 14;
    this.editorFontFamily = 'var(--font-sans)';
  }

  async init() {
    await this.render();
    this.setupEventListeners();
    this.setupSplitters();
    await this.initParentTable();
    await this.initChildTable();
    this.setupFooter();
    this.setupTabs();
    this.restorePanelState();
  }

  async render() {
    try {
      const response = await fetch('/src/managers/lookups/lookups.html');
      const html = await response.text();
      this.container.innerHTML = html;
    } catch (error) {
      console.error('[LookupsManager] Failed to load template:', error);
      this.container.innerHTML = '<div class="error">Failed to load Lookups Manager</div>';
      return;
    }

    this.elements = {
      container: this.container.querySelector('.lookups-manager-container'),
      leftPanel: this.container.querySelector('#lookups-left-panel'),
      middlePanel: this.container.querySelector('#lookups-middle-panel'),
      rightPanel: this.container.querySelector('#lookups-right-panel'),
      parentTableContainer: this.container.querySelector('#lookups-parent-table-container'),
      parentNavigator: this.container.querySelector('#lookups-parent-navigator'),
      childTableContainer: this.container.querySelector('#lookups-child-table-container'),
      childNavigator: this.container.querySelector('#lookups-child-navigator'),
      childTitle: this.container.querySelector('#lookups-child-title'),
      splitterLeft: this.container.querySelector('#lookups-splitter-left'),
      splitterRight: this.container.querySelector('#lookups-splitter-right'),
      collapseLeftBtn: this.container.querySelector('#lookups-collapse-left-btn'),
      collapseMiddleBtn: this.container.querySelector('#lookups-collapse-middle-btn'),
      collapseLeftIcon: this.container.querySelector('#lookups-collapse-left-icon'),
      collapseMiddleIcon: this.container.querySelector('#lookups-collapse-middle-icon'),
      fontBtn: this.container.querySelector('#lookups-font-btn'),
      tabBtns: this.container.querySelectorAll('.lookups-tab-btn'),
      tabPanes: this.container.querySelectorAll('.lookups-tab-pane'),
      jsonEditor: this.container.querySelector('#lookups-json-editor'),
      summaryEditor: this.container.querySelector('#lookups-summary-editor'),
      previewContent: this.container.querySelector('#lookups-preview-content'),
    };

    // Process icons
    processIcons(this.container);
  }

  setupEventListeners() {
    // Collapse/expand left panel button
    this.elements.collapseLeftBtn?.addEventListener('click', () => {
      this.toggleLeftPanel();
    });

    // Collapse/expand middle panel button
    this.elements.collapseMiddleBtn?.addEventListener('click', () => {
      this.toggleMiddlePanel();
    });

    // Font button
    this.elements.fontBtn?.addEventListener('click', (e) => {
      this.toggleFontPopup(e);
    });

    // Initialize font popup
    this.initFontPopup();
  }

  // ── Font Popup ─────────────────────────────────────────────────────────────

  initFontPopup() {
    const { popup, getState } = createFontPopup({
      anchor: this.elements.fontBtn,
      fontSize: parseInt(this.editorFontSize, 10) || 14,
      fontFamily: this.editorFontFamily,
      fontWeight: 'normal',
      onChange: () => this.applyFontSettings(),
    });
    this.fontPopup = popup;
    this._fontPopupGetState = getState;
  }

  toggleFontPopup(e) {
    e.stopPropagation();
    if (this.fontPopup) {
      this.fontPopup.classList.toggle('visible');
    }
  }

  applyFontSettings() {
    // Apply font settings to JSON editor
    if (this.elements.jsonEditor) {
      this.elements.jsonEditor.style.fontSize = this.editorFontSize;
      this.elements.jsonEditor.style.fontFamily = this.editorFontFamily;
    }

    // Apply font settings to SunEditor if initialized
    if (this.sunEditor) {
      const editorBody = this.sunEditor.getContext()?.element?.wysiwyg;
      if (editorBody) {
        editorBody.style.fontSize = this.editorFontSize;
        editorBody.style.fontFamily = this.editorFontFamily;
      }
    }

    // Apply font settings to preview content
    if (this.elements.previewContent) {
      this.elements.previewContent.style.fontSize = this.editorFontSize;
      this.elements.previewContent.style.fontFamily = this.editorFontFamily;
    }
  }

  setupTabs() {
    // Tab switching
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
      pane.classList.toggle('active', pane.id === `lookups-tab-${tabId}`);
    });

    // Initialize editors on first view
    if (tabId === 'json' && !this.collectionEditor) {
      const jsonContent = this.currentDetailData?.collection || this.currentDetailData?.json || {};
      this.initJsonEditor(jsonContent);
    }

    if (tabId === 'summary' && !this.sunEditor) {
      this.initSummaryEditor();
    }

    // Refresh preview if switching to preview tab
    if (tabId === 'preview') {
      this.refreshPreview();
    }
  }

  initSummaryEditor() {
    if (!this.elements.summaryEditor || this.sunEditor) return;

    try {
      this.sunEditor = SunEditor.create(this.elements.summaryEditor, {
        height: '100%',
        width: '100%',
        defaultStyle: 'font-family: var(--font-sans); font-size: 14px;',
        buttonList: [
          ['undo', 'redo'],
          ['fontSize', 'formatBlock'],
          ['bold', 'underline', 'italic', 'strike', 'subscript', 'superscript'],
          ['removeFormat'],
          '/', // Line break
          ['fontColor', 'hiliteColor'],
          ['outdent', 'indent'],
          ['align', 'horizontalRule', 'list', 'table'],
          ['link', 'image'],
          ['fullScreen', 'showBlocks', 'codeView'],
        ],
        placeholder: 'Enter summary here...',
        darkMode: true,
      });

      // Set initial content if we have detail data
      if (this.currentDetailData?.summary) {
        this.sunEditor.setContents(this.currentDetailData.summary);
      }
    } catch (error) {
      console.error('[LookupsManager] Failed to initialize SunEditor:', error);
    }
  }

  /**
   * Initialize CodeMirror JSON editor for JSON tab
   * @param {Object|string} initialContent - Initial JSON content
   */
  async initJsonEditor(initialContent = {}) {
    if (!this.elements.jsonEditor) return;

    // Parse content if it's a string
    let jsonData = initialContent;
    if (typeof initialContent === 'string') {
      try {
        jsonData = JSON.parse(initialContent);
      } catch (e) {
        jsonData = {};
      }
    }

    // If editor already exists, just update content
    if (this.collectionEditor) {
      setJsonTreeData(this.elements.jsonEditor, jsonData);
      return;
    }

    try {
      this.collectionEditor = await initJsonTree({
        target: this.elements.jsonEditor,
        data: jsonData,
        readOnly: true,
        onJsonEdit: () => {
          // Track dirty state when editor content changes
          if (this.elements.jsonEditor) {
            this.elements.jsonEditor._isDirty = true;
          }
        },
      });
    } catch (error) {
      console.error('[LookupsManager] Failed to initialize JSON editor:', error);
      // Fallback to textarea
      const jsonContent = typeof jsonData === 'object'
        ? JSON.stringify(jsonData, null, 2)
        : jsonData;
      this.elements.jsonEditor.innerHTML = `
        <textarea id="lookups-json-editor-fallback"
          style="width:100%;height:100%;background:var(--bg-secondary);color:var(--text-primary);border:none;padding:16px;font-family:var(--font-mono);font-size:14px;">
          ${jsonContent}
        </textarea>`;
    }
  }

  // ── Parent Table Initialization ────────────────────────────────────────────

  async initParentTable() {
    if (!this.elements.parentTableContainer || !this.elements.parentNavigator) return;

    this.parentTable = new LithiumTable({
      container: this.elements.parentTableContainer,
      navigatorContainer: this.elements.parentNavigator,
      tablePath: 'lookups/lookups-list',
      queryRef: 30, // QueryRef 30 - Get Lookups List
      searchQueryRef: 31, // QueryRef 31 - Get Lookups List + Search
      cssPrefix: 'lithium', // Use default prefix for shared styling
      storageKey: 'lookups_parent_table',
      app: this.app,
      readonly: false,
      onRowSelected: (rowData) => this.handleParentRowSelected(rowData),
      onRowDeselected: () => this.handleParentRowDeselected(),
      onDataLoaded: (rows) => {
        log(Subsystems.TABLE, Status.INFO, `[Lookups] Loaded ${rows.length} lookups`);
      },
      onSetTableWidth: (mode) => this.setTableWidth(mode, 'left'),
      onEditModeChange: (isEditing, rowData) => this.handleTableEditModeChange(this.parentTable, isEditing, rowData),
    });

    await this.parentTable.init();

    // Load parent data
    await this.parentTable.loadData();
  }

  // ── Child Table Initialization ─────────────────────────────────────────────

  async initChildTable() {
    if (!this.elements.childTableContainer || !this.elements.childNavigator) return;

    this.childTable = new LithiumTable({
      container: this.elements.childTableContainer,
      navigatorContainer: this.elements.childNavigator,
      tablePath: 'lookups/lookup-values',
      queryRef: 34, // QueryRef 34 - Get Lookup List (requires LOOKUPID param)
      cssPrefix: 'lithium', // Use default prefix for shared styling
      storageKey: 'lookups_child_table',
      app: this.app,
      readonly: false,
      onRowSelected: (rowData) => this.handleChildRowSelected(rowData),
      onRowDeselected: () => this.handleChildRowDeselected(),
      onDataLoaded: (rows) => {
        log(Subsystems.TABLE, Status.INFO, `[Lookups] Loaded ${rows.length} lookup values`);
      },
      onSetTableWidth: (mode) => this.setTableWidth(mode, 'middle'),
      onEditModeChange: (isEditing, rowData) => this.handleTableEditModeChange(this.childTable, isEditing, rowData),
    });

    await this.childTable.init();
  }

  // ── Table Width Control ────────────────────────────────────────────────────

  setTableWidth(mode, panel) {
    const panelElement = panel === 'left' ? this.elements.leftPanel : this.elements.middlePanel;
    if (!panelElement) return;

    // Match Query Manager width setpoints
    // Get width from CSS variables
    const widthVar = `--table-width-${mode}`;
    const computedStyle = getComputedStyle(document.documentElement);
    const width = computedStyle.getPropertyValue(widthVar).trim();

    if (mode === 'auto' || !width) {
      panelElement.style.width = '';
      return;
    }

    panelElement.style.width = width;
    log(Subsystems.MANAGER, Status.INFO, `[Lookups] ${panel} panel width set to: ${mode} (${width})`);
  }

  // ── Parent/Child Relationship ──────────────────────────────────────────────

  async handleParentRowSelected(rowData) {
    if (!rowData) return;

    const lookupId = rowData.key_idx; // key_idx is the lookup_id for lookup_id=0 entries
    const lookupName = rowData.value_txt;

    if (lookupId == null) return;

    this.selectedLookupId = lookupId;
    this.selectedLookupName = lookupName;

    // Load child data for this lookup
    await this.loadChildData(lookupId);
  }

  handleParentRowDeselected() {
    this.selectedLookupId = null;
    this.selectedLookupName = null;

    // Clear child table data
    if (this.childTable?.table) {
      this.childTable.table.setData([]);
    }

    // Clear detail view
    this.clearDetailView();
  }

  async loadChildData(lookupId) {
    if (!this.childTable || !this.app?.api) return;

    try {
      // Use the child table's loadData() method which handles row restoration
      await this.childTable.loadData('', {
        INTEGER: { LOOKUPID: lookupId },
      });

      log(Subsystems.TABLE, Status.INFO, `[Lookups] Loaded lookup values for lookup ${lookupId}`);
    } catch (error) {
      toast.error('Failed to load lookup values', {
        serverError: error.serverError,
        subsystem: 'Conduit',
        duration: 6000,
      });
    }
  }

  // ── Child/Detail Relationship ──────────────────────────────────────────────

  async handleChildRowSelected(rowData) {
    if (!rowData) return;

    this.selectedLookupValue = rowData;

    // Fetch full detail using QueryRef 35
    await this.loadDetailData(rowData);
  }

  handleChildRowDeselected() {
    this.selectedLookupValue = null;
    this.clearDetailView();
  }

  async loadDetailData(rowData) {
    if (!this.app?.api || this.selectedLookupId == null) return;

    try {
      const detail = await authQuery(this.app.api, 35, {
        INTEGER: {
          LOOKUPID: this.selectedLookupId,
          KEYIDX: rowData.key_idx,
        },
      });

      if (detail && detail.length > 0) {
        this.currentDetailData = detail[0];
        this.updateDetailView();
      }
    } catch (error) {
      log(Subsystems.TABLE, Status.ERROR, `[Lookups] Failed to load detail: ${error.message}`);
      // Don't show toast - detail view is optional
    }
  }

  updateDetailView() {
    if (!this.currentDetailData) return;

    // Update JSON editor
    const jsonContent = this.currentDetailData.collection || this.currentDetailData.json || {};
    if (this.collectionEditor) {
      // Editor already initialized, just update content
      const jsonData = typeof jsonContent === 'string' ? JSON.parse(jsonContent || '{}') : jsonContent;
      setJsonTreeData(this.elements.jsonEditor, jsonData);
    } else if (this.elements.jsonEditor && this.activeTab === 'json') {
      // Initialize editor if JSON tab is active
      this.initJsonEditor(jsonContent);
    }

    // Update Summary editor if initialized
    if (this.sunEditor) {
      this.sunEditor.setContents(this.currentDetailData.summary || '');
    }

    // Refresh preview if active
    if (this.activeTab === 'preview') {
      this.refreshPreview();
    }
  }

  clearDetailView() {
    this.currentDetailData = null;

    // Clear JSON editor
    if (this.collectionEditor) {
      setJsonTreeData(this.elements.jsonEditor, {});
    } else if (this.elements.jsonEditor) {
      this.elements.jsonEditor.innerHTML = '<p class="lookups-preview-placeholder">Select a lookup entry to view details</p>';
    }

    if (this.sunEditor) {
      this.sunEditor.setContents('');
    }

    if (this.elements.previewContent) {
      this.elements.previewContent.innerHTML = '<p class="lookups-preview-placeholder">Select a lookup entry to preview</p>';
    }
  }

  async refreshPreview() {
    if (!this.elements.previewContent) return;

    if (!this.currentDetailData) {
      this.elements.previewContent.innerHTML = '<p class="lookups-preview-placeholder">Select a lookup entry to preview</p>';
      return;
    }

    try {
      // Try to use highlight.js or prism.js if available
      let htmlContent = '';

      // Get content from SunEditor or raw summary
      const summaryContent = this.sunEditor
        ? this.sunEditor.getContents()
        : (this.currentDetailData.summary || '');

      if (summaryContent) {
        // Check if highlight.js is available
        if (window.hljs) {
          // Simple markdown-like processing with syntax highlighting
          htmlContent = this.processContentWithHighlighting(summaryContent);
        } else {
          // Basic HTML processing
          htmlContent = this.basicContentProcessing(summaryContent);
        }
      } else {
        htmlContent = '<p class="lookups-preview-placeholder">No summary content available</p>';
      }

      this.elements.previewContent.innerHTML = htmlContent;
    } catch (error) {
      console.error('[LookupsManager] Failed to refresh preview:', error);
      this.elements.previewContent.innerHTML = '<p class="lookups-preview-placeholder">Error rendering preview</p>';
    }
  }

  processContentWithHighlighting(content) {
    // Escape HTML first
    let html = content
      .replace(/&/g, '&')
      .replace(/</g, '<')
      .replace(/>/g, '>');

    // Process code blocks with highlight.js
    html = html.replace(/```(\w+)?\n([\s\S]*?)```/g, (match, lang, code) => {
      if (window.hljs) {
        try {
          const highlighted = lang
            ? window.hljs.highlight(code.trim(), { language: lang }).value
            : window.hljs.highlightAuto(code.trim()).value;
          return `<pre><code class="hljs">${highlighted}</code></pre>`;
        } catch (e) {
          return `<pre><code>${code.trim()}</code></pre>`;
        }
      }
      return `<pre><code>${code.trim()}</code></pre>`;
    });

    // Process inline code
    html = html.replace(/`([^`]+)`/g, '<code>$1</code>');

    // Process basic formatting
    html = html.replace(/\*\*([^*]+)\*\*/g, '<strong>$1</strong>');
    html = html.replace(/\*([^*]+)\*/g, '<em>$1</em>');
    html = html.replace(/^# (.+)$/gm, '<h1>$1</h1>');
    html = html.replace(/^## (.+)$/gm, '<h2>$1</h2>');
    html = html.replace(/^### (.+)$/gm, '<h3>$1</h3>');
    html = html.replace(/\n/g, '<br>');

    return `<div class="lookups-preview-rendered">${html}</div>`;
  }

  basicContentProcessing(content) {
    // Simple HTML escaping and line breaks
    return content
      .replace(/&/g, '&')
      .replace(/</g, '<')
      .replace(/>/g, '>')
      .replace(/\n/g, '<br>');
  }

  // ── Splitter Logic ─────────────────────────────────────────────────────────

  setupSplitters() {
    // Left splitter (between left and middle panels)
    this.leftSplitter = new LithiumSplitter({
      element: this.elements.splitterLeft,
      leftPanel: this.elements.leftPanel,
      minWidth: 157,
      maxWidth: 1000,
      onResize: (width) => {
        this.leftPanelWidth = width;
      },
      onResizeEnd: (width) => {
        this.leftPanelState.saveWidth(width);
        this.parentTable?.table?.redraw?.();
        this.childTable?.table?.redraw?.();
      },
    });

    // Right splitter (between middle and right panels)
    this.rightSplitter = new LithiumSplitter({
      element: this.elements.splitterRight,
      leftPanel: this.elements.middlePanel,
      minWidth: 157,
      maxWidth: 1000,
      onResize: (width) => {
        this.middlePanelWidth = width;
      },
      onResizeEnd: (width) => {
        this.middlePanelState.saveWidth(width);
        this.parentTable?.table?.redraw?.();
        this.childTable?.table?.redraw?.();
      },
    });
  }

  toggleLeftPanel() {
    this.isLeftPanelCollapsed = !this.isLeftPanelCollapsed;
    this.leftSplitter?.setCollapsed(this.isLeftPanelCollapsed);

    // Toggle rotation class on collapse button
    this.elements.collapseLeftBtn?.classList.toggle('collapsed', this.isLeftPanelCollapsed);

    // Save collapsed state
    this.leftPanelState.saveCollapsed(this.isLeftPanelCollapsed);

    if (!this.isLeftPanelCollapsed) {
      this.elements.leftPanel.style.width = `${this.leftPanelWidth}px`;
    }

    // Redraw tables after animation
    setTimeout(() => {
      this.parentTable?.table?.redraw?.();
      this.childTable?.table?.redraw?.();
    }, 350);
  }

  toggleMiddlePanel() {
    this.isMiddlePanelCollapsed = !this.isMiddlePanelCollapsed;
    this.rightSplitter?.setCollapsed(this.isMiddlePanelCollapsed);

    // Toggle rotation class on collapse button
    this.elements.collapseMiddleBtn?.classList.toggle('collapsed', this.isMiddlePanelCollapsed);

    // Save collapsed state
    this.middlePanelState.saveCollapsed(this.isMiddlePanelCollapsed);

    if (!this.isMiddlePanelCollapsed) {
      this.elements.middlePanel.style.width = `${this.middlePanelWidth}px`;
    }

    // Redraw tables after animation
    setTimeout(() => {
      this.parentTable?.table?.redraw?.();
      this.childTable?.table?.redraw?.();
    }, 350);
  }

  restorePanelState() {
    // Restore left panel
    this.elements.collapseLeftBtn?.classList.toggle('collapsed', this.isLeftPanelCollapsed);
    this.leftSplitter?.setCollapsed(this.isLeftPanelCollapsed);

    // Restore middle panel
    this.elements.collapseMiddleBtn?.classList.toggle('collapsed', this.isMiddlePanelCollapsed);
    this.rightSplitter?.setCollapsed(this.isMiddlePanelCollapsed);

    // Apply saved widths to panels (will be used when expanding)
    if (this.elements.leftPanel && !this.isLeftPanelCollapsed) {
      this.elements.leftPanel.style.width = `${this.leftPanelWidth}px`;
    }
    if (this.elements.middlePanel && !this.isMiddlePanelCollapsed) {
      this.elements.middlePanel.style.width = `${this.middlePanelWidth}px`;
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

    // Get the placeholder - manager buttons go before it, fixed buttons go after
    const placeholder = group.querySelector('.slot-footer-placeholder');

    const footerElements = setupManagerFooterIcons(group, {
      onPrint: () => this.handleFooterPrint(),
      onEmail: () => this.handleFooterEmail(),
      onExport: (e) => this.toggleFooterExportPopup(e),
      reportOptions: [
        { value: 'lookups-list-view', label: 'Lookups List View' },
        { value: 'lookups-list-data', label: 'Lookups List Data' },
        { value: 'lookup-list-view', label: 'Lookup List View' },
        { value: 'lookup-list-data', label: 'Lookup List Data' },
      ],
      fillerTitle: 'Lookups',
      anchor: placeholder, // Insert manager buttons before placeholder
      showSaveCancel: true,
    });

    this._footerDatasource = footerElements.reportSelect;

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

    log(Subsystems.MANAGER, Status.INFO, '[LookupsManager] Footer controls initialized');
  }

  /**
   * Get the selected data source mode from the footer combobox.
   * @returns {string} - selected data source value
   */
  _getFooterDatasource() {
    return this._footerDatasource?.value || 'lookups-list-view';
  }

  /**
   * Handle Print from the footer.
   */
  handleFooterPrint() {
    const mode = this._getFooterDatasource();
    log(Subsystems.MANAGER, Status.INFO, `[Lookups] Footer: Print (${mode})`);

    // Determine which table to print based on mode
    const isParentMode = mode.startsWith('lookups-list');
    const table = isParentMode ? this.parentTable?.table : this.childTable?.table;

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

  /**
   * Handle Email from the footer.
   */
  handleFooterEmail() {
    const mode = this._getFooterDatasource();
    log(Subsystems.MANAGER, Status.INFO, `[Lookups] Footer: Email (${mode})`);

    // Determine which table to email based on mode
    const isParentMode = mode.startsWith('lookups-list');
    const table = isParentMode ? this.parentTable?.table : this.childTable?.table;
    const tableName = isParentMode ? 'Lookups List' : 'Lookup List';

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

    // Build a plain-text table summary
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
    log(Subsystems.MANAGER, Status.INFO, `[Lookups] Footer email prepared with ${totalRows} row(s) (${mode})`);
  }

  /**
   * Toggle the export format popup from the footer export button.
   * @param {MouseEvent} e - click event
   */
  toggleFooterExportPopup(e) {
    e.stopPropagation();

    // If already open, close it
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

    // Build popup
    const popup = document.createElement('div');
    popup.className = 'lookups-footer-export-popup';
    formats.forEach(item => {
      const row = document.createElement('button');
      row.type = 'button';
      row.className = 'lookups-footer-export-popup-item';
      row.textContent = item.label;
      row.addEventListener('click', () => {
        this._closeFooterExportPopup();
        item.action();
      });
      popup.appendChild(row);
    });

    // Position popup above the button (appended to body to avoid overflow issues)
    const btnRect = btn.getBoundingClientRect();
    document.body.appendChild(popup);

    // Position popup above the button (bottom-left of popup aligns with top-left of button)
    requestAnimationFrame(() => {
      const popupRect = popup.getBoundingClientRect();
      popup.style.position = 'fixed';
      popup.style.top = `${btnRect.top - popupRect.height - 8}px`;
      popup.style.left = `${btnRect.left}px`;
    });

    // Animate in after a small delay
    setTimeout(() => {
      popup.classList.add('visible');
    }, 10);

    this._footerExportPopup = popup;

    // Close on click outside
    this._footerExportCloseHandler = (evt) => {
      if (!popup.contains(evt.target) && !btn.contains(evt.target)) {
        this._closeFooterExportPopup();
      }
    };
    document.addEventListener('click', this._footerExportCloseHandler);
  }

  /**
   * Close the footer export popup.
   */
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

  /**
   * Handle export from the footer with data source mode.
   * @param {'pdf'|'csv'|'txt'|'xls'} format
   * @param {string} mode - data source mode
   */
  handleFooterExport(format, mode) {
    log(Subsystems.MANAGER, Status.INFO, `[Lookups] Footer: Export ${format.toUpperCase()} (${mode})`);

    // Determine which table to export based on mode
    const isParentMode = mode.startsWith('lookups-list');
    const table = isParentMode ? this.parentTable?.table : this.childTable?.table;

    if (!table) {
      toast.info('No Data', { description: 'No table available to export', duration: 3000 });
      return;
    }

    const filename = `lookups-export-${new Date().toISOString().slice(0, 10)}`;
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

  // ── Footer Save/Cancel ─────────────────────────────────────────────────────

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
      log(Subsystems.MANAGER, Status.INFO, `[Lookups] Footer Save/Cancel enabled for table`);
    } else {
      if (this.activeEditingTable === lithiumTable) {
        this.activeEditingTable = null;
      }
      this.updateFooterSaveCancelState(true, false);
      log(Subsystems.MANAGER, Status.INFO, `[Lookups] Footer Save/Cancel disabled`);
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

  // ── Lifecycle Methods ──────────────────────────────────────────────────────

  /**
   * Called when the manager is activated
   */
  onActivate() {
    log(Subsystems.MANAGER, Status.INFO, '[Lookups] Activated');

    // Redraw tables to ensure proper layout
    if (this.parentTable?.table) {
      this.parentTable.table.redraw?.();
    }
    if (this.childTable?.table) {
      this.childTable.table.redraw?.();
    }
  }

  /**
   * Called when the manager is deactivated
   */
  onDeactivate() {
    log(Subsystems.MANAGER, Status.INFO, '[Lookups] Deactivated');
  }

  /**
   * Called before the manager is destroyed
   */
  cleanup() {
    log(Subsystems.MANAGER, Status.INFO, '[Lookups] Cleaning up...');

    // Destroy SunEditor
    if (this.sunEditor) {
      this.sunEditor.destroy();
      this.sunEditor = null;
    }

    // Destroy JSON editor
    if (this.collectionEditor) {
      destroyJsonTree(this.elements.jsonEditor);
      this.collectionEditor = null;
    }

    // Remove font popup
    if (this.fontPopup) {
      this.fontPopup.remove();
      this.fontPopup = null;
    }

    // Close any open footer export popup
    this._closeFooterExportPopup();

    // Clean up splitters
    this.leftSplitter?.destroy();
    this.rightSplitter?.destroy();
    this.leftSplitter = null;
    this.rightSplitter = null;

    // Clean up tables
    this.parentTable?.destroy();
    this.childTable?.destroy();

    this.parentTable = null;
    this.childTable = null;
  }
}
