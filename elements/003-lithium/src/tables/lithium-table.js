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
 * @module core/lithium-table
 */

import { log, Subsystems, Status } from '../core/log.js';
import { getTabulatorSchemas, fetchLookups } from '../shared/lookups.js';
import { eventBus, Events } from '../core/event-bus.js';
import { TabulatorFull as Tabulator } from 'tabulator-tables';

// ── Custom Formatters ───────────────────────────────────────────────────────

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

// ── Module-level cache ──────────────────────────────────────────────────────

/** @type {Object|null} Cached coltypes.json data (loaded once) */
let _coltypesCache = null;

/** @type {Map<string, Object>} Cached table definitions keyed by path */
const _tableDefCache = new Map();

/** @type {Map<string, Array<{id: number, label: string}>>} Cached lookup tables */
const _lookupCache = new Map();

function parseSchemaCollection(collection) {
  if (!collection) return null;

  try {
    return typeof collection === 'string' ? JSON.parse(collection) : collection;
  } catch {
    return null;
  }
}

function buildTablePathCandidates(tablePath) {
  const normalised = tablePath.replace(/\.json$/, '');
  const parts = normalised.split('/');
  const baseName = parts[parts.length - 1] || normalised;
  const parentPath = parts.slice(0, -1).join('/');
  const candidates = new Set([normalised, baseName]);

  if (baseName.endsWith('-list')) {
    const stripped = baseName.replace(/-list$/, '');
    candidates.add(stripped);
    if (parentPath) candidates.add(`${parentPath}/${stripped}`);
  } else {
    candidates.add(`${baseName}-list`);
    if (parentPath) candidates.add(`${parentPath}/${baseName}-list`);
  }

  return candidates;
}

function findSchemaEntryByCandidates(schemas, tablePath) {
  const candidates = buildTablePathCandidates(tablePath);
  const scoredMatches = [];

  for (const entry of schemas || []) {
    const data = parseSchemaCollection(entry?.collection);
    if (!data) continue;

    const schemaTable = data.table || null;
    const schemaPath = data._templateMeta?.tablePath || null;
    const entryName = entry?.value_txt || null;

    if (schemaPath && candidates.has(schemaPath)) {
      return { entry, data };
    }

    if (entryName && candidates.has(entryName)) {
      scoredMatches.push({ score: entryName === tablePath ? 3 : 2, entry, data });
    }

    if (schemaTable && candidates.has(schemaTable)) {
      const baseName = tablePath.split('/').pop() || tablePath;
      scoredMatches.push({ score: schemaTable === baseName ? 2 : 1, entry, data });
    }
  }

  scoredMatches.sort((a, b) => b.score - a.score);
  if (scoredMatches.length > 0) {
    const { entry, data } = scoredMatches[0];
    return { entry, data };
  }

  return { entry: null, data: null };
}

// ── Custom Calculation Functions ────────────────────────────────────────────

/**
 * Custom count function that counts all non-null/non-undefined values.
 * Unlike Tabulator's built-in count, this treats 0 as a valid value to count.
 * Only excludes null and undefined (not 0 or empty string).
 */
function lithiumCount(values, _data, _calcParams) {
  let count = 0;
  values.forEach((value) => {
    // Count everything except null and undefined
    // 0, empty string, false, etc. are all counted
    if (value !== null && value !== undefined) {
      count++;
    }
  });
  return count;
}

/**
 * Custom unique function that counts unique non-null/non-undefined values.
 * Unlike Tabulator's built-in unique, this treats 0 as a valid value.
 * Only excludes null and undefined (not 0 or empty string).
 */
function lithiumUnique(values, _data, _calcParams) {
  const uniqueSet = new Set();
  values.forEach((value) => {
    // Include everything except null and undefined
    if (value !== null && value !== undefined) {
      uniqueSet.add(value);
    }
  });
  return uniqueSet.size;
}

/**
 * Custom sum function that sums all numeric values.
 * Treats null/undefined as 0, includes 0 in calculations.
 */
