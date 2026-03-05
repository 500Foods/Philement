/**
 * Config Unit Tests
 */

import { describe, it, expect, beforeEach, vi } from 'vitest';
import {
  loadConfig,
  getConfig,
  getConfigValue,
  clearConfig,
  isConfigLoaded,
} from '../../src/core/config.js';

// Mock fetch globally
global.fetch = vi.fn();

describe('Config', () => {
  beforeEach(() => {
    clearConfig();
    vi.clearAllMocks();
  });

  describe('loadConfig', () => {
    it('should load and merge config from server', async () => {
      const mockConfig = {
        server: { url: 'https://example.com' },
        app: { name: 'Test App' },
      };
      
      global.fetch.mockResolvedValueOnce({
        ok: true,
        json: async () => mockConfig,
      });

      const config = await loadConfig();

      expect(config.server.url).toBe('https://example.com');
      expect(config.server.api_prefix).toBe('/api'); // from defaults
      expect(config.app.name).toBe('Test App');
      expect(config.app.tagline).toBe('A Philement Project'); // from defaults
    });

    it('should use defaults when config file not found', async () => {
      global.fetch.mockResolvedValueOnce({
        ok: false,
        status: 404,
      });

      const config = await loadConfig();

      expect(config.server.url).toBe('http://localhost:8080');
      expect(config.app.name).toBe('Lithium');
    });

    it('should use defaults when fetch fails', async () => {
      global.fetch.mockRejectedValueOnce(new Error('Network error'));

      const config = await loadConfig();

      expect(config.server.url).toBe('http://localhost:8080');
      expect(config.app.name).toBe('Lithium');
    });

    it('should return cached config on subsequent calls', async () => {
      global.fetch.mockResolvedValueOnce({
        ok: true,
        json: async () => ({ app: { name: 'First' } }),
      });

      await loadConfig();
      const config = await loadConfig();

      expect(global.fetch).toHaveBeenCalledTimes(1);
      expect(config.app.name).toBe('First');
    });
  });

  describe('getConfig', () => {
    it('should return defaults when config not loaded', () => {
      const config = getConfig();
      
      expect(config.server.url).toBe('http://localhost:8080');
    });

    it('should return loaded config', async () => {
      global.fetch.mockResolvedValueOnce({
        ok: true,
        json: async () => ({ app: { name: 'Loaded' } }),
      });

      await loadConfig();
      const config = getConfig();

      expect(config.app.name).toBe('Loaded');
    });
  });

  describe('getConfigValue', () => {
    beforeEach(async () => {
      global.fetch.mockResolvedValueOnce({
        ok: true,
        json: async () => ({
          server: { url: 'https://test.com', port: 3000 },
          app: { name: 'Test' },
        }),
      });
      await loadConfig();
    });

    it('should get nested value by dot notation', () => {
      expect(getConfigValue('server.url')).toBe('https://test.com');
      expect(getConfigValue('server.port')).toBe(3000);
    });

    it('should return default value for missing path', () => {
      expect(getConfigValue('missing.path', 'default')).toBe('default');
    });

    it('should return default for null/undefined values', () => {
      expect(getConfigValue('server.unknown', 'fallback')).toBe('fallback');
    });

    it('should handle root-level keys', () => {
      expect(getConfigValue('app.name')).toBe('Test');
    });
  });

  describe('clearConfig', () => {
    it('should clear cached config', async () => {
      global.fetch.mockResolvedValueOnce({
        ok: true,
        json: async () => ({ app: { name: 'Test' } }),
      });

      await loadConfig();
      expect(isConfigLoaded()).toBe(true);

      clearConfig();
      expect(isConfigLoaded()).toBe(false);
    });
  });

  describe('isConfigLoaded', () => {
    it('should return false before config loaded', () => {
      expect(isConfigLoaded()).toBe(false);
    });

    it('should return true after config loaded', async () => {
      global.fetch.mockResolvedValueOnce({
        ok: true,
        json: async () => ({}),
      });

      await loadConfig();
      expect(isConfigLoaded()).toBe(true);
    });
  });
});
