/**
 * Queries Manager
 *
 * Query builder and execution interface.
 * Uses LithiumTable + ManagerEditHelper (same pattern as Lookups/Style managers).
 *
 * Custom modules:
 *   queries-editors.js — CodeMirror editor init (SQL, Summary, Collection)
 *
 * Retired modules (functionality now in LithiumTable + ManagerEditHelper):
 *   queries-dirty.js      — replaced by ManagerEditHelper snapshot tracking
 *   queries-edit-mode.js  — replaced by LithiumTable.isEditing / editHelper
 *   queries-navigation.js — replaced by LithiumTable nav + onExecuteSave callback
 */

// Import Styles
import '../../styles/vendor-tabulator.css';
import '../../core/manager-panels.css';
import './queries.css';

import { LithiumTable } from '../../core/lithium-table-main.js';
import { LithiumSplitter } from '../../core/lithium-splitter.js';
import { PanelStateManager } from '../../core/panel-state-manager.js';
import { togglePanelCollapse } from '../../core/panel-collapse.js';
import { authQuery } from '../../shared/conduit.js';
import { toast } from '../../shared/toast.js';
import { log, Subsystems, Status } from '../../core/log.js';
import { processIcons } from '../../core/icons.js';
import { setupManagerFooterIcons, createFontPopup } from '../../core/manager-ui.js';
import { ManagerEditHelper } from '../../core/manager-edit-helper.js';

// Editor management (CodeMirror init, font, prettify, undo/redo)
import { createEditorManager } from './queries-editors.js';

/**
 * Queries Manager - Main class
 * Follows the standard LithiumTable + ManagerEditHelper pattern.
 */
export default class QueriesManager {
  constructor(app, container) {
    this.app = app;
    this.container = container;
    this.elements = {};

    // LithiumTable instance for query list
    this.queryTable = null;

    // Manager ID
    this.managerId = 4;

    // Current query data
    this.currentQuery = null;
    this._loadedDetailRowId = null;
    this._pendingSqlContent = null;
    this._pendingSummaryContent = null;
    this._pendingCollectionContent = null;

    // Panel state persistence
    this.leftPanelState = new PanelStateManager('lithium_queries_left');
    this.leftPanelWidth = this.leftPanelState.loadWidth(280);
    this.isLeftPanelCollapsed = this.leftPanelState.loadCollapsed(false);

    // Edit helper — consolidates edit mode, dirty tracking, and save/cancel buttons
    this.editHelper = new ManagerEditHelper({
      name: 'Queries',
      onAfterEditModeChange: (isEditing) => this._onEditModeChanged(isEditing),
    });

    // Editor manager (CodeMirror init, font, prettify)
    this.editorManager = createEditorManager(this);

    // Font popup state
    this.fontPopup = null;
    this.editorFontSize = 14;
    this.editorFontFamily = 'var(--font-mono)';
    this.editorFontWeight = 'normal';
    this._fontResetPending = false;

    // Expose editor references (set by editorManager after init)
    this.sqlEditor = null;
    this.summaryEditor = null;
    this.collectionEditor = null;
  }

  // ============ LIFECYCLE ============

  async init() {
    await this._render();
    this._setupEventListeners();
    await this._initQueryTable();
    this._setupSplitters();
    this._setupFooter();
    this._restorePanelState();
    this._loadSavedFontSettings();
  }

  /**
   * Load saved font settings from localStorage for the default tab
   */
  _loadSavedFontSettings() {
    const savedSettings = this._loadFontSettings('sql');
    if (savedSettings) {
      this.editorFontSize = savedSettings.fontSize;
      this.editorFontFamily = savedSettings.fontFamily;
      this.editorFontWeight = savedSettings.fontWeight;
    }
  }

