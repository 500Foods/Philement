/**
 * Profile Settings Service
 *
 * Centralized read/write service for the user profile JSON configuration.
 * All settings pages use this service to access and mutate their section
 * of the profile JSON. The service handles:
 *
 *   - Loading the full JSON blob from localStorage (and eventually API)
 *   - Writing values at arbitrary dotted paths (e.g. "-9.dates.short")
 *   - Reading values back with optional defaults
 *   - Notifying listeners when changes occur
 *   - Debounced persistence to avoid excessive saves
 *
 * Each settings page controls a unique top-level key in the JSON object.
 * For internal pages this is the negative index (e.g. "-9" for Date Formats).
 * For manager pages this is the manager ID (e.g. "23" for Lookups Manager).
 *
 * Example JSON structure:
 * {
 *   "-9": {
 *     "_name": "Date Formats",
 *     "dates": {
 *       "short": "YYYY-MM-DD",
 *       "long": "MMMM D, YYYY"
 *     },
 *     "times": {
 *       "short": "HH:mm",
 *       "long": "h:mm A"
 *     }
 *   },
 *   "23": {
 *     "_name": "Lookups Manager",
 *     "defaultView": "list",
 *     "pageSize": 50
 *   }
 * }
 *
 * The "_name" field is stored for human readability only; the service
 * never uses it for routing or lookup.
 */

import { log, Subsystems, Status } from '../../core/log.js';

const STORAGE_KEY = 'lithium_preferences';
const SAVE_DEBOUNCE_MS = 500;

/**
 * ProfileSettingsService class
 */
export class ProfileSettingsService {
  constructor(options = {}) {
    this._data = {};
    this._listeners = new Map(); // path -> Set<callback>
    this._globalListeners = new Set();
    this._saveTimer = null;
    this._pendingSave = false;
    this._onBeforeSave = options.onBeforeSave || null;
    this._onAfterSave = options.onAfterSave || null;

    this._load();
  }

  // ── Public API ────────────────────────────────────────────────────────────

  /**
   * Get a value from the settings using dotted path.
   * @param {string} path - Dotted path (e.g. 'collection.font')
   * @param {*} defaultValue - Default if not found
   * @returns {*}
   */
  get(path, defaultValue = undefined) {
    const keys = path.split('.');
    let current = this._data;
    for (const key of keys) {
      if (current && typeof current === 'object' && key in current) {
        current = current[key];
      } else {
        return defaultValue;
      }
    }
    return current !== undefined && current !== null ? current : defaultValue;
  }

  /**
   * Set a value in the settings using dotted path.
   * @param {string} path - Dotted path (e.g. 'collection.font')
   * @param {*} value - Value to set
   */
  set(path, value) {
    const keys = path.split('.');
    let current = this._data;
    for (let i = 0; i < keys.length - 1; i++) {
      const key = keys[i];
      if (!current[key] || typeof current[key] !== 'object') {
        current[key] = {};
      }
      current = current[key];
    }
    current[keys[keys.length - 1]] = value;
    this._schedulePersist();
    this._notifyListeners(path, value);
  }

  /**
   * Alias for get, for compatibility.
   */
  getSetting(path, defaultValue = undefined) {
    return this.get(path, defaultValue);
  }

  // ── Internal: Load / Persist ─────────────────────────────────────────────

  /**
   * Load the full JSON object from localStorage.
   * @private
   */
  _load() {
    const stored = localStorage.getItem(STORAGE_KEY);
    if (stored) {
      try {
        this._data = JSON.parse(stored);
      } catch (error) {
        log(Subsystems.MANAGER, Status.WARN,
          '[ProfileSettingsService] Failed to parse stored preferences, starting fresh:', error);
        this._data = {};
      }
    } else {
      this._data = {};
    }
  }

