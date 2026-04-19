/**
 * LithiumTable — JSON-driven Tabulator column resolution engine
 *
 * Loads column type definitions (coltypes.json) and per-table definitions
 * (e.g. queries/query-manager.json) at runtime, then resolves them into
 * fully-formed Tabulator column definition arrays.
 *
 * Resolution order (per LITHIUM-TAB.md):
 *   1. Start with coltype defaults from coltypes.json
 *   2. Apply column-level overrides from the tabledef
 *   3. Apply runtime overrides (Navigator size buttons, user prefs) [future]
 *
 * @module tables/lithium-table
 */

import { TabulatorFull as Tabulator } from 'tabulator-tables';

// Import from new resolution modules
import {
  loadColtypes,
  getColtypes,
  clearColtypesCache,
} from './resolution/coltype-loader.js';

import {
  loadTableDef,
  createAutoDiscoverTableDef,
  clearTableDefCache,
  clearLookup59Cache,
  _tableDefCache,
} from './resolution/tabledef-loader.js';

import {
  loadLookup,
  getLookup,
  resolveLookupLabel,
  createLookupFormatter,
  createIconFormatter,
  createIconLabelFormatter,
  createLookupEditor,
  createLookupSorter,
  compareLookupValues,
  preloadLookups,
  clearLookupCache,
} from './resolution/lookup-loader.js';

import {
  wrapFormatter,
  needsBlankZeroWrapper,
  formatBuiltinValue,
  LITHIUM_CALCULATIONS,
} from './resolution/formatters.js';

import {
  validateTableDef,
  TABLEDEF_VALID_PROPS,
  COLUMN_VALID_PROPS,
  VALID_COLTYPES,
} from './resolution/validator.js';

// ── Custom Formatter Registration ───────────────────────────────────────────

/**
 * Custom lookup formatter for lookupFixed coltype.
 * Looks up the cell value in formatterParams.lookup and returns the label.
 *
 * @param {Object} cell - Tabulator cell component
 * @param {Object} formatterParams - Formatter parameters
 * @param {Object} formatterParams.lookup - Map of value -> label
 * @returns {string} The display label, or the raw value if not found
 */
export function lookupFormatter(cell, formatterParams) {
  const value = cell.getValue();
  const lookup = formatterParams?.lookup || {};

  // Handle null/undefined/empty
  if (value === null || value === undefined || value === '') {
    return '';
  }

  // Look up the value in the lookup map
  const label = lookup[value];
  return label !== undefined ? label : String(value);
}

// Register the lookup formatter with Tabulator
if (typeof Tabulator !== 'undefined' && Tabulator.registerFormatter) {
  Tabulator.registerFormatter('lookup', lookupFormatter);
}

// ── Re-exports ──────────────────────────────────────────────────────────────

export {
  // Coltype loader
  loadColtypes,
  getColtypes,
  clearColtypesCache,

  // Tabledef loader
  loadTableDef,
  createAutoDiscoverTableDef,
  clearTableDefCache,
  clearLookup59Cache,
  _tableDefCache,

  // Lookup loader
  loadLookup,
  getLookup,
  resolveLookupLabel,
  createLookupFormatter,
  createIconFormatter,
  createIconLabelFormatter,
  createLookupEditor,
  createLookupSorter,
  compareLookupValues,
  preloadLookups,
  clearLookupCache,

  // Formatters
  wrapFormatter,
  needsBlankZeroWrapper,
  formatBuiltinValue,
  LITHIUM_CALCULATIONS,

  // Validator
  validateTableDef,
  TABLEDEF_VALID_PROPS,
  COLUMN_VALID_PROPS,
  VALID_COLTYPES,

  // Column metadata
  extractColumnMeta,
  resolveColumnsWithMeta,
};

// ── Column Resolution ───────────────────────────────────────────────────────

/**
 * Resolve a single column definition by merging coltype defaults with
 * column-level overrides, producing a Tabulator-ready column object.
 *
 * @param {string}  fieldName  - The column key from the tabledef
 * @param {Object}  colDef     - Column definition from the tabledef
 * @param {Object}  coltypes   - Full coltypes map
 * @param {Object}  [options]  - Runtime options
 * @param {boolean} [options.tableReadonly] - Table-level readonly flag
 * @param {Function} [options.filterEditor] - Custom header-filter editor fn
 * @returns {Object} Tabulator column definition
 */
