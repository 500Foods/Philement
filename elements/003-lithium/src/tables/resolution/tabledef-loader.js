/**
 * Table Definition Loader
 *
 * Loads and caches table definitions from Lookup 59 or provided data.
 *
 * @module tables/resolution/tabledef-loader
 */

import { log, Subsystems, Status } from '../../core/log.js';
import { getTabulatorSchemas, fetchLookups } from '../../shared/lookups.js';
import { eventBus, Events } from '../../core/event-bus.js';
import { validateTableDef } from './validator.js';

/** @type {Map<string, Object>} Cached table definitions keyed by path */
export const _tableDefCache = new Map();

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
export function createAutoDiscoverTableDef(tablePath) {
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
export async function loadTableDef(tablePath, providedData, lookupKeyIdx) {
  // Normalise path: strip trailing .json if present
  const normalised = tablePath.replace(/\.json$/, '');

  // ── Priority 1: Caller-provided data (e.g., Vite ES module import) ──
  if (providedData) {
    const validated = validateTableDef(providedData, 'import');
    if (!validated.valid) {
      validated.errors.forEach(err => log(Subsystems.MANAGER, Status.WARN, err));
    }
    _tableDefCache.set(normalised, providedData);
    log(Subsystems.MANAGER, Status.INFO,
      `[LithiumTable] Loaded tabledef "${providedData.title || normalised}" ` +
      `(${Object.keys(providedData.columns || {}).length} columns, from import)`);
    return providedData;
  }

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
export function clearTableDefCache() {
  _tableDefCache.clear();
}

/**
 * Clear cached table definitions that come from Lookup 59 (Tabulator Schemas).
 * This should be called when Lookup 59 data is refreshed to ensure
 * table definitions are re-fetched from the updated lookup data.
 */
export function clearLookup59Cache() {
  const lookup59Paths = [
    'queries/query-manager',
    'scripts/script-manager',
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
