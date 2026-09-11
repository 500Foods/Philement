/**
 * Terminal Manager Unit Tests
 *
 * Tests the terminalUrl getter, URL-building logic, iframe message handling,
 * and lifecycle without requiring the full DOM or Hydrogen backend.
 */

import { describe, it, expect, beforeEach, vi, afterEach } from 'vitest';

vi.mock('../../../src/core/icons.js', () => ({
  processIcons: vi.fn(),
}));

vi.mock('../../../src/core/log.js', () => ({
  log: vi.fn(),
  Subsystems: { MANAGER: 'Manager' },
  Status: { INFO: 'INFO', WARN: 'WARN', ERROR: 'ERROR' },
}));

vi.mock('../../../src/core/tooltip-api.js', () => ({
  initTooltips: vi.fn(),
}));

const configMock = vi.hoisted(() => {
  const mockFn = vi.fn();
  mockFn.mockReturnValue(null);
  return { getConfigValue: mockFn };
});

vi.mock('../../../src/core/config.js', () => configMock);

const jwtMock = vi.hoisted(() => {
  const mockFn = vi.fn();
  mockFn.mockReturnValue(null);
  return { retrieveJWT: mockFn };
});

vi.mock('../../../src/core/jwt.js', () => jwtMock);

describe('TerminalManager', () => {
  let getConfigValue;

  beforeEach(async () => {
    vi.clearAllMocks();
    getConfigValue = configMock.getConfigValue;
  });

  afterEach(() => {
    vi.resetModules();
  });

  async function getTerminalManager() {
    const mod = await import('../../../src/managers/terminal/terminal.js');
    return mod.TerminalManager;
  }

  describe('terminalUrl', () => {
    it('should build URL from server.url + server.terminal_path', async () => {
      getConfigValue.mockImplementation((path) => {
        if (path === 'server.url') return 'https://lithium.philement.com';
        if (path === 'server.terminal_path') return '/terminal';
        return null;
      });

      const TerminalManager = await getTerminalManager();
      const terminal = new TerminalManager();

      expect(terminal.terminalUrl).toBe('https://lithium.philement.com/terminal');
    });

    it('should default terminal_path to /terminal when config absent', async () => {
      getConfigValue.mockImplementation((path, defaultValue) => {
        if (path === 'server.url') return 'https://lithium.philement.com';
        if (path === 'server.terminal_path') return defaultValue;
        return null;
      });

      const TerminalManager = await getTerminalManager();
      const terminal = new TerminalManager();

      expect(terminal.terminalUrl).toBe('https://lithium.philement.com/terminal');
    });

    it('should strip trailing slash from server.url', async () => {
      getConfigValue.mockImplementation((path) => {
        if (path === 'server.url') return 'https://lithium.philement.com/';
        if (path === 'server.terminal_path') return '/terminal';
        return null;
      });

      const TerminalManager = await getTerminalManager();
      const terminal = new TerminalManager();

      expect(terminal.terminalUrl).toBe('https://lithium.philement.com/terminal');
    });

    it('should add leading slash to terminal_path if missing', async () => {
      getConfigValue.mockImplementation((path) => {
        if (path === 'server.url') return 'https://lithium.philement.com';
        if (path === 'server.terminal_path') return 'terminal';
        return null;
      });

      const TerminalManager = await getTerminalManager();
      const terminal = new TerminalManager();

      expect(terminal.terminalUrl).toBe('https://lithium.philement.com/terminal');
    });

    it('should return path-only when server.url is absent (warn)', async () => {
      getConfigValue.mockImplementation((path, defaultValue) => {
        if (path === 'server.url') return null;
        if (path === 'server.terminal_path') return defaultValue;
        return null;
      });

      const TerminalManager = await getTerminalManager();
      const terminal = new TerminalManager();

      expect(terminal.terminalUrl).toBe('/terminal');
    });

    it('should never return https://www.philement.com', async () => {
      getConfigValue.mockImplementation((path) => {
        if (path === 'server.url') return 'https://lithium.philement.com';
        if (path === 'server.terminal_path') return '/terminal';
        return null;
      });

      const TerminalManager = await getTerminalManager();
      const terminal = new TerminalManager();

      expect(terminal.terminalUrl).not.toContain('www.philement.com');
    });

     it('should use localhost default when server.url is localhost', async () => {
       getConfigValue.mockImplementation((path, defaultValue) => {
         if (path === 'server.url') return 'http://localhost:8080';
         if (path === 'server.terminal_path') return defaultValue;
         return null;
       });

       const TerminalManager = await getTerminalManager();
       const terminal = new TerminalManager();

       expect(terminal.terminalUrl).toBe('http://localhost:8080/terminal');
     });
   });

   describe('_getAllowedOrigin', () => {
     it('should derive origin from server.url', async () => {
       getConfigValue.mockImplementation((path) => {
         if (path === 'server.url') return 'https://lithium.philement.com';
         return null;
       });

       const TerminalManager = await getTerminalManager();
       const terminal = new TerminalManager();

       expect(terminal._getAllowedOrigin()).toBe('https://lithium.philement.com');
     });

     it('should derive origin with port from server.url', async () => {
       getConfigValue.mockImplementation((path) => {
         if (path === 'server.url') return 'http://localhost:8080';
         return null;
       });

       const TerminalManager = await getTerminalManager();
       const terminal = new TerminalManager();

       expect(terminal._getAllowedOrigin()).toBe('http://localhost:8080');
     });

     it('should fall back to window.location.origin when server.url is absent', async () => {
       getConfigValue.mockReturnValue(null);

       const TerminalManager = await getTerminalManager();
       const terminal = new TerminalManager();

       const expected = window.location.origin || 'http://localhost:3000';
       expect(terminal._getAllowedOrigin()).toBe(expected);
     });
   });

  describe('_handleIframeMessage', () => {
    let terminal;

    beforeEach(async () => {
      const TerminalManager = await getTerminalManager();
      terminal = new TerminalManager();
    });

    it('should respond with JWT on terminal-config-request from allowed origin and source', async () => {
      jwtMock.retrieveJWT.mockReturnValue('test.jwt.token');

      const fakeOrigin = window.location.origin || 'http://localhost:3000';
      const fakeEvent = {
        origin: fakeOrigin,
        data: { type: 'terminal-config-request' },
        source: {
          postMessage: vi.fn(),
        },
      };

      // Simulate iframe set by init()
      terminal.iframe = fakeEvent.source;

      terminal._handleIframeMessage(fakeEvent);

      expect(jwtMock.retrieveJWT).toHaveBeenCalledOnce();
      expect(fakeEvent.source.postMessage).toHaveBeenCalledWith(
        { type: 'terminal-config', config: { jwt: 'test.jwt.token' } },
        fakeOrigin
      );
    });

    it('should respond with error when no JWT available from allowed origin/source', async () => {
      jwtMock.retrieveJWT.mockReturnValue(null);

      const fakeOrigin = window.location.origin || 'http://localhost:3000';
      const fakeEvent = {
        origin: fakeOrigin,
        data: { type: 'terminal-config-request' },
        source: {
          postMessage: vi.fn(),
        },
      };

      terminal.iframe = fakeEvent.source;
      terminal._handleIframeMessage(fakeEvent);

      expect(fakeEvent.source.postMessage).toHaveBeenCalledWith(
        { type: 'terminal-config-error', error: 'No JWT available' },
        fakeOrigin
      );
    });

    it('should ignore messages from wrong origin', async () => {
      jwtMock.retrieveJWT.mockReturnValue('test.jwt.token');

      const fakeEvent = {
        origin: 'https://evil.example.com',
        data: { type: 'terminal-config-request' },
        source: { postMessage: vi.fn() },
      };

      terminal.iframe = fakeEvent.source;
      terminal._handleIframeMessage(fakeEvent);

      expect(jwtMock.retrieveJWT).not.toHaveBeenCalled();
      expect(fakeEvent.source.postMessage).not.toHaveBeenCalled();
    });

    it('should ignore messages from wrong source', async () => {
      jwtMock.retrieveJWT.mockReturnValue('test.jwt.token');

      const fakeOrigin = window.location.origin || 'http://localhost:3000';
      const fakeEvent = {
        origin: fakeOrigin,
        data: { type: 'terminal-config-request' },
        source: { postMessage: vi.fn() },
      };

      // iframe is a different object than event.source
      terminal.iframe = { postMessage: vi.fn() };
      terminal._handleIframeMessage(fakeEvent);

      expect(jwtMock.retrieveJWT).not.toHaveBeenCalled();
      expect(fakeEvent.source.postMessage).not.toHaveBeenCalled();
    });

    it('should ignore messages without data', async () => {
      const TerminalManager = await getTerminalManager();
      const localTerminal = new TerminalManager();

      localTerminal._handleIframeMessage({ data: null });
      localTerminal._handleIframeMessage({});
      localTerminal._handleIframeMessage({ data: 'string' });

      expect(jwtMock.retrieveJWT).not.toHaveBeenCalled();
    });

    it('should ignore non-config-request messages', async () => {
      jwtMock.retrieveJWT.mockReturnValue('test.jwt.token');

      const fakeOrigin = window.location.origin || 'http://localhost:3000';
      const fakeEvent = {
        origin: fakeOrigin,
        data: { type: 'some-other-message' },
        source: { postMessage: vi.fn() },
      };

      terminal.iframe = fakeEvent.source;
      terminal._handleIframeMessage(fakeEvent);

      expect(jwtMock.retrieveJWT).not.toHaveBeenCalled();
    });
  });

  describe('destroy', () => {
    it('should remove message and keydown event listeners', async () => {
      const TerminalManager = await getTerminalManager();
      const terminal = new TerminalManager();
      const boundHandler = terminal._handleIframeMessage;

      vi.spyOn(window, 'removeEventListener');
      vi.spyOn(document, 'removeEventListener');

      terminal.destroy();

      expect(window.removeEventListener).toHaveBeenCalledWith('message', boundHandler);
      expect(document.removeEventListener).toHaveBeenCalledWith('keydown', terminal.handleKeyDown);
    });

    it('should keep _handleIframeMessage as a bound function for re-init', async () => {
      const TerminalManager = await getTerminalManager();
      const terminal = new TerminalManager();
      const boundHandler = terminal._handleIframeMessage;

      expect(terminal._handleIframeMessage).toBeDefined();

      terminal.destroy();

      // The bound handler must remain so init() can re-register it
      expect(terminal._handleIframeMessage).toBe(boundHandler);
      expect(terminal._handleIframeMessage).not.toBeNull();
    });
  });
});
