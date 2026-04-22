/**
 * Table Events Module
 *
 * Wires Tabulator table events to LithiumTable handlers.
 *
 * @module tables/events/table-events
 */

import { processIcons } from '../../core/icons.js';

/**
 * Wire all Tabulator table events
 * @param {Object} table - LithiumTable instance
 */
export function wireTableEvents(table) {
  if (!table.table) return;

  const refreshScrollbars = () => table._updateTableScrollbars?.();

  table.table.on('rowClick', (e, row) => {
    if (table.isCalcRow(row)) return;

    // Skip row selection if clicking on drag handle cell (row reordering)
    if (e?.target) {
      const cell = e.target.closest?.('.tabulator-cell');
      if (cell?.getAttribute?.('data-tabulator-field') === 'drag_handle') {
        return;
      }
    }

    // When editing, check if clicking the same row
    // If so, don't re-select (which would trigger handleRowSelected)
    if (table.isEditing) {
      const pkFields = table.primaryKeyField;
      const clickedRowId = table._getCompositeRowId?.(row.getData(), pkFields);
      if (table.editingRowId === clickedRowId) {
        // Same row - just ensure it's selected without triggering row change logic
        if (!row.isSelected?.()) {
          row.select();
        }
        return;
      }
    }

    table.selectDataRow(row);
  });

  table.table.on('cellMouseDown', (e, cell) => {
    const field = cell.getField();
    if (field === '_selector' || field === 'drag_handle' || table.isCalcCell(cell)) return;

    const row = cell.getRow();

    // Only select row on mouse down if not already editing
    // When editing, we handle row changes via cellClick or rowSelected
    if (!table.isEditing) {
      table.selectDataRow(row);
      return;
    }

    // When editing, check if clicking the same row
    // If same row, don't re-select to avoid triggering row change logic
    if (table.isEditing) {
      const pkFields = table.primaryKeyField;
      const clickedRowId = table._getCompositeRowId?.(row.getData(), pkFields);
      if (table.editingRowId === clickedRowId) {
        // Same row - just ensure it's selected without triggering row change logic
        if (!row.isSelected?.()) {
          row.select();
        }
        return;
      }
      // Different row - let the selection proceed (will trigger handleRowSelected)
      table.selectDataRow(row);
    }
  });

  table.table.on('rowDblClick', (e, row) => {
    if (table.isCalcRow(row) || table.readonly || table.alwaysEditable) return;
    table.selectDataRow(row);
    void table.enterEditMode(row);
  });

  table.table.on('cellDblClick', (e, cell) => {
    const row = cell?.getRow?.();
    const field = cell?.getField?.();
    if (!row || table.isCalcRow(row) || table.readonly || table.alwaysEditable) return;
    if (field === '_selector' || field === 'drag_handle' || table.isCalcCell(cell)) return;

    table.selectDataRow(row);
    void table.enterEditMode(row, cell);
  });

  table.table.on('cellClick', (e, cell) => {
    const field = cell.getField();
    if (field === '_selector' || field === 'drag_handle' || table.isCalcCell(cell)) return;

    e?.stopPropagation?.();
    e?.stopImmediatePropagation?.();

    const row = cell.getRow();
    const pkFields = table.primaryKeyField;
    const clickedRowId = table._getCompositeRowId?.(row.getData(), pkFields);
    const isSameRow = table.isEditing && table.editingRowId === clickedRowId;

    // When editing, only select row if clicking a different row
    // (handleRowSelected will auto-save and exit edit mode)
    // For same row, we stay in edit mode and just try to open the cell editor
    if (!table.isEditing || !isSameRow) {
      table.selectDataRow(row);
    }

    if (table.isEditing && isSameRow) {
      // Same row: commit any active edit and open editor for clicked cell
      table.commitActiveCellEdit(cell);
      table.queueCellEdit(cell);
    }
  });

  table.table.on('cellEdited', () => {
    table.isDirty = true;
    table.updateSaveCancelButtonState();
    table.notifyDirtyChange();
  });

  table.table.on('rowSelected', async (row) => {
    if (table.isCalcRow(row)) return;
    await table.handleRowSelected(row);
  });

  table.table.on('rowDeselected', (row) => {
    if (table.isCalcRow(row)) return;
    table.updateSelectorCell(row, false);
  });

  table.table.on('rowSelectionChanged', () => {
    if (table._inSelectionTransition) return;

    const selectedRows = table.table.getSelectedRows();
    if (selectedRows.length > 1) {
      table._inSelectionTransition = true;
      const rowsToDeselect = selectedRows.slice(1);
      table.table.deselectRow(rowsToDeselect);
    }

    table.updateMoveButtonState();
    table.updateDuplicateButtonState();
  });

  table.table.on('columnVisibilityChanged', () => {
    table.updateVisibleColumnClasses();
    table.initColumnHeaderTooltips();
    refreshScrollbars();
  });

  table.table.on('columnMoved', () => {
    table.updateVisibleColumnClasses();
    table.initColumnHeaderTooltips();
    refreshScrollbars();
  });

  table.table.on('tableBuilt', () => {
    refreshScrollbars();
    setTimeout(() => {
      table.updateVisibleColumnClasses();
      table.initColumnHeaderTooltips();
      refreshScrollbars();
    }, 100);
  });

  table.table.on('rowRendered', (row) => {
    table.applyVisibleColumnClassesToRow(row);
    const rowEl = row?.getElement?.();
    if (rowEl) processIcons(rowEl);
  });

  table.table.on('dataLoaded', () => {
    refreshScrollbars();
    setTimeout(() => {
      table.updateVisibleColumnClasses();
      table.initColumnHeaderTooltips();
      processIcons(table.container);
      refreshScrollbars();
    }, 100);
  });

  // Group toggle icon sync
  table.table.on('groupVisibilityChanged', () => table.updateGroupIcons());
  table.table.on('dataLoaded', () => table.updateGroupIcons());
  table.table.on('renderComplete', () => {
    table.updateGroupIcons();
    processIcons(table.container);
    refreshScrollbars();
  });
  table.table.on('tableRedrawn', () => {
    table.updateGroupIcons();
    refreshScrollbars();
  });
}

export default { wireTableEvents };
