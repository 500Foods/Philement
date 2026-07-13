/**
 * Scripting Manager — Manager ID 33
 *
 * A two-panel interface for managing Lua scripts:
 * - Left panel: List of all scripts (scripts table)
 * - Right panel: Lua code editor
 *
 * Uses the reusable LithiumTable component.
 *
 * @module managers/scripting
 */

// Import Styles
import '../../styles/vendor-tabulator.css';
import '../../core/manager-panels.css';
import './scripting.css';

import { LithiumTable } from '../../tables/lithium-table-main.js';
import { LithiumSplitter } from '../../core/lithium-splitter.js';
import { PanelStateManager } from '../../core/panel-state-manager.js';
import { togglePanelCollapse } from '../../core/panel-collapse.js';
import { authQuery } from '../../shared/conduit.js';
import { toast } from '../../shared/toast.js';
import { log, Subsystems, Status } from '../../core/log.js';
import { processIcons } from '../../core/icons.js';
import { setupManagerFooterIcons, createFontPopup, initToolbars, positionPopup, closeAllPopups } from '../../core/manager-ui.js';
import { ManagerEditHelper } from '../../core/manager-edit-helper.js';
import { scrollbarManager } from '../../core/scrollbar-manager.js';
import { AuditInfoFooter } from '../../core/audit-info-footer.js';

// Editor management (CodeMirror init, font, prettify, undo/redo)
import { createEditorManager } from './scripting-editors.js';

/**
 * ScriptingManager - Main class
 * Follows the standard LithiumTable + ManagerEditHelper pattern.
 */
export default class ScriptingManager {
  constructor(app, container) {
    this.app = app;
    this.container = container;
    this.elements = {};

    // LithiumTable instance for script list
    this.scriptTable = null;

    // Manager ID
    this.managerId = 33;

    // Current script data
    this.currentScript = null;
    this._loadedDetailRowId = null;
    this._pendingLuaContent = null;

    // Audit info footer
    this.auditFooter = null;

    // Panel state persistence
    this.leftPanelState = new PanelStateManager('lithium_scripting_left');
    this.leftPanelWidth = this.leftPanelState.loadWidth(622);
    this.isLeftPanelCollapsed = this.leftPanelState.loadCollapsed(false);

    // Edit helper — consolidates edit mode, dirty tracking, and save/cancel buttons
    this.editHelper = new ManagerEditHelper({
      name: 'Scripts',
      onAfterEditModeChange: (isEditing) => this._onEditModeChanged(isEditing),
    });

    // Editor manager (CodeMirror init, font, undo/redo)
    this.editorManager = createEditorManager(this);

    // Font popup state
    this.fontPopup = null;
    this.editorFontSize = 14;
    this.editorFontFamily = 'var(--font-mono)';
    this.editorFontWeight = 'normal';
    this._fontResetPending = false;

    // Expose editor reference (set by editorManager after init)
    this.luaEditor = null;
  }

  // ============ LIFECYCLE ============

  async init() {
    await this._render();
    this._setupEventListeners();
    await this._initScriptTable();
    this._setupSplitters();
    this._setupFooter();
    this._restorePanelState();
    this._loadSavedFontSettings();
    this.setupAuditFooter();
  }

  _loadSavedFontSettings() {
    const savedSettings = this._loadFontSettings('lua');
    if (savedSettings) {
      this.editorFontSize = savedSettings.fontSize;
      this.editorFontFamily = savedSettings.fontFamily;
      this.editorFontWeight = savedSettings.fontWeight;
    }
  }

  async _render() {
    try {
      const response = await fetch('/src/managers/scripting/scripting.html');
      const html = await response.text();
      this.container.innerHTML = html;
    } catch (error) {
      console.error('[ScriptingManager] Failed to load template:', error);
      this.container.innerHTML = '<div class="error">Failed to load Scripting Manager</div>';
      return;
    }

    this.elements = {
      container: this.container.querySelector('.scripting-manager-container'),
      leftPanel: this.container.querySelector('.scripting-left-panel'),
      tableContainer: this.container.querySelector('#scripting-table-container'),
      navigatorContainer: this.container.querySelector('#scripting-navigator-container'),
      splitter: this.container.querySelector('#scripting-splitter'),
      rightPanel: this.container.querySelector('.scripting-right-panel'),
      tabBtns: this.container.querySelectorAll('[data-tab]'),
      tabPanes: this.container.querySelectorAll('.scripting-tab-pane'),
      collapseBtn: this.container.querySelector('#scripting-collapse-btn'),
      luaEditorContainer: this.container.querySelector('#scripting-lua-editor'),
      previewContainer: this.container.querySelector('#scripting-tab-preview'),
      undoBtn: this.container.querySelector('#scripting-undo-btn'),
      redoBtn: this.container.querySelector('#scripting-redo-btn'),
      foldAllBtn: this.container.querySelector('#scripting-fold-all-btn'),
      unfoldAllBtn: this.container.querySelector('#scripting-unfold-all-btn'),
      fontBtn: this.container.querySelector('#scripting-font-btn'),
      tabsHeader: this.container.querySelector('.scripting-tabs-header'),
    };

    processIcons(this.container);
    initToolbars();
  }

