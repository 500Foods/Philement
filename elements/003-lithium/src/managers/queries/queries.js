/**
 * Queries Manager
 *
 * Query builder and execution interface.
 */

import { TabulatorFull as Tabulator } from 'tabulator-tables';
import '../../styles/vendor-tabulator.css';
import '../../styles/vendor-vanilla-jsoneditor.css';
import { authQuery } from '../../shared/conduit.js';
import { toast } from '../../shared/toast.js';
import { log, Subsystems, Status } from '../../core/log.js';
import { processIcons } from '../../core/icons.js';
import {
  loadColtypes,
  loadTableDef,
  resolveColumns,
  resolveTableOptions,
  getQueryRefs,
  getPrimaryKeyField,
  preloadLookups,
} from '../../core/lithium-table.js';
import './queries.css';

// ── Tabulator schema loading ────────────────────────────────────────────────
// Schemas are now loaded from the lookup cache (QueryRef 060) or fetched
// from /config/tabulator/ as a fallback. The lookup data is loaded during
// app initialization in lookups.js.

// Font constants
const MIN_FONT_SIZE = 10;
const MAX_FONT_SIZE = 24;
const DEFAULT_FONT_SIZE = 14;
const FONT_SIZE_STEP = 2;

// Default font settings
const DEFAULT_FONT_FAMILY = '"Vanadium Mono", var(--font-mono, monospace)';

// localStorage key for persisting the selected row across sessions
const SELECTED_ROW_KEY = 'lithium_queries_selected_id';

// localStorage key for persisting the left panel width across sessions
const PANEL_WIDTH_KEY = 'lithium_queries_panel_width';

// localStorage key for persisting the Tabulator layout mode across sessions
const LAYOUT_MODE_KEY = 'lithium_queries_layout_mode';

// localStorage keys for template persistence
const TEMPLATES_KEY = 'lithium_queries_templates';
const DEFAULT_TEMPLATE_KEY = 'lithium_queries_default_template';

// History field for CodeMirror undo/redo state
let historyField;

export default class QueriesManager {
  constructor(app, container) {
    this.app = app;
    this.container = container;
    this.elements = {};
    this.isResizing = false;
    this.table = null;
    this._arrowRotation = 0;
    this._editingRowId = null;
    this._isEditing = false;
    this._columnChooserPopup = null;
    this._filtersVisible = false;
    this._tableWidthMode = 'compact'; // Default matches ~314px panel width
    this._tableLayoutMode = 'fitColumns'; // Tabulator layout mode (persisted)
    this._editTransitionPending = false;
    this._loadedDetailRowId = null;
    this._pendingCellEditToken = 0;

    // JSON-driven table configuration (loaded in initTable)
    this.tableDef = null;
    this.coltypes = null;
    this.queryRefs = null;
    this.primaryKeyField = null;

    // CodeMirror editor instance
    this.sqlEditor = null;
    this.summaryEditor = null;
    this.currentQuery = null;

    // JSONEditor instance for collection tab
    this.collectionEditor = null;

    // Font settings state - Vanadium Mono is our preferred font
    this.fontSettings = {
      size: DEFAULT_FONT_SIZE,
      family: '"Vanadium Mono", var(--font-mono, monospace)',
      bold: false,
      italic: false,
    };

    // Font popup element
    this.fontPopup = null;

    // SQL dialect (sqlite is our default)
    this.sqlDialect = 'sqlite';

    // Original data from server for change tracking and revert
    this._originalRowData = null;          // Original Tabulator row data
    this._originalSqlContent = null;       // Original SQL content
    this._originalSummaryContent = null;   // Original summary/markdown content
    this._originalCollectionContent = null; // Original collection/JSON content

    // Dirty state flags for tracking changes
    this._isDirty = {
      table: false,      // Tabulator row has been edited
      sql: false,        // SQL editor content changed
      summary: false,    // Summary editor content changed
      collection: false, // Collection editor content changed
    };
  }

  /**
   * Check if any content has been modified (dirty state)
   * @returns {boolean} true if any changes exist
   */
  _isAnyDirty() {
    return this._isDirty.table ||
           this._isDirty.sql ||
           this._isDirty.summary ||
           this._isDirty.collection;
  }

  /**
   * Update the dirty state and manage button states accordingly
   * @param {string} type - which type changed ('table', 'sql', 'summary', 'collection')
   * @param {boolean} isDirty - whether this type is now dirty
   */
  _setDirty(type, isDirty) {
    const wasDirty = this._isAnyDirty();
    this._isDirty[type] = isDirty;
    const nowDirty = this._isAnyDirty();

    // Only update button state if dirty state changed
    if (wasDirty !== nowDirty) {
      this._updateSaveCancelButtonState();
    }
  }

  /**
   * Update Save/Cancel button states based on edit mode AND dirty state
   * Save/Cancel should only be enabled when in edit mode AND there are changes
   */
  _updateSaveCancelButtonState() {
    const nav = this.elements.navigatorContainer;
    if (!nav) return;

    const saveBtn = nav.querySelector('#queries-nav-save');
    const cancelBtn = nav.querySelector('#queries-nav-cancel');

    // Buttons are enabled only when in edit mode AND there are changes
    const shouldEnable = this._isEditing && this._isAnyDirty();

    if (saveBtn) saveBtn.disabled = !shouldEnable;
    if (cancelBtn) cancelBtn.disabled = !shouldEnable;
  }

  /**
   * Mark all content as clean (after save or when loading new data)
   */
  _markAllClean() {
    this._isDirty.table = false;
    this._isDirty.sql = false;
    this._isDirty.summary = false;
    this._isDirty.collection = false;
    this._updateSaveCancelButtonState();
  }

  /**
   * Store original data from server for change tracking
   * Called when loading query details
   */
  _captureOriginalData(queryData) {
    if (!queryData) return;

    // Capture original row data (shallow copy of relevant fields)
    this._originalRowData = { ...queryData };

    // Capture original external content
    this._originalSqlContent = queryData.code || queryData.query_text || queryData.sql || '';
    this._originalSummaryContent = queryData.summary || queryData.markdown || '';
    const collection = queryData.collection || queryData.json || {};
    this._originalCollectionContent = typeof collection === 'string'
      ? collection
      : JSON.stringify(collection);

    // Reset dirty state since we just loaded fresh data
    this._markAllClean();
  }

  /**
   * Revert all changes to original values
   * - Tabulator: use undo()
   * - External content: restore from cached originals
   */
  async _revertAllChanges() {
    // Revert Tabulator changes
    if (this._isDirty.table && this.table) {
      // Get all edited cells and undo them
      const editedCells = typeof this.table.getEditedCells === 'function'
        ? this.table.getEditedCells()
        : [];
      if (editedCells.length > 0) {
        this.table.undo();
      }
    }

    // Revert SQL content
    if (this._isDirty.sql && this.sqlEditor && this._originalSqlContent != null) {
      this.sqlEditor.dispatch({
        changes: { from: 0, to: this.sqlEditor.state.doc.length, insert: this._originalSqlContent },
      });
    }

    // Revert Summary content
    if (this._isDirty.summary && this.summaryEditor && this._originalSummaryContent != null) {
      this.summaryEditor.dispatch({
        changes: { from: 0, to: this.summaryEditor.state.doc.length, insert: this._originalSummaryContent },
      });
    }

    // Revert Collection content
    if (this._isDirty.collection && this.collectionEditor && this._originalCollectionContent != null) {
      try {
        const jsonData = typeof this._originalCollectionContent === 'string'
          ? JSON.parse(this._originalCollectionContent)
          : this._originalCollectionContent;
        this.collectionEditor.set({ json: jsonData });
      } catch (e) {
        // If parse fails, set as empty object
        this.collectionEditor.set({ json: {} });
      }
    }

    // Mark all as clean
    this._markAllClean();
  }

  /**
   * Get current SQL content from editor
   */
  _getCurrentSqlContent() {
    if (!this.sqlEditor) return '';
    return this.sqlEditor.state.doc.toString();
  }

  /**
   * Get current Summary content from editor
   */
  _getCurrentSummaryContent() {
    if (!this.summaryEditor) return '';
    return this.summaryEditor.state.doc.toString();
  }

  /**
   * Get current Collection content from editor
   */
  _getCurrentCollectionContent() {
    if (!this.collectionEditor) return '';
    // Get content from vanilla-jsoneditor
    const content = this.collectionEditor.get();
    if (content && content.json !== undefined) {
      return JSON.stringify(content.json);
    }
    return JSON.stringify({});
  }

  /**
   * Check if SQL content has changed from original
   */
  _checkSqlDirty() {
    if (this._originalSqlContent == null) return false;
    const current = this._getCurrentSqlContent();
    return current !== this._originalSqlContent;
  }

  /**
   * Check if Summary content has changed from original
   */
  _checkSummaryDirty() {
    if (this._originalSummaryContent == null) return false;
    const current = this._getCurrentSummaryContent();
    return current !== this._originalSummaryContent;
  }

  /**
   * Check if Collection content has changed from original
   */
  _checkCollectionDirty() {
    if (this._originalCollectionContent == null) return false;
    const current = this._getCurrentCollectionContent();
    return current !== this._originalCollectionContent;
  }

  /**
   * Check all dirty states and update accordingly
   * Called periodically or when content changes
   */
  _refreshDirtyState() {
    this._isDirty.sql = this._checkSqlDirty();
    this._isDirty.summary = this._checkSummaryDirty();
    this._isDirty.collection = this._checkCollectionDirty();
    this._updateSaveCancelButtonState();
  }

  async init() {
    await this.render();
    this.setupEventListeners();
    this.setupSplitter();
    this._restorePanelWidth();
    await this.initTable();
    this._setupFooter();
  }

