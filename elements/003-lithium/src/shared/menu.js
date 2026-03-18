/**
 * Menu Service
 *
 * Fetches and caches the main menu data from QueryRef 046.
 * Menu items are grouped and include icons and count badges.
 */

import { authQuery } from './conduit.js';
import { log, Subsystems, Status } from '../core/log.js';

// QueryRef for main menu
const MENU_QUERY_REF = 46;

// localStorage key for caching menu data
export const MENU_CACHE_KEY = 'lithium_menu_data';
export const MENU_CACHE_TIMESTAMP_KEY = 'lithium_menu_cache_time';
export const CACHE_TTL_MS = 5 * 60 * 1000; // 5 minutes

// In-memory cache
let menuCache = null;

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
 * The collection contains JSON with an "Icon" field (HTML <i> tag) and "Index"
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
    
    // Get Icon HTML from server, ensuring it has a proper closing tag
    // Server may return: <fa fa-fw fa-xl fa-receipt> (no closing tag)
    // We need: <fa fa-fw fa-xl fa-receipt></fa>
    let iconHtml = parsed.Icon || fallback.iconHtml;
    
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
    
    // Items with negative Index should be hidden (Main Menu = -2, Login = -1)
    const index = parsed.Index !== undefined ? parsed.Index : 0;
    
    return {
      iconHtml,
      index,
      visible: index >= 0, // Only show items with Index >= 0
    };
  } catch (e) {
    return fallback;
  }
}

/**
 * Transform flat menu items into grouped structure
 * @param {Array} items - Raw menu items from QueryRef 046
 * @returns {Array} Grouped menu structure
 */
export function groupMenuItems(items) {
  const groups = new Map();

  items.forEach((item) => {
    const groupId = item.grpnum;
    const collectionInfo = parseCollection(item.collection);
    
    // Skip items that should be hidden (negative Index like Main Menu, Login)
    if (!collectionInfo.visible) {
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
      managerId: item.modnum,
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

    const items = await authQuery(api, MENU_QUERY_REF);

    if (!Array.isArray(items)) {
      throw new Error('Invalid menu data format');
    }

    const grouped = groupMenuItems(items);

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
};
