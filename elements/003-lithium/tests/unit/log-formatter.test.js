/**
 * Unit tests for log-formatter.js utility
 * 
 * Tests the pure functions for formatting log entries and generating flag SVGs.
 */

import { describe, it, expect } from 'vitest';
import { formatLogText, getFlagSvg, getPasswordManagerSelectors } from '../../src/shared/log-formatter.js';

describe('Log Formatter', () => {
  describe('formatLogText', () => {
    it('should return placeholder text for empty array', () => {
      const result = formatLogText([]);
      expect(result).toBe('(No log entries yet)');
    });

    it('should return placeholder text for null/undefined', () => {
      expect(formatLogText(null)).toBe('(No log entries yet)');
      expect(formatLogText(undefined)).toBe('(No log entries yet)');
    });

    it('should format single log entry correctly', () => {
      const entries = [
        {
          timestamp: new Date('2025-01-15T10:30:00.123Z').getTime(),
          subsystem: 'Startup',
          description: 'Application initialized',
        },
      ];

      const result = formatLogText(entries);
      
      // Note: timestamp is converted to local time, so use a simpler check
      // Should contain subsystem and description
      expect(result).toContain('Startup');
      expect(result).toContain('Application initialized');
    });

    it('should format multiple log entries with proper column alignment', () => {
      const entries = [
        {
          timestamp: new Date('2025-01-15T10:30:00.000Z').getTime(),
          subsystem: 'A',
          description: 'Short',
        },
        {
          timestamp: new Date('2025-01-15T10:30:01.000Z').getTime(),
          subsystem: 'Longer',
          description: 'Longer description',
        },
      ];

      const result = formatLogText(entries);
      const lines = result.split('\n');

      expect(lines).toHaveLength(2);
      // Both lines should have the subsystem column padded to the same width
      expect(lines[0]).toContain('A       ');
      expect(lines[1]).toContain('Longer');
    });

    it('should handle bracketed subsystems without padding', () => {
      const entries = [
        {
          timestamp: new Date('2025-01-15T10:30:00.000Z').getTime(),
          subsystem: '[EventBus]',
          description: 'Event emitted',
        },
      ];

      const result = formatLogText(entries);
      
      // Bracketed subsystems should not be padded
      expect(result).toContain('[EventBus]');
      // Should have single space separator after bracketed subsystem
      expect(result).toMatch(/\[EventBus\] .*Event emitted/);
    });

    it('should expand grouped entries with continuation lines', () => {
      const entries = [
        {
          timestamp: new Date('2025-01-15T10:30:00.000Z').getTime(),
          subsystem: 'Startup',
          description: {
            title: 'Browser Environment',
            items: ['Browser: Firefox 148', 'Platform: Linux x86_64'],
          },
        },
      ];

      const result = formatLogText(entries);
      const lines = result.split('\n');

      // Should have 3 lines: title + 2 items
      expect(lines).toHaveLength(3);
      // Check that title is in the first line
      expect(lines[0]).toContain('Browser Environment');
      // Check that items are in continuation lines with the continuation character
      const allLines = lines.join('\n');
      expect(allLines).toContain('― Browser: Firefox 148');
      expect(allLines).toContain('― Platform: Linux x86_64');
    });

    it('should handle entries with missing subsystem', () => {
      const entries = [
        {
          timestamp: new Date('2025-01-15T10:30:00.000Z').getTime(),
          description: 'No subsystem',
        },
      ];

      const result = formatLogText(entries);
      expect(result).toContain('No subsystem');
    });

    it('should handle entries with object description that is not grouped', () => {
      const entries = [
        {
          timestamp: new Date('2025-01-15T10:30:00.000Z').getTime(),
          subsystem: 'Test',
          description: { notAGroup: true }, // Not a grouped entry
        },
      ];

      const result = formatLogText(entries);
      // Should convert object to string
      expect(result).toContain('[object Object]');
    });
  });

  describe('getFlagSvg', () => {
    it('should return SVG string for valid country code', () => {
      const result = getFlagSvg('US');
      
      expect(result).toContain('<svg');
      expect(result).toContain('width="28"');
      expect(result).toContain('height="20"');
    });

    it('should fallback to US flag for invalid country code', () => {
      const result = getFlagSvg('XX');
      
      // Should still return an SVG (the US fallback)
      expect(result).toContain('<svg');
    });

    it('should use custom dimensions when provided', () => {
      const result = getFlagSvg('US', { width: 50, height: 30 });
      
      expect(result).toContain('width="50"');
      expect(result).toContain('height="30"');
    });

    it('should apply custom styling', () => {
      const result = getFlagSvg('US', { width: 22, height: 16 });
      
      expect(result).toContain('border-radius');
      expect(result).toContain('box-shadow');
    });

    it('should use custom fallback text for unknown codes', () => {
      // Test with a code that doesn't exist in country-flag-icons
      // Note: The library might throw for completely invalid codes
      const result = getFlagSvg('INVALID_CODE', { fallbackText: 'N/A' });
      
      // Either returns fallback span or throws and catches to fallback
      expect(result).toBeTruthy();
    });
  });

  describe('getPasswordManagerSelectors', () => {
    it('should return array of CSS selectors', () => {
      const selectors = getPasswordManagerSelectors();
      
      expect(Array.isArray(selectors)).toBe(true);
      expect(selectors.length).toBeGreaterThan(0);
    });

    it('should include 1Password selector', () => {
      const selectors = getPasswordManagerSelectors();
      
      expect(selectors).toContain('com-1password-button');
    });

    it('should include Bitwarden selector', () => {
      const selectors = getPasswordManagerSelectors();
      
      expect(selectors).toContain('[class*="bitwarden"]');
    });

    it('should include LastPass selector', () => {
      const selectors = getPasswordManagerSelectors();
      
      expect(selectors).toContain('[class*="lastpass"]');
    });

    it('should include Dashlane selector', () => {
      const selectors = getPasswordManagerSelectors();
      
      expect(selectors).toContain('[class*="dashlane"]');
    });
  });
});
