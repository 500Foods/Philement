const __vite__mapDeps=(i,m=__vite__mapDeps,d=(m.f||(m.f=["./codemirror-setup-lxLGSWU-.js","./codemirror-Bg1rM8n2.js","./index-CEuz7QhE.js","./index-ZlKPF4Cw.css"])))=>i.map(i=>d[i]);
import { l as log, a as Subsystems, S as Status, e as eventBus, E as Events, f as getConfigValue, _ as __vitePreload, c as getRawLog, d as getTip, s as storeJWT, j as hasLookup } from './index-CEuz7QhE.js';
import { g as getPasswordManagerSelectors, s as supportedLocales, a as saveLocalePreference, b as getLanguageData, c as getSavedLocale, d as getBestGuessLocale, e as getFlagSvg, h as getCountryCode, f as formatLogText } from './languages-Coru4QdC.js';
/* empty css                          */
import { s as scrollbarManager } from './scrollbar-manager-DHRoVvTV.js';

/**
 * Transitions Utility Library
 * 
 * Shared transition functions for fade effects across the application.
 * All transitions use CSS classes and return Promises for easy async/await usage.
 */

/**
 * CSS variable name for transition duration
 */
const TRANSITION_DURATION_VAR = '--transition-duration';

/**
 * Get the current transition duration from CSS variables
 * Reads the --transition-duration CSS custom property
 * @returns {number} Duration in milliseconds
 */
function getTransitionDuration() {
  if (typeof window === 'undefined') {
    return 500; // Default fallback for SSR/test environments
  }
  
  const root = document.documentElement;
  const computedStyle = getComputedStyle(root);
  const durationValue = computedStyle.getPropertyValue(TRANSITION_DURATION_VAR).trim();
  
  if (!durationValue) {
    return 500; // Default fallback
  }
  
  // Parse CSS duration value (e.g., "500ms" or "0.5s")
  if (durationValue.endsWith('ms')) {
    return parseInt(durationValue, 10);
  } else if (durationValue.endsWith('s')) {
    return parseFloat(durationValue) * 1000;
  }
  
  return 500; // Default fallback
}

/**
 * Wait for a transition to complete
 * Utility function for timing events with CSS transitions
 * @param {number} duration - Duration to wait in ms (default: reads from CSS)
 * @returns {Promise<void>}
 */
function waitForTransition(duration = null) {
  const dur = duration ?? getTransitionDuration();
  return new Promise(resolve => setTimeout(resolve, dur));
}

/**
 * Login Panels
 *
 * Manages panel switching, crossfade transitions, and overlay state
 * for the Login Manager. Extracted from login.js to keep files under
 * 1,000 lines per LITHIUM-INS.md rule #2.
 */


/**
 * LoginPanels class
 *
 * Owns panel visibility, crossfade transitions, and the transition overlay.
 * Delegates panel-specific init/cleanup to the LoginManager via callbacks.
 */
class LoginPanels {
  /**
   * @param {Object} options
   * @param {Object} options.panels - Map of panel names to DOM elements
   * @param {HTMLElement} options.loginPanel - The login panel element (reference for width/display rules)
   * @param {HTMLElement} options.overlay - The transition overlay element
   * @param {Function} [options.onBeforeSwitch] - Called before transition: (targetPanel) => void
   * @param {Function} [options.onAfterSwitch] - Called after transition: (fromPanel, toPanel, durationMs) => void
   */
  constructor({ panels, loginPanel, overlay, onBeforeSwitch, onAfterSwitch }) {
    this._panels = panels || {};
    this._loginPanel = loginPanel;
    this._overlay = overlay;
    this._onBeforeSwitch = onBeforeSwitch || (() => {});
    this._onAfterSwitch = onAfterSwitch || (() => {});
    this._currentPanel = 'login';
    this._focusTimer = null;
  }

  /** @returns {string} */
  get currentPanel() {
    return this._currentPanel;
  }

  /** @param {string} value */
  set currentPanel(value) {
    this._currentPanel = value;
  }

  /**
   * Switch to a different panel with crossfade transition
   * @param {string} targetPanel - Panel name to switch to
   */
  async switchPanel(targetPanel) {
    if (targetPanel === this._currentPanel) return;

    const fromPanel = this._panels[this._currentPanel];
    const toPanel = this._panels[targetPanel];

    if (!toPanel) return;

    // Cancel any pending focus timer
    this.cancelPendingFocus();

    // Let the owner do panel-specific prep (populate logs, init language, etc.)
    this._onBeforeSwitch(targetPanel);

    // Block interactions during transition
    this.enableOverlay();

    // Perform crossfade transition
    await this._performTransition(fromPanel, toPanel);

    // Update state
    const fromPanelName = this._currentPanel;
    this._currentPanel = targetPanel;
    const panelDuration = 0; // Timing is now the caller's responsibility

    // Re-enable interactions
    this.disableOverlay();

    // Let the owner do post-switch work (logging, focus management)
    this._onAfterSwitch(fromPanelName, targetPanel, panelDuration);
  }

  /**
   * Cancel any pending focus timer
   */
  cancelPendingFocus() {
    if (this._focusTimer !== null) {
      clearTimeout(this._focusTimer);
      this._focusTimer = null;
    }
  }

  /**
   * Schedule a focus action after a delay, with automatic cancellation
   * if another switch happens first.
   * @param {Function} focusFn - Function to call after delay
   * @param {number} [delay] - Delay in ms (defaults to transition duration)
   */
  scheduleFocus(focusFn, delay) {
    this.cancelPendingFocus();
    const actualDelay = delay ?? getTransitionDuration();
    this._focusTimer = setTimeout(() => {
      this._focusTimer = null;
      if (typeof focusFn === 'function') {
        focusFn();
      }
    }, actualDelay);
  }

  /**
   * Enable the transition overlay to block interactions
   */
  enableOverlay() {
    this._overlay?.classList.add('active');
  }

  /**
   * Disable the transition overlay
   */
  disableOverlay() {
    this._overlay?.classList.remove('active');
  }

  /**
   * Perform the actual panel transition animation with crossfade
   * @param {HTMLElement|null} fromElement
   * @param {HTMLElement|null} toElement
   * @private
   */
  async _performTransition(fromElement, toElement) {
    const duration = getTransitionDuration();

    // Determine the correct display value for the target panel
    const isLoginPanel = toElement === this._loginPanel;
    const targetDisplay = isLoginPanel ? 'block' : 'flex';

    // If we have both elements, do a true crossfade with absolute positioning
    if (fromElement && toElement) {
      // Get current dimensions before we modify anything
      const fromWidth = fromElement.offsetWidth;
      const fromHeight = fromElement.offsetHeight;
      toElement.offsetWidth || fromWidth; // fallback if not yet rendered
      toElement.offsetHeight || fromHeight;

      // Position both panels absolutely in the center of the container
      const centerStyles = `
        position: absolute;
        top: 50%;
        left: 50%;
        transform: translate(-50%, -50%);
        margin: 0;
      `;

      // Apply absolute positioning to from element (keep visible during fade)
      fromElement.style.cssText = `
        ${centerStyles}
        width: ${fromWidth}px;
        height: ${fromHeight}px;
        display: ${fromElement.style.display || (fromElement === this._loginPanel ? 'block' : 'flex')};
        opacity: 1;
        transition: opacity ${duration}ms ease-in-out;
        z-index: 1;
      `;

      // Prepare target element - also absolutely positioned in center
      let toCssWidth;
      if (toElement === this._loginPanel) {
        toCssWidth = '360px';
      } else if (toElement.id === 'language-panel') {
        toCssWidth = '484px'; // Language panel is wider
      } else {
        toCssWidth = '384px'; // Default subpanel width
      }
      toElement.style.cssText = `
        ${centerStyles}
        width: ${toCssWidth};
        display: ${targetDisplay};
        opacity: 0;
        transition: opacity ${duration}ms ease-in-out;
        z-index: 2;
      `;

      // Force reflow
      void toElement.offsetHeight;

      // Start crossfade
      requestAnimationFrame(() => {
        // Fade out current panel
        fromElement.style.opacity = '0';

        // Fade in new panel (with slight delay for overlap effect)
        setTimeout(() => {
          toElement.style.opacity = '1';
        }, duration / 4);
      });

      // Wait for transition to complete
      await waitForTransition(duration + duration / 4 + 50);

      // Clean up from element
      fromElement.style.cssText = 'display: none;';

      // Restore target element to normal flow
      toElement.style.cssText = '';
      toElement.style.display = targetDisplay;
      toElement.style.opacity = '1';
      toElement.classList.add('visible');
    } else if (toElement) {
      // No from element, just fade in normally
      toElement.style.display = targetDisplay;
      void toElement.offsetHeight;
      requestAnimationFrame(() => {
        toElement.classList.add('visible');
      });
      await waitForTransition(duration);
    }
  }

  /**
   * Teardown the panels system
   */
  teardown() {
    this.cancelPendingFocus();
    this._panels = {};
    this._loginPanel = null;
    this._overlay = null;
    this._currentPanel = 'login';
  }
}

/**
 * Login Password Manager
 *
 * Handles suppression and removal of password manager injected UI elements
 * (1Password, LastPass, Bitwarden, Dashlane, etc.).
 * Extracted from login.js to keep files under 1,000 lines per LITHIUM-INS.md rule #2.
 */


/**
 * PasswordManager class
 *
 * Owns MutationObserver-based suppression of password manager UI elements.
 * Provides show/hide/remove lifecycle methods for use by LoginManager.
 */
class PasswordManager {
  constructor() {
    this._observer = null;
  }

  /**
   * Apply suppression styles to a password manager element.
   * @param {HTMLElement} el - Element to suppress
   * @private
   */
  _suppressElement(el) {
    el.style.setProperty('display', 'none', 'important');
    el.style.setProperty('visibility', 'hidden', 'important');
    el.style.setProperty('opacity', '0', 'important');
    el.style.setProperty('pointer-events', 'none', 'important');
  }

  /**
   * Suppress elements matching a selector.
   * @param {string} selector - CSS selector
   * @private
   */
  _suppressSelector(selector) {
    try {
      document.querySelectorAll(selector).forEach((el) => this._suppressElement(el));
    } catch (e) {
      // Invalid selector, skip
    }
  }

  /**
   * Suppress all password manager elements and reconnect observer.
   * @param {string[]} selectors - Array of CSS selectors
   * @private
   */
  _suppressAndReconnect(selectors) {
    // Temporarily disconnect the observer while we mutate styles so that our
    // own setAttribute calls do not re-trigger this callback (infinite loop).
    // We immediately reconnect after the sweep so new 1Password injections
    // are still caught.
    this._observer?.disconnect();

    try {
      for (const selector of selectors) {
        this._suppressSelector(selector);
      }
    } finally {
      // Reconnect so future 1Password re-injections/re-shows are caught.
      // Guard against calling observe() after teardown (observer may be null).
      if (this._observer) {
        this._observer.observe(document.body, {
          childList: true,
          subtree: true,
          attributes: true,
          attributeFilter: ['style', 'class'],
        });
      }
    }
  }

  /**
   * Start password manager UI suppression with MutationObserver.
   * @param {string[]} selectors - Array of CSS selectors
   * @private
   */
  _startSuppression(selectors) {
    const suppressElements = () => this._suppressAndReconnect(selectors);

    // Create the observer (initially disconnected; suppressElements reconnects it).
    this._observer = new MutationObserver(suppressElements);

    // Run the initial suppression sweep, which also starts the observer.
    suppressElements();

    // Schedule a follow-up sweep on the next animation frame so we get the
    // last word after any 1Password re-assertion in the same microtask flush.
    requestAnimationFrame(suppressElements);
  }