export function resolveColumn(fieldName, colDef, coltypes, options = {}) {
  // CRITICAL: Start with coltypes.default, then overlay specific coltype, then column definition
  // This is the foundation - all coltypes inherit from the "default" stanza
  // Merge order: default → type-specific → column definition (later stages override earlier)

  const defaultCol = coltypes.default || {};
  const typeCol = coltypes[colDef.coltype] || {};

  // Merge strategy: coltypes.default → coltype specific → column definition
  const merged = { ...defaultCol, ...typeCol, ...colDef };

  // Extract Lithium-specific metadata (not passed to Tabulator)
  const lithiumMeta = {
    title: colDef.title,
    field: colDef.field,
    coltype: colDef.coltype,
    visible: colDef.visible,
    sort: colDef.sort,
    filter: colDef.filter,
    groupable: colDef.groupable,
    groupPri: colDef.groupPri,
    groupDir: colDef.groupDir,
    sortPri: colDef.sortPri,
    sortDir: colDef.sortDir,
    editable: colDef.editable,
    calculated: colDef.calculated,
    primaryKey: colDef.primaryKey,
    description: colDef.description,
    lookupRef: colDef.lookupRef,
    lookupStyle: colDef.lookupStyle,
    lookupEdit: colDef.lookupEdit,
    groupStyle: colDef.groupStyle,
    groupTitle: colDef.groupTitle,
  };

  // Filter out Lithium-specific keys from merged (rest goes to Tabulator)
  const tabulatorProps = {};
  for (const [key, value] of Object.entries(merged)) {
    if (!(key in lithiumMeta)) {
      tabulatorProps[key] = value;
    }
  }

  const normalizedEditor = merged.editor === 'select' ? 'list' : merged.editor;
  const normalizedEditorParams = merged.editor === 'select'
    ? {
        ...(merged.editorParams || {}),
        autocomplete: false,
      }
    : merged.editorParams;

  // Determine editability
  const isEditable = !options.tableReadonly
    && colDef.editable === true
    && colDef.calculated !== true;

  // Build Tabulator column definition
  const tabulatorCol = {
    title: colDef.title,
    field: colDef.field,
    visible: colDef.visible !== false,

    // Store only Tabulator-compatible properties
    // Lithium metadata is stored separately in LithiumTable._columnMeta
    columnPri: colDef.columnPri,

    // Alignment
    hozAlign: colDef.hozAlign || merged.align || 'left',
    vertAlign: merged.vertAlign || 'middle',

    // Sorting
    headerSort: colDef.sort !== false,
    headerSortTristate: true,
    sorter: merged.sorter || undefined,
  };

  // Include primaryKey marker when set (used by row ID functions)
  if (colDef.primaryKey === true) {
    tabulatorCol.primaryKey = true;
  }

  // Sorter params (e.g. date format)
  if (merged.sorterParams) {
    tabulatorCol.sorterParams = merged.sorterParams;
  }

  // Formatter — lookup columns use the cached lookup formatter;
  // all other columns wrap with blank/zero handler.
  if (colDef.lookupRef && getLookup(colDef.lookupRef)) {
    const lookupData = getLookup(colDef.lookupRef);
    // lookupStyle: 'icon' shows icon only; 'iconLabel' shows both; default 'label' shows text only
    if (colDef.lookupStyle === 'icon') {
      tabulatorCol.formatter = createIconFormatter(lookupData);
    } else if (colDef.lookupStyle === 'iconLabel') {
      tabulatorCol.formatter = createIconLabelFormatter(lookupData);
    } else {
      // Default: lookup formatter resolves integer IDs → human-readable labels
      tabulatorCol.formatter = createLookupFormatter(colDef.lookupRef);
    }
    // Use custom sorter that sorts by lookup label, not raw ID
    tabulatorCol.sorter = createLookupSorter(colDef.lookupRef);
  } else if (merged.formatter) {
    tabulatorCol.formatter = wrapFormatter(
      merged.formatter,
      merged.formatterParams || {},
      merged.blank,
      merged.zero,
    );
    // Preserve original formatterParams for Tabulator's built-in formatters
    // (the wrapper passes them through when calling the original)
    tabulatorCol.formatterParams = merged.formatterParams || {};
  }

  // Editor — lookup columns get a list editor from cached data;
  // other editable columns get the coltype editor.
  if (isEditable && colDef.lookupRef && getLookup(colDef.lookupRef)) {
    const lookupData = getLookup(colDef.lookupRef);
    const lookupEditorConfig = createLookupEditor(colDef.lookupRef, lookupData);
    if (typeof lookupEditorConfig === 'object') {
      tabulatorCol.editor = lookupEditorConfig.editor;
      tabulatorCol.editorParams = lookupEditorConfig.editorParams;

      // lookupEdit controls dropdown display: 'icon', 'iconLabel', or 'label' (default)
      const lookupEdit = colDef.lookupEdit || (colDef.lookupStyle === 'icon' ? 'icon' : 'label');
      if (lookupEdit !== 'label') {
        // Get hozAlign for dropdown item alignment
        // Only copy column's hozAlign when lookupEdit is 'icon'; otherwise default to 'left'
        const hozAlign = lookupEdit === 'icon'
          ? (colDef.hozAlign || tabulatorCol.hozAlign || 'left')
          : 'left';

        tabulatorCol.editorParams.itemFormatter = function(label, value, _item, _element) {
          const entry = lookupData.find(e => e.id == value);
          if (!entry) return label;

          switch (lookupEdit) {
            case 'icon':
              return `<div class="li-lookup-list-item" data-align="${hozAlign}">${entry.icon || '<span class="li-lookup-no-icon"></span>'}</div>`;
            case 'iconLabel': {
              const iconHtml = entry.icon ? entry.icon : '';
              // Wrap icon and label in separate spans to isolate FontAwesome mutations
              return `<div class="li-lookup-item" data-align="${hozAlign}"><span class="li-lookup-edit-icon">${iconHtml}</span><span class="li-lookup-edit-label">${entry.label}</span></div>`;
            }
            case 'iconText': {
              const iconHtml = entry.icon ? entry.icon : '';
              return `<div class="li-lookup-item" data-align="${hozAlign}"><span class="li-lookup-edit-icon">${iconHtml}</span><span class="li-lookup-edit-label">${entry.label}</span></div>`;
            }
            default:
              return `<div class="li-lookup-list-item" data-align="${hozAlign}">${entry.label}</div>`;
          }
        };
      }
    } else {
      tabulatorCol.editor = lookupEditorConfig; // 'number' fallback
    }
    // Ensure lookup values are always stored as integers
    tabulatorCol.mutator = function(value) {
      if (value === null || value === undefined || value === '') {
        return 0;
      }
      // If it's the label string, look up the ID
      if (typeof value === 'string') {
        const entry = lookupData.find(e => e.label === value);
        if (entry) {
          return entry.id;
        }
      }
      return parseInt(value, 10) || 0;
    };
  } else if (isEditable && normalizedEditor && normalizedEditor !== false) {
    tabulatorCol.editor = normalizedEditor;
    if (normalizedEditorParams && Object.keys(normalizedEditorParams).length > 0) {
      tabulatorCol.editorParams = normalizedEditorParams;
    }
  }

  // Header filter - enabled by default unless explicitly disabled with filter: false
  if (colDef.filter !== false) {
    if (options.filterEditor) {
      tabulatorCol.headerFilter = options.filterEditor;
    } else {
      tabulatorCol.headerFilter = 'input';
    }
    // Default to 'like' filtering if no specific function defined
    tabulatorCol.headerFilterFunc = merged.headerFilterFunc || 'like';
    if (merged.headerFilterPlaceholder) {
      tabulatorCol.headerFilterParams = {
        placeholder: merged.headerFilterPlaceholder,
      };
    }
  }

  // CSS class
  if (merged.cssClass) {
    tabulatorCol.cssClass = merged.cssClass;
  }

  // Width constraints
  if (merged.width != null) {
    tabulatorCol.width = merged.width;
  }
  if (merged.minWidth != null) {
    tabulatorCol.minWidth = merged.minWidth;
  }

  // Bottom calculations
  if (merged.bottomCalc != null) {
    // Use custom calculation functions that properly handle 0 values
    // instead of Tabulator's built-ins which exclude 0, empty string, etc.
    const calcName = merged.bottomCalc;
    if (LITHIUM_CALCULATIONS[calcName]) {
      tabulatorCol.bottomCalc = LITHIUM_CALCULATIONS[calcName];
    } else {
      // Pass through custom functions or unknown calculations as-is
      tabulatorCol.bottomCalc = calcName;
    }
  }
  if (merged.bottomCalcFormatter) {
    // Tabulator does not have a built-in "number" formatter. Our
    // formatBuiltinValue() replicates it (thousand seps, precision, etc.).
    // Wrap non-native formatter names into a function so Tabulator gets
    // a callable rather than an unrecognised string.
    const calcFmt = merged.bottomCalcFormatter;
    const calcParams = merged.bottomCalcFormatterParams || {};
    if (calcFmt === 'number' || calcFmt === 'money') {
      tabulatorCol.bottomCalcFormatter = (cell) =>
        formatBuiltinValue(cell.getValue(), calcFmt, calcParams);
      // params are captured in the closure — no need for bottomCalcFormatterParams
    } else {
      tabulatorCol.bottomCalcFormatter = calcFmt;
      if (merged.bottomCalcFormatterParams) {
        tabulatorCol.bottomCalcFormatterParams = merged.bottomCalcFormatterParams;
      }
    }
  }

  // Download accessor
  if (merged.accessorDownload) {
    tabulatorCol.accessorDownload = merged.accessorDownload;
  }

  // Frozen column (stays fixed when scrolling horizontally)
  if (colDef.frozen === true) {
    tabulatorCol.frozen = true;
  }

  // Row handle (marks this column as the drag handle for movableRows)
  if (colDef.rowHandle === true) {
    tabulatorCol.rowHandle = true;
  }

  // Resizable override (default is true; only set when explicitly false)
  if (colDef.resizable === false) {
    tabulatorCol.resizable = false;
  }

  // Custom formatter from colDef (function) takes precedence over coltype formatter
  if (typeof colDef.formatter === 'function') {
    tabulatorCol.formatter = colDef.formatter;
    // Clear formatterParams since a custom function manages its own output
    delete tabulatorCol.formatterParams;
  }

  return tabulatorCol;
}

