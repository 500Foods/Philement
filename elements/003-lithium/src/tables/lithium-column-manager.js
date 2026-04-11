/**
 * LithiumColumnManager
 *
 * A column management interface implemented as a LithiumTable.
 * Provides drag-to-reorder, visibility toggles, and column property editing
 * with real-time updates to the parent table.
 *
 * Features:
 * - Anchored to the column chooser icon position
 * - Dark blue themed background for visual distinction
 * - Draggable resize handle (bottom-right corner)
 * - Persistence of selected row, width, height
 * - Independent instance per parent table
 *
 * @module tables/column-manager
 */

import { log, Subsystems, Status } from '../core/log.js';

// Import modular components
import {
  updateButtonState,
  syncDirtyState,
  clearDirty,
  saveSetting,
  restoreSetting,
  handlePrimarySave,
  handlePrimaryCancel,
  getCurrentColumnTableData,
} from './column-manager/cm-state.js';

import {
  handleDragStart,
  handleDragMove,
  handleDragEnd,
  handleResizeStart,
  handleResizeMove,
  handleResizeEnd,
  handleKeyDown,
  handleClickOutside,
} from './column-manager/cm-drag-resize.js';

import {
  loadColumnData,
  refreshColumnData,
} from './column-manager/cm-data.js';

import {
  applyAllChangesToParent,
  discardAllChanges,
  buildPendingTemplateColumns,
  handleCellEdit,
  handleRowReorder,
} from './column-manager/cm-actions.js';

import {
  isDepth2ColumnManager,
  createPopup,
  positionPopup,
  show,
  hide,
  close,
  setTableWidth,
} from './column-manager/cm-ui.js';

import {
  initColumnTable,
  wireRowReorderEvents,
  setOrderingMode,
  configureNavigatorButtons,
  getColtypesForManager,
} from './column-manager/cm-table.js';

import { buildColumnManagerDefinition } from './column-manager/cm-columns.js';

export class LithiumColumnManager {
  /**
   * Create a ColumnManager instance
   * @param {Object} options - Configuration options
   */
  constructor(options) {
    this.parentContainer = options.parentContainer;
    this.anchorElement = options.anchorElement;
    this.parentTable = options.parentTable;
    this.app = options.app;
    this.managerId = options.managerId || 'default';
    this.cssPrefix = options.cssPrefix || 'col-manager';

    // State
    this.columnTable = null;
    this.columnData = [];
    this.originalColumns = [];
    this.isInitialized = false;
    this.popup = null;

    // Column ordering mode
    this.orderingMode = 'manual';

    // Resize state
    this.isResizing = false;
    this.resizeStartX = 0;
    this.resizeStartY = 0;
    this.popupStartWidth = 0;
    this.popupStartHeight = 0;
    this.popupStartLeft = 0;
    this.popupStartTop = 0;
    this.resizeCorner = 'br';

    // Drag state
    this.isDragging = false;
    this.dragStartX = 0;
    this.dragStartY = 0;
    this.popupStartX = 0;
    this.popupStartY = 0;

    // Storage key
    this.storageKey = `lithium_col_manager_${this.managerId}`;

    // Event handlers
    this.onColumnChange = options.onColumnChange || (() => {});
    this.onClose = options.onClose || (() => {});

    // Button references
    this.saveBtn = null;
    this.cancelBtn = null;

    // Dirty state
    this.isDirty = false;

    // Bind methods for event handlers
    this.handleResizeMove = (e) => handleResizeMove(this, e);
    this.handleResizeEnd = () => handleResizeEnd(this);
    this.handleDragMove = (e) => handleDragMove(this, e);
    this.handleDragEnd = () => handleDragEnd(this);
    this.handleKeyDown = (e) => handleKeyDown(this, e);
    this.handleClickOutside = (e) => handleClickOutside(this, e);
  }

  /**
   * Initialize the column manager (lazy - called when first opened)
   */
  async init() {
    if (this.isInitialized) {
      await refreshColumnData(this);
      return;
    }

    if (!this.parentContainer || !this.parentTable) {
      log(Subsystems.TABLE, Status.ERROR, '[ColumnManager] Missing required options');
      return;
    }

    if (!this.parentTable.table) {
      log(Subsystems.TABLE, Status.WARN, '[ColumnManager] Waiting for parent table to initialize...');
      await new Promise((resolve) => setTimeout(resolve, 100));
    }

    // Restore ordering mode preference
    const savedMode = restoreSetting(this, 'orderingMode', 'manual');
    if (savedMode === 'auto' || savedMode === 'manual') {
      this.orderingMode = savedMode;
    }

    // Create popup
    createPopup(this);

    await new Promise((resolve) => {
      requestAnimationFrame(() => {
        this.popup.classList.add('visible');
        setTimeout(resolve, 50);
      });
    });

    await loadColumnData(this);
    await initColumnTable(this);

    // Wire event handlers for buttons and drag/resize
    this.wireEventHandlers();

    this.isInitialized = true;
    log(Subsystems.TABLE, Status.INFO, `[ColumnManager] Initialized for ${this.managerId}`);
  }

