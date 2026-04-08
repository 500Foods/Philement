/**
 * Menu Service Unit Tests
 */

import { describe, it, expect, beforeEach, vi } from 'vitest';
import {
  fetchMenu,
  clearMenuCache,
  buildManagerIconsRegistry,
  getManagerIdsFromMenu,
  getCachedMenuData,
  parseCollection,
  groupMenuItems,
  clearEnabledManagerCache,
  MENU_CACHE_KEY,
  MENU_CACHE_TIMESTAMP_KEY,
} from '../../src/shared/menu.js';

// Mock conduit.js for fetchMenu tests
vi.mock('../../src/shared/conduit.js', () => ({
  authQuery: vi.fn(),
}));

// Mock config.js to return managers config for filtering tests
vi.mock('../../src/core/config.js', () => ({
  getConfigValue: vi.fn((path, defaultValue) => {
    // Return mock managers config with all test manager IDs enabled
    if (path === 'managers') {
      return {
        '002.Session Logs': true,
        '003.Version History': true,
        '004.Queries': true,
        '005.Lookups': true,
        '006.Chats': true,
        '007.AI Auditor': true,
        '008.Dashboard': true,
        '009.Document Library': true,
        '010.Media Library': true,
        '011.Diagram Library': true,
        '012.Reports': true,
        '099.Test Manager': true,
      };
    }
    return defaultValue;
  }),
  loadConfig: vi.fn(async () => ({})),
  getConfig: vi.fn(() => ({})),
  isConfigLoaded: vi.fn(() => true),
  clearConfig: vi.fn(),
}));

// Mock localStorage
const localStorageMock = (() => {
  let store = {};
  return {
    getItem: vi.fn((key) => store[key] || null),
    setItem: vi.fn((key, value) => { store[key] = value; }),
    removeItem: vi.fn((key) => { delete store[key]; }),
    clear: vi.fn(() => { store = {}; }),
  };
})();

Object.defineProperty(window, 'localStorage', { value: localStorageMock });