  /**
   * Stop password manager UI suppression.
   * @private
   */
  _stopSuppression() {
    document.body.classList.remove('hide-password-manager-ui');
    this._observer?.disconnect();
    this._observer = null;
  }

  /**
   * Show password manager UI (stop suppression).
   */
  show() {
    this._stopSuppression();
  }

  /**
   * Hide password manager UI (start suppression).
   */
  hide() {
    const selectors = getPasswordManagerSelectors();
    this._startSuppression(selectors);
  }

  /**
   * Remove all password manager injected elements from the DOM.
   * Called after a successful login — at that point we want the elements gone
   * entirely, not just hidden, so they do not linger in the post-login view.
   */
  removeAll() {
    const selectors = getPasswordManagerSelectors();
    for (const selector of selectors) {
      try {
        document.querySelectorAll(selector).forEach((el) => el.remove());
      } catch (e) {
        // Invalid selector, skip
      }
    }
  }

  /**
   * Destroy the password manager handler.
   * Disconnects observer, removes body class, and removes any lingering elements.
   */
  destroy() {
    this._observer?.disconnect();
    this._observer = null;
    document.body.classList.remove('hide-password-manager-ui');
    this.removeAll();
  }
}

// Convenience alias for this module's subsystem
const LOGIN$3 = Subsystems.LOGIN;

/**
 * LanguagePanel class
 *
 * Owns the Tabulator language table, locale detection, filter handling,
 * keyboard navigation, and the current-locale indicator.
 * Delegates to LoginManager via callbacks for panel visibility changes.
 */
class LanguagePanel {
  /**
   * @param {Object} options
   * @param {Object} options.elements - DOM element references from LoginManager
   * @param {Function} [options.onLocaleSelected] - Called when a locale is selected: (locale, previousLocale) => void
   */
  constructor({ elements, onLocaleSelected }) {
    this.elements = elements || {};
    this._onLocaleSelected = onLocaleSelected || (() => {});
    this._languageTable = null;
    this._languageData = [];
    this._currentLocale = null;
    this._bestGuessLocale = null;
    this._keydownHandler = null;
  }

  /**
   * Initialize the language panel (called when panel is shown)
   */
  async init() {
    // If table already exists, just refresh the data and selection
    if (this._languageTable) {
      this._refreshSelection();
      this._focusTable();
      return;
    }

    const tableContainer = this.elements.languageTable;
    if (!tableContainer) {
      console.warn('[LanguagePanel] Language table container not found');
      return;
    }

    try {
      await this._setupData();
      this._updateCurrentLocaleButton();
      await this._createTable(tableContainer);
      this._setupEvents();
      this._finalizeSetup();

      log(LOGIN$3, Status.INFO, `Language panel initialized. Best guess: ${this._bestGuessLocale}, Current: ${this._currentLocale}`);
    } catch (error) {
      this._handleError(error, tableContainer);
    }
  }

  /**
   * Clean up when panel is hidden
   */
  hide() {
    // Reset filter when leaving language panel
    if (this.elements.languageFilter) {
      this.elements.languageFilter.value = '';
    }
    // Remove keyboard navigation listener
    if (this._keydownHandler) {
      document.removeEventListener('keydown', this._keydownHandler);
      this._keydownHandler = null;
    }
  }

  /**
   * Show handler - re-initializes if needed and attaches keyboard listener
   */
  async show() {
    await this.init();
    // Add keyboard navigation listener
    this._keydownHandler = (e) => this._handleKeydown(e);
    document.addEventListener('keydown', this._keydownHandler);
  }

  /**
   * Destroy the language panel and clean up resources
   */
  destroy() {
    if (this._languageTable) {
      this._languageTable.destroy();
      this._languageTable = null;
    }
    if (this._keydownHandler) {
      document.removeEventListener('keydown', this._keydownHandler);
      this._keydownHandler = null;
    }
    this._languageData = [];
    this._currentLocale = null;
    this._bestGuessLocale = null;
  }

  /**
   * Filter the language table based on search input
   * @param {string} filterText - Text to filter by
   */
  filter(filterText) {
    if (!this._languageTable) return;

    const searchLower = filterText.toLowerCase().trim();

    if (!searchLower) {
      this._languageTable.clearFilter();
      return;
    }

    this._languageTable.setFilter((data) => {
      const localeMatch = data.locale.toLowerCase().includes(searchLower);
      const languageMatch = data.language && data.language.toLowerCase().includes(searchLower);
      const countryMatch = data.country && data.country.toLowerCase().includes(searchLower);
      const nativeMatch = data.nativeName && data.nativeName.toLowerCase().includes(searchLower);

      return localeMatch || languageMatch || countryMatch || nativeMatch;
    });
  }

  /**
   * Select a language and save preference
   * @param {string} locale - The selected locale
   */
  selectLanguage(locale) {
    if (!supportedLocales.includes(locale)) {
      console.warn(`[LanguagePanel] Unsupported locale: ${locale}`);
      return;
    }

    const previousLocale = this._currentLocale;
    this._currentLocale = locale;
    saveLocalePreference(locale);

    // Update the current-locale indicator in the header
    this._updateCurrentLocaleButton();

    // Update row selection in table and scroll to it
    if (this._languageTable) {
      this._languageTable.deselectRow();
      const row = this._languageTable.getRows().find(r => r.getData().locale === locale);
      if (row) {
        row.select();
        this._languageTable.scrollToRow(row, 'middle', false);
      }
    }

    // Log the selection
    const langData = this._languageData.find(l => l.locale === locale);
    const langName = langData ? `${langData.language} (${langData.country})` : locale;
    log(LOGIN$3, Status.SUCCESS, `Language selected: ${langName} (${locale})`);

    // Emit locale changed event
    eventBus.emit(Events.LOCALE_CHANGED, {
      lang: locale,
      previousLang: previousLocale,
    });

    // Notify delegate
    this._onLocaleSelected(locale, previousLocale);
  }

  /**
   * Get the current locale
   * @returns {string|null}
   */
  getCurrentLocale() {
    return this._currentLocale;
  }

  /**
   * Get the best guess locale
   * @returns {string|null}
   */
  getBestGuessLocale() {
    return this._bestGuessLocale;
  }

  // ---------------------------------------------------------------------------
  // Private methods
  // ---------------------------------------------------------------------------

  /**
   * Set up language data and determine current locale
   * @private
   */
  async _setupData() {
    this._languageData = getLanguageData();

    const savedLocale = getSavedLocale();
    if (savedLocale && supportedLocales.includes(savedLocale)) {
      this._currentLocale = savedLocale;
      this._bestGuessLocale = savedLocale;
    } else {
      const ipinfoToken = getConfigValue('services.ipinfo_token', null);
      this._bestGuessLocale = await getBestGuessLocale({ ipinfoToken });
      this._currentLocale = this._bestGuessLocale;
    }
  }

  /**
   * Create the Tabulator language table
   * @param {HTMLElement} tableContainer - Container element
   * @private
   */
  async _createTable(tableContainer) {
    const TabulatorModule = await __vitePreload(() => import('./tabulator_esm-DsDetXL7.js'),true              ?[]:void 0,import.meta.url);
    const Tabulator = TabulatorModule.TabulatorFull;

    if (typeof Tabulator !== 'function') {
      console.error('[LanguagePanel] Tabulator import failed. Module structure:', Object.keys(TabulatorModule));
      throw new Error(`Tabulator is not a constructor. Type: ${typeof Tabulator}`);
    }

    const sortElement = '<span class="lang-sort-icons"><span class="lang-sort-asc">▲</span><span class="lang-sort-desc">▼</span></span>';

    this._languageTable = new Tabulator(tableContainer, {
      data: this._languageData,
      layout: 'fitColumns',
      height: '100%',
      selectable: 1,
      resizableColumns: false,
      movableColumns: true,
      headerSortTristate: true,
      headerSortElement: sortElement,
      initialSort: [
        { column: 'language', dir: 'asc' },
        { column: 'country', dir: 'asc' },
      ],
      columns: this._getColumns(),
    });
  }

  /**
   * Get column definitions for language table
   * @returns {Array} Column definitions
   * @private
   */
  _getColumns() {
    return [
      {
        title: '',
        field: 'countryCode',
        width: 50,
        frozen: true,
        hozAlign: 'center',
        vertAlign: 'middle',
        cssClass: 'language-flag-cell',
        headerSort: false,
        resizable: false,
        formatter: (cell) => this._getFlagSvg(cell.getValue()),
      },
      {
        title: 'Language',
        field: 'language',
        widthGrow: 2,
        hozAlign: 'left',
        vertAlign: 'middle',
        headerSort: true,
        resizable: false,
      },
      {
        title: 'Country',
        field: 'country',
        widthGrow: 2,
        hozAlign: 'left',
        vertAlign: 'middle',
        headerSort: true,
        resizable: false,
      },
      {
        title: 'Locale',
        field: 'locale',
        width: 80,
        hozAlign: 'left',
        vertAlign: 'middle',
        headerSort: true,
        resizable: false,
      },
    ];
  }

  /**
   * Get SVG flag HTML for a country code
   * @param {string} countryCode - ISO 3166-1 alpha-2 country code
   * @returns {string} SVG HTML string
   * @private
   */
  _getFlagSvg(countryCode) {
    return getFlagSvg(countryCode);
  }

  /**
   * Set up language table event handlers
   * @private
   */
  _setupEvents() {
    this._languageTable.on('rowClick', (e, row) => {
      this.selectLanguage(row.getData().locale);
    });

    this._languageTable.on('rowSelected', (row) => {
      this._currentLocale = row.getData().locale;
    });
  }

  /**
   * Finalize table setup with selection and focus
   * @private
   */
  _finalizeSetup() {
    setTimeout(() => {
      this._refreshSelection();
      this._focusTable();
    }, 100);
  }

  /**
   * Handle language table initialization error
   * @param {Error} error - Error object
   * @param {HTMLElement} tableContainer - Container element
   * @private
   */
  _handleError(error, tableContainer) {
    console.error('[LanguagePanel] Failed to initialize language table:', error);
    log(LOGIN$3, Status.ERROR, `Failed to initialize language table: ${error.message}`);
    this._renderFallback(tableContainer);
  }

  /**
   * Render fallback language list
   * @param {HTMLElement} tableContainer - Container element
   * @private
   */
  _renderFallback(tableContainer) {
    tableContainer.innerHTML = this._languageData.map(lang => `
      <div class="language-item-fallback" data-locale="${lang.locale}" style="
        padding: var(--space-3);
        border-bottom: var(--border-standard);
        cursor: pointer;
        display: flex;
        align-items: center;
        gap: var(--space-3);
        ${lang.locale === this._currentLocale ? 'background: var(--accent-primary); color: white;' : ''}
      ">
        ${this._getFlagSvg(lang.countryCode)}
        <span style="flex: 1;">${lang.language}</span>
        <span style="color: ${lang.locale === this._currentLocale ? 'rgba(255,255,255,0.8)' : 'var(--text-muted)'}">${lang.country}</span>
        <span style="color: ${lang.locale === this._currentLocale ? 'rgba(255,255,255,0.8)' : 'var(--text-muted)'}">${lang.locale}</span>
      </div>
    `).join('');

    tableContainer.querySelectorAll('.language-item-fallback').forEach(item => {
      item.addEventListener('click', () => this.selectLanguage(item.dataset.locale));
    });
  }

