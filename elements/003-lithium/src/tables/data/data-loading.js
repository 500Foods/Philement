/**
 * Data Loading Module
 *
 * Table data loading, static data, and row ID management.
 *
 * @module tables/data/data-loading
 */

import { authQuery } from '../../shared/conduit.js';
import { toast } from '../../shared/toast.js';
import { log, Subsystems, Status } from '../../core/log.js';

/**
 * Load data from the configured queryRef
 * @param {Object} table - LithiumTable instance
 * @param {string} searchTerm - Optional search term
 * @param {Object} extraParams - Optional extra parameters for the query
 */
export async function loadData(table, searchTerm = '', extraParams = {}) {
  if (!table.app?.api) return;

  // Set transition flag to prevent button flashing during data reload
  table._inSelectionTransition = true;

  // Get currently selected row, or restore from localStorage
  const previouslySelectedId = table.getSelectedRowId?.() ?? table.restoreSelectedRowId?.();
  table.showLoading?.();

  try {
    const listRef = table.queryRefs.queryRef;
    const searchRef = table.queryRefs.searchQueryRef;
    const queryRef = searchTerm && searchRef ? searchRef : listRef;

    if (!queryRef) {
      log(Subsystems.TABLE, Status.WARN, `[LithiumTable] No queryRef configured for data loading`);
      return;
    }

    // Check if extraParams are required but missing
    if (extraParams === null || extraParams === undefined) {
      const hasParams = extraParams && Object.keys(extraParams).length > 0;
      if (!hasParams) {
        log(Subsystems.TABLE, Status.DEBUG,
          `[LithiumTable] Data load skipped - requires runtime params (no extraParams provided)`);
        return;
      }
    }

    log(Subsystems.CONDUIT, Status.INFO,
      `[LithiumTable] Loading data (queryRef: ${queryRef}${searchTerm ? `, search: "${searchTerm}"` : ''})`);

    const params = { ...extraParams };

    if (searchTerm && searchRef) {
      params.STRING = { SEARCH: searchTerm.toUpperCase() };
    }

    // Track last load params for refresh re-submit
    table.lastLoadParams = {
      searchTerm: searchTerm || '',
      extraParams: extraParams ? { ...extraParams } : {},
    };

    const rows = await authQuery(table.app.api, queryRef, Object.keys(params).length > 0 ? params : undefined);

    if (!table.table) return;

    table.currentData = rows;

    // If table was initialized without data but we now have data, rebuild columns
    if (table.table && rows.length > 0 && table._wasInitializedWithoutData) {
      await table._rebuildColumnsWithData?.(rows);
    }

    table.table.blockRedraw?.();
    try {
      table.table.setData(rows);
      table.discoverColumns?.(rows);
    } finally {
      table.table.restoreRedraw?.();
    }

    requestAnimationFrame(() => {
      table._updateTableScrollbars?.();
      table.autoSelectRow?.(previouslySelectedId);
      table.updateMoveButtonState?.();
      table.updateDuplicateButtonState?.();
      // Clear transition flag after selection is restored
      table._inSelectionTransition = false;
    });

    table.onDataLoaded(rows);
  } catch (error) {
    // Build informative error details
    const queryRef = searchTerm && table.queryRefs.searchQueryRef
      ? table.queryRefs.searchQueryRef
      : table.queryRefs.queryRef;
    const errorDetails = error.serverError || error.message || 'Unknown error';
    const errorContext = `QueryRef: ${queryRef}${searchTerm ? `, Search: "${searchTerm}"` : ''}`;

    log(Subsystems.TABLE, Status.ERROR,
      `[LithiumTable] Data load failed: ${errorDetails} (${errorContext})`);

    toast.error('Data Load Failed', {
      description: `${errorDetails} (${errorContext})`,
      duration: 8000,
    });
    table.currentData = [];
  } finally {
    table.hideLoading?.();
    // Ensure transition flag is cleared even on error
    table._inSelectionTransition = false;
  }
}

/**
 * Get the selected row ID from the table
 * @param {Object} table - LithiumTable instance
 * @returns {string|null} Row ID
 */
export function getSelectedRowId(table) {
  if (!table.table) return null;
  const selected = table.table.getSelectedRows();
  if (selected.length > 0) {
    const pkFields = table.primaryKeyField;
    return getCompositeRowId(selected[0].getData(), pkFields);
  }
  return null;
}

/**
 * Get the selected row data
 * @param {Object} table - LithiumTable instance
 * @returns {Object|null} Row data
 */
export function getSelectedRowData(table) {
  if (!table.table) return null;
  const selected = table.table.getSelectedRows();
  if (selected.length > 0) {
    return selected[0].getData();
  }
  return null;
}

/**
 * Auto-select a row by ID
 * @param {Object} table - LithiumTable instance
 * @param {string|number} targetId - Row ID to select
 */
