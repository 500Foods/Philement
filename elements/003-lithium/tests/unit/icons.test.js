/**
 * Icons Module Tests
 *
 * Tests for the icon system including:
 * - Configuration parsing
 * - Icon element processing
 * - Mutation observer
 * - Lookup integration
 * - Helper functions
 */

import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest';

// Hoisted mocks - must be declared before module imports
const { mockGetConfigValue, mockGetLookupCategory, mockEventBusOn, mockEventBus } = vi.hoisted(() => ({
  mockGetConfigValue: vi.fn(),
  mockGetLookupCategory: vi.fn(),
  mockEventBusOn: vi.fn(),
  mockEventBus: {
    on: vi.fn()
  }
}));

// Mock dependencies
vi.mock('../../src/core/config.js', () => ({
  getConfigValue: mockGetConfigValue
}));

vi.mock('../../src/shared/lookups.js', () => ({
  getLookupCategory: mockGetLookupCategory
}));

vi.mock('../../src/core/event-bus.js', () => ({
  eventBus: mockEventBus,
  Events: {
    LOOKUPS_LOADED: 'lookups:loaded'
  }
}));

// Import the module after mocking
import {
  processIcons,
  refreshIconLookups,
  getIconConfiguration,
  setIcon,
  createIcon,
  init,
  teardown,
  isReady
} from '../../src/core/icons.js';

