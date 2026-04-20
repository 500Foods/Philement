/**
 * Profile Manager - Language Page Handler
 *
 * Handles the Language settings page (index: -8)
 * Manages language selection with change detection
 */

import { SimpleSettingsPage } from './settings-page-base.js';
import { eventBus, Events } from '../../../core/event-bus.js';
import { log, Subsystems, Status } from '../../../core/log.js';

/**
 * Language Page Handler
 */
export class LanguagePage extends SimpleSettingsPage {
  constructor(options = {}) {
    super({
      ...options,
      index: -8,
      formSelector: '#preferences-form',
    });

    this._originalLanguage = null;
  }

  /**
   * Called when the page is initialized
   */
  async onInit() {
    await super.onInit();

    // Store original language for change detection
    const select = this.container?.querySelector('#pref-language');
    if (select) {
      this._originalLanguage = select.value;
    }

    log(Subsystems.MANAGER, Status.DEBUG, '[LanguagePage] Initialized');
  }

  /**
   * Load data into the form
   */
  async loadData() {
    // Load from localStorage or use defaults
    const stored = localStorage.getItem('lithium_preferences');
    if (stored) {
      try {
        const prefs = JSON.parse(stored);
        this.setFormData({ language: prefs.language || 'en-US' });
        this._originalLanguage = prefs.language || 'en-US';
      } catch (error) {
        log(Subsystems.MANAGER, Status.WARN, '[LanguagePage] Failed to load preferences:', error);
      }
    }
  }

  /**
   * Save the form data
   */
  async save() {
    const data = this.getFormData(this.formSelector);

    // Get current preferences
    let prefs = {};
    try {
      const stored = localStorage.getItem('lithium_preferences');
      if (stored) prefs = JSON.parse(stored);
    } catch (_e) {
      // Ignore
    }

    // Update language
    const previousLanguage = prefs.language;
    prefs.language = data.language;

    // Save to localStorage
    localStorage.setItem('lithium_preferences', JSON.stringify(prefs));

    // Emit locale changed event if language changed
    if (previousLanguage !== data.language) {
      eventBus.emit(Events.LOCALE_CHANGED, {
        lang: data.language,
      });
    }

    this._originalLanguage = data.language;
    this.setDirty(false);

    log(Subsystems.MANAGER, Status.INFO, '[LanguagePage] Saved language:', data.language);
    return { success: true, language: data.language };
  }

  /**
   * Cancel changes and reload
   */
  cancel() {
    this.loadData();
    this.setDirty(false);
    log(Subsystems.MANAGER, Status.DEBUG, '[LanguagePage] Cancelled changes');
  }

  /**
   * Check if the page is dirty
   */
  isDirty() {
    const select = this.container?.querySelector('#pref-language');
    if (!select) return false;
    return select.value !== this._originalLanguage;
  }
}

export default LanguagePage;
