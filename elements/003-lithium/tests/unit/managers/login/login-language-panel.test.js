/**
 * Login Language Panel Tests
 *
 * Unit tests for the LanguagePanel class extracted from login.js.
 */

import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest';
import { LanguagePanel } from '../../../../src/managers/login/login-language-panel.js';

// Mock the log module
vi.mock('../../../../src/core/log.js', () => ({
  log: vi.fn(),
  Subsystems: { LOGIN: 'LOGIN' },
  Status: { INFO: 'INFO', SUCCESS: 'SUCCESS', ERROR: 'ERROR', WARN: 'WARN' },
}));

// Mock the event bus
vi.mock('../../../../src/core/event-bus.js', () => ({
  eventBus: {
    emit: vi.fn(),
    on: vi.fn(),
    off: vi.fn(),
    once: vi.fn(),
  },
  Events: {
    LOCALE_CHANGED: 'locale-changed',
  },
}));

// Mock the config module
vi.mock('../../../../src/core/config.js', () => ({
  getConfigValue: vi.fn((key, defaultValue) => defaultValue),
}));

// Mock the languages module
vi.mock('../../../../src/shared/languages.js', () => ({
  getBestGuessLocale: vi.fn(() => Promise.resolve('en-US')),
  getLanguageData: vi.fn(() => [
    { locale: 'en-US', countryCode: 'US', language: 'English', country: 'United States', nativeName: 'English' },
    { locale: 'fr-CA', countryCode: 'CA', language: 'French', country: 'Canada', nativeName: 'Français' },
    { locale: 'es-MX', countryCode: 'MX', language: 'Spanish', country: 'Mexico', nativeName: 'Español' },
  ]),
  getCountryCode: vi.fn((locale) => locale.split('-')[1] || 'US'),
  saveLocalePreference: vi.fn(),
  getSavedLocale: vi.fn(() => null),
  supportedLocales: ['en-US', 'fr-CA', 'es-MX'],
}));

// Mock the log-formatter module
vi.mock('../../../../src/shared/log-formatter.js', () => ({
  getFlagSvg: vi.fn((code) => `<svg data-flag="${code}"></svg>`),
}));

// Mock tabulator-tables
vi.mock('tabulator-tables', () => ({
  TabulatorFull: vi.fn(function(container, options) {
    this.options = options;
    this._data = options.data || [];
    this._selectedRows = [];
    this._rows = this._data.map((d, i) => ({
      getData: () => d,
      select: () => {
        this._selectedRows = [d];
      },
      deselect: () => {},
    }));
    this._callbacks = {};

    this.on = vi.fn((event, callback) => {
      this._callbacks[event] = callback;
    });
    this.getRows = vi.fn((type) => {
      if (type === 'active') return this._rows;
      return this._rows;
    });
    this.getSelectedRows = vi.fn(() =>
      this._selectedRows.map(d => ({
        getData: () => d,
      }))
    );
    this.deselectRow = vi.fn(() => {
      this._selectedRows = [];
    });
    this.select = vi.fn((row) => {
      this._selectedRows = [row.getData()];
    });
    this.scrollToRow = vi.fn();
    this.clearFilter = vi.fn();
    this.setFilter = vi.fn();
    this.destroy = vi.fn();
  }),
}));

function createMockElements() {
  return {
    languageTable: document.createElement('div'),
    languageFilter: document.createElement('input'),
    languageClearFilter: document.createElement('button'),
    languageCurrentBtn: document.createElement('button'),
    languageCurrentFlag: document.createElement('span'),
    languageCurrentCode: document.createElement('span'),
  };
}

