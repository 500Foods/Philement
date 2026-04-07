/**
 * Lookups - Server-provided reference data with localStorage caching
 *
 * Caches and provides access to lookup data from the server.
 * Supports both "open" lookups (available before login) and authenticated lookups.
 * Lookups are cached in localStorage and loaded from there initially while
 * waiting for fresh copies from the server.
 *
 * QueryRefs used:
 * - 001: System Information
 * - 030: Lookup Names
 * - 053: Themes
 * - 054: Icons
 * - 060: Tabulator Schemas
 */

import { eventBus, Events } from '../core/event-bus.js';
import { getConfigValue } from '../core/config.js';
import { log, logGroup, Subsystems, Status, logHttpRequest, logHttpResponse } from '../core/log.js';

// In-memory cache for lookups
let cache = null;
let fetchPromise = null;

// Simple logger that works before any manager is loaded
const logger = {
  info: (message) => {
    log(Subsystems.LOOKUPS, Status.INFO, message);
  },
  warn: (message) => {
    log(Subsystems.LOOKUPS, Status.WARN, message);
  },
  error: (message) => {
    log(Subsystems.LOOKUPS, Status.ERROR, message);
  },
};

// Friendly metadata for each lookup category
// Maps category key → { queryRef, label, countLabel }
const LOOKUP_META = {
  system_info:       { queryRef: '001', label: 'System Info',       countLabel: 'elements' },
  lookup_names:      { queryRef: '030', label: 'Lookup Names',      countLabel: 'lookups' },
  themes:            { queryRef: '053', label: 'Themes',            countLabel: 'themes' },
  icons:             { queryRef: '054', label: 'Icons',             countLabel: 'icons' },
  tabulator_schemas: { queryRef: '060', label: 'Tabulator Schemas', countLabel: 'schemas' },
  macros:            { queryRef: '046', label: 'Macro Expansion', countLabel: 'macros' },
};

/**
 * Emit lookups-loaded events via the event bus.
 * For each category a rich logGroup entry is written directly, and the
 * per-category event is dispatched silently (no duplicate EventBus log line).
 * The general LOOKUPS_LOADED event is still emitted normally so any
 * subscribers (e.g. future analytics) can act on it.
 *
 * @param {string} source - 'cache', 'server', or 'cache_fallback'
 * @param {string[]} categoryKeys - Names of loaded categories
 * @param {Object} categoryData - Map of category → data (array or object)
 */
function emitLookupsLoaded(source, categoryKeys, categoryData) {
  // General event: only emitted when data is new or refreshed (server / cache_fallback),
  // NOT when restoring from localStorage cache — that's not a state change for subscribers.
  if (source !== 'cache') {
    eventBus.emit(Events.LOOKUPS_LOADED, { source, lookups: categoryKeys });
  }

  // Per-category: rich logGroup entry + silent event dispatch
  categoryKeys.forEach(category => {
    const eventName = `lookups:${category}:loaded`;
    const data = categoryData[category];
    const count = Array.isArray(data) ? data.length : Object.keys(data || {}).length;
    const meta = LOOKUP_META[category];
    const label = meta ? meta.label : category;
    const queryRef = meta ? meta.queryRef : '???';
    const countLabel = meta ? meta.countLabel : 'items';

    // Compute byte size of the serialised data
    let sizeStr = '';
    try {
      const bytes = new TextEncoder().encode(JSON.stringify(data)).length;
      sizeStr = bytes.toLocaleString() + ' bytes';
    } catch (_) {
      sizeStr = 'unknown';
    }

    // Rich grouped log entry using [Lookups] bracket notation — consistent with
    // event bus auto-log style so the log viewer renders it in the same column.
    logGroup('[Lookups]', Status.INFO,
      `Loaded lookup ${queryRef}: ${label}`,
      [
        `${countLabel.charAt(0).toUpperCase() + countLabel.slice(1)}: ${count}`,
        `Source: ${source}`,
        `Size: ${sizeStr}`,
      ]
    );

    // Silent event — callers (e.g. login.js) still receive it but no duplicate log line
    eventBus.emitSilent(eventName, { category, count });
  });
}

// Lookup categories and their QueryRefs
const LOOKUP_QUERYREFS = {
  system_info: 1,      // QueryRef 001 - System Information
  themes: 53,          // QueryRef 053 - Themes
  icons: 54,           // QueryRef 054 - Icons
  lookup_names: 30,    // QueryRef 030 - Names of all lookups
  managers: 1,         // Derived from system_info
  tabulator_schemas: 60, // QueryRef 060 - Tabulator Schemas
};

