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
import { createFontPopup, initToolbars, closeAllPopups } from '../../core/manager-ui.js';
import { ManagerEditHelper } from '../../core/manager-edit-helper.js';
import { AuditInfoFooter } from '../../core/audit-info-footer.js';

// Editor management (CodeMirror init, font, prettify, undo/redo)
import { createEditorManager } from './scripting-editors.js';
import { scriptingFooterMethods } from './scripting-footer.js';
import { createScriptInvoke } from './scripting-invoke.js';
import { scriptingSaveMethods } from './scripting-save.js';
import { clearScriptFont, isScriptFontEnabled, loadScriptFont, saveScriptFont } from './scripting-font.js';

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
    this.scriptInvoke = createScriptInvoke(this);

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
    if (!this._fontSettingsService()) {
      log(Subsystems.MANAGER, Status.WARN, '[Scripting] Font settings are unavailable');
    }
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
      prettifyBtn: this.container.querySelector('#scripting-prettify-btn'),
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
        this._rememberPersistedScripts(rows);
        log(Subsystems.TABLE, Status.INFO, `[Scripting] Loaded ${rows.length} scripts`);
      },
      onExecuteSave: (row, editHelper) => this._executeSave(row, editHelper),
      onDuplicate: (rowData) => this._executeDuplicate(rowData),
    });

    this.editHelper.registerTable(this.scriptTable);

    this._registerEditorWithEditHelper();

    await this.scriptTable.init();
    this._installScriptCrud();
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
    this.elements.prettifyBtn?.addEventListener('click', () => this.editorManager.handlePrettify());

    this._onKeyDown = (e) => this._handleKeyboardShortcuts(e);
    document.addEventListener('keydown', this._onKeyDown);
    this.scriptInvoke?.mount();
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
    if (this.elements.fontBtn) this.elements.fontBtn.disabled = !isScriptFontEnabled(tabId);
    if (this.elements.prettifyBtn) this.elements.prettifyBtn.disabled = tabId !== 'lua';

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
      await this.editorManager.renderSummaryPreview();
      requestAnimationFrame(() => this._applyFontToCurrentTab());
    }

    if (tabId === 'run') {
      await this.scriptInvoke?.show();
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
    this.scriptInvoke?.onRowChanged();

    if (!this._persistedScriptIds?.has(rowId)) {
      this._pendingLuaContent = rowData.code || '';
      await this.switchTab(this._getActiveTabId());
      return;
    }

    await this._loadScriptDetails(groupName, scriptName);
  }

  _handleRowDeselected() {
    this.currentScript = null;
    this._loadedDetailRowId = null;
    this._clearScriptDetails();
    this.auditFooter?.update(null);
    this.scriptInvoke?.onRowChanged();
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
        this.scriptInvoke?.sync();
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

  setupAuditFooter() {
    const slot = this.container.closest(".manager-slot");
    if (!slot) return;

    this.auditFooter = new AuditInfoFooter(this);
    this.auditFooter.init();

    if (this.scriptTable) this.scriptTable.auditFooter = this.auditFooter;

    log(Subsystems.MANAGER, Status.INFO, "[Scripting] Audit footer initialized");
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

  _fontSettingsService() {
    const settings = typeof window !== 'undefined' ? window.lithiumSettings : null;
    if (!settings || typeof settings.get !== 'function' || typeof settings.set !== 'function') {
      return null;
    }
    return settings;
  }

  _clearFontSettings(tabId) {
    clearScriptFont(this._fontSettingsService(), tabId, localStorage);
  }

  _loadFontSettings(tabId) {
    return loadScriptFont(this._fontSettingsService(), tabId, localStorage);
  }

  _saveFontSettings(tabId, settings) {
    const service = this._fontSettingsService();
    const saved = saveScriptFont(service, tabId, settings);
    if (!saved) {
      log(Subsystems.MANAGER, Status.ERROR, '[Scripting] Font settings were not saved');
      return;
    }
    // set() waits two seconds before the account-settings update.
    // A reload in that window fetches the previous server copy.
    if (typeof service.flush === 'function') {
      service.flush();
    }
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

    if (this._onKeyDown) {
      document.removeEventListener('keydown', this._onKeyDown);
      this._onKeyDown = null;
    }

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
    this.scriptInvoke?.destroy();

    this.scriptTable?.destroy();
    this.scriptTable = null;
  }
}

Object.assign(ScriptingManager.prototype, scriptingFooterMethods, scriptingSaveMethods);
