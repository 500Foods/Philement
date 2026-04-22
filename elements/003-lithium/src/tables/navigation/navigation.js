/**
 * Navigation Module
 *
 * Table navigation methods - First, Last, Previous, Next, Page Up, Page Down.
 *
 * @module tables/navigation/navigation
 */

import { log, Subsystems, Status } from '../../core/log.js';

/**
 * Helper shared by all navigation methods: if editing, auto-save when
 * dirty or exit when clean.
 * @param {Object} table - LithiumTable instance
 * @returns {boolean} true if navigation may proceed, false if blocked
 */
export async function exitEditModeForNavigation(table) {
  if (!table.isEditing) return true;

  const isDirty = table.isActuallyDirty?.();

  if (isDirty) {
    const saveSucceeded = await table.autoSaveBeforeRowChange?.();
    if (!saveSucceeded) return false;
    await table.exitEditMode?.('save');
  } else {
    await table.exitEditMode?.('cancel');
  }
  return true;
}

/**
 * Scroll a row into view using Tabulator's table-level API.
 * This is more reliable than row.scrollTo() for grouped tables and far jumps.
 * @param {Object} table - LithiumTable instance
 * @param {Object} row - Tabulator row component
 */
export async function scrollRowIntoView(table, row) {
  if (!table.table || !row) return;

  try {
    await expandRowGroups(row);
    await waitForNextFrame();

    if (typeof table.table.scrollToRow === 'function') {
      await table.table.scrollToRow(row, 'center', false);
    } else {
      await row.scrollTo?.('center', false);
    }

    await waitForNextFrame();

    if (isRowFullyVisible(table, row)) return;

    scrollRowElementIntoView(table, row, 'center');
    await waitForNextFrame();

    if (isRowFullyVisible(table, row)) return;

    const group = row.getGroup?.();
    await group?.scrollTo?.('top', true);
    await waitForNextFrame();

    scrollRowElementIntoView(table, row, 'center');
  } catch (error) {
    log(
      Subsystems.TABLE,
      Status.DEBUG,
      `[${table.cssPrefix}] Failed to scroll row into view during navigation: ${error.message}`,
    );
  }
}

/**
 * Expand the target row's parent group chain so the row can be rendered and
 * scrolled into view.
 * @param {Object} row - Tabulator row component
 */
export async function expandRowGroups(row) {
  const groups = [];
  let group = row?.getGroup?.();

  while (group) {
    groups.unshift(group);
    group = group.getParentGroup?.() || null;
  }

  for (const currentGroup of groups) {
    if (currentGroup?.isVisible?.()) continue;
    currentGroup?.show?.();
  }
}

/**
 * Wait one animation frame so Tabulator can render rows after group changes.
 * @returns {Promise<void>}
 */
export function waitForNextFrame() {
  return new Promise(resolve => requestAnimationFrame(() => resolve()));
}

/**
 * Get the Tabulator vertical scroll container.
 * @param {Object} table - LithiumTable instance
 * @returns {HTMLElement|null}
 */
export function getTableHolder(table) {
  return table.container?.querySelector?.('.tabulator-tableholder') || null;
}

/**
 * Check whether a row is fully visible inside the Tabulator holder.
 * @param {Object} table - LithiumTable instance
 * @param {Object} row - Tabulator row component
 * @returns {boolean}
 */
export function isRowFullyVisible(table, row) {
  const holder = getTableHolder(table);
  const rowEl = row?.getElement?.();
  if (!holder || !rowEl || !holder.contains(rowEl)) return false;

  const holderTop = holder.scrollTop;
  const holderBottom = holderTop + holder.clientHeight;
  const rowTop = rowEl.offsetTop;
  const rowBottom = rowTop + rowEl.offsetHeight;

  return rowTop >= holderTop && rowBottom <= holderBottom;
}

/**
 * Directly adjust the holder scroll position to place a rendered row in view.
 * This works around grouped virtual-DOM cases where Tabulator's scroll helpers
 * select the correct row but leave it outside the viewport.
 * @param {Object} table - LithiumTable instance
 * @param {Object} row - Tabulator row component
 * @param {string} position - top, center, or bottom
 */
export function scrollRowElementIntoView(table, row, position = 'center') {
  const holder = getTableHolder(table);
  const rowEl = row?.getElement?.();
  if (!holder || !rowEl || !holder.contains(rowEl)) return;

  const maxScrollTop = Math.max(0, holder.scrollHeight - holder.clientHeight);
  const rowTop = rowEl.offsetTop;
  const rowHeight = rowEl.offsetHeight;

  let nextScrollTop = rowTop;

  if (position === 'bottom') {
    nextScrollTop = rowTop - holder.clientHeight + rowHeight;
  } else if (position === 'center' || position === 'middle') {
    nextScrollTop = rowTop - ((holder.clientHeight - rowHeight) / 2);
  }

  holder.scrollTop = Math.min(maxScrollTop, Math.max(0, nextScrollTop));
}