  /**
   * Update the current-locale indicator button in the language panel header
   * @private
   */
  _updateCurrentLocaleButton() {
    if (!this._currentLocale) return;

    const countryCode = getCountryCode(this._currentLocale);

    if (this.elements.languageCurrentFlag) {
      try {
        const flagSvg = getFlagSvg(countryCode, { width: 22, height: 16 });
        this.elements.languageCurrentFlag.innerHTML = flagSvg;
      } catch (e) {
        this.elements.languageCurrentFlag.textContent = countryCode;
      }
    }

    if (this.elements.languageCurrentCode) {
      this.elements.languageCurrentCode.textContent = this._currentLocale;
    }
  }

  /**
   * Refresh the language table selection and scroll to selected row
   * @param {number} retryCount - Number of retries attempted (internal use)
   * @private
   */
  _refreshSelection(retryCount = 0) {
    if (!this._languageTable || !this._currentLocale) return;

    const rows = this._languageTable.getRows();
    const targetRow = rows.find(r => r.getData().locale === this._currentLocale);

    if (targetRow) {
      this._languageTable.deselectRow();
      targetRow.select();
      this._languageTable.scrollToRow(targetRow, 'middle', false);
    } else if (retryCount < 3) {
      setTimeout(() => {
        this._refreshSelection(retryCount + 1);
      }, 100);
    }
  }

  /**
   * Focus the language table for keyboard navigation
   * @private
   */
  _focusTable() {
    setTimeout(() => {
      const tableElement = this.elements.languageTable;
      if (tableElement) {
        tableElement.setAttribute('tabindex', '-1');
        tableElement.focus();
      }
    }, 100);
  }

  /**
   * Handle keyboard navigation in the language table
   * @param {KeyboardEvent} event - The keyboard event
   * @private
   */
  _handleKeydown(event) {
    if (!this._languageTable) return;

    const key = event.key;
    let handled = false;

    switch (key) {
      case 'ArrowDown':
        this._moveSelection(1);
        handled = true;
        break;
      case 'ArrowUp':
        this._moveSelection(-1);
        handled = true;
        break;
      case 'PageDown':
        this._moveSelection(10);
        handled = true;
        break;
      case 'PageUp':
        this._moveSelection(-10);
        handled = true;
        break;
      case 'Enter':
        this._selectCurrentRow();
        handled = true;
        break;
      case 'Home':
        this._moveSelectionTo(0);
        handled = true;
        break;
      case 'End':
        this._moveSelectionTo(-1);
        handled = true;
        break;
    }

    if (handled) {
      event.preventDefault();
      event.stopPropagation();
    }
  }

  /**
   * Move the language selection by a number of rows
   * @param {number} delta - Number of rows to move (positive = down, negative = up)
   * @private
   */
  _moveSelection(delta) {
    if (!this._languageTable) return;

    const rows = this._languageTable.getRows('active');
    if (rows.length === 0) return;

    let currentIndex = -1;
    const selectedRows = this._languageTable.getSelectedRows();
    if (selectedRows.length > 0) {
      currentIndex = rows.findIndex(r => r.getData().locale === selectedRows[0].getData().locale);
    }

    let newIndex = currentIndex + delta;
    if (newIndex < 0) newIndex = 0;
    if (newIndex >= rows.length) newIndex = rows.length - 1;

    this._selectRowAtIndex(newIndex);
  }

  /**
   * Move selection to a specific row index
   * @param {number} index - Row index (0 = first, -1 = last)
   * @private
   */
  _moveSelectionTo(index) {
    if (!this._languageTable) return;

    const rows = this._languageTable.getRows('active');
    if (rows.length === 0) return;

    let newIndex = index;
    if (newIndex < 0) newIndex = rows.length + newIndex;
    if (newIndex < 0) newIndex = 0;
    if (newIndex >= rows.length) newIndex = rows.length - 1;

    this._selectRowAtIndex(newIndex);
  }

  /**
   * Select a row at a specific index and scroll it into view
   * @param {number} index - The row index to select
   * @private
   */
  _selectRowAtIndex(index) {
    if (!this._languageTable) return;

    const rows = this._languageTable.getRows('active');
    if (index < 0 || index >= rows.length) return;

    const targetRow = rows[index];

    this._languageTable.deselectRow();
    targetRow.select();
    this._languageTable.scrollToRow(targetRow, 'middle', false);
  }

  /**
   * Select the currently selected language row (Enter key action)
   * @private
   */
  _selectCurrentRow() {
    if (!this._languageTable) return;

    const selectedRows = this._languageTable.getSelectedRows();
    if (selectedRows.length > 0) {
      const data = selectedRows[0].getData();
      this.selectLanguage(data.locale);
    }
  }
}

/**
 * LogsPanel class
 *
 * Owns the CodeMirror editor lifecycle for the logs panel. Follows the
 * show/hide/destroy pattern established by PasswordManager and LanguagePanel.
 *
 * The panel has trivial show/hide semantics: `show()` populates (or refreshes)
 * the editor; `hide()` is a no-op (the editor stays alive in case the user
 * reopens the panel). `destroy()` tears everything down.
 */
class LogsPanel {
  /**
   * @param {Object} options
   * @param {Object} options.elements - DOM element references; must contain `logViewer`
   * @param {Object} [options.imports] - Optional import overrides for testing.
   *   `imports.codemirror` and `imports.codemirrorSetup` may be supplied as
   *   pre-resolved modules; otherwise they are dynamically imported on first
   *   `show()`.
   */
  constructor({ elements, imports } = {}) {
    this.elements = elements || {};
    this._imports = imports || null;
    this._editor = null;
  }

  /**
   * Populate (or refresh) the logs panel. Called when the panel becomes visible.
   * Idempotent: subsequent calls update the editor's content rather than
   * reinitialising the editor.
   */
  async show() {
    const logViewer = this.elements.logViewer;
    if (!logViewer) {
      console.warn('[LogsPanel] logViewer element not found');
      return;
    }

    const entries = getRawLog();
    const logText = formatLogText(entries);

    // If CodeMirror is already initialised, just update the content.
    if (this._editor) {
      this._editor.dispatch({
        changes: { from: 0, to: this._editor.state.doc.length, insert: logText }
      });
      return;
    }

    // First-time init: try CodeMirror, fall back to <pre> on import failure.
    try {
      const cm = this._imports?.codemirror
        ? this._imports.codemirror
        : await __vitePreload(() => import('./codemirror-Bg1rM8n2.js').then(n => n.R),true              ?[]:void 0,import.meta.url);
      const cmSetup = this._imports?.codemirrorSetup
        ? this._imports.codemirrorSetup
        : await __vitePreload(() => import('./codemirror-setup-lxLGSWU-.js'),true              ?__vite__mapDeps([0,1]):void 0,import.meta.url);

      const { EditorState, EditorView } = cm;
      const { buildEditorExtensions, createReadOnlyCompartment } = cmSetup;

      const roCompartment = createReadOnlyCompartment();
      const extensions = buildEditorExtensions({
        language: 'log',
        readOnlyCompartment: roCompartment,
        readOnly: true,
        fontSize: 10,
        fontFamily: "'Vanadium Mono', 'Courier New', Courier, monospace",
      });

      const state = EditorState.create({ doc: logText, extensions });

      logViewer.innerHTML = '';
      this._editor = new EditorView({ state, parent: logViewer });
    } catch (error) {
      console.warn('[LogsPanel] CodeMirror failed to load, using plain text:', error);
      logViewer.innerHTML = `<pre class="log-content">${logText}</pre>`;
    }
  }

  /**
   * Called when the logs panel is being hidden. Currently a no-op:
   * the editor instance persists so reopening is instant.
   */
  hide() {
    // Intentional no-op.
  }

  /**
   * Destroy the editor and any associated scrollbar instance.
   * Called from LoginManager.teardown().
   */
  destroy() {
    // Clean up CodeMirror OverlayScrollbars first.
    if (this._editor && this._editor._osbInstance) {
      scrollbarManager.destroy(this._editor._osbInstance);
      this._editor._osbInstance = null;
    }

    // Destroy the editor view.
    if (this._editor) {
      this._editor.destroy();
      this._editor = null;
    }
  }

  /**
   * Test-only accessor for the editor instance. Avoid in production code.
   * @internal
   */
  get _editorInstance() {
    return this._editor;
  }
}

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
class HelpPanel {
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

/**
 * Login Form
 *
 * Owns the login form lifecycle: input handlers, credential submission to the
 * Hydrogen `/api/auth/login` endpoint, JWT storage, error mapping, loading
 * state, password visibility toggle, clear-username helper, and the
 * "remember last username" persistence in `localStorage`.
 *
 * Extracted from login.js to keep files under 1,000 lines per LITHIUM-INS.md
 * rule #2. Designed as a self-contained module: it receives DOM element
 * references and a small set of callbacks, never reaches up into LoginManager.
 *
 * Lifecycle: `init()` (binds listeners) → `destroy()` (removes listeners and
 * clears state). `LoginManager` calls `init()` from `render()` and
 * `destroy()` from `teardown()`.
 */


const LOGIN$2 = Subsystems.LOGIN;
const REMEMBERED_USERNAME_KEY = 'lithium_last_username';
const NO_SAVE_USERNAME_KEY = 'lithium_no_save_username';

class LoginForm {
  /**
   * @param {Object} options
   * @param {Object} options.elements - DOM element references. Required:
   *   `form`, `username`, `password`, `submit`, `error`, `errorText`,
   *   `clearUsernameBtn`, `togglePasswordBtn`, `passwordIcon`. Optional
   *   buttons used during loading-state toggling: `languageBtn`,
   *   `themeBtn`, `logsBtn`, `helpBtn`.
   * @param {Function} [options.onLoginSuccess] - Called with the parsed
   *   server response after JWT is stored, before the success log/event.
   *   Typical use: trigger `LoginManager.hide()`. Async; awaited.
   * @param {Function} [options.isStartupComplete] - Returns boolean.
   *   Used by `setLoading()` to decide if the logs button should be
   *   re-enabled when loading finishes. Defaults to `() => true`.
   * @param {Object} [options.deps] - Dependency overrides (for testing).
   *   - `deps.lithiumSettings` — override for `window.lithiumSettings`.
   */
  constructor({ elements, onLoginSuccess, isStartupComplete, deps } = {}) {
    this.elements = elements || {};
    this._onLoginSuccess = onLoginSuccess || (async () => {});
    this._isStartupComplete = isStartupComplete || (() => true);
    this._deps = deps || {};

    this.isSubmitting = false;
    this.isPasswordVisible = false;
    this._loginStart = null;

    // Bound handler refs so we can remove them in destroy().
    this._handlers = {};
  }

