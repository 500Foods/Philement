/**
 * Scripting font prefs live on window.lithiumSettings.
 * That service takes one dotted path: 33.font.<tab>.
 * An old lithium_scripting_font_* value is copied once, then removed.
 */

export const SCRIPT_FONT_SECTION = '33';

/** GlobalSettingsService.storageKey. Login replaces the in-memory copy from the server. */
export const SCRIPT_FONT_PREFERENCES_KEY = 'lithium_preferences';

/**
 * @param {string} tabId
 * @returns {string}
 */
export function scriptFontPath(tabId) {
  return `font.${tabId}`;
}

/**
 * @param {string} tabId
 * @returns {string}
 */
export function scriptFontStoragePath(tabId) {
  return `${SCRIPT_FONT_SECTION}.${scriptFontPath(tabId)}`;
}

/**
 * Font applies to the Lua editor and the Preview pane.
 * Run has no font target. Pretty Print stays on Lua only.
 * @param {string} tabId
 * @returns {boolean}
 */
export function isScriptFontEnabled(tabId) {
  return tabId === 'lua' || tabId === 'preview';
}

/**
 * @param {string} tabId
 * @returns {string}
 */
export function legacyScriptFontKey(tabId) {
  return `lithium_scripting_font_${tabId}`;
}

/**
 * @param {unknown} value
 * @returns {{ fontSize: string, fontFamily: string, fontWeight: string } | null}
 */
export function normalizeScriptFont(value) {
  if (!value || typeof value !== 'object') return null;
  const fontSize = value.fontSize;
  const fontFamily = value.fontFamily;
  if (typeof fontSize !== 'string' || fontSize.trim() === '') return null;
  if (typeof fontFamily !== 'string' || fontFamily.trim() === '') return null;
  const fontWeight = typeof value.fontWeight === 'string' && value.fontWeight.trim() !== ''
    ? value.fontWeight
    : 'normal';
  return { fontSize, fontFamily, fontWeight };
}

/**
 * Walk a dotted path through a preferences object.
 * @param {unknown} data
 * @param {string} path
 * @returns {unknown}
 */
export function valueAtPath(data, path) {
  const keys = path.split('.');
  let current = data;
  for (const key of keys) {
    if (!current || typeof current !== 'object' || !Object.prototype.hasOwnProperty.call(current, key)) {
      return null;
    }
    current = current[key];
  }
  return current;
}

/**
 * Read a font out of the raw lithium_preferences JSON.
 * @param {unknown} raw
 * @param {string} tabId
 * @returns {{ fontSize: string, fontFamily: string, fontWeight: string } | null}
 */
export function fontFromPreferences(raw, tabId) {
  if (typeof raw !== 'string' || raw === '') return null;
  let data;
  try {
    data = JSON.parse(raw);
  } catch {
    return null;
  }
  return normalizeScriptFont(valueAtPath(data, scriptFontStoragePath(tabId)));
}

/**
 * @param {{ get?: Function, set?: Function } | null | undefined} settings
 * @param {string} tabId
 * @param {{ getItem?: Function, removeItem?: Function } | null} [legacy]
 * @returns {{ fontSize: string, fontFamily: string, fontWeight: string } | null}
 */
export function loadScriptFont(settings, tabId, legacy = null) {
  const path = scriptFontStoragePath(tabId);
  const stored = normalizeScriptFont(
    typeof settings?.get === 'function' ? settings.get(path, null) : null,
  );
  if (stored) return stored;

  const fromPreferences = typeof legacy?.getItem === 'function'
    ? fontFromPreferences(legacy.getItem(SCRIPT_FONT_PREFERENCES_KEY), tabId)
    : null;
  if (fromPreferences) {
    if (typeof settings?.set === 'function') settings.set(path, fromPreferences);
    return fromPreferences;
  }

  if (typeof settings?.set !== 'function' || typeof legacy?.getItem !== 'function') return null;

  const raw = legacy.getItem(legacyScriptFontKey(tabId));
  if (typeof raw !== 'string' || raw === '') return null;

  let parsed;
  try {
    parsed = JSON.parse(raw);
  } catch {
    return null;
  }

  const font = normalizeScriptFont(parsed);
  if (!font) return null;
  settings.set(path, font);
  legacy.removeItem?.(legacyScriptFontKey(tabId));
  return font;
}

/**
 * @param {{ set?: Function } | null | undefined} settings
 * @param {string} tabId
 * @param {unknown} value
 * @returns {boolean}
 */
export function saveScriptFont(settings, tabId, value) {
  const font = normalizeScriptFont(value);
  if (!font || typeof settings?.set !== 'function') return false;
  settings.set(scriptFontStoragePath(tabId), font);
  return true;
}

/**
 * @param {{ removeSection?: Function } | null | undefined} settings
 * @param {string} tabId
 * @param {{ removeItem?: Function } | null} [legacy]
 * @returns {boolean}
 */
export function clearScriptFont(settings, tabId, legacy = null) {
  legacy?.removeItem?.(legacyScriptFontKey(tabId));
  if (typeof settings?.removeSection !== 'function') return false;
  settings.removeSection(scriptFontStoragePath(tabId));
  return true;
}