/**
 * Resolve all columns from a table definition into a Tabulator columns array.
 *
 * @param {Object}  tableDef   - Parsed table definition
 * @param {Object}  coltypes   - Parsed coltypes map
 * @param {Object}  [options]  - Runtime options passed to resolveColumn
 * @param {Function} [options.filterEditor] - Custom header-filter editor function
 * @returns {Array<Object>} Array of Tabulator column definitions
 */
export function resolveColumns(tableDef, coltypes, options = {}) {
  if (!tableDef?.columns) return [];

  const resolvedOptions = {
    ...options,
    tableReadonly: tableDef.readonly === true,
  };

  const columns = [];
  for (const [fieldName, colDef] of Object.entries(tableDef.columns)) {
    columns.push(resolveColumn(fieldName, colDef, coltypes, resolvedOptions));
  }

  return columns;
}

/**
 * Extract Lithium-only metadata from a column definition.
 * This metadata is stored separately from Tabulator columns to avoid console warnings.
 *
 * @param {Object} colDef - Column definition from tableDef
 * @returns {Object} Lithium metadata object
 */
function extractColumnMeta(colDef) {
  return {
    coltype: colDef.coltype || 'string',
    editable: colDef.editable === true,
    sort: colDef.sort !== false,
    filter: colDef.filter !== false,
    groupable: colDef.groupable === true,
    groupPri: colDef.groupPri ?? null,
    groupDir: colDef.groupDir || 'asc',
    sortPri: colDef.sortPri ?? null,
    sortDir: colDef.sortDir || 'asc',
    calculated: colDef.calculated === true,
    primaryKey: colDef.primaryKey === true,
    description: colDef.description || `${colDef.title || colDef.field} column`,
    lookupRef: colDef.lookupRef || null,
    lookupStyle: colDef.lookupStyle || null,
    lookupEdit: colDef.lookupEdit || null,
    groupStyle: colDef.groupStyle || null,
    groupTitle: colDef.groupTitle || null,
    columnPri: colDef.columnPri ?? null,
  };
}

