/**
 * Template Capture Module
 *
 * Captures complete current table state as a template.
 * Provides canonical extractor functions for tableDef generation.
 *
 * @module tables/template/capture
 */

import { COLUMN_VALID_PROPS } from '../resolution/validator.js';
import { resolveColumns } from '../lithium-table.js';

/**
 * Properties that are considered canonical for tableDef columns.
 * These are the properties that will be included in extracted column definitions.
 * Derived from COLUMN_VALID_PROPS minus internal/runtime-only properties.
 */
export const CANONICAL_COLUMN_PROPS = new Set([
  // Core identification
  'field', 'title',
  // Lithium-specific metadata
  'coltype', 'columnPri', 'primaryKey', 'calculated', 'description',
  // Lookup configuration
  'lookupRef', 'lookupFixed', 'lookupStyle', 'lookupEdit',
  // Visibility and layout
  'visible', 'width', 'minWidth', 'maxWidth', 'hozAlign', 'vertAlign',
  // Sorting and filtering
  'sort', 'filter', 'editable',
  // Grouping configuration
  'groupable', 'groupPri', 'groupDir', 'groupStyle', 'groupTitle',
  // Multi-column sort configuration
  'sortPri', 'sortDir',
  // Formatting
  'formatter', 'formatterParams',
  // Editing
  'editor', 'editorParams',
  // Sorting configuration
  'sorter', 'sorterParams', 'headerSort', 'headerSortTristate',
  // Header filtering
  'headerFilter', 'headerFilterFunc', 'headerFilterPlaceholder', 'headerFilterParams',
  // Header display
  'headerHozAlign', 'headerVertical', 'headerVisible',
  // Behavior
  'resizable', 'frozen', 'clipboard', 'cssClass', 'rowCssClass',
  // Interactions
  'vertHandle', 'hozHandle', 'contextMenu', 'tapComprehensive',
  // Tooltips
  'tooltip', 'tooltipHeader',
  // Calculations
  'bottomCalc', 'bottomCalcParams', 'bottomCalcFormatter', 'bottomCalcFormatterParams',
  // Styling
  'blank', 'zero',
]);

/**
 * Extract a single column as a canonical tableDef column entry.
 *
 * @param {Object} column - Tabulator column component
 * @param {Object} originalColDef - Original tableDef column (source of Lithium-only props)
 * @param {Object} lithiumMeta - Lithium metadata from LithiumTable._columnMeta
 * @param {Object} options - Extraction options
 * @param {boolean} [options.includeRuntimeOnly=false] - Include runtime properties like current width
 * @returns {Object} Canonical JSON-serializable column entry
 */