function lithiumSum(values, _data, _calcParams) {
  let sum = 0;
  values.forEach((value) => {
    const num = parseFloat(value);
    if (!isNaN(num)) {
      sum += num;
    }
  });
  return sum;
}

/**
 * Custom average function that averages all numeric values.
 * Treats null/undefined as 0 for sum, but excludes them from count.
 * Includes 0 in both sum and count.
 */
function lithiumAvg(values, _data, _calcParams) {
  let sum = 0;
  let count = 0;
  values.forEach((value) => {
    const num = parseFloat(value);
    if (!isNaN(num)) {
      sum += num;
      count++;
    }
  });
  return count > 0 ? sum / count : 0;
}

/**
 * Custom min function that finds the minimum numeric value.
 * Excludes null/undefined/NaN, includes 0.
 */
function lithiumMin(values, _data, _calcParams) {
  let min = Infinity;
  let hasValue = false;
  values.forEach((value) => {
    const num = parseFloat(value);
    if (!isNaN(num)) {
      if (!hasValue || num < min) {
        min = num;
        hasValue = true;
      }
    }
  });
  return hasValue ? min : null;
}

/**
 * Custom max function that finds the maximum numeric value.
 * Excludes null/undefined/NaN, includes 0.
 */
function lithiumMax(values, _data, _calcParams) {
  let max = -Infinity;
  let hasValue = false;
  values.forEach((value) => {
    const num = parseFloat(value);
    if (!isNaN(num)) {
      if (!hasValue || num > max) {
        max = num;
        hasValue = true;
      }
    }
  });
  return hasValue ? max : null;
}

/**
 * Map of calculation names to custom functions.
 * These replace Tabulator's built-in calculations to properly handle 0 values.
 */
const LITHIUM_CALCULATIONS = {
  count: lithiumCount,
  unique: lithiumUnique,
  sum: lithiumSum,
  avg: lithiumAvg,
  min: lithiumMin,
  max: lithiumMax,
};

// ── Public API ──────────────────────────────────────────────────────────────

/**
 * Load and cache coltypes.json (singleton — fetched once per session).
 *
 * Accepts an optional pre-loaded data object (e.g., from an ES module
 * import of the JSON file). When provided, the fetch is skipped entirely,
 * guaranteeing the coltypes are available regardless of whether the
 * config file is served by the production web server.
 *
 * @param {Object} [providedData] - Pre-loaded coltypes JSON (with .coltypes key)
 * @returns {Promise<Object>} The coltypes object (keyed by type name)
 */
export async function loadColtypes(providedData) {
  if (_coltypesCache) return _coltypesCache;

  // ── Priority 1: Caller-provided data (e.g., Vite ES module import) ──
  if (providedData) {
    _coltypesCache = providedData.coltypes || providedData;
    log(Subsystems.MANAGER, Status.INFO,
      `[LithiumTable] Loaded ${Object.keys(_coltypesCache).length} coltypes (from import)`);
    return _coltypesCache;
  }

  // ── Priority 2: Lookup cache (from database via QueryRef 060) ──────
  // If lookups aren't loaded yet, trigger the fetch and wait for it
  let schemas = getTabulatorSchemas();
  if (!schemas) {
    log(Subsystems.MANAGER, Status.INFO,
      '[LithiumTable] Tabulator schemas not in cache, triggering lookup fetch...');
    await fetchLookups();
    schemas = getTabulatorSchemas();
  }

  if (schemas) {
    const coltypesEntry = schemas.find(s => s.key_idx === 0);
    if (coltypesEntry?.collection) {
      try {
        const data = typeof coltypesEntry.collection === 'string'
          ? JSON.parse(coltypesEntry.collection)
          : coltypesEntry.collection;
        _coltypesCache = data.coltypes || data;
        log(Subsystems.MANAGER, Status.INFO,
          `[LithiumTable] Loaded ${Object.keys(_coltypesCache).length} coltypes (from lookup)`);
        return _coltypesCache;
      } catch (err) {
        log(Subsystems.MANAGER, Status.ERROR,
          `[LithiumTable] Failed to parse coltypes from lookup: ${err.message}`);
      }
    }
  }

  log(Subsystems.MANAGER, Status.ERROR,
    '[LithiumTable] Failed to load coltypes from lookups');
  return {};
}