/**
 * Resolve columns and extract Lithium metadata in one pass.
 * Returns both Tabulator-compatible columns and a Map of field -> metadata.
 *
 * @param {Object}  tableDef   - Parsed table definition
 * @param {Object}  coltypes   - Parsed coltypes map
 * @param {Object}  [options]  - Runtime options passed to resolveColumn
 * @returns {{ columns: Array<Object>, columnMeta: Map<string, Object> }}
 */
function resolveColumnsWithMeta(tableDef, coltypes, options = {}) {
  if (!tableDef?.columns) {
    return { columns: [], columnMeta: new Map() };
  }

  const resolvedOptions = {
    ...options,
    tableReadonly: tableDef.readonly === true,
  };

  const columns = [];
  const columnMeta = new Map();

  for (const [fieldName, colDef] of Object.entries(tableDef.columns)) {
    columns.push(resolveColumn(fieldName, colDef, coltypes, resolvedOptions));
    columnMeta.set(colDef.field, extractColumnMeta(colDef));
  }

  return { columns, columnMeta };
}

/**
 * Map table-level properties from a tabledef to Tabulator constructor options.
 *
 * @param {Object} tableDef - Parsed table definition
 * @returns {Object} Tabulator options (layout, selectableRows, etc.)
 */
export function resolveTableOptions(tableDef) {
  if (!tableDef) return {};

  const opts = {
    // Enable column movement by default — individual columns can opt out via frozen: true
    movableColumns: true,
    // Enable tristate sorting: asc → desc → no sort (original order)
    headerSortTristate: true,
  };

  if (tableDef.layout) opts.layout = tableDef.layout;
  if (tableDef.responsiveLayout) opts.responsiveLayout = tableDef.responsiveLayout;
  if (tableDef.selectableRows != null) opts.selectableRows = tableDef.selectableRows;
  if (tableDef.resizableColumns != null) opts.resizableColumns = tableDef.resizableColumns;
  if (tableDef.initialSort) opts.initialSort = tableDef.initialSort;

  // Resolve groupBy: if explicitly set in tableDef, use it; otherwise compute from column grouping props
  if (tableDef.groupBy) {
    // Explicit groupBy string takes precedence
    opts.groupBy = tableDef.groupBy;
  } else if (tableDef.columns) {
    // Auto-compute groupBy from column groupable/groupPri properties
    // groupable: false/null/undefined = not grouped
    // groupable: true + groupPri: null = not grouped (explicit opt-out)
    // groupable: true + groupPri: number = grouped with that priority (1 = outermost)
    const groupedCols = [];
    for (const [fieldName, colDef] of Object.entries(tableDef.columns)) {
      // Skip if not groupable
      if (colDef.groupable !== true) {
        continue;
      }
      // Skip if groupPri is null/undefined (not in a group)
      if (colDef.groupPri == null) {
        continue;
      }
      const order = Number(colDef.groupPri);
      if (order > 0) {
        groupedCols.push({
          field: fieldName,
          order,
          dir: colDef.groupDir || 'asc'
        });
      }
    }
    // Sort by group priority (ascending), then build array of field names
    if (groupedCols.length > 0) {
      groupedCols.sort((a, b) => a.order - b.order);
      opts.groupBy = groupedCols.map(c => c.field);

      // For lookup columns, add custom groupOrder that sorts by lookup label
      const firstGroupedCol = groupedCols[0];
      const firstColDef = tableDef.columns[firstGroupedCol.field];
      if (firstColDef.lookupRef && getLookup(firstColDef.lookupRef)) {
        const lookupRef = firstColDef.lookupRef;
        const dir = firstGroupedCol.dir;
        opts.groupOrder = function(a, b) {
          return compareLookupValues(a.key, b.key, lookupRef, dir);
        };
      }
    }
  }

  if (tableDef.groupStartOpen != null) opts.groupStartOpen = tableDef.groupStartOpen;
  if (tableDef.groupToggleElement) opts.groupToggleElement = tableDef.groupToggleElement;
  if (tableDef.columnCalcs) opts.columnCalcs = tableDef.columnCalcs;

  // Build initialSort from column sortPri if not explicitly set in tableDef
  if (!tableDef.initialSort && tableDef.columns) {
    const sortCols = [];
    for (const [fieldName, colDef] of Object.entries(tableDef.columns)) {
      // Skip if sortPri is null/undefined (not in sort)
      if (colDef.sortPri == null) {
        continue;
      }
      const order = Number(colDef.sortPri);
      if (order > 0) {
        sortCols.push({
          field: fieldName,
          order,
          dir: colDef.sortDir || 'asc'
        });
      }
    }
    // Sort by sort priority (ascending), then build initialSort array
    if (sortCols.length > 0) {
      sortCols.sort((a, b) => a.order - b.order);
      opts.initialSort = sortCols.map(c => ({ column: c.field, dir: c.dir }));
    }
  }

  // Include the toggle icon directly in the group header
  // Build a map of field -> column definition for lookup resolution
  const colDefMap = tableDef.columns || {};

  opts.groupHeader = (value, count, _data, group) => {
    // Check if this is a lookup column grouped field
    const field = group?.getField?.();
    const colDef = field ? colDefMap[field] : null;
    let displayValue = value;

    if (colDef?.lookupRef && getLookup(colDef.lookupRef)) {
      const lookupData = getLookup(colDef.lookupRef);
      const lookupEntry = lookupData.find(e => String(e.id) === String(value));

      if (lookupEntry) {
        // groupStyle controls display in group headers, fallback to lookupStyle
        const groupStyle = colDef.groupStyle || colDef.lookupStyle || 'label';
        switch (groupStyle) {
          case 'icon':
            displayValue = lookupEntry.icon || lookupEntry.label || value;
            break;
          case 'iconLabel':
            displayValue = `<span class="li-lookup-icon-label"><span class="li-lookup-icon">${lookupEntry.icon || ''}</span><span class="li-lookup-label">${lookupEntry.label || value}</span></span>`;
            break;
          case 'label':
          default:
            displayValue = lookupEntry.label || value;
            break;
        }
      }
    }

    return `<span class="lithium-group-header">
      <fa fa-angle-right class="lithium-group-toggle-icon"></fa>
      <span class="lithium-group-title">${displayValue}</span> <span class="lithium-group-count">(${count})</span>
    </span>`;
  };
  opts.groupToggleElement = 'header';
  if (tableDef.movableRows === true) opts.movableRows = true;
  if (tableDef.persistSort != null) opts.persistSort = tableDef.persistSort;
  if (tableDef.persistFilter != null) opts.persistFilter = tableDef.persistFilter;

  return opts;
}

