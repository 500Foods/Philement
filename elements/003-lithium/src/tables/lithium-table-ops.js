/**
 * LithiumTable Operations Module
 *
 * CRUD operations and edit mode handling:
 * - Add, duplicate, edit, save, cancel, delete operations
 * - Edit mode state management
 * - Dirty state tracking and auto-save
 * - Keyboard handling
 *
 * @module core/lithium-table-ops
 */

/* globals MouseEvent, KeyboardEvent */

import { authQuery } from '../shared/conduit.js';
import { toast } from '../shared/toast.js';
import { log, Subsystems, Status } from '../core/log.js';

// ── Mixin for CRUD Operations ───────────────────────────────────────────────

export const LithiumTableOpsMixin = {
  // ── CRUD Operations ───────────────────────────────────────────────────────

  async handleAdd() {
    if (!this.table || this.readonly || this.alwaysEditable) return;

    const newRow = await this.table.addRow({}, true);

    if (newRow) {
      this.table.deselectRow();
      newRow.select();
      newRow.scrollTo();
      await this.enterEditMode(newRow);

      // Focus first editable cell
      const cells = newRow.getCells();
      const firstEditable = cells.find(cell => {
        const field = cell.getField();
        return field !== '_selector' && this.columnEditors.has(field);
      });
      if (firstEditable) {
        this.queueCellEdit(firstEditable);
      }
    }
  },

  async handleDuplicate() {
    if (!this.table || this.readonly || this.alwaysEditable) return;

    const selected = this.table.getSelectedRows();
    if (selected.length === 0) {
      toast.info('No Row Selected', { description: 'Please select a row to duplicate', duration: 3000 });
      return;
    }

    const originalData = selected[0].getData();
    let duplicateData;

    // If manager provided custom duplicate logic, use it
    if (typeof this.onDuplicate === 'function') {
      try {
        duplicateData = await this.onDuplicate(originalData);
        if (!duplicateData) {
          // Custom handler aborted or handled the operation itself
          return;
        }
      } catch (error) {
        toast.error('Duplicate Failed', {
          serverError: error.serverError,
          subsystem: 'Conduit',
          duration: 6000,
        });
        return;
      }
    } else {
      // Default duplicate behavior
      duplicateData = { ...originalData };

      // Remove primary key for new record
      const pkField = this.primaryKeyField || 'id';
      delete duplicateData[pkField];

      // Append " (Copy)" to name if it exists
      if (duplicateData.name) {
        duplicateData.name = `${duplicateData.name} (Copy)`;
      }
    }

    const newRow = await this.table.addRow(duplicateData, true);

    if (newRow) {
      this.table.deselectRow();
      newRow.select();
      newRow.scrollTo();
      await this.enterEditMode(newRow);
    }
  },

  async handleEdit() {
    if (!this.table || this.readonly || this.alwaysEditable) return;

    const selected = this.table.getSelectedRows();
    if (selected.length === 0) {
      toast.info('No Row Selected', { description: 'Please select a row to edit', duration: 3000 });
      return;
    }

    const pkField = this.primaryKeyField || 'id';
    const selectedId = selected[0].getData()?.[pkField];

    // Toggle edit mode if already editing this row
    if (this.isEditing && this.editingRowId === selectedId) {
      // Synchronous dirty check — the rAF-deferred isDirty may be stale
      const actuallyDirty = this.isActuallyDirty();
      if (actuallyDirty) {
        await this.handleSave();
      } else {
        await this.exitEditMode('toggle');
      }
      return;
    }

    await this.enterEditMode(selected[0]);
  },

  async handleSave() {
    if (!this.table || this.readonly) return;

    if (!this.isDirty) {
      toast.info('No Changes', { description: 'Nothing to save', duration: 3000 });
      return;
    }

    const selected = this.table.getSelectedRows();
    if (selected.length === 0) {
      toast.info('No Row Selected', { description: 'Please select a row to save', duration: 3000 });
      return;
    }

    try {
      await this.executeSave(selected[0]);
      this.isDirty = false;
      this.originalRowData = null;
      this.updateSaveCancelButtonState();
      this.notifyDirtyChange();
      await this.exitEditMode('save');

      toast.success('Changes Saved', { duration: 3000 });
    } catch (error) {
      toast.error('Save Failed', {
        serverError: error.serverError,
        subsystem: 'Conduit',
        duration: 8000,
      });
    }
  },

  async executeSave(row) {
    // If the manager provided a custom save callback, use it exclusively.
    // This is the standard pattern — managers assemble their own API params
    // from table row data + registered editor content.
    if (typeof this.onExecuteSave === 'function') {
      await this.onExecuteSave(row, this._editHelper);
      return;
    }

    // Default save: use insert/update queryRefs for simple table-only saves
    const rowData = row.getData();
    const pkField = this.primaryKeyField || 'id';
    const pkValue = rowData[pkField];
    const isInsert = pkValue == null || pkValue === '' || pkValue === 0;

    if (isInsert && this.queryRefs.insertQueryRef) {
      const payload = { ...rowData };
      delete payload[pkField];

      const result = await authQuery(this.app.api, this.queryRefs.insertQueryRef, { JSON: payload });

      // Update the row with the new ID from the server
      if (result?.[0]?.[pkField] != null) {
        row.update({ [pkField]: result[0][pkField] });
      }
    } else if (!isInsert && this.queryRefs.updateQueryRef) {
      await authQuery(this.app.api, this.queryRefs.updateQueryRef, {
        INTEGER: { [pkField.toUpperCase()]: pkValue },
        JSON: rowData,
      });
    } else {
      // No appropriate queryRef configured — save not possible
      const operation = isInsert ? 'insertQueryRef' : 'updateQueryRef';
      log(Subsystems.TABLE, Status.WARN,
        `[LithiumTable] No ${operation} configured — save skipped`);
      throw Object.assign(new Error(`No ${operation} configured for this table`), {
        serverError: {
          error: 'Save Not Configured',
          message: `No ${operation} has been configured for this table. Please check the table definition.`,
          query_ref: isInsert ? this.queryRefs.insertQueryRef : this.queryRefs.updateQueryRef,
        },
      });
    }
  },

  async handleCancel() {
    if (!this.isDirty) {
      await this.exitEditMode('cancel');
      return;
    }

    // Revert table row changes
    if (this.originalRowData && this.table) {
      const selected = this.table.getSelectedRows();
      if (selected.length > 0) {
        selected[0].update(this.originalRowData);
      }
    }

    // Revert external editor changes (CodeMirror, SunEditor, etc.)
    // via the ManagerEditHelper that holds the snapshot
    if (this._editHelper?.restoreEditorSnapshots) {
      this._editHelper.restoreEditorSnapshots();
    }

    this.isDirty = false;
    this.originalRowData = null;
    this.notifyDirtyChange();
    await this.exitEditMode('cancel');

    toast.info('Changes Cancelled', { description: 'All changes have been reverted', duration: 3000 });
  },

  async handleDelete() {
    if (!this.table || this.readonly || this.alwaysEditable) return;

    const selected = this.table.getSelectedRows();
    if (selected.length === 0) {
      toast.info('No Row Selected', { description: 'Please select a row to delete', duration: 3000 });
      return;
    }

    const pkField = this.primaryKeyField || 'id';
    const selectedId = selected[0].getData()?.[pkField];
    const rowName = selected[0].getData()?.name || '';

    // Confirm deletion
    const confirmed = window.confirm(
      rowName
        ? `Delete "${rowName}" (${pkField}: ${selectedId})?\n\nThis action cannot be undone.`
        : `Are you sure you want to delete this row?\n\nThis action cannot be undone.`
    );

    if (!confirmed) return;

    // Execute delete on server
    if (this.queryRefs.deleteQueryRef && selectedId != null) {
      try {
        await authQuery(this.app.api, this.queryRefs.deleteQueryRef, {
          INTEGER: { [pkField.toUpperCase()]: selectedId },
        });
      } catch (error) {
        toast.error('Delete Failed', {
          serverError: error.serverError,
          subsystem: 'Conduit',
          duration: 8000,
        });
        return;
      }
    }

    // Find next row to select after deletion
    const allRows = this.table.getRows('active');
    const currentIndex = allRows.findIndex(row => row === selected[0]);
    const nextRow = allRows[currentIndex + 1] || allRows[currentIndex - 1] || null;

    // Delete the row
    selected.forEach(row => row.delete());

    // Select next row if available
    if (nextRow) {
      nextRow.select();
      nextRow.scrollTo();
    }

    await this.exitEditMode('cancel');

    toast.success('Row Deleted', { duration: 3000 });
  },

  // ── Edit Mode ─────────────────────────────────────────────────────────────

  async enterEditMode(row, targetCell = null) {
    if (!this.table || this.readonly || this.alwaysEditable) return;

    this.setEditTransitionState(true);

    try {
      if (!row) {
        const selected = this.table.getSelectedRows();
        if (selected.length === 0) return;
        row = selected[0];
      }

      const pkField = this.primaryKeyField || 'id';
      const rowId = row.getData()?.[pkField];

      if (this.isEditing && this.editingRowId === rowId) return;

      // Exit any existing edit mode first
      if (this.isEditing) {
        await this.exitEditMode();
      }

      this.editingRowId = rowId;
      this.isEditing = true;
      this.originalRowData = { ...row.getData() };

      this.container?.classList.add('lithium-edit-mode');
      this.updateSelectorCell(row, true);
      this.updateSaveCancelButtonState();
      this.updateEditButtonState();

      this.onEditModeChange(true, row.getData());

      if (targetCell) {
        this.queueCellEdit(targetCell);
      }

      log(Subsystems.TABLE, Status.INFO,
        `[LithiumTable] Entered edit mode (row ${rowId})`);
    } finally {
      requestAnimationFrame(() => this.setEditTransitionState(false));
    }
  },

  async exitEditMode(reason = 'cancel') {
    if (this.alwaysEditable || !this.isEditing) return;

    this.setEditTransitionState(true);

    try {
      const previousId = this.editingRowId;

      this.cancelActiveCellEdit();

      this.editingRowId = null;
      this.isEditing = false;

      this.container?.classList.remove('lithium-edit-mode');
      this.updateEditingIndicator();
      this.updateSaveCancelButtonState();
      this.updateEditButtonState();

      if (reason !== 'save') {
        this.isDirty = false;
        this.originalRowData = null;
        this.notifyDirtyChange();
      }

      this.onEditModeChange(false, null);

      log(Subsystems.TABLE, Status.INFO,
        `[LithiumTable] Exited edit mode (row ${previousId}, reason: ${reason})`);
    } finally {
      requestAnimationFrame(() => this.setEditTransitionState(false));
    }
  },

  setEditTransitionState(pending) {
    this.editTransitionPending = pending;
    this.container?.classList.toggle('lithium-edit-transition', pending);
    this.navigatorContainer?.classList.toggle('lithium-edit-transition', pending);
  },

  // Save/Cancel buttons have been moved to the Manager footer.
  // This method is kept as a no-op for compatibility with existing callers.
  updateSaveCancelButtonState() {
    // No-op: Save/Cancel buttons are now in the Manager footer, not the Navigator.
  },

  async autoSaveBeforeRowChange() {
    try {
      // IMPORTANT: Save the EDITING row, not the newly-selected row.
      // By the time this method is called, Tabulator has already selected
      // the new row.  getSelectedRows() would return the wrong one.
      const row = this.getEditingRow();

      if (!row) return true;

      await this.executeSave(row);
      this.isDirty = false;
      this.updateSaveCancelButtonState();
      this.notifyDirtyChange();

      toast.success('Changes Saved', { duration: 3000 });

      return true;
    } catch (error) {
      toast.error('Save Failed', {
        serverError: error.serverError,
        subsystem: 'Conduit',
        duration: 6000,
      });
      return false;
    }
  },

  updateEditingIndicator() {
    if (!this.table) return;
    const selected = this.table.getSelectedRows();
    if (selected.length > 0) {
      this.updateSelectorCell(selected[0], true);
    }
  },

  // ── Cell Editing ──────────────────────────────────────────────────────────

  queueCellEdit(cell) {
    if (!cell) return;

    const field = cell.getField?.();
    if (!field || !this.columnEditors.has(field)) return;

    const editorConfig = this.columnEditors.get(field) || {};

    requestAnimationFrame(() => {
      requestAnimationFrame(() => {
        if (!this.isEditing) return;

        const row = cell.getRow?.();
        if (!row || this.isCalcRow(row)) return;

        const pkField = this.primaryKeyField || 'id';
        const rowId = row.getData?.()?.[pkField];
        const isEditingRow = this.editingRowId != null
          ? rowId === this.editingRowId
          : row.isSelected?.();

        if (!isEditingRow) return;

        cell.edit();

        requestAnimationFrame(() => {
          this.openActiveCellEditor?.(cell, editorConfig);
        });
      });
    });
  },

  openActiveCellEditor(cell, editorConfig = {}) {
    const editorType = typeof editorConfig?.editor === 'string'
      ? editorConfig.editor
      : null;

    if (editorType !== 'select' && editorType !== 'list') return;

    const cellEl = cell?.getElement?.();
    if (!cellEl) return;

    const editorEl = cellEl.querySelector('select, input, textarea, [role="combobox"]');
    if (!editorEl) return;

    editorEl.focus?.({ preventScroll: true });

    if (editorType === 'select' && typeof editorEl.showPicker === 'function') {
      try {
        editorEl.showPicker();
        return;
      } catch {
        // Fall through to synthetic events for browsers that reject showPicker().
      }
    }

    const mouseEventOptions = {
      bubbles: true,
      cancelable: true,
      view: window,
    };

    editorEl.dispatchEvent(new MouseEvent('pointerdown', mouseEventOptions));
    editorEl.dispatchEvent(new MouseEvent('mousedown', mouseEventOptions));
    editorEl.dispatchEvent(new MouseEvent('click', mouseEventOptions));

    if (editorType === 'list') {
      editorEl.dispatchEvent(new KeyboardEvent('keydown', {
        key: 'ArrowDown',
        code: 'ArrowDown',
        bubbles: true,
      }));
    }
  },

  commitActiveCellEdit(nextCell = null) {
    const active = document.activeElement;
    if (!active || !this.container?.contains(active)) return;

    const activeCellEl = active.closest?.('.tabulator-cell');
    const nextCellEl = nextCell?.getElement?.() || null;
    if (activeCellEl && nextCellEl && activeCellEl === nextCellEl) {
      return;
    }

    active.blur();
  },

  cancelActiveCellEdit() {
    this.commitActiveCellEdit();
  },

  // ── Keyboard Handling ─────────────────────────────────────────────────────

  setupKeyboardHandling() {
    this.container?.setAttribute('tabindex', '0');
    this.container?.addEventListener('keydown', (e) => {
      this.handleTableKeydown(e);
    });
  },

  handleTableKeydown(e) {
    if (!this.table) return;

    // Check if we're currently editing a cell
    const activeEl = document.activeElement;
    const isEditingCell = activeEl?.closest?.('.tabulator-cell.tabulator-editing');

    // Handle Ctrl+Enter to save (when in edit mode)
    if (e.key === 'Enter' && e.ctrlKey) {
      if (this.isEditing) {
        e.preventDefault();
        e.stopPropagation();
        this.handleSave();
        return;
      }
    }

    // Handle Escape to cancel edit mode (always prevent propagation when in edit mode)
    if (e.key === 'Escape') {
      if (this.isEditing) {
        e.preventDefault();
        e.stopPropagation();
        this.handleCancel();
        return;
      }
      // If not in edit mode, let Escape propagate for other handlers
    }

    // If editing a cell, allow normal editing behavior for other keys
    if (isEditingCell) {
      return;
    }

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
      case 'Enter':
        e.preventDefault();
        if (!this.readonly && !this.alwaysEditable) {
          const selected = this.table.getSelectedRows();
          if (selected.length > 0) {
            this.enterEditMode(selected[0]);
          }
        }
        break;
      case 'F2':
        if (!this.readonly && !this.alwaysEditable) {
          e.preventDefault();
          this.handleEdit();
        }
        break;
      case 'Delete':
        if (!this.readonly && !this.alwaysEditable && e.ctrlKey) {
          e.preventDefault();
          this.handleDelete();
        }
        break;
    }
  },
};

export default LithiumTableOpsMixin;