  /**
   * Bind all form-related event listeners. Idempotent: a second call
   * removes the previous listeners before re-binding.
   */
  init() {
    this.destroy();

    const onSubmit = (e) => {
      e.preventDefault();
      this.handleSubmit();
    };
    const onUsernameInput = () => this.hideError();
    const onPasswordInput = () => this.hideError();
    const onUsernameBlur = () => {
      const hasValue = !!(this.elements.username?.value?.trim());
      log(LOGIN$2, Status.INFO, `Username field: ${hasValue ? 'filled' : 'empty'}`);
    };
    const onPasswordBlur = () => {
      const hasValue = !!(this.elements.password?.value);
      log(LOGIN$2, Status.INFO, `Password field: ${hasValue ? 'filled' : 'empty'}`);
    };
    const onClearUsernameClick = () => this.handleClearUsername();
    const onToggleVisibilityClick = () => this.handleTogglePassword();

    this.elements.form?.addEventListener('submit', onSubmit);
    this.elements.username?.addEventListener('input', onUsernameInput);
    this.elements.password?.addEventListener('input', onPasswordInput);
    this.elements.username?.addEventListener('blur', onUsernameBlur);
    this.elements.password?.addEventListener('blur', onPasswordBlur);
    this.elements.clearUsernameBtn?.addEventListener('click', onClearUsernameClick);
    this.elements.togglePasswordBtn?.addEventListener('click', onToggleVisibilityClick);

    this._handlers = {
      onSubmit,
      onUsernameInput,
      onPasswordInput,
      onUsernameBlur,
      onPasswordBlur,
      onClearUsernameClick,
      onToggleVisibilityClick,
    };
  }

  /**
   * Remove all event listeners and clear handler refs. Safe to call before
   * `init()` has run.
   */
  destroy() {
    const h = this._handlers;
    if (!h || Object.keys(h).length === 0) return;

    this.elements.form?.removeEventListener('submit', h.onSubmit);
    this.elements.username?.removeEventListener('input', h.onUsernameInput);
    this.elements.password?.removeEventListener('input', h.onPasswordInput);
    this.elements.username?.removeEventListener('blur', h.onUsernameBlur);
    this.elements.password?.removeEventListener('blur', h.onPasswordBlur);
    this.elements.clearUsernameBtn?.removeEventListener('click', h.onClearUsernameClick);
    this.elements.togglePasswordBtn?.removeEventListener('click', h.onToggleVisibilityClick);

    this._handlers = {};
    this.isPasswordVisible = false;
  }

  /**
   * Read a previously remembered username from `localStorage` and pre-fill
   * the username field. Returns true if a value was loaded (so callers can
   * skip the username focus step in favour of focusing the password field).
   * @returns {boolean}
   */
  loadRememberedUsername() {
    try {
      const remembered = localStorage.getItem(REMEMBERED_USERNAME_KEY);
      if (remembered && this.elements.username) {
        this.elements.username.value = remembered;
        return true;
      }
      return false;
    } catch (error) {
      console.warn('[LoginForm] Could not load remembered username:', error);
      return false;
    }
  }

  /**
   * Persist the supplied username to `localStorage` for next visit, unless
   * the session-scoped suppression flag is set (e.g. after a public/global
   * logout).
   * @param {string} username
   */
  saveRememberedUsername(username) {
    try {
      if (sessionStorage.getItem(NO_SAVE_USERNAME_KEY) === 'true') {
        sessionStorage.removeItem(NO_SAVE_USERNAME_KEY);
        return;
      }
      if (username) {
        localStorage.setItem(REMEMBERED_USERNAME_KEY, username);
      }
    } catch (error) {
      console.warn('[LoginForm] Could not save username:', error);
    }
  }

  /**
   * Pre-fill credentials and immediately submit the form. Used by
   * `LoginManager.checkForAutoLogin()` for the URL-driven auto-login flow.
   * @param {string} username
   * @param {string} password
   */
  prefillAndSubmit(username, password) {
    if (this.elements.username) this.elements.username.value = username;
    if (this.elements.password) this.elements.password.value = password;
    setTimeout(() => this.handleSubmit(), 100);
  }

  /**
   * Clear both fields, focus username, and dismiss any visible error.
   */
  handleClearUsername() {
    log(LOGIN$2, Status.INFO, 'Credentials cleared');
    if (this.elements.username) {
      this.elements.username.value = '';
      this.elements.username.focus();
    }
    if (this.elements.password) {
      this.elements.password.value = '';
    }
    this.hideError();
  }

  /**
   * Toggle the password field between `password` and `text` types, swap
   * the eye/eye-slash icon, and refresh the toggle button's tooltip text.
   */
  handleTogglePassword() {
    if (!this.elements.password || !this.elements.passwordIcon) return;

    this.isPasswordVisible = !this.isPasswordVisible;
    log(LOGIN$2, Status.INFO, `Password visibility: ${this.isPasswordVisible ? 'shown' : 'hidden'}`);

    const tooltipText = this.isPasswordVisible ? 'Hide password' : 'Show password';
    const addIcon = this.isPasswordVisible ? 'fa-eye-slash' : 'fa-eye';
    const removeIcon = this.isPasswordVisible ? 'fa-eye' : 'fa-eye-slash';

    this.elements.password.type = this.isPasswordVisible ? 'text' : 'password';
    this.elements.passwordIcon.classList.remove(removeIcon);
    this.elements.passwordIcon.classList.add(addIcon);

    if (this.elements.togglePasswordBtn) {
      this.elements.togglePasswordBtn.setAttribute('data-tooltip', tooltipText);
      const tip = getTip(this.elements.togglePasswordBtn);
      if (tip) tip.updateContent(tooltipText);
    }
  }

  /**
   * Validate form values and dispatch a login attempt. Guards against
   * concurrent submissions.
   */
  async handleSubmit() {
    if (this.isSubmitting) return;

    const username = this.elements.username?.value?.trim();
    const password = this.elements.password?.value;

    if (!username || !password) {
      log(LOGIN$2, Status.WARN, 'Login attempted with empty credentials');
      this.showError('Please enter both username and password');
      return;
    }

    log(LOGIN$2, Status.INFO, `Login attempt: user "${username}"`);
    this._loginStart = performance.now();

    this.isSubmitting = true;
    this.setLoading(true);
    this.hideError();

    try {
      await this.attemptLogin(username, password);
    } catch (error) {
      this.handleLoginError(error);
    } finally {
      this.isSubmitting = false;
      this.setLoading(false);
    }
  }

  /**
   * POST credentials to Hydrogen, store JWT on success, invoke the
   * `onLoginSuccess` callback, then emit `AUTH_LOGIN`.
   * @param {string} username
   * @param {string} password
   */
  async attemptLogin(username, password) {
    const serverUrl = getConfigValue('server.url', 'http://localhost:8080');
    const apiPrefix = getConfigValue('server.api_prefix', '/api');

    const loginData = {
      login_id: username,
      password,
      api_key: getConfigValue('auth.api_key', ''),
      tz: Intl.DateTimeFormat().resolvedOptions().timeZone,
      database: getConfigValue('auth.default_database', 'Demo_PG'),
    };

    const response = await fetch(`${serverUrl}${apiPrefix}/auth/login`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'Accept': 'application/json',
      },
      body: JSON.stringify(loginData),
    });

    if (!response.ok) {
      const error = new Error(`HTTP ${response.status}`);
      error.status = response.status;
      try {
        error.data = await response.json();
      } catch (e) {
        // Body is not JSON; ignore.
      }
      if (response.status === 429) {
        const retryAfter = response.headers.get('retry-after');
        error.retryAfter = retryAfter ? parseInt(retryAfter, 10) : null;
      }
      throw error;
    }

    const data = await response.json();

    if (!data.token) {
      throw new Error('No token received from server');
    }
    storeJWT(data.token);

    // Record the last-used login method so the UI can subtly highlight it
    // on the next visit (Phase 26).
    const settings = this._deps.lithiumSettings ??
      (typeof window !== 'undefined' ? window.lithiumSettings : null);
    settings?.set('auth.last_method', 'password');

    this.saveRememberedUsername(username);

    // Hand off to caller (e.g. LoginManager.hide()) before emitting the
    // login event, so any UI fade-out completes first.
    await this._onLoginSuccess(data);

    const loginDuration = this._loginStart
      ? Math.round(performance.now() - this._loginStart)
      : 0;
    this._loginStart = null;
    log(LOGIN$2, Status.SUCCESS, `Login successful: user "${username}"`, loginDuration);

    eventBus.emit(Events.AUTH_LOGIN, {
      userId: data.user_id,
      username,
      roles: data.roles || [],
      expiresAt: data.expires_at,
    });
  }

  /**
   * Map an error from `attemptLogin` to a user-facing message and surface it.
   * @param {Error & { status?: number, data?: any, retryAfter?: number|null }} error
   */
  handleLoginError(error) {
    const loginDuration = this._loginStart
      ? Math.round(performance.now() - this._loginStart)
      : 0;
    this._loginStart = null;
    log(LOGIN$2, Status.ERROR, `Login failed: HTTP ${error.status || error.message}`, loginDuration);
    console.error('[LoginForm] Login failed:', error);

    let message = 'Login failed. Please try again.';

    switch (error.status) {
      case 401:
        message = error.data?.message || 'Invalid username or password';
        if (this.elements.password) {
          this.elements.password.value = '';
          this.elements.password.focus();
        }
        break;

      case 429:
        if (error.retryAfter) {
          const minutes = Math.ceil(error.retryAfter / 60);
          message = `Too many attempts. Please try again in ${minutes} minute${minutes > 1 ? 's' : ''}.`;
        } else {
          message = 'Too many attempts. Please try again later.';
        }
        break;

      case 500:
      case 502:
      case 503:
      case 504:
        message = 'Server error. Please try again later.';
        break;

      default:
        if (typeof navigator !== 'undefined' && !navigator.onLine) {
          message = 'No internet connection. Please check your network.';
        } else {
          message = error.data?.message || 'Login failed. Please try again.';
        }
    }

    this.showError(message);
  }

  /**
   * Show an error banner below the form.
   * @param {string} message
   */
  showError(message) {
    if (this.elements.errorText) {
      this.elements.errorText.textContent = message;
    }
    this.elements.error?.classList.add('visible');
  }

  /**
   * Hide the error banner.
   */
  hideError() {
    this.elements.error?.classList.remove('visible');
  }

  /**
   * Toggle the loading state of the submit button and ancillary buttons.
   * Buttons whose enabled state depends on lookup availability or startup
   * completion are restored to their proper state when `loading` is false.
   * @param {boolean} loading
   */
  setLoading(loading) {
    if (this.elements.submit) {
      this.elements.submit.disabled = loading;
      this.elements.submit.innerHTML = loading
        ? '<span class="spinner-fancy spinner-fancy-sm"></span> &nbsp;&nbsp; <span>Logging in...</span>'
        : '<fa fa-sign-in-alt></fa> <span>Login</span>';
    }

    if (this.elements.languageBtn) {
      this.elements.languageBtn.disabled = loading || !hasLookup('lookup_names');
    }
    if (this.elements.themeBtn) {
      this.elements.themeBtn.disabled = loading || !hasLookup('themes');
    }
    if (this.elements.logsBtn) {
      this.elements.logsBtn.disabled = loading || !this._isStartupComplete();
    }
    if (this.elements.helpBtn) {
      this.elements.helpBtn.disabled = loading;
    }
    if (this.elements.clearUsernameBtn) {
      this.elements.clearUsernameBtn.disabled = loading;
    }
    if (this.elements.togglePasswordBtn) {
      this.elements.togglePasswordBtn.disabled = loading;
    }
  }
}

