/**
 * LithiumTable Base Module
 *
 * Core functionality including:
 * - Initialization and configuration loading
 * - Tabulator instance management
 * - Event wiring
 * - State management
 * - Navigation
 *
 * @module core/lithium-table-base
 */

import { TabulatorFull as Tabulator } from 'tabulator-tables';
import { authQuery } from '../shared/conduit.js';
import { toast } from '../shared/toast.js';
import { log, Subsystems, Status } from '../core/log.js';
import { tip, getTip } from '../core/tooltip-api.js';
import {
  loadColtypes,
  loadTableDef,
  resolveColumns,
  resolveTableOptions,
  getQueryRefs,
  getPrimaryKeyField,
  preloadLookups,
} from './lithium-table.js';

// ── Column Utility Functions ────────────────────────────────────────────────

/**
 * Sanitize column titles for safe use as CSS class names
 * Replaces spaces and special characters, but preserves case
 */
export function sanitizeColumnTitle(title) {
  if (!title || typeof title !== 'string') return 'col';
  return title.replace(/[^a-zA-Z0-9_-]/g, '_').replace(/_+/g, '_');
}

// ── LithiumTableBase Class ─────────────────────────────────────────────────

export class LithiumTableBase {
  /**
   * Create a LithiumTable instance
   * @param {Object} options - Configuration options
   */
  constructor(options) {
    // Core elements
    this.container = options.container;
    this.navigatorContainer = options.navigatorContainer;
    this.tablePath = options.tablePath;
    this.cssPrefix = options.cssPrefix || 'lithium';

    // Configuration (can be provided or loaded)
    this.tableDef = options.tableDef || null;
    this.coltypes = options.coltypes || null;

    // Query references
    this.queryRefs = {
      queryRef: options.queryRef || null,
      searchQueryRef: options.searchQueryRef || null,
      detailQueryRef: options.detailQueryRef || null,
      insertQueryRef: options.insertQueryRef || null,
      updateQueryRef: options.updateQueryRef || null,
      deleteQueryRef: options.deleteQueryRef || null,
    };

    // App instance for API calls
    this.app = options.app || null;

    // State flags
    this.readonly = options.readonly || false;
    this.isEditing = false;
    this.filtersVisible = false;
    this.isDirty = false;
    this.editTransitionPending = false;

    // Editing state
    this.editingRowId = null;
    this.originalRowData = null;
    this.columnEditors = new Map();

    // UI state
    this.tableWidthMode = 'compact';
    this.tableLayoutMode = 'fitColumns';

    // Storage key for persistence
    this.storageKey = options.storageKey || `${this.cssPrefix}_table`;

    // Panel persistence (centralized width + collapsed state)
    this.panel = options.panel || null;
    this.panelStateManager = options.panelStateManager || null;
    this.splitter = null; // Set via setSplitter() after init

    // Data cache
    this.currentData = [];

    // Metadata
    this.primaryKeyField = null;

    // References for cleanup
    this.table = null;
    this.loaderElement = null;

    // Callbacks
    this.onRowSelected = options.onRowSelected || (() => {});
    this.onRowDeselected = options.onRowDeselected || (() => {});
    this.onDataLoaded = options.onDataLoaded || (() => {});
    this.onEditModeChange = options.onEditModeChange || (() => {});
    this.onDirtyChange = options.onDirtyChange || null; // Called when dirty state changes (isDirty, rowData)
    this.onSetTableWidth = options.onSetTableWidth || null;
    this.onRefresh = options.onRefresh || null;
    this.onExecuteSave = options.onExecuteSave || null; // Custom save logic — async (row, editHelper) => {}
    this.onDuplicate = options.onDuplicate || null; // Custom duplicate logic — async (rowData) => newRowData

    // Column manager option
    this.useColumnManager = options.useColumnManager !== false; // Default to true

    // Always-editable mode: cells are directly editable without entering edit mode.
    // Used by the Column Manager which manages its own save/cancel workflow.
    this.alwaysEditable = options.alwaysEditable === true;

    // Popup state
    this.activeNavPopup = null;
    this.activeNavPopupId = null;
    this.navPopupCloseHandler = null;
  }

  // ── Initialization ────────────────────────────────────────────────────────

  async init() {
    await this.loadConfiguration();
    this.injectSortIconStyles();
    await this.initTable();
    this.buildNavigator();
    this.setupKeyboardHandling();
    await this.setupPersistence();
  }