describe('Menu Service', () => {
  beforeEach(() => {
    localStorageMock.clear();
    clearMenuCache();
    clearEnabledManagerCache();
    vi.clearAllMocks();
  });

  describe('parseCollection', () => {
    it('should parse collection JSON with icon HTML', () => {
      const collection = '{"icon":"<fa fa-receipt fa-fw></fa>"}';
      const result = parseCollection(collection);
      
      expect(result.iconHtml).toBe('<fa fa-receipt fa-fw></fa>');
      expect(result.visible).toBe(true);
    });

    it('should parse collection JSON with icon and index', () => {
      const collection = '{"icon":"<fa fa-history></fa>","index":0}';
      const result = parseCollection(collection);
      
      expect(result.iconHtml).toBe('<fa fa-history></fa>');
      expect(result.index).toBe(0);
      expect(result.visible).toBe(true);
    });

    it('should handle invalid JSON gracefully', () => {
      const collection = 'invalid json';
      const result = parseCollection(collection);
      
      expect(result.iconHtml).toBe('<fa fa-cube></fa>');
      expect(result.index).toBe(0);
      expect(result.visible).toBe(true);
    });

    it('should handle empty collection', () => {
      const result = parseCollection('');
      
      expect(result.iconHtml).toBe('<fa fa-cube></fa>');
      expect(result.visible).toBe(true);
    });

    it('should extract icon from complex HTML', () => {
      const collection = '{"icon":"<fa fa-chart-line fa-fw fa-xl></fa>"}';
      const result = parseCollection(collection);
      
      expect(result.iconHtml).toBe('<fa fa-chart-line fa-fw fa-xl></fa>');
    });

    it('should mark items with negative index as not visible', () => {
      const collection = '{"icon":"<fa fa-bars></fa>","index":-2}';
      const result = parseCollection(collection);
      
      expect(result.iconHtml).toBe('<fa fa-bars></fa>');
      expect(result.index).toBe(-2);
      expect(result.visible).toBe(false);
    });
  });

  describe('groupMenuItems integration', () => {
    it('should process raw menu data into grouped structure', () => {
      const rawData = [
        {
          grpname: 'System',
          grpnum: 0,
          grpsort: 0,
          modname: 'Session Logs',
          modnum: 2,
          modsort: 0,
          entries: 0,
          collection: '{"icon":"<i class=\'fa fa-receipt\'></i>"}',
        },
        {
          grpname: 'System',
          grpnum: 0,
          grpsort: 0,
          modname: 'Version History',
          modnum: 3,
          modsort: 1,
          entries: 5,
          collection: '{"icon":"<i class=\'fa fa-history\'></i>"}',
        },
      ];

      const result = groupMenuItems(rawData);

      expect(result).toHaveLength(1);
      expect(result[0].id).toBe(0);
      expect(result[0].name).toBe('System');
      expect(result[0].items).toHaveLength(2);
      expect(result[0].items[0].managerId).toBe(2);
      expect(result[0].items[0].name).toBe('Session Logs');
      expect(result[0].items[1].count).toBe(5);
    });

    it('should filter out items with negative index', () => {
      const rawData = [
        {
          grpname: 'System',
          grpnum: 0,
          modname: 'Session Logs',
          modnum: 2,
          collection: '{"icon":"<i class=\'fa fa-receipt\'></i>","index":0}',
        },
        {
          grpname: 'Hidden',
          grpnum: 0,
          modname: 'Main Menu',
          modnum: 1,
          collection: '{"icon":"<i class=\'fa fa-bars\'></i>","index":-2}',
        },
        {
          grpname: 'Hidden',
          grpnum: 0,
          modname: 'Login',
          modnum: 99,
          collection: '{"icon":"<i class=\'fa fa-sign-in\'></i>","index":-1}',
        },
      ];

      const result = groupMenuItems(rawData);

      expect(result[0].items).toHaveLength(1);
      expect(result[0].items[0].managerId).toBe(2);
    });
  });

  describe('groupMenuItems', () => {
    it('should group items by grpnum', () => {
      const items = [
        { grpnum: 0, grpname: 'System', grpsort: 0, modnum: 2, modsort: 0, modname: 'Session Logs', entries: 0, collection: '{"icon":"fa-receipt"}' },
        { grpnum: 0, grpname: 'System', grpsort: 0, modnum: 3, modsort: 1, modname: 'Version History', entries: 0, collection: '{"icon":"fa-history"}' },
        { grpnum: 1, grpname: 'Content', grpsort: 1, modnum: 8, modsort: 0, modname: 'Dashboard', entries: 0, collection: '{"icon":"fa-chart-line"}' },
      ];

      const result = groupMenuItems(items);

      expect(result).toHaveLength(2);
      // Groups are sorted by grpsort
      expect(result[0].id).toBe(0);
      expect(result[0].name).toBe('System');
      expect(result[0].items).toHaveLength(2);
      expect(result[1].id).toBe(1);
      expect(result[1].name).toBe('Content');
      expect(result[1].items).toHaveLength(1);
    });

    it('should sort groups by grpsort', () => {
      const items = [
        { grpnum: 1, grpname: 'Content', grpsort: 10, modnum: 8, modsort: 0, modname: 'Dashboard', entries: 0, collection: '{}' },
        { grpnum: 0, grpname: 'System', grpsort: 0, modnum: 2, modsort: 0, modname: 'Session Logs', entries: 0, collection: '{}' },
        { grpnum: 2, grpname: 'Data', grpsort: 20, modnum: 4, modsort: 0, modname: 'Queries', entries: 0, collection: '{}' },
      ];

      const result = groupMenuItems(items);

      expect(result[0].name).toBe('System');
      expect(result[1].name).toBe('Content');
      expect(result[2].name).toBe('Data');
    });

    it('should sort items within groups by modsort', () => {
      const items = [
        { grpnum: 0, grpname: 'System', modnum: 3, modsort: 1, modname: 'Version History', entries: 0, collection: '{}' },
        { grpnum: 0, grpname: 'System', modnum: 2, modsort: 0, modname: 'Session Logs', entries: 0, collection: '{}' },
      ];

      const result = groupMenuItems(items);

      expect(result[0].items[0].managerId).toBe(2);
      expect(result[0].items[1].managerId).toBe(3);
    });

    it('should filter out non-visible items (negative Index)', () => {
      const items = [
        { grpnum: 0, grpname: 'System', modnum: 2, modsort: 0, modname: 'Session Logs', entries: 0, collection: '{"icon":"fa-receipt","index":0}' },
        { grpnum: 0, grpname: 'System', modnum: 1, modsort: 0, modname: 'Main Menu', entries: 0, collection: '{"icon":"fa-bars","index":-2}' },
        { grpnum: 0, grpname: 'System', modnum: 3, modsort: 1, modname: 'Version History', entries: 0, collection: '{"icon":"fa-history","index":0}' },
      ];

      const result = groupMenuItems(items);

      expect(result[0].items).toHaveLength(2);
    });
  });

  describe('buildManagerIconsRegistry', () => {
    it('should build icon registry from grouped menu data', () => {
      const menuData = [
        {
          id: 0,
          name: 'System',
          items: [
            { managerId: 2, name: 'Session Logs', iconHtml: '<fa fa-receipt></fa>' },
            { managerId: 3, name: 'Version History', iconHtml: '<fa fa-history></fa>' },
          ],
        },
      ];

      const result = buildManagerIconsRegistry(menuData);

      expect(result[2]).toEqual({ iconHtml: '<fa fa-receipt></fa>', name: 'Session Logs' });
      expect(result[3]).toEqual({ iconHtml: '<fa fa-history></fa>', name: 'Version History' });
    });

    it('should return empty object for empty menu', () => {
      const result = buildManagerIconsRegistry([]);
      expect(result).toEqual({});
    });
  });

  describe('getManagerIdsFromMenu', () => {
    it('should extract manager IDs from menu data', () => {
      const menuData = [
        {
          id: 0,
          items: [
            { managerId: 2 },
            { managerId: 3 },
          ],
        },
        {
          id: 1,
          items: [
            { managerId: 8 },
          ],
        },
      ];

      const result = getManagerIdsFromMenu(menuData);

      expect(result).toEqual([2, 3, 8]);
    });
  });

  describe('Cache Management', () => {
    it('should clear cache when clearMenuCache is called', () => {
      clearMenuCache();
      
      expect(localStorageMock.removeItem).toHaveBeenCalledWith(MENU_CACHE_KEY);
      expect(localStorageMock.removeItem).toHaveBeenCalledWith(MENU_CACHE_TIMESTAMP_KEY);
    });

    it('should return null from getCachedMenuData if no cache', () => {
      localStorageMock.getItem.mockReturnValue(null);
      
      const result = getCachedMenuData();
      
      expect(result).toBeNull();
    });

    it('should return cached data from getCachedMenuData', () => {
      const menuData = [{ id: 0, name: 'Test', items: [] }];
      const now = Date.now();
      
      // Need to set up the mock to return both values
      localStorageMock.getItem.mockReturnValueOnce(JSON.stringify(menuData));
      localStorageMock.getItem.mockReturnValueOnce(String(now));
      
      const result = getCachedMenuData();
      
      expect(result).toEqual(menuData);
    });
  });

  describe('fetchMenu', () => {
    it('should handle API errors', async () => {
      // Import mocked authQuery and configure it to throw
      const { authQuery } = await import('../../src/shared/conduit.js');
      authQuery.mockRejectedValueOnce(new Error('Network error'));

      const api = {};
      await expect(fetchMenu(api)).rejects.toThrow('Network error');
    });
  });
});
