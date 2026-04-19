/**
 * Column Management Module
 *
 * Column discovery, building, merging, and management.
 *
 * @module tables/columns/column-management
 */

import { resolveColumn, resolveColumns } from '../lithium-table.js';
import { sanitizeColumnTitle } from '../lithium-table-base.js';

// ── Deep Merge Configuration ────────────────────────────────────────────────

/**
 * Column parameter keys that should be deep-merged across stages.
 * These are nested objects where properties from multiple sources
 * (coltype, tableDef, template) should be combined rather than replaced.
 */
export const DEEP_MERGE_PARAM_KEYS = [
  // Display & editing
  'formatterParams',
  'editorParams',
  'headerFilterParams',
  'sorterParams',
  'accessorParams',
  'mutatorParams',

  // Calculations
  'bottomCalcFormatterParams',

  // Import/export
  'downloadFormatterParams',
  'downloadCalcParams',
  'clipboardParams',
];

/**
 * Deep merge nested param objects across stage overlays.
 * Scalar properties and arrays continue to replace (not merge).
 *
 * @param {Object} base - Base column definition (e.g., from auto-discovery)
 * @param {Object} overlay - Overlay column definition (e.g., from tableDef)
 * @param {Array<string>} [paramKeys] - Keys to deep-merge (defaults to DEEP_MERGE_PARAM_KEYS)
 * @returns {Object} Merged column definition
 */
export function deepMergeParams(base, overlay, paramKeys = DEEP_MERGE_PARAM_KEYS) {
  if (!overlay || typeof overlay !== 'object') {
    return base;
  }

  const result = { ...base };

  for (const key of Object.keys(overlay)) {
    const overlayValue = overlay[key];

    // Check if this key should be deep-merged
    if (paramKeys.includes(key) &&
        overlayValue &&
        typeof overlayValue === 'object' &&
        !Array.isArray(overlayValue)) {
      // Deep merge: combine properties from both base and overlay
      const baseValue = base?.[key];
      if (baseValue && typeof baseValue === 'object' && !Array.isArray(baseValue)) {
        result[key] = { ...baseValue, ...overlayValue };
      } else {
        // No base value or base is scalar/array, use overlay
        result[key] = { ...overlayValue };
      }
    } else {
      // Scalar or array: replace
      result[key] = overlayValue;
    }
  }

  return result;
}

/**
 * Discover columns from data rows and add them to the table
 * @param {Object} table - LithiumTable instance
 * @param {Array} rows - Data rows
 */
export function discoverColumns(table, rows) {
  if (!table.table || !rows || rows.length === 0) return;

  const allKeys = [];
  const keyIndex = new Map();
  for (const row of rows) {
    for (const key of Object.keys(row)) {
      if (key !== '_selector' && !keyIndex.has(key)) {
        keyIndex.set(key, allKeys.length);
        allKeys.push(key);
      }
    }
  }

  const existingFields = new Set(
    table.table.getColumns().map(col => col.getField()).filter(Boolean)
  );

  const filterEditorFn = table.createFilterEditorFunction?.();
  for (const key of allKeys) {
    if (existingFields.has(key)) continue;

    const title = key
      .replace(/_/g, ' ')
      .replace(/\b\w/g, c => c.toUpperCase());

    const inferredColtype = detectColtype(rows, key);
    const columnPri = keyIndex.get(key) + 1;

    const colDef = {
      field: key,
      title: title,
      coltype: inferredColtype,
      columnPri,
    };

    const resolvedCol = resolveColumn(key, colDef, table.coltypes || {}, {
      filterEditor: filterEditorFn,
    });

    table.table.addColumn({
      ...resolvedCol,
      headerFilterFunc: resolvedCol.headerFilter ? 'like' : undefined,
      visible: false,
    });
  }

  table.initColumnHeaderTooltips?.();
}

