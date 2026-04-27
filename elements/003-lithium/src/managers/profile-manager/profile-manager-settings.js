/**
 * Profile Manager - Settings Tab Handler
 *
 * Handles the settings pages with index-based navigation and dynamic page loading.
 * Internal sections use negative indices, manager sections use actual manager IDs.
 */

import { log, Subsystems, Status } from '../../core/log.js';
import { SettingsPageRegistry } from './pages/page-registry.js';

  /**
   * SettingsTabHandler class
   * Manages the settings pages and crossfade transitions
   */
  export class SettingsTabHandler {
    constructor(options) {
      this.container = options.container;
      this.onPageChange = options.onPageChange || (() => {});
      this.onDirtyChange = options.onDirtyChange || (() => {});
      this.app = options.app || null;
      this.settingsService = options.settingsService || null;

      this.currentPageIndex = null;
      this._inPageTransition = false;
      this._transitionDuration = 350;

      // Page registry for loading handlers
      this._pageRegistry = null;

      // Queue for pending page requests during transitions
      this._pendingPageRequest = null;
    }

  /**
   * Initialize the settings tab handler
   */
  init() {
    // Get transition duration from CSS
    const duration = getComputedStyle(document.documentElement)
      .getPropertyValue('--transition-duration')
      .trim();
    this._transitionDuration = parseInt(duration, 10) || 350;

    // Initialize the page registry
    this._pageRegistry = new SettingsPageRegistry({
      app: this.app,
      settingsService: this.settingsService,
      onDirtyChange: this.onDirtyChange,
    });

    log(Subsystems.MANAGER, Status.INFO, '[SettingsTab] Handler initialized');
  }

  /**
   * Show a settings page by index with crossfade transition
   * @param {number} index - Page index (negative for internal, positive for managers)
   * @param {Object} pageData - Data about the page (label, icon, etc.)
   */
  async showPage(index, pageData = {}) {
    // Guard: same page requested
    if (index === this.currentPageIndex) return;

    // If a transition is in progress, queue this request and return
    if (this._inPageTransition) {
      this._pendingPageRequest = { index, pageData };
      return;
    }

    this._inPageTransition = true;

    let targetPage = this.container.querySelector(`.settings-page[data-page-index="${index}"]`);

    if (!targetPage) {
      // Page not loaded yet, try to load it via the registry
      log(Subsystems.MANAGER, Status.DEBUG, `[SettingsTab] Page ${index} not found, attempting to load...`);
      log(Subsystems.MANAGER, Status.DEBUG, `[SettingsTab] Container is: ${this.container.id}, has ${this.container.children.length} children`);
      log(Subsystems.MANAGER, Status.DEBUG, `[SettingsTab] Container children:`, Array.from(this.container.children).map(c => `${c.tagName}${c.id ? '#' + c.id : ''}${c.className ? '.' + c.className : ''}`));

      // The registry will load the HTML/CSS if needed
      const handler = await this._pageRegistry.getHandler(index, this.container, pageData);
      if (!handler) {
        log(Subsystems.MANAGER, Status.ERROR, `[SettingsTab] Failed to load page handler for ${index}`);
        this._inPageTransition = false;
        return;
      }

      // Now try to find the page element again
      targetPage = this.container.querySelector(`.settings-page[data-page-index="${index}"]`);
      log(Subsystems.MANAGER, Status.DEBUG, `[SettingsTab] After loading, found targetPage: ${targetPage ? targetPage.id : 'null'}`);
      if (!targetPage) {
        // Try searching the entire document
        const docTarget = document.querySelector(`.settings-page[data-page-index="${index}"]`);
        log(Subsystems.MANAGER, Status.DEBUG, `[SettingsTab] Document search found: ${docTarget ? docTarget.id : 'null'}`);
        log(Subsystems.MANAGER, Status.ERROR, `[SettingsTab] Page element still not found after loading: ${index}`);
        this._inPageTransition = false;
        return;
      }
    }

    // Find currently active page
    const activePage = this.container.querySelector('.settings-page.active');

    // Prepare new page (hidden initially for fade-in)
    targetPage.style.opacity = '0';
    targetPage.classList.add('active');

    // Force reflow for transition to work
    void targetPage.offsetHeight;

    // Apply transition and fade in
    const durationStr = `${this._transitionDuration}ms`;

    if (activePage) {
      activePage.style.transition = `opacity ${durationStr} ease-in-out`;
      activePage.style.opacity = '0';
    }

    targetPage.style.transition = `opacity ${durationStr} ease-in-out`;
    targetPage.style.opacity = '1';

    // Load page handler if not already loaded
    let handler = this._pageRegistry.getLoadedHandler(index);
    if (!handler) {
      handler = await this._pageRegistry.getHandler(index, targetPage, pageData);
    }

    // Ensure we have a handler
    if (!handler) {
      log(Subsystems.MANAGER, Status.ERROR, `[SettingsTab] No handler available for page ${index}`);
      this._inPageTransition = false;
      return;
    }
    if (handler?.onShow) {
      handler.onShow();
    }

    // After transition completes, clean up
    setTimeout(() => {
      if (activePage) {
        activePage.classList.remove('active');
        activePage.style.transition = '';
        activePage.style.opacity = '';

        // Notify previous handler it's hidden
        const prevHandler = this._pageRegistry.getLoadedHandler(this.currentPageIndex);
        if (prevHandler?.onHide) {
          prevHandler.onHide();
        }
      }
      targetPage.style.transition = '';
      targetPage.style.opacity = '';

      this.currentPageIndex = index;
      this._inPageTransition = false;

      this.onPageChange(index, pageData);
      log(Subsystems.MANAGER, Status.DEBUG, `[SettingsTab] Showing page: ${index}`);

      // Process any pending page request that came in during this transition
      const pending = this._pendingPageRequest;
      this._pendingPageRequest = null;
      if (pending) {
        log(Subsystems.MANAGER, Status.DEBUG, `[SettingsTab] Processing pending page request: ${pending.index}`);
        this.showPage(pending.index, pending.pageData);
      }
    }, this._transitionDuration);
  }

  /**
   * Get the currently active page index
   * @returns {number|null}
   */
  getCurrentPage() {
    return this.currentPageIndex;
  }

  /**
   * Check if a page exists in the DOM
   * @param {number} index - Page index
   * @returns {boolean}
   */
  hasPage(index) {
    return !!this.container.querySelector(`.settings-page[data-page-index="${index}"]`);
  }

  /**
   * Refresh the current page (called when data changes)
   */
  refreshCurrentPage() {
    const handler = this._pageRegistry.getLoadedHandler(this.currentPageIndex);
    if (handler?.refresh) {
      handler.refresh();
    }
  }

  /**
   * Check if any page has unsaved changes
   * @returns {boolean}
   */
  isDirty() {
    return this._pageRegistry.isDirty();
  }

  /**
   * Save all dirty pages
   * @returns {Promise<Array>}
   */
  async save() {
    return this._pageRegistry.saveAll();
  }

  /**
   * Reset/cancel changes on all pages
   */
  cancel() {
    this._pageRegistry.cancelAll();
  }

  /**
   * Get all available page indices from the DOM
   * @returns {Array<number>}
   */
  getAvailablePages() {
    const pages = this.container.querySelectorAll('.settings-page');
    return Array.from(pages)
      .map(p => parseInt(p.dataset.pageIndex, 10))
      .filter(n => !isNaN(n));
  }

  /**
   * Destroy all page handlers
   */
  destroy() {
    this._pageRegistry?.destroy();
    this._pageRegistry = null;
  }
}

export default SettingsTabHandler;
