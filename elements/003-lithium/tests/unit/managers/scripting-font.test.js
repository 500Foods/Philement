import { describe, it, expect } from 'vitest';
import {
  SCRIPT_FONT_SECTION,
  clearScriptFont,
  legacyScriptFontKey,
  fontFromPreferences,
  isScriptFontEnabled,
  loadScriptFont,
  saveScriptFont,
  SCRIPT_FONT_PREFERENCES_KEY,
} from '../../../src/managers/scripting/scripting-font.js';

function memorySettings(initial = {}) {
  const data = JSON.parse(JSON.stringify(initial));
  return {
    data,
    get(path, fallback = null) {
      const keys = String(path).split('.');
      let current = data;
      for (const key of keys) {
        if (current && typeof current === 'object' && Object.prototype.hasOwnProperty.call(current, key)) {
          current = current[key];
        } else {
          return fallback;
        }
      }
      return current;
    },
    set(path, value) {
      const keys = String(path).split('.');
      let current = data;
      for (let i = 0; i < keys.length - 1; i += 1) {
        const key = keys[i];
        if (!current[key] || typeof current[key] !== 'object') current[key] = {};
        current = current[key];
      }
      current[keys[keys.length - 1]] = value;
      return this;
    },
    removeSection(path) {
      const keys = String(path).split('.');
      let current = data;
      for (let i = 0; i < keys.length - 1; i += 1) {
        const key = keys[i];
        if (!current[key] || typeof current[key] !== 'object') return this;
        current = current[key];
      }
      delete current[keys[keys.length - 1]];
      return this;
    },
  };
}

function memoryLegacy(initial = {}) {
  const items = { ...initial };
  return {
    items,
    getItem(key) {
      return Object.prototype.hasOwnProperty.call(items, key) ? items[key] : null;
    },
    removeItem(key) {
      delete items[key];
    },
  };
}

const saved = {
  fontSize: '16px',
  fontFamily: 'var(--font-mono)',
  fontWeight: 'bold',
};

describe('isScriptFontEnabled', () => {
  it('is on for Lua and Preview, and off for Run', () => {
    expect(isScriptFontEnabled('lua')).toBe(true);
    expect(isScriptFontEnabled('preview')).toBe(true);
    expect(isScriptFontEnabled('run')).toBe(false);
  });
});

describe('loadScriptFont', () => {
  it('reads section 33 from lithiumSettings', () => {
    const settings = memorySettings({
      [SCRIPT_FONT_SECTION]: { font: { lua: saved } },
    });
    expect(loadScriptFont(settings, 'lua')).toEqual(saved);
  });

  it('restores a font the server copy dropped and writes it back', () => {
    const settings = memorySettings();
    const legacy = memoryLegacy({
      [SCRIPT_FONT_PREFERENCES_KEY]: JSON.stringify({
        [SCRIPT_FONT_SECTION]: { font: { lua: saved } },
      }),
    });

    expect(loadScriptFont(settings, 'lua', legacy)).toEqual(saved);
    expect(settings.get('33.font.lua')).toEqual(saved);
    expect(fontFromPreferences('not json', 'lua')).toBeNull();
  });

  it('copies an old localStorage font once and removes that key', () => {
    const settings = memorySettings();
    const legacy = memoryLegacy({
      [legacyScriptFontKey('lua')]: JSON.stringify(saved),
    });

    expect(loadScriptFont(settings, 'lua', legacy)).toEqual(saved);
    expect(settings.data[SCRIPT_FONT_SECTION].font.lua).toEqual(saved);
    expect(legacy.items[legacyScriptFontKey('lua')]).toBeUndefined();
    expect(loadScriptFont(settings, 'lua', memoryLegacy())).toEqual(saved);
  });

  it('does not read the old key when settings are missing', () => {
    const legacy = memoryLegacy({
      [legacyScriptFontKey('preview')]: JSON.stringify(saved),
    });
    expect(loadScriptFont(null, 'preview', legacy)).toBeNull();
    expect(legacy.items[legacyScriptFontKey('preview')]).toBe(JSON.stringify(saved));
  });
});

describe('saveScriptFont and clearScriptFont', () => {
  it('stores a normalized font and clears it with the old key', () => {
    const settings = memorySettings();
    const legacy = memoryLegacy({
      [legacyScriptFontKey('preview')]: '{"fontSize":"12px","fontFamily":"x"}',
    });

    expect(saveScriptFont(settings, 'preview', { ...saved, extra: true })).toBe(true);
    expect(settings.data[SCRIPT_FONT_SECTION].font.preview).toEqual(saved);
    expect(clearScriptFont(settings, 'preview', legacy)).toBe(true);
    expect(settings.data[SCRIPT_FONT_SECTION].font.preview).toBeUndefined();
    expect(legacy.items[legacyScriptFontKey('preview')]).toBeUndefined();
  });

  it('refuses a font with no family', () => {
    const settings = memorySettings();
    expect(saveScriptFont(settings, 'lua', { fontSize: '14px' })).toBe(false);
    expect(settings.data[SCRIPT_FONT_SECTION]).toBeUndefined();
  });
});