export function autoSelectRow(table, targetId) {
  if (!table.table) return;
  const rows = table.table.getRows('active');
  // If no rows yet and we have a target to find, wait for rows to render
  if (rows.length === 0 && targetId == null) return;

  const pkFields = table.primaryKeyField;

  // Set transition flag to suppress button updates during deselect/select
  table._inSelectionTransition = true;

  // Use blockRedraw to prevent Tabulator from processing events between deselect/select
  table.table.blockRedraw?.();
  try {
    // Deselect any existing selections first
    const selectedRows = table.table.getSelectedRows();
    if (selectedRows.length > 0) {
      table.table.deselectRow(selectedRows);
    }

    if (targetId != null && pkFields !== null && pkFields.length > 0) {
      for (const row of rows) {
        const rowId = getCompositeRowId(row.getData(), pkFields);
        if (String(rowId) === String(targetId)) {
          row.select();
          // Defer scroll to ensure Tabulator has rendered the row
          requestAnimationFrame(() => {
            row?.scrollTo?.('center', false);
          });
          // Save selected row ID for persistence across sessions
          table.saveSelectedRowId?.(getCompositeRowId(row.getData(), pkFields));
          break;
        }
      }
    } else {
      if (rows.length > 0) {
        rows[0].select();
        // Defer scroll to ensure Tabulator has rendered the row
        requestAnimationFrame(() => {
          rows[0]?.scrollTo?.('center', false);
        });
        // Save selected row ID for persistence across sessions
        table.saveSelectedRowId?.(getCompositeRowId(rows[0].getData(), pkFields));
      }
    }
  } finally {
    table.table.restoreRedraw?.();
  }

  // Clear transition flag and update buttons directly
  table._inSelectionTransition = false;
  table.updateMoveButtonState?.();
  table.updateDuplicateButtonState?.();
}

/**
 * Get composite row ID from row data using primary key fields
 * @param {Object} rowData - Row data object
 * @param {string[]} pkFields - Array of primary key field names
 * @returns {string} Composite ID
 */
export function getCompositeRowId(rowData, pkFields) {
  if (!pkFields || pkFields.length === 0) {
    // Fallback: try common primary key field names
    const fallbackFields = ['id', 'query_id', 'lookup_id', 'key_idx', 'style_id', 'version_id'];
    for (const field of fallbackFields) {
      if (rowData[field] != null) {
        return String(rowData[field]);
      }
    }
    // Last resort: use the first field that looks like an ID
    const idFields = Object.keys(rowData).filter(key => key.toLowerCase().includes('id') || key === 'key_idx');
    if (idFields.length > 0) {
      return String(rowData[idFields[0]]);
    }
    return '';
  }
  if (pkFields.length === 1) {
    return String(rowData[pkFields[0]] ?? '');
  }
  return pkFields.map(f => String(rowData[f] ?? '')).join('::');
}

/**
 * Load static (non-API) data into the table.
 * @param {Object} table - LithiumTable instance
 * @param {Array<Object>} rows - The data array to load
 * @param {Object} [options]
 * @param {boolean} [options.autoSelect=true] - Auto-select first row after load
 */
export function loadStaticData(table, rows, options = {}) {
  if (!table.table) return;
  const { autoSelect = true } = options;

  // Set transition flag to prevent button flashing during data reload
  table._inSelectionTransition = true;

  table.currentData = rows || [];

  // Get currently selected row, or restore from localStorage
  const previouslySelectedId = autoSelect ? (table.getSelectedRowId?.() ?? table.restoreSelectedRowId?.()) : null;

  table.table.blockRedraw?.();
  try {
    table.table.setData(table.currentData);
    table.discoverColumns?.(table.currentData);
  } finally {
    table.table.restoreRedraw?.();
  }

  if (autoSelect) {
    requestAnimationFrame(() => {
      table._updateTableScrollbars?.();
      if (previouslySelectedId != null) {
        table.autoSelectRow?.(previouslySelectedId);
      } else {
        const activeRows = table.table.getRows('active');
        if (activeRows.length > 0) {
          activeRows[0].select();
          // Defer scroll to ensure Tabulator has rendered the row
          requestAnimationFrame(() => {
            activeRows[0]?.scrollTo?.('center', false);
          });
          // Save selected row ID for persistence across sessions
          const pkFields = table.primaryKeyField;
          if (pkFields) {
            table.saveSelectedRowId?.(getCompositeRowId(activeRows[0].getData(), pkFields));
          }
        }
      }
      table.updateMoveButtonState?.();
      table.updateDuplicateButtonState?.();
      // Clear transition flag after selection is restored
      table._inSelectionTransition = false;
    });
  } else {
    table._updateTableScrollbars?.();
    table._inSelectionTransition = false;
    table.updateMoveButtonState?.();
    table.updateDuplicateButtonState?.();
  }

  table.onDataLoaded(table.currentData);
}

/**
 * Load initial data for the table
 * @param {Object} table - LithiumTable instance
 * @param {Object} extraParams - Optional extra parameters
 * @returns {Promise<Array|null>} Loaded rows or null
 */
export async function loadInitialData(table, extraParams = null) {
  if (!table.app?.api || !table.queryRefs.queryRef) return null;

  try {
    const rows = await authQuery(table.app.api, table.queryRefs.queryRef, extraParams);
    table.currentData = rows;
    return rows;
  } catch (err) {
    const isMissingParam = err.message?.includes('Missing required parameters') ||
                         err.message?.includes('Required:') ||
                         err.message?.includes('missing');
    if (isMissingParam) {
      log(Subsystems.CONDUIT, Status.DEBUG,
        `[LithiumTable] Initial load skipped - requires runtime params (${err.message})`);
    } else {
      log(Subsystems.CONDUIT, Status.ERROR, `[LithiumTable] Failed to load initial data: ${err.message}`);
    }
    return null;
  }
}

export default {
  loadData,
  getSelectedRowId,
  getSelectedRowData,
  autoSelectRow,
  getCompositeRowId,
  loadStaticData,
  loadInitialData,
};
