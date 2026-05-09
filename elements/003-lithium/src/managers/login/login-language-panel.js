/**
 * Login Language Panel
 *
 * Manages the language selection panel with Tabulator table, locale detection,
 * and keyboard navigation. Extracted from login.js to keep files under 1,000
 * lines per LITHIUM-INS.md rule #2.
 */

import { log, Subsystems, Status } from '../../core/log.js';
import { getFlagSvg } from '../../shared/log-formatter.js';
import {
  getBestGuessLocale,
  getLanguageData,
  getCountryCode,
  saveLocalePreference,
  getSavedLocale,
  supportedLocales,
} from '../../shared/languages.js';
import { getConfigValue } from '../../core/config.js';
import { eventBus, Events } from '../../core/event-bus.js';
import '../../styles/vendor-tabulator.css';

// Convenience alias for this module's subsystem
const LOGIN = Subsystems.LOGIN;

/**
 * LanguagePanel class
 *
 * Owns the Tabulator language table, locale detection, filter handling,
 * keyboard navigation, and the current-locale indicator.
 * Delegates to LoginManager via callbacks for panel visibility changes.
 */
export class LanguagePanel {
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

      log(LOGIN, Status.INFO, `Language panel initialized. Best guess: ${this._bestGuessLocale}, Current: ${this._currentLocale}`);
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
    log(LOGIN, Status.SUCCESS, `Language selected: ${langName} (${locale})`);

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
    const TabulatorModule = await import('tabulator-tables');
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
    log(LOGIN, Status.ERROR, `Failed to initialize language table: ${error.message}`);
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