/**
 * Load and cache a table definition JSON file.
 *
 * Accepts an optional pre-loaded data object (e.g., from an ES module
 * import). When provided, the fetch is skipped entirely.
 *
 * For Lookup 59 (Tabulator Schemas from QueryRef 060), the data always comes
 * from the lookup cache which is refreshed on app startup. We skip the
 * _tableDefCache for these dynamic schemas to ensure changes in Lookup 59
 * are reflected without requiring a page reload.
 *
 * @param {string} tablePath - Relative path under config/tabulator/
 *   e.g. 'queries/query-manager' (no .json extension needed)
 * @param {Object} [providedData] - Pre-loaded tabledef JSON object
 * @returns {Promise<Object>} The parsed table definition
 */
export async function loadTableDef(tablePath, providedData) {
  // Normalise path: strip trailing .json if present
  const normalised = tablePath.replace(/\.json$/, '');

  // ── Priority 1: Caller-provided data (e.g., Vite ES module import) ──
  if (providedData) {
    _tableDefCache.set(normalised, providedData);
    log(Subsystems.MANAGER, Status.INFO,
      `[LithiumTable] Loaded tabledef "${providedData.title || normalised}" ` +
      `(${Object.keys(providedData.columns || {}).length} columns, from import)`);
    return providedData;
  }

  // ── Priority 2: Lookup cache (from database via QueryRef 060) ──────
  // Map table paths to lookup key_idx values (Lookup 059 - Tabulator Schemas)
  const tableToKeyIdx = {
    'queries/query-manager': 1,      // Query Manager table schema
    'lookups/lookups-list': 2,       // Lookups Manager parent table schema
    'lookups/lookup-values': 3,      // Lookups Manager child table schema
    'style-manager/lookup-41': 6,    // Style Manager main table schema
    'version-manager/version-history': 7, // Version Manager table schema
  };
  const keyIdx = tableToKeyIdx[normalised];
  const isLookup59Schema = keyIdx != null;

  // For static schemas (non-Lookup 59), check the cache first
  if (!isLookup59Schema && _tableDefCache.has(normalised)) {
    return _tableDefCache.get(normalised);
  }

  // If lookups aren't loaded yet, trigger the fetch and wait for it
  let schemas = getTabulatorSchemas();
  if (!schemas) {
    log(Subsystems.MANAGER, Status.INFO,
      '[LithiumTable] Tabulator schemas not in cache, triggering lookup fetch...');
    await fetchLookups();
    schemas = getTabulatorSchemas();
  }

  if (schemas) {
    let matchedEntry = null;
    let matchedData = null;

    if (keyIdx != null) {
      const keyedEntry = schemas.find(s => String(s.key_idx) === String(keyIdx));
      const keyedData = parseSchemaCollection(keyedEntry?.collection);
      if (keyedEntry && keyedData) {
        matchedEntry = keyedEntry;
        matchedData = keyedData;
      }
    }

    if (!matchedData) {
      const fallbackMatch = findSchemaEntryByCandidates(schemas, normalised);
      matchedEntry = fallbackMatch.entry;
      matchedData = fallbackMatch.data;
    }

    if (matchedEntry && matchedData) {
      // Only cache static schemas. Lookup 59 schemas are dynamic and should
      // be fetched fresh each time to reflect any changes made in the database.
      if (!isLookup59Schema) {
        _tableDefCache.set(normalised, matchedData);
      }
      log(Subsystems.MANAGER, Status.INFO,
        `[LithiumTable] Loaded tabledef "${matchedData.title || normalised}" ` +
        `(${Object.keys(matchedData.columns || {}).length} columns, from lookup key ${matchedEntry.key_idx})`);
      return matchedData;
    }
  }

  log(Subsystems.MANAGER, Status.WARN,
    `[LithiumTable] No tabledef found for "${normalised}", using defaults`);
  return null;
}

