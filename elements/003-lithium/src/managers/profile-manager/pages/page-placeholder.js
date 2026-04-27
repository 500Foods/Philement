/**
 * Profile Manager - Placeholder Page Handler
 *
 * Displays a "settings not implemented" message for managers
 * that don't have specific settings pages yet.
 */

import { BaseSettingsPage } from './settings-page-base.js';
import { log, Subsystems, Status } from '../../../core/log.js';

/**
 * Placeholder Page Handler
 * Used when a manager doesn't have specific settings implemented
 */
export class PlaceholderPage extends BaseSettingsPage {
  constructor(options = {}) {
    super(options);
    this._managerName = options.managerName || 'This Manager';
  }

  /**
   * Called when the page is initialized
   */
  async onInit() {
    this._renderPlaceholder();
    log(Subsystems.MANAGER, Status.DEBUG, `[PlaceholderPage] Initialized for ${this._managerName}`);
  }

  /**
   * Render the placeholder message
   */
  _renderPlaceholder() {
    if (!this.container) return;

    // Find or create the settings page element
    let pageElement = this.container.querySelector(`.settings-page[data-page-index="${this.index}"]`);
    if (!pageElement) {
      pageElement = document.createElement('div');
      pageElement.className = 'settings-page';
      pageElement.setAttribute('data-page-index', this.index);
      pageElement.id = `settings-page-${this.index}`;
      this.container.appendChild(pageElement);
    }

    // Find or create the section
    let section = pageElement.querySelector('.profile-section');
    if (!section) {
      section = document.createElement('section');
      section.className = 'profile-section';
      pageElement.appendChild(section);
    }

    // Find or create the placeholder content
    let placeholderEl = section.querySelector('.settings-placeholder');
    if (!placeholderEl) {
      section.innerHTML = `
        <h3 class="section-title"><fa fa-wrench></fa> ${this._managerName} Settings</h3>
        <div class="settings-placeholder">
          <div class="placeholder-icon">
            <fa fa-wrench></fa>
          </div>
          <h4 class="placeholder-title">Settings Not Available</h4>
          <p class="placeholder-message">
            No settings are currently available for ${this._managerName}.
          </p>
          <p class="placeholder-hint">
            This feature will be implemented in a future update.
          </p>
        </div>
      `;
    }
  }

  /**
   * Always returns false - placeholder has no changes
   */
  isDirty() {
    return false;
  }

  /**
   * Nothing to save
   */
  async save() {
    return { success: true, message: 'No settings to save' };
  }
}

export default PlaceholderPage;
