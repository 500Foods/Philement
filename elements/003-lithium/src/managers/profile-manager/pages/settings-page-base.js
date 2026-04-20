/**
 * Profile Manager - Settings Page Base Class
 *
 * Base class for manager-specific settings pages. These pages are loaded
 * dynamically when a manager entry is selected in the user options table.
 */

import { log, Subsystems, Status } from '../../../core/log.js';

/**
 * BaseSettingsPage class
 * Extend this class to create manager-specific settings pages
 */
export class BaseSettingsPage {
  constructor(options = {}) {
    this.index = options.index || 0;
    this.managerId = options.managerId || 0;
    this.container = null;
    this._isDirty = false;
    this._originalData = null;
    this._onDirtyChange = options.onDirtyChange || (() => {});
  }

  /**
   * Initialize the page
   * @param {HTMLElement} container - The page container element
   * @param {Object} data - Page data from the user options table
   */
  async init(container, data) {
    this.container = container;
    this._originalData = JSON.parse(JSON.stringify(data));

    // Override in subclasses to perform setup
    await this.onInit();

    log(Subsystems.MANAGER, Status.DEBUG, `[SettingsPage] Initialized page ${this.index}`);
  }

  /**
   * Called when the page is initialized
   * Override in subclasses
   */
  async onInit() {
    // Override in subclass
  }

  /**
   * Called when the page becomes visible
   * Override in subclasses
   */
  onShow() {
    // Override in subclass
  }

  /**
   * Called when the page is hidden
   * Override in subclasses
   */
  onHide() {
    // Override in subclass
  }

  /**
   * Check if the page has unsaved changes
   * @returns {boolean}
   */
  isDirty() {
    return this._isDirty;
  }

  /**
   * Mark the page as dirty/clean
   * @param {boolean} dirty
   */
  setDirty(dirty) {
    this._isDirty = dirty;
    this._onDirtyChange(dirty);
  }

  /**
   * Save the page's data
   * Override in subclasses
   * @returns {Promise<Object>}
   */
  async save() {
    // Override in subclass
    this.setDirty(false);
    return { success: true };
  }

  /**
   * Cancel/reset the page's changes
   * Override in subclasses
   */
  cancel() {
    // Override in subclass
    this.setDirty(false);
  }

  /**
   * Refresh the page with current data
   * Override in subclasses
   */
  refresh() {
    // Override in subclass
  }

  /**
   * Get form data from the page
   * @param {string} formSelector - CSS selector for the form
   * @returns {Object}
   */
  getFormData(formSelector = 'form') {
    if (!this.container) return {};

    const form = this.container.querySelector(formSelector);
    if (!form) return {};

    const data = {};
    const elements = form.querySelectorAll('input, select, textarea');

    elements.forEach((el) => {
      const name = el.name;
      if (!name) return;

      if (el.type === 'checkbox') {
        data[name] = el.checked;
      } else if (el.type === 'number') {
        data[name] = parseFloat(el.value) || 0;
      } else {
        data[name] = el.value;
      }
    });

    return data;
  }

  /**
   * Set form data on the page
   * @param {Object} data - Data to set
   * @param {string} formSelector - CSS selector for the form
   */
  setFormData(data, formSelector = 'form') {
    if (!this.container) return;

    const form = this.container.querySelector(formSelector);
    if (!form) return;

    Object.entries(data).forEach(([name, value]) => {
      const el = form.querySelector(`[name="${name}"]`);
      if (!el) return;

      if (el.type === 'checkbox') {
        el.checked = !!value;
      } else {
        el.value = value ?? '';
      }
    });
  }

  /**
   * Setup change listeners for form elements
   * @param {string} formSelector - CSS selector for the form
   */
  setupChangeListeners(formSelector = 'form') {
    if (!this.container) return;

    const form = this.container.querySelector(formSelector);
    if (!form) return;

    const elements = form.querySelectorAll('input, select, textarea');
    elements.forEach((el) => {
      el.addEventListener('change', () => this.setDirty(true));
    });
  }

  /**
   * Destroy the page and clean up
   * Override in subclasses
   */
  destroy() {
    this.container = null;
    this._originalData = null;
  }
}

/**
 * SimpleSettingsPage class
 * A simple implementation that handles basic form binding
 */
export class SimpleSettingsPage extends BaseSettingsPage {
  constructor(options = {}) {
    super(options);
    this.formSelector = options.formSelector || 'form';
  }

  /**
   * Called when the page is initialized
   */
  async onInit() {
    this.setupChangeListeners(this.formSelector);
    await this.loadData();
  }

  /**
   * Load data into the form
   * Override in subclasses to fetch data from API
   */
  async loadData() {
    // Override in subclass to load from API
    // Then call setFormData with the loaded data
  }

  /**
   * Save the form data
   */
  async save() {
    const data = this.getFormData(this.formSelector);
    // Override in subclass to save to API
    this.setDirty(false);
    return { success: true, data };
  }

  /**
   * Cancel changes and reload data
   */
  cancel() {
    this.loadData();
    this.setDirty(false);
  }
}

export default BaseSettingsPage;
