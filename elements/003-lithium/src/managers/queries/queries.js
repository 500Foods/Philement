/**
 * Queries Manager
 *
 * Query builder and execution interface.
 */

import { TabulatorFull as Tabulator } from 'tabulator-tables';
import '../../styles/vendor-tabulator.css';
import '../../styles/vendor-jsoneditor.css';
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
} from '../../core/lithium-table.js';
import './queries.css';

// Font constants
const MIN_FONT_SIZE = 10;
const MAX_FONT_SIZE = 24;
const DEFAULT_FONT_SIZE = 14;
const FONT_SIZE_STEP = 2;

// Default font settings
const DEFAULT_FONT_FAMILY = '"Vanadium Mono", var(--font-mono, monospace)';

// localStorage key for persisting the selected row across sessions
const SELECTED_ROW_KEY = 'lithium_queries_selected_id';

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
    this._columnChooserPopup = null;
    this._filtersVisible = false;

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
  }

  async init() {
    await this.render();
    this.setupEventListeners();
    this.setupSplitter();
    await this.initTable();
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
   *   1. Control  — refresh, menu, print, email, export, import
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

    nav.innerHTML = `
      <div class="queries-nav-wrapper">
        <div class="queries-nav-block queries-nav-block-control">
          <button type="button" class="queries-nav-btn" id="queries-nav-refresh" title="Refresh">
            <fa fa-arrows-rotate></fa>
          </button>
          <button type="button" class="queries-nav-btn queries-nav-btn-has-popup" id="queries-nav-menu" title="Table Options">
            <fa fa-screwdriver-wrench></fa>
          </button>
          <button type="button" class="queries-nav-btn" id="queries-nav-print" title="Print">
            <fa fa-print></fa>
          </button>
          <button type="button" class="queries-nav-btn" id="queries-nav-email" title="Email">
            <fa fa-envelope></fa>
          </button>
          <button type="button" class="queries-nav-btn queries-nav-btn-has-popup" id="queries-nav-export" title="Export">
            <fa fa-download></fa>
          </button>
          <button type="button" class="queries-nav-btn queries-nav-btn-has-popup" id="queries-nav-import" title="Import">
            <fa fa-upload></fa>
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
          <button type="button" class="queries-nav-btn" id="queries-nav-save" title="Save">
            <fa fa-floppy-disk></fa>
          </button>
          <button type="button" class="queries-nav-btn" id="queries-nav-cancel" title="Cancel">
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

    // Process <fa> icons inside the navigator
    processIcons(nav);

    // --- Wire up control buttons ---
    nav.querySelector('#queries-nav-refresh')?.addEventListener('click', () => this.handleNavRefresh());
    nav.querySelector('#queries-nav-menu')?.addEventListener('click', (e) => this.toggleNavPopup(e, 'menu'));
    nav.querySelector('#queries-nav-print')?.addEventListener('click', () => this.handleNavPrint());
    nav.querySelector('#queries-nav-email')?.addEventListener('click', () => this.handleNavEmail());
    nav.querySelector('#queries-nav-export')?.addEventListener('click', (e) => this.toggleNavPopup(e, 'export'));
    nav.querySelector('#queries-nav-import')?.addEventListener('click', (e) => this.toggleNavPopup(e, 'import'));

    // --- Wire up move buttons ---
    nav.querySelector('#queries-nav-first')?.addEventListener('click', () => this.navigateFirst());
    nav.querySelector('#queries-nav-pgup')?.addEventListener('click',  () => this.navigatePrevPage());
    nav.querySelector('#queries-nav-prev')?.addEventListener('click',  () => this.navigatePrevRec());
    nav.querySelector('#queries-nav-next')?.addEventListener('click',  () => this.navigateNextRec());
    nav.querySelector('#queries-nav-pgdn')?.addEventListener('click',  () => this.navigateNextPage());
    nav.querySelector('#queries-nav-last')?.addEventListener('click',  () => this.navigateLast());

    // --- Wire up manage buttons ---
    nav.querySelector('#queries-nav-add')?.addEventListener('click',       () => this.handleNavAdd());
    nav.querySelector('#queries-nav-duplicate')?.addEventListener('click', () => this.handleNavDuplicate());
    nav.querySelector('#queries-nav-edit')?.addEventListener('click',      () => this.handleNavEdit());
    nav.querySelector('#queries-nav-save')?.addEventListener('click',      () => this.handleNavSave());
    nav.querySelector('#queries-nav-cancel')?.addEventListener('click',    () => this.handleNavCancel());
    nav.querySelector('#queries-nav-delete')?.addEventListener('click',    () => this.handleNavDelete());

    // --- Wire up search ---
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
      const row = document.createElement('button');
      row.type = 'button';
      row.className = 'queries-nav-popup-item';
      row.textContent = item.label;
      row.addEventListener('click', () => {
        this._closeNavPopup();
        item.action();
      });
      popup.appendChild(row);
    });

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
          { label: this._filtersVisible ? '\u2713 Column Filters' : '   Column Filters', action: () => this.toggleColumnFilters() },
          { label: 'Expand All',   action: () => this.handleMenuExpandAll() },
          { label: 'Collapse All', action: () => this.handleMenuCollapseAll() },
        ];
      case 'export':
        return [
          { label: 'PDF', action: () => this.handleExport('pdf') },
          { label: 'CSV', action: () => this.handleExport('csv') },
          { label: 'TXT', action: () => this.handleExport('txt') },
          { label: 'XLS', action: () => this.handleExport('xls') },
        ];
      case 'import':
        return [
          { label: 'CSV', action: () => this.handleImport('csv') },
          { label: 'TXT', action: () => this.handleImport('txt') },
          { label: 'XLS', action: () => this.handleImport('xls') },
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
    this._selectRowByIndex(0);
  }

  navigateLast() {
    const rows = this.table?.getRows('active') || [];
    if (rows.length > 0) {
      this._selectRowByIndex(rows.length - 1);
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
        const isEditing = this._editingRowId != null &&
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

  // ── Column chooser ────────────────────────────────────────────────────

  /**
   * Toggle the column chooser dropdown anchored to the selector column header.
   * Shows a scrollable checklist of all data columns for toggling visibility.
   * @param {MouseEvent} e - click event
   * @param {Object} column - Tabulator column component
   */
  toggleColumnChooser(e, column) {
    e.stopPropagation();

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

  handleNavAdd() {
    log(Subsystems.MANAGER, Status.INFO, 'Navigator: Add');
    if (!this.table) return;

    // Add a new empty row at the top of the table
    const newRow = this.table.addRow({}, true); // true = add at position 0

    // Select the new row
    if (newRow) {
      this.table.deselectRow();
      newRow.select();
      newRow.scrollTo();

      // Enter edit mode for the new row
      this._editingRowId = newRow.getData()?.[this.primaryKeyField || 'query_id'];
      this._updateEditingIndicator();

      // Focus on the Name column for editing
      const nameCell = newRow.getCell('name');
      if (nameCell) {
        nameCell.edit();
      }
    }

    log(Subsystems.MANAGER, Status.INFO, 'Added new query row');
  }

  handleNavDuplicate() {
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
    const newRow = this.table.addRow(duplicateData, true);

    if (newRow) {
      this.table.deselectRow();
      newRow.select();
      newRow.scrollTo();

      // Enter edit mode
      this._editingRowId = newRow.getData()?.[pkField];
      this._updateEditingIndicator();

      log(Subsystems.MANAGER, Status.INFO, 'Duplicated query row');
    }
  }

  handleNavEdit() {
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

    if (selectedId != null) {
      this._editingRowId = selectedId;
      this._updateEditingIndicator();

      // Start editing the Name cell
      const nameCell = selected[0].getCell('name');
      if (nameCell) {
        nameCell.edit();
      }

      log(Subsystems.MANAGER, Status.INFO, `Entered edit mode for row ${selectedId}`);
    }
  }

  handleNavSave() {
    log(Subsystems.MANAGER, Status.INFO, 'Navigator: Save');
    if (!this.table) return;

    // Get all edited rows
    const editedRows = this.table.getRows().filter(row => row.isValid() && row.isEdited());

    if (editedRows.length === 0) {
      toast.info('No Changes', {
        description: 'No pending changes to save',
        duration: 3000,
      });
      return;
    }

    // Collect the changed data
    const changes = editedRows.map(row => ({
      data: row.getData(),
      isNew: row.isNew(),
    }));

    // TODO: Send changes to API (QueryRef for insert/update)
    // For now, just clear the edit state and show success

    this._editingRowId = null;
    this._updateEditingIndicator();

    // Recalculate to clear edited state
    this.table.recalc();

    toast.success('Changes Saved', {
      description: `${changes.length} row(s) saved`,
      duration: 3000,
    });

    log(Subsystems.MANAGER, Status.INFO, `Saved ${changes.length} row(s)`);
  }

  handleNavCancel() {
    log(Subsystems.MANAGER, Status.INFO, 'Navigator: Cancel');
    if (!this.table) return;

    // Get all edited rows
    const editedRows = this.table.getRows().filter(row => row.isEdited());

    if (editedRows.length === 0) {
      // Just clear edit mode if no edits
      this._editingRowId = null;
      this._updateEditingIndicator();
      return;
    }

    // Revert all changes using Tabulator's undo
    this.table.undo();

    this._editingRowId = null;
    this._updateEditingIndicator();

    toast.info('Changes Cancelled', {
      description: `${editedRows.length} row(s) reverted`,
      duration: 3000,
    });

    log(Subsystems.MANAGER, Status.INFO, 'Cancelled changes and reverted edits');
  }

  handleNavDelete() {
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
    const rowCount = selected.length;

    // Confirm deletion
    const confirmed = window.confirm(
      `Are you sure you want to delete ${rowCount} row(s)?\n\nThis action cannot be undone.`
    );

    if (!confirmed) return;

    // TODO: Call API to delete the row(s)
    // For now, just delete from the table locally

    // Find the row to select after deletion (the next row, or previous, or first)
    const allRows = this.table.getRows('active');
    const currentIndex = allRows.findIndex(row => row === selected[0]);
    const nextRow = allRows[currentIndex + 1] || allRows[currentIndex - 1] || null;

    // Delete the selected rows
    selected.forEach(row => row.delete());

    // Select the next row
    if (nextRow) {
      nextRow.select();
      nextRow.scrollTo();
    }

    this._editingRowId = null;
    this._updateEditingIndicator();

    toast.success('Row(s) Deleted', {
      description: `${rowCount} row(s) deleted`,
      duration: 3000,
    });

    log(Subsystems.MANAGER, Status.INFO, `Deleted ${rowCount} row(s)`);
  }

  // ── Control button handlers (placeholder — wire up actions later) ─────

  handleNavPrint() {
    log(Subsystems.MANAGER, Status.INFO, 'Navigator: Print');
    if (this.table) {
      this.table.print();
    }
  }

  handleNavEmail() {
    log(Subsystems.MANAGER, Status.INFO, 'Navigator: Email');
    // TODO: implement email table data - build mailto with table data summary
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

  /**
   * Handle import from a specific format.
   * @param {'csv'|'txt'|'xls'} format
   */
  handleImport(format) {
    log(Subsystems.MANAGER, Status.INFO, `Navigator: Import from ${format.toUpperCase()}`);
    // TODO: implement import for each format
  }

  // ── Menu popup handlers ───────────────────────────────────────────────

  handleMenuExpandAll() {
    log(Subsystems.MANAGER, Status.INFO, 'Navigator: Expand All');
    // TODO: implement expand all rows / groups
  }

  handleMenuCollapseAll() {
    log(Subsystems.MANAGER, Status.INFO, 'Navigator: Collapse All');
    // TODO: implement collapse all rows / groups
  }

  /**
   * Toggle column filter row visibility in the table header.
   * When hidden, clears all active header filters AND resets
   * custom filter input values and clear-button visibility.
   */
  toggleColumnFilters() {
    this._filtersVisible = !this._filtersVisible;
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
    // ── Load JSON-driven column configuration (parallel fetch) ─────────
    [this.coltypes, this.tableDef] = await Promise.all([
      loadColtypes(),
      loadTableDef('queries/query-manager'),
    ]);

    if (this.tableDef) {
      this.queryRefs = getQueryRefs(this.tableDef);
      this.primaryKeyField = getPrimaryKeyField(this.tableDef);
    }

    // Resolve columns from JSON config → Tabulator column definitions
    const dataColumns = this.tableDef && this.coltypes
      ? resolveColumns(this.tableDef, this.coltypes, {
          filterEditor: this._createFilterEditor.bind(this),
        })
      : this._getFallbackColumns();

    // Resolve table-level options from the tabledef
    const tableOptions = this.tableDef
      ? resolveTableOptions(this.tableDef)
      : { layout: 'fitColumns', responsiveLayout: 'collapse' };

    // Build the selector column (row indicator + column chooser header)
    const selectorColumn = {
      title: "",
      field: "_selector",
      width: 20,
      minWidth: 20,
      maxWidth: 20,
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
          if (this._editingRowId != null && this._editingRowId === row.getData()[pkField]) {
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
        const row = cell.getRow();
        this.table.deselectRow();
        row.select();
      },
    };

    log(Subsystems.MANAGER, Status.INFO,
      `[QueriesManager] Initialising table with ${dataColumns.length} columns from JSON config`);

    // Initialize Tabulator with JSON-resolved columns and table options.
    // selectableRows: 1 is always set here as a safety net — even if the
    // JSON config failed to load and we're using fallback columns, clicking
    // a row should still select it (the fallback tableOptions omit this).
    this.table = new Tabulator(this.elements.tableContainer, {
      ...tableOptions,
      selectableRows: 1,
      // Scroll behavior: scroll to center when row is off-screen,
      // but don't scroll at all if the row is already visible.
      scrollToRowPosition: "center",
      scrollToRowIfVisible: false,
      // Custom dual-arrow sort indicator (▲/▼ with active direction highlighted)
      headerSortElement: '<span class="queries-sort-icons"><span class="queries-sort-asc">▲</span><span class="queries-sort-desc">▼</span></span>',
      columns: [selectorColumn, ...dataColumns],
    });

    // Explicit rowClick handler — select the clicked row.
    // With selectableRows: 1, Tabulator handles single-click selection
    // automatically. This handler is a safety net for edge cases where
    // Tabulator's built-in selection might not fire (e.g., responsive
    // collapse mode, custom formatters returning DOM elements, etc.).
    this.table.on("rowClick", (e, row) => {
      if (!row.isSelected()) {
        this.table.deselectRow();
        row.select();
      }
    });

    // Update selector indicator when rows are selected/deselected.
    // Also persist the selected row's primary key to localStorage so the
    // cursor position survives a refresh and even a re-login.
    this.table.on("rowSelected", (row) => {
      this._updateSelectorCell(row, true);
      const pkField = this.primaryKeyField || 'query_id';
      const pkValue = row.getData()?.[pkField];
      if (pkValue != null) {
        try {
          localStorage.setItem(SELECTED_ROW_KEY, String(pkValue));
        } catch (_) { /* storage full or restricted — ignore */ }
      }
    });

    this.table.on("rowDeselected", (row) => {
      this._updateSelectorCell(row, false);
    });

    this.table.on("rowSelectionChanged", (data, rows) => {
      if (data.length > 0) {
        this.loadQueryDetails(data[0]);
      } else {
        this.clearQueryDetails();
      }
    });

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

      this.table.setData(rows);

      // Discover all columns from the data (adds hidden columns for
      // fields not already defined, so they appear in the column chooser)
      this._discoverColumns(rows);

      // After data is loaded and columns are settled, select a row.
      // Defer to the next animation frame so that any pending Tabulator
      // re-renders triggered by _discoverColumns() (addColumn calls)
      // have a chance to complete first.  Without this deferral, the
      // selection can be silently reset by a trailing re-render.
      requestAnimationFrame(() => {
        this._autoSelectRow(previouslySelectedId);
      });
    } catch (error) {
      // Show detailed error toast with subsystem badge
      toast.error('Query Load Failed', {
        serverError: error.serverError,
        subsystem: 'Conduit',
        duration: 8000,
      });
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
    // Store current query reference
    this.currentQuery = queryData;
    
    // Fetch full query details using QueryRef 27
    this.fetchQueryDetails(queryData.query_id);
  }

  /**
   * Fetch full query details from the API.
   * HTTP request/response logging is handled by json-request.js.
   */
  async fetchQueryDetails(queryId) {
    log(Subsystems.CONDUIT, Status.INFO,
      `[QueriesManager] Fetching query details (queryRef: 27, queryId: ${queryId})`);

    try {
      // QueryRef 27: Get System Query - returns full query details
      const queryDetails = await authQuery(this.app.api, 27, {
        INTEGER: { QUERYID: queryId },
      });

      if (queryDetails && queryDetails.length > 0) {
        const fullQuery = queryDetails[0];

        // Update current query with full details
        this.currentQuery = fullQuery;

        // Get content from API response - using correct field names from QueryRef 27
        const sqlContent = fullQuery.code || fullQuery.query_text || fullQuery.sql || '';
        const summaryContent = fullQuery.summary || fullQuery.markdown || '';
        const collectionContent = fullQuery.collection || fullQuery.json || {};

        // Initialize or update the SQL editor with the query content
        // Only initialize if we're on the SQL tab or if editor doesn't exist
        if (this.sqlEditor) {
          this.sqlEditor.dispatch({
            changes: { from: 0, to: this.sqlEditor.state.doc.length, insert: sqlContent },
          });
        }

        // Update summary content if editor exists
        if (this.summaryEditor) {
          this.summaryEditor.dispatch({
            changes: { from: 0, to: this.summaryEditor.state.doc.length, insert: summaryContent },
          });
        }

        // Update collection content if editor exists
        if (this.collectionEditor) {
          const jsonData = typeof collectionContent === 'object'
            ? collectionContent
            : JSON.parse(collectionContent || '{}');
          this.collectionEditor.set(jsonData);
        }

        // Store data for lazy initialization
        this._pendingSqlContent = sqlContent;
        this._pendingSummaryContent = summaryContent;
        this._pendingCollectionContent = collectionContent;

        // If SQL tab is currently active, initialize the editor immediately
        const sqlTabBtn = document.querySelector('.queries-tab-btn[data-tab="sql"]');
        const isSqlTabActive = sqlTabBtn?.classList.contains('active');

        if (isSqlTabActive && !this.sqlEditor) {
          this.initSqlEditor(sqlContent);
        } else if (this.sqlEditor && sqlContent) {
          // Editor exists, update content
          this.sqlEditor.dispatch({
            changes: { from: 0, to: this.sqlEditor.state.doc.length, insert: sqlContent },
          });
        }

        log(Subsystems.CONDUIT, Status.INFO,
          `[QueriesManager] Loaded query details for ID ${queryId}`);
      }
    } catch (error) {
      // Show detailed error toast - title comes from serverError.error, description from serverError.message
      toast.error('Query Details Failed', {
        serverError: error.serverError,
        subsystem: 'Conduit',
        duration: 8000,
      });
    }
  }

  clearQueryDetails() {
    // Placeholder for clearing tabs
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
            }
          }),
        ],
      });

      this.sqlEditor = new EditorView({
        state: startState,
        parent: this.elements.sqlEditorContainer,
      });
      
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
        ],
      });

      this.summaryEditor = new EditorView({
        state: startState,
        parent: this.elements.summaryEditorContainer,
      });
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
      this.collectionEditor.set(jsonData);
      return;
    }

    try {
      // Dynamic import JSONEditor
      const { default: JSONEditor } = await import('jsoneditor');

      // Create editor container
      this.elements.collectionEditorContainer.innerHTML = '';
      const editorContainer = document.createElement('div');
      editorContainer.style.cssText = 'width:100%;height:100%;';
      this.elements.collectionEditorContainer.appendChild(editorContainer);

      // Initialize JSONEditor with tree view
      this.collectionEditor = new JSONEditor(editorContainer, {
        mode: 'tree',
        modes: ['tree', 'view', 'form', 'code', 'text'],
        search: true,
        history: true,
        navigationBar: true,
        statusBar: true,
        mainMenuBar: true,
        onChange: () => {
          // Optionally handle changes
        },
        onError: (error) => {
          console.error('[QueriesManager] JSONEditor error:', error);
        },
      });

      // Set initial data
      this.collectionEditor.set(jsonData);

      // Add dark theme styling
      editorContainer.classList.add('jsoneditor-theme-dark');

    } catch (error) {
      console.error('[QueriesManager] Failed to initialize JSONEditor:', error);
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
