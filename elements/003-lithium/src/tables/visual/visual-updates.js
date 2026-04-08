/**
 * Visual Updates Module
 *
 * Visual state updates for table cells and columns.
 *
 * @module tables/visual/visual-updates
 */

/**
 * Update selector cell indicator
 * @param {Object} table - LithiumTable instance
 * @param {Object} row - Tabulator row
 * @param {boolean} isSelected - Whether row is selected
 */
export function updateSelectorCell(table, row, isSelected) {
  try {
    const cell = row.getCell('_selector');
    if (!cell) return;
    const el = cell.getElement();
    if (!el) return;

    if (isSelected) {
      const pkField = table.primaryKeyField || 'id';
      const isEditing = table.isEditing && table.editingRowId === row.getData()[pkField];
      el.innerHTML = isEditing
        ? `<span class="${table.cssPrefix}-selector-indicator ${table.cssPrefix}-selector-edit">&#10073;</span>`
        : `<span class="${table.cssPrefix}-selector-indicator ${table.cssPrefix}-selector-active">&#9658;</span>`;
    } else {
      el.innerHTML = '';
    }
  } catch (_e) {
    // Silently handle
  }
}

/**
 * Update visible column boundary classes
 * @param {Object} table - LithiumTable instance
 */
export function updateVisibleColumnClasses(table) {
  if (!table.table || !table.container) return;

  const { firstField, lastField } = getVisibleColumnBoundaries(table);
  if (!firstField || !lastField) return;

  // Update header cells
  const headerCols = table.container.querySelectorAll(
    '.tabulator-header .tabulator-col[tabulator-field]'
  );
  headerCols.forEach((colEl) => {
    const field = colEl.getAttribute('tabulator-field');
    if (!field) return;
    colEl.classList.toggle('first-visible-col', field === firstField);
    colEl.classList.toggle('last-visible-col', field === lastField);
  });

  // Update body rows
  const bodyRows = table.container.querySelectorAll(
    '.tabulator-tableholder .tabulator-row:not(.tabulator-calcs):not(.tabulator-group)'
  );
  bodyRows.forEach((rowEl) => {
    applyVisibleColumnClassesToRowElement(rowEl, firstField, lastField);
  });
}

/**
 * Get first and last visible column boundaries
 * @param {Object} table - LithiumTable instance
 * @returns {Object} { first, last, firstField, lastField }
 */
export function getVisibleColumnBoundaries(table) {
  if (!table.table) {
    return { first: null, last: null, firstField: null, lastField: null };
  }

  const allColumns = table.table.getColumns();
  const visibleColumns = allColumns.filter((col) => {
    const field = col.getField?.();
    return col.isVisible() && field !== '_selector';
  });

  if (visibleColumns.length === 0) {
    return { first: null, last: null, firstField: null, lastField: null };
  }

  return {
    first: visibleColumns[0],
    last: visibleColumns[visibleColumns.length - 1],
    firstField: visibleColumns[0].getField(),
    lastField: visibleColumns[visibleColumns.length - 1].getField(),
  };
}

/**
 * Apply visible column classes to a row
 * @param {Object} table - LithiumTable instance
 * @param {Object} row - Tabulator row
 * @param {string} firstField - First visible field
 * @param {string} lastField - Last visible field
 */
export function applyVisibleColumnClassesToRow(table, row, firstField, lastField) {
  if (!row || row.getCells === undefined) return;

  if (firstField === undefined || lastField === undefined) {
    const boundaries = getVisibleColumnBoundaries(table);
    firstField = boundaries.firstField;
    lastField = boundaries.lastField;
  }

  if (!firstField || !lastField) return;

  const cells = row.getCells();
  cells.forEach((cell) => {
    const cellEl = cell.getElement?.();
    if (!cellEl) return;

    const cellField = cell.getField?.();
    if (!cellField) return;

    cellEl.classList.toggle('first-visible-col', cellField === firstField);
    cellEl.classList.toggle('last-visible-col', cellField === lastField);
  });
}

/**
 * Apply visible column classes to a row element
 * @param {HTMLElement} rowEl - Row element
 * @param {string} firstField - First visible field
 * @param {string} lastField - Last visible field
 */
export function applyVisibleColumnClassesToRowElement(rowEl, firstField, lastField) {
  if (!rowEl) return;

  const cells = rowEl.querySelectorAll('.tabulator-cell');
  cells.forEach((cell) => {
    const field = cell.getAttribute('tabulator-field');
    if (!field) return;

    cell.classList.toggle('first-visible-col', field === firstField);
    cell.classList.toggle('last-visible-col', field === lastField);
  });
}