/**
 * Login Shortcuts
 *
 * Owns the document-level keyboard handlers for the login screen:
 *
 *   - ESC: clear username/password (on login panel) or close subpanel (on
 *     theme/logs/language/help panels).
 *   - Ctrl+Shift+U: focus + select-all on username.
 *   - Ctrl+Shift+P: focus + select-all on password.
 *   - F1:           click the help button.
 *   - Ctrl+Shift+I: click the language button.
 *   - Ctrl+Shift+T: click the theme button.
 *   - Ctrl+Shift+L: click the logs button.
 *   - CAPS LOCK indicator: toggles `caps-lock-active` on the password field.
 *
 * Extracted from login.js to keep files under 1,000 lines per LITHIUM-INS.md
 * rule #2. Designed as a self-contained module with explicit callbacks for
 * ESC behaviour; the rest reads from the supplied `elements` map directly.
 */


const LOGIN$1 = Subsystems.LOGIN;

class LoginShortcuts {
  /**
   * @param {Object} options
   * @param {Object} options.elements - DOM element refs. Required:
   *   `username`, `password`. Optional: `helpBtn`, `languageBtn`, `themeBtn`,
   *   `logsBtn` (any missing btn is silently ignored).
   * @param {Function} options.getCurrentPanel - Returns the currently active
   *   panel name (e.g. `'login'`, `'theme'`, …). Used to gate non-ESC
   *   shortcuts to the login panel.
   * @param {Function} options.onClearUsername - Called when ESC fires on the
   *   login panel.
   * @param {Function} options.onSwitchToLogin - Called when ESC fires on a
   *   subpanel.
   * @param {Document} [options.doc] - Document override for tests.
   */
  constructor({ elements, getCurrentPanel, onClearUsername, onSwitchToLogin, doc } = {}) {
    this.elements = elements || {};
    this._getCurrentPanel = getCurrentPanel || (() => 'login');
    this._onClearUsername = onClearUsername || (() => {});
    this._onSwitchToLogin = onSwitchToLogin || (() => {});
    this._doc = doc || (typeof document !== 'undefined' ? document : null);

    this.isCapsLockOn = false;
    this._handleKeyDown = null;
    this._handleKeyUp = null;
  }

  /**
   * Attach `keydown`/`keyup` listeners on the document. Idempotent: a second
   * call before `detach()` is a no-op.
   */
  attach() {
    if (this._handleKeyDown || !this._doc) return;

    this._handleKeyDown = (e) => {
      this.checkCapsLock(e);
      this.handleKeyboardShortcuts(e);
    };
    this._handleKeyUp = (e) => this.checkCapsLock(e);

    this._doc.addEventListener('keydown', this._handleKeyDown);
    this._doc.addEventListener('keyup', this._handleKeyUp);
  }

  /**
   * Remove the document-level listeners.
   */
  detach() {
    if (!this._doc) return;
    if (this._handleKeyDown) {
      this._doc.removeEventListener('keydown', this._handleKeyDown);
      this._handleKeyDown = null;
    }
    if (this._handleKeyUp) {
      this._doc.removeEventListener('keyup', this._handleKeyUp);
      this._handleKeyUp = null;
    }
  }

  /**
   * Dispatch a keyboard event to the matching shortcut handler. Public so
   * tests can drive it without a real document listener.
   * @param {KeyboardEvent} event
   */
  handleKeyboardShortcuts(event) {
    const isOnLoginPanel = this._getCurrentPanel() === 'login';

    if (event.key === 'Escape') {
      this._handleEscapeKey(event, isOnLoginPanel);
      return;
    }

    if (!isOnLoginPanel) return;

    const shortcuts = [
      { check: (e) => e.ctrlKey && e.shiftKey && e.key.toLowerCase() === 'u', handler: () => this._focusUsername() },
      { check: (e) => e.ctrlKey && e.shiftKey && e.key.toLowerCase() === 'p', handler: () => this._focusPassword() },
      { check: (e) => e.key === 'F1', handler: () => this._clickButton('helpBtn') },
      { check: (e) => e.ctrlKey && e.shiftKey && e.key.toLowerCase() === 'i', handler: () => this._clickButton('languageBtn') },
      { check: (e) => e.ctrlKey && e.shiftKey && e.key.toLowerCase() === 't', handler: () => this._clickButton('themeBtn') },
      { check: (e) => e.ctrlKey && e.shiftKey && e.key.toLowerCase() === 'l', handler: () => this._clickButton('logsBtn') },
    ];

    for (const shortcut of shortcuts) {
      if (shortcut.check(event)) {
        shortcut.handler();
        event.preventDefault();
        return;
      }
    }
  }

  /**
   * Update the CAPS-LOCK indicator class on the password input. Public so
   * tests can drive it directly.
   * @param {KeyboardEvent} event
   */
  checkCapsLock(event) {
    const capsLockOn = event.getModifierState && event.getModifierState('CapsLock');
    if (this.isCapsLockOn !== capsLockOn) {
      this.isCapsLockOn = capsLockOn;
      this._updateCapsLockIndicator(capsLockOn);
    }
  }

  // -------- private helpers --------

  /** @private */
  _handleEscapeKey(event, isOnLoginPanel) {
    if (isOnLoginPanel) {
      this._onClearUsername();
    } else {
      log(LOGIN$1, Status.INFO, `Keyboard: ESC closed ${this._getCurrentPanel()} panel`);
      this._onSwitchToLogin();
    }
    event.preventDefault();
  }

  /** @private */
  _focusUsername() {
    if (this.elements.username) {
      log(LOGIN$1, Status.INFO, 'Keyboard: Ctrl+Shift+U focused username field');
      this.elements.username.focus();
      this.elements.username.select();
    }
  }

  /** @private */
  _focusPassword() {
    if (this.elements.password) {
      log(LOGIN$1, Status.INFO, 'Keyboard: Ctrl+Shift+P focused password field');
      this.elements.password.focus();
      this.elements.password.select();
    }
  }

  /** @private */
  _clickButton(btnName) {
    const btn = this.elements[btnName];
    if (btn && !btn.disabled) btn.click();
  }

  /** @private */
  _updateCapsLockIndicator(isCapsLockOn) {
    if (!this.elements.password) return;
    if (isCapsLockOn) {
      this.elements.password.classList.add('caps-lock-active');
    } else {
      this.elements.password.classList.remove('caps-lock-active');
    }
  }
}

/**
 * OIDC Client - OpenID Connect helper for Lithium
 *
 * Pure ES6 module. No DOM access. Fully unit-testable.
 *
 * Provides two entry points:
 *   startOidc(provider, returnTo, deps)   — redirect the browser to the OIDC
 *                                           /start endpoint for the given provider.
 *   exchangeHandoff(handoff, deps)        — POST the one-time handoff code to
 *                                           /api/auth/oidc/handoff and return the
 *                                           Hydrogen JWT envelope.
 *
 * Both functions accept an optional `deps` object for dependency injection
 * during tests:
 *   deps.getConfigValue  — override getConfigValue (default: real config)
 *   deps.window          — override window (default: globalThis.window)
 *   deps.fetch           — override fetch (default: globalThis.fetch)
 *
 * Error shapes thrown by exchangeHandoff:
 *   { message, code: "handoff_invalid" }   on 401
 *   { message, code: "server_error" }      on other non-2xx
 *   { message, code: "network" }           on network / fetch failure
 */


// ---------------------------------------------------------------------------
// startOidc
// ---------------------------------------------------------------------------

/**
 * Redirect the browser to the OIDC authorization start endpoint.
 *
 * Builds:
 *   <server.url>/api/auth/oidc/start?database=<db>&return_to=<returnTo>
 *
 * This is a full top-level navigation (window.location.href assignment) so
 * that the browser follows the server's 302 redirect to Keycloak naturally.
 * An XHR/fetch cannot follow a cross-origin redirect.
 *
 * @param {string} providerId - Provider id matching an entry in
 *   auth.oidc_providers[*].id (e.g. "500passwords").
 * @param {string|null} returnTo - Optional relative path to deep-link to after
 *   sign-in. Must start with "/". Not validated here — the server validates it.
 * @param {Object} [deps] - Dependency injection for tests.
 * @param {Function} [deps.getConfigValue] - Config accessor.
 * @param {Object}   [deps.window]         - Window substitute.
 * @throws {Error} If the provider id is not found in config.
 */
function startOidc(providerId, returnTo = null, deps = {}) {
  const gcv = deps.getConfigValue ?? getConfigValue;
  const win = deps.window ?? (typeof window !== 'undefined' ? window : null);

  if (!providerId || typeof providerId !== 'string') {
    throw new Error('startOidc: providerId is required');
  }

  // Locate the provider entry in config.
  const providers = gcv('auth.oidc_providers', []);
  const provider = Array.isArray(providers)
    ? providers.find(p => p && p.id === providerId)
    : null;

  if (!provider) {
    throw new Error(`startOidc: unknown provider "${providerId}"`);
  }

  // Build the start URL.
  const serverUrl = gcv('server.url', '');
  const startPath = provider.start_url ?? '/api/auth/oidc/start';
  const database = gcv('auth.default_database', '');

  const url = new URL(startPath, serverUrl || undefined);

  if (database) {
    url.searchParams.set('database', database);
  }

  if (returnTo && typeof returnTo === 'string' && returnTo.startsWith('/')) {
    url.searchParams.set('return_to', returnTo);
  }

  const target = url.toString();

  log(Subsystems.AUTH, Status.INFO, `OIDC start: navigating to provider "${providerId}"`);

  if (win) {
    win.location.href = target;
  }

  return target;
}

// ---------------------------------------------------------------------------
// exchangeHandoff
// ---------------------------------------------------------------------------

/**
 * Exchange a one-time handoff code for a Hydrogen JWT.
 *
 * POSTs { "handoff": "<code>" } to /api/auth/oidc/handoff.
 * Returns the parsed response envelope on success:
 *   { token, expires_at, user_id, username, roles, success }
 *
 * Throws a structured Error on every failure:
 *   error.code = "handoff_invalid"  — 401 from the server
 *   error.code = "server_error"     — other non-2xx response
 *   error.code = "network"          — network / fetch failure
 *
 * The fetch uses credentials: "omit" (no cookies) and Cache-Control: no-store,
 * consistent with the Hydrogen /handoff security requirements.
 *
 * @param {string} handoff - The one-time handoff code from the URL parameter.
 * @param {Object} [deps]  - Dependency injection for tests.
 * @param {Function} [deps.getConfigValue] - Config accessor.
 * @param {Function} [deps.fetch]          - Fetch substitute.
 * @returns {Promise<Object>} Parsed response envelope.
 * @throws {Error} With a `.code` property describing the failure mode.
 */