/**
 * Load a lookup table by reference (e.g., "a27").
 * The lookup data is fetched from the Hydrogen API using QueryRef 34.
 *
 * @param {string} lookupRef - Lookup table reference (e.g., "a27")
 * @param {Function} authQueryFn - The authQuery function from conduit.js (must be provided)
 * @param {Object} api - The Conduit API instance (must be provided)
 * @returns {Promise<Array<{id: number, label: string}>>} Array of {id, label} objects
 */
export async function loadLookup(lookupRef, authQueryFn, api) {
  if (!authQueryFn || !api) {
    log(Subsystems.MANAGER, Status.WARN,
      `[LithiumTable] Cannot load lookup "${lookupRef}": authQuery function and API instance required`);
    return [];
  }

  if (_lookupCache.has(lookupRef)) {
    return _lookupCache.get(lookupRef);
  }

  try {
    // Parse numeric lookup ID from reference (e.g., "a27" -> 27, "g42" -> 42)
    // Supports optional alphabetic prefix (a=Acuranzo, g=Gaius, etc.)
    const match = lookupRef.match(/^[a-z]?(\d+)$/i);
    if (!match) {
      log(Subsystems.MANAGER, Status.ERROR,
        `[LithiumTable] Invalid lookup reference format: "${lookupRef}"`);
      return [];
    }
    const lookupId = parseInt(match[1], 10);

    // QueryRef 34: Get Lookup List - takes LOOKUPID as INTEGER parameter
    const rows = await authQueryFn(api, 34, {
      INTEGER: { LOOKUPID: lookupId },
    });

    // Transform API response to {id, label} format
    // QueryRef 34 returns: lookup_id, key_idx, value_txt, value_int, sort_seq, etc.
    // We map key_idx → id (the lookup entry ID) and value_txt → label (display text)
    const lookupData = rows.map(row => ({
      id: row.key_idx ?? row.lookup_id ?? row.id ?? row.value,
      label: row.value_txt ?? row.lookup_label ?? row.label ?? row.name ?? String(row.key_idx ?? row.lookup_id ?? row.id ?? row.value),
    }));

    _lookupCache.set(lookupRef, lookupData);
    log(Subsystems.MANAGER, Status.INFO,
      `[LithiumTable] Loaded lookup "${lookupRef}" (${lookupData.length} entries)`);
    return lookupData;
  } catch (err) {
    log(Subsystems.MANAGER, Status.ERROR,
      `[LithiumTable] Failed to load lookup "${lookupRef}": ${err.message}`);
    return [];
  }
}

/**
 * Get cached lookup data by reference.
 *
 * @param {string} lookupRef - Lookup table reference
 * @returns {Array<{id: number, label: string}>|undefined}
 */
export function getLookup(lookupRef) {
  return _lookupCache.get(lookupRef);
}

/**
 * Resolve a lookup ID to its label.
 *
 * @param {number} id - The lookup ID to resolve
 * @param {string} lookupRef - Lookup table reference
 * @returns {string} The label, or the original ID if not found
 */
export function resolveLookupLabel(id, lookupRef) {
  if (id == null) return '';
  const lookupData = _lookupCache.get(lookupRef);
  if (!lookupData) return String(id);
  const entry = lookupData.find(item => item.id == id);
  return entry ? entry.label : String(id);
}

/**
 * Create a lookup formatter function for a specific lookup table.
 *
 * @param {string} lookupRef - Lookup table reference (e.g., "a27")
 * @returns {Function} A Tabulator formatter function
 */