  /**
   * Inject styles for the current cssPrefix
   * This ensures all LithiumTable styles work regardless of the prefix used
   */
  injectSortIconStyles() {
    // Only inject if using a custom prefix (not the default 'lithium')
    if (this.cssPrefix === 'lithium') return;

    // Check if styles for this prefix already exist
    const styleId = `lithium-table-${this.cssPrefix}`;
    if (document.getElementById(styleId)) {
      log(Subsystems.TABLE, Status.DEBUG, `[LithiumTable] Sort icons already injected for prefix: ${this.cssPrefix}`);
      return;
    }

    // Only inject sort icon styles — Navigator styles are provided globally
    // by lithium-table.css using the "lithium-" prefix. All tables share
    // the same Navigator styling via CSS variables.
    const cssText = `
@layer managers {
  /* ── Sort Icons for prefix: ${this.cssPrefix} ── */
  .${this.cssPrefix}-sort-icons {
    display: inline-flex;
    flex-direction: column;
    line-height: 1;
    font-size: 7px;
    margin-left: var(--space-1);
    vertical-align: middle;
    gap: 0;
  }

  .${this.cssPrefix}-sort-asc,
  .${this.cssPrefix}-sort-desc {
    color: var(--text-muted);
    opacity: 0.35;
    transition: opacity var(--transition-fast), color var(--transition-fast);
  }

  .lithium-table-container .tabulator-col[aria-sort="ascending"] .${this.cssPrefix}-sort-asc {
    color: white;
    opacity: 1;
  }

  .lithium-table-container .tabulator-col[aria-sort="descending"] .${this.cssPrefix}-sort-desc {
    color: white;
    opacity: 1;
  }

  .lithium-table-container .tabulator-col[aria-sort="none"] .${this.cssPrefix}-sort-asc,
  .lithium-table-container .tabulator-col[aria-sort="none"] .${this.cssPrefix}-sort-desc {
    color: var(--text-muted);
    opacity: 0.35;
  }
}
`;

    const styleEl = document.createElement('style');
    styleEl.id = styleId;
    styleEl.textContent = cssText;
    document.head.appendChild(styleEl);

    log(Subsystems.TABLE, Status.DEBUG, `[LithiumTable] Injected sort icons for prefix: ${this.cssPrefix}`);

    // Store reference for cleanup
    this._sortStyleEl = styleEl;
  }

  async loadConfiguration() {
    if (!this.coltypes) {
      this.coltypes = await loadColtypes();
    }

    if (!this.tableDef && this.tablePath) {
      this.tableDef = await loadTableDef(this.tablePath);
    }

    if (this.tableDef) {
      const defQueryRefs = getQueryRefs(this.tableDef);
      // Merge: JSON config values are the base, constructor options override only when non-null.
      // Without this filter, explicit null values from the constructor (when the manager
      // doesn't pass e.g. updateQueryRef) would overwrite valid values from the JSON config.
      const nonNullOverrides = Object.fromEntries(
        Object.entries(this.queryRefs).filter(([, v]) => v != null)
      );
      this.queryRefs = { ...defQueryRefs, ...nonNullOverrides };
      this.primaryKeyField = getPrimaryKeyField(this.tableDef);

      const lookupRefs = Object.values(this.tableDef.columns || {})
        .map(col => col.lookupRef)
        .filter(Boolean);
      const uniqueRefs = [...new Set(lookupRefs)];

      if (uniqueRefs.length > 0 && this.app?.api) {
        try {
          await preloadLookups(uniqueRefs, authQuery, this.app.api);
          log(Subsystems.TABLE, Status.SUCCESS,
            `[LithiumTable] Pre-loaded ${uniqueRefs.length} lookup table(s)`);
        } catch (err) {
          log(Subsystems.TABLE, Status.WARN,
            `[LithiumTable] Lookup pre-load failed: ${err.message}`);
        }
      }
    }
  }