// Categories that are "open" (available before login)
const OPEN_CATEGORIES = ['system_info', 'themes', 'icons', 'managers', 'tabulator_schemas'];

// localStorage key for caching lookups
const STORAGE_KEY = 'lithium_lookups';
const STORAGE_TIMESTAMP_KEY = 'lithium_lookups_timestamp';
const CACHE_TTL_MS = 24 * 60 * 60 * 1000; // 24 hours

/**
 * Load lookups from localStorage cache
 * @returns {Object|null} Cached lookup data or null
 */
function loadFromLocalStorage() {
  try {
    const stored = localStorage.getItem(STORAGE_KEY);
    const timestamp = localStorage.getItem(STORAGE_TIMESTAMP_KEY);

    if (!stored || !timestamp) {
      return null;
    }

    // Check if cache is still valid
    const age = Date.now() - parseInt(timestamp, 10);
    if (age > CACHE_TTL_MS) {
      logger.info('Cache expired, will fetch fresh data');
      return null;
    }

    const data = JSON.parse(stored);
    return data;
  } catch (error) {
    logger.warn(`Failed to load from localStorage: ${error.message}`);
    return null;
  }
}

/**
 * Save lookups to localStorage cache
 * @param {Object} data - Lookup data to cache
 */
function saveToLocalStorage(data) {
  try {
    localStorage.setItem(STORAGE_KEY, JSON.stringify(data));
    localStorage.setItem(STORAGE_TIMESTAMP_KEY, String(Date.now()));
    logger.info('Saved to localStorage cache');
  } catch (error) {
    logger.warn(`Failed to save to localStorage: ${error.message}`);
  }
}

/**
 * Clear localStorage cache
 */
function clearLocalStorage() {
  try {
    localStorage.removeItem(STORAGE_KEY);
    localStorage.removeItem(STORAGE_TIMESTAMP_KEY);
    logger.info('Cleared localStorage cache');
  } catch (error) {
    logger.warn(`Failed to clear localStorage: ${error.message}`);
  }
}

/**
 * Fetch multiple lookups from the conduit endpoint in one batch request
 * @param {number[]} queryRefs - Array of QueryRef numbers to fetch
 * @param {string} requestId - Identifier for logging this request
 * @returns {Promise<Object>} Combined lookup data
 */
async function fetchBatchQueries(queryRefs, requestId) {
  const serverUrl = getConfigValue('server.url', 'http://localhost:8080');
  const apiPrefix = getConfigValue('server.api_prefix', '/api');
  const database = getConfigValue('auth.default_database', 'Lithium');
  const startTime = performance.now();

  // Get API key for QueryRef 001
  const apiKey = getConfigValue('auth.api_key', '');

  // Build queries array for batch request
  const queries = queryRefs.map(ref => {
    // QueryRef 001 requires APIKEY parameter (wrapped in STRING datatype)
    if (ref === 1) {
      return {
        query_ref: ref,
        params: {
          STRING: {
            APIKEY: apiKey
          }
        }
      };
    }
    // Other queries don't require parameters
    return {
      query_ref: ref,
      params: {}
    };
  });

  // Log the request using RESTAPI-style logging
  const requestPath = `conduit/queries [${queryRefs.join(', ')}]`;
  const requestNum = logHttpRequest('POST', requestPath);

  const response = await fetch(`${serverUrl}${apiPrefix}/conduit/queries`, {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
      'Accept': 'application/json',
    },
    body: JSON.stringify({ database, queries }),
  });

  const duration = Math.round(performance.now() - startTime);

  if (!response.ok) {
    logHttpResponse(requestNum, 'POST', requestPath, response.status, null, duration);
    const error = new Error(`HTTP ${response.status}`);
    error.status = response.status;
    throw error;
  }

  const data = await response.json();
  const bytes = JSON.stringify(data).length;

  // Log the response with code, size, and duration
  logHttpResponse(requestNum, 'POST', requestPath, response.status, bytes, duration);

  return data;
}

/**
 * Process batch query results into lookup categories
 * @param {Object} batchResults - Results from batch query endpoint
 * @returns {Object} Processed lookup data by category
 */