export function createLookupFormatter(lookupRef) {
  return function(cell, _onRendered) {
    const value = cell.getValue();
    if (value == null || value === '') return '';
    return resolveLookupLabel(value, lookupRef);
  };
}

/**
 * Create a lookup editor (list) for a specific lookup table.
 *
 * @param {string} lookupRef - Lookup table reference (e.g., "a27")
 * @param {Array<{id: number, label: string}>} lookupData - Pre-loaded lookup data
 * @returns {Object} Tabulator editor configuration
 */
export function createLookupEditor(lookupRef, lookupData) {
  if (!lookupData || lookupData.length === 0) {
    return 'input'; // Fallback to plain input if no lookup data
  }

  // Convert to Tabulator list format: "label" for display, true/false for preselect
  const values = lookupData.map(entry => entry.label);

  return {
    editor: 'list',
    editorParams: {
      values: values,
      autocomplete: true,
      listOnEmpty: true,
      freetext: false,
      allowEmpty: true,
    },
    // Custom formatter to show label instead of raw value
    formatter: createLookupFormatter(lookupRef),
  };
}

/**
 * Pre-load multiple lookups at once.
 *
 * @param {Array<string>} lookupRefs - Array of lookup references
 * @param {Function} authQueryFn - The authQuery function from conduit.js
 * @param {Object} api - The Conduit API instance
 * @returns {Promise<Map<string, Array>>} Map of lookupRef -> data
 */
