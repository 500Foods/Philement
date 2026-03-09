/**
 * Languages Module Unit Tests
 *
 * Tests for the language support module including locale detection,
 * display name generation, and data ordering.
 */

import { describe, it, expect, beforeEach, vi } from 'vitest';
import {
  countryToLocale,
  supportedLocales,
  localeToCountryCode,
  getCountryCode,
  getLanguageName,
  getCountryName,
  getLocaleDisplayName,
  getLocaleNativeName,
  getLanguageData,
  getSortedLanguageList,
  saveLocalePreference,
  getSavedLocale,
} from '../../src/shared/languages.js';

describe('Languages Module', () => {
  describe('Constants', () => {
    it('should have country to locale mappings', () => {
      expect(countryToLocale['US']).toBe('en-US');
      expect(countryToLocale['GB']).toBe('en-GB');
      expect(countryToLocale['CA']).toBe('en-CA');
      expect(countryToLocale['FR']).toBe('fr-FR');
    });

    it('should have supported locales list', () => {
      expect(supportedLocales.length).toBeGreaterThan(0);
      expect(supportedLocales).toContain('en-US');
      expect(supportedLocales).toContain('fr-CA');
      expect(supportedLocales).toContain('en-CA');
    });

    it('should have country code mappings', () => {
      expect(localeToCountryCode['en-US']).toBe('US');
      expect(localeToCountryCode['fr-CA']).toBe('CA');
      expect(localeToCountryCode['en-CA']).toBe('CA');
    });

    it('should get country code for locale', () => {
      expect(getCountryCode('en-US')).toBe('US');
      expect(getCountryCode('fr-CA')).toBe('CA');
      expect(getCountryCode('unknown')).toBe('US');
    });
  });

  describe('getLanguageName', () => {
    it('should return language name without country', () => {
      const name = getLanguageName('en-US');
      expect(name.toLowerCase()).toContain('english');
      expect(name).not.toContain('United States');
    });

    it('should return language name for French', () => {
      const name = getLanguageName('fr-CA');
      expect(name.toLowerCase()).toContain('french');
    });

    it('should handle base language codes', () => {
      const name = getLanguageName('de');
      expect(name.toLowerCase()).toContain('german');
    });
  });

  describe('getCountryName', () => {
    it('should return country name for locale', () => {
      const name = getCountryName('en-US');
      expect(name).toContain('United States');
    });

    it('should return country name for Canadian French', () => {
      const name = getCountryName('fr-CA');
      expect(name).toContain('Canada');
    });

    it('should return empty string for base language codes', () => {
      const name = getCountryName('en');
      expect(name).toBe('');
    });
  });

  describe('getLocaleDisplayName', () => {
    it('should return English display name for locale', () => {
      const name = getLocaleDisplayName('en-US');
      expect(name).toContain('English');
      expect(name).toContain('United States');
    });

    it('should return display name for Canadian French', () => {
      const name = getLocaleDisplayName('fr-CA');
      expect(name).toContain('French');
      expect(name).toContain('Canada');
    });

    it('should handle base language codes', () => {
      const name = getLocaleDisplayName('en');
      expect(name.toLowerCase()).toContain('english');
    });
  });

  describe('getLocaleNativeName', () => {
    it('should return native display name for locale', () => {
      const name = getLocaleNativeName('fr-FR');
      // Native name should contain français or Français
      expect(name.toLowerCase()).toContain('français');
    });

    it('should return native name for Japanese', () => {
      const name = getLocaleNativeName('ja-JP');
      expect(name).toContain('日本語');
    });
  });

  describe('getLanguageData', () => {
    it('should return array of language data objects', () => {
      const data = getLanguageData();
      expect(Array.isArray(data)).toBe(true);
      expect(data.length).toBe(supportedLocales.length);

      // Check structure of first item
      const firstItem = data[0];
      expect(firstItem).toHaveProperty('locale');
      expect(firstItem).toHaveProperty('countryCode');
      expect(firstItem).toHaveProperty('language');
      expect(firstItem).toHaveProperty('country');
      expect(firstItem).toHaveProperty('nativeName');
    });

    it('should have valid country code for each locale', () => {
      const data = getLanguageData();
      for (const item of data) {
        expect(item.countryCode).toBeTruthy();
        expect(typeof item.countryCode).toBe('string');
        expect(item.countryCode.length).toBe(2);
      }
    });

    it('should have separate language and country fields', () => {
      const data = getLanguageData();
      const usItem = data.find(item => item.locale === 'en-US');
      expect(usItem).toBeDefined();
      expect(usItem.language.toLowerCase()).toContain('english');
      expect(usItem.country).toContain('United States');
    });
  });

  describe('getSortedLanguageList', () => {
    it('should return all supported locales', () => {
      const list = getSortedLanguageList();
      expect(list.length).toBe(supportedLocales.length);
    });

    it('should sort by language name then country name', () => {
      const list = getSortedLanguageList();

      for (let i = 1; i < list.length; i++) {
        const langCompare = list[i].language.localeCompare(list[i - 1].language);
        if (langCompare === 0) {
          // Same language — country must be in order
          expect(list[i].country.localeCompare(list[i - 1].country)).toBeGreaterThanOrEqual(0);
        } else {
          expect(langCompare).toBeGreaterThanOrEqual(0);
        }
      }
    });

    it('should have standard language data fields', () => {
      const list = getSortedLanguageList();
      const firstItem = list[0];
      expect(firstItem).toHaveProperty('locale');
      expect(firstItem).toHaveProperty('countryCode');
      expect(firstItem).toHaveProperty('language');
      expect(firstItem).toHaveProperty('country');
      expect(firstItem).toHaveProperty('nativeName');
      // No recommended field
      expect(firstItem).not.toHaveProperty('recommended');
    });

    it('should contain en-US', () => {
      const list = getSortedLanguageList();
      const usItem = list.find(item => item.locale === 'en-US');
      expect(usItem).toBeDefined();
    });
  });

  describe('saveLocalePreference / getSavedLocale', () => {
    beforeEach(() => {
      // Clear localStorage before each test
      vi.stubGlobal('localStorage', {
        getItem: vi.fn(),
        setItem: vi.fn(),
        removeItem: vi.fn(),
      });
    });

    it('should save locale to localStorage', () => {
      saveLocalePreference('fr-CA');
      expect(localStorage.setItem).toHaveBeenCalledWith('lithium_user_locale', 'fr-CA');
    });

    it('should get saved locale from localStorage', () => {
      localStorage.getItem.mockReturnValue('de-DE');
      const locale = getSavedLocale();
      expect(locale).toBe('de-DE');
      expect(localStorage.getItem).toHaveBeenCalledWith('lithium_user_locale');
    });

    it('should return null when no locale saved', () => {
      localStorage.getItem.mockReturnValue(null);
      const locale = getSavedLocale();
      expect(locale).toBeNull();
    });

    it('should handle localStorage errors gracefully', () => {
      localStorage.setItem.mockImplementation(() => {
        throw new Error('Quota exceeded');
      });
      // Should not throw
      expect(() => saveLocalePreference('en-US')).not.toThrow();
    });
  });
});