  async render() {
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
      splitter: this.container.querySelector('.queries-splitter'),
      rightPanel: this.container.querySelector('.queries-right-panel'),
      tabBtns: this.container.querySelectorAll('.queries-tab-btn:not(.queries-collapse-btn)'),
      tabPanes: this.container.querySelectorAll('.queries-tab-pane'),
      collapseBtn: this.container.querySelector('#queries-collapse-btn'),
      collapseIcon: this.container.querySelector('#queries-collapse-icon'),
      sqlEditorContainer: this.container.querySelector('#queries-sql-editor'),
      summaryEditorContainer: this.container.querySelector('#queries-tab-summary'),
      collectionEditorContainer: this.container.querySelector('#queries-tab-collection'),
      previewContainer: this.container.querySelector('#queries-tab-preview'),
      testContainer: this.container.querySelector('#queries-tab-test'),
      // Toolbar buttons
      undoBtn: this.container.querySelector('#queries-undo-btn'),
      redoBtn: this.container.querySelector('#queries-redo-btn'),
      fontBtn: this.container.querySelector('#queries-font-btn'),
      prettifyBtn: this.container.querySelector('#queries-prettify-btn'),
      tabsHeader: this.container.querySelector('.queries-tabs-header'),
    };
  }

  setupEventListeners() {
    // Tab switching
    this.elements.tabBtns?.forEach(btn => {
      btn.addEventListener('click', () => {
        const tabId = btn.dataset.tab;
        this.switchTab(tabId);
      });
    });

    // Collapse/Expand
    this.elements.collapseBtn?.addEventListener('click', () => {
      this.toggleCollapse();
    });

    // Build the four navigator blocks: Control, Move, Manage, Search
    this.buildNavigator();

    // Toolbar buttons
    this.elements.undoBtn?.addEventListener('click', () => this.handleUndo());
    this.elements.redoBtn?.addEventListener('click', () => this.handleRedo());
    this.elements.fontBtn?.addEventListener('click', () => this.handleFontClick());
    this.elements.prettifyBtn?.addEventListener('click', () => this.handlePrettify());
  }

  /**
   * Build the navigator control bar with four blocks:
   *   1. Control  — refresh, menu, width, layout, export, import
   *   2. Move     — first, prev-page, prev, next, next-page, last
   *   3. Manage   — add, duplicate, edit, save, cancel, delete
   *   4. Search   — magnifying-glass icon, input, X button
   *
   * Each block is a nowrap row of buttons that doesn't wrap internally,
   * but the blocks themselves can stack when the panel is narrow.
   */
  buildNavigator() {
    const nav = this.elements.navigatorContainer;
    if (!nav) return;

    nav.innerHTML = this._getNavigatorHTML();

    // Process <fa> icons inside the navigator
    processIcons(nav);

    // Wire up all button groups
    this._wireControlButtons(nav);
    this._wireMoveButtons(nav);
    this._wireManageButtons(nav);
    this._wireSearchButtons(nav);
  }

  /**
   * Get the navigator HTML template
   * @returns {string} HTML string
   * @private
   */
  _getNavigatorHTML() {
    return `
      <div class="queries-nav-wrapper">
        <div class="queries-nav-block queries-nav-block-control">
          <button type="button" class="queries-nav-btn" id="queries-nav-refresh" title="Refresh">
            <fa fa-arrows-rotate></fa>
          </button>
          <button type="button" class="queries-nav-btn ${this._filtersVisible ? 'queries-nav-btn-active' : ''}" id="queries-nav-filter" title="Toggle Filters">
            <fa fa-filter></fa>
          </button>
          <button type="button" class="queries-nav-btn queries-nav-btn-has-popup" id="queries-nav-menu" title="Table Options">
            <fa fa-layer-group></fa>
          </button>
          <button type="button" class="queries-nav-btn queries-nav-btn-has-popup" id="queries-nav-width" title="Table Width">
            <fa fa-left-right></fa>
          </button>
          <button type="button" class="queries-nav-btn queries-nav-btn-has-popup" id="queries-nav-layout" title="Table Layout">
            <fa fa-table-columns></fa>
          </button>
          <button type="button" class="queries-nav-btn queries-nav-btn-has-popup" id="queries-nav-template" title="Templates">
            <fa fa-screwdriver-wrench></fa>
          </button>
        </div>
        <div class="queries-nav-block queries-nav-block-move">
          <button type="button" class="queries-nav-btn" id="queries-nav-first" title="First Record">
            <fa fa-backward-fast></fa>
          </button>
          <button type="button" class="queries-nav-btn" id="queries-nav-pgup" title="Previous Page">
            <fa fa-backward-step></fa>
          </button>
          <button type="button" class="queries-nav-btn" id="queries-nav-prev" title="Previous Record">
            <fa fa-caret-left></fa>
          </button>
          <button type="button" class="queries-nav-btn" id="queries-nav-next" title="Next Record">
            <fa fa-caret-right></fa>
          </button>
          <button type="button" class="queries-nav-btn" id="queries-nav-pgdn" title="Next Page">
            <fa fa-forward-step></fa>
          </button>
          <button type="button" class="queries-nav-btn" id="queries-nav-last" title="Last Record">
            <fa fa-forward-fast></fa>
          </button>
        </div>
        <div class="queries-nav-block queries-nav-block-manage">
          <button type="button" class="queries-nav-btn" id="queries-nav-add" title="Add">
            <fa fa-plus></fa>
          </button>
          <button type="button" class="queries-nav-btn" id="queries-nav-duplicate" title="Duplicate">
            <fa fa-copy></fa>
          </button>
          <button type="button" class="queries-nav-btn" id="queries-nav-edit" title="Edit">
            <fa fa-pen-to-square></fa>
          </button>
          <button type="button" class="queries-nav-btn" id="queries-nav-save" title="Save" disabled>
            <fa fa-floppy-disk></fa>
          </button>
          <button type="button" class="queries-nav-btn" id="queries-nav-cancel" title="Cancel" disabled>
            <fa fa-ban></fa>
          </button>
          <button type="button" class="queries-nav-btn queries-nav-btn-danger" id="queries-nav-delete" title="Delete">
            <fa fa-trash-can></fa>
          </button>
        </div>
        <div class="queries-nav-block queries-nav-block-search">
          <button type="button" class="queries-nav-btn" id="queries-search-btn" title="Search">
            <fa fa-magnifying-glass></fa>
          </button>
          <input type="text" class="queries-nav-search-input" id="queries-search-input" placeholder="Search...">
          <button type="button" class="queries-nav-btn" id="queries-search-clear-btn" title="Clear Search">
            <fa fa-xmark></fa>
          </button>
        </div>
      </div>
    `;
  }

  /**
   * Wire up control buttons
   * @param {HTMLElement} nav - Navigator container
   * @private
   */
  _wireControlButtons(nav) {
    nav.querySelector('#queries-nav-refresh')?.addEventListener('click', () => this.handleNavRefresh());
    nav.querySelector('#queries-nav-filter')?.addEventListener('click', () => this.handleNavFilter());
    nav.querySelector('#queries-nav-menu')?.addEventListener('click', (e) => this.toggleNavPopup(e, 'menu'));
    nav.querySelector('#queries-nav-width')?.addEventListener('click', (e) => this.toggleNavPopup(e, 'width'));
    nav.querySelector('#queries-nav-layout')?.addEventListener('click', (e) => this.toggleNavPopup(e, 'layout'));
    nav.querySelector('#queries-nav-template')?.addEventListener('click', (e) => this.toggleNavPopup(e, 'template'));
  }

  /**
   * Wire up move buttons
   * @param {HTMLElement} nav - Navigator container
   * @private
   */
  _wireMoveButtons(nav) {
    nav.querySelector('#queries-nav-first')?.addEventListener('click', () => this.navigateFirst());
    nav.querySelector('#queries-nav-pgup')?.addEventListener('click',  () => this.navigatePrevPage());
    nav.querySelector('#queries-nav-prev')?.addEventListener('click',  () => this.navigatePrevRec());
    nav.querySelector('#queries-nav-next')?.addEventListener('click',  () => this.navigateNextRec());
    nav.querySelector('#queries-nav-pgdn')?.addEventListener('click',  () => this.navigateNextPage());
    nav.querySelector('#queries-nav-last')?.addEventListener('click',  () => this.navigateLast());
  }

  /**
   * Wire up manage buttons
   * @param {HTMLElement} nav - Navigator container
   * @private
   */
  _wireManageButtons(nav) {
    nav.querySelector('#queries-nav-add')?.addEventListener('click',       () => this.handleNavAdd());
    nav.querySelector('#queries-nav-duplicate')?.addEventListener('click', () => this.handleNavDuplicate());
    nav.querySelector('#queries-nav-edit')?.addEventListener('click',      () => this.handleNavEdit());
    nav.querySelector('#queries-nav-save')?.addEventListener('click',      () => this.handleNavSave());
    nav.querySelector('#queries-nav-cancel')?.addEventListener('click',    () => this.handleNavCancel());
    nav.querySelector('#queries-nav-delete')?.addEventListener('click',    () => this.handleNavDelete());
  }

  /**
   * Wire up search buttons and input
   * @param {HTMLElement} nav - Navigator container
   * @private
   */
  _wireSearchButtons(nav) {
    const searchInput = nav.querySelector('#queries-search-input');
    const searchBtn   = nav.querySelector('#queries-search-btn');
    const clearBtn    = nav.querySelector('#queries-search-clear-btn');

    const performSearch = () => {
      this.loadQueries(searchInput.value);
    };

    searchBtn?.addEventListener('click', performSearch);
    searchInput?.addEventListener('keypress', (e) => {
      if (e.key === 'Enter') {
        performSearch();
      }
    });

    clearBtn?.addEventListener('click', () => {
      searchInput.value = '';
      this.loadQueries();
    });
  }

  // ── Navigator popup menus ─────────────────────────────────────────────

  /**
   * Toggle a navigator popup menu anchored to a button.
   * @param {MouseEvent} e   - click event from the trigger button
   * @param {'menu'|'export'|'import'} popupId - which popup to show
   */
  toggleNavPopup(e, popupId) {
    e.stopPropagation();

    // If the same popup is already open, close it
    if (this._activeNavPopup && this._activeNavPopupId === popupId) {
      this._closeNavPopup();
      return;
    }

    // Close any existing popup first
    this._closeNavPopup();

    const btn = e.currentTarget;
    const items = this._getPopupItems(popupId);
    if (!items.length) return;

    // Build popup element
    const popup = document.createElement('div');
    popup.className = 'queries-nav-popup';
    items.forEach(item => {
      // Handle separator
      if (item.isSeparator) {
        const separator = document.createElement('div');
        separator.className = 'queries-nav-popup-separator';
        popup.appendChild(separator);
        return;
      }

      // Handle template items with delete button
      if (item.isTemplate) {
        const row = document.createElement('div');
        row.className = 'queries-nav-popup-template-item';
        
        // Template label/button
        const labelBtn = document.createElement('button');
        labelBtn.type = 'button';
        labelBtn.className = 'queries-nav-popup-template-label';
        labelBtn.innerHTML = `
          <span class="queries-nav-popup-template-check">${item.isDefault ? '✓' : ''}</span>
          <span class="queries-nav-popup-template-name">${item.label}</span>
          ${item.isDefault ? '<span class="queries-nav-popup-template-default">(default)</span>' : ''}
        `;
        labelBtn.addEventListener('click', () => {
          this._closeNavPopup();
          item.action();
        });
        row.appendChild(labelBtn);
        
        // Delete button
        const deleteBtn = document.createElement('button');
        deleteBtn.type = 'button';
        deleteBtn.className = 'queries-nav-popup-template-delete';
        deleteBtn.title = 'Delete template';
        deleteBtn.innerHTML = '<fa fa-trash-can></fa>';
        deleteBtn.addEventListener('click', (e) => {
          this._deleteTemplate(item.template.name, e);
        });
        row.appendChild(deleteBtn);
        
        popup.appendChild(row);
        return;
      }

      // Regular menu item with optional checkmark column
      const row = document.createElement('button');
      row.type = 'button';
      row.className = 'queries-nav-popup-item';
      
      // Build content with aligned checkmark column if checked property exists
      if (typeof item.checked === 'boolean') {
        row.innerHTML = `
          <span class="queries-nav-popup-check">${item.checked ? '✓' : ''}</span>
          <span class="queries-nav-popup-label">${item.label}</span>
        `;
      } else if (item.icon) {
        row.innerHTML = `<fa ${item.icon}></fa> ${item.label}`;
      } else {
        row.textContent = item.label;
      }
      
      row.addEventListener('click', () => {
        this._closeNavPopup();
        item.action();
      });
      popup.appendChild(row);
    });

    // Process icons in the popup (for delete buttons)
    processIcons(popup);

    // Position relative to the navigator container
    const navRect = this.elements.navigatorContainer.getBoundingClientRect();
    const btnRect = btn.getBoundingClientRect();
    popup.style.left = `${btnRect.left - navRect.left}px`;

    this.elements.navigatorContainer.style.position = 'relative';
    this.elements.navigatorContainer.appendChild(popup);

    // Force layout then add .visible for transition
    popup.getBoundingClientRect();
    popup.classList.add('visible');

    this._activeNavPopup = popup;
    this._activeNavPopupId = popupId;

    // Close when clicking outside
    this._navPopupCloseHandler = (evt) => {
      if (!popup.contains(evt.target) && !btn.contains(evt.target)) {
        this._closeNavPopup();
      }
    };
    document.addEventListener('click', this._navPopupCloseHandler);
  }

  /**
   * Close the currently-open navigator popup.
   */
  _closeNavPopup() {
    if (this._activeNavPopup) {
      this._activeNavPopup.remove();
      this._activeNavPopup = null;
      this._activeNavPopupId = null;
    }
    if (this._navPopupCloseHandler) {
      document.removeEventListener('click', this._navPopupCloseHandler);
      this._navPopupCloseHandler = null;
    }
  }

  /**
   * Return the menu items for a given popup type.
   * @param {'menu'|'export'|'import'} popupId
   * @returns {Array<{label: string, action: Function}>}
   */
  _getPopupItems(popupId) {
    switch (popupId) {
      case 'menu':
        return [
          { label: 'Expand All',   action: () => this.handleMenuExpandAll() },
          { label: 'Collapse All', action: () => this.handleMenuCollapseAll() },
        ];
      case 'template':
        return this._getTemplatePopupItems();
      case 'width':
        return [
          { label: 'Narrow',  checked: this._tableWidthMode === 'narrow',  action: () => this.setTableWidth('narrow') },
          { label: 'Compact', checked: this._tableWidthMode === 'compact', action: () => this.setTableWidth('compact') },
          { label: 'Normal',  checked: this._tableWidthMode === 'normal',  action: () => this.setTableWidth('normal') },
          { label: 'Wide',    checked: this._tableWidthMode === 'wide',    action: () => this.setTableWidth('wide') },
          { label: 'Auto',    checked: this._tableWidthMode === 'auto',    action: () => this.setTableWidth('auto') },
        ];
      case 'layout':
        return [
          { label: 'Fit Columns',  checked: this._tableLayoutMode === 'fitColumns',     action: () => this.setTableLayout('fitColumns') },
          { label: 'Fit Data',     checked: this._tableLayoutMode === 'fitData',        action: () => this.setTableLayout('fitData') },
          { label: 'Fit Fill',     checked: this._tableLayoutMode === 'fitDataFill',    action: () => this.setTableLayout('fitDataFill') },
          { label: 'Fit Stretch',  checked: this._tableLayoutMode === 'fitDataStretch', action: () => this.setTableLayout('fitDataStretch') },
          { label: 'Fit Table',    checked: this._tableLayoutMode === 'fitDataTable',   action: () => this.setTableLayout('fitDataTable') },
        ];
      default:
        return [];
    }
  }

  // ── Navigation helpers ────────────────────────────────────────────────

  /**
   * Get the array of visible (non-filtered) rows and the currently
   * selected row's index within that array.
   * @returns {{ rows: Array, selectedIndex: number }}
   */
  _getVisibleRowsAndIndex() {
    if (!this.table) return { rows: [], selectedIndex: -1 };
    const rows = this.table.getRows('active');  // visible/filtered rows
    const selected = this.table.getSelectedRows();
    let selectedIndex = -1;
    if (selected.length > 0) {
      const selPos = selected[0].getPosition(true); // position among active rows
      selectedIndex = selPos - 1; // 0-based
    }
    return { rows, selectedIndex };
  }

  /**
   * Select a row by its index in the visible row array and scroll into view.
   * @param {number} index - 0-based index in visible rows
   */
  _selectRowByIndex(index) {
    if (!this.table) return;
    const rows = this.table.getRows('active');
    if (index < 0 || index >= rows.length) return;
    this.table.deselectRow();
    rows[index].select();
    rows[index].scrollTo();
  }

  /**
   * Get the number of rows visible in one page (based on table viewport).
   * Falls back to 10 if we can't measure.
   * @returns {number}
   */
  _getPageSize() {
    if (!this.table) return 10;
    const holder = this.elements.tableContainer?.querySelector('.tabulator-tableholder');
    if (!holder) return 10;
    const rowEl = holder.querySelector('.tabulator-row');
    if (!rowEl) return 10;
    const rowHeight = rowEl.offsetHeight || 30;
    const visibleHeight = holder.clientHeight;
    return Math.max(1, Math.floor(visibleHeight / rowHeight));
  }

  navigateFirst() {
    if (!this.table) return;
    this._selectRowByIndex(0);
  }

  navigateLast() {
    const rows = this.table?.getRows('active') || [];
    if (rows.length > 0) {
      this._selectRowByIndex(rows.length - 1);
    }
  }

  // ── Table loading indicator ───────────────────────────────────────────

  /**
   * Show loading overlay by adding a CSS class to the table container.
   * The spinner is styled via CSS using :has() or a child element.
   */
  _showTableLoading() {
    if (!this.elements.tableContainer) return;

    // Remove any existing loader first
    this._hideTableLoading();

    // Create and append the loader element
    const loader = document.createElement('div');
    loader.className = 'queries-table-loader';
    loader.innerHTML = '<div class="spinner-fancy spinner-fancy-md"></div>';
    this.elements.tableContainer.appendChild(loader);
    this._tableLoaderElement = loader;
  }

  /**
   * Hide the loading overlay.
   */
  _hideTableLoading() {
    if (this._tableLoaderElement) {
      this._tableLoaderElement.remove();
      this._tableLoaderElement = null;
    }
  }

  navigatePrevRec() {
    const { selectedIndex } = this._getVisibleRowsAndIndex();
    if (selectedIndex > 0) {
      this._selectRowByIndex(selectedIndex - 1);
    }
  }

  navigateNextRec() {
    const { rows, selectedIndex } = this._getVisibleRowsAndIndex();
    if (selectedIndex < rows.length - 1) {
      this._selectRowByIndex(selectedIndex + 1);
    }
  }

  navigatePrevPage() {
    const { rows, selectedIndex } = this._getVisibleRowsAndIndex();
    const pageSize = this._getPageSize();
    const newIndex = Math.max(0, selectedIndex - pageSize);
    this._selectRowByIndex(newIndex);
  }

  navigateNextPage() {
    const { rows, selectedIndex } = this._getVisibleRowsAndIndex();
    const pageSize = this._getPageSize();
    const newIndex = Math.min(rows.length - 1, selectedIndex + pageSize);
    this._selectRowByIndex(newIndex);
  }

  /**
   * Update the enabled/disabled state of the Move navigation buttons
   * based on the current row position and table data.
   *
   * Logic:
   *   - FirstRec, PrevPage, PrevRec: disabled when at first row (index 0)
   *   - NextRec, NextPage, LastRec: disabled when at last row
   *   - PrevPage: also disabled if page size >= current position
   *   - NextPage: also disabled if remaining rows <= page size
   */
  _updateMoveButtonState() {
    if (!this.table) return;

    const nav = this.elements.navigatorContainer;
    if (!nav) return;

    // Guard against mock tables without getRows method (tests)
    if (typeof this.table.getRows !== 'function') return;

    const { rows, selectedIndex } = this._getVisibleRowsAndIndex();
    const rowCount = rows.length;

    // If no rows or no selection, disable all navigation
    const atFirst = selectedIndex <= 0;
    const atLast = selectedIndex < 0 || selectedIndex >= rowCount - 1;

    // Page-based logic for page buttons
    const pageSize = this._getPageSize();
    const canGoPrevPage = selectedIndex > 0 && selectedIndex >= pageSize;
    const canGoNextPage = selectedIndex >= 0 && selectedIndex < rowCount - 1 - pageSize;

    // Get button references
    const firstBtn = nav.querySelector('#queries-nav-first');
    const prevPageBtn = nav.querySelector('#queries-nav-pgup');
    const prevRecBtn = nav.querySelector('#queries-nav-prev');
    const nextRecBtn = nav.querySelector('#queries-nav-next');
    const nextPageBtn = nav.querySelector('#queries-nav-pgdn');
    const lastBtn = nav.querySelector('#queries-nav-last');

    // Update button states
    if (firstBtn) firstBtn.disabled = atFirst;
    if (prevPageBtn) prevPageBtn.disabled = !canGoPrevPage;
    if (prevRecBtn) prevRecBtn.disabled = atFirst;
    if (nextRecBtn) nextRecBtn.disabled = atLast;
    if (nextPageBtn) nextPageBtn.disabled = !canGoNextPage;
    if (lastBtn) lastBtn.disabled = atLast;
  }

  // ── Selector column helpers ────────────────────────────────────────────

  /**
   * Update the selector cell indicator for a row.
   * Shows ▸ for selected rows, │ for rows being edited.
   * @param {Object} row - Tabulator row component
   * @param {boolean} isSelected - whether the row is selected
   */
  _updateSelectorCell(row, isSelected) {
    try {
      const cell = row.getCell('_selector');
      if (!cell) return;
      const el = cell.getElement();
      if (!el) return;

      if (isSelected) {
        const pkField = this.primaryKeyField || 'query_id';
        const isEditing = this._isEditing &&
          this._editingRowId === row.getData()[pkField];
        el.innerHTML = isEditing
          ? '<fa fa-i-cursor queries-selector-indicator queries-selector-edit>'
          : '<fa fa-caret-right queries-selector-indicator queries-selector-active>';
      } else {
        el.innerHTML = '';
      }
    } catch (e) {
      // Silently handle — cell may not exist during rapid updates
    }
  }

  /**
   * Refresh the selector indicator for the currently selected row.
   * Called when editing state changes (enter/exit edit mode).
   */
  _updateEditingIndicator() {
    if (!this.table) return;
    const selected = this.table.getSelectedRows();
    if (selected.length > 0) {
      this._updateSelectorCell(selected[0], true);
    }
  }

  /**
   * Toggle a short-lived visual state while edit mode is entering/exiting.
   * This gives the user a wait cursor during any transition work.
   * @param {boolean} pending
   */
  _setEditTransitionState(pending) {
    this._editTransitionPending = pending;
    this.elements.tableContainer?.classList.toggle('queries-edit-transition', pending);
    this.elements.navigatorContainer?.classList.toggle('queries-edit-transition', pending);
  }

  // ── Edit Mode ──────────────────────────────────────────────────────────

  /**
   * Enter edit mode for the currently selected row.
   *
   * This method orchestrates a coordinated set of visual and behavioural
   * changes that signal "this row is now editable":
   *   - Adds the `.queries-edit-mode` CSS class to the table container,
   *     which switches the selected-row highlight from accent-primary
   *     (indigo) to accent-warning (amber)
   *   - Sets `_editingRowId` so the selector column shows the I-cursor
   *   - Captures original data for change tracking
   *   - Makes CodeMirror editors editable (they default to readOnly)
   *   - Optionally focuses an editable cell for immediate typing
   *
   * @param {Object} [row] - Tabulator row component. If omitted, uses the
   *   currently selected row.
   */
  async _enterEditMode(row) {
    if (!this.table) return;

    this._setEditTransitionState(true);

    try {

      // Resolve the target row
      if (!row) {
        const selected = this.table.getSelectedRows();
        if (selected.length === 0) return;
        row = selected[0];
      }

      const pkField = this.primaryKeyField || 'query_id';
      const rowId = row.getData()?.[pkField];

      // Already editing this row — nothing to do
      if (this._isEditing && this._editingRowId === rowId) return;

      // If we were editing a different row, exit that first
      if (this._isEditing) {
        await this._exitEditMode();
      }

      // ── Activate edit state ──────────────────────────────────────────
      this._editingRowId = rowId;
      this._isEditing = true;

      // Capture original data for change tracking
      this._captureOriginalData(row.getData());

      // CSS class drives the amber selected-row highlight + text cursor
      this.elements.tableContainer?.classList.add('queries-edit-mode');

      // Legacy no-op hook retained for compatibility
      await this._enableColumnEditors();

      // Selector indicator: I-cursor with blink
      this._updateEditingIndicator();

      // Update Save / Cancel buttons (disabled until changes are made)
      this._updateSaveCancelButtonState();

      // Make CodeMirror editors writable
      this._setCodeMirrorEditable(true);

      log(Subsystems.MANAGER, Status.INFO,
        `[QueriesManager] Entered edit mode (row ${rowId})`);
    } finally {
      requestAnimationFrame(() => this._setEditTransitionState(false));
    }
  }

  /**
   * Exit edit mode — revert all visual / behavioural changes made by
   * `_enterEditMode()`. Called on Save, Cancel, row-change, or
   * pressing Edit again.
   *
   * @param {'save'|'cancel'|'row-change'|'toggle'} [reason='cancel']
   *   Why we're exiting edit mode. Determines whether the exit is
   *   logged differently; the actual save/undo logic lives in the
   *   calling handler (handleNavSave / handleNavCancel), not here.
   */
  async _exitEditMode(reason = 'cancel') {
    if (!this._isEditing) return; // Not in edit mode

    this._setEditTransitionState(true);

    try {

      const previousId = this._editingRowId;

      // Cancel any active inline cell editor before changing state
      this._cancelActiveCellEdit();

      // ── Deactivate edit state ────────────────────────────────────────
      this._editingRowId = null;
      this._isEditing = false;

      // Remove amber highlight + text cursor
      this.elements.tableContainer?.classList.remove('queries-edit-mode');

      // Legacy no-op hook retained for compatibility
      await this._disableColumnEditors();

      // Selector indicator: back to ▸ arrow (or empty if deselected)
      this._updateEditingIndicator();

      // Disable Save / Cancel buttons (not in edit mode)
      this._updateSaveCancelButtonState();

      // Make CodeMirror editors read-only again
      this._setCodeMirrorEditable(false);

      // Clear dirty state when exiting edit mode
      // (on save, this happens after save completes; on cancel, after revert)
      if (reason !== 'save') {
        this._markAllClean();
        this._originalRowData = null;
        this._originalSqlContent = null;
        this._originalSummaryContent = null;
        this._originalCollectionContent = null;
      }

      log(Subsystems.MANAGER, Status.INFO,
        `[QueriesManager] Exited edit mode (row ${previousId}, reason: ${reason})`);
    } finally {
      requestAnimationFrame(() => this._setEditTransitionState(false));
    }
  }

  /**
   * Enable or disable the Save and Cancel nav buttons.
   *
   * In normal (non-edit) mode these buttons are disabled to prevent
   * accidental saves.  When edit mode is entered AND there are changes,
   * they are enabled.
   *
   * @param {boolean} editing - true when entering edit mode, false when exiting
   * @deprecated Use _updateSaveCancelButtonState() instead for dirty-aware state
   */
  _setManageButtonState(editing) {
    // This is now a no-op wrapper - actual logic is in _updateSaveCancelButtonState
    // which considers both edit mode AND dirty state
    this._updateSaveCancelButtonState();
  }

  /**
   * Toggle CodeMirror editor editability.
   *
   * Uses CodeMirror 6's `EditorView.readOnly` compartment-style
   * reconfiguration via `dispatch({ effects })`.  For simplicity,
   * we use the `EditorState.readOnly` facet via reconfigure.
   *
   * Since the editors are created dynamically (and may not exist yet),
   * we guard each access.
   *
   * @param {boolean} editable - true → writable, false → readOnly
   */
  _setCodeMirrorEditable(editable) {
    // The readOnly state is controlled by recreating the editor with or
    // without the readOnly extension.  However, CodeMirror 6 doesn't
    // make this trivial without compartments.  A simpler approach is to
    // toggle the `contenteditable` attribute on the CM DOM element and
    // set a CSS class for visual feedback.
    const editors = [
      { view: this.sqlEditor, container: this.elements.sqlEditorContainer },
      { view: this.summaryEditor, container: this.elements.summaryEditorContainer },
    ];

    for (const { view, container } of editors) {
      if (!view) continue;

      // Toggle DOM-level editability
      const contentEl = view.dom.querySelector('.cm-content');
      if (contentEl) {
        contentEl.contentEditable = editable ? 'true' : 'false';
      }

      // Visual feedback: dimmed appearance when read-only
      if (container) {
        container.classList.toggle('queries-cm-readonly', !editable);
      }
    }
  }

  /**
   * Cancel any active inline cell editor in the table.
   *
   * When exiting edit mode, an inline editor (input/textarea) may still
   * be focused inside a Tabulator cell.  Blurring it causes Tabulator
   * to close the editor cleanly (applying or reverting the value per
   * its own configuration).
   */
  _cancelActiveCellEdit() {
    this._commitActiveCellEdit();
  }

  /**
   * Commit the active inline cell editor by blurring it.
   *
   * Switching between editable cells on the same row should save the current
   * field before the next editor opens. Blurring the current editor lets
   * Tabulator apply its normal edit-complete flow.
   *
   * @param {Object|null} [nextCell=null] - Optional target cell. If the active
   *   editor already belongs to this cell, no blur is forced.
   */
  _commitActiveCellEdit(nextCell = null) {
    const active = document.activeElement;
    if (!active || !this.elements.tableContainer?.contains(active)) return;

    const activeCellEl = active.closest?.('.tabulator-cell');
    const nextCellEl = nextCell?.getElement?.() || null;
    if (activeCellEl && nextCellEl && activeCellEl === nextCellEl) {
      return;
    }

    active.blur();
  }

  /**
   * Determine whether a row is a non-data footer/calc row.
   *
   * @param {Object} row - Tabulator row component
   * @returns {boolean}
   */
  _isCalcRow(row) {
    if (!row) return false;

    const rowEl = row.getElement?.();
    return !!rowEl?.closest?.('.tabulator-calcs, .tabulator-calcs-holder, .tabulator-calcs-bottom');
  }

  /**
   * Determine whether a cell belongs to the footer calculation section.
   *
   * @param {Object} cell - Tabulator cell component
   * @returns {boolean}
   */
  _isCalcCell(cell) {
    if (!cell) return false;

    const cellEl = cell.getElement?.();
    if (cellEl?.closest?.('.tabulator-calcs, .tabulator-calcs-holder, .tabulator-calcs-bottom')) {
      return true;
    }

    return this._isCalcRow(cell.getRow?.());
  }

  /**
   * Compare two rows by component identity or primary-key identity.
   *
   * @param {Object|null} rowA - Tabulator row component
   * @param {Object|null} rowB - Tabulator row component
   * @returns {boolean}
   */
  _rowsMatch(rowA, rowB) {
    if (!rowA || !rowB) return false;
    if (rowA === rowB) return true;

    const pkField = this.primaryKeyField || 'query_id';
    const rowAId = rowA.getData?.()?.[pkField];
    const rowBId = rowB.getData?.()?.[pkField];
    return rowAId != null && rowBId != null && String(rowAId) === String(rowBId);
  }

  /**
   * Get the currently selected data row, excluding footer/calc rows.
   *
   * @returns {Object|null}
   */
  _getSelectedDataRow() {
    if (!this.table) return null;
    const selectedRows = this.table.getSelectedRows?.() || [];
    return selectedRows.find(row => !this._isCalcRow(row)) || null;
  }

  /**
   * Close transient popup UI that should not remain open during row changes.
   *
   * Row navigation can happen via mouse, keyboard, or programmatic selection.
   * Some of those paths do not bubble a document click, so popups such as the
   * column chooser can otherwise remain visible after the active row changes.
   */
  _closeTransientPopups() {
    this._closeColumnChooser();
    this._closeNavPopup();
    this._closeFooterExportPopup();
    this.hideFontPopup();
  }

  /**
   * Select a data row without churning selection if the same logical row is
   * already selected.
   *
   * @param {Object} row - Tabulator row component
   * @returns {boolean} True when selection changed, false otherwise
   */
  _selectDataRow(row) {
    if (!this.table || !row || this._isCalcRow(row)) return false;

    const currentRow = this._getSelectedDataRow();
    if (this._rowsMatch(currentRow, row)) {
      if (!row.isSelected?.()) {
        row.select();
      }
      return false;
    }

    this._closeTransientPopups();

    this.table.deselectRow();
    row.select();
    return true;
  }

  /**
   * Open a cell editor after any pending blur/save cycle has settled.
   *
   * A single rAF is not always sufficient when handing off from one active
   * editor to another. Using a tokenised double-rAF keeps only the latest edit
   * request and gives Tabulator time to finish the previous editor teardown
   * before opening the next field.
   *
   * @param {Object|null} cell - Tabulator cell component to edit
   */
  _queueCellEdit(cell) {
    if (!cell) return;

    const field = cell.getField?.();
    if (!field || !this._columnEditors?.has(field)) return;

    const editToken = ++this._pendingCellEditToken;

    requestAnimationFrame(() => {
      requestAnimationFrame(() => {
        if (editToken !== this._pendingCellEditToken) return;
        if (!this._isEditing) return;

        const row = cell.getRow?.();
        if (!row || this._isCalcRow(row)) return;

        const pkField = this.primaryKeyField || 'query_id';
        const rowId = row.getData?.()?.[pkField];
        const isEditingRow = (this._editingRowId != null)
          ? rowId === this._editingRowId
          : row.isSelected?.();

        if (!isEditingRow) return;

        cell.edit();
      });
    });
  }

  /**
   * Post-process resolved columns to strip editors from the Tabulator
   * column definitions and store them separately.
   *
   * **Why not use `editable` as a function gate?**
   * When a column has `editor` set (even with `editable: () => false`),
   * Tabulator's internal cell-click handler intercepts the click event
   * to check whether to open the editor. This can prevent the click from
   * reaching the row-selection handler, causing the row not to be
   * selected when clicking on editor-enabled cells (issue #4).
   *
   * By removing `editor` entirely from the column definition, we ensure
   * Tabulator treats all cells identically for click handling. Editors
   * are invoked programmatically via `cell.edit()` in the `cellClick`
   * handler when the manager is in edit mode.
   *
   * The stripped editor definitions are stored in `this._columnEditors`
   * so they can be re-applied when entering edit mode and removed when
   * exiting.
   *
   * @param {Array<Object>} columns - Resolved Tabulator column definitions
   */
  _applyEditModeGate(columns) {
    // Store editor definitions keyed by field name
    this._columnEditors = new Map();

    for (const col of columns) {
      if (!col.editor) continue;

      this._columnEditors.set(col.field, {
        editor: col.editor,
        editorParams: col.editorParams || undefined,
      });

      // Keep the editor definition attached so Tabulator always knows the
      // correct editor type for the column, but gate activation by row/edit mode.
      // This avoids expensive updateColumnDefinition() churn when switching modes.
      col.editable = (cell) => {
        if (!this._isEditing) return false;

        const pkField = this.primaryKeyField || 'query_id';
        const rowId = cell.getRow().getData()?.[pkField];
        return this._editingRowId != null
          ? rowId === this._editingRowId
          : cell.getRow().isSelected();
      };
    }
  }

  /**
   * Apply stored editor definitions to the Tabulator columns.
   * Called when entering edit mode so cells on the editing row become editable.
   */
  async _enableColumnEditors() {
    return Promise.resolve();
  }

  /**
   * Remove editors from all Tabulator columns.
   * Called when exiting edit mode so the table returns to read-only.
   */
  async _disableColumnEditors() {
    return Promise.resolve();
  }

  // ── Column chooser ────────────────────────────────────────────────────

  /**
   * Toggle the column chooser dropdown anchored to the selector column header.
   * Shows a scrollable checklist of all data columns for toggling visibility.
   * @param {MouseEvent} e - click event
   * @param {Object} column - Tabulator column component
   */
  toggleColumnChooser(e, column) {
    e.stopPropagation();
    if (!this.table) return;

    // If already open, close it
    if (this._columnChooserPopup) {
      this._closeColumnChooser();
      return;
    }

    // Build the popup
    const popup = document.createElement('div');
    popup.className = 'queries-col-chooser-popup';

    // Title
    const title = document.createElement('div');
    title.className = 'queries-col-chooser-title';
    title.textContent = 'Columns';
    popup.appendChild(title);

    // Get all columns except the selector
    const columns = this.table.getColumns().filter(
      col => col.getField() !== '_selector'
    );

    // Build scrollable list
    const list = document.createElement('div');
    list.className = 'queries-col-chooser-list';
    if (columns.length > 10) {
      list.style.maxHeight = `${10 * 30}px`;
    }

    columns.forEach(col => {
      const colTitle = col.getDefinition().title || col.getField();
      const isVisible = col.isVisible();

      const item = document.createElement('label');
      item.className = 'queries-col-chooser-item';

      const checkbox = document.createElement('input');
      checkbox.type = 'checkbox';
      checkbox.checked = isVisible;
      checkbox.className = 'queries-col-chooser-checkbox';
      checkbox.addEventListener('change', () => {
        if (checkbox.checked) {
          col.show();
        } else {
          col.hide();
        }
        // Force a full redraw to ensure columns are properly laid out
        // This is especially important for fitColumns layout mode
        if (this.table) {
          this.table.redraw(true);
        }
      });

      const labelText = document.createElement('span');
      labelText.className = 'queries-col-chooser-label';
      labelText.textContent = colTitle;

      item.appendChild(checkbox);
      item.appendChild(labelText);
      list.appendChild(item);
    });

    popup.appendChild(list);

    // Position below the selector column header
    const headerEl = column.getElement();
    const tableRect = this.elements.tableContainer.getBoundingClientRect();
    const headerRect = headerEl.getBoundingClientRect();

    popup.style.left = `${headerRect.left - tableRect.left}px`;
    popup.style.top = `${headerRect.bottom - tableRect.top}px`;

    this.elements.tableContainer.appendChild(popup);

    // Animate in
    requestAnimationFrame(() => {
      popup.classList.add('visible');
    });

    this._columnChooserPopup = popup;

    // Close on click outside
    this._columnChooserCloseHandler = (evt) => {
      if (!popup.contains(evt.target) && !headerEl.contains(evt.target)) {
        this._closeColumnChooser();
      }
    };
    setTimeout(() => {
      document.addEventListener('click', this._columnChooserCloseHandler);
    }, 0);
  }

  /**
   * Close the column chooser popup.
   */
  _closeColumnChooser() {
    if (this._columnChooserPopup) {
      this._columnChooserPopup.remove();
      this._columnChooserPopup = null;
    }
    if (this._columnChooserCloseHandler) {
      document.removeEventListener('click', this._columnChooserCloseHandler);
      this._columnChooserCloseHandler = null;
    }
  }

  // ── Manage button handlers (placeholder — wire up actions later) ──────

  /**
   * Refresh: re-submit the API call, redraw the table, and restore
   * the cursor to the same row (using in-memory ID, falling back
   * to localStorage so the position survives login round-trips).
   */
  handleNavRefresh() {
    log(Subsystems.MANAGER, Status.INFO, 'Navigator: Refresh');
    // loadQueries() already captures the current selection via
    // _getSelectedQueryId() and passes it to _autoSelectRow(),
    // which now also checks localStorage as a fallback.
    this.loadQueries();
  }

  async handleNavAdd() {
    log(Subsystems.MANAGER, Status.INFO, 'Navigator: Add');
    if (!this.table) return;

    // Add a new empty row at the top of the table
    const newRow = await this.table.addRow({}, true); // true = add at position 0

    // Select the new row and enter edit mode
    if (newRow) {
      this.table.deselectRow();
      newRow.select();
      newRow.scrollTo();

      // Enter edit mode for the new row (adds editors to columns)
      await this._enterEditMode(newRow);

      // Focus on the Name column for editing — defer to next frame
      // so the column definition update from _enableColumnEditors()
      // has been fully processed by Tabulator.
      const nameCell = newRow.getCell('name');
      this._queueCellEdit(nameCell);
    }

    log(Subsystems.MANAGER, Status.INFO, 'Added new query row');
  }

  async handleNavDuplicate() {
    log(Subsystems.MANAGER, Status.INFO, 'Navigator: Duplicate');
    if (!this.table) return;

    const selected = this.table.getSelectedRows();
    if (selected.length === 0) {
      toast.info('No Row Selected', {
        description: 'Please select a row to duplicate',
        duration: 3000,
      });
      return;
    }

    const originalData = selected[0].getData();
    // Create a copy with modified fields
    const duplicateData = { ...originalData };

    // Clear the primary key so a new record is created
    const pkField = this.primaryKeyField || 'query_id';
    delete duplicateData[pkField];

    // Append "(Copy)" to the name
    if (duplicateData.name) {
      duplicateData.name = `${duplicateData.name} (Copy)`;
    }

    // Add the duplicate row
    const newRow = await this.table.addRow(duplicateData, true);

    if (newRow) {
      this.table.deselectRow();
      newRow.select();
      newRow.scrollTo();

      // Enter edit mode for the duplicated row
      await this._enterEditMode(newRow);

      log(Subsystems.MANAGER, Status.INFO, 'Duplicated query row');
    }
  }

  /**
   * Toggle edit mode on the currently selected row.
   *
   * Behaviour:
   *   - If not in edit mode → enter edit mode for the selected row
   *   - If already editing the selected row → exit edit mode (toggle off)
   *   - If no row is selected → show toast
   */
  async handleNavEdit() {
    log(Subsystems.MANAGER, Status.INFO, 'Navigator: Edit');
    if (!this.table) return;

    const selected = this.table.getSelectedRows();
    if (selected.length === 0) {
      toast.info('No Row Selected', {
        description: 'Please select a row to edit',
        duration: 3000,
      });
      return;
    }

    const pkField = this.primaryKeyField || 'query_id';
    const selectedId = selected[0].getData()?.[pkField];

    // Toggle: if already editing this row, exit edit mode
    if (this._isEditing && this._editingRowId === selectedId) {
      await this._exitEditMode('toggle');
      return;
    }

    // Enter edit mode (adds editors to columns)
    await this._enterEditMode(selected[0]);

    // Start editing the Name cell (first editable column) — defer to
    // next frame so the column definition update has been processed.
    const editRow = selected[0];
    const nameCell = editRow.getCell('name');
    this._queueCellEdit(nameCell);
  }

  async handleNavSave() {
    log(Subsystems.MANAGER, Status.INFO, 'Navigator: Save');
    if (!this.table) return;

    if (!this._isAnyDirty()) {
      toast.info('No Changes', { description: 'Nothing to save — no changes have been made', duration: 3000 });
      return;
    }

    const selected = this.table.getSelectedRows();
    if (selected.length === 0) {
      toast.info('No Row Selected', { description: 'Please select a row to save', duration: 3000 });
      return;
    }

    const row = selected[0];
    const saveContext = this._buildSaveContext(row);
    
    if (!saveContext.queryRef) {
      await this._saveLocally(saveContext.isInsert);
      return;
    }

    try {
      await this._executeSave(row, saveContext);
      this._markAllClean();
      await this._exitEditMode('save');
      
      toast.success('Changes Saved', {
        description: `Query ${saveContext.isInsert ? 'inserted' : 'updated'} successfully`,
        duration: 3000,
      });
      log(Subsystems.MANAGER, Status.INFO, `${saveContext.isInsert ? 'Inserted' : 'Updated'} query (PK: ${saveContext.pkValue ?? 'new'})`);
    } catch (error) {
      toast.error('Save Failed', { serverError: error.serverError, subsystem: 'Conduit', duration: 8000 });
    }
  }

  /**
   * Build context object for save operation
   * @param {Object} row - Tabulator row
   * @returns {Object} Save context
   * @private
   */
  _buildSaveContext(row) {
    const rowData = row.getData();
    const pkField = this.primaryKeyField || 'query_id';
    const pkValue = rowData[pkField];
    const isInsert = pkValue == null || pkValue === '' || pkValue === 0;
    
    const insertRef = this.queryRefs?.insertQueryRef ?? null;
    const updateRef = this.queryRefs?.updateQueryRef ?? 28;
    
    return {
      pkField,
      pkValue,
      isInsert,
      queryRef: isInsert ? insertRef : updateRef,
      rowData,
    };
  }

  /**
   * Save locally when API is not configured
   * @param {boolean} isInsert - Whether this is an insert operation
   * @private
   */
  async _saveLocally(isInsert) {
    log(Subsystems.MANAGER, Status.WARN, `[QueriesManager] No ${isInsert ? 'insert' : 'update'}QueryRef configured — save is local only`);
    await this._exitEditMode('save');
    toast.info('Saved Locally', {
      description: `${isInsert ? 'Insert' : 'Update'} API not configured — changes are local only`,
      duration: 4000,
    });
  }

  /**
   * Execute the save operation via API
   * @param {Object} row - Tabulator row
   * @param {Object} context - Save context
   * @private
   */
  async _executeSave(row, context) {
    if (context.isInsert) {
      await this._executeInsert(row, context);
    } else {
      await this._executeUpdate(context);
    }
  }

  /**
   * Execute insert operation
   * @param {Object} row - Tabulator row
   * @param {Object} context - Save context
   * @private
   */
  async _executeInsert(row, context) {
    const payload = { ...context.rowData };
    delete payload[context.pkField];

    log(Subsystems.CONDUIT, Status.INFO, `[QueriesManager] Inserting row (queryRef: ${context.queryRef})`);

    const result = await authQuery(this.app.api, context.queryRef, { JSON: payload });

    if (result?.[0]?.[context.pkField] != null) {
      row.update({ [context.pkField]: result[0][context.pkField] });
    }
  }

  /**
   * Execute update operation
   * @param {Object} context - Save context
   * @private
   */
  async _executeUpdate(context) {
    const sqlContent = this._getCurrentSqlContent();
    const summaryContent = this._getCurrentSummaryContent();

    log(Subsystems.CONDUIT, Status.INFO, `[QueriesManager] Updating query (queryRef: ${context.queryRef}, queryId: ${context.pkValue})`);

    const params = {
      INTEGER: {
        QUERYID: context.pkValue,
        QUERYTYPE: context.rowData.query_type_a28 ?? 1,
        QUERYDIALECT: context.rowData.query_dialect_a30 ?? 1,
        QUERYSTATUS: context.rowData.query_status_a27 ?? 1,
        USERID: this.app?.user?.id ?? 0,
        QUERYREF: parseInt(context.rowData.query_ref, 10) || 0,
      },
      STRING: {
        QUERYCODE: sqlContent || context.rowData.code || '',
        QUERYNAME: context.rowData.name || '',
        QUERYSUMMARY: summaryContent || context.rowData.summary || '',
      },
    };

    await authQuery(this.app.api, context.queryRef, params);
  }

  async handleNavCancel() {
    log(Subsystems.MANAGER, Status.INFO, 'Navigator: Cancel');

    // Check if there are any changes to cancel
    if (!this._isAnyDirty()) {
      // Just exit edit mode if no edits
      await this._exitEditMode('cancel');
      return;
    }

    // Revert all changes (Tabulator + external content)
    await this._revertAllChanges();

    // Exit edit mode
    await this._exitEditMode('cancel');

    toast.info('Changes Cancelled', {
      description: 'All changes have been reverted',
      duration: 3000,
    });

    log(Subsystems.MANAGER, Status.INFO, 'Cancelled changes and reverted all edits');
  }

  async handleNavDelete() {
    log(Subsystems.MANAGER, Status.INFO, 'Navigator: Delete');
    if (!this.table) return;

    const selected = this.table.getSelectedRows();
    if (selected.length === 0) {
      toast.info('No Row Selected', {
        description: 'Please select a row to delete',
        duration: 3000,
      });
      return;
    }

    const pkField = this.primaryKeyField || 'query_id';
    const selectedId = selected[0].getData()?.[pkField];
    const rowName = selected[0].getData()?.name || '';
    const rowCount = selected.length;

    // Confirm deletion
    const confirmed = window.confirm(
      rowName
        ? `Delete "${rowName}" (${pkField}: ${selectedId})?\n\nThis action cannot be undone.`
        : `Are you sure you want to delete ${rowCount} row(s)?\n\nThis action cannot be undone.`
    );

    if (!confirmed) return;

    // Determine the delete QueryRef from config
    const deleteRef = this.queryRefs?.deleteQueryRef ?? null;

    if (deleteRef != null && selectedId != null) {
      // ── API delete ──────────────────────────────────────────────────
      try {
        log(Subsystems.CONDUIT, Status.INFO,
          `[QueriesManager] Deleting row (queryRef: ${deleteRef}, ${pkField}: ${selectedId})`);

        await authQuery(this.app.api, deleteRef, {
          INTEGER: { [pkField.toUpperCase()]: selectedId },
        });

        log(Subsystems.MANAGER, Status.INFO,
          `Deleted row via API (${pkField}: ${selectedId})`);
      } catch (error) {
        toast.error('Delete Failed', {
          serverError: error.serverError,
          subsystem: 'Conduit',
          duration: 8000,
        });
        return; // Don't remove from table if API call failed
      }
    } else if (deleteRef == null) {
      // No deleteQueryRef configured — log warning and proceed locally
      log(Subsystems.MANAGER, Status.WARN,
        '[QueriesManager] No deleteQueryRef configured — deleting locally only');
    }

    // ── Local table removal ─────────────────────────────────────────
    // Find the row to select after deletion (next, or previous, or first)
    const allRows = this.table.getRows('active');
    const currentIndex = allRows.findIndex(row => row === selected[0]);
    const nextRow = allRows[currentIndex + 1] || allRows[currentIndex - 1] || null;

    // Delete the selected rows from the local table
    selected.forEach(row => row.delete());

    // Select the next row
    if (nextRow) {
      nextRow.select();
      nextRow.scrollTo();
    }

    // Exit edit mode (delete always exits)
    await this._exitEditMode('cancel');

    toast.success('Row Deleted', {
      description: rowName ? `"${rowName}" deleted` : `${rowCount} row(s) deleted`,
      duration: 3000,
    });

    log(Subsystems.MANAGER, Status.INFO, `Deleted ${rowCount} row(s)`);
  }

  // ── Table Width Presets ─────────────────────────────────────────────────

  /**
   * Set the left panel width to a named preset based on nav block sizing.
   *
   * Width presets are derived from the navigator block dimensions:
   *   Nav block min-width: 150px, wrapper gap: 4px (--space-2),
   *   container padding: 6px left + 4px right = 10px.
   *
   * For the fixed presets (Narrow–Wide), the panel width is set directly
   * and Tabulator's built-in ResizeObserver detects the container change
   * and redraws automatically — no explicit `redraw()` call needed.
   *
   * For "Auto" mode, the table layout is temporarily switched to
   * `fitDataTable` so Tabulator sizes columns to their natural content
   * width.  After the browser paints, the rendered table width is read
   * from the DOM and used to set the panel width, then the layout is
   * restored to `fitColumns`.
   *
   * @param {'narrow'|'compact'|'normal'|'wide'|'auto'} mode
   */
  setTableWidth(mode) {
    const panel = this.elements.leftPanel;
    if (!panel) return;

    // Width presets: N blocks * 150px + (N-1) * 4px gap + 10px padding
    const widths = {
      narrow:  160,  // 1 block: 150 + 10
      compact: 314,  // 2 blocks: 300 + 4 + 10
      normal:  468,  // 3 blocks: 450 + 8 + 10
      wide:    622,  // 4 blocks: 600 + 12 + 10
    };

    this._tableWidthMode = mode;

    if (mode === 'auto') {
      this._applyAutoWidth(panel);
      return; // _applyAutoWidth handles save + log asynchronously
    }

    panel.style.width = `${widths[mode]}px`;

    // Persist to localStorage so the width survives refresh / re-login
    this._savePanelWidth(parseInt(panel.style.width, 10));

    log(Subsystems.MANAGER, Status.INFO, `Table width set to: ${mode} (${panel.style.width})`);
  }

  // ── Table Layout Mode ─────────────────────────────────────────────────

  /**
   * Change the Tabulator layout mode and persist the choice.
   *
   * Tabulator layout modes (see https://tabulator.info/docs/6.3/layout):
   *   - fitColumns     — columns stretch to fill the table width
   *   - fitData        — columns size to their data content
   *   - fitDataFill    — size to data, stretch last column to fill
   *   - fitDataStretch — size to data, stretch all columns proportionally
   *   - fitDataTable   — size to data, shrink table element to match
   *
   * After changing the layout, if the current width mode is "auto" the
   * panel width is recalculated to match the table's new natural width.
   *
   * @param {'fitColumns'|'fitData'|'fitDataFill'|'fitDataStretch'|'fitDataTable'} mode
   */
  setTableLayout(mode) {
    if (!this.table) return;

    this._tableLayoutMode = mode;
    this._saveLayoutMode(mode);

    // ── Preserve state before destroying the table ──────────────────
    const currentData = this.table.getData();
    const selectedId = this._getSelectedQueryId();

    // ── Destroy the old Tabulator instance ──────────────────────────
    // Tabulator doesn't support changing layout at runtime — the layout
    // engine is initialised once in the constructor.  Destroying and
    // recreating is the only reliable way to switch.
    this.table.destroy();
    this.table = null;

    // ── Resolve columns (same as initTable) ─────────────────────────
    const dataColumns = this.tableDef && this.coltypes
      ? resolveColumns(this.tableDef, this.coltypes, {
          filterEditor: this._createFilterEditor.bind(this),
        })
      : this._getFallbackColumns();

    // Gate cell editing on edit mode (same as initTable)
    this._applyEditModeGate(dataColumns);

    const tableOptions = this.tableDef
      ? resolveTableOptions(this.tableDef)
      : { layout: 'fitColumns', responsiveLayout: 'collapse' };
    tableOptions.layout = mode;

    // ── Selector column ─────────────────────────────────────────────
    const selectorColumn = this._buildSelectorColumn();

    // ── Create new Tabulator instance ───────────────────────────────
    this.table = new Tabulator(this.elements.tableContainer, {
      ...tableOptions,
      selectableRows: 1,
      scrollToRowPosition: "center",
      scrollToRowIfVisible: false,
      headerSortElement: '<span class="queries-sort-icons"><span class="queries-sort-asc">▲</span><span class="queries-sort-desc">▼</span></span>',
      columns: [selectorColumn, ...dataColumns],
    });

    // ── Wire table events ───────────────────────────────────────────
    this._wireTableEvents();

    // ── Restore data and selection ──────────────────────────────────
    this.table.on("tableBuilt", () => {
      this.table.setData(currentData);
      this._discoverColumns(currentData);
      requestAnimationFrame(() => {
        this._autoSelectRow(selectedId);
        // Update navigation button states after table recreation
        this._updateMoveButtonState();
      });

      // If the panel is in auto-width mode, recalculate to fit the new layout
      if (this._tableWidthMode === 'auto') {
        requestAnimationFrame(() => {
          requestAnimationFrame(() => {
            this._applyAutoWidth(this.elements.leftPanel);
          });
        });
      }
    });

    log(Subsystems.MANAGER, Status.INFO,
      `Table layout set to: ${mode} (table recreated, ${currentData.length} rows preserved)`);
  }

  /**
   * Persist the selected layout mode to localStorage.
   * @param {string} mode - Tabulator layout mode name
   */
  _saveLayoutMode(mode) {
    try {
      localStorage.setItem(LAYOUT_MODE_KEY, mode);
    } catch (_) { /* storage full or restricted — ignore */ }
  }

  /**
   * Restore the layout mode from localStorage.
   * @returns {string|null} The persisted layout mode, or null
   */
  _restoreLayoutMode() {
    try {
      const stored = localStorage.getItem(LAYOUT_MODE_KEY);
      if (stored && ['fitColumns', 'fitData', 'fitDataFill', 'fitDataStretch', 'fitDataTable'].includes(stored)) {
        return stored;
      }
    } catch (_) { /* ignore */ }
    return null;
  }

  /**
   * Apply "Auto" width by measuring the table's natural column widths.
   *
   * Strategy depends on the current layout mode:
   *   - In data-fitting modes (fitData, fitDataFill, fitDataStretch,
   *     fitDataTable) the columns are already at their natural content
   *     width — just sum them via the Tabulator column API.
   *   - In fitColumns mode the columns are stretched to fill the panel,
   *     so we must temporarily switch to fitDataTable, expand the panel
   *     so columns aren't constrained, measure, then restore.
   *
   * Uses a double requestAnimationFrame to ensure the browser has
   * painted the new layout before measuring column widths.
   *
   * @param {HTMLElement} panel - the `.queries-left-panel` element
   */
  _applyAutoWidth(panel) {
    if (!this.table) {
      panel.style.width = '314px';
      this._savePanelWidth(314);
      return;
    }

    const currentLayout = this._tableLayoutMode || 'fitColumns';
    const needsLayoutSwitch = (currentLayout === 'fitColumns');

    // Disable CSS transitions so the intermediate 2000px state is invisible
    panel.style.transition = 'none';

    if (needsLayoutSwitch) {
      // Expand the panel so columns can spread to their natural widths
      panel.style.width = '2000px';
      // Temporarily switch layout so columns size to their data content
      this.table.options.layout = 'fitDataTable';
      this.table.redraw(true);
    }

    // Double rAF: first fires after the current paint, second fires after
    // the next, guaranteeing Tabulator's redraw is fully committed to pixels.
    requestAnimationFrame(() => {
      requestAnimationFrame(() => {
        // Sum visible column widths via the Tabulator API
        let totalColWidth = 0;
        this.table.getColumns().forEach(col => {
          if (col.isVisible()) {
            totalColWidth += col.getWidth();
          }
        });

        // Fallback: if the API returned 0, measure the DOM element
        if (totalColWidth === 0) {
          const tableEl = this.elements.tableContainer?.querySelector('.tabulator-table');
          totalColWidth = tableEl?.scrollWidth || tableEl?.offsetWidth || 300;
        }

        // Account for container padding (6 + 4 = 10px), table border (2px),
        // column borders (~1px per col), and a small scrollbar buffer.
        const panelWidth = Math.max(160, totalColWidth + 28);

        // Set the final panel width
        panel.style.width = `${panelWidth}px`;

        // Restore original layout if we switched away
        if (needsLayoutSwitch) {
          this.table.options.layout = currentLayout;
          this.table.redraw(true);
        }

        // Re-enable CSS transitions after the layout has stabilised
        requestAnimationFrame(() => {
          panel.style.transition = '';
        });

        // Persist
        this._savePanelWidth(panelWidth);

        log(Subsystems.MANAGER, Status.INFO,
          `Table width set to: auto (${panelWidth}px, measured ${totalColWidth}px from columns)`);
      });
    });
  }

  /**
   * Persist the current panel width to localStorage.
   * Called after preset selection and manual splitter resize.
   * @param {number} widthPx - panel width in pixels
   */
  _savePanelWidth(widthPx) {
    try {
      localStorage.setItem(PANEL_WIDTH_KEY, String(Math.round(widthPx)));
    } catch (_) { /* storage full or restricted — ignore */ }
  }

  /**
   * Restore the panel width from localStorage on init.
   * Sets both the panel width and the width mode indicator.
   */
  _restorePanelWidth() {
    try {
      const stored = localStorage.getItem(PANEL_WIDTH_KEY);
      if (stored != null) {
        const px = parseInt(stored, 10);
        if (!isNaN(px) && px >= 100) {
          const panel = this.elements.leftPanel;
          if (panel) {
            panel.style.width = `${px}px`;
          }
          this._tableWidthMode = this._detectWidthMode(px);
          return;
        }
      }
    } catch (_) { /* ignore */ }
    // No stored value — keep default (compact / 313px from CSS)
  }

  /**
   * Detect which width mode a pixel value corresponds to.
   * Allows ±8px tolerance for each preset to account for rounding.
   * @param {number} px - panel width in pixels
   * @returns {string} mode name ('narrow', 'compact', 'normal', 'wide', or 'custom')
   */
  _detectWidthMode(px) {
    const presets = { narrow: 160, compact: 314, normal: 468, wide: 622 };
    const tolerance = 8;
    for (const [mode, target] of Object.entries(presets)) {
      if (Math.abs(px - target) <= tolerance) return mode;
    }
    return 'custom';
  }

  // ── Template System ───────────────────────────────────────────────────

  /**
   * Get template popup menu items.
   * Layout: saved templates (with delete buttons), separator, Save template, Make default
   * @returns {Array<{label: string, action: Function, isTemplate?: boolean, template?: Object, isSeparator?: boolean, icon?: string}>}
   */
  _getTemplatePopupItems() {
    const items = [];
    const templates = this._loadTemplates();
    const defaultTemplateName = this._loadDefaultTemplateName();

    // List saved templates at the TOP
    templates.forEach(template => {
      const isDefault = template.name === defaultTemplateName;
      items.push({
        label: template.name,
        action: () => this._applyTemplate(template),
        isTemplate: true,
        template,
        isDefault,
      });
    });

    // Separator between templates and functions
    if (templates.length > 0) {
      items.push({ label: '', action: () => {}, isSeparator: true });
    }

    // Function items at the BOTTOM
    items.push({ label: 'Save template...', action: () => this._showSaveTemplateDialog(), icon: 'fa-star' });
    
    if (templates.length > 0) {
      items.push({ label: 'Make default...', action: () => this._showMakeDefaultDialog(), icon: 'fa-heart' });
    }

    return items;
  }

  /**
   * Delete a template by name.
   * @param {string} templateName - Name of template to delete
   * @param {Event} e - Click event to stop propagation
   */
  _deleteTemplate(templateName, e) {
    e.stopPropagation();
    e.preventDefault();
    
    const confirmed = window.confirm(`Delete template "${templateName}"?\n\nThis action cannot be undone.`);
    if (!confirmed) return;

    const templates = this._loadTemplates();
    const filtered = templates.filter(t => t.name !== templateName);
    this._saveTemplates(filtered);

    // If this was the default, clear it
    const defaultName = this._loadDefaultTemplateName();
    if (defaultName === templateName) {
      this._saveDefaultTemplateName(null);
    }

    toast.success('Template Deleted', { description: `"${templateName}" has been deleted`, duration: 3000 });
    
    // Close and reopen popup to refresh
    this._closeNavPopup();
  }

  /**
   * Load templates from localStorage.
   * @returns {Array<{name: string, columns: Array, createdAt: string}>}
   */
  _loadTemplates() {
    try {
      const stored = localStorage.getItem(TEMPLATES_KEY);
      if (stored) {
        return JSON.parse(stored);
      }
    } catch (err) {
      log(Subsystems.MANAGER, Status.ERROR, `[QueriesManager] Failed to load templates: ${err.message}`);
    }
    return [];
  }

  /**
   * Save templates to localStorage.
   * @param {Array} templates - Array of template objects
   */
  _saveTemplates(templates) {
    try {
      localStorage.setItem(TEMPLATES_KEY, JSON.stringify(templates));
    } catch (err) {
      log(Subsystems.MANAGER, Status.ERROR, `[QueriesManager] Failed to save templates: ${err.message}`);
    }
  }

  /**
   * Load the default template name from localStorage.
   * @returns {string|null}
   */
  _loadDefaultTemplateName() {
    try {
      return localStorage.getItem(DEFAULT_TEMPLATE_KEY);
    } catch (err) {
      return null;
    }
  }

  /**
   * Save the default template name to localStorage.
   * @param {string|null} name - Template name, or null to clear default
   */
  _saveDefaultTemplateName(name) {
    try {
      if (name) {
        localStorage.setItem(DEFAULT_TEMPLATE_KEY, name);
      } else {
        localStorage.removeItem(DEFAULT_TEMPLATE_KEY);
      }
    } catch (err) {
      log(Subsystems.MANAGER, Status.ERROR, `[QueriesManager] Failed to save default template: ${err.message}`);
    }
  }

  /**
   * Show the save template dialog.
   * Prompts user for a template name and saves the current column configuration.
   */
  _showSaveTemplateDialog() {
    if (!this.table) {
      toast.info('No Table', { description: 'Table not initialized', duration: 3000 });
      return;
    }

    const dialog = document.createElement('div');
    dialog.className = 'queries-template-dialog';
    dialog.innerHTML = `
      <div class="queries-template-dialog-content">
        <h3>Save Template</h3>
        <input type="text" class="queries-template-input" placeholder="Template name..." maxlength="50">
        <div class="queries-template-dialog-buttons">
          <button type="button" class="queries-template-btn queries-template-btn-secondary" data-action="cancel">Cancel</button>
          <button type="button" class="queries-template-btn queries-template-btn-primary" data-action="save">Save</button>
        </div>
      </div>
    `;

    document.body.appendChild(dialog);

    const input = dialog.querySelector('.queries-template-input');
    const saveBtn = dialog.querySelector('[data-action="save"]');
    const cancelBtn = dialog.querySelector('[data-action="cancel"]');

    // Focus input after dialog is rendered
    requestAnimationFrame(() => input.focus());

    const closeDialog = () => {
      dialog.remove();
    };

    const handleSave = () => {
      const name = input.value.trim();
      if (!name) {
        input.classList.add('error');
        return;
      }

      this._saveTemplate(name);
      closeDialog();
    };

    saveBtn.addEventListener('click', handleSave);
    cancelBtn.addEventListener('click', closeDialog);

    input.addEventListener('keypress', (e) => {
      if (e.key === 'Enter') {
        handleSave();
      } else if (e.key === 'Escape') {
        closeDialog();
      }
    });

    // Close on backdrop click
    dialog.addEventListener('click', (e) => {
      if (e.target === dialog) {
        closeDialog();
      }
    });
  }

  /**
   * Save the current table configuration as a template.
   * Captures: column order, visibility, widths, sorting, filters, panel width, layout mode
   * @param {string} name - Template name
   */
  _saveTemplate(name) {
    if (!this.table) {
      console.log('[Template] Cannot save - no table');
      return;
    }

    const templates = this._loadTemplates();
    const existingIndex = templates.findIndex(t => t.name === name);

    // Get all columns in their current order with full configuration
    const allColumns = this.table.getColumns();
    const columnConfig = allColumns
      .filter(col => col.getField() && col.getField() !== '_selector')
      .map((col, index) => ({
        field: col.getField(),
        visible: col.isVisible(),
        width: col.getWidth(),
        position: index,
      }));

    console.log('[Template] Saving columns:', columnConfig);

    // Get current sorters
    const sorters = this.table.getSorters().map(s => ({
      field: s.field,
      dir: s.dir,
    }));
    console.log('[Template] Saving sorters:', sorters);

    // Get header filters - Tabulator stores these in the column definitions
    const filters = [];
    allColumns.forEach(col => {
      const field = col.getField();
      if (!field || field === '_selector') return;
      
      // Try to get filter value from the column's filter element
      const colDef = col.getDefinition();
      const filterVal = this.table.getHeaderFilterValue?.(field);
      if (filterVal !== undefined && filterVal !== '' && filterVal !== null) {
        filters.push({ field, value: filterVal });
      }
    });
    console.log('[Template] Saving filters:', filters);

    // Get panel width
    const panelWidth = this.elements.leftPanel?.offsetWidth || 314;
    console.log('[Template] Saving panel width:', panelWidth);

    // Get layout mode
    const layoutMode = this._tableLayoutMode || 'fitColumns';
    console.log('[Template] Saving layout mode:', layoutMode);

    // Get filters visibility state
    const filtersVisible = this._filtersVisible || false;
    console.log('[Template] Saving filters visible:', filtersVisible);

    const template = {
      name,
      columns: columnConfig,
      sorters,
      filters,
      panelWidth,
      layoutMode,
      filtersVisible,
      createdAt: new Date().toISOString(),
    };

    console.log('[Template] Full template object:', template);

    if (existingIndex >= 0) {
      templates[existingIndex] = template;
      toast.success('Template Updated', { description: `"${name}" has been updated`, duration: 3000 });
    } else {
      templates.push(template);
      toast.success('Template Saved', { description: `"${name}" has been saved`, duration: 3000 });
    }

    this._saveTemplates(templates);
    log(Subsystems.MANAGER, Status.INFO, `[QueriesManager] Template saved: ${name} (${columnConfig.length} columns)`);
  }

  /**
   * Show the make default dialog.
   * Lists all templates to choose which one should be the default.
   */
  _showMakeDefaultDialog() {
    const templates = this._loadTemplates();
    const currentDefault = this._loadDefaultTemplateName();

    if (templates.length === 0) {
      toast.info('No Templates', { description: 'Save a template first', duration: 3000 });
      return;
    }

    const dialog = document.createElement('div');
    dialog.className = 'queries-template-dialog';

    const templateOptions = templates.map(t => {
      const isDefault = t.name === currentDefault;
      return `<option value="${t.name}" ${isDefault ? 'selected' : ''}>${t.name}${isDefault ? ' (current default)' : ''}</option>`;
    }).join('');

    dialog.innerHTML = `
      <div class="queries-template-dialog-content">
        <h3>Set Default Template</h3>
        <select class="queries-template-select">
          <option value="">-- No Default --</option>
          ${templateOptions}
        </select>
        <div class="queries-template-dialog-buttons">
          <button type="button" class="queries-template-btn queries-template-btn-secondary" data-action="cancel">Cancel</button>
          <button type="button" class="queries-template-btn queries-template-btn-primary" data-action="set">Set Default</button>
        </div>
      </div>
    `;

    document.body.appendChild(dialog);

    const select = dialog.querySelector('.queries-template-select');
    const setBtn = dialog.querySelector('[data-action="set"]');
    const cancelBtn = dialog.querySelector('[data-action="cancel"]');

    const closeDialog = () => {
      dialog.remove();
    };

    setBtn.addEventListener('click', () => {
      const selectedName = select.value;
      if (selectedName) {
        this._saveDefaultTemplateName(selectedName);
        toast.success('Default Set', { description: `"${selectedName}" is now the default template`, duration: 3000 });
      } else {
        this._saveDefaultTemplateName(null);
        toast.info('Default Cleared', { description: 'No default template set', duration: 3000 });
      }
      closeDialog();
    });

    cancelBtn.addEventListener('click', closeDialog);

    // Close on backdrop click
    dialog.addEventListener('click', (e) => {
      if (e.target === dialog) {
        closeDialog();
      }
    });
  }

  /**
   * Apply a template to the current table.
   * Restores: column order, visibility, widths, sorting, filters, panel width, layout mode
   * @param {Object} template - Template object with full configuration
   */
  _applyTemplate(template) {
    console.log('[Template] Applying template:', template?.name);
    
    if (!this._validateTemplate(template)) return;

    try {
      this._applyTemplateLayout(template);
      this._applyTemplateColumns(template);
      this._applyTemplateFilters(template);
      
      this.table.redraw(true);
      toast.success('Template Applied', { description: `"${template.name}" has been applied`, duration: 3000 });
      log(Subsystems.MANAGER, Status.INFO, `[QueriesManager] Template applied: ${template.name}`);
    } catch (err) {
      this._handleTemplateError(err);
    }
  }

  /**
   * Validate template before applying
   * @param {Object} template - Template to validate
   * @returns {boolean} True if valid
   * @private
   */
  _validateTemplate(template) {
    if (!this.table || !template) {
      console.error('[Template] Cannot apply - no table or template');
      toast.error('Template Error', { description: 'Failed to apply template', duration: 3000 });
      return false;
    }

    if (!template.columns || template.columns.length === 0) {
      console.error('[Template] Template has no columns:', template);
      toast.error('Template Error', { description: 'Template has no column data', duration: 3000 });
      return false;
    }
    return true;
  }

  /**
   * Apply layout and panel settings from template
   * @param {Object} template - Template object
   * @private
   */
  _applyTemplateLayout(template) {
    if (template.layoutMode && template.layoutMode !== this._tableLayoutMode) {
      this.setTableLayout(template.layoutMode);
    }

    if (template.panelWidth && this.elements.leftPanel) {
      this.elements.leftPanel.style.width = `${template.panelWidth}px`;
      this._savePanelWidth(template.panelWidth);
      this._tableWidthMode = this._detectWidthMode(template.panelWidth);
    }
  }

  /**
   * Apply column configuration from template
   * @param {Object} template - Template object
   * @private
   */
  _applyTemplateColumns(template) {
    if (this.table.blockRedraw) {
      this.table.blockRedraw();
    }

    try {
      const sortedColumns = [...template.columns].sort((a, b) => a.position - b.position);
      this._reorderColumns(sortedColumns);
      
      if (template.sorters?.length > 0) {
        this.table.setSort(template.sorters);
      }
    } finally {
      if (this.table.restoreRedraw) {
        this.table.restoreRedraw();
      }
    }
  }

  /**
   * Reorder columns according to template
   * @param {Array} sortedColumns - Sorted column configurations
   * @private
   */
  _reorderColumns(sortedColumns) {
    for (let i = sortedColumns.length - 1; i >= 0; i--) {
      const colConfig = sortedColumns[i];
      const col = this.table.getColumn(colConfig.field);
      
      if (!col) {
        console.warn(`[Template] Column not found: ${colConfig.field}`);
        continue;
      }

      // Move column if needed
      if (i > 0) {
        const afterCol = this.table.getColumn(sortedColumns[i - 1].field);
        if (afterCol) {
          this.table.moveColumn(col, afterCol, false);
        }
      }
      
      // Apply visibility
      colConfig.visible ? col.show() : col.hide();
      
      // Apply width
      if (colConfig.width) {
        col.setWidth(colConfig.width);
      }
    }
  }

  /**
   * Apply filters from template
   * @param {Object} template - Template object
   * @private
   */
  _applyTemplateFilters(template) {
    if (template.filtersVisible !== undefined && template.filtersVisible !== this._filtersVisible) {
      this.toggleColumnFilters();
    }

    if (template.filters?.length > 0) {
      template.filters.forEach(filter => {
        this.table.setHeaderFilterValue?.(filter.field, filter.value);
      });
    }
  }

  /**
   * Handle template application error
   * @param {Error} err - Error object
   * @private
   */
  _handleTemplateError(err) {
    if (this.table?.restoreRedraw) {
      this.table.restoreRedraw();
    }
    console.error('[Template] Error applying template:', err);
    toast.error('Template Error', { description: err.message, duration: 3000 });
    log(Subsystems.MANAGER, Status.ERROR, `[QueriesManager] Failed to apply template: ${err.message}`);
  }

  /**
   * Apply the default template if one is set.
   * Called during table initialization.
   */
  _applyDefaultTemplate() {
    const defaultName = this._loadDefaultTemplateName();
    if (!defaultName) return;

    const templates = this._loadTemplates();
    const defaultTemplate = templates.find(t => t.name === defaultName);

    if (defaultTemplate) {
      // Defer application until table is fully built
      this.table.on('tableBuilt', () => {
        this._applyTemplate(defaultTemplate);
      });
    }
  }

  handleNavEmail() {
    log(Subsystems.MANAGER, Status.INFO, 'Navigator: Email');
    if (!this.table) return;

    const rows = this.table.getRows('active');
    if (rows.length === 0) {
      toast.info('No Data', {
        description: 'No rows to include in the email',
        duration: 3000,
      });
      return;
    }

    // Build a plain-text table summary from visible columns
    const visibleCols = this.table.getColumns()
      .filter(col => col.isVisible() && col.getField() !== '_selector');

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

    const subject = encodeURIComponent(`${this.tableDef?.title || 'Query Manager'} — ${totalRows} rows`);
    const body = encodeURIComponent(
      `${this.tableDef?.title || 'Query Manager'} Export\n` +
      `${totalRows} row(s)\n\n` +
      `${headers.join('\t')}\n${separator}\n` +
      `${dataLines.join('\n')}${truncated}\n`
    );

    window.open(`mailto:?subject=${subject}&body=${body}`, '_self');
    log(Subsystems.MANAGER, Status.INFO, `Email prepared with ${totalRows} row(s)`);
  }

  /**
   * Handle export to a specific format.
   * @param {'pdf'|'csv'|'txt'|'xls'} format
   */
  handleExport(format) {
    log(Subsystems.MANAGER, Status.INFO, `Navigator: Export as ${format.toUpperCase()}`);
    if (!this.table) return;

    const filename = `queries-export-${new Date().toISOString().slice(0, 10)}`;

    switch (format) {
      case 'pdf':
        this.table.download('pdf', `${filename}.pdf`, { orientation: 'landscape' });
        break;
      case 'csv':
        this.table.download('csv', `${filename}.csv`);
        break;
      case 'txt':
        this.table.download('csv', `${filename}.txt`); // Tabulator uses CSV format for .txt
        break;
      case 'xls':
        this.table.download('xlsx', `${filename}.xlsx`);
        break;
      default:
        log(Subsystems.MANAGER, Status.WARN, `Unknown export format: ${format}`);
    }
  }

  /**
   * Handle import from a file.
   * @param {'csv'|'txt'|'xls'} format
   */
  handleImport(format) {
    log(Subsystems.MANAGER, Status.INFO, `Navigator: Import from ${format.toUpperCase()}`);
    // Create file input programmatically
    const input = document.createElement('input');
    input.type = 'file';
    input.accept = `.${format}`;

    input.addEventListener('change', async (e) => {
      const file = e.target.files?.[0];
      if (!file) return;

      try {
        // Tabulator import
        await this.table.import(format, file.name);
        log(Subsystems.MANAGER, Status.INFO, `Imported ${format.toUpperCase()} successfully`);
      } catch (err) {
        log(Subsystems.MANAGER, Status.ERROR, `Import failed: ${err.message}`);
        toast.error('Import Failed', {
          description: err.message,
          subsystem: 'Manager',
          duration: 5000,
        });
      }
    });

    input.click();
  }


  // ── Slot footer setup ─────────────────────────────────────────────────

  /**
   * Customise the query manager's slot footer.
   *
   * Replaces the generic "Reports Placeholder" button with:
   *   1. Print button
   *   2. Email button
   *   3. Export button (with popup for format selection)
   *   4. Data-source select: "Query List View" / "Query List Data"
   *
   * All elements are injected as direct children of the footer's
   * `.subpanel-header-group` flex container so they integrate seamlessly
   * into the unified button strip (same sizing, gap, corner rounding).
   *
   * The data-source `<select>` gets `flex:1` to fill all remaining space
   * between the new buttons and the pre-existing right-side buttons
   * (notifications, tickets, annotations, tours, LMS).
   *
   * The slot element is found by walking up from the workspace container
   * that was passed by the app when the manager was created.
   */
  _setupFooter() {
    // Walk up to the .manager-slot from our workspace container
    const slot = this.container.closest('.manager-slot');
    if (!slot) return;

    const footer = slot.querySelector('.manager-slot-footer');
    if (!footer) return;

    // Find the subpanel-header-group — the unified flex container
    const group = footer.querySelector('.subpanel-header-group');
    if (!group) return;

    // Remove the generic "Reports Placeholder" button
    const reportsBtn = group.querySelector('.slot-reports-btn');
    if (reportsBtn) reportsBtn.remove();

    // Identify the first right-side fixed button to use as insertion anchor
    const anchor = group.querySelector('.slot-notifications-btn');

    // --- Helper: create a button matching the footer style ---
    const makeBtn = (id, icon, title) => {
      const btn = document.createElement('button');
      btn.type = 'button';
      btn.className = 'subpanel-header-btn subpanel-header-close';
      btn.id = id;
      btn.title = title;
      btn.innerHTML = `<fa ${icon}></fa>`;
      return btn;
    };

    // 1. Print button
    const printBtn = makeBtn('queries-footer-print', 'fa-print', 'Print');
    group.insertBefore(printBtn, anchor);

    // 2. Email button
    const emailBtn = makeBtn('queries-footer-email', 'fa-paper-plane', 'E-Mail');
    group.insertBefore(emailBtn, anchor);

    // 3. Export button (with popup)
    const exportBtn = makeBtn('queries-footer-export', 'fa-file-circle-question', 'Export');
    group.insertBefore(exportBtn, anchor);

    // 4. Data-source select — intrinsic width based on content
    const select = document.createElement('select');
    select.id = 'queries-footer-datasource';
    select.title = 'Data Source';
    select.className = 'queries-footer-datasource';
    select.innerHTML = `
      <option value="view">Query List View</option>
      <option value="data">Query List Data</option>
    `;
    group.insertBefore(select, anchor);

    // 5. Filler button — spans the gap between select and right-side buttons
    const fillerBtn = document.createElement('button');
    fillerBtn.type = 'button';
    fillerBtn.className = 'subpanel-header-btn queries-footer-filler';
    fillerBtn.title = 'Reports';
    group.insertBefore(fillerBtn, anchor);

    // Process <fa> icons inside the newly inserted buttons
    processIcons(group);

    // Wire up event handlers
    printBtn.addEventListener('click', () => this._handleFooterPrint());
    emailBtn.addEventListener('click', () => this._handleFooterEmail());
    exportBtn.addEventListener('click', (e) => this._toggleFooterExportPopup(e));

    // Store reference to the datasource select for use by export/print/email
    this._footerDatasource = select;

    log(Subsystems.MANAGER, Status.INFO, '[QueriesManager] Footer controls initialized');
  }

  // ── Footer action handlers ────────────────────────────────────────────

  /**
   * Get the selected data source mode from the footer combobox.
   * @returns {'view'|'data'} - 'view' = current table rendering, 'data' = raw JSON data
   */
  _getFooterDatasource() {
    return this._footerDatasource?.value || 'view';
  }

  /**
   * Handle Print from the footer.
   * 'view' mode: print the table as currently rendered (sort/filter/group).
   * 'data' mode: print from the raw data (no sorting/filtering/grouping applied).
   */
  _handleFooterPrint() {
    const mode = this._getFooterDatasource();
    log(Subsystems.MANAGER, Status.INFO, `Footer: Print (${mode})`);
    if (!this.table) return;

    if (mode === 'view') {
      // Print the table as currently rendered
      this.table.print();
    } else {
      // Print from raw data — use 'all' style to bypass current filters/sort
      this.table.print('all', true);
    }
  }

  /**
   * Handle Email from the footer.
   * 'view' mode: email using visible/filtered rows with current column visibility.
   * 'data' mode: email using all loaded data in native format.
   */
  _handleFooterEmail() {
    const mode = this._getFooterDatasource();
    log(Subsystems.MANAGER, Status.INFO, `Footer: Email (${mode})`);
    if (!this.table) return;

    const rows = mode === 'view'
      ? this.table.getRows('active')
      : this.table.getRows();

    if (rows.length === 0) {
      toast.info('No Data', {
        description: 'No rows to include in the email',
        duration: 3000,
      });
      return;
    }

    // Build a plain-text table summary
    const visibleCols = mode === 'view'
      ? this.table.getColumns().filter(col => col.isVisible() && col.getField() !== '_selector')
      : this.table.getColumns().filter(col => col.getField() !== '_selector');

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
    const modeLabel = mode === 'view' ? 'Filtered View' : 'Full Data';

    const subject = encodeURIComponent(`${this.tableDef?.title || 'Query Manager'} — ${totalRows} rows (${modeLabel})`);
    const body = encodeURIComponent(
      `${this.tableDef?.title || 'Query Manager'} Export (${modeLabel})\n` +
      `${totalRows} row(s)\n\n` +
      `${headers.join('\t')}\n${separator}\n` +
      `${dataLines.join('\n')}${truncated}\n`
    );

    window.open(`mailto:?subject=${subject}&body=${body}`, '_self');
    log(Subsystems.MANAGER, Status.INFO, `Footer email prepared with ${totalRows} row(s) (${mode})`);
  }

  /**
   * Toggle the export format popup from the footer export button.
   * @param {MouseEvent} e - click event
   */
  _toggleFooterExportPopup(e) {
    e.stopPropagation();

    // If already open, close it
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

    // Build popup
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

    // Position above the button
    const slot = this.container.closest('.manager-slot');
    const footer = slot?.querySelector('.manager-slot-footer');
    if (footer) {
      footer.style.position = 'relative';
      const btnRect = btn.getBoundingClientRect();
      const footerRect = footer.getBoundingClientRect();
      popup.style.left = `${btnRect.left - footerRect.left}px`;
      footer.appendChild(popup);
    }

    // Animate in
    popup.getBoundingClientRect();
    popup.classList.add('visible');

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
   * @param {'view'|'data'} mode
   */
  _handleFooterExport(format, mode) {
    log(Subsystems.MANAGER, Status.INFO, `Footer: Export ${format.toUpperCase()} (${mode})`);
    if (!this.table) return;

    const filename = `queries-export-${new Date().toISOString().slice(0, 10)}`;

    // For 'data' mode, we temporarily clear filters to export all loaded data
    // For 'view' mode, we export what's currently visible
    const downloadOpts = mode === 'data' ? { rowGroups: false } : {};

    switch (format) {
      case 'pdf':
        this.table.download('pdf', `${filename}.pdf`, { orientation: 'landscape', ...downloadOpts });
        break;
      case 'csv':
        this.table.download('csv', `${filename}.csv`, downloadOpts);
        break;
      case 'txt':
        this.table.download('csv', `${filename}.txt`, downloadOpts);
        break;
      case 'xls':
        this.table.download('xlsx', `${filename}.xlsx`, downloadOpts);
        break;
      default:
        log(Subsystems.MANAGER, Status.WARN, `Unknown export format: ${format}`);
    }
  }

  // ── Menu popup handlers ───────────────────────────────────────────────

  handleMenuExpandAll() {
    log(Subsystems.MANAGER, Status.INFO, 'Navigator: Expand All');
    if (!this.table) return;

    // Tabulator's group expand/collapse API
    const groups = this.table.getGroups();
    if (groups.length > 0) {
      groups.forEach(group => group.show());
    }

    // Also expand any responsive-collapsed rows
    const rows = this.table.getRows('active');
    rows.forEach(row => {
      try {
        const el = row.getElement();
        if (el?.classList.contains('tabulator-responsive-collapse')) {
          row.treeExpand?.();
        }
      } catch (_) { /* not all rows support tree ops */ }
    });
  }

  handleMenuCollapseAll() {
    log(Subsystems.MANAGER, Status.INFO, 'Navigator: Collapse All');
    if (!this.table) return;

    // Tabulator's group collapse API
    const groups = this.table.getGroups();
    if (groups.length > 0) {
      groups.forEach(group => group.hide());
    }

    // Also collapse any responsive-expanded rows
    const rows = this.table.getRows('active');
    rows.forEach(row => {
      try {
        row.treeCollapse?.();
      } catch (_) { /* not all rows support tree ops */ }
    });
  }

  /**
   * Toggle column filter row visibility in the table header.
   * When hidden, clears all active header filters AND resets
   * custom filter input values and clear-button visibility.
   */
  /**
   * Handle filter button click - toggle column filters visibility
   */
  handleNavFilter() {
    log(Subsystems.MANAGER, Status.INFO, 'Navigator: Toggle Filters');
    this.toggleColumnFilters();
  }

  toggleColumnFilters() {
    this._filtersVisible = !this._filtersVisible;

    // Update the filter button's active state
    const filterBtn = this.elements.navigatorContainer?.querySelector('#queries-nav-filter');
    if (filterBtn) {
      filterBtn.classList.toggle('queries-nav-btn-active', this._filtersVisible);
    }

    if (this._filtersVisible) {
      this.elements.tableContainer.classList.add('queries-filters-visible');
    } else {
      this.elements.tableContainer.classList.remove('queries-filters-visible');
      // Clear Tabulator's internal filter state
      if (this.table) {
        this.table.clearHeaderFilter();
      }
      // Reset custom filter inputs and hide clear buttons
      this.elements.tableContainer
        .querySelectorAll('.tabulator-header-filter input')
        .forEach(input => { input.value = ''; });
      this.elements.tableContainer
        .querySelectorAll('.queries-header-filter-clear')
        .forEach(btn => { btn.style.display = 'none'; });
    }
    // Force Tabulator to recalculate header height for the filter row
    if (this.table) {
      this.table.redraw(true);
    }
    log(Subsystems.MANAGER, Status.INFO,
      `Column filters ${this._filtersVisible ? 'shown' : 'hidden'}`);
  }

  /**
   * Custom header-filter editor with an inline clear (×) button.
   *
   * Returns a plain <input> element (which Tabulator expects) and
   * injects a positioned clear button via onRendered() once the
   * input is placed in the DOM.
   *
   * @param {Object}   cell         - Tabulator header-filter cell component
   * @param {Function} onRendered   - callback fired when element is in the DOM
   * @param {Function} success      - call with the new filter value
   * @param {Function} cancel       - call to cancel / clear
   * @param {Object}   editorParams - extra params (e.g. { placeholder })
   * @returns {HTMLElement} the <input> element
   */
  _createFilterEditor(cell, onRendered, success, cancel, editorParams) {
    const input = document.createElement('input');
    input.type = 'text';
    input.placeholder = editorParams?.placeholder || 'filter...';
    input.className = 'queries-header-filter-input';

    // Reference to the clear button (set in onRendered)
    let clearBtn = null;

    const updateClearBtn = () => {
      if (clearBtn) {
        clearBtn.style.display = input.value ? 'flex' : 'none';
      }
    };

    // Live-filter on every keystroke
    input.addEventListener('input', () => {
      updateClearBtn();
      success(input.value);
    });

    // Escape clears the filter
    input.addEventListener('keydown', (e) => {
      if (e.key === 'Escape') {
        input.value = '';
        updateClearBtn();
        success('');
        e.stopPropagation();
      }
    });

    // After the input is in the DOM, inject a clear (×) button as a sibling
    onRendered(() => {
      const parent = input.parentElement;
      if (!parent) return;
      parent.style.position = 'relative';

      clearBtn = document.createElement('span');
      clearBtn.className = 'queries-header-filter-clear';
      clearBtn.innerHTML = '&times;';
      clearBtn.title = 'Clear filter';
      clearBtn.style.display = input.value ? 'flex' : 'none';

      clearBtn.addEventListener('click', (e) => {
        e.stopPropagation();
        input.value = '';
        clearBtn.style.display = 'none';
        success('');
      });

      parent.appendChild(clearBtn);
    });

    return input;
  }

  /**
   * Fallback column definitions used when JSON config fails to load.
   * Provides the original hardcoded columns so the table still works.
   * @returns {Array<Object>} Tabulator column definitions
   */
  _getFallbackColumns() {
    log(Subsystems.MANAGER, Status.WARN,
      '[QueriesManager] Using fallback columns — JSON config not available');
    return [
      {
        title: "Ref",
        field: "query_ref",
        width: 80,
        resizable: true,
        headerSort: true,
        headerFilter: this._createFilterEditor.bind(this),
        headerFilterFunc: "like",
        headerFilterParams: { placeholder: "filter..." },
        bottomCalc: "count",
        bottomCalcFormatter: (cell) => {
          const val = cell.getValue();
          return val != null ? Number(val).toLocaleString() : '';
        },
      },
      {
        title: "Name",
        field: "name",
        resizable: true,
        headerSort: true,
        headerFilter: this._createFilterEditor.bind(this),
        headerFilterFunc: "like",
        headerFilterParams: { placeholder: "filter..." },
      },
    ];
  }

  /**
   * Discover additional columns from the loaded data and add them
   * as hidden columns, so the column chooser can list all available fields.
   * Only discovers fields not already defined in the JSON config.
   * Runs after setData(); idempotent (skips fields already defined).
   * @param {Array} rows - data rows from the API
   */
  _discoverColumns(rows) {
    if (!this.table || !rows || rows.length === 0) return;

    // Collect all unique keys from the data
    const allKeys = new Set();
    rows.forEach(row => {
      Object.keys(row).forEach(key => allKeys.add(key));
    });

    // Get already-defined column fields
    const existingFields = new Set(
      this.table.getColumns().map(col => col.getField()).filter(Boolean)
    );

    // Add hidden columns for newly discovered fields
    for (const key of allKeys) {
      if (existingFields.has(key) || key === '_selector') continue;

      // Generate a human-readable title from the field name
      const title = key
        .replace(/_/g, ' ')
        .replace(/\b\w/g, c => c.toUpperCase());

      this.table.addColumn({
        title,
        field: key,
        resizable: true,
        headerSort: true,
        headerFilter: this._createFilterEditor.bind(this),
        headerFilterFunc: "like",
        headerFilterParams: { placeholder: "filter..." },
        visible: false,
      });
    }
  }

  toggleCollapse() {
    const isCollapsed = this.elements.leftPanel.classList.toggle('collapsed');
    this.elements.splitter.classList.toggle('collapsed', isCollapsed);

    // Rotate the collapse arrow icon.
    // Font Awesome replaces <fa> → <i> → <svg>, so cached references go stale.
    // Always use a fresh DOM query (same pattern as main.js _getArrowEl).
    const iconEl = this._getCollapseIconEl();
    if (iconEl) {
      this._arrowRotation += 180;
      iconEl.style.transform = `rotate(${this._arrowRotation}deg)`;
    }
  }

  /**
   * Get the collapse arrow icon element via a fresh DOM query.
   * Font Awesome replaces <fa> → <i> → <svg>, so cached references go stale.
   * @returns {HTMLElement|null}
   */
  _getCollapseIconEl() {
    return document.querySelector('#queries-collapse-icon') ||
           document.querySelector('#queries-collapse-btn')?.firstElementChild;
  }

  switchTab(tabId) {
    // Update buttons
    this.elements.tabBtns.forEach(btn => {
      btn.classList.toggle('active', btn.dataset.tab === tabId);
    });

    // Update panes
    this.elements.tabPanes.forEach(pane => {
      pane.classList.toggle('active', pane.id === `queries-tab-${tabId}`);
    });
    
    // Enable/disable toolbar buttons based on tab
    const isSqlTab = tabId === 'sql';
    const buttons = [this.elements.undoBtn, this.elements.redoBtn, this.elements.fontBtn];
    buttons.forEach(btn => {
      if (btn) {
        btn.disabled = !isSqlTab;
      }
    });
    
    // Prettify button should be enabled when SQL tab is visible
    if (this.elements.prettifyBtn) {
      this.elements.prettifyBtn.disabled = !isSqlTab;
    }
    
    // Initialize editors when switching to their tabs
    if (tabId === 'sql') {
      const sqlContent = this._pendingSqlContent || this.currentQuery?.code || this.currentQuery?.query_text || this.currentQuery?.sql || '';
      if (!this.sqlEditor) {
        this.initSqlEditor(sqlContent);
      }
    }
    
    // Initialize summary editor if needed
    if (tabId === 'summary') {
      const summaryContent = this._pendingSummaryContent || this.currentQuery?.summary || this.currentQuery?.markdown || '';
      if (!this.summaryEditor) {
        this.initSummaryEditor(summaryContent);
      }
    }
    
    // Initialize collection editor if needed
    if (tabId === 'collection') {
      let collectionContent = this._pendingCollectionContent || this.currentQuery?.collection || this.currentQuery?.json || {};
      if (!this.collectionEditor) {
        this.initCollectionEditor(collectionContent);
      }
    }
    
    // Render preview if needed
    if (tabId === 'preview') {
      this.renderMarkdownPreview();
    }
  }

  async initTable() {
    // ── Load JSON-driven column configuration ──────────────────────────
    // Schemas are loaded from the lookup cache (loaded at app init) or
    // fetched from /config/tabulator/ as fallback. No provided data means
    // the loaders will check lookups first, then fall back to fetch.
    [this.coltypes, this.tableDef] = await Promise.all([
      loadColtypes(),
      loadTableDef('queries/query-manager'),
    ]);

    if (this.tableDef) {
      this.queryRefs = getQueryRefs(this.tableDef);
      this.primaryKeyField = getPrimaryKeyField(this.tableDef);

      // ── Pre-load lookup tables referenced by columns ────────────────
      // Collect all unique lookupRef values from the column definitions
      // and pre-fetch them so resolveColumn() can wire up formatters/editors.
      const lookupRefs = Object.values(this.tableDef.columns || {})
        .map(col => col.lookupRef)
        .filter(Boolean);
      const uniqueRefs = [...new Set(lookupRefs)];

      if (uniqueRefs.length > 0 && this.app?.api) {
        try {
          await preloadLookups(uniqueRefs, authQuery, this.app.api);
          log(Subsystems.MANAGER, Status.INFO,
            `[QueriesManager] Pre-loaded ${uniqueRefs.length} lookup table(s): ${uniqueRefs.join(', ')}`);
        } catch (err) {
          log(Subsystems.MANAGER, Status.WARN,
            `[QueriesManager] Lookup pre-load failed (non-fatal): ${err.message}`);
        }
      }
    }

    // Resolve columns from JSON config → Tabulator column definitions
    const dataColumns = this.tableDef && this.coltypes
      ? resolveColumns(this.tableDef, this.coltypes, {
          filterEditor: this._createFilterEditor.bind(this),
        })
      : this._getFallbackColumns();

    // Gate cell editing on edit mode — columns that have an editor assigned
    // by resolveColumn() should only be editable when _editingRowId is set.
    // This ensures the table is read-only by default and cells only become
    // editable when the user explicitly enters edit mode.
    this._applyEditModeGate(dataColumns);

    // Resolve table-level options from the tabledef
    const tableOptions = this.tableDef
      ? resolveTableOptions(this.tableDef)
      : { layout: 'fitColumns', responsiveLayout: 'collapse' };

    // Restore persisted layout mode (overrides the JSON config default).
    // This lets the user's last-chosen layout survive refresh / re-login.
    const persistedLayout = this._restoreLayoutMode();
    if (persistedLayout) {
      tableOptions.layout = persistedLayout;
    }
    this._tableLayoutMode = tableOptions.layout || 'fitColumns';

    // Build the selector column and create the Tabulator instance
    const selectorColumn = this._buildSelectorColumn();

    log(Subsystems.MANAGER, Status.INFO,
      `[QueriesManager] Initialising table with ${dataColumns.length} columns from JSON config`);

    this.table = new Tabulator(this.elements.tableContainer, {
      ...tableOptions,
      selectableRows: 1,
      scrollToRowPosition: "center",
      scrollToRowIfVisible: false,
      headerSortElement: '<span class="queries-sort-icons"><span class="queries-sort-asc">▲</span><span class="queries-sort-desc">▼</span></span>',
      columns: [selectorColumn, ...dataColumns],
    });

    // Wire up table event handlers (shared with setTableLayout recreation)
    this._wireTableEvents();

    // Apply default template if one is set
    this._applyDefaultTemplate();

    // Keyboard handling on the table container for navigation + Enter to select
    this.elements.tableContainer.setAttribute('tabindex', '0');
    this.elements.tableContainer.addEventListener('keydown', (e) => {
      this._handleTableKeydown(e);
    });

    // Load initial data asynchronously — the table will update when the
    // API response arrives; the manager is considered "loaded" immediately.
    this.loadQueries();
  }

  /**
   * Build the selector column definition (row indicator + column chooser header).
   * Extracted as a reusable method so both initTable() and setTableLayout()
   * can construct the same column without duplicating the definition.
   * @returns {Object} Tabulator column definition
   */
  _buildSelectorColumn() {
    return {
      title: "",
      field: "_selector",
      frozen: true,
      width: 16,
      minWidth: 16,
      maxWidth: 16,
      resizable: false,
      headerSort: false,
      hozAlign: "center",
      vertAlign: "middle",
      cssClass: "queries-selector-col",
      titleFormatter: () => {
        const wrapper = document.createElement('span');
        wrapper.className = 'queries-col-chooser-btn';
        wrapper.title = 'Column Chooser';
        wrapper.innerHTML = '<fa fa-ellipsis-stroke-vertical>';
        return wrapper;
      },
      formatter: (cell) => {
        const row = cell.getRow();
        const pkField = this.primaryKeyField || 'query_id';
        if (row.isSelected()) {
          if (this._isEditing && this._editingRowId === row.getData()[pkField]) {
            return '<fa fa-i-cursor fa-swap-opacity queries-selector-indicator queries-selector-edit>';
          }
          return '<fa fa-caret-right fa-swap-opacity queries-selector-indicator queries-selector-active>';
        }
        return '';
      },
      headerClick: (e, column) => {
        this.toggleColumnChooser(e, column);
      },
      cellClick: (e, cell) => {
        e?.stopPropagation?.();
        e?.stopImmediatePropagation?.();
        const row = cell.getRow();
        this._selectDataRow(row);
      },
    };
  }

  /**
   * Wire up Tabulator event handlers on the current table instance.
   * Extracted as a reusable method so both initTable() and setTableLayout()
   * can wire the same events without duplicating handler definitions.
   *
   * **Design note — row selection strategy:**
   * Editors are stripped from column definitions by `_applyEditModeGate()`
   * and only re-applied dynamically when entering edit mode via
   * `_enableColumnEditors()`. This means Tabulator never has editor-
   * related click handlers on cells in normal (non-edit) mode, so
   * `rowClick` fires reliably for row selection on all cells.
   *
   * In edit mode, `cellClick` programmatically invokes `cell.edit()`
   * on the editing row's editable cells, giving the user immediate
   * inline editing when clicking any field.
   */
  _wireTableEvents() {
    if (!this.table) return;

    this._bindRowClickEvents();
    this._bindCellClickEvents();
    this._bindEditEvents();
    this._bindSelectionEvents();
  }

  /**
   * Bind row-level click events
   * @private
   */
  _bindRowClickEvents() {
    // Row click — primary selection handler
    this.table.on("rowClick", (e, row) => {
      if (this._isCalcRow(row)) return;
      this._selectDataRow(row);
    });

    // Ensure row selection happens consistently before any cell editor logic
    this.table.on("cellMouseDown", (e, cell) => {
      if (cell.getField() === '_selector' || this._isCalcCell(cell)) return;
      this._selectDataRow(cell.getRow());
    });

    // Double-click — enter edit mode
    this.table.on("rowDblClick", (e, row) => {
      if (this._isCalcRow(row)) return;
      this._selectDataRow(row);
      void this._enterEditMode(row);
    });
  }

  /**
   * Bind cell-level click events
   * @private
   */
  _bindCellClickEvents() {
    this.table.on("cellClick", (e, cell) => {
      const field = cell.getField();
      if (field === '_selector' || this._isCalcCell(cell)) return;

      e?.stopPropagation?.();
      e?.stopImmediatePropagation?.();

      this._selectDataRow(cell.getRow());

      if (this._isEditing) {
        this._commitActiveCellEdit(cell);
        this._queueCellEdit(cell);
      }
    });
  }

  /**
   * Bind edit-related events
   * @private
   */
  _bindEditEvents() {
    // Cell edit tracking for dirty state
    this.table.on("cellEdited", () => {
      this._setDirty('table', true);
    });
  }

  /**
   * Bind selection-related events
   * @private
   */
  _bindSelectionEvents() {
    // Row selected
    this.table.on("rowSelected", async (row) => {
      if (this._isCalcRow(row)) return;
      this._closeTransientPopups();
      await this._handleRowSelected(row);
    });

    // Row deselected
    this.table.on("rowDeselected", (row) => {
      if (this._isCalcRow(row)) return;
      this._updateSelectorCell(row, false);
    });

    // Selection changed
    this.table.on("rowSelectionChanged", (data) => this._handleSelectionChanged(data));
  }

  /**
   * Handle row selected event
   * @param {Object} row - Tabulator row
   * @private
   */
  async _handleRowSelected(row) {
    // Exit edit mode if editing a different row
    if (this._isEditing) {
      const pkField = this.primaryKeyField || 'query_id';
      const newRowId = row.getData()?.[pkField];
      const isSameRow = (this._editingRowId != null)
        ? newRowId === this._editingRowId
        : newRowId == null;
      if (!isSameRow) {
        await this._exitEditMode('row-change');
      }
    }

    this._updateSelectorCell(row, true);
    this._persistSelectedRow(row);
  }

  /**
   * Persist selected row to localStorage
   * @param {Object} row - Tabulator row
   * @private
   */
  _persistSelectedRow(row) {
    const pkField = this.primaryKeyField || 'query_id';
    const pkValue = row.getData()?.[pkField];
    if (pkValue == null) return;
    
    try {
      localStorage.setItem(SELECTED_ROW_KEY, String(pkValue));
    } catch (_) { /* storage full or restricted — ignore */ }
  }

  /**
   * Handle selection changed event
   * @param {Array} data - Selected row data
   * @private
   */
  _handleSelectionChanged(data) {
    if (data.length > 0) {
      const pkField = this.primaryKeyField || 'query_id';
      const selectedId = data[0]?.[pkField];
      
      if (this._shouldSkipDetailLoad(selectedId)) return;
      
      if (selectedId != null && String(this._loadedDetailRowId) !== String(selectedId)) {
        this.loadQueryDetails(data[0]);
      }
    } else if (!this._isEditing) {
      this.clearQueryDetails();
    }

    this._updateMoveButtonState();
  }

  /**
   * Check if detail load should be skipped
   * @param {*} selectedId - Selected row ID
   * @returns {boolean} True if should skip
   * @private
   */
  _shouldSkipDetailLoad(selectedId) {
    return this._isEditing && this._editingRowId != null 
      && String(this._editingRowId) === String(selectedId);
  }

  /**
   * Handle keyboard events in the table container.
   * Arrow Up/Down move selection. Enter re-triggers selection (loads details).
   * PageUp/PageDown move a page. Home/End go to first/last.
   */
  _handleTableKeydown(e) {
    if (!this.table) return;

    switch (e.key) {
      case 'ArrowUp':
        e.preventDefault();
        this.navigatePrevRec();
        break;
      case 'ArrowDown':
        e.preventDefault();
        this.navigateNextRec();
        break;
      case 'PageUp':
        e.preventDefault();
        this.navigatePrevPage();
        break;
      case 'PageDown':
        e.preventDefault();
        this.navigateNextPage();
        break;
      case 'Home':
        e.preventDefault();
        this.navigateFirst();
        break;
      case 'End':
        e.preventDefault();
        this.navigateLast();
        break;
      case 'Enter': {
        e.preventDefault();
        // Re-trigger load-details on the currently selected row
        const selected = this.table.getSelectedRows();
        if (selected.length > 0) {
          const data = selected[0].getData();
          this.loadQueryDetails(data);
        }
        break;
      }
    }
  }

  async loadQueries(searchTerm = '') {
    // Remember which row was selected so we can re-select after reload
    const previouslySelectedId = this._getSelectedQueryId();

    // Use query refs from JSON config (fallback to hardcoded for safety)
    const listRef = this.queryRefs?.queryRef ?? 25;
    const searchRef = this.queryRefs?.searchQueryRef ?? 32;
    const queryRef = searchTerm ? searchRef : listRef;

    // Note: HTTP request/response logging is handled by json-request.js.
    // We only log the business-level operation here to avoid duplicate
    // request numbers in the log output.
    log(Subsystems.CONDUIT, Status.INFO,
      `[QueriesManager] Loading queries (queryRef: ${queryRef}${searchTerm ? `, search: "${searchTerm}"` : ''})`);

    // Show loading indicator
    this._showTableLoading();

    try {
      let rows;
      if (searchTerm) {
        rows = await authQuery(this.app.api, searchRef, {
          STRING: { SEARCH: searchTerm.toUpperCase() },
        });
      } else {
        rows = await authQuery(this.app.api, listRef);
      }

      // Guard: table may have been torn down if the manager was
      // switched away from while the request was in flight.
      if (!this.table) return;

      this.table.blockRedraw?.();
      try {
        this.table.setData(rows);

        // Discover all columns from the data (adds hidden columns for
        // fields not already defined, so they appear in the column chooser)
        this._discoverColumns(rows);
      } finally {
        this.table.restoreRedraw?.();
      }

      // After data is loaded and columns are settled, select a row.
      // Defer to the next animation frame so that any pending Tabulator
      // re-renders triggered by _discoverColumns() (addColumn calls)
      // have a chance to complete first.  Without this deferral, the
      // selection can be silently reset by a trailing re-render.
      requestAnimationFrame(() => {
        this._autoSelectRow(previouslySelectedId);
        // Update navigation button states after selection is restored
        this._updateMoveButtonState();
      });
    } catch (error) {
      // Show detailed error toast with subsystem badge
      toast.error('Query Load Failed', {
        serverError: error.serverError,
        subsystem: 'Conduit',
        duration: 8000,
      });
    } finally {
      // Hide loading indicator
      this._hideTableLoading();
    }
  }

  /**
   * Get the primary key value of the currently selected row, or null.
   */
  _getSelectedQueryId() {
    if (!this.table) return null;
    const selected = this.table.getSelectedRows();
    if (selected.length > 0) {
      const pkField = this.primaryKeyField || 'query_id';
      return selected[0].getData()[pkField] ?? null;
    }
    return null;
  }

  /**
   * After data load, re-select a previously selected row or fall back to row 0.
   *
   * Resolution order:
   *   1. In-memory previousId (from the current session's selection)
   *   2. localStorage  (survives refresh / re-login)
   *   3. First visible row
   *
   * @param {*} previousId - query_id to try to re-select
   */
  _autoSelectRow(previousId) {
    if (!this.table) return;
    const rows = this.table.getRows('active');
    if (rows.length === 0) return;

    // Determine the target ID: prefer in-memory, then localStorage
    let targetId = previousId;
    if (targetId == null) {
      try {
        const stored = localStorage.getItem(SELECTED_ROW_KEY);
        if (stored != null) targetId = stored;
      } catch (_) { /* ignore */ }
    }

    // Try to find and select the target row (loose compare for number/string)
    const pkField = this.primaryKeyField || 'query_id';
    if (targetId != null) {
      for (const row of rows) {
        if (String(row.getData()[pkField]) === String(targetId)) {
          row.select();
          row.scrollTo();
          return;
        }
      }
    }

    // Fall back to the first row
    rows[0].select();
    rows[0].scrollTo();
  }

  loadQueryDetails(queryData) {
    const pkField = this.primaryKeyField || 'query_id';
    const queryId = queryData?.[pkField];

    if (queryId == null) return;
    if (String(this._loadedDetailRowId) === String(queryId)) return;

    // Store current query reference
    this.currentQuery = queryData;
    
    // Fetch full query details using QueryRef 27
    this.fetchQueryDetails(queryId);
  }

  /**
   * Fetch full query details from the API.
   * HTTP request/response logging is handled by json-request.js.
   */
  async fetchQueryDetails(queryId) {
    if (queryId == null) return;

    const detailRef = this.queryRefs?.detailQueryRef ?? 27;
    log(Subsystems.CONDUIT, Status.INFO, `[QueriesManager] Fetching query details (queryRef: ${detailRef}, queryId: ${queryId})`);

    try {
      const queryDetails = await authQuery(this.app.api, detailRef, { INTEGER: { QUERYID: queryId } });
      await this._processQueryDetails(queryDetails, queryId);
    } catch (error) {
      toast.error('Query Details Failed', { serverError: error.serverError, subsystem: 'Conduit', duration: 8000 });
    }
  }

  /**
   * Process query details response
   * @param {Array} queryDetails - Query details from API
   * @param {number} queryId - Query ID
   * @private
   */
  async _processQueryDetails(queryDetails, queryId) {
    if (!queryDetails?.length) return;

    const fullQuery = queryDetails[0];
    this.currentQuery = fullQuery;

    if (!this._isEditing) {
      this._captureOriginalData(fullQuery);
    }

    const content = this._extractQueryContent(fullQuery);
    this._updateEditors(content);
    this._storePendingContent(content);
    this._checkSqlTabInit(content.sqlContent);

    this._loadedDetailRowId = queryId;
    log(Subsystems.CONDUIT, Status.INFO, `[QueriesManager] Loaded query details for ID ${queryId}`);
  }

  /**
   * Extract content from query data
   * @param {Object} query - Query data
   * @returns {Object} Extracted content
   * @private
   */
  _extractQueryContent(query) {
    return {
      sqlContent: query.code || query.query_text || query.sql || '',
      summaryContent: query.summary || query.markdown || '',
      collectionContent: query.collection || query.json || {},
    };
  }

  /**
   * Update editors with content
   * @param {Object} content - Content object
   * @private
   */
  _updateEditors(content) {
    if (this.sqlEditor) {
      this.sqlEditor.dispatch({
        changes: { from: 0, to: this.sqlEditor.state.doc.length, insert: content.sqlContent },
      });
    }

    if (this.summaryEditor) {
      this.summaryEditor.dispatch({
        changes: { from: 0, to: this.summaryEditor.state.doc.length, insert: content.summaryContent },
      });
    }

    if (this.collectionEditor) {
      const jsonData = typeof content.collectionContent === 'object'
        ? content.collectionContent
        : JSON.parse(content.collectionContent || '{}');
      this.collectionEditor.set({ json: jsonData });
    }
  }

  /**
   * Store pending content for lazy initialization
   * @param {Object} content - Content object
   * @private
   */
  _storePendingContent(content) {
    this._pendingSqlContent = content.sqlContent;
    this._pendingSummaryContent = content.summaryContent;
    this._pendingCollectionContent = content.collectionContent;
  }

  /**
   * Check if SQL tab needs initialization
   * @param {string} sqlContent - SQL content
   * @private
   */
  _checkSqlTabInit(sqlContent) {
    const sqlTabBtn = document.querySelector('.queries-tab-btn[data-tab="sql"]');
    const isSqlTabActive = sqlTabBtn?.classList.contains('active');

    if (isSqlTabActive && !this.sqlEditor) {
      this.initSqlEditor(sqlContent);
    }
  }

  clearQueryDetails() {
    this._loadedDetailRowId = null;
  }

  setupSplitter() {
    if (!this.elements.splitter) return;

    this.handleSplitterMouseDown = this.handleSplitterMouseDown.bind(this);
    this.handleSplitterMouseMove = this.handleSplitterMouseMove.bind(this);
    this.handleSplitterMouseUp = this.handleSplitterMouseUp.bind(this);

    this.elements.splitter.addEventListener('mousedown', this.handleSplitterMouseDown);
  }

  handleSplitterMouseDown(event) {
    event.preventDefault();
    this.isResizing = true;
    this.elements.splitter.classList.add('resizing');
    document.body.style.cursor = 'col-resize';
    
    // Remove width transition so resizing follows mouse immediately
    if (this.elements.leftPanel) {
      this.elements.leftPanel.style.transition = 'none';
    }
    
    document.addEventListener('mousemove', this.handleSplitterMouseMove);
    document.addEventListener('mouseup', this.handleSplitterMouseUp);
  }

  handleSplitterMouseMove(event) {
    if (!this.isResizing) return;
    
    const containerRect = this.elements.container.getBoundingClientRect();
    const newWidth = event.clientX - containerRect.left;
    
    // Constrain width
    const minWidth = 157;
    const maxWidth = containerRect.width - 313; // Ensure right panel has space
    const constrainedWidth = Math.max(minWidth, Math.min(maxWidth, newWidth));
    
    this.elements.leftPanel.style.width = `${constrainedWidth}px`;
  }

  handleSplitterMouseUp() {
    this.isResizing = false;
    this.elements.splitter.classList.remove('resizing');
    document.body.style.cursor = '';
    
    // Restore width transition for expand/collapse button
    if (this.elements.leftPanel) {
      this.elements.leftPanel.style.transition = '';

      // Persist the manually resized width and detect mode
      const finalWidth = this.elements.leftPanel.offsetWidth;
      this._savePanelWidth(finalWidth);
      this._tableWidthMode = this._detectWidthMode(finalWidth);
    }
    
    document.removeEventListener('mousemove', this.handleSplitterMouseMove);
    document.removeEventListener('mouseup', this.handleSplitterMouseUp);
  }

  /**
   * Initialize CodeMirror SQL editor
   */
  async initSqlEditor(initialContent = '') {
    if (!this.elements.sqlEditorContainer) return;
    
    // If editor already exists, just update content
    if (this.sqlEditor) {
      this.sqlEditor.dispatch({
        changes: { from: 0, to: this.sqlEditor.state.doc.length, insert: initialContent },
      });
      return;
    }

    try {
      // Dynamic import CodeMirror packages
      const [
        { EditorState },
        { EditorView, lineNumbers, highlightActiveLineGutter, highlightSpecialChars, drawSelection, highlightActiveLine, keymap, highlightActiveLine: highlightActiveLineView },
        { defaultKeymap, history, historyKeymap },
        { sql },
        { oneDark },
      ] = await Promise.all([
        import('@codemirror/state'),
        import('@codemirror/view'),
        import('@codemirror/commands'),
        import('@codemirror/lang-sql'),
        import('@codemirror/theme-one-dark'),
      ]);
      
      // Import history field for undo/redo state checking
      const historyModule = await import('@codemirror/commands');
      historyField = historyModule.historyField;

      // Create editor state with SQL support
      const startState = EditorState.create({
        doc: initialContent,
        extensions: [
          lineNumbers({
            formatNumber: (n) => String(n).padStart(4, '0'),
          }),
          highlightActiveLineGutter(),
          highlightSpecialChars(),
          history(),
          drawSelection(),
          highlightActiveLine(),
          keymap.of([...defaultKeymap, ...historyKeymap]),
          sql(),
          oneDark,
          EditorView.theme({
            '&': { height: '100%', fontSize: `${this.fontSettings.size}px` },
            '.cm-scroller': { overflow: 'auto' },
            '.cm-content': { fontFamily: '"Vanadium Mono", var(--font-mono, monospace)' },
          }),
          EditorView.updateListener.of((update) => {
            if (update.docChanged) {
              this.updateToolbarState();
              // Track dirty state for SQL content
              const isDirty = this._checkSqlDirty();
              this._setDirty('sql', isDirty);
            }
          }),
        ],
      });

      this.sqlEditor = new EditorView({
        state: startState,
        parent: this.elements.sqlEditorContainer,
      });

      // Default to read-only — edit mode enables editing
      const contentEl = this.sqlEditor.dom.querySelector('.cm-content');
      if (contentEl) {
        contentEl.contentEditable = 'false';
      }
      this.elements.sqlEditorContainer?.classList.add('queries-cm-readonly');
      
      // Update toolbar state after initialization
      this.updateToolbarState();
    } catch (error) {
      console.error('[QueriesManager] Failed to initialize CodeMirror:', error);
      // Fallback to textarea
      this.elements.sqlEditorContainer.innerHTML = `
        <textarea id="queries-sql-editor-fallback" 
          style="width:100%;height:100%;background:var(--bg-secondary);color:var(--text-primary);border:none;padding:16px;font-family:var(--font-mono);font-size:${this.fontSize}px;">
          ${initialContent}
        </textarea>`;
    }
  }

  /**
   * Initialize CodeMirror Summary editor (Markdown)
   */
  async initSummaryEditor(initialContent = '') {
    if (!this.elements.summaryEditorContainer) return;
    
    // Use pending content if available
    if (!initialContent && this._pendingSummaryContent) {
      initialContent = this._pendingSummaryContent;
    }
    
    // If editor already exists, just update content
    if (this.summaryEditor) {
      this.summaryEditor.dispatch({
        changes: { from: 0, to: this.summaryEditor.state.doc.length, insert: initialContent },
      });
      return;
    }

    try {
      // Dynamic import CodeMirror packages
      const [
        { EditorState },
        { EditorView, lineNumbers, highlightActiveLineGutter, highlightSpecialChars, drawSelection, highlightActiveLine, keymap },
        { defaultKeymap, history, historyKeymap },
        { markdown },
        { oneDark },
      ] = await Promise.all([
        import('@codemirror/state'),
        import('@codemirror/view'),
        import('@codemirror/commands'),
        import('@codemirror/lang-markdown'),
        import('@codemirror/theme-one-dark'),
      ]);

      // Create editor state with Markdown support
      const startState = EditorState.create({
        doc: initialContent,
        extensions: [
          lineNumbers(),
          highlightActiveLineGutter(),
          highlightSpecialChars(),
          history(),
          drawSelection(),
          highlightActiveLine(),
          keymap.of([...defaultKeymap, ...historyKeymap]),
          markdown(),
          oneDark,
          EditorView.theme({
            '&': { height: '100%', fontSize: '14px' },
            '.cm-scroller': { overflow: 'auto' },
            '.cm-content': { fontFamily: 'var(--font-mono, monospace)' },
          }),
          EditorView.updateListener.of((update) => {
            if (update.docChanged) {
              // Track dirty state for Summary content
              const isDirty = this._checkSummaryDirty();
              this._setDirty('summary', isDirty);
            }
          }),
        ],
      });

      this.summaryEditor = new EditorView({
        state: startState,
        parent: this.elements.summaryEditorContainer,
      });

      // Default to read-only — edit mode enables editing
      const contentEl = this.summaryEditor.dom.querySelector('.cm-content');
      if (contentEl) {
        contentEl.contentEditable = 'false';
      }
      this.elements.summaryEditorContainer?.classList.add('queries-cm-readonly');
    } catch (error) {
      console.error('[QueriesManager] Failed to initialize Summary editor:', error);
      // Fallback to textarea
      this.elements.summaryEditorContainer.innerHTML = `
        <textarea id="queries-summary-editor-fallback" 
          style="width:100%;height:100%;background:var(--bg-secondary);color:var(--text-primary);border:none;padding:16px;font-family:var(--font-mono);font-size:14px;">
          ${initialContent}
        </textarea>`;
    }
  }

  /**
   * Initialize JSONEditor for Collection tab (tree view JSON editor)
   */
  async initCollectionEditor(initialContent = {}) {
    if (!this.elements.collectionEditorContainer) return;

    // Use pending content if available
    let content = initialContent;
    if (!initialContent && this._pendingCollectionContent) {
      content = this._pendingCollectionContent;
    }

    // Parse content if it's a string
    let jsonData = content;
    if (typeof content === 'string') {
      try {
        jsonData = JSON.parse(content);
      } catch (e) {
        jsonData = {};
      }
    }

    // If editor already exists, just update content
    if (this.collectionEditor) {
      this.collectionEditor.set({ json: jsonData });
      return;
    }

    try {
      // Dynamic import vanilla-jsoneditor
      const { JSONEditor } = await import('vanilla-jsoneditor');

      // Create editor container
      this.elements.collectionEditorContainer.innerHTML = '';
      const editorContainer = document.createElement('div');
      editorContainer.style.cssText = 'width:100%;height:100%;';
      editorContainer.classList.add('jse-theme-dark');
      this.elements.collectionEditorContainer.appendChild(editorContainer);

      // Initialize vanilla-jsoneditor
      this.collectionEditor = new JSONEditor({
        target: editorContainer,
        props: {
          content: { json: jsonData },
          mode: 'tree',
          mainMenuBar: true,
          navigationBar: true,
          statusBar: true,
          onChange: (updatedContent, previousContent, { contentErrors, patchResult }) => {
            // Track dirty state for Collection content
            const isDirty = this._checkCollectionDirty();
            this._setDirty('collection', isDirty);
          },
        },
      });

    } catch (error) {
      console.error('[QueriesManager] Failed to initialize vanilla-jsoneditor:', error);
      // Fallback to textarea
      const jsonContent = typeof jsonData === 'object'
        ? JSON.stringify(jsonData, null, 2)
        : jsonData;
      this.elements.collectionEditorContainer.innerHTML = `
        <textarea id="queries-collection-editor-fallback"
          style="width:100%;height:100%;background:var(--bg-secondary);color:var(--text-primary);border:none;padding:16px;font-family:var(--font-mono);font-size:14px;">
          ${jsonContent}
        </textarea>`;
    }
  }

  /**
   * Render Markdown as HTML in Preview tab
   */
  async renderMarkdownPreview() {
    if (!this.elements.previewContainer) return;
    
    // Get content from summary editor or pending content
    let markdownContent = '';
    if (this.summaryEditor) {
      markdownContent = this.summaryEditor.state.doc.toString();
    } else if (this._pendingSummaryContent) {
      markdownContent = this._pendingSummaryContent;
    } else if (this.currentQuery?.summary) {
      markdownContent = this.currentQuery.summary;
    }
    
    if (!markdownContent.trim()) {
      this.elements.previewContainer.innerHTML = '<p class="text-muted">No summary content to preview.</p>';
      return;
    }
    
    try {
      // Dynamic import marked for markdown parsing
      const { marked } = await import('marked');
      
      // Parse markdown to HTML
      const htmlContent = await marked.parse(markdownContent);
      
      // Render in preview container with styling
      this.elements.previewContainer.innerHTML = `
        <div class="queries-preview-content">
          ${htmlContent}
        </div>
      `;
    } catch (error) {
      console.error('[QueriesManager] Failed to render markdown preview:', error);
      // Fallback: Show raw content
      const escapedContent = markdownContent
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;');
      this.elements.previewContainer.innerHTML = `
        <div class="queries-preview-content">
          <pre><code>${escapedContent}</code></pre>
        </div>
      `;
    }
  }

  /**
   * Update toolbar button states based on editor state
   */
  updateToolbarState() {
    if (!this.sqlEditor) return;

    // For CodeMirror 6, we use a simpler approach - check if there are any undo/redo changes available
    // The history field can tell us but it's complex to access. Instead, we'll track this via the update listener
    // For now, we'll enable both buttons and let the user try - CodeMirror will handle the actual state
  }

  /**
   * Handle undo button click
   */
  async handleUndo() {
    if (!this.sqlEditor) return;
    
    try {
      const { undo } = await import('@codemirror/commands');
      undo(this.sqlEditor);
    } catch (error) {
      console.error('[QueriesManager] Failed to undo:', error);
    }
  }

  /**
   * Handle redo button click
   */
  async handleRedo() {
    if (!this.sqlEditor) return;
    
    try {
      const { redo } = await import('@codemirror/commands');
      redo(this.sqlEditor);
    } catch (error) {
      console.error('[QueriesManager] Failed to redo:', error);
    }
  }

  /**
   * Handle font button click - toggle font popup
   */
  handleFontClick() {
    if (this.fontPopup?.classList.contains('visible')) {
      this.hideFontPopup();
    } else {
      this.showFontPopup();
    }
  }

  /**
   * Show the font settings popup
   */
  showFontPopup() {
    if (!this.fontPopup) {
      this.createFontPopup();
    }
    this.fontPopup.classList.add('visible');

    // Close popup when clicking outside
    this._fontPopupCloseHandler = (e) => {
      if (!this.fontPopup.contains(e.target) && !this.elements.fontBtn.contains(e.target)) {
        this.hideFontPopup();
      }
    };
    document.addEventListener('click', this._fontPopupCloseHandler);
  }

  /**
   * Hide the font settings popup
   */
  hideFontPopup() {
    if (this.fontPopup) {
      this.fontPopup.classList.remove('visible');
    }
    if (this._fontPopupCloseHandler) {
      document.removeEventListener('click', this._fontPopupCloseHandler);
      this._fontPopupCloseHandler = null;
    }
  }

  /**
   * Create the font popup element
   */
  createFontPopup() {
    // Detect fonts currently in use on the page
    const detectedFonts = this.detectPageFonts();

    this.fontPopup = document.createElement('div');
    this.fontPopup.className = 'queries-font-popup';
    this.fontPopup.innerHTML = `
      <div class="queries-font-popup-row">
        <span class="queries-font-popup-label">Font</span>
        <select class="queries-font-popup-select" id="font-family-select">
          ${detectedFonts.map(f => `<option value="${f.value}" ${f.value === this.fontSettings.family ? 'selected' : ''}>${f.name}</option>`).join('')}
        </select>
      </div>
      <div class="queries-font-popup-row">
        <span class="queries-font-popup-label">Size</span>
        <input type="number" class="queries-font-popup-input" id="font-size-input"
               min="${MIN_FONT_SIZE}" max="${MAX_FONT_SIZE}" value="${this.fontSettings.size}">
      </div>
      <div class="queries-font-popup-row">
        <span class="queries-font-popup-label">Style</span>
        <div class="queries-font-popup-styles">
          <button type="button" class="queries-font-popup-style-btn ${this.fontSettings.bold ? 'active' : ''}" id="font-bold-btn" title="Bold">B</button>
          <button type="button" class="queries-font-popup-style-btn ${this.fontSettings.italic ? 'active' : ''}" id="font-italic-btn" title="Italic">I</button>
        </div>
      </div>
    `;

    // Add to tabs header (positioned relative to it)
    this.elements.tabsHeader.style.position = 'relative';
    this.elements.tabsHeader.appendChild(this.fontPopup);

    // Setup event listeners
    const fontSelect = this.fontPopup.querySelector('#font-family-select');
    const sizeInput = this.fontPopup.querySelector('#font-size-input');
    const boldBtn = this.fontPopup.querySelector('#font-bold-btn');
    const italicBtn = this.fontPopup.querySelector('#font-italic-btn');

    fontSelect?.addEventListener('change', (e) => {
      this.fontSettings.family = e.target.value;
      this.applyFontSettings();
    });

    sizeInput?.addEventListener('change', (e) => {
      let size = parseInt(e.target.value, 10);
      size = Math.max(MIN_FONT_SIZE, Math.min(MAX_FONT_SIZE, size));
      this.fontSettings.size = size;
      e.target.value = size;
      this.applyFontSettings();
    });

    boldBtn?.addEventListener('click', () => {
      this.fontSettings.bold = !this.fontSettings.bold;
      boldBtn.classList.toggle('active', this.fontSettings.bold);
      this.applyFontSettings();
    });

    italicBtn?.addEventListener('click', () => {
      this.fontSettings.italic = !this.fontSettings.italic;
      italicBtn.classList.toggle('active', this.fontSettings.italic);
      this.applyFontSettings();
    });
  }

  /**
   * Detect fonts currently in use on the page
   * Returns array of { name, value } objects
   */
  detectPageFonts() {
    // Get computed fonts from various elements
    const fonts = [];

    // Vanadium Mono is our preferred default - add it first
    fonts.push({ name: 'Vanadium Mono', value: '"Vanadium Mono", var(--font-mono, monospace)' });

    // Add CSS variable fonts
    const styles = getComputedStyle(document.documentElement);
    const fontMono = styles.getPropertyValue('--font-mono').trim() || 'monospace';
    const fontSans = styles.getPropertyValue('--font-sans').trim() || 'system-ui, sans-serif';
    const fontSerif = styles.getPropertyValue('--font-serif').trim() || 'Georgia, serif';

    fonts.push({ name: 'Monospace', value: fontMono });
    fonts.push({ name: 'Sans Serif', value: fontSans });
    fonts.push({ name: 'Serif', value: fontSerif });

    // Common system fonts
    const commonFonts = [
      { name: 'System UI', value: 'system-ui, -apple-system, sans-serif' },
      { name: 'Arial', value: 'Arial, sans-serif' },
      { name: 'Courier New', value: '"Courier New", monospace' },
      { name: 'Consolas', value: 'Consolas, monospace' },
      { name: 'Fira Code', value: '"Fira Code", monospace' },
      { name: 'JetBrains Mono', value: '"JetBrains Mono", monospace' },
      { name: 'Source Code Pro', value: '"Source Code Pro", monospace' },
    ];

    commonFonts.forEach(f => fonts.push(f));

    return fonts;
  }

  /**
   * Apply current font settings to the editor
   */
  applyFontSettings() {
    if (!this.sqlEditor) return;

    const editorElement = this.sqlEditor.dom;
    if (editorElement) {
      editorElement.style.fontSize = `${this.fontSettings.size}px`;
      editorElement.style.fontFamily = this.fontSettings.family;
      editorElement.style.fontWeight = this.fontSettings.bold ? 'bold' : 'normal';
      editorElement.style.fontStyle = this.fontSettings.italic ? 'italic' : 'normal';
    }

    log(Subsystems.MANAGER, Status.INFO, `Font updated: ${this.fontSettings.size}px ${this.fontSettings.family}`);
  }

  /**
   * Handle prettify button click
   */
  async handlePrettify() {
    if (!this.sqlEditor) return;

    const currentContent = this.sqlEditor.state.doc.toString();
    if (!currentContent.trim()) return;

    try {
      // Dynamic import sql-formatter - exports { format } not default
      const { format } = await import('sql-formatter');

      const formatted = format(currentContent, {
        language: this.sqlDialect,
        tabWidth: 2,
        keywordCase: 'upper',
      });

      // Replace content in editor
      this.sqlEditor.dispatch({
        changes: { from: 0, to: this.sqlEditor.state.doc.length, insert: formatted },
      });

      log(Subsystems.CONDUIT, Status.INFO, 'SQL formatted successfully');
    } catch (error) {
      console.error('[QueriesManager] SQL format error:', error);
      // Show detailed error message in toast
      const errorMessage = error.message || 'Unknown formatting error';
      toast.error('SQL Format Failed', {
        title: 'SQL Format Failed',
        description: errorMessage,
        subsystem: 'Query',
        duration: 8000,
      });
    }
  }

  teardown() {
    // Close any open column chooser
    this._closeColumnChooser();

    // Cleanup event listeners
    if (this.elements.splitter) {
      this.elements.splitter.removeEventListener('mousedown', this.handleSplitterMouseDown);
    }
    document.removeEventListener('mousemove', this.handleSplitterMouseMove);
    document.removeEventListener('mouseup', this.handleSplitterMouseUp);

    // Close any open navigator popups
    this._closeNavPopup();

    // Close any open footer export popup
    this._closeFooterExportPopup();

    // Hide and remove font popup
    if (this.fontPopup) {
      this.hideFontPopup();
      this.fontPopup.remove();
      this.fontPopup = null;
    }

    // Destroy CodeMirror editors
    if (this.sqlEditor) {
      this.sqlEditor.destroy();
      this.sqlEditor = null;
    }
    if (this.summaryEditor) {
      this.summaryEditor.destroy();
      this.summaryEditor = null;
    }
    // Destroy JSONEditor
    if (this.collectionEditor) {
      this.collectionEditor.destroy();
      this.collectionEditor = null;
    }
  }
}
