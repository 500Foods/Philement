import { TabulatorFull } from './tabulator_esm-DsDetXL7.js';
import { a as authQuery } from './conduit-D2QlMBxM.js';
import { A as getTabulatorSchemas, l as log, S as Status, a as Subsystems, B as fetchLookups, e as eventBus, E as Events, p as processIcons, d as getTip, C as tip, t as toast, D as refreshTabulatorSchemas } from './index-DLPCDk6c.js';
import { s as scrollbarManager } from './scrollbar-manager-DTR0NQaT.js';

/**
 * Column Type (coltype) Loader
 *
 * Loads and caches coltype definitions from Lookup 59 or provided data.
 *
 * @module tables/resolution/coltype-loader
 */


/** @type {Object|null} Cached coltypes.json data (loaded once) */
let _coltypesCache = null;

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
async function loadColtypes(providedData) {
  if (_coltypesCache) return _coltypesCache;

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
 * Table Definition Validator
 *
 * Validates tableDef structures against canonical property lists.
 *
 * @module tables/resolution/validator
 */


/** @type {Set<string>} Valid table-level properties */
const TABLEDEF_VALID_PROPS = new Set([
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
const COLUMN_VALID_PROPS = new Set([
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
  'columnPri', 'primaryKey', 'calculated', 'sort', 'filter', 'editable',
  'groupable', 'groupPri', 'groupDir', 'sortPri', 'sortDir', 'filterPri',
  'lookupRef', 'lookupFixed', 'lookupStyle', 'lookupEdit', 'groupStyle', 'groupTitle', 'blank', 'zero',
]);

/** @type {Set<string>} Valid coltype names */
const VALID_COLTYPES = new Set([
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
function validateTableDef(tableDef, stageName = 'unknown') {
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

/**
 * Table Definition Loader
 *
 * Loads and caches table definitions from Lookup 59 or provided data.
 *
 * @module tables/resolution/tabledef-loader
 */


/** @type {Map<string, Object>} Cached table definitions keyed by path */
const _tableDefCache = new Map();

// Table path to Lookup 59 key_idx mapping
const TABLE_TO_KEY_IDX = {
  'column-manager': 2,             // acuranzo_1179
  'column-manager-manager': 3,      // acuranzo_1180
  'user-profile-sections': 4,          // acuranzo_1181
  'profile-manager/user-options': 4,   // acuranzo_1181 - Profile Manager user options
  'queries/query-manager': 5,        // acuranzo_1182
  'lookups/lookups-list': 6,      // acuranzo_1183 (legacy path)
  'lookups-manager-list': 6,       // acuranzo_1183
  'lookups/lookup-values': 7,     // acuranzo_1184 (legacy path)
  'lookups-manager-values': 7,    // acuranzo_1184
  'style-manager-list': 8,        // acuranzo_1185
  'style-manager-sections': 9,     // acuranzo_1186
  'version-manager': 10,          // acuranzo_1187
};

/**
 * Parse a schema collection JSON string or object.
 * @param {*} collection - The collection to parse
 * @returns {Object|null}
 */
function parseSchemaCollection(collection) {
  if (!collection) return null;

  try {
    return typeof collection === 'string' ? JSON.parse(collection) : collection;
  } catch {
    return null;
  }
}

/**
 * Build a set of candidate paths for table matching.
 * Handles both "-list" suffix variants.
 * @param {string} tablePath - The table path
 * @returns {Set<string>} Candidate paths
 */
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

/**
 * Find a schema entry by candidate path matching.
 * @param {Array} schemas - Array of schema entries
 * @param {string} tablePath - The table path to match
 * @returns {{entry: Object|null, data: Object|null}}
 */
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

/**
 * Create an auto-discover table definition from data.
 * This is used when Lookup 59 doesn't have a schema or has invalid JSON.
 *
 * @param {string} tablePath - The table path (used for title)
 * @returns {Object} A table definition with discoverable columns
 */
function createAutoDiscoverTableDef(tablePath) {
  const title = tablePath.split('/').pop() || 'Table';
  return {
    title: title,
    readonly: false,
    layout: 'fitColumns',
    responsiveLayout: false,
    _autoDiscover: true,  // Flag to indicate columns should be discovered
    columns: {},  // Empty columns - will be discovered from data
  };
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
 * @param {number} [lookupKeyIdx] - Optional direct Lookup 59 key_idx.
 *   When provided, this takes precedence over the tablePath mapping.
 *   Use this for unambiguous schema loading from Lookup 59.
 * @returns {Promise<Object>} The parsed table definition
 */
async function loadTableDef(tablePath, providedData, lookupKeyIdx) {
  // Normalise path: strip trailing .json if present
  const normalised = tablePath.replace(/\.json$/, '');

  // ── Priority 2: Direct lookupKeyIdx (overrides path mapping) ───────────
  // When lookupKeyIdx is provided directly, use it without ambiguity
  const keyIdx = lookupKeyIdx ?? null;
  const isLookup59Schema = lookupKeyIdx != null;

  // ── Priority 3: Lookup cache (from database via QueryRef 060) ────────
  // Map table paths to lookup key_idx values (Lookup 059 - Tabulator Schemas)
  // Only used when lookupKeyIdx is not provided
  if (!isLookup59Schema) {
    const mappedKeyIdx = TABLE_TO_KEY_IDX[normalised];
    if (mappedKeyIdx != null) {
      return loadTableDef(tablePath, null, mappedKeyIdx);
    }
  }

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

  // If a direct key_idx was requested but schemas still not available,
  // this may be the first time we're trying to load them. Try force-refresh.
  if (!schemas && keyIdx != null) {
    log(Subsystems.MANAGER, Status.INFO,
      `[LithiumTable] Schemas not loaded yet, force-fetching for key_idx ${keyIdx}...`);
    await fetchLookups({ force: true, silent: false });
    schemas = getTabulatorSchemas();
  }

  if (schemas) {
    let matchedEntry = null;
    let matchedData = null;

    if (keyIdx != null) {
      const keyedEntry = schemas.find(s => String(s.key_idx) === String(keyIdx));
      if (!keyedEntry) {
        log(Subsystems.MANAGER, Status.WARN,
          `[LithiumTable] Lookup 59 key_idx ${keyIdx} not found, using auto-discovery`);
        return createAutoDiscoverTableDef(normalised);
      }
      const keyedData = parseSchemaCollection(keyedEntry?.collection);
      if (!keyedData) {
        log(Subsystems.MANAGER, Status.WARN,
          `[LithiumTable] Lookup 59 key_idx ${keyIdx} has invalid/empty JSON, using auto-discovery`);
        return createAutoDiscoverTableDef(normalised);
      }
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
      const validated = validateTableDef(matchedData, `lookup-${matchedEntry.key_idx}`);
      if (!validated.valid) {
        validated.errors.forEach(err => log(Subsystems.MANAGER, Status.WARN, err));
      }
      log(Subsystems.MANAGER, Status.INFO,
        `[LithiumTable] Loaded tabledef "${matchedData.title || normalised}" ` +
        `(${Object.keys(matchedData.columns || {}).length} columns, from lookup key ${matchedEntry.key_idx})`);
      return matchedData;
    }
  }

  log(Subsystems.MANAGER, Status.WARN,
    `[LithiumTable] No tabledef found for "${normalised}", using auto-discovery`);
  return createAutoDiscoverTableDef(normalised);
}

/**
 * Clear all cached table definitions.
 */
function clearTableDefCache() {
  _tableDefCache.clear();
}

/**
 * Clear cached table definitions that come from Lookup 59 (Tabulator Schemas).
 * This should be called when Lookup 59 data is refreshed to ensure
 * table definitions are re-fetched from the updated lookup data.
 */
function clearLookup59Cache() {
  const lookup59Paths = [
    'queries/query-manager',
    'lookups/lookups-list',
    'lookups/lookup-values',
    'lookups-manager-list',
    'lookups-manager-values',
    'style-manager/lookup-41',
    'version-manager/version-history',
    'profile-manager/user-options',
    'user-profile-sections',
    'column-manager',
  ];

  let clearedCount = 0;
  lookup59Paths.forEach((path) => {
    if (_tableDefCache.has(path)) {
      _tableDefCache.delete(path);
      clearedCount++;
    }
  });

  log(Subsystems.MANAGER, Status.DEBUG,
    `[LithiumTable] Cleared ${clearedCount} Lookup 59 table definition(s) from cache`);
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

// Also clear cache when a specific lookup category is refreshed (e.g., tabulator_schemas)
eventBus.on(Events.LOOKUPS_REFRESHED, () => {
  clearLookup59Cache();
});

// Clear all table definition caches on auth logout to ensure fresh data on next login.
// This makes the cache session-only as required.
eventBus.on(Events.AUTH_LOGOUT, () => {
  clearTableDefCache();
  log(Subsystems.MANAGER, Status.DEBUG, '[LithiumTable] Cleared all table definition caches on auth logout');
});

/**
 * Lookup Data Loader
 *
 * Loads and caches lookup table data from the Hydrogen API.
 *
 * @module tables/resolution/lookup-loader
 */


/** @type {Map<string, Array<{id: number, label: string, icon?: string, sortSeq?: number}>>} Cached lookup tables */
const _lookupCache = new Map();

/**
 * Load a lookup table by reference (e.g., 27 or "a27").
 * The lookup data is fetched from the Hydrogen API using QueryRef 34.
 *
 * @param {string|number} lookupRef - Lookup table ID (integer preferred, e.g., 27)
 * @param {Function} authQueryFn - The authQuery function from conduit.js (must be provided)
 * @param {Object} api - The Conduit API instance (must be provided)
 * @returns {Promise<Array<{id: number, label: string}>>} Array of {id, label} objects
 */
async function loadLookup(lookupRef, authQueryFn, api) {
  // Normalize to string for consistent cache keys (handles both "27" and 27)
  const normalizedRef = String(lookupRef);

  if (!authQueryFn || !api) {
    log(Subsystems.MANAGER, Status.WARN,
      `[LithiumTable] Cannot load lookup "${normalizedRef}": authQuery function and API instance required`);
    return [];
  }

  if (_lookupCache.has(normalizedRef)) {
    return _lookupCache.get(normalizedRef);
  }

  try {
    // Parse numeric lookup ID from reference (e.g., "a27" -> 27, "g42" -> 42, "27" -> 27)
    // Supports optional alphabetic prefix (a=Acuranzo, g=Gaius, etc.)
    const match = normalizedRef.match(/^[a-z]?(\d+)$/i);
    if (!match) {
      log(Subsystems.MANAGER, Status.ERROR,
        `[LithiumTable] Invalid lookup reference format: "${normalizedRef}"`);
      return [];
    }
    const lookupId = parseInt(match[1], 10);

    // QueryRef 34: Get Lookup List - takes LOOKUPID as INTEGER parameter
    const rows = await authQueryFn(api, 34, {
      INTEGER: { LOOKUPID: lookupId },
    });

    // Transform API response to {id, label, icon} format
    // QueryRef 34 returns: lookup_id, key_idx, value_txt, value_int, sort_seq, collection, etc.
    // We map key_idx → id (the lookup entry ID), value_txt → label, collection.icon → icon
    const lookupData = rows.map(row => {
      // Parse collection JSON if present
      let collection = null;
      if (row.collection) {
        try {
          collection = typeof row.collection === 'string'
            ? JSON.parse(row.collection)
            : row.collection;
        } catch (_e) {
          // Invalid JSON, ignore
        }
      }

      return {
        id: row.key_idx ?? row.lookup_id ?? row.id ?? row.value,
        label: row.value_txt ?? row.lookup_label ?? row.label ?? row.name ?? String(row.key_idx ?? row.lookup_id ?? row.id ?? row.value),
        icon: collection?.icon || null,  // HTML string: "<fa fa-check></fa>" or "<i class='...'></i>"
        collection: collection,          // Full collection object for future use
        sortSeq: row.sort_seq ?? 0,      // Sort order from database
      };
    });

    _lookupCache.set(normalizedRef, lookupData);
    log(Subsystems.MANAGER, Status.INFO,
      `[LithiumTable] Loaded lookup "${normalizedRef}" (${lookupData.length} entries)`);
    return lookupData;
  } catch (err) {
    log(Subsystems.MANAGER, Status.ERROR,
      `[LithiumTable] Failed to load lookup "${normalizedRef}": ${err.message}`);
    return [];
  }
}

/**
 * Get cached lookup data by reference.
 *
 * @param {string|number} lookupRef - Lookup table reference (e.g., 27 or "a27")
 * @returns {Array<{id: number, label: string}>|undefined}
 */
function getLookup(lookupRef) {
  return _lookupCache.get(String(lookupRef));
}

/**
 * Resolve a lookup ID to its label.
 *
 * @param {number} id - The lookup ID to resolve
 * @param {string|number} lookupRef - Lookup table reference
 * @returns {string} The label, or the original ID if not found
 */
function resolveLookupLabel(id, lookupRef) {
  if (id == null) return '';
  const lookupData = _lookupCache.get(String(lookupRef));
  if (!lookupData) return String(id);
  const entry = lookupData.find(item => item.id == id);
  return entry ? entry.label : String(id);
}

/**
 * Create a lookup formatter function for a specific lookup table.
 *
 * @param {string|number} lookupRef - Lookup table reference (e.g., 27 or "a27")
 * @returns {Function} A Tabulator formatter function
 */
function createLookupFormatter(lookupRef) {
  return function(cell, _onRendered) {
    const value = cell.getValue();
    if (value == null || value === '') return '';
    return resolveLookupLabel(value, lookupRef);
  };
}

/**
 * Create an icon-only formatter for a specific lookup table.
 * Displays just the icon (from collection.icon) instead of the label.
 *
 * @param {Array<{id: number, label: string, icon: string}>} lookupData - Pre-loaded lookup data
 * @returns {Function} A Tabulator formatter function
 */
function createIconFormatter(lookupData) {
  return function(cell, _onRendered) {
    const value = cell.getValue();
    // Allow 0 as a valid lookup value (check for null/undefined/empty only)
    if (value == null || value === '') {
      return '';
    }
    const entry = lookupData.find(item => item.id == value);
    if (!entry?.icon) return '';
    return entry.icon;  // Return raw HTML (e.g., "<fa fa-check></fa>")
  };
}

/**
 * Create an icon + label formatter for a specific lookup table.
 * Displays both the icon and the label text side by side.
 *
 * @param {Array<{id: number, label: string, icon: string}>} lookupData - Pre-loaded lookup data
 * @returns {Function} A Tabulator formatter function
 */
function createIconLabelFormatter(lookupData) {
  return function(cell, _onRendered) {
    const value = cell.getValue();
    if (value == null || value === '') {
      return '';
    }
    const entry = lookupData.find(item => item.id == value);
    if (!entry) return String(value);

    const iconHtml = entry.icon || '';
    const label = entry.label || String(value);

    // Wrap in spans to isolate FontAwesome mutations and provide styling hooks
    return `<span class="li-lookup-icon-label"><span class="li-lookup-icon">${iconHtml}</span><span class="li-lookup-label">${label}</span></span>`;
  };
}

/**
 * Create a lookup editor (list) for a specific lookup table.
 *
 * @param {string|number} lookupRef - Lookup table reference (e.g., 27 or "a27")
 * @param {Array<{id: number, label: string}>} lookupData - Pre-loaded lookup data
 * @returns {Object} Tabulator editor configuration
 */
function createLookupEditor(lookupRef, lookupData) {
  if (!lookupData || lookupData.length === 0) {
    return {
      editor: 'number',
      editorParams: { min: 0 }
    };
  }

  // Sort lookup data by: sortSeq ascending, then label ascending, then id ascending
  const sortedData = [...lookupData].sort((a, b) => {
    // First: sort_seq (default 0)
    const sortDiff = (a.sortSeq ?? 0) - (b.sortSeq ?? 0);
    if (sortDiff !== 0) return sortDiff;

    // Second: label (case-insensitive)
    const labelA = String(a.label ?? '').toLowerCase();
    const labelB = String(b.label ?? '').toLowerCase();
    if (labelA !== labelB) return labelA.localeCompare(labelB);

    // Third: id (numeric)
    return (a.id ?? 0) - (b.id ?? 0);
  });

  // Build ID -> label map for Tabulator list editor
  // This ensures selecting stores the ID, not the label text
  const values = {};
  sortedData.forEach(entry => {
    values[entry.id] = entry.label;
  });

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
async function preloadLookups(lookupRefs, authQueryFn, api) {
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
 * Get a sortable value for a lookup ID.
 * Returns a tuple [sortSeq, label, id] for proper sorting.
 *
 * @param {number} id - The lookup ID
 * @param {string|number} lookupRef - Lookup table reference
 * @returns {Array} [sortSeq, label, id] tuple for sorting
 */
function getLookupSortValue(id, lookupRef) {
  if (id == null) return [0, '', 0];
  const lookupData = _lookupCache.get(String(lookupRef));
  if (!lookupData) return [0, String(id), Number(id) || 0];
  const entry = lookupData.find(item => item.id == id);
  if (!entry) return [0, String(id), Number(id) || 0];
  return [
    entry.sortSeq ?? 0,
    entry.label ?? '',
    entry.id ?? 0,
  ];
}

/**
 * Compare two lookup IDs for sorting.
 * Sorts by: sortSeq ascending, then label ascending (case-insensitive), then id ascending.
 *
 * @param {number} aId - First lookup ID
 * @param {number} bId - Second lookup ID
 * @param {string|number} lookupRef - Lookup table reference
 * @param {string} dir - Sort direction ('asc' or 'desc')
 * @returns {number} Comparison result (-1, 0, 1)
 */
function compareLookupValues(aId, bId, lookupRef, dir = 'asc') {
  const aVal = getLookupSortValue(aId, lookupRef);
  const bVal = getLookupSortValue(bId, lookupRef);

  // Compare sortSeq first
  if (aVal[0] !== bVal[0]) {
    return dir === 'asc' ? aVal[0] - bVal[0] : bVal[0] - aVal[0];
  }

  // Then compare label (case-insensitive)
  const labelA = String(aVal[1]).toLowerCase();
  const labelB = String(bVal[1]).toLowerCase();
  if (labelA !== labelB) {
    const cmp = labelA.localeCompare(labelB);
    return dir === 'asc' ? cmp : -cmp;
  }

  // Finally compare id
  const idDiff = aVal[2] - bVal[2];
  return dir === 'asc' ? idDiff : -idDiff;
}

/**
 * Create a custom sorter function for lookup columns.
 * This sorter compares lookup values by their display label rather than raw ID.
 *
 * @param {string|number} lookupRef - Lookup table reference
 * @returns {Function} Tabulator sorter function
 */
function createLookupSorter(lookupRef) {
  return function(a, b, aRow, bRow, column, dir) {
    // a and b are the raw cell values (lookup IDs)
    return compareLookupValues(a, b, lookupRef, dir);
  };
}

/**
 * Formatter Utilities and Custom Calculations
 *
 * Provides formatter wrappers and custom calculation functions
 * that properly handle blank and zero values.
 *
 * @module tables/resolution/formatters
 */


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
function formatBuiltinValue(value, formatterName, params) {
  switch (formatterName) {
    case 'number': {
      if (value == null || value === '') return '';
      const num = Number(value);
      if (isNaN(num)) return String(value);

      const precision = params?.precision ?? 0;
      let formatted = num.toFixed(precision);

      // Thousands separator - check for !== undefined (empty string "" disables separator)
      if (params?.thousand !== undefined) {
        const parts = formatted.split('.');
        if (params.thousand) {
          parts[0] = parts[0].replace(/\B(?=(\d{3})+(?!\d))/g, params.thousand);
        }
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

      // Thousands separator - check for !== undefined (empty string "" disables separator)
      if (params?.thousand !== undefined) {
        const parts = formatted.split('.');
        if (params.thousand) {
          parts[0] = parts[0].replace(/\B(?=(\d{3})+(?!\d))/g, params.thousand);
        }
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

    case 'html':
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
 * Create a custom HTML formatter that processes Lithium icons.
 * This formatter renders HTML content and calls processIcons on the cell
 * to convert <fa> tags to Font Awesome SVG icons.
 *
 * NOTE: This formatter relies on the global MutationObserver in icons.js
 * to process <fa> tags. The onRendered callback ensures icons are processed
 * after the cell is added to the DOM.
 *
 * @returns {Function} Tabulator formatter function
 */
function createHtmlFormatter() {
  return function(cell, _formatterParams, onRendered) {
    const value = cell.getValue();
    if (value == null || value === '') return '';

    // Process icons after the cell is rendered and added to DOM
    // This ensures the MutationObserver in icons.js catches the new <fa> elements
    if (typeof onRendered === 'function') {
      onRendered(function() {
        // Double requestAnimationFrame ensures DOM is fully updated
        requestAnimationFrame(() => {
          requestAnimationFrame(() => {
            const cellEl = cell.getElement();
            if (cellEl) {
              processIcons(cellEl);
            }
          });
        });
      });
    }

    return String(value);
  };
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
function wrapFormatter(formatter, formatterParams, blankValue, zeroValue) {
  // If no special handling needed, return the original formatter as-is
  if (!needsBlankZeroWrapper(blankValue, zeroValue)) {
    return formatter;
  }

  // For 'html' formatter, use our custom formatter that processes icons
  if (formatter === 'html') {
    return createHtmlFormatter();
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
function needsBlankZeroWrapper(blankValue, zeroValue) {
  // blank="" means "show nothing for blanks" — that's active
  // blank=null means "no special handling" — inactive
  // zero="" means "show nothing for zeros" — active
  // zero=null means "no special handling (show 0)" — inactive
  return blankValue !== null || zeroValue !== null;
}

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
function lookupFormatter(cell, formatterParams) {
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
if (typeof TabulatorFull !== 'undefined' && TabulatorFull.registerFormatter) {
  TabulatorFull.registerFormatter('lookup', lookupFormatter);
}

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
function resolveColumn(fieldName, colDef, coltypes, options = {}) {
  // CRITICAL: Start with coltypes.default, then overlay specific coltype, then column definition
  // This is the foundation - all coltypes inherit from the "default" stanza
  // Merge order: default → type-specific → column definition (later stages override earlier)

  const defaultCol = coltypes.default || {};
  const typeCol = coltypes[colDef.coltype] || {};

  // Merge strategy: coltypes.default → coltype specific → column definition
  const merged = { ...defaultCol, ...typeCol, ...colDef };

  // Extract Lithium-specific metadata (not passed to Tabulator)
  ({
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
  });
  for (const [key, value] of Object.entries(merged)) {
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
    // Use custom HTML formatter for 'html' type to process Lithium icons.
    // Also treat the dedicated html coltype as HTML output even if the
    // formatter inherited from coltypes.default is still plaintext.
    if (merged.formatter === 'html' || colDef.coltype === 'html') {
      tabulatorCol.formatter = createHtmlFormatter();
    } else {
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
function resolveColumns(tableDef, coltypes, options = {}) {
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
function resolveTableOptions(tableDef) {
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
function getPrimaryKeyField(tableDef) {
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
function getQueryRefs(tableDef) {
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
 * Group Icon Animator Module
 *
 * Handles expand/collapse arrow animation for Tabulator group headers.
 * Uses prior-state pinning pattern because Tabulator regenerates group
 * header DOM on every toggle, so CSS-only transitions don't work.
 *
 * @module tables/visual/group-icon-animator
 */


/**
 * GroupIconAnimator class for managing group toggle icon animations.
 * Instantiated per LithiumTable instance.
 */
class GroupIconAnimator {
  /**
   * Create a GroupIconAnimator
   * @param {Object} table - The LithiumTable instance
   */
  constructor(table) {
    this.table = table;
    // Map<string, boolean> — group path key → last-known visible state
    this._groupVisibilityState = new Map();
    // Set<HTMLElement> — group rows currently running animation
    this._groupAnimating = new Set();
  }

  /**
   * Build a stable key for a Tabulator group component based on its parent
   * chain. Nested groups are separated with "||" so sibling keys can't
   * collide with compound parent keys.
   */
  _groupPathKey(groupComponent) {
    const parts = [];
    let g = groupComponent;
    while (g && typeof g.getKey === 'function') {
      parts.unshift(String(g.getKey()));
      g = typeof g.getParentGroup === 'function' ? g.getParentGroup() : null;
    }
    return parts.join('||');
  }

  /**
   * Recursively collect all group components (including nested sub-groups).
   */
  _collectAllGroups(groupComponents, out = []) {
    for (const gc of groupComponents || []) {
      out.push(gc);
      if (typeof gc.getSubGroups === 'function') {
        const subs = gc.getSubGroups();
        if (subs && subs.length) this._collectAllGroups(subs, out);
      }
    }
    return out;
  }

  /**
   * Synchronise group toggle icons with their current visibility state and
   * animate any groups whose state changed since the last sync.
   *
   * Safe to call repeatedly — unchanged groups are no-ops.
   */
  updateGroupIcons() {
    if (!this.table.table || !this.table.container) return;

    // Defer one frame so Tabulator has finished regenerating the group DOM.
    requestAnimationFrame(() => this._syncGroupIconsNow());
  }

  _syncGroupIconsNow() {
    if (!this.table.table || !this.table.container) return;

    let groupComponents;
    try {
      groupComponents = this.table.table.getGroups?.();
    } catch {
      groupComponents = null;
    }
    if (!groupComponents) return;

    const allGroups = this._collectAllGroups(groupComponents);
    const priorState = this._groupVisibilityState;
    const nextState = new Map();
    const toAnimate = []; // { rowEl, fromVisible }

    for (const gc of allGroups) {
      const key = this._groupPathKey(gc);
      if (!key) continue;

      const rowEl = typeof gc.getElement === 'function' ? gc.getElement() : null;
      if (!rowEl || !this.table.container.contains(rowEl)) continue;

      // Ensure any freshly-inserted <fa> tags in this group header have
      // been converted to <i>. Safe to call repeatedly.
      processIcons(rowEl);

      const isVisible = rowEl.classList.contains('tabulator-group-visible');
      nextState.set(key, isVisible);

      // CRITICAL: if this row is mid-animation, leave it completely alone.
      // Tabulator fires several events for one toggle, and each triggers a sync.
      if (this._groupAnimating.has(rowEl)) continue;

      const hadPrior = priorState.has(key);
      const wasVisible = priorState.get(key);

      // Clean up any leftover anim classes on non-animating rows
      rowEl.classList.remove(
        'li-group-anim-from-collapsed',
        'li-group-anim-from-expanded',
      );

      if (!hadPrior || wasVisible === isVisible) continue;

      toAnimate.push({ rowEl, fromVisible: wasVisible });
    }

    // Commit new state immediately.
    this._groupVisibilityState = nextState;

    if (toAnimate.length === 0) return;

    // Step 1: apply "starting position" class and mark each row as animating
    for (const { rowEl, fromVisible } of toAnimate) {
      this._groupAnimating.add(rowEl);
      rowEl.classList.add(
        fromVisible
          ? 'li-group-anim-from-expanded'
          : 'li-group-anim-from-collapsed',
      );
    }

    // Force a paint of the starting position
    void this.table.container.offsetHeight;

    // Step 2: on the next frame, remove the class for transition to new state
    requestAnimationFrame(() => {
      for (const { rowEl } of toAnimate) {
        rowEl.classList.remove(
          'li-group-anim-from-collapsed',
          'li-group-anim-from-expanded',
        );
        this._groupAnimating.delete(rowEl);
      }
    });
  }

  /**
   * Reset the animation state. Called when table data changes completely.
   */
  reset() {
    this._groupVisibilityState.clear();
    this._groupAnimating.clear();
  }
}

/**
 * Table Events Module
 *
 * Wires Tabulator table events to LithiumTable handlers.
 *
 * @module tables/events/table-events
 */


/**
 * Wire all Tabulator table events
 * @param {Object} table - LithiumTable instance
 */
function wireTableEvents(table) {
  if (!table.table) return;

  const refreshScrollbars = () => table._updateTableScrollbars?.();

  table.table.on('rowClick', (e, row) => {
    if (table.isCalcRow(row)) return;

    // Skip row selection if clicking on drag handle cell (row reordering)
    if (e?.target) {
      const cell = e.target.closest?.('.tabulator-cell');
      if (cell?.getAttribute?.('data-tabulator-field') === 'drag_handle') {
        return;
      }
    }

    // When editing, check if clicking the same row
    // If so, don't re-select (which would trigger handleRowSelected)
    if (table.isEditing) {
      const pkFields = table.primaryKeyField;
      const clickedRowId = table._getCompositeRowId?.(row.getData(), pkFields);
      if (table.editingRowId === clickedRowId) {
        // Same row - just ensure it's selected without triggering row change logic
        if (!row.isSelected?.()) {
          row.select();
        }
        return;
      }
    }

    table.selectDataRow(row);
  });

  table.table.on('cellMouseDown', (e, cell) => {
    const field = cell.getField();
    if (field === '_selector' || field === 'drag_handle' || table.isCalcCell(cell)) return;

    const row = cell.getRow();

    // Only select row on mouse down if not already editing
    // When editing, we handle row changes via cellClick or rowSelected
    if (!table.isEditing) {
      table.selectDataRow(row);
      return;
    }

    // When editing, check if clicking the same row
    // If same row, don't re-select to avoid triggering row change logic
    if (table.isEditing) {
      const pkFields = table.primaryKeyField;
      const clickedRowId = table._getCompositeRowId?.(row.getData(), pkFields);
      if (table.editingRowId === clickedRowId) {
        // Same row - just ensure it's selected without triggering row change logic
        if (!row.isSelected?.()) {
          row.select();
        }
        return;
      }
      // Different row - let the selection proceed (will trigger handleRowSelected)
      table.selectDataRow(row);
    }
  });

  table.table.on('rowDblClick', (e, row) => {
    if (table.isCalcRow(row) || table.readonly || table.alwaysEditable) return;
    table.selectDataRow(row);
    void table.enterEditMode(row);
  });

  table.table.on('cellDblClick', (e, cell) => {
    const row = cell?.getRow?.();
    const field = cell?.getField?.();
    if (!row || table.isCalcRow(row) || table.readonly || table.alwaysEditable) return;
    if (field === '_selector' || field === 'drag_handle' || table.isCalcCell(cell)) return;

    table.selectDataRow(row);
    void table.enterEditMode(row, cell);
  });

  table.table.on('cellClick', (e, cell) => {
    const field = cell.getField();
    if (field === '_selector' || field === 'drag_handle' || table.isCalcCell(cell)) return;

    e?.stopPropagation?.();
    e?.stopImmediatePropagation?.();

    const row = cell.getRow();
    const pkFields = table.primaryKeyField;
    const clickedRowId = table._getCompositeRowId?.(row.getData(), pkFields);
    const isSameRow = table.isEditing && table.editingRowId === clickedRowId;

    // When editing, only select row if clicking a different row
    // (handleRowSelected will auto-save and exit edit mode)
    // For same row, we stay in edit mode and just try to open the cell editor
    if (!table.isEditing || !isSameRow) {
      table.selectDataRow(row);
    }

    if (table.isEditing && isSameRow) {
      // Same row: commit any active edit and open editor for clicked cell
      table.commitActiveCellEdit(cell);
      table.queueCellEdit(cell);
    }
  });

  table.table.on('cellEdited', () => {
    table.isDirty = true;
    table.updateSaveCancelButtonState();
    table.notifyDirtyChange();
  });

  table.table.on('rowSelected', async (row) => {
    if (table.isCalcRow(row)) return;
    await table.handleRowSelected(row);
  });

  table.table.on('rowDeselected', (row) => {
    if (table.isCalcRow(row)) return;
    table.updateSelectorCell(row, false);
  });

  table.table.on('rowSelectionChanged', () => {
    if (table._inSelectionTransition) return;

    const selectedRows = table.table.getSelectedRows();
    if (selectedRows.length > 1) {
      table._inSelectionTransition = true;
      const rowsToDeselect = selectedRows.slice(1);
      table.table.deselectRow(rowsToDeselect);
    }

    table.updateMoveButtonState();
    table.updateDuplicateButtonState();
  });

  table.table.on('columnVisibilityChanged', () => {
    table.updateVisibleColumnClasses();
    table.initColumnHeaderTooltips();
    refreshScrollbars();
  });

  table.table.on('columnMoved', () => {
    table.updateVisibleColumnClasses();
    table.initColumnHeaderTooltips();
    refreshScrollbars();
  });

  table.table.on('tableBuilt', () => {
    refreshScrollbars();
    setTimeout(() => {
      table.updateVisibleColumnClasses();
      table.initColumnHeaderTooltips();
      refreshScrollbars();
    }, 100);
  });

  table.table.on('rowRendered', (row) => {
    table.applyVisibleColumnClassesToRow(row);
    const rowEl = row?.getElement?.();
    if (rowEl) processIcons(rowEl);
  });

  table.table.on('dataLoaded', () => {
    refreshScrollbars();
    setTimeout(() => {
      table.updateVisibleColumnClasses();
      table.initColumnHeaderTooltips();
      processIcons(table.container);
      refreshScrollbars();
    }, 100);
  });

  // Group toggle icon sync
  table.table.on('groupVisibilityChanged', () => table.updateGroupIcons());
  table.table.on('dataLoaded', () => table.updateGroupIcons());
  table.table.on('renderComplete', () => {
    table.updateGroupIcons();
    processIcons(table.container);
    refreshScrollbars();
  });
  table.table.on('tableRedrawn', () => {
    table.updateGroupIcons();
    refreshScrollbars();
  });
}

/**
 * Column Header Tooltips Module
 *
 * FloatingUI tooltip initialization for column headers.
 *
 * @module tables/visual/column-tooltips
 */


/**
 * Initialize FloatingUI tooltips on column headers.
 * Called after the table is built to attach tooltips to all column header elements.
 * Safe to call multiple times - will skip headers that already have tooltips.
 *
 * @param {Object} table - LithiumTable instance
 */
function initColumnHeaderTooltips(table) {
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
function buildColumnTooltipContent(colDef) {
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
function escapeHtml(text) {
  if (!text) return '';
  const div = document.createElement('div');
  div.textContent = text;
  return div.innerHTML;
}

/**
 * Panel Width Persistence Module
 *
 * Panel width and collapsed state management.
 *
 * @module tables/persistence/panel-width
 */

/**
 * Get the width in pixels for a named mode.
 * @param {string} mode - 'narrow', 'compact', 'normal', 'wide', or 'auto'
 * @returns {number|null} Width in pixels, or null for 'auto'
 */
function getWidthForMode(mode) {
  const widths = { narrow: 160, compact: 314, normal: 468, wide: 622 };
  return widths[mode] || null;
}

/**
 * Apply width to the panel element based on mode.
 * Called during init (restore) and when user picks a width preset.
 * @param {Object} table - LithiumTable instance
 * @param {string|null} mode - Width mode or null for pixel fallback
 */
function applyPanelWidth$1(table, mode) {
  if (!table.panel) {
    console.warn('[LithiumTable] applyPanelWidth: no panel!', { storageKey: table.storageKey });
    return;
  }

  if (mode === null) {
    // No saved mode — use PanelStateManager pixel width as fallback
    if (table.panelStateManager) {
      const width = table.panelStateManager.loadWidth(280);
      table.panel.style.width = `${width}px`;
    }
  } else if (mode === 'auto') {
    table.panel.style.width = '';
    const currentWidth = table.panel.offsetWidth;
    if (table.panelStateManager) {
      table.panelStateManager.saveWidth(currentWidth);
    }
  } else {
    // Named mode
    const widthPx = getWidthForMode(mode);
    if (widthPx) {
      table.panel.style.width = `${widthPx}px`;
      if (table.panelStateManager) {
        table.panelStateManager.saveWidth(widthPx);
      }
    }
  }
}

/**
 * Save pixel width from splitter drag.
 * Called by the manager's onResizeEnd callback.
 * @param {Object} table - LithiumTable instance
 * @param {number} width
 */
function savePanelPixelWidth(table, width) {
  if (table.panelStateManager) {
    table.panelStateManager.saveWidth(width);
  }
}

/**
 * Load collapsed state from persistence.
 * @param {Object} table - LithiumTable instance
 * @param {boolean} defaultValue
 * @returns {boolean}
 */
function loadCollapsedState(table, defaultValue = false) {
  if (table.panelStateManager) {
    return table.panelStateManager.loadCollapsed(defaultValue);
  }
  return defaultValue;
}

/**
 * Save collapsed state to persistence.
 * @param {Object} table - LithiumTable instance
 * @param {boolean} collapsed
 */
function saveCollapsedState(table, collapsed) {
  if (table.panelStateManager) {
    table.panelStateManager.saveCollapsed(collapsed);
  }
}

/**
 * Search Utilities Module
 *
 * Provides search functionality for LithiumTable.
 *
 * @module tables/search/search-utils
 */

/**
 * Perform a client-side local search on the table's current data.
 * Filters rows by searching across specified fields (or all visible fields if none specified).
 *
 * @param {Object} table - LithiumTable instance
 * @param {string} searchTerm - The search term
 */
function performLocalSearch(table, searchTerm) {
  if (!table.table) return;

  table._localSearchTerm = searchTerm;

  const term = searchTerm.trim().toLowerCase();

  if (!term) {
    // Clear filter — show all rows
    table.table.clearFilter(true);
    table.updateMoveButtonState?.();
    return;
  }

  // Determine which fields to search
  let fieldsToSearch = table.localSearchFields;
  if (!fieldsToSearch || !Array.isArray(fieldsToSearch) || fieldsToSearch.length === 0) {
    // Default: search all visible columns that have a field defined
    fieldsToSearch = table.table.getColumns()
      .map(col => col.getDefinition().field)
      .filter(field => field && field !== '_selector');
  }

  // Apply a custom filter function
  table.table.setFilter((rowData) => {
    for (const field of fieldsToSearch) {
      const value = rowData[field];
      if (value != null && String(value).toLowerCase().includes(term)) {
        return true;
      }
    }
    return false;
  });

  table.updateMoveButtonState?.();
}

/**
 * Refresh Orchestrator Module
 *
 * Handles table configuration reload and refresh orchestration.
 *
 * @module tables/refresh-orchestrator
 */


/**
 * Reload the table configuration (schema) from Lookup 59.
 * This is called when the user clicks the refresh button to pick up
 * any changes to the table definition in Lookup 59.
 *
 * Enhanced to:
 * 0. Cancel edit mode first (save if dirty)
 * 1. Capture complete current state before refresh
 * 2. Reload Lookup 59
 * 3. Apply captured template instead of Default
 * 4. Re-submit original data request
 * 5. Re-select originally selected row and fire selection events
 *
 * @param {Object} table - LithiumTable instance
 */
async function reloadConfiguration(table) {
  log(Subsystems.TABLE, Status.INFO, `[${table.cssPrefix}] Reloading table configuration...`);

  // 0. CANCEL EDIT MODE FIRST - save if dirty, otherwise cancel
  if (table.isEditing) {
    if (table.isDirty) {
      // Save any pending changes before refresh
      await table.handleSave?.();
      log(Subsystems.TABLE, Status.DEBUG, `[${table.cssPrefix}] Saved changes before refresh`);
    }
    // Cancel edit mode
    await table.exitEditMode?.('cancel');
    log(Subsystems.TABLE, Status.DEBUG, `[${table.cssPrefix}] Exited edit mode before refresh`);
  }

  // 1. CAPTURE COMPLETE CURRENT STATE before clearing anything
  const capturedState = table.captureCurrentState?.();
  const capturedRowId = table.getSelectedRowId?.() || getCurrentlySelectedRowId(table);

  // Also capture local search term so it can be restored after reload
  const capturedLocalSearch = table._localSearchTerm || '';

  log(Subsystems.TABLE, Status.DEBUG,
    `[${table.cssPrefix}] Captured state: ${capturedRowId ? 'row selected' : 'no row'}, ${Object.keys(capturedState?.columns || {}).length} columns, local search: "${capturedLocalSearch}"`);

  // 2. Clear the table definition cache for this path so it re-fetches from Lookup 59
  if (table.tablePath && _tableDefCache.has(table.tablePath)) {
    _tableDefCache.delete(table.tablePath);
    log(Subsystems.TABLE, Status.DEBUG, `[${table.cssPrefix}] Cleared tableDef cache for "${table.tablePath}"`);
  }

  // 3. Reload configuration (fetches fresh tableDef from Lookup 59)
  table.tableDef = null;
  await table.loadConfiguration();

   // 4. Destroy existing Tabulator table and its OverlayScrollbar instance
   if (table._scrollbarInstance) {
     log(Subsystems.TABLE, Status.DEBUG, `[${table.cssPrefix}] refresh: destroying OSB before Tabulator destroy`);
     const oldHolder = table.container?.querySelector?.('.tabulator-tableholder');
     if (oldHolder && oldHolder.__overlayscrollbars) {
       delete oldHolder.__overlayscrollbars;
     }
     scrollbarManager.destroy(table._scrollbarInstance);
     table._scrollbarInstance = null;
     table.container?.classList?.remove('lithium-table-osb-enabled');
     log(Subsystems.TABLE, Status.DEBUG, `[${table.cssPrefix}] refresh: OSB destroyed`);
   }

   if (table.table) {
     table.table.destroy();
     table.table = null;
   }

  // 5. Recreate the Tabulator table with new configuration
  await table.initTable();
  log(Subsystems.TABLE, Status.DEBUG, `[${table.cssPrefix}] refresh: initTable completed, OSB instance after initTable: ${!!table._scrollbarInstance}`);

  // 6. Restore filters visible state (if previously enabled)
  if (table.filtersVisible && table.table) {
    table.toggleHeaderFilters(true);
  }

  // 7. Apply captured state (user customizations) for all tables
  if (capturedState) {
    const sampleWidth = Object.values(capturedState.columns || {})[0]?.width;
    log(Subsystems.TABLE, Status.DEBUG, `[${table.cssPrefix}] Applying captured state: ${Object.keys(capturedState.columns || {}).length} columns, first width: ${sampleWidth}`);

    // Clear OSB instance before template loads - setColumns() destroys table holder content
    if (table._scrollbarInstance) {
      log(Subsystems.TABLE, Status.DEBUG, `[${table.cssPrefix}] refresh: destroying OSB before loadTemplate`);
      scrollbarManager.destroy(table._scrollbarInstance);
      table._scrollbarInstance = null;
      table.container?.classList?.remove('lithium-table-osb-enabled');
      log(Subsystems.TABLE, Status.DEBUG, `[${table.cssPrefix}] refresh: OSB destroyed`);
    }

    await table.loadTemplate?.(capturedState);
    log(Subsystems.TABLE, Status.DEBUG, `[${table.cssPrefix}] refresh: loadTemplate completed, scheduling OSB reinit`);

    // Re-initialize OSB after setColumns() destroys and rebuilds everything
    setTimeout(() => {
      log(Subsystems.TABLE, Status.DEBUG, `[${table.cssPrefix}] refresh: _initTableScrollbars timeout fired`);
      table._initTableScrollbars();
    }, 0);
    setTimeout(() => {
      log(Subsystems.TABLE, Status.DEBUG, `[${table.cssPrefix}] refresh: _updateTableScrollbars timeout fired`);
      table._updateTableScrollbars();
    }, 100);
  }

  // 8. Load data WITHOUT automatic selection (we'll handle it explicitly)
  // Temporarily store the captured ID so loadData can use it
  const savedAutoSelectRow = table.autoSelectRow;
  table.autoSelectRow = () => {}; // No-op during refresh data load

  try {
    if (typeof table.onRefresh === 'function') {
      await table.onRefresh();
    } else if (table.lastLoadParams) {
      await table.loadData?.(table.lastLoadParams.searchTerm, table.lastLoadParams.extraParams);
    } else {
      if (table.currentData && table.currentData.length > 0) {
        // For static tables without onRefresh that have currentData, reload the static data
        await table.loadStaticData?.(table.currentData, { autoSelect: false });
      } else {
        await table.loadData?.();
      }
    }
  } finally {
    // Restore original autoSelectRow
    table.autoSelectRow = savedAutoSelectRow;
  }

  // 9. Explicitly restore the captured row selection AFTER data is loaded
  // Skip if table has custom onRefresh that handles selection itself
  if (!table.onRefresh && capturedRowId && table.table) {
    log(Subsystems.TABLE, Status.DEBUG, `[${table.cssPrefix}] Restoring row selection: ${capturedRowId}`);

    // Wait a tick for Tabulator to finish rendering
    await new Promise(resolve => requestAnimationFrame(resolve));

    table.autoSelectRow?.(capturedRowId);

    // Fire selection event to update manager UI
    const selectedRows = table.table.getSelectedRows();
    if (selectedRows.length > 0 && typeof table.onRowSelected === 'function') {
      const rowData = selectedRows[0].getData();
      log(Subsystems.TABLE, Status.DEBUG, `[${table.cssPrefix}] Row restored, firing onRowSelected`);
      table.onRowSelected(rowData);
    } else {
      log(Subsystems.TABLE, Status.WARN, `[${table.cssPrefix}] Could not restore row ${capturedRowId}`);
    }
  }

  // 10. Restore local search filter if it was active
  if (capturedLocalSearch && table.table) {
    log(Subsystems.TABLE, Status.DEBUG, `[${table.cssPrefix}] Restoring local search: "${capturedLocalSearch}"`);
    performLocalSearch(table, capturedLocalSearch);
  }

  log(Subsystems.TABLE, Status.INFO, `[${table.cssPrefix}] Table configuration reloaded`);

  // Notify the manager that refresh completed (for parent/child coordination)
  if (typeof table.onRefreshComplete === 'function') {
    table.onRefreshComplete();
  }
}

/**
 * Get the currently selected row ID directly from Tabulator
 * @param {Object} table - LithiumTable instance
 * @returns {string|null} Selected row ID
 */
function getCurrentlySelectedRowId(table) {
  if (!table.table) return null;
  const selectedRows = table.table.getSelectedRows();
  if (selectedRows.length === 0) return null;

  const row = selectedRows[0];
  const data = row.getData();

  // For compound keys, construct composite ID
  if (Array.isArray(table.primaryKeyField)) {
    return table.primaryKeyField.map(f => data[f]).join('::');
  }

  // For single keys
  return data[table.primaryKeyField] ?? data.key_idx ?? data.id ?? null;
}

/**
 * Template Capture Module
 *
 * Captures complete current table state as a template.
 * Provides canonical extractor functions for tableDef generation.
 *
 * @module tables/template/capture
 */


/**
 * Properties that are considered canonical for tableDef columns.
 * These are the properties that will be included in extracted column definitions.
 * Derived from COLUMN_VALID_PROPS minus internal/runtime-only properties.
 */
const CANONICAL_COLUMN_PROPS = new Set([
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
function extractTableDefColumn(column, originalColDef = {}, lithiumMeta = {}, options = {}) {
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
function extractTableDef(table, options = {}) {
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
function captureCurrentState(table) {
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
function createTemplateColumnFromTableDef(fieldName, colDef = {}) {
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
function extractTemplateColumnFromColumn(column) {
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
function generateTemplateJSON(table) {
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
function buildColumnDefinitionsFromTemplateState({
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

/**
 * Data Loading Module
 *
 * Table data loading, static data, and row ID management.
 *
 * @module tables/data/data-loading
 */


/**
 * Load data from the configured queryRef
 * @param {Object} table - LithiumTable instance
 * @param {string} searchTerm - Optional search term
 * @param {Object} extraParams - Optional extra parameters for the query
 */
async function loadData(table, searchTerm = '', extraParams = {}) {
  if (!table.app?.api) return;

  // Set transition flag to prevent button flashing during data reload
  table._inSelectionTransition = true;

  // Get currently selected row, or restore from localStorage
  const previouslySelectedId = table.getSelectedRowId?.() ?? table.restoreSelectedRowId?.();
  table.showLoading?.();

  try {
    const listRef = table.queryRefs.queryRef;
    const searchRef = table.queryRefs.searchQueryRef;
    const queryRef = searchTerm && searchRef ? searchRef : listRef;

    if (!queryRef) {
      log(Subsystems.TABLE, Status.WARN, `[LithiumTable] No queryRef configured for data loading`);
      return;
    }

    // Check if extraParams are required but missing
    if (extraParams === null || extraParams === undefined) {
      const hasParams = extraParams && Object.keys(extraParams).length > 0;
      if (!hasParams) {
        log(Subsystems.TABLE, Status.DEBUG,
          `[LithiumTable] Data load skipped - requires runtime params (no extraParams provided)`);
        return;
      }
    }

    log(Subsystems.CONDUIT, Status.INFO,
      `[LithiumTable] Loading data (queryRef: ${queryRef}${searchTerm ? `, search: "${searchTerm}"` : ''})`);

    const params = { ...extraParams };

    if (searchTerm && searchRef) {
      params.STRING = { SEARCH: searchTerm.toUpperCase() };
    }

    // Track last load params for refresh re-submit
    table.lastLoadParams = {
      searchTerm: searchTerm || '',
      extraParams: extraParams ? { ...extraParams } : {},
    };

    const rows = await authQuery(table.app.api, queryRef, Object.keys(params).length > 0 ? params : undefined);

    if (!table.table) return;

    table.currentData = rows;

    // If table was initialized without data but we now have data, rebuild columns
    if (table.table && rows.length > 0 && table._wasInitializedWithoutData) {
      await table._rebuildColumnsWithData?.(rows);
    }

    table.table.blockRedraw?.();
    try {
      table.table.setData(rows);
      table.discoverColumns?.(rows);
    } finally {
      table.table.restoreRedraw?.();
    }

    requestAnimationFrame(() => {
      table._updateTableScrollbars?.();
      table.autoSelectRow?.(previouslySelectedId);
      table.updateMoveButtonState?.();
      table.updateDuplicateButtonState?.();
      // Clear transition flag after selection is restored
      table._inSelectionTransition = false;
    });

    table.onDataLoaded(rows);
  } catch (error) {
    // Build informative error details
    const queryRef = searchTerm && table.queryRefs.searchQueryRef
      ? table.queryRefs.searchQueryRef
      : table.queryRefs.queryRef;
    const errorDetails = error.serverError || error.message || 'Unknown error';
    const errorContext = `QueryRef: ${queryRef}${searchTerm ? `, Search: "${searchTerm}"` : ''}`;

    log(Subsystems.TABLE, Status.ERROR,
      `[LithiumTable] Data load failed: ${errorDetails} (${errorContext})`);

    toast.error('Data Load Failed', {
      description: `${errorDetails} (${errorContext})`,
      duration: 8000,
    });
    table.currentData = [];
  } finally {
    table.hideLoading?.();
    // Ensure transition flag is cleared even on error
    table._inSelectionTransition = false;
  }
}

/**
 * Get the selected row ID from the table
 * @param {Object} table - LithiumTable instance
 * @returns {string|null} Row ID
 */
function getSelectedRowId(table) {
  if (!table.table) return null;
  const selected = table.table.getSelectedRows();
  if (selected.length > 0) {
    const pkFields = table.primaryKeyField;
    return getCompositeRowId$1(selected[0].getData(), pkFields);
  }
  return null;
}

/**
 * Get the selected row data
 * @param {Object} table - LithiumTable instance
 * @returns {Object|null} Row data
 */
function getSelectedRowData(table) {
  if (!table.table) return null;
  const selected = table.table.getSelectedRows();
  if (selected.length > 0) {
    return selected[0].getData();
  }
  return null;
}

/**
 * Auto-select a row by ID
 * @param {Object} table - LithiumTable instance
 * @param {string|number} targetId - Row ID to select
 */
function autoSelectRow(table, targetId) {
  if (!table.table) return;
  const rows = table.table.getRows('active');
  // If no rows yet and we have a target to find, wait for rows to render
  if (rows.length === 0 && targetId == null) return;

  const pkFields = table.primaryKeyField;

  // Set transition flag to suppress button updates during deselect/select
  table._inSelectionTransition = true;

  // Use blockRedraw to prevent Tabulator from processing events between deselect/select
  table.table.blockRedraw?.();
  try {
    // Deselect any existing selections first
    const selectedRows = table.table.getSelectedRows();
    if (selectedRows.length > 0) {
      table.table.deselectRow(selectedRows);
    }

    if (targetId != null && pkFields !== null && pkFields.length > 0) {
      for (const row of rows) {
        const rowId = getCompositeRowId$1(row.getData(), pkFields);
        if (String(rowId) === String(targetId)) {
          row.select();
          // Defer scroll to ensure Tabulator has rendered the row
          requestAnimationFrame(() => {
            row?.scrollTo?.('center', false);
          });
          // Save selected row ID for persistence across sessions
          table.saveSelectedRowId?.(getCompositeRowId$1(row.getData(), pkFields));
          break;
        }
      }
    } else {
      if (rows.length > 0) {
        rows[0].select();
        // Defer scroll to ensure Tabulator has rendered the row
        requestAnimationFrame(() => {
          rows[0]?.scrollTo?.('center', false);
        });
        // Save selected row ID for persistence across sessions
        table.saveSelectedRowId?.(getCompositeRowId$1(rows[0].getData(), pkFields));
      }
    }
  } finally {
    table.table.restoreRedraw?.();
  }

  // Clear transition flag and update buttons directly
  table._inSelectionTransition = false;
  table.updateMoveButtonState?.();
  table.updateDuplicateButtonState?.();
}

/**
 * Get composite row ID from row data using primary key fields
 * @param {Object} rowData - Row data object
 * @param {string[]} pkFields - Array of primary key field names
 * @returns {string} Composite ID
 */
function getCompositeRowId$1(rowData, pkFields) {
  if (!pkFields || pkFields.length === 0) {
    // Fallback: try common primary key field names
    const fallbackFields = ['id', 'query_id', 'lookup_id', 'key_idx', 'style_id', 'version_id'];
    for (const field of fallbackFields) {
      if (rowData[field] != null) {
        return String(rowData[field]);
      }
    }
    // Last resort: use the first field that looks like an ID
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
 * Load static (non-API) data into the table.
 * @param {Object} table - LithiumTable instance
 * @param {Array<Object>} rows - The data array to load
 * @param {Object} [options]
 * @param {boolean} [options.autoSelect=true] - Auto-select first row after load
 */
function loadStaticData(table, rows, options = {}) {
  if (!table.table) return;
  const { autoSelect = true } = options;

  // Set transition flag to prevent button flashing during data reload
  table._inSelectionTransition = true;

  table.currentData = rows || [];

  // Get currently selected row, or restore from localStorage
  const previouslySelectedId = autoSelect ? (table.getSelectedRowId?.() ?? table.restoreSelectedRowId?.()) : null;

  table.table.blockRedraw?.();
  try {
    table.table.setData(table.currentData);
    table.discoverColumns?.(table.currentData);
  } finally {
    table.table.restoreRedraw?.();
  }

  if (autoSelect) {
    requestAnimationFrame(() => {
      table._updateTableScrollbars?.();
      if (previouslySelectedId != null) {
        table.autoSelectRow?.(previouslySelectedId);
      } else {
        const activeRows = table.table.getRows('active');
        if (activeRows.length > 0) {
          activeRows[0].select();
          // Defer scroll to ensure Tabulator has rendered the row
          requestAnimationFrame(() => {
            activeRows[0]?.scrollTo?.('center', false);
          });
          // Save selected row ID for persistence across sessions
          const pkFields = table.primaryKeyField;
          if (pkFields) {
            table.saveSelectedRowId?.(getCompositeRowId$1(activeRows[0].getData(), pkFields));
          }
        }
      }
      table.updateMoveButtonState?.();
      table.updateDuplicateButtonState?.();
      // Clear transition flag after selection is restored
      table._inSelectionTransition = false;
    });
  } else {
    table._updateTableScrollbars?.();
    table._inSelectionTransition = false;
    table.updateMoveButtonState?.();
    table.updateDuplicateButtonState?.();
  }

  table.onDataLoaded(table.currentData);
}

/**
 * Load initial data for the table
 * @param {Object} table - LithiumTable instance
 * @param {Object} extraParams - Optional extra parameters
 * @returns {Promise<Array|null>} Loaded rows or null
 */
async function loadInitialData(table, extraParams = null) {
  if (!table.app?.api || !table.queryRefs.queryRef) return null;

  try {
    const rows = await authQuery(table.app.api, table.queryRefs.queryRef, extraParams);
    table.currentData = rows;
    return rows;
  } catch (err) {
    const isMissingParam = err.message?.includes('Missing required parameters') ||
                         err.message?.includes('Required:') ||
                         err.message?.includes('missing');
    if (isMissingParam) {
      log(Subsystems.CONDUIT, Status.DEBUG,
        `[LithiumTable] Initial load skipped - requires runtime params (${err.message})`);
    } else {
      log(Subsystems.CONDUIT, Status.ERROR, `[LithiumTable] Failed to load initial data: ${err.message}`);
    }
    return null;
  }
}

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
async function exitEditModeForNavigation(table) {
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
async function scrollRowIntoView(table, row) {
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
async function expandRowGroups(row) {
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
function waitForNextFrame() {
  return new Promise(resolve => requestAnimationFrame(() => resolve()));
}

/**
 * Get the Tabulator vertical scroll container.
 * @param {Object} table - LithiumTable instance
 * @returns {HTMLElement|null}
 */
function getTableHolder(table) {
  return table.container?.querySelector?.('.tabulator-tableholder') || null;
}

/**
 * Check whether a row is fully visible inside the Tabulator holder.
 * @param {Object} table - LithiumTable instance
 * @param {Object} row - Tabulator row component
 * @returns {boolean}
 */
function isRowFullyVisible(table, row) {
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
function scrollRowElementIntoView(table, row, position = 'center') {
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
async function navigateFirst(table) {
  if (!table.table) return;
  if (!(await exitEditModeForNavigation(table))) return;
  table.selectRowByIndex?.(0);
}

/**
 * Navigate to last record
 * @param {Object} table - LithiumTable instance
 */
async function navigateLast(table) {
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
async function navigatePrevRec(table) {
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
async function navigateNextRec(table) {
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
async function navigatePrevPage(table) {
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
async function navigateNextPage(table) {
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
function getPageSize(table) {
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
function selectRowByIndex(table, index) {
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
function getVisibleRowsAndIndex(table) {
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

/**
 * Row Selection Module
 *
 * Row selection, matching, and helper functions.
 *
 * @module tables/selection/row-selection
 */


/**
 * Handle row selected event
 * @param {Object} table - LithiumTable instance
 * @param {Object} row - Tabulator row component
 */
async function handleRowSelected(table, row) {
  // Enforce single row selection by deselecting all other rows
  const selectedRows = table.table.getSelectedRows();
  const otherRows = selectedRows.filter(selectedRow => selectedRow !== row);
  if (otherRows.length > 0) {
    table.table.deselectRow(otherRows);
    // Dispatch event to close all popups when selecting a new row
    document.dispatchEvent(new CustomEvent('close-all-popups'));
  }

  const pkFields = table.primaryKeyField;
  const newRowId = getCompositeRowId$1(row.getData(), pkFields);

  // If editing and clicked the same row, just update UI and return
  // This prevents exiting edit mode when clicking different cells in the same row
  if (table.isEditing && table.editingRowId === newRowId) {
    table.updateSelectorCell?.(row, true);
    return;
  }

  // Commit any active cell edit before checking dirty state
  // This ensures cell changes are captured before auto-save decision
  table.commitActiveCellEdit?.();

  if (table.isEditing && table.editingRowId !== newRowId) {
    // Synchronous dirty check — the rAF-deferred isDirty flag may be stale
    // if the user typed in an editor and immediately clicked a different row.
    const actuallyDirty = table.isActuallyDirty?.();

    if (actuallyDirty) {
      // Default behaviour: auto-save the editing row, then switch.
      const saveSucceeded = await table.autoSaveBeforeRowChange?.();
      if (!saveSucceeded) {
        const editingRow = getEditingRow(table);
        if (editingRow) editingRow.select();
        return;
      }
      await table.exitEditMode?.('save');
    } else {
      await table.exitEditMode?.('row-change');
    }
  }

  table.updateSelectorCell?.(row, true);

  // Save selected row ID for persistence across sessions
  if (newRowId != null) {
    table.saveSelectedRowId?.(newRowId);
  }

  table.onRowSelected(row.getData());

  // Update audit footer if attached
  table.auditFooter?.update(row.getData());
}

/**
 * Select a data row
 * @param {Object} table - LithiumTable instance
 * @param {Object} row - Tabulator row component
 * @returns {boolean} true if selection changed
 */
function selectDataRow(table, row) {
  if (!table.table || !row || isCalcRow(table, row)) return false;

  const currentRow = getSelectedDataRow(table);
  if (rowsMatch(table, currentRow, row)) {
    if (!row.isSelected?.()) row.select();
    return false;
  }

  table.closeTransientPopups?.();
  table.table.deselectRow();
  row.select();
  return true;
}

/**
 * Get the currently selected data row
 * @param {Object} table - LithiumTable instance
 * @returns {Object|null} Tabulator row component
 */
function getSelectedDataRow(table) {
  if (!table.table) return null;
  const selectedRows = table.table.getSelectedRows?.() || [];
  return selectedRows.find(row => !isCalcRow(table, row)) || null;
}

/**
 * Get the currently editing row
 * @param {Object} table - LithiumTable instance
 * @returns {Object|null} Tabulator row component
 */
function getEditingRow(table) {
  if (!table.table || !table.isEditing || table.editingRowId == null) return null;

  const pkFields = table.primaryKeyField;
  const rows = table.table.getRows('active');
  return rows.find(row => {
    const rowId = getCompositeRowId$1(row.getData(), pkFields);
    return rowId === table.editingRowId;
  }) || null;
}

/**
 * Check if row is a calculation row
 * @param {Object} table - LithiumTable instance
 * @param {Object} row - Tabulator row component
 * @returns {boolean}
 */
function isCalcRow(table, row) {
  if (!row) return false;
  const rowEl = row.getElement?.();
  return !!rowEl?.closest?.('.tabulator-calcs, .tabulator-calcs-holder, .tabulator-calcs-bottom');
}

/**
 * Check if cell is a calculation cell
 * @param {Object} table - LithiumTable instance
 * @param {Object} cell - Tabulator cell component
 * @returns {boolean}
 */
function isCalcCell(table, cell) {
  if (!cell) return false;
  const cellEl = cell.getElement?.();
  if (cellEl?.closest?.('.tabulator-calcs, .tabulator-calcs-holder, .tabulator-calcs-bottom')) {
    return true;
  }
  return isCalcRow(table, cell.getRow?.());
}

/**
 * Check if two rows match by ID
 * @param {Object} table - LithiumTable instance
 * @param {Object} rowA - First row
 * @param {Object} rowB - Second row
 * @returns {boolean}
 */
function rowsMatch(table, rowA, rowB) {
  if (!rowA || !rowB) return false;
  if (rowA === rowB) return true;

  const pkFields = table.primaryKeyField;
  const rowAId = getCompositeRowId$1(rowA.getData?.(), pkFields);
  const rowBId = getCompositeRowId$1(rowB.getData?.(), pkFields);
  return rowAId != null && rowBId != null && String(rowAId) === String(rowBId);
}

/**
 * Column Management Module
 *
 * Column discovery, building, merging, and management.
 *
 * @module tables/columns/column-management
 */


// ── Deep Merge Configuration ────────────────────────────────────────────────

/**
 * Column parameter keys that should be deep-merged across stages.
 * These are nested objects where properties from multiple sources
 * (coltype, tableDef, template) should be combined rather than replaced.
 */
const DEEP_MERGE_PARAM_KEYS = [
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
function deepMergeParams(base, overlay, paramKeys = DEEP_MERGE_PARAM_KEYS) {
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
function discoverColumns(table, rows) {
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
function detectColtype(rows, field) {
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
function buildColumnsFromData(table, data) {
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
function sortColumnsByPriority(columns) {
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
function mergeColumnsWithTableDef(table, autoColumns, tableDefColumns) {
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
function applyEditModeGate(table, columns) {
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
function buildSelectorColumn(table) {
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
          return `<div class="${table.cssPrefix}-selector-indicator ${table.cssPrefix}-selector-edit"><fa fa-pencil></fa></div>`;
        }
        return `<div class="${table.cssPrefix}-selector-indicator ${table.cssPrefix}-selector-active"><fa fa-caret-large-right></fa></div>`;
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

/**
 * LithiumTable Base Module
 *
 * Core functionality including:
 * - Initialization and configuration loading
 * - Tabulator instance management
 * - Event wiring
 * - State management
 * - Navigation
 *
 * @module core/lithium-table-base
 */


// ── Column Utility Functions ────────────────────────────────────────────────

/**
 * Sanitize column titles for safe use as CSS class names
 * Replaces spaces and special characters, but preserves case
 */
function sanitizeColumnTitle(title) {
  if (!title || typeof title !== 'string') return 'col';
  return title.replace(/[^a-zA-Z0-9_-]/g, '_').replace(/_+/g, '_');
}

// ── LithiumTableBase Class ─────────────────────────────────────────────────

class LithiumTableBase {
  /**
   * Create a LithiumTable instance
   * @param {Object} options - Configuration options
   */
  constructor(options) {
    // Core elements
    this.container = options.container;
    this.navigatorContainer = options.navigatorContainer;
    this.tablePath = options.tablePath;
    this.lookupKeyIdx = options.lookupKeyIdx || null;  // Direct Lookup 59 key_idx
    this.cssPrefix = options.cssPrefix || 'lithium';

    // Primary key field (explicit override, set before tableDef loading)
    if (options.primaryKeyField != null) {
      this.primaryKeyField = options.primaryKeyField;
      this._explicitPrimaryKeyField = true;
    } else {
      this.primaryKeyField = null;
      this._explicitPrimaryKeyField = false;
    }

    // Configuration (can be provided or loaded)
    this.tableDef = options.tableDef || null;
    this.coltypes = options.coltypes || null;

    // Query references
    this.queryRefs = {
      queryRef: options.queryRef || null,
      searchQueryRef: options.searchQueryRef || null,
      detailQueryRef: options.detailQueryRef || null,
      insertQueryRef: options.insertQueryRef || null,
      updateQueryRef: options.updateQueryRef || null,
      deleteQueryRef: options.deleteQueryRef || null,
    };

    // App instance for API calls
    this.app = options.app || null;

    // State flags
    this.readonly = options.readonly || false;
    this.isEditing = false;
    this.filtersVisible = false;
    this.isDirty = false;
    this.editTransitionPending = false;
    this._inSelectionTransition = false;

    // Editing state
    this.editingRowId = null;
    this.originalRowData = null;
    this.columnEditors = new Map();

    // Lithium-only column metadata (stored separately from Tabulator columns)
    // Map of field name -> { coltype, editable, sort, filter, calculated, primaryKey, description, ... }
    this._columnMeta = new Map();

    // UI state
    this.tableWidthMode = options.tableWidthMode || 'compact';
    this.tableLayoutMode = 'fitColumns';

    // Storage key for persistence
    this.storageKey = options.storageKey || `${this.cssPrefix}_table`;

    // Panel persistence (centralized width + collapsed state)
    this.panel = options.panel || null;
    this.panelStateManager = options.panelStateManager || null;
    this.splitter = null; // Set via setSplitter() after init

    // Data cache
    this.currentData = [];

    // Last data load parameters (for re-submit on refresh)
    this.lastLoadParams = null;

    // Metadata (primaryKeyField already set in constructor if explicitly provided)

    // References for cleanup
    this.table = null;
    this.loaderElement = null;

    // Callbacks
    this.onRowSelected = options.onRowSelected || (() => {});
    this.onRowDeselected = options.onRowDeselected || (() => {});
    this.onDataLoaded = options.onDataLoaded || (() => {});
    this.onEditModeChange = options.onEditModeChange || (() => {});
    this.onDirtyChange = options.onDirtyChange || null; // Called when dirty state changes (isDirty, rowData)
    this.onSetTableWidth = options.onSetTableWidth || null;
    this.onRefresh = options.onRefresh || null;
    this.onExecuteSave = options.onExecuteSave || null; // Custom save logic — async (row, editHelper) => {}
    this.onDuplicate = options.onDuplicate || null; // Custom duplicate logic — async (rowData) => newRowData

    // Audit footer reference (shared AuditInfoFooter instance)
    this.auditFooter = options.auditFooter || null;

    // Column manager option


    // Always-editable mode: cells are directly editable without entering edit mode.
    // Used by the Column Manager which manages its own save/cancel workflow.
    this.alwaysEditable = options.alwaysEditable === true;

    // Local search mode: filter rows client-side instead of server request.
    // Use localSearchFields to specify which fields to search (defaults to all visible fields).
    this.localSearch = options.localSearch === true;
    this.localSearchFields = options.localSearchFields || null;
    this.useOverlayScrollbars = options.useOverlayScrollbars !== false;
    this._localSearchTerm = '';

    // Popup state
    this.activeNavPopup = null;
    this.activeNavPopupId = null;
    this.navPopupCloseHandler = null;
  }

  // ── Initialization ────────────────────────────────────────────────────────

  async init() {
    await this.loadConfiguration();
    this.injectSortIconStyles();

    // Skip initial data load - wait for explicit loadData() call
    // This is needed for tables that require runtime params (e.g., child tables)
    // The parent table will trigger loadData() after selection

    this.table = null;
    await this.initTable(null);

    this.buildNavigator();
    this.setupKeyboardHandling();
    await this.setupPersistence();
  }

  async _loadInitialData(extraParams = null) {
    return loadInitialData(this, extraParams);
  }

  /**
   * Inject styles for the current cssPrefix
   * This ensures all LithiumTable styles work regardless of the prefix used
   */
  injectSortIconStyles() {
    // Only inject if using a custom prefix (not the default 'lithium')
    if (this.cssPrefix === 'lithium') return;

    // Check if styles for this prefix already exist
    const styleId = `lithium-table-${this.cssPrefix}`;
    if (document.getElementById(styleId)) {
      log(Subsystems.TABLE, Status.DEBUG, `[LithiumTable] Sort icons already injected for prefix: ${this.cssPrefix}`);
      return;
    }

    // Only inject sort icon styles — Navigator styles are provided globally
    // by lithium-table.css using the "lithium-" prefix. All tables share
    // the same Navigator styling via CSS variables.
    const cssText = `
@layer managers {
  /* ── Sort Icons for prefix: ${this.cssPrefix} ── */
  .${this.cssPrefix}-sort-icons {
    display: inline-flex;
    align-items: center;
    justify-content: center;
    width: 16px;
    height: 16px;
    line-height: 1;
    position: absolute;
    right: -4px;
    vertical-align: middle;
  }

  .${this.cssPrefix}-sort-icon {
    font-size: 12px;
    transition: color var(--transition-fast), opacity var(--transition-fast);
  }

  .${this.cssPrefix}-sort-icons[data-sort-dir="none"] .${this.cssPrefix}-sort-icon {
    color: var(--text-muted);
    opacity: 0.5;
  }

  .${this.cssPrefix}-sort-icons[data-sort-dir="asc"] .${this.cssPrefix}-sort-icon,
  .${this.cssPrefix}-sort-icons[data-sort-dir="desc"] .${this.cssPrefix}-sort-icon {
    color: white;
    opacity: 1;
  }
}
`;

    const styleEl = document.createElement('style');
    styleEl.id = styleId;
    styleEl.textContent = cssText;
    document.head.appendChild(styleEl);

    log(Subsystems.TABLE, Status.DEBUG, `[LithiumTable] Injected sort icons for prefix: ${this.cssPrefix}`);

    // Store reference for cleanup
    this._sortStyleEl = styleEl;
  }

  async loadConfiguration() {
    if (!this.coltypes) {
      this.coltypes = await loadColtypes();
    }

    if (!this.tableDef && this.tablePath) {
      this.tableDef = await loadTableDef(this.tablePath, null, this.lookupKeyIdx);
    }

    if (this.tableDef) {
      const defQueryRefs = getQueryRefs(this.tableDef);
      // Merge: JSON config values are the base, constructor options override only when non-null.
      // Without this filter, explicit null values from the constructor (when the manager
      // doesn't pass e.g. updateQueryRef) would overwrite valid values from the JSON config.
      const nonNullOverrides = Object.fromEntries(
        Object.entries(this.queryRefs).filter(([, v]) => v != null)
      );
      this.queryRefs = { ...defQueryRefs, ...nonNullOverrides };
      // Only set primaryKeyField from tableDef if not explicitly provided in constructor
      if (!this._explicitPrimaryKeyField) {
        this.primaryKeyField = getPrimaryKeyField(this.tableDef);
      }

      // Log table initialization details including primary key
      const tableName = this.tablePath || this.cssPrefix || 'auto';
      const isEditable = !this.readonly;
      const pkInfo = this.primaryKeyField ?
        (Array.isArray(this.primaryKeyField) ? this.primaryKeyField.join(', ') : this.primaryKeyField) :
        'none';
      const status = (isEditable && !this.primaryKeyField) ? Status.WARN : Status.INFO;
      const editMode = isEditable ? 'editable' : 'read-only';
      log(Subsystems.TABLE, status,
        `[${tableName}] ${editMode}, primary key: ${pkInfo}`);

      // Apply default template if available (last step before building table)
      this._applyDefaultTemplate();

      const lookupRefs = Object.values(this.tableDef.columns || {})
        .map(col => col.lookupRef)
        .filter(Boolean);
      const uniqueRefs = [...new Set(lookupRefs)];

      if (uniqueRefs.length > 0 && this.app?.api) {
        try {
          await preloadLookups(uniqueRefs, authQuery, this.app.api);
          log(Subsystems.TABLE, Status.SUCCESS,
            `[LithiumTable] Pre-loaded ${uniqueRefs.length} lookup table(s)`);
        } catch (err) {
          log(Subsystems.TABLE, Status.WARN,
            `[LithiumTable] Lookup pre-load failed: ${err.message}`);
        }
      }
    }
  }

  async initTable(preloadedData = null) {
    // Add base LithiumTable class for shared styling
    this.container.classList.add('lithium-table-container');

    // Group icon animator instance
    this._groupIconAnimator = new GroupIconAnimator(this);

    let dataColumns = [];
    const data = preloadedData || this.currentData;

    // Track if we're initializing without data (for data-first rebuild later)
    this._wasInitializedWithoutData = !data || data.length === 0;

    // First, auto-discover columns from data if available (data-first approach)
    if (data && data.length > 0) {
      dataColumns = this._buildColumnsFromData(data);
      log(Subsystems.TABLE, Status.DEBUG,
        `[${this.cssPrefix}] Auto-discovered ${dataColumns.length} columns from ${data.length} data rows`);
    }

    // Then apply tableDef overlays if available
    if (this.tableDef && this.coltypes) {
      const { columns: tableDefColumns, columnMeta } = resolveColumnsWithMeta(
        this.tableDef,
        this.coltypes,
        { filterEditor: this.createFilterEditorFunction() }
      );

      // Store the column metadata Map
      this._columnMeta = columnMeta;

      if (dataColumns.length > 0) {
        // Merge: apply tableDef properties to auto-discovered columns
        dataColumns = this._mergeColumnsWithTableDef(dataColumns, tableDefColumns);
        log(Subsystems.TABLE, Status.DEBUG,
          `[${this.cssPrefix}] Applied tableDef overlays to ${dataColumns.length} columns`);
      } else {
        // No data available yet, use tableDef columns
        dataColumns = tableDefColumns;
      }
    } else if (this.tableDef?.columns && Object.keys(this.tableDef.columns).length > 0) {
      const { columns: tableDefColumns, columnMeta } = resolveColumnsWithMeta(
        this.tableDef,
        this.coltypes || {},
        { filterEditor: this.createFilterEditorFunction() }
      );

      // Store the column metadata Map
      this._columnMeta = columnMeta;

      if (dataColumns.length > 0) {
        // Merge: apply tableDef properties to auto-discovered columns
        dataColumns = this._mergeColumnsWithTableDef(dataColumns, tableDefColumns);
      } else {
        dataColumns = tableDefColumns;
      }
    }

    if (dataColumns.length === 0) {
      if (this._wasInitializedWithoutData) {
        // For data-first tables, create with placeholder column until data loads
        log(Subsystems.TABLE, Status.INFO,
          `[${this.cssPrefix}] No data available yet, creating table with placeholder column`);
        dataColumns = [{
          field: '_placeholder',
          title: 'Loading...',
          visible: true,
          editable: false,
          resizable: false,
        }];
      } else {
        log(Subsystems.TABLE, Status.WARN,
          `[LithiumTable] No columns defined for table "${this.tablePath}" - table will be empty`);
        return;
      }
    }

    dataColumns = this._sortColumnsByPriority(dataColumns);

    this.applyEditModeGate(dataColumns);

    const tableOptions = this.tableDef
      ? resolveTableOptions(this.tableDef)
      : { layout: 'fitColumns', responsiveLayout: false };

    // Lithium tables always prefer horizontal scrolling over Tabulator's
    // responsive column hiding/collapse modes. Preserve resolveTableOptions()
    // as a faithful schema mapper, but override the runtime behavior here.
    tableOptions.responsiveLayout = false;

    const persistedLayout = this.restoreLayoutMode();
    if (persistedLayout) {
      tableOptions.layout = persistedLayout;
    }
    this.tableLayoutMode = tableOptions.layout || 'fitColumns';

    const selectorColumn = this.buildSelectorColumn();

    const columns = [selectorColumn, ...dataColumns];
    columns.forEach(col => {
      if (col.title && !col.cssClass?.includes('first-visible-col')) {
        col.cssClass = [col.cssClass, sanitizeColumnTitle(col.title)].filter(Boolean).join(' ');
      }
    });

    // Note: We do NOT set dataLoaded callback for auto-discover here.
    // loadData() already calls discoverColumns() after setData(), so
    // a dataLoaded callback would cause double-discovery.

    this.table = new TabulatorFull(this.container, {
      ...tableOptions,
      selectableRows: 1,
      // Note: We intentionally do NOT set editTriggerEvent. This ensures
      // Tabulator never auto-activates editors - we control all editor
      // activation programmatically via enterEditMode() and queueCellEdit().
      // This prevents editors from being active when not in edit mode.
      editTriggerEvent: false,
      scrollToRowPosition: 'center',
      scrollToRowIfVisible: false,
      headerSortTristate: true,
      headerSortElement: (column, dir) => {
        const prefix = this.cssPrefix;
        // Use Font Awesome icons matching the grouping popup style
        // Tri-state: unsorted (fa-angles-up-down) -> asc (fa-angle-up) -> desc (fa-angle-down) -> unsorted
        if (dir === 'asc') {
          return `<span class="${prefix}-sort-icons" data-sort-dir="asc"><fa class="${prefix}-sort-icon" fa-angle-up></fa></span>`;
        } else if (dir === 'desc') {
          return `<span class="${prefix}-sort-icons" data-sort-dir="desc"><fa class="${prefix}-sort-icon" fa-angle-down></fa></span>`;
        } else {
          return `<span class="${prefix}-sort-icons" data-sort-dir="none"><fa class="${prefix}-sort-icon" fa-angles-up-down></fa></span>`;
        }
      },
      columns: columns,
    });

    this.wireTableEvents();

    // Wait for tableBuilt event to ensure Tabulator is fully initialized
    // This prevents issues with calling methods like replaceData before
    // the renderer is ready
    await new Promise((resolve) => {
      if (this.table.modules?.table && this.table.renderer) {
        // Table is already built
        resolve();
        return;
      }

      let resolved = false;
      const once = () => {
        if (!resolved) {
          resolved = true;
          resolve();
        }
      };

      // tableBuilt fires after renderer is ready
      this.table.on('tableBuilt', once);

      // Fallback: check periodically if table is built
      let attempts = 0;
      const maxAttempts = 10;
      const checkInterval = setInterval(() => {
        attempts++;
        if (this.table.modules?.table && this.table.renderer) {
          clearInterval(checkInterval);
          once();
        } else if (attempts >= maxAttempts) {
          clearInterval(checkInterval);
          once(); // Resolve anyway after max attempts
        }
      }, 50);
    });

    this._initTableScrollbars();

    // If tableHolder not found yet, retry after a short delay
    // This can happen when initTable() is called before Tabulator finishes building
    if (!this._scrollbarInstance) {
      setTimeout(() => this._initTableScrollbars(), 100);
    }

    // Initialize FloatingUI tooltips on column headers
    // Use setTimeout to ensure Tabulator has rendered the header elements
    setTimeout(() => this.initColumnHeaderTooltips(), 100);

    // OverlayScrollbars is now enabled for all LithiumTable instances by default.
    // Set useOverlayScrollbars: false in constructor to disable for specific tables.
  }

  _initTableScrollbars() {
    if (!this.useOverlayScrollbars) {
      log(Subsystems.TABLE, Status.DEBUG, `[${this.cssPrefix}] _initTableScrollbars: disabled by option`);
      return;
    }
    if (this._scrollbarInstance) {
      log(Subsystems.TABLE, Status.DEBUG, `[${this.cssPrefix}] _initTableScrollbars: already present, skipping`);
      return;
    }
    if (!this.container) {
      log(Subsystems.TABLE, Status.DEBUG, `[${this.cssPrefix}] _initTableScrollbars: container missing, skipping`);
      return;
    }

    const tableHolder = this.container.querySelector('.tabulator-tableholder');
    if (!tableHolder) {
      log(Subsystems.TABLE, Status.WARN, `[${this.cssPrefix}] _initTableScrollbars: tableHolder not found`);
      return;
    }

    tableHolder.setAttribute('data-overlayscrollbars-initialize', '');
    this.container.classList.add('lithium-table-osb-enabled');

    log(Subsystems.TABLE, Status.DEBUG, `[${this.cssPrefix}] _initTableScrollbars: calling scrollbarManager.initTabulator`);

    this._scrollbarInstance = scrollbarManager.initTabulator(tableHolder, {
      initialized: () => {
        log(Subsystems.TABLE, Status.DEBUG, `[${this.cssPrefix}] _initTableScrollbars: OSB initialized callback received`);
        this._updateTableScrollbars();
      },
    });

    if (!this._scrollbarInstance) {
      log(Subsystems.TABLE, Status.WARN, `[${this.cssPrefix}] _initTableScrollbars: initTabulator returned null`);
      this.container.classList.remove('lithium-table-osb-enabled');
    } else {
      log(Subsystems.TABLE, Status.DEBUG, `[${this.cssPrefix}] _initTableScrollbars: OSB instance created`);
    }
   }

_updateTableScrollbars() {
    if (!this._scrollbarInstance) {
      return;
    }

    // Debounce all updates into a single requestAnimationFrame
    // This prevents multiple expensive OSB updates from competing with
    // radar animation and other UI during rapid data loading
    if (!this._scrollbarUpdateRaf1) {
      this._scrollbarUpdateRaf1 = requestAnimationFrame(() => {
        this._scrollbarUpdateRaf1 = 0;
        if (this._scrollbarInstance && !this._scrollbarInstance.destroyed) {
          scrollbarManager.update(this._scrollbarInstance);
        }
      });
    }
  }

  // ── Group Icon Animation (delegated to GroupIconAnimator) ─────────────────

  updateGroupIcons() {
    this._groupIconAnimator?.updateGroupIcons();
  }

  wireTableEvents() {
    wireTableEvents(this);
  }



  // ── Row Selection (delegated to selection/row-selection.js) ───────────────

  async handleRowSelected(row) {
    return handleRowSelected(this, row);
  }

  selectDataRow(row) {
    return selectDataRow(this, row);
  }

  getSelectedDataRow() {
    return getSelectedDataRow(this);
  }

  getEditingRow() {
    return getEditingRow(this);
  }

  isCalcRow(row) {
    return isCalcRow(this, row);
  }

  isCalcCell(cell) {
    return isCalcCell(this, cell);
  }

  rowsMatch(rowA, rowB) {
    return rowsMatch(this, rowA, rowB);
  }

  getVisibleRowsAndIndex() {
    return getVisibleRowsAndIndex(this);
  }

  selectRowByIndex(index) {
    return selectRowByIndex(this, index);
  }

  getPageSize() {
    return getPageSize(this);
  }

  async _exitEditModeForNavigation() {
    return exitEditModeForNavigation(this);
  }

  async navigateFirst() {
    return navigateFirst(this);
  }

  async navigateLast() {
    return navigateLast(this);
  }

  async navigatePrevRec() {
    return navigatePrevRec(this);
  }

  async navigateNextRec() {
    return navigateNextRec(this);
  }

  async navigatePrevPage() {
    return navigatePrevPage(this);
  }

  async navigateNextPage() {
    return navigateNextPage(this);
  }

  // ── Data Loading (delegated to data/data-loading.js) ───────────────────────

  async loadData(searchTerm = '', extraParams = {}) {
    return loadData(this, searchTerm, extraParams);
  }

  getSelectedRowId() {
    return getSelectedRowId(this);
  }

  getSelectedRowData() {
    return getSelectedRowData(this);
  }

  autoSelectRow(targetId) {
    return autoSelectRow(this, targetId);
  }

  _getCompositeRowId(rowData, pkFields) {
    return getCompositeRowId$1(rowData, pkFields);
  }

  loadStaticData(rows, options = {}) {
    return loadStaticData(this, rows, options);
  }

  // ── Column Management (delegated to columns/column-management.js) ──────────

  buildSelectorColumn() {
    return buildSelectorColumn(this);
  }

  applyEditModeGate(columns) {
    return applyEditModeGate(this, columns);
  }

  discoverColumns(rows) {
    return discoverColumns(this, rows);
  }

  _detectColtype(rows, field) {
    return detectColtype(rows, field);
  }

  _buildColumnsFromData(data) {
    return buildColumnsFromData(this, data);
  }

  _sortColumnsByPriority(columns) {
    return sortColumnsByPriority(columns);
  }

  // ── Lifecycle ─────────────────────────────────────────────────────────────

  cleanup() {
    this.closeTransientPopups();
  }

  destroy() {
    this.cleanup();

    this.container?.classList?.remove('lithium-table-osb-enabled');

    // Destroy OverlayScrollbars instance
    if (this._scrollbarInstance) {
      scrollbarManager.destroy(this._scrollbarInstance);
      this._scrollbarInstance = null;
    }

    if (this._scrollbarUpdateRaf1) {
      window.cancelAnimationFrame(this._scrollbarUpdateRaf1);
      this._scrollbarUpdateRaf1 = 0;
    }

    if (this.table) {
      this.table.destroy();
      this.table = null;
    }

    // Remove injected sort icon styles
    if (this._sortStyleEl) {
      this._sortStyleEl.remove();
      this._sortStyleEl = null;
    }

    this.hideLoading();
  }

  // ── Template Capture (delegated to template/capture.js) ────────────────────

  captureCurrentState() {
    return captureCurrentState(this);
  }

  _createTemplateColumnFromTableDef(fieldName, colDef) {
    return createTemplateColumnFromTableDef(fieldName, colDef);
  }

  _extractTemplateColumnFromColumn(column) {
    return extractTemplateColumnFromColumn(column);
  }

  // ── Refresh Orchestration (delegated to refresh-orchestrator.js) ────────────

  async reloadConfiguration() {
    return reloadConfiguration(this);
  }

  _getCurrentlySelectedRowId() {
    return getCurrentlySelectedRowId(this);
  }

  // ── Splitter Binding ────────────────────────────────────────────────────────

  /**
   * Bind a LithiumSplitter instance to this table.
   * Must be called AFTER init() and AFTER creating the splitter.
   * @param {LithiumSplitter} splitter
   */
  setSplitter(splitter) {
    this.splitter = splitter;
  }

  // ── Panel Width Persistence (delegated to persistence/panel-width.js) ──────

  getWidthForMode(mode) {
    return getWidthForMode(mode);
  }

  applyPanelWidth(mode) {
    return applyPanelWidth$1(this, mode);
  }

  savePanelPixelWidth(width) {
    return savePanelPixelWidth(this, width);
  }

  loadCollapsedState(defaultValue = false) {
    return loadCollapsedState(this, defaultValue);
  }

  saveCollapsedState(collapsed) {
    return saveCollapsedState(this, collapsed);
  }

  // ── Dirty State ────────────────────────────────────────────────────────────

  /**
   * Synchronous dirty check that also consults the ManagerEditHelper.
   *
   * The editHelper's `checkDirtyState()` is deferred via rAF, so the
   * table's `isDirty` flag may be stale if the user edits a CodeMirror
   * editor and immediately triggers a row change.  This method performs
   * an immediate comparison against the snapshot so callers always get
   * the true dirty state.
   *
   * @returns {boolean}
   */
  isActuallyDirty() {
    if (this.isDirty) return true;
    return this._editHelper?.isAnythingDirty?.() ?? false;
  }

  /**
   * Notify the manager that dirty state has changed.
   * Called whenever isDirty changes, allowing managers to update UI (e.g., Save/Cancel buttons).
   */
  notifyDirtyChange() {
    if (typeof this.onDirtyChange === 'function') {
      this.onDirtyChange(this.isDirty, this.isEditing ? this.getSelectedRowData() : null);
    }
  }

  // ── Column Header Tooltips (delegated to visual/column-tooltips.js) ─────────

  initColumnHeaderTooltips() {
    return initColumnHeaderTooltips(this);
  }

  /**
   * Rebuild table columns using data-first approach after data becomes available.
   * This is called when the table was initialized without data but now has data.
   * @param {Array} data - The loaded data rows
   */
  async _rebuildColumnsWithData(data) {
    if (!this.table || !data || data.length === 0) return;

    log(Subsystems.TABLE, Status.INFO, `[${this.cssPrefix}] Rebuilding columns with data-first approach`);

    // Auto-discover columns from data
    let dataColumns = this._buildColumnsFromData(data);

    // Apply tableDef overlays if available
    if (this.tableDef && this.coltypes) {
      const { columns: tableDefColumns, columnMeta } = resolveColumnsWithMeta(
        this.tableDef,
        this.coltypes,
        { filterEditor: this.createFilterEditorFunction() }
      );
      // Update the column metadata Map
      this._columnMeta = columnMeta;
      dataColumns = this._mergeColumnsWithTableDef(dataColumns, tableDefColumns);
    }

    if (dataColumns.length === 0) return;

    dataColumns = this._sortColumnsByPriority(dataColumns);
    this.applyEditModeGate(dataColumns);

    // Build new column definitions for Tabulator
    const selectorColumn = this.buildSelectorColumn();
    const columns = [selectorColumn, ...dataColumns];
    columns.forEach(col => {
      if (col.title && !col.cssClass?.includes('first-visible-col')) {
        col.cssClass = [col.cssClass, sanitizeColumnTitle(col.title)].filter(Boolean).join(' ');
      }
    });

    // Replace columns on the existing table
    this.table.setColumns(columns);

    // Clear the flag
    this._wasInitializedWithoutData = false;

    log(Subsystems.TABLE, Status.INFO, `[${this.cssPrefix}] Rebuilt with ${dataColumns.length} columns`);
  }

  _mergeColumnsWithTableDef(autoColumns, tableDefColumns) {
    return mergeColumnsWithTableDef(this, autoColumns, tableDefColumns);
  }

  /**
   * Apply default template to tableDef if available.
   * This is the last step before building the table, applied after loading tableDef from Lookup 59.
   *
   * Uses deep merge for param objects (formatterParams, editorParams, etc.) so that
   * coltype defaults are preserved even when templates specify partial overrides.
   */
  _applyDefaultTemplate() {
    if (!this.tableDef || !this.storageKey) return;

    const defaultTemplateKey = `lithium_table_template_${this.storageKey}_Default`;
    try {
      const defaultTemplateJson = localStorage.getItem(defaultTemplateKey);
      if (defaultTemplateJson) {
        const defaultTemplate = JSON.parse(defaultTemplateJson);
        log(Subsystems.TABLE, Status.INFO,
          `[${this.cssPrefix}] Applying default template from localStorage`);

        // Merge default template with tableDef
        // Use deep merge for param objects to preserve coltype defaults
        const mergedColumns = { ...this.tableDef.columns };
        if (defaultTemplate.columns) {
          for (const [field, col] of Object.entries(defaultTemplate.columns)) {
            if (mergedColumns[field]) {
              mergedColumns[field] = deepMergeParams(mergedColumns[field], col);
            } else {
              mergedColumns[field] = col;
            }
          }
        }

        // Apply other template properties, excluding columns which are already merged
        const { columns: _col, ...templateProps } = defaultTemplate;
        void _col; // unused but explicitly excluded
        this.tableDef = { ...this.tableDef, ...templateProps, columns: mergedColumns };
      }
    } catch (e) {
      log(Subsystems.TABLE, Status.WARN,
        `[${this.cssPrefix}] Failed to apply default template: ${e.message}`);
    }
  }

  /**
   * Get column definition from tableDef by field name.
   * @param {string} field - The field name
   * @returns {Object|null} The column definition or null
   */
  getColumnDefinition(field) {
    if (!this.tableDef?.columns) return null;

    for (const [, colDef] of Object.entries(this.tableDef.columns)) {
      if (colDef.field === field) {
        return colDef;
      }
    }
    return null;
  }

  // ── Abstract Methods (to be implemented by subclasses) ─────────────────────

  buildNavigator() { /* To be implemented */ }
  createFilterEditor() { return null; }
  updateMoveButtonState() { /* To be implemented */ }
  updateDuplicateButtonState() { /* To be implemented */ }
  closeTransientPopups() { /* To be implemented */ }
  enterEditMode() { /* To be implemented */ }
  exitEditMode() { /* To be implemented */ }
  updateSaveCancelButtonState() { /* To be implemented */ }
  autoSaveBeforeRowChange() { return Promise.resolve(true); }
  updateSelectorCell() { /* To be implemented */ }
  commitActiveCellEdit() { /* To be implemented */ }
  queueCellEdit() { /* To be implemented */ }
  updateVisibleColumnClasses() { /* To be implemented */ }
  applyVisibleColumnClassesToRow() { /* To be implemented */ }
  setupKeyboardHandling() { /* To be implemented */ }
  showLoading() { /* To be implemented */ }
  hideLoading() { /* To be implemented */ }
  setupPersistence() { /* To be implemented */ }
  restoreLayoutMode() { return null; }
  updateEditButtonState() { /* To be implemented */ }
}

/**
 * LithiumTable Operations Module
 *
 * CRUD operations and edit mode handling:
 * - Add, duplicate, edit, save, cancel, delete operations
 * - Edit mode state management
 * - Dirty state tracking and auto-save
 * - Keyboard handling
 *
 * @module core/lithium-table-ops
 */


// ── Mixin for CRUD Operations ───────────────────────────────────────────────

const LithiumTableOpsMixin = {
  // ── CRUD Operations ───────────────────────────────────────────────────────

  async handleAdd() {
    if (!this.table || this.readonly || this.alwaysEditable) return;

    const newRow = await this.table.addRow({}, true);

    if (newRow) {
      const selectedRows = this.table.getSelectedRows();
      if (selectedRows.length > 0) {
        this.table.deselectRow(selectedRows);
      }
      newRow.select();
      // Defer scroll to ensure Tabulator has rendered the row
      requestAnimationFrame(() => {
        newRow?.scrollTo?.('center', false);
      });
      await this.enterEditMode(newRow);

      // Focus first editable cell
      const cells = newRow.getCells();
      const firstEditable = cells.find(cell => {
        const field = cell.getField();
        return field !== '_selector' && this.columnEditors.has(field);
      });
      if (firstEditable) {
        this.queueCellEdit(firstEditable);
      }
    }
  },

  async handleDuplicate() {
    if (!this.table || this.readonly || this.alwaysEditable) return;

    const selected = this.table.getSelectedRows();
    if (selected.length === 0) {
      toast.info('No Row Selected', { description: 'Please select a row to duplicate', duration: 3000 });
      return;
    }

    const originalData = selected[0].getData();
    let duplicateData;

    // If manager provided custom duplicate logic, use it
    if (typeof this.onDuplicate === 'function') {
      try {
        duplicateData = await this.onDuplicate(originalData);
        if (!duplicateData) {
          // Custom handler aborted or handled the operation itself
          return;
        }
      } catch (error) {
        toast.error('Duplicate Failed', {
          serverError: error.serverError,
          subsystem: 'Conduit',
          duration: 6000,
        });
        return;
      }
    } else {
      // Default duplicate behavior
      duplicateData = { ...originalData };

      // Remove primary key fields for new record
      const pkFields = this.primaryKeyField;
      if (Array.isArray(pkFields)) {
        pkFields.forEach(f => delete duplicateData[f]);
      } else if (pkFields) {
        delete duplicateData[pkFields];
      } else {
        delete duplicateData.id;
      }

      // Append " (Copy)" to name if it exists
      if (duplicateData.name) {
        duplicateData.name = `${duplicateData.name} (Copy)`;
      }
    }

    const newRow = await this.table.addRow(duplicateData, true);

    if (newRow) {
      const selectedRows = this.table.getSelectedRows();
      if (selectedRows.length > 0) {
        this.table.deselectRow(selectedRows);
      }
      newRow.select();
      // Defer scroll to ensure Tabulator has rendered the row
      requestAnimationFrame(() => {
        newRow?.scrollTo?.('center', false);
      });
      await this.enterEditMode(newRow);
    }
  },

  async handleEdit() {
    if (!this.table || this.readonly || this.alwaysEditable) return;

    const selected = this.table.getSelectedRows();
    if (selected.length === 0) {
      toast.info('No Row Selected', { description: 'Please select a row to edit', duration: 3000 });
      return;
    }

    // If already in edit mode, save (if dirty) and exit — don't toggle back on
    if (this.isEditing) {
      // Synchronous dirty check — the rAF-deferred isDirty may be stale
      const actuallyDirty = this.isActuallyDirty();
      if (actuallyDirty) {
        await this.handleSave();
      } else {
        await this.exitEditMode('toggle');
      }
      return;
    }

    // Enter edit mode for the selected row
    await this.enterEditMode(selected[0]);
  },

  async handleSave() {
    if (!this.table || this.readonly) return;

    if (!this.isDirty) {
      toast.info('No Changes', { description: 'Nothing to save', duration: 3000 });
      return;
    }

    const selected = this.table.getSelectedRows();
    if (selected.length === 0) {
      toast.info('No Row Selected', { description: 'Please select a row to save', duration: 3000 });
      return;
    }

    try {
      await this.executeSave(selected[0]);
      this.isDirty = false;
      this.originalRowData = null;
      this.updateSaveCancelButtonState();
      this.notifyDirtyChange();
      await this.exitEditMode('save');

      toast.success('Changes Saved', { duration: 3000 });
    } catch (error) {
      toast.error('Save Failed', {
        serverError: error.serverError,
        subsystem: 'Conduit',
        duration: 8000,
      });
    }
  },

  async executeSave(row) {
    // If the manager provided a custom save callback, use it exclusively.
    // This is the standard pattern — managers assemble their own API params
    // from table row data + registered editor content.
    if (typeof this.onExecuteSave === 'function') {
      await this.onExecuteSave(row, this._editHelper);
      return;
    }

    // Default save: use insert/update queryRefs for simple table-only saves
    const rowData = row.getData();
    const pkField = this.primaryKeyField || 'id';
    const pkValue = rowData[pkField];
    const isInsert = pkValue == null || pkValue === '' || pkValue === 0;

    if (isInsert && this.queryRefs.insertQueryRef) {
      const payload = { ...rowData };
      delete payload[pkField];

      const result = await authQuery(this.app.api, this.queryRefs.insertQueryRef, { JSON: payload });

      // Update the row with the new ID from the server
      if (result?.[0]?.[pkField] != null) {
        row.update({ [pkField]: result[0][pkField] });
      }
    } else if (!isInsert && this.queryRefs.updateQueryRef) {
      await authQuery(this.app.api, this.queryRefs.updateQueryRef, {
        INTEGER: { [pkField.toUpperCase()]: pkValue },
        JSON: rowData,
      });
    } else {
      // No appropriate queryRef configured — save not possible
      const operation = isInsert ? 'insertQueryRef' : 'updateQueryRef';
      log(Subsystems.TABLE, Status.WARN,
        `[LithiumTable] No ${operation} configured — save skipped`);
      throw Object.assign(new Error(`No ${operation} configured for this table`), {
        serverError: {
          error: 'Save Not Configured',
          message: `No ${operation} has been configured for this table. Please check the table definition.`,
          query_ref: isInsert ? this.queryRefs.insertQueryRef : this.queryRefs.updateQueryRef,
        },
      });
    }
  },

  async handleCancel() {
    if (!this.isDirty) {
      await this.exitEditMode('cancel');
      return;
    }

    // Revert table row changes
    if (this.originalRowData && this.table) {
      const selected = this.table.getSelectedRows();
      if (selected.length > 0) {
        selected[0].update(this.originalRowData);
      }
    }

    // Revert external editor changes (CodeMirror, SunEditor, etc.)
    // via the ManagerEditHelper that holds the snapshot
    if (this._editHelper?.restoreEditorSnapshots) {
      this._editHelper.restoreEditorSnapshots();
    }

    this.isDirty = false;
    this.originalRowData = null;
    this.notifyDirtyChange();
    await this.exitEditMode('cancel');

    toast.info('Changes Cancelled', { description: 'All changes have been reverted', duration: 3000 });
  },

  async handleDelete() {
    if (!this.table || this.readonly || this.alwaysEditable) return;

    const selected = this.table.getSelectedRows();
    if (selected.length === 0) {
      toast.info('No Row Selected', { description: 'Please select a row to delete', duration: 3000 });
      return;
    }

    const pkField = this.primaryKeyField || 'id';
    const selectedId = selected[0].getData()?.[pkField];
    const rowName = selected[0].getData()?.name || '';

    // Confirm deletion
    const confirmed = window.confirm(
      rowName
        ? `Delete "${rowName}" (${pkField}: ${selectedId})?\n\nThis action cannot be undone.`
        : `Are you sure you want to delete this row?\n\nThis action cannot be undone.`
    );

    if (!confirmed) return;

    // Execute delete on server
    if (this.queryRefs.deleteQueryRef && selectedId != null) {
      try {
        await authQuery(this.app.api, this.queryRefs.deleteQueryRef, {
          INTEGER: { [pkField.toUpperCase()]: selectedId },
        });
      } catch (error) {
        toast.error('Delete Failed', {
          serverError: error.serverError,
          subsystem: 'Conduit',
          duration: 8000,
        });
        return;
      }
    }

    // Find next row to select after deletion
    const allRows = this.table.getRows('active');
    const currentIndex = allRows.findIndex(row => row === selected[0]);
    const nextRow = allRows[currentIndex + 1] || allRows[currentIndex - 1] || null;

    // Delete the row
    selected.forEach(row => row.delete());

    // Select next row if available
    if (nextRow) {
      nextRow.select();
      // Defer scroll to ensure Tabulator has rendered the row
      requestAnimationFrame(() => {
        nextRow?.scrollTo?.('center', false);
      });
    }

    await this.exitEditMode('cancel');

    toast.success('Row Deleted', { duration: 3000 });
  },

  // ── Edit Mode ─────────────────────────────────────────────────────────────

  async enterEditMode(row, targetCell = null) {
    if (!this.table || this.readonly || this.alwaysEditable) return;

    this.setEditTransitionState(true);

    try {
      if (!row) {
        const selected = this.table.getSelectedRows();
        if (selected.length === 0) return;
        row = selected[0];
      }

      // Use _getCompositeRowId for consistent row ID handling
      const pkFields = this.primaryKeyField;
      const rowId = this._getCompositeRowId(row.getData(), pkFields);

      if (this.isEditing && this.editingRowId === rowId) return;

      // Exit any existing edit mode first
      if (this.isEditing) {
        await this.exitEditMode();
      }

      this.editingRowId = rowId;
      this.isEditing = true;
      this.originalRowData = { ...row.getData() };

      this.container?.classList.add('lithium-edit-mode');
      this.updateSelectorCell(row, true);
      this.updateSaveCancelButtonState();
      this.updateEditButtonState();

      this.onEditModeChange(true, row.getData());

      if (targetCell) {
        this.queueCellEdit(targetCell);
      }

      log(Subsystems.TABLE, Status.INFO,
        `[LithiumTable] Entered edit mode (row ${rowId || 'unknown'})`);
    } finally {
      requestAnimationFrame(() => this.setEditTransitionState(false));
    }
  },

  async exitEditMode(reason = 'cancel') {
    if (this.alwaysEditable || !this.isEditing) return;

    this.setEditTransitionState(true);

    try {
      const previousId = this.editingRowId;

      this.cancelActiveCellEdit();

      this.editingRowId = null;
      this.isEditing = false;

      this.container?.classList.remove('lithium-edit-mode');
      this.updateEditingIndicator();
      this.updateSaveCancelButtonState();
      this.updateEditButtonState();

      if (reason !== 'save') {
        this.isDirty = false;
        this.originalRowData = null;
        this.notifyDirtyChange();
      }

      this.onEditModeChange(false, null);

      log(Subsystems.TABLE, Status.INFO,
        `[LithiumTable] Exited edit mode (row ${previousId}, reason: ${reason})`);
    } finally {
      requestAnimationFrame(() => this.setEditTransitionState(false));
    }
  },

  setEditTransitionState(pending) {
    this.editTransitionPending = pending;
    this.container?.classList.toggle('lithium-edit-transition', pending);
    this.navigatorContainer?.classList.toggle('lithium-edit-transition', pending);
  },

  // Save/Cancel buttons have been moved to the Manager footer.
  // This method is kept as a no-op for compatibility with existing callers.
  updateSaveCancelButtonState() {
    // No-op: Save/Cancel buttons are now in the Manager footer, not the Navigator.
  },

  async autoSaveBeforeRowChange() {
    // GUARD: Prevent concurrent save operations that could cause duplicate toasts
    if (this._saveInProgress) {
      log(Subsystems.TABLE, Status.DEBUG,
        `[LithiumTable] autoSaveBeforeRowChange: save already in progress, skipping`);
      return true;
    }

    try {
      // IMPORTANT: Save the EDITING row, not the newly-selected row.
      // By the time this method is called, Tabulator has already selected
      // the new row.  getSelectedRows() would return the wrong one.
      const row = this.getEditingRow();

      if (!row) return true;

      // GUARD: Only save if actually dirty. The caller should have checked,
      // but this prevents spurious saves and duplicate toasts.
      const actuallyDirty = this.isActuallyDirty?.() || this.isDirty;
      if (!actuallyDirty) {
        log(Subsystems.TABLE, Status.DEBUG,
          `[LithiumTable] autoSaveBeforeRowChange: row not dirty, skipping save`);
        return true;
      }

      this._saveInProgress = true;

      await this.executeSave(row);
      this.isDirty = false;
      this.updateSaveCancelButtonState();
      this.notifyDirtyChange();

      toast.success('Changes Saved', { duration: 3000 });

      return true;
    } catch (error) {
      toast.error('Save Failed', {
        serverError: error.serverError,
        subsystem: 'Conduit',
        duration: 6000,
      });
      return false;
    } finally {
      this._saveInProgress = false;
    }
  },

  updateEditingIndicator() {
    if (!this.table) return;
    const selected = this.table.getSelectedRows();
    if (selected.length > 0) {
      this.updateSelectorCell(selected[0], true);
    }
  },

  // ── Cell Editing ──────────────────────────────────────────────────────────

  queueCellEdit(cell) {
    if (!cell) return;

    const field = cell.getField?.();
    if (!field || !this.columnEditors.has(field)) return;

    const editorConfig = this.columnEditors.get(field) || {};

    requestAnimationFrame(() => {
      requestAnimationFrame(() => {
        if (!this.isEditing) return;

        const row = cell.getRow?.();
        if (!row || this.isCalcRow(row)) return;

        const pkFields = this.primaryKeyField;
        const rowId = this._getCompositeRowId(row.getData?.(), pkFields);
        const isEditingRow = this.editingRowId != null
          ? rowId === this.editingRowId
          : row.isSelected?.();

        if (!isEditingRow) return;

        cell.edit();

        requestAnimationFrame(() => {
          this.openActiveCellEditor?.(cell, editorConfig);
        });
      });
    });
  },

  openActiveCellEditor(cell, editorConfig = {}) {
    const editorType = typeof editorConfig?.editor === 'string'
      ? editorConfig.editor
      : null;

    if (editorType !== 'select' && editorType !== 'list') return;

    const cellEl = cell?.getElement?.();
    if (!cellEl) return;

    const editorEl = cellEl.querySelector('select, input, textarea, [role="combobox"]');
    if (!editorEl) return;

    editorEl.focus?.({ preventScroll: true });

    if (editorType === 'select' && typeof editorEl.showPicker === 'function') {
      try {
        editorEl.showPicker();
        return;
      } catch {
        // Fall through to synthetic events for browsers that reject showPicker().
      }
    }

    const mouseEventOptions = {
      bubbles: true,
      cancelable: true,
      view: window,
    };

    editorEl.dispatchEvent(new MouseEvent('pointerdown', mouseEventOptions));
    editorEl.dispatchEvent(new MouseEvent('mousedown', mouseEventOptions));
    editorEl.dispatchEvent(new MouseEvent('click', mouseEventOptions));

    if (editorType === 'list') {
      editorEl.dispatchEvent(new KeyboardEvent('keydown', {
        key: 'ArrowDown',
        code: 'ArrowDown',
        bubbles: true,
      }));
    }
  },

  commitActiveCellEdit(nextCell = null) {
    const active = document.activeElement;
    if (!active || !this.container?.contains(active)) return;

    const activeCellEl = active.closest?.('.tabulator-cell');
    const nextCellEl = nextCell?.getElement?.() || null;
    if (activeCellEl && nextCellEl && activeCellEl === nextCellEl) {
      return;
    }

    active.blur();
  },

  cancelActiveCellEdit() {
    this.commitActiveCellEdit();
  },

  // ── Keyboard Handling ─────────────────────────────────────────────────────

  setupKeyboardHandling() {
    this.container?.setAttribute('tabindex', '0');
    this.container?.addEventListener('keydown', (e) => {
      this.handleTableKeydown(e);
    });
  },

  handleTableKeydown(e) {
    if (!this.table) return;

    // Check if we're currently editing a cell
    const activeEl = document.activeElement;
    const isEditingCell = activeEl?.closest?.('.tabulator-cell.tabulator-editing');

    // Handle Ctrl+Enter to save (when in edit mode)
    if (e.key === 'Enter' && e.ctrlKey) {
      if (this.isEditing) {
        e.preventDefault();
        e.stopPropagation();
        this.handleSave();
        return;
      }
    }

    // Handle Escape to cancel edit mode (always prevent propagation when in edit mode)
    if (e.key === 'Escape') {
      if (this.isEditing) {
        e.preventDefault();
        e.stopPropagation();
        this.handleCancel();
        return;
      }
      // If not in edit mode, let Escape propagate for other handlers
    }

    // If editing a cell, allow normal editing behavior for other keys
    if (isEditingCell) {
      return;
    }

    switch (e.key) {
      case 'ArrowUp':
        e.preventDefault();
        this.navigatePrevRec();
        break;
      case 'ArrowDown':
        e.preventDefault();
        this.navigateNextRec();
        break;
      case 'PageUp':
        e.preventDefault();
        this.navigatePrevPage();
        break;
      case 'PageDown':
        e.preventDefault();
        this.navigateNextPage();
        break;
      case 'Home':
        e.preventDefault();
        this.navigateFirst();
        break;
      case 'End':
        e.preventDefault();
        this.navigateLast();
        break;
      case 'Enter':
        e.preventDefault();
        if (!this.readonly && !this.alwaysEditable) {
          const selected = this.table.getSelectedRows();
          if (selected.length > 0) {
            this.enterEditMode(selected[0]);
          }
        }
        break;
      case 'F2':
        if (!this.readonly && !this.alwaysEditable) {
          e.preventDefault();
          this.handleEdit();
        }
        break;
      case 'Delete':
        if (!this.readonly && !this.alwaysEditable && e.ctrlKey) {
          e.preventDefault();
          this.handleDelete();
        }
        break;
    }
  },
};

/**
 * Persistence Module
 *
 * State persistence for table settings and selection.
 *
 * @module tables/persistence/persistence
 */


/**
 * Save selected row ID to localStorage
 * @param {Object} table - LithiumTable instance
 * @param {string|number} rowId - Row ID
 */
function saveSelectedRowId(table, rowId) {
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
function restoreSelectedRowId(table) {
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
function clearSavedRowSelection(table) {
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
function saveFiltersVisible(table, visible) {
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
function restoreFiltersVisible(table) {
  try {
    const stored = localStorage.getItem(`${table.storageKey}_filters_visible`);
    return stored === '1';
  } catch (_e) {
    // Ignore storage errors
  }
  return false;
}

/**
 * Table Event Handlers
 *
 * Navigator button handlers and filter/group operations.
 *
 * @module tables/events/event-handlers
 */


/**
 * Handle refresh button click.
 * @param {Object} table - LithiumTable instance
 */
async function handleRefresh(table) {
  const refreshBtn = table.navigatorContainer?.querySelector(`#${table.cssPrefix}-nav-refresh`);
  if (refreshBtn) {
    refreshBtn.classList.add('lithium-nav-refresh-spin');
    setTimeout(() => refreshBtn.classList.remove('lithium-nav-refresh-spin'), 750);
  }

  if (table.isEditing && table.isDirty) {
    await table.handleSave?.();
  } else if (table.isEditing) {
    await table.exitEditMode?.('cancel');
  }

  // If this table uses Lookup 59 (indicated by lookupKeyIdx), refresh the schemas first
  // This ensures any changes to Lookup 59 are picked up without requiring a page reload
  if (table.lookupKeyIdx != null) {
    try {
      await refreshTabulatorSchemas();
      // After refreshing Lookup 59, reload the table configuration (schema + template)
      await table.reloadConfiguration?.();
      return;
    } catch (err) {
      log(Subsystems.TABLE, Status.WARN, `[LithiumTable] Schema refresh failed: ${err.message}`);
    }
  }

  // Fallback: just refresh data if no Lookup 59
  if (typeof table.onRefresh === 'function') {
    table.onRefresh();
  } else {
    table.loadData?.();
  }
}

/**
 * Handle filter button click - toggle header filters visibility.
 * @param {Object} table - LithiumTable instance
 */
function handleFilter(table) {
  table.filtersVisible = !table.filtersVisible;

  const filterBtn = table.navigatorContainer?.querySelector(`#${table.cssPrefix}-nav-filter`);
  if (filterBtn) {
    filterBtn.classList.toggle('lithium-nav-btn-active', table.filtersVisible);
  }

  saveFiltersVisible(table, table.filtersVisible);

  const transitionMs = parseInt(getComputedStyle(document.documentElement)
    .getPropertyValue('--transition-duration').trim()) || 330;

  if (table.filtersVisible) {
    toggleHeaderFilters(table, true);

    const filterInputs = table.container.querySelectorAll('.tabulator-header-filter');
    filterInputs.forEach((el) => {
      el.style.height = '0';
      el.style.minHeight = '0';
      el.style.maxHeight = '0';
      el.style.opacity = '0';
    });

    requestAnimationFrame(() => {
      requestAnimationFrame(() => {
        filterInputs.forEach((el) => {
          el.style.removeProperty('height');
          el.style.removeProperty('min-height');
          el.style.removeProperty('max-height');
          el.style.removeProperty('opacity');
        });
        table.container.classList.add('lithium-filters-visible');
      });
    });
  } else {
    table.container.classList.remove('lithium-filters-visible');
    setTimeout(() => {
      toggleHeaderFilters(table, false);
      table.table?.clearHeaderFilter();
    }, transitionMs);
  }
}

/**
 * Toggle header filters on/off by updating column definitions.
 * @param {Object} table - LithiumTable instance
 * @param {boolean} visible - Whether header filters should be visible
 */
function toggleHeaderFilters(table, visible) {
  if (!table.table) return;

  const columns = table.table.getColumns();
  const filterEditorFn = table.createFilterEditorFunction();

  const updatedColumns = columns.map((column) => {
    const definition = column.getDefinition();

    if (definition.field === '_selector') {
      return definition;
    }

    if (visible) {
      return {
        ...definition,
        headerFilter: filterEditorFn,
        headerFilterFunc: 'like',
      };
    } else {
      // eslint-disable-next-line no-unused-vars
      const { headerFilter: _, headerFilterFunc: __, headerFilterParams: ___, ...rest } = definition;
      return rest;
    }
  });

  table.table.setColumns(updatedColumns);

  // Re-enforce selectableRows: 1 after setColumns() - Tabulator may lose this setting
  table.table.modules.select?.setSelectionMode?.(1);

  requestAnimationFrame(() => {
    clearColumnInlineHeights(table);
  });
}

/**
 * Clear inline height styles from column headers.
 * @param {Object} table - LithiumTable instance
 */
function clearColumnInlineHeights(table) {
  if (!table.container) return;
  const cols = table.container.querySelectorAll('.tabulator-header .tabulator-col');
  cols.forEach((col) => {
    col.style.removeProperty('height');
    col.style.removeProperty('min-height');
  });
}

/**
 * Expand all grouped rows.
 * @param {Object} table - LithiumTable instance
 */
function expandAll(table) {
  if (!table.table) return;
  // NOTE: keep the prior _groupVisibilityState intact so updateGroupIcons()
  // can diff old→new and animate only the groups whose state actually
  // changed. Clearing it would make all groups "first sight" and skip
  // animation entirely.
  const currentGroupBy = table.table.options.groupBy;
  table.table.setGroupStartOpen(true);
  // Clear and re-apply grouping to trigger the change
  table.table.setGroupBy(false);
  table.table.setGroupBy(currentGroupBy);
  // Update icons - state comparison will animate changed groups
  setTimeout(() => table.updateGroupIcons(), 16);
}

/**
 * Collapse all grouped rows.
 * @param {Object} table - LithiumTable instance
 */
function collapseAll(table) {
  if (!table.table) return;
  // See note on expandAll() — we intentionally DO NOT clear
  // _groupVisibilityState here.
  const currentGroupBy = table.table.options.groupBy;
  table.table.setGroupStartOpen(false);
  // Clear and re-apply grouping to trigger the change
  table.table.setGroupBy(false);
  table.table.setGroupBy(currentGroupBy);
  // Update icons - state comparison will animate changed groups
  setTimeout(() => table.updateGroupIcons(), 16);
}

/**
 * Column Manager - State Management Module
 *
 * Dirty state tracking, persistence, and settings management.
 *
 * @module tables/column-manager/cm-state
 */


/**
 * Update save/cancel button state based on dirty flag
 * @param {Object} cm - ColumnManager instance
 */
function updateButtonState(cm) {
  const isRowEditing = !!cm.columnTable?.isEditing;
  const rowDirty = isRowEditing && !!cm.columnTable?.isActuallyDirty?.();
  const popupDirty = !!cm.isDirty;

  if (cm.saveBtn) {
    cm.saveBtn.disabled = isRowEditing ? !rowDirty : !popupDirty;
    cm.saveBtn.classList.toggle('disabled', cm.saveBtn.disabled);
    cm.saveBtn.title = isRowEditing ? 'Save Row' : 'Apply All Changes';
  }
  if (cm.cancelBtn) {
    cm.cancelBtn.disabled = isRowEditing ? !rowDirty : !popupDirty;
    cm.cancelBtn.classList.toggle('disabled', cm.cancelBtn.disabled);
    cm.cancelBtn.title = isRowEditing ? 'Cancel Row Changes' : 'Discard All Changes';
  }
}

/**
 * Recompute popup dirty state from current table rows.
 * @param {Object} cm - ColumnManager instance
 */
function syncDirtyState(cm) {
  const previousDirty = cm.isDirty;
  cm.isDirty = hasPendingChanges(cm);
  updateButtonState(cm);

  if (previousDirty !== cm.isDirty) {
    log(Subsystems.TABLE, Status.DEBUG,
      `[ColumnManager] Dirty state: ${cm.isDirty ? 'dirty' : 'clean'}`);
  }
}

/**
 * Clear dirty state
 * @param {Object} cm - ColumnManager instance
 */
function clearDirty(cm) {
  cm.isDirty = false;
  updateButtonState(cm);
  log(Subsystems.TABLE, Status.DEBUG, '[ColumnManager] Cleared dirty state');
}

/**
 * Check whether the popup state differs from the last applied baseline.
 * @param {Object} cm - ColumnManager instance
 * @returns {boolean} True if there are pending changes
 */
function hasPendingChanges(cm) {
  const currentRows = normalizeColumnRows(getCurrentColumnTableData(cm));
  const baselineRows = normalizeColumnRows(cm.columnData || []);
  return JSON.stringify(currentRows) !== JSON.stringify(baselineRows);
}

/**
 * Normalize column rows for comparison
 * @param {Array} rows - Column rows
 * @returns {Array} Normalized rows
 */
function normalizeColumnRows(rows = []) {
  return rows.map((row, index) => ({
    order: Number.isFinite(Number(row.order)) ? Number(row.order) : index + 1,
    visible: row.visible !== false,
    editable: row.editable !== false,
    field_name: row.field_name || row.field || '',
    column_name: row.column_name || '',
    format: row.format || 'string',
    summary: row.summary || 'none',
    alignment: row.alignment || 'left',
    width: row.width == null || row.width === '' ? null : Number(row.width),
    category: row.category || 'Default',
    field: row.field || row.field_name || '',
  }));
}

/**
 * Get current column table data
 * @param {Object} cm - ColumnManager instance
 * @returns {Array} Current data
 */
function getCurrentColumnTableData(cm) {
  if (!cm.columnTable?.table) return cm.columnData || [];
  return cm.columnTable.table.getRows().map((row) => ({ ...row.getData() }));
}

/**
 * Save setting to localStorage
 * @param {Object} cm - ColumnManager instance
 * @param {string} key - Setting key
 * @param {*} value - Setting value
 */
function saveSetting(cm, key, value) {
  try {
    localStorage.setItem(`${cm.storageKey}_${key}`, JSON.stringify(value));
  } catch (_e) {
    // Ignore storage errors
  }
}

/**
 * Restore setting from localStorage
 * @param {Object} cm - ColumnManager instance
 * @param {string} key - Setting key
 * @param {*} defaultValue - Default value if not found
 * @returns {*} Restored value or default
 */
function restoreSetting(cm, key, defaultValue = null) {
  try {
    const stored = localStorage.getItem(`${cm.storageKey}_${key}`);
    if (stored !== null) {
      return JSON.parse(stored);
    }
  } catch (_e) {
    // Ignore storage errors
  }
  return defaultValue;
}

/**
 * Handle primary save action
 * @param {Object} cm - ColumnManager instance
 */
async function handlePrimarySave(cm) {
  if (cm.columnTable?.isEditing) {
    if (!cm.columnTable.isActuallyDirty?.()) {
      // No edits in the table, but still apply column changes and close
      await cm.applyAllChangesToParent();
      cm.close();
      return;
    }
    await cm.columnTable.handleSave();
    syncDirtyState(cm);
  }

  // Always apply changes and close, regardless of dirty state
  await cm.applyAllChangesToParent();
  cm.close();
}

/**
 * Handle primary cancel action
 * @param {Object} cm - ColumnManager instance
 */
async function handlePrimaryCancel(cm) {
  if (cm.columnTable?.isEditing) {
    if (!cm.columnTable.isActuallyDirty?.()) {
      // No edits in the table, just discard and close
      await cm.discardAllChanges();
      cm.close();
      return;
    }
    await cm.columnTable.handleCancel();
    syncDirtyState(cm);
  }

  // Always discard changes and close, regardless of dirty state
  await cm.discardAllChanges();
  cm.close();
}

/**
 * Column Manager - Drag and Resize Module
 *
 * Popup resize handling (four corners), drag to move, and keyboard events.
 *
 * @module tables/column-manager/cm-drag-resize
 */


/**
 * Handle resize start
 * @param {Object} cm - ColumnManager instance
 * @param {MouseEvent} e - Mouse event
 */
function handleResizeStart(cm, e) {
  e.preventDefault();
  e.stopPropagation();

  cm.isResizing = true;
  cm.popup.classList.add('resizing');

  cm.resizeStartX = e.clientX;
  cm.resizeStartY = e.clientY;

  const rect = cm.popup.getBoundingClientRect();
  cm.popupStartWidth = rect.width;
  cm.popupStartHeight = rect.height;
  cm.popupStartLeft = rect.left;
  cm.popupStartTop = rect.top;

  // Determine which corner based on event target
  const handle = e.target;
  if (handle.classList.contains('col-manager-resize-handle-tl')) {
    cm.resizeCorner = 'tl';
  } else if (handle.classList.contains('col-manager-resize-handle-tr')) {
    cm.resizeCorner = 'tr';
  } else if (handle.classList.contains('col-manager-resize-handle-bl')) {
    cm.resizeCorner = 'bl';
  } else {
    cm.resizeCorner = 'br';
  }

  document.addEventListener('mousemove', cm.handleResizeMove);
  document.addEventListener('mouseup', cm.handleResizeEnd);
}

/**
 * Handle resize move
 * @param {Object} cm - ColumnManager instance
 * @param {MouseEvent} e - Mouse event
 */
function handleResizeMove(cm, e) {
  if (!cm.isResizing) return;

  const deltaX = e.clientX - cm.resizeStartX;
  const deltaY = e.clientY - cm.resizeStartY;

  let newWidth = cm.popupStartWidth;
  let newHeight = cm.popupStartHeight;
  let newLeft = cm.popupStartLeft;
  let newTop = cm.popupStartTop;

  const minWidth = 300;
  const minHeight = 200;
  const maxWidth = window.innerWidth * 0.95;
  const maxHeight = window.innerHeight * 0.95;

  switch (cm.resizeCorner) {
    case 'br':
      newWidth = Math.max(minWidth, Math.min(maxWidth, cm.popupStartWidth + deltaX));
      newHeight = Math.max(minHeight, Math.min(maxHeight, cm.popupStartHeight + deltaY));
      break;
    case 'bl':
      newWidth = Math.max(minWidth, Math.min(maxWidth, cm.popupStartWidth - deltaX));
      newHeight = Math.max(minHeight, Math.min(maxHeight, cm.popupStartHeight + deltaY));
      if (newWidth !== cm.popupStartWidth - deltaX) {
        const actualDeltaX = cm.popupStartWidth - newWidth;
        newLeft = cm.popupStartLeft + actualDeltaX;
      } else {
        newLeft = cm.popupStartLeft + deltaX;
      }
      break;
    case 'tr':
      newWidth = Math.max(minWidth, Math.min(maxWidth, cm.popupStartWidth + deltaX));
      newHeight = Math.max(minHeight, Math.min(maxHeight, cm.popupStartHeight - deltaY));
      if (newHeight !== cm.popupStartHeight - deltaY) {
        const actualDeltaY = cm.popupStartHeight - newHeight;
        newTop = cm.popupStartTop + actualDeltaY;
      } else {
        newTop = cm.popupStartTop + deltaY;
      }
      break;
    case 'tl':
      newWidth = Math.max(minWidth, Math.min(maxWidth, cm.popupStartWidth - deltaX));
      newHeight = Math.max(minHeight, Math.min(maxHeight, cm.popupStartHeight - deltaY));
      if (newWidth !== cm.popupStartWidth - deltaX) {
        const actualDeltaX = cm.popupStartWidth - newWidth;
        newLeft = cm.popupStartLeft + actualDeltaX;
      } else {
        newLeft = cm.popupStartLeft + deltaX;
      }
      if (newHeight !== cm.popupStartHeight - deltaY) {
        const actualDeltaY = cm.popupStartHeight - newHeight;
        newTop = cm.popupStartTop + actualDeltaY;
      } else {
        newTop = cm.popupStartTop + deltaY;
      }
      break;
  }

  cm.popup.style.width = `${newWidth}px`;
  cm.popup.style.height = `${newHeight}px`;

  if (cm.resizeCorner === 'bl' || cm.resizeCorner === 'tl') {
    cm.popup.style.left = `${newLeft}px`;
  }
  if (cm.resizeCorner === 'tr' || cm.resizeCorner === 'tl') {
    cm.popup.style.top = `${newTop}px`;
  }

  // Redraw column table to fit new dimensions
  if (cm.columnTable?.table) {
    cm.columnTable.table.redraw();
  }
}

/**
 * Handle resize end
 * @param {Object} cm - ColumnManager instance
 */
function handleResizeEnd(cm) {
  cm.isResizing = false;
  cm.popup.classList.remove('resizing');

  document.removeEventListener('mousemove', cm.handleResizeMove);
  document.removeEventListener('mouseup', cm.handleResizeEnd);

  // Save dimensions
  saveSetting(cm, 'width', parseInt(cm.popup.style.width, 10));
  saveSetting(cm, 'height', parseInt(cm.popup.style.height, 10));
}

/**
 * Handle drag start (header)
 * @param {Object} cm - ColumnManager instance
 * @param {MouseEvent} e - Mouse event
 */
function handleDragStart(cm, e) {
  // Allow dragging from the header, but not from actionable controls
  const header = cm.popup?.querySelector(`.${cm.cssPrefix}-header`);
  if (!header || !header.contains(e.target)) return;

  // Don't start drag if clicking on buttons
  if (e.target.closest('button')) return;

  cm.isDragging = true;
  cm.popup.classList.add('dragging');

  cm.dragStartX = e.clientX;
  cm.dragStartY = e.clientY;

  const rect = cm.popup.getBoundingClientRect();
  cm.popupStartX = rect.left;
  cm.popupStartY = rect.top;

  document.addEventListener('mousemove', cm.handleDragMove);
  document.addEventListener('mouseup', cm.handleDragEnd);

  log(Subsystems.TABLE, Status.DEBUG, '[ColumnManager] Drag started');
  e.preventDefault();
  e.stopPropagation();
}

/**
 * Handle drag move
 * @param {Object} cm - ColumnManager instance
 * @param {MouseEvent} e - Mouse event
 */
function handleDragMove(cm, e) {
  if (!cm.isDragging) return;

  const deltaX = e.clientX - cm.dragStartX;
  const deltaY = e.clientY - cm.dragStartY;

  let newX = cm.popupStartX + deltaX;
  let newY = cm.popupStartY + deltaY;

  const rect = cm.popup.getBoundingClientRect();
  const minVisible = 50;

  newX = Math.max(-rect.width + minVisible, Math.min(window.innerWidth - minVisible, newX));
  newY = Math.max(0, Math.min(window.innerHeight - minVisible, newY));

  cm.popup.style.left = `${newX}px`;
  cm.popup.style.top = `${newY}px`;
}

/**
 * Handle drag end
 * @param {Object} cm - ColumnManager instance
 */
function handleDragEnd(cm) {
  cm.isDragging = false;
  cm.popup.classList.remove('dragging');

  document.removeEventListener('mousemove', cm.handleDragMove);
  document.removeEventListener('mouseup', cm.handleDragEnd);

  // Save position
  saveSetting(cm, 'x', parseInt(cm.popup.style.left, 10));
  saveSetting(cm, 'y', parseInt(cm.popup.style.top, 10));
}

/**
 * Handle keyboard events
 * @param {Object} cm - ColumnManager instance
 * @param {KeyboardEvent} e - Keyboard event
 */
function handleKeyDown(cm, e) {
  if (e.key === 'Escape' && cm.popup?.classList.contains('visible')) {
    if (cm.columnTable?.isEditing) {
      // If editing a row, cancel the edit (stay in popup)
      e.preventDefault();
      e.stopPropagation();
      void cm.columnTable.handleCancel();
      void cm.columnTable.syncDirtyState?.();
      return;
    }
    // If not editing, close without applying changes
    e.preventDefault();
    e.stopPropagation();
    cm.close();
  }
}

/**
 * Handle click outside popup - do nothing, must use close/save/cancel buttons
 * @param {Object} cm - ColumnManager instance
 * @param {MouseEvent} e - Mouse event
 */
function handleClickOutside(cm, e) {
  // Click outside does nothing - user must explicitly close with buttons or Esc
  return;
}

/**
 * Column Manager - Data Module
 *
 * Column data loading and mapping from parent table.
 *
 * @module tables/column-manager/cm-data
 */


/**
 * Load column data from the parent table
 * @param {Object} cm - ColumnManager instance
 */
async function loadColumnData(cm) {
  if (!cm.parentTable?.table) {
    log(Subsystems.TABLE, Status.WARN, '[ColumnManager] Parent table not available');
    return;
  }

  try {
    const columns = cm.parentTable.table.getColumns();
    log(Subsystems.TABLE, Status.DEBUG, `[ColumnManager] Found ${columns.length} columns in parent table`);

    cm.originalColumns = columns.map((col) => {
      const def = col.getDefinition();
      const element = col.getElement();

      // Get actual width from the DOM element if available, fallback to definition
      let actualWidth = def.width || null;
      if (element && element.offsetWidth !== undefined) {
        try {
          const computedWidth = element.offsetWidth;
          // Only use computed width if it's reasonable (not collapsed/minimized)
          if (computedWidth && computedWidth > 20) {
            actualWidth = computedWidth;
          }
        } catch {
          log(Subsystems.TABLE, Status.DEBUG,
            `[ColumnManager] Could not get computed width for ${col.getField()}, using definition`);
        }
      }

      // Handle bottomCalc - ensure it's a string value, not a function
      let summaryValue = 'none';
      if (def.bottomCalc) {
        if (typeof def.bottomCalc === 'string') {
          summaryValue = def.bottomCalc;
        } else if (typeof def.bottomCalc === 'function') {
          summaryValue = def.bottomCalc.name || 'none';
        }
      }

      // Get Lithium metadata from parent table's _columnMeta Map
      const field = col.getField();
      const lithiumMeta = cm.parentTable._columnMeta?.get(field) || {};

      return {
        field: field,
        title: def.title || field,
        visible: col.isVisible(),
        coltype: lithiumMeta.coltype || 'string',
        bottomCalc: summaryValue,
        hozAlign: def.hozAlign || 'left',
        editable: lithiumMeta.editable ?? (def.editable === true),
        editor: def.editor || null,
        sorter: def.sorter || null,
        formatter: def.formatter || null,
        width: actualWidth,
        minWidth: def.minWidth || null,
        maxWidth: def.maxWidth || null,
        frozen: def.frozen || false,
        resizable: def.resizable !== false,
        sortable: def.headerSort !== false,
        filterable: !!def.headerFilter,
        category: def.category || 'Default',
      };
    });

    // Convert to column manager data format
    cm.columnData = cm.originalColumns.map((col, index) => ({
      id: index + 1,
      order: index + 1,
      drag_handle: '', // Formatter renders the icon; data value unused
      visible: col.visible,
      editable: col.editable !== false,
      field_name: col.field,
      column_name: col.title,
      format: col.coltype,
      summary: col.bottomCalc,
      alignment: col.hozAlign || 'left',
      width: col.width,
      category: col.category || 'Default',
      field: col.field,
    }));

    log(Subsystems.TABLE, Status.DEBUG, `[ColumnManager] Loaded ${cm.columnData.length} column records`);
  } catch (err) {
    log(Subsystems.TABLE, Status.ERROR, `[ColumnManager] Error loading column data: ${err.message}`);
    cm.columnData = [];
  }
}

/**
 * Refresh column data from parent table (preserving column order)
 * @param {Object} cm - ColumnManager instance
 */
async function refreshColumnData(cm) {
  if (!cm.parentTable?.table) return;

  // Get current column order from parent table
  const columns = cm.parentTable.table.getColumns();
  const currentOrder = columns
    .filter((col) => col.getField() !== '_selector')
    .map((col) => col.getField());

  // Load fresh column data
  await loadColumnData(cm);

  // Reorder column data to match parent table's current order
  if (currentOrder.length > 0) {
    const reorderedData = [];
    currentOrder.forEach((field, index) => {
      const colData = cm.columnData.find((c) => c.field_name === field);
      if (colData) {
        reorderedData.push({
          ...colData,
          order: index + 1,
        });
      }
    });

    // Add any columns not in currentOrder at the end
    cm.columnData.forEach((colData) => {
      if (!currentOrder.includes(colData.field_name)) {
        reorderedData.push({
          ...colData,
          order: reorderedData.length + 1,
        });
      }
    });

    cm.columnData = reorderedData;
  }

  // Update the column manager table if it exists
  if (cm.columnTable?.table) {
    await cm.columnTable.table.replaceData(cm.columnData);
  }
}

/**
 * Capture parent table state as the new baseline
 * @param {Object} cm - ColumnManager instance
 */
function captureParentStateAsOriginal(cm) {
  if (!cm.parentTable?.table) return;
  cm.originalColumns = cm.parentTable.table.getColumns().map((col) => {
    const def = col.getDefinition();
    const element = col.getElement();
    const field = col.getField();

    // Get Lithium metadata from parent table's _columnMeta Map
    const lithiumMeta = cm.parentTable._columnMeta?.get(field) || {};

    let actualWidth = def.width || null;
    if (element && element.offsetWidth !== undefined) {
      const computedWidth = element.offsetWidth;
      if (computedWidth && computedWidth > 20) {
        actualWidth = computedWidth;
      }
    }

    let summaryValue = 'none';
    if (def.bottomCalc) {
      if (typeof def.bottomCalc === 'string') {
        summaryValue = def.bottomCalc;
      } else if (typeof def.bottomCalc === 'function') {
        summaryValue = def.bottomCalc.name || 'none';
      }
    }

    return {
      field: field,
      title: def.title || field,
      visible: col.isVisible(),
      coltype: lithiumMeta.coltype || 'string',
      bottomCalc: summaryValue,
      hozAlign: def.hozAlign || 'left',
      width: actualWidth,
      category: def.category || 'Default',
      editable: lithiumMeta.editable ?? (def.editable === true),
    };
  });
}

/**
 * Column Manager - Actions Module
 *
 * Apply/discard changes and template building.
 *
 * @module tables/column-manager/cm-actions
 */


/**
 * Apply all pending changes to the parent table (called on save)
 * @param {Object} cm - ColumnManager instance
 */
async function applyAllChangesToParent(cm) {
  if (!cm.parentTable?.table) return;

  const templateColumns = buildPendingTemplateColumns(cm);
  if (Object.keys(templateColumns).length > 0) {
    log(Subsystems.TABLE, Status.INFO,
      `[ColumnManager] Applying column state to parent table (${Object.keys(templateColumns).length} columns)`);
    cm.parentTable.applyTemplateColumns(templateColumns);
  }

  captureParentStateAsOriginal(cm);
  cm.columnData = getCurrentColumnTableData(cm);
  cm._pendingReorder = null;

  clearDirty(cm);
}

/**
 * Discard all pending changes (called on cancel)
 * @param {Object} cm - ColumnManager instance
 */
async function discardAllChanges(cm) {
  if (!cm.columnTable?.table) {
    clearDirty(cm);
    return;
  }

  log(Subsystems.TABLE, Status.INFO, '[ColumnManager] Discarding pending changes');

  // Reload original column data to revert any edits
  await cm.columnTable.table.replaceData(cm.columnData);

  cm._pendingReorder = null;
  clearDirty(cm);
}

/**
 * Build a template-state patch from the current column manager rows.
 * @param {Object} cm - ColumnManager instance
 * @returns {Object} Template columns
 */
function buildPendingTemplateColumns(cm) {
  if (!cm.parentTable?.table) return {};

  const rowData = getCurrentColumnTableData(cm);
  const rowMap = new Map(rowData.map((row) => [row.field_name, row]));
  const parentColumns = cm.parentTable.table.getColumns();
  const templateColumns = {};

  // Get Lithium metadata from parent table
  const columnMeta = cm.parentTable._columnMeta || new Map();

  parentColumns.forEach((column) => {
    const field = column.getField();
    if (!field || field === '_selector') return;

    // Get original column definition from parent table's tableDef
    const originalColDef = cm.parentTable.tableDef?.columns?.[field] || {};

    // Get lithium metadata for this column
    const lithiumMeta = columnMeta.get(field) || {};

    // Use the canonical extractor to get the base column
    const baseColumn = extractTableDefColumn(column, originalColDef, lithiumMeta, {
      includeRuntimeOnly: true,
    });

    const managerRow = rowMap.get(field);
    if (!managerRow) {
      templateColumns[field] = baseColumn;
      return;
    }

    // Build patch column with flattened properties (no overrides wrapper)
    const patchColumn = {
      title: managerRow.column_name,
      field,
      coltype: managerRow.format,
      visible: managerRow.visible,
      editable: managerRow.editable,
    };

    // Flatten alignment and bottomCalc directly onto patchColumn
    if (managerRow.alignment && managerRow.alignment !== 'left') {
      patchColumn.hozAlign = managerRow.alignment;
    }
    if (managerRow.summary && managerRow.summary !== 'none') {
      patchColumn.bottomCalc = managerRow.summary;
    }

    // Add width if specified
    const parsedWidth = managerRow.width == null || managerRow.width === ''
      ? null
      : parseInt(managerRow.width, 10);
    if (Number.isFinite(parsedWidth) && parsedWidth > 0) {
      patchColumn.width = parsedWidth;
    }

    // Merge: start with base, apply patch (patch wins for overlapping properties)
    templateColumns[field] = { ...baseColumn, ...patchColumn };
  });

  // Order columns according to row data
  const orderedTemplateColumns = {};
  const orderedFields = rowData.map((row) => row.field_name);
  orderedFields.forEach((field) => {
    if (templateColumns[field]) {
      orderedTemplateColumns[field] = templateColumns[field];
    }
  });
  Object.entries(templateColumns).forEach(([field, config]) => {
    if (!orderedTemplateColumns[field]) {
      orderedTemplateColumns[field] = config;
    }
  });

  return orderedTemplateColumns;
}

/**
 * Handle cell edit in the column manager
 * @param {Object} cm - ColumnManager instance
 * @param {Object} cell - Tabulator cell
 */
async function handleCellEdit(cm, cell) {
  const rowData = cell.getRow().getData();
  const field = cell.getField();
  const value = cell.getValue();

  log(Subsystems.TABLE, Status.DEBUG,
    `[ColumnManager] Cell edited: ${field} = ${value} for column ${rowData.field_name}`);

  // In Auto mode, editing the 'order' column triggers a reorder
  if (field === 'order' && cm.orderingMode === 'auto') {
    handleOrderEdit(cm, cell.getRow(), value);
    return;
  }

  cm.syncDirtyState();

  // Notify callback
  cm.onColumnChange(rowData.field, field, value);
}

/**
 * Handle order column edit in Auto mode.
 * @param {Object} cm - ColumnManager instance
 * @param {Object} editedRow - The Tabulator row that was edited
 * @param {number} newOrder - The new order value
 */
function handleOrderEdit(cm, editedRow, newOrder) {
  if (!cm.columnTable?.table) return;

  const rows = cm.columnTable.table.getRows();
  const editedField = editedRow.getData().field_name;

  // Collect all rows with their current order, and apply the edit
  const rowOrders = rows.map((row) => {
    const data = row.getData();
    return {
      row,
      field_name: data.field_name,
      order: data.field_name === editedField ? parseInt(newOrder, 10) : data.order,
    };
  });

  // Sort by the new order values (stable sort, preserving original order for ties)
  rowOrders.sort((a, b) => a.order - b.order);

  // Renumber sequentially and update each row
  rowOrders.forEach((item, index) => {
    const newSeq = index + 1;
    if (item.row.getData().order !== newSeq) {
      item.row.update({ order: newSeq });
    }
  });

  // Track the reorder
  cm._pendingReorder = rowOrders.map((item) => item.field_name);
  cm.syncDirtyState();

  log(Subsystems.TABLE, Status.DEBUG,
    `[ColumnManager] Order edited, new sequence: ${cm._pendingReorder.join(', ')}`);
}

/**
 * Handle row reorder in the column manager
 * @param {Object} cm - ColumnManager instance
 */
async function handleRowReorder(cm) {
  if (!cm.columnTable?.table) return;

  // Get the current row order from the column manager table
  const rows = cm.columnTable.table.getRows();
  const newFieldOrder = rows.map((row) => row.getData().field_name);

  // Update order numbers in the column manager
  rows.forEach((row, index) => {
    const data = row.getData();
    row.update({ ...data, order: index + 1 });
  });

  // Track the reorder change
  cm._pendingReorder = newFieldOrder;
  cm.syncDirtyState();

  log(Subsystems.TABLE, Status.DEBUG, `[ColumnManager] Row reordered, new order: ${newFieldOrder.join(', ')}`);
}

/**
 * Column Manager - UI Module
 *
 * Popup DOM creation and positioning.
 *
 * @module tables/column-manager/cm-ui
 */


/**
 * Check if this is a depth-2 column manager
 * @param {Object} cm - ColumnManager instance
 * @returns {boolean} True if depth-2
 */
function isDepth2ColumnManager(cm) {
  if (cm.parentTable?.container) {
    return cm.parentTable.container.closest('.col-manager-popup') !== null;
  }
  return false;
}

/**
 * Create the popup container
 * @param {Object} cm - ColumnManager instance
 */
function createPopup(cm) {
  // Create popup element
  cm.popup = document.createElement('div');
  cm.popup.className = `${cm.cssPrefix}-popup`;

  // Add depth-2 class if this is a column manager of a column manager
  if (isDepth2ColumnManager(cm)) {
    cm.popup.classList.add('depth-2');
  }

  cm.popup.setAttribute('data-manager-id', cm.managerId);

  // Restore saved dimensions or use defaults
  const savedWidth = restoreSetting(cm, 'width', 500);
  const savedHeight = restoreSetting(cm, 'height', 400);

  cm.popup.style.width = `${savedWidth}px`;
  cm.popup.style.height = `${savedHeight}px`;

  // Build popup structure
  const isDepth2 = isDepth2ColumnManager(cm);
  const title = isDepth2 ? 'Column Manager Manager' : 'Column Manager';

  // Restore saved position (only if exists)
  const savedX = restoreSetting(cm, 'x', null);
  const savedY = restoreSetting(cm, 'y', null);
  if (savedX !== null && savedY !== null) {
    cm.popup.style.left = `${savedX}px`;
    cm.popup.style.top = `${savedY}px`;
  } else {
    // Only position relative to anchor if no saved position
    positionPopup(cm);
  }

  cm.popup.innerHTML = `
    <div class="${cm.cssPrefix}-header">
      <span class="${cm.cssPrefix}-title">${title}</span>
      <div class="${cm.cssPrefix}-mode-toggle">
        <button type="button" class="${cm.cssPrefix}-mode-btn${cm.orderingMode === 'manual' ? ' active' : ''}" data-mode="manual" title="Manual ordering (drag rows)">
          <fa fa-hand></fa> Manual
        </button>
        <button type="button" class="${cm.cssPrefix}-mode-btn${cm.orderingMode === 'auto' ? ' active' : ''}" data-mode="auto" title="Auto ordering (sort columns, edit order)">
          <fa fa-arrow-down-a-z></fa> Auto
        </button>
      </div>
      <div class="${cm.cssPrefix}-header-actions">
        <button type="button" class="${cm.cssPrefix}-save-btn" title="Save" disabled data-tooltip='Save row edit<br><span class="li-tip-kbd">Ctrl+Enter</span>' data-shortcut-id="editsave" data-tooltip-original='Save row edit<br><span class="li-tip-kbd">Ctrl+Enter</span>'>
          <fa fa-square-full></fa>
        </button>
        <button type="button" class="${cm.cssPrefix}-cancel-btn" title="Cancel" disabled data-tooltip='Cancel row edit<br><span class="li-tip-kbd">Esc</span>' data-shortcut-id="editcancel" data-tooltip-original='Cancel row edit<br><span class="li-tip-kbd">Esc</span>'>
          <fa fa-octagon></fa>
        </button>
        <button type="button" class="${cm.cssPrefix}-close-btn" title="Close" data-tooltip='Close and apply changes to table'>
          <fa fa-xmark></fa>
        </button>
      </div>
    </div>
    <div class="${cm.cssPrefix}-content">
      <div class="lithium-table-container" id="${cm.cssPrefix}-table-container-${cm.managerId}"></div>
      <div class="lithium-navigator-container" id="${cm.cssPrefix}-navigator-${cm.managerId}"></div>
    </div>
    <div class="${cm.cssPrefix}-resize-handle ${cm.cssPrefix}-resize-handle-tl" title="Resize"></div>
    <div class="${cm.cssPrefix}-resize-handle ${cm.cssPrefix}-resize-handle-tr" title="Resize"></div>
    <div class="${cm.cssPrefix}-resize-handle ${cm.cssPrefix}-resize-handle-bl" title="Resize"></div>
    <div class="${cm.cssPrefix}-resize-handle ${cm.cssPrefix}-resize-handle-br" title="Resize"></div>
  `;

  // Append to body (separate from table container)
  document.body.appendChild(cm.popup);

  // Store reference to this instance on the popup for cleanup
  cm.popup._columnManagerInstance = cm;

  processIcons(cm.popup);
}

/**
 * Position popup relative to anchor element
 * @param {Object} cm - ColumnManager instance
 */
function positionPopup(cm) {
  if (!cm.anchorElement || !cm.popup) return;

  const anchorRect = cm.anchorElement.getBoundingClientRect();

  // Position: top-left corner of popup anchored to bottom-left of icon
  cm.popup.style.left = `${anchorRect.left}px`;
  cm.popup.style.top = `${anchorRect.bottom + 4}px`;

  // Ensure popup doesn't go off-screen horizontally
  const maxLeft = window.innerWidth - parseInt(cm.popup.style.width, 10) - 20;
  if (anchorRect.left > maxLeft) {
    cm.popup.style.left = `${maxLeft}px`;
  }

  // Ensure popup doesn't go off-screen vertically
  const popupHeight = parseInt(cm.popup.style.height, 10);
  const maxTop = window.innerHeight - popupHeight - 20;
  if (anchorRect.bottom + 4 > maxTop) {
    // Position above the icon instead
    cm.popup.style.top = `${anchorRect.top - popupHeight - 4}px`;
  }
}

/**
 * Show the column manager
 * @param {Object} cm - ColumnManager instance
 */
function show(cm) {
  if (cm.popup) {
    // Only reposition if no saved position exists
    const savedX = restoreSetting(cm, 'x', null);
    const savedY = restoreSetting(cm, 'y', null);
    if (savedX === null || savedY === null) {
      positionPopup(cm);
    }
    
    cm.popup.classList.add('visible');

    if (cm.columnTable?.table) {
      setTimeout(() => {
        cm.columnTable.table.redraw(true);
      }, 100);
    }
  }
}

/**
 * Hide the column manager
 * @param {Object} cm - ColumnManager instance
 */
function hide(cm) {
  if (cm.popup) {
    cm.popup.classList.remove('visible');
  }
}

/**
 * Close the column manager
 * @param {Object} cm - ColumnManager instance
 */
function close(cm) {
  hide(cm);

  // Remove event listeners
  document.removeEventListener('keydown', cm.handleKeyDown);
  document.removeEventListener('click', cm.handleClickOutside);

  cm.onClose?.();
}

/**
 * Handle table width changes from the navigator
 * @param {Object} cm - ColumnManager instance
 * @param {string} mode - Width mode
 */
function setTableWidth$1(cm, mode) {
  if (!cm.popup) return;

  // Get width from CSS variables
  const widthVar = `--table-popup-width-${mode}`;
  const computedStyle = getComputedStyle(document.documentElement);
  const width = computedStyle.getPropertyValue(widthVar).trim();

  if (mode === 'auto' || !width) {
    cm.popup.style.width = '';
    cm.popup.style.maxWidth = '95vw';
  } else {
    cm.popup.style.width = width;
    cm.popup.style.maxWidth = '95vw';
  }

  // Save the width setting
  saveSetting(cm, 'width', parseInt(cm.popup.style.width, 10) || 500);

  // Redraw the table to fit new dimensions
  if (cm.columnTable?.table) {
    cm.columnTable.table.redraw();
  }
}

/**
 * Column Manager - Column Definition Builder Module
 *
 * Builds column definitions and lookup maps for the column manager table.
 *
 * @module tables/column-manager/cm-columns
 */

/**
 * Build the column definition for the column manager table.
 * @param {Object} cm - ColumnManager instance
 * @returns {Object} Column definition
 */
function buildColumnManagerDefinition(cm) {
  // Build lookup maps for the lookupFixed coltype columns
  const formatLookup = buildFormatLookup();
  const summaryLookup = buildSummaryLookup();
  const alignmentLookup = buildAlignmentLookup();

  const isManual = cm.orderingMode === 'manual';

  const columns = {};

  // --- Frozen left columns ---

  // Drag handle column (Manual mode only)
  if (isManual) {
    columns.drag_handle = {
      field: 'drag_handle',
      title: '',
      coltype: 'string',
      visible: true,
      sort: false,
      filter: false,
      editable: false,
      frozen: true,
      resizable: false,
      width: 16,
      minWidth: 16,
      maxWidth: 16,
      hozAlign: 'center',
      rowHandle: true,
      formatter: function() {
        return '<i class="fa-duotone fa-thin fa-grip-dots-vertical"></i>';
      },
    };
  }

  // Visible column
  columns.visible = {
    field: 'visible',
    title: 'Vis',
    coltype: 'boolean',
    visible: true,
    sort: false,
    filter: false,
    editable: true,
    width: 46,
    hozAlign: 'center',
    headerSort: false,
    formatter: 'tickCross',
    formatterParams: {
      allowEmpty: false,
      allowTruthy: true,
    },
    editor: 'tickCross',
  };

  // Order column (both modes, editable only in Auto mode)
  columns.order = {
    field: 'order',
    title: '#',
    coltype: 'integer',
    visible: true,
    sort: false,
    filter: false,
    editable: !isManual,
    frozen: true,
    width: 42,
    hozAlign: 'center',
    headerSort: false,
    ...(isManual ? {} : {
      editor: 'number',
      editorParams: {
        min: 1,
        max: 999,
        step: 1,
      },
    }),
  };

  // --- Data columns ---

  columns.field_name = {
    field: 'field_name',
    title: 'Field Name',
    coltype: 'string',
    visible: true,
    sort: !isManual,
    filter: true,
    editable: false,
    minWidth: 120,
    hozAlign: 'left',
    headerSort: !isManual,
  };

  columns.column_name = {
    field: 'column_name',
    title: 'Column Name',
    coltype: 'string',
    visible: true,
    sort: !isManual,
    filter: true,
    editable: true,
    minWidth: 150,
    hozAlign: 'left',
    headerSort: !isManual,
  };

  columns.editable = {
    field: 'editable',
    title: 'Edit',
    coltype: 'boolean',
    visible: true,
    sort: !isManual,
    filter: true,
    editable: true,
    width: 50,
    hozAlign: 'center',
    headerSort: false,
    formatter: 'tickCross',
    formatterParams: {
      allowEmpty: false,
      allowTruthy: true,
    },
    editor: 'tickCross',
  };

  columns.format = {
    field: 'format',
    title: 'Format',
    coltype: 'lookupFixed',
    visible: true,
    sort: !isManual,
    filter: true,
    editable: true,
    width: 120,
    hozAlign: 'left',
    headerSort: !isManual,
    editor: 'select',
    editorParams: {
      values: formatLookup,
    },
    formatter: 'lookup',
    formatterParams: {
      lookup: formatLookup,
    },
  };

  columns.summary = {
    field: 'summary',
    title: 'Summary',
    coltype: 'lookupFixed',
    visible: true,
    sort: !isManual,
    filter: true,
    editable: true,
    width: 100,
    hozAlign: 'left',
    headerSort: !isManual,
    editor: 'select',
    editorParams: {
      values: summaryLookup,
    },
    formatter: 'lookup',
    formatterParams: {
      lookup: summaryLookup,
    },
  };

  columns.alignment = {
    field: 'alignment',
    title: 'Alignment',
    coltype: 'lookupFixed',
    visible: true,
    sort: !isManual,
    filter: true,
    editable: true,
    width: 90,
    hozAlign: 'left',
    headerSort: !isManual,
    editor: 'select',
    editorParams: {
      values: alignmentLookup,
    },
    formatter: 'lookup',
    formatterParams: {
      lookup: alignmentLookup,
    },
  };

  columns.category = {
    field: 'category',
    title: 'Category',
    coltype: 'stringList',
    visible: true,
    sort: !isManual,
    filter: true,
    editable: true,
    minWidth: 100,
    hozAlign: 'left',
    headerSort: !isManual,
  };

  columns.width = {
    field: 'width',
    title: 'Width',
    coltype: 'integer',
    visible: true,
    sort: !isManual,
    filter: false,
    editable: true,
    width: 80,
    hozAlign: 'right',
    headerSort: false,
    editor: 'number',
    editorParams: {
      min: 20,
      max: 1000,
    },
  };

  return {
    title: 'Column Manager',
    readonly: false,
    layout: 'fitColumns',
    rowHeight: 32,
    movableRows: isManual,
    groupBy: 'category',
    groupStartOpen: true,
    groupToggleElement: 'header',
    columns,
  };
}

/**
 * Build lookup map for format column
 * @returns {Object} Format lookup
 */
function buildFormatLookup() {
  return {
    string: 'Text',
    integer: 'Integer',
    decimal: 'Decimal',
    index: 'Index',
    currency: 'Currency',
    percent: 'Percent',
    boolean: 'Boolean',
    booleanCheckbox: 'Checkbox',
    booleanIcon: 'Icon Bool',
    booleanSwitch: 'Switch',
    date: 'Date',
    datetime: 'DateTime',
    datetimeUTC: 'UTC DateTime',
    time: 'Time',
    duration: 'Duration',
    lookup: 'Lookup',
    lookupFixed: 'Fixed Lookup',
    stringList: 'String List',
    tags: 'Tags',
    color: 'Color',
    colorPicker: 'Color Picker',
    email: 'Email',
    url: 'URL',
    phone: 'Phone',
    ipAddress: 'IP Address',
    uuid: 'UUID',
    json: 'JSON',
    html: 'HTML',
    text: 'Text Area',
    password: 'Password',
    rating: 'Rating',
    star: 'Stars',
    progress: 'Progress',
    fileSize: 'File Size',
    image: 'Image',
    imageAvatar: 'Avatar',
    emoji: 'Emoji',
    slug: 'Slug',
    version: 'Version',
    rownum: 'Row Num',
  };
}

/**
 * Build lookup map for summary column
 * @returns {Object} Summary lookup
 */
function buildSummaryLookup() {
  return {
    none: 'None',
    count: 'Count',
    sum: 'Sum',
    avg: 'Average',
    min: 'Minimum',
    max: 'Maximum',
    unique: 'Unique',
  };
}

/**
 * Build lookup map for alignment column
 * @returns {Object} Alignment lookup
 */
function buildAlignmentLookup() {
  return {
    left: 'Left',
    center: 'Center',
    right: 'Right',
  };
}

/**
 * Column Manager - Table Initialization Module
 *
 * Inner LithiumTable setup and configuration.
 *
 * @module tables/column-manager/cm-table
 */


/**
 * Initialize the column manager as a LithiumTable
 * @param {Object} cm - ColumnManager instance
 */
async function initColumnTable(cm) {
  const tableContainer = cm.popup.querySelector(`#${cm.cssPrefix}-table-container-${cm.managerId}`);
  const navigatorContainer = cm.popup.querySelector(`#${cm.cssPrefix}-navigator-${cm.managerId}`);

  if (!tableContainer || !navigatorContainer) {
    log(Subsystems.TABLE, Status.ERROR, '[ColumnManager] Container elements not found');
    return;
  }

  log(Subsystems.TABLE, Status.DEBUG, `[ColumnManager] Initializing table with ${cm.columnData.length} rows`);

  // Create column definition for the column manager table
  const columnDef = buildColumnManagerDefinition(cm);

  // Check if this is a depth-2 column manager
  isDepth2ColumnManager(cm);

  const userEditModeChange = () => {
    requestAnimationFrame(() => cm.updateButtonState());
  };
  const userDirtyChange = () => {
    requestAnimationFrame(() => cm.updateButtonState());
  };

  cm.columnTable = new LithiumTable({
    container: tableContainer,
    navigatorContainer: navigatorContainer,
    app: cm.app,
    tablePath: getManagerTablePath(cm),
    tableDef: columnDef,
    coltypes: await getColtypesForManager(),
    cssPrefix: 'lithium',
    storageKey: getManagerTableStorageKey(cm),
    readonly: false,
    alwaysEditable: false,
    onRowSelected: (rowData) => onRowSelected(cm, rowData),
    onRowDeselected: () => onRowDeselected(cm),
    onExecuteSave: () => cm.handleColumnTableExecuteSave(),
    onSetTableWidth: (mode) => cm.setTableWidth(mode),
  });

  await cm.columnTable.init();

  const previousEditModeChange = cm.columnTable.onEditModeChange;
  cm.columnTable.onEditModeChange = (...args) => {
    previousEditModeChange?.(...args);
    userEditModeChange(...args);
  };

  const previousDirtyChange = cm.columnTable.onDirtyChange;
  cm.columnTable.onDirtyChange = (...args) => {
    previousDirtyChange?.(...args);
    userDirtyChange(...args);
  };

  // Set the data using replaceData
  if (cm.columnData.length > 0 && cm.columnTable.table) {
    await cm.columnTable.table.replaceData(cm.columnData);
    log(Subsystems.TABLE, Status.DEBUG, '[ColumnManager] Data set successfully');
  } else {
    log(Subsystems.TABLE, Status.WARN, '[ColumnManager] No data to set');
  }

  // Wire row reorder events
  wireRowReorderEvents(cm);
  configureNavigatorButtons(cm);

  // In manual mode, ensure no sorts are active
  if (cm.orderingMode === 'manual' && cm.columnTable?.table) {
    cm.columnTable.table.clearSort();
  }

  // Restore selected row
  restoreSelectedRow(cm);
  cm.syncDirtyState();
}

/**
 * Get storage key for manager table
 * @param {Object} cm - ColumnManager instance
 * @returns {string} Storage key
 */
function getManagerTableStorageKey(cm) {
  return isDepth2ColumnManager(cm)
    ? 'lithium_column_manager_manager'
    : 'lithium_column_manager';
}

/**
 * Get table path for manager table
 * @param {Object} cm - ColumnManager instance
 * @returns {string} Table path
 */
function getManagerTablePath(cm) {
  return isDepth2ColumnManager(cm)
    ? 'column-manager/column-manager-manager'
    : 'column-manager/column-manager';
}

/**
 * Get coltypes for the column manager table
 * @param {Object} cm - ColumnManager instance
 * @returns {Object} Coltypes
 */
async function getColtypesForManager(cm) {
  try {
    return await loadColtypes();
  } catch (err) {
    log(Subsystems.TABLE, Status.WARN, `[ColumnManager] Failed to load coltypes: ${err.message}`);
    return getDefaultColtypes();
  }
}

/**
 * Get default coltypes when loading fails
 * @returns {Object} Default coltypes
 */
function getDefaultColtypes() {
  return {
    string: { description: 'Text data', align: 'left', editor: 'input', sorter: 'string' },
    integer: { description: 'Whole numbers', align: 'right', editor: 'number', sorter: 'number' },
    boolean: {
      description: 'True/false values',
      align: 'center',
      formatter: 'tickCross',
      formatterParams: { allowEmpty: false, allowTruthy: true },
      editor: 'tickCross',
      sorter: 'boolean',
    },
    lookupFixed: {
      description: 'Fixed dropdown list',
      align: 'left',
      editor: 'select',
      sorter: 'string',
      formatter: 'lookup',
    },
    stringList: {
      description: 'Editable text lookup from column values',
      align: 'left',
      editor: 'list',
      editorParams: {
        autocomplete: true,
        freetext: true,
        listOnEmpty: true,
        valuesLookup: 'column',
      },
      sorter: 'string',
      formatter: 'plaintext',
    },
  };
}

/**
 * Configure navigator buttons for column manager
 * @param {Object} cm - ColumnManager instance
 */
function configureNavigatorButtons(cm) {
  if (!cm.columnTable?.navigatorContainer) return;

  const nav = cm.columnTable.navigatorContainer;
  const disableButton = (action, title) => {
    const btn = nav.querySelector(`#${cm.columnTable.cssPrefix}-nav-${action}`);
    if (!btn) return;
    btn.disabled = true;
    if (title) btn.title = title;
  };

  disableButton('add', 'Adding columns is not supported here');
  disableButton('duplicate', 'Duplicating columns is not supported here');
  disableButton('flag', 'Flag is not supported here');
  disableButton('annotate', 'Annotations are not supported here');
  disableButton('delete', 'Deleting columns is not supported here');

  cm.columnTable.updateDuplicateButtonState = () => {
    disableButton('duplicate', 'Duplicating columns is not supported here');
  };
  cm.columnTable.handleAdd = () => {};
  cm.columnTable.handleDuplicate = () => {};
  cm.columnTable.handleDelete = () => {};

  ['menu', 'width', 'layout', 'template'].forEach((action) => {
    const btn = nav.querySelector(`#${cm.columnTable.cssPrefix}-nav-${action}`);
    btn?.addEventListener('mousedown', (event) => {
      event?.stopPropagation?.();
    }, true);
  });
}

/**
 * Wire row reorder events
 * @param {Object} cm - ColumnManager instance
 */
function wireRowReorderEvents(cm) {
  if (!cm.columnTable?.table) return;

  if (cm.orderingMode === 'manual') {
    const tableEl = cm.columnTable.container;
    if (tableEl) {
      tableEl.addEventListener('mousedown', (e) => {
        const cell = e.target.closest('.tabulator-cell[data-tabulator-field="drag_handle"]');
        if (!cell) return;

        if (!cm.columnTable?.isEditing) {
          e.preventDefault();
          e.stopPropagation();
          return;
        }

        if (cm.columnTable?.table) {
          cm.columnTable.table.clearSort();
        }
      }, true);
    }

    cm.columnTable.table.on('rowMoved', () => {
      handleRowReorder(cm);
    });
  }

  cm.columnTable.table.on('cellEdited', (cell) => {
    handleCellEdit(cm, cell);
  });

  log(Subsystems.TABLE, Status.DEBUG, `[ColumnManager] Events wired (${cm.orderingMode} mode)`);
}

/**
 * Handle row selection
 * @param {Object} cm - ColumnManager instance
 * @param {Object} rowData - Row data
 */
function onRowSelected(cm, rowData) {
  restoreSetting(cm, 'selectedRow', rowData.field_name);
  log(Subsystems.TABLE, Status.DEBUG, `[ColumnManager] Row selected: ${rowData.field_name}`);
}

/**
 * Handle row deselection
 * @param {Object} cm - ColumnManager instance
 */
function onRowDeselected(cm) {
  restoreSetting(cm, 'selectedRow', null);
}

/**
 * Restore selected row from storage
 * @param {Object} cm - ColumnManager instance
 */
function restoreSelectedRow(cm) {
  const savedField = restoreSetting(cm, 'selectedRow', null);
  if (savedField && cm.columnTable?.table) {
    const rows = cm.columnTable.table.getRows();
    const targetRow = rows.find((row) => row.getData().field_name === savedField);
    if (targetRow) {
      targetRow.select();
    }
  }
}

/**
 * Switch between Manual and Auto ordering modes
 * @param {Object} cm - ColumnManager instance
 * @param {'manual'|'auto'} mode - The ordering mode to switch to
 */
async function setOrderingMode(cm, mode) {
  if (mode === cm.orderingMode) return;

  cm.orderingMode = mode;

  // Update mode toggle button states
  const modeBtns = cm.popup?.querySelectorAll(`.${cm.cssPrefix}-mode-btn`);
  modeBtns?.forEach((btn) => {
    btn.classList.toggle('active', btn.dataset.mode === mode);
  });

  // Save current data
  let currentData = cm.columnData;
  if (cm.columnTable?.table) {
    const rows = cm.columnTable.table.getRows();
    if (rows.length > 0) {
      currentData = rows.map((row) => row.getData());
    }
  }

  // Destroy existing table
  if (cm.columnTable) {
    cm.columnTable.destroy();
    cm.columnTable = null;
  }

  cm.columnData = currentData;

  const tableContainer = cm.popup.querySelector(`#${cm.cssPrefix}-table-container-${cm.managerId}`);
  const navigatorContainer = cm.popup.querySelector(`#${cm.cssPrefix}-navigator-${cm.managerId}`);

  if (!tableContainer || !navigatorContainer) return;

  tableContainer.innerHTML = '';
  navigatorContainer.innerHTML = '';

  await initColumnTable(cm);

  // Persist mode preference
  restoreSetting(cm, 'orderingMode', mode);

  log(Subsystems.TABLE, Status.INFO, `[ColumnManager] Switched to ${mode} ordering mode`);
}

/**
 * LithiumColumnManager
 *
 * A column management interface implemented as a LithiumTable.
 * Provides drag-to-reorder, visibility toggles, and column property editing
 * with real-time updates to the parent table.
 *
 * Features:
 * - Anchored to the column chooser icon position
 * - Dark blue themed background for visual distinction
 * - Draggable resize handle (bottom-right corner)
 * - Persistence of selected row, width, height
 * - Independent instance per parent table
 *
 * @module tables/column-manager
 */


class LithiumColumnManager {
  /**
   * Create a ColumnManager instance
   * @param {Object} options - Configuration options
   */
  constructor(options) {
    this.parentContainer = options.parentContainer;
    this.anchorElement = options.anchorElement;
    this.parentTable = options.parentTable;
    this.app = options.app;
    this.managerId = options.managerId || 'default';
    this.cssPrefix = options.cssPrefix || 'col-manager';

    // State
    this.columnTable = null;
    this.columnData = [];
    this.originalColumns = [];
    this.isInitialized = false;
    this.popup = null;

    // Column ordering mode
    this.orderingMode = 'manual';

    // Resize state
    this.isResizing = false;
    this.resizeStartX = 0;
    this.resizeStartY = 0;
    this.popupStartWidth = 0;
    this.popupStartHeight = 0;
    this.popupStartLeft = 0;
    this.popupStartTop = 0;
    this.resizeCorner = 'br';

    // Drag state
    this.isDragging = false;
    this.dragStartX = 0;
    this.dragStartY = 0;
    this.popupStartX = 0;
    this.popupStartY = 0;

    // Storage key
    this.storageKey = `lithium_col_manager_${this.managerId}`;

    // Event handlers
    this.onColumnChange = options.onColumnChange || (() => {});
    this.onClose = options.onClose || (() => {});

    // Button references
    this.saveBtn = null;
    this.cancelBtn = null;

    // Dirty state
    this.isDirty = false;

    // Bind methods for event handlers
    this.handleResizeMove = (e) => handleResizeMove(this, e);
    this.handleResizeEnd = () => handleResizeEnd(this);
    this.handleDragMove = (e) => handleDragMove(this, e);
    this.handleDragEnd = () => handleDragEnd(this);
    this.handleKeyDown = (e) => handleKeyDown(this, e);
    this.handleClickOutside = (e) => handleClickOutside();
  }

  /**
   * Initialize the column manager (lazy - called when first opened)
   */
  async init() {
    if (this.isInitialized) {
      await refreshColumnData(this);
      return;
    }

    if (!this.parentContainer || !this.parentTable) {
      log(Subsystems.TABLE, Status.ERROR, '[ColumnManager] Missing required options');
      return;
    }

    if (!this.parentTable.table) {
      log(Subsystems.TABLE, Status.WARN, '[ColumnManager] Waiting for parent table to initialize...');
      await new Promise((resolve) => setTimeout(resolve, 100));
    }

    // Restore ordering mode preference
    const savedMode = restoreSetting(this, 'orderingMode', 'manual');
    if (savedMode === 'auto' || savedMode === 'manual') {
      this.orderingMode = savedMode;
    }

    // Create popup
    createPopup(this);

    await new Promise((resolve) => {
      requestAnimationFrame(() => {
        this.popup.classList.add('visible');
        setTimeout(resolve, 50);
      });
    });

    await loadColumnData(this);
    await initColumnTable(this);

    // Wire event handlers for buttons and drag/resize
    this.wireEventHandlers();

    this.isInitialized = true;
    log(Subsystems.TABLE, Status.INFO, `[ColumnManager] Initialized for ${this.managerId}`);
  }

  /**
   * Refresh column data from parent table
   */
  async refreshColumnData() {
    await refreshColumnData(this);
  }

  /**
   * Handle parent table column order change
   */
  onParentColumnOrderChanged() {
    if (!this.isInitialized || !this.columnTable?.table) return;
    this.refreshColumnData();
    log(Subsystems.TABLE, Status.DEBUG, '[ColumnManager] Parent column order changed, refreshed');
  }

  /**
   * Check if this is a depth-2 column manager
   */
  isDepth2ColumnManager() {
    return isDepth2ColumnManager(this);
  }

  /**
   * Wire event handlers
   */
  wireEventHandlers() {
    // Close button - always applies changes and closes
    const closeBtn = this.popup.querySelector(`.${this.cssPrefix}-close-btn`);
    closeBtn?.addEventListener('click', () => {
      void this.handleClose();
    });

    // Save button - saves current row edit only
    this.saveBtn = this.popup.querySelector(`.${this.cssPrefix}-save-btn`);
    this.saveBtn?.addEventListener('click', async () => {
      if (this.columnTable?.isEditing) {
        await this.columnTable.handleSave();
        this.syncDirtyState();
      }
    });

    // Cancel button - cancels row edit only
    this.cancelBtn = this.popup.querySelector(`.${this.cssPrefix}-cancel-btn`);
    this.cancelBtn?.addEventListener('click', async () => {
      if (this.columnTable?.isEditing) {
        await this.columnTable.handleCancel();
        this.syncDirtyState();
      }
    });

    // Update button state (enabled when row is being edited)
    updateButtonState(this);

    // Mode toggle buttons
    const modeBtns = this.popup.querySelectorAll(`.${this.cssPrefix}-mode-btn`);
    modeBtns.forEach((btn) => {
      btn.addEventListener('click', () => {
        const mode = btn.dataset.mode;
        if (mode !== this.orderingMode) {
          setOrderingMode(this, mode);
        }
      });
    });

    // Resize handles (four corners)
    const resizeHandleBR = this.popup.querySelector(`.${this.cssPrefix}-resize-handle-br`);
    const resizeHandleBL = this.popup.querySelector(`.${this.cssPrefix}-resize-handle-bl`);
    const resizeHandleTR = this.popup.querySelector(`.${this.cssPrefix}-resize-handle-tr`);
    const resizeHandleTL = this.popup.querySelector(`.${this.cssPrefix}-resize-handle-tl`);
    resizeHandleBR?.addEventListener('mousedown', (e) => handleResizeStart(this, e));
    resizeHandleBL?.addEventListener('mousedown', (e) => handleResizeStart(this, e));
    resizeHandleTR?.addEventListener('mousedown', (e) => handleResizeStart(this, e));
    resizeHandleTL?.addEventListener('mousedown', (e) => handleResizeStart(this, e));

    // Header for dragging
    const header = this.popup.querySelector(`.${this.cssPrefix}-header`);
    header?.addEventListener('mousedown', (e) => this.handleDragStart(e));

    // Keyboard handler
    document.addEventListener('keydown', this.handleKeyDown);

    // Click outside
    setTimeout(() => {
      document.addEventListener('click', this.handleClickOutside);
    }, 0);
  }

  // Delegate state methods
  updateButtonState() { updateButtonState(this); }
  syncDirtyState() { syncDirtyState(this); }
  clearDirty() { clearDirty(this); }
  saveSetting(key, value) { saveSetting(this, key, value); }
  restoreSetting(key, defaultValue) { return restoreSetting(this, key, defaultValue); }
  handlePrimarySave() { return handlePrimarySave(this); }
  handlePrimaryCancel() { return handlePrimaryCancel(this); }

  // Delegate drag/resize methods
  handleDragStart(e) { handleDragStart(this, e); }

  // Delegate data methods
  loadColumnData() { return loadColumnData(this); }

  // Delegate action methods
  async applyAllChangesToParent() { return applyAllChangesToParent(this); }
  async discardAllChanges() { return discardAllChanges(this); }
  buildPendingTemplateColumns() { return buildPendingTemplateColumns(this); }
  getCurrentColumnTableData() {
    return getCurrentColumnTableData(this);
  }
  handleCellEdit(cell) { return handleCellEdit(this, cell); }
  handleRowReorder() { return handleRowReorder(this); }

  // Delegate UI methods
  createPopup() { createPopup(this); }
  positionPopup() { positionPopup(this); }
  show() { show(this); }
  hide() { hide(this); }
  close() { close(this); }
  setTableWidth(mode) { setTableWidth$1(this, mode); }

  // Handle close (always applies changes and closes)
  async handleClose() {
    await this.applyAllChangesToParent();
    this.close();
  }

  // Handle save (saves row edit if any, applies changes to parent)
  async handleSave() {
    // Save any current row edit
    if (this.columnTable?.isEditing) {
      await this.columnTable.handleSave();
    }
    // Apply changes to parent and close
    await this.applyAllChangesToParent();
    this.close();
  }

  // Handle cancel (cancels row edit if any, discards changes and closes)
  async handleCancel() {
    // Cancel any current row edit
    if (this.columnTable?.isEditing) {
      await this.columnTable.handleCancel();
    }
    // Discard changes and close
    await this.discardAllChanges();
    this.close();
  }

  // Delegate table methods
  async initColumnTable() { return initColumnTable(this); }
  wireRowReorderEvents() { wireRowReorderEvents(this); }
  configureNavigatorButtons() {
    configureNavigatorButtons(this);
  }
  async handleColumnTableExecuteSave() {
    this.syncDirtyState();
  }
  setOrderingMode(mode) { return setOrderingMode(this, mode); }
  getManagerTableStorageKey() {
    return this.isDepth2ColumnManager()
      ? 'lithium_column_manager_manager'
      : 'lithium_column_manager';
  }
  getManagerTablePath() {
    return this.isDepth2ColumnManager()
      ? 'column-manager/column-manager-manager'
      : 'column-manager/column-manager';
  }
  async getColtypesForManager() {
    return getColtypesForManager();
  }
  buildColumnManagerDefinition() {
    return buildColumnManagerDefinition(this);
  }
  onRowSelected(rowData) {
    this.saveSetting('selectedRow', rowData.field_name);
    log(Subsystems.TABLE, Status.DEBUG, `[ColumnManager] Row selected: ${rowData.field_name}`);
  }
  onRowDeselected() {
    this.saveSetting('selectedRow', null);
  }
  restoreSelectedRow() {
    const savedField = this.restoreSetting('selectedRow', null);
    if (savedField && this.columnTable?.table) {
      const rows = this.columnTable.table.getRows();
      const targetRow = rows.find((row) => row.getData().field_name === savedField);
      if (targetRow) {
        targetRow.select();
      }
    }
  }

  /**
   * Refresh column data from parent table (alias)
   */
  async refresh() {
    await loadColumnData(this);
    if (this.columnTable?.table) {
      await this.columnTable.table.replaceData(this.columnData);
    }
  }

  /**
   * Get available format options (for backward compatibility)
   */
  getFormatOptions() {
    return [
      { value: 'string', label: 'Text' },
      { value: 'integer', label: 'Integer' },
      { value: 'decimal', label: 'Decimal' },
      { value: 'index', label: 'Index' },
      { value: 'date', label: 'Date' },
      { value: 'datetime', label: 'DateTime' },
      { value: 'boolean', label: 'Boolean' },
      { value: 'lookup', label: 'Lookup' },
    ];
  }

  /**
   * Get available summary options (for backward compatibility)
   */
  getSummaryOptions() {
    return [
      { value: 'none', label: 'None' },
      { value: 'sum', label: 'Sum' },
      { value: 'count', label: 'Count' },
      { value: 'avg', label: 'Average' },
      { value: 'min', label: 'Minimum' },
      { value: 'max', label: 'Maximum' },
    ];
  }

  /**
   * Get available alignment options (for backward compatibility)
   */
  getAlignmentOptions() {
    return [
      { value: 'left', label: 'Left' },
      { value: 'center', label: 'Center' },
      { value: 'right', label: 'Right' },
    ];
  }

  /**
   * Get column group for a field (for backward compatibility)
   */
  getColumnGroup(field) {
    if (field.includes('id') || field.includes('ID')) return 'Identifiers';
    if (field.includes('name') || field.includes('Name')) return 'Names';
    if (field.includes('date') || field.includes('time')) return 'Dates';
    if (field.includes('status') || field.includes('state')) return 'Status';
    return 'General';
  }

  /**
   * Cleanup method
   */
  cleanup() {
    this.close();
    if (this.columnTable) {
      this.columnTable.destroy();
      this.columnTable = null;
    }
    if (this.popup) {
      this.popup.remove();
      this.popup = null;
    }
    this.isInitialized = false;
  }
}

/**
 * Column Manager Integration
 *
 * Handles Column Manager popup lifecycle and coordination.
 *
 * @module tables/column-manager/cm-integration
 */


/**
 * Toggle the Column Manager popup.
 * @param {Object} table - LithiumTable instance
 * @param {Event} e - Click event
 * @param {Object} column - Tabulator column component
 */
async function toggleColumnManager(table, e, column) {
  e.stopPropagation();

  if (table.columnManager) {
    closeColumnManager(table);
    return;
  }

  const parentColumnManager = table.container.closest('.col-manager-popup');
  closeAllColumnManagers(parentColumnManager);

  const managerId = getManagerId(table);

  if (!table.columnManager) {
    table.columnManager = new LithiumColumnManager({
      parentContainer: table.container,
      anchorElement: column.getElement(),
      parentTable: table,
      app: table.app,
      managerId: managerId,
      cssPrefix: 'col-manager',
      onColumnChange: (field, property, value) => {
        onColumnManagerChange(table, field, property, value);
      },
      onClose: () => {
        table.columnManager.hide();
      },
    });
  }

  await table.columnManager.init();
  table.columnManager.show();
}

/**
 * Close all Column Manager popups except the specified parent.
 * @param {HTMLElement} [parentColumnManager=null] - Popup to exclude from closing
 */
function closeAllColumnManagers(parentColumnManager = null) {
  const popups = document.querySelectorAll('.col-manager-popup');

  popups.forEach((popup) => {
    if (popup === parentColumnManager) {
      return;
    }

    if (popup._columnManagerInstance) {
      popup._columnManagerInstance.cleanup();
    } else {
      popup.remove();
    }
  });
}

/**
 * Get a unique manager ID for this table.
 * @param {Object} table - LithiumTable instance
 * @returns {string} Manager ID
 */
function getManagerId(table) {
  if (table.storageKey) {
    return table.storageKey;
  }

  if (table.tablePath) {
    return table.tablePath.replace(/\//g, '_');
  }

  if (table.container?.id) {
    return table.container.id;
  }

  return `table_${Date.now()}`;
}

/**
 * Close the Column Manager for this table.
 * @param {Object} table - LithiumTable instance
 */
function closeColumnManager(table) {
  if (table.columnManager) {
    table.columnManager.cleanup();
    table.columnManager = null;
  }
}

/**
 * Handle column changes from the Column Manager.
 * @param {Object} table - LithiumTable instance
 * @param {string} field - Column field name
 * @param {string} property - Changed property
 * @param {*} value - New value
 */
function onColumnManagerChange(table, field, property, value) {
  log(Subsystems.TABLE, Status.DEBUG,
    `[LithiumTable] Column manager change: ${field}.${property} = ${value}`);
}

/**
 * Navigator Builder Module
 *
 * Builds the navigator control bar HTML and wires up event handlers.
 *
 * @module tables/navigator/navigator-builder
 */


/**
 * Get the HTML for the navigator control bar
 * @param {Object} options - Configuration options
 * @param {string} options.cssPrefix - CSS class prefix
 * @param {boolean} options.filtersVisible - Whether filters are visible
 * @param {boolean} options.readonly - Whether table is readonly
 * @param {boolean} options.alwaysEditable - Whether table is always editable
 * @returns {string} HTML string
 */
function getNavigatorHTML({ cssPrefix, filtersVisible, readonly, alwaysEditable, localSearch }) {
  const searchPlaceholder = localSearch ? 'Search Local...' : 'Search Server...';
  return `
    <div class="lithium-nav-wrapper">
      <div class="lithium-nav-block lithium-nav-block-control">
        <button type="button" class="lithium-nav-btn" id="${cssPrefix}-nav-refresh" title="Refresh">
          <fa fa-arrows-rotate></fa>
        </button>
        <button type="button" class="lithium-nav-btn ${filtersVisible ? 'lithium-nav-btn-active' : ''}" id="${cssPrefix}-nav-filter" title="Toggle Filters">
          <fa fa-filter></fa>
        </button>
        <button type="button" class="lithium-nav-btn lithium-nav-btn-has-popup" id="${cssPrefix}-nav-grouping" title="Grouping">
          <fa fa-layer-group></fa>
        </button>
        <button type="button" class="lithium-nav-btn lithium-nav-btn-has-popup" id="${cssPrefix}-nav-width" title="Table Width">
          <fa fa-left-right></fa>
        </button>
        <button type="button" class="lithium-nav-btn lithium-nav-btn-has-popup" id="${cssPrefix}-nav-layout" title="Table Layout">
          <fa fa-table-layout></fa>
        </button>
        <button type="button" class="lithium-nav-btn lithium-nav-btn-has-popup" id="${cssPrefix}-nav-template" title="Templates">
          <fa fa-star></fa>
        </button>
      </div>
      <div class="lithium-nav-block lithium-nav-block-move">
        <button type="button" class="lithium-nav-btn" id="${cssPrefix}-nav-first" title="First Record">
          <fa fa-triple-chevrons-left></fa>
        </button>
        <button type="button" class="lithium-nav-btn" id="${cssPrefix}-nav-pgup" title="Previous Page">
          <fa fa-chevrons-left></fa>
        </button>
        <button type="button" class="lithium-nav-btn" id="${cssPrefix}-nav-prev" title="Previous Record">
          <fa fa-chevron-left></fa>
        </button>
        <button type="button" class="lithium-nav-btn" id="${cssPrefix}-nav-next" title="Next Record">
          <fa fa-chevron-right></fa>
        </button>
        <button type="button" class="lithium-nav-btn" id="${cssPrefix}-nav-pgdn" title="Next Page">
          <fa fa-chevrons-right></fa>
        </button>
        <button type="button" class="lithium-nav-btn" id="${cssPrefix}-nav-last" title="Last Record">
          <fa fa-triple-chevrons-right></fa>
        </button>
      </div>
      ${!readonly && !alwaysEditable ? `
      <div class="lithium-nav-block lithium-nav-block-manage">
        <button type="button" class="lithium-nav-btn" id="${cssPrefix}-nav-add" title="Add">
          <fa fa-plus></fa>
        </button>
        <button type="button" class="lithium-nav-btn" id="${cssPrefix}-nav-duplicate" title="Duplicate">
          <fa fa-copy></fa>
        </button>
        <button type="button" class="lithium-nav-btn" id="${cssPrefix}-nav-edit" title="Edit">
          <fa fa-pen-to-square></fa>
        </button>
        <button type="button" class="lithium-nav-btn" id="${cssPrefix}-nav-flag" title="Flag">
          <fa fa-flag></fa>
        </button>
        <button type="button" class="lithium-nav-btn" id="${cssPrefix}-nav-annotate" title="Annotate">
          <fa fa-note-sticky></fa>
        </button>
        <button type="button" class="lithium-nav-btn lithium-nav-btn-danger" id="${cssPrefix}-nav-delete" title="Delete">
          <fa fa-trash-can></fa>
        </button>
      </div>
      ` : ''}
      <div class="lithium-nav-block lithium-nav-block-search">
        <button type="button" class="lithium-nav-btn" id="${cssPrefix}-search-btn" title="Search">
          <fa fa-magnifying-glass></fa>
        </button>
        <input type="text" class="lithium-nav-search-input" id="${cssPrefix}-search-input" placeholder="${searchPlaceholder}">
        <button type="button" class="lithium-nav-btn" id="${cssPrefix}-search-clear-btn" title="Clear Search">
          <fa fa-xmark></fa>
        </button>
      </div>
    </div>
  `;
}

/**
 * Build navigator and attach to container
 * @param {Object} table - LithiumTable instance
 */
function buildNavigator(table) {
  if (!table.navigatorContainer) return;

  table.navigatorContainer.innerHTML = getNavigatorHTML({
    cssPrefix: table.cssPrefix,
    filtersVisible: table.filtersVisible,
    readonly: table.readonly,
    alwaysEditable: table.alwaysEditable,
    localSearch: table.localSearch,
  });
  processIcons(table.navigatorContainer);

  wireControlButtons(table);
  wireMoveButtons(table);
  wireManageButtons(table);
  wireSearchButtons(table);
}

/**
 * Wire control button events
 * @param {Object} table - LithiumTable instance
 */
function wireControlButtons(table) {
  const nav = table.navigatorContainer;
  if (!nav) return;

  nav.querySelector(`#${table.cssPrefix}-nav-refresh`)?.addEventListener('click', () => table.handleRefresh?.());
  nav.querySelector(`#${table.cssPrefix}-nav-filter`)?.addEventListener('click', () => table.handleFilter?.());
   nav.querySelector(`#${table.cssPrefix}-nav-grouping`)?.addEventListener('click', (e) => table.toggleNavPopup?.(e, 'grouping'));
  nav.querySelector(`#${table.cssPrefix}-nav-width`)?.addEventListener('click', (e) => table.toggleNavPopup?.(e, 'width'));
  nav.querySelector(`#${table.cssPrefix}-nav-layout`)?.addEventListener('click', (e) => table.toggleNavPopup?.(e, 'layout'));
  nav.querySelector(`#${table.cssPrefix}-nav-template`)?.addEventListener('click', (e) => table.toggleNavPopup?.(e, 'template'));
}

/**
 * Wire move/navigation button events
 * @param {Object} table - LithiumTable instance
 */
function wireMoveButtons(table) {
  const nav = table.navigatorContainer;
  if (!nav) return;

  nav.querySelector(`#${table.cssPrefix}-nav-first`)?.addEventListener('click', () => table.navigateFirst?.());
  nav.querySelector(`#${table.cssPrefix}-nav-pgup`)?.addEventListener('click', () => table.navigatePrevPage?.());
  nav.querySelector(`#${table.cssPrefix}-nav-prev`)?.addEventListener('click', () => table.navigatePrevRec?.());
  nav.querySelector(`#${table.cssPrefix}-nav-next`)?.addEventListener('click', () => table.navigateNextRec?.());
  nav.querySelector(`#${table.cssPrefix}-nav-pgdn`)?.addEventListener('click', () => table.navigateNextPage?.());
  nav.querySelector(`#${table.cssPrefix}-nav-last`)?.addEventListener('click', () => table.navigateLast?.());
}

/**
 * Wire manage (CRUD) button events
 * @param {Object} table - LithiumTable instance
 */
function wireManageButtons(table) {
  if (table.readonly || table.alwaysEditable) return;

  const nav = table.navigatorContainer;
  if (!nav) return;

  nav.querySelector(`#${table.cssPrefix}-nav-add`)?.addEventListener('click', () => table.handleAdd?.());
  nav.querySelector(`#${table.cssPrefix}-nav-duplicate`)?.addEventListener('click', () => table.handleDuplicate?.());
  nav.querySelector(`#${table.cssPrefix}-nav-edit`)?.addEventListener('click', () => table.handleEdit?.());
  nav.querySelector(`#${table.cssPrefix}-nav-flag`)?.addEventListener('click', () => table.handleFlag?.());
  nav.querySelector(`#${table.cssPrefix}-nav-annotate`)?.addEventListener('click', () => table.handleAnnotate?.());
  nav.querySelector(`#${table.cssPrefix}-nav-delete`)?.addEventListener('click', () => table.handleDelete?.());
}

/**
 * Wire search input events
 * @param {Object} table - LithiumTable instance
 */
function wireSearchButtons(table) {
  const nav = table.navigatorContainer;
  if (!nav) return;

  const searchInput = nav.querySelector(`#${table.cssPrefix}-search-input`);
  const searchBtn = nav.querySelector(`#${table.cssPrefix}-search-btn`);
  const clearBtn = nav.querySelector(`#${table.cssPrefix}-search-clear-btn`);

  const performSearch = () => {
    if (table.localSearch) {
      performLocalSearch(table, searchInput.value);
    } else {
      table.loadData?.(searchInput.value);
    }
  };

  searchBtn?.addEventListener('click', performSearch);
  searchInput?.addEventListener('keypress', (e) => {
    if (e.key === 'Enter') performSearch();
  });

  clearBtn?.addEventListener('click', () => {
    searchInput.value = '';
    if (table.localSearch) {
      performLocalSearch(table, '');
    } else {
      table.loadData?.();
    }
  });
}



/**
 * Update edit button state
 * @param {Object} table - LithiumTable instance
 */
function updateEditButtonState(table) {
  if (table.readonly || table.alwaysEditable || !table.navigatorContainer) return;
  const editBtn = table.navigatorContainer.querySelector(`#${table.cssPrefix}-nav-edit`);
  if (editBtn) {
    editBtn.classList.toggle('lithium-nav-btn-active', table.isEditing);
  }
}

/**
 * Update duplicate button state (disabled when no selection)
 * @param {Object} table - LithiumTable instance
 */
function updateDuplicateButtonState(table) {
  if (table.readonly || table.alwaysEditable || !table.navigatorContainer) return;
  const duplicateBtn = table.navigatorContainer.querySelector(`#${table.cssPrefix}-nav-duplicate`);
  if (duplicateBtn) {
    const hasSelection = table.table?.getSelectedRows().length > 0;
    duplicateBtn.disabled = !hasSelection;
  }
}

/**
 * Update move button states based on current position
 *
 * Simplified logic: Prior buttons (first/prevPage/prev) enabled when not on first record;
 * Next buttons (next/nextPage/last) enabled when not on last record.
 * @param {Object} table - LithiumTable instance
 */
function updateMoveButtonState(table) {
  if (!table.table) return;
  const nav = table.navigatorContainer;
  if (!nav) return;

  if (typeof table.table.getRows !== 'function') return;

  const { rows, selectedIndex } = getRowsAndIndex(table);
  const rowCount = rows.length;

  const atFirst = selectedIndex <= 0;
  const atLast = selectedIndex < 0 || selectedIndex >= rowCount - 1;

  const firstBtn = nav.querySelector(`#${table.cssPrefix}-nav-first`);
  const prevPageBtn = nav.querySelector(`#${table.cssPrefix}-nav-pgup`);
  const prevRecBtn = nav.querySelector(`#${table.cssPrefix}-nav-prev`);
  const nextRecBtn = nav.querySelector(`#${table.cssPrefix}-nav-next`);
  const nextPageBtn = nav.querySelector(`#${table.cssPrefix}-nav-pgdn`);
  const lastBtn = nav.querySelector(`#${table.cssPrefix}-nav-last`);

  if (firstBtn) firstBtn.disabled = atFirst;
  if (prevPageBtn) prevPageBtn.disabled = atFirst;
  if (prevRecBtn) prevRecBtn.disabled = atFirst;
  if (nextRecBtn) nextRecBtn.disabled = atLast;
  if (nextPageBtn) nextPageBtn.disabled = atLast;
  if (lastBtn) lastBtn.disabled = atLast;
}

/**
 * Get rows and current selected index
 * @param {Object} table - LithiumTable instance
 * @returns {Object} { rows, selectedIndex }
 */
function getRowsAndIndex(table) {
  try {
    // Get only the active (visible/filtered) rows, not all data rows
    const rows = table.table.getRows('active');

    const selectedRows = table.table.getSelectedRows();
    const selectedRow = selectedRows.length > 0 ? selectedRows[0] : null;
    let selectedIndex = -1;

    if (selectedRow) {
      const selectedPosition = selectedRow.getPosition?.(true);

      if (Number.isFinite(selectedPosition) && selectedPosition > 0) {
        selectedIndex = selectedPosition - 1;
      } else {
        selectedIndex = rows.findIndex((r) => r === selectedRow);
      }
    }

    return { rows, selectedIndex };
  } catch {
    return { rows: [], selectedIndex: -1 };
  }
}

/**
 * Grouping Popup Module
 *
 * Handles the grouping popup menu with tri-state sort direction
 * and drag-to-reorder functionality.
 *
 * @module tables/popups/grouping-popup
 */


/**
 * Build the grouping popup with column list
 * @param {Object} table - LithiumTable instance
 * @returns {HTMLElement} Popup element
 */
function buildGroupingPopup(table) {
  const popup = document.createElement('div');
  popup.className = 'lithium-nav-popup lithium-grouping-popup';

  // Collapse All (at top)
  const collapseBtn = createPopupItem({
    icon: 'angles-up',
    label: 'Collapse All',
    action: () => {
      closeGroupingPopup(table);
      table.collapseAll?.();
    },
  });
  popup.appendChild(collapseBtn);

  // Expand All (below collapse)
  const expandBtn = createPopupItem({
    icon: 'angles-down',
    label: 'Expand All',
    action: () => {
      closeGroupingPopup(table);
      table.expandAll?.();
    },
  });
  popup.appendChild(expandBtn);

  // Separator
  popup.appendChild(createSeparator());

  // Get columns that can be grouped (only those with groupable: true)
  const groupableColumns = getGroupableColumns(table);

  if (groupableColumns.length === 0) {
    const noCols = document.createElement('div');
    noCols.className = 'lithium-grouping-empty';
    noCols.textContent = 'No groupable columns';
    popup.appendChild(noCols);
  } else {
    // Sort by current group priority
    const sortedCols = sortColumnsByGroupPriority(groupableColumns);

    // Create list container
    const list = document.createElement('div');
    list.className = 'lithium-grouping-list';

    sortedCols.forEach((col, index) => {
      const item = createGroupingItem(table, col);
      list.appendChild(item);
    });

    popup.appendChild(list);

    // Setup drag and drop
    setupDragAndDrop(list, table);
  }

  // Separator
  popup.appendChild(createSeparator());

  // Remove Grouping
  const removeBtn = createPopupItem({
    icon: 'broom',
    label: 'Remove Grouping',
    action: () => {
      closeGroupingPopup(table);
      removeAllGrouping(table);
    },
  });
  popup.appendChild(removeBtn);

  return popup;
}

/**
 * Create a standard popup item with icon
 * @param {Object} options - Item options
 * @returns {HTMLElement} Item element
 */
function createPopupItem({ icon, label, action }) {
  const btn = document.createElement('button');
  btn.type = 'button';
  btn.className = 'lithium-nav-popup-item lithium-grouping-action';
  btn.innerHTML = `
    <fa class="lithium-grouping-action-icon" fa-${icon}></fa>
    <span class="lithium-grouping-action-label">${label}</span>
  `;
  btn.addEventListener('click', action);
  return btn;
}

/**
 * Create a separator element
 * @returns {HTMLElement} Separator element
 */
function createSeparator() {
  const sep = document.createElement('div');
  sep.className = 'lithium-nav-popup-separator';
  return sep;
}

/**
 * Check if a title contains an icon (fa- tag)
 * @param {string} title - Column title
 * @returns {boolean} True if title contains an icon
 */
function titleContainsIcon(title) {
  return typeof title === 'string' && /<fa\s+/.test(title);
}

/**
 * Get the display title for a column - uses title from tableDef
 * @param {Object} col - Column info from tableDef
 * @returns {string} Display title
 */
function getColumnDisplayTitle(col) {
  // Use the title property from the column definition
  if (col.title) {
    return col.title;
  }
  // Fallback to field name if no title
  return col.field || '';
}

/**
 * Create a lookup preview element for the grouping popup
 * For iconLabel: shows title (icon) + groupTitle (text label)
 * @param {Object} col - Column info from tableDef
 * @returns {string} HTML string for the preview, or empty string if not applicable
 */
function createLookupPreview(col) {
  if (!col.lookupRef) {
    return '';
  }

  // groupStyle controls display in grouping UI, fallback to lookupStyle
  const groupStyle = col.groupStyle || col.lookupStyle || 'label';

  switch (groupStyle) {
    case 'icon':
      // Title contains the icon
      return '';
    case 'iconLabel':
      // Title (icon) + groupTitle (text label)
      // groupTitle defaults to field name if not set
      {
        const groupTitle = col.groupTitle || col.field || '';
        if (groupTitle) {
          return `<span class="lithium-grouping-lookup-preview lithium-grouping-lookup-iconlabel"><span class="lithium-grouping-grouptitle">${groupTitle}</span></span>`;
        }
      }
      break;
    case 'label':
    default:
      return '';
  }

  return '';
}

/**
 * Create a grouping item for a column
 * @param {Object} table - LithiumTable instance
 * @param {Object} col - Column info
 * @param {number} index - Current index in list
 * @param {number} total - Total items in list
 * @returns {HTMLElement} Item element
 */
function createGroupingItem(table, col, index, total) {
  const item = document.createElement('div');
  item.className = 'lithium-grouping-item';
  item.dataset.field = col.field;
  item.draggable = true;

  // Determine current state
  const isGrouped = col.groupPri != null && col.groupable === true;
  const sortDir = col.groupDir || 'asc';

  // Get tooltip text for current state
  const tooltipText = getGroupingTooltip(isGrouped, sortDir);

  // Tri-state arrow (left side) - using Font Awesome icons
  const arrowBtn = document.createElement('button');
  arrowBtn.type = 'button';
  arrowBtn.className = 'lithium-grouping-arrow';
  arrowBtn.title = tooltipText;
  arrowBtn.innerHTML = getArrowIcon(isGrouped, sortDir);

  // Column title - handle icon titles, use the title value from column definition
  const title = document.createElement('span');
  title.className = 'lithium-grouping-title';
  title.title = tooltipText;

  const displayTitle = getColumnDisplayTitle(col);

  // Build title content with optional lookup preview
  const lookupPreview = createLookupPreview(col);
  if (lookupPreview) {
    // Show title with lookup preview
    title.innerHTML = `<span class="lithium-grouping-title-text">${displayTitle}</span><span class="lithium-grouping-title-separator">—</span>${lookupPreview}`;
  } else if (titleContainsIcon(displayTitle)) {
    // Title contains an icon - render as HTML
    title.innerHTML = displayTitle;
  } else {
    // Regular text title
    title.textContent = displayTitle;
  }

  if (isGrouped) {
    title.classList.add('lithium-grouping-active');
  }

  // Make the title clickable to cycle through states (same as clicking the arrow)
  title.style.cursor = 'pointer';
  title.addEventListener('click', (e) => {
    e.stopPropagation();
    cycleArrowState(table, col.field, isGrouped, sortDir);
  });

  // Also handle arrow button click
  arrowBtn.addEventListener('click', (e) => {
    e.stopPropagation();
    cycleArrowState(table, col.field, isGrouped, sortDir);
  });

  // Drag handle (right side) - using grip-dots-vertical
  const handle = document.createElement('span');
  handle.className = 'lithium-grouping-handle';
  handle.innerHTML = '<fa fa-grip-dots-vertical></fa>';
  handle.title = 'Drag to reorder grouping priority';

  item.appendChild(arrowBtn);
  item.appendChild(title);
  item.appendChild(handle);

  return item;
}

/**
 * Get tooltip text based on grouping state
 * @param {boolean} isGrouped - Whether column is grouped
 * @param {string} sortDir - Sort direction
 * @returns {string} Tooltip text
 */
function getGroupingTooltip(isGrouped, sortDir) {
  if (!isGrouped) return 'Click to group ascending';
  return sortDir === 'asc' ? 'Click to group descending' : 'Click to remove grouping';
}

/**
 * Get arrow icon based on state - using Font Awesome icons
 * @param {boolean} isGrouped - Whether column is grouped
 * @param {string} sortDir - Sort direction
 * @returns {string} HTML for icon
 */
function getArrowIcon(isGrouped, sortDir) {
  if (!isGrouped) {
    // Disabled state - show arrows up down icon muted
    return '<span class="lithium-grouping-sort-icons" data-sort-dir="none"><fa class="lithium-grouping-sort-none" fa-angles-up-down></fa></span>';
  }
  // Active state - show only the active icon
  return sortDir === 'asc'
    ? '<span class="lithium-grouping-sort-icons" data-sort-dir="asc"><fa class="lithium-grouping-sort-asc" fa-angle-up></fa></span>'
    : '<span class="lithium-grouping-sort-icons" data-sort-dir="desc"><fa class="lithium-grouping-sort-desc" fa-angle-down></fa></span>';
}

/**
 * Cycle through arrow states: disabled -> asc -> desc -> disabled
 * @param {Object} table - LithiumTable instance
 * @param {string} field - Column field name
 * @param {boolean} isGrouped - Current grouped state
 * @param {string} sortDir - Current sort direction
 */
function cycleArrowState(table, field, isGrouped, sortDir) {
  let newGroupPri = null;
  let newGroupOrd = 'asc';

  if (!isGrouped) {
    // Disabled -> Ascending: assign next available priority
    const maxPri = getMaxGroupPriority(table);
    newGroupPri = maxPri + 1;
    newGroupOrd = 'asc';
  } else if (sortDir === 'asc') {
    // Ascending -> Descending: keep priority, change direction
    const col = getColumnFromTableDef(table, field);
    newGroupPri = col?.groupPri || 1;
    newGroupOrd = 'desc';
  } else {
    // Descending -> Disabled: remove from grouping
    newGroupPri = null;
    newGroupOrd = 'asc';
  }

  // Update the column definition
  updateColumnGrouping(table, field, newGroupPri, newGroupOrd);
}

/**
 * Get the maximum group priority currently assigned
 * @param {Object} table - LithiumTable instance
 * @returns {number} Max priority
 */
function getMaxGroupPriority(table) {
  if (!table.tableDef?.columns) return 0;

  let max = 0;
  for (const col of Object.values(table.tableDef.columns)) {
    if (col.groupPri != null && col.groupPri > max) {
      max = col.groupPri;
    }
  }
  return max;
}

/**
 * Get column info from tableDef
 * @param {Object} table - LithiumTable instance
 * @param {string} field - Field name
 * @returns {Object|null} Column definition
 */
function getColumnFromTableDef(table, field) {
  return table.tableDef?.columns?.[field] || null;
}

/**
 * Update column grouping properties
 * @param {Object} table - LithiumTable instance
 * @param {string} field - Column field
 * @param {number|null} groupPri - Group priority (null to remove)
 * @param {string} groupDir - Group direction
 */
function updateColumnGrouping(table, field, groupPri, groupDir) {
  if (!table.tableDef?.columns?.[field]) return;

  const col = table.tableDef.columns[field];

  if (groupPri == null) {
    // Remove grouping - renumber remaining columns
    delete col.groupPri;
    delete col.groupDir;
    renumberGroupPriorities(table);
  } else {
    col.groupable = true;
    col.groupPri = groupPri;
    col.groupDir = groupDir;
  }

  // Apply the new grouping to the table
  applyGroupingChanges(table);

  // Update the popup UI in place instead of closing/reopening
  updatePopupUIInPlace(table);
}

/**
 * Renumber group priorities to be sequential (1, 2, 3...)
 * @param {Object} table - LithiumTable instance
 */
function renumberGroupPriorities(table) {
  if (!table.tableDef?.columns) return;

  // Collect grouped columns and sort by current priority
  const grouped = [];
  for (const [field, col] of Object.entries(table.tableDef.columns)) {
    if (col.groupPri != null) {
      grouped.push({ field, pri: col.groupPri });
    }
  }
  grouped.sort((a, b) => a.pri - b.pri);

  // Reassign sequential priorities
  grouped.forEach((item, index) => {
    table.tableDef.columns[item.field].groupPri = index + 1;
  });
}

/**
 * Apply grouping changes to the live table
 * @param {Object} table - LithiumTable instance
 */
function applyGroupingChanges(table) {
  if (!table.table) return;

  // Use the resolveTableOptions logic to rebuild groupBy
  const opts = resolveTableOptions(table.tableDef);

  if (opts.groupBy) {
    table.table.setGroupBy(opts.groupBy);
    table.table.setGroupStartOpen(true);
  } else {
    table.table.setGroupBy(false);
  }

  // Update group icons after change
  setTimeout(() => table.updateGroupIcons?.(), 50);

  log(Subsystems.TABLE, Status.INFO, `[LithiumTable] Grouping updated: ${JSON.stringify(opts.groupBy)}`);
}

/**
 * Remove all grouping from the table
 * @param {Object} table - LithiumTable instance
 */
function removeAllGrouping(table) {
  if (!table.tableDef?.columns) return;

  // Clear all groupPri values
  for (const col of Object.values(table.tableDef.columns)) {
    delete col.groupPri;
    delete col.groupDir;
  }

  // Remove grouping from the table
  table.table?.setGroupBy(false);

  log(Subsystems.TABLE, Status.INFO, '[LithiumTable] All grouping removed');
}

/**
 * Get list of columns that can be grouped (only those with groupable: true)
 * @param {Object} table - LithiumTable instance
 * @returns {Array} Array of column info objects
 */
function getGroupableColumns(table) {
  if (!table.tableDef?.columns) return [];

  const columns = [];
  for (const [field, col] of Object.entries(table.tableDef.columns)) {
    // Only include columns explicitly marked as groupable: true
    if (col.groupable !== true) continue;

    // Skip selector column
    if (field === '_selector') continue;

    columns.push({
      field,
      title: col.title,
      groupable: col.groupable,
      groupPri: col.groupPri,
      groupDir: col.groupDir,
      lookupRef: col.lookupRef,
      lookupStyle: col.lookupStyle,
      groupStyle: col.groupStyle,
      groupTitle: col.groupTitle,
    });
  }
  return columns;
}

/**
 * Sort columns by group priority (grouped first, then by priority, then alphabetically)
 * @param {Array} columns - Column array
 * @returns {Array} Sorted array
 */
function sortColumnsByGroupPriority(columns) {
  return [...columns].sort((a, b) => {
    // Grouped columns come first
    const aGrouped = a.groupPri != null && a.groupable === true;
    const bGrouped = b.groupPri != null && b.groupable === true;

    if (aGrouped && !bGrouped) return -1;
    if (!aGrouped && bGrouped) return 1;

    // Both grouped: sort by priority
    if (aGrouped && bGrouped) {
      return (a.groupPri || 0) - (b.groupPri || 0);
    }

    // Neither grouped: sort alphabetically by title (strip HTML for comparison)
    const aTitle = typeof a.title === 'string' ? a.title.replace(/<[^>]*>/g, '') : (a.title || a.field);
    const bTitle = typeof b.title === 'string' ? b.title.replace(/<[^>]*>/g, '') : (b.title || b.field);
    return aTitle.localeCompare(bTitle);
  });
}

/**
 * Setup drag and drop for the grouping list
 * @param {HTMLElement} list - List container
 * @param {Object} table - LithiumTable instance
 */
function setupDragAndDrop(list, table) {
  let draggedItem = null;

  list.addEventListener('dragstart', (e) => {
    draggedItem = e.target.closest('.lithium-grouping-item');
    if (!draggedItem) return;

    e.dataTransfer.effectAllowed = 'move';
    draggedItem.classList.add('lithium-grouping-dragging');
  });

  list.addEventListener('dragend', (e) => {
    if (draggedItem) {
      draggedItem.classList.remove('lithium-grouping-dragging');
      draggedItem = null;
    }
  });

  list.addEventListener('dragover', (e) => {
    e.preventDefault();
    e.dataTransfer.dropEffect = 'move';

    if (!draggedItem) return;

    const targetItem = e.target.closest('.lithium-grouping-item');
    if (!targetItem || targetItem === draggedItem) return;

    const rect = targetItem.getBoundingClientRect();
    const midpoint = rect.top + rect.height / 2;

    if (e.clientY < midpoint) {
      list.insertBefore(draggedItem, targetItem);
    } else {
      list.insertBefore(draggedItem, targetItem.nextSibling);
    }
  });

  list.addEventListener('drop', (e) => {
    e.preventDefault();
    if (!draggedItem) return;

    // Recalculate priorities based on new order
    updatePrioritiesFromOrder(list, table);
  });
}

/**
 * Update group priorities based on the visual order in the list
 * @param {HTMLElement} list - List container
 * @param {Object} table - LithiumTable instance
 */
function updatePrioritiesFromOrder(list, table) {
  const items = list.querySelectorAll('.lithium-grouping-item');
  let priority = 1;

  items.forEach((item) => {
    const field = item.dataset.field;
    const col = getColumnFromTableDef(table, field);

    if (col && col.groupPri != null) {
      col.groupPri = priority++;
    }
  });

  // Apply changes
  applyGroupingChanges(table);
}

/**
 * Close the grouping popup with animation
 * @param {Object} table - LithiumTable instance
 */
function closeGroupingPopup(table) {
  // Remove popup-active class from button
  if (table.activeNavPopupButton) {
    table.activeNavPopupButton.classList.remove('popup-active');
  }
  if (table.activeNavPopup) {
    table.activeNavPopup.classList.remove('visible');
    // Remove after animation completes
    const duration = 300; // Match transition-fast duration
    setTimeout(() => {
      if (table.activeNavPopup && table.activeNavPopup.parentNode) {
        table.activeNavPopup.remove();
      }
    }, duration);
    table.activeNavPopup = null;
    table.activeNavPopupId = null;
    table.activeNavPopupButton = null;
  }
  if (table.navPopupCloseHandler) {
    document.removeEventListener('click', table.navPopupCloseHandler);
    table.navPopupCloseHandler = null;
  }
  if (table.navPopupKeyHandler) {
    document.removeEventListener('keydown', table.navPopupKeyHandler);
    table.navPopupKeyHandler = null;
  }
  if (table.navPopupGlobalCloseHandler) {
    document.removeEventListener('close-all-popups', table.navPopupGlobalCloseHandler);
    table.navPopupGlobalCloseHandler = null;
  }
}

/**
 * Update the popup UI in place without closing/reopening
 * @param {Object} table - LithiumTable instance
 */
function updatePopupUIInPlace(table) {
  if (table.activeNavPopupId !== 'grouping' || !table.activeNavPopup) return;

  const popup = table.activeNavPopup;
  const list = popup.querySelector('.lithium-grouping-list');
  if (!list) return;

  // Get current groupable columns
  const groupableColumns = getGroupableColumns(table);
  const sortedCols = sortColumnsByGroupPriority(groupableColumns);

  // Get all current items
  const items = Array.from(list.querySelectorAll('.lithium-grouping-item'));
  const itemMap = new Map(items.map(item => [item.dataset.field, item]));

  // Track which fields we've processed
  const processedFields = new Set();

  // Update existing items and create new ones as needed
  sortedCols.forEach((col, index) => {
    processedFields.add(col.field);
    let item = itemMap.get(col.field);
    const isGrouped = col.groupPri != null && col.groupable === true;
  const sortDir = col.groupDir || 'asc';

    if (!item) {
      // Create new item if it doesn't exist (shouldn't happen normally)
      item = createGroupingItem(table, col);
      list.appendChild(item);
      processIcons(item);
    } else {
      // Update existing item
      const arrowBtn = item.querySelector('.lithium-grouping-arrow');
      const title = item.querySelector('.lithium-grouping-title');

      // Get current tooltip text for this state
      const tooltipText = getGroupingTooltip(isGrouped, sortDir);

      // Update arrow icon and tooltip
      if (arrowBtn) {
        arrowBtn.title = tooltipText;
        arrowBtn.innerHTML = getArrowIcon(isGrouped, sortDir);
      }

      // Update title styling and tooltip
      if (title) {
        title.classList.toggle('lithium-grouping-active', isGrouped);
        title.title = tooltipText;
      }

      // Update click handlers - look up current state at click time
      const newClickHandler = (e) => {
        e.stopPropagation();
        // Get fresh state from tableDef at click time
        const freshCol = getColumnFromTableDef(table, col.field);
        const freshIsGrouped = freshCol?.groupPri != null && freshCol?.groupable === true;
        const freshSortDir = freshCol?.groupDir || 'asc';
        cycleArrowState(table, col.field, freshIsGrouped, freshSortDir);
      };

      if (arrowBtn) {
        // Clear title to dismiss any visible tooltip before cloning
        const savedTitle = arrowBtn.title;
        arrowBtn.title = '';
        // Replace click handler by cloning
        const newArrowBtn = arrowBtn.cloneNode(true);
        newArrowBtn.title = savedTitle;
        newArrowBtn.addEventListener('click', newClickHandler);
        arrowBtn.parentNode.replaceChild(newArrowBtn, arrowBtn);
      }

      if (title) {
        // Clear title to dismiss any visible tooltip before cloning
        const savedTitle = title.title;
        title.title = '';
        const newTitle = title.cloneNode(true);
        newTitle.title = savedTitle;
        newTitle.style.cursor = 'pointer';
        newTitle.addEventListener('click', newClickHandler);
        title.parentNode.replaceChild(newTitle, title);
      }
    }
  });

  // Re-sort the DOM elements to match the new order
  sortedCols.forEach((col) => {
    const item = itemMap.get(col.field);
    if (item) {
      list.appendChild(item); // Moves to end, effectively re-ordering
    }
  });

  // Process any new icons
  processIcons(list);
}

/**
 * Template Popup Module
 *
 * Template menu popup with save/load/manage functionality.
 *
 * @module tables/popups/template-popup
 */


/**
 * Build template popup
 * @param {Object} table - LithiumTable instance
 * @returns {HTMLElement} Popup element
 */
function buildTemplatePopup(table) {
  const templates = getSavedTemplates(table);
  const selectedName = getSelectedTemplateName(table, templates);
  const selectedIndex = templates.findIndex(
    (template) => getTemplateName(table, template) === selectedName,
  );

  const popup = document.createElement('div');
  popup.className = 'lithium-nav-popup lithium-template-menu';

  popup.appendChild(createTemplateMenuAction(table, 'Save Template', async () => {
    logTemplateMenuSelection(table, 'save');
    const savedTemplate = saveTemplate(table);
    if (!savedTemplate) return;
    table.templateMenuSelectedName = getTemplateName(table, savedTemplate);
    refreshTemplatePopup(table);
  }, { icon: 'square' }));

  popup.appendChild(createTemplateMenuAction(table, 'Clear Template', () => {
    logTemplateMenuSelection(table, 'clear');
    closeNavPopup$1(table);
    table.clearTemplate?.();
  }, { icon: 'database' }));

  popup.appendChild(createPopupSeparator());

  const list = document.createElement('div');
  list.className = 'lithium-template-list';

  if (templates.length === 0) {
    const empty = document.createElement('div');
    empty.className = 'lithium-template-empty';
    empty.textContent = 'No saved templates';
    list.appendChild(empty);
  } else {
    templates.forEach((template) => {
      const templateName = getTemplateName(table, template);
      const row = document.createElement('button');
      row.type = 'button';
      row.className = 'lithium-nav-popup-item lithium-template-item';
      row.classList.toggle('is-selected', templateName === selectedName);

      const check = document.createElement('span');
      check.className = 'lithium-nav-popup-check';
      check.innerHTML = templateName === selectedName ? '&#10003;' : '';

      const nameSpan = document.createElement('span');
      nameSpan.className = 'lithium-template-name';
      nameSpan.textContent = templateName;

      row.appendChild(check);
      row.appendChild(nameSpan);

      row.addEventListener('click', async () => {
        logTemplateMenuSelection(table, 'select', templateName);
        await table.loadTemplate?.(template);
        refreshTemplatePopup(table);
      });

      list.appendChild(row);
    });
  }

  popup.appendChild(list);
  popup.appendChild(createPopupSeparator());

  popup.appendChild(createTemplateMenuAction(table, 'Copy to Clipboard', async () => {
    const selectedTemplate = selectedIndex >= 0 ? templates[selectedIndex] : null;
    logTemplateMenuSelection(table, 'copy', selectedTemplate ? getTemplateName(table, selectedTemplate) : 'current view');
    await table.copyTemplateToClipboard?.(selectedTemplate);
    closeNavPopup$1(table);
  }, { icon: 'copy' }));

  popup.appendChild(createTemplateMenuAction(table, 'Delete Template', () => {
    if (selectedIndex < 0) return;
    logTemplateMenuSelection(table, 'delete', selectedName);
    const deleted = deleteTemplate(table, selectedIndex);
    if (!deleted) return;
    table.templateMenuSelectedName = null;
    refreshTemplatePopup(table);
  }, { disabled: selectedIndex < 0, icon: 'trash-can' }));

  processIcons(popup);

  return popup;
}

/**
 * Create template menu action button
 * @param {Object} table - LithiumTable instance
 * @param {string} label - Button label
 * @param {Function} action - Click handler
 * @param {Object} options - Button options
 * @returns {HTMLElement} Button element
 */
function createTemplateMenuAction(table, label, action, options = {}) {
  const row = document.createElement('button');
  row.type = 'button';
  row.className = 'lithium-nav-popup-item';
  row.innerHTML = `
    <span class="lithium-nav-popup-icon"><fa fa-${options.icon || 'circle'}></fa></span>
    <span class="lithium-nav-popup-label">${label}</span>
  `;
  row.disabled = options.disabled === true;
  row.addEventListener('click', async () => {
    if (row.disabled) return;
    await action();
  });
  return row;
}

/**
 * Refresh template popup
 * @param {Object} table - LithiumTable instance
 */
function refreshTemplatePopup(table) {
  if (table.activeNavPopupId !== 'template') return;
  const btn = table.activeNavPopupButton
    || table.navigatorContainer?.querySelector(`#${table.cssPrefix}-nav-template`);
  if (!btn) return;

  const popup = buildTemplatePopup(table);
  if (!popup || !table.activeNavPopup) return;

  popup.style.position = 'fixed';
  popup.style.top = '0px';
  popup.style.left = '0px';
  popup.style.bottom = 'auto';
  table.activeNavPopup.replaceWith(popup);
  table.activeNavPopup = popup;
  
  // Re-position using popup-manager's logic
  const viewportPadding = 8;
  const gap = 4;
  const btnRect = btn.getBoundingClientRect();
  const popupRect = popup.getBoundingClientRect();
  const popupHeight = popupRect.height || popup.offsetHeight || 0;
  const availableAbove = btnRect.top - viewportPadding;
  const availableBelow = window.innerHeight - btnRect.bottom - viewportPadding;

  let top;
  if (popupHeight <= availableAbove || availableAbove >= availableBelow) {
    top = btnRect.top - popupHeight - gap;
  } else {
    top = btnRect.bottom + gap;
  }

  let left = btnRect.left;
  if (left + popupRect.width > window.innerWidth - viewportPadding) {
    left = window.innerWidth - popupRect.width - viewportPadding;
  }
  if (left < viewportPadding) {
    left = viewportPadding;
  }

  const maxTop = Math.max(viewportPadding, window.innerHeight - popupHeight - viewportPadding);
  popup.style.top = `${Math.min(Math.max(top, viewportPadding), maxTop)}px`;
  popup.style.left = `${left}px`;
  popup.style.right = 'auto';
  
  popup.classList.add('visible');
  requestAnimationFrame(() => refreshTemplatePopup(table));
}

/**
 * Get template storage key
 * @param {Object} table - LithiumTable instance
 * @returns {string} Storage key
 */
function getTemplateStorageKey(table) {
  return `${table.storageKey}_templates`;
}

/**
 * Get saved templates from localStorage
 * @param {Object} table - LithiumTable instance
 * @returns {Array} Saved templates
 */
function getSavedTemplates(table) {
  const storageKey = getTemplateStorageKey(table);
  try {
    const stored = localStorage.getItem(storageKey);
    const templates = stored ? JSON.parse(stored) : [];
    return Array.isArray(templates) ? templates : [];
  } catch (_e) {
    return [];
  }
}

/**
 * Save templates to localStorage
 * @param {Object} table - LithiumTable instance
 * @param {Array} templates - Templates to save
 */
function saveStoredTemplates(table, templates) {
  const storageKey = getTemplateStorageKey(table);
  localStorage.setItem(storageKey, JSON.stringify(templates));
}

/**
 * Get template name from template object
 * @param {Object} table - LithiumTable instance
 * @param {Object} template - Template object
 * @returns {string} Template name
 */
function getTemplateName(table, template) {
  return template?._templateMeta?.name || template?.name || 'Unnamed Template';
}

/**
 * Get selected template name
 * @param {Object} table - LithiumTable instance
 * @param {Array} templates - Available templates
 * @returns {string|null} Selected template name
 */
function getSelectedTemplateName(table, templates = []) {
  if (table.templateMenuSelectedName && templates.some(
    (template) => getTemplateName(table, template) === table.templateMenuSelectedName,
  )) {
    return table.templateMenuSelectedName;
  }

  if (table.activeTemplateName && templates.some(
    (template) => getTemplateName(table, template) === table.activeTemplateName,
  )) {
    return table.activeTemplateName;
  }

  return null;
}

/**
 * Log template menu selection
 * @param {Object} table - LithiumTable instance
 * @param {string} action - Action performed
 * @param {string} detail - Additional detail
 */
function logTemplateMenuSelection(table, action, detail = '') {
  const suffix = detail ? `: ${detail}` : '';
  log(Subsystems.TABLE, Status.INFO, `[LithiumTable] Template menu ${action}${suffix}`);
}

/**
 * Save current table configuration as template
 * @param {Object} table - LithiumTable instance
 * @returns {Object|null} Saved template or null
 */
function saveTemplate(table) {
  const template = table.generateTemplateJSON?.();
  if (!template) return null;

  const defaultName = `${template._templateMeta?.name || 'Template'} - ${new Date().toLocaleDateString()}`;
  const name = window.prompt('Enter template name:', defaultName);
  if (!name) return null;

  template._templateMeta = template._templateMeta || {};
  template._templateMeta.name = name;

  const templates = getSavedTemplates(table);
  const existingIndex = templates.findIndex((item) => getTemplateName(table, item) === name);

  if (existingIndex >= 0) {
    templates[existingIndex] = template;
  } else {
    templates.push(template);
  }

  try {
    saveStoredTemplates(table, templates);
    table.templateMenuSelectedName = name;
    log(Subsystems.TABLE, Status.INFO, `[LithiumTable] Template saved: ${name}`);
    return template;
  } catch (_e) {
    log(Subsystems.TABLE, Status.ERROR, '[LithiumTable] Failed to save template');
    return null;
  }
}

/**
 * Delete template by index
 * @param {Object} table - LithiumTable instance
 * @param {number} index - Template index
 * @returns {boolean} Success
 */
function deleteTemplate(table, index) {
  const templates = getSavedTemplates(table);

  if (index >= 0 && index < templates.length) {
    const name = getTemplateName(table, templates[index]);
    templates.splice(index, 1);
    try {
      saveStoredTemplates(table, templates);
      if (table.templateMenuSelectedName === name) {
        table.templateMenuSelectedName = null;
      }
      if (table.activeTemplateName === name) {
        table.activeTemplateName = null;
      }
      log(Subsystems.TABLE, Status.INFO, `[LithiumTable] Template deleted: ${name}`);
      return true;
    } catch (_e) {
      log(Subsystems.TABLE, Status.ERROR, `[LithiumTable] Failed to delete template: ${_e?.message}`);
    }
  }
  return false;
}

// Stub for closeNavPopup - will be imported from popup-manager in main mixin
function closeNavPopup$1(table) {
  if (table.activeNavPopup) {
    table.activeNavPopup.remove();
    table.activeNavPopup = null;
    table.activeNavPopupId = null;
    table.activeNavPopupButton = null;
  }
  if (table.navPopupCloseHandler) {
    document.removeEventListener('click', table.navPopupCloseHandler);
    table.navPopupCloseHandler = null;
  }
  if (table.navPopupRepositionHandler) {
    window.removeEventListener('resize', table.navPopupRepositionHandler);
    document.removeEventListener('scroll', table.navPopupRepositionHandler, true);
    table.navPopupRepositionHandler = null;
  }
}

/**
 * Popup Manager Module
 *
 * Handles navigation popup menus (table options, width, layout).
 *
 * @module tables/popups/popup-manager
 */


/**
 * Toggle navigation popup
 * @param {Object} table - LithiumTable instance
 * @param {Event} e - Click event
 * @param {string} popupId - Popup identifier
 */
function toggleNavPopup(table, e, popupId) {
  e.stopPropagation();

  // Prevent rapid toggling on the SAME button during transitions
  if (table._navPopupTransitioning && table.activeNavPopupId === popupId) return;

  if (table.activeNavPopup && table.activeNavPopupId === popupId) {
    closeNavPopup(table);
    return;
  }

  // Close any existing popup immediately, then open new one
  if (table.activeNavPopup) {
    closeNavPopupImmediate(table);
    openNewNavPopup(table, e, popupId);
  } else {
    openNewNavPopup(table, e, popupId);
  }
}

/**
 * Open a new navigation popup (helper for toggleNavPopup)
 * @param {Object} table - LithiumTable instance
 * @param {Event} e - Click event
 * @param {string} popupId - Popup identifier
 */
function openNewNavPopup(table, e, popupId) {
  // Dispatch event to close all manager popups
  document.dispatchEvent(new CustomEvent('close-all-popups'));

  const btn = e.currentTarget;
  let popup;

  if (popupId === 'template') {
    popup = buildTemplatePopup(table);
  } else if (popupId === 'grouping') {
    popup = buildGroupingPopup(table);
  } else {
    // Standard nav popups (width, layout)
    popup = buildStandardNavPopup(table, getPopupItems(table, popupId));
  }

  if (!popup) return;

  showNavPopup(table, btn, popup, popupId);
}

/**
 * Build standard navigation popup
 * @param {Object} table - LithiumTable instance
 * @param {Array} items - Popup items
 * @returns {HTMLElement} Popup element
 */
function buildStandardNavPopup(table, items) {
  if (!items.length) return null;

  const popup = document.createElement('div');
  popup.className = 'lithium-nav-popup';

  items.forEach((item) => {
    if (item.isSeparator) {
      const separator = document.createElement('div');
      separator.className = 'lithium-nav-popup-separator';
      popup.appendChild(separator);
      return;
    }

    const row = document.createElement('button');
    row.type = 'button';
    row.className = 'lithium-nav-popup-item';

    if (typeof item.checked === 'boolean') {
      row.innerHTML = `
        <span class="lithium-nav-popup-check">${item.checked ? '&#10003;' : ''}</span>
        <span class="lithium-nav-popup-label">${item.label}</span>
      `;
    } else {
      row.textContent = item.label;
    }

    row.addEventListener('click', () => {
      closeNavPopup(table);
      item.action();
    });
    popup.appendChild(row);
  });

  return popup;
}

/**
 * Show popup positioned near button
 * @param {Object} table - LithiumTable instance
 * @param {HTMLElement} btn - Button element
 * @param {HTMLElement} popup - Popup element
 * @param {string} popupId - Popup identifier
 */
function showNavPopup(table, btn, popup, popupId) {
  if (!btn) {
    log(Subsystems.TABLE, Status.ERROR, 'showNavPopup called with null button');
    return;
  }

  popup.style.position = 'fixed';
  // Footer-riseup positioning: bottom-right of popup 1px above top-right of button
  popup.style.bottom = 'auto';
  popup.style.top = 'auto';
  popup.style.left = 'auto';
  popup.style.right = 'auto';

  // Add popup-active class to button for toggle styling
  btn.classList.add('popup-active');

  // Check for Column Manager popup
  const hostPopup = btn?.closest?.('.col-manager-popup');
  if (hostPopup) {
    const hostZIndex = parseInt(window.getComputedStyle(hostPopup).zIndex, 10);
    if (Number.isFinite(hostZIndex)) {
      popup.style.zIndex = String(hostZIndex + 10);
    }
  } else {
    // Check for Crimson popup
    const crimsonPopup = btn?.closest?.('.crimson-citation-popup');
    if (crimsonPopup) {
      const crimsonZIndex = parseInt(window.getComputedStyle(crimsonPopup).zIndex, 10);
      if (Number.isFinite(crimsonZIndex)) {
        popup.style.zIndex = String(crimsonZIndex + 10);
      }
    }
  }

  document.body.appendChild(popup);

  // Process icons for popups that need them (e.g., grouping)
  processIcons(popup);

  const repositionPopup = () => {
    positionNavPopup(btn, popup);
  };

  repositionPopup();
  requestAnimationFrame(repositionPopup);

  // Trigger animation after positioning
  requestAnimationFrame(() => {
    requestAnimationFrame(() => {
      popup.classList.add('visible');
    });
  });

  table.activeNavPopup = popup;
  table.activeNavPopupId = popupId;
  table.activeNavPopupButton = btn;
  table.navPopupRepositionHandler = repositionPopup;

  table.navPopupCloseHandler = (evt) => {
    // Don't close if clicking on nav buttons that have popups (let toggle logic handle it)
    if (evt.target.closest('.lithium-nav-btn-has-popup')) {
      return;
    }
    if (!popup.contains(evt.target) && !btn.contains(evt.target)) {
      closeNavPopup(table);
    }
  };
  document.addEventListener('click', table.navPopupCloseHandler);

  // ESC key handler
  table.navPopupKeyHandler = (evt) => {
    if (evt.key === 'Escape') {
      closeNavPopup(table);
    }
  };
  document.addEventListener('keydown', table.navPopupKeyHandler);

  // Listen for close-all-popups from manager menus
  table.navPopupGlobalCloseHandler = () => {
    closeNavPopup(table);
  };
  document.addEventListener('close-all-popups', table.navPopupGlobalCloseHandler);

  window.addEventListener('resize', repositionPopup);
  document.addEventListener('scroll', repositionPopup, true);
}

/**
 * Position popup relative to button using footer-riseup style
 * Bottom-right corner of popup is 1px above top-right corner of button
 * @param {HTMLElement} btn - Button element
 * @param {HTMLElement} popup - Popup element
 */
function positionNavPopup(btn, popup) {
  if (!btn || !popup) return;

  const btnRect = btn.getBoundingClientRect();

  // Footer-riseup positioning: bottom-right of popup 1px above top-right of button
  popup.style.bottom = `${window.innerHeight - btnRect.top + 1}px`;
  popup.style.right = `${window.innerWidth - btnRect.right}px`;
  popup.style.top = 'auto';
  popup.style.left = 'auto';
}

/**
 * Close active navigation popup immediately (no animation)
 * Used when opening a new popup to avoid animation conflicts
 * @param {Object} table - LithiumTable instance
 */
function closeNavPopupImmediate(table) {
  // Remove popup-active class from button
  if (table.activeNavPopupButton) {
    table.activeNavPopupButton.classList.remove('popup-active');
  }
  if (table.activeNavPopup) {
    table.activeNavPopup.remove();
    table.activeNavPopup = null;
    table.activeNavPopupId = null;
    table.activeNavPopupButton = null;
  }
  if (table.navPopupCloseHandler) {
    document.removeEventListener('click', table.navPopupCloseHandler);
    table.navPopupCloseHandler = null;
  }
  if (table.navPopupKeyHandler) {
    document.removeEventListener('keydown', table.navPopupKeyHandler);
    table.navPopupKeyHandler = null;
  }
  if (table.navPopupGlobalCloseHandler) {
    document.removeEventListener('close-all-popups', table.navPopupGlobalCloseHandler);
    table.navPopupGlobalCloseHandler = null;
  }
  if (table.navPopupRepositionHandler) {
    window.removeEventListener('resize', table.navPopupRepositionHandler);
    document.removeEventListener('scroll', table.navPopupRepositionHandler, true);
    table.navPopupRepositionHandler = null;
  }
  table._navPopupTransitioning = false;
}

/**
 * Close active navigation popup with animation
 * @param {Object} table - LithiumTable instance
 * @param {Function} [callback] - Optional callback when animation completes
 */
function closeNavPopup(table, callback) {
  // If no popup is active, just call callback and return
  if (!table.activeNavPopup) {
    return;
  }

  // Prevent overlapping close animations, but allow if we're opening a new popup
  if (table._navPopupTransitioning && true) return;

  table._navPopupTransitioning = true;

  // Remove popup-active class from button
  if (table.activeNavPopupButton) {
    table.activeNavPopupButton.classList.remove('popup-active');
  }

  // Remove event listeners immediately to prevent conflicts
  if (table.navPopupCloseHandler) {
    document.removeEventListener('click', table.navPopupCloseHandler);
    table.navPopupCloseHandler = null;
  }
  if (table.navPopupKeyHandler) {
    document.removeEventListener('keydown', table.navPopupKeyHandler);
    table.navPopupKeyHandler = null;
  }
  if (table.navPopupGlobalCloseHandler) {
    document.removeEventListener('close-all-popups', table.navPopupGlobalCloseHandler);
    table.navPopupGlobalCloseHandler = null;
  }
  if (table.navPopupRepositionHandler) {
    window.removeEventListener('resize', table.navPopupRepositionHandler);
    document.removeEventListener('scroll', table.navPopupRepositionHandler, true);
    table.navPopupRepositionHandler = null;
  }



  // Start close animation
  table.activeNavPopup.classList.remove('visible');

  // Remove after animation completes
  const duration = 300; // Match transition duration
  setTimeout(() => {
    if (table.activeNavPopup && table.activeNavPopup.parentNode) {
      table.activeNavPopup.remove();
    }
    table.activeNavPopup = null;
    table.activeNavPopupId = null;
    table.activeNavPopupButton = null;
    table._navPopupTransitioning = false;
  }, duration);
}

/**
 * Close all transient popups
 * @param {Object} table - LithiumTable instance
 */
function closeTransientPopups(table) {
  closeNavPopup(table);
  if (typeof table.closeColumnChooser === 'function') {
    table.closeColumnChooser();
  }
}

/**
 * Get popup menu items
 * @param {Object} table - LithiumTable instance
 * @param {string} popupId - Popup identifier
 * @returns {Array} Menu items
 */
function getPopupItems(table, popupId) {
  switch (popupId) {
    case 'width':
      return [
        { label: 'Narrow', checked: table.tableWidthMode === 'narrow', action: () => table.setTableWidth?.('narrow') },
        { label: 'Compact', checked: table.tableWidthMode === 'compact', action: () => table.setTableWidth?.('compact') },
        { label: 'Normal', checked: table.tableWidthMode === 'normal', action: () => table.setTableWidth?.('normal') },
        { label: 'Wide', checked: table.tableWidthMode === 'wide', action: () => table.setTableWidth?.('wide') },
        { label: 'Auto', checked: table.tableWidthMode === 'auto', action: () => table.setTableWidth?.('auto') },
      ];
    case 'layout':
      return [
        { label: 'Fit Columns', checked: table.tableLayoutMode === 'fitColumns', action: () => table.setTableLayout?.('fitColumns') },
        { label: 'Fit Data', checked: table.tableLayoutMode === 'fitData', action: () => table.setTableLayout?.('fitData') },
        { label: 'Fit Fill', checked: table.tableLayoutMode === 'fitDataFill', action: () => table.setTableLayout?.('fitDataFill') },
        { label: 'Fit Stretch', checked: table.tableLayoutMode === 'fitDataStretch', action: () => table.setTableLayout?.('fitDataStretch') },
        { label: 'Fit Table', checked: table.tableLayoutMode === 'fitDataTable', action: () => table.setTableLayout?.('fitDataTable') },
      ];
    default:
      return [];
  }
}

/**
 * Create a popup separator element
 * @returns {HTMLElement} Separator element
 */
function createPopupSeparator() {
  const separator = document.createElement('div');
  separator.className = 'lithium-nav-popup-separator';
  return separator;
}

/**
 * Table Settings Module
 *
 * Table width and layout mode management.
 *
 * @module tables/settings/table-settings
 */


/**
 * Set table width mode
 * @param {Object} table - LithiumTable instance
 * @param {string} mode - Width mode (narrow, compact, normal, wide, auto)
 */
function setTableWidth(table, mode) {
  table.tableWidthMode = mode;
  saveTableWidth(table, mode);

  if (table.panel) {
    applyPanelWidth(table, mode);
  }

  if (typeof table.onSetTableWidth === 'function') {
    table.onSetTableWidth(mode);
  }

  log(Subsystems.TABLE, Status.INFO, `Table width set to: ${mode}`);
}

/**
 * Save table width mode to localStorage
 * @param {Object} table - LithiumTable instance
 * @param {string} mode - Width mode
 */
function saveTableWidth(table, mode) {
  try {
    localStorage.setItem(`${table.storageKey}_width_mode`, mode);
  } catch (_e) {
    console.error(`[LithiumTable] FAILED to save width mode: ${_e?.message}`, { storageKey: table.storageKey, mode });
  }
}

/**
 * Restore table width mode from localStorage
 * @param {Object} table - LithiumTable instance
 * @returns {string|null} Width mode or null
 */
function restoreTableWidth(table) {
  try {
    const stored = localStorage.getItem(`${table.storageKey}_width_mode`);
    if (stored && ['narrow', 'compact', 'normal', 'wide', 'auto'].includes(stored)) {
      return stored;
    }
  } catch (_e) {
    // Ignore storage errors
  }
  return null;
}

/**
 * Apply width mode to panel
 * Uses CSS custom properties for width values
 * @param {Object} table - LithiumTable instance
 * @param {string} mode - Width mode
 */
function applyPanelWidth(table, mode) {
  if (!table.panel) return;

  if (mode === 'auto') {
    table.panel.style.width = 'auto';
    table.panel.style.flex = '1';
    return;
  }

  // Get width from CSS variable
  const widthVar = `--table-width-${mode}`;
  const computedStyle = getComputedStyle(document.documentElement);
  const width = computedStyle.getPropertyValue(widthVar).trim();

  if (width) {
    table.panel.style.width = width;
    table.panel.style.flex = '0 0 auto';
  } else {
    // Fallback values if CSS vars not available
    const fallbacks = {
      narrow: '160px',
      compact: '314px',
      normal: '468px',
      wide: '622px',
    };
    table.panel.style.width = fallbacks[mode] || fallbacks.compact;
    table.panel.style.flex = '0 0 auto';
  }
}

/**
 * Set table layout mode
 * @param {Object} table - LithiumTable instance
 * @param {string} mode - Layout mode (fitColumns, fitData, fitDataFill, fitDataStretch, fitDataTable)
 */
async function setTableLayout(table, mode) {
  if (!table.table) return;

  table.tableLayoutMode = mode;
  saveLayoutMode(table, mode);

  const currentData = table.table.getData();
  const selectedId = table.getSelectedRowId?.();

  table.table.destroy();
  table.table = null;

  await table.initTable?.();
  table.table.setData(currentData);
  table.table.modules.select?.setSelectionMode?.(1);

  await new Promise((resolve) => {
    requestAnimationFrame(() => {
      table.autoSelectRow?.(selectedId);
      // Button state updates are handled internally by autoSelectRow
      resolve();
    });
  });

  log(Subsystems.TABLE, Status.INFO, `Table layout set to: ${mode}`);
}

/**
 * Save layout mode to localStorage
 * @param {Object} table - LithiumTable instance
 * @param {string} mode - Layout mode
 */
function saveLayoutMode(table, mode) {
  try {
    localStorage.setItem(`${table.storageKey}_layout_mode`, mode);
  } catch (_e) {
    // Silently handle
  }
}

/**
 * Restore layout mode from localStorage
 * @param {Object} table - LithiumTable instance
 * @returns {string|null} Layout mode or null
 */
function restoreLayoutMode(table) {
  try {
    const stored = localStorage.getItem(`${table.storageKey}_layout_mode`);
    if (stored && ['fitColumns', 'fitData', 'fitDataFill', 'fitDataStretch', 'fitDataTable'].includes(stored)) {
      return stored;
    }
  } catch (_e) {
    // Ignore storage errors
  }
  return null;
}

/**
 * Filter Editor Module
 *
 * Header filter editor creation and management.
 *
 * @module tables/filters/filter-editor
 */

/**
 * Create a header filter editor function that captures cssPrefix in closure.
 * Returns a function suitable for use as Tabulator's headerFilter option.
 * Tabulator calls this with `this.table.modules.edit` as context, so we
 * must not depend on `this` context - capture everything in the closure.
 * @param {string} cssPrefix - CSS class prefix
 * @returns {Function} Filter editor function
 */
function createFilterEditorFunction(cssPrefix) {
  return function(cell, onRendered, success, cancel, editorParams) {
    const input = document.createElement('input');
    input.type = 'text';
    input.placeholder = editorParams?.placeholder || 'filter...';
    input.className = `${cssPrefix}-header-filter-input`;

    let clearBtn = null;

    const updateClearBtn = () => {
      if (clearBtn) {
        clearBtn.style.display = input.value ? 'flex' : 'none';
      }
    };

    input.addEventListener('input', () => {
      updateClearBtn();
      success(input.value);
    });

    input.addEventListener('keydown', (e) => {
      if (e.key === 'Escape') {
        input.value = '';
        updateClearBtn();
        success('');
        e.stopPropagation();
      }
    });

    onRendered(() => {
      const parent = input.parentElement;
      if (!parent) return;
      parent.style.position = 'relative';

      clearBtn = document.createElement('span');
      clearBtn.className = `${cssPrefix}-header-filter-clear`;
      clearBtn.innerHTML = '&times;';
      clearBtn.title = 'Clear filter';
      clearBtn.style.display = input.value ? 'flex' : 'none';

      clearBtn.addEventListener('click', (e) => {
        e.stopPropagation();
        input.value = '';
        clearBtn.style.display = 'none';
        success('');
      });

      parent.appendChild(clearBtn);
    });

    return input;
  };
}

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
function updateSelectorCell(table, row, isSelected) {
  try {
    const cell = row.getCell('_selector');
    if (!cell) return;
    const el = cell.getElement();
    if (!el) return;

    if (isSelected) {
      const pkField = table.primaryKeyField || 'id';
      // const isEditing = table.isEditing && table.editingRowId === row.getData()[pkField];
      const isEditing = table.isEditing;
      el.innerHTML = isEditing
        ? `<div class="lithium-table-selector-icon ${table.cssPrefix}-selector-indicator ${table.cssPrefix}-selector-edit"><fa fa-pencil></fa></div>`
        : `<div class="lithium-table-selector-icon ${table.cssPrefix}-selector-indicator ${table.cssPrefix}-selector-active"><fa fa-caret-large-right></fa></div>`;
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
function updateVisibleColumnClasses(table) {
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
function getVisibleColumnBoundaries(table) {
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
function applyVisibleColumnClassesToRow(table, row, firstField, lastField) {
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
function applyVisibleColumnClassesToRowElement(rowEl, firstField, lastField) {
  if (!rowEl) return;

  const cells = rowEl.querySelectorAll('.tabulator-cell');
  cells.forEach((cell) => {
    const field = cell.getAttribute('tabulator-field');
    if (!field) return;

    cell.classList.toggle('first-visible-col', field === firstField);
    cell.classList.toggle('last-visible-col', field === lastField);
  });
}

/**
 * Loading Indicator Module
 *
 * Table loading state visualization.
 *
 * @module tables/visual/loading-indicator
 */

/**
 * Show loading indicator
 * @param {Object} table - LithiumTable instance
 */
function showLoading(table) {
  if (!table.container) return;

  hideLoading(table);

  const loader = document.createElement('div');
  loader.className = `lithium-table-loader ${table.cssPrefix}-table-loader`;
  loader.innerHTML = '<div class="spinner-fancy spinner-fancy-md"></div>';
  table.container.appendChild(loader);
  table.loaderElement = loader;
}

/**
 * Hide loading indicator
 * @param {Object} table - LithiumTable instance
 */
function hideLoading(table) {
  if (table.loaderElement) {
    table.loaderElement.remove();
    table.loaderElement = null;
  }
}

/**
 * LithiumTable UI Module (Refactored)
 *
 * Navigator, popups, column chooser, and visual updates.
 * Delegates to feature-specific modules.
 *
 * @module tables/lithium-table-ui
 */


// ── Mixin for UI Operations ─────────────────────────────────────────────────

const LithiumTableUIMixin = {
  // ── Navigator (delegated to navigator-builder.js) ─────────────────────────

  buildNavigator() {
    buildNavigator(this);
  },

  updateEditButtonState() {
    if (this._inSelectionTransition) return;
    updateEditButtonState(this);
  },

  updateDuplicateButtonState() {
    if (this._inSelectionTransition) {
      return;
    }
    updateDuplicateButtonState(this);
  },

  updateMoveButtonState() {
    if (this._inSelectionTransition) {
      return;
    }
    updateMoveButtonState(this);
  },

  // ── Popup Menus (delegated to popups/) ────────────────────────────────────

  toggleNavPopup(e, popupId) {
    // Unified popup handling for all nav popup types
    toggleNavPopup(this, e, popupId);
  },

  // Helper function for opening template popup
  openTemplatePopup(table, e) {
    // Dispatch event to close all manager popups
    document.dispatchEvent(new CustomEvent('close-all-popups'));

    const btn = e.currentTarget;
    const popup = buildTemplatePopup(table);

    if (!popup) return;

    showNavPopup(table, btn, popup, 'template');
  },

  buildTemplatePopup() {
    return buildTemplatePopup(this);
  },

  createPopupSeparator() {
    return createPopupSeparator();
  },

  getTemplateStorageKey() {
    return `${this.storageKey}_templates`;
  },

  getSavedTemplates() {
    return getSavedTemplates(this);
  },

  saveStoredTemplates(templates) {
    saveStoredTemplates(this, templates);
  },

  getTemplateName(template) {
    return getTemplateName(this, template);
  },

  getSelectedTemplateName(templates = []) {
    if (this.templateMenuSelectedName && templates.some(
      (template) => getTemplateName(this, template) === this.templateMenuSelectedName,
    )) {
      return this.templateMenuSelectedName;
    }

    if (this.activeTemplateName && templates.some(
      (template) => getTemplateName(this, template) === this.activeTemplateName,
    )) {
      return this.activeTemplateName;
    }

    return null;
  },

  logTemplateMenuSelection(action, detail = '') {
    const suffix = detail ? `: ${detail}` : '';
    log(Subsystems.TABLE, Status.INFO, `[LithiumTable] Template menu ${action}${suffix}`);
  },

  closeNavPopup() {
    closeNavPopup(this);
  },

  closeTransientPopups() {
    closeTransientPopups(this);
  },





  // ── Table Settings (delegated to settings/table-settings.js) ──────────────

  setTableWidth(mode) {
    setTableWidth(this, mode);
  },

  saveTableWidth(_mode) {
    // Delegated
  },

  restoreTableWidth() {
    return restoreTableWidth(this);
  },

  async setTableLayout(mode) {
    await setTableLayout(this, mode);
  },

  saveLayoutMode(_mode) {
    // Delegated
  },

  restoreLayoutMode() {
    return restoreLayoutMode(this);
  },

  // ── Column Manager (delegated to column-manager/cm-integration.js) ────────

  async toggleColumnManager(e, column) {
    return toggleColumnManager(this, e, column);
  },

  closeAllColumnManagers(parentColumnManager = null) {
    return closeAllColumnManagers(parentColumnManager);
  },

  getManagerId() {
    return getManagerId(this);
  },

  closeColumnManager() {
    return closeColumnManager(this);
  },

  onColumnManagerChange(field, property, value) {
    return onColumnManagerChange(this, field, property, value);
  },

  // ── Filter Editor (delegated to filters/filter-editor.js) ─────────────────

  createFilterEditorFunction() {
    return createFilterEditorFunction(this.cssPrefix);
  },

  createFilterEditor(cell, onRendered, success, cancel, editorParams) {
    return this.createFilterEditorFunction()(cell, onRendered, success, cancel, editorParams);
  },

  // ── Visual Updates (delegated to visual/visual-updates.js) ────────────────

  updateSelectorCell(row, isSelected) {
    updateSelectorCell(this, row, isSelected);
  },

  updateVisibleColumnClasses() {
    updateVisibleColumnClasses(this);
  },

  getVisibleColumnBoundaries() {
    return getVisibleColumnBoundaries(this);
  },

  applyVisibleColumnClassesToRow(row, firstField, lastField) {
    applyVisibleColumnClassesToRow(this, row, firstField, lastField);
  },

  applyVisibleColumnClassesToRowElement(rowEl, firstField, lastField) {
    applyVisibleColumnClassesToRowElement(rowEl, firstField, lastField);
  },

  // ── Loading Indicator (delegated to visual/loading-indicator.js) ──────────

  showLoading() {
    showLoading(this);
  },

  hideLoading() {
    hideLoading(this);
  },

  // ── Event Handlers (delegated to events/event-handlers.js) ────────────────

  async handleRefresh() {
    return handleRefresh(this);
  },

  handleFilter() {
    return handleFilter(this);
  },

  toggleHeaderFilters(visible) {
    return toggleHeaderFilters(this, visible);
  },

  clearColumnInlineHeights() {
    return clearColumnInlineHeights(this);
  },

  expandAll() {
    return expandAll(this);
  },

  collapseAll() {
    return collapseAll(this);
  },

  // ── Persistence (delegated to persistence/persistence.js) ─────────────────

  saveSelectedRowId(rowId) {
    saveSelectedRowId(this, rowId);
  },

  restoreSelectedRowId() {
    return restoreSelectedRowId(this);
  },

  clearSavedRowSelection() {
    clearSavedRowSelection(this);
  },

  saveFiltersVisible(visible) {
    saveFiltersVisible(this, visible);
  },

  restoreFiltersVisible() {
    return restoreFiltersVisible(this);
  },

  async setupPersistence() {
    const restoredWidth = restoreTableWidth(this);

    if (restoredWidth) {
      this.tableWidthMode = restoredWidth;
      if (this.panel) {
        this.applyPanelWidth(restoredWidth);
      } else if (typeof this.onSetTableWidth === 'function') {
        this.onSetTableWidth(restoredWidth);
      }
    } else {
      // No saved state, use current default mode
      if (this.panel) {
        this.applyPanelWidth(this.tableWidthMode);
      } else if (typeof this.onSetTableWidth === 'function') {
        this.onSetTableWidth(this.tableWidthMode);
      }
    }

    const savedFiltersVisible = restoreFiltersVisible(this);
    if (savedFiltersVisible && this.table) {
      this.filtersVisible = true;
      this.container.classList.add('lithium-filters-visible');
      const filterBtn = this.navigatorContainer?.querySelector(`#${this.cssPrefix}-nav-filter`);
      if (filterBtn) {
        filterBtn.classList.add('lithium-nav-btn-active');
      }
      this.toggleHeaderFilters(true);
    }

    const templates = getSavedTemplates(this);
    const defaultTemplate = templates.find((template) => getTemplateName(this, template) === 'Default');
    if (defaultTemplate) {
      await this.loadTemplate?.(defaultTemplate);
    }
  },

  // ── Template System ───────────────────────────────────────────────────────

  generateTemplateJSON() {
    return generateTemplateJSON(this);
  },

  async copyTemplateToClipboard(templateOverride = null) {
    try {
      const template = templateOverride || this.generateTemplateJSON();
      if (!template) {
        toast.error('Failed to generate template', { duration: 3000 });
        return;
      }

      const jsonString = JSON.stringify(template, null, 2);
      await navigator.clipboard.writeText(jsonString);

      const templateName = getTemplateName(this, template);

      toast.success('Template copied to clipboard', {
        description: templateOverride ? templateName : `${Object.keys(template.columns || {}).length} columns exported`,
        duration: 3000,
      });

      log(Subsystems.TABLE, Status.INFO,
        `[LithiumTable] Template copied to clipboard for ${this.storageKey}`);
    } catch (error) {
      toast.error('Failed to copy template', {
        description: error.message,
        duration: 5000,
      });
      log(Subsystems.TABLE, Status.ERROR,
        `[LithiumTable] Failed to copy template: ${error.message}`);
    }
  },

  saveTemplate() {
    const template = this.generateTemplateJSON();
    if (!template) return;

    const defaultName = `${template._templateMeta?.name || 'Template'} - ${new Date().toLocaleDateString()}`;
    const name = window.prompt('Enter template name:', defaultName);
    if (!name) return;

    template._templateMeta = template._templateMeta || {};
    template._templateMeta.name = name;

    const templates = getSavedTemplates(this);

    const existingIndex = templates.findIndex((item) => getTemplateName(this, item) === name);

    if (existingIndex >= 0) {
      templates[existingIndex] = template;
    } else {
      templates.push(template);
    }

    try {
      saveStoredTemplates(this, templates);
      this.templateMenuSelectedName = name;
      toast.success('Template saved', { description: name, duration: 3000 });
      log(Subsystems.TABLE, Status.INFO,
        `[LithiumTable] Template saved: ${name}`);
      return template;
    } catch (_e) {
      toast.error('Failed to save template', {
        description: 'Storage quota may be exceeded',
        duration: 5000,
      });
    }
  },

  deleteTemplate(index) {
    const templates = getSavedTemplates(this);

    if (index >= 0 && index < templates.length) {
      const name = getTemplateName(this, templates[index]);
      templates.splice(index, 1);
      try {
        saveStoredTemplates(this, templates);
        if (this.templateMenuSelectedName === name) {
          this.templateMenuSelectedName = null;
        }
        if (this.activeTemplateName === name) {
          this.activeTemplateName = null;
        }
        toast.success('Template deleted', { description: name, duration: 3000 });
        return true;
      } catch (_e) {
        console.error(`[LithiumTable] FAILED to delete template: ${_e?.message}`);
      }
    }
    return false;
  },

  async clearTemplate() {
    if (!this.table) return;

    const confirmed = window.confirm(
      'Reload the default template from the database?\n\nThis will reset the current table view, but will not delete any saved templates.'
    );
    if (!confirmed) return;

    try {
      localStorage.removeItem(`${this.storageKey}_width_mode`);
      localStorage.removeItem(`${this.storageKey}_layout_mode`);
      this.activeTemplateName = null;
      this.templateMenuSelectedName = null;

      const currentData = this.table.getData();
      const selectedId = this.getSelectedRowId?.();

      await this.loadConfiguration?.();

      this.table.destroy();
      this.table = null;

      await this.initTable?.();
      this.table.setData(currentData);
      this.table.modules.select?.setSelectionMode?.(1);

      await new Promise((resolve) => {
        requestAnimationFrame(() => {
          this.autoSelectRow?.(selectedId);
          // Button state updates are handled internally by autoSelectRow
          resolve();
        });
      });

      log(Subsystems.TABLE, Status.INFO, '[LithiumTable] Default template reloaded from database');
    } catch (error) {
      toast.error('Failed to load default template', {
        description: error.message,
        duration: 5000,
      });
      log(Subsystems.TABLE, Status.ERROR,
        `[LithiumTable] Failed to reload default template: ${error.message}`);
    }
  },

  async loadTemplate(template) {
    if (!this.table || !template) return;

    try {
      const templateName = template._templateMeta?.name || template.name || 'Template';

      // Ensure we have a base tableDef (from Lookup 59 or auto-discovered)
      // before applying the template. This ensures all columns are available
      // in the Column Manager, including new columns that may have been
      // added since the template was saved.
      if (!this.tableDef && this.tablePath) {
        log(Subsystems.TABLE, Status.INFO, `[${this.cssPrefix}] Reloading tableDef before applying template...`);
        await this.loadConfiguration?.();
      }

      const widthMode = template._templateMeta?.widthMode || template.layout?.widthMode;
      if (widthMode && this.setTableWidth) {
        try {
          this.setTableWidth(widthMode);
        } catch (e) {
          log(Subsystems.TABLE, Status.WARN, `[LithiumTable] Failed to apply width: ${e.message}`);
        }
      }

      if (template.layout && this.setTableLayout) {
        try {
          await this.setTableLayout(template.layout);
        } catch (e) {
          log(Subsystems.TABLE, Status.WARN, `[LithiumTable] Failed to apply layout: ${e.message}`);
        }
      }

      if (template.columns && typeof template.columns === 'object') {
        this.applyTemplateColumns(template.columns);
      }

      if (template.initialSort && Array.isArray(template.initialSort) && typeof this.table.setSort === 'function') {
        try {
          this.table.setSort(template.initialSort.map((s) => ({
            column: s.column,
            dir: s.dir,
          })));
        } catch (e) {
          log(Subsystems.TABLE, Status.WARN, `[LithiumTable] Failed to apply sort: ${e.message}`);
        }
      }

      // Restore filter visibility state
      if (template._filtersVisible === true && !this.filtersVisible) {
        this.toggleHeaderFilters?.(true);
      } else if (template._filtersVisible === false && this.filtersVisible) {
        this.toggleHeaderFilters?.(false);
      }

      // Restore filter values
      if (template._filterValues && typeof template._filterValues === 'object') {
        try {
          for (const [field, value] of Object.entries(template._filterValues)) {
            if (value != null && value !== '') {
              this.table.setHeaderFilterValue?.(field, value);
            }
          }
        } catch (e) {
          log(Subsystems.TABLE, Status.WARN, `[LithiumTable] Failed to apply filter values: ${e.message}`);
        }
      }

      if (typeof this.table.redraw === 'function') {
        this.table.redraw(true);
      }

      this.activeTemplateName = templateName;
      this.templateMenuSelectedName = templateName;

      log(Subsystems.TABLE, Status.INFO,
        `[LithiumTable] Template loaded: ${templateName}`);
      return true;
    } catch (error) {
      toast.error('Failed to load template', {
        description: error.message,
        duration: 5000,
      });
      log(Subsystems.TABLE, Status.ERROR,
        `[LithiumTable] Failed to load template: ${error.message}`);
      return false;
    }
  },

  applyTemplateColumns(templateColumns) {
    if (!this.table || !this.coltypes) return;

    const columns = buildColumnDefinitionsFromTemplateState({
      table: this.table,
      templateColumns,
      tableDefColumns: this.tableDef?.columns || {},
      coltypes: this.coltypes,
      filterEditor: this.createFilterEditorFunction(),
      filtersVisible: this.filtersVisible,
      primaryKeyField: this.primaryKeyField,
      readonly: this.readonly,
      selectorColumn: this.buildSelectorColumn?.(),
    });

    this.applyResolvedColumnDefinitions(columns);
  },

  applyResolvedColumnDefinitions(columns) {
    if (!this.table || !Array.isArray(columns) || columns.length === 0) return;

    const currentData = this.table.getData();
    const selectedId = this.getSelectedRowId?.();
    const currentSorters = this.table.getSorters?.() || [];
    const currentHeaderFilters = this.table.getHeaderFilters?.() || [];

    this.table.setColumns(columns);
    this.table.setData(currentData);

    // Re-enforce selectableRows: 1 after setColumns() - Tabulator may lose this setting
    this.table.modules.select?.setSelectionMode?.(1);

    requestAnimationFrame(() => {
      if (currentSorters.length > 0) {
        this.table.setSort(currentSorters.map((sorter) => ({
          column: sorter.field || sorter.column,
          dir: sorter.dir,
        })));
      }

      if (this.filtersVisible && currentHeaderFilters.length > 0) {
        currentHeaderFilters.forEach((filter) => {
          if (filter?.field != null) {
            this.table.setHeaderFilterValue(filter.field, filter.value);
          }
        });
      }

      this.autoSelectRow?.(selectedId);
      this.updateMoveButtonState?.();
      this.updateDuplicateButtonState?.();
      this.updateVisibleColumnClasses();
      this.clearColumnInlineHeights();
      this.table.redraw(true);
    });
  },

  resetTemplate() {
    return this.clearTemplate();
  },
};

/**
 * LithiumTable Main Module
 *
 * Combines all LithiumTable modules into a single reusable class.
 * This is the main export that managers should use.
 *
 * Usage:
 *   import { LithiumTable } from './core/lithium-table-main.js';
 *
 *   const table = new LithiumTable({
 *     container: document.getElementById('table-container'),
 *     navigatorContainer: document.getElementById('nav-container'),
 *     tablePath: 'queries/query-manager',
 *     queryRef: 25,
 *     app: this.app,
 *     cssPrefix: 'queries',
 *     onRowSelected: (rowData) => { ... },
 *   });
 *   await table.init();
 *
 * @module core/lithium-table-main
 */


// ── Combine Mixins into Final Class ─────────────────────────────────────────

/**
 * LithiumTable - Reusable Tabulator + Navigator Component
 *
 * A self-contained table component combining:
 * - Tabulator data grid with JSON-driven column configuration
 * - Navigator control bar (Control, Move, Manage, Search blocks)
 * - Edit mode with dirty state tracking
 * - Column chooser popup
 * - Template system for saving/loading column configurations
 * - Header filters with clear buttons
 */
class LithiumTable extends LithiumTableBase {
  constructor(options) {
    super(options);
  }
}

// Apply mixins to the class
Object.assign(LithiumTable.prototype, LithiumTableOpsMixin, LithiumTableUIMixin);

export { LithiumTable as L };
