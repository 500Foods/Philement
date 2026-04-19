/**
 * Navigation Module
 *
 * Table navigation methods - First, Last, Previous, Next, Page Up, Page Down.
 *
 * @module tables/navigation/navigation
 */

/**
 * Helper shared by all navigation methods: if editing, auto-save when
 * dirty or exit when clean.
 * @param {Object} table - LithiumTable instance
 * @returns {boolean} true if navigation may proceed, false if blocked
 */
export async function exitEditModeForNavigation(table) {
  if (!table.isEditing) return true;

  const isDirty = table.isActuallyDirty?.();
  console.log(`[Navigation] exitEditModeForNavigation: isEditing=${table.isEditing}, isActuallyDirty=${isDirty}, isDirty=${table.isDirty}`);

  if (isDirty) {
    console.log('[Navigation] Detected dirty, calling autoSaveBeforeRowChange');
    const saveSucceeded = await table.autoSaveBeforeRowChange?.();
    if (!saveSucceeded) return false;
    await table.exitEditMode?.('save');
  } else {
    console.log('[Navigation] Not dirty, exiting with cancel');
    await table.exitEditMode?.('cancel');
  }
  return true;
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
 * Select row by index
 * @param {Object} table - LithiumTable instance
 * @param {number} index - Row index
 */
export function selectRowByIndex(table, index) {
  if (!table.table) return;
  const rows = table.table.getRows('active');
  if (index < 0 || index >= rows.length) return;
  table.table.deselectRow();
  rows[index].select();
  rows[index].scrollTo();
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
  selectRowByIndex,
  getVisibleRowsAndIndex,
};