  async initTable() {
    // Add base LithiumTable class for shared styling
    this.container.classList.add('lithium-table-container');

    const dataColumns = this.tableDef && this.coltypes
      ? resolveColumns(this.tableDef, this.coltypes, {
          filterEditor: this.createFilterEditorFunction(),
        })
      : this.getFallbackColumns();

    this.applyEditModeGate(dataColumns);

    const tableOptions = this.tableDef
      ? resolveTableOptions(this.tableDef)
      : { layout: 'fitColumns', responsiveLayout: false };

    // Lithium tables always prefer horizontal scrolling over Tabulator's
    // responsive column hiding/collapse modes. Preserve resolveTableOptions()
    // as a faithful schema mapper, but override the runtime behavior here.
    tableOptions.responsiveLayout = false;

    const persistedLayout = this.restoreLayoutMode();
    if (persistedLayout) {
      tableOptions.layout = persistedLayout;
    }
    this.tableLayoutMode = tableOptions.layout || 'fitColumns';

    const selectorColumn = this.buildSelectorColumn();

    const columns = [selectorColumn, ...dataColumns];
    columns.forEach(col => {
      if (col.title && !col.cssClass?.includes('first-visible-col')) {
        col.cssClass = [col.cssClass, sanitizeColumnTitle(col.title)].filter(Boolean).join(' ');
      }
    });

    this.table = new Tabulator(this.container, {
      ...tableOptions,
      selectableRows: 1,
      // Note: We intentionally do NOT set editTriggerEvent. This ensures
      // Tabulator never auto-activates editors - we control all editor
      // activation programmatically via enterEditMode() and queueCellEdit().
      // This prevents editors from being active when not in edit mode.
      editTriggerEvent: false,
      scrollToRowPosition: 'center',
      scrollToRowIfVisible: false,
      headerSortTristate: true,
      headerSortElement: (column, dir) => {
        const prefix = this.cssPrefix;
        return `<span class="${prefix}-sort-icons" data-sort-dir="${dir || 'none'}"><span class="${prefix}-sort-asc">&#9650;</span><span class="${prefix}-sort-desc">&#9660;</span></span>`;
      },
      columns: columns,
    });

    this.wireTableEvents();

    // Wait for tableBuilt event to ensure Tabulator is fully initialized
    // This prevents issues with calling methods like replaceData before
    // the renderer is ready
    await new Promise((resolve) => {
      if (this.table.modules?.table && this.table.renderer) {
        // Table is already built
        resolve();
        return;
      }

      let resolved = false;
      const once = () => {
        if (!resolved) {
          resolved = true;
          resolve();
        }
      };

      // tableBuilt fires after renderer is ready
      this.table.on('tableBuilt', once);

      // Fallback: check periodically if table is built
      let attempts = 0;
      const maxAttempts = 10;
      const checkInterval = setInterval(() => {
        attempts++;
        if (this.table.modules?.table && this.table.renderer) {
          clearInterval(checkInterval);
          once();
        } else if (attempts >= maxAttempts) {
          clearInterval(checkInterval);
          once(); // Resolve anyway after max attempts
        }
      }, 50);
    });

    // Initialize FloatingUI tooltips on column headers
    // Use setTimeout to ensure Tabulator has rendered the header elements
    setTimeout(() => this.initColumnHeaderTooltips(), 100);
  }

  wireTableEvents() {
    if (!this.table) return;

    this.table.on('rowClick', (e, row) => {
      if (this.isCalcRow(row)) return;

      // Skip row selection if clicking on drag handle cell (row reordering)
      // The drag_handle column has rowHandle: true for movable rows
      if (e?.target) {
        const cell = e.target.closest?.('.tabulator-cell');
        if (cell?.getAttribute?.('data-tabulator-field') === 'drag_handle') {
          return;
        }
      }

      this.selectDataRow(row);
    });

    this.table.on('cellMouseDown', (e, cell) => {
      const field = cell.getField();
      // Skip selection for selector column and drag handle column (row reordering)
      if (field === '_selector' || field === 'drag_handle' || this.isCalcCell(cell)) return;
      this.selectDataRow(cell.getRow());
    });

    this.table.on('rowDblClick', (e, row) => {
      if (this.isCalcRow(row) || this.readonly || this.alwaysEditable) return;
      this.selectDataRow(row);
      void this.enterEditMode(row);
    });

    this.table.on('cellDblClick', (e, cell) => {
      const row = cell?.getRow?.();
      const field = cell?.getField?.();
      if (!row || this.isCalcRow(row) || this.readonly || this.alwaysEditable) return;
      if (field === '_selector' || field === 'drag_handle' || this.isCalcCell(cell)) return;

      this.selectDataRow(row);
      void this.enterEditMode(row, cell);
    });

    this.table.on('cellClick', (e, cell) => {
      const field = cell.getField();
      // Skip selection for selector column and drag handle column (row reordering)
      if (field === '_selector' || field === 'drag_handle' || this.isCalcCell(cell)) return;

      e?.stopPropagation?.();
      e?.stopImmediatePropagation?.();

      this.selectDataRow(cell.getRow());

      if (this.isEditing) {
        this.commitActiveCellEdit(cell);
        this.queueCellEdit(cell);
      }
    });

    this.table.on('cellEdited', () => {
      this.isDirty = true;
      this.updateSaveCancelButtonState();
      this.notifyDirtyChange();
    });

    this.table.on('rowSelected', async (row) => {
      if (this.isCalcRow(row)) return;
      await this.handleRowSelected(row);
    });

    this.table.on('rowDeselected', (row) => {
      if (this.isCalcRow(row)) return;
      this.updateSelectorCell(row, false);
    });

    this.table.on('rowSelectionChanged', () => {
      this.updateMoveButtonState();
      this.updateDuplicateButtonState();
    });

    this.table.on('columnVisibilityChanged', () => {
      this.updateVisibleColumnClasses();
      this.initColumnHeaderTooltips();
    });

    this.table.on('columnMoved', () => {
      this.updateVisibleColumnClasses();
      this.initColumnHeaderTooltips();
    });

    this.table.on('tableBuilt', () => {
      setTimeout(() => {
        this.updateVisibleColumnClasses();
        this.initColumnHeaderTooltips();
      }, 100);
    });

    this.table.on('rowRendered', (row) => {
      this.applyVisibleColumnClassesToRow(row);
    });

    this.table.on('dataLoaded', () => {
      setTimeout(() => {
        this.updateVisibleColumnClasses();
        this.initColumnHeaderTooltips();
      }, 100);
    });

    this.table.on('dataProcessed', () => {
      this.updateMoveButtonState();
      this.updateDuplicateButtonState();
    });
  }

