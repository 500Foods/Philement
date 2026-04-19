/**
 * Column Type (coltype) Loader
 *
 * Loads and caches coltype definitions from Lookup 59 or provided data.
 *
 * @module tables/resolution/coltype-loader
 */

import { log, Subsystems, Status } from '../../core/log.js';
import { getTabulatorSchemas, fetchLookups } from '../../shared/lookups.js';

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
 * Get cached coltypes (synchronous).
 * Returns null if not yet loaded.
 *
 * @returns {Object|null}
 */
export function getColtypes() {
  return _coltypesCache;
}

/**
 * Clear the coltypes cache.
 * Useful for testing or when coltype definitions change at runtime.
 */
export function clearColtypesCache() {
  _coltypesCache = null;
}