export function extractTableDefColumn(column, originalColDef = {}, lithiumMeta = {}, options = {}) {
  const def = column.getDefinition();
  const field = column.getField();

  // Start with canonical properties from the original tableDef
  const colDef = {};

  // Include canonical properties from original definition
  for (const prop of CANONICAL_COLUMN_PROPS) {
    if (prop in originalColDef && originalColDef[prop] !== undefined) {
      colDef[prop] = originalColDef[prop];
    }
  }

  // Override with runtime values from Tabulator column
  colDef.field = field;
  colDef.title = def.title || originalColDef.title || field;
  colDef.visible = column.isVisible();

  // Include Lithium metadata (now stored separately, not on Tabulator columns)
  if (lithiumMeta.coltype) {
    colDef.coltype = lithiumMeta.coltype;
  }
  if (lithiumMeta.primaryKey) {
    colDef.primaryKey = true;
  }
  if (lithiumMeta.calculated) {
    colDef.calculated = true;
  }
  if (lithiumMeta.description && lithiumMeta.description !== `${colDef.title || field} column`) {
    colDef.description = lithiumMeta.description;
  }
  if (lithiumMeta.lookupRef) {
    colDef.lookupRef = lithiumMeta.lookupRef;
  }
  if (lithiumMeta.lookupStyle) {
    colDef.lookupStyle = lithiumMeta.lookupStyle;
  }
  if (lithiumMeta.lookupEdit) {
    colDef.lookupEdit = lithiumMeta.lookupEdit;
  }
  if (lithiumMeta.groupStyle) {
    colDef.groupStyle = lithiumMeta.groupStyle;
  }
  if (lithiumMeta.groupTitle) {
    colDef.groupTitle = lithiumMeta.groupTitle;
  }

  // Column priority (display order)
  if (def.columnPri !== undefined && def.columnPri !== null) {
    colDef.columnPri = def.columnPri;
  }

  // Runtime width (if includeRuntimeOnly is true)
  if (options.includeRuntimeOnly) {
    const runtimeWidth = column.getWidth?.();
    if (Number.isFinite(runtimeWidth) && runtimeWidth > 0) {
      colDef.width = Math.round(runtimeWidth);
    }
  } else if (def.width !== undefined && def.width !== null) {
    // Use definition width if not including runtime
    colDef.width = def.width;
  }

  // Alignment - prefer hozAlign, fallback to align
  if (def.hozAlign && def.hozAlign !== 'left') {
    colDef.hozAlign = def.hozAlign;
  } else if (def.align && def.align !== 'left') {
    colDef.hozAlign = def.align;
  }

  // Vertical alignment
  if (def.vertAlign && def.vertAlign !== 'middle') {
    colDef.vertAlign = def.vertAlign;
  }

  // Formatter information
  if (def.formatter && typeof def.formatter === 'string') {
    colDef.formatter = def.formatter;
  }
  if (def.formatterParams && Object.keys(def.formatterParams).length > 0) {
    colDef.formatterParams = def.formatterParams;
  }

  // Editor information
  if (def.editor && typeof def.editor === 'string') {
    colDef.editor = def.editor;
  }
  if (def.editorParams && Object.keys(def.editorParams).length > 0) {
    colDef.editorParams = def.editorParams;
  }

  // Sorter configuration
  if (def.sorter && typeof def.sorter === 'string') {
    colDef.sorter = def.sorter;
  }
  if (def.sorterParams && Object.keys(def.sorterParams).length > 0) {
    colDef.sorterParams = def.sorterParams;
  }

  // Header filter configuration
  if (def.headerFilter === false) {
    colDef.headerFilter = false;
  } else if (def.headerFilter) {
    colDef.headerFilter = true;
  }
  if (def.headerFilterFunc && def.headerFilterFunc !== 'like') {
    colDef.headerFilterFunc = def.headerFilterFunc;
  }
  if (def.headerFilterPlaceholder) {
    colDef.headerFilterPlaceholder = def.headerFilterPlaceholder;
  }
  if (def.headerFilterParams && Object.keys(def.headerFilterParams).length > 0) {
    colDef.headerFilterParams = def.headerFilterParams;
  }

  // CSS class
  if (def.cssClass) {
    colDef.cssClass = def.cssClass;
  }

  // Frozen column
  if (def.frozen === true) {
    colDef.frozen = true;
  }

  // Row handle
  if (def.rowHandle === true) {
    colDef.rowHandle = true;
  }

  // Resizable override
  if (def.resizable === false) {
    colDef.resizable = false;
  }

  // Calculations
  if (def.bottomCalc) {
    // Store as string if it's a named calculation
    if (typeof def.bottomCalc === 'string') {
      colDef.bottomCalc = def.bottomCalc;
    }
    // Functions are not serializable, skip them
  }
  if (def.bottomCalcFormatter && typeof def.bottomCalcFormatter === 'string') {
    colDef.bottomCalcFormatter = def.bottomCalcFormatter;
  }
  if (def.bottomCalcFormatterParams && Object.keys(def.bottomCalcFormatterParams).length > 0) {
    colDef.bottomCalcFormatterParams = def.bottomCalcFormatterParams;
  }

  // Header sort configuration
  if (def.headerSort === false) {
    colDef.headerSort = false;
  }

  return colDef;
}

/**
 * Extract full tableDef from a live LithiumTable instance.
 *
 * @param {Object} table - LithiumTable instance
 * @param {Object} options - Extraction options
 * @param {boolean} [options.includeRuntimeOnly=false] - Include runtime-only properties
 * @param {boolean} [options.includeHiddenColumns=true] - Include hidden columns in output
 * @param {boolean} [options.includeTemplateMeta=true] - Include _templateMeta block
 * @returns {Object|null} Complete tableDef object or null if no table
 */