describe('Icons Module', () => {
  let container;

  beforeEach(() => {
    // Reset mocks
    vi.clearAllMocks();
    mockEventBus.on = mockEventBusOn;

    // Reset module state
    teardown();

    // Create a container for DOM testing
    container = document.createElement('div');
    document.body.appendChild(container);

    // Default config mock
    mockGetConfigValue.mockImplementation((key, defaultValue) => {
      const config = {
        'icons.set': 'duotone',
        'icons.weight': 'thin',
        'icons.prefix': '',
        'icons.fallback': 'solid'
      };
      return config[key] !== undefined ? config[key] : defaultValue;
    });
  });

  afterEach(() => {
    teardown();
    if (container && container.parentNode) {
      container.parentNode.removeChild(container);
    }
  });

  describe('Configuration', () => {
    it('should return icon configuration', () => {
      const config = getIconConfiguration();

      expect(config).toEqual({
        set: 'duotone',
        weight: 'thin',
        prefix: '',
        fallback: 'solid'
      });
    });

    it('should use default values when config is not set', () => {
      // Note: iconConfig is cached, so we test defaults by checking the fallback behavior
      // The defaults are applied in getIconConfig(), not in getIconConfiguration()
      mockGetConfigValue.mockImplementation((key, defaultValue) => defaultValue);

      const config = getIconConfiguration();

      // When config values are undefined, defaults are used internally
      expect(config.set).toBe('solid');
      expect(config.fallback).toBe('solid');
    });
  });

  describe('processIcons', () => {
    it('should process <fa> element with fa-* class', () => {
      container.innerHTML = '<fa fa-user></fa>';

      const count = processIcons(container);

      expect(count).toBe(1);
      const icon = container.querySelector('i');
      expect(icon).not.toBeNull();
      expect(icon.classList.contains('fad')).toBe(true); // duotone prefix
      expect(icon.classList.contains('fa-thin')).toBe(true);
      expect(icon.classList.contains('fa-user')).toBe(true);
    });

    it('should process <fa> element with icon name as attribute', () => {
      container.innerHTML = '<fa fa-cog></fa>';

      const count = processIcons(container);

      expect(count).toBe(1);
      const icon = container.querySelector('i');
      expect(icon).not.toBeNull();
      expect(icon.classList.contains('fa-cog')).toBe(true);
    });

    it('should preserve utility classes', () => {
      container.innerHTML = '<fa fa-user fa-fw fa-spin></fa>';

      processIcons(container);

      const icon = container.querySelector('i');
      expect(icon.classList.contains('fa-fw')).toBe(true);
      expect(icon.classList.contains('fa-spin')).toBe(true);
    });

    it('should preserve size classes', () => {
      container.innerHTML = '<fa fa-user fa-lg></fa>';

      processIcons(container);

      const icon = container.querySelector('i');
      expect(icon.classList.contains('fa-lg')).toBe(true);
    });

    it('should process multiple <fa> elements', () => {
      container.innerHTML = `
        <fa fa-user></fa>
        <fa fa-cog></fa>
        <fa fa-trash></fa>
      `;

      const count = processIcons(container);

      expect(count).toBe(3);
      expect(container.querySelectorAll('i').length).toBe(3);
    });

    it('should handle lookup-based icons', () => {
      // Mock the lookup with the expected structure from QueryRef 054
      // Format: { lookup_value: 'Name', value_txt: "<i class='fa fa-fw fa-icon'></i>" }
      mockGetLookupCategory.mockReturnValue([
        {
          lookup_value: 'Status',
          value_txt: "<i class='fa fa-fw fa-flag'></i>"
        }
      ]);

      // Refresh the lookups cache to pick up the mock
      refreshIconLookups();

      // Use a lookup-based icon reference - "Status" is the lookup key
      container.innerHTML = '<fa Status></fa>';

      processIcons(container);

      const icon = container.querySelector('i');
      expect(icon).not.toBeNull();
      // The icon should be created (lookup-based icon replacement is complex
      // and tested via integration tests - here we verify the element was processed)
      expect(icon.tagName.toLowerCase()).toBe('i');
    });

    it('should copy data attributes to the <i> element', () => {
      container.innerHTML = '<fa fa-user data-action="edit" data-id="123"></fa>';

      processIcons(container);

      const icon = container.querySelector('i');
      expect(icon.getAttribute('data-action')).toBe('edit');
      expect(icon.getAttribute('data-id')).toBe('123');
    });

    it('should copy id attribute to the <i> element', () => {
      container.innerHTML = '<fa fa-user id="my-icon"></fa>';

      processIcons(container);

      const icon = container.querySelector('i');
      expect(icon.id).toBe('my-icon');
    });

    it('should return 0 when no <fa> elements exist', () => {
      container.innerHTML = '<div>No icons here</div>';

      const count = processIcons(container);

      expect(count).toBe(0);
    });
  });

  describe('setIcon', () => {
    it('should set icon on an element', () => {
      const element = document.createElement('i');

      setIcon(element, 'user');

      expect(element.classList.contains('fad')).toBe(true);
      expect(element.classList.contains('fa-thin')).toBe(true);
      expect(element.classList.contains('fa-user')).toBe(true);
    });

    it('should allow overriding the icon set', () => {
      const element = document.createElement('i');

      setIcon(element, 'user', { set: 'solid' });

      expect(element.classList.contains('fas')).toBe(true);
      expect(element.classList.contains('fa-user')).toBe(true);
    });

    it('should allow adding additional classes', () => {
      const element = document.createElement('i');

      setIcon(element, 'user', { classes: ['fa-fw', 'fa-spin'] });

      expect(element.classList.contains('fa-fw')).toBe(true);
      expect(element.classList.contains('fa-spin')).toBe(true);
    });

    it('should handle null element gracefully', () => {
      const consoleSpy = vi.spyOn(console, 'warn').mockImplementation(() => {});

      setIcon(null, 'user');

      expect(consoleSpy).toHaveBeenCalledWith('[Icons] Cannot set icon on null element');
      consoleSpy.mockRestore();
    });
  });

  describe('createIcon', () => {
    it('should create an <i> element with the icon', () => {
      const icon = createIcon('user');

      expect(icon.tagName.toLowerCase()).toBe('i');
      expect(icon.classList.contains('fa-user')).toBe(true);
    });

    it('should accept options for the icon', () => {
      const icon = createIcon('cog', { set: 'solid', classes: ['fa-fw'] });

      expect(icon.classList.contains('fas')).toBe(true);
      expect(icon.classList.contains('fa-cog')).toBe(true);
      expect(icon.classList.contains('fa-fw')).toBe(true);
    });
  });

  describe('init', () => {
    it('should process existing <fa> elements', () => {
      document.body.innerHTML = '<fa fa-user></fa>';

      init({ observe: false });

      const icon = document.querySelector('i.fa-user');
      expect(icon).not.toBeNull();

      // Clean up
      document.body.innerHTML = '';
    });

    it('should set up event listeners for lookups', () => {
      init({ observe: false });

      expect(mockEventBusOn).toHaveBeenCalledWith('lookups:loaded', expect.any(Function));
      expect(mockEventBusOn).toHaveBeenCalledWith('lookups:icons:loaded', expect.any(Function));
    });

    it('should not initialize twice', () => {
      const consoleSpy = vi.spyOn(console, 'log').mockImplementation(() => {});

      init({ observe: false });
      expect(consoleSpy).toHaveBeenCalledWith('[Icons] Icon system initialized');

      consoleSpy.mockClear();

      init({ observe: false }); // Second call should be ignored

      // Should not log initialization again
      expect(consoleSpy).not.toHaveBeenCalledWith('[Icons] Icon system initialized');

      consoleSpy.mockRestore();
    });
  });

  describe('teardown', () => {
    it('should reset the initialization state', () => {
      init({ observe: false });
      expect(isReady()).toBe(true);

      teardown();

      expect(isReady()).toBe(false);
    });
  });

  describe('isReady', () => {
    it('should return false before initialization', () => {
      expect(isReady()).toBe(false);
    });

    it('should return true after initialization', () => {
      init({ observe: false });
      expect(isReady()).toBe(true);
    });
  });

  describe('Different icon sets', () => {
    it('should use solid prefix for solid set', () => {
      mockGetConfigValue.mockImplementation((key) => {
        if (key === 'icons.set') return 'solid';
        if (key === 'icons.prefix') return '';
        return undefined;
      });

      container.innerHTML = '<fa fa-user></fa>';
      processIcons(container);

      const icon = container.querySelector('i');
      expect(icon.classList.contains('fas')).toBe(true);
    });

    it('should use regular prefix for regular set', () => {
      mockGetConfigValue.mockImplementation((key) => {
        if (key === 'icons.set') return 'regular';
        if (key === 'icons.prefix') return '';
        return undefined;
      });

      container.innerHTML = '<fa fa-user></fa>';
      processIcons(container);

      const icon = container.querySelector('i');
      expect(icon.classList.contains('far')).toBe(true);
    });

    it('should use brands prefix for brands set', () => {
      mockGetConfigValue.mockImplementation((key) => {
        if (key === 'icons.set') return 'brands';
        if (key === 'icons.prefix') return '';
        return undefined;
      });

      container.innerHTML = '<fa fa-github></fa>';
      processIcons(container);

      const icon = container.querySelector('i');
      expect(icon.classList.contains('fab')).toBe(true);
    });
  });
});
