/**
 * Column Manager - Actions Module
 *
 * Apply/discard changes and template building.
 *
 * @module tables/column-manager/cm-actions
 */

import { log, Subsystems, Status } from '../../core/log.js';
import {
  extractTemplateColumnFromColumn,
  mergeTemplateColumn,
} from '../lithium-table-template.js';
import { captureParentStateAsOriginal } from './cm-data.js';
import { clearDirty, getCurrentColumnTableData } from './cm-state.js';

/**
 * Apply all pending changes to the parent table (called on save)
 * @param {Object} cm - ColumnManager instance
 */
export async function applyAllChangesToParent(cm) {
  if (!cm.parentTable?.table) return;

  const templateColumns = buildPendingTemplateColumns(cm);
  if (Object.keys(templateColumns).length > 0) {
    log(Subsystems.TABLE, Status.INFO,
      `[ColumnManager] Applying column state to parent table (${Object.keys(templateColumns).length} columns)`);
    cm.parentTable.applyTemplateColumns(templateColumns);
  }

  captureParentStateAsOriginal(cm);
  cm.columnData = getCurrentColumnTableData(cm);
  cm._pendingReorder = null;

  clearDirty(cm);
}

/**
 * Discard all pending changes (called on cancel)
 * @param {Object} cm - ColumnManager instance
 */
export async function discardAllChanges(cm) {
  if (!cm.columnTable?.table) {
    clearDirty(cm);
    return;
  }

  log(Subsystems.TABLE, Status.INFO, '[ColumnManager] Discarding pending changes');

  // Reload original column data to revert any edits
  await cm.columnTable.table.replaceData(cm.columnData);

  cm._pendingReorder = null;
  clearDirty(cm);
}

/**
 * Build a template-state patch from the current column manager rows.
 * @param {Object} cm - ColumnManager instance
 * @returns {Object} Template columns
 */
export function buildPendingTemplateColumns(cm) {
  if (!cm.parentTable?.table) return {};

  const rowData = getCurrentColumnTableData(cm);
  const rowMap = new Map(rowData.map((row) => [row.field_name, row]));
  const parentColumns = cm.parentTable.table.getColumns();
  const templateColumns = {};

  parentColumns.forEach((column) => {
    const field = column.getField();
    if (!field || field === '_selector') return;

    const baseColumn = extractTemplateColumnFromColumn(column, cm.parentTable.primaryKeyField);
    const managerRow = rowMap.get(field);
    if (!managerRow) {
      templateColumns[field] = baseColumn;
      return;
    }

    const patchColumn = {
      display: managerRow.column_name,
      field,
      coltype: managerRow.format,
      visible: managerRow.visible,
      editable: managerRow.editable,
    };

    const overrides = {
      align: managerRow.alignment,
      bottomCalc: managerRow.summary === 'none' ? null : managerRow.summary,
    };

    const parsedWidth = managerRow.width == null || managerRow.width === ''
      ? null
      : parseInt(managerRow.width, 10);
    if (Number.isFinite(parsedWidth) && parsedWidth > 0) {
      overrides.width = parsedWidth;
    }

    if (Object.keys(overrides).length > 0) {
      patchColumn.overrides = overrides;
    }
    templateColumns[field] = mergeTemplateColumn(baseColumn, patchColumn);
  });

  // Order columns according to row data
  const orderedTemplateColumns = {};
  const orderedFields = rowData.map((row) => row.field_name);
  orderedFields.forEach((field) => {
    if (templateColumns[field]) {
      orderedTemplateColumns[field] = templateColumns[field];
    }
  });
  Object.entries(templateColumns).forEach(([field, config]) => {
    if (!orderedTemplateColumns[field]) {
      orderedTemplateColumns[field] = config;
    }
  });

  return orderedTemplateColumns;
}

/**
 * Handle cell edit in the column manager
 * @param {Object} cm - ColumnManager instance
 * @param {Object} cell - Tabulator cell
 */
export async function handleCellEdit(cm, cell) {
  const rowData = cell.getRow().getData();
  const field = cell.getField();
  const value = cell.getValue();

  log(Subsystems.TABLE, Status.DEBUG,
    `[ColumnManager] Cell edited: ${field} = ${value} for column ${rowData.field_name}`);

  // In Auto mode, editing the 'order' column triggers a reorder
  if (field === 'order' && cm.orderingMode === 'auto') {
    handleOrderEdit(cm, cell.getRow(), value);
    return;
  }

  cm.syncDirtyState();

  // Notify callback
  cm.onColumnChange(rowData.field, field, value);
}

/**
 * Handle order column edit in Auto mode.
 * @param {Object} cm - ColumnManager instance
 * @param {Object} editedRow - The Tabulator row that was edited
 * @param {number} newOrder - The new order value
 */
function handleOrderEdit(cm, editedRow, newOrder) {
  if (!cm.columnTable?.table) return;

  const rows = cm.columnTable.table.getRows();
  const editedField = editedRow.getData().field_name;

  // Collect all rows with their current order, and apply the edit
  const rowOrders = rows.map((row) => {
    const data = row.getData();
    return {
      row,
      field_name: data.field_name,
      order: data.field_name === editedField ? parseInt(newOrder, 10) : data.order,
    };
  });

  // Sort by the new order values (stable sort, preserving original order for ties)
  rowOrders.sort((a, b) => a.order - b.order);

  // Renumber sequentially and update each row
  rowOrders.forEach((item, index) => {
    const newSeq = index + 1;
    if (item.row.getData().order !== newSeq) {
      item.row.update({ order: newSeq });
    }
  });

  // Track the reorder
  cm._pendingReorder = rowOrders.map((item) => item.field_name);
  cm.syncDirtyState();

  log(Subsystems.TABLE, Status.DEBUG,
    `[ColumnManager] Order edited, new sequence: ${cm._pendingReorder.join(', ')}`);
}

/**
 * Handle row reorder in the column manager
 * @param {Object} cm - ColumnManager instance
 */
export async function handleRowReorder(cm) {
  if (!cm.columnTable?.table) return;

  // Get the current row order from the column manager table
  const rows = cm.columnTable.table.getRows();
  const newFieldOrder = rows.map((row) => row.getData().field_name);

  // Update order numbers in the column manager
  rows.forEach((row, index) => {
    const data = row.getData();
    row.update({ ...data, order: index + 1 });
  });

  // Track the reorder change
  cm._pendingReorder = newFieldOrder;
  cm.syncDirtyState();

  log(Subsystems.TABLE, Status.DEBUG, `[ColumnManager] Row reordered, new order: ${newFieldOrder.join(', ')}`);
}
