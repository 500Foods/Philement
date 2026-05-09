/**
 * Login Shortcuts Tests
 *
 * Unit tests for the LoginShortcuts class extracted from login.js.
 */

import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest';
import { LoginShortcuts } from '../../../../src/managers/login/login-shortcuts.js';

vi.mock('../../../../src/core/log.js', () => ({
  log: vi.fn(),
  Subsystems: { LOGIN: 'LOGIN' },
  Status: { INFO: 'INFO', SUCCESS: 'SUCCESS', ERROR: 'ERROR', WARN: 'WARN' },
}));

function makeElements() {
  return {
    username: document.createElement('input'),
    password: document.createElement('input'),
    helpBtn: document.createElement('button'),
    languageBtn: document.createElement('button'),
    themeBtn: document.createElement('button'),
    logsBtn: document.createElement('button'),
  };
}

function makeKeyEvent(opts) {
  // Build a KeyboardEvent-like object with the bits the shortcut handler
  // touches. We don't need a real KeyboardEvent because the production code
  // only calls .key, .ctrlKey, .shiftKey, .preventDefault, and
  // .getModifierState — all of which we provide here.
  return {
    key: opts.key,
    ctrlKey: !!opts.ctrlKey,
    shiftKey: !!opts.shiftKey,
    preventDefault: vi.fn(),
    getModifierState: vi.fn(() => !!opts.capsLock),
  };
}