  // ============ SCRIPT TABLE INITIALIZATION ============

  async _initScriptTable() {
    if (!this.elements.tableContainer || !this.elements.navigatorContainer) return;

    this.scriptTable = new LithiumTable({
      container: this.elements.tableContainer,
      navigatorContainer: this.elements.navigatorContainer,
      tablePath: 'scripts/script-manager',
      lookupKeyIdx: null,
      primaryKeyField: ['group_name', 'script_name'],
      queryRef: 89,
      searchQueryRef: 90,
      cssPrefix: 'scripting',
      storageKey: 'scripting_table',
      app: this.app,
      readonly: false,
      tableWidthMode: 'wide',
      panel: this.elements.leftPanel,
      panelStateManager: this.leftPanelState,
      onRowSelected: (rowData) => this._handleRowSelected(rowData),
      onRowDeselected: () => this._handleRowDeselected(),
      onDataLoaded: (rows) => {
        log(Subsystems.TABLE, Status.INFO, `[Scripting] Loaded ${rows.length} scripts`);
      },
      onExecuteSave: (row, editHelper) => this._executeSave(row, editHelper),
      onDuplicate: (rowData) => this._executeDuplicate(rowData),
    });

    this.editHelper.registerTable(this.scriptTable);

    this._registerEditorWithEditHelper();

    await this.scriptTable.init();
    await this.scriptTable.loadData();
  }

  _registerEditorWithEditHelper() {
    this.editHelper.registerEditor('lua', {
      getContent: () => this.luaEditor?.state?.doc?.toString() || '',
      setContent: (content) => {
        this.editorManager._setContentNoHistory(this.luaEditor, content);
      },
      setEditable: (editable) => this.editorManager?._setCodeMirrorEditable(editable),
      boundTable: this.scriptTable,
    });
  }

  // ============ CUSTOM SAVE LOGIC ============

  async _executeSave(row) {
    const rowData = row.getData();
    const groupName = rowData.group_name || '';
    const scriptName = rowData.script_name || '';
    const isInsert = !this._loadedDetailRowId || this._loadedDetailRowId !== `${groupName}::${scriptName}`;

    const queryRef = isInsert
      ? (this.scriptTable.queryRefs?.insertQueryRef ?? 129)
      : (this.scriptTable.queryRefs?.updateQueryRef ?? 130);

    if (!queryRef) {
      throw Object.assign(new Error('No queryRef configured for save'), {
        serverError: 'Save not available: no query reference configured.',
      });
    }

    const luaContent = this.luaEditor?.state?.doc?.toString() || '';

    const params = {
      STRING: {
        GROUP_NAME: groupName,
        SCRIPT_NAME: scriptName,
        SCRIPT_TYPE: rowData.script_type ?? 1,
        SCHEDULE: rowData.schedule || null,
        NEXT_RUN: rowData.next_run || null,
        LAST_RUN_START: rowData.last_run_start || null,
        LAST_RUN_END: rowData.last_run_end || null,
        STATUS: rowData.status ?? 1,
        CODE: luaContent || rowData.code || '',
        SUMMARY: rowData.summary || '',
      },
      INTEGER: {
        USERID: this.app?.user?.id ?? 0,
      },
    };

    const result = await authQuery(this.app.api, queryRef, params);

    if (isInsert) {
      this._loadedDetailRowId = `${groupName}::${scriptName}`;
      this.scriptTable.saveSelectedRowId(`${groupName}::${scriptName}`);
    }
  }

