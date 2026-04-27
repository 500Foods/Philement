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
import AccountPage from './account/page-account.js';
import NamesPage from './names/page-names.js';
import AddressesPage from './addresses/page-addresses.js';
import EmailPage from './email/page-email.js';
import PhonePage from './phone/page-phone.js';
import AuthenticationPage from './authentication/page-authentication.js';
import TokensPage from './tokens/page-tokens.js';
import LanguagePage from './language/page-language.js';
import DateFormatsPage from './date-formats/page-date-formats.js';
import NumberFormatsPage from './number-formats/page-number-formats.js';
import StartupPage from './startup/page-startup.js';
import NotificationsPage from './notifications/page-notifications.js';
import ConciergePage from './concierge/page-concierge.js';
import AnnotationsPage from './annotations/page-annotations.js';
import ToursPage from './tours/page-tours.js';
import TrainingPage from './training/page-training.js';
import LoginHistoryPage from './login-history/page-login-history.js';

// Import placeholder and manager pages
import PlaceholderPage from './page-placeholder.js';
import LookupsManagerPage from './lookups/manager-23.js';

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
    this.settingsService = options.settingsService || null;
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
    log(Subsystems.MANAGER, Status.DEBUG, `[PageRegistry] getHandler called for index ${index}`);

    // Check cache first
    if (this._handlers.has(index)) {
      log(Subsystems.MANAGER, Status.DEBUG, `[PageRegistry] Handler found in cache for ${index}`);
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

    log(Subsystems.MANAGER, Status.DEBUG, `[PageRegistry] Starting load for ${index}`);
    this._loading.add(index);

    try {
      const handler = await this._loadHandler(index, container, pageData);
      if (handler) {
        this._handlers.set(index, handler);
        log(Subsystems.MANAGER, Status.DEBUG, `[PageRegistry] Handler cached for ${index}`);
      }
      return handler;
    } finally {
      this._loading.delete(index);
    }
  }

  /**
   * Load a page handler based on index type
   * @private
   */
  async _loadHandler(index, container, pageData) {
    log(Subsystems.MANAGER, Status.DEBUG, `[PageRegistry] _loadHandler called for index ${index}`);
    if (index < 0) {
      log(Subsystems.MANAGER, Status.DEBUG, `[PageRegistry] Dispatching to _loadInternalHandler for ${index}`);
      return this._loadInternalHandler(index, container, pageData);
    } else {
      log(Subsystems.MANAGER, Status.DEBUG, `[PageRegistry] Dispatching to _loadManagerHandler for ${index}`);
      return this._loadManagerHandler(index, container, pageData);
    }
  }

  /**
   * Load an internal page handler (negative indices)
   * @private
   */
  async _loadInternalHandler(index, container, pageData) {
    log(Subsystems.MANAGER, Status.DEBUG, `[PageRegistry] _loadInternalHandler called for index ${index}`);

    try {
      // Get the handler class from the map
      const HandlerClass = INTERNAL_PAGE_MAP[index];
      if (!HandlerClass) {
        log(Subsystems.MANAGER, Status.ERROR, `[PageRegistry] No handler class found for internal page ${index}`);
        return null;
      }

      // Check if page element already exists (might be embedded in main HTML)
      let pageElement = container.querySelector(`.settings-page[data-page-index="${index}"]`);

      if (!pageElement) {
        // Map index to page name for file paths
        const pageName = this._getPageNameFromIndex(index);
        if (!pageName) {
          log(Subsystems.MANAGER, Status.ERROR, `[PageRegistry] Could not map index ${index} to page name`);
          return null;
        }

        // Load HTML and CSS dynamically
        await this._loadPageAssets(pageName, container);

        // Find the page element in the container
        pageElement = container.querySelector(`.settings-page[data-page-index="${index}"]`);
        if (!pageElement) {
          log(Subsystems.MANAGER, Status.ERROR, `[PageRegistry] Page element not found after loading assets for ${index}`);
          return null;
        }
      } else {
        log(Subsystems.MANAGER, Status.DEBUG, `[PageRegistry] Page element already exists in DOM for ${index}`);
      }

      // Create and initialize the handler
      const handler = new HandlerClass({
        index: index,
        app: this.app,
        settings: this.settingsService,
        onDirtyChange: this.onDirtyChange,
      });

      await handler.init(pageElement, pageData);

      log(Subsystems.MANAGER, Status.DEBUG, `[PageRegistry] Loaded internal handler for ${index}`);
      return handler;
    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, `[PageRegistry] Error loading internal page ${index}:`, error.message);
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
          settings: this.settingsService,
          onDirtyChange: this.onDirtyChange,
        });
        await placeholder.init(container, pageData);
        return placeholder;
      }

      const handler = new HandlerClass({
        index: index,
        managerId: index,
        app: this.app,
        settings: this.settingsService,
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
        settings: this.settingsService,
        onDirtyChange: this.onDirtyChange,
      });
      await placeholder.init(container, pageData);
      return placeholder;
    }
  }
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
   * Map internal page index to page name for file paths
   * @private
   */
  _getPageNameFromIndex(index) {
    const indexToNameMap = {
      '-1': 'account',
      '-2': 'names',
      '-3': 'addresses',
      '-4': 'email',
      '-5': 'phone',
      '-6': 'authentication',
      '-7': 'tokens',
      '-8': 'language',
      '-9': 'date-formats',
      '-10': 'number-formats',
      '-11': 'startup',
      '-12': 'notifications',
      '-13': 'concierge',
      '-14': 'annotations',
      '-15': 'tours',
      '-16': 'training',
      '-17': 'login-history',
    };
    return indexToNameMap[index] || null;
  }

  /**
   * Load HTML and CSS assets for a page
   * @private
   */
  async _loadPageAssets(pageName, container) {
    try {
      // Load HTML - use absolute path that works in both dev and prod
      const htmlUrl = `/src/managers/profile-manager/pages/${pageName}/page-${pageName}.html`;
      const htmlResponse = await fetch(htmlUrl);
      if (!htmlResponse.ok) {
        throw new Error(`Failed to load HTML for ${pageName}: ${htmlResponse.status}`);
      }
      const htmlContent = await htmlResponse.text();

      // Parse and insert HTML
      const tempDiv = document.createElement('div');
      tempDiv.innerHTML = htmlContent;
      const pageElement = tempDiv.firstElementChild;
      if (pageElement) {
        container.appendChild(pageElement);
      }

      // Load CSS - use absolute path that works in both dev and prod
      const cssUrl = `/src/managers/profile-manager/pages/${pageName}/page-${pageName}.css`;
      const cssResponse = await fetch(cssUrl);
      if (cssResponse.ok) {
        const cssContent = await cssResponse.text();
        const styleElement = document.createElement('style');
        styleElement.textContent = cssContent;
        styleElement.setAttribute('data-page-css', pageName);
        document.head.appendChild(styleElement);
      } else {
        log(Subsystems.MANAGER, Status.DEBUG, `[PageRegistry] No CSS file found for ${pageName}, skipping`);
      }

      log(Subsystems.MANAGER, Status.DEBUG, `[PageRegistry] Loaded assets for page ${pageName}`);
    } catch (error) {
      log(Subsystems.MANAGER, Status.ERROR, `[PageRegistry] Error loading assets for ${pageName}:`, error.message);
      throw error;
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
export function createStaticPageHandler(index, _container, _pageData) {
  return new SimpleSettingsPage({
    index: index,
    managerId: index < 0 ? null : index,
    formSelector: 'form',
    settings: this.settingsService,
    onDirtyChange: () => {},
  });
}

export default SettingsPageRegistry;