/**
 * Navigate to first record
 * @param {Object} table - LithiumTable instance
 */
export async function navigateFirst(table) {
  if (!table.table) return;
  if (!(await exitEditModeForNavigation(table))) return;
  table.selectRowByIndex?.(0);
}

/**
 * Navigate to last record
 * @param {Object} table - LithiumTable instance
 */
export async function navigateLast(table) {
  if (!table.table) return;
  if (!(await exitEditModeForNavigation(table))) return;
  const rows = table.table?.getRows('active') || [];
  if (rows.length > 0) {
    table.selectRowByIndex?.(rows.length - 1);
  }
}

/**
 * Navigate to previous record
 * @param {Object} table - LithiumTable instance
 */
export async function navigatePrevRec(table) {
  if (!(await exitEditModeForNavigation(table))) return;
  const { selectedIndex } = table.getVisibleRowsAndIndex?.() || { selectedIndex: -1 };
  if (selectedIndex > 0) {
    table.selectRowByIndex?.(selectedIndex - 1);
  }
}

/**
 * Navigate to next record
 * @param {Object} table - LithiumTable instance
 */
export async function navigateNextRec(table) {
  if (!(await exitEditModeForNavigation(table))) return;
  const { rows, selectedIndex } = table.getVisibleRowsAndIndex?.() || { rows: [], selectedIndex: -1 };
  if (selectedIndex < rows.length - 1) {
    table.selectRowByIndex?.(selectedIndex + 1);
  }
}

/**
 * Navigate to previous page
 * @param {Object} table - LithiumTable instance
 */
export async function navigatePrevPage(table) {
  if (!(await exitEditModeForNavigation(table))) return;
  const { selectedIndex } = table.getVisibleRowsAndIndex?.() || { selectedIndex: -1 };
  const pageSize = getPageSize(table);
  const newIndex = Math.max(0, selectedIndex - pageSize);
  table.selectRowByIndex?.(newIndex);
}

/**
 * Navigate to next page
 * @param {Object} table - LithiumTable instance
 */
export async function navigateNextPage(table) {
  if (!(await exitEditModeForNavigation(table))) return;
  const { rows, selectedIndex } = table.getVisibleRowsAndIndex?.() || { rows: [], selectedIndex: -1 };
  const pageSize = getPageSize(table);
  const newIndex = Math.min(rows.length - 1, selectedIndex + pageSize);
  table.selectRowByIndex?.(newIndex);
}

/**
 * Get the page size based on visible row count
 * @param {Object} table - LithiumTable instance
 * @returns {number} Number of rows per page
 */
export function getPageSize(table) {
  if (!table.table) return 10;
  const holder = table.container?.querySelector('.tabulator-tableholder');
  if (!holder) return 10;
  const rowEl = holder.querySelector('.tabulator-row');
  if (!rowEl) return 10;
  const rowHeight = rowEl.offsetHeight || 30;
  const visibleHeight = holder.clientHeight;
  return Math.max(1, Math.floor(visibleHeight / rowHeight));
}

/**
 * Select row by index and scroll it into view
 * @param {Object} table - LithiumTable instance
 * @param {number} index - Row index
 */
export function selectRowByIndex(table, index) {
  if (!table.table) return;
  const rows = table.table.getRows('active');
  if (index < 0 || index >= rows.length) return;

  const targetRow = rows[index];
  table.table.deselectRow();
  targetRow.select();

  // Defer scroll to ensure Tabulator has rendered the row,
  // especially important for grouped tables and far jumps (first/last/page)
  requestAnimationFrame(() => {
    void scrollRowIntoView(table, targetRow);
  });
}

/**
 * Get visible rows and selected index
 * @param {Object} table - LithiumTable instance
 * @returns {Object} { rows, selectedIndex }
 */
export function getVisibleRowsAndIndex(table) {
  if (!table.table) return { rows: [], selectedIndex: -1 };
  const rows = table.table.getRows('active');
  const selected = table.table.getSelectedRows();
  let selectedIndex = -1;
  if (selected.length > 0) {
    const selPos = selected[0].getPosition(true);
    selectedIndex = selPos - 1;
  }
  return { rows, selectedIndex };
}

export default {
  exitEditModeForNavigation,
  navigateFirst,
  navigateLast,
  navigatePrevRec,
  navigateNextRec,
  navigatePrevPage,
  navigateNextPage,
  getPageSize,
  expandRowGroups,
  getTableHolder,
  isRowFullyVisible,
  scrollRowIntoView,
  scrollRowElementIntoView,
  selectRowByIndex,
  getVisibleRowsAndIndex,
  waitForNextFrame,
};
