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

    // Find or create the placeholder content
    let placeholderEl = this.container.querySelector('.settings-placeholder');

    if (!placeholderEl) {
      // Clear existing content and create placeholder
      const section = this.container.querySelector('.profile-section');
      if (section) {
        section.innerHTML = `
          <h3 class="section-title"><fa fa-cog></fa> ${this._managerName} Settings</h3>
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
