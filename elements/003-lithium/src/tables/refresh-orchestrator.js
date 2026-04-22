/**
 * Refresh Orchestrator Module
 *
 * Handles table configuration reload and refresh orchestration.
 *
 * @module tables/refresh-orchestrator
 */

import { log, Subsystems, Status } from '../core/log.js';
import { scrollbarManager } from '../core/scrollbar-manager.js';
import { _tableDefCache } from './lithium-table.js';
import { performLocalSearch } from './search/search-utils.js';

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

  // Also capture local search term so it can be restored after reload
  const capturedLocalSearch = table._localSearchTerm || '';

  log(Subsystems.TABLE, Status.DEBUG,
    `[${table.cssPrefix}] Captured state: ${capturedRowId ? 'row selected' : 'no row'}, ${Object.keys(capturedState?.columns || {}).length} columns, local search: "${capturedLocalSearch}"`);

  // 2. Clear the table definition cache for this path so it re-fetches from Lookup 59
  if (table.tablePath && _tableDefCache.has(table.tablePath)) {
    _tableDefCache.delete(table.tablePath);
    log(Subsystems.TABLE, Status.DEBUG, `[${table.cssPrefix}] Cleared tableDef cache for "${table.tablePath}"`);
  }

  // 3. Reload configuration (fetches fresh tableDef from Lookup 59)
  table.tableDef = null;
  await table.loadConfiguration();

   // 4. Destroy existing Tabulator table and its OverlayScrollbar instance
   if (table._scrollbarInstance) {
     log(Subsystems.TABLE, Status.DEBUG, `[${table.cssPrefix}] refresh: destroying OSB before Tabulator destroy`);
     const oldHolder = table.container?.querySelector?.('.tabulator-tableholder');
     if (oldHolder && oldHolder.__overlayscrollbars) {
       delete oldHolder.__overlayscrollbars;
     }
     scrollbarManager.destroy(table._scrollbarInstance);
     table._scrollbarInstance = null;
     table.container?.classList?.remove('lithium-table-osb-enabled');
     log(Subsystems.TABLE, Status.DEBUG, `[${table.cssPrefix}] refresh: OSB destroyed`);
   }

   if (table.table) {
     table.table.destroy();
     table.table = null;
   }

  // 5. Recreate the Tabulator table with new configuration
  await table.initTable();
  log(Subsystems.TABLE, Status.DEBUG, `[${table.cssPrefix}] refresh: initTable completed, OSB instance after initTable: ${!!table._scrollbarInstance}`);

  // 6. Restore filters visible state (if previously enabled)
  if (table.filtersVisible && table.table) {
    table.toggleHeaderFilters(true);
  }

  // 7. Apply captured state (user customizations) for all tables
  if (capturedState) {
    const sampleWidth = Object.values(capturedState.columns || {})[0]?.width;
    log(Subsystems.TABLE, Status.DEBUG, `[${table.cssPrefix}] Applying captured state: ${Object.keys(capturedState.columns || {}).length} columns, first width: ${sampleWidth}`);

    // Clear OSB instance before template loads - setColumns() destroys table holder content
    if (table._scrollbarInstance) {
      log(Subsystems.TABLE, Status.DEBUG, `[${table.cssPrefix}] refresh: destroying OSB before loadTemplate`);
      scrollbarManager.destroy(table._scrollbarInstance);
      table._scrollbarInstance = null;
      table.container?.classList?.remove('lithium-table-osb-enabled');
      log(Subsystems.TABLE, Status.DEBUG, `[${table.cssPrefix}] refresh: OSB destroyed`);
    }

    await table.loadTemplate?.(capturedState);
    log(Subsystems.TABLE, Status.DEBUG, `[${table.cssPrefix}] refresh: loadTemplate completed, scheduling OSB reinit`);

    // Re-initialize OSB after setColumns() destroys and rebuilds everything
    setTimeout(() => {
      log(Subsystems.TABLE, Status.DEBUG, `[${table.cssPrefix}] refresh: _initTableScrollbars timeout fired`);
      table._initTableScrollbars();
    }, 0);
    setTimeout(() => {
      log(Subsystems.TABLE, Status.DEBUG, `[${table.cssPrefix}] refresh: _updateTableScrollbars timeout fired`);
      table._updateTableScrollbars();
    }, 100);
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
      if (table.currentData && table.currentData.length > 0) {
        // For static tables without onRefresh that have currentData, reload the static data
        await table.loadStaticData?.(table.currentData, { autoSelect: false });
      } else {
        await table.loadData?.();
      }
    }
  } finally {
    // Restore original autoSelectRow
    table.autoSelectRow = savedAutoSelectRow;
  }

  // 9. Explicitly restore the captured row selection AFTER data is loaded
  // Skip if table has custom onRefresh that handles selection itself
  if (!table.onRefresh && capturedRowId && table.table) {
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

  // 10. Restore local search filter if it was active
  if (capturedLocalSearch && table.table) {
    log(Subsystems.TABLE, Status.DEBUG, `[${table.cssPrefix}] Restoring local search: "${capturedLocalSearch}"`);
    performLocalSearch(table, capturedLocalSearch);
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
