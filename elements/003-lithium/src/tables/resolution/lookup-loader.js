/**
 * Lookup Data Loader
 *
 * Loads and caches lookup table data from the Hydrogen API.
 *
 * @module tables/resolution/lookup-loader
 */

import { log, Subsystems, Status } from '../../core/log.js';

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
export async function loadLookup(lookupRef, authQueryFn, api) {
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
export function getLookup(lookupRef) {
  return _lookupCache.get(String(lookupRef));
}

/**
 * Resolve a lookup ID to its label.
 *
 * @param {number} id - The lookup ID to resolve
 * @param {string|number} lookupRef - Lookup table reference
 * @returns {string} The label, or the original ID if not found
 */
export function resolveLookupLabel(id, lookupRef) {
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
export function createLookupFormatter(lookupRef) {
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
export function createIconFormatter(lookupData) {
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
export function createIconLabelFormatter(lookupData) {
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
export function createLookupEditor(lookupRef, lookupData) {
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
 * Get a sortable value for a lookup ID.
 * Returns a tuple [sortSeq, label, id] for proper sorting.
 *
 * @param {number} id - The lookup ID
 * @param {string|number} lookupRef - Lookup table reference
 * @returns {Array} [sortSeq, label, id] tuple for sorting
 */
export function getLookupSortValue(id, lookupRef) {
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
export function compareLookupValues(aId, bId, lookupRef, dir = 'asc') {
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
export function createLookupSorter(lookupRef) {
  return function(a, b, aRow, bRow, column, dir) {
    // a and b are the raw cell values (lookup IDs)
    return compareLookupValues(a, b, lookupRef, dir);
  };
}

/**
 * Clear all cached lookup data.
 */
export function clearLookupCache() {
  _lookupCache.clear();
}
