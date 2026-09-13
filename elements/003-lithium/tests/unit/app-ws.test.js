/**
 * Application WebSocket Client — Unit Tests
 *
 * Tests key hygiene for app-ws.js: no hardcoded key fallback,
 * URL logging redaction, and visible failure when key is missing.
 */

import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest';

vi.mock('../../src/core/icons.js', () => ({
  processIcons: vi.fn(),
}));

const logMock = vi.hoisted(() => ({
  log: vi.fn(),
  Subsystems: { WEBSOCKET: 'WebSocket' },
  Status: { DEBUG: 'DEBUG', INFO: 'INFO', ERROR: 'ERROR' },
}));

vi.mock('../../src/core/log.js', () => logMock);

const configMock = vi.hoisted(() => {
  const mockFn = vi.fn();
  mockFn.mockReturnValue(null);
  return { getConfigValue: mockFn };
});

vi.mock('../../src/core/config.js', () => configMock);

vi.mock('../../src/shared/radar-controller.js', () => ({
  addTarget: vi.fn(),
  removeTarget: vi.fn(),
}));

describe('AppWebSocket key hygiene', () => {
  let getConfigValue;

  beforeEach(() => {
    vi.clearAllMocks();
    getConfigValue = configMock.getConfigValue;
  });

  afterEach(() => {
    vi.resetModules();
  });

  async function getAppWebSocket() {
    const mod = await import('../../src/shared/app-ws.js');
    return mod;
  }

  describe('getWebSocketUrl — no hardcoded key default', () => {
    it('should throw when websocket_key is missing from config', async () => {
      getConfigValue.mockImplementation((path) => {
        if (path === 'server.websocket_url') return 'wss://lithium.philement.com/wss';
        if (path === 'server.websocket_key') return null;
        return null;
      });

      const { AppWebSocket } = await getAppWebSocket();
      const ws = new AppWebSocket();

      expect(() => ws.getWebSocketUrl()).toThrow('WebSocket key not configured');
    });

    it('should not use any hardcoded key as a fallback', async () => {
      getConfigValue.mockImplementation((path) => {
        if (path === 'server.websocket_url') return 'wss://lithium.philement.com/wss';
        if (path === 'server.websocket_key') return null;
        return null;
      });

      const { AppWebSocket } = await getAppWebSocket();
      const ws = new AppWebSocket();

      expect(() => ws.getWebSocketUrl()).toThrow();
      expect(getConfigValue).toHaveBeenCalledWith('server.websocket_key');
    });

    it('should append key as query parameter when configured', async () => {
      getConfigValue.mockImplementation((path) => {
        if (path === 'server.websocket_url') return 'wss://lithium.philement.com/wss';
        if (path === 'server.websocket_key') return 'test-secret-key-value';
        return null;
      });

      const { AppWebSocket } = await getAppWebSocket();
      const ws = new AppWebSocket();
      const url = ws.getWebSocketUrl();

      expect(url).toBe('wss://lithium.philement.com/wss?key=test-secret-key-value');
    });
  });

  describe('connect — URL logging is redacted', () => {
    it('should not log the full URL with key query parameter', async () => {
      getConfigValue.mockImplementation((path) => {
        if (path === 'server.websocket_url') return 'wss://lithium.philement.com/wss';
        if (path === 'server.websocket_key') return 'my-secret-key';
        return null;
      });

      global.WebSocket = vi.fn().mockImplementation(function() {
        this.readyState = 1;
        this.onopen = null;
        this.onclose = null;
        this.onerror = null;
        this.onmessage = null;
        this.close = vi.fn();
        this.send = vi.fn();
        const self = this;
        setTimeout(() => { if (self.onopen) self.onopen(); }, 0);
      });

      const { AppWebSocket } = await getAppWebSocket();
      const ws = new AppWebSocket();

      await ws.connect();

      const debugCalls = logMock.log.mock.calls.filter((call) =>
        call[0] === logMock.Subsystems.WEBSOCKET && call[1] === logMock.Status.DEBUG
      );

      const connectingCall = debugCalls.find((call) =>
        typeof call[2] === 'string' && call[2].includes('Connecting to')
      );

      expect(connectingCall).toBeDefined();
      const logMessage = connectingCall[2];
      expect(logMessage).not.toContain('my-secret-key');
      expect(logMessage).not.toContain('?key=');
      expect(logMessage).not.toContain('key=my');
      expect(logMessage).toContain('wss://lithium.philement.com/wss');

      delete global.WebSocket;
    });
  });
});