/**
 * Detect column type from sample data
 * @param {Array} rows - Data rows
 * @param {string} field - Field name
 * @returns {string} Detected coltype ('string', 'integer', 'decimal', 'boolean')
 */
export function detectColtype(rows, field) {
  let hasNumber = false;
  let hasDecimal = false;
  let hasBoolean = false;
  let hasString = false;
  let hasNull = false;

  const sampleSize = Math.min(rows.length, 50);
  for (let i = 0; i < sampleSize; i++) {
    const value = rows[i][field];
    const type = typeof value;

    if (value === null || value === undefined) {
      hasNull = true;
    } else if (type === 'number') {
      hasNumber = true;
      if (value !== Math.floor(value)) {
        hasDecimal = true;
      }
    } else if (type === 'boolean') {
      hasBoolean = true;
    } else if (type === 'string') {
      hasString = true;
      if (value !== '') {
        const num = parseFloat(value);
        if (!isNaN(num) && isFinite(num)) {
          hasNumber = true;
          if (String(num) !== value.trim()) {
            hasDecimal = true;
          }
        }
      }
    }
  }

  if (hasBoolean && !hasNumber && !hasString) return 'boolean';
  if (hasDecimal) return 'decimal';
  if (hasNumber) return 'integer';
  if (hasString || hasNull) return 'string';
  return 'string';
}

/**
 * Build columns from data
 * @param {Object} table - LithiumTable instance
 * @param {Array} data - Data rows
 * @returns {Array} Column definitions
 */
export function buildColumnsFromData(table, data) {
  if (!data || data.length === 0) return [];

  const fields = [];
  const fieldIndex = new Map();
  for (const row of data) {
    for (const key of Object.keys(row)) {
      if (key !== '_selector' && !fieldIndex.has(key)) {
        fieldIndex.set(key, fields.length);
        fields.push(key);
      }
    }
  }

  const filterEditorFn = table.createFilterEditorFunction?.();
  const columns = [];

  for (const field of fields) {
    const title = field
      .replace(/_/g, ' ')
      .replace(/\b\w/g, c => c.toUpperCase());

    const inferredColtype = detectColtype(data, field);
    const columnPri = fieldIndex.get(field) + 1;

    const colDef = {
      field,
      title: title,
      coltype: inferredColtype,
      columnPri,
    };

    const resolvedCol = resolveColumn(field, colDef, table.coltypes || {}, {
      filterEditor: filterEditorFn,
    });

    columns.push(resolvedCol);
  }

  return columns;
}

/**
 * Sort columns by priority
 * @param {Array} columns - Column definitions
 * @returns {Array} Sorted columns
 */
export function sortColumnsByPriority(columns) {
  return [...columns].sort((a, b) => {
    const priA = a.columnPri ?? Infinity;
    const priB = b.columnPri ?? Infinity;
    return priA - priB;
  });
}

/**
 * Merge auto-discovered columns with tableDef overlays
 * @param {Object} table - LithiumTable instance
 * @param {Array} autoColumns - Auto-discovered columns
 * @param {Array} tableDefColumns - TableDef columns
 * @returns {Array} Merged columns
 */
export function mergeColumnsWithTableDef(table, autoColumns, tableDefColumns) {
  // Create a map of tableDef columns by field name for quick lookup
  const tableDefMap = new Map();
  for (const col of tableDefColumns) {
    if (col.field) {
      tableDefMap.set(col.field, col);
    }
  }

  // Merge: start with auto-discovered columns, overlay tableDef properties
  const mergedColumns = autoColumns.map(autoCol => {
    const tableDefCol = tableDefMap.get(autoCol.field);
    if (tableDefCol) {
      // Apply tableDef properties over auto-discovered properties
      // Use deep merge for param objects to preserve coltype defaults
      return deepMergeParams(autoCol, tableDefCol);
    }
    return autoCol;
  });

  // Add any tableDef columns that weren't in the auto-discovered set
  for (const tableDefCol of tableDefColumns) {
    if (!mergedColumns.find(col => col.field === tableDefCol.field)) {
      mergedColumns.push(tableDefCol);
    }
  }

  return mergedColumns;
}

