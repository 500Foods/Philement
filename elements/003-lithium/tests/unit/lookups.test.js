/**
 * Lookups Unit Tests
 */

import { describe, it, expect, beforeEach, vi } from 'vitest';

// Mock json-request module
const mockGet = vi.fn();
vi.mock('../../src/core/json-request.js', () => ({
  get: mockGet,
}));

// Mock event bus
const mockEventBus = {
  on: vi.fn(),
};
vi.mock('../../src/core/event-bus.js', () => ({
  eventBus: mockEventBus,
  Events: {
    LOCALE_CHANGED: 'locale:changed',
    AUTH_LOGOUT: 'auth:logout',
  },
}));

// Import after mocks are set up
const {
  fetchLookups,
  getLookup,
  getManagerName,
  getFeatureName,
  getLookupCategory,
  getAllLookups,
  isLoaded,
  clearCache,
  refreshLookups,
  init,
} = require('../../src/shared/lookups.js');

describe('Lookups', () => {
  beforeEach(() => {
    vi.clearAllMocks();
    clearCache();
  });

  describe('fetchLookups', () => {
    it('should fetch and cache lookups from server', async () => {
      const mockData = {
        managers: { 1: 'Style Manager', 2: 'Profile Manager' },
        features: { 1: 'View', 2: 'Edit' },
      };
      mockGet.mockResolvedValueOnce(mockData);

      const result = await fetchLookups();

      expect(mockGet).toHaveBeenCalledWith('/lookups');
      expect(result).toEqual(mockData);
      expect(isLoaded()).toBe(true);
    });

    it('should return cached data without fetching again', async () => {
      const mockData = { managers: { 1: 'Test' } };
      mockGet.mockResolvedValueOnce(mockData);

      await fetchLookups();
      const result = await fetchLookups();

      expect(mockGet).toHaveBeenCalledTimes(1);
      expect(result).toEqual(mockData);
    });

    it('should force refresh when force option is true', async () => {
      const mockData1 = { managers: { 1: 'First' } };
      const mockData2 = { managers: { 1: 'Second' } };
      mockGet
        .mockResolvedValueOnce(mockData1)
        .mockResolvedValueOnce(mockData2);

      await fetchLookups();
      await fetchLookups({ force: true });

      expect(mockGet).toHaveBeenCalledTimes(2);
    });

    it('should return empty cache on error', async () => {
      mockGet.mockRejectedValueOnce(new Error('Network error'));

      const result = await fetchLookups();

      expect(result).toEqual({});
      expect(isLoaded()).toBe(true);
    });

    it('should handle missing data gracefully', async () => {
      mockGet.mockResolvedValueOnce(null);

      const result = await fetchLookups();

      expect(result).toEqual({});
    });
  });

  describe('getLookup', () => {
    it('should return value from cache', async () => {
      mockGet.mockResolvedValueOnce({
        managers: { 1: 'Style Manager' },
      });
      await fetchLookups();

      expect(getLookup('managers', 1)).toBe('Style Manager');
    });

    it('should return default value when category not found', async () => {
      mockGet.mockResolvedValueOnce({});
      await fetchLookups();

      expect(getLookup('missing', 1, 'default')).toBe('default');
    });

    it('should return key when value not found', async () => {
      mockGet.mockResolvedValueOnce({ managers: {} });
      await fetchLookups();

      expect(getLookup('managers', 99)).toBe(99);
    });

    it('should warn and return default when cache not loaded', () => {
      const consoleSpy = vi.spyOn(console, 'warn').mockImplementation(() => {});

      expect(getLookup('managers', 1, 'fallback')).toBe('fallback');
      expect(consoleSpy).toHaveBeenCalledWith('Lookups not loaded yet');

      consoleSpy.mockRestore();
    });
  });

  describe('getManagerName', () => {
    it('should get manager name from managers category', async () => {
      mockGet.mockResolvedValueOnce({
        managers: { 1: 'Style Manager', 2: 'Profile Manager' },
      });
      await fetchLookups();

      expect(getManagerName(1)).toBe('Style Manager');
      expect(getManagerName(2)).toBe('Profile Manager');
    });

    it('should use default name parameter', async () => {
      mockGet.mockResolvedValueOnce({ managers: {} });
      await fetchLookups();

      expect(getManagerName(99, 'Unknown')).toBe('Unknown');
    });
  });

  describe('getFeatureName', () => {
    it('should get feature name from features_{managerId} category', async () => {
      mockGet.mockResolvedValueOnce({
        features_1: { 1: 'View', 2: 'Edit', 3: 'Delete' },
      });
      await fetchLookups();

      expect(getFeatureName(1, 1)).toBe('View');
      expect(getFeatureName(1, 2)).toBe('Edit');
    });

    it('should use default name parameter', async () => {
      mockGet.mockResolvedValueOnce({ features_1: {} });
      await fetchLookups();

      expect(getFeatureName(1, 99, 'Unknown Feature')).toBe('Unknown Feature');
    });
  });

  describe('getLookupCategory', () => {
    it('should return entire category object', async () => {
      mockGet.mockResolvedValueOnce({
        managers: { 1: 'Style', 2: 'Profile' },
        features: { 1: 'View' },
      });
      await fetchLookups();

      expect(getLookupCategory('managers')).toEqual({ 1: 'Style', 2: 'Profile' });
      expect(getLookupCategory('features')).toEqual({ 1: 'View' });
    });

    it('should return null for missing category', async () => {
      mockGet.mockResolvedValueOnce({});
      await fetchLookups();

      expect(getLookupCategory('missing')).toBe(null);
    });

    it('should warn when cache not loaded', () => {
      const consoleSpy = vi.spyOn(console, 'warn').mockImplementation(() => {});

      expect(getLookupCategory('managers')).toBe(null);
      expect(consoleSpy).toHaveBeenCalledWith('Lookups not loaded yet');

      consoleSpy.mockRestore();
    });
  });

  describe('getAllLookups', () => {
    it('should return all cached lookups', async () => {
      const mockData = { managers: { 1: 'Test' }, features: {} };
      mockGet.mockResolvedValueOnce(mockData);
      await fetchLookups();

      expect(getAllLookups()).toEqual(mockData);
    });

    it('should return null when not loaded', () => {
      expect(getAllLookups()).toBe(null);
    });
  });

  describe('clearCache', () => {
    it('should clear cached lookups', async () => {
      mockGet.mockResolvedValueOnce({ managers: { 1: 'Test' } });
      await fetchLookups();
      expect(isLoaded()).toBe(true);

      clearCache();

      expect(isLoaded()).toBe(false);
    });
  });

  describe('refreshLookups', () => {
    it('should clear cache and fetch fresh data', async () => {
      const mockData = { managers: { 1: 'Fresh' } };
      mockGet.mockResolvedValueOnce(mockData);

      await refreshLookups();

      expect(isLoaded()).toBe(true);
      expect(getLookup('managers', 1)).toBe('Fresh');
    });
  });

  describe('init', () => {
    it('should set up event listeners', () => {
      init();

      expect(mockEventBus.on).toHaveBeenCalledWith('locale:changed', expect.any(Function));
      expect(mockEventBus.on).toHaveBeenCalledWith('auth:logout', expect.any(Function));
    });
  });
});
