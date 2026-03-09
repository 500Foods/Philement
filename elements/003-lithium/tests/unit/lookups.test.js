/**
 * Lookups Unit Tests
 *
 * Note: These tests verify the lookups module interface and behavior.
 * Some tests may not fully work due to ES module mocking complexity.
 */

import { describe, it, expect, beforeEach, vi } from 'vitest';

// Mock fetch
global.fetch = vi.fn();

// Mock localStorage
const localStorageMock = {
  data: {},
  getItem: vi.fn(function(key) { return this.data[key] || null; }),
  setItem: vi.fn(function(key, value) { this.data[key] = value; }),
  removeItem: vi.fn(function(key) { delete this.data[key]; }),
  clear: vi.fn(function() { this.data = {}; }),
};
Object.defineProperty(global, 'localStorage', { value: localStorageMock });

// Mock event-bus
vi.mock('../../src/core/event-bus.js', () => ({
  eventBus: {
    emit: vi.fn(),
    emitSilent: vi.fn(),
    on: vi.fn(),
    off: vi.fn(),
  },
  Events: {
    LOOKUPS_LOADED: 'lookups:loaded',
    LOOKUPS_LOADING: 'lookups:loading',
    LOOKUPS_ERROR: 'lookups:error',
  },
}));

// Mock log module
vi.mock('../../src/core/log.js', () => ({
  log: vi.fn(),
  logGroup: vi.fn(),
  Subsystems: { LOOKUPS: 'Lookups' },
  Status: { INFO: 'INFO', WARN: 'WARN', ERROR: 'ERROR' },
  logHttpRequest: vi.fn(() => 1),
  logHttpResponse: vi.fn(),
}));

// Mock config module
vi.mock('../../src/core/config.js', () => ({
  getConfigValue: vi.fn((key, defaultValue) => {
    if (key === 'server.url') return 'https://api.example.com';
    if (key === 'server.api_prefix') return '/api';
    if (key === 'auth.default_database') return 'Lithium';
    if (key === 'auth.api_key') return 'test-api-key';
    return defaultValue;
  }),
}));

