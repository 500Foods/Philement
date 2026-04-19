/**
 * Refresh Orchestrator Module
 *
 * Handles table configuration reload and refresh orchestration.
 *
 * @module tables/refresh-orchestrator
 */

import { log, Subsystems, Status } from '../core/log.js';
import { _tableDefCache } from './lithium-table.js';

/**
 * Reload the table configuration (schema) from Lookup 59.
 * This is called when the user clicks the refresh button to pick up
 * any changes to the table definition in Lookup 59.
 *
 * Enhanced to:
 * 0. Cancel edit mode first (save if dirty)
 * 1. Capture complete current state before refresh
 * 2. Reload Lookup 59
 * 3. Apply captured template instead of Default
 * 4. Re-submit original data request
 * 5. Re-select originally selected row and fire selection events
 *
 * @param {Object} table - LithiumTable instance
 */
export async function reloadConfiguration(table) {
  log(Subsystems.TABLE, Status.INFO, `[${table.cssPrefix}] Reloading table configuration...`);

  // 0. CANCEL EDIT MODE FIRST - save if dirty, otherwise cancel
  if (table.isEditing) {
    if (table.isDirty) {
      // Save any pending changes before refresh
      await table.handleSave?.();
      log(Subsystems.TABLE, Status.DEBUG, `[${table.cssPrefix}] Saved changes before refresh`);
    }
    // Cancel edit mode
    await table.exitEditMode?.('cancel');
    log(Subsystems.TABLE, Status.DEBUG, `[${table.cssPrefix}] Exited edit mode before refresh`);
  }

  // 1. CAPTURE COMPLETE CURRENT STATE before clearing anything
  const capturedState = table.captureCurrentState?.();
  const capturedRowId = table.getSelectedRowId?.() || getCurrentlySelectedRowId(table);

  log(Subsystems.TABLE, Status.DEBUG,
    `[${table.cssPrefix}] Captured state: ${capturedRowId ? 'row selected' : 'no row'}, ${Object.keys(capturedState?.columns || {}).length} columns`);

  // 2. Clear the table definition cache for this path so it re-fetches from Lookup 59
  if (table.tablePath && _tableDefCache.has(table.tablePath)) {
    _tableDefCache.delete(table.tablePath);
    log(Subsystems.TABLE, Status.DEBUG, `[${table.cssPrefix}] Cleared tableDef cache for "${table.tablePath}"`);
  }

  // 3. Reload configuration (fetches fresh tableDef from Lookup 59)
  table.tableDef = null;
  await table.loadConfiguration();

  // 4. Destroy existing Tabulator table
  if (table.table) {
    table.table.destroy();
    table.table = null;
  }

  // 5. Recreate the Tabulator table with new configuration
  await table.initTable();

  // 6. Restore filters visible state (if previously enabled)
  if (table.filtersVisible && table.table) {
    table.toggleHeaderFilters(true);
  }

  // 7. Apply captured state (user customizations) for all tables
  if (capturedState) {
    const sampleWidth = Object.values(capturedState.columns || {})[0]?.width;
    log(Subsystems.TABLE, Status.DEBUG, `[${table.cssPrefix}] Applying captured state: ${Object.keys(capturedState.columns || {}).length} columns, first width: ${sampleWidth}`);

    await table.loadTemplate?.(capturedState);
    log(Subsystems.TABLE, Status.DEBUG, `[${table.cssPrefix}] Applied captured state template`);

    // Debug: verify widths were applied after setColumns
    if (table.table) {
      const cols = table.table.getColumns();
      const firstCol = cols.find(c => c.getField() !== '_selector');
      const afterWidth = firstCol?.getWidth?.();
      log(Subsystems.TABLE, Status.DEBUG, `[${table.cssPrefix}] After template, first column width: ${afterWidth}`);
    }
  }

  // 8. Load data WITHOUT automatic selection (we'll handle it explicitly)
  // Temporarily store the captured ID so loadData can use it
  const savedAutoSelectRow = table.autoSelectRow;
  table.autoSelectRow = () => {}; // No-op during refresh data load

  try {
    if (typeof table.onRefresh === 'function') {
      await table.onRefresh();
    } else if (table.lastLoadParams) {
      await table.loadData?.(table.lastLoadParams.searchTerm, table.lastLoadParams.extraParams);
    } else {
      await table.loadData?.();
    }
  } finally {
    // Restore original autoSelectRow
    table.autoSelectRow = savedAutoSelectRow;
  }

  // 9. Explicitly restore the captured row selection AFTER data is loaded
  if (capturedRowId && table.table) {
    log(Subsystems.TABLE, Status.DEBUG, `[${table.cssPrefix}] Restoring row selection: ${capturedRowId}`);

    // Wait a tick for Tabulator to finish rendering
    await new Promise(resolve => requestAnimationFrame(resolve));

    table.autoSelectRow?.(capturedRowId);

    // Fire selection event to update manager UI
    const selectedRows = table.table.getSelectedRows();
    if (selectedRows.length > 0 && typeof table.onRowSelected === 'function') {
      const rowData = selectedRows[0].getData();
      log(Subsystems.TABLE, Status.DEBUG, `[${table.cssPrefix}] Row restored, firing onRowSelected`);
      table.onRowSelected(rowData);
    } else {
      log(Subsystems.TABLE, Status.WARN, `[${table.cssPrefix}] Could not restore row ${capturedRowId}`);
    }
  }

  log(Subsystems.TABLE, Status.INFO, `[${table.cssPrefix}] Table configuration reloaded`);

  // Notify the manager that refresh completed (for parent/child coordination)
  if (typeof table.onRefreshComplete === 'function') {
    table.onRefreshComplete();
  }
}

/**
 * Get the currently selected row ID directly from Tabulator
 * @param {Object} table - LithiumTable instance
 * @returns {string|null} Selected row ID
 */
export function getCurrentlySelectedRowId(table) {
  if (!table.table) return null;
  const selectedRows = table.table.getSelectedRows();
  if (selectedRows.length === 0) return null;

  const row = selectedRows[0];
  const data = row.getData();

  // For compound keys, construct composite ID
  if (Array.isArray(table.primaryKeyField)) {
    return table.primaryKeyField.map(f => data[f]).join('::');
  }

  // For single keys
  return data[table.primaryKeyField] ?? data.key_idx ?? data.id ?? null;
}

export default { reloadConfiguration, getCurrentlySelectedRowId };