  async _executeDuplicate(rowData) {
    const groupName = rowData.group_name || '';
    const scriptName = rowData.script_name || '';
    if (!groupName || !scriptName) {
      throw new Error('Missing group_name or script_name for duplicate');
    }

    const insertQueryRef = this.scriptTable.queryRefs?.insertQueryRef ?? 129;
    const newScriptName = `${scriptName} (Copy)`;

    const result = await authQuery(this.app.api, insertQueryRef, {
      STRING: {
        GROUP_NAME: groupName,
        SCRIPT_NAME: newScriptName,
        SCRIPT_TYPE: rowData.script_type ?? 1,
        SCHEDULE: rowData.schedule || null,
        NEXT_RUN: null,
        LAST_RUN_START: null,
        LAST_RUN_END: null,
        STATUS: rowData.status ?? 1,
        CODE: rowData.code || '',
        SUMMARY: rowData.summary || '',
      },
      INTEGER: {
        USERID: this.app?.user?.id ?? 0,
      },
    });

    const newRowId = `${groupName}::${newScriptName}`;
    this._loadedDetailRowId = newRowId;
    this.scriptTable.saveSelectedRowId(newRowId);

    await this.scriptTable.loadData();
    return null;
  }

  // ============ EVENT HANDLERS ============

  _setupEventListeners() {
    this.elements.tabBtns?.forEach(btn => {
      btn.addEventListener('click', () => this.switchTab(btn.dataset.tab));
    });

    this.elements.collapseBtn?.addEventListener('click', () => this._toggleLeftPanel());

    this.elements.undoBtn?.addEventListener('click', () => this.editorManager.handleUndo());
    this.elements.redoBtn?.addEventListener('click', () => this.editorManager.handleRedo());
    this.elements.foldAllBtn?.addEventListener('click', () => this.editorManager.handleFoldAll());
    this.elements.unfoldAllBtn?.addEventListener('click', () => this.editorManager.handleUnfoldAll());
    this.elements.fontBtn?.addEventListener('click', () => this._handleFontClick());

    document.addEventListener('keydown', (e) => this._handleKeyboardShortcuts(e));
  }

  // ============ TAB SWITCHING ============

  async switchTab(tabId) {
    closeAllPopups();
    const currentActiveTab = this._getActiveTabId();
    const isAlreadyActive = currentActiveTab === tabId;

    if (!isAlreadyActive) {
      this.elements.tabBtns.forEach(btn => btn.classList.toggle('active', btn.dataset.tab === tabId));
      this.elements.tabPanes.forEach(pane => pane.classList.toggle('active', pane.id === `scripting-tab-${tabId}`));
    }

    const hasEditor = tabId === 'lua';
    if (this.elements.fontBtn) this.elements.fontBtn.disabled = !hasEditor;

    if (!hasEditor) {
      if (this.elements.undoBtn) this.elements.undoBtn.disabled = true;
      if (this.elements.redoBtn) this.elements.redoBtn.disabled = true;
    }

    this._restoreFontStateForTab(tabId);

    if (tabId === 'lua') {
      const luaContent = this._pendingLuaContent || this.currentScript?.code || '';
      if (!this.luaEditor) {
        await this.editorManager.initLuaEditor(luaContent);
        this.luaEditor = this.editorManager.luaEditor;
      } else {
        const currentContent = this.luaEditor.state.doc.toString();
        if (currentContent !== luaContent) {
          this.editorManager._setContentNoHistory(this.luaEditor, luaContent);
        }
      }
      requestAnimationFrame(() => this._applyFontToCurrentTab());
      this.editorManager._updateUndoRedoButtons();
    }

    if (tabId === 'preview') {
      this.editorManager.renderLuaPreview();
      requestAnimationFrame(() => this._applyFontToCurrentTab());
    }
  }

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

    const groupName = rowData.group_name || '';
    const scriptName = rowData.script_name || '';
    const rowId = `${groupName}::${scriptName}`;

    this.auditFooter?.update(rowData);
    if (!groupName || !scriptName) return;

    if (this._loadedDetailRowId === rowId) return;

    log(Subsystems.MANAGER, Status.INFO, `[Scripting] Row selected: ${groupName}/${scriptName}`);

    this.currentScript = rowData;
    this._loadedDetailRowId = rowId;

