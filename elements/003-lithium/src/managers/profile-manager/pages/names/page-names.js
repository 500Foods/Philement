/**
 * Profile Manager - Names Page Handler
 *
 * Handles the Names settings page (index: -2)
 */

import { SimpleSettingsPage } from '../settings-page-base.js';
import { log, Subsystems, Status } from '../../../../core/log.js';

/**
 * Names Page Handler
 */
export class NamesPage extends SimpleSettingsPage {
  constructor(options = {}) {
    super({
      ...options,
      index: -2,
      formSelector: 'form',
    });
  }

  /**
   * Called when the page is initialized
   */
  async onInit() {
    await super.onInit();
    log(Subsystems.MANAGER, Status.DEBUG, '[NamesPage] Initialized (stub)');
  }

  /**
   * Load data - to be implemented when API is available
   */
  async loadData() {
    // TODO: Load user profile data from API
    log(Subsystems.MANAGER, Status.DEBUG, '[NamesPage] loadData (stub)');
  }

  /**
   * Save data - to be implemented when API is available
   */
  async save() {
    // TODO: Save to API
    log(Subsystems.MANAGER, Status.DEBUG, '[NamesPage] save (stub)');
    this.setDirty(false);
    return { success: true };
  }
}

export default NamesPage;