  /**
   * Persist the full JSON object to localStorage immediately.
   * @private
   */
  _persistNow() {
    try {
      localStorage.setItem(STORAGE_KEY, JSON.stringify(this._data));
      this._pendingSave = false;
      log(Subsystems.MANAGER, Status.DEBUG,
        '[ProfileSettingsService] Preferences persisted');
    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR,
        '[ProfileSettingsService] Failed to persist preferences:', error);
    }
  }

  /**
   * Schedule a debounced persist. Multiple rapid writes coalesce into one save.
   * @private
   */
  _schedulePersist() {
    this._pendingSave = true;
    if (this._saveTimer) {
      clearTimeout(this._saveTimer);
    }
    this._saveTimer = setTimeout(() => {
      this._saveTimer = null;
      this._persistNow();
      if (this._onAfterSave) {
        try {
          this._onAfterSave(this._data);
        } catch (_e) {
          // ignore callback errors
        }
      }
    }, SAVE_DEBOUNCE_MS);
  }

  /**
   * Notify listeners for a specific path.
   * @private
   */
  _notifyListeners(path, value) {
    // Notify path-specific listeners
    for (const [listenerPath, callbacks] of this._listeners) {
      if (path.startsWith(listenerPath) || listenerPath === '*') {
        for (const callback of callbacks) {
          try {
            callback(path, value);
          } catch (_e) {
            // ignore listener errors
          }
        }
      }
    }
    // Notify global listeners
    for (const callback of this._globalListeners) {
      try {
        callback(path, value);
      } catch (_e) {
        // ignore listener errors
      }
    }
  }

  /**
   * Notify all global listeners of any change.
   * @private
   */
  _notifyGlobal() {
    for (const callback of this._globalListeners) {
      try {
        callback(this._data);
      } catch (_e) {
        // ignore listener errors
      }
    }
  }

  // ── Public: Get / Set / Delete ───────────────────────────────────────────

  /**
   * Get the entire profile JSON object (shallow clone).
   * Useful for the Collection tab editor.
   * @returns {Object}
   */
  getAll() {
    return { ...this._data };
  }

  /**
   * Replace the entire profile JSON object.
   * Use with caution — typically only called by the Collection tab.
   * @param {Object} data
   */
  setAll(data) {
    this._data = data && typeof data === 'object' ? data : {};
    this._persistNow();
    this._notifyGlobal();
  }

  /**
   * Read a value from the profile JSON using a dotted path.
   *
   * @param {string} sectionKey - Top-level section key (e.g. "-9" or "23")
   * @param {string} path - Dotted path within the section (e.g. "dates.short")
   * @param {*} defaultValue - Value to return if path does not exist
   * @returns {*}
   *
   * Example:
   *   service.get("-9", "dates.short", "YYYY-MM-DD")
   *   // returns "YYYY-MM-DD" if not set
   */
  get(sectionKey, path, defaultValue = undefined) {
    const section = this._data[sectionKey];
    if (!section || typeof section !== 'object') {
      return defaultValue;
    }
    const keys = path.split('.');
    let current = section;
    for (const key of keys) {
      if (current == null || !Object.prototype.hasOwnProperty.call(current, key)) {
        return defaultValue;
      }
      current = current[key];
    }
    return current;
  }

  /**
   * Write a value into the profile JSON at a dotted path.
   *
   * @param {string} sectionKey - Top-level section key (e.g. "-9" or "23")
   * @param {string} path - Dotted path within the section (e.g. "dates.short")
   * @param {*} value - Value to store
   *
   * Example:
   *   service.set("-9", "dates.short", "YYYY-MM-DD")
   *   // writes { "-9": { dates: { short: "YYYY-MM-DD" } } }
   */
  set(sectionKey, path, value) {
    // Ensure section exists
    if (!this._data[sectionKey] || typeof this._data[sectionKey] !== 'object') {
      this._data[sectionKey] = {};
    }

    const keys = path.split('.');
    let current = this._data[sectionKey];

    for (let i = 0; i < keys.length - 1; i++) {
      const key = keys[i];
      if (!current[key] || typeof current[key] !== 'object') {
        current[key] = {};
      }
      current = current[key];
    }

    const lastKey = keys[keys.length - 1];
    const oldValue = current[lastKey];
    current[lastKey] = value;

    // Notify listeners
    const fullPath = `${sectionKey}.${path}`;
    this._notifyPath(fullPath, value, oldValue);
    this._notifyGlobal();

    this._schedulePersist();
  }

  /**
   * Delete a value at a dotted path.
   *
   * @param {string} sectionKey - Top-level section key
   * @param {string} path - Dotted path within the section
   * @returns {boolean} true if a value was deleted
   */
  delete(sectionKey, path) {
    const section = this._data[sectionKey];
    if (!section || typeof section !== 'object') return false;

    const keys = path.split('.');
    let current = section;

    for (let i = 0; i < keys.length - 1; i++) {
      const key = keys[i];
      if (!current || typeof current !== 'object' || !Object.prototype.hasOwnProperty.call(current, key)) {
        return false;
      }
      current = current[key];
    }

    const lastKey = keys[keys.length - 1];
    if (!Object.prototype.hasOwnProperty.call(current, lastKey)) {
      return false;
    }

    const oldValue = current[lastKey];
    delete current[lastKey];

    const fullPath = `${sectionKey}.${path}`;
    this._notifyPath(fullPath, undefined, oldValue);
    this._notifyGlobal();

    this._schedulePersist();
    return true;
  }

  /**
   * Get an entire section object (shallow clone).
   * Returns an empty object if the section does not exist.
   *
   * @param {string} sectionKey - Top-level section key
   * @returns {Object}
   */
  getSection(sectionKey) {
    const section = this._data[sectionKey];
    return section && typeof section === 'object' ? { ...section } : {};
  }

  /**
   * Replace an entire section object.
   * This is the preferred way for a settings page to bulk-write its data.
   *
   * @param {string} sectionKey - Top-level section key
   * @param {Object} data - Section data to store
   * @param {string} [sectionName] - Human-readable name (stored as _name)
   */
  setSection(sectionKey, data, sectionName = null) {
    const oldSection = this._data[sectionKey];
    const newSection = data && typeof data === 'object' ? { ...data } : {};
    if (sectionName) {
      newSection._name = sectionName;
    }
    this._data[sectionKey] = newSection;

    this._notifyPath(sectionKey, newSection, oldSection);
    this._notifyGlobal();
    this._schedulePersist();
  }

  /**
   * Remove an entire section from the profile JSON.
   *
   * @param {string} sectionKey - Top-level section key
   * @returns {boolean} true if a section was removed
   */
  removeSection(sectionKey) {
    if (!Object.prototype.hasOwnProperty.call(this._data, sectionKey)) {
      return false;
    }
    const oldSection = this._data[sectionKey];
    delete this._data[sectionKey];

    this._notifyPath(sectionKey, undefined, oldSection);
    this._notifyGlobal();
    this._schedulePersist();
    return true;
  }

  /**
   * Check whether a section exists in the profile JSON.
   *
   * @param {string} sectionKey - Top-level section key
   * @returns {boolean}
   */
  hasSection(sectionKey) {
    return Object.prototype.hasOwnProperty.call(this._data, sectionKey) &&
      this._data[sectionKey] != null &&
      typeof this._data[sectionKey] === 'object';
  }

  /**
   * Return an array of all section keys currently stored.
   * @returns {Array<string>}
   */
  getSectionKeys() {
    return Object.keys(this._data).filter(k =>
      this._data[k] != null && typeof this._data[k] === 'object'
    );
  }

  // ── Public: Batch Operations ─────────────────────────────────────────────

  /**
   * Apply multiple set operations in one batch. Only one persist is triggered.
   *
   * @param {string} sectionKey - Top-level section key
   * @param {Object} changes - Map of dotted paths to values
   *
   * Example:
   *   service.batchSet("-9", {
   *     "dates.short": "YYYY-MM-DD",
   *     "dates.long": "MMMM D, YYYY",
   *     "times.short": "HH:mm",
   *   })
   */
  batchSet(sectionKey, changes) {
    if (!this._data[sectionKey] || typeof this._data[sectionKey] !== 'object') {
      this._data[sectionKey] = {};
    }

    for (const [path, value] of Object.entries(changes)) {
      const keys = path.split('.');
      let current = this._data[sectionKey];

      for (let i = 0; i < keys.length - 1; i++) {
        const key = keys[i];
        if (!current[key] || typeof current[key] !== 'object') {
          current[key] = {};
        }
        current = current[key];
      }

      current[keys[keys.length - 1]] = value;
      this._notifyPath(`${sectionKey}.${path}`, value, undefined);
    }

    this._notifyGlobal();
    this._schedulePersist();
  }

  // ── Public: Listeners ────────────────────────────────────────────────────

  /**
   * Register a callback to be called when a specific path changes.
   * Path may include wildcards: "-9.dates.*" matches any key under dates.
   *
   * @param {string} pathPattern - Dotted path, may end with "*"
   * @param {Function} callback - (newValue, oldValue, fullPath) => void
   * @returns {Function} unsubscribe function
   */
  onChange(pathPattern, callback) {
    if (!this._listeners.has(pathPattern)) {
      this._listeners.set(pathPattern, new Set());
    }
    this._listeners.get(pathPattern).add(callback);

    return () => {
      const set = this._listeners.get(pathPattern);
      if (set) {
        set.delete(callback);
        if (set.size === 0) {
          this._listeners.delete(pathPattern);
        }
      }
    };
  }

  /**
   * Register a callback for any change anywhere in the profile JSON.
   *
   * @param {Function} callback - (data) => void
   * @returns {Function} unsubscribe function
   */
  onAnyChange(callback) {
    this._globalListeners.add(callback);
    return () => this._globalListeners.delete(callback);
  }

  /**
   * Notify listeners for a specific path.
   * @private
   */
  _notifyPath(fullPath, newValue, oldValue) {
    for (const [pattern, callbacks] of this._listeners.entries()) {
      if (this._pathMatches(pattern, fullPath)) {
        callbacks.forEach(cb => {
          try {
            cb(newValue, oldValue, fullPath);
          } catch (_e) {
            // ignore listener errors
          }
        });
      }
    }
  }

  /**
   * Notify all global listeners.
   * @private
   */
  _notifyGlobal() {
    const data = this.getAll();
    this._globalListeners.forEach(cb => {
      try {
        cb(data);
      } catch (_e) {
        // ignore listener errors
      }
    });
  }

  /**
   * Check if a path pattern matches a full path.
   * Supports trailing wildcard: "-9.dates.*" matches "-9.dates.short".
   * @private
   */
  _pathMatches(pattern, fullPath) {
    if (pattern === fullPath) return true;
    if (pattern.endsWith('.*')) {
      const prefix = pattern.slice(0, -2); // remove .*
      return fullPath === prefix || fullPath.startsWith(prefix + '.');
    }
    return false;
  }

  // ── Public: Import / Export ──────────────────────────────────────────────

  /**
   * Export the profile JSON as a formatted string.
   * @returns {string}
   */
  exportJSON() {
    return JSON.stringify(this._data, null, 2);
  }

  /**
   * Import a JSON string, replacing the entire profile.
   * Returns false if the string is not valid JSON.
   *
   * @param {string} jsonString
   * @returns {boolean}
   */
  importJSON(jsonString) {
    try {
      const parsed = JSON.parse(jsonString);
      if (!parsed || typeof parsed !== 'object') return false;
      this._data = parsed;
      this._persistNow();
      this._notifyGlobal();
      return true;
    } catch (_e) {
      return false;
    }
  }

  // ── Public: Lifecycle ────────────────────────────────────────────────────

  /**
   * Force an immediate save, flushing any pending debounced write.
   * Call this before the user navigates away or logs out.
   */
  flush() {
    if (this._saveTimer) {
      clearTimeout(this._saveTimer);
      this._saveTimer = null;
    }
    if (this._pendingSave) {
      this._persistNow();
    }
  }

  /**
   * Destroy the service, clearing timers and listeners.
   */
  destroy() {
    this.flush();
    this._listeners.clear();
    this._globalListeners.clear();
  }
}

export default ProfileSettingsService;
