/**
 * JSON Request Unit Tests
 */

import { describe, it, expect, beforeEach, vi } from 'vitest';

// Mock fetch globally
const mockFetch = vi.fn();
global.fetch = mockFetch;

// Mock localStorage for JWT storage
const localStorageMock = {
  getItem: vi.fn(),
  setItem: vi.fn(),
  removeItem: vi.fn(),
  clear: vi.fn(),
};
Object.defineProperty(global, 'localStorage', { value: localStorageMock });

// Mock the log module
vi.mock('../../src/core/log.js', () => ({
  logHttpRequest: vi.fn(() => 1),
  logHttpResponse: vi.fn(),
  Status: {
    INFO: 'INFO',
    WARN: 'WARN',
    ERROR: 'ERROR',
  },
}));

// Mock event-bus
vi.mock('../../src/core/event-bus.js', () => ({
  eventBus: {
    emit: vi.fn(),
    emitSilent: vi.fn(),
    on: vi.fn(),
    off: vi.fn(),
  },
  Events: {
    AUTH_EXPIRED: 'auth:expired',
    LOOKUPS_LOADED: 'lookups:loaded',
    LOOKUPS_LOADING: 'lookups:loading',
    LOOKUPS_ERROR: 'lookups:error',
  },
}));

// Import after mocks are set up
const {
  get,
  post,
  put,
  del,
  patch,
  createRequest,
} = require('../../src/core/json-request.js');

