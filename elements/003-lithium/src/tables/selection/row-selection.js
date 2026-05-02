/**
 * Row Selection Module
 *
 * Row selection, matching, and helper functions.
 *
 * @module tables/selection/row-selection
 */

import { getCompositeRowId } from '../data/data-loading.js';

/**
 * Handle row selected event
 * @param {Object} table - LithiumTable instance
 * @param {Object} row - Tabulator row component
 */
export async function handleRowSelected(table, row) {
  // Enforce single row selection by deselecting all other rows
  const selectedRows = table.table.getSelectedRows();
  const otherRows = selectedRows.filter(selectedRow => selectedRow !== row);
  if (otherRows.length > 0) {
    table.table.deselectRow(otherRows);
    // Dispatch event to close all popups when selecting a new row
    document.dispatchEvent(new CustomEvent('close-all-popups'));
  }

  const pkFields = table.primaryKeyField;
  const newRowId = getCompositeRowId(row.getData(), pkFields);

  // If editing and clicked the same row, just update UI and return
  // This prevents exiting edit mode when clicking different cells in the same row
  if (table.isEditing && table.editingRowId === newRowId) {
    table.updateSelectorCell?.(row, true);
    return;
  }

  // Commit any active cell edit before checking dirty state
  // This ensures cell changes are captured before auto-save decision
  table.commitActiveCellEdit?.();

  if (table.isEditing && table.editingRowId !== newRowId) {
    // Synchronous dirty check — the rAF-deferred isDirty flag may be stale
    // if the user typed in an editor and immediately clicked a different row.
    const actuallyDirty = table.isActuallyDirty?.();

    if (actuallyDirty) {
      // Default behaviour: auto-save the editing row, then switch.
      const saveSucceeded = await table.autoSaveBeforeRowChange?.();
      if (!saveSucceeded) {
        const editingRow = getEditingRow(table);
        if (editingRow) editingRow.select();
        return;
      }
      await table.exitEditMode?.('save');
    } else {
      await table.exitEditMode?.('row-change');
    }
  }

  table.updateSelectorCell?.(row, true);

  // Save selected row ID for persistence across sessions
  if (newRowId != null) {
    table.saveSelectedRowId?.(newRowId);
  }

  table.onRowSelected(row.getData());

  // Update audit footer if attached
  table.auditFooter?.update(row.getData());
}

/**
 * Select a data row
 * @param {Object} table - LithiumTable instance
 * @param {Object} row - Tabulator row component
 * @returns {boolean} true if selection changed
 */
export function selectDataRow(table, row) {
  if (!table.table || !row || isCalcRow(table, row)) return false;

  const currentRow = getSelectedDataRow(table);
  if (rowsMatch(table, currentRow, row)) {
    if (!row.isSelected?.()) row.select();
    return false;
  }

  table.closeTransientPopups?.();
  table.table.deselectRow();
  row.select();
  return true;
}

/**
 * Get the currently selected data row
 * @param {Object} table - LithiumTable instance
 * @returns {Object|null} Tabulator row component
 */
export function getSelectedDataRow(table) {
  if (!table.table) return null;
  const selectedRows = table.table.getSelectedRows?.() || [];
  return selectedRows.find(row => !isCalcRow(table, row)) || null;
}

/**
 * Get the currently editing row
 * @param {Object} table - LithiumTable instance
 * @returns {Object|null} Tabulator row component
 */
export function getEditingRow(table) {
  if (!table.table || !table.isEditing || table.editingRowId == null) return null;

  const pkFields = table.primaryKeyField;
  const rows = table.table.getRows('active');
  return rows.find(row => {
    const rowId = getCompositeRowId(row.getData(), pkFields);
    return rowId === table.editingRowId;
  }) || null;
}

/**
 * Check if row is a calculation row
 * @param {Object} table - LithiumTable instance
 * @param {Object} row - Tabulator row component
 * @returns {boolean}
 */
export function isCalcRow(table, row) {
  if (!row) return false;
  const rowEl = row.getElement?.();
  return !!rowEl?.closest?.('.tabulator-calcs, .tabulator-calcs-holder, .tabulator-calcs-bottom');
}

/**
 * Check if cell is a calculation cell
 * @param {Object} table - LithiumTable instance
 * @param {Object} cell - Tabulator cell component
 * @returns {boolean}
 */
export function isCalcCell(table, cell) {
  if (!cell) return false;
  const cellEl = cell.getElement?.();
  if (cellEl?.closest?.('.tabulator-calcs, .tabulator-calcs-holder, .tabulator-calcs-bottom')) {
    return true;
  }
  return isCalcRow(table, cell.getRow?.());
}

/**
 * Check if two rows match by ID
 * @param {Object} table - LithiumTable instance
 * @param {Object} rowA - First row
 * @param {Object} rowB - Second row
 * @returns {boolean}
 */
export function rowsMatch(table, rowA, rowB) {
  if (!rowA || !rowB) return false;
  if (rowA === rowB) return true;

  const pkFields = table.primaryKeyField;
  const rowAId = getCompositeRowId(rowA.getData?.(), pkFields);
  const rowBId = getCompositeRowId(rowB.getData?.(), pkFields);
  return rowAId != null && rowBId != null && String(rowAId) === String(rowBId);
}

export default {
  handleRowSelected,
  selectDataRow,
  getSelectedDataRow,
  getEditingRow,
  isCalcRow,
  isCalcCell,
  rowsMatch,
};