  // ── Row Selection & Navigation ────────────────────────────────────────────

  async handleRowSelected(row) {
    const pkField = this.primaryKeyField || 'id';
    const newRowId = row.getData()?.[pkField];

    if (this.isEditing && this.editingRowId !== newRowId) {
      // Synchronous dirty check — the rAF-deferred isDirty flag may be stale
      // if the user typed in an editor and immediately clicked a different row.
      const actuallyDirty = this.isActuallyDirty();

      if (actuallyDirty) {
        // Default behaviour: auto-save the editing row, then switch.
        const saveSucceeded = await this.autoSaveBeforeRowChange();
        if (!saveSucceeded) {
          const editingRow = this.getEditingRow();
          if (editingRow) editingRow.select();
          return;
        }
        await this.exitEditMode('save');
      } else {
        await this.exitEditMode('row-change');
      }
    }

    this.updateSelectorCell(row, true);

    // Save selected row ID for persistence across sessions
    if (newRowId != null) {
      this.saveSelectedRowId(newRowId);
    }

    this.onRowSelected(row.getData());
  }

  selectDataRow(row) {
    if (!this.table || !row || this.isCalcRow(row)) return false;

    const currentRow = this.getSelectedDataRow();
    if (this.rowsMatch(currentRow, row)) {
      if (!row.isSelected?.()) row.select();
      return false;
    }

    this.closeTransientPopups();
    this.table.deselectRow();
    row.select();
    return true;
  }

  getSelectedDataRow() {
    if (!this.table) return null;
    const selectedRows = this.table.getSelectedRows?.() || [];
    return selectedRows.find(row => !this.isCalcRow(row)) || null;
  }

  getEditingRow() {
    if (!this.table || !this.isEditing || this.editingRowId == null) return null;

    const pkField = this.primaryKeyField || 'id';
    const rows = this.table.getRows('active');
    return rows.find(row => row.getData()?.[pkField] === this.editingRowId) || null;
  }

  isCalcRow(row) {
    if (!row) return false;
    const rowEl = row.getElement?.();
    return !!rowEl?.closest?.('.tabulator-calcs, .tabulator-calcs-holder, .tabulator-calcs-bottom');
  }

  isCalcCell(cell) {
    if (!cell) return false;
    const cellEl = cell.getElement?.();
    if (cellEl?.closest?.('.tabulator-calcs, .tabulator-calcs-holder, .tabulator-calcs-bottom')) {
      return true;
    }
    return this.isCalcRow(cell.getRow?.());
  }

  rowsMatch(rowA, rowB) {
    if (!rowA || !rowB) return false;
    if (rowA === rowB) return true;

    const pkField = this.primaryKeyField || 'id';
    const rowAId = rowA.getData?.()?.[pkField];
    const rowBId = rowB.getData?.()?.[pkField];
    return rowAId != null && rowBId != null && String(rowAId) === String(rowBId);
  }

  getVisibleRowsAndIndex() {
    if (!this.table) return { rows: [], selectedIndex: -1 };
    const rows = this.table.getRows('active');
    const selected = this.table.getSelectedRows();
    let selectedIndex = -1;
    if (selected.length > 0) {
      const selPos = selected[0].getPosition(true);
      selectedIndex = selPos - 1;
    }
    return { rows, selectedIndex };
  }

  selectRowByIndex(index) {
    if (!this.table) return;
    const rows = this.table.getRows('active');
    if (index < 0 || index >= rows.length) return;
    this.table.deselectRow();
    rows[index].select();
    rows[index].scrollTo();
  }

  getPageSize() {
    if (!this.table) return 10;
    const holder = this.container?.querySelector('.tabulator-tableholder');
    if (!holder) return 10;
    const rowEl = holder.querySelector('.tabulator-row');
    if (!rowEl) return 10;
    const rowHeight = rowEl.offsetHeight || 30;
    const visibleHeight = holder.clientHeight;
    return Math.max(1, Math.floor(visibleHeight / rowHeight));
  }

  /**
   * Helper shared by all navigation methods: if editing, auto-save when
   * dirty or exit when clean.  Uses isActuallyDirty() for a synchronous
   * dirty check.
   *
   * @returns {boolean} true if navigation may proceed, false if blocked
   */
  async _exitEditModeForNavigation() {
    if (!this.isEditing) return true;

    if (this.isActuallyDirty()) {
      const saveSucceeded = await this.autoSaveBeforeRowChange();
      if (!saveSucceeded) return false;
      await this.exitEditMode('save');
    } else {
      await this.exitEditMode('cancel');
    }
    return true;
  }

