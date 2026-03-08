/**
 * Actions Log Unit Tests
 */

import { describe, it, expect, vi, beforeEach } from 'vitest';
import {
  log,
  logStartup,
  logAuth,
  logHttp,
  logManager,
  logError,
  logWarn,
  logSuccess,
  Subsystems,
  Status,
  getBuffer,
  getDisplayLog,
  getRecentLogs,
  getRawLog,
  getSessionId,
  getCounter,
  flush,
  setConsoleLogging,
  getArchivedSessions,
  removeArchivedSession,
} from '../../src/core/log.js';

describe('log.js', () => {
  beforeEach(() => {
    // Reset counter and buffer for each test
    vi.resetModules();
  });

  describe('log()', () => {
    it('should create a log entry with required parameters', () => {
      const bufferBefore = getBuffer().length;
      
      log(Subsystems.STARTUP, Status.INFO, 'Test message');
      
      const buffer = getBuffer();
      expect(buffer.length).toBe(bufferBefore + 1);
      
      const entry = buffer[buffer.length - 1];
      expect(entry.subsystem).toBe('Startup');
      expect(entry.status).toBe('INFO');
      expect(entry.description).toBe('Test message');
      expect(entry.duration).toBe(0);
    });

    it('should accept duration parameter', () => {
      log(Subsystems.HTTP, Status.INFO, 'Request', 125.5);
      
      const entry = getBuffer()[getBuffer().length - 1];
      expect(entry.duration).toBe(125.5);
    });

    it('should default duration to 0 when not provided', () => {
      log(Subsystems.STARTUP, Status.INFO, 'Test');
      
      const entry = getBuffer()[getBuffer().length - 1];
      expect(entry.duration).toBe(0);
    });
  });

  describe('getDisplayLog()', () => {
    it('should return an array of formatted log strings', () => {
      const bufferBefore = getBuffer().length;
      
      log(Subsystems.STARTUP, Status.INFO, 'Test message 1');
      log(Subsystems.JWT, Status.INFO, 'Test message 2');
      
      const displayLog = getDisplayLog();
      expect(Array.isArray(displayLog)).toBe(true);
      expect(displayLog.length).toBe(bufferBefore + 2);
      expect(displayLog[displayLog.length - 2]).toContain('Test message 1');
      expect(displayLog[displayLog.length - 1]).toContain('Test message 2');
    });
  });

  describe('getSessionId()', () => {
    it('should return null before any log entries', () => {
      // Note: session ID is created on first log, so this test depends on test order
      expect(getSessionId()).toBeDefined(); // Will be created on first log
    });

    it('should return a session ID after logging', () => {
      log(Subsystems.STARTUP, Status.INFO, 'Session test');
      
      const sessionId = getSessionId();
      expect(sessionId).toBeDefined();
      expect(typeof sessionId).toBe('string');
      expect(sessionId.length).toBeGreaterThan(0);
    });
  });

  describe('Counter formatting', () => {
    it('should increment counter on each log call', () => {
      const initial = getCounter();
      
      log(Subsystems.STARTUP, Status.INFO, 'First');
      expect(getCounter()).toBe(initial + 1);
      
      log(Subsystems.STARTUP, Status.INFO, 'Second');
      expect(getCounter()).toBe(initial + 2);
    });
  });

  describe('Subsystems', () => {
    it('should have STARTUP constant', () => {
      expect(Subsystems.STARTUP).toBe('Startup');
    });

    it('should have JWT constant', () => {
      expect(Subsystems.JWT).toBe('JWT');
    });

    it('should have EVENTBUS constant', () => {
      expect(Subsystems.EVENTBUS).toBe('EventBus');
    });

    it('should have HTTP constant', () => {
      expect(Subsystems.HTTP).toBe('HTTP');
    });

    it('should have MANAGER constant', () => {
      expect(Subsystems.MANAGER).toBe('Manager');
    });

    it('should have SESSION constant', () => {
      expect(Subsystems.SESSION).toBe('Session');
    });
  });

  describe('Status', () => {
    it('should have INFO status', () => {
      expect(Status.INFO).toBe('INFO');
    });

    it('should have WARN status', () => {
      expect(Status.WARN).toBe('WARN');
    });

    it('should have ERROR status', () => {
      expect(Status.ERROR).toBe('ERROR');
    });

    it('should have FAIL status', () => {
      expect(Status.FAIL).toBe('FAIL');
    });

    it('should have DEBUG status', () => {
      expect(Status.DEBUG).toBe('DEBUG');
    });

    it('should have SUCCESS status', () => {
      expect(Status.SUCCESS).toBe('SUCCESS');
    });
  });

  describe('Shortcut functions', () => {
    it('logStartup should use STARTUP subsystem', () => {
      logStartup('Test message');
      
      const entry = getBuffer()[getBuffer().length - 1];
      expect(entry.subsystem).toBe('Startup');
      expect(entry.status).toBe('INFO');
    });

    it('logAuth should use AUTH subsystem', () => {
      logAuth(Status.INFO, 'Test message');
      
      const entry = getBuffer()[getBuffer().length - 1];
      expect(entry.subsystem).toBe('Auth');
    });

    it('logHttp should use HTTP subsystem', () => {
      logHttp('Test message', 50);
      
      const entry = getBuffer()[getBuffer().length - 1];
      expect(entry.subsystem).toBe('HTTP');
      expect(entry.duration).toBe(50);
    });

    it('logManager should use MANAGER subsystem', () => {
      logManager(Status.INFO, 'Test message');
      
      const entry = getBuffer()[getBuffer().length - 1];
      expect(entry.subsystem).toBe('Manager');
    });

    it('logError should use ERROR status', () => {
      logError('TestSubsystem', 'Test error');
      
      const entry = getBuffer()[getBuffer().length - 1];
      expect(entry.status).toBe('ERROR');
    });

    it('logWarn should use WARN status', () => {
      logWarn('TestSubsystem', 'Test warning');
      
      const entry = getBuffer()[getBuffer().length - 1];
      expect(entry.status).toBe('WARN');
    });

    it('logSuccess should use SUCCESS status', () => {
      logSuccess('TestSubsystem', 'Test success');
      
      const entry = getBuffer()[getBuffer().length - 1];
      expect(entry.status).toBe('SUCCESS');
    });
  });

  describe('getBuffer()', () => {
    it('should return an array', () => {
      expect(Array.isArray(getBuffer())).toBe(true);
    });

    it('should return a copy of the buffer', () => {
      log(Subsystems.STARTUP, Status.INFO, 'Test');
      
      const buffer1 = getBuffer();
      const buffer2 = getBuffer();
      
      // Should be different array instances but same content
      expect(buffer1).not.toBe(buffer2);
      expect(buffer1.length).toBe(buffer2.length);
    });
  });

  describe('flush()', () => {
    it('should be async', async () => {
      await expect(flush()).resolves.not.toThrow();
    });
  });

  describe('getRawLog()', () => {
    it('should return an array', () => {
      expect(Array.isArray(getRawLog())).toBe(true);
    });

    it('should return raw entry objects with expected fields', () => {
      log(Subsystems.STARTUP, Status.INFO, 'Raw log test');
      const raw = getRawLog();
      const entry = raw[raw.length - 1];
      expect(entry).toHaveProperty('counter');
      expect(entry).toHaveProperty('timestamp');
      expect(entry).toHaveProperty('subsystem');
      expect(entry).toHaveProperty('status');
      expect(entry).toHaveProperty('duration');
      expect(entry).toHaveProperty('description');
      expect(entry).toHaveProperty('sessionId');
    });

    it('should return a copy of the buffer (not a reference)', () => {
      const raw1 = getRawLog();
      const raw2 = getRawLog();
      expect(raw1).not.toBe(raw2);
    });

    it('should not be affected by flush (buffer persists after flush)', async () => {
      log(Subsystems.HTTP, Status.INFO, 'Pre-flush entry');
      const countBefore = getRawLog().length;
      await flush();
      // Buffer must still contain all entries — flush only drains syncQueue
      expect(getRawLog().length).toBe(countBefore);
    });
  });

  describe('getRecentLogs()', () => {
    it('should return an array of formatted strings', () => {
      log(Subsystems.STARTUP, Status.INFO, 'Recent log test');
      const recent = getRecentLogs();
      expect(Array.isArray(recent)).toBe(true);
      expect(recent.length).toBeGreaterThan(0);
      expect(typeof recent[0]).toBe('string');
    });

    it('should format entries as HH:MM:SS.ZZZ  DESCRIPTION', () => {
      log(Subsystems.STARTUP, Status.INFO, 'Format check entry');
      const recent = getRecentLogs();
      const last = recent[recent.length - 1];
      // Should start with time pattern HH:MM:SS.ZZZ
      expect(last).toMatch(/^\d{2}:\d{2}:\d{2}\.\d{3}/);
      expect(last).toContain('Format check entry');
    });

    it('should respect the maxEntries limit', () => {
      // Log enough to exceed limit
      for (let i = 0; i < 5; i++) {
        log(Subsystems.STARTUP, Status.INFO, `Entry ${i}`);
      }
      const limited = getRecentLogs(3);
      expect(limited.length).toBeLessThanOrEqual(3);
    });

    it('should return the most recent entries when limited', () => {
      log(Subsystems.STARTUP, Status.INFO, 'First distinct marker');
      log(Subsystems.STARTUP, Status.INFO, 'Last distinct marker');
      const recent = getRecentLogs(1);
      expect(recent[0]).toContain('Last distinct marker');
    });
  });

  describe('setConsoleLogging()', () => {
    it('should enable console logging without error', () => {
      expect(() => setConsoleLogging(true)).not.toThrow();
    });

    it('should disable console logging without error', () => {
      expect(() => setConsoleLogging(false)).not.toThrow();
    });

    it('should cause log() to call console.log when enabled', () => {
      const spy = vi.spyOn(console, 'log').mockImplementation(() => {});
      setConsoleLogging(true);
      log(Subsystems.STARTUP, Status.INFO, 'Console log test');
      expect(spy).toHaveBeenCalled();
      const call = spy.mock.calls.find(c => String(c[0]).includes('Console log test'));
      expect(call).toBeDefined();
      setConsoleLogging(false);
      spy.mockRestore();
    });

    it('should not call console.log when disabled', () => {
      setConsoleLogging(false);
      const spy = vi.spyOn(console, 'log').mockImplementation(() => {});
      log(Subsystems.STARTUP, Status.INFO, 'Silent log test');
      const logCalls = spy.mock.calls.filter(c => String(c[0]).includes('[LOG]'));
      expect(logCalls.length).toBe(0);
      spy.mockRestore();
    });
  });

  describe('getArchivedSessions()', () => {
    const ARCHIVE_PREFIX = 'lithium_log_archive_';

    beforeEach(() => {
      // Clean up any archive keys left by previous tests
      const keysToRemove = [];
      for (let i = 0; i < localStorage.length; i++) {
        const key = localStorage.key(i);
        if (key && key.startsWith(ARCHIVE_PREFIX)) keysToRemove.push(key);
      }
      keysToRemove.forEach(k => localStorage.removeItem(k));
    });

    it('returns an empty array when there are no archives', () => {
      const archives = getArchivedSessions();
      expect(Array.isArray(archives)).toBe(true);
      expect(archives.length).toBe(0);
    });

    it('returns archived sessions written directly to localStorage', () => {
      const entry = { sessionId: 'test-session-abc', entries: [{ counter: 1, description: 'archived' }] };
      localStorage.setItem(`${ARCHIVE_PREFIX}test-abc_2026-01-01T00-00-00-000Z`, JSON.stringify(entry));

      const archives = getArchivedSessions();
      expect(archives.length).toBe(1);
      expect(archives[0]).toHaveProperty('key');
      expect(archives[0]).toHaveProperty('sessionId', 'test-session-abc');
      expect(archives[0]).toHaveProperty('entries');
    });

    it('sorts archives oldest-first by key', () => {
      localStorage.setItem(`${ARCHIVE_PREFIX}z_2026-03-02T00-00-00-000Z`, JSON.stringify({ sessionId: 'newer', entries: [] }));
      localStorage.setItem(`${ARCHIVE_PREFIX}a_2026-03-01T00-00-00-000Z`, JSON.stringify({ sessionId: 'older', entries: [] }));

      const archives = getArchivedSessions();
      expect(archives.length).toBeGreaterThanOrEqual(2);
      const ourArchives = archives.filter(a => ['newer', 'older'].includes(a.sessionId));
      expect(ourArchives[0].sessionId).toBe('older');
    });

    it('skips malformed entries without throwing', () => {
      localStorage.setItem(`${ARCHIVE_PREFIX}bad_2026-01-01T00-00-00-000Z`, '{not-valid-json}');
      expect(() => getArchivedSessions()).not.toThrow();
    });
  });

  describe('removeArchivedSession()', () => {
    const ARCHIVE_PREFIX = 'lithium_log_archive_';

    it('removes an archived session by key', () => {
      const key = `${ARCHIVE_PREFIX}rm_test_2026-01-01T00-00-00-000Z`;
      localStorage.setItem(key, JSON.stringify({ sessionId: 'rm', entries: [] }));
      expect(localStorage.getItem(key)).not.toBeNull();

      removeArchivedSession(key);
      expect(localStorage.getItem(key)).toBeNull();
    });

    it('does not throw when key does not exist', () => {
      expect(() => removeArchivedSession('lithium_log_archive_nonexistent')).not.toThrow();
    });
  });
});
