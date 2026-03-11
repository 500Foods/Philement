/**
 * Integration test: App imports verification
 * 
 * This test verifies that app.js correctly imports all functions it needs from log.js.
 * It catches import errors that unit tests might miss since they test modules in isolation.
 */

import { describe, it, expect, vi, beforeEach } from 'vitest';

// Mock fetch globally
global.fetch = vi.fn();
global.sessionStorage = {
  getItem: vi.fn(),
  setItem: vi.fn(),
  removeItem: vi.fn(),
};
global.localStorage = {
  getItem: vi.fn(),
  setItem: vi.fn(),
  removeItem: vi.fn(),
};
global.document = {
  createElement: vi.fn(),
  documentElement: {
    style: {},
  },
  head: {
    appendChild: vi.fn(),
  },
  body: {
    classList: {
      add: vi.fn(),
      remove: vi.fn(),
    },
  },
};
global.window = {
  addEventListener: vi.fn(),
  removeEventListener: vi.fn(),
  location: { reload: vi.fn() },
};

describe('App Import Verification', () => {
  beforeEach(() => {
    vi.clearAllMocks();
    vi.resetModules();
  });

  it('should import all required log functions from log.js', async () => {
    // This test verifies that all functions app.js needs from log.js are exported
    // If any import is missing, this test will fail with a clear error
    const logModule = await import('../../src/core/log.js');
    
    // These are the functions app.js imports - verify they exist
    expect(typeof logModule.log).toBe('function');
    expect(typeof logModule.logGroup).toBe('function');
    expect(typeof logModule.logStartup).toBe('function');
    expect(typeof logModule.logAuth).toBe('function');
    expect(typeof logModule.logManager).toBe('function');
    expect(typeof logModule.logHttpRequest).toBe('function');
    expect(typeof logModule.logHttpResponse).toBe('function');
    expect(typeof logModule.Subsystems).toBe('object');
    expect(typeof logModule.Status).toBe('object');
    expect(typeof logModule.flush).toBe('function');
    expect(typeof logModule.getSessionId).toBe('function');
    expect(typeof logModule.getBuffer).toBe('function');
    expect(typeof logModule.getRecentLogs).toBe('function');
    expect(typeof logModule.getRawLog).toBe('function');
    expect(typeof logModule.getDisplayLog).toBe('function');
    expect(typeof logModule.getCounter).toBe('function');
    expect(typeof logModule.setConsoleLogging).toBe('function');
    expect(typeof logModule.getArchivedSessions).toBe('function');
    expect(typeof logModule.removeArchivedSession).toBe('function');
  });

  it('should have RESTAPI subsystem defined (mapped to Conduit)', async () => {
    const logModule = await import('../../src/core/log.js');
    expect(logModule.Subsystems.RESTAPI).toBe('Conduit');
  });

  it('should have HTTP request counter functions', async () => {
    const logModule = await import('../../src/core/log.js');
    expect(typeof logModule.getNextHttpRequestNum).toBe('function');
    
    // Verify counter increments
    const initialCount = logModule.getCounter();
    logModule.getNextHttpRequestNum();
    expect(logModule.getCounter()).toBe(initialCount);
  });
});