  async _render() {
    try {
      const response = await fetch('/src/managers/queries/queries.html');
      const html = await response.text();
      this.container.innerHTML = html;
    } catch (error) {
      console.error('[QueriesManager] Failed to load template:', error);
      this.container.innerHTML = '<div class="error">Failed to load Queries Manager</div>';
      return;
    }

    this.elements = {
      container: this.container.querySelector('.queries-manager-container'),
      leftPanel: this.container.querySelector('.queries-left-panel'),
      tableContainer: this.container.querySelector('#queries-table-container'),
      navigatorContainer: this.container.querySelector('#queries-navigator-container'),
      splitter: this.container.querySelector('#queries-splitter'),
      rightPanel: this.container.querySelector('.queries-right-panel'),
      tabBtns: this.container.querySelectorAll('.queries-tab-btn:not(.queries-collapse-btn)'),
      tabPanes: this.container.querySelectorAll('.queries-tab-pane'),
      collapseBtn: this.container.querySelector('#queries-collapse-btn'),
      sqlEditorContainer: this.container.querySelector('#queries-sql-editor'),
      summaryEditorContainer: this.container.querySelector('#queries-tab-summary'),
      collectionEditorContainer: this.container.querySelector('#queries-tab-collection'),
      previewContainer: this.container.querySelector('#queries-tab-preview'),
      undoBtn: this.container.querySelector('#queries-undo-btn'),
      redoBtn: this.container.querySelector('#queries-redo-btn'),
      foldAllBtn: this.container.querySelector('#queries-fold-all-btn'),
      unfoldAllBtn: this.container.querySelector('#queries-unfold-all-btn'),
      fontBtn: this.container.querySelector('#queries-font-btn'),
      prettifyBtn: this.container.querySelector('#queries-prettify-btn'),
      tabsHeader: this.container.querySelector('.queries-tabs-header'),
    };

    processIcons(this.container);
  }

  // ============ QUERY TABLE INITIALIZATION ============

  async _initQueryTable() {
    if (!this.elements.tableContainer || !this.elements.navigatorContainer) return;

    this.queryTable = new LithiumTable({
      container: this.elements.tableContainer,
      navigatorContainer: this.elements.navigatorContainer,
      tablePath: 'queries/query-manager',
      queryRef: 25,           // QueryRef 25 - Query List
      searchQueryRef: 32,     // QueryRef 32 - Query Search
      cssPrefix: 'queries',
      storageKey: 'queries_table',
      app: this.app,
      readonly: false,
      panel: this.elements.leftPanel,
      panelStateManager: this.leftPanelState,
      onRowSelected: (rowData) => this._handleRowSelected(rowData),
      onRowDeselected: () => this._handleRowDeselected(),
      onDataLoaded: (rows) => {
        log(Subsystems.TABLE, Status.INFO, `[Queries] Loaded ${rows.length} queries`);
      },
      // Custom save: Query Manager assembles params from table + editors
      onExecuteSave: (row, editHelper) => this._executeSave(row, editHelper),
      // onEditModeChange and onDirtyChange are auto-wired by editHelper.registerTable()
    });

    // Register with editHelper — auto-wires onEditModeChange + onDirtyChange
    this.editHelper.registerTable(this.queryTable);

    // Register external editors for dirty tracking + cancel restore
    this._registerEditorsWithEditHelper();

    await this.queryTable.init();
    await this.queryTable.loadData();
  }

  /**
   * Register CodeMirror editors with editHelper for dirty tracking.
   */
  _registerEditorsWithEditHelper() {
    // Register SQL editor
    this.editHelper.registerEditor('sql', {
      getContent: () => this.sqlEditor?.state?.doc?.toString() || '',
      setContent: (content) => {
        // Cancel restore — exclude from undo history
        this.editorManager._setContentNoHistory(this.sqlEditor, content);
      },
      setEditable: (editable) => this.editorManager?._setCodeMirrorEditable(editable),
      boundTable: this.queryTable,
    });

    // Register Summary editor
    this.editHelper.registerEditor('summary', {
      getContent: () => this.summaryEditor?.state?.doc?.toString() || '',
      setContent: (content) => {
        this.editorManager._setContentNoHistory(this.summaryEditor, content);
      },
      setEditable: () => {}, // Handled by 'sql' editor registration above
      boundTable: this.queryTable,
    });

    // Register Collection editor
    this.editHelper.registerEditor('collection', {
      getContent: () => this.editorManager?.getCollectionContent() || '{}',
      setContent: (content) => {
        this.editorManager._setContentNoHistory(this.collectionEditor, content);
      },
      setEditable: () => {}, // Handled by 'sql' editor registration above
      boundTable: this.queryTable,
    });
  }

  // ============ CUSTOM SAVE LOGIC ============

