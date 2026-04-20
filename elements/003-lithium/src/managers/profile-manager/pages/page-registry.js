/**
 * Profile Manager - Settings Page Registry
 *
 * Manages the registration and loading of settings page handlers.
 * Internal pages (negative indices) use the pattern: page-{name}.js
 * Manager pages (positive indices) use the pattern: manager-{id}.js
 */

import { log, Subsystems, Status } from '../../../core/log.js';
import { BaseSettingsPage, SimpleSettingsPage } from './settings-page-base.js';

// Import all internal page handlers statically for Vite compatibility
import AccountPage from './page-account.js';
import NamesPage from './page-names.js';
import AddressesPage from './page-addresses.js';
import EmailPage from './page-email.js';
import PhonePage from './page-phone.js';
import AuthenticationPage from './page-authentication.js';
import TokensPage from './page-tokens.js';
import LanguagePage from './page-language.js';
import DateFormatsPage from './page-date-formats.js';
import NumberFormatsPage from './page-number-formats.js';
import StartupPage from './page-startup.js';
import NotificationsPage from './page-notifications.js';
import ConciergePage from './page-concierge.js';
import AnnotationsPage from './page-annotations.js';
import ToursPage from './page-tours.js';
import TrainingPage from './page-training.js';
import LoginHistoryPage from './page-login-history.js';

// Import placeholder and manager pages
import PlaceholderPage from './page-placeholder.js';
import LookupsManagerPage from './manager-23.js';

/**
 * Map of internal page indices to their handler classes
 * Negative indices for internal Lithium pages
 */
const INTERNAL_PAGE_MAP = {
  '-1': AccountPage,
  '-2': NamesPage,
  '-3': AddressesPage,
  '-4': EmailPage,
  '-5': PhonePage,
  '-6': AuthenticationPage,
  '-7': TokensPage,
  '-8': LanguagePage,
  '-9': DateFormatsPage,
  '-10': NumberFormatsPage,
  '-11': StartupPage,
  '-12': NotificationsPage,
  '-13': ConciergePage,
  '-14': AnnotationsPage,
  '-15': ToursPage,
  '-16': TrainingPage,
  '-17': LoginHistoryPage,
};

/**
 * Settings Page Registry
 * Manages loading and caching of page handlers
 */
export class SettingsPageRegistry {
  constructor(options = {}) {
    this.app = options.app || null;
    this.onDirtyChange = options.onDirtyChange || (() => {});

    // Cache of loaded handlers: index -> handler instance
    this._handlers = new Map();

    // Track which pages are currently loading
    this._loading = new Set();
  }

  /**
   * Get or load a page handler for the given index
   * @param {number} index - Page index (negative for internal, positive for managers)
   * @param {HTMLElement} container - The page container element
   * @param {Object} pageData - Page data from the user options table
   * @returns {Promise<BaseSettingsPage|null>}
   */
  async getHandler(index, container, pageData) {
    // Check cache first
    if (this._handlers.has(index)) {
      const handler = this._handlers.get(index);
      // Update container reference in case DOM was rebuilt
      handler.container = container;
      return handler;
    }

    // Prevent duplicate loading
    if (this._loading.has(index)) {
      log(Subsystems.MANAGER, Status.DEBUG, `[PageRegistry] Page ${index} already loading, waiting...`);
      await this._waitForLoad(index);
      return this._handlers.get(index) || null;
    }

    this._loading.add(index);

    try {
      const handler = await this._loadHandler(index, container, pageData);
      if (handler) {
        this._handlers.set(index, handler);
      }
      return handler;
    } finally {
      this._loading.delete(index);
    }
  }

  /**
   * Load a handler module
   * @private
   */
  async _loadHandler(index, container, pageData) {
    const isInternal = index < 0;

    if (isInternal) {
      return this._loadInternalHandler(index, container, pageData);
    } else {
      return this._loadManagerHandler(index, container, pageData);
    }
  }

