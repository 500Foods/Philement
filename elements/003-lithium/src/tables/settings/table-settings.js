/**
 * Table Settings Module
 *
 * Table width and layout mode management.
 *
 * @module tables/settings/table-settings
 */

import { log, Subsystems, Status } from '../../core/log.js';

/**
 * Set table width mode
 * @param {Object} table - LithiumTable instance
 * @param {string} mode - Width mode (narrow, compact, normal, wide, auto)
 */
export function setTableWidth(table, mode) {
  table.tableWidthMode = mode;
  saveTableWidth(table, mode);

  if (table.panel) {
    applyPanelWidth(table, mode);
  }

  if (typeof table.onSetTableWidth === 'function') {
    table.onSetTableWidth(mode);
  }

  log(Subsystems.TABLE, Status.INFO, `Table width set to: ${mode}`);
}

/**
 * Save table width mode to localStorage
 * @param {Object} table - LithiumTable instance
 * @param {string} mode - Width mode
 */
export function saveTableWidth(table, mode) {
  try {
    localStorage.setItem(`${table.storageKey}_width_mode`, mode);
  } catch (_e) {
    console.error(`[LithiumTable] FAILED to save width mode: ${_e?.message}`, { storageKey: table.storageKey, mode });
  }
}

/**
 * Restore table width mode from localStorage
 * @param {Object} table - LithiumTable instance
 * @returns {string|null} Width mode or null
 */
export function restoreTableWidth(table) {
  try {
    const stored = localStorage.getItem(`${table.storageKey}_width_mode`);
    if (stored && ['narrow', 'compact', 'normal', 'wide', 'auto'].includes(stored)) {
      return stored;
    }
  } catch (_e) {
    // Ignore storage errors
  }
  return null;
}

/**
 * Apply width mode to panel
 * Uses CSS custom properties for width values
 * @param {Object} table - LithiumTable instance
 * @param {string} mode - Width mode
 */
function applyPanelWidth(table, mode) {
  if (!table.panel) return;

  if (mode === 'auto') {
    table.panel.style.width = 'auto';
    table.panel.style.flex = '1';
    return;
  }

  // Get width from CSS variable
  const widthVar = `--table-width-${mode}`;
  const computedStyle = getComputedStyle(document.documentElement);
  const width = computedStyle.getPropertyValue(widthVar).trim();

  if (width) {
    table.panel.style.width = width;
    table.panel.style.flex = '0 0 auto';
  } else {
    // Fallback values if CSS vars not available
    const fallbacks = {
      narrow: '160px',
      compact: '314px',
      normal: '468px',
      wide: '622px',
    };
    table.panel.style.width = fallbacks[mode] || fallbacks.compact;
    table.panel.style.flex = '0 0 auto';
  }
}

/**
 * Set table layout mode
 * @param {Object} table - LithiumTable instance
 * @param {string} mode - Layout mode (fitColumns, fitData, fitDataFill, fitDataStretch, fitDataTable)
 */
export async function setTableLayout(table, mode) {
  if (!table.table) return;

  table.tableLayoutMode = mode;
  saveLayoutMode(table, mode);

  const currentData = table.table.getData();
  const selectedId = table.getSelectedRowId?.();

  table.table.destroy();
  table.table = null;

  await table.initTable?.();
  table.table.setData(currentData);

  await new Promise((resolve) => {
    requestAnimationFrame(() => {
      table.autoSelectRow?.(selectedId);
      table.updateMoveButtonState?.();
      resolve();
    });
  });

  log(Subsystems.TABLE, Status.INFO, `Table layout set to: ${mode}`);
}

/**
 * Save layout mode to localStorage
 * @param {Object} table - LithiumTable instance
 * @param {string} mode - Layout mode
 */
export function saveLayoutMode(table, mode) {
  try {
    localStorage.setItem(`${table.storageKey}_layout_mode`, mode);
  } catch (_e) {
    // Silently handle
  }
}

/**
 * Restore layout mode from localStorage
 * @param {Object} table - LithiumTable instance
 * @returns {string|null} Layout mode or null
 */
export function restoreLayoutMode(table) {
  try {
    const stored = localStorage.getItem(`${table.storageKey}_layout_mode`);
    if (stored && ['fitColumns', 'fitData', 'fitDataFill', 'fitDataStretch', 'fitDataTable'].includes(stored)) {
      return stored;
    }
  } catch (_e) {
    // Ignore storage errors
  }
  return null;
}
