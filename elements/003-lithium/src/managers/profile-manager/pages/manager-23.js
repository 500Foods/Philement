/**
 * Profile Manager - Lookups Manager Settings Page
 *
 * Settings for the Lookups Manager (manager ID: 23)
 * Allows users to configure lookup display preferences,
 * default views, and caching options.
 */

import { SimpleSettingsPage } from './settings-page-base.js';
import { log, Subsystems, Status } from '../../../core/log.js';

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
   * Load settings from localStorage
   */
  async loadData() {
    const stored = localStorage.getItem('lithium_lookups_settings');
    let data = { ...this._defaultValues };

    if (stored) {
      try {
        const parsed = JSON.parse(stored);
        data = { ...this._defaultValues, ...parsed };
      } catch (error) {
        log(Subsystems.MANAGER, Status.WARN, '[LookupsManagerPage] Failed to parse settings:', error);
      }
    }

    this.setFormData(data);
    this._originalData = JSON.parse(JSON.stringify(data));
    this.setDirty(false);
  }

  /**
   * Save settings to localStorage
   */
  async save() {
    const data = this.getFormData(this.formSelector);

    // Save to localStorage
    localStorage.setItem('lithium_lookups_settings', JSON.stringify(data));

    this._originalData = JSON.parse(JSON.stringify(data));
    this.setDirty(false);

    log(Subsystems.MANAGER, Status.INFO, '[LookupsManagerPage] Settings saved:', data);
    return { success: true, data };
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