describe('LanguagePanel', () => {
  let panel;
  let elements;
  let onLocaleSelected;

  beforeEach(() => {
    elements = createMockElements();
    onLocaleSelected = vi.fn();
    panel = new LanguagePanel({ elements, onLocaleSelected });
  });

  afterEach(() => {
    panel.destroy();
    vi.clearAllMocks();
  });

  describe('constructor', () => {
    it('should initialize with null state', () => {
      expect(panel._languageTable).toBeNull();
      expect(panel._currentLocale).toBeNull();
      expect(panel._bestGuessLocale).toBeNull();
      expect(panel._languageData).toEqual([]);
    });

    it('should store elements and callback', () => {
      expect(panel.elements).toBe(elements);
      expect(panel._onLocaleSelected).toBe(onLocaleSelected);
    });

    it('should use default callback when none provided', () => {
      const defaultPanel = new LanguagePanel({ elements });
      expect(() => defaultPanel._onLocaleSelected('en-US', null)).not.toThrow();
      defaultPanel.destroy();
    });
  });

  describe('init', () => {
    it('should create a language table on first init', async () => {
      await panel.init();
      expect(panel._languageTable).not.toBeNull();
      expect(panel._languageData.length).toBeGreaterThan(0);
    });

    it('should set current locale from best guess when no saved locale', async () => {
      await panel.init();
      expect(panel._currentLocale).toBe('en-US');
      expect(panel._bestGuessLocale).toBe('en-US');
    });

    it('should not recreate table on subsequent init calls', async () => {
      await panel.init();
      const firstTable = panel._languageTable;
      await panel.init();
      expect(panel._languageTable).toBe(firstTable);
    });

    it('should handle missing table container gracefully', async () => {
      panel.elements.languageTable = null;
      await expect(panel.init()).resolves.not.toThrow();
      expect(panel._languageTable).toBeNull();
    });
  });

  describe('show/hide', () => {
    it('should init and attach keyboard handler on show', async () => {
      await panel.show();
      expect(panel._languageTable).not.toBeNull();
      expect(panel._keydownHandler).not.toBeNull();
    });

    it('should reset filter and remove keyboard handler on hide', async () => {
      await panel.show();
      elements.languageFilter.value = 'test';
      panel.hide();
      expect(elements.languageFilter.value).toBe('');
      expect(panel._keydownHandler).toBeNull();
    });
  });

  describe('destroy', () => {
    it('should destroy table and clean up', async () => {
      await panel.init();
      const table = panel._languageTable;
      panel.destroy();
      expect(table.destroy).toHaveBeenCalled();
      expect(panel._languageTable).toBeNull();
      expect(panel._keydownHandler).toBeNull();
    });

    it('should not throw when called without init', () => {
      expect(() => panel.destroy()).not.toThrow();
    });
  });

  describe('selectLanguage', () => {
    beforeEach(async () => {
      await panel.init();
    });

    it('should update current locale', () => {
      panel.selectLanguage('fr-CA');
      expect(panel._currentLocale).toBe('fr-CA');
    });

    it('should reject unsupported locales', () => {
      const consoleSpy = vi.spyOn(console, 'warn').mockImplementation(() => {});
      panel.selectLanguage('unsupported');
      expect(panel._currentLocale).not.toBe('unsupported');
      expect(consoleSpy).toHaveBeenCalled();
      consoleSpy.mockRestore();
    });

    it('should call onLocaleSelected callback', () => {
      panel.selectLanguage('fr-CA');
      expect(onLocaleSelected).toHaveBeenCalledWith('fr-CA', 'en-US');
    });

    it('should emit LOCALE_CHANGED event', async () => {
      const { eventBus, Events } = await import('../../../../src/core/event-bus.js');
      panel.selectLanguage('fr-CA');
      expect(eventBus.emit).toHaveBeenCalledWith(
        Events.LOCALE_CHANGED,
        expect.objectContaining({ lang: 'fr-CA', previousLang: 'en-US' })
      );
    });
  });

  describe('filter', () => {
    beforeEach(async () => {
      await panel.init();
    });

    it('should apply filter to table', () => {
      panel.filter('French');
      expect(panel._languageTable.setFilter).toHaveBeenCalled();
    });

    it('should clear filter when text is empty', () => {
      panel.filter('');
      expect(panel._languageTable.clearFilter).toHaveBeenCalled();
    });

    it('should do nothing when table is not initialized', () => {
      panel.destroy();
      expect(() => panel.filter('test')).not.toThrow();
    });
  });

  describe('keyboard navigation', () => {
    beforeEach(async () => {
      await panel.init();
    });

    it('should handle ArrowDown', () => {
      const event = new KeyboardEvent('keydown', { key: 'ArrowDown' });
      const preventDefault = vi.spyOn(event, 'preventDefault');
      panel._handleKeydown(event);
      expect(preventDefault).toHaveBeenCalled();
    });

    it('should handle ArrowUp', () => {
      const event = new KeyboardEvent('keydown', { key: 'ArrowUp' });
      const preventDefault = vi.spyOn(event, 'preventDefault');
      panel._handleKeydown(event);
      expect(preventDefault).toHaveBeenCalled();
    });

    it('should handle Enter to select current row', () => {
      // Select a row first
      panel._languageTable._selectedRows = [panel._languageData[1]];
      const event = new KeyboardEvent('keydown', { key: 'Enter' });
      const preventDefault = vi.spyOn(event, 'preventDefault');
      panel._handleKeydown(event);
      expect(preventDefault).toHaveBeenCalled();
      expect(panel._currentLocale).toBe('fr-CA');
    });

    it('should ignore unhandled keys', () => {
      const event = new KeyboardEvent('keydown', { key: 'a' });
      const preventDefault = vi.spyOn(event, 'preventDefault');
      panel._handleKeydown(event);
      expect(preventDefault).not.toHaveBeenCalled();
    });
  });

  describe('getters', () => {
    it('should return current locale', async () => {
      await panel.init();
      expect(panel.getCurrentLocale()).toBe('en-US');
    });

    it('should return best guess locale', async () => {
      await panel.init();
      expect(panel.getBestGuessLocale()).toBe('en-US');
    });
  });

  describe('fallback rendering', () => {
    it('should render fallback on table error', async () => {
      // Force Tabulator to throw by temporarily breaking the mock
      const { TabulatorFull } = await import('tabulator-tables');
      TabulatorFull.mockImplementationOnce(() => {
        throw new Error('Tabulator fail');
      });

      const container = document.createElement('div');
      panel.elements.languageTable = container;
      await panel.init();

      expect(container.querySelectorAll('.language-item-fallback').length).toBe(3);
    });
  });
});