  /**
   * Custom save for Query Manager.
   * Assembles table row data + CodeMirror editor content into API params.
   * Called by LithiumTable.executeSave() via onExecuteSave callback.
   *
   * @param {Tabulator.Row} row - The Tabulator row being saved
   * @param {ManagerEditHelper} editHelper - The edit helper (for editor access)
   */
  async _executeSave(row) {
    const rowData = row.getData();
    const pkField = this.queryTable.primaryKeyField || 'query_id';
    const pkValue = rowData[pkField];
    const isInsert = pkValue == null || pkValue === '' || pkValue === 0;
    const queryRef = isInsert
      ? (this.queryTable.queryRefs?.insertQueryRef ?? null)
      : (this.queryTable.queryRefs?.updateQueryRef ?? 28);

    if (!queryRef) {
      throw Object.assign(new Error('No queryRef configured for save'), {
        serverError: 'Save not available: no query reference configured.',
      });
    }

    // Gather content from all editors
    const sqlContent = this.sqlEditor?.state?.doc?.toString() || '';
    const summaryContent = this.summaryEditor?.state?.doc?.toString() || '';
    const collectionString = this.editorManager?.getCollectionContent() || '{}';

    const params = {
      INTEGER: {
        QUERYID: pkValue,
        QUERYTYPE: rowData.query_type_a28 ?? 1,
        QUERYDIALECT: rowData.query_dialect_a30 ?? 1,
        QUERYSTATUS: rowData.query_status_a27 ?? 1,
        USERID: this.app?.user?.id ?? 0,
        QUERYREF: parseInt(rowData.query_ref, 10) || 0,
      },
      STRING: {
        QUERYCODE: sqlContent || rowData.code || '',
        QUERYNAME: rowData.name || '',
        QUERYSUMMARY: summaryContent || rowData.summary || '',
        COLLECTION: collectionString,
      },
    };

    const result = await authQuery(this.app.api, queryRef, params);

    // Update row with server-returned PK on insert
    if (isInsert && result?.[0]?.[pkField] != null) {
      row.update({ [pkField]: result[0][pkField] });
    }
  }

  // ============ EVENT HANDLERS ============

  _setupEventListeners() {
    // Tab buttons
    this.elements.tabBtns?.forEach(btn => {
      btn.addEventListener('click', () => this.switchTab(btn.dataset.tab));
    });

    // Collapse button
    this.elements.collapseBtn?.addEventListener('click', () => this._toggleLeftPanel());

    // Toolbar buttons
    this.elements.undoBtn?.addEventListener('click', () => this.editorManager.handleUndo());
    this.elements.redoBtn?.addEventListener('click', () => this.editorManager.handleRedo());
    this.elements.foldAllBtn?.addEventListener('click', () => this.editorManager.handleFoldAll());
    this.elements.unfoldAllBtn?.addEventListener('click', () => this.editorManager.handleUnfoldAll());
    this.elements.fontBtn?.addEventListener('click', () => this._handleFontClick());
    this.elements.prettifyBtn?.addEventListener('click', () => this.editorManager.handlePrettify());

    // Keyboard shortcuts
    document.addEventListener('keydown', (e) => this._handleKeyboardShortcuts(e));
  }

  // ============ TABLE WIDTH CONTROL ============
  // Width persistence is now handled centrally by LithiumTable.
  // The Width popup in the Navigator calls LithiumTable.setTableWidth() directly,
  // which saves the mode to localStorage and applies the width to the panel.
  // Splitter drag clears the width mode via LithiumSplitter._clearWidthModes().
  // Panel pixel width is saved by the onResizeEnd callback in _setupSplitters().

  // ============ EDIT MODE ============

  /**
   * Called by editHelper.onAfterEditModeChange after edit mode transitions.
   * Updates undo/redo button state.
   */
  _onEditModeChanged(_isEditing) {
    this.editorManager._updateUndoRedoButtons();
  }

  // ============ TAB SWITCHING ============