function processBatchResults(batchResults) {
  const lookups = {};

  if (!batchResults || !batchResults.results || !Array.isArray(batchResults.results)) {
    logger.warn('Invalid batch results format');
    return lookups;
  }

  // Process each result in the array
  batchResults.results.forEach(result => {
    if (!result.success) {
      logger.warn(`Query ${result.query_ref} failed: ${result.error || 'Unknown error'}`);
      return;
    }

    const queryRef = result.query_ref;
    const rows = result.rows || [];

    // Map QueryRef to lookup category
    switch (queryRef) {
      case 1:
        // QueryRef 001 - System Information (first row contains license info)
        lookups.system_info = rows[0] || {};
        break;

      case 30:
        // QueryRef 030 - Names of all lookups
        lookups.lookup_names = rows;
        break;

      case 53:
        // QueryRef 053 - Themes
        lookups.themes = rows;
        break;

      case 54:
        // QueryRef 054 - Icons
        lookups.icons = rows;
        break;

      case 46:
        // QueryRef 046 - Macro Expansion
        lookups.macros = rows;
        break;

      case 60:
        // QueryRef 060 - Tabulator Schemas
        lookups.tabulator_schemas = rows;
        break;

      default:
        logger.info(`Unhandled query_ref: ${queryRef} with ${rows.length} rows`);
    }
  });

  return lookups;
}

/**
 * Fetch lookups from the server using batch query
 * @param {Object} options - Fetch options
 * @param {boolean} options.force - Force refresh even if cached
 * @param {boolean} options.silent - Don't emit events (for background refresh)
 * @returns {Promise<Object>} Lookup data
 */
export async function fetchLookups(options = {}) {
  const { force = false, silent = false } = options;

  // Return cached data if available and not forcing refresh
  if (cache && !force) {
    return cache;
  }

  // Return existing promise if fetch is in progress
  if (fetchPromise && !force) {
    return fetchPromise;
  }

  // Try loading from localStorage first (if not forcing refresh)
  if (!force && !cache) {
    const localData = loadFromLocalStorage();
    if (localData) {
      cache = localData;
      const qrefs = Object.keys(cache)
        .map(k => LOOKUP_META[k]?.queryRef)
        .filter(Boolean)
        .join(', ');
      logger.info(`Loading lookups [${qrefs}] from localStorage`);
      // Emit event that lookups are loaded from cache
      if (!silent) {
        emitLookupsLoaded('cache', Object.keys(cache), cache);
      }
      // Still fetch fresh data in background
      fetchFreshLookups(silent);
      return cache;
    }
  }

  // Fetch fresh data from server
  fetchPromise = fetchFreshLookups(silent);
  return fetchPromise;
}

/**
 * Fetch fresh lookups from the server
 * @param {boolean} silent - Don't emit events
 * @returns {Promise<Object>} Fresh lookup data
 */
async function fetchFreshLookups(silent = false) {
  try {
    if (!silent) {
      eventBus.emitSilent(Events.LOOKUPS_LOADING, {});
    }

    // Fetch all open lookups in one batch
    // Note: QueryRef 046 (Macros) is loaded separately post-login - see loadMacrosPostLogin()
    const queryRefs = [1, 30, 53, 54, 60]; // QueryRefs 001, 030, 053, 054, 060
    const qrefLabels = queryRefs.map(r => String(r).padStart(3, '0')).join(', ');
    logger.info(`Loading lookups [${qrefLabels}] from server`);
    const batchResults = await fetchBatchQueries(queryRefs, '001');

    // Process results into lookup categories
    const freshLookups = processBatchResults(batchResults);

    // Merge with existing cache if present
    cache = { ...cache, ...freshLookups };

    // Save to localStorage
    saveToLocalStorage(cache);

    // Emit server-loaded event
    if (!silent) {
      emitLookupsLoaded('server', Object.keys(freshLookups), freshLookups);
    }

    fetchPromise = null;
    return cache;
  } catch (error) {
    fetchPromise = null;
    logger.warn(`Failed to fetch from server: ${error.message}`);

    // If we have cache, return it even on error
    if (cache) {
      if (!silent) {
        emitLookupsLoaded('cache_fallback', Object.keys(cache), cache);
      }
      return cache;
    }

    // Emit error event
    eventBus.emit(Events.LOOKUPS_ERROR, { error: error.message });

    // Return empty object on error
    return {};
  }
}

/**
 * Check if a lookup category is available (loaded from any source)
 * @param {string} category - Lookup category name
 * @returns {boolean} True if available
 */
export function hasLookup(category) {
  return cache !== null && cache[category] !== undefined;
}

/**
 * Get a lookup value by category and key
 * @param {string} category - Lookup category (e.g., 'managers', 'themes', 'icons')
 * @param {string|number} key - Lookup key
 * @param {*} defaultValue - Default value if not found
 * @returns {*} Lookup value or default
 */
