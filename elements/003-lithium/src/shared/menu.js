/**
 * Menu Service
 *
 * Fetches and caches the main menu data from QueryRef 046.
 * Menu items are grouped and include icons and count badges.
 *
 * The lithium.json managers section acts as a restrictive filter:
 * only managers listed and enabled (true) in that config will appear
 * in the menu, regardless of what the database returns.
 */

import { authQuery } from './conduit.js';
import { log, Subsystems, Status } from '../core/log.js';
import { getConfigValue, isConfigLoaded, loadConfig } from '../core/config.js';

// QueryRef for main menu
const MENU_QUERY_REF = 46;

// localStorage key for caching menu data
export const MENU_CACHE_KEY = 'lithium_menu_data';
export const MENU_CACHE_TIMESTAMP_KEY = 'lithium_menu_cache_time';
export const CACHE_TTL_MS = 5 * 60 * 1000; // 5 minutes

// In-memory cache
let menuCache = null;

// Cached set of enabled manager IDs from lithium.json
let enabledManagerIds = null;

/**
 * Parse the enabled manager IDs from lithium.json managers config.
 * Keys are in format "NNN.Name" where NNN is the numeric ID.
 * Only keys with value === true are considered enabled.
 * @returns {Set<number>} Set of enabled manager IDs
 */
/**
 * Get enabled manager IDs from lithium.json config.
 * Fetches the config directly if not already loaded.
 * @returns {Set<number>} Set of enabled manager IDs
 */
export async function getEnabledManagerIdsAsync() {
  // Return cached result if available
  if (enabledManagerIds !== null && enabledManagerIds.size > 0) {
    return enabledManagerIds;
  }

  // Ensure config is loaded
  if (!isConfigLoaded()) {
    await loadConfig();
  }

  const managersConfig = getConfigValue('managers', {});
  const ids = new Set();

  if (managersConfig && typeof managersConfig === 'object') {
    for (const [key, value] of Object.entries(managersConfig)) {
      // Only include managers that are explicitly enabled (true)
      if (value === true) {
        // Parse the numeric ID from the key (e.g., "007.Query Manager" -> 7)
        const match = key.match(/^(\d+)\./);
        if (match) {
          const id = parseInt(match[1], 10);
          if (!isNaN(id) && id > 0) {
            ids.add(id);
          }
        }
      }
    }
  }

  enabledManagerIds = ids;
  log(Subsystems.MANAGER, Status.INFO, `[MenuService] Filter enabled: ${ids.size} managers`);
  
  return ids;
}

/**
 * Get enabled manager IDs (synchronous version - requires config already loaded).
 * @returns {Set<number>} Set of enabled manager IDs (empty if config not ready)
 */
export function getEnabledManagerIds() {
  // Return cached result if available
  if (enabledManagerIds !== null) {
    return enabledManagerIds;
  }

  // If config not loaded, return empty - use async version instead
  if (!isConfigLoaded()) {
    return new Set();
  }

  const managersConfig = getConfigValue('managers', {});
  const ids = new Set();

  if (managersConfig && typeof managersConfig === 'object') {
    for (const [key, value] of Object.entries(managersConfig)) {
      if (value === true) {
        const match = key.match(/^(\d+)\./);
        if (match) {
          const id = parseInt(match[1], 10);
          if (!isNaN(id) && id > 0) {
            ids.add(id);
          }
        }
      }
    }
  }

  enabledManagerIds = ids;
  return ids;
}

/**
 * Clear the cached enabled manager IDs (useful when config is reloaded)
 */
export function clearEnabledManagerCache() {
  enabledManagerIds = null;
}

/**
 * Menu item structure from QueryRef 046:
 * {
 *   grpname: string,      // Group name
 *   grpnum: number,       // Group ID
 *   modname: string,      // Module/manager name
 *   modnum: number,       // Module/manager ID
 *   grpsort: number,      // Group sort order
 *   modsort: number,      // Module sort order
 *   entries: number,      // Count badge value
 *   collection: string|Object // JSON containing icon info
 * }
 */

/**
 * Parse collection field to extract icon information
 * The collection contains JSON with an "icon" field (HTML <i> tag) and index"
 * @param {string|Object} collection - Collection field from query result
 * @returns {Object} Parsed icon info with fallback, includes index for filtering
 */
