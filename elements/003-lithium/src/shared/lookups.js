/**
 * Lookups - Server-provided reference data
 *
 * Caches and provides access to lookup data from the server.
 * Lookups include manager names, feature labels, status values, etc.
 * Fetched after login and refreshed on locale changes.
 */

import * as jsonRequest from '../core/json-request.js';
import { eventBus, Events } from '../core/event-bus.js';

// In-memory cache for lookups
let cache = null;
let fetchPromise = null;

/**
 * Fetch lookups from the server
 * @param {Object} options - Fetch options
 * @param {boolean} options.force - Force refresh even if cached
 * @returns {Promise<Object>} Lookup data
 */
export async function fetchLookups(options = {}) {
  const { force = false } = options;

  // Return cached data if available and not forcing refresh
  if (cache && !force) {
    return cache;
  }

  // Return existing promise if fetch is in progress
  if (fetchPromise && !force) {
    return fetchPromise;
  }

  // Fetch from server
  fetchPromise = jsonRequest
    .get('/lookups')
    .then((data) => {
      cache = data || {};
      fetchPromise = null;
      return cache;
    })
    .catch((error) => {
      fetchPromise = null;
      console.warn('Failed to fetch lookups:', error.message);
      // Return empty cache on error
      cache = cache || {};
      return cache;
    });

  return fetchPromise;
}

/**
 * Get a lookup value by category and key
 * @param {string} category - Lookup category (e.g., 'managers', 'features')
 * @param {string|number} key - Lookup key
 * @param {*} defaultValue - Default value if not found
 * @returns {*} Lookup value or default
 */
export function getLookup(category, key, defaultValue = null) {
  if (!cache) {
    console.warn('Lookups not loaded yet');
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
    console.warn('Lookups not loaded yet');
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
 * Clear the lookup cache
 */
export function clearCache() {
  cache = null;
  fetchPromise = null;
}

/**
 * Refresh lookups from server (clears cache and re-fetches)
 * @returns {Promise<Object>} Fresh lookup data
 */
export async function refreshLookups() {
  clearCache();
  return fetchLookups({ force: true });
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

  // Clear cache on logout
  eventBus.on(Events.AUTH_LOGOUT, () => {
    clearCache();
  });
}

// Default export
export default {
  fetch: fetchLookups,
  get: getLookup,
  getManagerName,
  getFeatureName,
  getCategory: getLookupCategory,
  getAll: getAllLookups,
  isLoaded,
  clearCache,
  refresh: refreshLookups,
  init,
};