  async switchTab(tabId) {
    this.elements.tabBtns.forEach(btn => btn.classList.toggle('active', btn.dataset.tab === tabId));
    this.elements.tabPanes.forEach(pane => pane.classList.toggle('active', pane.id === `queries-tab-${tabId}`));

    const hasEditor = ['sql', 'summary', 'collection'].includes(tabId);
    // Undo/redo enabled only for editor tabs; actual enabled state is managed
    // by _updateUndoRedoButtons based on edit mode + history depth.
    if (this.elements.fontBtn) this.elements.fontBtn.disabled = !['sql', 'summary', 'preview', 'collection'].includes(tabId);
    if (this.elements.prettifyBtn) this.elements.prettifyBtn.disabled = tabId !== 'sql';
    // Disable undo/redo for non-editor tabs (preview/test), otherwise let
    // the history-aware updater control them.
    if (!hasEditor) {
      if (this.elements.undoBtn) this.elements.undoBtn.disabled = true;
      if (this.elements.redoBtn) this.elements.redoBtn.disabled = true;
    }

    // Restore font state for the new tab and apply fonts
    this._restoreFontStateForTab(tabId);

    if (tabId === 'sql') {
      const sqlContent = this._pendingSqlContent || this.currentQuery?.code || this.currentQuery?.query_text || this.currentQuery?.sql || '';
      if (!this.sqlEditor) {
        await this.editorManager.initSqlEditor(sqlContent);
        this.sqlEditor = this.editorManager.sqlEditor;
      } else {
        const currentContent = this.sqlEditor.state.doc.toString();
        if (currentContent !== sqlContent) {
          // Programmatic content load — exclude from undo history
          this.editorManager._setContentNoHistory(this.sqlEditor, sqlContent);
        }
      }
      // Apply font after editor is visible
      requestAnimationFrame(() => this._applyFontToCurrentTab());
      this.editorManager._updateUndoRedoButtons();
    }
    if (tabId === 'summary') {
      const summaryContent = this._pendingSummaryContent || this.currentQuery?.summary || this.currentQuery?.markdown || '';
      if (!this.summaryEditor) {
        await this.editorManager.initSummaryEditor(summaryContent);
        this.summaryEditor = this.editorManager.summaryEditor;
      } else {
        const currentContent = this.summaryEditor.state.doc.toString();
        if (currentContent !== summaryContent) {
          this.editorManager._setContentNoHistory(this.summaryEditor, summaryContent);
        }
      }
      // Apply font after editor is visible
      requestAnimationFrame(() => this._applyFontToCurrentTab());
      this.editorManager._updateUndoRedoButtons();
    }
    if (tabId === 'collection') {
      let collectionContent = this._pendingCollectionContent || this.currentQuery?.collection || this.currentQuery?.json || {};
      if (typeof collectionContent === 'string') {
        try { collectionContent = JSON.parse(collectionContent); } catch { collectionContent = {}; }
      }
      if (this.collectionEditor) {
        // Update content directly via CodeMirror — exclude from undo history
        const jsonStr = JSON.stringify(collectionContent, null, 2);
        if (this.collectionEditor.state.doc.toString() !== jsonStr) {
          this.editorManager._setContentNoHistory(this.collectionEditor, jsonStr);
        }
      } else {
        await this.editorManager.initCollectionEditor(collectionContent);
        this.collectionEditor = this.editorManager.collectionEditor;
      }
      // Apply font after editor is visible
      requestAnimationFrame(() => this._applyFontToCurrentTab());
      this.editorManager._updateUndoRedoButtons();
    }
    if (tabId === 'preview') {
      this.editorManager.renderMarkdownPreview();
      // Apply font after preview is rendered
      requestAnimationFrame(() => this._applyFontToCurrentTab());
    }
  }

  /**
   * Apply current font settings to the active tab's content
   */
  _applyFontToCurrentTab() {
    const tabId = this._getActiveTabId();
    const fontSettings = {
      fontSize: this.editorFontSize,
      fontFamily: this.editorFontFamily,
      fontWeight: this.editorFontWeight,
    };
    this.editorManager.applyFontSettingsToActiveTab(fontSettings, tabId);
  }

  // ============ ROW SELECTION ============

  async _handleRowSelected(rowData) {
    if (!rowData) return;

    const queryId = rowData.query_id || rowData.id;
    if (queryId == null) return;

    // Don't refetch if the same row is already selected
    if (this._loadedDetailRowId === queryId) return;

    log(Subsystems.MANAGER, Status.INFO, `[Queries] Row selected: query_id=${queryId}`);

    this.currentQuery = rowData;
    this._loadedDetailRowId = queryId;

    // Load full query details
    await this._loadQueryDetails(queryId);
  }

  _handleRowDeselected() {
    this.currentQuery = null;
    this._loadedDetailRowId = null;
    this._clearQueryDetails();
  }

