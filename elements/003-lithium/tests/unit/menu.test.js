/**
 * Menu Service Unit Tests
 */

import { describe, it, expect, beforeEach, vi } from 'vitest';
import {
  getMenu,
  fetchMenu,
  clearMenuCache,
  buildManagerIconsRegistry,
  getManagerIdsFromMenu,
  getCachedMenuData,
  parseCollection,
  groupMenuItems,
  MENU_CACHE_KEY,
  MENU_CACHE_TIMESTAMP_KEY,
  CACHE_TTL_MS,
} from '../../src/shared/menu.js';

// Mock conduit.js for fetchMenu tests
vi.mock('../../src/shared/conduit.js', () => ({
  authQuery: vi.fn(),
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
    vi.clearAllMocks();
  });

  describe('parseCollection', () => {
    it('should parse collection JSON with icon HTML', () => {
      const collection = '{"Icon":"<i class=\'fa fa-fw fa-xl fa-receipt\'></i>"}';
      const result = parseCollection(collection);
      
      expect(result.icon).toBe('fa-receipt');
      expect(result.visible).toBe(true);
    });

    it('should parse collection JSON with Icon and Index', () => {
      const collection = '{"Icon":"<i class=\'fa fa-history\'></i>","Index":0}';
      const result = parseCollection(collection);
      
      expect(result.icon).toBe('fa-history');
      expect(result.index).toBe(0);
      expect(result.visible).toBe(true);
    });

    it('should handle invalid JSON gracefully', () => {
      const collection = 'invalid json';
      const result = parseCollection(collection);
      
      expect(result.icon).toBe('fa-cube');
      expect(result.index).toBe(0);
      expect(result.visible).toBe(true);
    });

    it('should handle empty collection', () => {
      const result = parseCollection('');
      
      expect(result.icon).toBe('fa-cube');
      expect(result.visible).toBe(true);
    });

    it('should extract icon from complex HTML', () => {
      const collection = '{"Icon":"<i class=\'fa fa-fw fa-xl fa-chart-line\'></i>"}';
      const result = parseCollection(collection);
      
      expect(result.icon).toBe('fa-chart-line');
    });

    it('should mark items with negative Index as not visible', () => {
      const collection = '{"Icon":"<i class=\'fa fa-bars\'></i>","Index":-2}';
      const result = parseCollection(collection);
      
      expect(result.icon).toBe('fa-bars');
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
          collection: '{"Icon":"<i class=\'fa fa-receipt\'></i>"}',
        },
        {
          grpname: 'System',
          grpnum: 0,
          grpsort: 0,
          modname: 'Version History',
          modnum: 3,
          modsort: 1,
          entries: 5,
          collection: '{"Icon":"<i class=\'fa fa-history\'></i>"}',
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

    it('should filter out items with negative Index', () => {
      const rawData = [
        {
          grpname: 'System',
          grpnum: 0,
          modname: 'Session Logs',
          modnum: 2,
          collection: '{"Icon":"<i class=\'fa fa-receipt\'></i>","Index":0}',
        },
        {
          grpname: 'Hidden',
          grpnum: 0,
          modname: 'Main Menu',
          modnum: 1,
          collection: '{"Icon":"<i class=\'fa fa-bars\'></i>","Index":-2}',
        },
        {
          grpname: 'Hidden',
          grpnum: 0,
          modname: 'Login',
          modnum: 99,
          collection: '{"Icon":"<i class=\'fa fa-sign-in\'></i>","Index":-1}',
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
        { grpnum: 0, grpname: 'System', grpsort: 0, modnum: 2, modsort: 0, modname: 'Session Logs', entries: 0, collection: '{"Icon":"fa-receipt"}' },
        { grpnum: 0, grpname: 'System', grpsort: 0, modnum: 3, modsort: 1, modname: 'Version History', entries: 0, collection: '{"Icon":"fa-history"}' },
        { grpnum: 1, grpname: 'Content', grpsort: 1, modnum: 8, modsort: 0, modname: 'Dashboard', entries: 0, collection: '{"Icon":"fa-chart-line"}' },
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
        { grpnum: 0, grpname: 'System', modnum: 2, modsort: 0, modname: 'Session Logs', entries: 0, collection: '{"Icon":"fa-receipt","Index":0}' },
        { grpnum: 0, grpname: 'System', modnum: 1, modsort: 0, modname: 'Main Menu', entries: 0, collection: '{"Icon":"fa-bars","Index":-2}' },
        { grpnum: 0, grpname: 'System', modnum: 3, modsort: 1, modname: 'Version History', entries: 0, collection: '{"Icon":"fa-history","Index":0}' },
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
            { managerId: 2, name: 'Session Logs', icon: 'fa-receipt' },
            { managerId: 3, name: 'Version History', icon: 'fa-history' },
          ],
        },
      ];

      const result = buildManagerIconsRegistry(menuData);

      expect(result[2]).toEqual({ icon: 'fa-receipt', name: 'Session Logs' });
      expect(result[3]).toEqual({ icon: 'fa-history', name: 'Version History' });
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