  /**
   * Refresh column data from parent table
   */
  async refreshColumnData() {
    await refreshColumnData(this);
  }

  /**
   * Handle parent table column order change
   */
  onParentColumnOrderChanged() {
    if (!this.isInitialized || !this.columnTable?.table) return;
    this.refreshColumnData();
    log(Subsystems.TABLE, Status.DEBUG, '[ColumnManager] Parent column order changed, refreshed');
  }

  /**
   * Check if this is a depth-2 column manager
   */
  isDepth2ColumnManager() {
    return isDepth2ColumnManager(this);
  }

  /**
   * Wire event handlers
   */
  wireEventHandlers() {
    // Close button - always applies changes and closes
    const closeBtn = this.popup.querySelector(`.${this.cssPrefix}-close-btn`);
    closeBtn?.addEventListener('click', () => {
      void this.handleClose();
    });

    // Save button - saves current row edit only
    this.saveBtn = this.popup.querySelector(`.${this.cssPrefix}-save-btn`);
    this.saveBtn?.addEventListener('click', async () => {
      if (this.columnTable?.isEditing) {
        await this.columnTable.handleSave();
        this.syncDirtyState();
      }
    });

    // Cancel button - cancels row edit only
    this.cancelBtn = this.popup.querySelector(`.${this.cssPrefix}-cancel-btn`);
    this.cancelBtn?.addEventListener('click', async () => {
      if (this.columnTable?.isEditing) {
        await this.columnTable.handleCancel();
        this.syncDirtyState();
      }
    });

    // Update button state (enabled when row is being edited)
    updateButtonState(this);

    // Mode toggle buttons
    const modeBtns = this.popup.querySelectorAll(`.${this.cssPrefix}-mode-btn`);
    modeBtns.forEach((btn) => {
      btn.addEventListener('click', () => {
        const mode = btn.dataset.mode;
        if (mode !== this.orderingMode) {
          setOrderingMode(this, mode);
        }
      });
    });

    // Resize handles (four corners)
    const resizeHandleBR = this.popup.querySelector(`.${this.cssPrefix}-resize-handle-br`);
    const resizeHandleBL = this.popup.querySelector(`.${this.cssPrefix}-resize-handle-bl`);
    const resizeHandleTR = this.popup.querySelector(`.${this.cssPrefix}-resize-handle-tr`);
    const resizeHandleTL = this.popup.querySelector(`.${this.cssPrefix}-resize-handle-tl`);
    resizeHandleBR?.addEventListener('mousedown', (e) => handleResizeStart(this, e));
    resizeHandleBL?.addEventListener('mousedown', (e) => handleResizeStart(this, e));
    resizeHandleTR?.addEventListener('mousedown', (e) => handleResizeStart(this, e));
    resizeHandleTL?.addEventListener('mousedown', (e) => handleResizeStart(this, e));

    // Header for dragging
    const header = this.popup.querySelector(`.${this.cssPrefix}-header`);
    header?.addEventListener('mousedown', (e) => this.handleDragStart(e));

    // Keyboard handler
    document.addEventListener('keydown', this.handleKeyDown);

    // Click outside
    setTimeout(() => {
      document.addEventListener('click', this.handleClickOutside);
    }, 0);
  }

  // Delegate state methods
  updateButtonState() { updateButtonState(this); }
  syncDirtyState() { syncDirtyState(this); }
  clearDirty() { clearDirty(this); }
  saveSetting(key, value) { saveSetting(this, key, value); }
  restoreSetting(key, defaultValue) { return restoreSetting(this, key, defaultValue); }
  handlePrimarySave() { return handlePrimarySave(this); }
  handlePrimaryCancel() { return handlePrimaryCancel(this); }

  // Delegate drag/resize methods
  handleDragStart(e) { handleDragStart(this, e); }

  // Delegate data methods
  loadColumnData() { return loadColumnData(this); }

  // Delegate action methods
  async applyAllChangesToParent() { return applyAllChangesToParent(this); }
  async discardAllChanges() { return discardAllChanges(this); }
  buildPendingTemplateColumns() { return buildPendingTemplateColumns(this); }
  getCurrentColumnTableData() {
    return getCurrentColumnTableData(this);
  }
  handleCellEdit(cell) { return handleCellEdit(this, cell); }
  handleRowReorder() { return handleRowReorder(this); }