async function exchangeHandoff(handoff, deps = {}) {
  const gcv = deps.getConfigValue ?? getConfigValue;
  const fetchFn = deps.fetch ?? (typeof fetch !== 'undefined' ? fetch : null);

  if (!handoff || typeof handoff !== 'string') {
    const err = new Error('exchangeHandoff: handoff code is required');
    err.code = 'network';
    throw err;
  }

  if (!fetchFn) {
    const err = new Error('exchangeHandoff: fetch is not available');
    err.code = 'network';
    throw err;
  }

  const serverUrl = gcv('server.url', '');
  const url = `${serverUrl}/api/auth/oidc/handoff`;

  log(Subsystems.AUTH, Status.INFO, 'OIDC handoff: exchanging handoff code');

  let response;
  try {
    response = await fetchFn(url, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'Accept': 'application/json',
        'Cache-Control': 'no-store',
      },
      credentials: 'omit',
      body: JSON.stringify({ handoff }),
    });
  } catch (fetchErr) {
    const err = new Error(`OIDC handoff exchange failed: ${fetchErr.message}`);
    err.code = 'network';
    err.cause = fetchErr;
    log(Subsystems.AUTH, Status.ERROR, `OIDC handoff: network error — ${fetchErr.message}`);
    throw err;
  }

  // Attempt to parse the response body regardless of status code.
  let data = null;
  try {
    const contentType = response.headers.get('content-type') ?? '';
    if (contentType.includes('application/json')) {
      data = await response.json();
    }
  } catch (_parseErr) {
    // Best-effort parse; a missing body is fine for error cases.
  }

  if (!response.ok) {
    const serverMessage = data?.error ?? data?.message ?? `HTTP ${response.status}`;
    const err = new Error(`OIDC handoff invalid: ${serverMessage}`);
    err.code = response.status === 401 ? 'handoff_invalid' : 'server_error';
    err.status = response.status;
    err.data = data;
    log(Subsystems.AUTH, Status.WARN,
      `OIDC handoff: server returned ${response.status} (${err.code})`);
    throw err;
  }

  log(Subsystems.AUTH, Status.SUCCESS,
    `OIDC handoff: exchange successful, user_id=${data?.user_id ?? 'unknown'}`);

  return data;
}

/**
 * OIDC Login — return-from-IdP processor
 *
 * Detects and handles the OIDC return leg: when Lithium boots with
 * `?oidc=1&handoff=<code>` (or `?oidc_error=<code>`) in the URL, this
 * module exchanges the handoff code for a Hydrogen JWT and completes login
 * exactly as if a password login had succeeded.
 *
 * Entry point: `processOidcReturn(loginManager)` — call once from
 * `LoginManager.init()`, before the login form is shown.
 *
 * Return value:
 *   true   — the URL contained OIDC parameters; caller should skip showing
 *            the normal login form immediately (we handled it).
 *   false  — URL had no OIDC parameters; nothing was done; caller proceeds
 *            normally.
 *
 * On success the function:
 *   1. Wipes `?oidc=1&handoff=…` from the URL via `history.replaceState`
 *      so the code is never visible in browser history.
 *   2. Stores the Hydrogen JWT via `storeJWT`.
 *   3. Hides the login UI.
 *   4. Emits `AUTH_LOGIN` with the same payload shape as password login.
 *
 * On error (oidc_error param or failed exchange):
 *   1. Wipes OIDC params from the URL.
 *   2. Displays a user-friendly, non-leaky error message via
 *      `loginManager.showError()`.
 *   3. Returns true so the caller still shows the login form.
 *
 * Security:
 *   - The URL is cleaned *immediately*, before any async work, so the
 *     handoff code is not visible for more than one event-loop tick.
 *   - Error codes from the server are mapped to enumerated user-facing
 *     strings; the raw server value is never echoed into the DOM.
 */


const AUTH = Subsystems.AUTH;

// ---------------------------------------------------------------------------
// User-facing error messages (never echo the raw server code to the DOM)
// ---------------------------------------------------------------------------

const OIDC_ERROR_MESSAGES = {
  // Errors forwarded from the IdP via ?oidc_error= on /callback
  state_invalid:              'Sign-in session expired or was tampered with. Please try again.',
  idp_error:                  'The identity provider reported an error. Please try again.',
  no_account:                 'No account is linked to this identity. Contact your administrator.',
  no_api_key:                 'Server configuration error. Contact your administrator.',
  email_ambiguous:            'Multiple accounts share this email address. Contact your administrator.',
  provision_disallowed_email: 'Your email domain is not allowed. Contact your administrator.',
  server_error:               'An unexpected server error occurred. Please try again later.',

  // Errors produced locally by exchangeHandoff
  handoff_invalid:            'Sign-in could not be verified. Please try again.',
  network:                    'Could not connect to the server. Check your connection and try again.',
};

const DEFAULT_OIDC_ERROR_MESSAGE =
  'Sign-in could not complete. Please try again.';

/**
 * Map a raw oidc_error code to a user-facing message.
 * Accepts both exact matches and prefix matches (e.g. "token_invalid_grant").
 * @param {string} code
 * @returns {string}
 */
function mapOidcError(code) {
  if (!code) return DEFAULT_OIDC_ERROR_MESSAGE;

  // Exact match first.
  if (code in OIDC_ERROR_MESSAGES) {
    return OIDC_ERROR_MESSAGES[code];
  }

  // Prefix match for compound codes like "token_invalid_grant",
  // "id_token_expired", etc.
  for (const [prefix, message] of Object.entries(OIDC_ERROR_MESSAGES)) {
    if (code.startsWith(prefix + '_') || code.startsWith(prefix)) {
      return message;
    }
  }

  return DEFAULT_OIDC_ERROR_MESSAGE;
}

// ---------------------------------------------------------------------------
// processOidcReturn
// ---------------------------------------------------------------------------

/**
 * Process an OIDC return-from-IdP URL.
 *
 * @param {Object} loginManager - The LoginManager instance. Must expose:
 *   - `showError(message)` — display an error in the login form.
 *   - `hide()` — hide the login UI (async, awaited on success).
 * @param {Object} [deps] - Dependency injection for tests:
 *   - `deps.exchangeHandoff(code)` — override handoff exchange
 *   - `deps.storeJWT(token)`       — override JWT storage
 *   - `deps.eventBus`              — override event bus
 *   - `deps.window`                — override window (for URL / history)
 *   - `deps.lithiumSettings`       — override window.lithiumSettings
 * @returns {Promise<boolean>} true if OIDC params were present, false otherwise.
 */
async function processOidcReturn(loginManager, deps = {}) {
  const win = deps.window ?? (typeof window !== 'undefined' ? window : null);

  if (!win) return false;

  const params = new URLSearchParams(win.location.search);

  // Fast exit — no OIDC involvement.
  if (params.get('oidc') !== '1') return false;

  const handoff   = params.get('handoff');
  const oidcError = params.get('oidc_error');
  const returnTo  = params.get('return_to');

  // Wipe OIDC params from the URL immediately — before any async work.
  // Keep return_to if it is present so navigation after login works.
  const cleanUrl = returnTo ? win.location.pathname + returnTo
                            : win.location.pathname;
  win.history.replaceState({}, '', cleanUrl);

  // --- Error path: IdP or callback reported an error ---
  if (oidcError) {
    log(AUTH, Status.WARN, `OIDC return: error code "${oidcError}"`);
    loginManager.showError(mapOidcError(oidcError));
    return true;
  }

  // --- Missing handoff code after no error flag ---
  if (!handoff) {
    log(AUTH, Status.WARN, 'OIDC return: missing handoff code');
    loginManager.showError(DEFAULT_OIDC_ERROR_MESSAGE);
    return true;
  }

  // --- Success path: exchange the handoff code ---
  const exchangeFn  = deps.exchangeHandoff ?? exchangeHandoff;
  const storeJWTFn  = deps.storeJWT       ?? storeJWT;
  const bus         = deps.eventBus       ?? eventBus;
  const settings    = deps.lithiumSettings ??
    (typeof window !== 'undefined' ? window.lithiumSettings : null);

  try {
    const data = await exchangeFn(handoff);

    storeJWTFn(data.token);

    // Record the last-used login method. The provider ID was written to
    // settings in the click handler (pre-navigation) as 'oidc:<id>'.
    // Writing 'oidc' here confirms the full exchange completed — it also
    // covers edge cases where the pre-navigation write was skipped.
    settings?.set('auth.last_method', 'oidc');

    log(AUTH, Status.SUCCESS,
      `OIDC login successful: user_id=${data.user_id}, username="${data.username ?? ''}"`);

    await loginManager.hide();

    bus.emit(Events.AUTH_LOGIN, {
      userId:    data.user_id,
      username:  data.username  ?? '',
      roles:     data.roles     ?? [],
      expiresAt: data.expires_at,
    });
  } catch (err) {
    const code = err.code ?? 'server_error';
    log(AUTH, Status.ERROR,
      `OIDC handoff exchange failed (${code}): ${err.message}`);
    loginManager.showError(mapOidcError(code));
  }

  return true;
}

// Convenience alias for this module's subsystem
const LOGIN = Subsystems.LOGIN;

/**
 * Login Manager Class
 */
class LoginManager {
  constructor(app, container) {
    this.app = app;
    this.container = container;
    this.elements = {};
    this.lookupListeners = [];
    this._startupComplete = false; // Tracks whether background startup has finished
    this._loginPanels = null; // LoginPanels instance (initialized in render)
    this._passwordManager = null; // PasswordManager instance (initialized in render)
    this._languagePanel = null; // LanguagePanel instance (initialized in render)
    this._logsPanel = null; // LogsPanel instance (initialized in render)
    this._helpPanel = null; // HelpPanel instance (initialized in render)
    this._loginForm = null; // LoginForm instance (initialized in render)
    this._shortcuts = null; // LoginShortcuts instance (initialized in render)
  }

  /**
   * Initialize the login manager
   */
  async init() {
    await this.render();
    this.setupEventListeners();
    this.setupLookupListeners();
    this.initializeButtonStates();
    this._helpPanel?.init();

    // Handle OIDC return-from-IdP before showing the login form.
    // If the URL contains ?oidc=1, processOidcReturn handles the entire
    // login completion (exchange handoff → store JWT → hide → AUTH_LOGIN)
    // or shows an error message, and returns true to signal we're done.
    // In the success case the manager hides itself; in the error case
    // the caller still needs to show the login form for a retry.
    const wasOidcReturn = await processOidcReturn(this);
    if (wasOidcReturn) {
      // On a successful OIDC return, hide() was called inside
      // processOidcReturn and AUTH_LOGIN was emitted — nothing more to do.
      // On an error return, showError() was called and the form needs to be
      // shown so the user can retry; fall through to the show() call below.
      // We check whether the JWT was stored to decide which case we are in.
      const { retrieveJWT } = await __vitePreload(async () => { const { retrieveJWT } = await import('./index-CEuz7QhE.js').then(n => n.F);return { retrieveJWT }},true              ?__vite__mapDeps([2,3]):void 0,import.meta.url);
      if (retrieveJWT()) return;
    }

    // Load remembered username before showing (sets initial values)
    const hadRememberedUsername = this._loginForm?.loadRememberedUsername() || false;

    // Show the panel with fade-in (pass focus info to avoid double focus)
    await this.show(hadRememberedUsername);

    // Check for URL query parameters for auto-login
    this.checkForAutoLogin();
  }

  /**
   * Initialize button states based on current lookups availability
   * Buttons start disabled and are enabled when data arrives
   */
  initializeButtonStates() {
    // Start with buttons disabled
    this.setLanguageButtonEnabled(false);
    this.setThemeButtonEnabled(false);
    // Logs button starts disabled; enabled once background startup completes
    this.setLogsButtonEnabled(false);

    // If lookups are already loaded, enable buttons immediately
    if (hasLookup('lookup_names')) {
      this.setLanguageButtonEnabled(true);
    }
    if (hasLookup('themes')) {
      this.setThemeButtonEnabled(true);
    }
  }