/**
 * Apply edit mode gate to columns
 * @param {Object} table - LithiumTable instance
 * @param {Array} columns - Column definitions
 */
export function applyEditModeGate(table, columns) {
  table.columnEditors = new Map();

  for (const col of columns) {
    if (!col.editor) continue;

    table.columnEditors.set(col.field, {
      editor: col.editor,
      editorParams: col.editorParams || undefined,
    });

    // In alwaysEditable mode, cells are directly editable without entering edit mode
    if (table.alwaysEditable) {
      col.editable = (cell) => {
        const row = cell.getRow();
        return row && !table.isCalcRow?.(row);
      };
      continue;
    }

    col.editable = (cell) => {
      if (!table.isEditing) return false;

      const pkFields = table.primaryKeyField;
      const row = cell.getRow();
      if (!row || table.isCalcRow?.(row)) return false;

      const rowId = getCompositeRowId(row.getData?.(), pkFields);
      return table.editingRowId != null
        ? rowId === table.editingRowId
        : row.isSelected?.();
    };
  }
}

/**
 * Get composite row ID from row data using primary key fields
 * (duplicate from data-loading.js to avoid circular dependency)
 * @param {Object} rowData - Row data object
 * @param {string[]} pkFields - Array of primary key field names
 * @returns {string} Composite ID
 */
function getCompositeRowId(rowData, pkFields) {
  if (!pkFields || pkFields.length === 0) {
    const fallbackFields = ['id', 'query_id', 'lookup_id', 'key_idx', 'style_id', 'version_id'];
    for (const field of fallbackFields) {
      if (rowData[field] != null) {
        return String(rowData[field]);
      }
    }
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
 * Build selector column definition
 * @param {Object} table - LithiumTable instance
 * @returns {Object} Selector column definition
 */
export function buildSelectorColumn(table) {
  return {
    title: '',
    field: '_selector',
    frozen: true,
    width: 16,
    minWidth: 16,
    maxWidth: 16,
    resizable: false,
    headerSort: false,
    hozAlign: 'center',
    vertAlign: 'middle',
    cssClass: `${table.cssPrefix}-selector-col`,
    titleFormatter: () => {
      const wrapper = document.createElement('span');
      wrapper.className = `${table.cssPrefix}-col-chooser-btn`;
      wrapper.title = 'Column Manager';
      wrapper.innerHTML = '<fa fa-ellipsis-stroke-vertical>';
      return wrapper;
    },
    formatter: (cell) => {
      const row = cell.getRow();
      const pkFields = table.primaryKeyField;
      const rowData = row.getData?.() || {};
      const rowId = getCompositeRowId(rowData, pkFields);
      if (row.isSelected()) {
        if (table.isEditing && table.editingRowId === rowId) {
          return `<span class="${table.cssPrefix}-selector-indicator ${table.cssPrefix}-selector-edit">&#10073;</span>`;
        }
        return `<span class="${table.cssPrefix}-selector-indicator ${table.cssPrefix}-selector-active">&#9658;</span>`;
      }
      return '';
    },
    headerClick: (e, column) => {
      table.toggleColumnManager?.(e, column);
    },
    cellClick: (e, cell) => {
      e?.stopPropagation?.();
      e?.stopImmediatePropagation?.();
      const row = cell.getRow();
      table.selectDataRow?.(row);
    },
  };
}

export default {
  discoverColumns,
  detectColtype,
  buildColumnsFromData,
  sortColumnsByPriority,
  mergeColumnsWithTableDef,
  applyEditModeGate,
  buildSelectorColumn,
  DEEP_MERGE_PARAM_KEYS,
  deepMergeParams,
};