  // Delegate UI methods
  createPopup() { createPopup(this); }
  positionPopup() { positionPopup(this); }
  show() { show(this); }
  hide() { hide(this); }
  close() { close(this); }
  setTableWidth(mode) { setTableWidth(this, mode); }

  // Handle close (always applies changes and closes)
  async handleClose() {
    await this.applyAllChangesToParent();
    this.close();
  }

  // Handle save (saves row edit if any, applies changes to parent)
  async handleSave() {
    // Save any current row edit
    if (this.columnTable?.isEditing) {
      await this.columnTable.handleSave();
    }
    // Apply changes to parent and close
    await this.applyAllChangesToParent();
    this.close();
  }

  // Handle cancel (cancels row edit if any, discards changes and closes)
  async handleCancel() {
    // Cancel any current row edit
    if (this.columnTable?.isEditing) {
      await this.columnTable.handleCancel();
    }
    // Discard changes and close
    await this.discardAllChanges();
    this.close();
  }

  // Delegate table methods
  async initColumnTable() { return initColumnTable(this); }
  wireRowReorderEvents() { wireRowReorderEvents(this); }
  configureNavigatorButtons() {
    configureNavigatorButtons(this);
  }
  async handleColumnTableExecuteSave() {
    this.syncDirtyState();
  }
  setOrderingMode(mode) { return setOrderingMode(this, mode); }
  getManagerTableStorageKey() {
    return this.isDepth2ColumnManager()
      ? 'lithium_column_manager_manager'
      : 'lithium_column_manager';
  }
  getManagerTablePath() {
    return this.isDepth2ColumnManager()
      ? 'column-manager/column-manager-manager'
      : 'column-manager/column-manager';
  }
  async getColtypesForManager() {
    return getColtypesForManager(this);
  }
  buildColumnManagerDefinition() {
    return buildColumnManagerDefinition(this);
  }
  onRowSelected(rowData) {
    this.saveSetting('selectedRow', rowData.field_name);
    log(Subsystems.TABLE, Status.DEBUG, `[ColumnManager] Row selected: ${rowData.field_name}`);
  }
  onRowDeselected() {
    this.saveSetting('selectedRow', null);
  }
  restoreSelectedRow() {
    const savedField = this.restoreSetting('selectedRow', null);
    if (savedField && this.columnTable?.table) {
      const rows = this.columnTable.table.getRows();
      const targetRow = rows.find((row) => row.getData().field_name === savedField);
      if (targetRow) {
        targetRow.select();
      }
    }
  }

  /**
   * Refresh column data from parent table (alias)
   */
  async refresh() {
    await loadColumnData(this);
    if (this.columnTable?.table) {
      await this.columnTable.table.replaceData(this.columnData);
    }
  }

  /**
   * Get available format options (for backward compatibility)
   */
  getFormatOptions() {
    return [
      { value: 'string', label: 'Text' },
      { value: 'integer', label: 'Integer' },
      { value: 'decimal', label: 'Decimal' },
      { value: 'index', label: 'Index' },
      { value: 'date', label: 'Date' },
      { value: 'datetime', label: 'DateTime' },
      { value: 'boolean', label: 'Boolean' },
      { value: 'lookup', label: 'Lookup' },
    ];
  }

  /**
   * Get available summary options (for backward compatibility)
   */
  getSummaryOptions() {
    return [
      { value: 'none', label: 'None' },
      { value: 'sum', label: 'Sum' },
      { value: 'count', label: 'Count' },
      { value: 'avg', label: 'Average' },
      { value: 'min', label: 'Minimum' },
      { value: 'max', label: 'Maximum' },
    ];
  }

  /**
   * Get available alignment options (for backward compatibility)
   */
  getAlignmentOptions() {
    return [
      { value: 'left', label: 'Left' },
      { value: 'center', label: 'Center' },
      { value: 'right', label: 'Right' },
    ];
  }

  /**
   * Get column group for a field (for backward compatibility)
   */
  getColumnGroup(field) {
    if (field.includes('id') || field.includes('ID')) return 'Identifiers';
    if (field.includes('name') || field.includes('Name')) return 'Names';
    if (field.includes('date') || field.includes('time')) return 'Dates';
    if (field.includes('status') || field.includes('state')) return 'Status';
    return 'General';
  }

  /**
   * Cleanup method
   */
  cleanup() {
    this.close();
    if (this.columnTable) {
      this.columnTable.destroy();
      this.columnTable = null;
    }
    if (this.popup) {
      this.popup.remove();
      this.popup = null;
    }
    this.isInitialized = false;
  }
}

export default LithiumColumnManager;