export function getLookup(category, key, defaultValue = null) {
  if (!cache) {
    logger.warn('Lookups not loaded yet');
    return defaultValue !== null ? defaultValue : key;
  }

  const categoryData = cache[category];
  if (!categoryData) {
    return defaultValue !== null ? defaultValue : key;
  }

  const value = categoryData[String(key)];
  return value !== undefined ? value : defaultValue !== null ? defaultValue : key;
}

/**
 * Get manager name by ID
 * @param {number|string} managerId - Manager ID
 * @param {string} defaultName - Default name if not found
 * @returns {string} Manager name
 */
export function getManagerName(managerId, defaultName = null) {
  return getLookup('managers', managerId, defaultName);
}

/**
 * Get all managers lookup data
 * @returns {Object|null} Managers data or null
 */
export function getManagers() {
  return cache?.managers || null;
}

/**
 * Get themes lookup data
 * @returns {Object|null} Themes data or null
 */
export function getThemes() {
  return cache?.themes || null;
}

/**
 * Get icons lookup data
 * @returns {Object|null} Icons data or null
 */
export function getIcons() {
  return cache?.icons || null;
}

/**
 * Get Tabulator schemas lookup data
 * @returns {Object|null} Tabulator schemas data or null
 */
export function getTabulatorSchemas() {
  return cache?.tabulator_schemas || null;
}

/**
 * Get system info lookup data
 * @returns {Object|null} System info data or null
 */
export function getSystemInfo() {
  return cache?.system_info || null;
}

/**
 * Get lookup names (all available lookups)
 * @returns {Object|null} Lookup names data or null
 */
export function getLookupNames() {
  return cache?.lookup_names || null;
}

/**
 * Get feature name by manager ID and feature ID
 * @param {number|string} managerId - Manager ID
 * @param {number|string} featureId - Feature ID
 * @param {string} defaultName - Default name if not found
 * @returns {string} Feature name
 */
export function getFeatureName(managerId, featureId, defaultName = null) {
  const category = `features_${managerId}`;
  return getLookup(category, featureId, defaultName);
}

/**
 * Get all lookups for a category
 * @param {string} category - Lookup category
 * @returns {Object|null} Category data or null
 */
export function getLookupCategory(category) {
  if (!cache) {
    logger.warn('Lookups not loaded yet');
    return null;
  }
  return cache[category] || null;
}

/**
 * Get all cached lookups
 * @returns {Object|null} All lookup data
 */
export function getAllLookups() {
  return cache;
}

/**
 * Check if lookups have been loaded
 * @returns {boolean} True if loaded
 */
export function isLoaded() {
  return cache !== null;
}

/**
 * Clear the lookup cache (both memory and localStorage)
 */
export function clearCache() {
  cache = null;
  fetchPromise = null;
  clearLocalStorage();
}

/**
 * Refresh lookups from server (clears cache and re-fetches)
 * @param {Object} options - Refresh options
 * @param {boolean} options.silent - Don't emit events
 */
export async function refreshLookups(options = {}) {
  clearCache();
  return fetchLookups({ force: true, silent: options.silent });
}

/**
 * Load macros lookup post-login (requires authentication)
 * This should be called after successful login when JWT is available
 * Checks cache first, then fetches from server if needed
 * @param {Object} api - The API/conduit instance with auth
 * @param {Object} options - Options
 * @param {boolean} options.force - Force refresh even if cached
 * @returns {Promise<boolean>} True if macros loaded successfully
 */
export async function loadMacrosPostLogin(api, options = {}) {
  const { force = false } = options;

  // DEBUG: Always log when this function is called
  logger.info(`[Lookups] loadMacrosPostLogin called, force=${force}`);

  // Check if already loaded in memory cache (and has content)
  if (!force && cache?.macros && Object.keys(cache.macros).length > 0) {
    logger.info(`[Lookups] Macros already loaded (${Object.keys(cache.macros).length} entries)`);
    return true;
  }

  // Check localStorage cache before making API call
  if (!force && !cache?.macros) {
    const localData = loadFromLocalStorage();
    if (localData?.macros && Object.keys(localData.macros).length > 0) {
      // Merge with existing cache, don't replace (preserve other lookups like tabulator_schemas)
      cache = { ...localData, ...cache };
      logger.info(`[Lookups] Macros loaded from localStorage (${Object.keys(cache.macros).length} entries)`);
      eventBus.emitSilent('lookups:macros:loaded', { count: Object.keys(cache.macros).length, source: 'cache' });
      // Still fetch fresh in background
      fetchMacrosInBackground(api);
      return true;
    }
  }

  // Fetch from server
  return fetchMacrosFromServer(api);
}

/**
 * Fetch macros from server (internal)
 * Uses QueryRef 26 with LOOKUPID=46 (same as Version Manager)
 * @private
 */
