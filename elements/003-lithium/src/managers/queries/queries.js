/**
 * Queries Manager
 *
 * Query builder and execution interface.
 * Uses LithiumTable component for the query list table.
 */
   
// Import Styles
import '../../styles/vendor-tabulator.css';
import '../../core/manager-panels.css';
import './queries.css';

import { LithiumTable } from '../../core/lithium-table-main.js';
import { LithiumSplitter } from '../../core/lithium-splitter.js';
import { PanelStateManager } from '../../core/panel-state-manager.js';
import { togglePanelCollapse, restorePanelState } from '../../core/panel-collapse.js';
import { authQuery } from '../../shared/conduit.js';
import { toast } from '../../shared/toast.js';
import { log, Subsystems, Status } from '../../core/log.js';
import { processIcons } from '../../core/icons.js';
import { setupManagerFooterIcons, createFontPopup } from '../../core/manager-ui.js';

// Import modular components
import { createDirtyStateTracker } from './queries-dirty.js';
import { createEditModeManager } from './queries-edit-mode.js';
import { createEditorManager } from './queries-editors.js';

// Constants
const SELECTED_ROW_KEY = 'lithium_queries_selected_id';

/**
 * Queries Manager - Main class
 * Orchestrates the Query Manager functionality using LithiumTable
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
    
    // Initialize modular managers
    this.dirtyTracker = createDirtyStateTracker(this);
    this.editModeManager = createEditModeManager(this);
    this.editorManager = createEditorManager(this);
    
    // Font popup state
    this.fontPopup = null;
    this.editorFontSize = 14;
    this.editorFontFamily = 'var(--font-mono)';
    this.editorFontWeight = 'normal';
    
    // Expose editor references for backward compatibility
    this.sqlEditor = null;
    this.summaryEditor = null;
    this.collectionEditor = null;
    
    // Active editing table reference (for footer Save/Cancel)
    this.activeEditingTable = null;
  }

  // ============ LIFECYCLE ============

  async init() {
    await this._render();
    this._setupEventListeners();
    await this._initQueryTable();
    this._setupSplitters();
    this._setupFooter();
    this._restorePanelState();
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
      queryRef: 25, // QueryRef 25 - Query List
      searchQueryRef: 32, // QueryRef 32 - Query Search
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
      onEditModeChange: (isEditing, rowData) => this._handleTableEditModeChange(this.queryTable, isEditing, rowData),
    });

    await this.queryTable.init();
    await this.queryTable.loadData();
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

  // ============ TAB SWITCHING ============

  async switchTab(tabId) {
    this.elements.tabBtns.forEach(btn => btn.classList.toggle('active', btn.dataset.tab === tabId));
    this.elements.tabPanes.forEach(pane => pane.classList.toggle('active', pane.id === `queries-tab-${tabId}`));

    const isSqlTab = tabId === 'sql';
    if (this.elements.undoBtn) this.elements.undoBtn.disabled = !isSqlTab;
    if (this.elements.redoBtn) this.elements.redoBtn.disabled = !isSqlTab;
    if (this.elements.fontBtn) this.elements.fontBtn.disabled = !['sql', 'summary', 'preview'].includes(tabId);
    if (this.elements.prettifyBtn) this.elements.prettifyBtn.disabled = !isSqlTab;

    if (tabId === 'sql') {
      const sqlContent = this._pendingSqlContent || this.currentQuery?.code || this.currentQuery?.query_text || this.currentQuery?.sql || '';
      if (!this.sqlEditor) {
        await this.editorManager.initSqlEditor(sqlContent);
        this.sqlEditor = this.editorManager.sqlEditor;
      } else {
        const currentContent = this.sqlEditor.state.doc.toString();
        if (currentContent !== sqlContent) {
          this.sqlEditor.dispatch({ changes: { from: 0, to: this.sqlEditor.state.doc.length, insert: sqlContent } });
        }
      }
    }
    if (tabId === 'summary') {
      const summaryContent = this._pendingSummaryContent || this.currentQuery?.summary || this.currentQuery?.markdown || '';
      if (!this.summaryEditor) {
        await this.editorManager.initSummaryEditor(summaryContent);
        this.summaryEditor = this.editorManager.summaryEditor;
      } else {
        const currentContent = this.summaryEditor.state.doc.toString();
        if (currentContent !== summaryContent) {
          this.summaryEditor.dispatch({ changes: { from: 0, to: this.summaryEditor.state.doc.length, insert: summaryContent } });
        }
      }
    }
    if (tabId === 'collection') {
      let collectionContent = this._pendingCollectionContent || this.currentQuery?.collection || this.currentQuery?.json || {};
      if (typeof collectionContent === 'string') {
        try { collectionContent = JSON.parse(collectionContent); } catch { collectionContent = {}; }
      }
      if (this.collectionEditor) {
        // Update content directly via CodeMirror
        const jsonStr = JSON.stringify(collectionContent, null, 2);
        if (this.collectionEditor.state.doc.toString() !== jsonStr) {
          this.collectionEditor.dispatch({
            changes: { from: 0, to: this.collectionEditor.state.doc.length, insert: jsonStr }
          });
        }
      } else {
        await this.editorManager.initCollectionEditor(collectionContent);
        this.collectionEditor = this.editorManager.collectionEditor;
      }
    }
    if (tabId === 'preview') {
      this.editorManager.renderMarkdownPreview();
    }
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

    // Persist selection
    try { localStorage.setItem(SELECTED_ROW_KEY, String(queryId)); } catch {}

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

    if (this.sqlEditor) {
      this.sqlEditor.dispatch({ changes: { from: 0, to: this.sqlEditor.state.doc.length, insert: '' } });
    }
    if (this.summaryEditor) {
      this.summaryEditor.dispatch({ changes: { from: 0, to: this.summaryEditor.state.doc.length, insert: '' } });
    }
    if (this.collectionEditor) {
      // Clear JSON editor content
      this.collectionEditor.dispatch({
        changes: { from: 0, to: this.collectionEditor.state.doc.length, insert: '{}' }
      });
    }
  }

  // ============ ACTIVE TAB HELPER ============

  _getActiveTabId() {
    const activeBtn = this.container?.querySelector('.queries-tab-btn.active');
    return activeBtn?.dataset?.tab || 'sql';
  }

  // ============ TABLE EDIT MODE HANDLER ============

  _handleTableEditModeChange(lithiumTable, isEditing, rowData) {
    if (isEditing) {
      if (this.activeEditingTable && this.activeEditingTable !== lithiumTable) {
        this.activeEditingTable.exitEditMode('cancel');
      }
      this.activeEditingTable = lithiumTable;
      
      // Sync editModeManager state so isEditing() returns true
      // This is needed because the table's edit mode callback fires directly,
      // not through editModeManager.enterEditMode()
      this.editModeManager._isEditing = true;
      this.editModeManager._editingRowId = rowData?.query_id;
      
      // Enable CodeMirror editors when entering edit mode
      this.editorManager._setCodeMirrorEditable(true);
      
      // Buttons start DISABLED - only enable when dirty state is detected
      // (capturedOriginalData was called by editModeManager.enterEditMode)
      this.updateFooterSaveCancelState(true, false);
      
      // Sync table's isDirty with our dirty tracker state
      lithiumTable.isDirty = this.dirtyTracker.isAnyDirty();
      
      log(Subsystems.MANAGER, Status.INFO, '[Queries] Edit mode entered');
    } else {
      // Sync editModeManager state
      this.editModeManager._isEditing = false;
      this.editModeManager._editingRowId = null;
      if (this.activeEditingTable === lithiumTable) {
        this.activeEditingTable = null;
      }
      // Disable CodeMirror editors when exiting edit mode
      this.editorManager._setCodeMirrorEditable(false);
      this.updateFooterSaveCancelState(true, false);
      log(Subsystems.MANAGER, Status.INFO, '[Queries] Edit mode exited');
    }
  }

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
    // console.log('[Queries] _toggleLeftPanel called', { isCollapsed: this.isLeftPanelCollapsed });
    this.isLeftPanelCollapsed = togglePanelCollapse({
      panel: this.elements.leftPanel,
      splitter: this.splitter,
      collapseBtn: this.elements.collapseBtn,
      panelWidth: this.leftPanelWidth,
      isCollapsed: this.isLeftPanelCollapsed,
      onAfterToggle: () => this.queryTable?.table?.redraw?.(),
    });
    // console.log('[Queries] After toggle', { isCollapsed: this.isLeftPanelCollapsed });

    // Save collapsed state
    this.leftPanelState.saveCollapsed(this.isLeftPanelCollapsed);
    // console.log('[Queries] After saveCollapsed');
  }

  _restorePanelState() {
    // Re-read collapsed state from localStorage as a safety net
    // (handles cases where constructor loaded before persistence was ready)
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
    this.footerSaveBtn = footerElements.saveBtn;
    this.footerCancelBtn = footerElements.cancelBtn;
    this.footerDummyBtn = footerElements.dummyBtn;

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
    this.updateFooterSaveCancelState(true, false);
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
    this.fontPopup?.classList.toggle('visible');
  }

  _initFontPopup() {
    const { popup, getState } = createFontPopup({
      anchor: this.elements.fontBtn,
      fontSize: parseInt(this.editorFontSize, 10) || 14,
      fontFamily: this.editorFontFamily,
      fontWeight: this.editorFontWeight,
      onChange: () => this._applyFontSettings(),
    });
    this.fontPopup = popup;
    this._fontPopupGetState = getState;
  }

  _applyFontSettings() {
    if (this._fontPopupGetState) {
      const state = this._fontPopupGetState();
      this.editorFontSize = state.fontSize;
      this.editorFontFamily = state.fontFamily;
      this.editorFontWeight = state.fontWeight;
    }

    // Apply font settings to editors
    if (this.sqlEditor) {
      // CodeMirror theme customization would go here
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