/**
 * Get the primary key field name from a table definition.
 *
 * @param {Object} tableDef - Parsed table definition
 * @returns {string[]|null} Array of field names for compound primary key, or single-field array, or null
 */
export function getPrimaryKeyField(tableDef) {
  if (!tableDef?.columns) return null;

  const pkFields = [];
  for (const [, colDef] of Object.entries(tableDef.columns)) {
    if (colDef.primaryKey === true) {
      pkFields.push(colDef.field);
    }
  }
  return pkFields.length > 0 ? pkFields : null;
}

/**
 * Get the query reference numbers from a table definition.
 *
 * @param {Object} tableDef - Parsed table definition
 * @returns {{ queryRef: number|null, searchQueryRef: number|null, detailQueryRef: number|null, insertQueryRef: number|null, updateQueryRef: number|null, deleteQueryRef: number|null }}
 */
export function getQueryRefs(tableDef) {
  return {
    queryRef: tableDef?.queryRef ?? null,
    searchQueryRef: tableDef?.searchQueryRef ?? null,
    detailQueryRef: tableDef?.detailQueryRef ?? null,
    insertQueryRef: tableDef?.insertQueryRef ?? null,
    updateQueryRef: tableDef?.updateQueryRef ?? null,
    deleteQueryRef: tableDef?.deleteQueryRef ?? null,
  };
}

/**
 * Clear all cached configuration data.
 * Useful for testing or when configs change at runtime.
 */
export function clearCache() {
  clearColtypesCache();
  clearTableDefCache();
  clearLookupCache();
}