export function extractTableDef(table, options = {}) {
  const {
    includeRuntimeOnly = false,
    includeHiddenColumns = true,
    includeTemplateMeta = true,
  } = options;

  if (!table.table) return null;

  const columns = table.table.getColumns();
  const columnDefs = {};

  // Get runtime sorters
  const sorters = table.table.getSorters?.() || [];
  const initialSort = sorters.map((s) => ({
    column: s.field,
    dir: s.dir,
  }));

  // Get filter values
  const filterValues = {};
  if (table.table.headerFilters) {
    for (const [field, filter] of Object.entries(table.table.headerFilters)) {
      if (filter?.value != null && filter.value !== '') {
        filterValues[field] = filter.value;
      }
    }
  }

  // Process each column
  columns.forEach((col) => {
    const field = col.getField();
    if (!field || field === '_selector') return;

    // Skip hidden columns if not including them
    if (!includeHiddenColumns && !col.isVisible()) return;

    // Get original column definition from tableDef
    const originalColDef = table.tableDef?.columns?.[field] || {};

    // Get lithium metadata from the table's metadata Map
    const lithiumMeta = table._columnMeta?.get(field) || {};

    const extractedCol = extractTableDefColumn(col, originalColDef, lithiumMeta, {
      includeRuntimeOnly,
    });

    columnDefs[field] = extractedCol;
  });

  // Include columns from tableDef that aren't in the table yet
  if (table.tableDef?.columns) {
    for (const [fieldName, colDef] of Object.entries(table.tableDef.columns)) {
      if (!fieldName || fieldName === '_selector') continue;
      if (!(fieldName in columnDefs)) {
        // Column exists in tableDef but not in table - use original definition
        const lithiumMeta = table._columnMeta?.get(fieldName) || {};
        columnDefs[fieldName] = extractColumnFromTableDef(colDef, lithiumMeta);
      }
    }
  }

  // Build the tableDef
  const tableDef = {
    $schema: '../tabledef-schema.json',
    $version: '1.0.0',
    table: table.tablePath?.split('/').pop() || 'table',
    title: table.tableDef?.title || 'Table',
    queryRef: table.queryRefs?.queryRef || null,
    searchQueryRef: table.queryRefs?.searchQueryRef || null,
    detailQueryRef: table.queryRefs?.detailQueryRef || null,
    readonly: table.readonly || false,
    selectableRows: 1,
    layout: table.tableLayoutMode || 'fitColumns',
    resizableColumns: true,
    columns: columnDefs,
  };

  if (initialSort.length > 0) {
    tableDef.initialSort = initialSort;
  }

  // Capture filter visibility state
  tableDef._filtersVisible = table.filtersVisible;

  if (Object.keys(filterValues).length > 0) {
    tableDef._filterValues = filterValues;
  }

  if (includeTemplateMeta) {
    tableDef._templateMeta = {
      name: table.tableDef?.title || 'Custom Template',
      tablePath: table.tablePath,
      managerId: table.getManagerId?.() || table.storageKey,
      createdAt: new Date().toISOString(),
      widthMode: table.tableWidthMode,
    };
  }

  return tableDef;
}

/**
 * Extract column definition from tableDef entry (for columns not yet in table).
 *
 * @param {Object} colDef - Column definition from tableDef
 * @param {Object} lithiumMeta - Lithium metadata
 * @returns {Object} Canonical column definition
 */
function extractColumnFromTableDef(colDef, lithiumMeta = {}) {
  const extracted = {};

  // Include all canonical properties
  for (const prop of CANONICAL_COLUMN_PROPS) {
    if (prop in colDef && colDef[prop] !== undefined) {
      extracted[prop] = colDef[prop];
    }
  }

  // Override with lithium metadata
  if (lithiumMeta.coltype) {
    extracted.coltype = lithiumMeta.coltype;
  }
  if (lithiumMeta.primaryKey) {
    extracted.primaryKey = true;
  }
  if (lithiumMeta.calculated) {
    extracted.calculated = true;
  }

  return extracted;
}

/**
 * Generate migration-ready seed JSON from a live table.
 * Drops _templateMeta and _-prefixed runtime flags; uses stable key ordering.
 *
 * @param {Object} table - LithiumTable instance
 * @returns {string} JSON string suitable for pasting into a migration
 */
