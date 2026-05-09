/**
 * Login Help Panel Tests
 *
 * Unit tests for the HelpPanel class extracted from login.js.
 */

import { describe, it, expect, vi, beforeEach } from 'vitest';
import { HelpPanel } from '../../../../src/managers/login/login-help-panel.js';

function makeElements() {
  return {
    versionBuild: document.createElement('span'),
    versionYear: document.createElement('span'),
    versionDate: document.createElement('span'),
    versionTime: document.createElement('span'),
    helpAppVersion: document.createElement('span'),
    helpBuildDate: document.createElement('span'),
  };
}

const SAMPLE_DATA = {
  build: 1234,
  // 2026-05-08 22:40 UTC = local-tz dependent for header fields, but UTC
  // for help-panel buildDate. Use fixed UTC string to keep assertions stable.
  timestamp: '2026-05-08T22:40:00Z',
  version: '0.1.1234',
};

describe('HelpPanel', () => {
  describe('constructor', () => {
    it('accepts elements and defaults to global window/fetch', () => {
      const elements = makeElements();
      const panel = new HelpPanel({ elements });
      expect(panel.elements).toBe(elements);
    });

    it('handles missing options', () => {
      const panel = new HelpPanel();
      expect(panel.elements).toEqual({});
    });
  });

  describe('init() data resolution', () => {
    it('reads from window.__lithiumVersionData when present (preferred path)', async () => {
      const elements = makeElements();
      const win = { __lithiumVersionData: SAMPLE_DATA };
      const fetchImpl = vi.fn();

      const panel = new HelpPanel({ elements, win, fetchImpl });
      await panel.init();

      expect(fetchImpl).not.toHaveBeenCalled();
      expect(elements.helpAppVersion.textContent).toBe('0.1.1234');
      expect(elements.helpBuildDate.textContent).toBe('2026-May-08 22:40 UTC');
      expect(elements.versionBuild.textContent).toBe('1234');
    });

    it('awaits window.__lithiumVersionPromise when no cache is present', async () => {
      const elements = makeElements();
      const win = { __lithiumVersionPromise: Promise.resolve(SAMPLE_DATA) };
      const fetchImpl = vi.fn();

      const panel = new HelpPanel({ elements, win, fetchImpl });
      await panel.init();

      expect(fetchImpl).not.toHaveBeenCalled();
      expect(elements.helpAppVersion.textContent).toBe('0.1.1234');
    });

    it('falls back to fetch(/version.json) when neither cache exists', async () => {
      const elements = makeElements();
      const win = {};
      const fetchImpl = vi.fn(() => Promise.resolve({
        ok: true,
        json: () => Promise.resolve(SAMPLE_DATA),
      }));

      const panel = new HelpPanel({ elements, win, fetchImpl });
      await panel.init();

      expect(fetchImpl).toHaveBeenCalledWith('/version.json');
      expect(elements.helpAppVersion.textContent).toBe('0.1.1234');
    });

    it('does nothing when fetch returns !ok and no cache exists', async () => {
      const elements = makeElements();
      const win = {};
      const fetchImpl = vi.fn(() => Promise.resolve({ ok: false }));

      const panel = new HelpPanel({ elements, win, fetchImpl });
      await panel.init();

      expect(elements.helpAppVersion.textContent).toBe('');
      expect(elements.versionBuild.textContent).toBe('');
    });

    it('catches and logs network errors without throwing', async () => {
      const elements = makeElements();
      const win = {};
      const fetchImpl = vi.fn(() => Promise.reject(new Error('offline')));
      const warn = vi.spyOn(console, 'warn').mockImplementation(() => {});

      const panel = new HelpPanel({ elements, win, fetchImpl });
      await expect(panel.init()).resolves.toBeUndefined();
      expect(warn).toHaveBeenCalledWith(
        expect.stringContaining('Could not load version info'),
        expect.any(Error),
      );
      warn.mockRestore();
    });
  });

  describe('init() rendering', () => {
    it('populates all six DOM fields when version data is complete', async () => {
      const elements = makeElements();
      const panel = new HelpPanel({
        elements,
        win: { __lithiumVersionData: SAMPLE_DATA },
      });
      await panel.init();

      // Header (uses local time — assert non-empty rather than exact value).
      expect(elements.versionBuild.textContent).toBe('1234');
      expect(elements.versionYear.textContent).not.toBe('');
      expect(elements.versionDate.textContent).toMatch(/^\d{4}$/);
      expect(elements.versionTime.textContent).toMatch(/^\d{4}$/);
      // Help panel (UTC, deterministic).
      expect(elements.helpAppVersion.textContent).toBe('0.1.1234');
      expect(elements.helpBuildDate.textContent).toBe('2026-May-08 22:40 UTC');
    });

    it('falls back to "0.1.<build>" when version field is missing', async () => {
      const elements = makeElements();
      const data = { build: 99, timestamp: SAMPLE_DATA.timestamp };
      const panel = new HelpPanel({
        elements,
        win: { __lithiumVersionData: data },
      });
      await panel.init();

      expect(elements.helpAppVersion.textContent).toBe('0.1.99');
    });

    it('skips header fields when timestamp is missing', async () => {
      const elements = makeElements();
      const data = { build: 99, version: '0.1.99' };
      const panel = new HelpPanel({
        elements,
        win: { __lithiumVersionData: data },
      });
      await panel.init();

      expect(elements.versionBuild.textContent).toBe('');
      expect(elements.helpAppVersion.textContent).toBe('0.1.99');
      // helpBuildDate falls back to the raw timestamp (which is undefined,
      // coerced by DOM textContent setter to the empty string).
      expect(elements.helpBuildDate.textContent).toBe('');
    });

    it('handles missing optional element refs gracefully', async () => {
      // Only some of the fields are present.
      const elements = {
        helpAppVersion: document.createElement('span'),
      };
      const panel = new HelpPanel({
        elements,
        win: { __lithiumVersionData: SAMPLE_DATA },
      });
      await expect(panel.init()).resolves.toBeUndefined();
      expect(elements.helpAppVersion.textContent).toBe('0.1.1234');
    });
  });

  describe('lifecycle', () => {
    it('show() and hide() are no-ops', () => {
      const panel = new HelpPanel({ elements: makeElements() });
      expect(() => panel.show()).not.toThrow();
      expect(() => panel.hide()).not.toThrow();
    });

    it('destroy() clears the element references', () => {
      const panel = new HelpPanel({ elements: makeElements() });
      panel.destroy();
      expect(panel.elements).toEqual({});
    });
  });
});