    await this._loadScriptDetails(groupName, scriptName);
  }

  _handleRowDeselected() {
    this.currentScript = null;
    this._loadedDetailRowId = null;
    this._clearScriptDetails();
    this.auditFooter?.update(null);
  }

  async _loadScriptDetails(groupName, scriptName) {
    if (!this.app?.api) return;

    try {
      const detailQueryRef = this.scriptTable.queryRefs?.detailQueryRef ?? 87;
      const details = await authQuery(this.app.api, detailQueryRef, {
        STRING: { GROUP_NAME: groupName, SCRIPT_NAME: scriptName },
      });

      if (details && details.length > 0) {
        this.currentScript = { ...this.currentScript, ...details[0] };
        this._pendingLuaContent = this.currentScript.code || '';

        const activeTab = this._getActiveTabId();
        this.switchTab(activeTab);
      }
    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, `[Scripting] Failed to load script details: ${error.message}`);
      toast.error('Failed to load script details');
    }
  }

  _clearScriptDetails() {
    this.currentScript = null;
    this._loadedDetailRowId = null;
    this._pendingLuaContent = null;

    if (this.luaEditor) {
      this.editorManager._setContentNoHistory(this.luaEditor, '');
    }
  }

  // ============ ACTIVE TAB HELPER ============

  _getActiveTabId() {
    const activeBtn = this.container?.querySelector('.lithium-toolbar-tab.active');
    return activeBtn?.dataset?.tab || 'lua';
  }

  // ============ SPLITTER ============

  _setupSplitters() {
    this.splitter = new LithiumSplitter({
      element: this.elements.splitter,
      leftPanel: this.elements.leftPanel,
      minWidth: 157,
      maxWidth: 1000,
      tables: this.scriptTable,
      onResize: (width) => {
        this.leftPanelWidth = width;
      },
      onResizeEnd: (width) => {
        this.leftPanelState.saveWidth(width);
        this.scriptTable?.table?.redraw?.();
      },
    });

    this.scriptTable?.setSplitter(this.splitter);
  }

  _toggleLeftPanel() {
    this.isLeftPanelCollapsed = togglePanelCollapse({
      panel: this.elements.leftPanel,
      splitter: this.splitter,
      collapseBtn: this.elements.collapseBtn,
      panelWidth: this.leftPanelWidth,
      isCollapsed: this.isLeftPanelCollapsed,
      onAfterToggle: () => this.scriptTable?.table?.redraw?.(),
    });

    this.leftPanelState.saveCollapsed(this.isLeftPanelCollapsed);
  }

  _restorePanelState() {
    this.isLeftPanelCollapsed = this.leftPanelState.loadCollapsed(this.isLeftPanelCollapsed);

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
      onDownload: () => this._handleFooterDownload(),
      onExport: (value, label) => this._handleFooterExport(value, label),
      onClipboard: (value, label) => this._handleFooterClipboard(value, label),
      reportOptions: [
        { value: 'scripts-view', label: 'Scripts View' },
        { value: 'scripts-data', label: 'Scripts Data' },
      ],
      fillerTitle: 'Scripts',
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

    this.editHelper.wireFooterButtons(
      footerElements.saveBtn,
      footerElements.cancelBtn,
      footerElements.dummyBtn,
    );
  }

  setupAuditFooter() {
    const slot = this.container.closest(".manager-slot");
    if (!slot) return;

    this.auditFooter = new AuditInfoFooter(this);
    this.auditFooter.init();

    if (this.scriptTable) this.scriptTable.auditFooter = this.auditFooter;

    log(Subsystems.MANAGER, Status.INFO, "[Scripting] Audit footer initialized");
  }

  _getFooterDatasource() {
    return this._footerDatasource?.value || 'scripts-view';
  }

  _handleFooterPrint() {
    const mode = this._getFooterDatasource();
    const table = this.scriptTable?.table;

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
    const table = this.scriptTable?.table;

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

    const subject = encodeURIComponent(`Scripts — ${totalRows} rows (${modeLabel})`);
    const body = encodeURIComponent(
      `Scripts Export (${modeLabel})\n` +
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

    closeAllPopups();

    const btn = e.currentTarget;
    this._footerExportButton = btn;
    btn.classList.add('popup-active');

    const mode = this._getFooterDatasource();
    const formats = [
      { label: 'PDF', action: () => this._handleFooterExport('pdf', mode) },
      { label: 'CSV', action: () => this._handleFooterExport('csv', mode) },
      { label: 'TXT', action: () => this._handleFooterExport('txt', mode) },
      { label: 'XLS', action: () => this._handleFooterExport('xls', mode) },
    ];

    const popup = document.createElement('div');
    popup.className = 'manager-footer-export-popup manager-footer-popup';
    formats.forEach(item => {
      const row = document.createElement('button');
      row.type = 'button';
      row.className = 'manager-footer-export-popup-item';
      row.textContent = item.label;
      row.addEventListener('click', () => {
        this._closeFooterExportPopup();
        item.action();
      });
      popup.appendChild(row);
    });

    document.body.appendChild(popup);
    positionPopup(popup, btn, 'footer-riseup');

    setTimeout(() => popup.classList.add('visible'), 10);

    this._footerExportPopup = popup;

    this._footerExportCloseHandler = (evt) => {
      if (!popup.contains(evt.target) && !btn.contains(evt.target)) {
        this._closeFooterExportPopup();
      }
    };
    document.addEventListener('click', this._footerExportCloseHandler);

    this._footerExportGlobalCloseHandler = () => {
      this._closeFooterExportPopup();
    };
    document.addEventListener('close-all-popups', this._footerExportGlobalCloseHandler);
  }

  _closeFooterExportPopup() {
    if (this._footerExportButton) {
      this._footerExportButton.classList.remove('popup-active');
      this._footerExportButton = null;
    }
    if (this._footerExportPopup) {
      const popup = this._footerExportPopup;
      this._footerExportPopup = null;
      popup.classList.remove('visible');
      const duration = parseFloat(getComputedStyle(popup).transitionDuration) * 1000 || 350;
      setTimeout(() => popup.remove(), duration);
    }
    if (this._footerExportCloseHandler) {
      document.removeEventListener('click', this._footerExportCloseHandler);
      this._footerExportCloseHandler = null;
    }
    if (this._footerExportGlobalCloseHandler) {
      document.removeEventListener('close-all-popups', this._footerExportGlobalCloseHandler);
      this._footerExportGlobalCloseHandler = null;
    }
  }

  _handleFooterExport(value, label) {
    const mode = this._getFooterDatasource();
    const table = this.scriptTable?.table;

    if (!table) {
      toast.info('No Data', { description: 'No table available to export', duration: 3000 });
      return;
    }

    const filename = `scripts-export-${new Date().toISOString().slice(0, 10)}`;
    const isViewMode = mode.endsWith('-view');
    const downloadOpts = isViewMode ? {} : { rowGroups: false };

    this._doDownload(table, value, filename, downloadOpts);
  }

  _handleFooterDownload() {
    const mode = this._getFooterDatasource();
    const table = this.scriptTable?.table;

    if (!table) {
      toast.info('No Data', { description: 'No table available to download', duration: 3000 });
      return;
    }

    const filename = `scripts-export-${new Date().toISOString().slice(0, 10)}`;
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
        this._handleMarkdownExport(table, this._getFooterDatasource(), filename);
        break;
      default:
        log(Subsystems.MANAGER, Status.WARN, `[Scripting] Unknown export format: ${format}`);
    }
  }

  _handleFooterClipboard(value, label) {
    const mode = this._getFooterDatasource();
    const table = this.scriptTable?.table;

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

  _handleMarkdownExport(table, mode, filename) {
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
      return visibleCols.map(col => {
        const val = data[col.getField()];
        return val != null ? String(val).replace(/\|/g, '\\|') : '';
      }).join(' | ');
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

  // ============ FONT POPUP ============

  _handleFontClick() {
    if (!this.fontPopup) {
      this._initFontPopup();
    }
    this._loadCurrentFontIntoPopup();
    this._fontPopupToggle?.();
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
    try { localStorage.removeItem(`lithium_scripting_font_${tabId}`); } catch { /* ignore */ }
  }

  _loadFontSettings(tabId) {
    try {
      const stored = localStorage.getItem(`lithium_scripting_font_${tabId}`);
      return stored ? JSON.parse(stored) : null;
    } catch { return null; }
  }

  _saveFontSettings(tabId, settings) {
    try { localStorage.setItem(`lithium_scripting_font_${tabId}`, JSON.stringify(settings)); } catch { /* ignore */ }
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

  // ============ EDIT MODE ============

  _onEditModeChanged(_isEditing) {
    this.editorManager._updateUndoRedoButtons();
  }

  // ============ LIFECYCLE ============

  onActivate() {
    log(Subsystems.MANAGER, Status.INFO, '[Scripting] Activated');
    if (this.scriptTable?.table) {
      this.scriptTable.table.redraw?.();
    }
  }

  onDeactivate() {
    log(Subsystems.MANAGER, Status.INFO, '[Scripting] Deactivated');
  }

  cleanup() {
    log(Subsystems.MANAGER, Status.INFO, '[Scripting] Cleaning up...');

    this.editHelper?.destroy();

    if (this.fontPopup) {
      this.fontPopup.remove();
      this.fontPopup = null;
    }

    this._closeFooterExportPopup();

    this.splitter?.destroy();
    if (this.auditFooter) {
      this.auditFooter.destroy();
      this.auditFooter = null;
    }
    this.splitter = null;

    this.editorManager.destroy();
    this.luaEditor = null;

    this.scriptTable?.destroy();
    this.scriptTable = null;
  }
}
