/**
 * Queries Manager - Edit Mode Module
 *
 * Handles edit mode transitions, cell editing, and edit gating.
 */

import { log, Subsystems, Status } from '../../core/log.js';

/**
 * EditModeManager - Manages edit mode state and transitions
 */
export class EditModeManager {
  constructor(manager) {
    this.manager = manager;
    this._isEditing = false;
    this._editingRowId = null;
    this._editTransitionPending = false;
    this._pendingCellEditToken = 0;
    this._columnEditors = new Map();
  }

  /**
   * Check if currently in edit mode
   */
  isEditing() {
    return this._isEditing;
  }

  /**
   * Get the ID of the row being edited
   */
  getEditingRowId() {
    return this._editingRowId;
  }

  /**
   * Check if edit transition is pending
   */
  isTransitionPending() {
    return this._editTransitionPending;
  }

  /**
   * Enter edit mode for a specific row
   */
  async enterEditMode(row, targetCell = null) {
    const table = this.manager.queryTable?.table;
    if (!table) return;

    this._setEditTransitionState(true);

    try {
      if (!row) {
        const selected = table.getSelectedRows();
        if (selected.length === 0) return;
        row = selected[0];
      }

      const pkField = 'query_id';
      const rowId = row.getData()?.[pkField];

      if (this._isEditing && this._editingRowId === rowId) return;

      if (this._isEditing) await this.exitEditMode();

      this._editingRowId = rowId;
      this._isEditing = true;

      // Capture original data for dirty tracking
      // Pass editors (if they exist) and pending content (for lazy-loaded editors)
      // so we capture full database content, not truncated table row data
      this.manager.dirtyTracker.captureOriginalData(row.getData(), {
        sql: this.manager.sqlEditor,
        summary: this.manager.summaryEditor,
        collection: this.manager.collectionEditor,
        // Pending content for lazy-loaded editors (full DB content)
        sqlContent: this.manager._pendingSqlContent,
        summaryContent: this.manager._pendingSummaryContent,
        collectionContent: this.manager._pendingCollectionContent,
      });

      // Update UI
      this.manager.elements.tableContainer?.classList.add('queries-edit-mode');
      this._updateEditingIndicator();
      this.manager._updateSaveCancelButtonState();
      this.manager._setCodeMirrorEditable(true);

      // Focus target cell or default to name cell
      if (targetCell) {
        this._queueCellEdit(targetCell);
      } else {
        const nameCell = row.getCell('name');
        if (nameCell) this._queueCellEdit(nameCell);
      }

      log(Subsystems.MANAGER, Status.INFO, `[QueriesManager] Entered edit mode (row ${rowId})`);
    } finally {
      requestAnimationFrame(() => this._setEditTransitionState(false));
    }
  }

  /**
   * Exit edit mode
   */
  async exitEditMode(reason = 'cancel') {
    if (!this._isEditing) return;

    this._setEditTransitionState(true);

    try {
      const previousId = this._editingRowId;

      this._cancelActiveCellEdit();

      this._editingRowId = null;
      this._isEditing = false;

      this.manager.elements.tableContainer?.classList.remove('queries-edit-mode');
      this._updateEditingIndicator();
      this.manager._updateSaveCancelButtonState();
      this.manager._setCodeMirrorEditable(false);

      if (reason !== 'save') {
        this.manager.dirtyTracker.markAllClean();
        this.manager.dirtyTracker.clearOriginalData();
      }

      log(Subsystems.MANAGER, Status.INFO, `[QueriesManager] Exited edit mode (row ${previousId}, reason: ${reason})`);
    } finally {
      requestAnimationFrame(() => this._setEditTransitionState(false));
    }
  }

  /**
   * Toggle edit mode for the currently selected row
   */
  async toggleEditMode() {
    const table = this.manager.queryTable?.table;
    if (!table) return;
    const selected = table.getSelectedRows();
    if (selected.length === 0) return;

    const pkField = 'query_id';
    const selectedId = selected[0].getData()?.[pkField];

    if (this._isEditing && this._editingRowId === selectedId) {
      if (this.manager.dirtyTracker.isAnyDirty()) {
        await this.manager.handleNavSave();
      } else {
        await this.exitEditMode('toggle');
      }
    } else {
      await this.enterEditMode(selected[0]);
    }
  }