export function parseCollection(collection) {
  const fallback = { iconHtml: '<fa fa-cube></fa>', index: 0, visible: true };

  if (!collection) {
    return fallback;
  }

  try {
    const parsed = typeof collection === 'string' ? JSON.parse(collection) : collection;
    
    // Get icon HTML from server, ensuring it has a proper closing tag
    // Server may return: <fa fa-fw fa-xl fa-receipt> (no closing tag)
    // We need: <fa fa-fw fa-xl fa-receipt></fa>
    let iconHtml = parsed.icon || fallback.iconHtml;
    
    // Ensure the icon tag is properly closed
    if (iconHtml && !iconHtml.includes('</fa>')) {
      // Extract the tag content and wrap with proper opening/closing tags
      const match = iconHtml.match(/<fa\s+(.+?)\/?>/);
      if (match) {
        iconHtml = `<fa ${match[1]}></fa>`;
      } else {
        iconHtml = fallback.iconHtml;
      }
    }
    
    // Items with negative index should be hidden (Main Menu = -2, Login = -1)
    const index = parsed.index !== undefined ? parsed.index : 0;
    
    return {
      iconHtml,
      index,
      visible: index >= 0, // Only show items with index >= 0
    };
  } catch (e) {
    return fallback;
  }
}

/**
 * Transform flat menu items into grouped structure.
 * Filters out managers not enabled in lithium.json config.
 * Uses the 'index' field from collection (matches lithium.json key numbers like 001, 007, etc.)
 * @param {Array} items - Raw menu items from QueryRef 046
 * @param {Set<number>} allowedManagerIds - Optional set of allowed manager IDs (if not provided, will be fetched)
 * @returns {Array} Grouped menu structure
 */
export function groupMenuItems(items, allowedManagerIds = null) {
  const groups = new Map();
  // Use provided set, or fall back to sync version (which may be empty if config not loaded)
  const allowedIds = allowedManagerIds || getEnabledManagerIds();

  items.forEach((item) => {
    const groupId = item.grpnum;
    const collectionInfo = parseCollection(item.collection);
    
    // Skip items that should be hidden (negative index like Main Menu, Login)
    if (!collectionInfo.visible) {
      return;
    }

    // Filter by index from collection (matches lithium.json key numbers)
    // Index 0 means no index specified - allow through (for backwards compatibility)
    // Non-zero index must be in allowed list
    if (allowedIds.size > 0 && collectionInfo.index > 0 && !allowedIds.has(collectionInfo.index)) {
      return;
    }

    if (!groups.has(groupId)) {
      groups.set(groupId, {
        id: groupId,
        name: item.grpname,
        sortOrder: item.grpsort,
        items: [],
      });
    }

    const group = groups.get(groupId);
    group.items.push({
      managerId: collectionInfo.index || item.modnum,  // Use index (matches lithium.json keys), fallback to modnum
      name: item.modname,
      sortOrder: item.modsort,
      count: item.entries || 0,
      iconHtml: collectionInfo.iconHtml,
      index: collectionInfo.index,
    });
  });

  // Sort groups by sortOrder, then by name
  const sortedGroups = Array.from(groups.values()).sort((a, b) => {
    if (a.sortOrder !== b.sortOrder) {
      return a.sortOrder - b.sortOrder;
    }
    return a.name.localeCompare(b.name);
  });

  // Sort items within each group
  sortedGroups.forEach((group) => {
    group.items.sort((a, b) => {
      if (a.sortOrder !== b.sortOrder) {
        return a.sortOrder - b.sortOrder;
      }
      return a.name.localeCompare(b.name);
    });
  });

  return sortedGroups;
}

/**
 * Fetch menu data from server using QueryRef 046
 * @param {Object} api - API client instance
 * @returns {Promise<Array>} Grouped menu data
 */
export async function fetchMenu(api) {
  try {
    log(Subsystems.MANAGER, Status.INFO, '[MenuService] Fetching menu from QueryRef 046');

    // Ensure config is loaded before filtering
    const allowedManagerIds = await getEnabledManagerIdsAsync();

    const items = await authQuery(api, MENU_QUERY_REF);

    if (!Array.isArray(items)) {
      throw new Error('Invalid menu data format');
    }

    const grouped = groupMenuItems(items, allowedManagerIds);

    // Update cache
    menuCache = grouped;
    cacheMenuData(grouped);

    log(Subsystems.MANAGER, Status.INFO, `[MenuService] Menu loaded: ${grouped.length} groups, ${items.length} items`);

    return grouped;
  } catch (error) {
    log(Subsystems.MANAGER, Status.ERROR, `[MenuService] Failed to fetch menu: ${error.message}`);
    throw error;
  }
}