export async function preloadLookups(lookupRefs, authQueryFn, api) {
  const results = new Map();
  await Promise.all(
    lookupRefs.map(async (ref) => {
      const data = await loadLookup(ref, authQueryFn, api);
      results.set(ref, data);
    })
  );
  return results;
}

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
  const coltype = coltypes[colDef.coltype] || {};
  const overrides = colDef.overrides || {};

  // Start with coltype defaults, then apply overrides
  const merged = { ...coltype, ...overrides };
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
    title: colDef.display,
    field: colDef.field,
    visible: colDef.visible !== false,

    // Store Lithium metadata for template export and runtime re-application
    coltype: colDef.coltype || 'string',
    lithiumColtype: colDef.coltype || 'string',
    editable: colDef.editable === true,
    lithiumEditable: colDef.editable === true,
    lithiumSort: colDef.sort !== false,
    lithiumFilter: colDef.filter !== false,
    lithiumCalculated: colDef.calculated === true,
    lithiumPrimaryKey: colDef.primaryKey === true,
    lithiumDescription: colDef.description || `${colDef.display || colDef.field} column`,

    // Alignment
    hozAlign: merged.align || 'left',
    vertAlign: merged.vertAlign || 'middle',

    // Sorting
    headerSort: colDef.sort !== false,
    headerSortTristate: true,
    sorter: merged.sorter || undefined,
  };

  // Sorter params (e.g. date format)
  if (merged.sorterParams) {
    tabulatorCol.sorterParams = merged.sorterParams;
  }

  // Formatter — lookup columns use the cached lookup formatter;
  // all other columns wrap with blank/zero handler.
  if (colDef.lookupRef && _lookupCache.has(colDef.lookupRef)) {
    // Lookup formatter resolves integer IDs → human-readable labels
    tabulatorCol.formatter = createLookupFormatter(colDef.lookupRef);
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
  if (isEditable && colDef.lookupRef && _lookupCache.has(colDef.lookupRef)) {
    const lookupData = _lookupCache.get(colDef.lookupRef);
    const lookupEditorConfig = createLookupEditor(colDef.lookupRef, lookupData);
    if (typeof lookupEditorConfig === 'object') {
      tabulatorCol.editor = lookupEditorConfig.editor;
      tabulatorCol.editorParams = lookupEditorConfig.editorParams;
    } else {
      tabulatorCol.editor = lookupEditorConfig; // 'input' fallback
    }
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
  if (tableDef.groupBy) opts.groupBy = tableDef.groupBy;
  if (tableDef.groupStartOpen != null) opts.groupStartOpen = tableDef.groupStartOpen;
  if (tableDef.groupToggleElement) opts.groupToggleElement = tableDef.groupToggleElement;
  if (tableDef.movableRows === true) opts.movableRows = true;
  if (tableDef.persistSort != null) opts.persistSort = tableDef.persistSort;
  if (tableDef.persistFilter != null) opts.persistFilter = tableDef.persistFilter;

  return opts;
}

/**
 * Get the primary key field name from a table definition.
 *
 * @param {Object} tableDef - Parsed table definition
 * @returns {string|null} The field name of the primary key column, or null
 */
export function getPrimaryKeyField(tableDef) {
  if (!tableDef?.columns) return null;

  for (const [, colDef] of Object.entries(tableDef.columns)) {
    if (colDef.primaryKey === true) {
      return colDef.field;
    }
  }
  return null;
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

// ── Formatter Wrapper ───────────────────────────────────────────────────────

/**
 * Format a value using the logic of a built-in Tabulator formatter.
 *
 * When wrapFormatter() wraps a built-in formatter (string name like "number"),
 * it can't delegate to Tabulator's internal formatter for non-blank/non-zero
 * values — Tabulator sees a function and uses its return value directly.
 * This utility replicates the formatting for common built-in formatters so
 * the wrapper produces identical output.
 *
 * Supported: "number", "money", "plaintext", "lookup".
 * Unsupported formatters fall through and return the raw value.
 *
 * @param {*}      value         - The cell value to format
 * @param {string} formatterName - Built-in Tabulator formatter name
 * @param {Object} params        - formatterParams for the formatter
 * @returns {string|*} Formatted display value
 */
export function formatBuiltinValue(value, formatterName, params) {
  switch (formatterName) {
    case 'number': {
      if (value == null || value === '') return '';
      const num = Number(value);
      if (isNaN(num)) return String(value);

      const precision = params?.precision ?? 0;
      let formatted = num.toFixed(precision);

      // Thousands separator
      if (params?.thousand) {
        const parts = formatted.split('.');
        parts[0] = parts[0].replace(/\B(?=(\d{3})+(?!\d))/g, params.thousand);
        formatted = parts.join('.');
      }

      // Decimal character override
      if (params?.decimal && formatted.includes('.')) {
        formatted = formatted.replace('.', params.decimal);
      }

      // Prefix / suffix
      if (params?.prefix) formatted = params.prefix + formatted;
      if (params?.suffix) formatted = formatted + params.suffix;

      return formatted;
    }

    case 'money': {
      if (value == null || value === '') return '';
      const num = Number(value);
      if (isNaN(num)) return String(value);

      const precision = params?.precision ?? 2;
      let formatted = num.toFixed(precision);

      // Thousands separator
      if (params?.thousand) {
        const parts = formatted.split('.');
        parts[0] = parts[0].replace(/\B(?=(\d{3})+(?!\d))/g, params.thousand);
        formatted = parts.join(params?.decimal || '.');
      } else if (params?.decimal && formatted.includes('.')) {
        formatted = formatted.replace('.', params.decimal);
      }

      // Currency symbol
      if (params?.symbol) {
        formatted = params.symbolAfter
          ? formatted + params.symbol
          : params.symbol + formatted;
      }

      return formatted;
    }

    case 'plaintext':
      return value != null ? String(value) : '';

    case 'lookup': {
      if (value == null || value === '') return '';
      const lookup = params?.lookup || {};
      return lookup[value] !== undefined ? lookup[value] : String(value);
    }

    default:
      // For formatters we don't replicate (datetime, tickCross, link, etc.),
      // return the raw value. The built-in formatting won't apply when wrapped,
      // but this is an acceptable fallback — the value is still displayed.
      return value;
  }
}

/**
 * Wrap a Tabulator formatter to intercept blank and zero values.
 *
 * - If the cell value is null, undefined, or "" → return blankValue
 * - If the cell value is 0 and zeroValue differs from null → return zeroValue
 * - Otherwise → delegate to the original formatter
 *
 * For built-in formatters (string names like "number"), the wrapper
 * replicates the formatting via formatBuiltinValue() so that thousand
 * separators, precision, currency symbols, etc. are preserved.
 *
 * @param {string|Function} formatter     - Tabulator formatter name or function
 * @param {Object}          formatterParams - Params for the formatter
 * @param {*}               blankValue    - What to show for blank values
 * @param {*}               zeroValue     - What to show for zero (null = show '0')
 * @returns {string|Function} The original formatter (when no wrapping needed)
 *                            or a Tabulator formatter function
 */
export function wrapFormatter(formatter, formatterParams, blankValue, zeroValue) {
  // If no special handling needed, return the original formatter as-is
  if (!needsBlankZeroWrapper(blankValue, zeroValue)) {
    return formatter;
  }

  // Return a wrapper function that handles blank/zero before delegating
  return function(cell, onRendered) {
    const value = cell.getValue();

    // Handle blank (null, undefined, empty string)
    if (value === null || value === undefined || value === '') {
      if (blankValue != null) return blankValue;
      if (blankValue === '') return '';
    }

    // Handle zero
    if (value === 0 && zeroValue !== null && zeroValue !== undefined) {
      return zeroValue;
    }

    // For built-in formatters (string name), replicate their formatting
    // so the wrapper produces the same output (thousand seps, precision, etc.)
    if (typeof formatter === 'string') {
      return formatBuiltinValue(value, formatter, formatterParams);
    }

    // For custom formatter functions, call them directly
    if (typeof formatter === 'function') {
      return formatter(cell, formatterParams, onRendered);
    }

    // Fallback: return raw value
    return value;
  };
}

/**
 * Check if a column needs the blank/zero wrapper at all.
 * If blank is null and zero is null, no wrapper needed.
 *
 * @param {*} blankValue - coltype/override blank value
 * @param {*} zeroValue  - coltype/override zero value
 * @returns {boolean}
 */
export function needsBlankZeroWrapper(blankValue, zeroValue) {
  // blank="" means "show nothing for blanks" — that's active
  // blank=null means "no special handling" — inactive
  // zero="" means "show nothing for zeros" — active
  // zero=null means "no special handling (show 0)" — inactive
  return blankValue !== null || zeroValue !== null;
}

// ── Cache Management ────────────────────────────────────────────────────────

/**
 * Clear all cached configuration data.
 * Useful for testing or when configs change at runtime.
 */
export function clearCache() {
  _coltypesCache = null;
  _tableDefCache.clear();
  _lookupCache.clear();
}

/**
 * Clear cached table definitions that come from Lookup 59 (Tabulator Schemas).
 * This should be called when Lookup 59 data is refreshed to ensure
 * table definitions are re-fetched from the updated lookup data.
 */
export function clearLookup59Cache() {
  const lookup59Paths = [
    'queries/query-manager',
    'lookups/lookups-list',
    'lookups/lookup-values',
    'style-manager/lookup-41',
    'version-manager/version-history',
  ];

  let clearedCount = 0;
  lookup59Paths.forEach((path) => {
    if (_tableDefCache.has(path)) {
      _tableDefCache.delete(path);
      clearedCount++;
    }
  });

  if (clearedCount > 0) {
    log(Subsystems.MANAGER, Status.DEBUG,
      `[LithiumTable] Cleared ${clearedCount} Lookup 59 table definition(s) from cache`);
  }
}

// ── Event Listeners ─────────────────────────────────────────────────────────

// Clear Lookup 59 table definition cache when lookups are refreshed from server.
// This ensures table schemas from Lookup 059 (Tabulator Schemas) are re-fetched
// and reflect any changes made to the lookup data.
eventBus.on(Events.LOOKUPS_LOADED, (e) => {
  if (e?.detail?.source === 'server' || e?.detail?.source === 'cache_fallback') {
    clearLookup59Cache();
  }
});