  /**
   * Load an internal page handler (static import)
   * @private
   */
  async _loadInternalHandler(index, container, pageData) {
    const HandlerClass = INTERNAL_PAGE_MAP[index];

    if (!HandlerClass) {
      log(Subsystems.MANAGER, Status.WARN, `[PageRegistry] Unknown internal page index: ${index}`);
      return null;
    }

    try {
      const handler = new HandlerClass({
        index: index,
        managerId: null,
        app: this.app,
        onDirtyChange: this.onDirtyChange,
      });

      await handler.init(container, pageData);

      log(Subsystems.MANAGER, Status.DEBUG, `[PageRegistry] Loaded internal handler for page ${index}`);
      return handler;
    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, `[PageRegistry] Error loading internal handler ${index}:`, error);
      return null;
    }
  }

  /**
   * Load a manager page handler
   * @private
   */
  async _loadManagerHandler(index, container, pageData) {
    // Get the manager name from page data or use a default
    const managerName = pageData?.label || `Manager ${index}`;

    try {
      // Use a switch to get the appropriate handler class
      const HandlerClass = this._getManagerHandlerClass(index);

      if (!HandlerClass) {
        // No specific handler - return a placeholder
        log(Subsystems.MANAGER, Status.DEBUG, `[PageRegistry] Using placeholder for manager ${index}`);
        const placeholder = new PlaceholderPage({
          index: index,
          managerId: index,
          managerName: managerName,
          app: this.app,
          onDirtyChange: this.onDirtyChange,
        });
        await placeholder.init(container, pageData);
        return placeholder;
      }

      const handler = new HandlerClass({
        index: index,
        managerId: index,
        app: this.app,
        onDirtyChange: this.onDirtyChange,
      });

      await handler.init(container, pageData);

      log(Subsystems.MANAGER, Status.DEBUG, `[PageRegistry] Loaded manager handler for ${index}`);
      return handler;
    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, `[PageRegistry] Error loading manager ${index}:`, error.message);
      // Return a placeholder on error
      const placeholder = new PlaceholderPage({
        index: index,
        managerId: index,
        managerName: managerName,
        app: this.app,
        onDirtyChange: this.onDirtyChange,
      });
      await placeholder.init(container, pageData);
      return placeholder;
    }
  }

  /**
   * Get the handler class for a manager
   * Returns null if no specific handler exists (placeholder will be used)
   * @private
   */
  _getManagerHandlerClass(index) {
    // Map of manager IDs to their handler classes
    // Add new managers here as they are implemented
    switch (index) {
      case 23: // Lookups Manager
        return LookupsManagerPage;
      default:
        // Return null to use placeholder
        return null;
    }
  }

  /**
   * Wait for a page to finish loading
   * @private
   */
  async _waitForLoad(index) {
    const startTime = Date.now();
    const timeout = 5000;

    while (this._loading.has(index)) {
      if (Date.now() - startTime > timeout) {
        log(Subsystems.MANAGER, Status.WARN, `[PageRegistry] Timeout waiting for page ${index}`);
        break;
      }
      await new Promise(resolve => setTimeout(resolve, 50));
    }
  }

  /**
   * Check if any loaded page is dirty
   * @returns {boolean}
   */
  isDirty() {
    for (const handler of this._handlers.values()) {
      if (handler?.isDirty?.()) {
        return true;
      }
    }
    return false;
  }

  /**
   * Save all dirty pages
   * @returns {Promise<Array>}
   */
  async saveAll() {
    const results = [];
    for (const [index, handler] of this._handlers.entries()) {
      if (handler?.isDirty?.() && handler?.save) {
        try {
          const result = await handler.save();
          results.push({ index, success: true, result });
        } catch (error) {
          results.push({ index, success: false, error: error.message });
        }
      }
    }
    return results;
  }

  /**
   * Cancel/reset all pages
   */
  cancelAll() {
    for (const handler of this._handlers.values()) {
      if (handler?.cancel) {
        handler.cancel();
      }
    }
  }

  /**
   * Get a specific handler if loaded
   * @param {number} index - Page index
   * @returns {BaseSettingsPage|null}
   */
  getLoadedHandler(index) {
    return this._handlers.get(index) || null;
  }

  /**
   * Check if a handler is loaded
   * @param {number} index - Page index
   * @returns {boolean}
   */
  hasHandler(index) {
    return this._handlers.has(index);
  }

  /**
   * Unload a specific handler
   * @param {number} index - Page index
   */
  unloadHandler(index) {
    const handler = this._handlers.get(index);
    if (handler?.destroy) {
      handler.destroy();
    }
    this._handlers.delete(index);
  }

  /**
   * Destroy all handlers
   */
  destroy() {
    for (const [index, handler] of this._handlers.entries()) {
      if (handler?.destroy) {
        try {
          handler.destroy();
        } catch (error) {
          log(Subsystems.MANAGER, Status.ERROR, `[PageRegistry] Error destroying handler ${index}:`, error);
        }
      }
    }
    this._handlers.clear();
    this._loading.clear();
  }
}

/**
 * Create a simple static page handler for pages that just need form binding
 * This is used as a fallback when no dynamic handler exists
 */
export function createStaticPageHandler(index, container, pageData) {
  return new SimpleSettingsPage({
    index: index,
    managerId: index < 0 ? null : index,
    formSelector: 'form',
    onDirtyChange: () => {},
  });
}

export default SettingsPageRegistry;
