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
  system_info:   { queryRef: '001', label: 'System Info',    countLabel: 'elements' },
  lookup_names:  { queryRef: '030', label: 'Lookup Names',   countLabel: 'lookups' },
  themes:        { queryRef: '053', label: 'Themes',         countLabel: 'themes' },
  icons:         { queryRef: '054', label: 'Icons',          countLabel: 'icons' },
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
};

// Categories that are "open" (available before login)
const OPEN_CATEGORIES = ['system_info', 'themes', 'icons', 'managers'];

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
    const queryRefs = [1, 30, 53, 54]; // QueryRefs 001, 030, 053, 054
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
 * @returns {Promise<Object>} Fresh lookup data
 */
export async function refreshLookups(options = {}) {
  const { silent = false } = options;
  clearCache();
  return fetchLookups({ force: true, silent });
}

/**
 * Initialize lookups module
 * Sets up event listeners for auto-refresh
 */
export function init() {
  // Refresh lookups when locale changes
  eventBus.on(Events.LOCALE_CHANGED, () => {
    refreshLookups();
  });

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
  getSystemInfo,
  getLookupNames,
  getFeatureName,
  getCategory: getLookupCategory,
  getAll: getAllLookups,
  isLoaded,
  clearCache,
  refresh: refreshLookups,
  init,
};
