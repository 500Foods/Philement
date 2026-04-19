/**
 * Table Definition Validator
 *
 * Validates tableDef structures against canonical property lists.
 *
 * @module tables/resolution/validator
 */

import { log, Subsystems, Status } from '../../core/log.js';

/** @type {Set<string>} Valid table-level properties */
export const TABLEDEF_VALID_PROPS = new Set([
  // Core identification
  'title', 'table', 'columns',
  // JSON Schema metadata
  '$schema', '$version', '$description',
  // Query references
  'queryRef', 'searchQueryRef', 'detailQueryRef', 'insertQueryRef', 'updateQueryRef', 'deleteQueryRef',
  // Layout and behavior
  'layout', 'responsiveLayout', 'selectableRows',
  'groupBy', 'groupStartOpen', 'groupToggleElement', 'columnCalcs',
  'pagination', 'paginationSize', 'persistSort', 'persistFilter',
  'movableRows', 'movableColumns', 'resizableColumns',
  'readonly', 'initialSort',
  // Internal/runtime properties (underscore prefixed)
  '_autoDiscover', '_templateMeta', '_filtersVisible', '_filterValues',
]);

/** @type {Set<string>} Valid column-level properties */
export const COLUMN_VALID_PROPS = new Set([
  'field', 'title', 'coltype', 'description', 'visible',
  'width', 'minWidth', 'maxWidth', 'hozAlign', 'vertAlign',
  'formatter', 'formatterParams', 'editor', 'editorParams',
  'sorter', 'sorterParams', 'headerSort', 'headerFilter', 'headerFilterFunc',
  'headerFilterPlaceholder', 'headerFilterParams', 'headerVisible',
  'headerHozAlign', 'headerVertical', 'headerSortTristate',
  'resizable', 'frozen', 'clipboard', 'cssClass', 'rowCssClass',
  'vertHandle', 'hozHandle', 'contextMenu', 'tapComprehensive',
  'tooltip', 'tooltipHeader', 'bottomCalc', 'bottomCalcParams',
  'bottomCalcFormatter', 'bottomCalcFormatterParams',
  'columnPri', 'primaryKey', 'calculated', 'sort', 'filter', 'group', 'editable',
  'lookupRef', 'lookupFixed', 'blank', 'zero',
]);

/** @type {Set<string>} Valid coltype names */
export const VALID_COLTYPES = new Set([
  'default', 'string', 'text', 'integer', 'index', 'decimal',
  'boolean', 'date', 'datetime', 'time', 'currency', 'percent',
  'progress', 'email', 'url', 'lookup', 'enum', 'html',
  'image', 'color', 'star', 'rownum', 'json',
]);

/**
 * Validate a tableDef against canonical property lists.
 *
 * @param {Object} tableDef - The table definition to validate
 * @param {string} [stageName='unknown'] - Identifier for the validation context
 * @returns {{valid: boolean, errors: string[]}} Validation result
 */
export function validateTableDef(tableDef, stageName = 'unknown') {
  if (!tableDef) {
    return { valid: true, errors: [] };
  }

  const errors = [];

  if (typeof tableDef !== 'object') {
    errors.push(`Stage ${stageName}: tableDef must be an object`);
    return { valid: false, errors };
  }

  const tableProps = Object.keys(tableDef);
  for (const prop of tableProps) {
    if (!TABLEDEF_VALID_PROPS.has(prop) && !prop.startsWith('_')) {
      errors.push(`Stage ${stageName}: Unknown table property "${prop}"`);
    }
  }

  const columns = tableDef.columns;
  if (columns && typeof columns === 'object') {
    for (const [field, colDef] of Object.entries(columns)) {
      if (!colDef || typeof colDef !== 'object') {
        errors.push(`Stage ${stageName}: Column "${field}" must be an object`);
        continue;
      }

      const colProps = Object.keys(colDef);
      for (const prop of colProps) {
        if (!COLUMN_VALID_PROPS.has(prop) && !prop.startsWith('_')) {
          errors.push(`Stage ${stageName}: Column "${field}" has unknown property "${prop}"`);
        }
      }

      if (colDef.coltype && !VALID_COLTYPES.has(colDef.coltype)) {
        errors.push(`Stage ${stageName}: Column "${field}" has invalid coltype "${colDef.coltype}"`);
      }
    }
  }

  return { valid: errors.length === 0, errors };
}