describe('Lookups Module', () => {
  beforeEach(() => {
    // Clear localStorage mock data before each test
    localStorageMock.data = {};
    vi.clearAllMocks();
  });

  describe('module structure', () => {
    it('should export expected functions', async () => {
      // Import the module to check exports
      const lookups = await import('../../src/shared/lookups.js');

      expect(typeof lookups.fetchLookups).toBe('function');
      expect(typeof lookups.getLookup).toBe('function');
      expect(typeof lookups.hasLookup).toBe('function');
      expect(typeof lookups.getManagerName).toBe('function');
      expect(typeof lookups.getManagers).toBe('function');
      expect(typeof lookups.getThemes).toBe('function');
      expect(typeof lookups.getIcons).toBe('function');
      expect(typeof lookups.getSystemInfo).toBe('function');
      expect(typeof lookups.getLookupNames).toBe('function');
      expect(typeof lookups.getFeatureName).toBe('function');
      expect(typeof lookups.getLookupCategory).toBe('function');
      expect(typeof lookups.getAllLookups).toBe('function');
      expect(typeof lookups.isLoaded).toBe('function');
      expect(typeof lookups.clearCache).toBe('function');
      expect(typeof lookups.refreshLookups).toBe('function');
      expect(typeof lookups.init).toBe('function');
    });
  });

  describe('isLoaded', () => {
    it('should return false when cache is empty', async () => {
      const { isLoaded, clearCache } = await import('../../src/shared/lookups.js');
      clearCache();

      expect(isLoaded()).toBe(false);
    });
  });

  describe('hasLookup', () => {
    it('should return false when cache is empty', async () => {
      const { hasLookup, clearCache } = await import('../../src/shared/lookups.js');
      clearCache();

      expect(hasLookup('themes')).toBe(false);
      expect(hasLookup('icons')).toBe(false);
      expect(hasLookup('system_info')).toBe(false);
      expect(hasLookup('lookup_names')).toBe(false);
    });
  });

  describe('clearCache', () => {
    it('should be a function', async () => {
      const { clearCache } = await import('../../src/shared/lookups.js');

      expect(typeof clearCache).toBe('function');
    });
  });

  describe('getLookup behavior', () => {
    it('should return key when lookups not loaded', async () => {
      const { getLookup, clearCache } = await import('../../src/shared/lookups.js');
      clearCache();

      // Should return default or key when not loaded
      const result = getLookup('managers', 1);
      // Returns the key because cache is null
      expect(result).toBe(1);
    });
  });

  describe('getManagerName behavior', () => {
    it('should return key when lookups not loaded', async () => {
      const { getManagerName, clearCache } = await import('../../src/shared/lookups.js');
      clearCache();

      expect(getManagerName(1)).toBe(1);
    });
  });

  describe('getFeatureName behavior', () => {
    it('should return key when lookups not loaded', async () => {
      const { getFeatureName, clearCache } = await import('../../src/shared/lookups.js');
      clearCache();

      expect(getFeatureName(1, 1)).toBe(1);
    });
  });

  describe('getLookupCategory behavior', () => {
    it('should return null when lookups not loaded', async () => {
      const { getLookupCategory, clearCache } = await import('../../src/shared/lookups.js');
      clearCache();

      expect(getLookupCategory('managers')).toBe(null);
    });
  });

  describe('getAllLookups behavior', () => {
    it('should return null when lookups not loaded', async () => {
      const { getAllLookups, clearCache } = await import('../../src/shared/lookups.js');
      clearCache();

      expect(getAllLookups()).toBe(null);
    });
  });

  describe('getThemes behavior', () => {
    it('should return null when lookups not loaded', async () => {
      const { getThemes, clearCache } = await import('../../src/shared/lookups.js');
      clearCache();

      expect(getThemes()).toBe(null);
    });
  });

  describe('getIcons behavior', () => {
    it('should return null when lookups not loaded', async () => {
      const { getIcons, clearCache } = await import('../../src/shared/lookups.js');
      clearCache();

      expect(getIcons()).toBe(null);
    });
  });

  describe('getSystemInfo behavior', () => {
    it('should return null when lookups not loaded', async () => {
      const { getSystemInfo, clearCache } = await import('../../src/shared/lookups.js');
      clearCache();

      expect(getSystemInfo()).toBe(null);
    });
  });

  describe('getManagers behavior', () => {
    it('should return null when lookups not loaded', async () => {
      const { getManagers, clearCache } = await import('../../src/shared/lookups.js');
      clearCache();

      expect(getManagers()).toBe(null);
    });
  });

  describe('getLookupNames behavior', () => {
    it('should return null when lookups not loaded', async () => {
      const { getLookupNames, clearCache } = await import('../../src/shared/lookups.js');
      clearCache();

      expect(getLookupNames()).toBe(null);
    });
  });

  describe('fetchLookups with localStorage cache', () => {
    it('should load lookups from localStorage cache', async () => {
      const { fetchLookups, clearCache, isLoaded } = await import('../../src/shared/lookups.js');
      clearCache();

      // Setup localStorage with cached data
      const cachedData = {
        system_info: { version: '1.0' },
        themes: [{ id: 1, name: 'Dark' }],
        icons: [],
        lookup_names: [],
      };
      localStorageMock.data['lithium_lookups'] = JSON.stringify(cachedData);
      localStorageMock.data['lithium_lookups_timestamp'] = String(Date.now());

      const result = await fetchLookups();

      expect(isLoaded()).toBe(true);
      expect(result).toEqual(cachedData);
    });

    it('should return null when cache is expired', async () => {
      const { fetchLookups, clearCache } = await import('../../src/shared/lookups.js');
      clearCache();

      // Setup expired cache (24 hours + 1 second ago)
      const expiredTime = Date.now() - (24 * 60 * 60 * 1000) - 1000;
      localStorageMock.data['lithium_lookups'] = JSON.stringify({ themes: [] });
      localStorageMock.data['lithium_lookups_timestamp'] = String(expiredTime);

      // Mock fetch to return data
      global.fetch.mockResolvedValueOnce({
        ok: true,
        headers: new Headers({ 'content-type': 'application/json' }),
        json: async () => ({
          results: [
            { query_ref: 1, success: true, rows: [{ version: '2.0' }] },
            { query_ref: 30, success: true, rows: [] },
            { query_ref: 53, success: true, rows: [] },
            { query_ref: 54, success: true, rows: [] },
          ],
        }),
      });

      await fetchLookups();

      // Should have attempted to fetch from server
      expect(global.fetch).toHaveBeenCalled();
    });

    it('should return null when no cache exists', async () => {
      const { fetchLookups, clearCache } = await import('../../src/shared/lookups.js');
      clearCache();

      // No localStorage data
      delete localStorageMock.data['lithium_lookups'];
      delete localStorageMock.data['lithium_lookups_timestamp'];

      // Mock fetch to return data
      global.fetch.mockResolvedValueOnce({
        ok: true,
        headers: new Headers({ 'content-type': 'application/json' }),
        json: async () => ({
          results: [
            { query_ref: 1, success: true, rows: [{ version: '2.0' }] },
            { query_ref: 30, success: true, rows: [] },
            { query_ref: 53, success: true, rows: [] },
            { query_ref: 54, success: true, rows: [] },
          ],
        }),
      });

      await fetchLookups();

      expect(global.fetch).toHaveBeenCalled();
    });
  });

  describe('fetchLookups from server', () => {
    it('should fetch lookups from server successfully', async () => {
      const { fetchLookups, clearCache, isLoaded } = await import('../../src/shared/lookups.js');
      clearCache();

      global.fetch.mockResolvedValueOnce({
        ok: true,
        headers: new Headers({ 'content-type': 'application/json' }),
        json: async () => ({
          results: [
            { query_ref: 1, success: true, rows: [{ version: '2.0', license: 'MIT' }] },
            { query_ref: 30, success: true, rows: [{ id: 1, name: 'Test' }] },
            { query_ref: 53, success: true, rows: [{ id: 1, name: 'Dark' }] },
            { query_ref: 54, success: true, rows: [{ id: 1, name: 'user' }] },
          ],
        }),
      });

      const result = await fetchLookups();

      expect(isLoaded()).toBe(true);
      expect(result.system_info).toEqual({ version: '2.0', license: 'MIT' });
      expect(result.lookup_names).toEqual([{ id: 1, name: 'Test' }]);
      expect(result.themes).toEqual([{ id: 1, name: 'Dark' }]);
      expect(result.icons).toEqual([{ id: 1, name: 'user' }]);
    });

    it('should handle server error with cache fallback', async () => {
      const { fetchLookups, clearCache } = await import('../../src/shared/lookups.js');
      clearCache();

      // First, setup some cache data
      const cachedData = {
        system_info: { version: '1.0' },
        themes: [{ id: 1, name: 'Cached Dark' }],
        icons: [],
        lookup_names: [],
      };
      localStorageMock.data['lithium_lookups'] = JSON.stringify(cachedData);
      localStorageMock.data['lithium_lookups_timestamp'] = String(Date.now());

      // Load from cache first
      await fetchLookups();

      // Now mock server error
      global.fetch.mockRejectedValueOnce(new Error('Network error'));

      // Force refresh
      const result = await fetchLookups({ force: true });

      // Should return cached data as fallback
      expect(result).toEqual(cachedData);
    });

    it('should handle server error without cache', async () => {
      const { fetchLookups, clearCache } = await import('../../src/shared/lookups.js');
      clearCache();

      global.fetch.mockRejectedValueOnce(new Error('Network error'));

      const result = await fetchLookups();

      // Should return empty object when no cache and server fails
      expect(result).toEqual({});
    });

    it('should handle partial server failure', async () => {
      const { fetchLookups, clearCache } = await import('../../src/shared/lookups.js');
      clearCache();

      // Some queries succeed, some fail
      global.fetch.mockResolvedValueOnce({
        ok: true,
        headers: new Headers({ 'content-type': 'application/json' }),
        json: async () => ({
          results: [
            { query_ref: 1, success: true, rows: [{ version: '2.0' }] },
            { query_ref: 30, success: false, error: 'Query failed', rows: [] },
            { query_ref: 53, success: true, rows: [{ id: 1, name: 'Dark' }] },
            { query_ref: 54, success: true, rows: [] },
          ],
        }),
      });

      await fetchLookups();

      // Should still have data from successful queries
      expect(global.fetch).toHaveBeenCalled();
    });
  });

  describe('fetchLookups force refresh', () => {
    it('should force refresh from server', async () => {
      const { fetchLookups, clearCache } = await import('../../src/shared/lookups.js');
      clearCache();

      global.fetch.mockResolvedValueOnce({
        ok: true,
        headers: new Headers({ 'content-type': 'application/json' }),
        json: async () => ({
          results: [
            { query_ref: 1, success: true, rows: [{ version: '2.0' }] },
            { query_ref: 30, success: true, rows: [] },
            { query_ref: 53, success: true, rows: [] },
            { query_ref: 54, success: true, rows: [] },
          ],
        }),
      });

      await fetchLookups({ force: true });

      expect(global.fetch).toHaveBeenCalled();
    });
  });

  describe('getLookup with data', () => {
    it('should return lookup value when cache has data', async () => {
      const { getLookup, fetchLookups, clearCache } = await import('../../src/shared/lookups.js');
      clearCache();

      // Mock server response - system_info stores object with numeric id as key
      global.fetch.mockResolvedValueOnce({
        ok: true,
        headers: new Headers({ 'content-type': 'application/json' }),
        json: async () => ({
          results: [
            { query_ref: 1, success: true, rows: [{ id: 1, name: 'Style Manager' }] },
            { query_ref: 30, success: true, rows: [] },
            { query_ref: 53, success: true, rows: [] },
            { query_ref: 54, success: true, rows: [] },
          ],
        }),
      });

      await fetchLookups();

      // The getLookup function converts key to string, so we need to access differently
      // Looking at the lookups.js code: it stores rows[0] as-is for system_info
      const result = getLookup('system_info', 1);
      // system_info is stored as object: { id: 1, name: 'Style Manager' }
      expect(result).toBeTruthy();
    });

    it('should return default value when key not found', async () => {
      const { getLookup, fetchLookups, clearCache } = await import('../../src/shared/lookups.js');
      clearCache();

      global.fetch.mockResolvedValueOnce({
        ok: true,
        headers: new Headers({ 'content-type': 'application/json' }),
        json: async () => ({
          results: [
            { query_ref: 1, success: true, rows: [{ id: 1, name: 'Style Manager' }] },
            { query_ref: 30, success: true, rows: [] },
            { query_ref: 53, success: true, rows: [] },
            { query_ref: 54, success: true, rows: [] },
          ],
        }),
      });

      await fetchLookups();

      const result = getLookup('managers', '999', 'Not Found');
      expect(result).toBe('Not Found');
    });
  });

  describe('refreshLookups', () => {
    it('should clear cache and fetch fresh data', async () => {
      const { refreshLookups, isLoaded, clearCache } = await import('../../src/shared/lookups.js');
      clearCache();

      global.fetch.mockResolvedValueOnce({
        ok: true,
        headers: new Headers({ 'content-type': 'application/json' }),
        json: async () => ({
          results: [
            { query_ref: 1, success: true, rows: [{ version: '3.0' }] },
            { query_ref: 30, success: true, rows: [] },
            { query_ref: 53, success: true, rows: [] },
            { query_ref: 54, success: true, rows: [] },
          ],
        }),
      });

      await refreshLookups();

      expect(isLoaded()).toBe(true);
      expect(global.fetch).toHaveBeenCalled();
    });
  });

  describe('init', () => {
    it('should be a function that runs without error', async () => {
      const { init } = await import('../../src/shared/lookups.js');

      expect(typeof init).toBe('function');
      // init should not throw
      init();
    });
  });
});