  async _loadQueryDetails(queryId) {
    if (!this.app?.api) return;

    try {
      // QueryRef 27 - Get System Query
      const details = await authQuery(this.app.api, 27, {
        INTEGER: { QUERYID: queryId },
      });

      if (details && details.length > 0) {
        this.currentQuery = { ...this.currentQuery, ...details[0] };
        this._pendingSqlContent = this.currentQuery.code || this.currentQuery.query_text || this.currentQuery.sql || '';
        this._pendingSummaryContent = this.currentQuery.summary || this.currentQuery.markdown || '';
        this._pendingCollectionContent = this.currentQuery.collection || this.currentQuery.json || {};

        // Update the active tab with new content
        const activeTab = this._getActiveTabId();
        this.switchTab(activeTab);
      }
    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, `[Queries] Failed to load query details: ${error.message}`);
      toast.error('Failed to load query details');
    }
  }

  _clearQueryDetails() {
    this._pendingSqlContent = null;
    this._pendingSummaryContent = null;
    this._pendingCollectionContent = null;

    // Programmatic clear — exclude from undo history
    if (this.sqlEditor) {
      this.editorManager._setContentNoHistory(this.sqlEditor, '');
    }
    if (this.summaryEditor) {
      this.editorManager._setContentNoHistory(this.summaryEditor, '');
    }
    if (this.collectionEditor) {
      this.editorManager._setContentNoHistory(this.collectionEditor, '{}');
    }
  }

  // ============ ACTIVE TAB HELPER ============

  _getActiveTabId() {
    const activeBtn = this.container?.querySelector('.queries-tab-btn.active');
    return activeBtn?.dataset?.tab || 'sql';
  }

  // ============ SPLITTER ============

  _setupSplitters() {
    this.splitter = new LithiumSplitter({
      element: this.elements.splitter,
      leftPanel: this.elements.leftPanel,
      minWidth: 157,
      maxWidth: 1000,
      tables: this.queryTable,
      onResize: (width) => {
        this.leftPanelWidth = width;
      },
      onResizeEnd: (width) => {
        this.leftPanelState.saveWidth(width);
        this.queryTable?.table?.redraw?.();
      },
    });

    // Bind splitter to table for centralized width mode clearing
    this.queryTable?.setSplitter(this.splitter);
  }

  _toggleLeftPanel() {
    this.isLeftPanelCollapsed = togglePanelCollapse({
      panel: this.elements.leftPanel,
      splitter: this.splitter,
      collapseBtn: this.elements.collapseBtn,
      panelWidth: this.leftPanelWidth,
      isCollapsed: this.isLeftPanelCollapsed,
      onAfterToggle: () => this.queryTable?.table?.redraw?.(),
    });

    // Save collapsed state
    this.leftPanelState.saveCollapsed(this.isLeftPanelCollapsed);
  }

  _restorePanelState() {
    // Re-read collapsed state from localStorage as a safety net
    this.isLeftPanelCollapsed = this.leftPanelState.loadCollapsed(this.isLeftPanelCollapsed);

    // Restore collapsed state directly via inline styles
    const panel = this.elements.leftPanel;
    if (panel && this.elements.collapseBtn && this.splitter) {
      if (this.isLeftPanelCollapsed) {
        panel.style.width = '0px';
        panel.style.minWidth = '0px';
        panel.style.maxWidth = '0px';
        panel.style.overflow = 'hidden';
        this.elements.collapseBtn.classList.add('collapsed');
        this.splitter.setCollapsed(true);
      } else {
        this.elements.collapseBtn.classList.remove('collapsed');
        this.splitter.setCollapsed(false);
      }
    }
  }

  // ============ FOOTER ============

  _setupFooter() {
    const slot = this.container.closest('.manager-slot');
    if (!slot) return;

    const footer = slot.querySelector('.manager-slot-footer');
    if (!footer) return;

    const group = footer.querySelector('.subpanel-header-group');
    if (!group) return;

    const placeholder = group.querySelector('.slot-footer-placeholder');

    const footerElements = setupManagerFooterIcons(group, {
      onPrint: () => this._handleFooterPrint(),
      onEmail: () => this._handleFooterEmail(),
      onExport: (e) => this._toggleFooterExportPopup(e),
      reportOptions: [
        { value: 'queries-view', label: 'Queries View' },
        { value: 'queries-data', label: 'Queries Data' },
      ],
      fillerTitle: 'Queries',
      anchor: placeholder,
      showSaveCancel: true,
    });

    this._footerDatasource = footerElements.reportSelect;

    // Wire save/cancel buttons through editHelper (standard pattern)
    this.editHelper.wireFooterButtons(
      footerElements.saveBtn,
      footerElements.cancelBtn,
      footerElements.dummyBtn,
    );
  }

  _getFooterDatasource() {
    return this._footerDatasource?.value || 'queries-view';
  }

  _handleFooterPrint() {
    const mode = this._getFooterDatasource();
    const table = this.queryTable?.table;

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

  _handleFooterEmail() {
    const mode = this._getFooterDatasource();
    const table = this.queryTable?.table;

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

    const subject = encodeURIComponent(`Queries — ${totalRows} rows (${modeLabel})`);
    const body = encodeURIComponent(
      `Queries Export (${modeLabel})\n` +
      `${totalRows} row(s)\n\n` +
      `${headers.join('\t')}\n${separator}\n` +
      `${dataLines.join('\n')}${truncated}\n`
    );

    window.open(`mailto:?subject=${subject}&body=${body}`, '_self');
  }

  _toggleFooterExportPopup(e) {
    e.stopPropagation();

    if (this._footerExportPopup) {
      this._closeFooterExportPopup();
      return;
    }

    const btn = e.currentTarget;
    const mode = this._getFooterDatasource();
    const formats = [
      { label: 'PDF', action: () => this._handleFooterExport('pdf', mode) },
      { label: 'CSV', action: () => this._handleFooterExport('csv', mode) },
      { label: 'TXT', action: () => this._handleFooterExport('txt', mode) },
      { label: 'XLS', action: () => this._handleFooterExport('xls', mode) },
    ];

    const popup = document.createElement('div');
    popup.className = 'queries-footer-export-popup';
    formats.forEach(item => {
      const row = document.createElement('button');
      row.type = 'button';
      row.className = 'queries-footer-export-popup-item';
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

  _handleFooterExport(format, mode) {
    const table = this.queryTable?.table;

    if (!table) {
      toast.info('No Data', { description: 'No table available to export', duration: 3000 });
      return;
    }

    const filename = `queries-export-${new Date().toISOString().slice(0, 10)}`;
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

  // ============ FONT POPUP ============

  _handleFontClick() {
    if (!this.fontPopup) {
      this._initFontPopup();
    }
    // Always reload saved settings when opening the popup
    this._loadCurrentFontIntoPopup();
    this.fontPopup?.classList.toggle('visible');
  }

  _initFontPopup() {
    const activeTabId = this._getActiveTabId();
    const savedSettings = this._loadFontSettings(activeTabId);

    const initialSettings = {
      fontSize: savedSettings?.fontSize || this.editorFontSize,
      fontFamily: savedSettings?.fontFamily || this.editorFontFamily,
      fontWeight: savedSettings?.fontWeight || this.editorFontWeight,
    };

    const { popup, toggle, getState, setState } = createFontPopup({
      anchor: this.elements.fontBtn,
      fontSize: initialSettings.fontSize,
      fontFamily: initialSettings.fontFamily,
      fontWeight: initialSettings.fontWeight,
      onPreview: (state) => this._updateFontPreview(state),
      onSave: (state) => this._saveAndApplyFontSettings(state),
      onCancel: () => this._cancelFontChanges(),
      onReset: () => this._resetFontToDefault(),
    });
    this.fontPopup = popup;
    this._fontPopupToggle = toggle;
    this._fontPopupGetState = getState;
    this._fontPopupSetState = setState;
  }

  _loadCurrentFontIntoPopup() {
    if (!this._fontPopupSetState) return;

    const activeTabId = this._getActiveTabId();
    const savedSettings = this._loadFontSettings(activeTabId);

    if (savedSettings) {
      this.editorFontSize = savedSettings.fontSize;
      this.editorFontFamily = savedSettings.fontFamily;
      this.editorFontWeight = savedSettings.fontWeight;
      this._fontPopupSetState(savedSettings);
    }
  }

  _updateFontPreview(_state) {
    // Preview is handled by the popup itself
  }

  _saveAndApplyFontSettings(state) {
    const activeTabId = this._getActiveTabId();

    if (this._fontResetPending) {
      this._fontResetPending = false;
      this.editorManager.clearFontOverride(activeTabId);
    } else {
      this.editorFontSize = state.fontSize;
      this.editorFontFamily = state.fontFamily;
      this.editorFontWeight = state.fontWeight;

      const fontSettings = {
        fontSize: this.editorFontSize,
        fontFamily: this.editorFontFamily,
        fontWeight: this.editorFontWeight,
      };

      this._saveFontSettings(activeTabId, fontSettings);
      this.editorManager.applyFontSettingsToActiveTab(fontSettings, activeTabId);
    }
  }

  _cancelFontChanges() {
    this._fontResetPending = false;
    const activeTabId = this._getActiveTabId();
    const savedSettings = this._loadFontSettings(activeTabId);
    if (savedSettings) {
      this.editorManager.applyFontSettingsToActiveTab(savedSettings, activeTabId);
    }
  }

  _resetFontToDefault() {
    const activeTabId = this._getActiveTabId();
    this._clearFontSettings(activeTabId);
    this._fontResetPending = true;
    return {
      fontSize: '14px',
      fontFamily: '"Vanadium Mono", var(--font-mono, monospace)',
      fontWeight: 'normal',
    };
  }

  _clearFontSettings(tabId) {
    try { localStorage.removeItem(`lithium_queries_font_${tabId}`); } catch { /* ignore */ }
  }

  _loadFontSettings(tabId) {
    try {
      const stored = localStorage.getItem(`lithium_queries_font_${tabId}`);
      return stored ? JSON.parse(stored) : null;
    } catch { return null; }
  }

  _saveFontSettings(tabId, settings) {
    try { localStorage.setItem(`lithium_queries_font_${tabId}`, JSON.stringify(settings)); } catch { /* ignore */ }
  }

  _restoreFontStateForTab(tabId) {
    const savedSettings = this._loadFontSettings(tabId);
    if (savedSettings) {
      this.editorFontSize = savedSettings.fontSize;
      this.editorFontFamily = savedSettings.fontFamily;
      this.editorFontWeight = savedSettings.fontWeight;
    }

    if (this._fontPopupSetState) {
      if (savedSettings) {
        this._fontPopupSetState(savedSettings);
      } else {
        this._fontPopupSetState({
          fontSize: this.editorFontSize,
          fontFamily: this.editorFontFamily,
          fontWeight: this.editorFontWeight,
        });
      }
    }
  }

  // ============ KEYBOARD SHORTCUTS ============

  _handleKeyboardShortcuts(e) {
    const isMac = navigator.platform.toUpperCase().indexOf('MAC') >= 0;
    const modifier = isMac ? e.metaKey : e.ctrlKey;

    if (!modifier) return;

    switch (e.key.toLowerCase()) {
      case 'z':
        if (e.shiftKey) {
          e.preventDefault();
          this.editorManager.handleRedo();
        } else {
          e.preventDefault();
          this.editorManager.handleUndo();
        }
        break;
      case 'y':
        e.preventDefault();
        this.editorManager.handleRedo();
        break;
      case 'p':
        e.preventDefault();
        this.editorManager.handlePrettify();
        break;
    }
  }

  // ============ LIFECYCLE ============

  onActivate() {
    log(Subsystems.MANAGER, Status.INFO, '[Queries] Activated');
    if (this.queryTable?.table) {
      this.queryTable.table.redraw?.();
    }
  }

  onDeactivate() {
    log(Subsystems.MANAGER, Status.INFO, '[Queries] Deactivated');
  }

  cleanup() {
    log(Subsystems.MANAGER, Status.INFO, '[Queries] Cleaning up...');

    // Clean up edit helper
    this.editHelper?.destroy();

    // Clean up font popup
    if (this.fontPopup) {
      this.fontPopup.remove();
      this.fontPopup = null;
    }

    // Close any open export popup
    this._closeFooterExportPopup();

    // Clean up splitter
    this.splitter?.destroy();
    this.splitter = null;

    // Clean up editor managers
    this.editorManager.destroy();
    this.sqlEditor = null;
    this.summaryEditor = null;
    this.collectionEditor = null;

    // Clean up LithiumTable
    this.queryTable?.destroy();
    this.queryTable = null;
  }
}