  /**
   * Set up event listeners for lookups loaded events
   */
  setupLookupListeners() {
    // Listen for lookup_names lookup - enable language button when translations are available
    const handleLookupNamesLoaded = () => {
      this.setLanguageButtonEnabled(true);
    };

    // Listen for themes lookup - enable theme button when themes are loaded
    const handleThemesLoaded = () => {
      this.setThemeButtonEnabled(true);
    };

    // Listen for startup complete - enable logs button once background init is done
    const handleStartupComplete = () => {
      this._startupComplete = true;
      this.setLogsButtonEnabled(true);
    };

    // Subscribe to specific lookup events
    eventBus.on(Events.LOOKUPS_LOOKUP_NAMES_LOADED, handleLookupNamesLoaded);
    eventBus.on(Events.LOOKUPS_THEMES_LOADED, handleThemesLoaded);
    eventBus.once(Events.STARTUP_COMPLETE, handleStartupComplete);

    // Store unsubscribe functions for cleanup
    this.lookupListeners.push(() => eventBus.off(Events.LOOKUPS_LOOKUP_NAMES_LOADED, handleLookupNamesLoaded));
    this.lookupListeners.push(() => eventBus.off(Events.LOOKUPS_THEMES_LOADED, handleThemesLoaded));
    this.lookupListeners.push(() => eventBus.off(Events.STARTUP_COMPLETE, handleStartupComplete));
  }

  /**
   * Enable or disable the theme button
   * @param {boolean} enabled - Whether to enable the button
   */
  setThemeButtonEnabled(enabled) {
    if (this.elements.themeBtn) {
      this.elements.themeBtn.disabled = !enabled;
    }
  }

  /**
   * Enable or disable the logs button
   * @param {boolean} enabled - Whether to enable the button
   */
  setLogsButtonEnabled(enabled) {
    if (this.elements.logsBtn) {
      this.elements.logsBtn.disabled = !enabled;
    }
  }

  /**
   * Enable or disable the language button
   * @param {boolean} enabled - Whether to enable the button
   */
  setLanguageButtonEnabled(enabled) {
    if (this.elements.languageBtn) {
      this.elements.languageBtn.disabled = !enabled;
    }
  }

  /**
   * Check for USER and PASS URL query parameters and auto-login if present
   */
  checkForAutoLogin() {
    const urlParams = new URLSearchParams(window.location.search);
    const username = urlParams.get('USER');
    const password = urlParams.get('PASS');

    if (username && password && this._loginForm) {
      log(LOGIN, Status.INFO, `Auto-login: credentials detected in URL for user "${username}"`);
      this._loginForm.prefillAndSubmit(username, password);
    }
  }

  /**
   * Render the login page
   */
  async render() {
    // Load HTML template
    try {
      const response = await fetch('/src/managers/login/login.html');
      const html = await response.text();
      this.container.innerHTML = html;
    } catch (error) {
      console.error('[LoginManager] Failed to load template:', error);
      this.renderFallback();
    }

    // Cache DOM elements
    this.elements = {
      page: this.container.querySelector('#login-page'),
      overlay: this.container.querySelector('#transition-overlay'),
      loginPanel: this.container.querySelector('#login-panel'),
      themePanel: this.container.querySelector('#theme-panel'),
      logsPanel: this.container.querySelector('#logs-panel'),
      logViewer: this.container.querySelector('#log-viewer'),
      languagePanel: this.container.querySelector('#language-panel'),
      helpPanel: this.container.querySelector('#help-panel'),
      form: this.container.querySelector('#login-form'),
      username: this.container.querySelector('#login-username'),
      password: this.container.querySelector('#login-password'),
      submit: this.container.querySelector('#login-submit'),
      languageBtn: this.container.querySelector('#login-language-btn'),
      themeBtn: this.container.querySelector('#login-theme-btn'),
      logsBtn: this.container.querySelector('#login-logs-btn'),
      helpBtn: this.container.querySelector('#login-help-btn'),
      clearUsernameBtn: this.container.querySelector('#login-clear-username'),
      togglePasswordBtn: this.container.querySelector('#login-toggle-password'),
      passwordIcon: this.container.querySelector('#login-toggle-password i'),
      themeCloseBtn: this.container.querySelector('#theme-close-btn'),
      logsCloseBtn: this.container.querySelector('#logs-close-btn'),
      languageCloseBtn: this.container.querySelector('#language-close-btn'),
      helpCloseBtn: this.container.querySelector('#help-close-btn'),
      error: this.container.querySelector('#login-error'),
      errorText: this.container.querySelector('#login-error-text'),
      versionBox: this.container.querySelector('#login-version-btn'),
      versionBuild: this.container.querySelector('#login-version-build'),
      versionYear: this.container.querySelector('#login-version-year'),
      versionDate: this.container.querySelector('#login-version-date'),
      versionTime: this.container.querySelector('#login-version-time'),
      helpAppVersion: this.container.querySelector('#help-app-version'),
      helpBuildDate: this.container.querySelector('#help-build-date'),
      languageFilter: this.container.querySelector('#language-filter'),
      languageClearFilter: this.container.querySelector('#language-clear-filter'),
      languageTable: this.container.querySelector('#language-table'),
      languageCurrentBtn: this.container.querySelector('#language-current-btn'),
      languageCurrentFlag: this.container.querySelector('#language-current-flag'),
      languageCurrentCode: this.container.querySelector('#language-current-code'),
      oidcDivider: this.container.querySelector('#login-oidc-divider'),
      oidcProviders: this.container.querySelector('#login-providers'),
    };

    // Initialize PasswordManager
    this._passwordManager = new PasswordManager();

    // Initialize LanguagePanel
    this._languagePanel = new LanguagePanel({
      elements: {
        languageTable: this.elements.languageTable,
        languageFilter: this.elements.languageFilter,
        languageClearFilter: this.elements.languageClearFilter,
        languageCurrentBtn: this.elements.languageCurrentBtn,
        languageCurrentFlag: this.elements.languageCurrentFlag,
        languageCurrentCode: this.elements.languageCurrentCode,
      },
      onLocaleSelected: (locale, previousLocale) => {
        // Locale selection is handled entirely within LanguagePanel;
        // this callback exists for future extension if LoginManager
        // needs to react to locale changes.
      },
    });

    // Initialize LogsPanel
    this._logsPanel = new LogsPanel({
      elements: { logViewer: this.elements.logViewer },
    });

    // Initialize HelpPanel (loads and renders version info on init)
    this._helpPanel = new HelpPanel({
      elements: {
        versionBuild: this.elements.versionBuild,
        versionYear: this.elements.versionYear,
        versionDate: this.elements.versionDate,
        versionTime: this.elements.versionTime,
        helpAppVersion: this.elements.helpAppVersion,
        helpBuildDate: this.elements.helpBuildDate,
      },
    });

    // Initialize LoginForm (owns submit, attemptLogin, error mapping,
    // password visibility, remembered username, etc.)
    this._loginForm = new LoginForm({
      elements: {
        form: this.elements.form,
        username: this.elements.username,
        password: this.elements.password,
        submit: this.elements.submit,
        error: this.elements.error,
        errorText: this.elements.errorText,
        clearUsernameBtn: this.elements.clearUsernameBtn,
        togglePasswordBtn: this.elements.togglePasswordBtn,
        passwordIcon: this.elements.passwordIcon,
        languageBtn: this.elements.languageBtn,
        themeBtn: this.elements.themeBtn,
        logsBtn: this.elements.logsBtn,
        helpBtn: this.elements.helpBtn,
      },
      onLoginSuccess: async () => {
        await this.hide();
      },
      isStartupComplete: () => this._startupComplete,
    });

    // Initialize LoginPanels with callbacks for panel-specific logic
    this._loginPanels = new LoginPanels({
      panels: {
        login: this.elements.loginPanel,
        theme: this.elements.themePanel,
        logs: this.elements.logsPanel,
        language: this.elements.languagePanel,
        help: this.elements.helpPanel,
      },
      loginPanel: this.elements.loginPanel,
      overlay: this.elements.overlay,
      onBeforeSwitch: (targetPanel) => this._onBeforePanelSwitch(targetPanel),
      onAfterSwitch: (fromPanel, toPanel, duration) => this._onAfterPanelSwitch(fromPanel, toPanel, duration),
    });

    // Initialize LoginShortcuts (keyboard handlers + CAPS LOCK indicator).
    this._shortcuts = new LoginShortcuts({
      elements: {
        username: this.elements.username,
        password: this.elements.password,
        helpBtn: this.elements.helpBtn,
        languageBtn: this.elements.languageBtn,
        themeBtn: this.elements.themeBtn,
        logsBtn: this.elements.logsBtn,
      },
      getCurrentPanel: () => this._loginPanels?.currentPanel,
      onClearUsername: () => this._loginForm?.handleClearUsername(),
      onSwitchToLogin: () => this.switchPanel('login'),
    });

    // Render OIDC provider buttons from config (no-op if array is empty).
    this.renderOidcProviders();

    // Subtly highlight the last-used login method (Phase 26).
    this.applyPasswordRecentHighlight();
  }

  /**
   * Render a fallback login form if template fails to load
   */
  renderFallback() {
    this.container.innerHTML = `
      <div id="login-page">
        <div class="login-container" id="login-panel">
          <div class="login-header">
            <h1>Lithium</h1>
            <p>A Philement Project</p>
          </div>
          <form class="login-form" id="login-form" novalidate>
            <div class="form-group">
              <div class="input-with-icon">
                <input type="text" id="login-username" placeholder="Username" required>
                <button type="button" class="input-icon-btn" id="login-clear-username" data-tooltip="Clear" data-tip-placement="right">
                  <fa fa-xmark></fa>
                </button>
              </div>
            </div>
            <div class="form-group">
              <div class="input-with-icon">
                <input type="password" id="login-password" placeholder="Password" required>
                <button type="button" class="input-icon-btn" id="login-toggle-password" data-tooltip="Show password" data-tip-placement="right">
                  <fa fa-eye></fa>
                </button>
              </div>
            </div>
            <div class="login-error" id="login-error">
              <span id="login-error-text">Error</span>
            </div>
            <div class="login-btn-group">
              <button type="button" class="login-btn-icon" id="login-language-btn" data-tooltip="Select language" data-tip-placement="left">
                <fa fa-earth-americas></fa>
              </button>
              <button type="button" class="login-btn-icon" id="login-theme-btn" data-tooltip="Select theme" data-tip-placement="left">
                <fa fa-palette></fa>
              </button>
              <button type="submit" class="login-btn-primary" id="login-submit" data-tooltip="Login" data-tip-placement="left">
                <fa fa-sign-in-alt></fa>
                <span>Login</span>
              </button>
              <button type="button" class="login-btn-icon" id="login-logs-btn" data-tooltip="View Session Log" data-tip-placement="left">
                <fa fa-receipt></fa>
              </button>
              <button type="button" class="login-btn-icon" id="login-help-btn" data-tooltip="Help" data-tip-placement="left">
                <fa circle-question></fa>
              </button>
            </div>
          </form>
        </div>
      </div>
    `;
  }

