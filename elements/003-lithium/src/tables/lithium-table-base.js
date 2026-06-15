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
import { log, Subsystems, Status } from '../core/log.js';
import {
  loadColtypes,
  loadTableDef,
  resolveColumnsWithMeta,
  resolveTableOptions,
  getQueryRefs,
  getPrimaryKeyField,
  preloadLookups,
} from './lithium-table.js';
import { GroupIconAnimator } from './visual/group-icon-animator.js';
import { wireTableEvents } from './events/table-events.js';
import { initColumnHeaderTooltips } from './visual/column-tooltips.js';
import {
  getWidthForMode,
  applyPanelWidth,
  savePanelPixelWidth,
  loadCollapsedState,
  saveCollapsedState,
} from './persistence/panel-width.js';
import { reloadConfiguration, getCurrentlySelectedRowId } from './refresh-orchestrator.js';
import {
  captureCurrentState,
  createTemplateColumnFromTableDef,
  extractTemplateColumnFromColumn,
} from './template/capture.js';
import {
  loadData,
  getSelectedRowId,
  getSelectedRowData,
  autoSelectRow,
  loadStaticData,
  loadInitialData,
  getCompositeRowId,
} from './data/data-loading.js';
import {
  navigateFirst,
  navigateLast,
  navigatePrevRec,
  navigateNextRec,
  navigatePrevPage,
  navigateNextPage,
  getPageSize,
  selectRowByIndex,
  getVisibleRowsAndIndex,
  exitEditModeForNavigation,
} from './navigation/navigation.js';
import {
  handleRowSelected,
  selectDataRow,
  getSelectedDataRow,
  getEditingRow,
  isCalcRow,
  isCalcCell,
  rowsMatch,
} from './selection/row-selection.js';
import {
  discoverColumns,
  buildColumnsFromData,
  sortColumnsByPriority,
  mergeColumnsWithTableDef,
  applyEditModeGate,
  buildSelectorColumn,
  detectColtype,
  deepMergeParams,
} from './columns/column-management.js';
import { scrollbarManager } from '../core/scrollbar-manager.js';

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
    this.lookupKeyIdx = options.lookupKeyIdx || null;  // Direct Lookup 59 key_idx
    this.cssPrefix = options.cssPrefix || 'lithium';

    // Primary key field (explicit override, set before tableDef loading)
    if (options.primaryKeyField != null) {
      this.primaryKeyField = options.primaryKeyField;
      this._explicitPrimaryKeyField = true;
    } else {
      this.primaryKeyField = null;
      this._explicitPrimaryKeyField = false;
    }

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
    this._inSelectionTransition = false;

    // Editing state
    this.editingRowId = null;
    this.originalRowData = null;
    this.columnEditors = new Map();

    // Lithium-only column metadata (stored separately from Tabulator columns)
    // Map of field name -> { coltype, editable, sort, filter, calculated, primaryKey, description, ... }
    this._columnMeta = new Map();

    // UI state
    this.tableWidthMode = options.tableWidthMode || 'compact';
    this.tableLayoutMode = 'fitColumns';

    // Storage key for persistence
    this.storageKey = options.storageKey || `${this.cssPrefix}_table`;

    // Panel persistence (centralized width + collapsed state)
    this.panel = options.panel || null;
    this.panelStateManager = options.panelStateManager || null;
    this.splitter = null; // Set via setSplitter() after init

    // Data cache
    this.currentData = [];

    // Last data load parameters (for re-submit on refresh)
    this.lastLoadParams = null;

    // Metadata (primaryKeyField already set in constructor if explicitly provided)

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

    // Audit footer reference (shared AuditInfoFooter instance)
    this.auditFooter = options.auditFooter || null;

    // Column manager option


    // Always-editable mode: cells are directly editable without entering edit mode.
    // Used by the Column Manager which manages its own save/cancel workflow.
    this.alwaysEditable = options.alwaysEditable === true;

    // Local search mode: filter rows client-side instead of server request.
    // Use localSearchFields to specify which fields to search (defaults to all visible fields).
    this.localSearch = options.localSearch === true;
    this.localSearchFields = options.localSearchFields || null;
    this.useOverlayScrollbars = options.useOverlayScrollbars !== false;
    this._localSearchTerm = '';

    // Popup state
    this.activeNavPopup = null;
    this.activeNavPopupId = null;
    this.navPopupCloseHandler = null;
  }

  // ── Initialization ────────────────────────────────────────────────────────

  async init() {
    await this.loadConfiguration();
    this.injectSortIconStyles();

    // Skip initial data load - wait for explicit loadData() call
    // This is needed for tables that require runtime params (e.g., child tables)
    // The parent table will trigger loadData() after selection

    this.table = null;
    await this.initTable(null);

    this.buildNavigator();
    this.setupKeyboardHandling();
    await this.setupPersistence();
  }

  async _loadInitialData(extraParams = null) {
    return loadInitialData(this, extraParams);
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
    align-items: center;
    justify-content: center;
    width: 16px;
    height: 16px;
    line-height: 1;
    position: absolute;
    right: -4px;
    vertical-align: middle;
  }

  .${this.cssPrefix}-sort-icon {
    font-size: 12px;
    transition: color var(--transition-fast), opacity var(--transition-fast);
  }

  .${this.cssPrefix}-sort-icons[data-sort-dir="none"] .${this.cssPrefix}-sort-icon {
    color: var(--text-muted);
    opacity: 0.5;
  }

  .${this.cssPrefix}-sort-icons[data-sort-dir="asc"] .${this.cssPrefix}-sort-icon,
  .${this.cssPrefix}-sort-icons[data-sort-dir="desc"] .${this.cssPrefix}-sort-icon {
    color: white;
    opacity: 1;
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
      this.tableDef = await loadTableDef(this.tablePath, null, this.lookupKeyIdx);
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
      // Only set primaryKeyField from tableDef if not explicitly provided in constructor
      if (!this._explicitPrimaryKeyField) {
        this.primaryKeyField = getPrimaryKeyField(this.tableDef);
      }

      // Log table initialization details including primary key
      const tableName = this.tablePath || this.cssPrefix || 'auto';
      const isEditable = !this.readonly;
      const pkInfo = this.primaryKeyField ?
        (Array.isArray(this.primaryKeyField) ? this.primaryKeyField.join(', ') : this.primaryKeyField) :
        'none';
      const status = (isEditable && !this.primaryKeyField) ? Status.WARN : Status.INFO;
      const editMode = isEditable ? 'editable' : 'read-only';
      log(Subsystems.TABLE, status,
        `[${tableName}] ${editMode}, primary key: ${pkInfo}`);

      // Apply default template if available (last step before building table)
      this._applyDefaultTemplate();

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

  async initTable(preloadedData = null) {
    // Add base LithiumTable class for shared styling
    this.container.classList.add('lithium-table-container');

    // Group icon animator instance
    this._groupIconAnimator = new GroupIconAnimator(this);

    let dataColumns = [];
    const data = preloadedData || this.currentData;

    // Track if we're initializing without data (for data-first rebuild later)
    this._wasInitializedWithoutData = !data || data.length === 0;

    // First, auto-discover columns from data if available (data-first approach)
    if (data && data.length > 0) {
      dataColumns = this._buildColumnsFromData(data);
      log(Subsystems.TABLE, Status.DEBUG,
        `[${this.cssPrefix}] Auto-discovered ${dataColumns.length} columns from ${data.length} data rows`);
    }

    // Then apply tableDef overlays if available
    if (this.tableDef && this.coltypes) {
      const { columns: tableDefColumns, columnMeta } = resolveColumnsWithMeta(
        this.tableDef,
        this.coltypes,
        { filterEditor: this.createFilterEditorFunction() }
      );

      // Store the column metadata Map
      this._columnMeta = columnMeta;

      if (dataColumns.length > 0) {
        // Merge: apply tableDef properties to auto-discovered columns
        dataColumns = this._mergeColumnsWithTableDef(dataColumns, tableDefColumns);
        log(Subsystems.TABLE, Status.DEBUG,
          `[${this.cssPrefix}] Applied tableDef overlays to ${dataColumns.length} columns`);
      } else {
        // No data available yet, use tableDef columns
        dataColumns = tableDefColumns;
      }
    } else if (this.tableDef?.columns && Object.keys(this.tableDef.columns).length > 0) {
      const { columns: tableDefColumns, columnMeta } = resolveColumnsWithMeta(
        this.tableDef,
        this.coltypes || {},
        { filterEditor: this.createFilterEditorFunction() }
      );

      // Store the column metadata Map
      this._columnMeta = columnMeta;

      if (dataColumns.length > 0) {
        // Merge: apply tableDef properties to auto-discovered columns
        dataColumns = this._mergeColumnsWithTableDef(dataColumns, tableDefColumns);
      } else {
        dataColumns = tableDefColumns;
      }
    }

    if (dataColumns.length === 0) {
      if (this._wasInitializedWithoutData) {
        // For data-first tables, create with placeholder column until data loads
        log(Subsystems.TABLE, Status.INFO,
          `[${this.cssPrefix}] No data available yet, creating table with placeholder column`);
        dataColumns = [{
          field: '_placeholder',
          title: 'Loading...',
          visible: true,
          editable: false,
          resizable: false,
        }];
      } else {
        log(Subsystems.TABLE, Status.WARN,
          `[LithiumTable] No columns defined for table "${this.tablePath}" - table will be empty`);
        return;
      }
    }

    dataColumns = this._sortColumnsByPriority(dataColumns);

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

    // Note: We do NOT set dataLoaded callback for auto-discover here.
    // loadData() already calls discoverColumns() after setData(), so
    // a dataLoaded callback would cause double-discovery.

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
        // Use Font Awesome icons matching the grouping popup style
        // Tri-state: unsorted (fa-angles-up-down) -> asc (fa-angle-up) -> desc (fa-angle-down) -> unsorted
        if (dir === 'asc') {
          return `<span class="${prefix}-sort-icons" data-sort-dir="asc"><fa class="${prefix}-sort-icon" fa-angle-up></fa></span>`;
        } else if (dir === 'desc') {
          return `<span class="${prefix}-sort-icons" data-sort-dir="desc"><fa class="${prefix}-sort-icon" fa-angle-down></fa></span>`;
        } else {
          return `<span class="${prefix}-sort-icons" data-sort-dir="none"><fa class="${prefix}-sort-icon" fa-angles-up-down></fa></span>`;
        }
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

    this._initTableScrollbars();

    // If tableHolder not found yet, retry after a short delay
    // This can happen when initTable() is called before Tabulator finishes building
    if (!this._scrollbarInstance) {
      setTimeout(() => this._initTableScrollbars(), 100);
    }

    // Initialize FloatingUI tooltips on column headers
    // Use setTimeout to ensure Tabulator has rendered the header elements
    setTimeout(() => this.initColumnHeaderTooltips(), 100);

    // OverlayScrollbars is now enabled for all LithiumTable instances by default.
    // Set useOverlayScrollbars: false in constructor to disable for specific tables.
  }

  _initTableScrollbars() {
    if (!this.useOverlayScrollbars) {
      log(Subsystems.TABLE, Status.DEBUG, `[${this.cssPrefix}] _initTableScrollbars: disabled by option`);
      return;
    }
    if (this._scrollbarInstance) {
      log(Subsystems.TABLE, Status.DEBUG, `[${this.cssPrefix}] _initTableScrollbars: already present, skipping`);
      return;
    }
    if (!this.container) {
      log(Subsystems.TABLE, Status.DEBUG, `[${this.cssPrefix}] _initTableScrollbars: container missing, skipping`);
      return;
    }

    const tableHolder = this.container.querySelector('.tabulator-tableholder');
    if (!tableHolder) {
      log(Subsystems.TABLE, Status.WARN, `[${this.cssPrefix}] _initTableScrollbars: tableHolder not found`);
      return;
    }

    tableHolder.setAttribute('data-overlayscrollbars-initialize', '');
    this.container.classList.add('lithium-table-osb-enabled');

    log(Subsystems.TABLE, Status.DEBUG, `[${this.cssPrefix}] _initTableScrollbars: calling scrollbarManager.initTabulator`);

    this._scrollbarInstance = scrollbarManager.initTabulator(tableHolder, {
      initialized: () => {
        log(Subsystems.TABLE, Status.DEBUG, `[${this.cssPrefix}] _initTableScrollbars: OSB initialized callback received`);
        this._updateTableScrollbars();
      },
    });

    if (!this._scrollbarInstance) {
      log(Subsystems.TABLE, Status.WARN, `[${this.cssPrefix}] _initTableScrollbars: initTabulator returned null`);
      this.container.classList.remove('lithium-table-osb-enabled');
    } else {
      log(Subsystems.TABLE, Status.DEBUG, `[${this.cssPrefix}] _initTableScrollbars: OSB instance created`);
    }
   }

_updateTableScrollbars() {
    if (!this._scrollbarInstance) {
      return;
    }

    // Cancel any pending update - we only need one deferred update
    if (this._scrollbarUpdateTimer) {
      window.clearTimeout(this._scrollbarUpdateTimer);
      this._scrollbarUpdateTimer = 0;
    }

    // Debounce all updates into a single requestAnimationFrame
    // This prevents multiple expensive OSB updates from competing with
    // radar animation and other UI during rapid data loading
    if (!this._scrollbarUpdateRaf1) {
      this._scrollbarUpdateRaf1 = requestAnimationFrame(() => {
        this._scrollbarUpdateRaf1 = 0;
        if (this._scrollbarInstance && !this._scrollbarInstance.destroyed) {
          scrollbarManager.update(this._scrollbarInstance);
        }
      });
    }
  }

  // ── Group Icon Animation (delegated to GroupIconAnimator) ─────────────────

  updateGroupIcons() {
    this._groupIconAnimator?.updateGroupIcons();
  }

  wireTableEvents() {
    wireTableEvents(this);
  }



  // ── Row Selection (delegated to selection/row-selection.js) ───────────────

  async handleRowSelected(row) {
    return handleRowSelected(this, row);
  }

  selectDataRow(row) {
    return selectDataRow(this, row);
  }

  getSelectedDataRow() {
    return getSelectedDataRow(this);
  }

  getEditingRow() {
    return getEditingRow(this);
  }

  isCalcRow(row) {
    return isCalcRow(this, row);
  }

  isCalcCell(cell) {
    return isCalcCell(this, cell);
  }

  rowsMatch(rowA, rowB) {
    return rowsMatch(this, rowA, rowB);
  }

  getVisibleRowsAndIndex() {
    return getVisibleRowsAndIndex(this);
  }

  selectRowByIndex(index) {
    return selectRowByIndex(this, index);
  }

  getPageSize() {
    return getPageSize(this);
  }

  async _exitEditModeForNavigation() {
    return exitEditModeForNavigation(this);
  }

  async navigateFirst() {
    return navigateFirst(this);
  }

  async navigateLast() {
    return navigateLast(this);
  }

  async navigatePrevRec() {
    return navigatePrevRec(this);
  }

  async navigateNextRec() {
    return navigateNextRec(this);
  }

  async navigatePrevPage() {
    return navigatePrevPage(this);
  }

  async navigateNextPage() {
    return navigateNextPage(this);
  }

  // ── Data Loading (delegated to data/data-loading.js) ───────────────────────

  async loadData(searchTerm = '', extraParams = {}) {
    return loadData(this, searchTerm, extraParams);
  }

  getSelectedRowId() {
    return getSelectedRowId(this);
  }

  getSelectedRowData() {
    return getSelectedRowData(this);
  }

  autoSelectRow(targetId) {
    return autoSelectRow(this, targetId);
  }

  _getCompositeRowId(rowData, pkFields) {
    return getCompositeRowId(rowData, pkFields);
  }

  loadStaticData(rows, options = {}) {
    return loadStaticData(this, rows, options);
  }

  // ── Column Management (delegated to columns/column-management.js) ──────────

  buildSelectorColumn() {
    return buildSelectorColumn(this);
  }

  applyEditModeGate(columns) {
    return applyEditModeGate(this, columns);
  }

  discoverColumns(rows) {
    return discoverColumns(this, rows);
  }

  _detectColtype(rows, field) {
    return detectColtype(rows, field);
  }

  _buildColumnsFromData(data) {
    return buildColumnsFromData(this, data);
  }

  _sortColumnsByPriority(columns) {
    return sortColumnsByPriority(columns);
  }

  // ── Lifecycle ─────────────────────────────────────────────────────────────

  cleanup() {
    this.closeTransientPopups();
  }

  destroy() {
    this.cleanup();

    this.container?.classList?.remove('lithium-table-osb-enabled');

    // Destroy OverlayScrollbars instance
    if (this._scrollbarInstance) {
      scrollbarManager.destroy(this._scrollbarInstance);
      this._scrollbarInstance = null;
    }

    if (this._scrollbarUpdateTimer) {
      window.clearTimeout(this._scrollbarUpdateTimer);
      this._scrollbarUpdateTimer = 0;
    }

    if (this._scrollbarUpdateRaf1) {
      window.cancelAnimationFrame(this._scrollbarUpdateRaf1);
      this._scrollbarUpdateRaf1 = 0;
    }

    if (this._scrollbarUpdateRaf2) {
      window.cancelAnimationFrame(this._scrollbarUpdateRaf2);
      this._scrollbarUpdateRaf2 = 0;
    }

    if (this._scrollbarInstance) {
      scrollbarManager.destroy(this._scrollbarInstance);
      this._scrollbarInstance = null;
    }

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

  // ── Template Capture (delegated to template/capture.js) ────────────────────

  captureCurrentState() {
    return captureCurrentState(this);
  }

  _createTemplateColumnFromTableDef(fieldName, colDef) {
    return createTemplateColumnFromTableDef(fieldName, colDef);
  }

  _extractTemplateColumnFromColumn(column) {
    return extractTemplateColumnFromColumn(column);
  }

  // ── Refresh Orchestration (delegated to refresh-orchestrator.js) ────────────

  async reloadConfiguration() {
    return reloadConfiguration(this);
  }

  _getCurrentlySelectedRowId() {
    return getCurrentlySelectedRowId(this);
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

  // ── Panel Width Persistence (delegated to persistence/panel-width.js) ──────

  getWidthForMode(mode) {
    return getWidthForMode(mode);
  }

  applyPanelWidth(mode) {
    return applyPanelWidth(this, mode);
  }

  savePanelPixelWidth(width) {
    return savePanelPixelWidth(this, width);
  }

  loadCollapsedState(defaultValue = false) {
    return loadCollapsedState(this, defaultValue);
  }

  saveCollapsedState(collapsed) {
    return saveCollapsedState(this, collapsed);
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

  // ── Column Header Tooltips (delegated to visual/column-tooltips.js) ─────────

  initColumnHeaderTooltips() {
    return initColumnHeaderTooltips(this);
  }

  /**
   * Rebuild table columns using data-first approach after data becomes available.
   * This is called when the table was initialized without data but now has data.
   * @param {Array} data - The loaded data rows
   */
  async _rebuildColumnsWithData(data) {
    if (!this.table || !data || data.length === 0) return;

    log(Subsystems.TABLE, Status.INFO, `[${this.cssPrefix}] Rebuilding columns with data-first approach`);

    // Auto-discover columns from data
    let dataColumns = this._buildColumnsFromData(data);

    // Apply tableDef overlays if available
    if (this.tableDef && this.coltypes) {
      const { columns: tableDefColumns, columnMeta } = resolveColumnsWithMeta(
        this.tableDef,
        this.coltypes,
        { filterEditor: this.createFilterEditorFunction() }
      );
      // Update the column metadata Map
      this._columnMeta = columnMeta;
      dataColumns = this._mergeColumnsWithTableDef(dataColumns, tableDefColumns);
    }

    if (dataColumns.length === 0) return;

    dataColumns = this._sortColumnsByPriority(dataColumns);
    this.applyEditModeGate(dataColumns);

    // Build new column definitions for Tabulator
    const selectorColumn = this.buildSelectorColumn();
    const columns = [selectorColumn, ...dataColumns];
    columns.forEach(col => {
      if (col.title && !col.cssClass?.includes('first-visible-col')) {
        col.cssClass = [col.cssClass, sanitizeColumnTitle(col.title)].filter(Boolean).join(' ');
      }
    });

    // Replace columns on the existing table
    this.table.setColumns(columns);

    // Clear the flag
    this._wasInitializedWithoutData = false;

    log(Subsystems.TABLE, Status.INFO, `[${this.cssPrefix}] Rebuilt with ${dataColumns.length} columns`);
  }

  _mergeColumnsWithTableDef(autoColumns, tableDefColumns) {
    return mergeColumnsWithTableDef(this, autoColumns, tableDefColumns);
  }

  /**
   * Apply default template to tableDef if available.
   * This is the last step before building the table, applied after loading tableDef from Lookup 59.
   *
   * Uses deep merge for param objects (formatterParams, editorParams, etc.) so that
   * coltype defaults are preserved even when templates specify partial overrides.
   */
  _applyDefaultTemplate() {
    if (!this.tableDef || !this.storageKey) return;

    const defaultTemplateKey = `lithium_table_template_${this.storageKey}_Default`;
    try {
      const defaultTemplateJson = localStorage.getItem(defaultTemplateKey);
      if (defaultTemplateJson) {
        const defaultTemplate = JSON.parse(defaultTemplateJson);
        log(Subsystems.TABLE, Status.INFO,
          `[${this.cssPrefix}] Applying default template from localStorage`);

        // Merge default template with tableDef
        // Use deep merge for param objects to preserve coltype defaults
        const mergedColumns = { ...this.tableDef.columns };
        if (defaultTemplate.columns) {
          for (const [field, col] of Object.entries(defaultTemplate.columns)) {
            if (mergedColumns[field]) {
              mergedColumns[field] = deepMergeParams(mergedColumns[field], col);
            } else {
              mergedColumns[field] = col;
            }
          }
        }

        // Apply other template properties, excluding columns which are already merged
        const { columns: _col, ...templateProps } = defaultTemplate;
        void _col; // unused but explicitly excluded
        this.tableDef = { ...this.tableDef, ...templateProps, columns: mergedColumns };
      }
    } catch (e) {
      log(Subsystems.TABLE, Status.WARN,
        `[${this.cssPrefix}] Failed to apply default template: ${e.message}`);
    }
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

  // ── Abstract Methods (to be implemented by subclasses) ─────────────────────

  buildNavigator() { /* To be implemented */ }
  createFilterEditor() { return null; }
  updateMoveButtonState() { /* To be implemented */ }
  updateDuplicateButtonState() { /* To be implemented */ }
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
