/**
 * Persistence Module
 *
 * State persistence for table settings and selection.
 *
 * @module tables/persistence/persistence
 */

import { log, Subsystems, Status } from '../../core/log.js';

/**
 * Save selected row ID to localStorage
 * @param {Object} table - LithiumTable instance
 * @param {string|number} rowId - Row ID
 */
export function saveSelectedRowId(table, rowId) {
  if (rowId == null) return;
  try {
    localStorage.setItem(`${table.storageKey}_selected_row`, String(rowId));
  } catch (_e) {
    // Ignore storage errors
  }
}

/**
 * Restore selected row ID from localStorage
 * @param {Object} table - LithiumTable instance
 * @returns {string|null} Row ID or null
 */
export function restoreSelectedRowId(table) {
  try {
    const stored = localStorage.getItem(`${table.storageKey}_selected_row`);
    return stored ?? null;
  } catch (_e) {
    // Ignore storage errors
  }
  return null;
}

/**
 * Clear saved row selection
 * @param {Object} table - LithiumTable instance
 */
export function clearSavedRowSelection(table) {
  try {
    localStorage.removeItem(`${table.storageKey}_selected_row`);
  } catch (_e) {
    // Ignore storage errors
  }
}

/**
 * Save filters visible state
 * @param {Object} table - LithiumTable instance
 * @param {boolean} visible - Filters visible
 */
export function saveFiltersVisible(table, visible) {
  try {
    localStorage.setItem(`${table.storageKey}_filters_visible`, visible ? '1' : '0');
  } catch (_e) {
    // Ignore storage errors
  }
}

/**
 * Restore filters visible state
 * @param {Object} table - LithiumTable instance
 * @returns {boolean} Filters visible
 */
export function restoreFiltersVisible(table) {
  try {
    const stored = localStorage.getItem(`${table.storageKey}_filters_visible`);
    return stored === '1';
  } catch (_e) {
    // Ignore storage errors
  }
  return false;
}