export function generateMigrationSeed(table) {
  const tableDef = extractTableDef(table, {
    includeRuntimeOnly: true,
    includeHiddenColumns: true,
    includeTemplateMeta: false,
  });

  if (!tableDef) return '';

  // Remove _-prefixed properties
  const cleaned = {};
  for (const [key, value] of Object.entries(tableDef)) {
    if (!key.startsWith('_')) {
      cleaned[key] = value;
    }
  }

  // Also clean columns
  if (cleaned.columns) {
    const cleanedColumns = {};
    for (const [field, colDef] of Object.entries(cleaned.columns)) {
      const cleanedCol = {};
      for (const [key, value] of Object.entries(colDef)) {
        if (!key.startsWith('_')) {
          cleanedCol[key] = value;
        }
      }
      cleanedColumns[field] = cleanedCol;
    }
    cleaned.columns = cleanedColumns;
  }

  // Generate with stable key ordering and consistent formatting
  return JSON.stringify(cleaned, null, 2);
}

/**
 * Capture the complete current state as a template.
 * This captures everything about the current table including:
 * - All column definitions (visible and hidden)
 * - Column order (as they appear at runtime)
 * - Column widths, visibility, sorting, filtering, etc.
 * - Sort order
 * - Filter values
 * - Layout mode
 * - Width mode
 *
 * @deprecated Use extractTableDef() instead
 * @param {Object} table - LithiumTable instance
 * @returns {Object|null} Complete template or null if no table
 */
export function captureCurrentState(table) {
  return extractTableDef(table, {
    includeRuntimeOnly: true,
    includeHiddenColumns: true,
    includeTemplateMeta: true,
  });
}

/**
 * Create a template column from tableDef (helper for captureCurrentState).
 *
 * @deprecated Use extractTableDefColumn() instead
 * @param {string} fieldName - Field name
 * @param {Object} colDef - Column definition
 * @returns {Object} Copied column definition
 */
export function createTemplateColumnFromTableDef(fieldName, colDef = {}) {
  // Copy all canonical properties from the colDef
  const extracted = {};
  for (const prop of CANONICAL_COLUMN_PROPS) {
    if (prop in colDef && colDef[prop] !== undefined) {
      extracted[prop] = colDef[prop];
    }
  }
  return extracted;
}

/**
 * Extract template column from Tabulator column (helper for captureCurrentState).
 *
 * @deprecated Use extractTableDefColumn() instead
 * @param {Object} column - Tabulator column component
 * @returns {Object} Extracted column definition
 */
export function extractTemplateColumnFromColumn(column) {
  const def = column.getDefinition();
  const field = column.getField();

  // Build a minimal colDef for extraction
  const originalColDef = { field, title: def.title };

  // Extract with runtime width
  return extractTableDefColumn(column, originalColDef, {}, { includeRuntimeOnly: true });
}

/**
 * Generate a complete template JSON from the current table state.
 * This is the canonical template extractor used by the Template menu.
 *
 * @deprecated Use extractTableDef() instead
 * @param {Object} table - LithiumTable instance
 * @returns {Object|null} Template JSON or null if no table
 */
export function generateTemplateJSON(table) {
  // Handle Column Manager pending columns if present
  if (table.columnManager?.columnTable?.table) {
    const pendingColumns = table.columnManager.buildPendingTemplateColumns?.();
    if (pendingColumns && Object.keys(pendingColumns).length > 0) {
      const sorters = table.table.getSorters?.() || [];
      const initialSort = sorters.map((s) => ({
        column: s.field,
        dir: s.dir,
      }));

      const tableDef = {
        $schema: '../tabledef-schema.json',
        $version: '1.0.0',
        $description: `Table definition for ${table.tableDef?.title || table.tablePath}`,
        table: table.tablePath?.split('/').pop() || 'table',
        title: table.tableDef?.title || 'Table',
        queryRef: table.queryRefs?.queryRef || null,
        searchQueryRef: table.queryRefs?.searchQueryRef || null,
        detailQueryRef: table.queryRefs?.detailQueryRef || null,
        readonly: table.readonly || false,
        selectableRows: 1,
        layout: table.tableLayoutMode || 'fitColumns',
        resizableColumns: true,
        columns: pendingColumns,
      };

      if (initialSort.length > 0) {
        tableDef.initialSort = initialSort;
      }

      return {
        ...tableDef,
        _templateMeta: {
          name: table.tableDef?.title || 'Custom Template',
          tablePath: table.tablePath,
          managerId: table.getManagerId?.() || table.storageKey,
          createdAt: new Date().toISOString(),
          widthMode: table.tableWidthMode,
        },
      };
    }
  }

  // Use the canonical extractor
  return extractTableDef(table, {
    includeRuntimeOnly: true,
    includeHiddenColumns: true,
    includeTemplateMeta: true,
  });
}