  async navigateFirst() {
    if (!this.table) return;
    if (!(await this._exitEditModeForNavigation())) return;
    this.selectRowByIndex(0);
  }

  async navigateLast() {
    if (!this.table) return;
    if (!(await this._exitEditModeForNavigation())) return;
    const rows = this.table?.getRows('active') || [];
    if (rows.length > 0) {
      this.selectRowByIndex(rows.length - 1);
    }
  }

  async navigatePrevRec() {
    if (!(await this._exitEditModeForNavigation())) return;
    const { selectedIndex } = this.getVisibleRowsAndIndex();
    if (selectedIndex > 0) {
      this.selectRowByIndex(selectedIndex - 1);
    }
  }

  async navigateNextRec() {
    if (!(await this._exitEditModeForNavigation())) return;
    const { rows, selectedIndex } = this.getVisibleRowsAndIndex();
    if (selectedIndex < rows.length - 1) {
      this.selectRowByIndex(selectedIndex + 1);
    }
  }

  async navigatePrevPage() {
    if (!(await this._exitEditModeForNavigation())) return;
    const { selectedIndex } = this.getVisibleRowsAndIndex();
    const pageSize = this.getPageSize();
    const newIndex = Math.max(0, selectedIndex - pageSize);
    this.selectRowByIndex(newIndex);
  }

  async navigateNextPage() {
    if (!(await this._exitEditModeForNavigation())) return;
    const { rows, selectedIndex } = this.getVisibleRowsAndIndex();
    const pageSize = this.getPageSize();
    const newIndex = Math.min(rows.length - 1, selectedIndex + pageSize);
    this.selectRowByIndex(newIndex);
  }

  // ── Data Loading ──────────────────────────────────────────────────────────

  async loadData(searchTerm = '', extraParams = {}) {
    if (!this.app?.api) return;

    // Get currently selected row, or restore from localStorage
    const previouslySelectedId = this.getSelectedRowId() ?? this.restoreSelectedRowId();
    this.showLoading();

    try {
      const listRef = this.queryRefs.queryRef;
      const searchRef = this.queryRefs.searchQueryRef;
      const queryRef = searchTerm && searchRef ? searchRef : listRef;

      if (!queryRef) {
        log(Subsystems.TABLE, Status.WARN, `[LithiumTable] No queryRef configured for data loading`);
        return;
      }

      log(Subsystems.CONDUIT, Status.INFO,
        `[LithiumTable] Loading data (queryRef: ${queryRef}${searchTerm ? `, search: "${searchTerm}"` : ''})`);

      const params = { ...extraParams };

      if (searchTerm && searchRef) {
        params.STRING = { SEARCH: searchTerm.toUpperCase() };
      }

      const rows = await authQuery(this.app.api, queryRef, Object.keys(params).length > 0 ? params : undefined);

      if (!this.table) return;

      this.currentData = rows;

      this.table.blockRedraw?.();
      try {
        this.table.setData(rows);
        this.discoverColumns(rows);
      } finally {
        this.table.restoreRedraw?.();
      }

      requestAnimationFrame(() => {
        this.autoSelectRow(previouslySelectedId);
        this.updateMoveButtonState();
        this.updateDuplicateButtonState();
      });

      this.onDataLoaded(rows);
    } catch (error) {
      // Build informative error details
      const queryRef = searchTerm && this.queryRefs.searchQueryRef
        ? this.queryRefs.searchQueryRef
        : this.queryRefs.queryRef;
      const errorDetails = error.serverError || error.message || 'Unknown error';
      const errorContext = `QueryRef: ${queryRef}${searchTerm ? `, Search: "${searchTerm}"` : ''}`;

      log(Subsystems.TABLE, Status.ERROR,
        `[LithiumTable] Data load failed: ${errorDetails} (${errorContext})`);

      toast.error('Data Load Failed', {
        description: `${errorDetails} (${errorContext})`,
        duration: 8000,
      });
      this.currentData = [];
    } finally {
      this.hideLoading();
    }
  }

  getSelectedRowId() {
    if (!this.table) return null;
    const selected = this.table.getSelectedRows();
    if (selected.length > 0) {
      const pkField = this.primaryKeyField || 'id';
      return selected[0].getData()[pkField] ?? null;
    }
    return null;
  }

  getSelectedRowData() {
    if (!this.table) return null;
    const selected = this.table.getSelectedRows();
    if (selected.length > 0) {
      return selected[0].getData();
    }
    return null;
  }

  autoSelectRow(targetId) {
    if (!this.table) return;
    const rows = this.table.getRows('active');
    if (rows.length === 0) return;

    const pkField = this.primaryKeyField;

    if (targetId != null && pkField !== null) {
      for (const row of rows) {
        if (String(row.getData()[pkField]) === String(targetId)) {
          row.select();
          row.scrollTo();
          return;
        }
      }
    }

    rows[0].select();
    rows[0].scrollTo();
  }

