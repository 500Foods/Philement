/**
 * Login Help Panel
 *
 * Manages the help/version sub-panel on the login page: loads `version.json`
 * (with caching paths shared with `index.html`'s early-fetch optimisation) and
 * populates the version-info box in the login header plus the help-panel
 * version/build-date fields. Extracted from login.js to keep files under
 * 1,000 lines per LITHIUM-INS.md rule #2.
 */

const MONTHS = [
  'Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun',
  'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec',
];

/**
 * HelpPanel class
 *
 * Owns the version-info display lifecycle. Follows the show/hide/destroy
 * pattern established by sibling panels, although show/hide are no-ops here:
 * version info is loaded once at `init()` and remains visible until teardown.
 */
export class HelpPanel {
  /**
   * @param {Object} options
   * @param {Object} options.elements - DOM element references; expected fields:
   *   `versionBuild`, `versionYear`, `versionDate`, `versionTime`,
   *   `helpAppVersion`, `helpBuildDate`. Any missing field is silently skipped.
   * @param {Object} [options.win] - Optional `window`-like object for tests.
   *   Defaults to the global `window`.
   * @param {Function} [options.fetchImpl] - Optional `fetch` implementation
   *   for tests. Defaults to the global `fetch`.
   */
  constructor({ elements, win, fetchImpl } = {}) {
    this.elements = elements || {};
    this._win = win || (typeof window !== 'undefined' ? window : {});
    this._fetch = fetchImpl || (typeof fetch !== 'undefined' ? fetch : null);
  }

  /**
   * Load version information and populate all version display fields.
   * Idempotent: safe to call multiple times.
   *
   * Lookup order:
   *   1. `window.__lithiumVersionData` (cache from index.html early fetch)
   *   2. `window.__lithiumVersionPromise` (in-flight fetch from index.html)
   *   3. `fetch('/version.json')` as last resort
   *
   * Failure at any stage is logged and swallowed; the version display simply
   * remains blank rather than blocking login.
   */
  async init() {
    try {
      const versionData = await this._loadVersionData();
      if (!versionData) return;
      this._renderVersion(versionData);
    } catch (error) {
      console.warn('[HelpPanel] Could not load version info:', error);
    }
  }

  /**
   * Resolve version data from cache, in-flight promise, or network.
   * @returns {Promise<Object|null>} version data or null if unavailable
   * @private
   */
  async _loadVersionData() {
    if (this._win.__lithiumVersionData) {
      return this._win.__lithiumVersionData;
    }
    if (this._win.__lithiumVersionPromise) {
      return await this._win.__lithiumVersionPromise;
    }
    if (this._fetch) {
      const response = await this._fetch('/version.json');
      if (!response.ok) return null;
      return await response.json();
    }
    return null;
  }

  /**
   * Populate the DOM fields from the resolved version data.
   * @param {Object} versionData - { build, timestamp, version }
   * @private
   */
  _renderVersion(versionData) {
    const { build, timestamp, version } = versionData;

    // Header version-info box: build, year, MMDD, HHMM (local time).
    if (this.elements.versionBuild && timestamp) {
      const date = new Date(timestamp);
      const year = date.getFullYear();
      const month = String(date.getMonth() + 1).padStart(2, '0');
      const day = String(date.getDate()).padStart(2, '0');
      const hours = String(date.getHours()).padStart(2, '0');
      const minutes = String(date.getMinutes()).padStart(2, '0');

      this.elements.versionBuild.textContent = build;
      if (this.elements.versionYear) this.elements.versionYear.textContent = year;
      if (this.elements.versionDate) this.elements.versionDate.textContent = month + day;
      if (this.elements.versionTime) this.elements.versionTime.textContent = hours + minutes;
    }

    // Help panel version + formatted UTC build date.
    if (this.elements.helpAppVersion) {
      this.elements.helpAppVersion.textContent = version || `0.1.${build}`;
    }
    if (this.elements.helpBuildDate) {
      let buildDate = '';
      if (timestamp) {
        const date = new Date(timestamp);
        const hours = String(date.getUTCHours()).padStart(2, '0');
        const minutes = String(date.getUTCMinutes()).padStart(2, '0');
        const day = String(date.getUTCDate()).padStart(2, '0');
        buildDate = `${date.getUTCFullYear()}-${MONTHS[date.getUTCMonth()]}-${day} ${hours}:${minutes} UTC`;
      }
      this.elements.helpBuildDate.textContent = buildDate || timestamp;
    }
  }

  /**
   * Called when the help panel becomes visible. No-op: the panel content is
   * loaded once at `init()` and does not need refreshing.
   */
  show() {
    // Intentional no-op.
  }

  /**
   * Called when the help panel is hidden. No-op for symmetry with siblings.
   */
  hide() {
    // Intentional no-op.
  }

  /**
   * Tear down: clears element references. No external resources to release.
   */
  destroy() {
    this.elements = {};
  }
}