async function fetchMacrosFromServer(api) {
  try {
    logger.info('[Lookups] Loading macros from server (QueryRef 026, LOOKUPID=46)');

    // Import authQuery dynamically to avoid circular dependency
    const { authQuery } = await import('./conduit.js');

    // QueryRef 026 with LOOKUPID=46 returns macro definitions from lookup table
    const rows = await authQuery(api, 26, { INTEGER: { LOOKUPID: 46 } });

    // Process macros into a simple key-value map
    // Each row has: key_idx, value_txt (macro name like "{ACZ.CLIENT}"), code (macro value), collection (locale variants)
    const macros = {};
    if (Array.isArray(rows)) {
      rows.forEach(row => {
        // value_txt contains the macro name like "{ACZ.CLIENT}"
        // Strip the braces to get the clean key for lookup
        let key = row.value_txt;
        if (!key) return;
        
        // Remove { and } from key if present
        key = key.replace(/^\{/, '').replace(/\}$/, '');

        // Try to get value from code first, then parse collection for locale
        let value = row.code || '';

        // If collection exists, try to get locale-specific value
        if (row.collection) {
          let collection = row.collection;
          if (typeof collection === 'string') {
            try {
              collection = JSON.parse(collection);
            } catch (_e) {
              collection = {};
            }
          }
          if (collection && typeof collection === 'object') {
            const currentLocale = navigator.language || 'en-US';
            // Priority: exact locale > language match > Default
            if (collection[currentLocale]) {
              value = collection[currentLocale];
            } else if (currentLocale.includes('-')) {
              const langOnly = currentLocale.split('-')[0];
              for (const [k, v] of Object.entries(collection)) {
                if (k.toLowerCase().startsWith(langOnly.toLowerCase())) {
                  value = v;
                  break;
                }
              }
            } else if (collection.Default) {
              value = collection.Default;
            }
          }
        }

        macros[key] = value;
        logger.info(`[Lookups] Macro loaded: "${key}" = "${value}"`);
      });
    }

    // Store in cache
    if (!cache) cache = {};
    cache.macros = macros;

    // Save to localStorage
    saveToLocalStorage(cache);

    logger.info(`[Lookups] Loaded ${Object.keys(macros).length} macros from server`);
    logger.info(`[Lookups] Macro keys: ${Object.keys(macros).join(', ')}`);
    eventBus.emitSilent('lookups:macros:loaded', { count: Object.keys(macros).length, source: 'server' });

    return true;
  } catch (error) {
    logger.error(`[Lookups] Failed to load macros: ${error.message}`);
    return false;
  }
}

/**
 * Fetch macros in background (don't await)
 * @private
 */
function fetchMacrosInBackground(api) {
  fetchMacrosFromServer(api).catch(err => {
    logger.warn(`[Lookups] Background macros refresh failed: ${err.message}`);
  });
}

/**
 * Get all macros as a key-value object
 * @returns {Object|null} Macros object or null if not loaded
 */
export function getMacros() {
  return cache?.macros || null;
}

/**
 * Get a specific macro value by name
 * @param {string} name - Macro name (e.g., 'APP_NAME')
 * @param {string} defaultValue - Default value if not found
 * @returns {string} Macro value or default
 */
export function getMacro(name, defaultValue = '') {
  if (!cache?.macros) {
    return defaultValue !== '' ? defaultValue : name;
  }
  return cache.macros[name] !== undefined ? cache.macros[name] : defaultValue;
}

/**
 * Initialize lookups module
 * Sets up event listeners for auto-refresh
 */
export function init() {
  // Note: We intentionally do NOT refresh lookups when locale changes.
  // The lookup data (themes, icons, system_info, lookup_names) is not
  // locale-dependent, so there's no need for a server round-trip.
  // If locale-specific lookups are needed in the future, add a
  // filtered getter that applies Intl to the cached data instead.

  // Note: We intentionally do NOT clear cache on logout.
  // Lookups contain "open" data (themes, system_info, icons, lookup_names)
  // that don't require authentication and should persist for the login page.
  // The in-memory cache is naturally cleared on page reload.
}

// Default export
export default {
  fetch: fetchLookups,
  get: getLookup,
  has: hasLookup,
  getManagerName,
  getManagers,
  getThemes,
  getIcons,
  getTabulatorSchemas,
  getSystemInfo,
  getLookupNames,
  getFeatureName,
  getCategory: getLookupCategory,
  getAll: getAllLookups,
  isLoaded,
  clearCache,
  refresh: refreshLookups,
  init,
  loadMacrosPostLogin,
  getMacros,
  getMacro,
};