  /**
   * Load static (non-API) data into the table.
   * Encapsulates the blockRedraw/setData/discoverColumns pattern used when
   * data is hardcoded, pre-fetched, or otherwise not loaded via queryRef.
   *
   * @param {Array<Object>} rows - The data array to load
   * @param {Object} [options]
   * @param {boolean} [options.autoSelect=true] - Auto-select first row after load
   */
  loadStaticData(rows, options = {}) {
    if (!this.table) return;
    const { autoSelect = true } = options;

    this.currentData = rows || [];

    this.table.blockRedraw?.();
    try {
      this.table.setData(this.currentData);
      this.discoverColumns(this.currentData);
    } finally {
      this.table.restoreRedraw?.();
    }

    if (autoSelect) {
      requestAnimationFrame(() => {
        const activeRows = this.table.getRows('active');
        if (activeRows.length > 0) {
          activeRows[0].select();
          activeRows[0].scrollTo();
        }
        this.updateMoveButtonState();
        this.updateDuplicateButtonState();
      });
    }

    this.onDataLoaded(this.currentData);
  }

  // ── Column Management ─────────────────────────────────────────────────────

  buildSelectorColumn() {
    return {
      title: '',
      field: '_selector',
      frozen: true,
      width: 16,
      minWidth: 16,
      maxWidth: 16,
      resizable: false,
      headerSort: false,
      hozAlign: 'center',
      vertAlign: 'middle',
      cssClass: `${this.cssPrefix}-selector-col`,
      titleFormatter: () => {
        const wrapper = document.createElement('span');
        wrapper.className = `${this.cssPrefix}-col-chooser-btn`;
        wrapper.title = 'Column Manager';
        wrapper.innerHTML = '<fa fa-ellipsis-stroke-vertical>';
        return wrapper;
      },
      formatter: (cell) => {
        const row = cell.getRow();
        const pkField = this.primaryKeyField || 'id';
        const rowData = row.getData?.() || {};
        if (row.isSelected()) {
          if (this.isEditing && this.editingRowId === rowData[pkField]) {
            return `<span class="${this.cssPrefix}-selector-indicator ${this.cssPrefix}-selector-edit">&#10073;</span>`;
          }
          return `<span class="${this.cssPrefix}-selector-indicator ${this.cssPrefix}-selector-active">&#9658;</span>`;
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
        this.selectDataRow(row);
      },
    };
  }

  applyEditModeGate(columns) {
    this.columnEditors = new Map();

    for (const col of columns) {
      if (!col.editor) continue;

      this.columnEditors.set(col.field, {
        editor: col.editor,
        editorParams: col.editorParams || undefined,
      });

      // In alwaysEditable mode, cells are directly editable without
      // entering edit mode. Used by the Column Manager's LithiumTable.
      if (this.alwaysEditable) {
        col.editable = (cell) => {
          const row = cell.getRow();
          return row && !this.isCalcRow(row);
        };
        continue;
      }

      col.editable = (cell) => {
        if (!this.isEditing) return false;

        const pkField = this.primaryKeyField || 'id';
        const row = cell.getRow();
        if (!row || this.isCalcRow(row)) return false;

        const rowId = row.getData?.()?.[pkField];
        return this.editingRowId != null
          ? rowId === this.editingRowId
          : row.isSelected?.();
      };
    }
  }

  discoverColumns(rows) {
    if (!this.table || !rows || rows.length === 0) return;

    const allKeys = new Set();
    for (const row of rows) {
      for (const key of Object.keys(row)) {
        if (key !== '_selector') {
          allKeys.add(key);
        }
      }
    }

    const existingFields = new Set(
      this.table.getColumns().map(col => col.getField()).filter(Boolean)
    );

    const filterEditorFn = this.createFilterEditorFunction();
    for (const key of allKeys) {
      if (existingFields.has(key)) continue;

      const title = key
        .replace(/_/g, ' ')
        .replace(/\b\w/g, c => c.toUpperCase());

      this.table.addColumn({
        title,
        field: key,
        resizable: true,
        headerSort: true,
        headerFilter: filterEditorFn,
        headerFilterFunc: 'like',
        visible: false,
      });
    }

    // Attach tooltips to newly discovered columns
    this.initColumnHeaderTooltips();
  }

  getFallbackColumns() {
    log(Subsystems.TABLE, Status.WARN, `[LithiumTable] Using fallback columns`);
    const filterEditorFn = this.createFilterEditorFunction();
    const cols = [
      {
        title: 'ID',
        field: 'id',
        width: 80,
        resizable: true,
        headerSort: true,
        headerSortTristate: true,
        headerFilter: filterEditorFn,
        headerFilterFunc: 'like',
      },
      {
        title: 'Name',
        field: 'name',
        resizable: true,
        headerSort: true,
        headerSortTristate: true,
        headerFilter: filterEditorFn,
        headerFilterFunc: 'like',
      },
    ];
    cols.forEach(col => {
      if (col.title) {
        col.cssClass = sanitizeColumnTitle(col.title);
      }
    });
    return cols;
  }

  // ── Lifecycle ─────────────────────────────────────────────────────────────

  cleanup() {
    this.closeTransientPopups();
  }

  destroy() {
    this.cleanup();

    if (this.table) {
      this.table.destroy();
      this.table = null;
    }

    // Remove injected sort icon styles
    if (this._sortStyleEl) {
      this._sortStyleEl.remove();
      this._sortStyleEl = null;
    }

    this.hideLoading();
  }

  // ── Splitter Binding ────────────────────────────────────────────────────────

  /**
   * Bind a LithiumSplitter instance to this table.
   * Must be called AFTER init() and AFTER creating the splitter.
   * @param {LithiumSplitter} splitter
   */
  setSplitter(splitter) {
    this.splitter = splitter;
  }

  // ── Width Presets ──────────────────────────────────────────────────────────

  /**
   * Get the width in pixels for a named mode.
   * @param {string} mode - 'narrow', 'compact', 'normal', 'wide', or 'auto'
   * @returns {number|null} Width in pixels, or null for 'auto'
   */
  getWidthForMode(mode) {
    const widths = { narrow: 160, compact: 314, normal: 468, wide: 622 };
    return widths[mode] || null;
  }

  // ── Panel Width Persistence ────────────────────────────────────────────────

  /**
   * Apply width to the panel element based on mode.
   * Called during init (restore) and when user picks a width preset.
   * @param {string|null} mode - Width mode or null for pixel fallback
   */
  applyPanelWidth(mode) {
    if (!this.panel) {
      console.warn('[LithiumTable] applyPanelWidth: no panel!', { storageKey: this.storageKey });
      return;
    }

    // Note: before/offsetBefore variables available for debugging if needed
    // const before = this.panel.style.width;
    // const offsetBefore = this.panel.offsetWidth;

    if (mode === null) {
      // No saved mode — use PanelStateManager pixel width as fallback
      if (this.panelStateManager) {
        const width = this.panelStateManager.loadWidth(280);
        this.panel.style.width = `${width}px`;
      }
    } else if (mode === 'auto') {
      this.panel.style.width = '';
      const currentWidth = this.panel.offsetWidth;
      if (this.panelStateManager) {
        this.panelStateManager.saveWidth(currentWidth);
      }
    } else {
      // Named mode
      const widthPx = this.getWidthForMode(mode);
      if (widthPx) {
        this.panel.style.width = `${widthPx}px`;
        if (this.panelStateManager) {
          this.panelStateManager.saveWidth(widthPx);
        }
      }
    }

    // console.log('[LithiumTable] applyPanelWidth', {
    //   storageKey: this.storageKey,
    //   mode,
    //   panelId: this.panel.id,
    //   styleBefore: before,
    //   styleAfter: this.panel.style.width,
    //   offsetBefore,
    //   offsetAfter: this.panel.offsetWidth,
    // });
  }

  /**
   * Save pixel width from splitter drag.
   * Called by the manager's onResizeEnd callback.
   * @param {number} width
   */
  savePanelPixelWidth(width) {
    if (this.panelStateManager) {
      this.panelStateManager.saveWidth(width);
    }
  }

  // ── Panel Collapsed State Persistence ─────────────────────────────────────

  /**
   * Load collapsed state from persistence.
   * @param {boolean} defaultValue
   * @returns {boolean}
   */
  loadCollapsedState(defaultValue = false) {
    if (this.panelStateManager) {
      return this.panelStateManager.loadCollapsed(defaultValue);
    }
    return defaultValue;
  }

  /**
   * Save collapsed state to persistence.
   * @param {boolean} collapsed
   */
  saveCollapsedState(collapsed) {
    if (this.panelStateManager) {
      this.panelStateManager.saveCollapsed(collapsed);
    }
  }

  // ── Dirty State ────────────────────────────────────────────────────────────

  /**
   * Synchronous dirty check that also consults the ManagerEditHelper.
   *
   * The editHelper's `checkDirtyState()` is deferred via rAF, so the
   * table's `isDirty` flag may be stale if the user edits a CodeMirror
   * editor and immediately triggers a row change.  This method performs
   * an immediate comparison against the snapshot so callers always get
   * the true dirty state.
   *
   * @returns {boolean}
   */
  isActuallyDirty() {
    if (this.isDirty) return true;
    return this._editHelper?.isAnythingDirty?.() ?? false;
  }

  /**
   * Notify the manager that dirty state has changed.
   * Called whenever isDirty changes, allowing managers to update UI (e.g., Save/Cancel buttons).
   */
  notifyDirtyChange() {
    if (typeof this.onDirtyChange === 'function') {
      this.onDirtyChange(this.isDirty, this.isEditing ? this.getSelectedRowData() : null);
    }
  }

  // ── Column Header Tooltips ─────────────────────────────────────────────────

  /**
   * Initialize FloatingUI tooltips on column headers.
   * Called after the table is built to attach tooltips to all column header elements.
   * Safe to call multiple times - will skip headers that already have tooltips.
   */
  initColumnHeaderTooltips() {
    if (!this.table) return;

    // Find all column header title elements (the actual text container)
    const titleEls = this.container.querySelectorAll('.tabulator-col-title');

    titleEls.forEach((titleEl) => {
      // Skip if this element already has a tooltip
      if (getTip(titleEl)) return;

      // Find the parent column element to get the field name
      const headerEl = titleEl.closest('.tabulator-col');
      if (!headerEl) return;

      const field = headerEl.getAttribute('tabulator-field');
      if (!field || field === '_selector') return;

      // Find the column definition from tableDef
      let colDef = this.getColumnDefinition(field);

      // If not in tableDef (e.g., discovered column), create a minimal definition
      if (!colDef) {
        const display = titleEl.textContent?.trim() || field;
        colDef = {
          field: field,
          display: display,
          description: null,
          calculated: false,
        };
      }

      // Build tooltip content
      const tooltipContent = this.buildColumnTooltipContent(colDef);
      if (!tooltipContent) return;

      // Attach FloatingUI tooltip to the title element with shorter delay
      tip(titleEl, tooltipContent, {
        placement: 'top',
        maxWidth: 400,
        delay: { show: 600, hide: 100 },
      });
    });
  }

  /**
   * Get column definition from tableDef by field name.
   * @param {string} field - The field name
   * @returns {Object|null} The column definition or null
   */
  getColumnDefinition(field) {
    if (!this.tableDef?.columns) return null;

    for (const [, colDef] of Object.entries(this.tableDef.columns)) {
      if (colDef.field === field) {
        return colDef;
      }
    }
    return null;
  }

  /**
   * Build tooltip content for a column based on the business rules:
   * 1. If description and field exist: show description + newline + field (styled)
   * 2. If no description: use column name (display/title)
   * 3. If calculated: add asterisk before field name
   * 4. If no field: use "*auto"
   *
   * @param {Object} colDef - The column definition
   * @returns {string} HTML content for the tooltip
   */
  buildColumnTooltipContent(colDef) {
    const description = colDef.description || null;
    const display = colDef.display || colDef.field || '';
    const field = colDef.field || null;
    const isCalculated = colDef.calculated === true;

    // Rule 4: If no field name available (or null), use "*auto"
    if (!field) {
      return '<span class="li-tip-field">*auto</span>';
    }

    // Rule 3: If calculated, add asterisk before field name
    const fieldDisplay = isCalculated ? `*${field}` : field;

    // Rule 1: If we have both description and field, show both
    if (description && field) {
      return `${this.escapeHtml(description)}<br><span class="li-tip-field">${this.escapeHtml(fieldDisplay)}</span>`;
    }

    // Rule 2: If no description, use column name (display/title)
    if (!description) {
      return `${this.escapeHtml(display)}<br><span class="li-tip-field">${this.escapeHtml(fieldDisplay)}</span>`;
    }

    // Fallback: just return description with field
    return `${this.escapeHtml(description)}<br><span class="li-tip-field">${this.escapeHtml(fieldDisplay)}</span>`;
  }

  /**
   * Escape HTML special characters to prevent XSS.
   * @param {string} text - The text to escape
   * @returns {string} Escaped text
   */
  escapeHtml(text) {
    if (!text) return '';
    const div = document.createElement('div');
    div.textContent = text;
    return div.innerHTML;
  }

  // ── Abstract Methods (to be implemented by subclasses) ─────────────────────

  buildNavigator() { /* To be implemented */ }
  createFilterEditor() { return null; }
  updateMoveButtonState() { /* To be implemented */ }
  updateDuplicateButtonState() { /* To be implemented */ }
  toggleColumnChooser() { /* To be implemented */ }
  closeTransientPopups() { /* To be implemented */ }
  enterEditMode() { /* To be implemented */ }
  exitEditMode() { /* To be implemented */ }
  updateSaveCancelButtonState() { /* To be implemented */ }
  autoSaveBeforeRowChange() { return Promise.resolve(true); }
  updateSelectorCell() { /* To be implemented */ }
  commitActiveCellEdit() { /* To be implemented */ }
  queueCellEdit() { /* To be implemented */ }
  updateVisibleColumnClasses() { /* To be implemented */ }
  applyVisibleColumnClassesToRow() { /* To be implemented */ }
  setupKeyboardHandling() { /* To be implemented */ }
  showLoading() { /* To be implemented */ }
  hideLoading() { /* To be implemented */ }
  setupPersistence() { /* To be implemented */ }
  restoreLayoutMode() { return null; }
  updateEditButtonState() { /* To be implemented */ }
}

export default LithiumTableBase;