  /**
   * Render zero-or-more OIDC provider buttons beneath the login form.
   *
   * Reads `auth.oidc_providers` from the Lithium config. If the array is
   * absent or empty, neither the divider nor the providers container is shown.
   * Per LITHIUM-INS.md rule #1 (no fallback path), there is no greyed-out
   * placeholder — the UI simply does not render when OIDC is disabled.
   *
   * Each button:
   *   - Has class `login-btn-oidc` and `data-provider="<id>"`.
   *   - Displays a Font Awesome icon (from `provider.icon`) followed by the
   *     `provider.label` text.
   *   - On click: records `auth.last_method` as `'oidc:<id>'` in
   *     `lithiumSettings` (before navigation away), then calls
   *     `startOidc(provider.id, window.location.pathname)`.
   *
   * On render, if `auth.last_method` in `lithiumSettings` starts with
   * `'oidc:'` and matches this provider's id, the `.is-recent` class is
   * added to the button as a subtle highlight (Phase 26).
   *
   * @param {Object} [deps] - Dependency injection for tests.
   * @param {Function} [deps.getConfigValue]   - Config accessor override.
   * @param {Function} [deps.startOidc]        - startOidc override.
   * @param {Object}   [deps.window]           - Window override.
   * @param {Object}   [deps.lithiumSettings]  - lithiumSettings override.
   */
  renderOidcProviders(deps = {}) {
    const gcv             = deps.getConfigValue ?? getConfigValue;
    const startOidcFn     = deps.startOidc      ?? startOidc;
    const win             = deps.window         ?? (typeof window !== 'undefined' ? window : null);
    const settings        = deps.lithiumSettings ??
      (typeof window !== 'undefined' ? window.lithiumSettings : null);

    const providers = gcv('auth.oidc_providers', []);

    // Nothing to render — leave divider and container hidden.
    if (!Array.isArray(providers) || providers.length === 0) return;

    const container = this.elements.oidcProviders;
    const divider   = this.elements.oidcDivider;

    if (!container || !divider) return;

    // Read the stored last-used login method for the `.is-recent` highlight.
    const lastMethod = settings?.get('auth.last_method') ?? null;

    // Build one button per valid provider.
    let rendered = 0;
    providers.forEach((provider) => {
      if (!provider || !provider.id || !provider.label) return;

      const btn = document.createElement('button');
      btn.type = 'button';
      btn.className = 'login-btn-oidc';
      btn.setAttribute('data-provider', provider.id);
      btn.setAttribute('aria-label', provider.label);

      // Icon (Font Awesome class, e.g. "fa-key").
      if (provider.icon) {
        const iconEl = document.createElement('span');
        iconEl.className = `login-btn-oidc__icon fa-duotone fa-thin ${provider.icon}`;
        iconEl.setAttribute('aria-hidden', 'true');
        btn.appendChild(iconEl);
      }

      // Label text.
      const labelEl = document.createElement('span');
      labelEl.textContent = provider.label;
      btn.appendChild(labelEl);

      // Subtle highlight if this provider was the last login method used.
      // Matches 'oidc:<id>' (written pre-navigation) or plain 'oidc'
      // (written on successful handoff exchange).
      if (lastMethod === `oidc:${provider.id}` || lastMethod === 'oidc') {
        btn.classList.add('is-recent');
      }

      // Click handler: full-page navigation to /oidc/start.
      btn.addEventListener('click', () => {
        log(LOGIN, Status.INFO, `OIDC provider button clicked: "${provider.id}"`);
        try {
          // Record the selected provider *before* navigating away so it
          // survives the page reload and can be used for the highlight on
          // the next render (Phase 26).
          settings?.set('auth.last_method', `oidc:${provider.id}`);

          const returnTo = win?.location?.pathname ?? null;
          startOidcFn(provider.id, returnTo, { getConfigValue: gcv, window: win });
        } catch (err) {
          log(LOGIN, Status.ERROR, `OIDC startOidc failed: ${err.message}`);
          this.showError('Sign-in could not start. Please try again.');
        }
      });

      container.appendChild(btn);
      rendered++;
    });

    // Show the divider and the container only if at least one button was rendered.
    if (rendered === 0) return;
    divider.style.display = '';
    container.style.display = '';

    log(LOGIN, Status.INFO, `OIDC: rendered ${rendered} provider button(s)`);
  }

  /**
   * Apply the `.is-recent` highlight to the password submit button when
   * `auth.last_method === 'password'` in `lithiumSettings` (Phase 26).
   *
   * Called from `render()` after all sub-modules are initialised.
   *
   * @param {Object} [deps] - Dependency injection for tests.
   * @param {Object} [deps.lithiumSettings] - lithiumSettings override.
   */
  applyPasswordRecentHighlight(deps = {}) {
    const settings = deps.lithiumSettings ??
      (typeof window !== 'undefined' ? window.lithiumSettings : null);
    const lastMethod = settings?.get('auth.last_method') ?? null;
    if (lastMethod === 'password' && this.elements.submit) {
      this.elements.submit.classList.add('is-recent');
    }
  }

  /**
   * Show an error banner in the login form.
   * Delegates to LoginForm; safe to call even before render() completes
   * (LoginForm may not yet be initialised).
   * @param {string} message
   */
  showError(message) {
    this._loginForm?.showError(message);
  }

  /**
   * Set up event listeners. Form-related listeners (submit, input, blur,
   * clear, password-toggle) are owned by LoginForm; this method wires the
   * panel-switching buttons and the language filter UI only.
   */
  setupEventListeners() {
    // Bind form-related listeners via LoginForm.
    this._loginForm?.init();

    // Panel-switch buttons.
    const panelButton = (btn, name) => {
      this.elements[btn]?.addEventListener('click', () => {
        log(LOGIN, Status.INFO, `Button clicked: ${name}`);
        this.switchPanel(name.toLowerCase());
      });
    };
    panelButton('languageBtn', 'Language');
    panelButton('themeBtn', 'Theme');
    panelButton('logsBtn', 'Logs');
    panelButton('helpBtn', 'Help');

    // Language filter input (delegated to LanguagePanel).
    this.elements.languageFilter?.addEventListener('input', (e) => {
      this._languagePanel?.filter(e.target.value);
    });
    this.elements.languageClearFilter?.addEventListener('click', () => {
      if (this.elements.languageFilter) {
        this.elements.languageFilter.value = '';
        this._languagePanel?.filter('');
        this.elements.languageFilter.focus();
      }
    });

    // Close buttons on each subpanel.
    const closeButton = (btn, name) => {
      this.elements[btn]?.addEventListener('click', () => {
        log(LOGIN, Status.INFO, `Button clicked: ${name} Close`);
        this.switchPanel('login');
      });
    };
    closeButton('themeCloseBtn', 'Theme');
    closeButton('logsCloseBtn', 'Logs');
    closeButton('languageCloseBtn', 'Language');
    closeButton('helpCloseBtn', 'Help');

    // Attach keyboard shortcuts (LoginShortcuts handles ESC, Ctrl+Shift+*,
    // F1, and CAPS-LOCK indicator).
    this._shortcuts?.attach();
  }

  /**
   * Switch between panels with fade transition
   * Delegates to LoginPanels; keeps panel-specific logic here.
   * @param {string} targetPanel - 'login', 'theme', 'logs', 'language', 'help'
   */
  async switchPanel(targetPanel) {
    if (!this._loginPanels) return;
    await this._loginPanels.switchPanel(targetPanel);
  }

  /**
   * Called by LoginPanels before performing a panel transition.
   * Handles panel-specific prep: password manager visibility, logs population,
   * language panel init/cleanup.
   * @param {string} targetPanel
   * @private
   */
  _onBeforePanelSwitch(targetPanel) {
    // Hide password manager UI when leaving login panel, show when returning
    if (targetPanel === 'login') {
      this._passwordManager?.show();
    } else {
      this._passwordManager?.hide();
    }

    // If switching to logs panel, populate it with action logs
    if (targetPanel === 'logs') {
      this._logsPanel?.show();
    }

    // If switching to language panel, show it; otherwise hide it
    if (targetPanel === 'language') {
      this._languagePanel?.show();
    } else {
      this._languagePanel?.hide();
    }
  }

  /**
   * Called by LoginPanels after a panel transition completes.
   * Handles logging and focus management.
   * @param {string} fromPanel
   * @param {string} toPanel
   * @private
   */
  _onAfterPanelSwitch(fromPanel, toPanel) {
    log(LOGIN, Status.INFO, `Panel: ${fromPanel} → ${toPanel}`);

    // Focus management when returning to login panel
    if (toPanel === 'login') {
      this._loginPanels.scheduleFocus(() => {
        if (this._loginPanels.currentPanel === 'login') {
          this.elements.username?.focus();
        }
      });
    }
  }

  /**
   * Show the login page with fade-in
   * @param {boolean} skipUsernameFocus - If true, focus password (username has value)
   */
  async show(skipUsernameFocus = false) {
    // Enable password manager UI when showing login
    this._passwordManager?.show();
    
    if (this.elements.loginPanel) {
      // Step 1: Set display to block (but opacity is still 0 from CSS)
      this.elements.loginPanel.style.display = 'block';

      // Step 2: Force reflow to ensure display is applied
      void this.elements.loginPanel.offsetHeight;

      // Step 3: Add visible class to trigger opacity transition from 0 to 1
      requestAnimationFrame(() => {
        this.elements.loginPanel.classList.add('visible');
      });
    }

    // Focus appropriate field after transition completes
    // Use a slightly longer delay to ensure transition is fully done and element is interactive
    setTimeout(() => {
      if (skipUsernameFocus && this.elements.password) {
        // Username was pre-filled, focus password
        this.elements.password.focus();
      } else if (this.elements.username) {
        // No username or empty, focus username
        this.elements.username.focus();
      }
    }, getTransitionDuration() + 50);
  }

  /**
   * Hide the login page with fade-out
   */
  async hide() {
    return new Promise((resolve) => {
      const duration = getTransitionDuration();
      
      // Hide password manager UI elements before fading out to main.
      // This adds the body class, starts the MutationObserver, and sweeps
      // any existing elements so they disappear immediately.
      this._passwordManager?.hide();

      // Physically remove all injected password manager elements from the DOM.
      // This ensures they do not linger in the post-login view as invisible orphan
      // nodes (e.g. 1Password's live-region divs) and that 1Password cannot simply
      // re-show them by attribute mutation without re-inserting them first.
      this._passwordManager?.removeAll();
      
      // Fade out the current panel only
      // Let the app handle the transition to main manager
      const currentPanelEl = this._loginPanels?._panels[this._loginPanels?.currentPanel];
      if (currentPanelEl) {
        currentPanelEl.style.transition = `opacity ${duration}ms ease-in-out`;
        currentPanelEl.style.opacity = '0';
      }
      
      setTimeout(resolve, duration);
    });
  }

  /**
   * Teardown the login manager
   */
  teardown() {
    // Teardown LoginForm (removes form listeners, clears state)
    this._loginForm?.destroy();
    this._loginForm = null;

    // Teardown LogsPanel (cleans up CodeMirror editor + OverlayScrollbars)
    this._logsPanel?.destroy();
    this._logsPanel = null;

    // Teardown HelpPanel
    this._helpPanel?.destroy();
    this._helpPanel = null;

    // Teardown LanguagePanel
    this._languagePanel?.destroy();
    this._languagePanel = null;

    // Cancel any pending post-login-return focus timer
    this._loginPanels?.cancelPendingFocus();

    // Detach keyboard shortcut event listeners
    this._shortcuts?.detach();
    this._shortcuts = null;

    // Clean up password manager UI observer and remove any lingering injected elements
    this._passwordManager?.destroy();
    this._passwordManager = null;

    // Remove lookup event listeners
    this.lookupListeners.forEach(unsubscribe => unsubscribe());
    this.lookupListeners = [];

    // Teardown LoginPanels
    this._loginPanels?.teardown();
    this._loginPanels = null;

    // Clear references
    this.elements = {};
  }
}

export { LoginManager as default };