describe('LoginShortcuts', () => {
  let elements;
  let shortcuts;
  let getCurrentPanel;
  let onClearUsername;
  let onSwitchToLogin;
  let panelName;

  beforeEach(() => {
    vi.clearAllMocks();
    elements = makeElements();
    panelName = 'login';
    getCurrentPanel = vi.fn(() => panelName);
    onClearUsername = vi.fn();
    onSwitchToLogin = vi.fn();
    shortcuts = new LoginShortcuts({
      elements,
      getCurrentPanel,
      onClearUsername,
      onSwitchToLogin,
    });
  });

  afterEach(() => {
    if (shortcuts) shortcuts.detach();
  });

  describe('attach() / detach()', () => {
    it('attaches keydown and keyup listeners on document', () => {
      const addSpy = vi.spyOn(document, 'addEventListener');
      shortcuts.attach();
      expect(addSpy).toHaveBeenCalledWith('keydown', expect.any(Function));
      expect(addSpy).toHaveBeenCalledWith('keyup', expect.any(Function));
      addSpy.mockRestore();
    });

    it('detach() removes both listeners', () => {
      shortcuts.attach();
      const removeSpy = vi.spyOn(document, 'removeEventListener');
      shortcuts.detach();
      expect(removeSpy).toHaveBeenCalledWith('keydown', expect.any(Function));
      expect(removeSpy).toHaveBeenCalledWith('keyup', expect.any(Function));
      removeSpy.mockRestore();
    });

    it('attach() is idempotent', () => {
      const addSpy = vi.spyOn(document, 'addEventListener');
      shortcuts.attach();
      shortcuts.attach();
      // Two calls (keydown + keyup), only once.
      expect(addSpy).toHaveBeenCalledTimes(2);
      addSpy.mockRestore();
    });
  });

  describe('ESC key', () => {
    it('on login panel calls onClearUsername', () => {
      panelName = 'login';
      const e = makeKeyEvent({ key: 'Escape' });
      shortcuts.handleKeyboardShortcuts(e);
      expect(onClearUsername).toHaveBeenCalled();
      expect(onSwitchToLogin).not.toHaveBeenCalled();
      expect(e.preventDefault).toHaveBeenCalled();
    });

    it('on subpanel calls onSwitchToLogin', () => {
      panelName = 'logs';
      const e = makeKeyEvent({ key: 'Escape' });
      shortcuts.handleKeyboardShortcuts(e);
      expect(onSwitchToLogin).toHaveBeenCalled();
      expect(onClearUsername).not.toHaveBeenCalled();
    });
  });

  describe('Ctrl+Shift focus shortcuts', () => {
    it('Ctrl+Shift+U focuses and selects username', () => {
      const focusSpy = vi.spyOn(elements.username, 'focus');
      const selectSpy = vi.spyOn(elements.username, 'select');
      const e = makeKeyEvent({ key: 'u', ctrlKey: true, shiftKey: true });
      shortcuts.handleKeyboardShortcuts(e);
      expect(focusSpy).toHaveBeenCalled();
      expect(selectSpy).toHaveBeenCalled();
      expect(e.preventDefault).toHaveBeenCalled();
    });

    it('Ctrl+Shift+P focuses and selects password', () => {
      const focusSpy = vi.spyOn(elements.password, 'focus');
      const selectSpy = vi.spyOn(elements.password, 'select');
      const e = makeKeyEvent({ key: 'p', ctrlKey: true, shiftKey: true });
      shortcuts.handleKeyboardShortcuts(e);
      expect(focusSpy).toHaveBeenCalled();
      expect(selectSpy).toHaveBeenCalled();
    });

    it('case-insensitive (uppercase keys)', () => {
      const focusSpy = vi.spyOn(elements.username, 'focus');
      const e = makeKeyEvent({ key: 'U', ctrlKey: true, shiftKey: true });
      shortcuts.handleKeyboardShortcuts(e);
      expect(focusSpy).toHaveBeenCalled();
    });
  });

  describe('button shortcuts', () => {
    it.each([
      ['F1', 'helpBtn'],
      ['i', 'languageBtn'],
      ['t', 'themeBtn'],
      ['l', 'logsBtn'],
    ])('%s clicks %s', (key, btn) => {
      const clickSpy = vi.spyOn(elements[btn], 'click');
      const e = makeKeyEvent({ key, ctrlKey: key !== 'F1', shiftKey: key !== 'F1' });
      shortcuts.handleKeyboardShortcuts(e);
      expect(clickSpy).toHaveBeenCalled();
    });

    it('does not click a disabled button', () => {
      elements.helpBtn.disabled = true;
      const clickSpy = vi.spyOn(elements.helpBtn, 'click');
      const e = makeKeyEvent({ key: 'F1' });
      shortcuts.handleKeyboardShortcuts(e);
      expect(clickSpy).not.toHaveBeenCalled();
    });

    it('does not fire shortcuts when not on login panel (except ESC)', () => {
      panelName = 'theme';
      const clickSpy = vi.spyOn(elements.helpBtn, 'click');
      const e = makeKeyEvent({ key: 'F1' });
      shortcuts.handleKeyboardShortcuts(e);
      expect(clickSpy).not.toHaveBeenCalled();
      expect(e.preventDefault).not.toHaveBeenCalled();
    });

    it('ignores keys without the required modifiers', () => {
      const clickSpy = vi.spyOn(elements.languageBtn, 'click');
      // 'i' alone, no Ctrl+Shift.
      const e = makeKeyEvent({ key: 'i' });
      shortcuts.handleKeyboardShortcuts(e);
      expect(clickSpy).not.toHaveBeenCalled();
    });
  });

  describe('CAPS-LOCK indicator', () => {
    it('adds caps-lock-active when caps lock is on', () => {
      const e = makeKeyEvent({ key: 'a', capsLock: true });
      shortcuts.checkCapsLock(e);
      expect(elements.password.classList.contains('caps-lock-active')).toBe(true);
      expect(shortcuts.isCapsLockOn).toBe(true);
    });

    it('removes caps-lock-active when caps lock turns off', () => {
      const on = makeKeyEvent({ key: 'a', capsLock: true });
      const off = makeKeyEvent({ key: 'a', capsLock: false });
      shortcuts.checkCapsLock(on);
      shortcuts.checkCapsLock(off);
      expect(elements.password.classList.contains('caps-lock-active')).toBe(false);
      expect(shortcuts.isCapsLockOn).toBe(false);
    });

    it('does nothing when state has not changed', () => {
      const e = makeKeyEvent({ key: 'a', capsLock: false });
      const addSpy = vi.spyOn(elements.password.classList, 'remove');
      shortcuts.checkCapsLock(e);
      expect(addSpy).not.toHaveBeenCalled();
      addSpy.mockRestore();
    });

    it('handles missing password element gracefully', () => {
      const partial = { username: elements.username };
      const s = new LoginShortcuts({
        elements: partial,
        getCurrentPanel,
        onClearUsername,
        onSwitchToLogin,
      });
      const e = makeKeyEvent({ key: 'a', capsLock: true });
      expect(() => s.checkCapsLock(e)).not.toThrow();
    });
  });
});
