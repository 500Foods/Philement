/**
 * Utils Unit Tests
 */

import { describe, it, expect, beforeEach, vi } from 'vitest';
import {
  getPreferences,
  savePreferences,
  formatDate,
  formatTime,
  formatDateTime,
  formatNumber,
  formatCurrency,
  formatPercent,
  formatRelativeTime,
  debounce,
  throttle,
  generateId,
  deepClone,
  escapeHtml,
} from '../../src/core/utils.js';

// Mock localStorage
const localStorageMock = (() => {
  let store = {};
  return {
    getItem: vi.fn((key) => store[key] || null),
    setItem: vi.fn((key, value) => { store[key] = value; }),
    removeItem: vi.fn((key) => { delete store[key]; }),
    clear: vi.fn(() => { store = {}; }),
  };
})();

Object.defineProperty(global, 'localStorage', { value: localStorageMock });

// Mock getClaims from jwt.js
vi.mock('../../src/core/jwt.js', () => ({
  getClaims: vi.fn(() => null),
}));

describe('Utils', () => {
  beforeEach(() => {
    localStorage.clear();
    vi.clearAllMocks();
  });

  describe('getPreferences', () => {
    it('should return default preferences when nothing stored', () => {
      const prefs = getPreferences();
      
      expect(prefs.lang).toBe('en-US');
      expect(prefs.dateFormat).toBe('medium');
      expect(prefs.timeFormat).toBe('24h');
    });

    it('should return stored preferences', () => {
      localStorage.setItem('lithium_preferences', JSON.stringify({
        lang: 'fr-FR',
        dateFormat: 'short',
      }));

      const prefs = getPreferences();

      expect(prefs.lang).toBe('fr-FR');
      expect(prefs.dateFormat).toBe('short');
      expect(prefs.timeFormat).toBe('24h'); // default
    });

    it('should return defaults for invalid stored data', () => {
      localStorage.setItem('lithium_preferences', 'invalid json');

      // Suppress the expected console.warn from getPreferences' catch block
      const warnSpy = vi.spyOn(console, 'warn').mockImplementation(() => {});
      const prefs = getPreferences();
      warnSpy.mockRestore();

      expect(prefs.lang).toBe('en-US');
    });
  });

  describe('savePreferences', () => {
    it('should save preferences to localStorage', () => {
      savePreferences({ lang: 'de-DE' });

      expect(localStorage.setItem).toHaveBeenCalledWith(
        'lithium_preferences',
        expect.stringContaining('"lang":"de-DE"')
      );
    });

    it('should merge with existing preferences', () => {
      localStorage.setItem('lithium_preferences', JSON.stringify({
        lang: 'en-US',
        dateFormat: 'long',
      }));

      savePreferences({ timeFormat: '12h' });

      expect(localStorage.setItem).toHaveBeenCalledWith(
        'lithium_preferences',
        expect.stringContaining('"dateFormat":"long"')
      );
    });
  });

  describe('formatDate', () => {
    it('should format a date with default preferences', () => {
      const date = new Date(2025, 0, 15);
      const formatted = formatDate(date);
      
      expect(formatted).toBeTruthy();
      expect(typeof formatted).toBe('string');
    });

    it('should handle date string input', () => {
      const formatted = formatDate('2025-01-15');
      expect(formatted).toBeTruthy();
    });

    it('should return "Invalid date" for invalid input', () => {
      expect(formatDate('not-a-date')).toBe('Invalid date');
    });

    it('should accept format options override', () => {
      const formatted = formatDate(new Date(), { dateStyle: 'short' });
      expect(formatted).toBeTruthy();
    });
  });

  describe('formatTime', () => {
    it('should format time with default preferences', () => {
      const formatted = formatTime(new Date());
      expect(formatted).toBeTruthy();
    });

    it('should return "Invalid time" for invalid input', () => {
      expect(formatTime('invalid')).toBe('Invalid time');
    });
  });

  describe('formatDateTime', () => {
    it('should format datetime with default preferences', () => {
      const formatted = formatDateTime(new Date());
      expect(formatted).toBeTruthy();
    });

    it('should return "Invalid datetime" for invalid input', () => {
      expect(formatDateTime('invalid')).toBe('Invalid datetime');
    });
  });

  describe('formatNumber', () => {
    it('should format a number', () => {
      expect(formatNumber(1234.56)).toBe('1,234.56');
    });

    it('should return "Invalid number" for NaN', () => {
      expect(formatNumber(NaN)).toBe('Invalid number');
    });

    it('should return "Invalid number" for non-number', () => {
      expect(formatNumber('not a number')).toBe('Invalid number');
    });

    it('should accept formatting options', () => {
      expect(formatNumber(0.1234, { minimumFractionDigits: 2, maximumFractionDigits: 2 })).toBe('0.12');
    });
  });

  describe('formatCurrency', () => {
    it('should format currency with default currency', () => {
      const formatted = formatCurrency(1234.56);
      expect(formatted).toBeTruthy();
    });

    it('should format currency with specified currency', () => {
      const formatted = formatCurrency(100, 'EUR');
      // EUR uses € symbol, so we check for either the symbol or code
      expect(formatted).toMatch(/€|EUR/);
    });

    it('should return "Invalid amount" for NaN', () => {
      expect(formatCurrency(NaN)).toBe('Invalid amount');
    });
  });

  describe('formatPercent', () => {
    it('should format a percentage', () => {
      expect(formatPercent(0.5)).toBe('50%');
    });

    it('should return "Invalid percentage" for NaN', () => {
      expect(formatPercent(NaN)).toBe('Invalid percentage');
    });

    it('should respect decimal options', () => {
      expect(formatPercent(0.333, { minimumFractionDigits: 1, maximumFractionDigits: 1 })).toBe('33.3%');
    });
  });

  describe('formatRelativeTime', () => {
    it('should format relative time for past date', () => {
      const pastDate = new Date(Date.now() - 3600000); // 1 hour ago
      const formatted = formatRelativeTime(pastDate);
      expect(formatted).toBeTruthy();
    });

    it('should format relative time for future date', () => {
      const futureDate = new Date(Date.now() + 3600000); // 1 hour from now
      const formatted = formatRelativeTime(futureDate);
      expect(formatted).toBeTruthy();
    });

    it('should return "Invalid date" for invalid input', () => {
      expect(formatRelativeTime('invalid')).toBe('Invalid date');
    });
  });

  describe('debounce', () => {
    it('should delay function execution', async () => {
      const func = vi.fn();
      const debounced = debounce(func, 100);

      debounced();
      expect(func).not.toHaveBeenCalled();

      await new Promise(resolve => setTimeout(resolve, 150));
      expect(func).toHaveBeenCalledTimes(1);
    });

    it('should only call once for multiple rapid calls', async () => {
      const func = vi.fn();
      const debounced = debounce(func, 100);

      debounced();
      debounced();
      debounced();

      await new Promise(resolve => setTimeout(resolve, 150));
      expect(func).toHaveBeenCalledTimes(1);
    });
  });

  describe('throttle', () => {
    it('should limit function execution rate', async () => {
      const func = vi.fn();
      const throttled = throttle(func, 100);

      throttled();
      throttled();
      throttled();

      expect(func).toHaveBeenCalledTimes(1);

      await new Promise(resolve => setTimeout(resolve, 150));
      throttled();
      expect(func).toHaveBeenCalledTimes(2);
    });
  });

  describe('generateId', () => {
    it('should generate unique IDs', () => {
      const id1 = generateId();
      const id2 = generateId();

      expect(id1).not.toBe(id2);
      expect(id1).toMatch(/^id-\d+-[a-z0-9]+$/);
    });

    it('should use custom prefix', () => {
      const id = generateId('custom');
      expect(id).toMatch(/^custom-\d+-[a-z0-9]+$/);
    });
  });

  describe('deepClone', () => {
    it('should clone a plain object', () => {
      const original = { a: 1, b: { c: 2 } };
      const cloned = deepClone(original);

      expect(cloned).toEqual(original);
      expect(cloned).not.toBe(original);
      expect(cloned.b).not.toBe(original.b);
    });

    it('should clone an array', () => {
      const original = [1, 2, { a: 3 }];
      const cloned = deepClone(original);

      expect(cloned).toEqual(original);
      expect(cloned).not.toBe(original);
    });

    it('should clone a Date', () => {
      const original = new Date('2025-01-15');
      const cloned = deepClone(original);

      expect(cloned).toEqual(original);
      expect(cloned).not.toBe(original);
    });

    it('should return primitives as-is', () => {
      expect(deepClone(null)).toBe(null);
      expect(deepClone(42)).toBe(42);
      expect(deepClone('string')).toBe('string');
    });
  });

  describe('escapeHtml', () => {
    it('should escape HTML special characters', () => {
      // escapeHtml escapes < and > but not quotes (textContent approach)
      expect(escapeHtml('<script>alert("xss")</script>')).toBe('&lt;script&gt;alert("xss")&lt;/script&gt;');
      expect(escapeHtml('A & B')).toBe('A &amp; B');
      expect(escapeHtml('5 > 3')).toBe('5 &gt; 3');
      expect(escapeHtml('1 < 2')).toBe('1 &lt; 2');
    });

    it('should return empty string for non-string input', () => {
      expect(escapeHtml(null)).toBe('');
      expect(escapeHtml(123)).toBe('');
    });

    it('should return unchanged string without special chars', () => {
      expect(escapeHtml('plain text')).toBe('plain text');
    });
  });
});