/**
 * Build column definitions from template state.
 * Applies template column patches to current columns and resolves the result.
 *
 * @param {Object} params - Build parameters
 * @param {Object} params.table - Tabulator table instance
 * @param {Object} params.templateColumns - Template column patches (from user customization)
 * @param {Object} params.tableDefColumns - Original tableDef columns
 * @param {Object} params.coltypes - Coltypes map
 * @param {Function} params.filterEditor - Filter editor function
 * @param {boolean} params.filtersVisible - Whether filters are visible
 * @param {Array<string>} params.primaryKeyField - Primary key field(s)
 * @param {boolean} params.readonly - Readonly flag
 * @param {Object} params.selectorColumn - Selector column definition
 * @param {Map} params.columnMeta - Lithium column metadata Map
 * @returns {Array<Object>} Resolved column definitions
 */
export function buildColumnDefinitionsFromTemplateState({
  table,
  templateColumns,
  tableDefColumns,
  coltypes,
  filterEditor,
  filtersVisible,
  primaryKeyField,
  readonly,
  selectorColumn,
  columnMeta = new Map(),
}) {
  const currentColumns = table.getColumns();
  const currentColumnMap = new Map();

  currentColumns.forEach((column) => {
    const field = column.getField();
    if (!field || field === '_selector') return;
    const originalColDef = tableDefColumns?.[field] || {};
    const lithiumMeta = columnMeta.get(field) || {};
    currentColumnMap.set(field, extractTableDefColumn(column, originalColDef, lithiumMeta, {
      includeRuntimeOnly: true,
    }));
  });

  Object.entries(tableDefColumns || {}).forEach(([fieldName, colDef]) => {
    if (!fieldName || fieldName === '_selector') return;
    if (!currentColumnMap.has(fieldName)) {
      const lithiumMeta = columnMeta.get(fieldName) || {};
      currentColumnMap.set(fieldName, extractColumnFromTableDef(colDef, lithiumMeta));
    }
  });

  const orderedEntries = [];

  Object.entries(templateColumns || {}).forEach(([field, patchColumn]) => {
    const baseColumn = currentColumnMap.get(field);
    if (!baseColumn) return;
    currentColumnMap.delete(field);
    // Merge: patchColumn properties override baseColumn
    orderedEntries.push([field, { ...baseColumn, ...patchColumn }]);
  });

  currentColumnMap.forEach((baseColumn, field) => {
    orderedEntries.push([field, baseColumn]);
  });

  const resolvedColumns = resolveColumns(
    {
      readonly,
      columns: Object.fromEntries(orderedEntries),
    },
    coltypes,
    { filterEditor },
  );

  if (!filtersVisible) {
    removeHiddenHeaderFilters(resolvedColumns);
  }

  return selectorColumn ? [selectorColumn, ...resolvedColumns] : resolvedColumns;
}

/**
 * Remove header filter configuration from columns.
 * @param {Array<Object>} columns - Column definitions
 */
function removeHiddenHeaderFilters(columns) {
  columns.forEach((column) => {
    delete column.headerFilter;
    delete column.headerFilterFunc;
    delete column.headerFilterParams;
  });
}

export default {
  CANONICAL_COLUMN_PROPS,
  extractTableDefColumn,
  extractTableDef,
  generateMigrationSeed,
  buildColumnDefinitionsFromTemplateState,
  captureCurrentState,
  createTemplateColumnFromTableDef,
  extractTemplateColumnFromColumn,
  generateTemplateJSON,
};
