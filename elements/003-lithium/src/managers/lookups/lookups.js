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
import { togglePanelCollapse, restorePanelState } from '../../core/panel-collapse.js';
import '../../styles/vendor-tabulator.css';
import { authQuery } from '../../shared/conduit.js';
import { toast } from '../../shared/toast.js';
import { log, Subsystems, Status } from '../../core/log.js';
import { processIcons } from '../../core/icons.js';
import { setupManagerFooterIcons, createFontPopup } from '../../core/manager-ui.js';
import { ManagerEditHelper } from '../../core/manager-edit-helper.js';
import SunEditor from 'suneditor';
import 'suneditor/css/editor';
import { EditorState, EditorView, undo, redo } from '../../core/codemirror.js';
import {
  buildEditorExtensions,
  createReadOnlyCompartment,
  setEditorEditable,
  foldAllInEditor,
  unfoldAllInEditor,
  formatSortedJson,
  parseAndSortJson,
} from '../../core/codemirror-setup.js';
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

    // Edit helper — consolidates edit mode, dirty tracking, and save/cancel buttons
    this.editHelper = new ManagerEditHelper({ name: 'Lookups' });

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
    await this.initParentTable();
    await this.initChildTable();
    this.setupSplitters();
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
      undoBtn: this.container.querySelector('#lookups-undo-btn'),
      redoBtn: this.container.querySelector('#lookups-redo-btn'),
      foldAllBtn: this.container.querySelector('#lookups-fold-all-btn'),
      unfoldAllBtn: this.container.querySelector('#lookups-unfold-all-btn'),
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

    // Undo/Redo buttons
    this.elements.undoBtn?.addEventListener('click', () => {
      if (this.collectionEditor) undo(this.collectionEditor);
    });
    this.elements.redoBtn?.addEventListener('click', () => {
      if (this.collectionEditor) redo(this.collectionEditor);
    });

    // Fold/Unfold buttons
    this.elements.foldAllBtn?.addEventListener('click', () => {
      if (this.collectionEditor) foldAllInEditor(this.collectionEditor);
    });
    this.elements.unfoldAllBtn?.addEventListener('click', () => {
      if (this.collectionEditor) unfoldAllInEditor(this.collectionEditor);
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

      // Track dirty state when editor content changes (via onChange callback)
      this.sunEditor.onChange = () => {
        if (this.childTable?.isEditing) {
          this.editHelper.checkDirtyState();
        }
      };

      // Double-click to enter edit mode on child table
      this.elements.summaryEditor?.addEventListener('dblclick', () => {
        if (!this.childTable?.isEditing && this.childTable?.table) {
          const selected = this.childTable.table.getSelectedRows();
          if (selected.length > 0) void this.childTable.enterEditMode(selected[0]);
        }
      });
    } catch (error) {
      console.error('[LookupsManager] Failed to initialize SunEditor:', error);
    }
  }

  /**
   * Initialize CodeMirror JSON editor for JSON tab (Collection tab).
   * Uses the shared codemirror-setup.js for consistent configuration.
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

    const jsonStr = typeof jsonData === 'object' ? formatSortedJson(jsonData, 2) : jsonData;

    // If editor already exists, just update content
    if (this.collectionEditor) {
      this._setJsonEditorContent(jsonStr);
      return;
    }

    try {
      this._jsonReadOnlyCompartment = createReadOnlyCompartment();

      const extensions = buildEditorExtensions({
        language: 'json',
        readOnlyCompartment: this._jsonReadOnlyCompartment,
        readOnly: !this.childTable?.isEditing,
        fontSize: 13,
        onUpdate: (update) => {
          if (update.docChanged && this.childTable?.isEditing) {
            this.editHelper.checkDirtyState();
          }
        },
      });

      const state = EditorState.create({ doc: jsonStr, extensions });

      this.elements.jsonEditor.innerHTML = '';
      this.collectionEditor = new EditorView({
        state,
        parent: this.elements.jsonEditor,
      });

      // Store references on the container for compatibility
      this.elements.jsonEditor._cmView = this.collectionEditor;
      this.elements.jsonEditor._cmReadOnlyCompartment = this._jsonReadOnlyCompartment;

      // Set initial visual state
      const isEditing = this.childTable?.isEditing || false;
      setEditorEditable(this.collectionEditor, this._jsonReadOnlyCompartment, isEditing, this.elements.jsonEditor);

      // Double-click to enter edit mode on child table
      this.elements.jsonEditor?.addEventListener('dblclick', () => {
        if (!this.childTable?.isEditing && this.childTable?.table) {
          const selected = this.childTable.table.getSelectedRows();
          if (selected.length > 0) void this.childTable.enterEditMode(selected[0]);
        }
      });
    } catch (error) {
      console.error('[LookupsManager] Failed to initialize JSON editor:', error);
    }
  }

  /**
   * Helper to set JSON editor content via dispatch.
   * @param {string} jsonStr - JSON string to set
   */
  _setJsonEditorContent(jsonStr) {
    if (!this.collectionEditor) return;
    const current = this.collectionEditor.state.doc.toString();
    if (current !== jsonStr) {
      this.collectionEditor.dispatch({
        changes: { from: 0, to: this.collectionEditor.state.doc.length, insert: jsonStr },
      });
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
      updateQueryRef: 43, // QueryRef 43 - Update Lookup
      insertQueryRef: 42, // QueryRef 42 - Insert Lookup
      deleteQueryRef: 44, // QueryRef 44 - Delete Lookup
      cssPrefix: 'lithium', // Use default prefix for shared styling
      storageKey: 'lookups_parent_table',
      app: this.app,
      readonly: false,
      panel: this.elements.leftPanel,
      panelStateManager: this.leftPanelState,
      onRowSelected: (rowData) => this.handleParentRowSelected(rowData),
      onRowDeselected: () => this.handleParentRowDeselected(),
      onDataLoaded: (rows) => {
        log(Subsystems.TABLE, Status.INFO, `[Lookups] Loaded ${rows.length} lookups`);
      },
      // Custom save: maps aliased field names to QueryRef 43 named params
      onExecuteSave: (row) => this._executeParentSave(row),
    });

    // Register with editHelper — auto-wires onEditModeChange + onDirtyChange
    this.editHelper.registerTable(this.parentTable);

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
      updateQueryRef: 43, // QueryRef 43 - Update Lookup Value
      insertQueryRef: 42, // QueryRef 42 - Insert Lookup Value
      deleteQueryRef: 44, // QueryRef 44 - Delete Lookup Value
      cssPrefix: 'lithium', // Use default prefix for shared styling
      storageKey: 'lookups_child_table',
      app: this.app,
      readonly: false,
      panel: this.elements.middlePanel,
      panelStateManager: this.middlePanelState,
      onRowSelected: (rowData) => this.handleChildRowSelected(rowData),
      onRowDeselected: () => this.handleChildRowDeselected(),
      onDataLoaded: (rows) => {
        log(Subsystems.TABLE, Status.INFO, `[Lookups] Loaded ${rows.length} lookup values`);
      },
      // Custom save: assembles table row data + JSON/Summary editor content
      onExecuteSave: (row, editHelper) => this._executeChildSave(row, editHelper),
      // Custom duplicate: clones a lookup value with next available key_idx
      onDuplicate: (rowData) => this._executeChildDuplicate(rowData),
      // Custom refresh: re-query with the current LOOKUPID parameter
      onRefresh: () => {
        if (this.selectedLookupId != null) {
          this.loadChildData(this.selectedLookupId);
        }
      },
    });

    // Register with editHelper — auto-wires onEditModeChange + onDirtyChange
    this.editHelper.registerTable(this.childTable);

    // Register external editors bound to the child table
    this.editHelper.registerEditor('json', {
      getContent: () => this._getJsonEditorContent(),
      setContent: (content) => this._setJsonEditorContent(content),
      setEditable: (editable) => this.setEditorsEditable(editable),
      boundTable: this.childTable,
    });
    this.editHelper.registerEditor('summary', {
      getContent: () => this._getSummaryEditorContent(),
      setContent: (content) => { if (this.sunEditor) this.sunEditor.setContents(content); },
      setEditable: () => {}, // setEditable handled by 'json' editor registration above
      boundTable: this.childTable,
    });

    await this.childTable.init();
  }

  // ── Table Width Control ────────────────────────────────────────────────────
  // Width persistence is now handled centrally by LithiumTable.
  // The Width popup in the Navigator calls LithiumTable.setTableWidth() directly,
  // which saves the mode to localStorage and applies the width to the panel.
  // Splitter drag clears the width mode via LithiumSplitter._clearWidthModes().
  // Panel pixel width is saved by the onResizeEnd callbacks in setupSplitters().

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
      // Clear any current selection BEFORE getting the saved selection
      // This prevents loadData() from capturing the old lookup's selected row
      this.childTable.table?.deselectRow?.();

      // Override the child table's saved row selection with the per-lookup selection
      const savedChildId = this._loadChildSelection(lookupId);
      if (savedChildId != null) {
        this.childTable.saveSelectedRowId(savedChildId);
      } else {
        this.childTable.clearSavedRowSelection();
      }

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

    // Persist child selection keyed by current lookup
    this._saveChildSelection(rowData);

    // Fetch full detail using QueryRef 35
    await this.loadDetailData(rowData);
  }

  handleChildRowDeselected() {
    this.selectedLookupValue = null;
    this.clearDetailView();
  }

  // ── Save Logic ─────────────────────────────────────────────────────────────
  // Both tables share QueryRef 43 for updates, which expects explicit named
  // params matching the SQL placeholders (see LITHIUM-MGR-LOOKUPS.md).
  // The WHERE clause uses a composite key: (lookup_id, key_idx), and the
  // ORIG variants carry the pre-edit values so the correct row is targeted.

  /**
   * Build the params object for QueryRef 43 (update) from row data.
   * Shared by both parent and child saves.
   *
   * Uses valueOrFallback() to ensure 0 is treated as a valid value (not falsy).
   * The JS ?? operator treats 0 as nullish, which would lose valid zero values.
   *
   * @param {Object} rowData      - Current (possibly edited) row data
   * @param {Object} originalData - Row data captured when edit mode was entered
   * @param {Object} options      - { summary, collection } override strings
   * @returns {Object} { INTEGER, STRING } params matching QueryRef 43 placeholders
   */
  _buildLookupSaveParams(rowData, originalData, { summary = '', collection = '{}' } = {}) {
    // Helper: return first value that is not null/undefined (0 is valid)
    const valueOrFallback = (primary, secondary, fallback) => {
      if (primary != null) return primary;
      if (secondary != null) return secondary;
      return fallback;
    };

    return {
      INTEGER: {
        LOOKUPID:     valueOrFallback(rowData.lookup_id, originalData.lookup_id, 0),
        KEYIDX:       valueOrFallback(rowData.key_idx, originalData.key_idx, 0),
        VALUEINT:     valueOrFallback(rowData.value_int, originalData.value_int, 0),
        SORTSEQ:      valueOrFallback(rowData.sort_seq, originalData.sort_seq, 0),
        STATUSA1:     valueOrFallback(rowData.status_a1, originalData.status_a1, 1),
        USERID:       this.app?.user?.id ?? 0,
        ORIGLOOKUPID: valueOrFallback(originalData.lookup_id, rowData.lookup_id, 0),
        ORIGKEYIDX:   valueOrFallback(originalData.key_idx, rowData.key_idx, 0),
      },
      STRING: {
        VALUETXT:   rowData.value_txt ?? rowData.name ?? originalData.value_txt ?? '',
        SUMMARY:    summary,
        CODE:       rowData.code ?? originalData.code ?? '',
        COLLECTION: collection,
      },
    };
  }

  /**
   * Custom save for the parent table (lookups list).
   * Maps aliased field names from QueryRef 30 to QueryRef 43 named params.
   *
   * @param {Tabulator.Row} row - The Tabulator row being saved
   */
  async _executeParentSave(row) {
    const rowData = row.getData();
    const originalData = this.parentTable.originalRowData || rowData;
    const pkField = this.parentTable.primaryKeyField || 'lookup_id';
    const pkValue = rowData[pkField];
    // Note: pkValue of 0 is VALID - only null/undefined/empty string indicate insert
    const isInsert = pkValue == null || pkValue === '';
    const queryRef = isInsert
      ? (this.parentTable.queryRefs?.insertQueryRef ?? 42)
      : (this.parentTable.queryRefs?.updateQueryRef ?? 43);

    const params = this._buildLookupSaveParams(rowData, originalData, {
      summary:    rowData.summary ?? originalData.summary ?? '',
      collection: rowData.collection ?? originalData.collection ?? '{}',
    });

    const result = await authQuery(this.app.api, queryRef, params);

    if (isInsert && result?.[0]?.[pkField] != null) {
      row.update({ [pkField]: result[0][pkField] });
    }
  }

  /**
   * Custom save for the child table (lookup values).
   * Includes JSON/Summary editor content alongside table row data.
   *
   * @param {Tabulator.Row} row - The Tabulator row being saved
   */
  async _executeChildSave(row) {
    const rowData = row.getData();
    const originalData = this.childTable.originalRowData || rowData;
    const pkField = this.childTable.primaryKeyField || 'lookup_value_id';
    const pkValue = rowData[pkField];
    // Note: pkValue of 0 is VALID - only null/undefined/empty string indicate insert
    const isInsert = pkValue == null || pkValue === '';
    const queryRef = isInsert
      ? (this.childTable.queryRefs?.insertQueryRef ?? 42)
      : (this.childTable.queryRefs?.updateQueryRef ?? 43);

    // Gather content from external editors
    // Normalize JSON with sorted keys before saving to ensure consistency
    const rawJsonContent = this._getJsonEditorContent() || '{}';
    const jsonContent = parseAndSortJson(rawJsonContent, 2) || rawJsonContent;
    const summaryContent = this._getSummaryEditorContent() || '';

    const params = this._buildLookupSaveParams(rowData, originalData, {
      summary:    summaryContent,
      collection: jsonContent,
    });

    const result = await authQuery(this.app.api, queryRef, params);

    if (isInsert && result?.[0]?.[pkField] != null) {
      row.update({ [pkField]: result[0][pkField] });
    }
  }

  /**
   * Custom duplicate for the child table (lookup values).
   * Calls QueryRef 42 to insert a clone of the selected lookup value.
   * The server calculates the next key_idx using the CTE in the query.
   * After insertion, refreshes the table and selects the newly created row.
   *
   * @param {Object} rowData - The original row data being duplicated
   * @returns {null} Returns null to abort default duplicate behavior (handled internally)
   */
  async _executeChildDuplicate(rowData) {
    if (this.selectedLookupId == null) {
      throw new Error('No lookup selected');
    }

    // Fetch full detail data (summary, code, collection) via QueryRef 35
    // These fields aren't in the table row data, they're loaded separately
    let fullDetail = {};
    try {
      const detail = await authQuery(this.app.api, 35, {
        INTEGER: {
          LOOKUPID: this.selectedLookupId,
          KEYIDX: rowData.key_idx,
        },
      });
      if (detail && detail.length > 0) {
        fullDetail = detail[0];
      }
    } catch (error) {
      log(Subsystems.TABLE, Status.WARN, `[Lookups] Failed to load detail for clone: ${error.message}`);
      // Continue with empty values if detail fetch fails
    }

    // Calculate next sort_seq (max + 10)
    const currentRows = this.childTable?.table?.getData() || [];
    const maxSortSeq = currentRows.reduce((max, row) => {
      const sortSeq = row.sort_seq;
      return sortSeq != null && sortSeq > max ? sortSeq : max;
    }, 0);
    const nextSortSeq = maxSortSeq + 10;

    // Call QueryRef 42 to insert the cloned lookup value
    // The server handles calculating the next key_idx via the next_key_idx CTE
    const result = await authQuery(this.app.api, 42, {
      INTEGER: {
        LOOKUPID: this.selectedLookupId,
        VALUEINT: rowData.value_int ?? 0,
        SORTSEQ: nextSortSeq,
        STATUSLUA1: rowData.status_a1 ?? 1,
        USERID: this.app?.user?.id ?? 0,
      },
      STRING: {
        VALUETXT: rowData.value_txt ? `${rowData.value_txt} (Copy)` : '',
        SUMMARY: fullDetail.summary ?? '',
        CODE: fullDetail.code ?? '',
        COLLECTION: fullDetail.collection ?? '{}',
      },
    });

    // The insert returns the new key_idx
    const newKeyIdx = result?.[0]?.key_idx;
    if (newKeyIdx == null) {
      throw new Error('Clone operation did not return a new key_idx');
    }

    // Save the new key_idx so it gets selected after refresh
    this.childTable.saveSelectedRowId(newKeyIdx);

    // Refresh the table data to show the new row
    await this.loadChildData(this.selectedLookupId);

    // Return null to abort default duplicate behavior (we handled it)
    return null;
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
      const jsonStr = formatSortedJson(jsonData, 2);
      this._setJsonEditorContent(jsonStr);
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
      this._setJsonEditorContent('{}');
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
      tables: this.parentTable,
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
      tables: this.childTable,
      onResize: (width) => {
        this.middlePanelWidth = width;
      },
      onResizeEnd: (width) => {
        this.middlePanelState.saveWidth(width);
        this.parentTable?.table?.redraw?.();
        this.childTable?.table?.redraw?.();
      },
    });

    // Bind splitters to tables for centralized width mode clearing
    this.parentTable?.setSplitter(this.leftSplitter);
    this.childTable?.setSplitter(this.rightSplitter);
  }

  toggleLeftPanel() {
    this.isLeftPanelCollapsed = togglePanelCollapse({
      panel: this.elements.leftPanel,
      splitter: this.leftSplitter,
      collapseBtn: this.elements.collapseLeftBtn,
      panelWidth: this.leftPanelWidth,
      isCollapsed: this.isLeftPanelCollapsed,
      onAfterToggle: () => {
        this.parentTable?.table?.redraw?.();
        this.childTable?.table?.redraw?.();
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
        this.parentTable?.table?.redraw?.();
        this.childTable?.table?.redraw?.();
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

    this._footerDatasource = footerElements.reportSelect;

    // Wire save/cancel buttons to the editHelper (handles all state management)
    this.editHelper.wireFooterButtons(
      footerElements.saveBtn,
      footerElements.cancelBtn,
      footerElements.dummyBtn,
    );

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
  // Edit mode, dirty tracking, and save/cancel button management are now
  // handled by this.editHelper (ManagerEditHelper).

  /**
   * Get current JSON editor content as string (for editHelper snapshot comparison).
   */
  _getJsonEditorContent() {
    const jsonContainer = this.elements.jsonEditor;
    if (jsonContainer?._cmView) {
      return jsonContainer._cmView.state.doc.toString();
    }
    const fallback = this.elements.jsonEditor?.querySelector('#lookups-json-editor-fallback');
    if (fallback) return fallback.value || '';
    return '';
  }

  /**
   * Get current Summary editor content as string (for editHelper snapshot comparison).
   */
  _getSummaryEditorContent() {
    if (this.sunEditor) {
      return this.sunEditor.getContents() || '';
    }
    return '';
  }

  /**
   * Set JSON and Summary editors editable state.
   * Called by editHelper via the registered editor's setEditable callback.
   * @param {boolean} editable - Whether editors should be editable
   */
  setEditorsEditable(editable) {
    // Update JSON editor via shared utility
    if (this.collectionEditor && this._jsonReadOnlyCompartment) {
      setEditorEditable(this.collectionEditor, this._jsonReadOnlyCompartment, editable, this.elements.jsonEditor);
    }

    // Update SunEditor readonly state
    if (this.sunEditor) {
      this.sunEditor.disabled = !editable;
      if (editable) {
        this.sunEditor.enable();
      } else {
        this.sunEditor.disable();
      }
    }

    log(Subsystems.MANAGER, Status.INFO, `[Lookups] Editors set to ${editable ? 'editable' : 'readonly'}`);
  }

  // ── Per-Lookup Child Selection Persistence ──────────────────────────────────

  /**
   * Save the selected child row for the current lookup.
   * Stores per-lookup selections so cycling through parents restores the
   * last-selected child for each.
   * @param {Object} rowData - The selected child row data
   */
  _saveChildSelection(rowData) {
    if (this.selectedLookupId == null || !rowData) return;
    const pkField = this.childTable?.primaryKeyField || 'key_idx';
    const childId = rowData[pkField];
    if (childId == null) return;

    try {
      const storageKey = 'lithium_lookups_child_selections';
      const stored = localStorage.getItem(storageKey);
      const selections = stored ? JSON.parse(stored) : {};
      selections[String(this.selectedLookupId)] = String(childId);
      localStorage.setItem(storageKey, JSON.stringify(selections));
    } catch {
      // Ignore storage errors
    }
  }

  /**
   * Load the previously selected child row ID for a given lookup.
   * @param {number|string} lookupId - The lookup ID
   * @returns {string|null} - The saved child row ID, or null
   */
  _loadChildSelection(lookupId) {
    if (lookupId == null) return null;
    try {
      const storageKey = 'lithium_lookups_child_selections';
      const stored = localStorage.getItem(storageKey);
      if (!stored) return null;
      const selections = JSON.parse(stored);
      return selections[String(lookupId)] ?? null;
    } catch {
      return null;
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

    // Clean up edit helper
    this.editHelper?.destroy();

    // Destroy SunEditor
    if (this.sunEditor) {
      this.sunEditor.destroy();
      this.sunEditor = null;
    }

    // Destroy JSON editor (CodeMirror EditorView)
    if (this.collectionEditor) {
      this.collectionEditor.destroy();
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