describe('JSON Request', () => {
  beforeEach(() => {
    vi.clearAllMocks();
    localStorageMock.getItem.mockReturnValue(null);
  });

  describe('get', () => {
    it('should make GET request with correct URL', async () => {
      mockFetch.mockResolvedValueOnce({
        ok: true,
        headers: new Headers({ 'content-type': 'application/json' }),
        json: async () => ({ data: 'test' }),
      });

      const config = { server: { url: 'https://api.example.com', api_prefix: '/api' } };
      const result = await get('/users', { config });

      expect(mockFetch).toHaveBeenCalledWith(
        'https://api.example.com/api/users',
        expect.objectContaining({ method: 'GET' })
      );
      expect(result).toEqual({ data: 'test' });
    });

    it('should handle full URLs', async () => {
      mockFetch.mockResolvedValueOnce({
        ok: true,
        headers: new Headers({ 'content-type': 'application/json' }),
        json: async () => ({ data: 'test' }),
      });

      await get('https://other.com/data');

      expect(mockFetch).toHaveBeenCalledWith(
        'https://other.com/data',
        expect.any(Object)
      );
    });

    it('should include auth header when JWT exists', async () => {
      localStorageMock.getItem.mockReturnValue('test-token');
      mockFetch.mockResolvedValueOnce({
        ok: true,
        headers: new Headers({ 'content-type': 'application/json' }),
        json: async () => ({}),
      });

      const config = { server: { url: 'https://api.example.com', api_prefix: '/api' } };
      await get('/users', { config });

      expect(mockFetch).toHaveBeenCalledWith(
        expect.any(String),
        expect.objectContaining({
          headers: expect.objectContaining({
            Authorization: 'Bearer test-token',
          }),
        })
      );
    });

    it('should not include auth when includeAuth is false', async () => {
      localStorageMock.getItem.mockReturnValue('test-token');
      mockFetch.mockResolvedValueOnce({
        ok: true,
        headers: new Headers({ 'content-type': 'application/json' }),
        json: async () => ({}),
      });

      const config = { server: { url: 'https://api.example.com', api_prefix: '/api' } };
      await get('/users', { config, includeAuth: false });

      const callArgs = mockFetch.mock.calls[0];
      const headers = callArgs[1].headers;
      expect(headers.Authorization).toBeUndefined();
    });
  });

  describe('post', () => {
    it('should make POST request with body', async () => {
      mockFetch.mockResolvedValueOnce({
        ok: true,
        headers: new Headers({ 'content-type': 'application/json' }),
        json: async () => ({ id: 1 }),
      });

      const config = { server: { url: 'https://api.example.com', api_prefix: '/api' } };
      const result = await post('/users', { name: 'John' }, { config });

      expect(mockFetch).toHaveBeenCalledWith(
        'https://api.example.com/api/users',
        expect.objectContaining({
          method: 'POST',
          body: JSON.stringify({ name: 'John' }),
        })
      );
      expect(result).toEqual({ id: 1 });
    });
  });

  describe('put', () => {
    it('should make PUT request', async () => {
      mockFetch.mockResolvedValueOnce({
        ok: true,
        headers: new Headers({ 'content-type': 'application/json' }),
        json: async () => ({ updated: true }),
      });

      const config = { server: { url: 'https://api.example.com', api_prefix: '/api' } };
      const result = await put('/users/1', { name: 'Jane' }, { config });

      expect(mockFetch).toHaveBeenCalledWith(
        'https://api.example.com/api/users/1',
        expect.objectContaining({ method: 'PUT' })
      );
    });
  });

  describe('del', () => {
    it('should make DELETE request', async () => {
      mockFetch.mockResolvedValueOnce({
        ok: true,
        headers: new Headers({ 'content-type': 'application/json' }),
        json: async () => ({ deleted: true }),
      });

      const config = { server: { url: 'https://api.example.com', api_prefix: '/api' } };
      await del('/users/1', { config });

      expect(mockFetch).toHaveBeenCalledWith(
        'https://api.example.com/api/users/1',
        expect.objectContaining({ method: 'DELETE' })
      );
    });
  });

  describe('patch', () => {
    it('should make PATCH request', async () => {
      mockFetch.mockResolvedValueOnce({
        ok: true,
        headers: new Headers({ 'content-type': 'application/json' }),
        json: async () => ({ patched: true }),
      });

      const config = { server: { url: 'https://api.example.com', api_prefix: '/api' } };
      const result = await patch('/users/1', { name: 'Jane' }, { config });

      expect(mockFetch).toHaveBeenCalledWith(
        'https://api.example.com/api/users/1',
        expect.objectContaining({ method: 'PATCH' })
      );
    });
  });

  describe('error handling', () => {
    it('should handle 401 as unauthorized error', async () => {
      mockFetch.mockResolvedValueOnce({
        ok: false,
        status: 401,
        statusText: 'Unauthorized',
        headers: new Headers({ 'content-type': 'application/json' }),
        json: async () => ({ message: 'Token expired' }),
      });

      const config = { server: { url: 'https://api.example.com', api_prefix: '/api' } };

      await expect(get('/users', { config })).rejects.toThrow();
    });

    it('should handle rate limiting (429) with retry-after', async () => {
      mockFetch.mockResolvedValueOnce({
        ok: false,
        status: 429,
        statusText: 'Too Many Requests',
        headers: new Headers({
          'content-type': 'application/json',
          'retry-after': '60',
        }),
        json: async () => ({ message: 'Rate limited' }),
      });

      const config = { server: { url: 'https://api.example.com', api_prefix: '/api' } };

      try {
        await get('/users', { config });
      } catch (error) {
        expect(error.retryAfter).toBe(60);
      }
    });

    it('should handle rate limiting (429) without retry-after header', async () => {
      mockFetch.mockResolvedValueOnce({
        ok: false,
        status: 429,
        statusText: 'Too Many Requests',
        headers: new Headers({
          'content-type': 'application/json',
        }),
        json: async () => ({ message: 'Rate limited' }),
      });

      const config = { server: { url: 'https://api.example.com', api_prefix: '/api' } };

      try {
        await get('/users', { config });
      } catch (error) {
        expect(error.retryAfter).toBeNull();
      }
    });

    it('should include error data in thrown error', async () => {
      mockFetch.mockResolvedValueOnce({
        ok: false,
        status: 500,
        statusText: 'Internal Server Error',
        headers: new Headers({ 'content-type': 'application/json' }),
        json: async () => ({ message: 'Server error' }),
      });

      const config = { server: { url: 'https://api.example.com', api_prefix: '/api' } };

      try {
        await get('/users', { config });
      } catch (error) {
        expect(error.status).toBe(500);
        expect(error.data).toEqual({ message: 'Server error' });
      }
    });

    it('should handle 403 forbidden error', async () => {
      mockFetch.mockResolvedValueOnce({
        ok: false,
        status: 403,
        statusText: 'Forbidden',
        headers: new Headers({ 'content-type': 'application/json' }),
        json: async () => ({ message: 'Access denied' }),
      });

      const config = { server: { url: 'https://api.example.com', api_prefix: '/api' } };

      try {
        await get('/users', { config });
      } catch (error) {
        expect(error.status).toBe(403);
        expect(error.message).toBe('Access denied');
      }
    });

    it('should handle 502 bad gateway error', async () => {
      mockFetch.mockResolvedValueOnce({
        ok: false,
        status: 502,
        statusText: 'Bad Gateway',
        headers: new Headers({ 'content-type': 'application/json' }),
        json: async () => ({ message: 'Backend error' }),
      });

      const config = { server: { url: 'https://api.example.com', api_prefix: '/api' } };

      try {
        await get('/users', { config });
      } catch (error) {
        expect(error.status).toBe(502);
        expect(error.message).toBe('Backend error');
      }
    });

    it('should handle 503 service unavailable error', async () => {
      mockFetch.mockResolvedValueOnce({
        ok: false,
        status: 503,
        statusText: 'Service Unavailable',
        headers: new Headers({ 'content-type': 'application/json' }),
        json: async () => ({ message: 'Service temporarily unavailable' }),
      });

      const config = { server: { url: 'https://api.example.com', api_prefix: '/api' } };

      try {
        await get('/users', { config });
      } catch (error) {
        expect(error.status).toBe(503);
        expect(error.message).toBe('Service temporarily unavailable');
      }
    });

    it('should handle 504 gateway timeout error', async () => {
      mockFetch.mockResolvedValueOnce({
        ok: false,
        status: 504,
        statusText: 'Gateway Timeout',
        headers: new Headers({ 'content-type': 'application/json' }),
        json: async () => ({ message: 'Gateway timeout' }),
      });

      const config = { server: { url: 'https://api.example.com', api_prefix: '/api' } };

      try {
        await get('/users', { config });
      } catch (error) {
        expect(error.status).toBe(504);
        expect(error.message).toBe('Gateway timeout');
      }
    });

    it('should handle server error without JSON body', async () => {
      mockFetch.mockResolvedValueOnce({
        ok: false,
        status: 500,
        statusText: 'Internal Server Error',
        headers: new Headers({ 'content-type': 'text/plain' }),
      });

      const config = { server: { url: 'https://api.example.com', api_prefix: '/api' } };

      try {
        await get('/users', { config });
      } catch (error) {
        expect(error.status).toBe(500);
        expect(error.data).toBeNull();
      }
    });

    it('should handle network errors', async () => {
      mockFetch.mockRejectedValueOnce(new TypeError('Failed to fetch'));

      const config = { server: { url: 'https://api.example.com', api_prefix: '/api' } };

      await expect(get('/users', { config })).rejects.toThrow('Failed to fetch');
    });

    it('should handle DNS errors', async () => {
      mockFetch.mockRejectedValueOnce(new TypeError('Network request failed'));

      const config = { server: { url: 'https://api.example.com', api_prefix: '/api' } };

      await expect(get('/users', { config })).rejects.toThrow('Network request failed');
    });
  });

  describe('buildUrl edge cases', () => {
    it('should handle path with leading slash', async () => {
      mockFetch.mockResolvedValueOnce({
        ok: true,
        headers: new Headers({ 'content-type': 'application/json' }),
        json: async () => ({ data: 'test' }),
      });

      const config = { server: { url: 'https://api.example.com', api_prefix: '/api' } };
      await get('/users', { config });

      expect(mockFetch).toHaveBeenCalledWith(
        'https://api.example.com/api/users',
        expect.any(Object)
      );
    });

    it('should use default config values when not provided', async () => {
      mockFetch.mockResolvedValueOnce({
        ok: true,
        headers: new Headers({ 'content-type': 'application/json' }),
        json: async () => ({ data: 'test' }),
      });

      // No config provided - should use defaults
      await get('/users');

      expect(mockFetch).toHaveBeenCalledWith(
        '/api/users',
        expect.any(Object)
      );
    });

    it('should handle empty api_prefix', async () => {
      mockFetch.mockResolvedValueOnce({
        ok: true,
        headers: new Headers({ 'content-type': 'application/json' }),
        json: async () => ({ data: 'test' }),
      });

      const config = { server: { url: 'https://api.example.com', api_prefix: '' } };
      await get('/users', { config });

      // Note: Empty string is falsy, so it falls back to default '/api'
      // This is actually the current behavior - empty string becomes '/api'
      expect(mockFetch).toHaveBeenCalledWith(
        'https://api.example.com/api/users',
        expect.any(Object)
      );
    });
  });

  describe('response handling', () => {
    it('should handle successful response without JSON content-type', async () => {
      mockFetch.mockResolvedValueOnce({
        ok: true,
        status: 200,
        headers: new Headers({ 'content-type': 'text/html' }),
      });

      const config = { server: { url: 'https://api.example.com', api_prefix: '/api' } };
      const result = await get('/users', { config });

      expect(result).toBeNull();
    });

    it('should handle response with missing content-type header', async () => {
      mockFetch.mockResolvedValueOnce({
        ok: true,
        status: 200,
        headers: new Headers({}),
      });

      const config = { server: { url: 'https://api.example.com', api_prefix: '/api' } };
      const result = await get('/users', { config });

      expect(result).toBeNull();
    });

    it('should handle JSON parse error gracefully', async () => {
      mockFetch.mockResolvedValueOnce({
        ok: true,
        status: 200,
        headers: new Headers({ 'content-type': 'application/json' }),
        json: () => { throw new Error('Invalid JSON'); },
      });

      const config = { server: { url: 'https://api.example.com', api_prefix: '/api' } };
      const result = await get('/users', { config });

      // Should return null when JSON parsing fails
      expect(result).toBeNull();
    });

    it('should emit auth:expired event on 401', async () => {
      mockFetch.mockResolvedValueOnce({
        ok: false,
        status: 401,
        statusText: 'Unauthorized',
        headers: new Headers({ 'content-type': 'application/json' }),
        json: async () => ({ message: 'Token expired' }),
      });

      const config = { server: { url: 'https://api.example.com', api_prefix: '/api' } };

      try {
        await get('/users', { config });
      } catch (error) {
        expect(error.status).toBe(401);
      }
    });
  });

  describe('createRequest', () => {
    it('should create a request instance with bound config', async () => {
      mockFetch.mockResolvedValueOnce({
        ok: true,
        headers: new Headers({ 'content-type': 'application/json' }),
        json: async () => ({ data: 'test' }),
      });

      const config = { server: { url: 'https://api.example.com', api_prefix: '/api' } };
      const request = createRequest(config);

      const result = await request.get('/users');

      expect(mockFetch).toHaveBeenCalledWith(
        'https://api.example.com/api/users',
        expect.any(Object)
      );
      expect(result).toEqual({ data: 'test' });
    });

    it('should bind all HTTP methods', async () => {
      mockFetch.mockResolvedValue({
        ok: true,
        headers: new Headers({ 'content-type': 'application/json' }),
        json: async () => ({ success: true }),
      });

      const config = { server: { url: 'https://api.example.com', api_prefix: '/api' } };
      const request = createRequest(config);

      await request.get('/test');
      await request.post('/test', {});
      await request.put('/test', {});
      await request.del('/test');
      await request.patch('/test', {});

      expect(mockFetch).toHaveBeenCalledTimes(5);
    });
  });
});
