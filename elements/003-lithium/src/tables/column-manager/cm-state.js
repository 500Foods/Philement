/**
 * Column Manager - State Management Module
 *
 * Dirty state tracking, persistence, and settings management.
 *
 * @module tables/column-manager/cm-state
 */

import { log, Subsystems, Status } from '../../core/log.js';

/**
 * Update save/cancel button state based on dirty flag
 * @param {Object} cm - ColumnManager instance
 */
export function updateButtonState(cm) {
  const isRowEditing = !!cm.columnTable?.isEditing;
  const rowDirty = isRowEditing && !!cm.columnTable?.isActuallyDirty?.();
  const popupDirty = !!cm.isDirty;

  if (cm.saveBtn) {
    cm.saveBtn.disabled = isRowEditing ? !rowDirty : !popupDirty;
    cm.saveBtn.classList.toggle('disabled', cm.saveBtn.disabled);
    cm.saveBtn.title = isRowEditing ? 'Save Row' : 'Apply All Changes';
  }
  if (cm.cancelBtn) {
    cm.cancelBtn.disabled = isRowEditing ? !rowDirty : !popupDirty;
    cm.cancelBtn.classList.toggle('disabled', cm.cancelBtn.disabled);
    cm.cancelBtn.title = isRowEditing ? 'Cancel Row Changes' : 'Discard All Changes';
  }
}

/**
 * Recompute popup dirty state from current table rows.
 * @param {Object} cm - ColumnManager instance
 */
export function syncDirtyState(cm) {
  const previousDirty = cm.isDirty;
  cm.isDirty = hasPendingChanges(cm);
  updateButtonState(cm);

  if (previousDirty !== cm.isDirty) {
    log(Subsystems.TABLE, Status.DEBUG,
      `[ColumnManager] Dirty state: ${cm.isDirty ? 'dirty' : 'clean'}`);
  }
}

/**
 * Clear dirty state
 * @param {Object} cm - ColumnManager instance
 */
export function clearDirty(cm) {
  cm.isDirty = false;
  updateButtonState(cm);
  log(Subsystems.TABLE, Status.DEBUG, '[ColumnManager] Cleared dirty state');
}

/**
 * Check whether the popup state differs from the last applied baseline.
 * @param {Object} cm - ColumnManager instance
 * @returns {boolean} True if there are pending changes
 */
export function hasPendingChanges(cm) {
  const currentRows = normalizeColumnRows(getCurrentColumnTableData(cm));
  const baselineRows = normalizeColumnRows(cm.columnData || []);
  return JSON.stringify(currentRows) !== JSON.stringify(baselineRows);
}

/**
 * Normalize column rows for comparison
 * @param {Array} rows - Column rows
 * @returns {Array} Normalized rows
 */
export function normalizeColumnRows(rows = []) {
  return rows.map((row, index) => ({
    order: Number.isFinite(Number(row.order)) ? Number(row.order) : index + 1,
    visible: row.visible !== false,
    editable: row.editable !== false,
    field_name: row.field_name || row.field || '',
    column_name: row.column_name || '',
    format: row.format || 'string',
    summary: row.summary || 'none',
    alignment: row.alignment || 'left',
    width: row.width == null || row.width === '' ? null : Number(row.width),
    category: row.category || 'Default',
    field: row.field || row.field_name || '',
  }));
}

/**
 * Get current column table data
 * @param {Object} cm - ColumnManager instance
 * @returns {Array} Current data
 */
export function getCurrentColumnTableData(cm) {
  if (!cm.columnTable?.table) return cm.columnData || [];
  return cm.columnTable.table.getRows().map((row) => ({ ...row.getData() }));
}

/**
 * Save setting to localStorage
 * @param {Object} cm - ColumnManager instance
 * @param {string} key - Setting key
 * @param {*} value - Setting value
 */
export function saveSetting(cm, key, value) {
  try {
    localStorage.setItem(`${cm.storageKey}_${key}`, JSON.stringify(value));
  } catch (_e) {
    // Ignore storage errors
  }
}

/**
 * Restore setting from localStorage
 * @param {Object} cm - ColumnManager instance
 * @param {string} key - Setting key
 * @param {*} defaultValue - Default value if not found
 * @returns {*} Restored value or default
 */
export function restoreSetting(cm, key, defaultValue = null) {
  try {
    const stored = localStorage.getItem(`${cm.storageKey}_${key}`);
    if (stored !== null) {
      return JSON.parse(stored);
    }
  } catch (_e) {
    // Ignore storage errors
  }
  return defaultValue;
}

/**
 * Handle primary save action
 * @param {Object} cm - ColumnManager instance
 */
export async function handlePrimarySave(cm) {
  if (cm.columnTable?.isEditing) {
    if (!cm.columnTable.isActuallyDirty?.()) return;
    await cm.columnTable.handleSave();
    syncDirtyState(cm);
    return;
  }

  if (!cm.isDirty) return;
  await cm.applyAllChangesToParent();
  cm.close();
}

/**
 * Handle primary cancel action
 * @param {Object} cm - ColumnManager instance
 */
export async function handlePrimaryCancel(cm) {
  if (cm.columnTable?.isEditing) {
    if (!cm.columnTable.isActuallyDirty?.()) return;
    await cm.columnTable.handleCancel();
    syncDirtyState(cm);
    return;
  }

  if (!cm.isDirty) return;
  await cm.discardAllChanges();
  cm.close();
}
