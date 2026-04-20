/**
 * Profile Manager - Date Formats Page Handler
 *
 * Handles the Date Formats settings page (index: -9)
 * Manages date/time format selection with live preview
 */

import { SimpleSettingsPage } from './settings-page-base.js';
import { log, Subsystems, Status } from '../../../core/log.js';

/**
 * Date Formats Page Handler
 */
export class DateFormatsPage extends SimpleSettingsPage {
  constructor(options = {}) {
    super({
      ...options,
      index: -9,
      formSelector: 'form',
    });

    this._originalValues = {};
  }

  /**
   * Called when the page is initialized
   */
  async onInit() {
    await super.onInit();

    // Store original values
    this._saveOriginalValues();

    // Setup preview listeners
    this._setupPreviewListeners();

    log(Subsystems.MANAGER, Status.DEBUG, '[DateFormatsPage] Initialized');
  }

  /**
   * Save original form values for dirty detection
   */
  _saveOriginalValues() {
    const dateFormat = this.container?.querySelector('#pref-date-format')?.value;
    const timeFormat = this.container?.querySelector('#pref-time-format')?.value;

    this._originalValues = {
      dateFormat: dateFormat || 'MM/DD/YYYY',
      timeFormat: timeFormat || '12h',
    };
  }

  /**
   * Setup listeners to update preview on change
   */
  _setupPreviewListeners() {
    const dateSelect = this.container?.querySelector('#pref-date-format');
    const timeSelect = this.container?.querySelector('#pref-time-format');

    dateSelect?.addEventListener('change', () => this.updatePreview());
    timeSelect?.addEventListener('change', () => this.updatePreview());
  }

  /**
   * Load data into the form
   */
  async loadData() {
    const stored = localStorage.getItem('lithium_preferences');
    if (!stored) return;

    try {
      const prefs = JSON.parse(stored);
      this.setFormData({
        dateFormat: prefs.dateFormat || 'MM/DD/YYYY',
        timeFormat: prefs.timeFormat || '12h',
      });
      this._saveOriginalValues();
      this.updatePreview();
    } catch (error) {
      log(Subsystems.MANAGER, Status.WARN, '[DateFormatsPage] Failed to load preferences:', error);
    }
  }

  /**
   * Update the preview display
   */
  updatePreview() {
    // This page doesn't have a preview panel, but we could add one
    // For now, just log the current selection
    const dateFormat = this.container?.querySelector('#pref-date-format')?.value;
    const timeFormat = this.container?.querySelector('#pref-time-format')?.value;

    log(Subsystems.MANAGER, Status.DEBUG, `[DateFormatsPage] Preview: ${dateFormat}, ${timeFormat}`);
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

    // Update formats
    prefs.dateFormat = data.dateFormat;
    prefs.timeFormat = data.timeFormat;

    // Save to localStorage
    localStorage.setItem('lithium_preferences', JSON.stringify(prefs));

    this._originalValues = { ...data };
    this.setDirty(false);

    log(Subsystems.MANAGER, Status.INFO, '[DateFormatsPage] Saved formats:', data);
    return { success: true, data };
  }

  /**
   * Cancel changes
   */
  cancel() {
    this.loadData();
    this.setDirty(false);
    log(Subsystems.MANAGER, Status.DEBUG, '[DateFormatsPage] Cancelled changes');
  }

  /**
   * Check if the page is dirty
   */
  isDirty() {
    const currentDateFormat = this.container?.querySelector('#pref-date-format')?.value;
    const currentTimeFormat = this.container?.querySelector('#pref-time-format')?.value;

    return (
      currentDateFormat !== this._originalValues.dateFormat ||
      currentTimeFormat !== this._originalValues.timeFormat
    );
  }

  /**
   * Called when the page becomes visible
   */
  onShow() {
    this.updatePreview();
  }
}

export default DateFormatsPage;
