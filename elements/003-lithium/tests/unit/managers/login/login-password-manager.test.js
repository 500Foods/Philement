/**
 * Login Password Manager Tests
 *
 * Unit tests for the PasswordManager class extracted from login.js.
 */

import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest';
import { PasswordManager } from '../../../../src/managers/login/login-password-manager.js';

// Mock the log-formatter module
vi.mock('../../../../src/shared/log-formatter.js', () => ({
  getPasswordManagerSelectors: vi.fn(() => [
    'com-1password-button',
    '[id*="1p-"]',
    '[class*="lastpass"]',
  ]),
}));

describe('PasswordManager', () => {
  let pm;

  beforeEach(() => {
    pm = new PasswordManager();
    // Clean up any injected test elements
    document.body.innerHTML = '';
    document.body.className = '';
  });

  afterEach(() => {
    pm.destroy();
    document.body.innerHTML = '';
    document.body.className = '';
  });

  describe('constructor', () => {
    it('should initialize with no observer', () => {
      expect(pm._observer).toBeNull();
    });
  });

  describe('show', () => {
    it('should remove hide class from body', () => {
      document.body.classList.add('hide-password-manager-ui');
      pm.show();
      expect(document.body.classList.contains('hide-password-manager-ui')).toBe(false);
    });

    it('should disconnect observer if active', () => {
      pm.hide();
      expect(pm._observer).not.toBeNull();
      const disconnectSpy = vi.spyOn(pm._observer, 'disconnect');
      pm.show();
      expect(disconnectSpy).toHaveBeenCalled();
      expect(pm._observer).toBeNull();
    });

    it('should not throw when called without prior hide', () => {
      expect(() => pm.show()).not.toThrow();
    });
  });

  describe('hide', () => {
    it('should create a MutationObserver', () => {
      pm.hide();
      expect(pm._observer).toBeInstanceOf(MutationObserver);
    });

    it('should suppress existing password manager elements', () => {
      const el = document.createElement('com-1password-button');
      document.body.appendChild(el);
      pm.hide();
      expect(el.style.display).toBe('none');
      expect(el.style.visibility).toBe('hidden');
      expect(el.style.opacity).toBe('0');
      expect(el.style.pointerEvents).toBe('none');
    });

    it('should suppress elements by id pattern', () => {
      const el = document.createElement('div');
      el.id = '1p-inject';
      document.body.appendChild(el);
      pm.hide();
      expect(el.style.display).toBe('none');
    });

    it('should suppress elements by class pattern', () => {
      const el = document.createElement('div');
      el.className = 'lastpass-container';
      document.body.appendChild(el);
      pm.hide();
      expect(el.style.display).toBe('none');
    });

    it('should not throw for invalid selectors', async () => {
      // Mock returns an invalid selector to test error handling
      const { getPasswordManagerSelectors } = await import('../../../../src/shared/log-formatter.js');
      getPasswordManagerSelectors.mockReturnValueOnce(['[invalid']);
      expect(() => pm.hide()).not.toThrow();
    });

    it('should observe body for mutations', () => {
      const observeSpy = vi.spyOn(MutationObserver.prototype, 'observe');
      pm.hide();
      expect(observeSpy).toHaveBeenCalledWith(document.body, {
        childList: true,
        subtree: true,
        attributes: true,
        attributeFilter: ['style', 'class'],
      });
    });
  });

  describe('removeAll', () => {
    it('should remove password manager elements from DOM', () => {
      const el1 = document.createElement('com-1password-button');
      const el2 = document.createElement('div');
      el2.id = '1p-inject';
      document.body.appendChild(el1);
      document.body.appendChild(el2);
      pm.removeAll();
      expect(document.body.contains(el1)).toBe(false);
      expect(document.body.contains(el2)).toBe(false);
    });

    it('should not throw when no elements exist', () => {
      expect(() => pm.removeAll()).not.toThrow();
    });

    it('should not throw for invalid selectors', async () => {
      const { getPasswordManagerSelectors } = await import('../../../../src/shared/log-formatter.js');
      getPasswordManagerSelectors.mockReturnValueOnce(['[invalid']);
      expect(() => pm.removeAll()).not.toThrow();
    });

    it('should leave non-matching elements intact', () => {
      const el = document.createElement('div');
      el.className = 'normal-element';
      document.body.appendChild(el);
      pm.removeAll();
      expect(document.body.contains(el)).toBe(true);
    });
  });

  describe('destroy', () => {
    it('should disconnect observer', () => {
      pm.hide();
      const disconnectSpy = vi.spyOn(pm._observer, 'disconnect');
      pm.destroy();
      expect(disconnectSpy).toHaveBeenCalled();
    });

    it('should null out observer', () => {
      pm.hide();
      pm.destroy();
      expect(pm._observer).toBeNull();
    });

    it('should remove hide class from body', () => {
      document.body.classList.add('hide-password-manager-ui');
      pm.destroy();
      expect(document.body.classList.contains('hide-password-manager-ui')).toBe(false);
    });

    it('should remove all password manager elements', () => {
      const el = document.createElement('com-1password-button');
      document.body.appendChild(el);
      pm.destroy();
      expect(document.body.contains(el)).toBe(false);
    });

    it('should not throw when called without prior hide', () => {
      expect(() => pm.destroy()).not.toThrow();
    });
  });

  describe('dynamic injection suppression', () => {
    it('should suppress dynamically injected elements', async () => {
      pm.hide();
      const el = document.createElement('com-1password-button');
      document.body.appendChild(el);
      // Wait for MutationObserver to process the mutation
      await new Promise((resolve) => setTimeout(resolve, 10));
      expect(el.style.display).toBe('none');
    });
  });
});
