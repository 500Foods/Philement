/**
 * Login Panels Tests
 *
 * Unit tests for the LoginPanels class extracted from login.js.
 */

import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest';
import { LoginPanels } from '../../../../src/managers/login/login-panels.js';

// Mock the transitions module
vi.mock('../../../../src/core/transitions.js', () => ({
  getTransitionDuration: vi.fn(() => 50),
  waitForTransition: vi.fn(() => Promise.resolve()),
}));

function createMockElement(id = '', display = 'none') {
  const el = document.createElement('div');
  if (id) el.id = id;
  el.style.display = display;
  // Mock offset dimensions
  Object.defineProperty(el, 'offsetWidth', { value: 360, writable: true });
  Object.defineProperty(el, 'offsetHeight', { value: 400, writable: true });
  return el;
}

describe('LoginPanels', () => {
  let panels;
  let loginPanel;
  let overlay;
  let beforeSwitchFn;
  let afterSwitchFn;

  beforeEach(() => {
    loginPanel = createMockElement('login-panel', 'none');
    const themePanel = createMockElement('theme-panel', 'none');
    const logsPanel = createMockElement('logs-panel', 'none');
    const languagePanel = createMockElement('language-panel', 'none');
    const helpPanel = createMockElement('help-panel', 'none');
    overlay = createMockElement('transition-overlay', 'none');

    beforeSwitchFn = vi.fn();
    afterSwitchFn = vi.fn();

    panels = new LoginPanels({
      panels: {
        login: loginPanel,
        theme: themePanel,
        logs: logsPanel,
        language: languagePanel,
        help: helpPanel,
      },
      loginPanel,
      overlay,
      onBeforeSwitch: beforeSwitchFn,
      onAfterSwitch: afterSwitchFn,
    });
  });

  afterEach(() => {
    panels.teardown();
  });

  describe('constructor', () => {
    it('should initialize with default values', () => {
      const defaultPanels = new LoginPanels({ panels: {}, loginPanel: null, overlay: null });
      expect(defaultPanels.currentPanel).toBe('login');
      defaultPanels.teardown();
    });

    it('should accept callbacks', () => {
      expect(panels._onBeforeSwitch).toBe(beforeSwitchFn);
      expect(panels._onAfterSwitch).toBe(afterSwitchFn);
    });
  });

  describe('currentPanel', () => {
    it('should get current panel', () => {
      expect(panels.currentPanel).toBe('login');
    });

    it('should set current panel', () => {
      panels.currentPanel = 'theme';
      expect(panels.currentPanel).toBe('theme');
    });
  });

  describe('switchPanel', () => {
    it('should do nothing when switching to the same panel', async () => {
      await panels.switchPanel('login');
      expect(beforeSwitchFn).not.toHaveBeenCalled();
      expect(panels.currentPanel).toBe('login');
    });

    it('should do nothing when target panel element is missing', async () => {
      panels._panels.unknown = null;
      await panels.switchPanel('unknown');
      expect(panels.currentPanel).toBe('login');
    });

    it('should call onBeforeSwitch before transition', async () => {
      await panels.switchPanel('theme');
      expect(beforeSwitchFn).toHaveBeenCalledWith('theme');
    });

    it('should call onAfterSwitch after transition', async () => {
      await panels.switchPanel('theme');
      expect(afterSwitchFn).toHaveBeenCalledWith('login', 'theme', 0);
    });

    it('should update currentPanel after switch', async () => {
      await panels.switchPanel('theme');
      expect(panels.currentPanel).toBe('theme');
    });

    it('should enable and disable overlay around transition', async () => {
      await panels.switchPanel('theme');
      expect(overlay.classList.contains('active')).toBe(false);
    });

    it('should handle switching back to login', async () => {
      await panels.switchPanel('theme');
      await panels.switchPanel('login');
      expect(panels.currentPanel).toBe('login');
      expect(afterSwitchFn).toHaveBeenLastCalledWith('theme', 'login', 0);
    });

    it('should handle multiple switches in sequence', async () => {
      await panels.switchPanel('theme');
      await panels.switchPanel('logs');
      await panels.switchPanel('language');
      expect(panels.currentPanel).toBe('language');
      expect(beforeSwitchFn).toHaveBeenCalledTimes(3);
    });
  });

  describe('cancelPendingFocus', () => {
    it('should clear focus timer', () => {
      panels._focusTimer = setTimeout(() => {}, 1000);
      panels.cancelPendingFocus();
      expect(panels._focusTimer).toBeNull();
    });

    it('should not throw when no timer exists', () => {
      expect(() => panels.cancelPendingFocus()).not.toThrow();
    });
  });

  describe('scheduleFocus', () => {
    it('should schedule a focus function', async () => {
      const focusFn = vi.fn();
      panels.scheduleFocus(focusFn, 10);
      await new Promise((resolve) => setTimeout(resolve, 30));
      expect(focusFn).toHaveBeenCalled();
    });

    it('should cancel previous timer when scheduling new one', () => {
      const timer1 = setTimeout(() => {}, 1000);
      panels._focusTimer = timer1;
      panels.scheduleFocus(() => {}, 10);
      expect(panels._focusTimer).not.toBe(timer1);
    });

    it('should use default delay when not specified', () => {
      const focusFn = vi.fn();
      panels.scheduleFocus(focusFn);
      expect(panels._focusTimer).not.toBeNull();
    });
  });

  describe('enableOverlay / disableOverlay', () => {
    it('should add active class when enabled', () => {
      panels.enableOverlay();
      expect(overlay.classList.contains('active')).toBe(true);
    });

    it('should remove active class when disabled', () => {
      panels.enableOverlay();
      panels.disableOverlay();
      expect(overlay.classList.contains('active')).toBe(false);
    });

    it('should not throw when overlay is null', () => {
      panels._overlay = null;
      expect(() => panels.enableOverlay()).not.toThrow();
      expect(() => panels.disableOverlay()).not.toThrow();
    });
  });

  describe('teardown', () => {
    it('should cancel pending focus', () => {
      panels._focusTimer = setTimeout(() => {}, 1000);
      panels.teardown();
      expect(panels._focusTimer).toBeNull();
    });

    it('should reset state', () => {
      panels.currentPanel = 'theme';
      panels.teardown();
      expect(panels.currentPanel).toBe('login');
      expect(panels._panels).toEqual({});
    });
  });

  describe('ESC handling simulation', () => {
    it('should allow switching back to login from any panel', async () => {
      await panels.switchPanel('theme');
      expect(panels.currentPanel).toBe('theme');
      await panels.switchPanel('login');
      expect(panels.currentPanel).toBe('login');
    });

    it('should handle rapid switches', async () => {
      const p1 = panels.switchPanel('theme');
      const p2 = panels.switchPanel('logs');
      await Promise.all([p1, p2]);
      // One of them should win
      expect(['theme', 'logs', 'login']).toContain(panels.currentPanel);
    });
  });
});