/**
 * Get cached menu data from localStorage
 * @returns {Array|null} Cached menu data or null
 */
export function getCachedMenuData() {
  try {
    const cached = localStorage.getItem(MENU_CACHE_KEY);
    const timestamp = localStorage.getItem(MENU_CACHE_TIMESTAMP_KEY);

    if (!cached || !timestamp) {
      return null;
    }

    // Check TTL
    const age = Date.now() - parseInt(timestamp, 10);
    if (age > CACHE_TTL_MS) {
      clearMenuCache();
      return null;
    }

    return JSON.parse(cached);
  } catch (e) {
    return null;
  }
}

/**
 * Cache menu data to localStorage
 * @param {Array} data - Menu data to cache
 */
function cacheMenuData(data) {
  try {
    localStorage.setItem(MENU_CACHE_KEY, JSON.stringify(data));
    localStorage.setItem(MENU_CACHE_TIMESTAMP_KEY, String(Date.now()));
  } catch (e) {
    // Non-fatal - localStorage may be unavailable or full
    log(Subsystems.MANAGER, Status.WARN, '[MenuService] Failed to cache menu data');
  }
}

/**
 * Clear cached menu data
 */
export function clearMenuCache() {
  menuCache = null;
  try {
    localStorage.removeItem(MENU_CACHE_KEY);
    localStorage.removeItem(MENU_CACHE_TIMESTAMP_KEY);
  } catch (e) {
    // Non-fatal
  }
}

/**
 * Get menu data - returns cached data if available, otherwise fetches
 * @param {Object} api - API client instance
 * @param {Object} options - Options
 * @param {boolean} options.refresh - Force refresh from server
 * @returns {Promise<Array>} Menu data
 */
export async function getMenu(api, options = {}) {
  // Return in-memory cache if available
  if (menuCache && !options.refresh) {
    return menuCache;
  }

  // Try localStorage cache
  const cached = getCachedMenuData();
  if (cached && !options.refresh) {
    menuCache = cached;
    return cached;
  }

  // Fetch from server
  return fetchMenu(api);
}

/**
 * Get flat list of all manager IDs from menu
 * @param {Array} menuData - Grouped menu data
 * @returns {Array<number>} Array of manager IDs
 */
export function getManagerIdsFromMenu(menuData) {
  if (!menuData || !Array.isArray(menuData)) {
    return [];
  }

  const ids = [];
  menuData.forEach((group) => {
    group.items.forEach((item) => {
      ids.push(item.managerId);
    });
  });

  return ids;
}

/**
 * Get menu item by manager ID
 * @param {Array} menuData - Grouped menu data
 * @param {number} managerId - Manager ID to find
 * @returns {Object|null} Menu item or null
 */
export function getMenuItemById(menuData, managerId) {
  if (!menuData || !Array.isArray(menuData)) {
    return null;
  }

  for (const group of menuData) {
    const item = group.items.find((i) => i.managerId === managerId);
    if (item) {
      return { ...item, groupName: group.name, groupId: group.id };
    }
  }

  return null;
}

/**
 * Build manager icons registry from menu data
 * @param {Array} menuData - Grouped menu data
 * @returns {Object} Manager icons registry (managerId -> {iconHtml, name})
 */
export function buildManagerIconsRegistry(menuData) {
  const registry = {};

  if (!menuData || !Array.isArray(menuData)) {
    return registry;
  }

  menuData.forEach((group) => {
    group.items.forEach((item) => {
      registry[item.managerId] = {
        iconHtml: item.iconHtml || '<fa fa-cube></fa>',
        name: item.name,
      };
    });
  });

  return registry;
}

// Default export
export default {
  fetchMenu,
  getMenu,
  clearMenuCache,
  getManagerIdsFromMenu,
  getMenuItemById,
  buildManagerIconsRegistry,
  getEnabledManagerIds,
  getEnabledManagerIdsAsync,
  clearEnabledManagerCache,
};
