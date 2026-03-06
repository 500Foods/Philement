/**
 * Lookups Unit Tests
 *
 * Note: These tests verify the lookups module interface and behavior.
 * Some tests may not fully work due to ES module mocking complexity.
 */

import { describe, it, expect, beforeEach } from 'vitest';

describe('Lookups Module', () => {
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
});