  /**
   * Set edit transition state
   */
  _setEditTransitionState(pending) {
    this._editTransitionPending = pending;
    this.manager.elements.tableContainer?.classList.toggle('queries-edit-transition', pending);
    this.manager.elements.navigatorContainer?.classList.toggle('queries-edit-transition', pending);
  }

  /**
   * Update the editing indicator in the selector column
   */
  _updateEditingIndicator() {
    const table = this.manager.queryTable?.table;
    if (!table) return;
    const selected = table.getSelectedRows();
    if (selected.length > 0) {
      const row = selected[0];
      const cell = row.getCell('_selector');
      if (cell) {
        const el = cell.getElement();
        const pkField = 'query_id';
        const isEditing = this._isEditing && this._editingRowId === row.getData()[pkField];
        el.innerHTML = isEditing
          ? '<fa fa-i-cursor fa-swap-opacity queries-selector-indicator queries-selector-edit>'
          : '<fa fa-caret-right fa-swap-opacity queries-selector-indicator queries-selector-active>';
      }
    }
  }

  /**
   * Apply edit mode gate to columns
   */
  applyEditModeGate(columns) {
    this._columnEditors = new Map();

    for (const col of columns) {
      if (!col.editor) continue;

      this._columnEditors.set(col.field, {
        editor: col.editor,
        editorParams: col.editorParams || undefined,
      });

      col.editable = (cell) => {
        if (!this._isEditing) return false;
        const pkField = 'query_id';
        const rowId = cell.getRow().getData()?.[pkField];
        return this._editingRowId != null ? rowId === this._editingRowId : cell.getRow().isSelected();
      };
    }
  }

  /**
   * Cancel active cell edit
   */
  _cancelActiveCellEdit() {
    this._commitActiveCellEdit();
  }

  /**
   * Commit active cell edit
   */
  _commitActiveCellEdit(nextCell = null) {
    const active = document.activeElement;
    if (!active || !this.manager.elements.tableContainer?.contains(active)) return;

    const activeCellEl = active.closest?.('.tabulator-cell');
    const nextCellEl = nextCell?.getElement?.() || null;
    if (activeCellEl && nextCellEl && activeCellEl === nextCellEl) return;

    active.blur();
  }

  /**
   * Check if row is a calculation row
   */
  _isCalcRow(row) {
    if (!row) return false;
    const rowEl = row.getElement?.();
    return !!rowEl?.closest?.('.tabulator-calcs, .tabulator-calcs-holder, .tabulator-calcs-bottom');
  }

  /**
   * Check if cell is in a calculation row
   */
  _isCalcCell(cell) {
    if (!cell) return false;
    const cellEl = cell.getElement?.();
    if (cellEl?.closest?.('.tabulator-calcs, .tabulator-calcs-holder, .tabulator-calcs-bottom')) return true;
    return this._isCalcRow(cell.getRow?.());
  }

  /**
   * Queue a cell edit for the next animation frame
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

        const pkField = 'query_id';
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
   * Get the row currently being edited
   */
  getEditingRow() {
    const table = this.manager.queryTable?.table;
    if (!table || !this._isEditing || this._editingRowId == null) return null;
    const pkField = 'query_id';
    const rows = table.getRows('active');
    return rows.find(row => row.getData()?.[pkField] === this._editingRowId) || null;
  }

  /**
   * Handle cell edited event
   */
  handleCellEdited() {
    this.manager.dirtyTracker.setDirty('table', true);
  }

  /**
   * Attach input listener to track dirty state
   */
  attachInputListener(cell) {
    requestAnimationFrame(() => {
      const cellEl = cell.getElement?.();
      if (!cellEl) return;

      const inputEl = cellEl.querySelector?.('input, textarea');
      if (!inputEl) return;

      const handleInput = () => this.manager.dirtyTracker.setDirty('table', true);

      inputEl.addEventListener('input', handleInput, { once: true });
      inputEl.addEventListener('keydown', handleInput, { once: true });
    });
  }
}

/**
 * Create an edit mode manager for the manager
 */
export function createEditModeManager(manager) {
  return new EditModeManager(manager);
}
