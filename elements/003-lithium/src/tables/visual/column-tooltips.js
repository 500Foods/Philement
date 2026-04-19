/**
 * Column Header Tooltips Module
 *
 * FloatingUI tooltip initialization for column headers.
 *
 * @module tables/visual/column-tooltips
 */

import { tip, getTip } from '../../core/tooltip-api.js';

/**
 * Initialize FloatingUI tooltips on column headers.
 * Called after the table is built to attach tooltips to all column header elements.
 * Safe to call multiple times - will skip headers that already have tooltips.
 *
 * @param {Object} table - LithiumTable instance
 */
export function initColumnHeaderTooltips(table) {
  if (!table.table) return;

  // Find all column header title elements (the actual text container)
  const titleEls = table.container.querySelectorAll('.tabulator-col-title');

  titleEls.forEach((titleEl) => {
    // Skip if this element already has a tooltip
    if (getTip(titleEl)) return;

    // Find the parent column element to get the field name
    const headerEl = titleEl.closest('.tabulator-col');
    if (!headerEl) return;

    const field = headerEl.getAttribute('tabulator-field');
    if (!field || field === '_selector') return;

    // Find the column definition from tableDef
    let colDef = table.getColumnDefinition?.(field);

    // If not in tableDef (e.g., discovered column), create a minimal definition
    if (!colDef) {
      const title = titleEl.textContent?.trim() || field;
      colDef = {
        field: field,
        title: title,
        description: null,
        calculated: false,
      };
    }

    // Build tooltip content
    const tooltipContent = buildColumnTooltipContent(colDef);
    if (!tooltipContent) return;

    // Attach FloatingUI tooltip to the title element with shorter delay
    tip(titleEl, tooltipContent, {
      placement: 'top',
      maxWidth: 400,
      delay: { show: 600, hide: 100 },
    });
  });
}

/**
 * Build tooltip content for a column based on the business rules:
 * 1. If description and field exist: show description + newline + field (styled)
 * 2. If no description: use column name (title)
 * 3. If calculated: add asterisk before field name
 * 4. If no field: use "*auto"
 *
 * @param {Object} colDef - The column definition
 * @returns {string} HTML content for the tooltip
 */
export function buildColumnTooltipContent(colDef) {
  const description = colDef.description || null;
  const title = colDef.title || colDef.field || '';
  const field = colDef.field || null;
  const isCalculated = colDef.calculated === true;

  // Rule 4: If no field name available (or null), use "*auto"
  if (!field) {
    return '<span class="li-tip-field">*auto</span>';
  }

  // Rule 3: If calculated, add asterisk before field name
  const fieldDisplay = isCalculated ? `*${field}` : field;

  // Rule 1: If we have both description and field, show both
  if (description && field) {
    return `${escapeHtml(description)}<br><span class="li-tip-field">${escapeHtml(fieldDisplay)}</span>`;
  }

  // Rule 2: If no description, use column name (title)
  if (!description) {
    return `${escapeHtml(title)}<br><span class="li-tip-field">${escapeHtml(fieldDisplay)}</span>`;
  }

  // Fallback: just return description with field
  return `${escapeHtml(description)}<br><span class="li-tip-field">${escapeHtml(fieldDisplay)}</span>`;
}

/**
 * Escape HTML special characters to prevent XSS.
 * @param {string} text - The text to escape
 * @returns {string} Escaped text
 */
export function escapeHtml(text) {
  if (!text) return '';
  const div = document.createElement('div');
  div.textContent = text;
  return div.innerHTML;
}

export default { initColumnHeaderTooltips, buildColumnTooltipContent, escapeHtml };
