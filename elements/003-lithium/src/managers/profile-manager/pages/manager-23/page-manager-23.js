/**
 * Profile Manager - Lookups Manager Settings Page
 *
 * Settings for the Lookups Manager (manager ID: 23)
 * Allows users to configure lookup display preferences,
 * default views, and caching options.
 *
 * Uses the ProfileSettingsService for all persistence.
 * Section key: "23"
 * JSON structure:
 *   {
 *     "_name": "Lookups Manager",
 *     "defaultView": "list",
 *     "pageSize": 50,
 *     "autoRefresh": true,
 *     "refreshInterval": 60,
 *     "showInactive": false,
 *     "cacheLookups": true
 *   }
 */

import { SimpleSettingsPage } from '../settings-page-base.js';
import { log, Subsystems, Status } from '../../../../core/log.js';

/**
 * Lookups Manager Settings Page
 */
export class LookupsManagerPage extends SimpleSettingsPage {
  constructor(options = {}) {
    super({
      ...options,
      index: 23,
      managerId: 23,
      formSelector: 'form',
    });

    this._defaultValues = {
      defaultView: 'list',
      pageSize: 50,
      autoRefresh: true,
      refreshInterval: 60,
      showInactive: false,
      cacheLookups: true,
    };
  }

  /**
   * Called when the page is initialized
   */
  async onInit() {
    await super.onInit();
    await this.loadData();
    log(Subsystems.MANAGER, Status.DEBUG, '[LookupsManagerPage] Initialized');
  }

  /**
   * Load settings from the Settings Service.
   * Falls back to legacy localStorage for migration.
   */
  async loadData() {
    const section = this.getSectionData();
    const hasData = Object.keys(section).length > 1; // >1 because _name may be present

    let data;
    if (hasData) {
      // Load from Settings Service
      data = { ...this._defaultValues };
      for (const key of Object.keys(this._defaultValues)) {
        if (section[key] !== undefined) {
          data[key] = section[key];
        }
      }
    } else {
      // Fallback: migrate from legacy localStorage
      const stored = localStorage.getItem('lithium_lookups_settings');
      data = { ...this._defaultValues };
      if (stored) {
        try {
          const parsed = JSON.parse(stored);
          data = { ...this._defaultValues, ...parsed };
        } catch (error) {
          log(Subsystems.MANAGER, Status.WARN, '[LookupsManagerPage] Failed to parse legacy settings:', error);
        }
      }
    }

    this.setFormData(data);
    this._originalData = JSON.parse(JSON.stringify(data));
    this.setDirty(false);
  }

  /**
   * Save settings via the Settings Service.
   */
  async save() {
    const data = this.getFormData(this.formSelector);

    // Write to Settings Service under section "23"
    this.setSectionData(data, 'Lookups Manager');

    // Also update legacy localStorage for backward compatibility
    this._updateLegacyStorage(data);

    this._originalData = JSON.parse(JSON.stringify(data));
    this.setDirty(false);

    log(Subsystems.MANAGER, Status.INFO, '[LookupsManagerPage] Settings saved:', data);
    return { success: true, data };
  }

  /**
   * Update legacy localStorage for backward compatibility.
   * @private
   */
  _updateLegacyStorage(data) {
    try {
      localStorage.setItem('lithium_lookups_settings', JSON.stringify(data));
    } catch (_e) {
      // ignore
    }
  }

  /**
   * Cancel changes and reload
   */
  cancel() {
    this.loadData();
    log(Subsystems.MANAGER, Status.DEBUG, '[LookupsManagerPage] Changes cancelled');
  }

  /**
   * Check if page is dirty
   */
  isDirty() {
    if (!this.container) return false;

    const currentData = this.getFormData(this.formSelector);
    return JSON.stringify(currentData) !== JSON.stringify(this._originalData);
  }
}

export default LookupsManagerPage;
